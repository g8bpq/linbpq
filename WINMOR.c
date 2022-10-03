/*
Copyright 2001-2018 John Wiseman G8BPQ

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
//	DLL to provide interface to allow G8BPQ switch to use WINMOR as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//


//  Version 1.0 January 2009 - Initial Version
//

// March 22 2010

// Send FAULTS to Monitor Window
// Force PROTOCOL = WINMOR/PACTOR (to simplifiy Config)

// July 2010
// Support up to 32 BPQ Ports
// Support up to 32 Applications

// Version 1.2.1.2 August 2010 

// Save Minimized State
// Handle new "BLOCKED by Busy channel" message from TNC

// Version 1.2.1.4 August 2010 

// Add Scan control of BW setting
// Reset TNC if stuck in Disconnecting
// Add option to send reports to WL2K
// Disconnect if appl not available

// Version 1.2.1.5 August 2010 

// Updates to WL2K Reporting
// Send Watchdog polls every minute and restart if no response.
// Don't connect if channel is busy (unless specifically overridden)

// Version 1.2.1.6 September 2010

// Add option to kill and restart TNC after each transfer
// Fix PTT operation after Node reconfig

// Version 1.2.2.1 September 2010

// Add option to get config from bpq32.cfg
// Merge with BPQ32.dll


#define _CRT_SECURE_NO_DEPRECATE

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

int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);
int KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
int SendReporttoWL2K(struct TNCINFO * TNC);
char * CheckAppl(struct TNCINFO * TNC, char * Appl);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
BOOL KillOldTNC(char * Path);
int standardParams(struct TNCINFO * TNC, char * buf);

static char ClassName[]="WINMORSTATUS";
static char WindowTitle[] = "WINMOR";
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

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

// There seem to be timing issues when calling SendMessage from multiple threads.
// Queue and process in main thread

UINT * WINMORTraceQ;
UINT * SetWindowTextQ;

VOID WritetoTraceSupport(struct TNCINFO * TNC, char * Msg, int Len)
{
	int index = 0;
	UCHAR * ptr1 = Msg, * ptr2;
	UCHAR Line[1000];
	int LineLen, i;
	UCHAR Save;
	int SaveLen = Len;
	if (Len < 0)
		return;

	Save = Msg[Len];
	Msg[Len] = 0;

#ifndef LINBPQ
	index=SendMessage(TNC->hMonitor, LB_SETCURSEL, -1, 0);
#endif

lineloop:

	if (Len > 0)
	{
		//	copy text to control a line at a time	

		ptr2 = memchr(ptr1, 13, Len);

		if (ptr2)
		{
			ptr2++;
			LineLen = (int)(ptr2 - ptr1);
			Len -= LineLen;
			memcpy(Line, ptr1, LineLen);
			memcpy(&Line[LineLen - 1], "<cr>", 4);
			LineLen += 3;

			if ((*ptr2) == 10)
			{
				memcpy(&Line[LineLen], "<lf>", 4);
				LineLen += 4;
				ptr2++;
				Len --;
			}

			Line[LineLen] = 0;

			// If line contains any data above 7f, assume binary and dont display

			for (i = 0; i < LineLen; i++)
			{
				if (Line[i] > 126 || Line[i] < 32)
					goto Skip;
			}

			// We now also pass to Monitor Window

			if (strlen(Line) < 250)
			{
				MESSAGE Monframe;
				memset(&Monframe, 0, sizeof(Monframe));

				Monframe.PORT = TNC->Port;	
				Monframe.LENGTH = 12 + strlen(Line);
				Monframe.DEST[0] = 1;			// Plain Text Monitor
				strcpy(&Monframe.DEST[1], Line);

				time(&Monframe.Timestamp);
				BPQTRACE((MESSAGE *)&Monframe, FALSE);
			}

#ifdef LINBPQ
#else
			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Line);
#endif
			// Write to Web Buffer

			strcat(TNC->WebBuffer, Line);
			strcat(TNC->WebBuffer, "\r\n");
			if (strlen(TNC->WebBuffer) > 4500)
				memmove(TNC->WebBuffer, &TNC->WebBuffer[500], 4490);	// Make sure null is moved
		Skip:
			ptr1 = ptr2;

			goto lineloop;

		}

		// Process incomplete line

		for (i = 0; i < Len; i++)
		{
			if (ptr1[i] > 126 || ptr1[i] < 32)
				break;
		}

		if (i == Len)
		{
			if (Len < 250)
			{
				MESSAGE Monframe;
				memset(&Monframe, 0, sizeof(Monframe));

				Monframe.PORT = TNC->Port;	
				Monframe.LENGTH = 12 + Len;
				Monframe.DEST[0] = 1;			// Plain Text Monitor

				memcpy(&Monframe.DEST[1], ptr1, Len);
				Monframe.DEST[1 + Len] = 0;

				time(&Monframe.Timestamp);
				BPQTRACE((MESSAGE *)&Monframe, FALSE);
			}


#ifdef LINBPQ
#else
			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) ptr1 );
#endif
			strcat(TNC->WebBuffer, ptr1);
			strcat(TNC->WebBuffer, "\r\n");
			if (strlen(TNC->WebBuffer) > 4500)
				memmove(TNC->WebBuffer, &TNC->WebBuffer[500], 4490);	// Make sure null is moved
		}
	}

#ifdef LINBPQ
#else

	if (index > 1200)					
	do
		index=index=SendMessage(TNC->hMonitor, LB_DELETESTRING, 0, 0);
	while (index > 1000);

	if (index > -1)
		index=SendMessage(TNC->hMonitor, LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));
#endif
	Msg[SaveLen] = Save;

}
				
VOID MySetWindowTextWithSem(HWND hWnd, char * Msg)
{
#ifndef LINBPQ

	PMSGWITHLEN buffptr;

	buffptr = GetBuff();

	if (buffptr)
	{
		buffptr->Len= (UINT)hWnd;
		memcpy(&buffptr->Data[0], Msg, strlen(Msg) + 1);
	
		C_Q_ADD(&SetWindowTextQ, buffptr);
	}

#endif
}

int C_Q_ADD_NP(VOID *PQ, VOID *PBUFF);

struct SEM SetWindTextSem = {0, 0, 0, 0};

VOID MySetWindowText(HWND hWnd, char * Msg)
{
#ifndef LINBPQ

	PMSGWITHLEN buffptr;

	GetSemaphore(&SetWindTextSem, 61);
	buffptr = zalloc(400);

	if (buffptr)
	{
		buffptr->Len= (UINT)hWnd;
		memcpy(&buffptr->Data[0], Msg, strlen(Msg) + 1);
	
		C_Q_ADD_NP(&SetWindowTextQ, buffptr);
	}

	FreeSemaphore(&SetWindTextSem);
#endif
}

VOID SetWindowTextSupport()
{
	PMSGWITHLEN Buffer;

	while (SetWindowTextQ)
	{
		GetSemaphore(&SetWindTextSem, 61);
		Buffer = Q_REM_NP(&SetWindowTextQ);
		SetWindowText((HWND)Buffer->Len, Buffer->Data);
		FreeSemaphore(&SetWindTextSem);
		free(Buffer);
	}
}


VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len)
{
	//	It seems writing from multiple threads can cause problems in Windows
	//	Queue and process in main thread

#ifdef LINBPQ
	WritetoTraceSupport(TNC, Msg, Len);
}
#else
	UINT * buffptr;
	BOOL Sem = FALSE;

	if (Len < 0)
		return;

	// Get semaphore if it isn't set

	if (InterlockedExchange(&Semaphore.Flag, 1) == 0) 
	{
		Sem = TRUE;
		Semaphore.Gets++;
	}

	buffptr = GetBuff();

	if (buffptr)
	{
		if (Len > 340)
			Len = 340;

		buffptr[1] = (UINT)TNC;
		buffptr[2] = (UINT)Len;
		memcpy(&buffptr[3], Msg, Len + 1);	

		C_Q_ADD(&WINMORTraceQ, buffptr);
	}

	if (Sem)
		FreeSemaphore(&Semaphore);

}
#endif

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
			if (_memicmp(buf, "STARTINROBUST", 13) == 0)
				TNC->StartInRobust = TRUE;
			
			else
			if (_memicmp(buf, "ROBUST", 6) == 0)
			{
				if (_memicmp(&buf[7], "TRUE", 4) == 0)
					TNC->Robust = TRUE;
				
				strcat (TNC->InitScript, buf);
			}
			else if (standardParams(TNC, buf) == FALSE)
				strcat (TNC->InitScript, buf);
		}


	return (TRUE);	
}



void WINMORThread(void * portptr);
VOID ProcessDataSocketData(int port);
int ConnecttoWINMOR();
static int ProcessReceivedData(struct TNCINFO * TNC);
int V4ProcessReceivedData(struct TNCINFO * TNC);
VOID ReleaseTNC(struct TNCINFO * TNC);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);


#define MAXBPQPORTS 32

static time_t ltime;


static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);



VOID ChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

//	send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);

	datalen = sprintf(TXMsg, "MYC %s\r\n", Call);
	send(TNC->TCPSock,TXMsg, datalen, 0);

//	send(TNC->TCPSock, "CODEC TRUE\r\n", 12, 0);
//	TNC->StartSent = TRUE;

	send(TNC->TCPSock, "MYC\r\n", 5, 0);
}

static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	int i,winerr;
	size_t datalen;
	PMSGWITHLEN buffptr;
	char txbuff[500];
	unsigned int bytes;
	size_t txlen = 0;
	char ErrMsg[255];
	size_t Param;
	HKEY hKey=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct ScanEntry * Scan;
	fd_set readfs;
	fd_set writefs;
	fd_set errorfs;
	struct timeval timeout;

	if (TNC == NULL)
		return 0;							// Port not defined

	switch (fn)
	{
	case 1:				// poll

		// Check session limit timer

		if ((STREAM->Connecting || STREAM->Connected) && !STREAM->Disconnecting)
		{
			if (TNC->SessionTimeLimit && STREAM->ConnectTime && time(NULL) > (TNC->SessionTimeLimit + STREAM->ConnectTime))
			{
				send(TNC->TCPSock,"DISCONNECT\r\n", 12, 0);
				STREAM->Disconnecting = TRUE;
			}
		}

		while (TNC->PortRecord->UI_Q)			// Release anything accidentally put on UI_Q
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

				send(TNC->TCPSock, TNC->ConnectCmd, (int)strlen(TNC->ConnectCmd), 0);
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

					PMSGWITHLEN buffptr = GetBuff();

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

		if (TNC->HeartBeat++ > 600 || (TNC->Streams[0].Connected && TNC->HeartBeat > 50))			// Every Minute unless connected
		{	
			if (TNC->HeartBeat > 600 && TNC->hWnd)
			{
				char wtext[100];
				sprintf (wtext, "WINMOR Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
				MySetWindowText(TNC->hWnd, wtext);
			}

			TNC->HeartBeat = 0;

			if (TNC->CONNECTED)
			{
				// Probe link

				if (TNC->Streams[0].Connecting || TNC->Streams[0].Connected)
					send(TNC->TCPSock, "MODE\r\n", 6, 0);
				else
				{
					if (time(NULL) - TNC->WinmorRestartCodecTimer > 900)	// 15 mins
					{
						send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);
						send(TNC->TCPSock, "CODEC TRUE\r\n", 12, 0);
					}
					else
						send(TNC->TCPSock, "STATE\r\n", 7, 0);
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
					send(TNC->TCPSock, "SENDID 0\r\n", 10, 0);
				}
			}
			if (TNC->FECPending)	// Check if FEC Send needed
			{
				if (!TNC->Busy)
				{
					TNC->FECPending = 0;

					if (TNC->FEC1600)
						send(TNC->TCPSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->TCPSock,"FECSEND 500\r\n", 13, 0);
				}
			}
		}

		if (STREAM->NeedDisc)
		{
			STREAM->NeedDisc--;

			if (STREAM->NeedDisc == 0)
			{
				// Send the DISCONNECT

				send(TNC->TCPSock, "DISCONNECT\r\n", 12, 0);
			}
		}

		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC

				if (TNC->PID)
				{
					KillTNC(TNC);
					RestartTNC(TNC);
				}
			}
		}

		if (TNC->TimeSinceLast++ > 800)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath && TNC->CONNECTED)
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

					SetWindowText(TNC->xIDC_RESTARTTIME, Time);
					strcpy(TNC->WEB_RESTARTTIME, Time);

					sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
					SetWindowText(TNC->xIDC_RESTARTS, Time);
					strcpy(TNC->WEB_RESTARTS, Time);
	
					KillTNC(TNC);
					RestartTNC(TNC);

					TNC->TimeSinceLast = 0;
				}
			}
		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Streams[0].Attached == 0)
		{
			// New Attach

			int calllen;
			char Msg[80];

			TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

			TNC->Streams[0].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
			TNC->Streams[0].MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

			send(TNC->TCPSock, "LISTEN FALSE\r\n", 14, 0);
			ChangeMYC(TNC, TNC->Streams[0].MyCall);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			// Stop Scanning

			sprintf(Msg, "%d SCANSTOP", TNC->Port);
	
			Rig_Command(-1, Msg);

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
					ConnecttoWINMOR(port);
					TNC->lasttime = ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&readfs);
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->TCPDataSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->TCPDataSock,&errorfs);

			timeout.tv_sec = 0;
			timeout.tv_usec = 0;				// poll

			if (select((int)TNC->TCPDataSock + 1, &readfs, &writefs, &errorfs, &timeout) > 0)
			{
				//	See what happened

				if (FD_ISSET(TNC->TCPDataSock, &readfs))
					ProcessDataSocketData(port);			
				
				if (FD_ISSET(TNC->TCPDataSock, &writefs))
				{
					// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen = buffptr->Len;
					memcpy(txbuff,buffptr->Data,txlen);
					bytes=send(TNC->TCPSock, (const char FAR *)&txbuff, (int)txlen, 0);
					ReleaseBuffer(buffptr);
				}
					
				if (FD_ISSET(TNC->TCPDataSock, &errorfs))
				{
					i=sprintf(ErrMsg, "WINMOR Data Connection lost for BPQ Port %d\r\n", port);
					WritetoConsole(ErrMsg);
					TNC->CONNECTING = FALSE;
					TNC->CONNECTED = FALSE;
					TNC->Streams[0].ReportDISC = TRUE;
				}
			}
		
		// See if any frames for this port

		if (TNC->WINMORtoBPQ_Q != 0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen = buffptr->Len;

			buff->PORT = 0;						// Compatibility with Kam Driver
			buff->PID = 0xf0;
			memcpy(&buff->L2DATA[0], buffptr->Data, datalen);	// Data goes to +7, but we have an extra byte
			datalen = buffptr->Len;

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

			buffptr->Len = 36;
			memcpy(buffptr->Data, "No Connection to WINMOR Virtual TNC\r", 36);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
		{
			PMSGWITHLEN buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
			txlen = buffptr->Len;
			memcpy(txbuff, buffptr->Data, txlen);
			bytes = send(TNC->TCPDataSock, txbuff, (int)txlen, 0);
			STREAM->BytesTXed += bytes;
			WritetoTrace(TNC, txbuff, (int)txlen);
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
			STREAM->PacketsSent++;

			if (STREAM->PacketsSent == 3)
			{
				if (TNC->Robust)
					send(TNC->TCPSock, "ROBUST TRUE\r\n", 13, 0);
				else
					send(TNC->TCPSock, "ROBUST FALSE\r\n", 14, 0);
			}

			bytes = send(TNC->TCPDataSock,buff->L2DATA, (int)txlen, 0);
			STREAM->BytesTXed += bytes;
			WritetoTrace(TNC, &buff->L2DATA[0], (int)txlen);

		}
		else
		{
			if (_memicmp(buff->L2DATA, "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}
	
			if (TNC->FECMode)
			{
				char Buffer[300];
				int len;

				// Send FEC Data

				buff->L2DATA[txlen] = 0;
				len = sprintf(Buffer, "%-9s: %s", TNC->Streams[0].MyCall, &buff->L2DATA);

				send(TNC->TCPDataSock, Buffer, len, 0);

				if (TNC->BusyFlags)
				{
					TNC->FECPending = 1;
				}
				else
				{
					if (TNC->FEC1600)
						send(TNC->TCPSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->TCPSock,"FECSEND 500\r\n", 13, 0);
				}
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
					PMSGWITHLEN buffptr = GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr->Len = sprintf(buffptr->Data, "%s", &buff->L2DATA[0]);
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(&buff->L2DATA[0], "OVERRIDEBUSY", 12) == 0)
			{
				PMSGWITHLEN buffptr = GetBuff();

				TNC->OverrideBusy = TRUE;

				if (buffptr)
				{
					buffptr->Len = sprintf(&buffptr->Data[0], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}

				return 0;

			}

			if (_memicmp(&buff->L2DATA[0], "MAXCONREQ", 9) == 0)
			{
				if (buff->L2DATA[9] != 13)
				{
					// Limit connects

					int tries = atoi(&buff->L2DATA[10]);
					int len;

					if (tries > 10) tries = 10;
					len = sprintf(&buff->L2DATA[0], "MAXCONREQ %d\r\nMAXCONREQ\r\n", tries);

					send(TNC->TCPSock, &buff->L2DATA[0], len, 0);
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
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "Winmor} OK\r");
						C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					}
					return 0;
				}
			}

			if ((_memicmp(buff->L2DATA, "BW 500", 6) == 0) || (_memicmp(buff->L2DATA, "BW 1600", 7) == 0))
			{
				// Generate a local response
				
				PMSGWITHLEN buffptr = GetBuff();

				if (_memicmp(buff->L2DATA, "BW 500", 6) == 0)
					TNC->WL2KMode = 21;
				else
					TNC->WL2KMode = 22;
	
				if (buffptr)
				{
					buffptr->Len = sprintf(&buffptr->Data[0], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				TNC->WinmorCurrentMode = 0;			// So scanner will set next value
			}

			if (_memicmp(buff->L2DATA, "CODEC TRUE", 9) == 0)
				TNC->StartSent = TRUE;

			if (_memicmp(buff->L2DATA, "ROBUST", 6) == 0)
			{
				if (_memicmp(&buff->L2DATA[7], "TRUE", 4) == 0)
					TNC->Robust = TRUE;
				else
					TNC->Robust = FALSE;
			}

			if (_memicmp(buff->L2DATA, "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			if (_memicmp(buff->L2DATA, "FEC\r", 4) == 0 || _memicmp(buff->L2DATA, "FEC ", 4) == 0)
			{
				TNC->FECMode = TRUE;
				TNC->FECIDTimer = 0;
				send(TNC->TCPSock,"FECRCV TRUE\r\nFECRCV\r\n", 21, 0);
		
				if (_memicmp(buff->L2DATA, "FEC 1600", 8) == 0)
					TNC->FEC1600 = TRUE;
				else
					TNC->FEC1600 = FALSE;

				return 0;
			}

			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80] = "CONNECT ";

				memcpy(&Connect[8], &buff->L2DATA[2], txlen);
				txlen += 6;
				Connect[txlen++] = 0x0a;
				Connect[txlen] = 0;

				_strupr(Connect);

				ChangeMYC(TNC, TNC->Streams[0].MyCall);

				// See if Busy
				
				if (InterlockedCheckBusy(TNC))
				{
					// Channel Busy. Unless override set, wait

					if (TNC->OverrideBusy == 0)
					{
						// Save Command, and wait up to 10 secs
						
						sprintf(TNC->WEB_TNCSTATE, "Waiting for clear channel");
						SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

						TNC->ConnectCmd = _strdup(Connect);
						TNC->BusyDelay = TNC->BusyWait * 10;		// BusyWait secs
						return 0;
					}
				}

				TNC->OverrideBusy = FALSE;

				bytes = send(TNC->TCPSock, Connect, (int)txlen, 0);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &Connect[8], txlen-10);

				sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
			}
			else
			{
				buff->L2DATA[txlen++] = 0x0a;
				bytes = send(TNC->TCPSock, &buff->L2DATA[0], (int)txlen, 0);
			}
		}
		if (bytes != txlen)
		{

			// WINMOR doesn't seem to recover from a blocked write. For now just reset
			
			winerr = WSAGetLastError();
			sprintf(ErrMsg, "WINMOR Write Failed for port %d - error code = %d\r\n", port, winerr);
			WritetoConsole(ErrMsg);	
			closesocket(TNC->TCPSock);	
			TNC->CONNECTED = FALSE;

			return (0);
		}

		return (0);

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		if (TNC->Streams[0].Attached == 0)
			return TNC->CONNECTED << 8 | 1;

		return (TNC->CONNECTED << 8 | TNC->Streams[0].Disconnecting << 15);		// OK
			
		break;

	case 4:				// reinit

		return (0);

	case 5:				// Close

		send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);
		Sleep(100);
		shutdown(TNC->TCPDataSock, SD_BOTH);
		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->TCPDataSock);
		closesocket(TNC->TCPSock);

		if (TNC->PID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
		}

		return (0);

	case 6:				// Scan Stop Interface

		Param = (size_t)buff;

		if (Param == 2)		// Check  Permission (shouldn't happen)
		{
			Debugprintf("Scan Check Permission called on FLDIGI");
			return 1;		// OK to change
		}

		if (!TNC->TCPSock)
			return 0;					// No connection so no interlock
	
		if (Param == 1)		// Request Permission
		{
			if (TNC->ConnectPending)
				return TRUE;	// Not OK to Change

			if (TNC->CONNECTED)
			{
				TNC->GavePermission = TRUE;
				send(TNC->TCPSock, "LISTEN FALSE\r\n", 14, 0);
			}
			return FALSE;
		}

		if (Param == 3)		// Release  Permission
		{
			if (TNC->CONNECTED)
			{
				if (TNC->GavePermission)
				{
					TNC->GavePermission = FALSE;
					send(TNC->TCPSock, "LISTEN TRUE\r\n", 13, 0);
				}
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;


		if (Scan->Bandwidth == 'W')		// Set Wide Mode
		{
			if (TNC->WinmorCurrentMode != 1600)
			{
				if (TNC->WinmorCurrentMode == 0)
					if (TNC->CONNECTED)
						send(TNC->TCPSock, "LISTEN TRUE\r\n", 13, 0);

				if (TNC->CONNECTED)
					send(TNC->TCPSock, "BW 1600\r\n", 9, 0);
				TNC->WinmorCurrentMode = 1600;
			}
			TNC->WL2KMode = 22;
			return 0;
		}


		if (Scan->Bandwidth == 'N')		// Set Narrow Mode
		{
			if (TNC->WinmorCurrentMode != 500)
			{
				if (TNC->WinmorCurrentMode == 0)
					if (TNC->CONNECTED)
						send(TNC->TCPSock, "LISTEN TRUE\r\n", 13, 0);

				TNC->WinmorCurrentMode = 500;
				if (TNC->CONNECTED)
					send(TNC->TCPSock, "BW 500\r\n", 8, 0);
			}
			TNC->WL2KMode = 21;
			return 0;
		}

		if (Scan->Bandwidth == 'X')		// Dont Allow Connects
		{
			if (TNC->WinmorCurrentMode != 0)
			{
				if (TNC->CONNECTED)
					send(TNC->TCPSock, "LISTEN FALSE\r\n", 14, 0);
				TNC->WinmorCurrentMode = 0;
			}

			TNC->WL2KMode = 0;
			return 0;
		}

		return 0;
	}
	return 0;
}

VOID ReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[256];
	char wtext[100];

	ChangeMYC(TNC, TNC->NodeCall);

	if (TNC->CONNECTED)
		send(TNC->TCPSock, "LISTEN TRUE\r\nMAXCONREQ 4\r\n", 26, 0);

	strcpy(TNC->WEB_TNCSTATE, "Free");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	if (TNC->hWnd)
	{
		sprintf (wtext, "WINMOR Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
		MySetWindowText(TNC->hWnd, wtext);
	}

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command(-1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID SuspendOtherPorts(struct TNCINFO * ThisTNC)
{
	// Disable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i;
	int rxInterlock = ThisTNC->RXRadio;
	int txInterlock = ThisTNC->TXRadio;

	if (rxInterlock == 0 || txInterlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

		if (rxInterlock == TNC->RXRadio || txInterlock == TNC->TXRadio)	// Same Group	
			if (TNC->SuspendPortProc)
				TNC->SuspendPortProc(TNC);
	}
}

VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC)
{
	// Enable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i;
	int rxInterlock = ThisTNC->RXRadio;
	int txInterlock = ThisTNC->TXRadio;

	if (rxInterlock == 0 && txInterlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

		if (rxInterlock == TNC->RXRadio || txInterlock == TNC->TXRadio)	// Same Group	
			if (TNC->ReleasePortProc)
				TNC->ReleasePortProc(TNC);
	}
}

VOID WinmorSuspendPort(struct TNCINFO * TNC)
{
	if (TNC->CONNECTED)
		send(TNC->TCPSock, "CODEC FALSE\r\n", 14, 0);

	if (TNC->Busy)
	{
		TNC->Busy = FALSE;		// Can't clear detector if CODEC off.
		MySetWindowText(TNC->xIDC_CHANSTATE, "Clear");
		strcpy(TNC->WEB_CHANSTATE, "Clear");
	}
}

VOID WinmorReleasePort(struct TNCINFO * TNC)
{
	if (TNC->CONNECTED)
		send(TNC->TCPSock, "CODEC TRUE\r\n", 13, 0);
}

extern char WebProcTemplate[];
extern char sliderBit[];

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, WebProcTemplate, TNC->Port, TNC->Port, "WINMOR Status", "WINMOR Status");

	if (TNC->TXFreq)
		Len += sprintf(&Buff[Len], sliderBit, TNC->TXOffset, TNC->TXOffset);


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


void * WinmorExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	APPLCALLS * APPL;
	struct TNCINFO * TNC;
	char Aux[100] = "MYAUX ";
	char Appl[11];
	char * TempScript;

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

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = RestartTNC(TNC);

	TNC->Hardware = H_WINMOR;

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
	PortEntry->MAXHOSTMODESESSIONS = 1;	
	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = WinmorSuspendPort;
	TNC->ReleasePortProc = WinmorReleasePort;

	TNC->ModemCentre = 1500;				// WINMOR is always 1500 Offset

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = malloc(1000);

	strcpy(TempScript, "DebugLog True\r\n");
	strcat(TempScript, "CWID False\r\n");
	strcat(TempScript, "BW 1600\r\n");
	strcat(TempScript, "ROBUST False\r\n");
	strcat(TempScript, "MODE AUTO\r\n");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	TNC->WL2KMode = 22;				// in case not scanning

	// Set MYCALL

	strcat(TNC->InitScript,"FECRCV True\r\n");
	strcat(TNC->InitScript,"AUTOBREAK True\r\n");

	sprintf(Msg, "MYC %s\r\nCODEC TRUE\r\nLISTEN TRUE\r\nMYC\r\n", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);
	strcat(TNC->InitScript,"PROCESSID\r\n");

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
	strcat(TNC->InitScript, Aux);
	strcat(TNC->InitScript,"\r\nMYAUX\r\n");

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


#ifndef LINBPQ

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450, ForcedClose);

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

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill Winmor TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart Winmor TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after failed Connection");
	AppendMenu(TNC->hMenu, MF_STRING, ARDOP_ABORT, "Abort Current Session");

	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	MoveWindows(TNC);
#endif
	Consoleprintf("WINMOR Host %s %d", TNC->HostName, htons(TNC->destaddr.sin_port));

	ConnecttoWINMOR(port);

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}

int ConnecttoWINMOR(int port)
{
	_beginthread(WINMORThread, 0, (void *)(size_t)port);

	return 0;
}

VOID WINMORThread(void * portptr)
{
	// Opens both sockets and looks for data on control socket. Data socket is polled from BG,
	// but we need fast response to control messages for PTT porcessing

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
	if (TNC->HostName == NULL)
		return;

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
		closesocket(TNC->TCPSock);

	TNC->TCPSock = 0;
	
	if (TNC->TCPDataSock)
		closesocket(TNC->TCPDataSock);

	TNC->TCPDataSock = 0;

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}
 
	setsockopt (TNC->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->TCPSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=sprintf(Msg, "Bind Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);
			
		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
	 	TNC->CONNECTING = FALSE;

  	 	return; 
	}

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
			err=WSAGetLastError();
   			i=sprintf(Msg, "Connect Failed for WINMOR socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPSock);
	 	TNC->CONNECTING = FALSE;
		TNC->TCPSock = 0;

		return;
	}

	Sleep(1000);

	TNC->LastFreq = 0;			//	so V4 display will be updated

	TNC->TCPDataSock=socket(AF_INET,SOCK_STREAM,0);

	setsockopt (TNC->TCPDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	if (TNC->TCPDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
	 	TNC->CONNECTING = FALSE;

  	 	return; 
	}
 
	if (bind(TNC->TCPDataSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=sprintf(Msg, "Bind Failed for WINMOR Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);
		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;
		TNC->CONNECTING = FALSE;

  	 	return; 
	}

	if (connect(TNC->TCPDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		ioctlsocket (TNC->TCPDataSock,FIONBIO,&param);		// Set nonblocking
		TNC->CONNECTED = TRUE;
	 	TNC->CONNECTING = FALSE;

		// Send INIT script

		send(TNC->TCPSock, TNC->InitScript , (int)strlen(TNC->InitScript), 0);
		TNC->Alerted = TRUE;

		if (TNC->Hardware == H_V4)
			sprintf(TNC->WEB_COMMSSTATE, "Connected to V4 TNC");
		else
			sprintf(TNC->WEB_COMMSSTATE, "Connected to WINMOR TNC");
		
		GetSemaphore(&Semaphore, 40);
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
		FreeSemaphore(&Semaphore);

	}
	else
	{
		sprintf(Msg, "Connect Failed for WINMOR Data socket Port %d - error code = %d\r\n", port, WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);
		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;
	 	TNC->CONNECTING = FALSE;

		return;
	}

	TNC->HeartBeat = 0;

	while (TRUE)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TNC->TCPSock,&readfs);
		FD_SET(TNC->TCPSock,&errorfs);

		timeout.tv_sec = 90;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select((int)TNC->TCPSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			printf("Select failed %d ", WSAGetLastError());
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TNC->TCPSock, &readfs))
			{
				if (TNC->Hardware == H_V4)
					V4ProcessReceivedData(TNC);			
				else
					ProcessReceivedData(TNC);
			}
								
			if (FD_ISSET(TNC->TCPSock, &errorfs))
			{
Lost:				
				sprintf(Msg, "WINMOR Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);
				
				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				GetSemaphore(&Semaphore, 40);
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
				FreeSemaphore(&Semaphore);
			
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
		}
		else
		{
			// 90 secs without data. Shouldn't happen

			sprintf(Msg, "WINMOR Connection Timeout Port %d\r\n", TNC->Port);
			WritetoConsole(Msg);

			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
			GetSemaphore(&Semaphore, 40);
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			FreeSemaphore(&Semaphore);
	
			TNC->CONNECTED = FALSE;
			TNC->Alerted = FALSE;

			if (TNC->PTTMode)
				Rig_PTT(TNC, FALSE);			// Make sure PTT is down

			if (TNC->Streams[0].Attached)
				TNC->Streams[0].ReportDISC = TRUE;

			send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);
	
			Sleep(100);
			shutdown(TNC->TCPDataSock, SD_BOTH);
			shutdown(TNC->TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->TCPDataSock);
			closesocket(TNC->TCPSock);
			TNC->TCPDataSock = 0;
			TNC->TCPSock= 0;

			if (TNC->PID && TNC->WeStartedTNC)
			{
				KillTNC(TNC);
				RestartTNC(TNC);
			}
			return;
		}
	}
}

#ifdef WIN32

BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"WINMOR Sound Card TNC", 21) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->PID == ProcessId)
		{
			 // Our Process

			TNC->hWnd = hwnd;		// save so we can reset title when sessicn closes
			sprintf (wtext, "WINMOR Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}
#endif

VOID ProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	// Response on WINMOR control channel. Could be a reply to a command, or
	// an Async  Response

	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	Buffer[MsgLen - 2] = 0;

	if (_memicmp(Buffer, "FAULT failure to Restart Sound card", 20) == 0)
	{
		Debugprintf(Buffer);
	
		// Force a restart

			send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);
			send(TNC->TCPSock, "CODEC TRUE\r\n", 12, 0);
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

			send(TNC->TCPSock, "CODEC FALSE\r\n", 13, 0);
			send(TNC->TCPSock, "CODEC TRUE\r\n", 12, 0);
		}
		return;
	}
	
	Buffer[MsgLen - 2] = 0;			// Remove CRLF

	if (_memicmp(Buffer, "PTT T", 5) == 0)
	{
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		if (TNC->PTTMode)
			Rig_PTT(TNC, TRUE);
		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		if (TNC->PTTMode)
			Rig_PTT(TNC, FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		SetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		strcpy(TNC->WEB_CHANSTATE, "Busy");

		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->BusyHold)
			strcpy(TNC->WEB_CHANSTATE, "BusyHold");
		else
			strcpy(TNC->WEB_CHANSTATE, "Clear");

		SetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}

	if (_memicmp(Buffer, "TARGET", 6) == 0)
	{
		Debugprintf(Buffer);
		GetSemaphore(&Semaphore, 50);
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		FreeSemaphore(&Semaphore);
		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "OFFSET", 6) == 0)
	{
//		WritetoTrace(TNC, Buffer, MsgLen - 2);
//		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "CONNECTED", 9) == 0)
	{
		char Call[11];
		char * ptr;
		APPLCALLS * APPL;
		char * ApplPtr = APPLS;
		int App;
		char Appl[10];
		struct WL2KInfo * WL2K = TNC->WL2K;

		Debugprintf(Buffer);

		GetSemaphore(&Semaphore, 50);
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		FreeSemaphore(&Semaphore);

		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = STREAM->PacketsSent = 0;

		if (TNC->StartInRobust)
			send(TNC->TCPSock, "ROBUST TRUE\r\n", 13, 0);

		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
		{
			TRANSPORTENTRY * SESS;
			
			// Incomming Connect

			TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			GetSemaphore(&Semaphore, 50);

			ProcessIncommingConnectEx(TNC, Call, 0, TRUE, TRUE);
			FreeSemaphore(&Semaphore);
				
			SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];
			
			SESS->Mode = TNC->WL2KMode;

			if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
			{
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
				SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
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
					char Status[64];

					TidyClose(TNC, 0);
					sprintf(Status, "%d SCANSTART 15", TNC->Port);
					Rig_Command(-1, Status);
					Debugprintf("WINMOR Call from %s rejected", Call);
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
						Debugprintf("WINMOR Call from %s not in ValidCalls - rejected", Call);
						return;
					}
				}
			}

			if (STREAM->BPQtoPACTOR_Q)		//Used for CTEXT
			{
				PMSGWITHLEN buffptr = Q_REM(&STREAM->BPQtoPACTOR_Q);

				send(TNC->TCPDataSock, buffptr->Data, (int)buffptr->Len, 0);
				STREAM->BytesTXed += (int)buffptr->Len;
				WritetoTrace(TNC, buffptr->Data, (int)buffptr->Len);
				ReleaseBuffer(buffptr);
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

					GetSemaphore(&Semaphore, 50);

					buffptr = GetBuff();

					if (buffptr == 0)
					{
						FreeSemaphore(&Semaphore);
						return;			// No buffers, so ignore
					}

					buffptr->Len = MsgLen;
					memcpy(buffptr->Data, Buffer, MsgLen);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
		
					FreeSemaphore(&Semaphore);

					TNC->SwallowSignon = TRUE;

					// Save Appl Call in case needed for 

				}
				else
				{
					char Msg[] = "Application not available\r\n";
					
					// Send a Message, then a disconenct
					
					send(TNC->TCPDataSock, Msg, (int)strlen(Msg), 0);
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

			GetSemaphore(&Semaphore, 50);
			
			buffptr = GetBuff();

			if (buffptr == 0)
			{
				FreeSemaphore(&Semaphore);
				return;			// No buffers, so ignore
			}
			ReplyLen = sprintf(Reply, "*** Connected to %s\r", &Buffer[10]);

			buffptr->Len = ReplyLen;
			memcpy(buffptr->Data, Reply, ReplyLen);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			FreeSemaphore(&Semaphore);	

			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = TRUE;			// Subsequent data to data channel


			if (TNC->RIG)
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s",  TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
			else
				sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			
			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

			UpdateMH(TNC, Call, '+', 'O');
			return;
		}
	}

	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0)
	{
		Debugprintf(Buffer);

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

			GetSemaphore(&Semaphore, 50);

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				FreeSemaphore(&Semaphore);
				return;			// No buffers, so ignore
			}

			buffptr->Len = sprintf(buffptr->Data, "Winmor} Failure with %s\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			FreeSemaphore(&Semaphore);

			if (TNC->RestartAfterFailure)
			{
				if (TNC->PID)
				{
					KillTNC(TNC);
					RestartTNC(TNC);
				}
			}

			return;
		}


		// Release Session

		if (TNC->Streams[0].Connected)
		{
			// Create a traffic record
		
			char logmsg[120];	
			time_t Duration;

			Duration = time(NULL) - STREAM->ConnectTime;

			if (Duration == 0)
				Duration = 1;		// Avoid zero divide
	
			sprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, (int)(STREAM->BytesTXed/Duration),
				STREAM->BytesRXed, (int)(STREAM->BytesRXed/Duration), (int)Duration);

			Debugprintf(logmsg);

			GetSemaphore(&Semaphore, 50);
			WritetoTrace(TNC, Buffer, MsgLen - 2);
			FreeSemaphore(&Semaphore);
		}


		TNC->Streams[0].Connecting = FALSE;
		TNC->Streams[0].Connected = FALSE;		// Back to Command Mode
		TNC->Streams[0].ReportDISC = TRUE;		// Tell Node

		if (TNC->Streams[0].Disconnecting)		// 
			ReleaseTNC(TNC);

		TNC->Streams[0].Disconnecting = FALSE;

		return;
	}

	if (_memicmp(Buffer, "MONCALL", 7) == 0)
	{
		Debugprintf(Buffer);

		// Add to MHEARD

		GetSemaphore(&Semaphore, 50);
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		FreeSemaphore(&Semaphore);
		UpdateMH(TNC, &Buffer[8], '!', 0);
		
		if (!TNC->FECMode)
			return;							// If in FEC mode pass ID messages to user.
	}
		
	if (_memicmp(Buffer, "CMD", 3) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "BUFFERS", 7) == 0)
	{
		int inq, inrx, Sent, BPM;

		sscanf(&Buffer[8], "%d%d%d%d%d", &inq, &inrx, &TNC->Streams[0].BytesOutstanding, &Sent, &BPM);

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
//				send(TNC->TCPSock,"OVER\r\n", 6, 0);

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

		SetWindowText(TNC->xIDC_TRAFFIC, &Buffer[8]);
		strcpy(TNC->WEB_TRAFFIC, &Buffer[8]);
		return;
	}

	Debugprintf(Buffer);

	if (_memicmp(Buffer, "MODE", 4) == 0)
	{
	//	Debugprintf("WINMOR RX: %s", Buffer);

		strcpy(TNC->WEB_MODE, &Buffer[5]);
		GetSemaphore(&Semaphore, 50);
		MySetWindowText(TNC->xIDC_MODE, &Buffer[5]);
		FreeSemaphore(&Semaphore);
		return;
	}

	if (_memicmp(Buffer, "PENDING", 6) == 0)
		return;

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	if (_memicmp(Buffer, "NEWSTATE", 8) == 0)
	{
		TNC->WinmorRestartCodecTimer = time(NULL);

		SetWindowText(TNC->xIDC_PROTOSTATE, &Buffer[9]);
		strcpy(TNC->WEB_PROTOSTATE,  &Buffer[9]);

		if (_memicmp(&Buffer[9], "CONNECTPENDING", 14) == 0)	// Save Pending state for scan control
			TNC->ConnectPending = TRUE;
		else
			TNC->ConnectPending = FALSE;
	
		if (_memicmp(&Buffer[9], "DISCONNECTING", 13) == 0)	// So we can timout stuck discpending
		{
			TNC->DiscPending = 600;
			return;
		}
		if (_memicmp(&Buffer[9], "DISCONNECTED", 12) == 0)
		{
			TNC->DiscPending = FALSE;
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

		TNC->PID = atoi(&Buffer[10]);

#ifdef WIN32

		// Get the File Name in case we want to restart it.

		if (TNC->ProgramPath == NULL)
		{
			if (GetModuleFileNameExPtr)
			{
				hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->PID);
	
				if (hProc)
				{
					GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);
					CloseHandle(hProc);

					TNC->ProgramPath = _strdup(ExeName);
				}
			}
		}

		// Set Window Title to reflect BPQ Port Description

		EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);
#endif
	}

	if ((_memicmp(Buffer, "FAULT Not from state FEC", 24) == 0) || (_memicmp(Buffer, "FAULT Blocked by Busy Lock", 24) == 0))
	{
		if (TNC->FECMode)
		{
			Sleep(1000);
			
			if (TNC->FEC1600)
				send(TNC->TCPSock,"FECSEND 1600\r\n", 14, 0);
			else
				send(TNC->TCPSock,"FECSEND 500\r\n", 13, 0);
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
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	if (_memicmp(Buffer, "OVER", 4) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	GetSemaphore(&Semaphore, 50);

	buffptr = GetBuff();

	if (buffptr == 0)
	{
		FreeSemaphore(&Semaphore);
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf(buffptr->Data, "Winmor} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
	FreeSemaphore(&Semaphore);
}

static int ProcessReceivedData(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (TNC->InputLen > 1000)	// Shouldnt have lines longer  than this on command connection
		TNC->InputLen=0;
				
	InputLen=recv(TNC->TCPSock, &TNC->TCPBuffer[TNC->InputLen], 1000 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		closesocket(TNC->TCPSock);
		closesocket(TNC->TCPDataSock);

		TNC->TCPSock = 0;
		TNC->TCPDataSock = 0;

		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		return 0;					
	}

	TNC->InputLen += InputLen;

loop:
	
	ptr = memchr(TNC->TCPBuffer, '\n', TNC->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &TNC->TCPBuffer[TNC->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			ProcessResponse(TNC, TNC->TCPBuffer, TNC->InputLen);
			TNC->InputLen=0;
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = TNC->InputLen - (int)(ptr2-ptr);

			memcpy(Buffer, TNC->TCPBuffer, MsgLen);

			ProcessResponse(TNC, Buffer, MsgLen);
			memmove(TNC->TCPBuffer, ptr, TNC->InputLen-MsgLen);

			TNC->InputLen -= MsgLen;
			goto loop;
		}
	}
	return 0;
}


VOID ProcessDataSocketData(int port)
{
	// Info on Data Socket - just packetize and send on
	
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int InputLen, PacLen = 236;
	PMSGWITHLEN buffptr;
	char * msg;
		
	TNC->TimeSinceLast = 0;

loop:
	buffptr = GetBuff();

	if (buffptr == NULL) return;			// No buffers, so ignore
			
	InputLen = recv(TNC->TCPDataSock, buffptr->Data, PacLen, 0);

	if (InputLen == -1)
	{
		ReleaseBuffer(buffptr);
		return;
	}


	//Debugprintf("Winmor: RXD %d bytes", InputLen);

	if (InputLen == 0)
	{
		// Does this mean closed?
		
		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		ReleaseBuffer(buffptr);
		return;					
	}

	STREAM->BytesRXed += InputLen;

	msg = &buffptr->Data[0];
	msg[InputLen] = 0;	
	
	WritetoTrace(TNC, msg, InputLen);
		
	if (TNC->FECMode)
	{	
		InputLen = (int)strlen(&buffptr->Data[0]);

		if (msg[InputLen - 1] == 3)		// End of errored block
			msg[InputLen++] = 13;		// Add CR

	}
	buffptr->Len = InputLen;
	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

	goto loop;
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

#ifdef LINBPQ
#include <signal.h>
#endif


int KillTNC(struct TNCINFO * TNC)
{
	if (TNC->ProgramPath && _memicmp(TNC->ProgramPath, "REMOTE:", 7) == 0)
	{
		// Try to Kill TNC on a remote host

		SOCKET sock = socket(AF_INET,SOCK_DGRAM,0);
		struct sockaddr_in destaddr;
		char Msg[80];
		int Len;

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

		if (TNC->PID)
			Len = sprintf(Msg, "KILL %d", TNC->PID);
		else
			Len = sprintf(Msg, "KILLBYNAME %s", &TNC->ProgramPath[7]);

		sendto(sock, Msg, Len, 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
		Sleep(100);
		closesocket(sock);

		TNC->PID = 0;			// So we don't try again
		return 1;				// Cant tell if it worked, but assume ok
	}

	if (TNC->PID == 0)
		return 0;

#ifdef WIN32
	{
		HANDLE hProc;

		Debugprintf("KillTNC Called for Pid %d", TNC->PID);

		if (TNC->PTTMode)
			Rig_PTT(TNC, FALSE);			// Make sure PTT is down

		hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->PID);

		if (hProc)
		{
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
		}
	}
#else

	printf("KillTNC Called for Pid %d Returned %d\n", TNC->PID, kill(TNC->PID, SIGTERM));

#endif
	TNC->PID = 0;			// So we don't try again

	return 0;
}

BOOL RestartTNC(struct TNCINFO * TNC)
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

	// Extract any parameters from command string

#ifndef WIN32
	{
		char * arg_list[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		pid_t child_pid;
		char * Copy, * Context;
		signal(SIGCHLD, SIG_IGN); // Silently (and portably) reap children. 

		Copy = _strdup(TNC->ProgramPath);	// Save as strtok mangles it

		arg_list[0] = strtok_s(Copy, " \n\r", &Context);
		if (arg_list[0])
			arg_list[1] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[1])
			arg_list[2] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[2])
			arg_list[3] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[3])
			arg_list[4] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[4])
			arg_list[5] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[5])
			arg_list[6] = strtok_s(NULL, " \n\r", &Context);
		if (arg_list[6])
			arg_list[7] = strtok_s(NULL, " \n\r", &Context);

		//	Fork and Exec TNC

		printf("Trying to start %s\n", TNC->ProgramPath);

		/* Duplicate this process. */ 

		child_pid = fork (); 

		if (child_pid == -1) 
 		{    				
			printf ("StartTNC fork() Failed\n"); 
			free(Copy);
			return 0;
		}

		if (child_pid == 0) 
 		{    				
			execvp (arg_list[0], arg_list); 
        
			/* The execvp  function returns only if an error occurs.  */ 

			printf ("Failed to start TNC\n"); 
			exit(0);			// Kill the new process
		}
		else
		{
			TNC->PID = child_pid;
			printf("Started TNC, Process ID = %d\n", TNC->PID);
		}
		free(Copy);
		return TRUE;
	}								 
#else

	{
		int n = 0;
		
		STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
	    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
//		char workingDirectory[256];

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
	  	SInfo.lpReserved2=NULL; 

		Debugprintf("RestartTNC Called for %s", TNC->ProgramPath);

		while (KillOldTNC(TNC->ProgramPath) && n++ < 100)
		{
			Sleep(100);
		}


		if (CreateProcess(NULL, TNC->ProgramPath, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo))
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

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[0].BytesOutstanding == 0)
		send(TNC->TCPSock,"DISCONNECT\r\n", 12, 0);
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	send(TNC->TCPSock,"DIRTYDISCONNECT\r\n", 17, 0);
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	ReleaseTNC(TNC);

	if (TNC->FECMode)
	{
		TNC->FECMode = FALSE;
		send(TNC->TCPSock,"SENDID 0\r\n", 10, 0);
	}
}

BOOL KillOldTNC(char * Path)
{
#ifdef WIN32
	HANDLE hProc;
	char ExeName[256] = "";
	DWORD Pid = 0;

    DWORD Processes[1024], Needed, Count;
    unsigned int i;

	if (EnumProcessesPtr == NULL)
		return FALSE;

    if (!EnumProcessesPtr(Processes, sizeof(Processes), &Needed))
        return FALSE;
    
    // Calculate how many process identifiers were returned.

    Count = Needed / sizeof(DWORD);

    for (i = 0; i < Count; i++)
    {
        if (Processes[i] != 0)
        {
			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Processes[i]);
	
			if (hProc)
			{
				GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);

				// Path could have parameters, so use memcmp

				if (_memicmp(ExeName, Path, strlen(ExeName)) == 0)
				{
					Debugprintf("Killing Pid %d %s", Processes[i], ExeName);
					TerminateProcess(hProc, 0);
					CloseHandle(hProc);
					return TRUE;
				}
				CloseHandle(hProc);
			}
		}
	}
#endif
	return FALSE;
}
