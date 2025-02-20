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
//	Interface to allow G8BPQ switch to use  Serial TNC in character mode


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifndef WIN32
#ifndef MACBPQ
#ifndef FREEBSD
#include <sys/ioctl.h>
#include <linux/serial.h>
#endif
#endif
#endif


#include "cheaders.h"


extern int (WINAPI FAR *GetModuleFileNameExPtr)();
extern int (WINAPI FAR *EnumProcessesPtr)();

#include "bpq32.h"

#include "tncinfo.h"

static int Socket_Data(int sock, int error, int eventcode);

VOID MoveWindows(struct TNCINFO * TNC);
static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen);
int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);
BOOL SerialWriteCommBlock(struct TNCINFO * TNC);
void SerialCheckRX(struct TNCINFO * TNC);
int SerialSendData(struct TNCINFO * TNC, UCHAR * data, int txlen);
int SerialSendCommand(struct TNCINFO * TNC, UCHAR * data);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
VOID SendInitScript(struct TNCINFO * TNC);
int SerialGetLine(char * buf);
int ProcessEscape(UCHAR * TXMsg);
BOOL KAMStartPort(struct PORTCONTROL * PORT);
BOOL KAMStopPort(struct PORTCONTROL * PORT);

static char ClassName[]="SERIALSTATUS";
static char WindowTitle[] = "SERIAL";
static int RigControlRow = 165;

#ifndef LINBPQ
#include <commctrl.h>
#endif


extern int SemHeldByAPI;

static RECT Rect;

static int ProcessLine(char * buf, int Port);

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr;
	int len=510;
	struct TNCINFO * TNC = TNCInfo[Port];
	char errbuf[256];

	// Read Initialisation lines

	while(TRUE)
	{
		if (SerialGetLine(buf) == 0)
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
				
		if (_memicmp(buf, "LOGDIR ", 7) == 0)
			TNC->LogPath = _strdup(&buf[7]);
		else if (_memicmp(buf, "DRAGON", 6) == 0)
			TNC->Dragon = TRUE;
		else
			strcat (TNC->InitScript, buf);
	}

	return (TRUE);	
}

char * Config;
static char * ptr1, * ptr2;

int SerialGetLine(char * buf)
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

BOOL SerialReadConfigFile(int Port, int ProcLine(char * buf, int Port))
{
	char buf[256],errbuf[256];

	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		if (strlen(Config) == 0)
		{
			return TRUE;
		}

		ptr1 = Config;
		ptr2 = strchr(ptr1, 13);

		if (!ProcLine(buf, Port))
		{
			WritetoConsoleLocal("\n");
			WritetoConsoleLocal("Bad config record ");
			WritetoConsoleLocal(errbuf);
		}
	}
	else
	{
		sprintf(buf," ** Error - No Configuration info in bpq32.cfg");
		WritetoConsoleLocal(buf);
	}

	return (TRUE);
}



VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

static time_t ltime;

static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen)
{
	if (TNC->hDevice)
	{
		// Serial mode. Queue to Hostmode driver
		
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;

		return;
	}
}


VOID SerialChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

	datalen = sprintf(TXMsg, "MYCALL %s\r", Call);
	SerialSendCommand(TNC, TXMsg);
}

static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	int datalen;
	PMSGWITHLEN buffptr;
//	char txbuff[500];
	unsigned int bytes,txlen = 0;
	UCHAR * TXMsg;

	size_t Param;
	int Stream = 0;
	HKEY hKey=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct ScanEntry * Scan;

	if (TNC == NULL)
		return 0;							// Port not defined

	if (TNC->hDevice == 0)
	{
		// Clear anything from UI_Q

		while (TNC->PortRecord->UI_Q)
		{
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);
		}

		// Try to reopen every 30 secs

		if (fn > 3  && fn < 7)
			goto ok;

		TNC->ReopenTimer++;

		if (TNC->ReopenTimer < 300)
			return 0;

		TNC->ReopenTimer = 0;
		
		if (TNC->PortRecord->PORTCONTROL.PortStopped == 0)
			OpenCOMMPort(TNC, TNC->PortRecord->PORTCONTROL.SerialPortName, TNC->PortRecord->PORTCONTROL.BAUDRATE, TRUE);

		if (TNC->hDevice == 0)
			return 0;

#ifndef WIN32
#ifndef MACBPQ
#ifndef FREEBSD

		if (TNC->Dragon)
		{
			struct serial_struct sstruct;

			// Need to set custom baud rate

			if (ioctl(TNC->hDevice, TIOCGSERIAL, &sstruct) < 0)
			{
				Debugprintf("Error: Dragon could not get comm ioctl\n");
			}
			else
			{
				// set custom divisor to get 829440 baud
	
				sstruct.custom_divisor = 29;
				sstruct.flags |= ASYNC_SPD_CUST;

				// set serial_struct
		
				if (ioctl(TNC->hDevice, TIOCSSERIAL, &sstruct) < 0)
					Debugprintf("Error: Dragon could not set custom comm baud divisor\n");
				else
					Debugprintf("Dragon custom baud rate set\n");
			}
		}
#endif
#endif
#endif
		SendInitScript(TNC);

	}
ok:

	switch (fn)
	{
		case 7:			

		// 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		SerialCheckRX(TNC);
		return 0;

	case 1:				// poll

		while (TNC->PortRecord->UI_Q)
		{
			char FECMsg[512];
			char Call[12] = "           ";		
			struct _MESSAGE * buffptr;
			char * ptr = FECMsg;

			buffptr = Q_REM(&TNC->PortRecord->UI_Q);

			ReleaseBuffer(buffptr);
			continue;
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

					SerialSendCommand(TNC, "DISCONNECT\r");
				}
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				Debugprintf("Serial New Attach Stream %d", Stream);
			
				STREAM->Attached = TRUE;
			
				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;
			
	
				SerialChangeMYC(TNC, TNC->Streams[0].MyCall);
		
				// Stop other ports in same group

				SuspendOtherPorts(TNC);
	
				//sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
				//MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

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
				STREAM->bytesTXed += txlen;

				bytes=SerialSendData(TNC, data, txlen);
				WritetoTrace(TNC, data, txlen);
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

		if (!TNC->hDevice)
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

		// for now just send, but allow sending control
		// characters with \\ or ^ escape

//		if (STREAM->Connected)
		{
			STREAM->PacketsSent++;

			if (strstr(TXMsg, "\\\\") || strchr(TXMsg, '^'))
			{
				txlen = ProcessEscape(TXMsg);

				if (txlen == 0)			// Must be \\D
				{
					STREAM->ReportDISC = TRUE;		// Tell Node
					return 0;
				}
			}

			bytes=SerialSendData(TNC, TXMsg, txlen);
			TNC->Streams[Stream].BytesOutstanding += bytes;		// So flow control works - will be updated by BUFFER response
			STREAM->bytesTXed += bytes;
//			WritetoTrace(TNC, &buff->L2DATA[0], txlen);
	
			return 1;
		}
/*
		if (_memicmp(&buff->L2DATA[0], "D\r", 2) == 0 || _memicmp(&buff->L2DATA[0], "BYE\r", 4) == 0)
		{
			STREAM->ReportDISC = TRUE;		// Tell Node
			return 0;
		}
	

		// See if Local command (eg RADIO)

		if (_memicmp(&buff->L2DATA[0], "RADIO ", 6) == 0)
		{
			sprintf(&buff->L2DATA[0], "%d %s", TNC->Port, &buff->L2DATA[6]);

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
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "ARDOP} OK\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;

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

			txlen = sprintf(Connect, "C %s\r", &buff->L2DATA[2]);

			SerialChangeMYC(TNC, TNC->Streams[0].MyCall);

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

			memset(TNC->Streams[0].RemoteCall, 0, 10);
			strcpy(TNC->Streams[0].RemoteCall, &buff->L2DATA[2]);

			sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", STREAM->MyCall, STREAM->RemoteCall);
			SerialSendCommand(TNC, Connect);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
				
			STREAM->Connecting = TRUE;
			return 0;

		}

		// Normal data. Send to TNC


		SerialSendData(TNC, TXMsg, txlen);
	
		return 0;
*/
	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		Stream = (int)(size_t)buff;

		// I think we should check buffer space for all comms modes

		{
		int Queued;
		int Outstanding = TNC->Streams[Stream].BytesOutstanding;

		if (Stream == 0)
			Queued = TNC->Streams[13].FramesQueued;		// ARDOP Native Mode Send Queue
		else
			Queued = TNC->Streams[Stream].FramesQueued;

		Outstanding = Queued = 0;

 		if (TNC->Mode == 'O')		// OFDM version has more buffer space
		{
			if (Queued > 4 || Outstanding > 8500)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}
		else
		{
			if (Queued > 4 || Outstanding > 2000)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}

		}
		if (TNC->Streams[Stream].Attached == 0)
			return (TNC->hDevice != 0) << 8 | 1;

		return ((TNC->hDevice != 0) << 8 | TNC->Streams[Stream].Disconnecting << 15);		// OK
		

	case 4:				// reinit7

		return 0;

	case 5:				// Close

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
				SerialSendCommand(TNC, "CONOK OFF");
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
					SerialSendCommand(TNC, "CONOK ON");
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;
		return 0;
	}
	return 0;
}

VOID SerialReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	SerialChangeMYC(TNC, TNC->NodeCall);

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID SerialSuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC)
{
	SerialSendCommand(TNC, "CONOK OFF\r");
}

VOID SerialReleasePort(struct TNCINFO * TNC)
{
	SerialSendCommand(TNC, "CONOK ON\r");
}


VOID * SerialExtInit(EXTPORTDATA * PortEntry)
{
	int port;
	char Msg[512];
	char * ptr;
	struct TNCINFO * TNC;
	char * TempScript;
	
	sprintf(Msg,"Serial TNC %s\n", PortEntry->PORTCONTROL.SerialPortName);
	WritetoConsole(Msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	if (TNCInfo[port])					// If restarting, free old config
		free(TNCInfo[port]);

	TNC = TNCInfo[port] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;

	if (PortConfig[port])			// May not have config
		SerialReadConfigFile(port, ProcessLine);

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
	TNC->PortRecord->PORTCONTROL.HWType = TNC->Hardware = H_SERIAL;


	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

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

	TNC->SuspendPortProc = SerialSuspendPort;
	TNC->ReleasePortProc = SerialReleasePort;

	PortEntry->PORTCONTROL.PORTSTARTCODE = KAMStartPort;
	PortEntry->PORTCONTROL.PORTSTOPCODE = KAMStopPort;


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
		if (PortEntry->PORTCONTROL.WL2KInfo.RMSCall[0])			// Alrerady decoded
			TNC->WL2K = &PortEntry->PORTCONTROL.WL2KInfo;

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.SerialPortName, PortEntry->PORTCONTROL.BAUDRATE, FALSE);

#ifndef WIN32
#ifndef MACBPQ
#ifndef FREEBSD

	if (TNC->Dragon)
	{
		struct serial_struct sstruct;

		// Need to set custom baud rate

		if (ioctl(TNC->hDevice, TIOCGSERIAL, &sstruct) < 0)
		{
			printf("Error: Dragon could not get comm ioctl\n");
		}
		else
		{
			// set custom divisor to get 829440 baud
	
			sstruct.custom_divisor = 29;
			sstruct.flags |= ASYNC_SPD_CUST;

			// set serial_struct
		
			if (ioctl(TNC->hDevice, TIOCSSERIAL, &sstruct) < 0)
				Debugprintf("Error: Dragon could not set custom comm baud divisor\n");
			else
				Debugprintf("Dragon custom baud rate set\n");
		}
	}
#endif
#endif
#endif

	SendInitScript(TNC);

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}


VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[Stream].BytesOutstanding == 0)
		SerialSendCommand(TNC, "DISCONNECT\r");
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	SerialSendCommand(TNC, "DISCONNECT\r");
}



VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
	{
		SerialReleaseTNC(TNC);
	}
}

VOID SerialAbort(struct TNCINFO * TNC)
{
	SerialSendCommand(TNC, "ABORT\r");
}

// Host Mode Stuff (we reuse some routines in SCSPactor)

VOID SerialDoTermModeTimeout(struct TNCINFO * TNC)
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

VOID SendInitScript(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;
	char * ptr1, * ptr2;
	int len;

	// Send INIT script

	ptr1 = &TNC->InitScript[0];

	while (ptr1 && ptr1[0])
	{
		ptr2 = strchr(ptr1, 13);

		if (ptr2 == 0)
		{
			TNC->ReinitState = 3;
			break;
		}

		len = (int)(ptr2 - ptr1) + 1;

		memcpy(Poll, ptr1, len);

		TNC->TXLen = len;
		SerialWriteCommBlock(TNC);

		Sleep(50);

		if (ptr2)
			*(ptr2++) = 13;		// Put CR back for next time 

		ptr1 = ptr2;
	}
}



VOID SerialProcessTNCMessage(struct TNCINFO * TNC)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	buffptr = GetBuff();

	if (buffptr == 0)
	{
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "%s", TNC->RXBuffer);

	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

}


void SerialCheckRX(struct TNCINFO * TNC)
{
	int Length, Len = 0;

	if (TNC->RXLen == 250)
		TNC->RXLen = 0;

	if (TNC->hDevice)
		Len = ReadCOMBlock(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], 250 - TNC->RXLen);

	if (Len == 0)
		return;
	
	TNC->RXLen += Len;

	Length = TNC->RXLen;

	TNC->RXBuffer[TNC->RXLen] = 0;

//		if (TNC->Streams[Stream].RXBuffer[TNC->Streams[Stream].RXLen-2] != ':')

	if (strlen(TNC->RXBuffer) < TNC->RXLen)
		TNC->RXLen = 0;

	// Also need timeout for incomplete lines

//	if ((strchr(TNC->RXBuffer, 10) == 0) && (strchr(TNC->RXBuffer, 13) == 0))
//		return;				// Wait for rest of frame


//		OpenLogFile(TNC->Port);
//		WriteLogLine(TNC->Port, TNC->RXBuffer, (int)strlen(TNC->RXBuffer));
//		CloseLogFile(TNC->Port);

	TNC->RXLen = 0;		// Ready for next frame
					
	SerialProcessTNCMessage(TNC);
	return;
}

int SerialWriteCommBlock(struct TNCINFO * TNC)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, TNC->TXBuffer, TNC->TXLen);

	return 0;
}

int SerialSendData(struct TNCINFO * TNC, UCHAR * data, int txlen)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, data, txlen);

	return 0;
}

int SerialSendCommand(struct TNCINFO * TNC, UCHAR * data)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, data, (int)strlen(data));

	return 0;
}

int ProcessEscape(UCHAR * TXMsg)
{
	UCHAR * Orig = TXMsg;
	UCHAR * ptr1 = TXMsg;
	UCHAR * ptr2 = strstr(TXMsg, "\\\\");
	UCHAR * ptr3 = strchr(TXMsg, '^');

	BOOL HexEscape = FALSE;
	int NewLen;

	// Now using ^C for ctrl/c, etc
	// Still use \\d
	// ^^ for ^, \\\ for \\

	while (ptr2)
	{
		UCHAR Next;

		ptr1 = ptr2;		// over stuff before escape

		ptr2 += 2;			// over \\
		
		Next = *ptr2;

		if (Next == '\\')
		{
			ptr1 = ptr2;			// put \\ in message
			memmove(ptr2, ptr2 + 1, strlen(ptr2));
		}
		else if (Next == 'd' || Next == 'D')
			return 0;

		ptr2 = strstr(ptr2, "\\\\");
	}

	// now look for ^

	ptr1 = Orig;
	ptr2 = strchr(Orig, '^');

	while (ptr2)
	{
		UCHAR Next;

		ptr1 = ptr2;		// over stuff before escape
		ptr2 ++;			// over ^
		
		Next = *ptr2;

		if (Next != '^')
		{
			Next &= 0x1F;	// Mask to control char
			HexEscape = TRUE;
		}

		*ptr1 = Next;
			
		memmove(ptr2, ptr2 + 1, strlen(ptr2));

		ptr2 = strchr(ptr2, '^');
	}
	
	NewLen = (int)strlen(Orig);

	if (HexEscape)
	{
		//	remove trailing CR

		NewLen --;

		Orig[NewLen] = 0;
	}

	return NewLen;
}
	