/*
Copyright 2001-2022 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/	

//
//	Interface to allow G8BPQ switch to use ARDOP Virtual TNC


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifdef WIN32
//#include <Psapi.h>
#else

// For serial over i2c support

#ifdef MACBPQ
#define NOI2C
#endif

#ifdef FREEBSD
#define NOI2C
#endif

#ifndef NOI2C
#include "i2c-dev.h"
#endif
#endif

#include "CHeaders.h"


int (WINAPI FAR *GetModuleFileNameExPtr)();
int (WINAPI FAR *EnumProcessesPtr)();

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#define APMaxStreams 10			// First is used for ARDOP, even though ARDOP uses channel 31

#include "bpq32.h"

#include "tncinfo.h"


#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

static int Socket_Data(int sock, int error, int eventcode);

int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);
int KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
int SendReporttoWL2K(struct TNCINFO * TNC);
char * CheckAppl(struct TNCINFO * TNC, char * Appl);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
BOOL KillOldTNC(char * Path);
int ARDOPSendData(struct TNCINFO * TNC, char * Buff, int Len);
VOID ARDOPSendCommand(struct TNCINFO * TNC, char * Buff, BOOL Queue);
VOID ARDOPSendPktCommand(struct TNCINFO * TNC, int Stream, char * Buff);
VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen);
VOID ARDOPProcessDataPacket(struct TNCINFO * TNC, UCHAR * Type, UCHAR * Data, int Length);
void ARDOPSCSCheckRX(struct TNCINFO * TNC);
VOID ARDOPSCSPoll(struct TNCINFO * TNC);
VOID ARDOPDoTNCReinit(struct TNCINFO * TNC);
int SerialGetTCPMessage(struct TNCINFO * TNC, unsigned char * Buffer, int Len);
int SerialConnecttoTCP(struct TNCINFO * TNC);
int ARDOPWriteCommBlock(struct TNCINFO * TNC);
int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);
int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len);
BOOL WriteCommBlock(struct TNCINFO * TNC);
VOID AddVirtualKISSPort(struct TNCINFO * TNC, int Port, char * buf);
int ConfigVirtualKISSPort(struct TNCINFO * TNC, char * Cmd);
void ProcessKISSBytes(struct TNCINFO * TNC, UCHAR * Data, int Len);
void ProcessKISSPacket(struct TNCINFO * TNC, UCHAR * KISSBuffer, int Len);
int ARDOPProcessDEDFrame(struct TNCINFO * TNC, UCHAR * Msg, int framelen);
int ConnecttoARDOP(struct TNCINFO * TNC);
int standardParams(struct TNCINFO * TNC, char * buf);

#ifndef LINBPQ
BOOL CALLBACK EnumARDOPWindowsProc(HWND hwnd, LPARAM  lParam);
#endif

static char ClassName[]="ARDOPSTATUS";
static char WindowTitle[] = "ARDOP";
static int RigControlRow = 165;

#define WINMOR
#define NARROWMODE 21
#define WIDEMODE 22

#ifndef LINBPQ
#include <commctrl.h>
#endif

extern char * PortConfig[33];
extern int SemHeldByAPI;

static RECT Rect;

extern struct TNCINFO * TNCInfo[41];		// Records are Malloc'd

static int ProcessLine(char * buf, int Port);


BOOL ARDOPStopPort(struct PORTCONTROL * PORT)
{
	// Disable Port - close TCP Sockets or Serial Port

	struct TNCINFO * TNC = PORT->TNC;

	TNC->CONNECTED = FALSE;
	TNC->Alerted = FALSE;

	if (TNC->PTTMode)
		Rig_PTT(TNC, FALSE);			// Make sure PTT is down

	if (TNC->Streams[0].Attached)
		TNC->Streams[0].ReportDISC = TRUE;

	if (TNC->TCPSock)
	{
		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->TCPSock);
	}

	if (TNC->TCPDataSock)
	{
		shutdown(TNC->TCPDataSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->TCPDataSock);
	}

	TNC->TCPSock = 0;
	TNC->TCPDataSock = 0;

	if (TNC->hDevice)
	{
		CloseCOMPort(TNC->hDevice);
		TNC->hDevice = 0;
	}

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Stopped");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}

BOOL ARDOPStartPort(struct PORTCONTROL * PORT)
{
	// Restart Port - Open Sockets or Serial Port

	struct TNCINFO * TNC = PORT->TNC;

	if (TNC->ARDOPCommsMode == 'T')
	{
		ConnecttoARDOP(TNC);
		TNC->lasttime = time(NULL);;
	}

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Restarted");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}


int GenCRC16(unsigned char * Data, unsigned short length)
{
	// For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length); j++)	
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}
 
	return intRegister;
}

BOOL checkcrc16(unsigned char * Data, unsigned short length)
{
	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length - 2); j++)		// ' 2 bytes short of data length
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}

    if (Data[length - 2] == intRegister >> 8)
		if (Data[length - 1] == (intRegister & 0xFF))
			return TRUE;
   
	return FALSE;
}


// Logging Interface. Used for Log over Serial Mode

BOOL ARDOPOpenLogFiles(struct TNCINFO * TNC)
{
	UCHAR FN[MAX_PATH];

	time_t T;
	struct tm * tm;

	T = time(NULL);
	tm = gmtime(&T);

	strlop(TNC->LogPath, 13);
	strlop(TNC->LogPath, 32);

	sprintf(FN,"%s/ARDOPDebugLog_%02d%02d_%d.txt", TNC->LogPath, tm->tm_mon + 1, tm->tm_mday, TNC->Port);
	TNC->DebugHandle = fopen(FN, "ab");
	sprintf(FN,"%s/ARDOPLog_%02d%02d_%d.txt", TNC->LogPath, tm->tm_mon + 1, tm->tm_mday, TNC->Port);
	TNC->LogHandle = fopen(FN, "ab");
	
	return (TNC->LogHandle != NULL);
}

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

void SendARDOPorPacketData(struct TNCINFO * TNC, int Stream, UCHAR * Buff, int txlen)
{
	struct STREAMINFO * STREAM = &TNC->Streams[Stream];

	if (Stream == 0)
	{
		ARDOPSendData(TNC, Buff, txlen);
		STREAM->BytesTXed += txlen;
		WritetoTrace(TNC, Buff, txlen);
	}
	else
	{
		// Packet. Only works over Serial

		PMSGWITHLEN buffptr;
		UCHAR * buffp;

		if (TNC->ARDOPCommsMode == 'T')
			return;

		buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = txlen + 1;
	
		buffp = &buffptr->Data[0];
		buffp[0] = 0;					// CMD/Data Flag on front

		memcpy(buffp +1, Buff, txlen);
				
		C_Q_ADD(&STREAM->BPQtoPACTOR_Q, buffptr);
		STREAM->FramesQueued++;

		if (TNC->Timeout == 0)		// if link idle can send now
			ARDOPSCSPoll(TNC);
	}
}


static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	char * PktPort = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	// Must start ADDR or SERIAL

	ptr = strtok(NULL, " \t\n\r");
	BPQport = Port;
	p_ipad = ptr;

	if (_stricmp(buf, "ADDR") == 0 || _stricmp(buf, "TCPSERIAL") == 0)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));
	
		if (p_ipad == NULL)
			p_ipad = strtok(NULL, " \t\n\r");

		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		PktPort = strlop(p_port, '/');

		if (PktPort)
			TNC->PacketPort = atoi(PktPort);

		WINMORport = atoi(p_port);

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(WINMORport);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(WINMORport+1);

		TNC->HostName = malloc(strlen(p_ipad)+1);

		if (TNC->HostName == NULL) return TRUE;

		strcpy(TNC->HostName,p_ipad);

		if (buf[0] == 'A')
			TNC->ARDOPCommsMode = 'T';		// TCP
		else
			TNC->ARDOPCommsMode = 'E';		// TCPSERIAL (ESP01)
	}
	else if (_stricmp(buf, "SERIAL") == 0)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));
	
		if (p_ipad == NULL)
			p_ipad = strtok(NULL, " \t\n\r");

		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		TNC->ARDOPSerialPort = _strdup(p_ipad);
		TNC->ARDOPSerialSpeed = atoi(p_port);

		TNC->ARDOPCommsMode = 'S';
	}
	else if (_stricmp(buf, "I2C") == 0)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));
	
		if (p_ipad == NULL)
			p_ipad = strtok(NULL, " \t\n\r");

		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		TNC->ARDOPSerialPort = _strdup(p_ipad);
		TNC->ARDOPSerialSpeed = atoi(p_port);

		TNC->ARDOPCommsMode = 'I';
	}
	else
		return FALSE;						// Must start with ADDR

		TNC->InitScript = malloc(1000);
		TNC->InitScript[0] = 0;

		ptr = strtok(NULL, " \t\n\r");

		if (ptr)
		{
			if (_stricmp(ptr, "PTT") == 0)
			{
				ptr = strtok(NULL, " \t\n\r");

				if (ptr)
				{
					DecodePTTString(TNC, ptr);
					ptr = strtok(NULL, " \t\n\r");
				}
			}
		}
		
		if (ptr)
		{
			if (_memicmp(ptr, "PATH", 4) == 0)
			{
				p_cmd = strtok(NULL, "\n\r");
				if (p_cmd) TNC->ProgramPath = _strdup(p_cmd);
			}
		}

		TNC->MaxConReq = 10;		// Default
		TNC->OldMode = FALSE;		// Default 

		// Read Initialisation lines

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			strcpy(errbuf, buf);

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}
				
			if ((_memicmp(buf, "CAPTURE", 7) == 0) || (_memicmp(buf, "PLAYBACK", 8) == 0))
			{}		// Ignore
			else
/*
			if (_memicmp(buf, "PATH", 4) == 0)
			{
				char * Context;
				p_cmd = strtok_s(&buf[5], "\n\r", &Context);
				if (p_cmd) TNC->ProgramPath = _strdup(p_cmd);
			}
			else
*/

			if (_memicmp(buf, "PACKETCHANNELS", 14) == 0)	// Packet Channels
				TNC->PacketChannels = atoi(&buf[14]);
			else
			if (_memicmp(buf, "MAXCONREQ", 9) == 0)		// Hold Time for Busy Detect
				TNC->MaxConReq = atoi(&buf[9]);

			else
			if (_memicmp(buf, "STARTINROBUST", 13) == 0)
				TNC->StartInRobust = TRUE;
			
			else
			if (_memicmp(buf, "ROBUST", 6) == 0)
			{
				if (_memicmp(&buf[7], "TRUE", 4) == 0)
					TNC->Robust = TRUE;
				
				strcat (TNC->InitScript, buf);
			}
			else
			if (_memicmp(buf, "LOGDIR ", 7) == 0)
				TNC->LogPath = _strdup(&buf[7]);
			else
			if (_memicmp(buf, "ENABLEPACKET", 12) == 0)
			{
				if (TNC->PacketChannels == 0)
					TNC->PacketChannels = 5;
			//	AddVirtualKISSPort(TNC, Port, buf);
			}

//			else if (_memicmp(buf, "PAC ", 4) == 0 && _memicmp(buf, "PAC MODE", 8) != 0)
//			{
				// PAC MODE goes to TNC, others are parsed locally
//
//				ConfigVirtualKISSPort(TNC, buf);
//			}
			else if (standardParams(TNC, buf) == FALSE)
				strcat(TNC->InitScript, buf);
		}


	return (TRUE);	
}


void ARDOPThread(struct TNCINFO * TNC);
VOID ARDOPProcessDataSocketData(int port);
int ConnecttoARDOP();
static VOID ARDOPProcessReceivedData(struct TNCINFO * TNC);
static VOID ARDOPProcessReceivedControl(struct TNCINFO * TNC);
int V4ProcessReceivedData(struct TNCINFO * TNC);
VOID ARDOPReleaseTNC(struct TNCINFO * TNC);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);



#define MAXBPQPORTS 32

static time_t ltime;


static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);

unsigned short int compute_crc(unsigned char *buf,int len);

VOID ARDOPSendPktCommand(struct TNCINFO * TNC, int Stream, char * Buff)
{
	// Encode and send to TNC. May be TCP or Serial

	int EncLen;
	UCHAR Encoded[256];

	if (Stream == 0)
	{
		if (Buff[0] == 0)		// Terminal Keepalive?
			return;
	}
	else
	{
		if (Buff[1] == 0)		// Terminal Keepalive?
			return;
	}

	if (TNC->PacketSock)		// Packet Data over separate TCP Connectoion?
	{
		// Chan, Cmd/Data, Len-1

		int SentLen; 
		
		EncLen = sprintf(Encoded, "%c%c%c%s\r", Buff[0], 1, (int)strlen(Buff) -2, &Buff[1]);
		SentLen = send(TNC->PacketSock, Encoded, EncLen, 0);
		
		if (SentLen != EncLen)
		{			
			int winerr=WSAGetLastError();
			char ErrMsg[80];
				
			sprintf(ErrMsg, "ARDOP Pkt Write Failed for port %d - error code = %d\r\n", TNC->PacketPort, winerr);
			WritetoConsole(ErrMsg);
							
			closesocket(TNC->PacketSock);
			TNC->PacketSock = 0;
		}
		return;

	}

	EncLen = sprintf(Encoded, "%s\r", Buff);
	SendToTNC(TNC, Stream, Encoded, EncLen);

	return;
}


VOID ARDOPSendCommand(struct TNCINFO * TNC, char * Buff, BOOL Queue)
{
	// Encode and send to TNC. May be TCP or Serial

	// Command Formst is C:TEXT<CR><CRC>

	int i, EncLen;
	UCHAR Encoded[260];		// could get 256 byte packet

	if (Buff[0] == 0)		// Terminal Keepalive?
		return;

	EncLen = sprintf(Encoded, "%s\r", Buff);

	// it is possible for binary data to be dumped into the command
	// handler if a session disconnects in middle of transfer

	for (i = 0; i < EncLen; i++)
	{
		if (Encoded[i] > 0x7f)
			return;
	}

	SendToTNC(TNC, 12, Encoded, EncLen); // Use streams 12 aad 13 for Host Mode Schannels 32 and 33
	return;
}

VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen)
{
	int SentLen;

	if (TNC->hDevice || (TNC->ARDOPCommsMode == 'E' && TNC->TCPSock))
	{
		// Serial mode. Queue to Hostmode driver
		
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;


		if (TNC->Timeout == 0)		// if link idle can send now
			ARDOPSCSPoll(TNC);

		return;
	}

	if(TNC->ARDOPCommsMode == 'T' && TNC->TCPSock)
	{
		SentLen = send(TNC->TCPSock, Encoded, EncLen, 0);
		
		if (SentLen != EncLen)
		{			
			int winerr=WSAGetLastError();
			char ErrMsg[80];
				
			sprintf(ErrMsg, "ARDOP Write Failed for port %d - error code = %d\r\n", TNC->Port, winerr);
			WritetoConsole(ErrMsg);
							
			closesocket(TNC->TCPSock);
			TNC->TCPSock = 0;		
			TNC->CONNECTED = FALSE;
			return;
		}
	}
}

VOID SendDataToTNC(struct TNCINFO * TNC,  int Streem , UCHAR * Encoded, int EncLen)
{
	int SentLen;

	if (TNC->hDevice || (TNC->ARDOPCommsMode == 'E' && TNC->TCPSock))
	{
		// Serial mode. Queue to Hostmode driver
		
		int Stream = 13;			// use 12 and 13 for scs channels 32 and 33
	
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;

		if (TNC->Timeout == 0)		// if link idle can send now
			ARDOPSCSPoll(TNC);

		return;
	}

	if(TNC->TCPDataSock)
	{
		SentLen = send(TNC->TCPDataSock, Encoded, EncLen, 0);
		
		if (SentLen != EncLen)
		{
			// WINMOR doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
			int winerr=WSAGetLastError();
			char ErrMsg[80];
				
			sprintf(ErrMsg, "ARDOP Write Failed for port %d - error code = %d\r\n", TNC->Port, winerr);
			WritetoConsole(ErrMsg);
					
	
//				if (winerr != WSAEWOULDBLOCK)
//				{
		
			closesocket(TNC->TCPSock);
			closesocket(TNC->TCPDataSock);
			TNC->TCPSock = 0;		
			TNC->TCPDataSock = 0;		
					
			TNC->CONNECTED = FALSE;
			return;
		}
	}
}

int ARDOPSenPktdData(struct TNCINFO * TNC, int Stream, char * Buff, int Len)
{
	// Encode and send to TNC. May be TCP or Serial

	int EncLen;

	UCHAR Msg[400];
	UCHAR * Encoded = Msg;
	
	*(Encoded++) = Len >> 8;
	*(Encoded++) = Len & 0xff;

	memcpy(Encoded, Buff, Len);
	EncLen = Len + 2;

	SendDataToTNC(TNC, Stream, Msg, EncLen);
	return Len;
}



int ARDOPSendData(struct TNCINFO * TNC, char * Buff, int Len)
{
	// Encode and send to TNC. May be TCP or Serial

	int EncLen;

	UCHAR Msg[400];
	UCHAR * Encoded = Msg;
	
	*(Encoded++) = Len >> 8;
	*(Encoded++) = Len & 0xff;

	memcpy(Encoded, Buff, Len);

	EncLen = Len + 2;

	SendDataToTNC(TNC, 13, Msg, EncLen);
	return Len;
}



VOID ARDOPChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

//	ARDOPSendCommand(TNC, "CODEC FALSE");

	datalen = sprintf(TXMsg, "MYCALL %s", Call);
	ARDOPSendCommand(TNC, TXMsg, TRUE);

//	ARDOPSendCommand(TNC, "CODEC TRUE");
//	TNC->StartSent = TRUE;

//	ARDOPSendCommand(TNC, "MYCALL", TRUE);
}

static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	int datalen;
	PMSGWITHLEN buffptr;
//	char txbuff[500];
	unsigned int bytes,txlen=0;
	size_t Param;
	int Stream = 0;
	HKEY hKey=0;

	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct ScanEntry * Scan;

	if (TNC == NULL)
		return 0;							// Port not defined

	if (TNC->CONNECTED == 0)
	{
		// clear Q if not connected

		while(TNC->BPQtoWINMOR_Q)
		{
			buffptr = (PMSGWITHLEN)Q_REM(&TNC->BPQtoWINMOR_Q);

			if (buffptr)
				ReleaseBuffer(buffptr);
		}
	}


	switch (fn)
	{
		case 7:			

		// approx 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		// Check session limit timer

		if ((STREAM->Connecting || STREAM->Connected) && !STREAM->Disconnecting)
		{
			if (TNC->SessionTimeLimit && STREAM->ConnectTime && time(NULL) > (TNC->SessionTimeLimit + STREAM->ConnectTime))
			{
				ARDOPSendCommand(TNC, "DISCONNECT", TRUE);
				STREAM->Disconnecting = TRUE;
			}
		}

		if (TNC->ARDOPCommsMode != 'T') // S I or E
		{
			ARDOPSCSCheckRX(TNC);
			ARDOPSCSPoll(TNC);
		}

		return 0;

	case 1:				// poll

		// If not using serial interface, Rig Contol Frames are sent as 
		// ARDOP COmmand Frames. These are hex encoded

		if (TNC->ARDOPCommsMode == 'T' && TNC->BPQtoRadio_Q)
		{
			PMSGWITHLEN buffptr;
			
			buffptr = (PMSGWITHLEN)Q_REM(&TNC->BPQtoRadio_Q);
		
			if (TNC->CONNECTED)
			{
				int len = (int)buffptr->Len;
				UCHAR * ptr = &buffptr->Data[0];
				char RigCommand[256] = "RADIOHEX ";
				char * ptr2 = &RigCommand[9] ;
				int i, j;

				if (len < 120)
				{
					while (len--)
					{
						i = *(ptr++);
						j = i >>4;
						j += '0';		// ascii
						if (j > '9')
							j += 7;
						*(ptr2++) = j;

						j = i & 0xf;
						j += '0';		// ascii
						if (j > '9')
							j += 7;
						*(ptr2++) = j;
					}
					ARDOPSendCommand(TNC, RigCommand, FALSE);
				}
			}
			ReleaseBuffer(buffptr);	

		}

		while (TNC->PortRecord->UI_Q)
		{
			int datalen;
			char * Buffer;
			char FECMsg[512];
			char Call[12] = "           ";		
			struct _MESSAGE * buffptr;
			int CallLen;
			char * ptr = FECMsg;

			buffptr = Q_REM(&TNC->PortRecord->UI_Q);

			if (TNC->CONNECTED == 0 ||
				TNC->Streams[0].Connecting ||
				TNC->Streams[0].Connected)
			{
				// discard if TNC not connected or sesison active

				ReleaseBuffer(buffptr);
				continue;
			}
	
			datalen = buffptr->LENGTH - MSGHDDRLEN;
			Buffer = &buffptr->DEST[0];		// Raw Frame
			Buffer[datalen] = 0;

			*ptr++ = '^';		// delimit fram ewith ^

			// Frame has ax.25 format header. Convert to Text

			CallLen = ConvFromAX25(Buffer + 7, Call);		// Origin
			memcpy(ptr, Call, CallLen);
			ptr += CallLen;

			*ptr++ = '>';

			CallLen = ConvFromAX25(Buffer, Call);			// Dest
			memcpy(ptr, Call, CallLen);
			ptr += CallLen;

			Buffer += 14;						// TO Digis
			datalen -= 14;

			while ((Buffer[-1] & 1) == 0)
			{
				*ptr++ = ',';
				CallLen = ConvFromAX25(Buffer,  Call);
				memcpy(ptr, Call, CallLen);
				ptr += CallLen;
				Buffer += 7;	// End of addr
				datalen -= 7;
			}

			*ptr++ = '|';		// delimit calls

			if (Buffer[0] == 3)				// UI
			{
				Buffer += 2;
				datalen -= 2;
			}

			memcpy(ptr, Buffer, datalen);
			ptr += datalen;
			*ptr++ = '^';		// delimit fram ewith ^

			ARDOPSendData(TNC, FECMsg, (int)(ptr - FECMsg));
			TNC->FECPending = 1;
		
			ReleaseBuffer((UINT *)buffptr);
		}

		if (TNC->Busy)							//  Count down to clear
		{
			if ((TNC->BusyFlags & CDBusy) == 0)	// TNC Has reported not busy
			{
				TNC->Busy--;
				if (TNC->Busy == 0)
					MySetWindowText(TNC->xIDC_CHANSTATE, "Clear");
					strcpy(TNC->WEB_CHANSTATE, "Clear");
			}
		}

		if (TNC->BusyDelay)
		{
			// Still Busy?

			if (InterlockedCheckBusy(TNC) == FALSE)
			{
				// No, so send

//				ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);  // !!!! Temp bug workaround !!!!

				ARDOPSendCommand(TNC, TNC->ConnectCmd, TRUE);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &TNC->ConnectCmd[8], (int)strlen(TNC->ConnectCmd)-10);

				sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

				free(TNC->ConnectCmd);
				TNC->BusyDelay = 0;
			}
			else
			{
				// Wait Longer

				TNC->BusyDelay--;

				if (TNC->BusyDelay == 0)
				{
					// Timed out - Send Error Response

					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

					if (buffptr == 0) return (0);			// No buffers, so ignore

					buffptr->Len = 39;
					memcpy(&buffptr->Data[0], "Sorry, Can't Connect - Channel is busy\r", 39);

					C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
					free(TNC->ConnectCmd);

					sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

				}
			}
		}

		if (TNC->HeartBeat++ > 600 || (TNC->Streams[0].Connected && TNC->HeartBeat > 50))			// Every Minute unless connected
		{
			TNC->HeartBeat = 0;

			if (TNC->CONNECTED)
			{
				// Probe link

				if (TNC->Streams[0].Connecting || TNC->Streams[0].Connected)
					fn =fn; //ARDOPSendCommand(TNC, "MODE", TRUE);
				else
				{
//					if (time(NULL) - TNC->WinmorRestartCodecTimer > 300)	// 5 mins
//					{
//						ARDOPSendCommand(TNC, "CODEC FALSE", TRUE);
//						ARDOPSendCommand(TNC, "CODEC TRUE", TRUE);
//					}
//					else
						ARDOPSendCommand(TNC, "STATE", TRUE);
				}
			}
		}

		if (TNC->FECMode)
		{
			if (TNC->FECIDTimer++ > 6000)		// ID every 10 Mins
			{
				if (!TNC->Busy)
				{
					TNC->FECIDTimer = 0;
					ARDOPSendCommand(TNC, "SENDID", TRUE);
				}
			}
		}

		//	FECPending can be set if not in FEC Mode (eg beacon)
		
		if (TNC->FECPending)	// Check if FEC Send needed
		{
			if (TNC->Streams[0].BytesOutstanding)  //Wait for data to be queued (async data session)
			{
				if (TNC->Busy == 0 && TNC->GavePermission == 0)
				{
					TNC->FECPending = 0;
					ARDOPSendCommand(TNC,"FECSEND TRUE", TRUE);
				}
			}
		}

		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC

				if (TNC->PID)
					KillTNC(TNC);

				RestartTNC(TNC);
			}
		}

		if (TNC->TimeSinceLast++ > 800)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath && TNC->CONNECTED && 0)
			{
				struct tm * tm;
				char Time[80];
				
				TNC->Restarts++;
				TNC->LastRestart = time(NULL);

				tm = gmtime(&TNC->LastRestart);	
				
				sprintf_s(Time, sizeof(Time),"%04d/%02d/%02d %02d:%02dZ",
					tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

				MySetWindowText(TNC->xIDC_RESTARTTIME, Time);
				strcpy(TNC->WEB_RESTARTTIME, Time);

				sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
				MySetWindowText(TNC->xIDC_RESTARTS, Time);
				strcpy(TNC->WEB_RESTARTS, Time);
	
				KillTNC(TNC);
				RestartTNC(TNC);

				TNC->TimeSinceLast = 0;
			}
		}

		for (Stream = 0; Stream <= APMaxStreams; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			if (STREAM->NeedDisc)
			{
				STREAM->NeedDisc--;

				if (STREAM->NeedDisc == 0)
				{
					// Send the DISCONNECT

					if (Stream == 0)
						ARDOPSendCommand(TNC, "DISCONNECT", TRUE);
					else
					{
						char Cmd[32];
						sprintf(Cmd, "%cDISCONNECT", Stream);			
						ARDOPSendPktCommand(TNC, Stream, Cmd);
					}
				}
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				Debugprintf("ARDOP New Attach Stream %d DEDStream %d", Stream, STREAM->DEDStream);
			
				STREAM->Attached = TRUE;
			
				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;
			
				if (Stream == 0)
				{
						// If Pactor, stop scanning and take out of listen mode.
	//		if (Stream == 0)
	//			STREAM->DEDStream = 31;	// Pactor

					// Stop Listening, and set MYCALL to user's call

					ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
					ARDOPChangeMYC(TNC, TNC->Streams[0].MyCall);
		
					TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

					// Stop other ports in same group

					SuspendOtherPorts(TNC);
	
					sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

					// Stop Scanning

					sprintf(Msg, "%d SCANSTOP", TNC->Port);
	
					Rig_Command(-1, Msg);
				}
				else
				{
					// Packet Connect

				}
			}
		
				
			if (STREAM->Attached)
				CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

		}
		
		if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
		{
			//	See if time to reconnect
		
			time(&ltime);
			if (ltime - TNC->lasttime > 9 )
			{
				if (TNC->ARDOPCommsMode == 'T' && TNC->PortRecord->PORTCONTROL.PortStopped == 0)
					ConnecttoARDOP(TNC);
				TNC->lasttime = ltime;
			}
		}
		
		// See if any frames for this port

		for (Stream = 0; Stream <= APMaxStreams; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			if (TNC->ARDOPCommsMode == 'T')
			{
				// For serial mode packets are taken from the queue by ARDOPSCSPoll
			
				if (STREAM->BPQtoPACTOR_Q)
				{
					PMSGWITHLEN buffptr = (PMSGWITHLEN)Q_REM(&STREAM->BPQtoPACTOR_Q);
					UCHAR * data = &buffptr->Data[0];
					STREAM->FramesQueued--;
					txlen = (int)buffptr->Len;
					STREAM->BytesTXed += txlen;

					if (Stream == 0)
					{
						bytes=ARDOPSendData(TNC, data, txlen);
						WritetoTrace(TNC, data, txlen);
					}
					else
					{
						if (TNC->PacketSock)
						{
							// Using Packet over TCP)
							// Chan, Cmd/Data, Len-1

							UCHAR Encoded[280];
							int EncLen;
							int SentLen; 
		
							EncLen = sprintf(Encoded, "%c%c%c%s\r", Stream, 0, txlen - 1, data);
							SentLen = send(TNC->PacketSock, Encoded, EncLen, 0);
		
							if (SentLen != EncLen)
							{			
								int winerr=WSAGetLastError();
								char ErrMsg[80];
				
								sprintf(ErrMsg, "ARDOP Pkt Write Failed for port %d - error code = %d\r\n", TNC->PacketPort, winerr);
								WritetoConsole(ErrMsg);
							
								closesocket(TNC->PacketSock);
								TNC->PacketSock = 0;
							}
						}
					}
					ReleaseBuffer(buffptr);
				}
			}

			if (STREAM->PACTORtoBPQ_Q != 0)
			{
				buffptr = Q_REM(&STREAM->PACTORtoBPQ_Q);

				datalen = (int)buffptr->Len;

				buff->PORT = Stream;						// Compatibility with Kam Driver
				buff->PID = 0xf0;
				memcpy(&buff->L2DATA, &buffptr->Data[0], datalen);		// Data goes to + 7, but we have an extra byte
				datalen += sizeof(void *) + 4;

				PutLengthinBuffer(buff, datalen);
		
				ReleaseBuffer(buffptr);

				return (1);
			}

			if (STREAM->ReportDISC)		// May need a delay so treat as a counter
			{
				STREAM->ReportDISC--;
				if (STREAM->ReportDISC == 0)
				{
					buff->PORT = Stream;
//					STREAM->Connected = 0;
//					STREAM->Attached = 0;
					return -1;
				}
			}
		}
		return (0);

	case 2:				// send

		Stream = buff->PORT;

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr->Len = sprintf(&buffptr->Data[0], "No Connection to ARDOP TNC\r");

			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		STREAM = &TNC->Streams[Stream];
		
		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}

		txlen = GetLengthfromBuffer(buff) - (sizeof(void *) + 4);
		
		if (STREAM->Connected)
		{
			STREAM->PacketsSent++;

			if (Stream == 0)
			{
				bytes=ARDOPSendData(TNC, &buff->L2DATA[0], txlen);
				TNC->Streams[Stream].BytesOutstanding += bytes;		// So flow control works - will be updated by BUFFER response
				STREAM->BytesTXed += bytes;
				WritetoTrace(TNC, &buff->L2DATA[0], txlen);
			}
			else
			{
				// Packet. Only works over Serial

				PMSGWITHLEN buffptr;
				UCHAR * buffp;

				if (TNC->PacketSock)
				{
					// Using Packet over TCP)
					// Chan, Cmd/Data, Len-1

					UCHAR Encoded[280];
					int EncLen;
					int SentLen; 

					Encoded[0] = Stream;
					Encoded[1] = 0;			// Data
					Encoded[2] = txlen - 1;

					memcpy(&Encoded[3], &buff->L2DATA[0], txlen);
						
					EncLen = txlen + 3;
					SentLen = send(TNC->PacketSock, Encoded, EncLen, 0);

					// We should increment outstanding here as TCP interface can fill buffer
					// very quickly

					TNC->Streams[Stream].BytesOutstanding += txlen;
		
					if (SentLen != EncLen)
					{			
						int winerr=WSAGetLastError();
						char ErrMsg[80];
				
						sprintf(ErrMsg, "ARDOP Pkt Write Failed for port %d - error code = %d\r\n", TNC->PacketPort, winerr);
						WritetoConsole(ErrMsg);
							
						closesocket(TNC->PacketSock);
						TNC->PacketSock = 0;
					}
					return 0;
				}

				if (TNC->ARDOPCommsMode == 'T')
					return 0;

				buffptr = (PMSGWITHLEN)GetBuff();

				if (buffptr == 0) return 0;			// No buffers, so ignore

				buffptr->Len = txlen + 1;
				buffp = &buffptr->Data[0];
			
				buffp[0] = 0;					// CMD/Data Flag on front

				memcpy(buffp + 1, &buff->L2DATA[0], txlen);
			
				C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
				STREAM->FramesQueued++;

				if (TNC->Timeout == 0)		// if link idle can send now
					ARDOPSCSPoll(TNC);

				return 0;
			}
		}
		else
		{
			if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0 || _memicmp(&buff->L2DATA[0], "BYE\r", 4) == 0)
			{
				STREAM->ReportDISC = TRUE;		// Tell Node
				return 0;
			}
	
			if (TNC->FECMode)
			{
				char Buffer[300];
				int len;

				// Send FEC Data

				buff->L2DATA[txlen] = 0;
				len = sprintf(Buffer, "%-9s: %s", TNC->Streams[0].MyCall, &buff->L2DATA[0]);

				ARDOPSendData(TNC, Buffer, len);
				TNC->FECPending = 1;

				return 0;
			}


			// See if Local command (eg RADIO)

			if (_memicmp(&buff->L2DATA[0], "RADIO ", 6) == 0)
			{
				sprintf(&buff->L2DATA[0], "%d %s", TNC->Port, &buff->L2DATA[6]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &buff->L2DATA[0]))
				{
				}
				else
				{
					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr->Len  = sprintf((UCHAR *)&buffptr->Data[0], "%s", &buff->L2DATA[0]);
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}
				return 1;
			}
			
//			if (_memicmp(&buff[8], "PAC ", 4) == 0 && _memicmp(&buff[8], "PAC MODE", 8) != 0)
//			{
				// PAC MODE goes to TNC, others are parsed locally

//				buff[7 + txlen] = 0;
//				ConfigVirtualKISSPort(TNC, &buff[8]);
//				return 1;
//			}

			if (_memicmp(&buff->L2DATA[0], "OVERRIDEBUSY", 12) == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				TNC->OverrideBusy = TRUE;

				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} OK\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}

				return 0;

			}


			if (_memicmp(&buff->L2DATA[0], "MAXCONREQ", 9) == 0)
			{
				if (buff->L2DATA[9] != 13)
				{
					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
				
					// Limit connects

					int tries = atoi(&buff->L2DATA[10]);
					if (tries > 10) tries = 10;

					TNC->MaxConReq = tries;

					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} OK\r");
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					}
					return 0;
				}
			}

			if (_memicmp(&buff->L2DATA[0], "SessionTimeLimit", 16) == 0)
			{
				if (buff->L2DATA[16] != 13)
				{					
					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
	
					TNC->SessionTimeLimit = atoi(&buff->L2DATA[16]) * 60;

					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} OK\r");
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					}
					return 0;
				}
			}

			if (_memicmp(&buff->L2DATA[0], "ARQBW ", 6) == 0)
				TNC->WinmorCurrentMode = 0;			// So scanner will set next value

			if (_memicmp(&buff->L2DATA[0], "CODEC TRUE", 9) == 0)
				TNC->StartSent = TRUE;

			if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0)
			{
				STREAM->ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			if (_memicmp(&buff->L2DATA[0], "FEC\r", 4) == 0 || _memicmp(&buff->L2DATA[0], "FEC ", 4) == 0)
			{
				TNC->FECMode = TRUE;
				TNC->FECIDTimer = 0;
//				ARDOPSendCommand(TNC,"FECRCV TRUE");
		
				return 0;
			}

			if (_memicmp(&buff->L2DATA[0], "PING ", 5) == 0)
			{
				if (InterlockedCheckBusy(TNC))
				{
					// Channel Busy. Unless override set, reject

					if (TNC->OverrideBusy == 0)
					{
						// Reject
				
						PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
						
						if (buffptr)
						{
							buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} Ping blocked by Busy\r");
							C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
						}
						return 0;
					}
				}
				TNC->OverrideBusy = FALSE;
			}

			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80];
				char * ptr = strchr(&buff->L2DATA[2], 13);

				if (ptr)
					*ptr = 0;

				_strupr(&buff->L2DATA[2]);

				if (strlen(&buff->L2DATA[2]) > 9)
					buff->L2DATA[11] = 0;

				if (Stream == 0)
				{
					sprintf(Connect, "ARQCALL %s %d", &buff->L2DATA[2], TNC->MaxConReq);

					ARDOPChangeMYC(TNC, TNC->Streams[0].MyCall);

					// See if Busy

					if (InterlockedCheckBusy(TNC))
					{
						// Channel Busy. Unless override set, wait

						if (TNC->OverrideBusy == 0)
						{
							// Save Command, and wait up to 10 secs
						
							sprintf(TNC->WEB_TNCSTATE, "Waiting for clear channel");
							MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

							TNC->ConnectCmd = _strdup(Connect);
							TNC->BusyDelay = TNC->BusyWait * 10;		// BusyWait secs
							return 0;
						}
					}

					TNC->OverrideBusy = FALSE;

//					ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);  // !!!! Temp bug workaround !!!!

					memset(TNC->Streams[0].RemoteCall, 0, 10);
					strcpy(TNC->Streams[0].RemoteCall, &buff->L2DATA[2]);

					sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", STREAM->MyCall, STREAM->RemoteCall);
					ARDOPSendCommand(TNC, Connect, TRUE);
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
				}
				else
				{
					// Packet Connect
				
					sprintf(Connect, "%cPKTCALL %s %s", Stream, &buff->L2DATA[2], STREAM->MyCall);
					ARDOPSendPktCommand(TNC, Stream, Connect);
				}

				STREAM->Connecting = TRUE;
				STREAM->ConnectTime = time(NULL);
				return 0;

			}
			buff->L2DATA[txlen - 1] = 0;			// Remove CR
			ARDOPSendCommand(TNC, &buff->L2DATA[0], TRUE);
		}
		return 0;

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		Stream = (int)(size_t)buff;

		// I think we should check buffer space for all comms modes

		//if (!(TNC->ARDOPCommsMode == 'T'))
		//{
		//	// if serial mode must check buffer space

		{
		int Queued;
		int Outstanding = TNC->Streams[Stream].BytesOutstanding;

		if (Stream == 0)
			Queued = TNC->Streams[13].FramesQueued;		// ARDOP Native Mode Send Queue
		else
			Queued = TNC->Streams[Stream].FramesQueued;

 		if (TNC->Mode == 'O')		// OFDM version has more buffer space
		{
			if (Queued > 4 || Outstanding > 8500)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}
		else if (TNC->Mode == '3')	// ARDOP3 has a bit more buffer space
		{
			if (Queued > 4 || Outstanding > 5000)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}
		else
		{
			if (Queued > 4 || Outstanding > 2000)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}

		}
		if (TNC->Streams[Stream].Attached == 0)
			return TNC->CONNECTED << 8 | 1;

		return (TNC->CONNECTED << 8 | TNC->Streams[Stream].Disconnecting << 15);		// OK
		

	case 4:				// reinit7

		return 0;

	case 5:				// Close

		if (TNC->CONNECTED)
		{
			if (TNC->WeStartedTNC)
			{
				GetSemaphore(&Semaphore, 52);
				ARDOPSendCommand(TNC, "CLOSE", FALSE);
				FreeSemaphore(&Semaphore);
				Sleep(100);
			}
		}

		if (TNC->TCPSock)
		{
			shutdown(TNC->TCPSock, SD_BOTH);
			Sleep(100);
			closesocket(TNC->TCPSock);
		}

		if (TNC->TCPDataSock)
		{
			shutdown(TNC->TCPDataSock, SD_BOTH);
			Sleep(100);
			closesocket(TNC->TCPDataSock);
		}

		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;

		return 0;

	case 6:				// Scan Stop Interface

		Param = (size_t)buff;
	
		if (Param == 2)		// Check  Permission (Shouldn't happen)
		{
			Debugprintf("Scan Check Permission called on ARDOP");
			return 1;		// OK to change
		}

		if (Param == 1)		// Request Permission
		{
			if (TNC->ARDOPCommsMode == 'T')		// TCP Mode
			{
				if (!TNC->CONNECTED)
					return 0;					// No connection so no interlock
			}
			else
			{
				// Serial Modes

				if (!TNC->HostMode)
					return 0;					// No connection so no interlock
			}
			

			if (TNC->ConnectPending == 0 && TNC->PTTState == 0)
			{
				ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
				TNC->GavePermission = TRUE;
				return 0;	// OK to Change
			}

			if (TNC->ConnectPending)
				TNC->ConnectPending--;		// Time out if set too long

			return TRUE;
		}

		if (Param == 3)		// Release  Permission
		{
			if (TNC->GavePermission)
			{
				TNC->GavePermission = FALSE;
				if (TNC->ARDOPCurrentMode[0] != 'S')	// Skip
					ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;

		if (Scan->ARDOPMode[0] == 0)
		{
			// Not specified, so no change from previous

			return 0;
		}

		if (strcmp(Scan->ARDOPMode, TNC->ARDOPCurrentMode) != 0)
		{
			// Mode changed

			char CMD[32];
			
			if (TNC->ARDOPCurrentMode[0] == 'S')	// Skip
				ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);

//			Debugprintf("ARDOPMODE %s", Scan->ARDOPMode);
			
			memcpy(TNC->ARDOPCurrentMode, Scan->ARDOPMode, 6); 
			
			if (Scan->ARDOPMode[0] == 'S') // SKIP - Dont Allow Connects
			{
				if (TNC->ARDOPCurrentMode[0] != 'S')
				{
					ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
					TNC->ARDOPCurrentMode[0] = 'S';
				}

				TNC->WL2KMode = 0;
				return 0;
			}

			if (strchr(Scan->ARDOPMode, 'F'))
				sprintf(CMD, "ARQBW %sORCED", Scan->ARDOPMode);
			else if (strchr(Scan->ARDOPMode, 'M'))
				sprintf(CMD, "ARQBW %sAX", Scan->ARDOPMode);
			else 
				sprintf(CMD, "ARQBW %s", Scan->ARDOPMode);		// ARDOPOFDM doesn't use MAX/FORCED

			ARDOPSendCommand(TNC, CMD, TRUE);

			return 0;
		}
		return 0;
	}
	return 0;
}

VOID ARDOPReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	ARDOPChangeMYC(TNC, TNC->NodeCall);

	ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);

	strcpy(TNC->WEB_TNCSTATE, "Free");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	//	Start Scanner
				
	//	Start Scanner

	if (TNC->DefaultRadioCmd)
	{
		sprintf(TXMsg, "%d %s", TNC->Port, TNC->DefaultRadioCmd);
		Rig_Command(-1, TXMsg);
	}

	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);
	Rig_Command(-1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID ARDOPSuspendPort(struct TNCINFO * TNC)
{
	ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
}

VOID ARDOPReleasePort(struct TNCINFO * TNC)
{
	ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);
}

extern char WebProcTemplate[];
extern char sliderBit[];

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, WebProcTemplate, TNC->Port, TNC->Port, "ARDOP Status", "ARDOP Status");

	if (TNC->TXFreq)
		Len += sprintf(&Buff[Len], sliderBit, TNC->TXOffset, TNC->TXOffset);

	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>TNC State</td><td>%s</td></tr>", TNC->WEB_TNCSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Mode</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "<tr><td>Channel State</td><td>%s &nbsp; %s</td></tr>", TNC->WEB_CHANSTATE, TNC->WEB_LEVELS);
	Len += sprintf(&Buff[Len], "<tr><td>Proto State</td><td>%s</td></tr>", TNC->WEB_PROTOSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Traffic</td><td>%s</td></tr>", TNC->WEB_TRAFFIC);
//	Len += sprintf(&Buff[Len], "<tr><td>TNC Restarts</td><td></td></tr>", TNC->WEB_RESTARTS);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}


VOID * ARDOPExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	APPLCALLS * APPL;
	struct TNCINFO * TNC;
	char Aux[100] = "MYAUX ";
	char Appl[11];
	char * TempScript;
	int Stream;

	//
	//	Will be called once for each WINMOR port 
	//
	//	The Socket to connect to is in IOBASE
	//
	
	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		sprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return ExtProc;
	}

	TNC->Port = port;

	if (TNC->LogPath)
		ARDOPOpenLogFiles(TNC); 

	TNC->ARDOPBuffer = malloc(8192);
	TNC->ARDOPDataBuffer = malloc(16384);
	TNC->ARDOPAPRS = zalloc(512);
	TNC->ARDOPAPRSLen = 0;

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = RestartTNC(TNC);

	TNC->Hardware = H_ARDOP;

	if (TNC->BusyWait == 0)
		TNC->BusyWait = 10;

	if (TNC->BusyHold == 0)
		TNC->BusyHold = 1;

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	if (PortEntry->PORTCONTROL.PORTINTERLOCK && TNC->RXRadio == 0 && TNC->TXRadio == 0)
		TNC->RXRadio = TNC->TXRadio = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	for (Stream = 1; Stream <= APMaxStreams; Stream++)
	{
		TNC->Streams[Stream].DEDStream = Stream;
	}

	if (TNC->PacketChannels > APMaxStreams)
		TNC->PacketChannels = APMaxStreams;

	PortEntry->MAXHOSTMODESESSIONS = TNC->PacketChannels + 1;

	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only
	PortEntry->PERMITGATEWAY = TRUE;				// Can change ax.25 call on each stream

	PortEntry->PORTCONTROL.UICAPABLE = TRUE;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = ARDOPSuspendPort;
	TNC->ReleasePortProc = ARDOPReleasePort;

	PortEntry->PORTCONTROL.PORTSTARTCODE = ARDOPStartPort;
	PortEntry->PORTCONTROL.PORTSTOPCODE = ARDOPStopPort;

	TNC->ModemCentre = 1500;				// ARDOP is always 1500 Offset

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = malloc(1000);

	strcpy(TempScript, "INITIALIZE\r");
	strcat(TempScript, "VERSION\r");
	strcat(TempScript, "CWID False\r");
	strcat(TempScript, "PROTOCOLMODE ARQ\r");
	strcat(TempScript, "ARQTIMEOUT 90\r");
//	strcat(TempScript, "ROBUST False\r");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Set MYCALL

//	strcat(TNC->InitScript,"FECRCV True\r");
//	strcat(TNC->InitScript,"AUTOBREAK True\r");

	sprintf(Msg, "MYCALL %s\r", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);
//	strcat(TNC->InitScript,"PROCESSID\r");
//	strcat(TNC->InitScript,"CODEC TRUE\r");
//	strcat(TNC->InitScript,"LISTEN TRUE\r");

	for (i = 0; i < 32; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLCALL_TEXT[0] > ' ')
		{
			char * ptr;
			memcpy(Appl, APPL->APPLCALL_TEXT, 10);
			ptr=strchr(Appl, ' ');

			if (ptr)
			{
				*ptr++ = ',';
				*ptr = 0;
			}
			strcat(Aux, Appl);
		}
	}

	if (strlen(Aux) > 8)
	{
		Aux[strlen(Aux) - 1] = '\r';
		strcat(TNC->InitScript, Aux);
	}

	strcpy(TNC->CurrentMYC, TNC->NodeCall);

	if (TNC->WL2K == NULL)
		if (PortEntry->PORTCONTROL.WL2KInfo.RMSCall[0])			// Alrerady decoded
			TNC->WL2K = &PortEntry->PORTCONTROL.WL2KInfo;

	if (TNC->destaddr.sin_family == 0)
	{
		// not defined in config file, so use localhost and port from IOBASE

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE+1);

		TNC->HostName=malloc(10);

		if (TNC->HostName != NULL) 
			strcpy(TNC->HostName,"127.0.0.1");

	}

	PortEntry->PORTCONTROL.TNC = TNC;

	TNC->WebWindowProc = WebProc;
	TNC->WebWinX = 520;
	TNC->WebWinY = 500;
	TNC->WebBuffer = zalloc(5000);

	TNC->WEB_COMMSSTATE = zalloc(100);
	TNC->WEB_TNCSTATE = zalloc(100);
	TNC->WEB_CHANSTATE = zalloc(100);
	TNC->WEB_BUFFERS = zalloc(100);
	TNC->WEB_PROTOSTATE = zalloc(100);
	TNC->WEB_RESTARTTIME = zalloc(100);
	TNC->WEB_RESTARTS = zalloc(100);

	TNC->WEB_MODE = zalloc(20);
	TNC->WEB_TRAFFIC = zalloc(100);
	TNC->WEB_LEVELS =zalloc(32);

#ifndef LINBPQ

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450, ForcedClose);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,6,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,6,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,28,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,28,520,20, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,50,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,50,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,72,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,72,82,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_LEVELS = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 200,72,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
 	CreateWindowEx(0, "STATIC", "Proto State", WS_CHILD | WS_VISIBLE,10,94,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,120,94,374,20 , TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,116,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,120,116,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,120,138,40,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTTIME = CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,138,200,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill ARDOP TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart ARDOP TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after failed Connection");
	AppendMenu(TNC->hMenu, MF_STRING, ARDOP_ABORT, "Abort Current Session");
	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	MoveWindows(TNC);
#endif

	if (TNC->ARDOPCommsMode == 'T')
	{
		Consoleprintf("ARDOP Host %s %d", TNC->HostName, htons(TNC->destaddr.sin_port));
		ConnecttoARDOP(TNC);
	}

	else if (TNC->ARDOPCommsMode == 'E')
	{
		Consoleprintf("ARDOP TCPSerial %s:%d", TNC->HostName, htons(TNC->destaddr.sin_port));
		SerialConnecttoTCP(TNC);
	}

	else if (TNC->ARDOPCommsMode == 'S')
	{
		TNC->PortRecord->PORTCONTROL.SerialPortName = _strdup(TNC->ARDOPSerialPort); // for common routines
		Consoleprintf("ARDOP Serial %s", TNC->ARDOPSerialPort);
		OpenCOMMPort(TNC, TNC->ARDOPSerialPort, TNC->ARDOPSerialSpeed, FALSE);
	}
	else if (TNC->ARDOPCommsMode == 'I')
	{
#ifdef WIN32
		sprintf(Msg,"ARDOP I2C is not supported on WIN32 systems\n");
		WritetoConsoleLocal(Msg);
#else
#ifdef NOI2C
		sprintf(Msg,"I2C is not supported on this systems\n");
		WritetoConsoleLocal(Msg);
#else
		char i2cname[30];
		int fd;
		int retval;

		if (strlen(TNC->ARDOPSerialPort) < 3)
		{
			sprintf(i2cname, "/dev/i2c-%s", TNC->ARDOPSerialPort);
			TNC->ARDOPSerialPort = _strdup(i2cname);
		}
		else
			strcpy(i2cname, TNC->ARDOPSerialPort);
	
		TNC->PortRecord->PORTCONTROL.SerialPortName = _strdup(i2cname); // for common routines

		Consoleprintf("ARDOP I2C Bus %s Addr %d ", i2cname, TNC->ARDOPSerialSpeed);

		// Open and configure the i2c interface
		                         
		fd = TNC->hDevice = open(i2cname, O_RDWR);
		
		if (fd < 0)
			printf("Cannot find i2c bus %s \n", i2cname);
		else
		{
			retval = ioctl(TNC->hDevice,  I2C_SLAVE, TNC->ARDOPSerialSpeed);
		
			if(retval == -1)
				printf("Cannot open i2c device %x\n", TNC->ARDOPSerialSpeed);
 
 			ioctl(TNC->hDevice,  I2C_TIMEOUT, 10);	//100 mS	
		}
#endif
#endif
	}

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}

VOID TNCLost(struct TNCINFO * TNC)
{
	int Stream = 0;
	struct STREAMINFO * STREAM;

	if (TNC->TCPSock)
		closesocket(TNC->TCPSock);
	if (TNC->TCPDataSock)
		closesocket(TNC->TCPDataSock);
	if (TNC->PacketSock)
		closesocket(TNC->PacketSock);

	TNC->TCPSock = 0;
	TNC->TCPDataSock = 0;
	TNC->PacketSock = 0;
	TNC->CONNECTED = FALSE;	

	for (Stream = 0; Stream <= APMaxStreams; Stream++)
	{
		STREAM = &TNC->Streams[Stream];

		STREAM->BytesOutstanding = 0;

		if (Stream == 0)
		{
			sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->BytesTXed - STREAM->BytesOutstanding, STREAM->BytesRXed, STREAM->BytesOutstanding);
			MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);
		}

		if (STREAM->Attached)
		{
			STREAM->Connected = FALSE;
			STREAM->Connecting = FALSE;
			STREAM->ReportDISC = TRUE;
		}
	}
}


int ConnecttoARDOP(struct TNCINFO * TNC)
{
	_beginthread(ARDOPThread, 0, (void *)TNC);

	return 0;
}

VOID ARDOPThread(struct TNCINFO * TNC)
{
	// Opens sockets and looks for data on control and data sockets.
	
	// Socket may be TCP/IP or Serial

	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	char * ptr1, * ptr2;
	PMSGWITHLEN buffptr;
	char Cmd[64];

	if (TNC->HostName == NULL)
		return;

	TNC->BusyFlags = 0;

	TNC->CONNECTING = TRUE;

	Sleep(3000);		// Allow init to complete 

#ifdef WIN32
	if (strcmp(TNC->HostName, "127.0.0.1") == 0)
	{
		// can only check if running on local host
		
		TNC->PID = GetListeningPortsPID(TNC->destaddr.sin_port);
		if (TNC->PID == 0)
		{
			TNC->CONNECTING = FALSE;
			return;						// Not listening so no point trying to connect
		}
		
		// Get the File Name in case we want to restart it.

		if (TNC->ProgramPath == NULL)
		{
			if (GetModuleFileNameExPtr)
			{
				HANDLE hProc;
				char ExeName[256] = "";

				hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->PID);
	
				if (hProc)
				{
					GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);
					CloseHandle(hProc);

					TNC->ProgramPath = _strdup(ExeName);
				}
			}
		}

	}
#endif


//	// If we started the TNC make sure it is still running.

//	if (!IsProcess(TNC->PID))
//	{
//		RestartTNC(TNC);
//		Sleep(3000);
//	}


	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		HostEnt = gethostbyname (TNC->HostName);
		 
		 if (!HostEnt)
		 {
			 	TNC->CONNECTING = FALSE;
				return;			// Resolve failed
		 }
		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	if (TNC->TCPSock)
	{
		Debugprintf("ARDOP Closing Sock %d", TNC->TCPSock); 
		closesocket(TNC->TCPSock);
	}
	if (TNC->TCPDataSock)
	{
		Debugprintf("ARDOP Closing Sock %d", TNC->TCPDataSock); 
		closesocket(TNC->TCPDataSock);
	}

	if (TNC->PacketSock)
	{
		Debugprintf("ARDOP Closing Sock %d", TNC->PacketSock); 
		closesocket(TNC->PacketSock);
	}
	TNC->TCPSock = 0;
	TNC->TCPDataSock = 0;
	TNC->PacketSock = 0;

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for ARDOP socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}

	TNC->TCPDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for ARDOP Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;

  	 	return; 
	}

	setsockopt(TNC->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(TNC->TCPDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
//	setsockopt(TNC->TCPDataSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (connect(TNC->TCPSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
  			sprintf(Msg, "Connect Failed for ARDOP socket - error code = %d Port %d\n",
				WSAGetLastError(), htons(TNC->destaddr.sin_port));

			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);

		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;
	 	TNC->CONNECTING = FALSE;
		return;
	}

	// Connect Data Port

	if (connect(TNC->TCPDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=sprintf(Msg, "Connect Failed for ARDOP Data socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);
		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;
	 	TNC->CONNECTING = FALSE;
		return;
	}

	if (TNC->PacketPort)
	{
		struct sockaddr_in destaddr;

		TNC->PacketSock = socket(AF_INET,SOCK_STREAM,0);

		if (TNC->PacketSock == INVALID_SOCKET)
		{
			i=sprintf(Msg, "Socket Failed for ARDOP Packet socket - error code = %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);
			TNC->PacketSock = 0;
		}
		else
		{
			setsockopt(TNC->PacketSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
		//	setsockopt(TNC->PacketSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 
		

			destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
			destaddr.sin_family = AF_INET;
			destaddr.sin_port = htons(TNC->PacketPort);
	
			if (connect(TNC->PacketSock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
			{
				//	Connected successful
			}
			else
			{
				if (TNC->Alerted == FALSE)
				{
					err=WSAGetLastError();
   					i=sprintf(Msg, "Connect Failed for ARDOP Packet socket - error code = %d\r\n", err);
					WritetoConsole(Msg);
					TNC->Alerted = TRUE;
				}
				closesocket(TNC->PacketSock);
				TNC->PacketSock = 0;
			}
		}
	}


#ifndef LINBPQ
//	FreeSemaphore(&Semaphore);
	EnumWindows(EnumARDOPWindowsProc, (LPARAM)TNC);
//	GetSemaphore(&Semaphore, 52);
#endif
	Sleep(1000);

	TNC->LastFreq = 0;			//	so V4 display will be updated

 	TNC->CONNECTING = FALSE;
	TNC->CONNECTED = TRUE;
	TNC->BusyFlags = 0;
	TNC->InputLen = 0;

	// Send INIT script

	// ARDOP needs each command in a separate send

	ptr1 = &TNC->InitScript[0];

	// We should wait for first RDY. Cheat by queueing a null command

	GetSemaphore(&Semaphore, 52);

	while(TNC->BPQtoWINMOR_Q)
	{
		buffptr = (PMSGWITHLEN)Q_REM(&TNC->BPQtoWINMOR_Q);

		if (buffptr)
			ReleaseBuffer(buffptr);
	}

	buffptr = (PMSGWITHLEN)GetBuff();
	buffptr->Len = 0;
	C_Q_ADD(&TNC->BPQtoWINMOR_Q, buffptr);

	while (ptr1 && ptr1[0])
	{
		ptr2 = strchr(ptr1, 13);
		if (ptr2)
			*(ptr2) = 0; 
	
		// if Date or Time command add current time

		if (_memicmp(ptr1, "DATETIME", 4) == 0)
		{
			time_t T;
			struct tm * tm;

			T = time(NULL);
			tm = gmtime(&T);	

			sprintf(Cmd, "DATETIME %02d %02d %02d %02d %02d %02d",
				tm->tm_mday, tm->tm_mon + 1, tm->tm_year - 100,
				tm->tm_hour, tm->tm_min, tm->tm_sec);

			ptr1 = Cmd;
		}

		ARDOPSendCommand(TNC, ptr1, TRUE);

		if (ptr2)
			*(ptr2++) = 13;		// Put CR back for next time 

		ptr1 = ptr2;
	}
	
	TNC->Alerted = TRUE;

	sprintf(TNC->WEB_COMMSSTATE, "Connected to ARDOP TNC");		
	MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

	FreeSemaphore(&Semaphore);

	sprintf(Msg, "Connected to ARDOP TNC Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);

	while (TNC->CONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TNC->TCPSock,&readfs);
		FD_SET(TNC->TCPSock,&errorfs);

		if (TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&readfs);
			
//		FD_ZERO(&writefs);

//		if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->TCPDataSock,&writefs);	// Need notification of busy clearing
		
		if (TNC->PacketSock)
		{
			FD_SET(TNC->PacketSock,&errorfs);
			FD_SET(TNC->PacketSock,&readfs);
		}
			
//		FD_ZERO(&writefs);

//		if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->TCPDataSock,&writefs);	// Need notification of busy clearing
		
		if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&errorfs);
		timeout.tv_sec = 600;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		if (TNC->PacketSock)
			ret = select((int)TNC->PacketSock + 1, &readfs, NULL, &errorfs, &timeout);
		else		
			ret = select((int)TNC->TCPDataSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (ret == SOCKET_ERROR)
		{
			Debugprintf("ARDOP Select failed %d ", WSAGetLastError());
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TNC->TCPSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				ARDOPProcessReceivedControl(TNC);
				FreeSemaphore(&Semaphore);
			}
								
			if (FD_ISSET(TNC->TCPDataSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				ARDOPProcessReceivedData(TNC);
				FreeSemaphore(&Semaphore);
			}

			if (FD_ISSET(TNC->PacketSock, &readfs))
			{
				int InputLen, Used;
				UCHAR Buffer[4096];
				
				InputLen = recv(TNC->PacketSock, Buffer, 4096, 0);

				if (InputLen == 0 || InputLen == SOCKET_ERROR)
				{
					sprintf(Msg, "ARDOP Connection lost for Port %d\r\n", TNC->Port);
					WritetoConsole(Msg);

					sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
					MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

					TNC->CONNECTED = FALSE;
					TNC->Alerted = FALSE;

					if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

					TNCLost(TNC);
					return;					
				}

				// Could be more than one frame in buffer

				while (InputLen > 0)
				{
					GetSemaphore(&Semaphore, 52);
					Used = ARDOPProcessDEDFrame(TNC, Buffer, InputLen);
					FreeSemaphore(&Semaphore);
					
					if (Used == 0)
						break;			// need to check 

					InputLen -= Used;

					if (InputLen > 0)
						memmove(Buffer, &Buffer[Used], InputLen);

				}

			}

			if (FD_ISSET(TNC->TCPSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "ARDOP Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->TCPSock);
				closesocket(TNC->TCPDataSock);
				TNC->TCPSock = 0;
				TNC->TCPDataSock = 0;
				return;
			}

			if (FD_ISSET(TNC->TCPDataSock, &errorfs))
			{
				sprintf(Msg, "ARDOP Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->TCPSock);
				closesocket(TNC->TCPDataSock);
				TNC->TCPSock = 0;
				TNC->TCPDataSock = 0;
				return;
			}

			if (FD_ISSET(TNC->PacketSock, &errorfs))
			{
				sprintf(Msg, "ARDOP Packet Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->TCPSock);
				closesocket(TNC->TCPDataSock);
				TNC->TCPSock = 0;
				TNC->TCPDataSock = 0;
				return;
			}

	
			continue;
		}
		else
		{
			// 60 secs without data. Shouldn't happen

			sprintf(Msg, "ARDOP No Data Timeout Port %d\r\n", TNC->Port);
			WritetoConsole(Msg);

//			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
//			GetSemaphore(&Semaphore, 52);
//			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
//			FreeSemaphore(&Semaphore);
	

			TNC->CONNECTED = FALSE;
			TNC->Alerted = FALSE;

			if (TNC->PTTMode)
				Rig_PTT(TNC, FALSE);			// Make sure PTT is down

			if (TNC->Streams[0].Attached)
				TNC->Streams[0].ReportDISC = TRUE;

			GetSemaphore(&Semaphore, 52);
			ARDOPSendCommand(TNC, "CODEC FALSE", FALSE);
			FreeSemaphore(&Semaphore);

			shutdown(TNC->TCPSock, SD_BOTH);
			shutdown(TNC->TCPDataSock, SD_BOTH);
			Sleep(100);
			closesocket(TNC->TCPSock);
			closesocket(TNC->TCPDataSock);

			TNC->TCPSock = 0;
			TNC->TCPDataSock = 0;

			if (TNC->PID && TNC->WeStartedTNC)
			{
//				KillTNC(TNC);
			}
			return;
		}
	}
	sprintf(Msg, "ARDOP Thread Terminated Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);
}

#ifndef LINBPQ

BOOL CALLBACK EnumARDOPWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"ARDOP_Win ", 10) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->PID == ProcessId)
		{
			 // Our Process

			sprintf (wtext, "ARDOP Virtual TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}
#endif

VOID ARDOPProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	Buffer[MsgLen - 1] = 0;		// Remove CR
	
	if (_memicmp(Buffer, "RDY", 3) == 0)
		return;		// RDY not used now

	if (_memicmp(Buffer, "RADIOHEX ", 9) == 0)
	{
		// Parameter is block to send to radio, in hex
		
		char c;
		int val;
 		char * ptr1 = &Buffer[9];
		UCHAR * ptr2 = Buffer;
		PMSGWITHLEN buffptr;
		int Len;

			// if not configured to use PTC Rig Control, Ignore

		if (TNC->RIG->PORT == NULL || TNC->RIG->PORT->PTC == NULL)
			return;
			
		buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == NULL)
			return;

		while (c = *(ptr1++))
		{
			val = c - 0x30;
			if (val > 15) val -= 7;
			val <<= 4;
			c = *(ptr1++) - 0x30;
			if (c > 15) c -= 7;
			val |= c;
			*(ptr2++) = val;
		}
 		
		*(ptr2) = 0; 

		Len = (int)(ptr2 - Buffer);

		buffptr->Len = Len;
		memcpy(&buffptr->Data[0], Buffer, Len);
		C_Q_ADD(&TNC->RadiotoBPQ_Q, buffptr);

//		WriteCOMBlock(hRIGDevice, ptrParams, ptr2 - ptrParams);
		return;
		
	}


	if (_memicmp(Buffer, "INPUTPEAKS", 10) == 0)
	{
		sscanf(&Buffer[10], "%i %i", &TNC->InputLevelMin, &TNC->InputLevelMax);
		sprintf(TNC->WEB_LEVELS, "Input peaks %s", &Buffer[10]);
		MySetWindowText(TNC->xIDC_LEVELS, TNC->WEB_LEVELS);
		return;				// Response shouldn't go to user
	}

	if (_memicmp(Buffer, "LISTEN NOW", 10) == 0)
		return;				// Response shouldn't go to user

	if (_memicmp(Buffer, "ARQCALL ", 7) == 0)
		return;				// Response shouldn't go to user

	if (_memicmp(Buffer, "FAULT failure to Restart Sound card", 20) == 0)
	{
		Debugprintf(Buffer);
	
		// Force a restart

			ARDOPSendCommand(TNC, "CODEC FALSE", TRUE);
			ARDOPSendCommand(TNC, "CODEC TRUE", TRUE);
	}
	else
	{
		TNC->TimeSinceLast = 0;
	}


 	if (_memicmp(Buffer, "STATE ", 6) == 0)
	{	
		if (_memicmp(&Buffer[6], "OFFLINE", 7) == 0)
		{
			// Force a restart

			ARDOPSendCommand(TNC, "CODEC FALSE", TRUE);
			ARDOPSendCommand(TNC, "CODEC TRUE", TRUE);
		}
		return;
	}
	
	if (_memicmp(Buffer, "PTT T", 5) == 0)
	{
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay
		TNC->PTTState = TRUE;

		if (TNC->PTTMode)
			Rig_PTT(TNC, TRUE);

		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		TNC->PTTState = FALSE;
		if (TNC->PTTMode)
			Rig_PTT(TNC, FALSE);

		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		MySetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		strcpy(TNC->WEB_CHANSTATE, "Busy");

		TNC->WinmorRestartCodecTimer = time(NULL);

		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->Busy)
			strcpy(TNC->WEB_CHANSTATE, "BusyHold");
		else
			strcpy(TNC->WEB_CHANSTATE, "Clear");

		MySetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}

	if (_memicmp(Buffer, "TARGET", 6) == 0)
	{
		TNC->ConnectPending = 6;					// This comes before Pending
		Debugprintf(Buffer);
		WritetoTrace(TNC, Buffer, MsgLen - 1);
		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "OFFSET", 6) == 0)
	{
//		WritetoTrace(TNC, Buffer, MsgLen - 5);
		return;
	}

	if (_memicmp(Buffer, "BUFFER", 6) == 0)
	{
		sscanf(&Buffer[7], "%d", &STREAM->BytesOutstanding);

		if (STREAM->BytesOutstanding == 0)
		{
			// all sent
			
			if (STREAM->Disconnecting)			// Disconnect when all sent
			{
				if (STREAM->NeedDisc == 0)
					STREAM->NeedDisc = 1;
			}
		}
		else
		{
			// Make sure Node Keepalive doesn't kill session.

			TRANSPORTENTRY * SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

			if (SESS)
			{
				SESS->L4KILLTIMER = 0;
				SESS = SESS->L4CROSSLINK;
				if (SESS)
					SESS->L4KILLTIMER = 0;
			}
		}

		sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->BytesTXed - STREAM->BytesOutstanding, STREAM->BytesRXed, STREAM->BytesOutstanding);
		MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);
		return;
	}

	if (_memicmp(Buffer, "CONNECTED ", 10) == 0)
	{
		char Call[11];
		char * ptr;
		APPLCALLS * APPL;
		char * ApplPtr = APPLS;
		int App;
		char Appl[10];
		struct WL2KInfo * WL2K = TNC->WL2K;
		int Speed = 0;

		Debugprintf(Buffer);
		WritetoTrace(TNC, Buffer, MsgLen - 1);

		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = STREAM->PacketsSent = 0;

		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		// Get Speed

		ptr = strchr(&Buffer[10], ' ');	
		if (ptr)
		{
			Speed = atoi(ptr);

			if (Speed == 200)
				TNC->WL2KMode = 40;
			else if (Speed == 500)
				TNC->WL2KMode = 41;
			else if (Speed == 1000)
				TNC->WL2KMode = 42;
			else if (Speed == 2000)
				TNC->WL2KMode = 43;
		}
	
		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
		{
			TRANSPORTENTRY * SESS;
			
			// Incoming Connect

			TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			ProcessIncommingConnectEx(TNC, Call, 0, TRUE, TRUE);
				
			SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

			SESS->Mode = TNC->WL2KMode;
			
			TNC->ConnectPending = FALSE;

			if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
				SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
			}
			else
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->TargetCall);
				if (WL2K)
				{
					SESS->Frequency = WL2K->Freq;
				}
			}
			
			if (WL2K)
				strcpy(SESS->RMSCall, WL2K->RMSCall);

			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			
			// Check for ExcludeList

			if (ExcludeList[0])
			{
				if (CheckExcludeList(SESS->L4USER) == FALSE)
				{
					char Status[64];

					TidyClose(TNC, 0);
					sprintf(Status, "%d SCANSTART 15", TNC->Port);
					Rig_Command(-1, Status);
					Debugprintf("ARDOP Call from %s rejected", Call);
					return;
				}
			}

			//	IF WE HAVE A PERMITTED CALLS LIST, SEE IF HE IS IN IT

			if (TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS)
			{
				UCHAR * ptr = TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS;

				while (TRUE)
				{
					if (memcmp(SESS->L4USER, ptr, 6) == 0)	// Ignore SSID
						break;

					ptr += 7;

					if ((*ptr) == 0)							// Not in list
					{
						char Status[64];

						TidyClose(TNC, 0);
						sprintf(Status, "%d SCANSTART 15", TNC->Port);
						Rig_Command(-1, Status);
						Debugprintf("ARDOP Call from %s not in ValidCalls - rejected", Call);
						return;
					}
				}
			}


			// See which application the connect is for

			for (App = 0; App < 32; App++)
			{
				APPL=&APPLCALLTABLE[App];
				memcpy(Appl, APPL->APPLCALL_TEXT, 10);
				ptr=strchr(Appl, ' ');

				if (ptr)
					*ptr = 0;
	
				if (_stricmp(TNC->TargetCall, Appl) == 0)
					break;
			}

			if (App < 32)
			{
				char AppName[13];

				memcpy(AppName, &ApplPtr[App * sizeof(CMDX)], 12);
				AppName[12] = 0;

				if (TNC->SendTandRtoRelay && memcmp(AppName, "RMS ", 4) == 0
					&& (strstr(Call, "-T" ) || strstr(Call, "-R")))
						strcpy(AppName, "RELAY       ");

				// Make sure app is available

				if (CheckAppl(TNC, AppName))
				{
					MsgLen = sprintf(Buffer, "%s\r", AppName);

					buffptr = GetBuff();

					if (buffptr == 0)
					{
						return;			// No buffers, so ignore
					}

					buffptr->Len = MsgLen;
					memcpy(&buffptr->Data[0], Buffer, MsgLen);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
		
					TNC->SwallowSignon = TRUE;

					// Save Appl Call in case needed for 

				}
				else
				{
					char Msg[] = "Application not available\r\n";
					
					// Send a Message, then a disconenct

					// Send CTEXT First

					if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
					{
						PMSGWITHLEN buffptr = (PMSGWITHLEN)Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
						UCHAR * data = &buffptr->Data[0];
						int txlen = (int)(buffptr->Len);
						SendARDOPorPacketData(TNC, 0, data, txlen);
					}
					
					SendARDOPorPacketData(TNC, 0, Msg, (int)strlen(Msg));
					STREAM->NeedDisc = 100;	// 10 secs
				}
			}
		
			return;
		}
		else
		{
			// Connect Complete

			char Reply[80];
			int ReplyLen;
			
			buffptr = GetBuff();

			if (buffptr == 0)
			{
				return;			// No buffers, so ignore
			}
			ReplyLen = sprintf(Reply, "*** Connected to %s\r", Call);

			buffptr->Len = ReplyLen;
			memcpy(&buffptr->Data[0], Reply, ReplyLen);

			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG && TNC->RIG->Valchar[0])
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s",  TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
			else
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			UpdateMH(TNC, Call, '+', 'O');
			return;
		}
	}


	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0
		|| _memicmp(Buffer, "STATUS CONNECT TO", 17) == 0  
		|| _memicmp(Buffer, "STATUS ARQ TIMEOUT FROM PROTOCOL STATE", 24) == 0
//		|| _memicmp(Buffer, "NEWSTATE DISC", 13) == 0
		|| _memicmp(Buffer, "ABORT", 5) == 0)

	{
		TNC->ConnectPending = FALSE;			// Cancel Scan Lock

		if (TNC->FECMode)
			return;

		if (TNC->StartSent)
		{
			TNC->StartSent = FALSE;		// Disconnect reported following start codec
			return;
		}

		if (TNC->Streams[0].Connecting)
		{
			// Report Connect Failed, and drop back to command mode

			TNC->Streams[0].Connecting = FALSE;

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				return;			// No buffers, so ignore
			}

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "*** Failure with %s\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

			if (TNC->RestartAfterFailure)
			{
				if (TNC->PID)
					KillTNC(TNC);
					
				RestartTNC(TNC);
			}

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 1);

		// Release Session3

		if (TNC->Streams[0].Connected)
		{
			// Create a traffic record
		
			char logmsg[120];	
			time_t Duration;

			Duration = time(NULL) - STREAM->ConnectTime;

			if (Duration == 0)
				Duration = 1;
				
			sprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, (int)(STREAM->BytesTXed/Duration),
				STREAM->BytesRXed, (int)(STREAM->BytesRXed/Duration), (int)Duration);

			Debugprintf(logmsg);
		}


		TNC->Streams[0].Connecting = FALSE;
		TNC->Streams[0].Connected = FALSE;		// Back to Command Mode
		TNC->Streams[0].ReportDISC = TRUE;		// Tell Node

		if (TNC->Streams[0].Disconnecting)		// 
			ARDOPReleaseTNC(TNC);

		TNC->Streams[0].Disconnecting = FALSE;

		return;
	}

	if (_memicmp(Buffer, "MODE", 4) == 0)
	{
		strcpy(TNC->WEB_MODE, &Buffer[5]);
		MySetWindowText(TNC->xIDC_MODE, &Buffer[5]);
		return;
	}

	if (_memicmp(Buffer, "STATUS ", 7) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "RADIOMODELS", 11) == 0)
		return;

	if (_memicmp(&Buffer[0], "PENDING", 7) == 0)	// Save Pending state for scan control
	{
		TNC->ConnectPending = 6;				// Time out after 6 Scanintervals
		return;
	}

	// REJECTEDBW and REJECTEDBUSY are sent to both calling and called

	if (_memicmp(&Buffer[0], "REJECTEDBUSY", 12) == 0)
	{
		TNC->ConnectPending = FALSE;

		if (TNC->Streams[0].Connecting)
		{
			// Report Connect Failed, and drop back to command mode

			TNC->Streams[0].Connecting = FALSE;

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				return;			// No buffers, so ignore
			}

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} Connection to %s Rejected - Channel Busy\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			return;
		}
	}

	if (_memicmp(&Buffer[0], "REJECTEDBW", 10) == 0)
	{
		TNC->ConnectPending = FALSE;

		if (TNC->Streams[0].Connecting)
		{
			// Report Connect Failed, and drop back to command mode

			TNC->Streams[0].Connecting = FALSE;

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				return;			// No buffers, so ignore
			}

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} Connection to %s Rejected - Incompatible Bandwidth\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			return;
		}
	}



	if (_memicmp(&Buffer[0], "CANCELPENDING", 13) == 0
		|| _memicmp(&Buffer[0], "REJECTEDB", 9) == 0)  //REJECTEDBUSY or REJECTEDBW
	{
		TNC->ConnectPending = FALSE;
		return;
	}

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		Debugprintf(Buffer);
		WritetoTrace(TNC, Buffer, MsgLen - 1);
//		return;
	}

	if (_memicmp(Buffer, "NEWSTATE", 8) == 0)
	{
		TNC->WinmorRestartCodecTimer = time(NULL);

		MySetWindowText(TNC->xIDC_PROTOSTATE, &Buffer[9]);
		strcpy(TNC->WEB_PROTOSTATE,  &Buffer[9]);
	
		if (_memicmp(&Buffer[9], "DISC", 4) == 0)
		{
			TNC->DiscPending = FALSE;
			TNC->ConnectPending = FALSE;

			return;
		}

		if (strcmp(&Buffer[9], "ISS") == 0)	// Save Pending state for scan control
			TNC->TXRXState = 'S';
		else if (strcmp(&Buffer[9], "IRS") == 0)
			TNC->TXRXState = 'R';
	
		return;
	}

	if ((_memicmp(Buffer, "FAULT Not from state FEC", 24) == 0) || (_memicmp(Buffer, "FAULT Blocked by Busy Lock", 24) == 0))
	{
		if (TNC->FECMode)
		{
			Sleep(1000);
			
//			if (TNC->FEC1600)
//				ARDOPSendCommand(TNC,"FECSEND 1600");
//			else
//				ARDOPSendCommand(TNC,"FECSEND 500");
			return;
		}
	}

	if (_memicmp(Buffer, "PLAYBACKDEVICES", 15) == 0)
	{
		TNC->PlaybackDevices = _strdup(&Buffer[16]);
	}
	// Others should be responses to commands

	if (_memicmp(Buffer, "BLOCKED", 6) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 1);
		return;
	}

	if (_memicmp(Buffer, "OVER", 4) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 1);
		return;
	}

	if (_memicmp(Buffer, "PING ", 5) == 0)
	{
		char Call[32];

		// Make sure not Echoed PING

		// c:ping gm8bpq-1 5
		// c:PING GM8BPQ>GM8BPQ-1 15 98

		if (strchr(Buffer, '>') == 0)	// Echoed
			return;
		
		WritetoTrace(TNC, Buffer, MsgLen - 1);
	
		// Release scanlock after another interval (to allow time for response to be sent)
		// ?? use cancelpending TNC->ConnectPending = 1;


		memcpy(Call, &Buffer[5], 20);
		strlop(Call, '>'); 
		UpdateMH(TNC, Call, '!', 'I');

		return;
	}

	if (_memicmp(Buffer, "VERSION ", 8) == 0)
	{
		// If contains "OFDM" or "ARDOP3" increase data session busy level

		if (strstr(&Buffer[8], "OFDM"))
			TNC->Mode = 'O';
		else if (strstr(&Buffer[8], "TNC_3"))
			TNC->Mode = '3';
		else
			TNC->Mode = 0;
	}

	if (_memicmp(Buffer, "PINGACK ", 8) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 1);
		// Drop through to return touser
	}

	if (_memicmp(Buffer, "CQ ", 3) == 0 && MsgLen > 10)
	{
		char Call[32];
		char * Loc;

		WritetoTrace(TNC, Buffer, MsgLen - 1);

		// Update MH
			{
			memcpy(Call, &Buffer[3], 32);
			Loc = strlop(Call, ' '); 
			strlop(Loc, ']');
			UpdateMHEx(TNC, Call, '!', 'I', &Loc[1], TRUE);
		}
		// Drop through to go to user if attached but not connected

	}
	//	Return others to user (if attached but not connected)

	if (TNC->Streams[0].Attached == 0)
		return;

	if (TNC->Streams[0].Connected || TNC->Streams[0].Connecting)
		return;

	if (MsgLen > 200)
		MsgLen = 200;

	buffptr = GetBuff();

	if (buffptr == 0)
	{
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} %s\r", Buffer);

	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
}

static VOID ARDOPProcessReceivedData(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;

	// May get several messages per packet
	// May get message split over packets

	//	Data  has a length field
	//	ARQ|FEC|ERR|, 2 byte count (Hex 0001 – FFFF), binary data”
	//	New standard doesnt have d:

	if (TNC->DataInputLen > 16000)	// Shouldnt have packets longer than this
		TNC->DataInputLen=0;

	//	OFDM can return large packets (up to 10160)
			
	// in serial mode, data has been put in input buffer by comms code

	if (TNC->ARDOPCommsMode == 'T')
	{
		InputLen=recv(TNC->TCPDataSock, &TNC->ARDOPDataBuffer[TNC->DataInputLen], 16384 - TNC->DataInputLen, 0);

		if (InputLen == 0 || InputLen == SOCKET_ERROR)
		{
			TNCLost(TNC);
			return;					
		}

		TNC->DataInputLen += InputLen;
	}
loop:

	if (TNC->OldMode)
		goto OldRX;

	else
	{	// No D:

		// Data = check we have it all

		int DataLen = (TNC->ARDOPDataBuffer[0] << 8) + TNC->ARDOPDataBuffer[1]; // HI First
		UCHAR DataType[4];
		UCHAR * Data;
			
		if (TNC->DataInputLen < DataLen + 2)
			return;					// Wait for more

		MsgLen = DataLen + 2;		// Len 

		memcpy(DataType, &TNC->ARDOPDataBuffer[2] , 3);
		DataType[3] = 0;
		
		Data = &TNC->ARDOPDataBuffer[5];
		DataLen -= 3;

		ARDOPProcessDataPacket(TNC, DataType, Data, DataLen);

		// See if anything else in buffer

		TNC->DataInputLen -= MsgLen;

		if (TNC->DataInputLen == 0)
			return;

		memmove(TNC->ARDOPDataBuffer, &TNC->ARDOPDataBuffer[MsgLen],  TNC->DataInputLen);
		goto loop;
	}
		
OldRX:

	if (TNC->DataInputLen < 8)
		return;					// Wait for more to arrive (?? timeout??)

	if (TNC->ARDOPDataBuffer[1] = ':')	// At least message looks reasonable
	{
		if (TNC->ARDOPDataBuffer[0] == 'd')
		{
			// Data = check we have it all

			int DataLen = (TNC->ARDOPDataBuffer[2] << 8) + TNC->ARDOPDataBuffer[3]; // HI First
//			unsigned short CRC;
			UCHAR DataType[4];
			UCHAR * Data;
			
			if (TNC->DataInputLen < DataLen + 4)
				return;					// Wait for more

			MsgLen = DataLen + 4;		// d: Len CRC

			memcpy(DataType, &TNC->ARDOPDataBuffer[4] , 3);
			DataType[3] = 0;
			Data = &TNC->ARDOPDataBuffer[7];
			DataLen -= 3;

			ARDOPProcessDataPacket(TNC, DataType, Data, DataLen);

			// See if anything else in buffer

			TNC->DataInputLen -= MsgLen;

			if (TNC->DataInputLen == 0)
				return;

			memmove(TNC->ARDOPDataBuffer, &TNC->ARDOPDataBuffer[MsgLen],  TNC->DataInputLen);
			goto loop;
		}
		else
			// Duff - clear input buffer
			TNC->DataInputLen = 0;

	}	
	return;
}



static VOID ARDOPProcessReceivedControl(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[4096];

	// May get several messages per packet
	// May get message split over packets

	//	Commands end with CR.

	if (TNC->InputLen > 8000)	// Shouldnt have packets longer than this
		TNC->InputLen=0;

	// in serial mode, data has been put in input buffer by comms code

	if (TNC->ARDOPCommsMode == 'T')
	{
		//	I don't think it likely we will get packets this long, but be aware...
				
		InputLen=recv(TNC->TCPSock, &TNC->ARDOPBuffer[TNC->InputLen], 8192 - TNC->InputLen, 0);

		if (InputLen == 0 || InputLen == SOCKET_ERROR)
		{
			TNCLost(TNC);
			return;					
		}

		TNC->InputLen += InputLen;
	}

loop:

	ptr = memchr(TNC->ARDOPBuffer, '\r', TNC->InputLen);

	if (ptr == 0)	//  CR in buffer
		return;		// Wait for it

	ptr2 = &TNC->ARDOPBuffer[TNC->InputLen];

	if ((ptr2 - ptr) == 1)	// CR (no CRC in new version)
	{
		// Usual Case - single meg in buffer
	
		ARDOPProcessResponse(TNC, TNC->ARDOPBuffer, TNC->InputLen);
		TNC->InputLen=0;
		return;
	}
	else
	{
		// buffer contains more that 1 message


		MsgLen = TNC->InputLen - (int)(ptr2-ptr) + 1;	// Include CR 

		memcpy(Buffer, TNC->ARDOPBuffer, MsgLen);

		ARDOPProcessResponse(TNC, Buffer, MsgLen);

		if (TNC->InputLen < MsgLen)
		{
			TNC->InputLen = 0;
			return;
		}
		memmove(TNC->ARDOPBuffer, ptr + 1,  TNC->InputLen-MsgLen);

		TNC->InputLen -= MsgLen;
		goto loop;
	}	
	return;
}



VOID ARDOPProcessDataPacket(struct TNCINFO * TNC, UCHAR * Type, UCHAR * Data, int Length)
{
	// Info on Data Socket - just packetize and send on
	
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int PacLen = 236;
	PMSGWITHLEN buffptr;
		
	TNC->TimeSinceLast = 0;

	if (strcmp(Type, "IDF") == 0)
	{
		// Place ID frames in Monitor Window and MH

		char Call[20];
		char * Loc;

// GM8BPQ-2:[IO68VL] 
//ID:GM8BPQ-2 IO68VL : 
// GM8BPQ-2:[IO68VL] 
//ID:GM8BPQ-2 [IO68vl]: 
//ID:HB9AVK JN47HG : 

// TX BPQ IDF  GM8BPQ-2:[IO68VL] 
// RX Rick IDF ID:GM8BPQ-2 [IO68vl]: 

// TX Rick IDF  GM8BPQ-2:[IO68VL] 
// RX BPQ IDF ID:GM8BPQ-2 IO68VL : 

//ID:GM8BPQ-2 [IO68vl] : 

		Data[Length] = 0;
		WritetoTrace(TNC, Data, Length);

		Debugprintf("ARDOP IDF %s", Data);

		// Loos like transmitted ID doesnt have ID:
	
		if (memcmp(Data, "ID:", 3) == 0)	// These seem to be received ID's
		{
			memcpy(Call, &Data[3], 20);
			Loc = strlop(Call, ' '); 
			strlop(Loc, ']');
			UpdateMHEx(TNC, Call, '!', 'I', &Loc[1], TRUE);
		}
		return;
	}

	STREAM->BytesRXed += Length;

	Data[Length] = 0;	
	Debugprintf("ARDOP: RXD %d bytes", Length);

	sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->BytesTXed - STREAM->BytesOutstanding, STREAM->BytesRXed, STREAM->BytesOutstanding);
	MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

	
	if (TNC->FECMode)
	{	
		Length = (int)strlen(Data);
		if (Data[Length - 1] == 10)
			Data[Length - 1] = 13;	

	}

	if (strcmp(Type, "FEC") == 0)
	{
		// May be an APRS Message
		// These are delimired with ^ characters
		// As they are likely to be split across 
		// FEC blocks they need to be recombined

		char * ptr = Data;
		char * ptr1;
		char * ptr2;
		char c;
		int Len = Length;

		Debugprintf(Data);

		if (*ptr == '^' || TNC->ARDOPAPRSLen)
		{
			// New Packet or continuation

			while (Len--)
			{
				c = *(ptr++);
				if (c == '^')
				{
					// may be start or end

					Debugprintf("Start/end of beacon Len = %d", TNC->ARDOPAPRSLen);

					if (TNC->ARDOPAPRSLen == 0)
						continue;		// Start

					// Validate and Process Block

					Debugprintf("beacon %s", TNC->ARDOPAPRS);

					ptr1 = TNC->ARDOPAPRS;		
					ptr2 = strchr(ptr1, '>');

					if (ptr2 && (ptr2 - ptr1) < 10)
					{
						// Could be APRS

//						if ((memcmp(ptr2 + 1, "AP", 2) == 0) || (memcmp(ptr2 + 1, "BE", 2) == 0))
						if (1)			// People using other dests
						{
							int APLen;

							// assume it is

							char * ptr3 = strchr(ptr2, '|');
							struct _MESSAGE * buffptr;

							if (ptr3 == 0)
							{
								TNC->ARDOPAPRSLen = 0;	
								Debugprintf("no |");
								continue;
							}

							buffptr = GetBuff();
							*(ptr3++) = 0;		// Terminate TO call
		
							APLen = TNC->ARDOPAPRSLen - (int)(ptr3 - ptr1);
											
							TNC->ARDOPAPRSLen = 0;	

							Debugprintf("Good APRS %d Left", Len);

							// Convert to ax.25 format

							if (buffptr == 0)
								continue;			// No buffers, so ignore

							buffptr->PORT = TNC->Port;

							ConvToAX25(ptr1, buffptr->ORIGIN);
							ConvToAX25(ptr2 + 1, buffptr->DEST);
							buffptr->ORIGIN[6] |= 1;				// Set end of address
							buffptr->CTL = 3;
							buffptr->PID = 0xF0;
							memcpy(buffptr->L2DATA, ptr3, APLen);
							buffptr->LENGTH  = 16 + MSGHDDRLEN + APLen;
							time(&buffptr->Timestamp);

							BPQTRACE((MESSAGE *)buffptr, TRUE);
						}
						else
						{
							Debugprintf("Not APRS");
							TNC->ARDOPAPRSLen = 0;		// in case not aprs
						}
					}
					else
					{
						Debugprintf("cant be APRS");
						TNC->ARDOPAPRSLen = 0;		// in case not aprs
					}
					continue;
				}

				// Normal Char

				TNC->ARDOPAPRS[TNC->ARDOPAPRSLen++] = c;
				if (TNC->ARDOPAPRSLen == 512)
					TNC->ARDOPAPRSLen = 0;
			}
			// End of packet.

			Debugprintf("End of Packet Len %d", TNC->ARDOPAPRSLen);
		}

		// FEC but not APRS. Discard if connected

		if (TNC->Streams[0].Connected)
			return;
	}

	WritetoTrace(TNC, Data, Length);

	// We can get messages of form ARQ [ConReq2000M: GM8BPQ-2 > OE3FQU]
	// Noe (V2) [ConReq2500 >  G8XXX]
	
	// when not connected.

	if (TNC->Streams[0].Connected == FALSE)
	{
		if (strcmp(Type, "ARQ") == 0)
		{
			if (Data[1] == '[')
			{
				// Log to MH
			
				char Call[20];
				char * ptr;

				// Add a Newline for monitoring 

				Data[Length++] = 13;
				Data[Length] = 0;

				ptr = strchr(Data, ':');

				if (ptr)
				{
					memcpy(Call, &ptr[2], 20);
					strlop(Call, ' '); 
					UpdateMH(TNC, Call, '!', 'I');
				}
			}
		}
	}

	if (TNC->Streams[0].Attached == 0)
		return;

	//	May need to fragment

	while (Length)
	{
		int Fraglen = Length;

		if (Length > PACLEN)
			Fraglen = PACLEN;

		Length -= Fraglen;

		buffptr = GetBuff();	

		if (buffptr == 0)
			return;			// No buffers, so ignore
				
		memcpy(&buffptr->Data[0], Data, Fraglen);

		Data += Fraglen;

		buffptr->Len = Fraglen;

		C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
	}

	return;
}

/*
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct TNCINFO * TNC = (struct TNCINFO * )lParam;
		char * ptr1, *ptr2;
		int ptr3 = 0;
		char Line[1000];
		int len;

		ptr1 = TNC->CaptureDevices;

		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_CAPTURE, Line);

		ptr3 = 0;

		ptr1 = TNC->PlaybackDevices;
	
		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_PLAYBACK, Line);

		SendDlgItemMessage(hDlg, IDC_PLAYBACK, EM_SETSEL, -1, 0);

//		KillTNC(TNC);

		return TRUE; 
	}

	case WM_SIZING:
	{
		return TRUE;
	}

	case WM_ACTIVATE:

//		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:


		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}
*/


VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[Stream].BytesOutstanding == 0)
	{
		if (Stream == 0)
			ARDOPSendCommand(TNC, "DISCONNECT", TRUE);
		else
		{
			char Cmd[32];
			sprintf(Cmd, "%cDISCONNECT", 1);			
			ARDOPSendPktCommand(TNC, Stream, Cmd);
		}
	}
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
		ARDOPSendCommand(TNC, "ABORT", TRUE);
	else
	{
		char Cmd[32];
		sprintf(Cmd, "%cDISCONNECT", Stream);			
		ARDOPSendPktCommand(TNC, Stream, Cmd);
	}
}



VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
	{
		ARDOPReleaseTNC(TNC);

		if (TNC->FECMode)
		{
			TNC->FECMode = FALSE;
			ARDOPSendCommand(TNC, "SENDID", TRUE);
		}
	}
}

VOID ARDOPAbort(struct TNCINFO * TNC)
{
	ARDOPSendCommand(TNC, "ABORT", TRUE);
}

// Host Mode Stuff (we reuse some routines in SCSPactor)

VOID ARDOPCRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	unsigned short int crc;
	UCHAR StuffedMsg[500];
	int i, j;

    Msg[3] |= TNC->Toggle;

//	Debugprintf("ARDOP TX Toggle %x", TNC->Toggle);

	crc = compute_crc(&Msg[2], Len-2);
	crc ^= 0xffff;

	Msg[Len++] = (crc&0xff);
	Msg[Len++] = (crc>>8);

	for (i = j = 2; i < Len; i++)
	{
		StuffedMsg[j++] = Msg[i];
		if (Msg[i] == 170)
		{
			StuffedMsg[j++] = 0;
		}
	}

	if (j != i)
	{
		Len = j;
		memcpy(Msg, StuffedMsg, j);
	}

	TNC->TXLen = Len;

	Msg[0] = 170;
	Msg[1] = 170;

	ARDOPWriteCommBlock(TNC);

	TNC->Retries = 5;
}

VOID ARDOPExitHost(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	// Try to exit Host Mode

	TNC->TXBuffer[2] = 31;
	TNC->TXBuffer[3] = 0x41;
	TNC->TXBuffer[4] = 0x5;
	memcpy(&TNC->TXBuffer[5], "JHOST0", 6);

	ARDOPCRCStuffAndSend(TNC, Poll, 11);
	return;
}


VOID ARDOPDoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		ARDOPExitHost(TNC);
		TNC->Retries = 1;

		return;
	}

	if (TNC->ReinitState == 1)
	{
		// Forcing back to Term Mode

		TNC->ReinitState = 0;
		ARDOPDoTNCReinit(TNC);				// See if worked
		return;
	}

	if (TNC->ReinitState == 3)
	{
		// Entering Host Mode
	
		// Assume ok

		TNC->HostMode = TRUE;
		TNC->IntCmdDelay = 10;

		return;
	}
}


VOID ARDOPDoTNCReinit(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Just Starting - Send a TNC Mode Command to see if in Terminal or Host Mode
	
		Poll[0] = 13;
		Poll[1] = 0x1B;
		TNC->TXLen = 2;

//		Debugprintf("Sending CR ESC, Mode %c", TNC->ARDOPCommsMode);

		if (TNC->ARDOPCommsMode == 'E')
		{
			if (TNC->TCPCONNECTED)	
			{
				int SentLen = send(TNC->TCPSock, TNC->TXBuffer, TNC->TXLen, 0);
			
				if (SentLen != TNC->TXLen)
				{
					// ARDOP doesn't seem to recover from a blocked write. For now just reset
			
					int winerr=WSAGetLastError();
					char ErrMsg[80];
				
					sprintf(ErrMsg, "ARDOP Write Failed for port %d - error code = %d\r\n", TNC->Port, winerr);
					WritetoConsole(ErrMsg);
					
					closesocket(TNC->TCPSock);
					TNC->TCPSock = 0;
					TNC->TCPCONNECTED = FALSE;
				}
			}

			TNC->TNCOK = FALSE;
			sprintf(TNC->WEB_COMMSSTATE,"%s Initialising TNC", TNC->ARDOPSerialPort);
			SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			TNC->Timeout = 20;				// 2 secs
			TNC->Retries = 1;
			return;
		}

		if (TNC->hDevice == 0)				// Dont try to init if device not open
		{
			if (TNC->PortRecord->PORTCONTROL.PortStopped == 0)
				OpenCOMMPort(TNC, TNC->ARDOPSerialPort, TNC->ARDOPSerialSpeed, TRUE);

			TNC->Timeout = 100;				// 10 secs
			TNC->Retries = 1;
			return;
		}

		TNC->TNCOK = FALSE;
		sprintf(TNC->WEB_COMMSSTATE,"%s Initialising TNC", TNC->ARDOPSerialPort);
		SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);


		if (ARDOPWriteCommBlock(TNC) == FALSE)
		{
			if (TNC->hDevice)
			{
				Debugprintf("ARDOPWriteCommBlock Failed Mode %c", TNC->ARDOPCommsMode);
				CloseCOMPort(TNC->hDevice);
			}
			if (TNC->ARDOPCommsMode == 'S')
			{
				OpenCOMMPort(TNC, TNC->ARDOPSerialPort, TNC->ARDOPSerialSpeed, TRUE);
			}
			else
			{
#ifdef WIN32
#else
#ifdef NOI2C
#else	
				char i2cname[30];
				int fd;
				int retval;

				// Open and configure the i2c interface

				if (strlen(TNC->ARDOPSerialPort) < 3)
					sprintf(i2cname, "/dev/i2c-%s", TNC->ARDOPSerialPort);
				else
					strcpy(i2cname, TNC->ARDOPSerialPort);
		                      
				fd = TNC->hDevice = open(i2cname, O_RDWR);

				if (fd < 0)
					printf("Cannot find i2c bus %s\n", i2cname);
				else
				{
	 				retval = ioctl(fd,  I2C_SLAVE, TNC->ARDOPSerialSpeed);
		
					if(retval == -1)
						printf("Cannot open i2c device %x\n", TNC->ARDOPSerialSpeed);
 
 					ioctl(fd,  I2C_TIMEOUT, 10);
				}			 
#endif
#endif
			}
		}
		TNC->Retries = 1;
	}

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		Debugprintf("DOTNCReinit %d Complete - Entering Hostmode", TNC->Port);

		TNC->TXBuffer[2] = 0;
		TNC->Toggle = 0;

		memcpy(Poll, "JHOST4\r", 7);

		TNC->TXLen = 7;
		ARDOPWriteCommBlock(TNC);

		// Timeout will enter host mode

		TNC->Timeout = 1;
		TNC->Retries = 1;
		TNC->Toggle = 0;
		TNC->ReinitState = 3;	// Set toggle force bit
		TNC->OKToChangeFreq = 1;	// In case failed whilst waiting for permission
		TNC->CONNECTED = TRUE;

		return;
	}
}

VOID ARDOPProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;
	char * ptr1, * ptr2;
	int len;

	if (TNC->ReinitState == 0)
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;

		// Send ARDOP to make sure TNC is in a known state

		strcpy(Poll, "ARDOP\r");

//		OpenLogFile(TNC->Port);
//		WriteLogLine(TNC->Port, Poll, 7);
//		CloseLogFile(TNC->Port);

		TNC->TXLen = 6;
		ARDOPWriteCommBlock(TNC);

		TNC->Timeout = 60;				// 6 secs

		return;
	}
	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

	// Send INIT script

	ptr1 = &TNC->InitScript[0];

/*	GetSemaphore(&Semaphore, 52);

	while(TNC->BPQtoWINMOR_Q)
	{
		void ** buffptr = Q_REM(&TNC->BPQtoWINMOR_Q);

		if (buffptr)
			ReleaseBuffer(buffptr);
	}

	FreeSemaphore(&Semaphore, 52);
*/
	while (ptr1 && ptr1[0])
	{
		ptr2 = strchr(ptr1, 13);

		if (ptr2 == 0)
			break;

		len = (int)(ptr2 - ptr1) + 1;

		memcpy(Poll, ptr1, len);

		// if Date or Time command add current time

		if (_memicmp(ptr1, "DATETIME", 4) == 0)
		{
			time_t T;
			struct tm * tm;

			T = time(NULL);
			tm = gmtime(&T);	

			len = sprintf(Poll, "DATETIME %02d %02d %02d %02d %02d %02d\r",
				tm->tm_mday, tm->tm_mon + 1, tm->tm_year - 100,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
		}

		// if RADIOPTTON ? or RADIOPTTOFF ? replace ? 
		// with correct string

		if (_memicmp(ptr1, "RADIOPTTOFF ?", 13) == 0)
		{
			int Len = TNC->RIG->PTTOffLen;
			UCHAR * Cmd = TNC->RIG->PTTOff;
			char Hex[256];
			char * hexptr = Hex;
			int i, j;
			while(Len--)
			{
				i = *(Cmd++);
				j = i >>4;
				j += '0';		// ascii
				if (j > '9')
					j += 7;
				*(hexptr++) = j;

				j = i & 0xf;
				j += '0';		// ascii
				if (j > '9')
					j += 7;
				*(hexptr++) = j;
			}
			*(hexptr++) = 0;
			
			len = sprintf(Poll, "RADIOPTTOFF %s\r", Hex);
		}

		if (_memicmp(ptr1, "RADIOPTTON ?", 12) == 0)
		{
			int Len = TNC->RIG->PTTOnLen;
			UCHAR * Cmd = TNC->RIG->PTTOn;
			char Hex[256];
			char * hexptr = Hex;
			int i, j;
			while(Len--)
			{
				i = *(Cmd++);
				j = i >>4;
				j += '0';		// ascii
				if (j > '9')
					j += 7;
				*(hexptr++) = j;

				j = i & 0xf;
				j += '0';		// ascii
				if (j > '9')
					j += 7;
				*(hexptr++) = j;
			}
			*(hexptr++) = 0;
			
			len = sprintf(Poll, "RADIOPTTON %s\r", Hex);
		}

		TNC->TXLen = len;
		ARDOPWriteCommBlock(TNC);

		Sleep(50);

		TNC->Timeout = 60;				// 6 secs

		if (ptr2)
			*(ptr2++) = 13;		// Put CR back for next time 

		ptr1 = ptr2;
	}
	


		// All Sent - enter Host Mode

		ARDOPDoTNCReinit(TNC);		// Send Next Command
		return;
	}
}

int ARDOPProcessDEDFrame(struct TNCINFO * TNC, UCHAR * Msg, int framelen)
{
	PMSGWITHLEN buffptr;
	UCHAR * Buffer;				// Data portion of frame
	unsigned int Stream = 0, RealStream;

	if (Msg[0] == 255 && Msg[1] == 255)
	{
		goto tcpHostFrame;
	}

	if (TNC->HostMode == 0)
		return framelen;

	// Check toggle

//	Debugprintf("ARDOP RX Toggle = %x MSG[3] = %x", TNC->Toggle, Msg[3]);

	if (TNC->Toggle != (Msg[3] & 0x80))
	{
		Debugprintf("ARDOP PTC Seq Error");
		return framelen;		// should check if retrying
	}

	// Any valid frame is an ACK

	TNC->Toggle ^= 0x80;			// update toggle

	TNC->Timeout = 0;

	Msg[3] &= 0x7f;					// remove toggle

	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		
		TNC->TNCOK = TRUE;
		sprintf(TNC->WEB_COMMSSTATE,"%s TNC link OK", TNC->ARDOPSerialPort);
		SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}

tcpHostFrame:

	Stream = RealStream = Msg[2];

	//	See if Poll Reply or Data

	if (Msg[3] == 1 && Stream > 0 && Stream <= APMaxStreams)
	{
		// Ardop Packet Data. Probably Buffer Status

		int Len = (int)strlen(&Msg[4]);

		if (memcmp(&Msg[4], "Queued ", 7) == 0)
		{
			int Count = atoi(&Msg[11]);
			TNC->Streams[Stream].BytesOutstanding = Count;
		}

		return Len + 5;
	}

	if (Stream == 32)		// Native Mode Command Response
	{
		if (Msg[3] == 1)	// Null terminated response
		{
			int Len = (int)strlen(&Msg[4]) + 1;					
			ARDOPProcessResponse(TNC, &Msg[4], Len);
			return Len + 5;
		}
		if (Msg[3] == 0)	// Success, no response
			return 5;

		if (Msg[3] == 7)	// Status Reports
		{
			int Len = Msg[4] + 1;

			// may have more than one message in buffer,
			// so pass to unpack

			memcpy(&TNC->ARDOPBuffer[TNC->InputLen],&Msg[5], Len);
			TNC->InputLen += Len;

			ARDOPProcessReceivedControl(TNC);
			return Len + 5;
		}
		return 0;
	}
	if (Stream == 33)		// Native Mode Data
	{
		// May be connected, FEC or ID

		if (Msg[3] == 1)	// Null terminated response
			return 0;

		if (Msg[3] == 0)	// Success, no response
			return 0;

		if (Msg[3] == 7)	// Data
		{
			int Len = Msg[4] + 1;

			// may have more than one message in buffer,
			// so pass to unpack

			memcpy(&TNC->ARDOPDataBuffer[TNC->DataInputLen],&Msg[5], Len);
			TNC->DataInputLen += Len;

			ARDOPProcessReceivedData(TNC);
			return 0;
		}
		return 0;

	}		
		
	if (Stream == 34)		// Native Mode Log
	{
		int Len = Msg[4] + 1;
		char timebuf[32];
		char Line[256];
		char * ptr, * ptr2;
#ifdef WIN32
		SYSTEMTIME st;
#else
		struct timespec tp;
		int hh;
		int mm;
		int ss;
#endif
		if (TNC->LogHandle == 0 && TNC->DebugHandle == 0)
			return 0;
			
#ifdef WIN32
		GetSystemTime(&st);
		sprintf(timebuf, "%02d:%02d:%02d.%03d ",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
		clock_gettime(CLOCK_REALTIME, &tp);
		ss = tp.tv_sec % 86400;		// Secs int day
		hh = ss / 3600;
		mm = (ss - (hh * 3600)) / 60;
		ss = ss % 60;

		sprintf(timebuf, "%02d:%02d:%02d.%03d ",
			hh, mm, ss, (int)tp.tv_nsec/1000000);
#endif
		// Messages may be blocked with top bit of first byte set

		ptr = &Msg[5];
		ptr2 = Line;

		while(Len--)
		{
			int c = (*ptr++);
			
			if (c & 0x80)
			{
				// New Message

				c &= 0x7f;

				*ptr2 = 0;
				fputs(Line, TNC->DebugHandle);		// rest of last line
				if (TNC->LastLogType < '7')
					fputs(Line, TNC->LogHandle);

				TNC->LastLogType = c; 

				// Timestamp new message and add type

				fputs(timebuf, TNC->DebugHandle);
				fputc(c, TNC->DebugHandle);
				fputc(' ', TNC->DebugHandle);
				if (TNC->LastLogType < '7')
				{
					fputs(timebuf, TNC->LogHandle);
					fputc(c, TNC->LogHandle);
					fputc(' ', TNC->LogHandle);
				}
				ptr2 = Line;
			}
			else 
				*(ptr2++) = c;
		}	
		*ptr2 = 0;

		fputs(Line, TNC->DebugHandle);		// rest of last line
		fflush(TNC->DebugHandle);
	
		if (TNC->LastLogType < '7')
		{
			fputs(Line, TNC->LogHandle);
			fflush(TNC->LogHandle);
		}

		return 0;
	}

	if (Msg[3] == 4 || Msg[3] == 5)
	{
		MESSAGE Monframe;

		// Packet Monitor Data.
		// DED Host uses 4 and 5 as Null Terminated ascii encoded header
		// and 6 byte count format info.
			
		// In ARDOP Native mode I pass both header and data
		// in byte count raw format, as there is no point 
		// in ascii coding then converting back to pass to 
		// monitor code

		// The First byte is TX/RX Flag

		int Len = Msg[4];		// Would be +1 but first is Flag
							
		memset(&Monframe, 0, sizeof(Monframe));

		memcpy(Monframe.DEST, &Msg[6], Len);
		Monframe.LENGTH = Len + MSGHDDRLEN;
		Monframe.PORT = TNC->Port | Msg[5];		// or in TX Flag

		time(&Monframe.Timestamp);
	
		if (Msg[3] == 5)								// More to come
		{
			// Save the header till the data arrives

			if (TNC->Monframe)
				free(TNC->Monframe);
			
			TNC->Monframe = malloc(sizeof(MESSAGE));
		
			if (TNC->Monframe)
				memcpy(TNC->Monframe, &Monframe, sizeof(MESSAGE));

			return Len + 6;	
		}

		BPQTRACE((MESSAGE *)&Monframe, TRUE);
		return Len + 6;
	}

	if (Msg[3] == 6)
	{
		// Second part of I or UI

		int Len = Msg[4] + 1;

		MESSAGE Monframe;
		UCHAR * ptr = (UCHAR *)&Monframe;

		memset(&Monframe, 0, sizeof(Monframe));

		if (TNC->Monframe)
		{
			memcpy(&Monframe, TNC->Monframe, TNC->Monframe->LENGTH);
			memcpy(&ptr[TNC->Monframe->LENGTH], &Msg[5], Len);

			Monframe.LENGTH += Len;

			time(&Monframe.Timestamp);
			BPQTRACE((MESSAGE *)&Monframe, TRUE);

			free(TNC->Monframe);
			TNC->Monframe = NULL;
		}
		return Len + 6;
	}
	
	if (Msg[3] == 0)
	{
		// Success - Nothing Follows

		if (Stream < 32)
			if (TNC->Streams[Stream].CmdSet)
				return 4;						// Response to Command Set

		if ((TNC->TXBuffer[3] & 1) == 0)	// Data
			return 4;

		if (Stream > 0 && Stream <= APMaxStreams)
		{
			// Packet Response

			return 4;
		}

		// If the response to a Command, then we should convert to a text OK" for forward scripts, etc

		if (TNC->TXBuffer[5] == 'G')	// Poll
			return 4;

		if (TNC->TXBuffer[5] == 'C')	// Connect - reply we need is async
			return 4;

		if (TNC->TXBuffer[5] == 'L')	// Shouldnt happen!
			return 4;

		if (TNC->TXBuffer[5] == '#')	// Shouldnt happen!
			return 4;

		if (TNC->TXBuffer[5] == '%' && TNC->TXBuffer[6] == 'W')	// Scan Control - Response to W1
			if (TNC->InternalCmd)
				return 4;					// Just Ignore

		if (TNC->TXBuffer[5] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[10] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return 4;
			}
		}
		
		if (TNC->Streams[Stream].Connected)
			return 4;

		buffptr = GetBuff();

		if (buffptr == NULL) return 4;			// No buffers, so ignore

		buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0],"ARDOP} Ok\r");


		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
//		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return 4;
	}

	if (Msg[3] > 0 && Msg[3] < 6)
	{
		// Success with message - null terminated

		UCHAR * ptr;
		int len;

		if (Msg[2] == 0xff)			// General Poll Response
		{
			UCHAR * Poll = TNC->TXBuffer;
			UCHAR Chan = Msg[4] - 1;

			if (Chan == 255)			// Nothing doing
				return 0;	
		
			if (Msg[5] != 0)
			{
				// More than one to poll - save the list of channels to poll

				strcpy(TNC->NexttoPoll, &Msg[5]);
			}

			// Poll the channel that had data

			Poll[2] = Chan;			// Channel
			Poll[3] = 0x1;			// Command

			if (Chan == 254)		// Status - Send Extended Status (G3)
			{
				Poll[4] = 1;			// Len-1
				Poll[5] = 'G';			// Extended Status Poll
				Poll[6] = '3';
			}
			else
			{
				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll
			}

			ARDOPCRCStuffAndSend(TNC, Poll, Poll[4] + 6);
			TNC->InternalCmd = FALSE;

			return 0;
		}

		Buffer = &Msg[4];
		
		ptr = strchr(Buffer, 0);

		if (ptr == 0)
			return 0;

		*(ptr++) = 13;
		*(ptr) = 0;

		len = (int)(ptr - Buffer);

		if (len > 256)
			return 0;

		if (Stream > 0 && Stream <= APMaxStreams)
		{
			// Packet Mode Response. Could be command response or status.

			struct STREAMINFO * STREAM = &TNC->Streams[Stream];
			PMSGWITHLEN buffptr;

			if (strstr(Buffer, "Incoming"))
			{
				// incoming call. Check which application it is for

				char Call[11];
				char TargetCall[11] = "";
				char * ptr;
				APPLCALLS * APPL;
				char * ApplPtr = APPLS;
				int App;
				char Appl[10];
				TRANSPORTENTRY * SESS;

				Buffer[len-1] = 0;
				WritetoTrace(TNC, Buffer, len);

				STREAM->ConnectTime = time(NULL); 
				STREAM->BytesRXed = STREAM->BytesTXed = STREAM->PacketsSent = 0;

				memcpy(Call, &Buffer[19], 10);
				ptr = strchr(Call, ' ');	
				if (ptr) *ptr = 0;

				ptr = strstr(&Buffer[19], " to ");	
				if (ptr)
				{
					memcpy(TargetCall, ptr + 4, 10);
					ptr = strchr(TargetCall, 13);
					if (ptr)
						*ptr = 0;
				}

				ProcessIncommingConnectEx(TNC, Call, Stream, TRUE, FALSE);
				
				SESS = TNC->PortRecord->ATTACHEDSESSIONS[Stream];
						
				// Check for ExcludeList

				if (ExcludeList[0])
				{
					if (CheckExcludeList(SESS->L4USER) == FALSE)
					{
						TidyClose(TNC, 0);
						Debugprintf("ARDOP Packet Call from %s rejected", Call);
						return 0;
					}
				}

				//	IF WE HAVE A PERMITTED CALLS LIST, SEE IF HE IS IN IT

				if (TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS)
				{
					UCHAR * ptr = TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS;

					while (TRUE)
					{
						if (memcmp(SESS->L4USER, ptr, 6) == 0)	// Ignore SSID
							break;

						ptr += 7;

						if ((*ptr) == 0)							// Not in list
						{
							TidyClose(TNC, 0);
							Debugprintf("ARDOP Packet Call from %s not in ValidCalls - rejected", Call);
							return 0;
						}
					}
				}


				// See which application the connect is for
	
				for (App = 0; App < 32; App++)
				{
					APPL=&APPLCALLTABLE[App];
					memcpy(Appl, APPL->APPLCALL_TEXT, 10);
					ptr=strchr(Appl, ' ');

					if (ptr)
						*ptr = 0;
	
					if (_stricmp(TargetCall, Appl) == 0)
						break;
				}

				if (App < 32)
				{
					char AppName[13];

					memcpy(AppName, &ApplPtr[App * sizeof(CMDX)], 12);
					AppName[12] = 0;

				// Make sure app is available

					if (CheckAppl(TNC, AppName))
					{
						char ApplCmd[80];
						int Len = sprintf(ApplCmd, "%s\r", AppName);

						buffptr = GetBuff();
	
						if (buffptr == 0)
						{
							return len + 5;			// No buffers, so ignore
						}

						buffptr->Len = Len;
						memcpy(&buffptr->Data[0], ApplCmd, Len);

						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
		
						TNC->SwallowSignon = TRUE;

						// Save Appl Call in case needed for 

					}
					else
					{
						char Msg[] = "Application not available\r\n";
					
						// Send a Message, then a disconenct

						// Send CTEXT First

						if (STREAM->BPQtoPACTOR_Q)		//Used for CTEXT
						{
							PMSGWITHLEN buffptr = Q_REM(&STREAM->BPQtoPACTOR_Q);
							UCHAR * data = &buffptr->Data[0];
							int txlen = (int)buffptr->Len;
							SendARDOPorPacketData(TNC, Stream, data, txlen);
							ReleaseBuffer(buffptr);
						}
					
						SendARDOPorPacketData(TNC, Stream, Msg, (int)strlen(Msg));
						STREAM->NeedDisc = 100;	// 10 secs
					}
				}
			
				STREAM->Connected = TRUE;
				return len + 5;
			}

			// Send to host

			buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0)
				return len + 5;			// No buffers, so ignore
		
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "%s", Buffer);
			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

			// Unless Connected response close session

			STREAM->Connecting = FALSE;

			if (strstr(Buffer, "Connected"))
				STREAM->Connected = TRUE;
			else
			if (strstr(Buffer, "Failure with"))
				STREAM->ReportDISC = 10;		// Gives time for failure message to display
			else
			if (strstr(Buffer, "Busy from"))
				STREAM->ReportDISC = 10;		// Gives time for failure message to display
			else
			if (strstr(Buffer, "Disconnected from"))
			{
				if (STREAM->Disconnecting)		// We requested disconnect
				{
					STREAM->Connecting = FALSE;
					STREAM->Connected = FALSE;		// Back to Command Mode
					STREAM->Disconnecting = FALSE;
				}
				else
				{
					STREAM->Connected = 0;
					STREAM->ReportDISC = 10;
				}
			}
			else
				STREAM->NeedDisc = 10;

			return len + 5;
		}

		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		if (Msg[3] < 3)						// 1 or 2 - Success or Fail
		{
			return 0;
		}

		if (Msg[3] == 3)					// Status
		{			
			struct STREAMINFO * STREAM = &TNC->Streams[Stream];
			return 0;
		}

		// 1, 2, 4, 5 - pass to Appl

		return 0;
	}

	if (Msg[3] == 6)
	{
		return 0;
	}

	if (Msg[3] == 7)				// Data
	{
		if (Stream > 0 && Stream <= APMaxStreams)
		{
			// Packet Response

			int len = Msg[4] + 1;

			if (TNC->Streams[Stream].Connected == 0)
				return len + 5;
	
			buffptr = GetBuff();
	
			if (buffptr == NULL) return 0;			// No buffers, so ignore

			buffptr->Len = len;
			memcpy((UCHAR *)&buffptr->Data[0], &Msg[5], buffptr->Len);

			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			return len + 5;
		}

		if (Stream == 32)			// Command string
		{
			int Len = Msg[4] + 1;
							
			ARDOPProcessResponse(TNC, &Msg[5], Len);

			return 0;
		}
		
		if (Msg[2] == 0xfe)						// Status Poll Response
		{
			return 0;
		}

		if (Msg[2] == 248)	// Log Message
		{
			// Monitor Data - Length format
			// first 4 bytes contain a 32 bits long timestamp.
			// That timestamp holds the number of seconds that elapsed since date 01.01.2000 at 00:00:00.
			// The MS byte is sent first. The timestamp can be corrected to the usual C timestamp (seconds
			//since 01.01.1970, 00:00:00) simply by adding 946684800 (seconds) to it.
			// Teminated with LF

			int datalen = Msg[4] + 1;
			time_t timestamp = (Msg[5] << 24) + (Msg[6] << 16)
				+ (Msg[7] << 8) + Msg[8] + 946684800;
			char c;
			char timebuf[32] = "HH:MM:SS.MMM";
			struct tm * tm;

			if (TNC->LogHandle == 0 || TNC->DebugHandle == 0)
				return 0;

			tm = gmtime(&timestamp);			

			sprintf(timebuf, "%02d:%02d:%02d.    ",
				tm->tm_hour, tm->tm_min, tm->tm_sec);

			// ARDOP Messages have a millisec time in first 3 bytes
			// and a log type in 4th

			c = Msg[12];		// Type

			memcpy(&timebuf[9], &Msg[9], 3);	// copy millisecs
			fputs(timebuf, TNC->DebugHandle);
			fputc(c, TNC->DebugHandle);
			fputc(' ', TNC->DebugHandle);
			fwrite(&Msg[13], 1, datalen - 8, TNC->DebugHandle);
			fflush(TNC->DebugHandle);

			if (c < '7')
			{
				// All types below debug go to log file

				fputs(timebuf, TNC->LogHandle);
				fputc(c, TNC->LogHandle);
				fputc(' ', TNC->LogHandle);
				fwrite(&Msg[13], 1, datalen - 8, TNC->LogHandle);
				fflush(TNC->LogHandle);
			}
			return 0;
		}

		if (Msg[2] == 253)						// Rig Port Response
		{
			// Queue for Rig Control Driver
			
			int datalen = Msg[4] + 1;
			PMSGWITHLEN buffptr;

			// if not configured to use PTC Rig Control, Ignore

			if (TNC->RIG->PORT == NULL || TNC->RIG->PORT->PTC == NULL)
				return 0;
			
			buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr)
			{
				buffptr->Len = datalen;
				memcpy(&buffptr->Data[0], &Msg[5], datalen);
				C_Q_ADD(&TNC->RadiotoBPQ_Q, buffptr);
			}
			return 0;
		}	

		if (Msg[2] == 250)						// KISS
		{
			// Pass to KISS Code
			
			int datalen = Msg[4] + 1;
			void ** buffptr = NULL;

			ProcessKISSBytes(TNC, &Msg[5], datalen);
			return datalen + 5;
		}
		
		return 0;
	}
	return 0;
}

void ARDOPSCSCheckRX(struct TNCINFO * TNC)
{
	int Length, Len = 0;
	unsigned short crc;
	char UnstuffBuffer[500];

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	if (TNC->ARDOPCommsMode == 'I')
	{
		unsigned char Buffer[33];
		BOOL Error;	
		int gotThisTime = 0, i2clen;

		// i2c mode always returns as much as requested or error
		// First two bytes of block are length

//		if (TNC->hDevice < 0)
//			return;

		while ((TNC->RXLen + Len) < 460)
		{
			i2clen = ReadCOMBlockEx(TNC->hDevice, Buffer, 33, &Error);
				
			if (i2clen < 33 || i2clen == 5)
				return;

			if (Error)
			{
				Debugprintf("ARDOP i2c returned %d bytes Error %d", i2clen, Error);
				return;
			}
			gotThisTime = Buffer[0];


			if (gotThisTime == 0)
			{
				if (Len)
					break;			// Something to process

				return;				// No More
			}

//			if (gotThisTime != 7)
//				Debugprintf("ARDOP i2c Len %d  RXL %d %x %x %x %x %x %x %x %x %x %x %x %x",
//					gotThisTime, TNC->RXLen + Len,
//					Buffer[0], Buffer[1], Buffer[2], Buffer[3],
//					Buffer[4], Buffer[5], Buffer[6], Buffer[7],
//					Buffer[8], Buffer[9], Buffer[10], Buffer[11]);

			memcpy(&TNC->RXBuffer[TNC->RXLen + Len], &Buffer[1], gotThisTime);
	
			Len += gotThisTime;

			if (Buffer[0] < 32)
				break;				// no more
		}
	}

	else if (TNC->ARDOPCommsMode =='E')		//Serial over TCP
		Len = SerialGetTCPMessage(TNC, &TNC->RXBuffer[TNC->RXLen], 500 - TNC->RXLen);
	else
		if (TNC->hDevice)
			Len = ReadCOMBlock(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], 500 - TNC->RXLen);

	if (Len == 0)
		return;
	
	TNC->RXLen += Len;

	Length = TNC->RXLen;

	// DED mode doesn't have an end of frame delimiter. We need to know if we have a full frame

	// Fortunately this is a polled protocol, so we only get one frame at a time

	// If first char != 170, then probably a Terminal Mode Frame. Wait for CR on end

	// If first char is 170, we could check rhe length field, but that could be corrupt, as
	// we haen't checked CRC. All I can think of is to check the CRC and if it is ok, assume frame is
	// complete. If CRC is duff, we will eventually time out and get a retry. The retry code
	// can clear the RC buffer
			
	if (TNC->RXBuffer[0] != 170)
	{
		// Char Mode Frame I think we need to see cmd: on end

			// If we think we are in host mode, then to could be noise - just discard.

		if (TNC->HostMode)
		{
			TNC->RXLen = 0;		// Ready for next frame
			return;
		}

		TNC->RXBuffer[TNC->RXLen] = 0;

//		if (TNC->Streams[Stream].RXBuffer[TNC->Streams[Stream].RXLen-2] != ':')

		if (strlen(TNC->RXBuffer) < TNC->RXLen)
			TNC->RXLen = 0;

		if ((strstr(TNC->RXBuffer, "cmd: ") == 0) && (strstr(TNC->RXBuffer, "pac: ") == 0))

			return;				// Wait for rest of frame

		// Complete Char Mode Frame

//		OpenLogFile(TNC->Port);
//		WriteLogLine(TNC->Port, TNC->RXBuffer, (int)strlen(TNC->RXBuffer));
//		CloseLogFile(TNC->Port);

		TNC->RXLen = 0;		// Ready for next frame
					
		if (TNC->HostMode == 0)
		{
			// We think TNC is in Terminal Mode
			ARDOPProcessTermModeResponse(TNC);
			return;
		}
		// We thought it was in Host Mode, but are wrong.

		TNC->HostMode = FALSE;
		return;
	}

	if (TNC->HostMode == FALSE)
	{
		TNC->RXLen = 0;			// clear input and wait for char mode response
		return;
	}


	// Receiving a Host Mode frame

	if (Length < 6)				// Minimum Frame Sise
		return;

	if (TNC->RXBuffer[2] == 170)
	{
		// Retransmit Request
	
		TNC->RXLen = 0;
		return;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice


	Length = Unstuff(&TNC->RXBuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return;				// Ignore for now
	}

	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		UnstuffBuffer[0] = 0;		// Make sure not seen as TCP Frame
		ARDOPProcessDEDFrame(TNC, UnstuffBuffer, Length);

		// If there are more channels to poll (more than 1 entry in general poll response,
		// and link is not active, poll the next one

		if (TNC->Timeout == 0)
		{
			UCHAR * Poll = TNC->TXBuffer;

			if (TNC->NexttoPoll[0])
			{
				UCHAR Chan = TNC->NexttoPoll[0] - 1;

				memmove(&TNC->NexttoPoll[0], &TNC->NexttoPoll[1], 19);

				Poll[2] = Chan;			// Channel
				Poll[3] = 0x1;			// Command

				if (Chan == 254)		// Status - Send Extended Status (G3)
				{
					Poll[4] = 1;			// Len-1
					Poll[5] = 'G';			// Extended Status Poll
					Poll[6] = '3';
				}
				else
				{
					Poll[4] = 0;			// Len-1
					Poll[5] = 'G';			// Poll
				}

				ARDOPCRCStuffAndSend(TNC, Poll, Poll[4] + 6);
				TNC->InternalCmd = FALSE;

				return;
			}
			else
			{
				// if last message wasn't a general poll, send one now

				if (TNC->PollSent)
					return;

				TNC->PollSent = TRUE;

				// Use General Poll (255)

				Poll[2] = 255 ;			// Channel
				Poll[3] = 0x1;			// Command

				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll

				ARDOPCRCStuffAndSend(TNC, Poll, 6);
				TNC->InternalCmd = FALSE;
			}
		}
		return;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return;
}

VOID ARDOPSCSPoll(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;
	int Stream = 0;

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		TNC->Retries--;

		if(TNC->Retries >= 0)
		{
			if (TNC->HostMode)
				Debugprintf("ARDOP Timeout - Retransmit PTC Block");
			ARDOPWriteCommBlock(TNC);	// Retransmit Block
			return;
		}

		// Retried out.

		if (TNC->HostMode == 0)
		{
			ARDOPDoTermModeTimeout(TNC);
			return;
		}

		// Retried out in host mode - Clear any connection and reinit the TNC

		Debugprintf("ARDOP - Link to TNC Lost");
		TNC->TNCOK = FALSE;

		sprintf(TNC->WEB_COMMSSTATE,"%s Open but TNC not responding", TNC->PortRecord->PORTCONTROL.SerialPortName);
		SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		// Clear anything from UI_Q

		while (TNC->PortRecord->UI_Q)
		{
			void ** buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);
		}


		TNC->HostMode = 0;
		TNC->ReinitState = 0;
		TNC->CONNECTED = FALSE;
		
		// Disconenct any attached sessions
	

		for (Stream = 0; Stream <= APMaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			}
		}
	}

	if (TNC->Timeout)
		return;				// We've sent something


	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		ARDOPDoTNCReinit(TNC);
		return;
	}

	TNC->PollSent = FALSE;

	if (TNC->TNCOK && TNC->BPQtoRadio_Q)
	{
		int datalen;
		PMSGWITHLEN buffptr;
			
		buffptr = (PMSGWITHLEN)Q_REM(&TNC->BPQtoRadio_Q);
		datalen = (int)buffptr->Len;
		Poll[2] = 253;		// Radio Channel
		Poll[3] = 0;		// Data?
		Poll[4] = datalen - 1;
		memcpy(&Poll[5], &buffptr->Data[0], datalen);
	
		ReleaseBuffer(buffptr);	
		ARDOPCRCStuffAndSend(TNC, Poll, datalen + 5);
		return;
	}

	for (Stream = 0; Stream < 14; Stream++)	// Priority to commands
	{
		if (TNC->TNCOK && TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			PMSGWITHLEN buffptr;
			char * Buffer;
		
			buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
			TNC->Streams[Stream].FramesQueued--;

			datalen = (int)buffptr->Len;
			Buffer = &buffptr->Data[0];	// Data portion of frame

			Poll[3]= 0;

			if (Stream > 11)
				Poll[2] = Stream + 20;		// 12 and 13 to Channels 32 and 33
			else
				if (Stream == 0)
					Poll[2] = 33;
				else
				{
					// Packet Frame

					Poll[2] = Stream;
					Poll[3] = Buffer[0];			// First Byte is CMD/Data FLag
					datalen--;
					Buffer++;
			}

			Poll[4] = datalen - 1;

			memcpy(&Poll[5], Buffer, datalen);
		
			ReleaseBuffer(buffptr);

			ARDOPCRCStuffAndSend(TNC, Poll, datalen + 5);
			return;
		}
	}

//0x04421CB0  aa aa 21 00 07 00 06 48 65 6c 6c 6f 0d c8 3e 38 42 50 51 2d 32 20 35 0d 4a 8a 4d 38 42 50 51  ªª!....Hello.È>8BPQ-2 5.JŠM8BPQ
//0x04421CCF  2d 31 30 2c 47 4d 38 42 50 51 2d 35 2c 47 4d 38 42 50 51 2c 47 4d 38 42 50 51 2d 31 35 0d 00  -10,GM8BPQ-5,GM8BPQ,GM8BPQ-15..
//0x04421CEE  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ...............................


	if (TNC->TNCOK && TNC->KISSTX_Q)
	{
		int datalen;
		PMSGWITHLEN buffptr;
			
		buffptr = (PMSGWITHLEN)Q_REM(&TNC->KISSTX_Q);
		datalen = (int)buffptr->Len;
		Poll[2] = 250;		// KISS Channel
		Poll[3] = 0;		// Data
		Poll[4] = datalen - 1;
		memcpy(&Poll[5], &buffptr->Data[0], datalen);
	
		ReleaseBuffer(buffptr);	
		ARDOPCRCStuffAndSend(TNC, Poll, datalen + 5);
		return;
	}

	TNC->PollSent = TRUE;

	// Use General Poll (255)

	Poll[2] = 255 ;			// Channel
	Poll[3] = 0x1;			// Command

	if (TNC->ReinitState == 3)
	{
		TNC->ReinitState = 0;
		Poll[3] = 0x41;
	}

	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	ARDOPCRCStuffAndSend(TNC, Poll, 6);
	TNC->InternalCmd = FALSE;

	return;

}

// ARDOP Serial over TCP Routines

// Probably only for Teensy with ESP01. Runs SCS Emulator over a TCP Link


VOID SerialConnecttoTCPThread(struct TNCINFO * TNC);

int SerialConnecttoTCP(struct TNCINFO * TNC)
{
	_beginthread(SerialConnecttoTCPThread, 0, (void *)TNC);

	return 0;
}

VOID SerialConnecttoTCPThread(struct TNCINFO * TNC)
{
	char Msg[255];
	int i;
	u_long param = 1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	SOCKADDR_IN sinx; 
	int addrlen=sizeof(sinx);

	if (TNC->HostName == NULL)
		return;

	Sleep(5000);		// Allow init to complete 

	while(1)
	{
		if (TNC->TCPCONNECTED)
		{
			Sleep(57000);
			continue;
		}


		TNC->BusyFlags = 0;

		TNC->CONNECTING = TRUE;

		TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
		TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

		if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
		{
			//	Resolve name to address

			HostEnt = gethostbyname (TNC->HostName);
		 
			if (!HostEnt)
			{
				TNC->CONNECTING = FALSE;
				Sleep (57000);
				return;			// Resolve failed
			}
			 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
			 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		}

		if (TNC->TCPSock)
		{
			Debugprintf("ARDOP Closing Sock %d", TNC->TCPSock); 
			closesocket(TNC->TCPSock);
		}
	
		TNC->TCPSock = 0;
		TNC->TCPSock = socket(AF_INET,SOCK_STREAM,0);

		if (TNC->TCPSock == INVALID_SOCKET)
		{
			i=sprintf(Msg, "Socket Failed for ARDOP socket - error code = %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);

			TNC->CONNECTING = FALSE;
  			return; 
		}

		setsockopt(TNC->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
//	setsockopt(TNC->TCPDataSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

		sinx.sin_family = AF_INET;
		sinx.sin_addr.s_addr = INADDR_ANY;
		sinx.sin_port = 0;

		if (connect(TNC->TCPSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
		{
			//
			//	Connected successful

			TNC->TCPCONNECTED = TRUE;
			ioctl(TNC->TCPSock, FIONBIO, &param);
			Debugprintf("ARDOP TCPSerial connected, Socket %d", TNC->TCPSock);
		}
		else
		{
			if (TNC->Alerted == FALSE)
			{
 				sprintf(Msg, "Connect Failed for ARDOP socket - error code = %d Port %d\n",
					WSAGetLastError(), htons(TNC->destaddr.sin_port));

				WritetoConsole(Msg);
				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->Alerted = TRUE;
			}
		
			closesocket(TNC->TCPSock);
	
			TNC->TCPSock = 0;
			TNC->CONNECTING = FALSE;
			Sleep (57000);						// 1 Mins
		}
	}
}

int SerialGetTCPMessage(struct TNCINFO * TNC, unsigned char * Buffer, int Len)
{
	int index=0;
	ULONG param = 1;

	if (TNC->TCPCONNECTED)
	{
		int InputLen;

		//	Poll TCP COnnection for data

		// May have several messages per packet, or message split over packets

		InputLen = recv(TNC->TCPSock, Buffer, Len, 0);

		if (InputLen < 0)
		{
			int err = WSAGetLastError();

			if (err == 10035 || err == 11)
			{
				InputLen = 0;
				return 0;
			}
			Debugprintf("ARDOP Serial TCP RX Error  %d received for socket %d", err, TNC->TCPSock);
	
			TNC->TCPCONNECTED = 0;
			closesocket(TNC->TCPSock);
			TNC->TCPSock = 0;
			return 0;
		}

		if (InputLen > 0)
			return InputLen;
		else
		{
			Debugprintf("ARDOP Serial TCP Close received for socket %d", TNC->TCPSock);

			TNC->TCPCONNECTED = 0;
			closesocket(TNC->TCPSock);
			TNC->TCPSock = 0;
			return 0;
		}
	}

	return 0;
}

int ARDOPWriteCommBlock(struct TNCINFO * TNC)
{
	if (TNC->ARDOPCommsMode == 'E')
	{
		if (TNC->TCPCONNECTED)
		{
			int SentLen = send(TNC->TCPSock, TNC->TXBuffer, TNC->TXLen, 0);
		
			if (SentLen != TNC->TXLen)
			{
				// ARDOP doesn't seem to recover from a blocked write. For now just reset
			
				int winerr=WSAGetLastError();
				char ErrMsg[80];
				
				sprintf(ErrMsg, "ARDOP Write Failed for port %d - error code = %d\r\n", TNC->Port, winerr);
				WritetoConsole(ErrMsg);
					
				closesocket(TNC->TCPSock);
				TNC->TCPSock = 0;
				TNC->TCPCONNECTED = FALSE;
			}
		}
		TNC->Timeout = 20;				// 2 secs
		return 1;
	}
	if (TNC->hDevice)
		return (WriteCommBlock(TNC));
	else
	{
		TNC->Timeout = 20;				// 2 secs
		return 0;
	}
}

// Teensy Combined ARDOP/AX.25 Support

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD
/*

VOID ARAXINIT(struct PORTCONTROL * PORT)
{
	char Msg[80] = "";
	
	memcpy(Msg, PORT->PORTDESCRIPTION, 30);
	sprintf(Msg, "%s\n", Msg);
		
	WritetoConsoleLocal(Msg);
}

VOID ARAXTX(struct PORTCONTROL * PORT, PMESSAGE Buffer)
{
	// KISS Encode and Queue to Host Mode KISS Queue

	UINT * TXMsg = NULL;			// KISS Message to queue to Hostmode KISS Queue
	UCHAR * TXPtr;
	int TXLen = 0;
	struct _LINKTABLE * ACKWORD = Buffer->Linkptr;
	struct _MESSAGE * Message = (struct _MESSAGE *)Buffer;
	UCHAR c;

	char * ptr1;
	int Len;

	struct TNCINFO * TNC = PORT->TNC;

	if (TNC && TNC->CONNECTED)		// Have a Host Session
		TXMsg = GetBuff();	// KISS Message to queue to Hostmode KISS Queue
	
	if (TXMsg == NULL)		// No Session or No buffers
	{
		// Reset any ACKMODE Timer and release buffer	C_Q_ADD(&TRACE_Q, Buffer);

		struct _LINKTABLE * LINK = Buffer->Linkptr;

		if (LINK)
		{
			if (LINK->L2TIMER)
				LINK->L2TIMER = LINK->L2TIME;

			Buffer->Linkptr = 0;	// CLEAR FLAG FROM BUFFER
		}
		C_Q_ADD(&TRACE_Q, Buffer);
		return;
	}

	TXPtr = (UCHAR *)&TXMsg[2];

	ptr1 = &Message->DEST[0];
	Len = Message->LENGTH - 7;
	*(TXPtr++) = FEND;

	if (ACKWORD)					// Frame Needs ACK
	{
		*TXPtr++ = 0x0c;			// ACK OPCODE 
		ACKWORD -= (UINT)LINKS;		// Con only send 16 bits, so use offset into LINKS
		*TXPtr++ = ACKWORD & 0xff;
		*TXPtr++ = (ACKWORD >> 8) &0xff;

		// have to reset flag so trace doesnt clear it

		Buffer->Linkptr = 0;
		TXLen = 4;struct _LINKTABLE *
	}
	else	
	{
		*TXPtr++ = 0;
		TXLen = 2;
	}

	while (Len--)
	{
		c = *(ptr1++);

		switch (c)
		{
		case FEND:
			(*TXPtr++) = FESC;
			(*TXPtr++) = TFEND;
			TXLen += 2;
			break;

		case FESC:
			(*TXPtr++) = FESC;
			(*TXPtr++) = TFESC;
			TXLen += 2;
			break;

		default:
			(*TXPtr++) = c;
			TXLen++;
		}
		if (TXLen > 250)
		{
			// Queue frame to KISS Channel and get another buffer
			// can take up to 256, but sometimes add 2 at a time
			TXMsg[1] = (int)(TXPtr - (UCHAR *)&TXMsg[2]);
		}
	}

	(*TXPtr++) = FEND;
	TXLen++;

	TXMsg[1] = TXLen;

	C_Q_ADD(&TNC->KISSTX_Q, TXMsg);

	// Pass buffer to trace routines

	C_Q_ADD(&TRACE_Q, Buffer);

}


VOID ARAXRX()
{
}


VOID ARAXTIMER()
{
}
	
VOID ARAXCLOSE()
{
}

BOOL ARAXTXCHECK()
{
	return 0;
}


#define DATABYTES 400000		// WAS 320000
extern UCHAR * NEXTFREEDATA;	// ADDRESS OF NEXT FREE BYTE in shared memory
extern UCHAR DATAAREA[DATABYTES];



VOID AddVirtualKISSPort(struct TNCINFO * TNC, int ARDOPPort, char * buf)
{
	// Adds a Virtual KISS port for simultaneous ARDOP and Packet on Teensy TNC
	// or ARDOP_PTC ovet a single host mode port. 

	// Not needed if using TCP interface as that  uses KISS over TCP		

	struct PORTCONTROL * PORTVEC=PORTTABLE;
	struct PORTCONTROL * PORT;
	int pl = sizeof(struct PORTCONTROL);
	int mh = MHENTRIES * sizeof(MHSTRUC);
	int space = (int)(&DATAAREA[DATABYTES] - NEXTFREEDATA);
	char Msg[64];
	unsigned char * ptr3;
	unsigned int3;
	int newPortNumber = 0;

	if (TNC->ARDOPCommsMode == 'T')			// TCP
		return;

	if (buf[12] == '=')
		newPortNumber = atoi(&buf[13]);

	if (space < (pl + mh))
	{
		WritetoConsoleLocal("Insufficient space to add ARDOP/Packet Port\n");
		return;
	}


	while (PORTVEC->PORTPOINTER)
	{
		PORTVEC=PORTVEC->PORTPOINTER;
	}
	
	// PORTVEC is now last port in chain
	
	ptr3 = NEXTFREEDATA;
		
	PORT = (struct PORTCONTROL *)ptr3;
	
	ptr3 += sizeof (struct PORTCONTROL);

	//	Round to word boundary (for ARM5 etc)

	int3 = (int)ptr3;
	int3 += 3;
	int3 &= 0xfffffffc;
	ptr3 = (UCHAR *)int3;

	PORTVEC->PORTPOINTER = PORT;		// Chain to previous last port

	if (newPortNumber == 0)
		newPortNumber = 32;;

	if (GetPortTableEntryFromPortNum(newPortNumber))
	{
		// Number in use

		// If user specified search up, if default search down

		if (newPortNumber == 32)
			while(newPortNumber && GetPortTableEntryFromPortNum(newPortNumber))		// Try next lower
				newPortNumber--;
		else
			while(newPortNumber < 32 && GetPortTableEntryFromPortNum(newPortNumber))		// Try next highest
				newPortNumber++;

	}

	if (newPortNumber == 0 || newPortNumber > 32)
	{
		WritetoConsoleLocal("No free Port Number to add ARDOP/Packet Port\n");
		return;
	}



	NUMBEROFPORTS++;

	PORT->PORTNUMBER = newPortNumber;
	PORT->PortSlot = PORTVEC->PortSlot + 1;
	
	sprintf(Msg, "Packet Port for ARDOP Port %d  ", ARDOPPort);
	memcpy(PORT->PORTDESCRIPTION, Msg, 30);

	PORT->TNC = TNC;
	TNC->VirtualPORT = PORT;		// Link TNC and PORT both ways
	PORT->PORTINITCODE = ARAXINIT;
	PORT->PORTTIMERCODE = ARAXTIMER;
	PORT->PORTRXROUTINE = ARAXRX;
	PORT->PORTTXROUTINE = ARAXTX;
	PORT->PORTCLOSECODE = ARAXCLOSE;
	PORT->PORTTXCHECKCODE = ARAXTXCHECK;

	// Default L2 Params

	PORT->PORTN2 = 5;
	PORT->PORTT1 = 5000/333;		// FRACK 5 secs

	// As we use IPoll we can set RESPTIME very long and FRACK short

	PORT->PORTT2 =  20000/333;		// RESPTIME
	PORT->PORTPACLEN = 128;
	PORT->PORTWINDOW = 1;

	//	ADD MH AREA IF NEEDED

	NEEDMH = 1;								// Include MH in Command List

	PORT->PORTMHEARD = (PMHSTRUC)ptr3;

	ptr3 += MHENTRIES * sizeof(MHSTRUC);
	
	//	Round to word boundary (for ARM5 etc)

	int3 = (int)ptr3;
	int3 += 3;
	int3 &= 0xfffffffc;
	ptr3 = (UCHAR *)int3;

	NEXTFREEDATA = ptr3;

	return;
}

int ConfigVirtualKISSPort(struct TNCINFO * TNC, char * Cmd)
{
	struct PORTCONTROL * PORT = TNC->VirtualPORT;
	void ** buffptr = GetBuff();
	char * Context;
	char * Param;
	char * Command;
	int Value;
	int Stream = 0;
	if (buffptr == NULL)
		return 0;
	
	if (PORT == NULL)
	{
		buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} Packet Mode nor Enabled\r");
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}

	_strupr(Cmd);

	Command = strtok_s(&Cmd[4], " \n\r", &Context);
	Param = strtok_s(NULL, " \n\r", &Context);

	if (Param)
		Value = atoi(Param);

	if (strcmp(Command, "PACLEN") == 0)
	{
		if (Param == NULL)
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s %d\r", Command, PORT->PORTPACLEN);
		else
		{
			if (Value > 0 && Value <= 256)
				PORT->PORTPACLEN = Value;

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s now %d\r", Command, PORT->PORTPACLEN);
		}
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}
	else if (strcmp(Command, "RETRIES") == 0)
	{
		if (Param == NULL)
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s %d\r", Command, PORT->PORTN2);
		else
		{
			if (Value > 0 && Value <= 16)
				PORT->PORTN2 = Value;

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s now %d\r", Command, PORT->PORTN2);
		}
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}
	else if (strcmp(Command, "WINDOW") == 0 || strcmp(Command, "MAXFRAME") == 0)
	{
		if (Param == NULL)
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s %d\r", Command, PORT->PORTWINDOW);
		else
		{
			if (Value > 0 && Value <= 7)
				PORT->PORTWINDOW = Value;

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s now %d\r", Command, PORT->PORTWINDOW);
		}
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}
	else if (strcmp(Command, "FRACK") == 0)
	{
		if (Param == NULL)
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s %d mS\r", Command, PORT->PORTT1 * 333);
		else
		{
			if (Value > 0 && Value <= 20000)
				PORT->PORTT1 = Value / 333;

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s now %d mS\r", Command, PORT->PORTT1 * 333);
		}
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}
	else if (strcmp(Command, "RESPTIME") == 0)
	{
		if (Param == NULL)
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s %d mS\r", Command, PORT->PORTT2 * 333);
		else
		{
			if (Value > 0 && Value <= 20000)
				PORT->PORTT2 = Value / 333;

			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} PAC %s now %d mS\r", Command, PORT->PORTT2 * 333);
		}
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		return 0;
	}
	else
		buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} Invalid Command %s\r", Cmd);


	C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
	return 0;


}
*/
void ProcessKISSBytes(struct TNCINFO * TNC, UCHAR * Data, int Len)
{
	// Kiss data received from TNC but not necessarrily a full packet
	// and could be multiple packets

	// The TNC record is for the ARDOP Port, but we need to queue data 
	// to the linked Virtual Packet Port

	struct PORTCONTROL * PORT = TNC->VirtualPORT;
	UCHAR * KISSBuffer = TNC->KISSBuffer;
	UCHAR c;
	UCHAR * inptr = Data;
	int outptr = TNC->KISSInputLen;

	if (PORT == NULL)
		return;

	while(Len--)
	{
		c = *(inptr++);

		if (TNC->ESCFLAG)
		{
			//
			//	FESC received - next should be TFESC or TFEND

			TNC->ESCFLAG = FALSE;

			if (c == TFESC)
				c = FESC;
	
			if (c == TFEND)
				c = FEND;
		}
		else
		{
			switch (c)
			{
			case FEND:		
	
				//
				//	Either start of message or message complete
				//
				
				if (outptr == 0)
				{
					// Start of Message. If polling, extend timeout

					continue;
				}

				ProcessKISSPacket(TNC, KISSBuffer, outptr);
				outptr = 0;
				return;

			case FESC:
		
				TNC->ESCFLAG = TRUE;
				continue;

			}
		}
		
		//
		//	Ok, a normal char
		//

		KISSBuffer[outptr++] = c;
	}

	if (outptr > 510)
		outptr = 0;			// Protect Buffer
	
	TNC->KISSInputLen = outptr;
	
 	return;
}

void ProcessKISSPacket(struct TNCINFO * TNC, UCHAR * KISSBuffer, int Len)
{
	if (KISSBuffer[0] == 0x0c)		// ACK Frame
	{
		//	ACK FRAME - reset link timer

		struct _LINKTABLE * LINK;
		UINT ACKWORD = KISSBuffer[1] | KISSBuffer[2] << 8;

		LINK = LINKS + ACKWORD;

		if (LINK->L2TIMER)
			LINK->L2TIMER = LINK->L2TIME;

		return;
	}
	if (KISSBuffer[0] == 0)		// Data Frame
	{
		PDATAMESSAGE Buffer = (PDATAMESSAGE)GetBuff();
		
		if (Buffer)
		{
			memcpy(&Buffer->PID, &KISSBuffer[1], --Len);
			Len += sizeof(void *) + 3;

			PutLengthinBuffer(Buffer, Len);

			C_Q_ADD(&TNC->VirtualPORT->PORTRX_Q, (UINT *)Buffer);
		}
	}
}
