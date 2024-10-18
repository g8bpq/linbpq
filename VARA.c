/*
Copyright 2001-2015 John Wiseman G8BPQ

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
//	Interface to allow G8BPQ switch to use VARA Virtual TNC in a form 
//	of ax.25


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>


#include "CHeaders.h"

#ifdef WIN32
#include <Psapi.h>
#endif

extern int (WINAPI FAR *GetModuleFileNameExPtr)();
extern int (WINAPI FAR *EnumProcessesPtr)();

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"

#include "tncinfo.h"


#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

#define FEND 0xC0 
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

static int Socket_Data(int sock, int error, int eventcode);

int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);
int KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
int SendReporttoWL2K(struct TNCINFO * TNC);
char * CheckAppl(struct TNCINFO * TNC, char * Appl);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
BOOL KillOldTNC(char * Path);
int VARASendData(struct TNCINFO * TNC, UCHAR * Buff, int Len);
VOID VARASendCommand(struct TNCINFO * TNC, char * Buff, BOOL Queue);
VOID VARAProcessDataPacket(struct TNCINFO * TNC, UCHAR * Data, int Length);
void CountRestarts(struct TNCINFO * TNC);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
VOID PROCESSNODEMESSAGE(MESSAGE * Msg, struct PORTCONTROL * PORT);
VOID NETROMMSG(struct _LINKTABLE * LINK, L3MESSAGEBUFFER * L3MSG);

#ifndef LINBPQ
BOOL CALLBACK EnumVARAWindowsProc(HWND hwnd, LPARAM  lParam);
#endif

static char ClassName[]="VARASTATUS";
static char WindowTitle[] = "VARA";
static int RigControlRow = 165;

#define WINMOR
#define NARROWMODE 21
#define WIDEMODE 22

#ifndef LINBPQ
#include <commctrl.h>
#endif

extern int SemHeldByAPI;

static RECT Rect;
extern void * TRACE_Q;

BOOL VARAStopPort(struct PORTCONTROL * PORT)
{
	// Disable Port - close TCP Sockets or Serial Port

	struct TNCINFO * TNC = PORT->TNC;

	TNC->CONNECTED = FALSE;
	TNC->Alerted = FALSE;

	if (TNC->PTTMode)
		Rig_PTT(TNC, FALSE);			// Make sure PTT is down

	if (TNC->Streams[0].Attached)
		TNC->Streams[0].ReportDISC = TRUE;

	TNC->Streams[0].Disconnecting = FALSE;

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

	KillTNC(TNC);

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Stopped");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}

int ConnecttoVARA(int port);

BOOL VARAStartPort(struct PORTCONTROL * PORT)
{
	// Restart Port - Open Sockets or Serial Port

	struct TNCINFO * TNC = PORT->TNC;

	ConnecttoVARA(TNC->Port);
	TNC->lasttime = time(NULL);;

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Restarted");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}



static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
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

	if (_stricmp(buf, "ADDR"))
		return FALSE;						// Must start with ADDR

	ptr = strtok(NULL, " \t\n\r");

	BPQport = Port;
	p_ipad = ptr;

	TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
	if (p_ipad == NULL)
		p_ipad = strtok(NULL, " \t\n\r");

	if (p_ipad == NULL) return (FALSE);
	
	p_port = strtok(NULL, " \t\n\r");
			
	if (p_port == NULL) return (FALSE);

	WINMORport = atoi(p_port);

	TNC->destaddr.sin_family = AF_INET;
	TNC->destaddr.sin_port = htons(WINMORport);
	TNC->Datadestaddr.sin_family = AF_INET;
	TNC->Datadestaddr.sin_port = htons(WINMORport+1);

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

	TNC->MaxConReq = 10;		// Default

	// Read Initialisation lines

	while (TRUE)
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
	
		else if (_memicmp(buf, "VARAAC", 6) == 0)
			TNC->VaraACAllowed = atoi(&buf[7]);

		else if (_memicmp(buf, "BW2300", 6) == 0)
		{
			TNC->ARDOPCurrentMode[0] = 'W';				// Save current state for scanning
			strcat(TNC->InitScript, buf);
			TNC->DefaultMode = TNC->WL2KMode = 50;
		}
		else if (_memicmp(buf, "BW500", 5) == 0)
		{
			TNC->ARDOPCurrentMode[0] = 'N';
			strcat(TNC->InitScript, buf);
			TNC->DefaultMode = TNC->WL2KMode = 53;
		}
		else if (_memicmp(buf, "BW2750", 6) == 0)
		{
			TNC->ARDOPCurrentMode[0] = 'W';				// Save current state for scanning
			strcat(TNC->InitScript, buf);
			TNC->DefaultMode = TNC->WL2KMode = 54;
		}
		else if (_memicmp(buf, "FM1200", 6) == 0)
			TNC->DefaultMode = TNC->WL2KMode = 51;
		else if (_memicmp(buf, "FM9600", 5) == 0)
			TNC->DefaultMode = TNC->WL2KMode = 52;
		else if (standardParams(TNC, buf) == FALSE)
			strcat(TNC->InitScript, buf);

	}

	return (TRUE);	
}

void VARAThread(void * portptr);
int ConnecttoVARA(int port);
VOID VARAProcessReceivedData(struct TNCINFO * TNC);
VOID VARAProcessReceivedControl(struct TNCINFO * TNC);
VOID VARAReleaseTNC(struct TNCINFO * TNC);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

static time_t ltime;


static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);

void doVarACSend(struct TNCINFO * TNC)
{
	int hdrlen;
	int txlen = strlen(TNC->VARACMsg);
	char txbuff[64];

	txlen--;		// remove cr
	hdrlen = sprintf(txbuff, "%d ", txlen); 
	send(TNC->TCPDataSock, txbuff, hdrlen, 0);		// send length
	send(TNC->TCPDataSock, TNC->VARACMsg, txlen, 0);

	free (TNC->VARACMsg);
	TNC->VARACMsg = 0;
	TNC->VARACSize = 0;

	TNC->VarACTimer = 0;
	return ;
}
static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	size_t datalen;
	PMSGWITHLEN buffptr;
	char txbuff[500];
	unsigned int bytes;
	size_t txlen=0;
	size_t Param;
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
			buffptr = Q_REM(&TNC->BPQtoWINMOR_Q);

			if (buffptr)
				ReleaseBuffer(buffptr);
		}
	}

	switch (fn)
	{
	case 8:

		return 0;

	case 7:

		// approx 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		if (TNC->VarACTimer)
		{
			TNC->VarACTimer--;

			if (TNC->VarACTimer == 0)
				doVarACSend(TNC);
		}

		// G7TAJ's code to record activity for stats display
			
		if ( TNC->BusyFlags && CDBusy )
			TNC->PortRecord->PORTCONTROL.ACTIVE += 2;

		if ( TNC->PTTState )
			TNC->PortRecord->PORTCONTROL.SENDING += 2;
		
		// Check session limit timer

		if ((STREAM->Connecting || STREAM->Connected) && !STREAM->Disconnecting)
		{
			if (TNC->SessionTimeLimit && STREAM->ConnectTime && time(NULL) > (TNC->SessionTimeLimit + STREAM->ConnectTime))
			{
				VARASendCommand(TNC, "CLEANTXBUFFER\r", TRUE);
				VARASendCommand(TNC, "DISCONNECT\r", TRUE);
				STREAM->Disconnecting = TRUE;
				TNC->SessionTimeLimit += 120;	// Don't retrigger unless things have gone horribly wrong
			}
		}

		while (TNC->PortRecord->UI_Q)
		{
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);	
		}

		if (TNC->Busy)							//  Count down to clear
		{
			if ((TNC->BusyFlags & CDBusy) == 0)	// TNC Has reported not busy
			{
				TNC->Busy--;
				if (TNC->Busy == 0)
					SetWindowText(TNC->xIDC_CHANSTATE, "Clear");
					strcpy(TNC->WEB_CHANSTATE, "Clear");
			}
		}

		if (TNC->BusyDelay)
		{
			// Still Busy?

			if (InterlockedCheckBusy(TNC) == FALSE)
			{
				// No, so send

				VARASendCommand(TNC, TNC->ConnectCmd, TRUE);
				TNC->Streams[0].Connecting = TRUE;
				TNC->Streams[0].ConnectTime = time(NULL); 

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &TNC->ConnectCmd[8], strlen(TNC->ConnectCmd)-10);

				sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

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

					PMSGWITHLEN buffptr = GetBuff();

					TNC->Streams[0].Connecting = FALSE;
					TNC->Streams[0].ConnectTime = time(NULL); 

					if (buffptr == 0) return (0);			// No buffers, so ignore

					buffptr->Len = 39;
					memcpy(buffptr->Data,"Sorry, Can't Connect - Channel is busy\r", 39);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					free(TNC->ConnectCmd);

					sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
					SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

				}
			}
		}

		return 0;

	case 1:				// poll

		if (STREAM->NeedDisc)
		{
			STREAM->NeedDisc--;

			if (STREAM->NeedDisc == 0)
			{
				// Send the DISCONNECT

				VARASendCommand(TNC, "DISCONNECT\r", TRUE);
			}
		}

	/*
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
*/	

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Streams[0].Attached == 0)
		{
			// New Attach

			int calllen;
			char Msg[80];

			TNC->Streams[0].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
			TNC->Streams[0].MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

			VARASendCommand(TNC, "LISTEN OFF\r", TRUE);

			TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			// Stop Scanning

			sprintf(Msg, "%d SCANSTOP", TNC->Port);
	
			Rig_Command( (TRANSPORTENTRY *) -1, Msg);
		}

		if (TNC->Streams[0].Attached)
			CheckForDetach(TNC, 0, &TNC->Streams[0], TidyClose, ForcedClose, CloseComplete);

		if (TNC->Streams[0].ReportDISC)
		{
			TNC->Streams[0].ReportDISC = FALSE;
			buff->PORT = 0;
			return -1;
		}

		if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
		{
			//	See if time to reconnect
		
			time(&ltime);
			if (ltime - TNC->lasttime > 9 )
			{
				TNC->lasttime = ltime;
				ConnecttoVARA(port);
			}
		}
		
		// See if any frames for this port

		if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
		{
			PMSGWITHLEN buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
			txlen = (int)buffptr->Len;

			if(TNC->VaraACMode || TNC->VaraModeSet == 0)
			{
				// Send in varac format - 

				// 5 Hello, 15 de G8 BPQ

				buffptr->Data[txlen] = 0;		//  Null terminate

				STREAM->BytesTXed += txlen;
				WritetoTrace(TNC, buffptr->Data, txlen);

				// Always add to stored data and set timer. If it expires send message

				if (TNC->VARACMsg == 0)
				{
					TNC->VARACMsg = zalloc(4096);
					TNC->VARACSize = 4096;
				}
				else
				{
					if (strlen(TNC->VARACMsg) + txlen >= (TNC->VARACSize - 10))
					{
						TNC->VARACSize += 4096;
						TNC->VARACMsg = realloc(TNC->VARACMsg, TNC->VARACSize);
					}
				}

				strcat(TNC->VARACMsg, buffptr->Data);

				TNC->VarACTimer = 10;		// One second
				return 0;
			}


	
			else
				memcpy(txbuff, buffptr->Data, txlen);
	
			bytes = VARASendData(TNC, &txbuff[0], txlen);
			STREAM->BytesTXed += bytes;
			ReleaseBuffer(buffptr);
		}


		if (TNC->WINMORtoBPQ_Q != 0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen = buffptr->Len;

			buff->PORT = 0;						// Compatibility with Kam Driver
			buff->PID = 0xf0;
			memcpy(&buff->L2DATA[0], buffptr->Data, datalen);
			
			datalen += sizeof(void *) + 4;
			PutLengthinBuffer(buff, (int)datalen);

			ReleaseBuffer(buffptr);

			return (1);
		}

		return (0);

	case 2:				// send

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			PMSGWITHLEN buffptr = GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr->Len = sprintf(buffptr->Data,"No Connection to VARA TNC\r");

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
		{
			PMSGWITHLEN buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
			txlen = (int)buffptr->Len;
			memcpy(txbuff, buffptr->Data, txlen);
			bytes=send(TNC->TCPDataSock, buff->L2DATA, txlen, 0);
			STREAM->BytesTXed += bytes;
			WritetoTrace(TNC, txbuff, txlen);
			ReleaseBuffer(buffptr);
		}
		
		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}


		txlen = GetLengthfromBuffer(buff) - (MSGHDDRLEN + 1);		// 1 as no PID

		if (TNC->Streams[0].Connected)
		{
			unsigned char txbuff[512];

			STREAM->PacketsSent++;

			if(TNC->VaraACMode == 0 && TNC->VaraModeSet == 1)
			{
				// Normal Send

				memcpy(txbuff, buff->L2DATA, txlen);

				bytes=send(TNC->TCPDataSock, txbuff, txlen, 0);
				STREAM->BytesTXed += bytes;
				WritetoTrace(TNC, buff->L2DATA, txlen);
				return 0;
			}

			// Send in varac format - len space data. No cr on end, but is implied

			// 5 Hello

			// I think we have to send a whole message (something terminated with a new line)
			// may need to combine packets. Also I think we need to combine seqential sends
			// (eg CTEXT and SID)


			buff->L2DATA[txlen] = 0;		//  Null terminate

			STREAM->BytesTXed += txlen;
			WritetoTrace(TNC, buff->L2DATA, txlen);
		
			// Always add to stored data and set timer. If it expires send message

			if (TNC->VARACMsg == 0)
			{
				TNC->VARACMsg = zalloc(4096);
				TNC->VARACSize = 4096;
			}
			else
			{
				if (strlen(TNC->VARACMsg) + txlen >= (TNC->VARACSize - 10))
				{
					TNC->VARACSize += 4096;
					TNC->VARACMsg = realloc(TNC->VARACMsg, TNC->VARACSize);
				}
			}

			strcat(TNC->VARACMsg, buff->L2DATA);

			TNC->VarACTimer = 10;		// One second
			return 0;
		}
		else
		{
			if (_memicmp(buff->L2DATA, "D\r", 2) == 0 || _memicmp(buff->L2DATA, "BYE\r", 4) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			// See if Local command (eg RADIO)

			if (_memicmp(buff->L2DATA, "RADIO ", 6) == 0)
			{
				sprintf(buff->L2DATA, "%d %s", TNC->Port, &buff->L2DATA[6]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK, buff->L2DATA))
				{
				}
				else
				{
					PMSGWITHLEN buffptr = GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr->Len = sprintf(buffptr->Data, "%s", buff->L2DATA);
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(buff->L2DATA, "OVERRIDEBUSY", 12) == 0)
			{
				PMSGWITHLEN buffptr = GetBuff();

				TNC->OverrideBusy = TRUE;

				if (buffptr)
				{
					buffptr->Len = sprintf(buffptr->Data, "VARA} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}

				return 0;

			}

			if (_memicmp(&buff->L2DATA[0], "SessionTimeLimit", 16) == 0)
			{
				if (buff->L2DATA[16] != 13)
				{					
					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

					TNC->SessionTimeLimit = atoi(&buff->L2DATA[16]) * 60;

					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "VARA} OK\r");
						C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					}
					return 0;
				}
			}


			if (_memicmp(&buff->L2DATA[0], "CODEC TRUE", 9) == 0)
				TNC->StartSent = TRUE;

			if (_memicmp(&buff->L2DATA[0], "BW2300", 6) == 0)
			{
				TNC->ARDOPCurrentMode[0] = 'W';			// Save current state for scanning
				TNC->WL2KMode = 50;
			}

			if (_memicmp(&buff->L2DATA[0], "BW500", 5) == 0)
			{
				TNC->ARDOPCurrentMode[0] = 'N';
				TNC->WL2KMode = 53;
			}

			if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			// See if a Connect Command. If so set Connecting

			if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80];
				char * ptr = strchr(&buff->L2DATA[2], 13);

				if (ptr)
					*ptr = 0;

				_strupr(&buff->L2DATA[2]);

				sprintf(Connect, "CONNECT %s %s\r", TNC->Streams[0].MyCall, &buff->L2DATA[2]);

				// Need to set connecting here as if we delay for busy we may incorrectly process OK response

				TNC->Streams[0].Connecting = TRUE;

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

				VARASendCommand(TNC, Connect, TRUE);
				TNC->Streams[0].ConnectTime = time(NULL); 


				memset(TNC->Streams[0].RemoteCall, 0, 10);
				strcpy(TNC->Streams[0].RemoteCall, &buff->L2DATA[2]);

				sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			}
			else
			{
				buff->L2DATA[(txlen++) - 1] = 13;
				buff->L2DATA[(txlen) - 1] = 0;
				VARASendCommand(TNC, &buff->L2DATA[0], TRUE);
			}
		}
		return (0);

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)
	
		if (TNC->Streams[0].Attached == 0)
			return TNC->CONNECTED << 8 | 1;

		return (TNC->CONNECTED << 8 | TNC->Streams[0].Disconnecting << 15);		// OK


	case 4:				// reinit

		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->TCPSock);

		if (TNC->PID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
			RestartTNC(TNC);
		}
		return 0;

	case 5:				// Close

		if (TNC->CONNECTED)
		{
			if (TNC->Streams[0].Connected)
				VARASendCommand(TNC, "ABORT\r", TRUE);
//			GetSemaphore(&Semaphore, 52);
//			VARASendCommand(TNC, "CLOSE", FALSE);
//			FreeSemaphore(&Semaphore);
			Sleep(100);
		}

		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->TCPSock);
		if (TNC->WeStartedTNC)
			KillTNC(TNC);
	
		return 0;


	case 6:				// Scan Stop Interface

		Param = (size_t)buff;

		if (Param == 2)		// Check  Permission (shouldn't happen)
		{
			Debugprintf("Scan Check Permission called on VARA");
			return 1;		// OK to change
		}

		if (!TNC->CONNECTED)
			return 0;		// No connection so no interlock
	
		if (Param == 1)		// Request Permission
		{
			if (TNC->ConnectPending == 0 && TNC->PTTState == 0)
			{
				VARASendCommand(TNC, "LISTEN OFF\r", TRUE);
				TNC->GavePermission = TRUE;
				return 0;	// OK to Change
			}

			if (TNC->ConnectPending)
				TNC->ConnectPending--;		// Time out if set too long

			if (!TNC->ConnectPending)
				return 0;	// OK to Change

			return TRUE;
		}

		if (Param == 3)		// Release  Permission
		{
			if (TNC->GavePermission)
			{
				TNC->GavePermission = FALSE;
				if (TNC->ARDOPCurrentMode[0] != 'S')	// Skip
					VARASendCommand(TNC, "LISTEN ON\r", TRUE);
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;

		if (Scan->VARAMode != TNC->ARDOPCurrentMode[0])
		{
			// Mode changed
	
			if (TNC->ARDOPCurrentMode[0] == 'S')
			{
				VARASendCommand(TNC, "LISTEN ON\r", TRUE);
			}
	
			if (Scan->VARAMode == 'W')		// Set Wide Mode
			{
				VARASendCommand(TNC, "BW2300\r", TRUE);
				TNC->WL2KMode = 50;
			}
			if (Scan->VARAMode == 'T')		// Set Wide Mode
			{
				VARASendCommand(TNC, "BW2750\r", TRUE);
				TNC->WL2KMode = 54;
			}
			else if (Scan->VARAMode == 'N')		// Set Narrow Mode
			{
				VARASendCommand(TNC, "BW500\r", TRUE);
				TNC->WL2KMode = 53;
			}
			else if (Scan->VARAMode == 'S')		// Skip
			{
				VARASendCommand(TNC, "LISTEN OFF\r", TRUE);			
			}
	
			TNC->ARDOPCurrentMode[0] = Scan->VARAMode;
		}
		return 0;
	}
	return 0;
}

void CountRestarts(struct TNCINFO * TNC)
{
	struct tm * tm;
	char Time[80];

	TNC->Restarts++;
	TNC->LastRestart = time(NULL);

	tm = gmtime(&TNC->LastRestart);	

	sprintf_s(Time, sizeof(Time),"%04d/%02d/%02d %02d:%02dZ",
		tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	//MySetWindowText(TNC->xIDC_RESTARTTIME, Time);
	//strcpy(TNC->WEB_RESTARTTIME, Time);

	sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
	MySetWindowText(TNC->xIDC_RESTARTS, Time);
	strcpy(TNC->WEB_RESTARTS, Time);
}
/*
char WebProcTemplate[] = "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<link rel='stylesheet' href='webproc.css'>\r\n"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}\r\n"
		"function xxx(data){\r\n"
		"req = new XMLHttpRequest();\r\n"
		"req.open('POST', 'PortAction?%d', true);\r\n"
		"req.send(data);alert(data + ' Sent');}\r\n"
		"</script>"
		"</head><title>%s</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<span class='dropdown'>"
		"<button class='dropbtn'>Actions</button><span style=\"margin-left:120px;font-size:16px\"><b>%s</b></span>"
		"<div class='dropdown-content'>"
		"<a href='javascript:xxx(\"Abort\");'>Abort Session</a>"
		"<a href='javascript:xxx(\"Kill\");'>Kill TNC</a>"
		"<a href='javascript:xxx(\"KillRestart\");'>Kill and Restart TNC</a>"
		"</div></span>";
*/
char WebProcTemplate[] = "<html><meta http-equiv=expires content=0>"
		"<link rel='stylesheet' href='webproc.css'>\r\n"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}\r\n"
		"function xxx(data){\r\n"
		"req = new XMLHttpRequest();\r\n"
		"req.open('POST', 'PortAction?%d', true);\r\n"
		"req.send(data);alert(data + ' Sent');}\r\n"
		"function yyy(data){\r\n"
		"req = new XMLHttpRequest();\r\n"
		"req.open('POST', 'freqOffset?%d', true);\r\n"
		"req.send(data);}\r\n"
		"myInt = setInterval('Refresh()', 15000 );\n"
		"function Refresh( )\n"
		"{location.reload()}\n"
		"</script>\r\n"
		"</head><title>%s</title></head><body id=Text onload=\"ScrollOutput()\">\r\n"
		"<h2 style=\"margin-bottom: 0.2em; text-align:center\">%s</h2>";

char Menubit[] = "<span class='dropdown' style=\"position: absolute; left: 10;top: 12;\">"
		"<button class='dropbtn'>Actions</button>\r\n"
		"<span class='dropdown-content'>"
		"<a href='javascript:xxx(\"Abort\");'>Abort Session</a>"
		"<a href='javascript:xxx(\"Kill\");'>Kill TNC</a>"
		"<a href='javascript:xxx(\"KillRestart\");'>Kill and Restart TNC</a>"
		"</span></span>";

char sliderBit[] = "<span style=\"position: absolute; left: 380;top: 2;\"> TX Offset <span id='val'>%d</span></span>"
		"<span style=\"position: absolute; left: 350;top: 22;\"><input type='range' min=-200 max=200 value=%d class='slider' id='myRange'></span>\r\n"
		"<script>\r\n"
		"var slider = document.getElementById('myRange');"
		"var output = document.getElementById('val');"
		"slider.oninput = function() {output.innerHTML = this.value;}\r\n"
		"slider.onmouseout = function() {myInt = setInterval('Refresh()', 15000 );"
		"output.innerHTML = this.value;yyy(this.value);}\r\n"
		"slider.onmouseover = function() {clearInterval(myInt)};"
		"</script>\r\n";

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, WebProcTemplate, TNC->Port, TNC->Port, "VARA Status", "VARA Status");

	if (LOCAL)
		Len += sprintf(&Buff[Len], Menubit, TNC->TXOffset, TNC->TXOffset);

	if (TNC->TXFreq)
		Len += sprintf(&Buff[Len], sliderBit, TNC->TXOffset, TNC->TXOffset);

	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>TNC State</td><td>%s</td></tr>", TNC->WEB_TNCSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Mode</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "<tr><td>Channel State</td><td>%s</td></tr>", TNC->WEB_CHANSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>S/N</td><td>%s</td></tr>", TNC->WEB_PROTOSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Traffic</td><td>%s</td></tr>", TNC->WEB_TRAFFIC);
//	Len += sprintf(&Buff[Len], "<tr><td>TNC Restarts</td><td></td></tr>", TNC->WEB_RESTARTS);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}

VOID VARASuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC)
{
	TNC->PortRecord->PORTCONTROL.PortSuspended = TRUE;
	VARASendCommand(TNC, "LISTEN OFF\r", TRUE);
	strcpy(TNC->WEB_TNCSTATE, "Interlocked");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

}

VOID VARAReleasePort(struct TNCINFO * TNC)
{
	TNC->PortRecord->PORTCONTROL.PortSuspended = FALSE;
	VARASendCommand(TNC, "LISTEN ON\r", TRUE);
	strcpy(TNC->WEB_TNCSTATE, "Free");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
}


void * VARAExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	int line;
	APPLCALLS * APPL;
	struct TNCINFO * TNC;
	int AuxCount = 0;
	char Appl[11];
	char * TempScript;
	struct PORTCONTROL * PORT = &PortEntry->PORTCONTROL;
	//
	//	Will be called once for each VARA port 
	//
	//	The Socket to connect to is in IOBASE
	//

	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC->AutoStartDelay == 0)
		TNC->AutoStartDelay = 2000;

	if (TNC == NULL)
	{
		// Not defined in Config file

		sprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return ExtProc;
	}
	
	TNC->Port = port;

	TNC->ARDOPBuffer = malloc(8192);
	TNC->ARDOPDataBuffer = malloc(8192);

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = 1;

	TNC->Hardware = H_VARA;

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
	PortEntry->MAXHOSTMODESESSIONS = 1;	
	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only

	PortEntry->PORTCONTROL.UICAPABLE = FALSE;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = VARASuspendPort;
	TNC->ReleasePortProc = VARAReleasePort;

	PortEntry->PORTCONTROL.PORTSTARTCODE = VARAStartPort;
	PortEntry->PORTCONTROL.PORTSTOPCODE = VARAStopPort;

	TNC->ModemCentre = 1500;				// WINMOR is always 1500 Offset

	if (TNC->NRNeighbour)
	{
		// NETROM over VARA Link

		TNC->NetRomMode = 1;
		TNC->LISTENCALLS = MYNETROMCALL;
		PORT->PortNoKeepAlive = 1;
		TNC->DummyLink = zalloc(sizeof(struct _LINKTABLE));
		TNC->DummyLink->LINKPORT = &TNC->PortRecord->PORTCONTROL;
	}
	else
		PORT->PORTQUALITY = 0;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = zalloc(1000);

//	strcat(TempScript, "ROBUST False\r");

	// Set MYCALL(S)

	if (TNC->LISTENCALLS)
	{
		sprintf(Msg, "MYCALL %s", TNC->LISTENCALLS);
	}
	else
	{
		sprintf(Msg, "MYCALL %s", TNC->NodeCall);

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
					//				*ptr++ = ' ';
					*ptr = 0;
				}
				strcat(Msg, " ");
				strcat(Msg, Appl);
				AuxCount++;
				if (AuxCount == 4)			// Max 5 in MYCALL
					break;
			}
		}
	}

	strcat(Msg, "\r");

	strcat(TempScript, Msg);

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;


	strcat(TNC->InitScript,"LISTEN ON\r");	

	strcpy(TNC->CurrentMYC, TNC->NodeCall);

	if (TNC->WL2K == NULL)
		if (PortEntry->PORTCONTROL.WL2KInfo.RMSCall[0])			// Alrerady decoded
			TNC->WL2K = &PortEntry->PORTCONTROL.WL2KInfo;

	// if mode hasn't been set explicitly or via WL2KREPORT set to HF Wide mode (BW2300)

	if (TNC->DefaultMode == 0)
	{
		if (TNC->WL2K && TNC->WL2K->mode >= 50 && TNC->WL2K->mode <= 53)		// A VARA Mode
			TNC->DefaultMode = TNC->WL2KMode = TNC->WL2K->mode;
		else
			TNC->DefaultMode = TNC->WL2KMode = 50;		// Default to 2300
	}

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


#ifndef LINBPQ

	line = 6;

	if (TNC->TXFreq)
	{
		CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow + 22, PacWndProc, 550, 450, ForcedClose);

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
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	line += 22;
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,line,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,520,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,line,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
	line += 22;
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,line,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,line,144,20, TNC->hDlg, NULL, hInstance, NULL);
 
 	line += 22;
	CreateWindowEx(0, "STATIC", "S/N", WS_CHILD | WS_VISIBLE,10,line,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,116,line,374,20 , TNC->hDlg, NULL, hInstance, NULL);
 
	line += 22;
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,line,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,line,116,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,line,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,line,20,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,line,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTTIME = CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,line,200,20, TNC->hDlg, NULL, hInstance, NULL);

	line += 22;
	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,line,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill VARA TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart VARA TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after failed Connection");	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);
	AppendMenu(TNC->hMenu, MF_STRING, ARDOP_ABORT, "Abort Current Session");

	MoveWindows(TNC);
#endif
	Consoleprintf("VARA Host %s %d", TNC->HostName, htons(TNC->destaddr.sin_port));

	ConnecttoVARA(port);

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}

int ConnecttoVARA(int port)
{
	if (TNCInfo[port]->CONNECTING || TNCInfo[port]->PortRecord->PORTCONTROL.PortStopped)
		return 0;

	_beginthread(VARAThread, 0, (void *)(size_t)port);

	return 0;
}

VOID VARAThread(void * portptr)
{
	// Opens sockets and looks for data on control and data sockets.
	
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
	char * ptr1;
	char * ptr2;
	PMSGWITHLEN buffptr;

	if (TNC->HostName == NULL)
		return;

	TNC->BusyFlags = 0;

	TNC->CONNECTING = TRUE;

	Sleep(3000);		// Allow init to complete 

	if (TNCInfo[port]->PortRecord->PORTCONTROL.PortStopped)
	{
		TNC->CONNECTING = FALSE;
		return;
	}


//	printf("Starting VARA Thread\n");

// if on Windows and Localhost see if TNC is running

#ifdef WIN32

	if (strcmp(TNC->HostName, "127.0.0.1") == 0)
	{
		// can only check if running on local host
		
		TNC->PID = GetListeningPortsPID(TNC->destaddr.sin_port);
		
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
		Consoleprintf("Trying to (re)start TNC %s", TNC->ProgramPath);

		if (RestartTNC(TNC))
			CountRestarts(TNC);

		Sleep(TNC->AutoStartDelay);
	}

TNCRunning:

	if (TNC->Alerted == FALSE)
	{
		sprintf(TNC->WEB_COMMSSTATE, "Connecting to TNC");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		HostEnt = gethostbyname (TNC->HostName);
		 
		 if (!HostEnt)
		 {
		 	TNC->CONNECTING = FALSE;
			sprintf(Msg, "Resolve Failed for VARA socket - error code = %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);
			return;			// Resolve failed
		 }
		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//	closesocket(TNC->TCPSock);
//	closesocket(TNC->TCPDataSock);

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for VARA socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}

	TNC->TCPDataSock = socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for VARA Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
		closesocket(TNC->TCPSock);

  	 	return; 
	}

	setsockopt(TNC->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(TNC->TCPDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
//	setsockopt(TNC->TCPDataSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

//	printf("Trying to connect to VARA TNC\n");

	if (connect(TNC->TCPSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//	Connected successful

		goto VConnected;

	}

	if (TNC->Alerted == FALSE)
	{
		err=WSAGetLastError();
		sprintf(Msg, "Connect Failed for VARA socket - error code = %d Port %d\n",
				err, htons(TNC->destaddr.sin_port));

		WritetoConsole(Msg);
		TNC->Alerted = TRUE;

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}

//	printf("VARA Connect failed\n");

	closesocket(TNC->TCPSock);
	closesocket(TNC->TCPDataSock);

	TNC->TCPSock = 0;
	TNC->CONNECTING = FALSE;
	return;

VConnected:

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
   			i=sprintf(Msg, "Connect Failed for VARA Data socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);
		TNC->TCPSock = 0;
	 	TNC->CONNECTING = FALSE;
			
		RestartTNC(TNC);
		return;
	}

	Sleep(1000);

	TNC->LastFreq = 0;

 	TNC->CONNECTING = FALSE;
	TNC->CONNECTED = TRUE;
	TNC->BusyFlags = 0;
	TNC->InputLen = 0;

	// Send INIT script

	// VARA needs each command in a separate send

	ptr1 = &TNC->InitScript[0];

	// We should wait for first RDY. Cheat by queueing a null command

	GetSemaphore(&Semaphore, 52);

	while(TNC->BPQtoWINMOR_Q)
	{
		buffptr = Q_REM(&TNC->BPQtoWINMOR_Q);

		if (buffptr)
			ReleaseBuffer(buffptr);
	}


	while (ptr1 && ptr1[0])
	{
		unsigned char c;
		
		ptr2 = strchr(ptr1, 13);
		
		if (ptr2)
		{
			c = *(ptr2 + 1);		// Save next char
			*(ptr2 + 1) = 0;		// Terminate string
		}
		VARASendCommand(TNC, ptr1, TRUE);

		if (ptr2)
			*(1 + ptr2++) = c;		// Put char back 

		ptr1 = ptr2;
	}
	
	TNC->Alerted = TRUE;

	sprintf(TNC->WEB_COMMSSTATE, "Connected to VARA TNC");		
	MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

	FreeSemaphore(&Semaphore);

	sprintf(Msg, "Connected to VARA TNC Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);


	#ifndef LINBPQ
//	FreeSemaphore(&Semaphore);
	Sleep(1000);		// Give VARA time to update Window title
	EnumWindows(EnumVARAWindowsProc, (LPARAM)TNC);
//	GetSemaphore(&Semaphore, 52);
#endif

	while (TNC->CONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TNC->TCPSock,&readfs);
		FD_SET(TNC->TCPSock,&errorfs);

		if (TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&readfs);
			
//		FD_ZERO(&writefs);

//		if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->TCPDataSock,&writefs);	// Need notification of busy clearing
		
		if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&errorfs);

		timeout.tv_sec = 90;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select((int)TNC->TCPDataSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			Debugprintf("VARA Select failed %d ", WSAGetLastError());
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TNC->TCPSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				VARAProcessReceivedControl(TNC);
				FreeSemaphore(&Semaphore);
			}
								
			if (FD_ISSET(TNC->TCPDataSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				VARAProcessReceivedData(TNC);
				FreeSemaphore(&Semaphore);
			}

			if (FD_ISSET(TNC->TCPSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "VARA Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;
				TNC->ConnectPending = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->TCPSock);
				closesocket(TNC->TCPDataSock);
				TNC->TCPSock = 0;
				break;
			}

			if (FD_ISSET(TNC->TCPDataSock, &errorfs))
			{
				sprintf(Msg, "VARA Connection lost for Port %d\r\n", TNC->Port);
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
				break;
			}
			continue;
		}
		else
		{
			// 60 secs without data. Shouldn't happen

			continue;

			sprintf(Msg, "VARA No Data Timeout Port %d\r\n", TNC->Port);
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

//			GetSemaphore(&Semaphore, 52);
//			VARASendCommand(TNC, "CODEC FALSE", FALSE);
//			FreeSemaphore(&Semaphore);

			Sleep(100);
			shutdown(TNC->TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->TCPSock);

			Sleep(100);
			shutdown(TNC->TCPDataSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->TCPDataSock);

//	if (TNC->PID && TNC->WeStartedTNC)
//	{
//		KillTNC(TNC);
//
			break;
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

	sprintf(Msg, "VARA Thread Terminated Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);
}


VOID VARAProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	Buffer[MsgLen - 1] = 0;		// Remove CR

	TNC->TimeSinceLast = 0;
		
	if (_memicmp(Buffer, "PTT ON", 6) == 0)
	{
//		Debugprintf("PTT On");

		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		TNC->PTTonTime = GetTickCount();

		// Cancel Busy timer (stats include ptt on time in port active)

		if (TNC->BusyonTime)
		{
			TNC->BusyActivemS += (GetTickCount() - TNC->BusyonTime);
			TNC->BusyonTime = 0;
		}
		if (TNC->PTTMode)
			Rig_PTT(TNC, TRUE);

		return;
	}

	if (_memicmp(Buffer, "PTT OFF", 6) == 0)
	{
//		Debugprintf("PTT Off");

		if (TNC->PTTonTime)
		{
			TNC->PTTActivemS += (GetTickCount() - TNC->PTTonTime);
			TNC->PTTonTime = 0;
		}

		if (TNC->PTTMode)
			Rig_PTT(TNC, FALSE);

		return;
	}

	if (_memicmp(Buffer, "SN ", 3) == 0)
	{
		strcpy(TNC->WEB_PROTOSTATE,  &Buffer[3]);
		MySetWindowText(TNC->xIDC_PROTOSTATE, TNC->WEB_PROTOSTATE);

		TNC->SNR = atof(&Buffer[3]);
		return;
	}

	if (_stricmp(Buffer, "BUSY ON") == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		TNC->BusyonTime = GetTickCount();

		MySetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		strcpy(TNC->WEB_CHANSTATE, "Busy");

		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}

	if (_stricmp(Buffer, "BUSY OFF") == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->BusyHold)
			strcpy(TNC->WEB_CHANSTATE, "BusyHold");
		else
			strcpy(TNC->WEB_CHANSTATE, "Clear");

		if (TNC->BusyonTime)
		{
			TNC->BusyActivemS += (GetTickCount() - TNC->BusyonTime);
			TNC->BusyonTime = 0;
		}


		MySetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}


	if (_memicmp(&Buffer[0], "PENDING", 7) == 0)	// Save Pending state for scan control
	{
		TNC->ConnectPending = 6;				// Time out after 6 Scanintervals
		Debugprintf(Buffer);
//		WritetoTrace(TNC, Buffer, MsgLen - 1);
		return;
	}

	if (_memicmp(&Buffer[0], "CANCELPENDING", 13) == 0)
	{
		TNC->ConnectPending = FALSE;
		Debugprintf(Buffer);

		// If a callsign is present it is the calling station - add to MH

		if (TNC->SeenCancelPending == 0)
		{
			WritetoTrace(TNC, Buffer, MsgLen - 1);
			TNC->SeenCancelPending = 1;
		}

		if (Buffer[13] == ' ')
			UpdateMH(TNC, &Buffer[14], '!', 'I');

		return;
	}

	TNC->SeenCancelPending = 0;

	if (strcmp(Buffer, "OK") == 0)
	{
		// Need to discard response to LISTEN OFF after attach

		if (TNC->DiscardNextOK)
		{
			TNC->DiscardNextOK = 0;
			return;
		}

		if (TNC->Streams[0].Connecting == TRUE)
			return;		// Discard response or it will mess up connect scripts
	}

	if (_memicmp(Buffer, "BUFFER", 6) == 0)
	{
		Debugprintf(Buffer);

		sscanf(&Buffer[7], "%d", &TNC->Streams[0].BytesOutstanding);

		if (TNC->Streams[0].BytesOutstanding == 0)
		{
			// all sent
			
			if (TNC->Streams[0].Disconnecting)						// Disconnect when all sent
			{
				if (STREAM->NeedDisc == 0)
					STREAM->NeedDisc = 60;								// 6 secs
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

		sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %s",
			STREAM->BytesTXed, STREAM->BytesRXed, &Buffer[7]);
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

		if (TNC->VARACMsg)
			free(TNC->VARACMsg);
		
		TNC->VaraACMode = 0;
		TNC->VARACMsg = 0;
		TNC->VARACSize = 0;
		if (TNC->VaraACAllowed == 0)
			TNC->VaraModeSet = 1;			// definitly not varaac
		else
			TNC->VaraModeSet = 0;			// Don't know yet

		strcpy(TNC->WEB_MODE, "");

		if (strstr(Buffer, "2300"))
		{
			Speed = 50;
			strcpy(TNC->WEB_MODE, "2300");
		}
		else if (strstr(Buffer, "NARROW"))
		{
			Speed = 51;
			strcpy(TNC->WEB_MODE, "NARROW");
		}
		else if (strstr(Buffer, "WIDE"))
		{
			Speed = 52;
			strcpy(TNC->WEB_MODE, "WIDE");
		}
		else if (strstr(Buffer, "500"))
		{
			Speed = 53;
			strcpy(TNC->WEB_MODE, "500");
		}
		else if (strstr(Buffer, "2750"))
		{
			Speed = 54;
			strcpy(TNC->WEB_MODE, "2750");
		}

		MySetWindowText(TNC->xIDC_MODE, TNC->WEB_MODE);
		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		// Get Target Call

		ptr = strchr(&Buffer[10], ' ');	

		if (ptr)
		{
			memcpy(TNC->TargetCall, ++ptr, 10);
			strlop(TNC->TargetCall, ' ');
		}
	
		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0 || (TNC->NetRomMode && STREAM->Connecting == 0))
		{
			TRANSPORTENTRY * SESS;
			
			// Incoming Connect

			// Stop other ports in same group

			SuspendOtherPorts(TNC);
						
			TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

			// Only allow VarAC mode for incomming sessions

			ProcessIncommingConnectEx(TNC, Call, 0, (TNC->NetRomMode == 0), TRUE);
				
			SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

			if (Speed)
				SESS->Mode = Speed;
			else
				SESS->Mode = TNC->WL2KMode;

			TNC->ConnectPending = FALSE;

			if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
				SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
				if (SESS->Frequency == 1500)
				{
					// try to get from WL2K record

					if (WL2K)
						SESS->Frequency = WL2K->Freq;
				}
			}
			else
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->TargetCall);
				if (WL2K)
					SESS->Frequency = WL2K->Freq;
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
					Rig_Command((TRANSPORTENTRY *) -1, Status);
					Debugprintf("VARA Call from %s rejected", Call);
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
						Rig_Command((TRANSPORTENTRY *) -1, Status);
						Debugprintf("VARA Call from %s not in ValidCalls - rejected", Call);
						return;
					}
				}
			}

			if (TNC->NetRomMode)
			{
				// send any queued data

				int bytes;

				if (TNC->NetRomTxLen)
				{

					STREAM->PacketsSent++;

					bytes = send(TNC->TCPDataSock, TNC->NetRomTxBuffer, TNC->NetRomTxLen, 0);
					STREAM->BytesTXed += TNC->NetRomTxLen;

					free(TNC->NetRomTxBuffer);
					TNC->NetRomTxBuffer = NULL;
					TNC->NetRomTxLen = 0;
				}
				return;
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

				// if SendTandRtoRelay set and Appl is RMS change to RELAY

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
					memcpy(buffptr->Data, Buffer, MsgLen);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
		
					TNC->SwallowSignon = TRUE;

					// Save Appl Call in case needed for 

				}
				else
				{
					char Msg[] = "Application not available\r";
					
					// Send a Message, then a disconenct

					// Send CTEXT First


					if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
					{
						PMSGWITHLEN buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
						int txlen = (int)buffptr->Len;
						VARASendData(TNC, buffptr->Data, txlen);
						ReleaseBuffer(buffptr);
					}
				
					VARASendData(TNC, Msg, (int)strlen(Msg));
					STREAM->NeedDisc = 100;	// 10 secs
				}
			}

			strcpy(STREAM->MyCall, TNC->TargetCall);
			return;
		}
		else
		{
			// Connect Complete

			char Reply[80];
			int ReplyLen;
			

			if (TNC->NetRomMode)
			{
				// send any queued data

				int bytes;

				if (TNC->NetRomTxLen)
				{
					STREAM->PacketsSent++;

					bytes = send(TNC->TCPDataSock, TNC->NetRomTxBuffer, TNC->NetRomTxLen, 0);
					STREAM->BytesTXed += TNC->NetRomTxLen;
					free(TNC->NetRomTxBuffer);
					TNC->NetRomTxBuffer = NULL;
					TNC->NetRomTxLen = 0;
				}
			}
			else
			{
				TNC->VaraACMode = 0;
				TNC->VaraModeSet = 1;		// Don't allow connect to VaraAC

				buffptr = GetBuff();

				if (buffptr == 0)
					return;			// No buffers, so ignore

				ReplyLen = sprintf(Reply, "*** Connected to %s\r", TNC->TargetCall);

				buffptr->Len = ReplyLen;
				memcpy(buffptr->Data, Reply, ReplyLen);

				C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			}

			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG && TNC->RIG->Valchar[0])
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s",  TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
			else
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			UpdateMH(TNC, TNC->TargetCall, '+', 'O');
			return;
		}
	}


	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0)
	{
		Debugprintf(Buffer);

		TNC->ConnectPending = FALSE;			// Cancel Scan Lock

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

			buffptr->Len = sprintf(buffptr->Data, "VARA} Failure with %s\r", STREAM->RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			if (TNC->RestartAfterFailure)
			{
				if (TNC->ProgramPath)
					KillTNC(TNC);
			}

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 1);

		// Release Session

		if (STREAM->Connected && STREAM->ConnectTime)
		{
			// Create a traffic record
		
			hookL4SessionDeleted(TNC, STREAM);
		}


		STREAM->Connecting = FALSE;
		STREAM->Connected = FALSE;		// Back to Command Mode
		STREAM->ReportDISC = TRUE;		// Tell Node

		if (STREAM->Disconnecting)		// 
			VARAReleaseTNC(TNC);

		STREAM->Disconnecting = FALSE;

		return;
	}


	if (_memicmp(Buffer, "IAMALIVE", 8) == 0)
	{
//		strcat(Buffer, "\r\n");
//		WritetoTrace(TNC, Buffer, strlen(Buffer));
		return;
	}

//	Debugprintf(Buffer);

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 3);
//		return;
	}

	if (_memicmp(Buffer, "REGISTERED", 9) == 0)
	{
		strcat(Buffer, "\r");
		WritetoTrace(TNC, Buffer, (int)strlen(Buffer));
		return;
	}

	if (_memicmp(Buffer, "ENCRYPTION ", 11) == 0)
	{
		strcat(Buffer, "\r");
		WritetoTrace(TNC, Buffer, (int)strlen(Buffer));
		return;
	}

	if (_memicmp(Buffer, "MISSING SOUNDCARD", 17) == 0)
	{
		strcat(Buffer, "\r");
		WritetoTrace(TNC, Buffer, (int)strlen(Buffer));
		return;
	}

	// Others should be responses to commands

	//	Return others to user (if attached but not connected)

	if (TNC->Streams[0].Attached == 0)
		return;

	if (TNC->Streams[0].Connected)
		return;

	if (MsgLen > 200)
		MsgLen = 200;

	buffptr = GetBuff();

	if (buffptr == 0)
	{
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf(buffptr->Data, "VARA} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
}



VOID VARAProcessReceivedData(struct TNCINFO * TNC)
{
	int InputLen;

	InputLen = recv(TNC->TCPDataSock, &TNC->ARDOPDataBuffer[TNC->DataInputLen], 8192 - TNC->DataInputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
//		closesocket(TNC->TCPSock);
	
		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		TNC->CONNECTED = FALSE;
		TNC->Alerted = FALSE;

		if (TNC->PTTMode)
			Rig_PTT(TNC, FALSE);			// Make sure PTT is down

		if (TNC->Streams[0].Attached)
			TNC->Streams[0].ReportDISC = TRUE;

		TNC->Streams[0].Disconnecting = FALSE;

		closesocket(TNC->TCPDataSock);
		TNC->TCPSock = 0;

		return;					
	}

	TNC->DataInputLen += InputLen;

	if (TNC->NetRomMode)
	{
		// Unpack KISS frames from data stream

		unsigned char c;
		int n = 0;
		int Len = TNC->DataInputLen;
		unsigned char KISSBuffer[600];
		int KissLen = 0;

		while (Len)
		{
			Len--;

			c = TNC->ARDOPDataBuffer[n++];

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

					if (KissLen == 0)
					{
						// Start of Message.

						continue;
					}

					// Have a complete KISS frame - remove from buffer and process

					if (KISSBuffer[0] == 255)
					{
						// NODE Message

						MESSAGE * Msg = GetBuff();

						if (Msg)
						{
							// Set up header

							Msg->LENGTH = KissLen + (Msg->L2DATA - (unsigned char *)Msg);
							memcpy(Msg->L2DATA, KISSBuffer, KissLen);
							ConvToAX25(TNC->NRNeighbour, Msg->ORIGIN);
							memcpy(Msg->DEST, NETROMCALL, 7);

							PROCESSNODEMESSAGE(Msg, &TNC->PortRecord->PORTCONTROL);

							Msg->PID = 0xcf;
							Msg->PORT = TNC->Port | 0x80;
							Msg->CTL = 3;
							Msg->DEST[6] |= 0x80;			// set Command Bit
							Msg->ORIGIN[6] |= 1;		// set end of address
							time(&Msg->Timestamp);
							BPQTRACE(Msg, FALSE);
						}
					}
					else
					{
						// Netrom Message

						L3MESSAGEBUFFER * L3MSG = GetBuff();
						MESSAGE * Buffer = GetBuff();

						if (L3MSG)
						{
							// Set up header

							L3MSG->LENGTH = KissLen + (L3MSG->L3SRCE - (unsigned char *)L3MSG);
							memcpy(L3MSG->L3SRCE, KISSBuffer, KissLen);
							L3MSG->L3PID = 0xcf;

							// Create copy to pass to monitor
							// To trace we need to reformat as MESSAGE

							Buffer->PID = 0xcf;
							Buffer->PORT = TNC->Port;
							Buffer->CTL = 3;
							Buffer->LENGTH = KissLen + (Buffer->L2DATA - (unsigned char *)Buffer);
							memcpy(Buffer->L2DATA, KISSBuffer, KissLen);

							ConvToAX25(TNC->NRNeighbour, Buffer->DEST);
							memcpy(Buffer->ORIGIN, NETROMCALL, 7);
							Buffer->ORIGIN[6] |= 1;		// set end of address
							Buffer->DEST[6] |= 0x80;			// set Command Bit

							time(&Buffer->Timestamp);
							BPQTRACE(Buffer, FALSE);				// TRACE
							NETROMMSG(TNC->DummyLink, L3MSG);
						}
					}

					if (Len == 0)		// All used
					{
						TNC->DataInputLen = 0;
					}
					else
					{
						memmove(TNC->ARDOPDataBuffer, &TNC->ARDOPDataBuffer[n], Len);
						TNC->DataInputLen = Len;
						KissLen = 0;
						n = 0;
					}

					continue;

				case FESC:

					TNC->ESCFLAG = TRUE;
					continue;

				}
			}

			//	Ok, a normal char

			KISSBuffer[KissLen++] = c;

			if (KissLen > 590)
				KissLen = 0;

		}

		// End of input - if there is stuff left in the input buffer we will add the next block to it

		return;
	}
		
	VARAProcessDataPacket(TNC, TNC->ARDOPDataBuffer, TNC->DataInputLen);
	TNC->DataInputLen = 0;
	return;
}



VOID VARAProcessReceivedControl(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[4096];

	// shouldn't get several messages per packet, as each should need an ack
	// May get message split over packets

	if (TNC->InputLen > 8000)	// Shouldnt have packets longer than this
		TNC->InputLen=0;
				
	InputLen=recv(TNC->TCPSock, &TNC->ARDOPBuffer[TNC->InputLen], 8192 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{		
		closesocket(TNC->TCPSock);

		TNC->TCPSock = 0;

		TNC->CONNECTED = FALSE;

		if (TNC->Streams[0].Connecting || TNC->Streams[0].Connected)
			TNC->Streams[0].ReportDISC = TRUE;

		TNC->Streams[0].Disconnecting = FALSE;

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		return;					
	}

	TNC->InputLen += InputLen;

loop:

	ptr = memchr(TNC->ARDOPBuffer, '\r', TNC->InputLen);

	if (ptr == 0)	//  CR in buffer
		return;		// Wait for it

	ptr2 = &TNC->ARDOPBuffer[TNC->InputLen];

	if ((ptr2 - ptr) == 1)	// CR 
	{
		// Usual Case - single msg in buffer

		VARAProcessResponse(TNC, TNC->ARDOPBuffer, TNC->InputLen);
		TNC->InputLen=0;
		return;
	}
	else
	{
		MsgLen = TNC->InputLen - (int)(ptr2-ptr) + 1;	// Include CR 

		memcpy(Buffer, TNC->ARDOPBuffer, MsgLen);

		VARAProcessResponse(TNC, Buffer, MsgLen);

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



VOID VARAProcessDataPacket(struct TNCINFO * TNC, UCHAR * Data, int Length)
{
	// Info on Data Socket - just packetize and send on
	
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int PacLen = 236;
	PMSGWITHLEN buffptr;
		
	TNC->TimeSinceLast = 0;

	STREAM->BytesRXed += Length;

	Data[Length] = 0;	
//	Debugprintf("VARA: RXD %d bytes", Length);

	sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->BytesTXed, STREAM->BytesRXed,STREAM->BytesOutstanding);
	MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

	// if VARAAC Mode, remove byte count from front and add cr
	// could possibly be longer than buffer size

	if (TNC->VaraModeSet == 0)		// Could be normal or VaraAC
	{
		unsigned char *ptr = memchr(Data, ' ', Length);	// contains a space
		
		if (ptr)
		{
			int ACLen = atoi(Data);
			int lenLen = (ptr - Data) + 1;

			if (ACLen == (Length - lenLen))
				TNC->VaraACMode = 1;	// AC Mode
		}
		TNC->VaraModeSet = 1;		// Know which mode
	}

	if (TNC->VaraACMode)
	{
		char * lenp;
		char * msg;
		int len;

		lenp = Data;
		msg = strlop(lenp, ' ');

		len = atoi(lenp);
		if (len != strlen(msg))
			return;

		msg[len++] = 13;
		msg[len] = 0;

		Length = len;
		memmove(Data, msg, len + 1);

	}

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
				
		memcpy(buffptr->Data, Data, Fraglen);
		
		WritetoTrace(TNC, Data, Fraglen);

		Data += Fraglen;

		buffptr->Len = Fraglen;

		C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
	}
	return;
}

static VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[0].BytesOutstanding == 0)
		VARASendCommand(TNC, "DISCONNECT\r", TRUE);
}

static VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	char Abort[] = "ABORT\r";
		
	VARASendCommand(TNC, Abort, TRUE);
	WritetoTrace(TNC, Abort, 5);
}

static VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	VARAReleaseTNC(TNC);
	TNC->ARDOPCurrentMode[0] = 0;		// Force Mode select on next scan change

	// Also reset mode in case incoming call has changed it

	if (TNC->DefaultMode == 50)
		VARASendCommand(TNC, "BW2300\r", TRUE);
	else if (TNC->DefaultMode == 53)
		VARASendCommand(TNC, "BW500\r", TRUE);
	else if (TNC->DefaultMode == 54)
		VARASendCommand(TNC, "BW2750\r", TRUE);

}


VOID VARASendCommand(struct TNCINFO * TNC, char * Buff, BOOL Queue)
{
	int SentLen;

	if (Buff[0] == 0)		// Terminal Keepalive?
		return;

	if (memcmp(Buff, "LISTEN O", 8) == 0)
		TNC->DiscardNextOK = TRUE;			// Responding to LISTEN  messes up forwarding

	if (TNC->CONNECTED == 0)
		return;

	if (TNC->TCPSock)
	{
		SentLen = send(TNC->TCPSock, Buff, (int)strlen(Buff), 0);
		
		if (SentLen != strlen(Buff))
		{			
			int winerr=WSAGetLastError();
			char ErrMsg[80];
				
			sprintf(ErrMsg, "VARA Write Failed for port %d - error code = %d\r\n", TNC->Port, winerr);
			WritetoConsole(ErrMsg);
							
			closesocket(TNC->TCPSock);
			TNC->TCPSock = 0;		
			TNC->CONNECTED = FALSE;
			return;
		}
	}
	return;
}

int VARASendData(struct TNCINFO * TNC, UCHAR * Buff, int Len)
{
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int bytes=send(TNC->TCPDataSock,(const char FAR *)Buff, Len, 0);
	STREAM->BytesTXed += bytes;
	WritetoTrace(TNC, Buff, Len);
	return bytes;
}

#ifndef LINBPQ

BOOL CALLBACK EnumVARAWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[128];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;
	int n;

	n = GetWindowText(hwnd, wtext, 127);

	if (memcmp(wtext,"VARA", 4) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->PID == ProcessId)
		{
			 // Our Process

			char msg[512];
			char ID[64] = "";
			int i = 29;

			memcpy(ID, TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION, 30);

			while (ID[i] == ' ')
				ID[i--] = 0;

			wtext[n] = 0;
			sprintf (msg, "BPQ %s - %s", ID, wtext);
			SetWindowText(hwnd, msg);
			return FALSE;
		}
	}
	
	return (TRUE);
}
#endif

VOID VARAReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

//	ARDOPChangeMYC(TNC, TNC->NodeCall);

	VARASendCommand(TNC, "LISTEN ON\r", TRUE);

	strcpy(TNC->WEB_TNCSTATE, "Free");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	//	Start Scanner

	if (TNC->DefaultRadioCmd)
	{
		sprintf(TXMsg, "%d %s", TNC->Port, TNC->DefaultRadioCmd);
		Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);
	}

	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);
	Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);

	ReleaseOtherPorts(TNC);
}

void SendVARANetrom(struct TNCINFO * TNC, unsigned char * Data, int Len)
{
	// Check that PID is 0xcf, then just send the data portion of packet

	// We need to delimit packets, and KISS encoding seems as good as any. Also
	// need to buffer to avoid sending lots of small packets and maybe to wait for
	// link to connect

	unsigned char Kiss[600];
	int KissLen;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	if (TNC->CONNECTED == 0)
		return;					// Don't Queue if no connection to TNC
	
	KissLen = KissEncode(Data, Kiss, Len);

	TNC->NetRomTxBuffer = realloc(TNC->NetRomTxBuffer, TNC->NetRomTxLen + KissLen);
	memcpy(&TNC->NetRomTxBuffer[TNC->NetRomTxLen], Kiss, KissLen);
	TNC->NetRomTxLen += KissLen;

	if (STREAM->Connected)
	{
		int bytes;

		STREAM->PacketsSent++;

		bytes = send(TNC->TCPDataSock, TNC->NetRomTxBuffer, TNC->NetRomTxLen, 0);
		STREAM->BytesTXed += TNC->NetRomTxLen;

		free(TNC->NetRomTxBuffer);
		TNC->NetRomTxBuffer = NULL;
		TNC->NetRomTxLen = 0;
		return;
	}

	if (TNC->Streams[0].Connecting == 0 && TNC->Streams[0].Connected == 0)
	{
		// Try to connect to Neighbour

		char Connect[32];

		TNC->NetRomMode = 1;

		sprintf(Connect, "CONNECT %s %s\r", MYNETROMCALL, TNC->NRNeighbour);

		// Need to set connecting here as if we delay for busy we may incorrectly process OK response

		TNC->Streams[0].Connecting = TRUE;

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
				return;
			}
		}
		TNC->OverrideBusy = FALSE;

		VARASendCommand(TNC, Connect, TRUE);
		TNC->Streams[0].ConnectTime = time(NULL); 

		memset(TNC->Streams[0].RemoteCall, 0, 10);
		strcpy(TNC->Streams[0].RemoteCall, MYNETROMCALL);

		sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
	}
}

void SendVARANetromNodes(struct TNCINFO * TNC, MESSAGE *Buffer)
{
	// Check that PID is 0xcf, then just send the data portion of packet

	int Len = Buffer->LENGTH - (Buffer->L2DATA - (unsigned char *)Buffer);

	if (Buffer->PID != 0xcf)
	{
		ReleaseBuffer(Buffer);
		return;
	}

	SendVARANetrom(TNC, Buffer->L2DATA, Len);
	time(&Buffer->Timestamp);

	C_Q_ADD(&TRACE_Q, Buffer);

}

void SendVARANetromMsg(struct TNCINFO * TNC, L3MESSAGEBUFFER * MSG)
{
	MESSAGE * Buffer = (MESSAGE *)MSG;

	int Len = MSG->LENGTH - (MSG->L3SRCE - (unsigned char *)MSG);

	if (MSG->L3PID != 0xcf)
	{
		ReleaseBuffer(MSG);
		return;
	}

	SendVARANetrom(TNC, MSG->L3SRCE, Len);

	memmove(Buffer->L2DATA, MSG->L3SRCE, Len);

	// To trace we need to reformat as MESSAGE

	Buffer->PID = 0xcf;
	Buffer->PORT = TNC->Port | 0x80;
	Buffer->CTL = 3;
	Buffer->LENGTH = Len + (Buffer->L2DATA - (unsigned char *)Buffer);
	ConvToAX25(TNC->NRNeighbour, Buffer->DEST);
	memcpy(Buffer->ORIGIN, NETROMCALL, 7);
	Buffer->ORIGIN[6] |= 1;		// set end of address
	Buffer->DEST[6] |= 0x80;			// set Command Bit


	time(&Buffer->Timestamp);
	
	C_Q_ADD(&TRACE_Q, Buffer);

}






