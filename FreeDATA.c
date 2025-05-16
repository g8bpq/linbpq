﻿/*
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
//	Interface to allow G8BPQ switch to use  FreeData TNC 


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifndef WIN32
#ifndef MACBPQ
#include <sys/ioctl.h>
#endif
#endif

#include "cheaders.h"
#include "bpq32.h"
#include "tncinfo.h"

#define SD_BOTH         0x02

#define FREEDATABUFLEN 16384				// TCP buffer size

int KillTNC(struct TNCINFO * TNC);
static int RestartTNC(struct TNCINFO * TNC);

extern int (WINAPI FAR *GetModuleFileNameExPtr)();
extern int (WINAPI FAR *EnumProcessesPtr)();
static int Socket_Data(int sock, int error, int eventcode);
VOID MoveWindows(struct TNCINFO * TNC);
static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
VOID SendInitScript(struct TNCINFO * TNC);
int ProcessEscape(UCHAR * TXMsg);
static void SendPoll(struct TNCINFO * TNC);
void SendMode(struct TNCINFO * TNC);
static int ConnecttoFreeData(int port);
void ConnectTNCPort(struct TNCINFO * TNC);
int FreeDataSendCommand(struct TNCINFO * TNC, char * data);
static void SendPing(struct TNCINFO * TNC, char * Call);
static void SendCQ(struct TNCINFO * TNC);
char * stristr (char *ch1, char *ch2);
int zEncode(unsigned char * in, unsigned char * out, int len, unsigned char * Banned);
static void SendDataMsg(struct TNCINFO * TNC, char * Call, char * Msg, int Len);
static int SendAsRaw(struct TNCINFO * TNC, char * Call, char * myCall, char * Msg, int Len);
static int SendAsFile(struct TNCINFO * TNC, char * Call, char * Msg, int Len);
char * byte_base64_encode(char *str, int len);
void xdecodeblock( unsigned char in[4], unsigned char out[3] );
void FlushData(struct TNCINFO * TNC);
void CountRestarts(struct TNCINFO * TNC);
void StopTNC(struct TNCINFO * TNC);
int FreeDataConnect(struct TNCINFO * TNC, char * Call);
int FreeDataDisconnect(struct TNCINFO * TNC);
int FreeGetData(struct TNCINFO * TNC);
static void SendBeacon(struct TNCINFO * TNC, int Interval);
void buildParamString(struct TNCINFO * TNC, char * line);
VOID FreeDataSuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC);
VOID FreeDataReleasePort(struct TNCINFO * TNC);


static char ClassName[]="FREEDATASTATUS";
static char WindowTitle[] = "FreeData Modem";
static int RigControlRow = 205;

#ifndef WIN32
#include <netinet/tcp.h>
#define MAXPNAMELEN 32 
#else
#include <commctrl.h>
#endif


extern int SemHeldByAPI;

static RECT Rect;

static int ProcessLine(char * buf, int Port);

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

#define MAXRXSIZE 512000		// Sets max size for file transfer (less base64 overhead

int CaptureCount = 0;
int PlaybackCount = 0;

int CaptureIndex = -1;		// Card number
int PlayBackIndex = -1;



char CaptureNames[16][MAXPNAMELEN + 2] = { "" };
char PlaybackNames[16][MAXPNAMELEN + 2] = { "" };


#ifdef WIN32

#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 24000, 2, 16, 0 };

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;


char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

#endif


char * gen_uuid()
{
    char v[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	int i;

    //3fb17ebc-bc38-4939-bc8b-74f2443281d4
    //8 dash 4 dash 4 dash 4 dash 12

    static char buf[37] = {0};

    //gen random for all spaces because lazy
    for (i = 0; i < 36; ++i)
	{
        buf[i] = v[rand()%16];
    }

    //put dashes in place
    buf[8] = '-';
    buf[13] = '-';
    buf[18] = '-';
    buf[23] = '-';

    //needs end byte
    buf[36] = '\0';

    return buf;
}

static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC = TNCInfo[Port];
	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if (ptr == NULL) return (TRUE);

	if (*ptr =='#') return (TRUE);			// comment

	if (*ptr ==';') return (TRUE);			// comment


	if (_stricmp(buf, "ADDR"))
		return FALSE;						// Must start with ADDR

	ptr = strtok(NULL, " \t\n\r");

	BPQport = Port;
	p_ipad = ptr;

	TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->FreeDataInfo = zalloc(sizeof(struct FreeDataINFO));

//	TNC->FreeDataInfo->useBaseCall = 1;		// Default

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;

	TNC->MaxConReq = 10;		// Default
	
	if (p_ipad == NULL)
		p_ipad = strtok(NULL, " \t\n\r");

	if (p_ipad == NULL) return (FALSE);
	
	p_port = strtok(NULL, " \t\n\r");
			
	if (p_port == NULL) return (FALSE);

	WINMORport = atoi(p_port);

	TNC->TCPPort = WINMORport;

	TNC->Datadestaddr.sin_family = AF_INET;
	TNC->Datadestaddr.sin_port = htons(WINMORport);

	TNC->HostName = malloc(strlen(p_ipad)+1);

	if (TNC->HostName == NULL) return TRUE;

	strcpy(TNC->HostName,p_ipad);

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

		if (_memicmp(buf, "CAPTURE", 7) == 0)
		{
			TNC->FreeDataInfo->Capture = _strupr(_strdup(&buf[8]));
			strlop(TNC->FreeDataInfo->Capture, 13);
		}
		else if (_memicmp(buf, "PLAYBACK", 8) == 0)
		{
			TNC->FreeDataInfo->Playback = _strupr(_strdup(&buf[9]));
			strlop(TNC->FreeDataInfo->Playback, 13);
		}

		else if (_memicmp(buf, "LOGDIR ", 7) == 0)
			TNC->LogPath = _strdup(&buf[7]);

		else if (_memicmp(buf, "HAMLIBHOST", 10) == 0)
		{
			TNC->FreeDataInfo->hamlibHost = _strdup(&buf[11]);
			strlop(TNC->FreeDataInfo->hamlibHost, 13);
		}
		
		else if (_memicmp(buf, "TuningRange", 11) == 0)
			TNC->FreeDataInfo->TuningRange = atoi(&buf[12]);
		
		else if (_memicmp(buf, "TXLevel", 6) == 0)
			TNC->FreeDataInfo->TXLevel = atoi(&buf[7]);

		else if (_memicmp(buf, "Explorer", 8) == 0)
			TNC->FreeDataInfo->Explorer = atoi(&buf[9]);
	
		
		else if (_memicmp(buf, "LimitBandWidth", 14) == 0)
			TNC->FreeDataInfo->LimitBandWidth = atoi(&buf[15]);
		
		else if (_memicmp(buf, "HAMLIBPORT", 10) == 0)
			TNC->FreeDataInfo->hamlibPort = atoi(&buf[11]);

		else if (_memicmp(buf, "USEBASECALL", 11) == 0)
			TNC->FreeDataInfo->useBaseCall = atoi(&buf[12]);

		else if (_memicmp(buf, "RXDIRECTORY", 11) == 0)
		{
			TNC->FreeDataInfo->RXDir = _strdup(&buf[12]);
			strlop(TNC->FreeDataInfo->RXDir, 13);
		}

		else if (standardParams(TNC, buf) == FALSE)
			strcat(TNC->InitScript, buf);	

	}

	return (TRUE);	
}

char * Config;
static char * ptr1, * ptr2;

int FreeDataGetLine(char * buf)
{
loop:

	if (ptr2 == NULL)
		return 0;

	memcpy(buf, ptr1, ptr2 - ptr1 + 2);
	buf[ptr2 - ptr1 + 2] = 0;
	ptr1 = ptr2 + 2;
	ptr2 = strchr(ptr1, 13);

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}


VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);



static time_t ltime;



static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen)
{
	if (TNC->hDevice)
	{
		// FreeData mode. Queue to Hostmode driver
		
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;

		return;
	}
}


VOID FreeDataChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

	datalen = sprintf(TXMsg, "MYCALL %s\r", Call);
//	FreeDataSendCommand(TNC, TXMsg);
}

static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	int datalen;
	PMSGWITHLEN buffptr;
//	char txbuff[500];
	unsigned int txlen = 0;
	UCHAR * TXMsg;

	size_t Param;
	int Stream = 0;
	HKEY hKey=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct ScanEntry * Scan;

	if (TNC == NULL)
		return 0;							// Port not defined

	switch (fn)
	{
		case 7:			

		// 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		// G7TAJ's code to record activity for stats display
			
		if ( TNC->BusyFlags && CDBusy )
			TNC->PortRecord->PORTCONTROL.ACTIVE += 2;

		if ( TNC->PTTState )
			TNC->PortRecord->PORTCONTROL.SENDING += 2;
		
		SendPoll(TNC);

			// Check for buffered data to send

			if (TNC->FreeDataInfo->toSendTimeout)
			{
				TNC->FreeDataInfo->toSendTimeout--;
				if (TNC->FreeDataInfo->toSendTimeout <= 0)
					FlushData(TNC);
			}

			return 0;

		case 1:				// poll

//		FreeDataCheckRX(TNC);

		if (TNC->TNCCONNECTED == FALSE && TNC->TNCCONNECTING == FALSE)
		{
			//	See if time to reconnect
		
			time(&ltime);
			if (ltime - TNC->lasttime > 19 )
			{
				TNC->lasttime = ltime;
				ConnecttoFreeData(port);
			}
			while (TNC->PortRecord->UI_Q)
			{
				buffptr = Q_REM(&TNC->PortRecord->UI_Q);
				ReleaseBuffer(buffptr);	
			}
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

/*			if (TNC->CONNECTED == 0 ||
				TNC->Streams[0].Connecting ||
				TNC->Streams[0].Connected)
			{
				// discard if TNC not connected or sesison active

				ReleaseBuffer(buffptr);
				continue;
			}
*/	
			datalen = buffptr->LENGTH - MSGHDDRLEN;
			Buffer = &buffptr->DEST[0];		// Raw Frame
			Buffer[datalen] = 0;

			// Frame has ax.25 format header. Convert to Text

			CallLen = ConvFromAX25(Buffer + 7, Call);		// Origin
			memcpy(ptr, Call, CallLen);
			ptr += CallLen;

			*ptr++ = '!';

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

			*ptr++ = '_';
			*ptr++ = 'U';					// UI Frame
			*ptr++ = 0;						// delimit calls

			if (Buffer[0] == 3)				// UI
			{
				Buffer += 2;
				datalen -= 2;
			}

//			FreeDataSendSingleData(TNC, FECMsg, Buffer, datalen);

			ReleaseBuffer(buffptr);
		}
	
	
		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC
			}
		}


		for (Stream = 0; Stream <= 2; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			if (STREAM->NeedDisc)
			{
				STREAM->NeedDisc--;

				if (STREAM->NeedDisc == 0)
				{
					// Send the DISCONNECT

					FreeDataDisconnect(TNC);
					strcpy(TNC->WEB_TNCSTATE, "Disconnecting");
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

				}
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				Debugprintf("FreeData New Attach Stream %d", Stream);
			
				STREAM->Attached = TRUE;
			
				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, STREAM->MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;
					
				// Stop other ports in same group

				SuspendOtherPorts(TNC);

				// Stop Listening, and set MYCALL to user's call

				FreeDataSuspendPort(TNC, TNC);	
				FreeDataChangeMYC(TNC, TNC->Streams[0].MyCall);
				TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

				sprintf(TNC->WEB_TNCSTATE, "In Use by %s", STREAM->MyCall);
				MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

					// Stop Scanning

				sprintf(Msg, "%d SCANSTOP", TNC->Port);
	
				Rig_Command( (TRANSPORTENTRY *) -1, Msg);
			}
				
			if (STREAM->Attached)
				CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

		}
				
		// See if any frames for this port

		for (Stream = 0; Stream <= 2; Stream++)
		{
			STREAM = &TNC->Streams[Stream];
			
			if (STREAM->BPQtoPACTOR_Q)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)Q_REM(&STREAM->BPQtoPACTOR_Q);
				UCHAR * data = &buffptr->Data[0];
				STREAM->FramesQueued--;
				txlen = (int)buffptr->Len;

				SendAsFile(TNC, TNC->FreeDataInfo->farCall, data, txlen);
			}

			if (STREAM->PACTORtoBPQ_Q != 0)
			{
				buffptr = (PMSGWITHLEN)Q_REM(&STREAM->PACTORtoBPQ_Q);

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

		if (!TNC->TNCCONNECTED)
		{
			// Send Error Response

			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr->Len = sprintf(&buffptr->Data[0], "No Connection to TNC\r");

			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		STREAM = &TNC->Streams[Stream];
		
		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}

		txlen = GetLengthfromBuffer(buff) - (MSGHDDRLEN + 1);		// 1 as no PID
		TXMsg = &buff->L2DATA[0];
		TXMsg[txlen] = 0;

		if (STREAM->Connected)
		{
			STREAM->PacketsSent++;

			SendDataMsg(TNC, TNC->FreeDataInfo->farCall, buff->L2DATA, txlen);	
			return 1;
		}

		if (TNC->FreeDataInfo->Chat)
		{
			// Chat Mode - Send to other end

			char reply[512] = "m";
			char * p;
			int Len;

			if (_stricmp(TXMsg, "/ex\r") == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
				TNC->FreeDataInfo->Chat = 0;

				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Chat with %s ended. \r", TNC->FreeDataInfo->ChatCall);
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}

				return 0;
			}

			// Send as chat message

			//m�;send_message�;123�;64730c5c-d32c-47b4-9b11-c958fd07a185�;hhhhhhhhhhhhhhhhhh
			//�;�;plain/text�;

			strlop(TXMsg, 13);

			strcpy(&reply[2], ";send_message");
			strcpy(&reply[16], ";123");
			reply[21] = ';';
			strcpy(&reply[22], gen_uuid());
			sprintf(&reply[59], ";%s\n", TXMsg);

			p = strchr(&reply[59], 0);

			p[1] = ';';
			strcpy(&p[3], ";plain/text");
			p[15] = ';';

			Len = &p[16] - reply;

			SendAsRaw(TNC, TNC->FreeDataInfo->ChatCall, "", reply, Len);

			return 0;
		}



		if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0 || _memicmp(&buff->L2DATA[0], "BYE\r", 4) == 0)
		{
			STREAM->ReportDISC = TRUE;		// Tell Node
			TNC->FreeDataInfo->Chat = 0;
			return 0;
		}


		// See if Local command (eg RADIO)

		if (_memicmp(&buff->L2DATA[0], "RADIO ", 6) == 0)
		{
			char cmd[56];

			strcpy(cmd, &buff->L2DATA[6]);
			sprintf(&buff->L2DATA[0], "%d %s", TNC->Port, cmd);

			if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK, &buff->L2DATA[0]))
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

		if (_memicmp(&buff->L2DATA[0], "OVERRIDEBUSY", 12) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			TNC->OverrideBusy = TRUE;

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} OK\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;
		}

		if (_memicmp(&buff->L2DATA[0], "PING ", 5) == 0)
		{
			char * Call = &buff->L2DATA[5];
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			strlop(Call, 13);
			strlop(Call, ' ');
			SendPing(TNC, _strupr(Call));

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Ping Sent\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;

		}

		if (_memicmp(&buff->L2DATA[0], "CQ", 2) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			SendCQ(TNC);

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} CQ Sent\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;
		}

		if (_memicmp(&buff->L2DATA[0], "TXLEVEL ", 8) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
			int Level = atoi(&buff->L2DATA[8]);
			char TXL[] = "{\"type\" : \"set\", \"command\" : \"tx_audio_level\", \"value\": \"%d\"}\n";
			char Message[256];
			int Len, ret;

			Len = sprintf(Message, TXL, Level);
			ret = send(TNC->TCPDataSock, (char *)&Message, Len, 0);
			
			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} TXLevel Set\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}
			return 0;
		}

		if (_memicmp(&buff->L2DATA[0], "SENDTEST", 8) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
			char TXF[] = "{\"type\" : \"set\", \"command\" : \"send_test_frame\"}\n";
			char Message[256];
			int Len, ret;

			Len = sprintf(Message, "%s", TXF);
			ret = send(TNC->TCPDataSock, (char *)&Message, Len, 0);
			
			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Test Frame Requested\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}
			return 0;
		}

		if (_memicmp(&buff->L2DATA[0], "CHAT ", 5) == 0)
		{
			char * Call = &buff->L2DATA[5];
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			strlop(Call, 13);
			strlop(Call, ' ');

			TNC->FreeDataInfo->Chat = 1;
			memcpy(TNC->FreeDataInfo->ChatCall, _strupr(Call), 10);

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Chat with %s. Enter /ex to exit\r", Call);
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;
		}


		if (_memicmp(&TXMsg[0], "BEACON ", 7) == 0)
		{
			int Interval = atoi(&TXMsg[7]);

			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			SendBeacon(TNC, Interval);

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Ok\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;
		}


		// See if a Connect Command.

		if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
		{
			char Connect[80];
			char loppedCall[10];
			
			char * ptr = strchr(&buff->L2DATA[2], 13);

			if (ptr)
				*ptr = 0;

			// FreeDATA doesn't have the concept of a connection, so need to simulate it between the nodes
			_strupr(&buff->L2DATA[2]);

			if (strlen(&buff->L2DATA[2]) > 9)
				buff->L2DATA[11] = 0;

			strcpy(TNC->FreeDataInfo->toCall, &buff->L2DATA[2]);
			strcpy(loppedCall, TNC->FreeDataInfo->toCall);
			if (TNC->FreeDataInfo->useBaseCall)
				strlop(loppedCall, '-');
			strcpy(TNC->FreeDataInfo->farCall, loppedCall); 

			// MYCALL and Target call are end to end concepts - the TNC cam can only use one call, set at TNC start. and no SSID's
			// Messages are sent at TNC level to the tnc call, so we send our tnc call to the other end
	

			txlen = sprintf(Connect, "C %s %s %s ", &buff->L2DATA[2], STREAM->MyCall, TNC->FreeDataInfo->ourCall);

			// See if Busy
/*
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
*/
			TNC->OverrideBusy = FALSE;

			memset(STREAM->RemoteCall, 0, 10);
			strcpy(STREAM->RemoteCall, &buff->L2DATA[2]);
			STREAM->ConnectTime = time(NULL); 
			STREAM->bytesRXed = STREAM->bytesTXed = STREAM->PacketsSent = 0;

			sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", STREAM->MyCall, STREAM->RemoteCall);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
				
//			FreeDataSendCommand(TNC, Connect);
			FreeDataConnect(TNC, STREAM->RemoteCall);
			STREAM->Connecting = TRUE;
			return 0;

		}

		// Normal data. Send to TNC

		// The TNC doesn't have any commands, so send error message

		buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr)
		{
			buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "FreeData} Not connected\r");
			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		}

	//	strlop(buff->L2DATA, 13);
	//	txlen = SendDataMsg(TNC, TNC->, buff->L2DATA);
	//	FreeDataSendData(TNC, TXMsg, txlen);
	
		return 0;

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		Stream = (int)(size_t)buff;

		// FreeData TNC can buffer unlimited data 
	
		if (TNC->Streams[Stream].Attached == 0)
			return (TNC->TNCCONNECTED != 0) << 8 | 1;

		return ((TNC->TNCCONNECTED != 0) << 8 | TNC->Streams[Stream].Disconnecting << 15);		// OK
		
	case 5:				// Close

		StopTNC(TNC);

		// Drop through

	case 4:				// reinit7

		if (TNC->TCPDataSock)
		{
			shutdown(TNC->TCPDataSock, SD_BOTH);
			Sleep(100);
			closesocket(TNC->TCPDataSock);
		}

		printf("FreeData Closing %d\n", TNC->WeStartedTNC);

		if (TNC->WeStartedTNC)
		{
			printf("Calling KillTNC\n");
			KillTNC(TNC);
		}

		return 0;

	case 6:				// Scan Stop Interface

		Param = (size_t)buff;
	
		if (Param == 2)		// Check  Permission (Shouldn't happen)
		{
			Debugprintf("Scan Check Permission called on FreeDATA");
			return 1;		// OK to change
		}

		if (Param == 1)		// Request Permission
		{
			if (!TNC->CONNECTED)
				return 0;					// No connection so no interlock
			
			if (TNC->ConnectPending == 0 && TNC->PTTState == 0)
			{
				FreeDataSuspendPort(TNC, TNC);
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
					FreeDataReleasePort(TNC);
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;
		return 0;
	}
	return 0;
}

VOID FreeDataReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	FreeDataChangeMYC(TNC, TNC->NodeCall);

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);

	FreeDataReleasePort(TNC);
	ReleaseOtherPorts(TNC);

}

VOID FreeDataSuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC)
{
//	char CMD[] = "{\"type\" : \"set\", \"command\" : \"listen\", \"state\": \"False\"}\n";
//	send(TNC->TCPDataSock, CMD, strlen(CMD), 0);
}

VOID FreeDataReleasePort(struct TNCINFO * TNC)
{
	char CMD[] = "{\"type\" : \"set\", \"command\" : \"listen\", \"state\": \"True\"}\n";
	send(TNC->TCPDataSock, CMD, strlen(CMD), 0);
}


static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}</script>"
		"</head><title>FreDATA Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2><form method=post target=\"POPUPW\" onsubmit=\"POPUPW = window.open('about:blank','POPUPW',"
		"'width=440,height=150');\" action=ARDOPAbort?%d>FreeData Status"
		"<input name=Save value=\"Abort Session\" type=submit style=\"position: absolute; right: 20;\"></form></h2>",
		TNC->Port);


	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>TNC State</td><td>%s</td></tr>", TNC->WEB_TNCSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Mode</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "<tr><td>Channel State</td><td>%s</td></tr>", TNC->WEB_CHANSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Proto State</td><td>%s</td></tr>", TNC->WEB_PROTOSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Traffic</td><td>%s</td></tr>", TNC->WEB_TRAFFIC);
//	Len += sprintf(&Buff[Len], "<tr><td>TNC Restarts</td><td></td></tr>", TNC->WEB_RESTARTS);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}

#ifndef LINBPQ

#define BGCOLOUR RGB(236,233,216)
static HBRUSH RedBrush = NULL;
HBRUSH GreenBrush;
HBRUSH BlueBrush;
static HBRUSH bgBrush = NULL;

extern HWND ClientWnd, FrameWnd;
extern int OffsetH, OffsetW;

extern HMENU hMainFrameMenu;
extern HMENU hBaseMenu;
extern HANDLE hInstance;

extern HKEY REGTREE;


/*
static LRESULT CALLBACK PacWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	MINMAXINFO * mmi;
	PAINTSTRUCT ps;
	HDC hdc;

	int i;
	struct TNCINFO * TNC;

	HKEY hKey;
	char Key[80];
	int retCode, disp;

	for (i=0; i<41; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;
		
		if (TNC->hDlg == hWnd)
			break;
	}

	if (TNC == NULL)
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	switch (message) { 

	case WM_CREATE:

		break;

	case WM_PAINT:
			
		hdc = BeginPaint(hWnd, &ps);

//		SelectObject(ps.hdc, RedBrush); 
		SelectObject(ps.hdc, GreenBrush); 
//		SelectObject(ps.hdc, GetStockObject(GRAY_BRUSH)); 

		EndPaint(hWnd, &ps);
		break;        

	case WM_GETMINMAXINFO:

 		if (TNC->ClientHeight)
		{
			mmi = (MINMAXINFO *)lParam;
			mmi->ptMaxSize.x = TNC->ClientWidth;
			mmi->ptMaxSize.y = TNC->ClientHeight;
			mmi->ptMaxTrackSize.x = TNC->ClientWidth;
			mmi->ptMaxTrackSize.y = TNC->ClientHeight;
		}

		break;


	case WM_MDIACTIVATE:
	{
			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);

			if (TNC->hMenu)
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)TNC->hMenu, "Actions");
			
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);

//			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) TNC->hMenu, (LPARAM) TNC->hWndMenu);
		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		 }
			 
		// call DrawMenuBar after the menu items are set
		DrawMenuBar(FrameWnd);

		return DefMDIChildProc(hWnd, message, wParam, lParam);
	}



	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)TNC->hMenu)
		{
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, " TNC") || strstr(TNC->ProgramPath, "ARDOP") || strstr(TNC->ProgramPath, "VARA"))
				{
					EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_ENABLED);
		
					break;
				}
			}
			EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_GRAYED);
		}
			
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case WINMOR_KILL:

			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			KillTNC(TNC);
			RestartTNC(TNC);
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			TNC->RestartAfterFailure = !TNC->RestartAfterFailure;
			CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
			retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				RegSetValueEx(hKey,"TNC->RestartAfterFailure",0,REG_DWORD,(BYTE *)&TNC->RestartAfterFailure, 4);
				RegCloseKey(hKey);
			}
			break;

		case ARDOP_ABORT:

			if (TNC->ForcedCloseProc)
				TNC->ForcedCloseProc(TNC, 0);

			break;
		}
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SIZING:
	case WM_SIZE:

		MoveWindows(TNC);
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{ 

		case SC_RESTORE:

			TNC->Minimized = FALSE;
			break;

		case SC_MINIMIZE: 

			TNC->Minimized = TRUE;
			break;
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_CTLCOLORDLG:
		return (LONG)bgBrush;

		 
	case WM_CTLCOLORSTATIC:
	{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)bgBrush;
	}

	case WM_DESTROY:
		
		break;
	}
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}
*/

#endif



VOID * FreeDataExtInit(EXTPORTDATA * PortEntry)
{
	int port;
	char Msg[255];
	char * ptr;
	struct TNCINFO * TNC;
	char * TempScript;
	u_long param = 1;
	int line;
	int i;
	int n = 0;

	srand(time(NULL));
	
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

	if (TNC->FreeDataInfo->TuningRange == 0)
		TNC->FreeDataInfo->TuningRange = 50;

	if (TNC->FreeDataInfo->TXLevel == 0)
			TNC->FreeDataInfo->TXLevel = 50;

	if (TNC->AutoStartDelay == 0)
		TNC->AutoStartDelay = 2000;

#ifndef LINBPQ

	if (bgBrush == NULL)
	{
		bgBrush = CreateSolidBrush(BGCOLOUR);
		RedBrush = CreateSolidBrush(RGB(255,0,0));
		GreenBrush = CreateSolidBrush(RGB(0,255,0));
		BlueBrush = CreateSolidBrush(RGB(0,0,255));
	}

#endif

	Consoleprintf("FreeData Host %s %d", TNC->HostName, TNC->TCPPort);

	TNC->Port = port;
	TNC->PortRecord = PortEntry;
	TNC->PortRecord->PORTCONTROL.HWType = TNC->Hardware = H_FREEDATA;

	TNC->WeStartedTNC = 1;

	TNC->ARDOPDataBuffer = malloc(MAXRXSIZE);
	TNC->ARDOPBuffer = malloc(FREEDATABUFLEN);


	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	memcpy(TNC->FreeDataInfo->ourCall, TNC->NodeCall, 10);
	strlop(TNC->FreeDataInfo->ourCall, ' ');
	if (TNC->FreeDataInfo->useBaseCall)
		strlop(TNC->FreeDataInfo->ourCall, '-');

	if (PortEntry->PORTCONTROL.PORTINTERLOCK && TNC->RXRadio == 0 && TNC->TXRadio == 0)
		TNC->RXRadio = TNC->TXRadio = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	if (TNC->PacketChannels > 1)
		TNC->PacketChannels = 1;

	PortEntry->MAXHOSTMODESESSIONS = TNC->PacketChannels + 1;

	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only
	PortEntry->PERMITGATEWAY = TRUE;				// Can change ax.25 call on each stream

	PortEntry->PORTCONTROL.UICAPABLE = TRUE;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = FreeDataSuspendPort;
	TNC->ReleasePortProc = FreeDataReleasePort;

//	PortEntry->PORTCONTROL.PORTSTARTCODE = KAMStartPort;
//	PortEntry->PORTCONTROL.PORTSTOPCODE = KAMStopPort;


	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = zalloc(1000);

	// cant think of any yet

	if (TNC->InitScript)
	{
		strcat(TempScript, TNC->InitScript);
		free(TNC->InitScript);
	}

	TNC->InitScript = TempScript;

	// Set MYCALL

	sprintf(Msg, "MYCALL %s\r", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);

	strcpy(TNC->CurrentMYC, TNC->NodeCall);

	if (TNC->WL2K == NULL)
		if (PortEntry->PORTCONTROL.WL2KInfo.RMSCall[0])			// Already decoded
			TNC->WL2K = &PortEntry->PORTCONTROL.WL2KInfo;

	PortEntry->PORTCONTROL.TNC = TNC;

	// Build SSID List

	if (TNC->LISTENCALLS)
	{		
		strcpy(TNC->FreeDataInfo->SSIDList, TNC->LISTENCALLS);
	}
	else
	{
		APPLCALLS * APPL;
		char Appl[11] = "";
		char * List = TNC->FreeDataInfo->SSIDList;
		char * SSIDptr;
		int SSID;
		int Listptr;

		// list is a set of numbers separated by spaces eg 0 2 10 

		SSIDptr = strchr(TNC->NodeCall, '-');
		if (SSIDptr)
			SSID = atoi(SSIDptr + 1);
		else
			SSID = 0;

		Listptr = sprintf(List, "%d", SSID);

		for (i = 0; i < 32; i++)
		{
			APPL=&APPLCALLTABLE[i];

			if (APPL->APPLCALL_TEXT[0] > ' ')
			{
				memcpy(Appl, APPL->APPLCALL_TEXT, 10);

				SSIDptr = strchr(Appl, '-');
				if (SSIDptr)
					SSID = atoi(SSIDptr + 1);
				else
					SSID = 0;

				Listptr += sprintf(&List[Listptr], " %d", SSID);
			}
		}
		List[Listptr] = 0;
	}
	
	// Build SSID List for Linux

	TNC->FreeDataInfo->SSIDS[n] = _strdup(TNC->FreeDataInfo->SSIDList);

	while (TNC->FreeDataInfo->SSIDS[n]) 
	{
		TNC->FreeDataInfo->SSIDS[n+1] = strlop(TNC->FreeDataInfo->SSIDS[n], ' ');
		n++;
	}

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

#ifndef LINBPQ

	line = 6;

	if (TNC->TXFreq)
	{
		CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow + 22, PacWndProc, 500, 450, ForcedClose);

		InitCommonControls(); // loads common control's DLL 

		CreateWindowEx(0, "STATIC", "TX Tune", WS_CHILD | WS_VISIBLE, 10,line,120,20, TNC->hDlg, NULL, hInstance, NULL);
		TNC->xIDC_TXTUNE = CreateWindowEx(0, TRACKBAR_CLASS, "", WS_CHILD | WS_VISIBLE, 116,line,200,20, TNC->hDlg, NULL, hInstance, NULL);
		TNC->xIDC_TXTUNEVAL = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE, 320,line,30,20, TNC->hDlg, NULL, hInstance, NULL);

		SendMessage(TNC->xIDC_TXTUNE, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(-200, 200));  // min. & max. positions

		line += 22;
	}
	else
		CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450, ForcedClose);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,line,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,450,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,line,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,520,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,line,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,200,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,line,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,144,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "Proto State", WS_CHILD | WS_VISIBLE,10,line,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,116,line,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,line,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,116,line,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,line,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,line,20,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,line,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTTIME = CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,line,200,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
		LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
		0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill Freedata TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart Freedata TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after failed Connection");	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);
	AppendMenu(TNC->hMenu, MF_STRING, ARDOP_ABORT, "Abort Current Session");

	MoveWindows(TNC);
#endif

	strcpy(TNC->WEB_CHANSTATE, "Idle");
	SetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);

	// Convert sound card name to index

#ifdef WIN32

	if (CaptureDevices == NULL)				// DOn't do it again if more that one port
	{
		CaptureCount = waveInGetNumDevs();

		CaptureDevices = malloc((MAXPNAMELEN + 2) * CaptureCount);
		CaptureDevices[0] = 0;

		printf("Capture Devices");

		for (i = 0; i < CaptureCount; i++)
		{
			waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
			waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));

			if (CaptureDevices)
				strcat(CaptureDevices, ",");
			strcat(CaptureDevices, pwic.szPname);
			Debugprintf("%d %s", i + 1, pwic.szPname);
			memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
			_strupr(&CaptureNames[i][0]);
		}

		PlaybackCount = waveOutGetNumDevs();

		PlaybackDevices = malloc((MAXPNAMELEN + 2) * PlaybackCount);
		PlaybackDevices[0] = 0;

		Debugprintf("Playback Devices");

		for (i = 0; i < PlaybackCount; i++)
		{
			waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
			waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

			if (PlaybackDevices[0])
				strcat(PlaybackDevices, ",");
			strcat(PlaybackDevices, pwoc.szPname);
			Debugprintf("%i %s", i + 1, pwoc.szPname);
			memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
			_strupr(&PlaybackNames[i][0]);
			waveOutClose(hWaveOut);
		}
	}

#endif

	time(&TNC->lasttime);			// Get initial time value

	ConnecttoFreeData(port);
	return ExtProc;
}


VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// We don't get data acks, so can't check for bytes outstanding
	
	FreeDataDisconnect(TNC);
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	FreeDataDisconnect(TNC);
}



VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
	{
		FreeDataReleaseTNC(TNC);
	}
}

VOID FreeDataAbort(struct TNCINFO * TNC)
{
	FreeDataSendCommand(TNC, "ABORT\r");
}

// Host Mode Stuff (we reuse some routines in SCSPactor)

VOID FreeDataDoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		return;
	}

	if (TNC->ReinitState == 1)
	{
		// Forcing back to Term Mode

		TNC->ReinitState = 0;
		return;
	}

	if (TNC->ReinitState == 3)
	{
		return;
	}
}

static RECT Rect1 = {30, 160, 400, 195};

int zEncode(unsigned char * in, unsigned char * out, int len, unsigned char * Banned)
{
	// Replace forbidden chars with =xx

	unsigned char * ptr = out;
	unsigned char c;

	while (len--)
	{
		c = *(in++);

		if (strchr(&Banned[0], c))
		{
			*(out++) = '=';
			*(out++) = (c >> 4) + 'A';
			*(out++) = (c & 15) + 'A';
		}
		else
			*(out++) = c;
	}

	return (out - ptr);
}



VOID FreeDataProcessTNCMessage(struct TNCINFO * TNC, char * Call, unsigned char * Msg, int Len)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct FreeDataINFO * Modem = TNC->FreeDataInfo;
	char * toCall, * fromCall, * tncCall, *Context;
	char * ptr;
	char a, b;
	unsigned char axcall[7];
	char AppName[13] = "";
	APPLCALLS * APPL;
	char * ApplPtr = APPLS;
	int App;
	char Appl[10];
	struct WL2KInfo * WL2K = TNC->WL2K;
	TRANSPORTENTRY * SESS;

	// First Byte of Message is Type. Messages can be commands or short (<120) data packets
	// Data is encoded with =xx replacing restricted chars

	Msg[Len] = 0;

	switch (Msg[0])
	{
	case 'C':

		// Connect Request. C G8BPQ-10 GM8BPQ-2 (Target, Origin) 

		toCall = strtok_s(&Msg[2], " ", &Context);
		fromCall = strtok_s(NULL, " ", &Context);
		tncCall = strtok_s(NULL, " ", &Context);

		strcpy(TNC->FreeDataInfo->farCall, tncCall);

		ConvToAX25Ex(fromCall, axcall);		// Allow -T and -R SSID's for MPS

		// Check for ExcludeList

		if (ExcludeList[0])
		{
			if (CheckExcludeList(axcall) == FALSE)
			{
				Debugprintf("FreeData Call from %s rejected", fromCall);

				// Send 'd'

				Sleep(1000);
				FreeDataDisconnect(TNC);
				return;	
			}
		}

		//	IF WE HAVE A PERMITTED CALLS LIST, SEE IF HE IS IN IT

		if (TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS)
		{
			UCHAR * ptr = TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS;

			while (TRUE)
			{
				if (memcmp(axcall, ptr, 6) == 0)	// Ignore SSID
					break;

				ptr += 7;

				if ((*ptr) == 0)							// Not in list
				{
					Sleep(1000);
					FreeDataSendCommand(TNC, "d");

					Debugprintf("FreeDara Call from %s not in ValidCalls - rejected", Call);
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

			if (_stricmp(toCall, Appl) == 0)
				break;
		}

		if (App < 32)
		{

			memcpy(AppName, &ApplPtr[App * sizeof(struct CMDX)], 12);
			AppName[12] = 0;

			// if SendTandRtoRelay set and Appl is RMS change to RELAY

			if (TNC->SendTandRtoRelay && memcmp(AppName, "RMS ", 4) == 0
				&& (strstr(Call, "-T" ) || strstr(Call, "-R")))
				strcpy(AppName, "RELAY       ");

			// Make sure app is available

			if (!CheckAppl(TNC, AppName))
			{
				Sleep(1000);
				FreeDataSendCommand(TNC, "dApplication not Available");
				return;
			}
		}

		ProcessIncommingConnectEx(TNC, fromCall, 0, TRUE, TRUE);
		SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

		// if connect to an application, send command

		if (AppName[0])
		{
			buffptr = GetBuff();

			if (buffptr)
			{
				buffptr->Len = sprintf(buffptr->Data, "%s\r", AppName);
				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				TNC->SwallowSignon = TRUE;
			}
		}

		TNC->ConnectPending = FALSE;

		if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, toCall, TNC->RIG->Valchar);
			SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
			if (SESS->Frequency == 1500)
			{
				// try to get from WL2K record

				if (WL2K)
				{
					SESS->Frequency = WL2K->Freq;
				}
			}
			SESS->Mode = TNC->WL2KMode;
		}
		else
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, toCall);
			if (WL2K)
			{
				SESS->Frequency = WL2K->Freq;
				SESS->Mode = WL2K->mode;
			}
		}

		if (WL2K)
			strcpy(SESS->RMSCall, WL2K->RMSCall);

		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

		STREAM->ConnectTime = time(NULL); 
		STREAM->bytesRXed = STREAM->bytesTXed = STREAM->PacketsSent = 0;
		STREAM->Connected = TRUE;

		// Send Connect ACK
		Sleep(1000);
		FreeDataSendCommand(TNC, "c");
		return;

	case 'c':

		// Connect ACK

		sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s", STREAM->MyCall, STREAM->RemoteCall);
		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
		STREAM->Connected = TRUE;
		STREAM->Connecting = FALSE;

		buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = sprintf(buffptr->Data, "*** Connected to %s\r", TNC->FreeDataInfo->toCall);
		
		C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
		STREAM->FramesQueued++;

		return;

	case 'D':

		// Disconnect Command

		FreeDataSendCommand(TNC, "d");
	
		// Drop through to disconnect this end

	case 'd':

		// Disconnect complete (response to sending "D")
		// Or connect refused in response to "C"

		if (STREAM->Connecting)
		{
			// Connection Refused - If there is a message, pass to appl

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", STREAM->MyCall);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			
			STREAM->Connecting = FALSE;
			buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0) return;			// No buffers, so ignore

			if (Msg[1])
				buffptr->Len = sprintf(buffptr->Data, "Connect Rejected - %s\r", &Msg[1]);
			else	
				buffptr->Len = sprintf(buffptr->Data, "Connect Rejected\r");

			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			STREAM->FramesQueued++;
			return;
		}

		// Release Session

		if (STREAM->Connected)
		{
			// Create a traffic record
		
			hookL4SessionDeleted(TNC, STREAM);
		}

		STREAM->Connected = FALSE;		// Back to Command Mode
		STREAM->ReportDISC = TRUE;		// Tell Node
		STREAM->Disconnecting = FALSE;

		strcpy(TNC->WEB_TNCSTATE, "Free");
		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

		return;

	case 'B':

		// Was Base64, but has been expanded - just send to User

		// If len > blocksize, fragment

		Len--;
		Msg++;			// Remove Type

		while (Len > 256)
		{
			buffptr = GetBuff();
			buffptr->Len = 256;
			memcpy(buffptr->Data, Msg, 256);
			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			WritetoTrace(TNC, Msg, 256);
			Len -= 256;
			Msg += 256;
			STREAM->bytesRXed += 256;

		}

		buffptr = GetBuff();
		buffptr->Len = Len;
		memcpy(buffptr->Data, Msg, Len);
		C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
		WritetoTrace(TNC, Msg, Len);
		STREAM->bytesRXed += Len;
		sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
				STREAM->bytesTXed - TNC->FreeDataInfo->toSendCount, STREAM->bytesRXed, TNC->FreeDataInfo->toSendCount);
		MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

		return;

	case 'I':

		// Encoded Response

		// Undo = transparency

		ptr = Msg + 1;

		while (ptr = strchr(ptr, '='))
		{
			// Next two chars are a hex value

			a = ptr[1] - 'A';
			b = ptr[2] - 'A';
			memmove(ptr, ptr + 2, Len);
			ptr[0] = (a << 4) + b;
			ptr++;
		}

		buffptr = GetBuff();
		buffptr->Len = sprintf(buffptr->Data, "%s", &Msg[1]);
		C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
		return;

	case 'f':

		// FreeDATA File Transfer

		// Seems to be f null fn null data
		//f.;Makefile.;.;123123123.;
	{
		char * FN;
		time_t CRC;
		int FileLen;
		char Filename[256];
		FILE * fp1;
		unsigned char * ptr;
		char Text[64];
		int textLen;


		if (TNC->FreeDataInfo->RXDir == NULL)
		{
			Debugprintf("FreeDATA RXDIRECTORY not set - file transfer ignored");
			return;
		}

		FN = _strdup(&Msg[3]);
		ptr = &Msg[4] + strlen(FN);

		ptr = ptr + strlen(ptr) + 2;
		CRC = atoi(ptr);
		ptr = ptr + strlen(ptr) + 2;

		FileLen = Len - (ptr - Msg);

		sprintf(Filename, "%s\\%s", TNC->FreeDataInfo->RXDir, FN); 

		fp1 = fopen(Filename, "wb");

		if (fp1)
		{
			fwrite(ptr, 1, FileLen, fp1);
			fclose(fp1);
			textLen = sprintf(Text, "File %s received from %s \r", FN, Call);	
			WritetoTrace(TNC, Text, textLen);
		}
		else
			Debugprintf("FreeDATA - File %s create failed %s", Filename);


		free(FN);
		return;

	}


	}

	if (STREAM->Attached == 0)
		return;


	buffptr = GetBuff();

	if (buffptr == 0)
	{
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "%s", Msg);

	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

}


VOID FreeDataProcessNewConnect(struct TNCINFO * TNC, char * fromCall, char * toCall)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct FreeDataINFO * Modem = TNC->FreeDataInfo;
	char * ptr;
	unsigned char axcall[7];
	char AppName[13] = "";
	APPLCALLS * APPL;
	char * ApplPtr = APPLS;
	int App;
	char Appl[10];
	struct WL2KInfo * WL2K = TNC->WL2K;
	TRANSPORTENTRY * SESS;

	strcpy(TNC->FreeDataInfo->farCall, fromCall);

	ConvToAX25Ex(fromCall, axcall);		// Allow -T and -R SSID's for MPS

	// Check for ExcludeList

	if (ExcludeList[0])
	{
		if (CheckExcludeList(axcall) == FALSE)
		{
			Debugprintf("FreeData Call from %s rejected", fromCall);

			// Send 'd'

			Sleep(1000);
			FreeDataDisconnect(TNC);
			return;	
		}
	}

	//	IF WE HAVE A PERMITTED CALLS LIST, SEE IF HE IS IN IT

	if (TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS)
	{
		UCHAR * ptr = TNC->PortRecord->PORTCONTROL.PERMITTEDCALLS;

		while (TRUE)
		{
			if (memcmp(axcall, ptr, 6) == 0)	// Ignore SSID
				break;

			ptr += 7;

			if ((*ptr) == 0)							// Not in list
			{
				Sleep(1000);
				FreeDataDisconnect(TNC);

				Debugprintf("FreeData Call from %s not in ValidCalls - rejected", fromCall);
				return;
			}
		}
	}

	// The TNC responds to any SSID so we can use incomming call as Appl Call
	// No we can't - it responds but reports the configured call not the called call

	// See which application the connect is for

	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];
		memcpy(Appl, APPL->APPLCALL_TEXT, 10);
		ptr=strchr(Appl, ' ');

		if (ptr)
			*ptr = 0;

		if (_stricmp(toCall, Appl) == 0)
			break;
	}

	if (App < 32)
	{

		memcpy(AppName, &ApplPtr[App * sizeof(struct CMDX)], 12);
		AppName[12] = 0;

		// if SendTandRtoRelay set and Appl is RMS change to RELAY

		if (TNC->SendTandRtoRelay && memcmp(AppName, "RMS ", 4) == 0
			&& (strstr(fromCall, "-T" ) || strstr(fromCall, "-R")))
			strcpy(AppName, "RELAY       ");

		// Make sure app is available

		if (!CheckAppl(TNC, AppName))
		{
			Sleep(1000);
			FreeDataSendCommand(TNC, "dApplication not Available");
			return;
		}
	}



	ProcessIncommingConnectEx(TNC, fromCall, 0, TRUE, TRUE);
	SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

	// if connect to an application, send command

	if (AppName[0])
	{
		buffptr = GetBuff();

		if (buffptr)
		{
			buffptr->Len = sprintf(buffptr->Data, "%s\r", AppName);
			C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			TNC->SwallowSignon = TRUE;
		}
	}

	TNC->ConnectPending = FALSE;

	if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
	{
		sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, toCall, TNC->RIG->Valchar);
		SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
		if (SESS->Frequency == 1500)
		{
			// try to get from WL2K record

			if (WL2K)
			{
				SESS->Frequency = WL2K->Freq;
			}
		}
		SESS->Mode = TNC->WL2KMode;
	}
	else
	{
		sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, toCall);
		if (WL2K)
		{
			SESS->Frequency = WL2K->Freq;
			SESS->Mode = WL2K->mode;
		}
	}

	if (WL2K)
		strcpy(SESS->RMSCall, WL2K->RMSCall);

	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	STREAM->ConnectTime = time(NULL); 
	STREAM->bytesRXed = STREAM->bytesTXed = STREAM->PacketsSent = 0;
	STREAM->Connected = TRUE;

	return;

}

VOID FreeDataProcessConnectAck(struct TNCINFO * TNC, char * Call, unsigned char * Msg, int Len)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct FreeDataINFO * Modem = TNC->FreeDataInfo;

	sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s", STREAM->MyCall, STREAM->RemoteCall);
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
	STREAM->Connected = TRUE;
	STREAM->Connecting = FALSE;

	buffptr = (PMSGWITHLEN)GetBuff();

	if (buffptr == 0) return;			// No buffers, so ignore

	buffptr->Len = sprintf(buffptr->Data, "*** Connected to %s\r", TNC->FreeDataInfo->toCall);

	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
	STREAM->FramesQueued++;

	return;

}
extern char LOC[7];
/*
Line 104:                 if received_json["type"] == 'PING' and received_json["command"] == "PING":
Line 111:                 if received_json["type"] == 'ARQ' and received_json["command"] == "sendFile":
Line 142:                 if received_json["type"] == 'ARQ' and received_json["command"] == "sendMessage":
Line 173:                 if received_json["type"] == 'ARQ' and received_json["command"] == "stopTransmission":
Line 182:                 if received_json["type"] == 'GET' and received_json["command"] == 'STATION_INFO':
Line 195:                 if received_json["type"] == 'GET' and received_json["command"] == 'TNC_STATE':
Line 244:                 if received_json["type"] == 'GET' and received_json["command"] == 'RX_BUFFER':
Line 258:                 if received_json["type"] == 'GET' and received_json["command"] == 'RX_MSG_BUFFER':
Line 272:                 if received_json["type"] == 'SET' and received_json["command"] == 'DEL_RX_BUFFER':
Line 275:                 if received_json["type"] == 'SET' and received_json["command"] == 'DEL_RX_MSG_BUFFER':
*/

//{\"type\" : \"ARQ\", \"command\" : \"sendMessage\",  \"dxcallsign\" : \"G8BPQ\", \"mode\" : \"10\", \"n_frames\" : \"1\", \"data\" :  \"Hello Hello\" , \"checksum\" : \"123\", \"timestamp\" : 1642580748576}



static unsigned char BANNED[] = {'"', '=', ':', '{', '}', '[', ']', '/', 13, 0};	// I think only need to escape = ":  CR Null


static void SendDataMsg(struct TNCINFO * TNC, char * Call, char * Msg, int Len)
{
	// We can't base64 encode chunks. so buffer as original data and encode on send

	SendAsFile(TNC, TNC->FreeDataInfo->farCall, Msg, Len);
	WritetoTrace(TNC, Msg, Len);

	return;
}



static int SendAsRaw(struct TNCINFO * TNC, char * Call, char * myCall, char * Msg, int Len)
{
	char Message[16284];
	char * Base64;

	// TNC now only supports send_raw, with base64 encoded data

	char Template[] = "{\"type\" : \"arq\", \"command\" : \"send_raw\", \"uuid\" : \"%s\",\"parameter\":"
		"[{\"dxcallsign\" : \"%s\", \"mode\": \"255\", \"n_frames\" : \"1\", \"data\" : \"%s\"}]}\n";


	Base64 = byte_base64_encode(Msg, Len);

	Len = sprintf(Message, Template, gen_uuid(), Call, Base64);
	
	free(Base64);
	return send(TNC->TCPDataSock, Message, Len, 0);
}

void FlushData(struct TNCINFO * TNC)
{
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct FreeDataINFO * Info = TNC->FreeDataInfo;
	int Len = Info->toSendCount;

	// We need to flag as data (B) then base64 encode it

	memmove(&Info->toSendData[1], Info->toSendData, Len);
	Info->toSendData[0] = 'B';
	Len++;

	SendAsRaw(TNC, Info->farCall, Info->ourCall, Info->toSendData, Len);

	Info->toSendCount = 0;
	Info->toSendTimeout = 0;

	sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->bytesTXed - TNC->FreeDataInfo->toSendCount, STREAM->bytesRXed, TNC->FreeDataInfo->toSendCount);
	MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

}

static int SendAsFile(struct TNCINFO * TNC, char * Call, char * Msg, int Len)
{
	struct STREAMINFO * STREAM = &TNC->Streams[0]; 
	struct FreeDataINFO * Info = TNC->FreeDataInfo;

	// Add to buffer

	if ((Info->toSendCount + Len) > 8192)			// Reasonable Limit
	{
		// Send the buffered bit

		 FlushData(TNC);
	}

	memcpy(&Info->toSendData[Info->toSendCount], Msg, Len);
	Info->toSendCount += Len;
	Info->toSendTimeout = 10;		// About a second

	STREAM->bytesTXed += Len;

	sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->bytesTXed - TNC->FreeDataInfo->toSendCount, STREAM->bytesRXed, TNC->FreeDataInfo->toSendCount);
	MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

	return Len;
}

static int SendTNCCommand(struct TNCINFO * TNC, char * Type, char * Command)
{
	char Message[256];
	int Len;

	Len = sprintf(Message, "{\"type\" : \"%s\", \"command\" : \"%s\"}\n", Type, Command);
	return send(TNC->TCPDataSock, Message, Len, 0);
}

static void SendPoll(struct TNCINFO * TNC)
{
	return;
}

static void SendPing(struct TNCINFO * TNC, char * Call)
{
	char Ping[] = "{\"type\" : \"ping\", \"command\" : \"ping\", \"dxcallsign\" : \"%s\", \"timestamp\" : %d}\n";
	char Message[256];
	int Len, ret;

	Len = sprintf(Message, Ping, Call, time(NULL));
	ret = send(TNC->TCPDataSock, (char *)&Message, Len, 0);
}

static void SendCQ(struct TNCINFO * TNC)
{
	char CQ[] = "{\"type\" : \"broadcast\", \"command\" : \"cqcqcq\"}\n";

	char Message[256];
	int Len, ret;

	Len = sprintf(Message, "%s", CQ);
	ret = send(TNC->TCPDataSock, (char *)&Message, Len, 0);
}

static void SendBeacon(struct TNCINFO * TNC, int Interval)
{
	char Template1[] = "{\"type\" : \"broadcast\", \"command\" : \"start_beacon\", \"parameter\" : \"%d\"}\n";
	char Template2[] = "{\"type\" : \"broadcast\", \"command\" : \"stop_beacon\"}\n";

	char Message[256];
	int Len, ret;

	if (Interval > 0)
		Len = sprintf(Message, Template1, Interval);
	else
		Len = sprintf(Message, "%s", Template2);

	ret = send(TNC->TCPDataSock, (char *)&Message, Len, 0);
}


unsigned short int compute_crc(unsigned char *buf,int len);


int FreeDataWriteCommBlock(struct TNCINFO * TNC)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, TNC->TXBuffer, TNC->TXLen);

	return 0;
}

char * getObjectFromArray(char * Msg)
{
	// This gets the next object from an array ({} = object, [] = array
	// We look for the end of the object same number of { and }, teminate after } and return pointer to next object
	// So we have terminated Msg, and returned next object in array

	// Only call if Msg is the next object in array


	char * ptr = Msg;
	char c;

	int Open = 0;
	int Close = 0;
		
	while (c = *(ptr++))
	{
		if (c == '{') Open ++; else if (c == '}') Close ++;

		if (Open == Close)
		{
			*(ptr++) = 0;
			return ptr;
		}
	}
	return 0;
}
/*
 ["DATACHANNEL;RECEIVEDOPENER","ARQ;RECEIVING","ARQ;RECEIVING;SUCCESS"] [{"DXCALLSIGN":"GM8BPQ
{"DXCALLSIGN":"GM8BPQ","DXGRID":"","TIMESTAMP":1642847440,"RXDATA":[{"dt":"f","fn":"config.json",
"ft":"application\/json","d":"data:application\/json;base64,
ewogICAgInRuY19ob3N0IiA6ICIxMjcuMC4wLjEiLAogICAgInRuY19wb3J0IiA6ICIzMDAwIiwKICAgICJkYWVtb25faG9zdCIgOiAiMTI3LjAuMC4xIiwKICAgICJkYWVtb25fcG9ydCIgOiAiMzAwMSIsCiAgICAibXljYWxsIiA6ICJBQTBBQSIsCiAgICAibXlncmlkIiA6ICJBQTExZWEiICAgIAp9"
,"crc":"123123123"}]}
*/
void ProcessFileObject(struct TNCINFO * TNC, char * This)
{
	char * Call;
	char * LOC;
	char * FN;
	char * Type;
	char * ptr, * ptr2;
	int Len, NewLen;

	Call = strchr(This, ':');
	Call += 2;
	This = strlop(Call, '"');

	LOC = strchr(This, ':');
	LOC += 2;
	This = strlop(LOC, '"');

	FN = strstr(This, "fn");
	FN += 5;
	This = strlop(FN, '"');

	Type = strstr(This, "base64");
	Type += 7;
	This = strlop(Type, '"');

	// Decode Base64

	Len = strlen(Type);

	Debugprintf("RX %d %s %d", TNC->Port, FN, Len);

	ptr = ptr2 = Type;

	while (Len > 0)
	{
		xdecodeblock(ptr, ptr2);
		ptr += 4;
		ptr2 += 3;
		Len -= 4;
	}

	NewLen = (int)(ptr2 - Type);

	if (*(ptr-1) == '=')
		NewLen--;

	if (*(ptr-2) == '=')
		NewLen--;

	Type[NewLen] = 0;

	Type --;

	Type[0] = 'B';	; // Base64 Info

	FreeDataProcessTNCMessage(TNC, Call, Type, NewLen + 1);


}

void ProcessMessageObject(struct TNCINFO * TNC, char * This)
{
	// This gets Message from a RX_MSG_BUFFER array element.

	char * Call;
	char * LOC;
	char * Msg;
	int Len;
	char * ptr, * ptr2;

	char * ID;
	char * TYPE;
	char * SEQ;
	char * UUID;
	char * TEXT;
	char * NOIDEA;
	char * FORMAT;
	char * FILENAME;
	int fileLen;

	int n;


	Call = strstr(This, "dxcallsign");
	Call += 13;
	This = strlop(Call, '"');

	LOC = strchr(This, ':');
	LOC += 2;
	This = strlop(LOC, '"');

	Msg = strstr(This, "\"data\"");
	Msg += 8;
	This = strlop(Msg, '"');

	// Decode Base64

	// FreeData replaces / with \/ so need to undo

	ptr2 = strstr(Msg, "\\/");

	while (ptr2)
	{
		memmove(ptr2, ptr2 + 1, strlen(ptr2));
		ptr2 = strstr(ptr2, "\\/");
	}

	Len = strlen(Msg);

	ptr = ptr2 = Msg;

	while (Len > 0)
	{
		xdecodeblock(ptr, ptr2);
		ptr += 4;
		ptr2 += 3;
		Len -= 4;
	}

	Len = (int)(ptr2 - Msg);

	if (*(ptr-1) == '=')
		Len--;

	if (*(ptr-2) == '=')
		Len--;

	Msg[Len] = 0;

//m�;send_message�;123�;64730c5c-d32c-47b4-9b11-c958fd07a185�;hhhhhhhhhhhhhhhhhh
//�;�;plain/text�;

//m;send_message;123;64730c5c-d32c-47b4-9b11-c958fd07a185;hhhhhhhhhhhhhhhhhh
//;;plain/text;
	
	// Message elements seem to be delimited by null ;
	// Guessing labels

	ID = Msg;

	if (ID[0] == 'B')
	{
		// BPQ Message

		struct STREAMINFO * STREAM = &TNC->Streams[0];
		PMSGWITHLEN buffptr;


		if (STREAM->Attached)
		{
			if (STREAM->Connected == 1 && STREAM->Connecting == 0)
			{
				char * Line = &ID[1];
				Len -= 1;

				while (Len > 256)
				{
					buffptr = GetBuff();
					buffptr->Len = 256;
					memcpy(buffptr->Data, Line, 256);
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					WritetoTrace(TNC, Line, 256);
					Len -= 256;
					Line += 256;
					STREAM->bytesRXed += 256;
				}

				buffptr = GetBuff();
				buffptr->Len = Len;
				memcpy(buffptr->Data, Line, Len);
				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				WritetoTrace(TNC, Line, Len);
				STREAM->bytesRXed += Len;

			}

			sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
				STREAM->bytesTXed - TNC->FreeDataInfo->toSendCount, STREAM->bytesRXed, TNC->FreeDataInfo->toSendCount);
			MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);
		}
		return;
	}

	n = strlen(ID) + 2;
	Msg += n;
	Len -= n;

	if (ID[0] == 'm')
	{
		// ?? Chat ?? comes from a send raw ??

		struct STREAMINFO * STREAM = &TNC->Streams[0];
		PMSGWITHLEN buffptr;

		TYPE = Msg;
		n = strlen(TYPE) + 2;
		Msg += n;
		Len -= n;

		SEQ = Msg;
		n = strlen(SEQ) + 2;
		Msg += n;
		Len -= n;

		UUID = Msg;
		n = strlen(UUID) + 2;
		Msg += n;
		Len -= n;

		TEXT = Msg;
		n = strlen(TEXT) + 2;
		Msg += n;
		Len -= n;

		NOIDEA = Msg;
		n = strlen(NOIDEA) + 2;
		Msg += n;
		Len -= n;

		FORMAT = Msg;
		n = strlen(FORMAT) + 2;
		Msg += n;
		Len -= n;

		// if Atached, send to user

		if (STREAM->Attached)
		{
			if (STREAM->Connected == 0 && STREAM->Connecting == 0)
			{
				// Just attached - send as Chat Message

				char Line[560];
				char * rest;

				// Send line by line

				rest = strlop(TEXT, 10);		// FreeData chat uses LF

				while (TEXT && TEXT[0])
				{
					Len = strlen(TEXT);
					if (Len > 512)
						TEXT[512] = 0;

					Len = sprintf(Line, "Chat From %-10s%s\r", Call, TEXT);

					while (Len > 256)
					{
						buffptr = GetBuff();
						buffptr->Len = 256;
						memcpy(buffptr->Data, Line, 256);
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
						WritetoTrace(TNC, Line, 256);
						Len -= 256;
						TEXT += 256;
						STREAM->bytesRXed += 256;
					}

					buffptr = GetBuff();
					buffptr->Len = Len;
					memcpy(buffptr->Data, Line, Len);
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					WritetoTrace(TNC, Line, Len);
					STREAM->bytesRXed += Len;

					TEXT = rest;
					rest = strlop(TEXT, 10);		// FreeData chat ues LF
				}
				
				sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
					STREAM->bytesTXed - TNC->FreeDataInfo->toSendCount, STREAM->bytesRXed, TNC->FreeDataInfo->toSendCount);
				MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);
			}
		}
		else
		{
			// Send Not Available Message
//m�;send_message�;123�;64730c5c-d32c-47b4-9b11-c958fd07a185�;hhhhhhhhhhhhhhhhhh
//�;�;plain/text�;

			char reply[512] = "m";
			char * p;

			strcat(TEXT, "\r");
			WritetoTrace(TNC, TEXT, strlen(TEXT));

			strcpy(&reply[2], ";send_message");
			strcpy(&reply[16], ";123");
			reply[21] = ';';
			strcpy(&reply[22], gen_uuid());
			sprintf(&reply[59], ";Message received but user not on line\n");

			p = strchr(&reply[59], 0);

			p[1] = ';';
			strcpy(&p[3], ";plain/text");
			p[15] = ';';

			Len = &p[16] - reply;

			SendAsRaw(TNC, Call, TNC->FreeDataInfo->ourCall, reply, Len);
		}
		return;

	}
	else if (ID[0] == 'f')
	{
		// File Tranfer

		char Filename[256];
		FILE * fp1;
		char Text[64];
		int textLen;


		FILENAME = Msg;
		n = strlen(FILENAME) + 2;
		Msg += n;
		Len -= n;

		TYPE = Msg;
		n = strlen(TYPE) + 2;
		Msg += n;
		Len -= n;

		SEQ = Msg;						// ?? Maybe = 123123123
		n = strlen(SEQ) + 2;
		Msg += n;
		Len -= n;

		TEXT = Msg;						// The file
		fileLen = Len;

		if (TNC->FreeDataInfo->RXDir == NULL)
		{
			Debugprintf("FreeDATA RXDIRECTORY not set - file transfer ignored");
			return;
		}

		sprintf(Filename, "%s\\%s", TNC->FreeDataInfo->RXDir, FILENAME); 

		fp1 = fopen(Filename, "wb");

		if (fp1)
		{
			fwrite(TEXT, 1, fileLen, fp1);
			fclose(fp1);
			textLen = sprintf(Text, "File %s received from %s \r", FILENAME, Call);	
			WritetoTrace(TNC, Text, textLen);
		}
		else
			Debugprintf("FreeDATA - File %s create failed %s", Filename);

		return;
	}
	else if (ID[0] == 'm')
	{

	}








	

//	FreeDataProcessTNCMessage(TNC, Call, Msg, strlen(Msg));
}

void processJSONINFO(struct TNCINFO * TNC, char * Info, char * Call, double snr)
{
	char * LOC = "";
	char * ptr, * Context;

	// Info is an array. Normally only one element, but should check

	ptr = strtok_s(&Info[1], ",]", &Context);

	while (ptr && ptr[1])
	{
		if (strstr(ptr, "BEACON;RECEIVING"))
		{
			char CQ[64];
			int Len;

			Len = sprintf(CQ, "Beacon received from %s SNR %3.1f\r", Call, snr);	
			WritetoTrace(TNC, CQ, Len);
		
			// Add to MH

			if (Call)
				UpdateMH(TNC, Call, '!', 'I');
		}
		if (strstr(ptr, "PING;RECEIVING"))
		{
			// Add to MH

			if (Call)
				UpdateMH(TNC, Call, '!', 'I');
		}
		else if (strstr(ptr, "CQ;RECEIVING"))
		{
			char CQ[64];
			int Len;

			Len = sprintf(CQ, "CQ received from %s SNR %3.1f\r", Call, snr);	
			WritetoTrace(TNC, CQ, Len);

			// Add to MH

			UpdateMH(TNC, Call, '!', 'I');
		}
		else if (strstr(ptr, "PING;RECEIVEDACK"))
		{
			char Msg[128];
			int Len;

			Len = sprintf(Msg, "Ping Response from %s SNR %3.1f\r", Call, snr);
			FreeDataProcessTNCMessage(TNC, Call, Msg, Len);

			// Add to MH

			UpdateMH(TNC, Call, '!', 'I');
		}
		else if (strstr(ptr, "TRANSMITTING;FAILED"))
		{
			// Failed to send a message - if it was a connect request tell appl

			struct STREAMINFO * STREAM = &TNC->Streams[0];
			PMSGWITHLEN buffptr;

			if (STREAM->Connecting)
			{
				sprintf(TNC->WEB_TNCSTATE, "In Use by %s", STREAM->MyCall);
				MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			
				STREAM->Connecting = FALSE;
				buffptr = (PMSGWITHLEN)GetBuff();

				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr->Len = sprintf(buffptr->Data, "*** Connect Failed\r");

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				STREAM->FramesQueued++;
			}
		}
		ptr = strtok_s(NULL, ",]", &Context);
	}
}






char * getJSONValue(char * Msg, char * Key)
{
	char * ptr, *ptr2, *value = 0;
	int vallen, keylen = strlen(Key);
	char c;

	// We Null Terminate the value, so we must look for keys in reverse order

	ptr = strstr(Msg, Key);

	if (ptr)
	{
		ptr += (keylen + 1);

		if (*(ptr) == '[')
		{
			// Array

			int Open = 0;
			int Close = 0;
		
			ptr2 = ptr;

			while (c = *(ptr++))
			{
				if (c == '[')
					Open ++;
				else if (c == ']')
					Close ++;

				if (Open == Close)
				{
					vallen = ptr - ptr2;
					value = ptr2;
					value[vallen] = 0;
					return value;
				}
			}
		}
		else if (*(ptr) == '\"')
		{
			// String

			ptr2 = ptr;
			ptr = strchr(ptr + 1, '\"');
			if (ptr)
			{
				ptr++;
				vallen = ptr - ptr2;
				value = ptr2;
				value[vallen] = 0;
			}
		}
	}
	return value;
}


char stopTNC[] = "{\"type\" : \"SET\", \"command\": \"STOPTNC\" , \"parameter\": \"---\" }\r";


void StopTNC(struct TNCINFO * TNC)
{
	if (TNC->TCPDataSock)
		closesocket(TNC->TCPDataSock);
}


void ProcessTNCJSON(struct TNCINFO * TNC, char * Msg, int Len)
{
	char * ptr;

	if (memcmp(Msg, "{\"ptt\":", 7) == 0)
	{
		if (strstr(Msg, "True"))
		{
//			TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

			if (TNC->PTTMode)
				Rig_PTT(TNC, TRUE);
		}
		else
		{
			if (TNC->PTTMode)
				Rig_PTT(TNC, FALSE);
		}
		return;
	}

	if (memcmp(Msg, "{\"command_response\"", 19) == 0)
	{
		Debugprintf("%d %s", TNC->Port, Msg);
		return;
	}

	if (memcmp(Msg, "{\"freedata\"", 10) == 0)
	{
		// {"freedata":"tnc-message","arq":"session","status":"connected","mycallsign":"G8BPQ-10","dxcallsign":"G8BPQ-0"}
		// {"freedata":"tnc-message","arq":"session","status":"connected","heartbeat":"transmitting","mycallsign":"G8BPQ-10","dxcallsign":"G8BPQ-0"}

		char myCall[12] = "";
		char farCall[12] = "";
		char * ptr2;

		ptr = strstr(Msg, "\"tnc-message\",\"arq\":\"session\"");

		if (ptr)
		{
			ptr = strstr(ptr, "status");

			if (ptr)
			{
				struct STREAMINFO * STREAM = &TNC->Streams[0];
				ptr += 9;

				ptr2 = strstr(ptr, "mycallsign");

				if (ptr2)
				{
					memcpy(myCall, &ptr2[13], 10);
					strlop(myCall, '"');
				}

				ptr2 = strstr(ptr, "dxcallsign");

				if (ptr2)
				{
					memcpy(farCall, &ptr2[13], 10);
					strlop(farCall, '"');
				}

				if (memcmp(ptr, "disconnected", 10) == 0)
				{
					if (TNC->FreeDataInfo->arqstate != 1)
					{
						TNC->FreeDataInfo->arqstate = 1;
						Debugprintf("%d arq_session_state %s", TNC->Port, "disconnected");
					}

					// if connected this is a new disconnect

					if (STREAM->Connected)
					{
	
						hookL4SessionDeleted(TNC, STREAM);

						STREAM->Connected = FALSE;		// Back to Command Mode
						STREAM->ReportDISC = TRUE;		// Tell Node
						STREAM->Disconnecting = FALSE;

						strcpy(TNC->WEB_TNCSTATE, "Free");
						MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
					}
				}
				else if (memcmp(ptr, "connecting", 10) == 0)
				{
					if (TNC->FreeDataInfo->arqstate != 2)
					{
						TNC->FreeDataInfo->arqstate = 2;
						Debugprintf("%d arq_session_state %s", TNC->Port, "connecting");
					}
				}
				else if (memcmp(ptr, "connected", 9) == 0)
				{
					// if connection is idle this is an incoming connect

					if (TNC->FreeDataInfo->arqstate != 3)
					{
						TNC->FreeDataInfo->arqstate = 3;
						Debugprintf("%d arq_session_state %s", TNC->Port, "connected");
					}

					if (STREAM->Connecting == FALSE && STREAM->Connected == FALSE)
					{
						FreeDataProcessNewConnect(TNC, farCall, myCall);
					}

					// if connecting it is a connect ack

					else if (STREAM->Connecting)
					{
						FreeDataProcessConnectAck(TNC, farCall, Msg, Len);
					}
				}

				else if (memcmp(ptr, "close", 5) == 0)
				{
					if (TNC->FreeDataInfo->arqstate != 4)
					{
						TNC->FreeDataInfo->arqstate = 4;
						Debugprintf("%d arq_session_state %s", TNC->Port, "Closed");
					}
				}
				else if (memcmp(ptr, "failed", 5) == 0)
				{
					PMSGWITHLEN buffptr;

					if (TNC->FreeDataInfo->arqstate != 5)
					{
						TNC->FreeDataInfo->arqstate = 5;
						Debugprintf("%d arq_session_state %s", TNC->Port, "failed");
					}

					if (STREAM->Connecting)
					{
						sprintf(TNC->WEB_TNCSTATE, "In Use by %s", STREAM->MyCall);
						MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

						STREAM->Connecting = FALSE;
						buffptr = (PMSGWITHLEN)GetBuff();

						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr->Len = sprintf(buffptr->Data, "*** Connect Failed\r");

						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
						STREAM->FramesQueued++;
					}
				}
			}
	
			Debugprintf("%d %s", TNC->Port, Msg);
			return;
		}

}

	if (memcmp(Msg, "{\"command\":\"tnc_state\"", 22) == 0)
	{
/*
{"command":"tnc_state","ptt_state":"False","tnc_state":"IDLE","arq_state":"False","arq_session":"False",
"arq_session_state":"disconnected","audio_rms":"0","snr":"0","frequency":"None","speed_level":"1",
"mode":"None","bandwidth":"None","fft":"[0]","channel_busy":"False","scatter":[],"rx_buffer_length":"0",
"rx_msg_buffer_length":"0","arq_bytes_per_minute":"0","arq_bytes_per_minute_burst":"0","arq_compression_factor":"0",
"arq_transmission_percent":"0","total_bytes":"0","beacon_state":"False",
"stations":[],"mycallsign":"GM8BPQ-6","dxcallsign":"AA0AA","dxgrid":""}
*/
		char * LOC = 0;
		char * Stations;
		char * myCall = 0;
		char * farCall = 0;
		double snr;
		int arqstate = 0;
		int rx_buffer_length = 0;
		int rx_msg_buffer_length = 0;

		Msg += 23;

		ptr = strstr(Msg, "tnc_state");

		if (ptr)
		{
			if (ptr[12] == 'B' && TNC->Busy == FALSE)
			{
				TNC->Busy = TRUE;
				strcpy(TNC->WEB_CHANSTATE, "Busy");
				SetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
			}
			else if (ptr[12] == 'I' && TNC->Busy)
			{
				TNC->Busy = FALSE;
				strcpy(TNC->WEB_CHANSTATE, "Idle");
				SetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
			}
		}

		ptr = strstr(Msg, "rx_buffer_length");

		if (ptr)
			rx_buffer_length = atoi(&ptr[19]);

		ptr = strstr(Msg, "rx_msg_buffer_length");

		if (ptr)
			rx_msg_buffer_length = atoi(&ptr[23]);

		ptr = strstr(Msg, "snr");

		if (ptr)
			snr = atof(ptr + 6);

		Stations = getJSONValue(Msg, "\"stations\"");

		if (Stations)
		{
			ptr = Stations + strlen(Stations) + 1;
			LOC = getJSONValue(ptr, "\"dxgrid\"");
			farCall = getJSONValue(ptr, "\"dxcallsign\"");
			myCall = getJSONValue(ptr, "\"mycallsign\"");

			if (myCall && farCall)
			{
				myCall++;
				strlop(myCall, '"');
				farCall++;
				strlop(farCall, '"');
			}
		}

		// Look for changes in arq_session_state

		ptr = strstr(Msg, "\"arq_session_state\"");

		if (ptr)
		{
			struct STREAMINFO * STREAM = &TNC->Streams[0];
			ptr += 21;
 
			if (memcmp(ptr, "disconnected", 10) == 0)
			{
				if (TNC->FreeDataInfo->arqstate != 1)
				{
					TNC->FreeDataInfo->arqstate = 1;
					Debugprintf("%d arq_session_state %s", TNC->Port, "disconnected");
				}

				// if connected this is a new disconnect

				if (STREAM->Connected)
				{
					// Create a traffic record

					hookL4SessionDeleted(TNC, STREAM);

					STREAM->Connected = FALSE;		// Back to Command Mode
					STREAM->ReportDISC = TRUE;		// Tell Node
					STREAM->Disconnecting = FALSE;

					strcpy(TNC->WEB_TNCSTATE, "Free");
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
				}
			}
			else if (memcmp(ptr, "connecting", 10) == 0)
			{
				if (TNC->FreeDataInfo->arqstate != 2)
				{
					TNC->FreeDataInfo->arqstate = 2;
					Debugprintf("%d arq_session_state %s", TNC->Port, "connecting");
				}
			}
			else if (memcmp(ptr, "connected", 9) == 0)
			{
				// if connection is idle this is an incoming connect

				if (TNC->FreeDataInfo->arqstate != 3)
				{
					TNC->FreeDataInfo->arqstate = 3;
					Debugprintf("%d arq_session_state %s", TNC->Port, "connected");
				}

				if (STREAM->Connecting == FALSE && STREAM->Connected == FALSE)
				{
					FreeDataProcessNewConnect(TNC, farCall, myCall);
				}

				// if connecting it is a connect ack

				else if (STREAM->Connecting)
				{
					FreeDataProcessConnectAck(TNC, farCall, Msg, Len);
				}
			}

			else if (memcmp(ptr, "disconnecting", 12) == 0)
			{
				if (TNC->FreeDataInfo->arqstate != 4)
				{
					TNC->FreeDataInfo->arqstate = 4;
					Debugprintf("%d arq_session_state %s", TNC->Port, "disconnecting");
				}
			}
			else if (memcmp(ptr, "failed", 5) == 0)
			{
				PMSGWITHLEN buffptr;
				
				if (TNC->FreeDataInfo->arqstate != 5)
				{
					TNC->FreeDataInfo->arqstate = 5;
					Debugprintf("%d arq_session_state %s", TNC->Port, "failed");
				}

				if (STREAM->Connecting)
				{
					sprintf(TNC->WEB_TNCSTATE, "In Use by %s", STREAM->MyCall);
					MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			
					STREAM->Connecting = FALSE;
					buffptr = (PMSGWITHLEN)GetBuff();

					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr->Len = sprintf(buffptr->Data, "*** Connect Failed\r");

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					STREAM->FramesQueued++;
				}
			}
		}

		if (rx_buffer_length || rx_msg_buffer_length)
			FreeGetData(TNC);

		ptr = getJSONValue(Msg, "\"info\"");

		if (ptr == NULL)
			return;

		if (strcmp(ptr, "[]") != 0)
		{

			processJSONINFO(TNC, ptr, farCall, snr);
			Debugprintf("%d %s %s", TNC->Port, ptr, farCall);
		}

		return;
	}

	if (memcmp(Msg, "{\"freedata\":\"tnc-message\"", 25) == 0)
	{
		char * mycall = strstr(Msg, "mycall");
		char * dxcall = strstr(Msg, "dxcall");
		char * dxgrid = strstr(Msg, "dxgrid");
		char * snrptr = strstr(Msg, "snr");
		float snr = 0;
		char CQ[64];
		int Len;

		Msg += 26;

		if (mycall && dxcall && dxgrid)
		{
			mycall += 13;
			strlop(mycall, '"');

			dxcall += 13;
			strlop(dxcall, '"');

			dxgrid += 9;
			strlop(dxgrid, '"');
		}


		if (dxcall && strstr(dxcall, "-0"))
			strlop(dxcall, '-');

		if (snrptr)
			snr = atof(&snrptr[6]);

		if (memcmp(Msg, "\"beacon\":\"received\"", 18) == 0)
		{
			if (mycall && dxcall && dxgrid)
			{
				Len = sprintf(CQ, "Beacon received from %s SNR %3.1f", dxcall, snr);	
				WritetoTrace(TNC, CQ, Len);

				// Add to MH

				if (dxcall)
					UpdateMH(TNC, dxcall, '!', 'I');

				return;
			}
		}

		if (memcmp(Msg, "\"cq\":\"received\"", 14) == 0)
		{
			if (mycall && dxcall && dxgrid)
			{
				Len = sprintf(CQ, "CQ received from %s", dxcall);	
				WritetoTrace(TNC, CQ, Len);

				// Add to MH

				if (dxcall)
					UpdateMH(TNC, dxcall, '!', 'I');

				return;
			}
		}

		if (memcmp(Msg, "\"ping\":\"received\"", 16) == 0)
		{
			if (mycall && dxcall && dxgrid)
			{
				Len = sprintf(CQ, "PING received from %s SNR %3.1f", dxcall, snr);	
				WritetoTrace(TNC, CQ, Len);

				// Add to MH

				if (dxcall)
					UpdateMH(TNC, dxcall, '!', 'I');

				return;
			}
		}

		if (memcmp(Msg, "\"ping\":\"acknowledge\"", 16) == 0)
		{
			if (mycall && dxcall && dxgrid)
			{
				char Msg[128];
	
				Len = sprintf(Msg, "Ping Response from %s SNR %3.1f\r", dxcall, snr);
				FreeDataProcessTNCMessage(TNC, dxcall, Msg, Len);

				UpdateMH(TNC, dxcall, '!', 'I');

				return;
			}
		}






		Debugprintf("%d %s", TNC->Port, Msg);
		return;
	}

	if (memcmp(Msg, "{\"command\":\"rx_buffer\"", 22) == 0)
	{
		char * Next, * This;

		// Delete from TNC
			
		SendTNCCommand(TNC, "set", "del_rx_buffer");		Msg += 22;

		ptr = getJSONValue(Msg, "\"eof\"");
		ptr = getJSONValue(Msg, "\"data-array\"");

		This = ptr;

		if (This[1] == '{')		// Array of objects
		{
			This++;
			do
			{
				Next = getObjectFromArray(This);
				ProcessMessageObject(TNC, This);
				This = Next;

			} while (Next && Next[0] == '{');
		}

		return;
	}

	Debugprintf("%d %s", TNC->Port, Msg);


//	{"COMMAND":"RX_BUFFER","DATA-ARRAY":[],"EOF":"EOF"}
/* {"COMMAND":"RX_BUFFER","DATA-ARRAY":[{"DXCALLSIGN":"GM8BPQ","DXGRID":"","TIMESTAMP":1642579504,
"RXDATA":[{"dt":"f","fn":"main.js","ft":"text\/javascript"
,"d":"data:text\/javascript;base64,Y29uc3Qge.....9KTsK","crc":"123123123"}]}],"EOF":"EOF"}



{"arq":"received","uuid":"a1346319-6eb0-42aa-b5a0-c9493c8ccdca","timestamp":1645812393,"dxcallsign":"G8BPQ-2","dxgrid":"","data":"QyBHOEJQUS0yIEc4QlBRLTIgRzhCUFEtMiA="}
{"ptt":"True"}


*/
	if (memcmp(Msg, "{\"arq\":\"received\"", 17) == 0)
	{
		int NewLen;
		char * ptr, *ptr2, *Type;
		char * Call = 0;
		char * myCall = 0;

		Msg += 17;
		
		ptr = getJSONValue(Msg, "\"data\"");
		Type = ++ptr;

		// Decode Base64

		// FreeData replaces / with \/ so need to undo

		ptr2 = strstr(Type, "\\/");

		while (ptr2)
		{
			memmove(ptr2, ptr2 + 1, strlen(ptr2));
			ptr2 = strstr(ptr2, "\\/");
		}

		Len = strlen(Type) - 1;

		//	Debugprintf("RX %d %s %d", TNC->Port, FN, Len);

		ptr = ptr2 = Type;

		while (Len > 0)
		{
			xdecodeblock(ptr, ptr2);
			ptr += 4;
			ptr2 += 3;
			Len -= 4;
		}

		NewLen = (int)(ptr2 - Type);

		if (*(ptr-1) == '=')
			NewLen--;

		if (*(ptr-2) == '=')
			NewLen--;

		Type[NewLen] = 0;

		myCall = getJSONValue(Msg, "\"mycallsign\"");
		Call = getJSONValue(Msg, "\"dxcallsign\"");

		if (Call)
		{
			Call++;
			strlop(Call, '"');
		}

		if (myCall)
		{
			myCall++;
			strlop(myCall, '"');
		}


		FreeDataProcessTNCMessage(TNC, Call, Type, NewLen);

		return;
	}

	if (memcmp(Msg, "{\"COMMAND\":\"RX_MSG_BUFFER\"", 26) == 0)
	{
		char * Next, * This;

		Msg += 26;
		ptr = getJSONValue(Msg, "\"EOF\"");
		ptr = getJSONValue(Msg, "\"DATA-ARRAY\"");

		This = ptr;

		if (This[1] == '{')		// Array of objects
		{
			This++;
			do {
				Next = getObjectFromArray(This);
				ProcessMessageObject(TNC, This);
				This = Next;
			} while (Next && Next[0] == '{');
		
			// Delete from TNC
			
			SendTNCCommand(TNC, "SET", "DEL_RX_MSG_BUFFER");
		}


		return;
	}
}
	
int FreeDataConnect(struct TNCINFO * TNC, char * Call)
{
	char Connect[] = "{\"type\" : \"arq\", \"command\": \"connect\" , \"dxcallsign\": \"%s\", \"attempts\": \"%d\"}\n";
	char Msg[128];
	int Len;

	Len = sprintf(Msg, Connect, Call, TNC->MaxConReq);

	return send(TNC->TCPDataSock, Msg, Len, 0);
}

int FreeDataDisconnect(struct TNCINFO * TNC)
{
	char Disconnect[] = "{\"type\" : \"arq\", \"command\": \"disconnect\"}\n";
	char Msg[128];
	int Len;

//	return FreeDataSendCommand(TNC, "D");

	Len = sprintf(Msg, "%s", Disconnect);

	return send(TNC->TCPDataSock, Msg, Len, 0);
}


int FreeGetData(struct TNCINFO * TNC)
{
	char GetData[] = "{\"type\" : \"get\", \"command\": \"rx_buffer\"}\n";
	char Msg[128];
	int Len;

	Len = sprintf(Msg, "%s", GetData);

	return send(TNC->TCPDataSock, Msg, Len, 0);
}

int FreeDataSendCommand(struct TNCINFO * TNC, char * Msg)
{
	// Commands are simulated as Messages to the remote BPQ. The TNC itself does not handle any commands

	// First Byte of MSG is a Type - Command or Data. MSG has a limited character set Use =xx for Now.

	// Current Types - C = Connect, D = Disconnect, I = info

	SendAsRaw(TNC, TNC->FreeDataInfo->farCall, TNC->FreeDataInfo->ourCall, Msg, strlen(Msg));
	return 0;
}

void FreeDataProcessTNCMsg(struct TNCINFO * TNC)
{
	int DataInputLen, MsgLen;
	char * ptr, * endptr;
	int maxlen;

	// May get message split over packets or multiple messages per packet

	// A complete file transfer arrives as one message, so can bw very long


	if (TNC->DataInputLen > MAXRXSIZE)	// Shouldnt have packets longer than this
		TNC->DataInputLen=0;

	maxlen = MAXRXSIZE - TNC->DataInputLen;

	if (maxlen >1400)
		maxlen = 1400;
				
	DataInputLen = recv(TNC->TCPDataSock, &TNC->ARDOPDataBuffer[TNC->DataInputLen], maxlen, 0);

	if (DataInputLen == 0 || DataInputLen == SOCKET_ERROR)
	{
		struct STREAMINFO * STREAM = &TNC->Streams[0];
				
		closesocket(TNC->TCPDataSock);

		TNC->TCPDataSock = 0;

		TNC->TNCCONNECTED = FALSE;

		STREAM->Connected = FALSE;		// Back to Command Mode
		STREAM->ReportDISC = TRUE;		// Tell Node
		STREAM->Disconnecting = FALSE;

		strcpy(TNC->WEB_TNCSTATE, "Free");
		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		return;					
	}

	TNC->DataInputLen += DataInputLen;

	TNC->ARDOPDataBuffer[TNC->DataInputLen] = 0;	// So we can use string functions

	// Message should be json. We know the format, so don't need a general parser, but need to know if complete.
	// I think counting { and } and stopping if equal should work;

//	Debugprintf(TNC->ARDOPDataBuffer);

	//I think now messages end with LF

loop:

	endptr = strchr(TNC->ARDOPDataBuffer, 10);

	if (endptr == 0)
		return;

	*(endptr) = 0;

	if (TNC->ARDOPDataBuffer[0] != '{')
	{
		TNC->DataInputLen = 0;
		return;
	}

	ptr = &TNC->ARDOPDataBuffer[0];
	
	MsgLen = endptr - ptr;
	
	ProcessTNCJSON(TNC, ptr, MsgLen);

	// MsgLen doesnt include lf

	MsgLen++;

	if (TNC->DataInputLen == MsgLen)
	{
		TNC->DataInputLen = 0;
		return;
	}

	// More in buffer

	ptr += MsgLen;
	TNC->DataInputLen -= MsgLen;

	memmove(TNC->ARDOPDataBuffer, ptr, TNC->DataInputLen + 1);

	goto loop;
	
	// Message Incomplete - wait for rest;
}



VOID FreeDataThread(void * portptr);

int ConnecttoFreeData(int port)
{
	_beginthread(FreeDataThread, 0, (void *)(size_t)port);

	return 0;
}

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;


VOID FreeDataThread(void * portptr)
{
	// Messages are JSON encapulated
	// Now We run tnc directly so don't open daemon socket
	// Looks for data on socket(s)
	
	int port = (int)(size_t)portptr;
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	char Params[512];

	if (TNC->HostName == NULL)
		return;

	buildParamString(TNC, Params);

	TNC->BusyFlags = 0;

	TNC->TNCCONNECTING = TRUE;

	Sleep(3000);		// Allow init to complete 

//	printf("Starting FreeDATA Thread\n");

// if on Windows and Localhost see if TNC is running

#ifdef WIN32

	if (strcmp(TNC->HostName, "127.0.0.1") == 0)
	{
		// can only check if running on local host
		
		TNC->PID = GetListeningPortsPID(TNC->Datadestaddr.sin_port);
		
		if (TNC->PID == 0)
			goto TNCNotRunning;

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
		goto TNCRunning;
	}

#endif

TNCNotRunning:

	// Not running or can't check, restart if we have a path 

	if (TNC->ProgramPath)
	{
		Consoleprintf("Trying to (re)start TNC  %s", TNC->ProgramPath);

		if (RestartTNC(TNC))
			CountRestarts(TNC);

		Sleep(TNC->AutoStartDelay);

#ifdef LINBPQ
		Sleep(5000);
#endif

	}

TNCRunning:

	if (TNC->Alerted == FALSE)
	{
		sprintf(TNC->WEB_COMMSSTATE, "Connecting to TNC");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}

	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->Datadestaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		HostEnt = gethostbyname (TNC->HostName);
		 
		 if (!HostEnt)
		 {
		 	TNC->TNCCONNECTING = FALSE;
			sprintf(Msg, "Resolve Failed for FreeData Host - error code = %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);
			return;			// Resolve failed
		 }
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

	TNC->TCPDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for FreeData TNC socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->TNCCONNECTING = FALSE;
  	 	return; 
	}

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	// Connect TNC Port

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
   			i=sprintf(Msg, "Connect Failed for FreeData TNC socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPDataSock);
		TNC->TCPDataSock = 0;
	 	TNC->TNCCONNECTING = FALSE;
			
		return;
	}

	Sleep(1000);

	TNC->LastFreq = 0;

 	TNC->TNCCONNECTING = FALSE;
	TNC->TNCCONNECTED = TRUE;
	TNC->BusyFlags = 0;
	TNC->InputLen = 0;
	TNC->Alerted = FALSE;

	TNC->Alerted = TRUE;

	sprintf(TNC->WEB_COMMSSTATE, "Connected to FreeData TNC");		
	MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

	sprintf(Msg, "Connected to FreeData TNC Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);

	while (TNC->TNCCONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		if (TNC->TNCCONNECTED)
			FD_SET(TNC->TCPDataSock,&readfs);
					
		if (TNC->TNCCONNECTING || TNC->TNCCONNECTED) FD_SET(TNC->TCPDataSock,&errorfs);

		timeout.tv_sec = 300;
		timeout.tv_usec = 0;

		ret = select((int)TNC->TCPDataSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			Debugprintf("FreeData Select failed %d ", WSAGetLastError());
			goto Lost;
		}

		// If nothing doing send get rx_buffer as link validation poll

		if (ret == 0)
		{
			char GetData[] = "{\"type\" : \"get\", \"command\": \"rx_buffer\"}\n";
			int Len;
			
			Len =  send(TNC->TCPDataSock, GetData, strlen(GetData), 0);

			if (Len != strlen(GetData))
				goto closeThread;
		}
		else
		{
			//	See what happened

			if (FD_ISSET(TNC->TCPDataSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				FreeDataProcessTNCMsg(TNC);
				FreeSemaphore(&Semaphore);
			}
								
			if (FD_ISSET(TNC->TCPDataSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "FreeData TNC Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->TNCCONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->TCPDataSock);
				TNC->TCPDataSock = 0;
			}

			continue;
		}
	}

closeThread:

	if (TNC->TCPDataSock)
	{
		shutdown(TNC->TCPDataSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->TCPDataSock);
	}

	sprintf(Msg, "FreeData Thread Terminated Port %d\r\n", TNC->Port);
	TNC->lasttime = time(NULL);				// Prevent immediate restart

	WritetoConsole(Msg);
}

void ConnectTNCPort(struct TNCINFO * TNC)
{
	char Msg[255];
	int err;
	int bcopt = TRUE;

	TNC->TNCCONNECTING = TRUE;

	TNC->TCPDataSock = socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPDataSock == INVALID_SOCKET)
	{
		sprintf(Msg, "Socket Failed for FreeData TNC socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->TNCCONNECTING = FALSE;
  	 	return; 
	}

	setsockopt(TNC->TCPDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(TNC->TCPDataSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	if (connect(TNC->TCPDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		//	Connected successful

		sprintf(TNC->WEB_COMMSSTATE, "Connected to FreeData TNC");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
		TNC->TNCCONNECTING = FALSE;
		TNC->TNCCONNECTED = TRUE;
		TNC->Alerted = FALSE;
		return;
	}

	if (TNC->Alerted == FALSE)
	{
		err=WSAGetLastError();
		sprintf(Msg, "Connect Failed for FreeData TNC socket - error code = %d Port %d\n",
				err, htons(TNC->Datadestaddr.sin_port));

		WritetoConsole(Msg);
		TNC->Alerted = TRUE;
		TNC->TNCCONNECTING = FALSE;

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}

	closesocket(TNC->TCPDataSock);

	TNC->TCPDataSock = 0;
	TNC->TNCCONNECTING = FALSE;
	return;
}

void buildParamString(struct TNCINFO * TNC, char * line)
{
	//  choices=[, "direct", "rigctl", "rigctld"],

	struct FreeDataINFO * FDI = TNC->FreeDataInfo;
	int capindex = -1, playindex = -1;
	int i;

	// Python adds sound mapper on front and counts Playback after Capture

	for (i = 0; i < CaptureCount; i++)
	{
		if (strstr(&CaptureNames[i][0], TNC->FreeDataInfo->Capture))
		{
			capindex = i + 1;
			break;
		}
	}

	for (i = 0; i < PlaybackCount; i++)
	{
		if (strstr(&PlaybackNames[i][0], TNC->FreeDataInfo->Playback))
		{
			playindex = i + CaptureCount + 2;
			break;
		}
	}

	sprintf(line,
		"--mycall %s --ssid %s --mygrid %s --rx %d --tx %d --port %d --radiocontrol %s "
		"--tuning_range_fmin %3.1f --tuning_range_fmax %3.1f --tx-audio-level %d",

		FDI->ourCall, FDI->SSIDList, LOC, capindex, playindex, TNC->TCPPort, FDI->hamlibHost ? "rigctld" : "disabled",
		FDI->TuningRange * -1.0, FDI->TuningRange * 1.0, FDI->TXLevel);

	if (FDI->hamlibHost)
		sprintf(&line[strlen(line)], " --rigctld_ip %s --rigctld_port %d", FDI->hamlibHost, FDI->hamlibPort);

	if (FDI->LimitBandWidth)
		strcat(line, " --500hz");

	if (FDI->Explorer)
		strcat(line, " --explorer");


	// Add these to the end if needed 
	//				--scatter
	//				--fft
	//				--fsk
	//				--qrv (respond to cq)

}

// We need a local restart tnc as we need to add params and start a python progrm on Linux

BOOL KillOldTNC(char * Path);

static BOOL RestartTNC(struct TNCINFO * TNC)
{
	if (TNC->ProgramPath == NULL || TNC->DontRestart)
		return 0;

	if (_memicmp(TNC->ProgramPath, "REMOTE:", 7) == 0)
	{
		int n;
		
		// Try to start TNC on a remote host

		SOCKET sock = socket(AF_INET,SOCK_DGRAM,0);
		struct sockaddr_in destaddr;

		Debugprintf("trying to restart TNC %s", TNC->ProgramPath);

		if (sock == INVALID_SOCKET)
			return 0;

		destaddr.sin_family = AF_INET;
		destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
		destaddr.sin_port = htons(8500);

		if (destaddr.sin_addr.s_addr == INADDR_NONE)
		{
			//	Resolve name to address

			struct hostent * HostEnt = gethostbyname (TNC->HostName);
		 
			if (!HostEnt)
				return 0;			// Resolve failed

			memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		}

		n = sendto(sock, TNC->ProgramPath, (int)strlen(TNC->ProgramPath), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
	
		Debugprintf("Restart TNC - sendto returned %d", n);

		Sleep(100);
		closesocket(sock);

		return 1;				// Cant tell if it worked, but assume ok
	}

	// Not Remote

	// Add  parameters to command string

#ifndef WIN32
	{
		char * arg_list[64];
		char rxVal[16];
		char txVal[16];
		char tunePlus[16];
		char tuneMinus[16];
		char portVal[16];
		char txLevelVal[16];
		char RigPort[16];
		int n = 0;
		int i = 0;
		pid_t child_pid;

		struct FreeDataINFO * FDI = TNC->FreeDataInfo;

		signal(SIGCHLD, SIG_IGN); // Silently (and portably) reap children. 

		arg_list[n++] = "python3";
		arg_list[n++] = TNC->ProgramPath;
		arg_list[n++] = "--mycall";
		arg_list[n++] = FDI->ourCall;
		arg_list[n++] = "--ssid";

		while(TNC->FreeDataInfo->SSIDS[i])
			arg_list[n++] = TNC->FreeDataInfo->SSIDS[i++];
	
		arg_list[n++] = "--mygrid";
		arg_list[n++] = LOC;
		arg_list[n++] = "--rx";
		sprintf(rxVal, "%s", TNC->FreeDataInfo->Capture);
		arg_list[n++] = rxVal;
		arg_list[n++] = "--tx";
		sprintf(txVal, "%s", TNC->FreeDataInfo->Playback);
		arg_list[n++] = txVal;
		arg_list[n++] = "--port";
		sprintf(portVal, "%d", TNC->TCPPort);
		arg_list[n++] = portVal;
		arg_list[n++] = "--radiocontrol";
		arg_list[n++] = FDI->hamlibHost ? "rigctld" : "disabled";
		arg_list[n++] = "--tuning_range_fmin";
		sprintf(tuneMinus, "%3.1f", FDI->TuningRange * -1.0);
		arg_list[n++] = tuneMinus;
		arg_list[n++] = "--tuning_range_fmax";
		sprintf(tunePlus, "%3.1f", FDI->TuningRange * 1.0);
		arg_list[n++] = tunePlus;
		arg_list[n++] = "--tx-audio-level";
		sprintf(txLevelVal, "%d", FDI->TXLevel);
		arg_list[n++] = txLevelVal;

		if (FDI->hamlibHost)
		{
			arg_list[n++] = "--rigctld_ip";
			arg_list[n++] = FDI->hamlibHost;
			arg_list[n++] = "--rigctld_port";
			sprintf(RigPort, "%d", FDI->hamlibPort);
			arg_list[n++] = RigPort;
		}

		if (FDI->LimitBandWidth)
			arg_list[n++] = "--500hz";

		if (FDI->Explorer)
			arg_list[n++] = "--explorer";

		arg_list[n++] = 0;

		n = 0;

		//	Fork and Exec TNC

		printf("Trying to start %s\n", TNC->ProgramPath);

		/* Duplicate this process. */ 

		child_pid = fork (); 

		if (child_pid == -1) 
		{    				
			printf ("StartTNC fork() Failed\n"); 
			return 0;
		}

		if (child_pid == 0) 
		{    				
			execvp (arg_list[0], arg_list); 

			/* The execvp  function returns only if an error occurs.  */ 

			printf ("Failed to start TNC\n"); 
			exit(0);			// Kill the new process
		}
		
		TNC->PID = child_pid;
		printf("Started TNC, Process ID = %d\n", TNC->PID);

		return TRUE;
	}								 
#else

	{
		int n = 0;

		STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
		PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
		char workingDirectory[256];
		char commandLine[512];
		char Params[512];

		int i = strlen(TNC->ProgramPath);

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
		SInfo.lpReserved2=NULL; 

		Debugprintf("RestartTNC Called for %s", TNC->ProgramPath);


		strcpy(workingDirectory, TNC->ProgramPath);

		while (i--)
		{
			if (workingDirectory[i] == '\\' || workingDirectory[i] == '/')
			{
				workingDirectory[i] = 0; 
				break;
			}
		}

		buildParamString(TNC, Params);

		if (TNC->PID)
		{
			KillTNC(TNC);
			Sleep(100);
		}

		sprintf(commandLine, "\"%s\" %s", TNC->ProgramPath, Params);

		if (CreateProcess(NULL, commandLine, NULL, NULL, FALSE,0, NULL, workingDirectory, &SInfo, &PInfo))
		{
			Debugprintf("Restart TNC OK");
			TNC->PID = PInfo.dwProcessId;
			return TRUE;
		}
		else
		{
			Debugprintf("Restart TNC Failed %d ", GetLastError());
			return FALSE;
		}
	}
#endif
	return 0;
}
