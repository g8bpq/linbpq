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
//	DLL to provide interface to allow G8BPQ switch to use UZ7HOPE as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//

// Interlock and scanning with UZ7HO driver.

// A UZ7HO port can be used in much the same way as any other HF port, so that it only allows one connect at
// a time and takes part in Interlock and Scan Control proessing. But it can also be used as a multisession
// driver so it does need special treatment.


#define _CRT_SECURE_NO_DEPRECATE

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#include "CHeaders.h"
#include "tncinfo.h"

#include "bpq32.h"

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#define TIMESTAMP 352

#define CONTIMEOUT 1200

#define AGWHDDRLEN sizeof(struct AGWHEADER)

extern int (WINAPI FAR *GetModuleFileNameExPtr)();

//int ResetExtDriver(int num);
extern char * PortConfig[33];

extern struct TNCINFO * TNCInfo[41];		// Records are Malloc'd

void ConnecttoUZ7HOThread(void * portptr);

void CreateMHWindow();
int Update_MH_List(struct in_addr ipad, char * call, char proto);

int ConnecttoUZ7HO();
static int ProcessReceivedData(int bpqport);
static int ProcessLine(char * buf, int Port);
int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);
VOID ProcessAGWPacket(struct TNCINFO * TNC, UCHAR * Message);
struct TNCINFO * GetSessionKey(char * key, struct TNCINFO * TNC);
static VOID SendData(int Stream, struct TNCINFO * TNC, char * key, char * Msg, int MsgLen);
static VOID DoMonitorHddr(struct TNCINFO * TNC, struct AGWHEADER * RXHeader, UCHAR * Msg);
VOID SendRPBeacon(struct TNCINFO * TNC);
VOID MHPROC(struct PORTCONTROL * PORT, MESSAGE * Buffer);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
int standardParams(struct TNCINFO * TNC, char * buf);

extern UCHAR BPQDirectory[];

#define MAXBPQPORTS 32
#define MAXUZ7HOPORTS 16

static char ClassName[]="ARDOPSTATUS";
static char WindowTitle[] = "UZ7HO";
static int RigControlRow = 165;


//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static int UZ7HOChannel[MAXBPQPORTS+1];				// BPQ Port to UZ7HO Port
static int BPQPort[MAXUZ7HOPORTS][MAXBPQPORTS+1];	// UZ7HO Port and Connection to BPQ Port
static void * UZ7HOtoBPQ_Q[MAXBPQPORTS+1];			// Frames for BPQ, indexed by BPQ Port
static void * BPQtoUZ7HO_Q[MAXBPQPORTS+1];			// Frames for UZ7HO. indexed by UZ7HO port. Only used it TCP session is blocked

static int MasterPort[MAXBPQPORTS+1];			// Pointer to first BPQ port for a specific UZ7HO host
static struct TNCINFO * SlaveTNC[MAXBPQPORTS+1];// TNC Record Slave if present

//	Each port may be on a different machine. We only open one connection to each UZ7HO instance

static char * UZ7HOSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static unsigned int UZ7HOInst = 0;
static int AttachedProcesses=0;

static HWND hResWnd,hMHWnd;
static BOOL GotMsg;

static HANDLE STDOUT=0;

//SOCKET sock;

static  struct sockaddr_in  sinx; 
static  struct sockaddr_in  rxaddr;
static  struct sockaddr_in  destaddr[MAXBPQPORTS+1];

static int addrlen=sizeof(sinx);

//static short UZ7HOPort=0;

static time_t ltime,lasttime[MAXBPQPORTS+1];

static BOOL CONNECTING[MAXBPQPORTS+1];
static BOOL CONNECTED[MAXBPQPORTS+1];

//HANDLE hInstance;


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

unsigned int reverse(unsigned int val)
{
	char x[4];
	char y[4];

	memcpy(x, &val,4);
	y[0] = x[3];
	y[1] = x[2];
	y[2] = x[1];
	y[3] = x[0];

	memcpy(&val, y, 4);

	return val;
}


#ifndef LINBPQ

static BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;
	char FN[MAX_PATH] = "";
	HANDLE hProc;

	if (TNC->ProgramPath == NULL)
		return FALSE;

	GetWindowText(hwnd,wtext,99);

	if (strstr(wtext,"SoundModem"))
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->PID == ProcessId)
		{
			 // Our Process

			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);

			if (hProc && GetModuleFileNameExPtr)
			{
				GetModuleFileNameExPtr(hProc, NULL, FN, MAX_PATH);

				// Make sure this is the right copy

				CloseHandle(hProc);

				if (_stricmp(FN, TNC->ProgramPath))
					return TRUE;					//Wrong Copy
			}

			TNC->PID = ProcessId;

			sprintf (wtext, "Soundmodem - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}
#endif



void RegisterAPPLCalls(struct TNCINFO * TNC, BOOL Unregister)
{
	// Register/Deregister Nodecall and all applcalls

	struct AGWINFO * AGW;
	APPLCALLS * APPL;
	char * ApplPtr = APPLS;
	int App;
	char Appl[10];
	char * ptr;

	char NodeCall[11];

	memcpy(NodeCall, MYNODECALL, 10);
	strlop(NodeCall, ' ');

	AGW = TNC->AGWInfo;

	
	AGW->TXHeader.Port=0;
	AGW->TXHeader.DataLength=0;

	if (Unregister)
		AGW->TXHeader.DataKind = 'x';		// UnRegister
	else
		AGW->TXHeader.DataKind = 'X';		// Register

	memset(AGW->TXHeader.callfrom, 0, 10);
	strcpy(AGW->TXHeader.callfrom, TNC->NodeCall);
	send(TNC->TCPSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);
					
	memset(AGW->TXHeader.callfrom, 0, 10);
	strcpy(AGW->TXHeader.callfrom, NodeCall);
	send(TNC->TCPSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);

	// Add Alias

	memcpy(NodeCall, MYALIASTEXT, 10);
	strlop(NodeCall, ' ');
	memset(AGW->TXHeader.callfrom, 0, 10);
	strcpy(AGW->TXHeader.callfrom, NodeCall);
	send(TNC->TCPSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);

	
	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];
		memcpy(Appl, APPL->APPLCALL_TEXT, 10);
		ptr=strchr(Appl, ' ');

		if (ptr)
			*ptr = 0;

		if (Appl[0])
		{
			memset(AGW->TXHeader.callfrom, 0, 10);
			strcpy(AGW->TXHeader.callfrom, Appl);
			send(TNC->TCPSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);
		}
	}
}


BOOL UZ7HOStopPort(struct PORTCONTROL * PORT)
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

	KillTNC(TNC);

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Stopped");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}

int ConnecttoUZ7HO(int port);

BOOL UZ7HOStartPort(struct PORTCONTROL * PORT)
{
	// Restart Port - Open Sockets or Serial Port

	struct TNCINFO * TNC = PORT->TNC;

	ConnecttoUZ7HO(TNC->Port);
	TNC->lasttime = time(NULL);;

	sprintf(PORT->TNC->WEB_COMMSSTATE, "%s", "Port Restarted");
	MySetWindowText(PORT->TNC->xIDC_COMMSSTATE, PORT->TNC->WEB_COMMSSTATE);

	return TRUE;
}





VOID UZ7HOSuspendPort(struct TNCINFO * TNC)
{
	TNC->PortRecord->PORTCONTROL.PortSuspended = TRUE;
	RegisterAPPLCalls(TNC, TRUE);
}

VOID UZ7HOReleasePort(struct TNCINFO * TNC)
{
	TNC->PortRecord->PORTCONTROL.PortSuspended = FALSE;
	RegisterAPPLCalls(TNC, FALSE);
}

static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	PMSGWITHLEN buffptr;
	char txbuff[500];
	unsigned int bytes,txlen=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct AGWINFO * AGW;
	int Stream = 0;
	struct STREAMINFO * STREAM;
	int TNCOK;

	if (TNC == NULL)
		return 0;					// Port not defined

	AGW = TNC->AGWInfo;

	// Look for attach on any call

	for (Stream = 0; Stream <= TNC->AGWInfo->MaxSessions; Stream++)
	{
		STREAM = &TNC->Streams[Stream];
	
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			char Cmd[80];

			// New Attach

			int calllen;
			STREAM->Attached = TRUE;

			TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER[6] |= 0x60; // Ensure P or T aren't used on ax.25
			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, STREAM->MyCall);
			STREAM->MyCall[calllen] = 0;
			STREAM->FramesOutstanding = 0;

			// Stop Scanning

			sprintf(Cmd, "%d SCANSTOP", TNC->Port);
			Rig_Command(-1, Cmd);

			SuspendOtherPorts(TNC);			// Prevent connects on other ports in same scan gruop

		}
	}

	switch (fn)
	{
	case 1:				// poll

		if (MasterPort[port] == port)
		{
			// Only on first port using a host

			time(&ltime);

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect

				if (ltime - lasttime[port] > 9)
				{
					ConnecttoUZ7HO(port);
					lasttime[port] = ltime;
				}
			}
			else
			{
				// See if time to refresh registrations

				if (TNC->CONNECTED)
				{
					if (ltime - AGW->LastParamTime > 60)
					{
						AGW->LastParamTime = ltime;

						if (TNC->PortRecord->PORTCONTROL.PortSuspended == FALSE)
							RegisterAPPLCalls(TNC, FALSE);
					}
				}
			}

			FD_ZERO(&readfs);

			if (TNC->CONNECTED) FD_SET(TNC->TCPSock, &readfs);


			FD_ZERO(&writefs);

			if (TNC->CONNECTING) FD_SET(TNC->TCPSock, &writefs);	// Need notification of Connect

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->TCPSock, &writefs);	// Need notification of busy clearing


			FD_ZERO(&errorfs);

			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->TCPSock, &errorfs);

			timeout.tv_sec = 0;
			timeout.tv_usec = 0;

			if (select((int)TNC->TCPSock + 1, &readfs, &writefs, &errorfs, &timeout) > 0)
			{
				//	See what happened

				if (FD_ISSET(TNC->TCPSock, &readfs))
				{
					// data available

					ProcessReceivedData(port);
				}

				if (FD_ISSET(TNC->TCPSock, &writefs))
				{
					if (BPQtoUZ7HO_Q[port] == 0)
					{
						APPLCALLS * APPL;
						char * ApplPtr = APPLS;
						int App;
						char Appl[10];
						char * ptr;

						char NodeCall[11];

						memcpy(NodeCall, MYNODECALL, 10);
						strlop(NodeCall, ' ');

						//	Connect success

						TNC->CONNECTED = TRUE;
						TNC->CONNECTING = FALSE;

						sprintf(TNC->WEB_COMMSSTATE, "Connected to TNC");
						MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

						// If required, send signon

						if (UZ7HOSignon[port])
							send(TNC->TCPSock, UZ7HOSignon[port], 546, 0);

						// Request Raw Frames

						AGW->TXHeader.Port = 0;
						AGW->TXHeader.DataKind = 'k';		// Raw Frames
						AGW->TXHeader.DataLength = 0;
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						AGW->TXHeader.DataKind = 'm';		// Monitor Frames
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						AGW->TXHeader.DataKind = 'R';		// Version
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						AGW->TXHeader.DataKind = 'g';		// Port Capabilities
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						// Register all applcalls

						AGW->TXHeader.DataKind = 'X';		// Register
						memset(AGW->TXHeader.callfrom, 0, 10);
						strcpy(AGW->TXHeader.callfrom, TNC->NodeCall);
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						memset(AGW->TXHeader.callfrom, 0, 10);
						strcpy(AGW->TXHeader.callfrom, NodeCall);
						send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);

						for (App = 0; App < 32; App++)
						{
							APPL = &APPLCALLTABLE[App];
							memcpy(Appl, APPL->APPLCALL_TEXT, 10);
							ptr = strchr(Appl, ' ');

							if (ptr)
								*ptr = 0;

							if (Appl[0])
							{
								memset(AGW->TXHeader.callfrom, 0, 10);
								strcpy(AGW->TXHeader.callfrom, Appl);
								send(TNC->TCPSock, (const char FAR *)&AGW->TXHeader, AGWHDDRLEN, 0);
							}
						}
#ifndef LINBPQ
						EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);
#endif
					}
					else
					{
						// Write block has cleared. Send rest of packet

						buffptr = Q_REM(&BPQtoUZ7HO_Q[port]);

						txlen = (int)buffptr->Len;

						memcpy(txbuff, buffptr->Data, txlen);

						bytes = send(TNC->TCPSock, (const char FAR *)&txbuff, txlen, 0);

						ReleaseBuffer(buffptr);

					}

				}

				if (FD_ISSET(TNC->TCPSock, &errorfs))
				{

					//	if connecting, then failed, if connected then has just disconnected

//					if (CONNECTED[port])
//					if (!CONNECTING[port])
//					{
//						i=sprintf(ErrMsg, "UZ7HO Connection lost for BPQ Port %d\r\n", port);
//						WritetoConsole(ErrMsg);
//					}

					CONNECTING[port] = FALSE;
					CONNECTED[port] = FALSE;

				}

			}

		}

		// See if any frames for this port

		for (Stream = 0; Stream <= TNC->AGWInfo->MaxSessions; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			// Have to time out connects, as TNC doesn't report failure

			if (STREAM->Connecting)
			{
				STREAM->Connecting--;

				if (STREAM->Connecting == 0)
				{
					// Report Connect Failed, and drop back to command mode

					buffptr = GetBuff();

					if (buffptr)
					{
						buffptr->Len = sprintf(buffptr->Data, "UZ7HO} Failure with %s\r", STREAM->RemoteCall);
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}

					STREAM->Connected = FALSE;		// Back to Command Mode
					STREAM->DiscWhenAllSent = 10;

					// Send Disc to TNC

					TidyClose(TNC, Stream);
				}
			}

			if (STREAM->Attached)
				CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

			if (STREAM->ReportDISC)
			{
				STREAM->ReportDISC = FALSE;
				buff->PORT = Stream;

				return -1;
			}

			// if Busy, send buffer status poll

			if (STREAM->Connected && STREAM->FramesOutstanding)
			{
				struct AGWINFO * AGW = TNC->AGWInfo;

				AGW->PollDelay++;

				if (AGW->PollDelay > 10)
				{
					char * Key = &STREAM->AGWKey[0];

					AGW->PollDelay = 0;

					AGW->TXHeader.Port = Key[0] - '1';
					AGW->TXHeader.DataKind = 'Y';
					strcpy(AGW->TXHeader.callfrom, &Key[11]);
					strcpy(AGW->TXHeader.callto, &Key[1]);
					AGW->TXHeader.DataLength = 0;

					send(TNCInfo[MasterPort[port]]->TCPSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
				}
			}

			if (STREAM->PACTORtoBPQ_Q == 0)
			{
				if (STREAM->DiscWhenAllSent)
				{
					STREAM->DiscWhenAllSent--;
					if (STREAM->DiscWhenAllSent == 0)
						STREAM->ReportDISC = TRUE;				// Dont want to leave session attached. Causes too much confusion
				}
			}
			else
			{
				int datalen;

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
		}

		if (TNC->PortRecord->UI_Q)
		{
			struct AGWINFO * AGW = TNC->AGWInfo;

			int MsgLen;
			struct _MESSAGE * buffptr;
			char * Buffer;
			SOCKET Sock;
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);

			if (TNC->PortRecord->PORTCONTROL.PortSuspended == TRUE)		// Interlock Disabled Port
			{
				ReleaseBuffer((UINT *)buffptr);
				return (0);
			}

			Sock = TNCInfo[MasterPort[port]]->TCPSock;

			MsgLen = buffptr->LENGTH - 6;	// 7 Header, need extra Null
			buffptr->LENGTH = 0;				// Need a NULL on front	
			Buffer = &buffptr->DEST[0];		// Raw Frame
			Buffer--;						// Need to send an extra byte on front

			AGW->TXHeader.Port = UZ7HOChannel[port];
			AGW->TXHeader.DataKind = 'K';
			memset(AGW->TXHeader.callfrom, 0, 10);
			memset(AGW->TXHeader.callto, 0, 10);
#ifdef __BIG_ENDIAN__
			AGW->TXHeader.DataLength = reverse(MsgLen);
#else
			AGW->TXHeader.DataLength = MsgLen;
#endif
			send(Sock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
			send(Sock, Buffer, MsgLen, 0);

			ReleaseBuffer((UINT *)buffptr);
		}


		return (0);



	case 2:				// send

		if (TNC->PortRecord->PORTCONTROL.PortSuspended == TRUE)		// Interlock Disabled Port
			return 0;

		if (!TNCInfo[MasterPort[port]]->CONNECTED) return 0;		// Don't try if not connected to TNC

		Stream = buff->PORT;

		STREAM = &TNC->Streams[Stream];
		AGW = TNC->AGWInfo;

		txlen = GetLengthfromBuffer(buff) - (MSGHDDRLEN + 1);		// 1 as no PID
		if (STREAM->Connected)
		{
			SendData(Stream, TNC, &STREAM->AGWKey[0], &buff->L2DATA[0], txlen);
			STREAM->FramesOutstanding++;
		}
		else
		{
			if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0)
			{
				TidyClose(TNC, buff->PORT);
				STREAM->ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			if (STREAM->Connecting)
				return 0;

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

					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "%s", &buff->L2DATA[0]);
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(&buff->L2DATA[0], "INUSE?", 6) == 0)
			{
				// Return Error if in use, OK if not

				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
				int s = 0;

				while (s <= TNC->AGWInfo->MaxSessions)
				{
					if (s != Stream)
					{
						if (TNC->PortRecord->ATTACHEDSESSIONS[s])
						{
							buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Error - In use\r");
							C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
							return 1;							// Busy
						}
					}
					s++;
				}
				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Ok - Not in use\r");
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(&buff->L2DATA[0], "VERSION", 7) == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Version %d.%d.%d.%d\r",
					AGW->Version[0], AGW->Version[1], AGW->Version[2], AGW->Version[3]);

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				return 1;
			}

			if (_memicmp(&buff->L2DATA[0], "FREQ", 4) == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				// May be read or set frequency

				if (txlen == 5)
				{
					// Read Freq

					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Modem Freqency %d\r", AGW->CenterFreq);
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}
					return 1;
				}

				AGW->CenterFreq = atoi(&buff->L2DATA[5]);

				if (AGW->CenterFreq == 0)
				{
					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Invalid Modem Freqency\r");
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}
					return 1;
				}

				if (TNCInfo[MasterPort[port]]->AGWInfo->isQTSM == 3)
				{
					// QtSM so can send Set Freq Command

					char Buffer[32] = "";
					int MsgLen = 32;

					memcpy(Buffer, &AGW->CenterFreq, 4);

					AGW->TXHeader.Port = UZ7HOChannel[port];
					AGW->TXHeader.DataKind = 'g';
					memset(AGW->TXHeader.callfrom, 0, 10);
					memset(AGW->TXHeader.callto, 0, 10);
#ifdef __BIG_ENDIAN__
					AGW->TXHeader.DataLength = reverse(MsgLen);
#else
					AGW->TXHeader.DataLength = MsgLen;
#endif
					send(TNCInfo[MasterPort[port]]->TCPSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
					send(TNCInfo[MasterPort[port]]->TCPSock, Buffer, MsgLen, 0);
				}
#ifdef WIN32
				else if (AGW->hFreq)
				{
					//Using real UZ7HO on Windows

					char Freq[16];
					sprintf(Freq, "%d", AGW->CenterFreq - 1);

					SendMessage(AGW->hFreq, WM_SETTEXT, 0, (LPARAM)Freq);
					SendMessage(AGW->hSpin, WM_LBUTTONDOWN, 1, 1);
					SendMessage(AGW->hSpin, WM_LBUTTONUP, 0, 1);
				}
#endif
				else
				{
					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Sorry Setting UZ7HO params not supported on this system\r");
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}

					return 1;
				}

				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Modem Freq Set Ok\r");
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				}

				return 1;
			}

			if (_memicmp(&buff->L2DATA[0], "MODEM", 5) == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				if (txlen == 6)
				{
					// Read Modem

					if (buffptr)
					{
						if (AGW->ModemName[0])
							buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Modem %s\r", AGW->ModemName);
						else
							buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Modem Number %d\r", AGW->Modem);

						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}
					return 1;
				}
				else if (TNCInfo[MasterPort[port]]->AGWInfo->isQTSM == 3)
				{
					// Can send modem name to QTSM

					char Buffer[32] = "";
					int MsgLen = 32;

					strlop(buff->L2DATA, '\r');
					strlop(buff->L2DATA, '\n');

					if (strlen(&buff->L2DATA[6]) > 20)
						buff->L2DATA[26] = 0;

					strcpy(&Buffer[4], &buff->L2DATA[6]);

					AGW->TXHeader.Port = UZ7HOChannel[port];
					AGW->TXHeader.DataKind = 'g';
					memset(AGW->TXHeader.callfrom, 0, 10);
					memset(AGW->TXHeader.callto, 0, 10);
#ifdef __BIG_ENDIAN__
					AGW->TXHeader.DataLength = reverse(MsgLen);
#else
					AGW->TXHeader.DataLength = MsgLen;
#endif
					send(TNCInfo[MasterPort[port]]->TCPSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
					send(TNCInfo[MasterPort[port]]->TCPSock, Buffer, MsgLen, 0);
				}
#ifdef WIN32
				else if (AGW->cbinfo.cbSize)
				{
					// Real QTSM on Windows

					AGW->Modem = atoi(&buff->L2DATA[6]);

					if (AGW->cbinfo.cbSize && AGW->cbinfo.hwndCombo)
					{
						// Set it

						LRESULT ret = SendMessage(AGW->cbinfo.hwndCombo, CB_SETCURSEL, AGW->Modem, 0);
						int pos = 13 * AGW->Modem + 7;

						ret = SendMessage(AGW->cbinfo.hwndCombo, WM_LBUTTONDOWN, 1, 1);
						ret = SendMessage(AGW->cbinfo.hwndCombo, WM_LBUTTONUP, 0, 1);
						ret = SendMessage(AGW->cbinfo.hwndList, WM_LBUTTONDOWN, 1, pos << 16);
						ret = SendMessage(AGW->cbinfo.hwndList, WM_LBUTTONUP, 0, pos << 16);
						ret = 0;
					}
				}
#endif
				else
				{
					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Sorry Setting UZ7HO params not supported this system\r");
						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}
					return 1;
				}

				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "UZ7HO} Modem Set Ok\r");
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				}
				return 1;
			}
			// See if a Connect Command.

			if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
			{
				struct AGWINFO * AGW = TNC->AGWInfo;
				char ViaList[82] = "";
				int Digis = 0;
				char * viaptr;
				char * ptr;
				char * context;
				int S;
				struct STREAMINFO * TSTREAM;
				char Key[21];
				int sent = 0;

				buff->L2DATA[txlen] = 0;
				_strupr(&buff->L2DATA[0]);

				memset(STREAM->RemoteCall, 0, 10);

				// See if any digis - accept V VIA or nothing, seps space or comma

				ptr = strtok_s(&buff->L2DATA[2], " ,\r", &context);

				if (ptr == 0)
				{
					PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

					if (buffptr)
					{
						buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0],
							"UZ7HO} Error - Call missing from C command\r", STREAM->MyCall, STREAM->RemoteCall);

						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					}

					STREAM->DiscWhenAllSent = 10;
					return 0;
				}

				if (*ptr == '!')
					ptr++;

				strcpy(STREAM->RemoteCall, ptr);

				Key[0] = UZ7HOChannel[port] + '1';
				memset(&Key[1], 0, 20);
				strcpy(&Key[11], STREAM->MyCall);
				strcpy(&Key[1], ptr);

				// Make sure we don't already have a session for this station

				S = 0;

				while (S <= AGW->MaxSessions)
				{
					TSTREAM = &TNC->Streams[S];

					if (memcmp(TSTREAM->AGWKey, Key, 21) == 0)
					{
						// Found it;

						PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

						if (buffptr)
						{
							buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0],
								"UZ7HO} Sorry - Session between %s and %s already Exists\r", STREAM->MyCall, STREAM->RemoteCall);

							C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
						}
						STREAM->DiscWhenAllSent = 10;

						return 0;
					}
					S++;
				}

				// Not Found

				memcpy(&STREAM->AGWKey[0], &Key[0], 21);

				AGW->TXHeader.Port = UZ7HOChannel[port];
				AGW->TXHeader.DataKind = 'C';
				memcpy(AGW->TXHeader.callfrom, &STREAM->AGWKey[11], 10);
				memcpy(AGW->TXHeader.callto, &STREAM->AGWKey[1], 10);
				AGW->TXHeader.DataLength = 0;

				ptr = strtok_s(NULL, " ,\r", &context);

				if (ptr)
				{
					// we have digis

					viaptr = &ViaList[1];

					if (strcmp(ptr, "V") == 0 || strcmp(ptr, "VIA") == 0)
						ptr = strtok_s(NULL, " ,\r", &context);

					while (ptr)
					{
						strcpy(viaptr, ptr);
						Digis++;
						viaptr += 10;
						ptr = strtok_s(NULL, " ,\r", &context);
					}

#ifdef __BIG_ENDIAN__
					AGW->TXHeader.DataLength = reverse(Digis * 10 + 1);
#else
					AGW->TXHeader.DataLength = Digis * 10 + 1;
#endif

					AGW->TXHeader.DataKind = 'v';
					ViaList[0] = Digis;
				}

				sent = send(TNCInfo[MasterPort[port]]->TCPSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
				if (Digis)
					send(TNCInfo[MasterPort[port]]->TCPSock, ViaList, Digis * 10 + 1, 0);

				STREAM->Connecting = TNC->AGWInfo->ConnTimeOut;	// It doesn't report failure

//				sprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
//				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			}
		}
		return (0);

	case 3:	

		Stream = (int)(size_t)buff;

		TNCOK = TNCInfo[MasterPort[port]]->CONNECTED;

		STREAM = &TNC->Streams[Stream];

		if (STREAM->FramesOutstanding > 8)	
			return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);

		return TNCOK << 8 | STREAM->Disconnecting << 15;		// OK, but lock attach if disconnecting
	
		break;

	case 4:				// reinit

		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->TCPSock);
		TNC->CONNECTED = FALSE;
		TNC->Alerted = FALSE;

		sprintf(TNC->WEB_COMMSSTATE, "Disconnected from TNC");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		if (TNC->WeStartedTNC)
		{
			KillTNC(TNC);
			RestartTNC(TNC);
		}

		return (0);

	case 5:				// Close

		shutdown(TNC->TCPSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->TCPSock);

		if (TNC->WeStartedTNC)
		{
			KillTNC(TNC);
		}

		return 0;
	}

	return 0;
}

extern char sliderBit[];
extern char WebProcTemplate[];

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
/*
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}"
		"myInt = setInterval('Refresh()', 15000 );\n"
		"function Refresh( )\n"
		"{location.reload()}\n"
		"</script>"
		"</head><title>UZ7HO Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2><form method=post target=\"POPUPW\" onsubmit=\"POPUPW = window.open('about:blank','POPUPW',"
		"'width=440,height=150');\" action=ARDOPAbort?%d>UZ7HO Status"
		"<input name=Save value=\"Abort Session\" type=submit style=\"position: absolute; right: 10;top:10;\"></form></h2>",
		TNC->Port);
*/
	int Len = sprintf(Buff, WebProcTemplate, TNC->Port, TNC->Port, "UZ7HO Status", "UZ7HO Status");


	if (TNC->TXFreq)
		Len += sprintf(&Buff[Len], sliderBit, TNC->TXOffset, TNC->TXOffset);

	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Modem</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}




void * UZ7HOExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	struct TNCINFO * TNC;
	char * ptr;

	//
	//	Will be called once for each UZ7HO port to be mapped to a BPQ Port
	//	The UZ7HO port number is in CHANNEL - A=0, B=1 etc
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

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	if (PortEntry->PORTCONTROL.PORTINTERLOCK && TNC->RXRadio == 0 && TNC->TXRadio == 0)
		TNC->RXRadio = TNC->TXRadio = PortEntry->PORTCONTROL.PORTINTERLOCK;

	TNC->SuspendPortProc = UZ7HOSuspendPort;
	TNC->ReleasePortProc = UZ7HOReleasePort;

	PortEntry->PORTCONTROL.PORTSTARTCODE = UZ7HOStartPort;
	PortEntry->PORTCONTROL.PORTSTOPCODE = UZ7HOStopPort;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.UICAPABLE = 1;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->PERMITGATEWAY = TRUE;					// Can change ax.25 call on each stream
	PortEntry->SCANCAPABILITIES = NONE;					// Scan Control - pending connect only

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 64;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	TNC->Hardware = H_UZ7HO;

	UZ7HOChannel[port] = PortEntry->PORTCONTROL.CHANNELNUM-65;
	
	PortEntry->MAXHOSTMODESESSIONS = TNC->AGWInfo->MaxSessions;	

	i=sprintf(Msg,"UZ7HO Host %s Port %d Chan %c\n",
		TNC->HostName, TNC->TCPPort, PortEntry->PORTCONTROL.CHANNELNUM);
	WritetoConsole(Msg);

	// See if we already have a port for this host

	MasterPort[port] = port;

	for (i = 1; i < port; i++)
	{
		if (i == port) continue;

		if (TNCInfo[i] && TNCInfo[i]->TCPPort == TNC->TCPPort &&
			 _stricmp(TNCInfo[i]->HostName, TNC->HostName) == 0)
		{
			MasterPort[port] = i;
			SlaveTNC[i] = TNC;
			break;
		}
	}

	BPQPort[PortEntry->PORTCONTROL.CHANNELNUM-65][MasterPort[port]] = port;
			
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

	TNC->WEB_MODE = zalloc(50);
	TNC->WEB_TRAFFIC = zalloc(100);
	TNC->WEB_LEVELS = zalloc(32);

#ifndef LINBPQ

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450, ForcedClose);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,6,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,6,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,28,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,28,520,20, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Modem", WS_CHILD | WS_VISIBLE, 10,50,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 120,50,520,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();
	
	MoveWindows(TNC);

	if (MasterPort[port] == port)
	{
		// First port for this TNC - start TNC if sonfigured and connect

#ifndef LINBPQ
		if (EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC))
			if (TNC->ProgramPath)
				TNC->WeStartedTNC = RestartTNC(TNC);
#else
		if (TNC->ProgramPath)
			TNC->WeStartedTNC = RestartTNC(TNC);
#endif
		ConnecttoUZ7HO(port);
	}
	else
	{
		// Slave Port

		sprintf(TNC->WEB_COMMSSTATE, "Slave to Port %d", MasterPort[port] );		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
	}


	time(&lasttime[port]);			// Get initial time value

#endif
	return ExtProc;
}

/*

#	Config file for BPQtoUZ7HO
#
#	For each UZ7HO port defined in BPQCFG.TXT, Add a line here
#	Format is BPQ Port, Host/IP Address, Port

#
#	Any unspecified Ports will use 127.0.0.1 and port for BPQCFG.TXT IOADDR field
#

1 127.0.0.1 8000
2 127.0.0.1 8001

*/


static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC = TNCInfo[Port];
	struct AGWINFO * AGW;

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

	TNC = TNCInfo[BPQport] = zalloc(sizeof(struct TNCINFO));
	AGW = TNC->AGWInfo = zalloc(sizeof(struct AGWINFO)); // AGW Sream Mode Specific Data

	AGW->MaxSessions = 10;
	AGW->ConnTimeOut = CONTIMEOUT;

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
	if (p_ipad == NULL)
		p_ipad = strtok(NULL, " \t\n\r");

	if (p_ipad == NULL) return (FALSE);
	
	p_port = strtok(NULL, " \t\n\r");
			
	if (p_port == NULL) return (FALSE);

	TNC->TCPPort = atoi(p_port);

	if (TNC->TCPPort == 0)
		TNC->TCPPort = 8000;

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(TNC->TCPPort);
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
		
			if (ptr &&_memicmp(ptr, "PATH", 4) == 0)
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
			
			if (_memicmp(buf, "MAXSESSIONS", 11) == 0)
			{
				AGW->MaxSessions = atoi(&buf[12]);
				if (AGW->MaxSessions > 26 ) AGW->MaxSessions = 26;
			}
			if (_memicmp(buf, "CONTIMEOUT", 10) == 0)
				AGW->ConnTimeOut = atoi(&buf[11]) * 10;
			else
			if (_memicmp(buf, "UPDATEMAP", 9) == 0)
				TNC->PktUpdateMap = TRUE;
			else
			if (_memicmp(buf, "BEACONAFTERSESSION", 18) == 0) // Send Beacon after each session 
				TNC->RPBEACON = TRUE;
			else
			if (_memicmp(buf, "WINDOW", 6) == 0)
				TNC->Window = atoi(&buf[7]);
			else
			if (_memicmp(buf, "DEFAULTMODEM", 12) == 0) 
				TNC->AGWInfo->Modem = atoi(&buf[13]);
			else
			if (_memicmp(buf, "MODEMCENTER", 11) == 0 || _memicmp(buf, "MODEMCENTRE", 11) == 0)
				TNC->AGWInfo->CenterFreq = atoi(&buf[12]);
			else
			if (standardParams(TNC, buf) == FALSE)
				strcat(TNC->InitScript, buf);
		}


	return (TRUE);	
}

#ifndef LINBPQ

typedef struct hINFO
{
	HWND Freq1;
	HWND Freq2;
	HWND Spin1;
	HWND Spin2;
	COMBOBOXINFO cinfo1;
	COMBOBOXINFO cinfo2;
};


BOOL CALLBACK EnumChildProc(HWND handle, LPARAM lParam)
{
	char classname[100];
	struct hINFO * hInfo = (struct hINFO *)lParam;

	// We collect the handles for the two modem and freq boxs here and set into correct TNC record later

	GetClassName(handle, classname, 99);

	if (strcmp(classname, "TComboBox") == 0)
	{
		// Get the Combo Box Info
		
		if (hInfo->cinfo1.cbSize == 0)
		{
			hInfo->cinfo1.cbSize = sizeof(COMBOBOXINFO);
			GetComboBoxInfo(handle, &hInfo->cinfo1);
		}
		else 
		{
			hInfo->cinfo2.cbSize = sizeof(COMBOBOXINFO);
			GetComboBoxInfo(handle, &hInfo->cinfo2);
		}
	}

	if (strcmp(classname, "TSpinEdit") == 0)
	{
		if (hInfo->Freq1 == 0)
			hInfo->Freq1 = handle;
		else 
			hInfo->Freq2 = handle;
	}

	if (strcmp(classname, "TSpinButton") == 0)
	{
		if (hInfo->Spin1 == 0)
			hInfo->Spin1 = handle;
		else 
			hInfo->Spin2 = handle;
	}
	return TRUE;
}

BOOL CALLBACK uz_enum_windows_callback(HWND handle, LPARAM lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	struct AGWINFO * AGW = TNC->AGWInfo;
	char Freq[16];

	UINT ProcessId;

	GetWindowText(handle, wtext, 99);

	GetWindowThreadProcessId(handle, &ProcessId);

	if (TNC->PID == ProcessId)
	{
		if (strstr(wtext,"SoundModem "))
		{
			// Our Process

			// Enumerate Child Windows

			struct hINFO hInfo; 

			memset(&hInfo, 0, sizeof(hInfo));

			EnumChildWindows(handle, EnumChildProc,  (LPARAM)&hInfo);

			// Set handles

			if (TNC->PortRecord->PORTCONTROL.CHANNELNUM == 'A')
			{
				AGW->hFreq = hInfo.Freq1;
				AGW->hSpin = hInfo.Spin1;
				memcpy(&AGW->cbinfo, &hInfo.cinfo1, sizeof(COMBOBOXINFO));
			}
			else
			{
				AGW->hFreq = hInfo.Freq2;
				AGW->hSpin = hInfo.Spin2;
				memcpy(&AGW->cbinfo, &hInfo.cinfo2, sizeof(COMBOBOXINFO));
			}

			if (AGW->CenterFreq && AGW->hFreq)
			{
				// Set it

				sprintf(Freq, "%d", AGW->CenterFreq - 1);

				SendMessage(AGW->hFreq, WM_SETTEXT, 0, (LPARAM)Freq);
				SendMessage(AGW->hSpin, WM_LBUTTONDOWN, 0, 1);
				SendMessage(AGW->hSpin, WM_LBUTTONUP, 0, 1);
			}

			// Set slave port

			TNC = SlaveTNC[TNC->Port];

			if (TNC)
			{
				AGW = TNC->AGWInfo;

				if (TNC->PortRecord->PORTCONTROL.CHANNELNUM == 'A')
				{
					AGW->hFreq = hInfo.Freq1;
					AGW->hSpin = hInfo.Spin1;
					memcpy(&AGW->cbinfo, &hInfo.cinfo1, sizeof(COMBOBOXINFO));
				}
				else
				{
					AGW->hFreq = hInfo.Freq2;
					AGW->hSpin = hInfo.Spin2;
					memcpy(&AGW->cbinfo, &hInfo.cinfo2, sizeof(COMBOBOXINFO));
				}

				if (AGW->CenterFreq && AGW->hFreq)
				{
					// Set it

					sprintf(Freq, "%d", AGW->CenterFreq - 1);

					SendMessage(AGW->hFreq, WM_SETTEXT, 0, (LPARAM)Freq);
					SendMessage(AGW->hSpin, WM_LBUTTONDOWN, 0, 1);
					SendMessage(AGW->hSpin, WM_LBUTTONUP, 0, 1);
				}

			}

 			return FALSE;
		}
	}
	
	return (TRUE);
}

#endif

int ConnecttoUZ7HO(int port)
{
	if (TNCInfo[port]->CONNECTING || TNCInfo[port]->PortRecord->PORTCONTROL.PortStopped)
		return 0;

	_beginthread(ConnecttoUZ7HOThread, 0, (void *)(size_t)port);
	return 0;
}

VOID ConnecttoUZ7HOThread(void * portptr)
{
	int port = (int)(size_t)portptr;
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];
	struct AGWINFO * AGW = TNC->AGWInfo;
	
	Sleep(3000);		// Allow init to complete 

	TNC->AGWInfo->isQTSM = 0;

#ifndef LINBPQ

	AGW->hFreq = AGW->hSpin = 0;
	AGW->cbinfo.cbSize = 0;

	if (strcmp(TNC->HostName, "127.0.0.1") == 0)
	{
		// can only check if running on local host
		
		TNC->PID = GetListeningPortsPID(TNC->destaddr.sin_port);
		
		if (TNC->PID == 0)
			return;
	
//		goto TNCNotRunning;

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

		// Get Window Handles so we can change centre freq and modem

		EnumWindows(uz_enum_windows_callback, (LPARAM)TNC);
	}
#endif

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (TNC->HostName);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	if (TNC->TCPSock)
	{
		Debugprintf("UZ7HO Closing Sock %d", TNC->TCPSock); 
		closesocket(TNC->TCPSock);
	}

	TNC->TCPSock = 0;

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for UZ7HO socket - error code = %d\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	TNC->CONNECTING = TRUE;

	if (connect(TNC->TCPSock,(struct sockaddr *) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connect successful
		//

		TNC->CONNECTED = TRUE;

		sprintf(TNC->WEB_COMMSSTATE, "Connected to TNC");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		ioctl(TNC->TCPSock, FIONBIO, &param);
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			sprintf(Msg, "Connect Failed for UZ7HO socket - error code = %d Port %d\n",
				err, htons(TNC->destaddr.sin_port));
		
			WritetoConsole(Msg);

			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");		
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
	
		return;
	}

	TNC->LastFreq = 0;					// so V4 display will be updated
	RegisterAPPLCalls(TNC, FALSE);		// Register Calls

	return;

}

static int ProcessReceivedData(int port)
{
	unsigned int bytes;
	int datalen,i;
	char ErrMsg[255];
	char Message[1000];
	struct TNCINFO * TNC = TNCInfo[port];
	struct AGWINFO * AGW = TNC->AGWInfo;
	struct TNCINFO * SaveTNC;

	//	Need to extract messages from byte stream

	//	Use MSG_PEEK to ensure whole message is available

	bytes = recv(TNC->TCPSock, (char *) &AGW->RXHeader, AGWHDDRLEN, MSG_PEEK);

	if (bytes == SOCKET_ERROR)
	{
//		i=sprintf(ErrMsg, "Read Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
//		WritetoConsole(ErrMsg);
				
		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
					
		TNC->CONNECTED = FALSE;
		TNC->Alerted = FALSE;

		sprintf(TNC->WEB_COMMSSTATE, "Disconnected from TNC");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);


		return (0);
	}

	if (bytes == 0)
	{
		//	zero bytes means connection closed

		i=sprintf(ErrMsg, "UZ7HO Connection closed for BPQ Port %d\n", port);
		WritetoConsole(ErrMsg);

		TNC->CONNECTED = FALSE;
		TNC->Alerted = FALSE;

		sprintf(TNC->WEB_COMMSSTATE, "Disconnected from TNC");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);


		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
		return (0);
	}

	//	Have some data
	
	if (bytes == AGWHDDRLEN)
	{
		//	Have a header - see if we have any associated data
		
		datalen = AGW->RXHeader.DataLength;

#ifdef __BIG_ENDIAN__
		datalen = reverse(datalen);
#endif
		if (datalen < 0 || datalen > 500)
		{
			// corrupt - reset connection

			shutdown(TNC->TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TNC->TCPSock);
			TNC->CONNECTED = FALSE;
			TNC->Alerted = FALSE;

			sprintf(TNC->WEB_COMMSSTATE, "Disconnected from TNC");		
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			return 0;
		}

		if (datalen > 0)
		{
			// Need data - See if enough there
				
			bytes = recv(TNC->TCPSock, (char *)&Message, AGWHDDRLEN + datalen, MSG_PEEK);
		}

		if (bytes == AGWHDDRLEN + datalen)
		{
			bytes = recv(TNC->TCPSock, (char *)&AGW->RXHeader, AGWHDDRLEN,0);

			if (datalen > 0)
			{
				bytes = recv(TNC->TCPSock,(char *)&Message, datalen,0);
			}

			// Have header, and data if needed

			SaveTNC = TNC;
			ProcessAGWPacket(TNC, Message);			// Data may be for another port
			TNC = SaveTNC;

			return (0);
		}

		// Have header, but not sufficient data

		return (0);
	
	}

	// Dont have at least header bytes
	
	return (0);

}
/*
VOID ConnecttoMODEMThread(port);

int ConnecttoMODEM(int port)
{
	_beginthread(ConnecttoMODEMThread,0,port);

	return 0;
}

VOID ConnecttoMODEMThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(5000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname(TNC->HostName);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(TNC->TCPSock);
	closesocket(TNC->TCPDataSock);

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);
	TNC->TCPDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET || TNC->TCPDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for UZ7HO socket - error code = %d\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	setsockopt (TNC->TCPDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->TCPSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=sprintf(Msg, "Bind Failed for UZ7HO socket - error code = %d\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	TNC->CONNECTING = TRUE;

	if (connect(TNC->TCPSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		TNC->CONNECTED=TRUE;
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connected to UZ7HO TNC");
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=sprintf(Msg, "Connect Failed for UZ7HO socket - error code = %d\n", err);
			WritetoConsole(Msg);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC failed");

			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	TNC->LastFreq = 0;			//	so V4 display will be updated

	return;
}
*/
/*
UZ7HO C GM8BPQ GM8BPQ-2 *** CONNECTED To Station GM8BPQ-0

UZ7HO D GM8BPQ GM8BPQ-2 asasasas
M8BPQ
New Disconnect Port 7 Q 0
UZ7HO d GM8BPQ GM8BPQ-2 *** DISCONNECTED From Station GM8BPQ-0

New Disconnect Port 7 Q 0
*/

extern VOID PROCESSUZ7HONODEMESSAGE();

VOID ProcessAGWPacket(struct TNCINFO * TNC, UCHAR * Message)
{
	PMSGWITHLEN buffptr;
	MESSAGE Monframe;
	struct HDDRWITHDIGIS MonDigis;

 	struct AGWINFO * AGW = TNC->AGWInfo;
	struct AGWHEADER * RXHeader = &AGW->RXHeader;
	char Key[21];
	int Stream;
	struct STREAMINFO * STREAM;
	UCHAR AGWPort;
	UCHAR MHCall[10] = "";
	int n;
	unsigned char c;
	unsigned char * ptr;
	int Totallen = 0;
	struct PORTCONTROL * PORT = &TNC->PortRecord->PORTCONTROL;


#ifdef __BIG_ENDIAN__
	RXHeader->DataLength = reverse(RXHeader->DataLength);
#endif

	switch (RXHeader->DataKind)
	{
	case 'D':			// Appl Data

		TNC = GetSessionKey(Key, TNC);
	
		if (TNC == NULL)
			return;

		// Find our Session

		Stream = 0;

		while (Stream <= AGW->MaxSessions)
		{
			STREAM = &TNC->Streams[Stream];

			if (memcmp(STREAM->AGWKey, Key, 21) == 0)
			{
				// Found it;

				buffptr = GetBuff();
				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr->Len  = RXHeader->DataLength;
				memcpy(buffptr->Data, Message, RXHeader->DataLength);

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				return;
			}
			Stream++;
		}		
			
		// Not Found

		return;


	case 'd':			// Disconnected

		TNC = GetSessionKey(Key, TNC);

		if (TNC == NULL)
			return;

		// Find our Session

		Stream = 0;

		while (Stream <= AGW->MaxSessions)
		{
			STREAM = &TNC->Streams[Stream];

			if (memcmp(STREAM->AGWKey, Key, 21) == 0)
			{
				// Found it;

				if (STREAM->DiscWhenAllSent)
					return;						// Already notified

				if (STREAM->Connecting)
				{
					// Report Connect Failed, and drop back to command mode

					STREAM->Connecting = FALSE;
					buffptr = GetBuff();

					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr->Len = sprintf(buffptr->Data, "UZ7HO} Failure with %s\r", STREAM->RemoteCall);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					STREAM->DiscWhenAllSent = 10;

					if (TNC->RPBEACON)
						SendRPBeacon(TNC);
			
					return;
				}

				// Release Session

				STREAM->Connecting = FALSE;
				STREAM->Connected = FALSE;		// Back to Command Mode
				STREAM->ReportDISC = TRUE;		// Tell Node

		//		if (STREAM->Disconnecting)		// 
		//			ReleaseTNC(TNC);

				STREAM->Disconnecting = FALSE;
				STREAM->DiscWhenAllSent = 10;
				STREAM->FramesOutstanding = 0;

				if (TNC->RPBEACON)
					SendRPBeacon(TNC);

				return;
			}
			Stream++;
		}

		return;

	case 'C':

        //   Connect. Can be Incoming or Outgoing

		// "*** CONNECTED To Station [CALLSIGN]" When the other station starts the connection
		// "*** CONNECTED With [CALLSIGN]" When we started the connection

        //   Create Session Key from port and callsign pair

		TNC = GetSessionKey(Key, TNC);

		if (TNC == NULL)
			return;

		if (strstr(Message, " To Station"))
		{
			char noStreams[] = "No free sessions - disconnecting\r";

			// Incoming. Look for a free Stream

			Stream = 1;

			while(Stream <= AGW->MaxSessions)
			{
				if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
					goto GotStream;

				Stream++;
			}

			// No free streams - send message then Disconnect

			// Do we need to swap From and To? - yes

			memcpy(RXHeader->callfrom, &Key[11], 10);
			memcpy(RXHeader->callto, &Key[1], 10);

#ifdef __BIG_ENDIAN__
			AGW->RXHeader.DataLength = reverse(strlen(noStreams));
#else
			AGW->RXHeader.DataLength = (int)strlen(noStreams);
#endif
			RXHeader->DataKind = 'D';
			AGW->RXHeader.PID = 0xf0;

			send(TNCInfo[MasterPort[TNC->Port]]->TCPSock, (char *)&AGW->RXHeader, AGWHDDRLEN, 0);
			send(TNCInfo[MasterPort[TNC->Port]]->TCPSock, noStreams, (int)strlen(noStreams), 0);

			Sleep(500);
			RXHeader->DataKind = 'd';
			RXHeader->DataLength = 0;


			send(TNCInfo[MasterPort[TNC->Port]]->TCPSock, (char *)&AGW->RXHeader, AGWHDDRLEN, 0);
			return;

GotStream:

			STREAM = &TNC->Streams[Stream];
			memcpy(STREAM->AGWKey, Key, 21);
			STREAM->Connected = TRUE;
			STREAM->ConnectTime = time(NULL); 
			STREAM->BytesRXed = STREAM->BytesTXed = 0;

			SuspendOtherPorts(TNC);

			strcpy(TNC->TargetCall, RXHeader->callto);

			ProcessIncommingConnect(TNC, RXHeader->callfrom, Stream, FALSE);

			// if Port CTEXT defined, use it

			if (PORT->CTEXT)
			{
				Totallen = strlen(PORT->CTEXT);
				ptr = PORT->CTEXT;
			}
			else if (HFCTEXTLEN > 0)
			{
				Totallen = HFCTEXTLEN;
				ptr = HFCTEXT;
			}
			else if (FULL_CTEXT)
			{
				Totallen = CTEXTLEN;
				ptr = CTEXTMSG;
			}

			while (Totallen)
			{
				int sendLen = TNC->PortRecord->ATTACHEDSESSIONS[Stream]->SESSPACLEN;

				if (sendLen == 0)
					sendLen = 80;

				if (Totallen < sendLen)
					sendLen = Totallen;

				SendData(Stream, TNC, &STREAM->AGWKey[0], ptr, sendLen);

				Totallen -= sendLen;
				ptr += sendLen;
			}

/*
			if (HFCTEXTLEN)
			{
				if (HFCTEXTLEN > 1)
					SendData(Stream, TNC, &STREAM->AGWKey[0], HFCTEXT, HFCTEXTLEN);
			}
			else
			{
				if (FULL_CTEXT)
				{
					int Len = CTEXTLEN, CTPaclen = 50;
					int Next = 0;

					while (Len > CTPaclen)		// CTEXT Paclen
					{
						SendData(Stream, TNC, &STREAM->AGWKey[0], &CTEXTMSG[Next], CTPaclen);
						Next += CTPaclen;
						Len -= CTPaclen;
					}
					SendData(Stream, TNC, &STREAM->AGWKey[0], &CTEXTMSG[Next], Len);
				}
			}
*/
			if (strcmp(RXHeader->callto, TNC->NodeCall) != 0)		// Not Connect to Node Call
			{
				APPLCALLS * APPL;
				char * ApplPtr = APPLS;
				int App;
				char Appl[10];
				char * ptr;
				char Buffer[80];				// Data portion of frame

				for (App = 0; App < 32; App++)
				{
					APPL=&APPLCALLTABLE[App];
					memcpy(Appl, APPL->APPLCALL_TEXT, 10);
					ptr=strchr(Appl, ' ');

					if (ptr)
						*ptr = 0;
	
					if (_stricmp(RXHeader->callto, Appl) == 0)
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
						int MsgLen = sprintf(Buffer, "%s\r", AppName);
						buffptr = GetBuff();

						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr->Len = MsgLen;
						memcpy(buffptr->Data, Buffer, MsgLen);

						C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
						TNC->SwallowSignon = TRUE;
					}
					else
					{
						char Msg[] = "Application not available\r\n";
					
						// Send a Message, then a disconenct
					
						SendData(Stream, TNC, Key, Msg, (int)strlen(Msg));

						STREAM->DiscWhenAllSent = 100;	// 10 secs
					}
					return;
				}
			}
		
			// Not to a known appl - drop through to Node

			return;
		}
		else
		{
			// Connect Complete

			// Find our Session

			Stream = 0;

			while (Stream <= AGW->MaxSessions)
			{
				STREAM = &TNC->Streams[Stream];

				if (memcmp(STREAM->AGWKey, Key, 21) == 0)
				{
					// Found it;

					STREAM->Connected = TRUE;
					STREAM->Connecting = FALSE;
					STREAM->ConnectTime = time(NULL); 
					STREAM->BytesRXed = STREAM->BytesTXed = 0;

					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr->Len  = sprintf(buffptr->Data, "*** Connected to %s\r", RXHeader->callfrom);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

					UpdateMH(TNC, RXHeader->callfrom, '+', 'O');
					return;
				}
				Stream++;
			}		
			
			// Not Found

			return;
		}

	case 'T':				// Trasmitted Dats

		DoMonitorHddr(TNC, RXHeader, Message);

		return;

	case 'S':				// Monitored Supervisory

		// Check for SABM

		if (strstr(Message, "<SABM") == 0 && strstr(Message, "<UA") == 0)
			return;

		// Drop through

	case 'U':

		AGWPort = Message[1];

		if (AGWPort < ' ' || AGWPort > '8')
			return;

		AGWPort = BPQPort[AGWPort - '1'][TNC->Port];

		TNC = TNCInfo[AGWPort];

		if (TNC == NULL)
			return;

		strlop(Message, '<');

//		if (strchr(Message, '*'))
//			UpdateMH(TNC, RXHeader->callfrom, '*', 0);
//		else
//			UpdateMH(TNC, RXHeader->callfrom, ' ', 0);
		
		return;

	case 'K':				// raw data	

		memset(&Monframe, 0, sizeof(Monframe));
		memset(&MonDigis, 0, sizeof(MonDigis));

		Monframe.PORT = BPQPort[RXHeader->Port][TNC->Port];

		if (Monframe.PORT == 0)		// Unused UZ7HO port?
			return;

		//	Get real port 

		TNC = TNCInfo[Monframe.PORT];

		if (TNC == 0)
			return;
		
		Monframe.LENGTH = RXHeader->DataLength + (MSGHDDRLEN - 1);

		memcpy(&Monframe.DEST[0], &Message[1], RXHeader->DataLength);

		// Check address - may have Single bit correction activated and non-x.25 filter off

		ptr = &Monframe.ORIGIN[0];
		n = 6;

		while(n--)
		{
			// Try a bit harder to detect corruption

			c = *(ptr++);

			if (c & 1)
				return;			// bottom bit set

			c = c >> 1;

			if (!isalnum(c) && !(c == '#') && !(c == ' '))
				return;
		}


/*		// if NETROM is enabled, and it is a NODES broadcast, process it

		if (TNC->PortRecord->PORTCONTROL.PORTQUALITY)
		{
			int i;
			char * fiddle;
			
			if (Message[15] == 3 && Message[16] == 0xcf && Message[17] == 255)
				i = 0;

			_asm
			{
				pushad

				mov al, Monframe.PORT
				lea edi, Monframe

				call PROCESSUZ7HONODEMESSAGE

				popad
			}
		}
*/
		// Pass to Monitor

		time(&Monframe.Timestamp);

		MHCall[ConvFromAX25(&Monframe.ORIGIN[0], MHCall)] = 0;

		// I think we need to check if UI and if so report to nodemap

		// should skip digis and report last digi but for now keep it simple

		// if there are digis process them

		if (Monframe.ORIGIN[6]  & 1)		// No digis
		{
			if (Monframe.CTL == 3)
				UpdateMHEx(TNC, MHCall, ' ', 0, NULL, TRUE);
			else
				UpdateMHEx(TNC, MHCall, ' ', 0, NULL, FALSE);
		
			BPQTRACE((MESSAGE *)&Monframe, TRUE);
		}
		else
		{
			UCHAR * ptr1 = Monframe.DEST;
			UCHAR * ptr2 = MonDigis.DEST;
			int Rest = Monframe.LENGTH - (MSGHDDRLEN - 1);

			MonDigis.PORT = Monframe.PORT;
			MonDigis.LENGTH = Monframe.LENGTH;
			MonDigis.Timestamp = Monframe.Timestamp;


			while ((ptr1[6] & 1) == 0)		// till end of address
			{
				memcpy(ptr2, ptr1, 7);
				ptr2 += 7;
				ptr1 += 7;
				Rest -= 7;
			}

			memcpy(ptr2, ptr1, 7);			// Copy Last
			ptr2 += 7;
			ptr1 += 7;
			Rest -= 7;

			// Now copy CTL PID and Data

			if (Rest < 0 || Rest > 256)
				return;

			memcpy(ptr2, ptr1, Rest);

			BPQTRACE((MESSAGE *)&MonDigis, TRUE);

			if (TNC->PortRecord->PORTCONTROL.PORTMHEARD)
				MHPROC((struct PORTCONTROL *)TNC->PortRecord, (MESSAGE *)&MonDigis);

//			if (ptr1[0] == 3)
//				UpdateMHEx(TNC, MHCall, ' ', 0, NULL, TRUE);
//			else
//				UpdateMHEx(TNC, MHCall, ' ', 0, NULL, FALSE);



		}
		
		return;

	case 'I':
		break;

	case 'R':
	{
		// Version

		int v1, v2;

		memcpy(&v1, Message, 4);
		memcpy(&v2, &Message[4], 4);

		if (v1 == 2019 && v2 == 'B')
			TNC->AGWInfo->isQTSM |= 1;

		break;
	}

	case 'g':

		// Capabilities - along with Version used to indicate QtSoundModem
		// with ability to set and read Modem type and frequency/

		if (Message[2] == 24 && Message[3] == 3 && Message[4] == 100)
		{
			// Set flag on any other ports on same TNC (all ports with this as master port)

			int p;
			int This = TNC->Port;

			if (RXHeader->DataLength == 12)
			{
				// First reply - request Modem Freq and Name
				for (p = This; p < 33; p++)
				{
					if (MasterPort[p] == This)
					{
						TNC = TNCInfo[p];

						if (TNC)
						{
							char Buffer[32] = "";
							int MsgLen = 32;
							SOCKET sock = TNCInfo[MasterPort[This]]->TCPSock;


							TNC->AGWInfo->isQTSM |= 2;

							AGW->TXHeader.Port = UZ7HOChannel[p];
							AGW->TXHeader.DataKind = 'g';
							memset(AGW->TXHeader.callfrom, 0, 10);
							memset(AGW->TXHeader.callto, 0, 10);
#ifdef __BIG_ENDIAN__
							AGW->TXHeader.DataLength = reverse(MsgLen);
#else
							AGW->TXHeader.DataLength = MsgLen;
#endif
							send(sock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
							send(sock, Buffer, MsgLen, 0);
						}	
					}
				}
				return;
			}

			if (RXHeader->DataLength == 44)
			{
				// Modem Freq and Type Report from QtSM

				int p = BPQPort[RXHeader->Port][TNC->Port];		// Get subchannel port

				TNC = TNCInfo[p];
				
				if (p == 0 || TNC == NULL)
					return;
				
				memcpy(&TNC->AGWInfo->CenterFreq, &Message[12], 4);
				memcpy(&TNC->AGWInfo->ModemName, &Message[16], 20);
				memcpy(&TNC->AGWInfo->Version, &Message[38], 4);

				sprintf(TNC->WEB_MODE, "%s / %d Hz", TNC->AGWInfo->ModemName, TNC->AGWInfo->CenterFreq);
				SetWindowText(TNC->xIDC_MODE, TNC->WEB_MODE);
			}
		}
		break;

	case 'X':
		break;

	case 'x':
		break;

	case 'Y':				// Session Queue

		AGWPort = RXHeader->Port;
		Key[0] = AGWPort + '1'; 
        
		memset(&Key[1], 0, 20);
		strcpy(&Key[11], RXHeader->callfrom);		// Wrong way round for GetSessionKey
		strcpy(&Key[1], RXHeader->callto);

		// Need to get BPQ Port from AGW Port

		if (AGWPort > 8)
			return;

		AGWPort = BPQPort[AGWPort][TNC->Port];
		
		TNC = TNCInfo[AGWPort];

		if (TNC == NULL)
			return;

//		Debugprintf("UZ7HO Port %d %d %c %s %s %d", TNC->Port, RXHeader->Port,
//			RXHeader->DataKind, RXHeader->callfrom, RXHeader->callto, Message[0]); 

		Stream = 0;

		while (Stream <= AGW->MaxSessions)
		{
			STREAM = &TNC->Streams[Stream];

			if (memcmp(STREAM->AGWKey, Key, 21) == 0)
			{
				// Found it;

				memcpy(&STREAM->FramesOutstanding, Message, 4);

				if (STREAM->FramesOutstanding == 0)			// All Acked
					if (STREAM->Disconnecting && STREAM->BPQtoPACTOR_Q == 0)
						TidyClose(TNC, 0);

				return;
			}
			Stream++;
		}		
			
		// Not Found

		return;

	default:

		Debugprintf("UZ7HO Port %d %c %s %s %s %d", TNC->Port, RXHeader->DataKind, RXHeader->callfrom, RXHeader->callto, Message, Message[0]); 

		return;
	}
}
struct TNCINFO * GetSessionKey(char * key, struct TNCINFO * TNC)
{
	struct AGWINFO * AGW = TNC->AGWInfo;
	struct AGWHEADER * RXHeader = &AGW->RXHeader;
	int AGWPort;

//   Create Session Key from port and callsign pair
        
	AGWPort = RXHeader->Port;
	key[0] = AGWPort + '1'; 
        
	memset(&key[1], 0, 20);
	strcpy(&key[1], RXHeader->callfrom);
	strcpy(&key[11], RXHeader->callto);

	// Need to get BPQ Port from AGW Port

	if (AGWPort > 8)
		return 0;

	AGWPort = BPQPort[AGWPort][TNC->Port];

	if (AGWPort == 0)
		return 0;

	TNC = TNCInfo[AGWPort];
	return TNC;
}

/*
Port field is the port where we want the data to tx
DataKind field =MAKELONG('D',0); The ASCII value of letter D
CallFrom is our call
CallTo is the call of the other station
DataLen is the length of the data that follow
*/

VOID SendData(int Stream, struct TNCINFO * TNC, char * Key, char * Msg, int MsgLen)
{
	struct AGWINFO * AGW = TNC->AGWInfo;
	SOCKET sock = TNCInfo[MasterPort[TNC->Port]]->TCPSock;
	int Paclen = 0;
	
	AGW->TXHeader.Port = Key[0] - '1';
	AGW->TXHeader.DataKind='D';
	memcpy(AGW->TXHeader.callfrom, &Key[11], 10);
	memcpy(AGW->TXHeader.callto, &Key[1], 10);

	// If Length is greater than Paclen we should fragment

	if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])
		Paclen = TNC->PortRecord->ATTACHEDSESSIONS[Stream]->SESSPACLEN;

	if (Paclen == 0)
		Paclen = 80;

	if (MsgLen > Paclen)
	{
		// Fragment it. 
		// Is it best to send Paclen packets then short or equal length?
		// I think equal length;

		int Fragments = (MsgLen + Paclen - 1) / Paclen;
		int Fraglen = MsgLen / Fragments;

		if ((MsgLen & 1))		// Odd
			Fraglen ++;

		while (MsgLen > Fraglen)
		{
#ifdef __BIG_ENDIAN__
			AGW->TXHeader.DataLength = reverse(MsgLen);
#else
			AGW->TXHeader.DataLength = Fraglen;
#endif
			AGW->TXHeader.PID = 0xf0;

			send(sock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
			send(sock, Msg, Fraglen, 0);

			Msg += Fraglen;
			MsgLen -= Fraglen;
		}

		// Drop through to send last bit
	}
#ifdef __BIG_ENDIAN__
	AGW->TXHeader.DataLength = reverse(MsgLen);
#else
	AGW->TXHeader.DataLength = MsgLen;
#endif
	AGW->TXHeader.PID = 0xf0;
	send(sock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
	send(sock, Msg, MsgLen, 0);
}

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	char * Key = &TNC->Streams[Stream].AGWKey[0];
	struct AGWINFO * AGW = TNC->AGWInfo;
	
	AGW->TXHeader.Port = Key[0] - '1';
	AGW->TXHeader.DataKind='d';
	strcpy(AGW->TXHeader.callfrom, &Key[11]);
	strcpy(AGW->TXHeader.callto, &Key[1]);
	AGW->TXHeader.DataLength = 0;

	send(TNCInfo[MasterPort[TNC->Port]]->TCPSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
}



VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	TidyClose(TNC, Stream);			// I don't think Hostmode has a DD
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	char Status[80];
	int s;

	// Clear Session Key
	
	memset(TNC->Streams[Stream].AGWKey, 0, 21);

	// if all streams are free, start scanner

	s = 0;

	while(s <= TNC->AGWInfo->MaxSessions)
	{
		if (s != Stream)
		{		
			if (TNC->PortRecord->ATTACHEDSESSIONS[s])
				return;										// Busy
		}
		s++;
	}

	ReleaseOtherPorts(TNC);

	sprintf(Status, "%d SCANSTART 15", TNC->Port);
	Rig_Command(-1, Status);
}

static MESSAGE Monframe;		// I frames come in two parts.


MESSAGE * AdjMsg;		// Adjusted fir digis


static VOID DoMonitorHddr(struct TNCINFO * TNC, struct AGWHEADER * RXHeader, UCHAR * Msg)
{
	// Convert to ax.25 form and pass to monitor

	// Only update MH on UI, SABM, UA

	UCHAR * ptr, * starptr, * CPPtr, * nrptr, * nsptr;
	char * context;
	char MHCall[11];
	int ILen;
	char * temp;

	Msg[RXHeader->DataLength] = 0;

	Monframe.LENGTH = MSGHDDRLEN + 16;				// Control Frame
	Monframe.PORT = BPQPort[RXHeader->Port][TNC->Port];

	if (RXHeader->DataKind == 'T')		// Transmitted
		Monframe.PORT += 128;

	/*
UZ7HO T GM8BPQ-2 G8XXX  1:Fm GM8BPQ-2 To G8XXX <SABM P>[12:08:42]
UZ7HO d G8XXX GM8BPQ-2 *** DISCONNECTED From Station G8XXX-0
UZ7HO T GM8BPQ-2 G8XXX  1:Fm GM8BPQ-2 To G8XXX <DISC P>[12:08:48]
UZ7HO T GM8BPQ-2 APRS  1:Fm GM8BPQ-2 To APRS Via WIDE2-2 <UI F pid=F0 Len=28 >[12:08:54]
=5828.54N/00612.69W- {BPQ32}
*/

	
	AdjMsg = &Monframe;					// Adjusted for digis
	ptr = strstr(Msg, "Fm ");

	ConvToAX25(&ptr[3], Monframe.ORIGIN);

	memcpy(MHCall, &ptr[3], 11);
	strlop(MHCall, ' ');

	ptr = strstr(ptr, "To ");

	ConvToAX25(&ptr[3], Monframe.DEST);

	ptr = strstr(ptr, "Via ");

	if (ptr)
	{
		// We have digis

		char Save[100];

		memcpy(Save, &ptr[4], 60);

		ptr = strtok_s(Save, ", ", &context);
DigiLoop:

		temp = (char *)AdjMsg;
		temp += 7;
		AdjMsg = (MESSAGE *)temp;

		Monframe.LENGTH += 7;

		starptr = strchr(ptr, '*');
		if (starptr)
			*(starptr) = 0;

		ConvToAX25(ptr, AdjMsg->ORIGIN);

		if (starptr)
			AdjMsg->ORIGIN[6] |= 0x80;				// Set end of address

		ptr = strtok_s(NULL, ", ", &context);

		if (ptr[0] != '<')
			goto DigiLoop;
	}
	AdjMsg->ORIGIN[6] |= 1;				// Set end of address

	ptr = strstr(Msg, "<");

	if (memcmp(&ptr[1], "SABM", 4) == 0)
	{
		AdjMsg->CTL = 0x2f;
//		UpdateMH(TNC, MHCall, ' ', 0);
	}
	else  
	if (memcmp(&ptr[1], "DISC", 4) == 0)
		AdjMsg->CTL = 0x43;
	else 
	if (memcmp(&ptr[1], "UA", 2) == 0)
	{
		AdjMsg->CTL = 0x63;
//		UpdateMH(TNC, MHCall, ' ', 0);
	}
	else  
	if (memcmp(&ptr[1], "DM", 2) == 0)
		AdjMsg->CTL = 0x0f;
	else 
	if (memcmp(&ptr[1], "UI", 2) == 0)
	{
		AdjMsg->CTL = 0x03;

		if (RXHeader->DataKind != 'T')
		{
			// only report RX

			if (strstr(Msg, "To BEACON "))
			{
				// Update MH with Received Beacons

				char * ptr1 = strchr(Msg, ']');

				if (ptr1)
				{
					ptr1 += 2;						// Skip ] and cr
					if (memcmp(ptr1, "QRA ", 4) == 0)
					{
						char Call[10], Loc[10] = "";
						sscanf(&ptr1[4], "%s %s", &Call[0], &Loc[0]);

						UpdateMHEx(TNC, MHCall, ' ', 0, Loc, TRUE);
					}
				}
				else
					UpdateMH(TNC, MHCall, ' ', 0);
			}
		}
	}
	else 
	if (memcmp(&ptr[1], "RR", 2) == 0)
	{
		nrptr = strchr(&ptr[3], '>');
		AdjMsg->CTL = 0x1 | (nrptr[-2] << 5);
	}
	else 
	if (memcmp(&ptr[1], "RNR", 3) == 0)
	{
		nrptr = strchr(&ptr[4], '>');
		AdjMsg->CTL = 0x5 | (nrptr[-2] << 5);
	}
	else 
	if (memcmp(&ptr[1], "REJ", 3) == 0)
	{
		nrptr = strchr(&ptr[4], '>');
		AdjMsg->CTL = 0x9 | (nrptr[-2] << 5);
	}
	else 
	if (memcmp(&ptr[1], "FRMR", 4) == 0)
		AdjMsg->CTL = 0x87;
	else  
	if (ptr[1] == 'I')
	{
		nsptr = strchr(&ptr[3], 'S');

		AdjMsg->CTL = (nsptr[-2] << 5) | (nsptr[1] & 7) << 1 ;
	}

	CPPtr = strchr(ptr, ' ');		
	if (CPPtr == NULL)
		return;

	if (strchr(&CPPtr[1], 'P'))
	{
		if (AdjMsg->CTL != 3)
			AdjMsg->CTL |= 0x10;
//		Monframe.DEST[6] |= 0x80;				// SET COMMAND
	}

	if (strchr(&CPPtr[1], 'F'))
	{
		if (AdjMsg->CTL != 3)
			AdjMsg->CTL |= 0x10;
//		Monframe.ORIGIN[6] |= 0x80;				// SET P/F bit
	}

	if ((AdjMsg->CTL & 1) == 0 || AdjMsg->CTL == 3)	// I or UI
	{
		ptr = strstr(ptr, "pid");	
		sscanf(&ptr[4], "%x", (unsigned int *)&AdjMsg->PID);
	
		ptr = strstr(ptr, "Len");	
		ILen = atoi(&ptr[4]);

		ptr = strstr(ptr, "]");
		ptr += 2;						// Skip ] and cr
		memcpy(AdjMsg->L2DATA, ptr, ILen);
		Monframe.LENGTH += ILen;
	}
	else if (AdjMsg->CTL == 0x97)		// FRMR
	{
		ptr = strstr(ptr, ">");
		sscanf(ptr+1, "%hhx %hhx %hhx", &AdjMsg->PID, &AdjMsg->L2DATA[0], &AdjMsg->L2DATA[1]);
		Monframe.LENGTH += 3;
	}

	time(&Monframe.Timestamp);
	BPQTRACE((MESSAGE *)&Monframe, TRUE);

}

/*

1:Fm GM8BPQ To GM8BPQ-2 <RR R1 >[17:36:17]
 1:Fm GM8BPQ To GM8BPQ-2 <I R1 S7 pid=F0 Len=56 >[17:36:29]
BPQ:GM8BPQ-2} G8BPQ Win32 Test Switch, Skigersta, Isle o

 1:Fm GM8BPQ-2 To GM8BPQ <RR R0 >[17:36:32]
 1:Fm GM8BPQ To GM8BPQ-2 <I R1 S0 pid=F0 Len=9 >[17:36:33]
f Lewis.

 1:Fm GM8BPQ-2 To GM8BPQ <RR R1 >[17:36:36]

1:Fm GM8BPQ To GM8BPQ-2 <RR F/R R1> [17:36:18R]
1:Fm GM8BPQ To GM8BPQ-2 <I F/C R1 S7 Pid=F0 Len=56> [17:36:30R]
BPQ:GM8BPQ-2} G8BPQ Win32 Test Switch, Skigersta, Isle o
1:Fm GM8BPQ-2 To GM8BPQ <RR F/R R0> [17:36:32T]
1:Fm GM8BPQ To GM8BPQ-2 <I F/C R1 S0 Pid=F0 Len=9> [17:36:34R]
f Lewis.

1:Fm GM8BPQ-2 To GM8BPQ <RR F/R R1> [17:36:36T]



1:Fm GM8BPQ To GM8BPQ-2 <RR F/R R1> [17:36:17T]
1:Fm GM8BPQ To GM8BPQ-2 <I F/C R1 S7 Pid=F0 Len=56> [17:36:29T]
BPQ:GM8BPQ-2} G8BPQ Win32 Test Switch, Skigersta, Isle o
1:Fm GM8BPQ-2 To GM8BPQ <RR F/R R0> [17:36:32R]
1:Fm GM8BPQ To GM8BPQ-2 <I F/C R1 S0 Pid=F0 Len=9> [17:36:33T]
f Lewis.

1:Fm GM8BPQ-2 To GM8BPQ <RR F/R R1> [17:36:36R]
*/
