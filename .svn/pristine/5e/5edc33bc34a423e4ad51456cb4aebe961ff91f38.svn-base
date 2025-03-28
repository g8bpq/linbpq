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
//	Interface to allow G8BPQ switch to use ARDOP Virtual TNC in a form 
//	of ax.25

//	Uses a lot of routines in ARDOP.c

//	Includes its own ax.25 L2 stack, as I want to do dynamic packet size and
//	maxframe

//	Actually may be better to use existing L2, but modify for dynamic paclen, but we
//	need tighter coupling than the KISS driver provides. So this may become a "SuperKISS" TNC
//	as a minimum I think this needs channel busy info connected to L2. Not sure yet!! 


#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <stdio.h>
#include <time.h>


#include "CHeaders.h"

#ifdef WIN32
#include <Psapi.h>
#endif

int (WINAPI FAR *GetModuleFileNameExPtr)();
int (WINAPI FAR *EnumProcessesPtr)();

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"

#include "tncinfo.h"


#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

static int Socket_Data(int sock, int error, int eventcode);

int ARDOPKillTNC(struct TNCINFO * TNC);
int ARDOPRestartTNC(struct TNCINFO * TNC);
int KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
int SendReporttoWL2K(struct TNCINFO * TNC);
char * CheckAppl(struct TNCINFO * TNC, char * Appl);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
BOOL KillOldTNC(char * Path);
int ARDOPSendData(struct TNCINFO * TNC, char * Buff, int Len);
VOID ARDOPSendCommand(struct TNCINFO * TNC, char * Buff, BOOL Queue);
VOID SendToTNC(struct TNCINFO * TNC, UCHAR * Encoded, int EncLen);
VOID ARDOPProcessDataPacket(struct TNCINFO * TNC, UCHAR * Type, UCHAR * Data, int Length);

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

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

static int ProcessLine(char * buf, int Port);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

static ProcessLine(char * buf, int Port)
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

		TNC->WINMORHostName = malloc(strlen(p_ipad)+1);

		if (TNC->WINMORHostName == NULL) return TRUE;

		strcpy(TNC->WINMORHostName,p_ipad);

		ptr = strtok(NULL, " \t\n\r");

		if (ptr)
		{
			if (_stricmp(ptr, "PTT") == 0)
			{
				ptr = strtok(NULL, " \t\n\r");

				if (ptr)
				{
					if (_stricmp(ptr, "CI-V") == 0)
						TNC->PTTMode = PTTCI_V;
					else if (_stricmp(ptr, "CAT") == 0)
						TNC->PTTMode = PTTCI_V;
					else if (_stricmp(ptr, "RTS") == 0)
						TNC->PTTMode = PTTRTS;
					else if (_stricmp(ptr, "DTR") == 0)
						TNC->PTTMode = PTTDTR;
					else if (_stricmp(ptr, "DTRRTS") == 0)
						TNC->PTTMode = PTTDTR | PTTRTS;

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
//				if (p_cmd) TNC->ProgramPath = _strdup(_strupr(p_cmd));
			}
		}

		TNC->MaxConReq = 10;		// Default

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
			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
				TNC->WL2K = DecodeWL2KReportLine(buf);
			else
			if (_memicmp(buf, "BUSYHOLD", 8) == 0)		// Hold Time for Busy Detect
				TNC->BusyHold = atoi(&buf[8]);

			else
			if (_memicmp(buf, "BUSYWAIT", 8) == 0)		// Wait time beofre failing connect if busy
				TNC->BusyWait = atoi(&buf[8]);

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

			strcat (TNC->InitScript, buf);
		}


	return (TRUE);	
}



void ARDOPThread(int port);
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

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int datalen;
	UINT * buffptr;
	char txbuff[500];
	unsigned int bytes,txlen=0;
	int Param;
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
	case 1:				// poll

		while (TNC->PortRecord->UI_Q)
		{
			int datalen;
			char * Buffer;
			char FECMsg[256] = "";
			char Call[12] = "           ";		
			struct _MESSAGE * buffptr;
			
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);

			if (TNC->CONNECTED == 0)
			{
				// discard if not connected

				ReleaseBuffer(buffptr);
				continue;
			}
	
			datalen = buffptr->LENGTH - 7;
			Buffer = &buffptr->DEST[0];		// Raw Frame
			Buffer[datalen] = 0;

			// Frame has ax.25 format header. Convert to Text

			ConvFromAX25(Buffer + 7, Call);		// Origin
			strlop(Call, ' ');
			strcat(FECMsg, Call);
			strcat(FECMsg, ">");

			ConvFromAX25(Buffer, Call);			// Dest
			strlop(Call, ' ');
			strcat(FECMsg, Call);

			Buffer += 14;						// TO Digis
			datalen -= 7;

			while ((Buffer[-1] & 1) == 0)
			{
				Call[0] = ',';
				ConvFromAX25(Buffer, &Call[1]);
				strlop(&Call[1], ' ');
				strcat(FECMsg, Call);
				Buffer += 7;	// End of addr
				datalen -= 7;
			}

			strcat(FECMsg, "|");

			if (Buffer[0] == 3)				// UI
			{
				Buffer += 2;
				datalen -= 2;

			}
			strcat(FECMsg, Buffer);

			ARDOPSendData(TNC, FECMsg, strlen(FECMsg));
			TNC->FECPending = 1;
		
			ReleaseBuffer((UINT *)buffptr);
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

//				ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);  // !!!! Temp bug workaround !!!!

				ARDOPSendCommand(TNC, TNC->ConnectCmd, TRUE);
				TNC->Streams[0].Connecting = TRUE;

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

					UINT * buffptr = GetBuff();

					if (buffptr == 0) return (0);			// No buffers, so ignore

					buffptr[1]=39;
					memcpy(buffptr+2,"Sorry, Can't Connect - Channel is busy\r", 39);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					free(TNC->ConnectCmd);

					sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
					SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

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
				if (!TNC->Busy)
				{
					TNC->FECPending = 0;
					ARDOPSendCommand(TNC,"FECSEND TRUE", TRUE);
				}
			}
		}

		if (STREAM->NeedDisc)
		{
			STREAM->NeedDisc--;

			if (STREAM->NeedDisc == 0)
			{
				// Send the DISCONNECT

				ARDOPSendCommand(TNC, "DISCONNECT", TRUE);
			}
		}

		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC

				if (TNC->WIMMORPID)
				{
					ARDOPKillTNC(TNC);
					ARDOPRestartTNC(TNC);
				}
			}
		}

		if (TNC->TimeSinceLast++ > 800)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "WINMOR TNC"))
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
	
					ARDOPKillTNC(TNC);
					ARDOPRestartTNC(TNC);

					TNC->TimeSinceLast = 0;
				}
			}
		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Streams[0].Attached == 0)
		{
			// New Attach

			int calllen;
			char Msg[80];

			TNC->Streams[0].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
			TNC->Streams[0].MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

			ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
			ARDOPChangeMYC(TNC, TNC->Streams[0].MyCall);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			// Stop Scanning

			sprintf(Msg, "%d SCANSTOP", TNC->Port);
	
			Rig_Command(-1, Msg);

		}

		if (TNC->Streams[0].Attached)
			CheckForDetach(TNC, 0, &TNC->Streams[0], TidyClose, ForcedClose, CloseComplete);

		if (TNC->Streams[0].ReportDISC)
		{
			TNC->Streams[0].ReportDISC = FALSE;
			buff[4] = 0;
			return -1;
		}

		if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
		{
			//	See if time to reconnect
		
			time(&ltime);
			if (ltime - TNC->lasttime > 9 )
			{
				ConnecttoARDOP(port);
				TNC->lasttime = ltime;
			}
		}
		
		// See if any frames for this port

		if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
		{
			UINT * buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
			txlen=buffptr[1];
			memcpy(txbuff,buffptr+2,txlen);
			bytes = ARDOPSendData(TNC, &txbuff[0], txlen);
			STREAM->BytesTXed += bytes;
			WritetoTrace(TNC, txbuff, txlen);
			ReleaseBuffer(buffptr);
		}


		if (TNC->WINMORtoBPQ_Q != 0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen=buffptr[1];

			buff[4] = 0;						// Compatibility with Kam Driver
			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);	// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);
		}

		return (0);

	case 2:				// send

		if (!TNC->CONNECTED)
		{			
			return 0;		// Don't try if not connected
		}


		txlen=(buff[6]<<8) + buff[5] - 7;	
		
			{
				char Buffer[300];
				int len;

				// Send FEC Data

				buff[8 + txlen] = 0;
				len = sprintf(Buffer, "%-9s: %s", TNC->Streams[0].MyCall, &buff[8]);

				ARDOPSendData(TNC, Buffer, len);
				TNC->FECPending = 1;

				return 0;
			}

		return (0);

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		return 0;			
		break;

	case 4:				// reinit

		return 0;

	case 5:				// Close

		if (TNC->CONNECTED)
		{
			GetSemaphore(&Semaphore, 52);
			ARDOPSendCommand(TNC, "CLOSE", FALSE);
			FreeSemaphore(&Semaphore);
			Sleep(100);
		}
		shutdown(TNC->WINMORSock, SD_BOTH);
		Sleep(100);
		closesocket(TNC->WINMORSock);
		return 0;

	case 6:				// Scan Stop Interface

		Param = (int)buff;
	
		if (Param == 1)		// Request Permission
		{
			if (TNC->ConnectPending)
				TNC->ConnectPending--;		// Time out if set too long

			if (!TNC->ConnectPending)
				return 0;	// OK to Change

//			ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);

			return TRUE;
		}

		if (Param == 2)		// Check  Permission
		{
			if (TNC->ConnectPending)
			{
				TNC->ConnectPending--;
				return -1;	// Skip Interval
			}
			return 1;		// OK to change
		}

		if (Param == 3)		// Release  Permission
		{
//			ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;

		if (strcmp(Scan->ARDOPMode, TNC->ARDOPCurrentMode) != 0)
		{
			// Mode changed

			char CMD[32];
			
			if (TNC->ARDOPCurrentMode[0] == 0)
					ARDOPSendCommand(TNC, "LISTEN TRUE", TRUE);

			strcpy(TNC->ARDOPCurrentMode, Scan->ARDOPMode); 

			
			if (Scan->ARDOPMode[0] == 'S') // SKIP - Dont Allow Connects
			{
				if (TNC->ARDOPCurrentMode[0] != 0)
				{
					ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);
					TNC->ARDOPCurrentMode[0] = 0;
				}

				TNC->WL2KMode = 0;
				return 0;
			}

			if (strchr(Scan->ARDOPMode, 'F'))
				sprintf(CMD, "ARQBW %sORCED", Scan->ARDOPMode);
			else
				sprintf(CMD, "ARQBW %sAX", Scan->ARDOPMode);
	
			return 0;
		}
		return 0;
	}
	return 0;
}


static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}</script>"
		"</head><title>ARDOP Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2><form method=post action=ARDOPAbort?%d>ARDOP Status <input name=Save value=\"Abort Session\" type=submit style=\"position: absolute; right: 20;\"></form></h2>",
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

VOID ARDOPSuspendPort(struct TNCINFO * TNC);
VOID ARDOPReleasePort(struct TNCINFO * TNC);


UINT ARDOPAXExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	APPLCALLS * APPL;
	struct TNCINFO * TNC;
	char Aux[100] = "MYAUX ";
	char Appl[11];
	char * TempScript;
	struct PORTCONTROL * PORT = &PortEntry->PORTCONTROL;
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

		return (int) ExtProc;
	}

	PORT->PORTT1 = 15;
	PORT->PORTT2 = 3;
	PORT->PORTN2 = 5;
	PORT->PORTPACLEN = 128;
	
	TNC->Port = port;

	TNC->ARDOPBuffer = malloc(8192);
	TNC->ARDOPDataBuffer = malloc(8192);

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = ARDOPRestartTNC(TNC);

	TNC->Hardware = H_AXARDOP;

	if (TNC->BusyWait == 0)
		TNC->BusyWait = 10;

	if (TNC->BusyHold == 0)
		TNC->BusyHold = 1;

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 0;			// KISS ???
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->MAXHOSTMODESESSIONS = 0;	
	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only

	PortEntry->PORTCONTROL.UICAPABLE = TRUE;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = ARDOPSuspendPort;
	TNC->ReleasePortProc = ARDOPReleasePort;

	TNC->ModemCentre = 1500;				// WINMOR is always 1500 Offset

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
	strcat(TNC->InitScript,"LISTEN FALSE\r");		// Will use TNC in FEC mode only
	strcat(TNC->InitScript,"MYCALL\r");

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

		TNC->WINMORHostName=malloc(10);

		if (TNC->WINMORHostName != NULL) 
			strcpy(TNC->WINMORHostName,"127.0.0.1");

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

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,6,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,6,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,28,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,28,520,20, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,50,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,50,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,72,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,72,144,20, TNC->hDlg, NULL, hInstance, NULL);
 
 	CreateWindowEx(0, "STATIC", "Proto State", WS_CHILD | WS_VISIBLE,10,94,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,116,94,374,20 , TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,116,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,116,116,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,138,40,20 , TNC->hDlg, NULL, hInstance, NULL);
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
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after each Connection");
	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	MoveWindows(TNC);
#endif
	Consoleprintf("ARDOPAX Host %s %d", TNC->WINMORHostName, htons(TNC->destaddr.sin_port));

	ConnecttoARDOP(port);

	time(&TNC->lasttime);			// Get initial time value

	return ((int) ExtProc);
}

static int ConnecttoARDOP(int port)
{
	_beginthread(ARDOPThread,0,port);

	return 0;
}

static VOID ARDOPThread(port)
{
	// Opens sockets and looks for data on control and data sockets.
	
	// Socket may be TCP/IP or Serial

	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	char * ptr1, * ptr2;
	UINT * buffptr;
	char Cmd[32];

	if (TNC->WINMORHostName == NULL)
		return;

	TNC->BusyFlags = 0;

	TNC->CONNECTING = TRUE;

	Sleep(5000);		// Allow init to complete 

//	// If we started the TNC make sure it is still running.

//	if (!IsProcess(TNC->WIMMORPID))
//	{
//		ARDOPRestartTNC(TNC);
//		Sleep(3000);
//	}


	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		HostEnt = gethostbyname (TNC->WINMORHostName);
		 
		 if (!HostEnt)
		 {
		 	TNC->CONNECTING = FALSE;
			return;			// Resolve failed
		 }
		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//	closesocket(TNC->WINMORSock);
//	closesocket(TNC->WINMORDataSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for ARDOP socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}

	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for ARDOP Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
		closesocket(TNC->WINMORSock);

  	 	return; 
	}

 
	setsockopt(TNC->WINMORSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(TNC->WINMORDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
//	setsockopt(TNC->WINMORDataSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

/*	if (bind(TNC->WINMORSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=sprintf(Msg, "Bind Failed for ARDOP socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);
			
		closesocket(TNC->WINMORSock);
	 	TNC->CONNECTING = FALSE;

  	 	return; 
	}
*/
	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
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
   			i=sprintf(Msg, "Connect Failed for ARDOP socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->WINMORSock);
		TNC->WINMORSock = 0;
	 	TNC->CONNECTING = FALSE;
		return;
	}

	// Connect Data Port

	if (connect(TNC->WINMORDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
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
		
		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);
		TNC->WINMORSock = 0;
	 	TNC->CONNECTING = FALSE;
		return;
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
		buffptr = Q_REM(&TNC->BPQtoWINMOR_Q);

		if (buffptr)
			ReleaseBuffer(buffptr);
	}

	buffptr = GetBuff();
	buffptr[1] = 0;
	C_Q_ADD(&TNC->BPQtoWINMOR_Q, buffptr);

	while (ptr1 && ptr1[0])
	{
		ptr2 = strchr(ptr1, 13);
		if (ptr2)
			*(ptr2) = 0; 

		// if Date or Time command add current time

		if (_memicmp(ptr1, "DATE", 4) == 0)
		{
			time_t T;
			struct tm * tm;

			T = time(NULL);
			tm = gmtime(&T);	

			sprintf(Cmd,"DATE %02d%02d%02d\r",  tm->tm_mday, tm->tm_mon + 1, tm->tm_year - 100);
			ptr1 = Cmd;
		}
		else if (_memicmp(ptr1, "TIME", 4) == 0)
		{
			time_t T;
			struct tm * tm;

			T = time(NULL);
			tm = gmtime(&T);	

			sprintf(Cmd,"TIME %02d%02d%02d\r",  tm->tm_hour, tm->tm_min, tm->tm_sec);
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

		FD_SET(TNC->WINMORSock,&readfs);
		FD_SET(TNC->WINMORSock,&errorfs);

		if (TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&readfs);
			
//		FD_ZERO(&writefs);

//		if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORDataSock,&writefs);	// Need notification of busy clearing
		
		if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&errorfs);

		timeout.tv_sec = 600;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select(TNC->WINMORDataSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			Debugprintf("ARDOP Select failed %d ", WSAGetLastError());
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TNC->WINMORSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				ARDOPProcessReceivedControl(TNC);
				FreeSemaphore(&Semaphore);
			}
								
			if (FD_ISSET(TNC->WINMORDataSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				ARDOPProcessReceivedData(TNC);
				FreeSemaphore(&Semaphore);
			}

			if (FD_ISSET(TNC->WINMORSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "ARDOP Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC->RIG, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->WINMORDataSock);
				TNC->WINMORSock = 0;
				return;
			}

			if (FD_ISSET(TNC->WINMORDataSock, &errorfs))
			{
				sprintf(Msg, "ARDOP Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC->RIG, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				closesocket(TNC->WINMORSock);
				closesocket(TNC->WINMORDataSock);
				TNC->WINMORSock = 0;
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
				Rig_PTT(TNC->RIG, FALSE);			// Make sure PTT is down

			if (TNC->Streams[0].Attached)
				TNC->Streams[0].ReportDISC = TRUE;

			GetSemaphore(&Semaphore, 52);
			ARDOPSendCommand(TNC, "CODEC FALSE", FALSE);
			FreeSemaphore(&Semaphore);

			Sleep(100);
			shutdown(TNC->WINMORSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->WINMORDataSock);

			Sleep(100);
			shutdown(TNC->WINMORDataSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->WINMORDataSock);

			if (TNC->WIMMORPID && TNC->WeStartedTNC)
			{
				ARDOPKillTNC(TNC);
			}
			return;
		}
	}
	sprintf(Msg, "ARDOP Thread Terminated Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);
}


static VOID ARDOPProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	UINT * buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
//	unsigned int CRC;

//	CRC = checkcrc16(&Buffer[2], MsgLen - 2);

	Buffer[MsgLen - 1] = 0;		// Remove CR

//	if (CRC == 0)
//	{
//		Debugprintf("ADDOP CRC Error %s", Buffer);
//		return;
//	}
	
	Buffer+=2;					// Skip c:

	if (_memicmp(Buffer, "RDY", 3) == 0)
	{
		//	Command ACK. Remove from bufer and send next if any

		UINT * buffptr;
		UINT * Q;
	
	
		buffptr = Q_REM(&TNC->BPQtoWINMOR_Q);

		if (buffptr)
			ReleaseBuffer(buffptr);

		// See if another

		// Leave on Queue till acked

		// Q may not be word aligned, so copy as bytes (for ARM5)

		Q = (UINT *)&TNC->BPQtoWINMOR_Q;

		buffptr = (UINT *)Q[0];

		if (buffptr)
			SendToTNC(TNC, (UCHAR *)&buffptr[2], buffptr[1]);

		return;
	}

	if (_memicmp(Buffer, "CRCFAULT", 8) == 0)
	{
		//	Command NAK. Resend 

		UINT * buffptr;
		UINT * Q;
	
		// Leave on Queue till acked

		// Q may not be word aligned, so copy as bytes (for ARM5)

		Q = (UINT *)&TNC->BPQtoWINMOR_Q;

		buffptr = (UINT *)Q[0];

		if (buffptr)
			SendToTNC(TNC, (UCHAR *)&buffptr[2], buffptr[1]);

		Debugprintf("ARDP CRCFAULT Received");
		return;
	}

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
		Debugprintf(Buffer);
	
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

		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, TRUE);

		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, FALSE);
		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		MySetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		strcpy(TNC->WEB_CHANSTATE, "Busy");

		TNC->WinmorRestartCodecTimer = time(NULL);
		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->BusyHold)
			strcpy(TNC->WEB_CHANSTATE, "BusyHold");
		else
			strcpy(TNC->WEB_CHANSTATE, "Clear");

		MySetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
		TNC->WinmorRestartCodecTimer = time(NULL);
		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}

	if (_memicmp(Buffer, "TARGET", 6) == 0)
	{
		TNC->ConnectPending = 6;					// This comes before Pending
		Debugprintf(Buffer);
		WritetoTrace(TNC, Buffer, MsgLen - 3);
		memcpy(TNC->TargetCall, &Buffer[7], 10);
		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}

	if (_memicmp(Buffer, "OFFSET", 6) == 0)
	{
//		WritetoTrace(TNC, Buffer, MsgLen - 5);
//		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
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
//			else
//			if (TNC->TXRXState == 'S')
//				ARDOPSendCommand(TNC,"OVER");

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
		ARDOPSendCommand(TNC, "RDY", FALSE);
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
		WritetoTrace(TNC, Buffer, MsgLen - 3);

		ARDOPSendCommand(TNC, "RDY", FALSE);
	
		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = STREAM->PacketsSent = 0;

//		if (TNC->StartInRobust)
//			ARDOPSendCommand(TNC, "ROBUST TRUE");

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
			
			// Incomming Connect

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			ProcessIncommingConnectEx(TNC, Call, 0, TRUE, TRUE);
				
			SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];
			
			TNC->ConnectPending = FALSE;

			if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
				SESS->Frequency = (atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
				SESS->Mode = TNC->WL2KMode;
			}
			else
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->TargetCall);
				if (WL2K)
				{
					SESS->Frequency = WL2K->Freq;
					SESS->Mode = WL2K->mode;
				}
			}
			
			if (WL2K)
				strcpy(SESS->RMSCall, WL2K->RMSCall);

			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			
			// Check for ExcludeList

			if (ExcludeList[0])
			{
				if (CheckExcludeList(SESS->L4USER) == FALSE)
				{
					char Status[32];

					TidyClose(TNC, 0);
					sprintf(Status, "%d SCANSTART 15", TNC->Port);
					Rig_Command(-1, Status);
					Debugprintf("ARDOP Call from %s rejected", Call);
					return;
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

				// Make sure app is available

				if (CheckAppl(TNC, AppName))
				{
					MsgLen = sprintf(Buffer, "%s\r", AppName);

					buffptr = GetBuff();

					if (buffptr == 0)
					{
						return;			// No buffers, so ignore
					}

					buffptr[1] = MsgLen;
					memcpy(buffptr+2, Buffer, MsgLen);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
		
					TNC->SwallowSignon = TRUE;

					// Save Appl Call in case needed for 

				}
				else
				{
					char Msg[] = "Application not available\r\n";
					
					// Send a Message, then a disconenct
					
					ARDOPSendData(TNC, Msg, strlen(Msg));
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

			buffptr[1] = ReplyLen;
			memcpy(buffptr+2, Reply, ReplyLen);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG && TNC->RIG->Valchar[0])
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s",  TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
			else
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			
			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			UpdateMH(TNC, Call, '+', 'O');
			return;
		}
	}


	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0
		|| _memicmp(Buffer, "STATUS CONNECT TO", 17) == 0  
		|| _memicmp(Buffer, "STATUS ARQ TIMEOUT FROM PROTOCOL STATE", 24) == 0)
	{
		Debugprintf(Buffer);

		ARDOPSendCommand(TNC, "RDY", FALSE);
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

			buffptr[1] = sprintf((UCHAR *)&buffptr[2], "ARDOP} Failure with %s\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

//			ARDOPSendCommand(TNC, "LISTEN FALSE", TRUE);  // !!!! Temp bug workaround !!!!

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 3);

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

	Debugprintf(Buffer);

	if (_memicmp(Buffer, "STATUS ", 7) == 0)
	{
		ARDOPSendCommand(TNC, "RDY", FALSE);
		return;
	}

	if (_memicmp(Buffer, "RADIOMODELS", 11) == 0)
		return;

	if (_memicmp(Buffer, "MODE", 4) == 0)
	{
	//	Debugprintf("WINMOR RX: %s", Buffer);

		strcpy(TNC->WEB_MODE, &Buffer[5]);
		SetWindowText(TNC->xIDC_MODE, &Buffer[5]);
		return;
	}

	if (_memicmp(&Buffer[0], "PENDING", 7) == 0)	// Save Pending state for scan control
	{
		ARDOPSendCommand(TNC, "RDY", FALSE);
		TNC->ConnectPending = 6;				// Time out after 6 Scanintervals
		return;
	}

	if (_memicmp(&Buffer[0], "CANCELPENDING", 13) == 0
		|| _memicmp(&Buffer[0], "REJECTEDB", 9) == 0)  //REJECTEDBUSY or REJECTEDBW
	{
		ARDOPSendCommand(TNC, "RDY", FALSE);
		TNC->ConnectPending = FALSE;
		return;
	}

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 3);
//		return;
	}

	if (_memicmp(Buffer, "NEWSTATE", 8) == 0)
	{
		ARDOPSendCommand(TNC, "RDY", FALSE);

		TNC->WinmorRestartCodecTimer = time(NULL);

		MySetWindowText(TNC->xIDC_PROTOSTATE, &Buffer[9]);
		strcpy(TNC->WEB_PROTOSTATE,  &Buffer[9]);
	
		if (_memicmp(&Buffer[9], "DISCONNECTING", 13) == 0)	// So we can timout stuck discpending
		{
			TNC->DiscPending = 600;
			return;
		}
		if (_memicmp(&Buffer[9], "DISCONNECTED", 12) == 0)
		{
			TNC->DiscPending = FALSE;
			TNC->ConnectPending = FALSE;

			if (TNC->RestartAfterFailure)
			{
				if (TNC->HadConnect)
				{
					TNC->HadConnect = FALSE;

					if (TNC->WIMMORPID)
					{
						ARDOPKillTNC(TNC);
						ARDOPRestartTNC(TNC);
					}
				}
			}
			return;
		}

		if (strcmp(&Buffer[9], "ISS") == 0)	// Save Pending state for scan control
			TNC->TXRXState = 'S';
		else if (strcmp(&Buffer[9], "IRS") == 0)
			TNC->TXRXState = 'R';
	
		return;
	}


	if (_memicmp(Buffer, "PROCESSID", 9) == 0)
	{
		HANDLE hProc;
		char ExeName[256] = "";

		TNC->WIMMORPID = atoi(&Buffer[10]);

#ifndef LINBPQ

		// Get the File Name in case we want to restart it.

		if (TNC->ProgramPath == NULL)
		{
			if (GetModuleFileNameExPtr)
			{
				hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->WIMMORPID);
	
				if (hProc)
				{
					GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);
					CloseHandle(hProc);

					TNC->ProgramPath = _strdup(ExeName);
				}
			}
		}

		// Set Window Title to reflect BPQ Port Description

		EnumWindows(EnumARDOPWindowsProc, (LPARAM)TNC);
#endif
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
		WritetoTrace(TNC, Buffer, MsgLen - 3);
		return;
	}

	if (_memicmp(Buffer, "OVER", 4) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 3);
		return;
	}

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
	
	buffptr[1] = sprintf((UCHAR *)&buffptr[2], "ARDOP} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
}

static VOID ARDOPProcessReceivedData(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
//	char * ptr, * ptr2;
//	char Buffer[4096];

	// shouldn't get several messages per packet, as each should need an ack
	// May get message split over packets

	//	Both command and data arrive here, which complicated things a bit

	//	Commands start with c: and end with CR.
	//	Data starts with d: and has a length field
	//	“d:ARQ|FEC|ERR|, 2 byte count (Hex 0001 – FFFF), binary data, +2 Byte CRC”

	//	As far as I can see, shortest frame is “c:RDY<Cr> + 2 byte CRC” = 8 bytes

	if (TNC->DataInputLen > 8000)	// Shouldnt have packets longer than this
		TNC->DataInputLen=0;

	//	I don't think it likely we will get packets this long, but be aware...

	//	We can get pretty big ones in the faster 
				
	InputLen=recv(TNC->WINMORDataSock, &TNC->ARDOPDataBuffer[TNC->DataInputLen], 8192 - TNC->DataInputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
//		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);

		TNC->WINMORDataSock = 0;

		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		return;					
	}

	TNC->DataInputLen += InputLen;

loop:

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

			// Check CRC

//			CRC = compute_crc(&TNC->ARDOPBuffer[2], DataLen + 4);

//			CRC = checkcrc16(&TNC->ARDOPBuffer[2], DataLen + 4);

//			if (CRC == 0)
//			{
//				Debugprintf("ADDOP CRC Error %s", &TNC->ARDOPBuffer[2]);
//				return;
//			}


			memcpy(DataType, &TNC->ARDOPDataBuffer[4] , 3);
			DataType[3] = 0;
			Data = &TNC->ARDOPDataBuffer[7];
			DataLen -= 3;

			ARDOPProcessDataPacket(TNC, DataType, Data, DataLen);
		
//			ARDOPSendCommand(TNC, "RDY", FALSE);

			// See if anything else in buffer

			TNC->DataInputLen -= MsgLen;

			if (TNC->InputLen == 0)
				return;

			memmove(TNC->ARDOPDataBuffer, &TNC->ARDOPDataBuffer[MsgLen],  TNC->DataInputLen);
			goto loop;
		}
	}	
	return;
}



static VOID ARDOPProcessReceivedControl(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[4096];

	// shouldn't get several messages per packet, as each should need an ack
	// May get message split over packets

	//	Both command and data arrive here, which complicated things a bit

	//	Commands start with c: and end with CR.
	//	Data starts with d: and has a length field
	//	“d:ARQ|FEC|ERR|, 2 byte count (Hex 0001 – FFFF), binary data, +2 Byte CRC”

	//	As far as I can see, shortest frame is “c:RDY<Cr> + 2 byte CRC” = 8 bytes

	if (TNC->InputLen > 8000)	// Shouldnt have packets longer than this
		TNC->InputLen=0;

	//	I don't think it likely we will get packets this long, but be aware...

	//	We can get pretty big ones in the faster 
				
	InputLen=recv(TNC->WINMORSock, &TNC->ARDOPBuffer[TNC->InputLen], 8192 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		closesocket(TNC->WINMORSock);

		TNC->WINMORSock = 0;

		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		return;					
	}

	TNC->InputLen += InputLen;

loop:

	if (TNC->InputLen < 6)
		return;					// Wait for more to arrive (?? timeout??)

	if (TNC->ARDOPBuffer[1] = ':')	// At least message looks reasonable
	{
		if (TNC->ARDOPBuffer[0] == 'c')
		{
			// Command = look for CR

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

				//	I dont think this should happen, but...

				MsgLen = TNC->InputLen - (ptr2-ptr) + 1;	// Include CR 

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
		}
	}	
	return;
}



static VOID ARDOPProcessDataPacket(struct TNCINFO * TNC, UCHAR * Type, UCHAR * Data, int Length)
{
	// Info on Data Socket - just packetize and send on
	
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int PacLen = 236;
	UINT * buffptr;
		
	TNC->TimeSinceLast = 0;

	if (strcmp(Type, "IDF") == 0)
	{
		// Place ID frames in Monitor Window and MH

		char Call[20];

		Data[Length] = 0;
		WritetoTrace(TNC, Data, Length);

		if (memcmp(Data, "ID:", 3) == 0)	// These seem to be transmitted ID's
		{
			memcpy(Call, &Data[3], 20);
			strlop(Call, ':'); 
			UpdateMH(TNC, Call, '!', 'I');
		}
		return;
	}

	STREAM->BytesRXed += Length;

	Data[Length] = 0;	
	Debugprintf("ARDOP: RXD %d bytes", Length);

	sprintf(TNC->WEB_TRAFFIC, "Sent %d RXed %d Queued %d",
			STREAM->BytesTXed, STREAM->BytesRXed,STREAM->BytesOutstanding);
	MySetWindowText(TNC->xIDC_TRAFFIC, TNC->WEB_TRAFFIC);

	
	if (TNC->FECMode)
	{	
		Length = strlen(Data);
		if (Data[Length - 1] == 10)
			Data[Length - 1] = 13;	

	}

	if (strcmp(Type, "FEC") == 0)
	{
		// Make sure it at least looks like an ax.25 format message
		
	
		char * ptr1 = Data;
		char * ptr2 = strchr(ptr1, '>');
		int Len = 80;

		if (ptr2 && (ptr2 - ptr1) < 10)
		{
			// Could be APRS

			if (memcmp(ptr2 + 1, "AP", 2) == 0)
			{
				// assume it is

				char * ptr3 = strchr(ptr2, '|');
				struct _MESSAGE * buffptr = GetBuff();

				if (ptr3 == 0)
					return;

				*(ptr3++) = 0;		// Terminate TO call

				Len = strlen(ptr3);

				// Convert to ax.25 format

				if (buffptr == 0)
					return;			// No buffers, so ignore

				buffptr->PORT = TNC->Port;

				ConvToAX25(ptr1, buffptr->ORIGIN);
				ConvToAX25(ptr2 + 1, buffptr->DEST);
				buffptr->ORIGIN[6] |= 1;				// Set end of address
				buffptr->CTL = 3;
				buffptr->PID = 0xF0;
				memcpy(buffptr->L2DATA, ptr3, Len);
				buffptr->LENGTH  = 23 + Len;
				time(&buffptr->Timestamp);

				BPQTRACE((MESSAGE *)buffptr, TRUE);

				return;

			}
		}

		// FEC but not AX.25 APRS. Discard if connected

		return;
	}

	// Discard ARQ frames

	return;
}

static VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[0].BytesOutstanding == 0)
		ARDOPSendCommand(TNC, "DISCONNECT", TRUE);
}

static VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	ARDOPSendCommand(TNC, "ABORT", TRUE);
}

static VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	ARDOPReleaseTNC(TNC);

	if (TNC->FECMode)
	{
		TNC->FECMode = FALSE;
		ARDOPSendCommand(TNC, "SENDID", TRUE);
	}
}

