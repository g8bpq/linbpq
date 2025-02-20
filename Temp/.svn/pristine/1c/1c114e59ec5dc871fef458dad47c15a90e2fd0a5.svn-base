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
//	Interface to allow G8BPQ switch to use  HSMODEM TNC 


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifndef WIN32
#ifndef MACBPQ
#include <sys/ioctl.h>
#endif
#endif


#include "cheaders.h"

#pragma pack(1)

struct BroadcastMsg
{
	unsigned char Type;
	unsigned char initialVolTX;
	unsigned char initialVolRX;
	unsigned char AudioTimespan;
	unsigned char intialVolSpeaker;
	unsigned char initalVolMic;
	unsigned char Retransmits;
	unsigned char SendAudio;
	unsigned char RTTYAutoSync;
	unsigned char Speed;
	char playbackDevice[100];
	char captureDevice[100];
	char Callsign[20];
	char Locator[10];
	char Name[20];
};

struct FileHeader
{
	unsigned char Type;
	unsigned char Info;		// 0 - First, 1 - Continuation 2 Last 3  - Only
	char filename[50];
	unsigned short CRC;		// of filename = transfer id
	unsigned char Size[3];	// Big endian
	unsigned char Data[164];
};

struct FileData
{
	unsigned char Type;
	unsigned char Info;		// 0 - First, 1 - Continuation 2 Last 3  - Only
	unsigned char Data[219];
};

#pragma pack()

struct HSFILEINFO
{
	struct HSFILEINFO * Next;	// May want to chain entries for partial files

	char fileName[50];
	unsigned short CRC;			// Used as a transfer ID
	int fileSize;
	int Sequence;
	int State;
	int Type;
	time_t LastRX;
	unsigned char goodBlocks[1024];
	unsigned char * Data;
	int dataPointer;
	int lastBlock;
	int lostBlocks;
	unsigned char * txData;
	int txSize;
	int txLeft;
};


struct HSMODEMINFO
{
	struct HSFILEINFO * File;

	int Mode;
	char * Capture;				// Capture Device Name
	char * Playback;			// Playback Device Name
	int Seq;					// To make CRC more Unique
	int txFifo;
	int rxFifo;
	int Sync;
	int DCD;
};


int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);

extern int (WINAPI FAR *GetModuleFileNameExPtr)();
extern int (WINAPI FAR *EnumProcessesPtr)();

#include "bpq32.h"

#include "tncinfo.h"

static int Socket_Data(int sock, int error, int eventcode);

VOID MoveWindows(struct TNCINFO * TNC);
static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen);
int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);
BOOL HSMODEMWriteCommBlock(struct TNCINFO * TNC);
void HSMODEMCheckRX(struct TNCINFO * TNC);
int HSMODEMSendData(struct TNCINFO * TNC, UCHAR * data, int txlen);
int HSMODEMSendSingleData(struct TNCINFO * TNC, UCHAR * FN, UCHAR * data, int txlen);
int HSMODEMSendCommand(struct TNCINFO * TNC, UCHAR * data);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
VOID SendInitScript(struct TNCINFO * TNC);
int HSMODEMGetLine(char * buf);
int ProcessEscape(UCHAR * TXMsg);
BOOL KAMStartPort(struct PORTCONTROL * PORT);
BOOL KAMStopPort(struct PORTCONTROL * PORT);
void SendPoll(struct TNCINFO * TNC);
void SendMode(struct TNCINFO * TNC);

static char ClassName[]="HSMODEMSTATUS";
static char WindowTitle[] = "HSMODEM";
static int RigControlRow = 205;

#ifndef LINBPQ
#include <commctrl.h>
#endif

extern int SemHeldByAPI;

static RECT Rect;

static int ProcessLine(char * buf, int Port);

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

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

	TNC->HSModemInfo = zalloc(sizeof(struct HSMODEMINFO));
	TNC->HSModemInfo->File = zalloc(sizeof(struct HSFILEINFO));

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
	if (p_ipad == NULL)
		p_ipad = strtok(NULL, " \t\n\r");

	if (p_ipad == NULL) return (FALSE);
	
	p_port = strtok(NULL, " \t\n\r");
			
	if (p_port == NULL) return (FALSE);

	WINMORport = atoi(p_port);

	TNC->TCPPort = WINMORport;

	TNC->destaddr.sin_family = AF_INET;
	TNC->destaddr.sin_port = htons(WINMORport + 2);		// We only receive on Port + 2

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
			TNC->HSModemInfo->Capture = _strdup(&buf[8]);
			strlop(TNC->HSModemInfo->Capture, 13);
		}
		else if (_memicmp(buf, "PLAYBACK", 8) == 0)
		{
			TNC->HSModemInfo->Playback = _strdup(&buf[9]);
			strlop(TNC->HSModemInfo->Playback, 13);
		}
		else if (_memicmp(buf, "MODE ", 5) == 0)
			TNC->HSModemInfo->Mode = atoi(&buf[5]);	
		else if (_memicmp(buf, "LOGDIR ", 7) == 0)
			TNC->LogPath = _strdup(&buf[7]);
		else
			strcat (TNC->InitScript, buf);
	}

	return (TRUE);	
}

static char * Config;
static char * ptr1, * ptr2;

int HSMODEMGetLine(char * buf)
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
		// HSMODEM mode. Queue to Hostmode driver
		
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;

		return;
	}
}


VOID HSMODEMChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

	datalen = sprintf(TXMsg, "MYCALL %s\r", Call);
	HSMODEMSendCommand(TNC, TXMsg);
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

	switch (fn)
	{
		case 7:			

			// 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		// G7TAJ's code to record activity for stats display
			
		if ( TNC->BusyFlags && CDBusy )
			TNC->PortRecord->PORTCONTROL.ACTIVE += 2;

		if ( TNC->PTTState )
			TNC->PortRecord->PORTCONTROL.SENDING += 2;

		if (TNC->CONNECTED)
			{
				TNC->CONNECTED--;

				if (TNC->CONNECTED == 0)
				{
					sprintf(TNC->WEB_COMMSSTATE, "Connection to HSMODEM lost");		
					MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
				}
			}

			TNC->PollDelay++;

			if (TNC->PollDelay > 20)
			{
				TNC->PollDelay = 0;
	
				SendPoll(TNC);
			}
		
			return 0;

		case 1:				// poll

		HSMODEMCheckRX(TNC);

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

			HSMODEMSendSingleData(TNC, FECMsg, Buffer, datalen);

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

					HSMODEMSendCommand(TNC, "DISCONNECT\r");
				}
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				Debugprintf("HSMODEM New Attach Stream %d", Stream);
			
				STREAM->Attached = TRUE;
			
				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;
			
	
				HSMODEMChangeMYC(TNC, TNC->Streams[0].MyCall);
		
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

				bytes=HSMODEMSendData(TNC, data, txlen);
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

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr->Len = 21;
			memcpy(&buffptr->Data[0], "No Connection to TNC\r", 21);

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

		if (STREAM->Connected)
		{
			STREAM->PacketsSent++;

			bytes=HSMODEMSendData(TNC, TXMsg, txlen);
			TNC->Streams[Stream].BytesOutstanding += bytes;		// So flow control works - will be updated by BUFFER response
			STREAM->bytesTXed += bytes;
//			WritetoTrace(TNC, &buff->L2DATA[0], txlen);
	
			return 1;
		}

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
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "HSMODEM} OK\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;

		}

		if (_memicmp(&buff->L2DATA[0], "MODE ", 5) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			TNC->HSModemInfo->Mode = atoi(&buff->L2DATA[5]);

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "HSMODEM} OK\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			SendMode(TNC);

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

			HSMODEMChangeMYC(TNC, TNC->Streams[0].MyCall);

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
			HSMODEMSendCommand(TNC, Connect);
			MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
				
			STREAM->Connecting = TRUE;
			return 0;

		}

		// Normal data. Send to TNC


		HSMODEMSendData(TNC, TXMsg, txlen);
	
		return 0;

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

			if (Queued > 4 || Outstanding > 8500)
				return (1 | (TNC->HostMode | TNC->CONNECTED) << 8 | STREAM->Disconnecting << 15);
		}
		
		if (TNC->Streams[Stream].Attached == 0)
			return (TNC->CONNECTED != 0) << 8 | 1;

		return ((TNC->CONNECTED != 0) << 8 | TNC->Streams[Stream].Disconnecting << 15);		// OK
		

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
			if (!TNC->CONNECTED)
				return 0;					// No connection so no interlock
			
			if (TNC->ConnectPending == 0 && TNC->PTTState == 0)
			{
				HSMODEMSendCommand(TNC, "CONOK OFF");
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
					HSMODEMSendCommand(TNC, "CONOK ON");
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;
		return 0;
	}
	return 0;
}

VOID HSMODEMReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	HSMODEMChangeMYC(TNC, TNC->NodeCall);

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID HSMODEMSuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC)
{
	HSMODEMSendCommand(TNC, "CONOK OFF\r");
}

VOID HSMODEMReleasePort(struct TNCINFO * TNC)
{
	HSMODEMSendCommand(TNC, "CONOK ON\r");
}


static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}</script>"
		"</head><title>VARA Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2><form method=post target=\"POPUPW\" onsubmit=\"POPUPW = window.open('about:blank','POPUPW',"
		"'width=440,height=150');\" action=ARDOPAbort?%d>HSMODEM Status"
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
HBRUSH RedBrush = NULL;
HBRUSH GreenBrush;
HBRUSH BlueBrush;
static HBRUSH bgBrush = NULL;

extern HWND ClientWnd, FrameWnd;
extern int OffsetH, OffsetW;

extern HMENU hMainFrameMenu;
extern HMENU hBaseMenu;
extern HANDLE hInstance;

extern HKEY REGTREE;



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

		TextOut(hdc, 10, 162, "RX", 4);
		TextOut(hdc, 10, 182, "TX", 4);

		if (TNC->HSModemInfo->Sync)
			TextOut(hdc, 305, 162, "Sync", 4);

//		SelectObject(ps.hdc, RedBrush); 
		SelectObject(ps.hdc, GreenBrush); 
//		SelectObject(ps.hdc, GetStockObject(GRAY_BRUSH)); 

		Rectangle(ps.hdc, 40, 165, TNC->HSModemInfo->rxFifo + 42, 175); 
		SelectObject(ps.hdc, RedBrush); 
		Rectangle(ps.hdc, 40, 185, (TNC->HSModemInfo->txFifo * 10) + 42, 195); 

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
#endif



VOID * HSMODEMExtInit(EXTPORTDATA * PortEntry)
{
	int port;
	char Msg[255];
	char * ptr;
	struct TNCINFO * TNC;
	char * TempScript;
	u_long param = 1;
	int ret;
	
	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		sprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return ExtProc;
	}

#ifndef LINBPQ

	if (bgBrush == NULL)
	{
		bgBrush = CreateSolidBrush(BGCOLOUR);
		RedBrush = CreateSolidBrush(RGB(255,0,0));
		GreenBrush = CreateSolidBrush(RGB(0,255,0));
		BlueBrush = CreateSolidBrush(RGB(0,0,255));
	}

#endif

	Consoleprintf("HSMODEM Host %s %d", TNC->HostName, TNC->TCPPort);

	TNC->Port = port;
	TNC->Hardware = H_HSMODEM;

	TNC->PortRecord = PortEntry;

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

	TNC->SuspendPortProc = HSMODEMSuspendPort;
	TNC->ReleasePortProc = HSMODEMReleasePort;

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
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,138,20,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTTIME = CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,138,200,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

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


	// Open and bind UDP socket

	TNC->TCPSock = socket(AF_INET,SOCK_DGRAM,0);

	if (TNC->TCPSock  == INVALID_SOCKET)
	{
		WritetoConsole("Failed to create UDP socket for HSMODEM");
		ret = WSAGetLastError();
	}
	else
		ioctl (TNC->TCPSock, FIONBIO, &param);

	ret = bind(TNC->TCPSock, (struct sockaddr *) &TNC->destaddr, sizeof(struct sockaddr_in));

	if (ret != 0)
	{
		//	Bind Failed

		ret = WSAGetLastError();
		sprintf(Msg, "Bind Failed for UDP port %d - error code = %d", TNC->TCPPort + 2, ret);
		WritetoConsole(Msg);
	}

 
//	SendInitScript(TNC);

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}


VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[Stream].BytesOutstanding == 0)
		HSMODEMSendCommand(TNC, "DISCONNECT\r");
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	HSMODEMSendCommand(TNC, "DISCONNECT\r");
}



VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
	{
		HSMODEMReleaseTNC(TNC);
	}
}

VOID HSMODEMAbort(struct TNCINFO * TNC)
{
	HSMODEMSendCommand(TNC, "ABORT\r");
}

// Host Mode Stuff (we reuse some routines in SCSPactor)

VOID HSMODEMDoTermModeTimeout(struct TNCINFO * TNC)
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

RECT Rect1 = {30, 160, 400, 195};


VOID HSMODEMProcessTNCMessage(struct TNCINFO * TNC, unsigned char * Msg, int Len)
{
	PMSGWITHLEN buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct FileHeader * FH;
	struct HSMODEMINFO * Modem = TNC->HSModemInfo;
	struct HSFILEINFO * Info = Modem->File;
	int fileLen, Seq, Offset;

	// Any message indicates Ok

	Msg[Len] = 0;

	if (TNC->CONNECTED == 0)
	{
		// Just come up

		sprintf(TNC->WEB_COMMSSTATE, "Connected to HSMODEM");		
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
		SendMode(TNC);
	}

	TNC->CONNECTED = 100;					// time out after 10 secs

	/*
	3: responses to broadcast messages (see: GUI Interface: UDP/IP/Initialization)
	1: received payload data
	4: FFT data for a spectrum monitor
	5: IQ data for a constellation display
	6: received RTTY characters
	*/
	switch (Msg[0])
	{
	case 1:
		/*
		Byte 0 ... 0x01
		Byte 1 ... frame type (which was inserted by the sender)
		Byte 2 ... frame counter MSB
		Byte 3 ... frame counter LSB (10 bits used)
		Byte 4 ... frame information (which was inserted by the sender)
		Byte 5 ... unused
		Byte 6 ... measured line speed MSB
		Byte 7 ... measured line speed LSB
		Bytes 8-10 ... unused
		Bytes 11-229 ... 219 bytes payload		return;

		1 … BER Test Pattern
		2 … Image
		3 … Ascii File
		4 … HTML File
		5 … Binary File
		6 … Voice Audio (for Codec 2 or Opus)
		7 … UserInfo
		*/

		Seq = Msg[2] << 8 | Msg[3];

		switch (Msg[1])
		{
		case 1:
		case 6:
		case 7:

			Debugprintf("%d %d %02x %s %s %s", Msg[1], Seq, Msg[4], &Msg[11], &Msg[31], &Msg[41]);
			return;

		case 2:
		case 3:
		case 4:
		case 5:

			// File transfer types

			switch (Msg[4])
			{
			case 0:
			case 3:

				// File Header

				FH = (struct FileHeader *) &Msg[9];

				if (FH->CRC == Info->CRC)
				{
					Debugprintf("Dup Header %X", Info->CRC);
					return;
				}

				Info->CRC = FH->CRC;

				fileLen = FH->Size[0] * 65536 + FH->Size[1] * 256 + FH->Size[2];

				Info->Data = zalloc(fileLen + 512);

				if (Info->Data == NULL)
					return;

				Info->fileSize = fileLen;
				strcpy(Info->fileName, FH->filename);

				memset(Info->goodBlocks, 0, 1024);
				Info->goodBlocks[0] = 1;

				Info->lastBlock = 0;
				Info->lostBlocks = 0;
				Info->LastRX = time(NULL);
				
				Debugprintf("%d %d %04X %02x %s %d %s", Msg[1], Seq, FH->CRC, Msg[4],
					FH->filename, fileLen, FH->Data);

				memcpy(Info->Data, FH->Data, 164);
				Info->dataPointer = 164;
				break;

			case 1:
			case 2:

				// Data Frame

				if (Seq == Info->lastBlock)
				{
					Debugprintf("Duplicate data frame %d", Seq);
					return;
				}
					
				Info->lastBlock++;
				
				if (Info->lastBlock != Seq)
					Info->lostBlocks += Seq - Info->lastBlock;

				Info->goodBlocks[Seq] = 1;

				Offset = (Seq - 1) * 221 + 164;

				memcpy(&Info->Data[Offset], &Msg[11], 221);

				Debugprintf("%d %d %02x %s %d %s", Msg[1], Seq, Msg[4], &Msg[11]);
				break;

			default:

				Debugprintf("%d %d %02x %s %d %s", Msg[1], Seq, Msg[4], &Msg[11]);
				return;

			}

			// End of Data Frame Case

			if (Msg[4] == 2 || Msg[4] == 3)
			{
				// Last Frame - check file

				if (Info->lostBlocks == 0)
				{
					// filename is encoding of calls and frame type

					struct _MESSAGE * buffptr;

//					FILE * fp1 = fopen(Info->fileName, "wb");
//					int WriteLen;

//					if (fp1)
//					{
//						WriteLen = (int)fwrite(Info->Data, 1, Info->fileSize, fp1);
//						fclose(fp1);
//					}

					if (strchr(Info->fileName, '!'))
					{
						// Callsigns encoded in filename



						char * Origin = &Info->fileName[0];
						char * Type =  strlop(Origin, '_');
						char * Dest = strlop(Origin, '!');
						unsigned char * Packet;


						// Convert to ax.25 format

						buffptr = GetBuff();

						// Convert to ax.25 format
			
						if (buffptr == 0)
							return;				// No buffers, so ignore

						Type =  strlop(Origin, '_');
				
						Packet = &buffptr->ORIGIN[0];

						buffptr->PORT = TNC->Port;
						buffptr->LENGTH = 16 + MSGHDDRLEN + Info->fileSize;

						ConvToAX25(Origin, buffptr->ORIGIN);
						ConvToAX25(Dest, buffptr->DEST);


						while (strchr(Dest, ','))
						{
							Dest = strlop(Dest, ',');	// Next digi
							Packet += 7;
							ConvToAX25(Dest, Packet);
							buffptr->LENGTH += 7;
						}
							
						Packet[6] |= 1;				// Set end of address
						
						Packet += 7;

						*(Packet++) = 3;
						*(Packet++) = 0xF0;
	
						memcpy(Packet, Info->Data, Info->fileSize);
						time(&buffptr->Timestamp);

						BPQTRACE((MESSAGE *)buffptr, TRUE);
					}
				}

				return;
			}
		}

		return;

	case 4:		// FFT data for a spectrum monitor

		Modem->txFifo = Msg[1];
		Modem->rxFifo = Msg[2];
		Modem->DCD = Msg[3];
		Modem->Sync = Msg[4];

#ifndef LINBPQ
		InvalidateRect(TNC->hDlg, &Rect1, TRUE);
#endif

//		if (Info->Sync || Info->txFifo)
//			Debugprintf("%d %d %d %d", Info->txFifo, Info->rxFifo, Info->DCD, Info->Sync);
		/*
 Byte 0 ... 0x04
 Byte 1 ... usage of the TX fifo (used by the transmitter to sync its data 
            output to the modem). This is a value between 0..255. During 
            an active transmission keep it above 4.
 Byte 2 ... usage of RX fifo (not important, but can be displayed to the 
            user). A very high RX fifo usage indicates the the computer 
            is too slow for HSmodem.
 Byte 3 ... 0 or 1. Indicates that an RF level was detected
 Byte 4 ... 0 or 1. Indicates that the HSmodem receiver is synchronized 
            with a signal
 Byte 5 ... maximum audio level (0..100%) of the audio input from the 
            transceiver. Can be used to detect clipping.
 Byte 6 ... maximum audio level (0..100%) of the audio output to the 
            transceiver. Can be used to detect clipping.
 Byte 7 ... in RTTY mode this is the auto-locked RTTY frequency MSB
 Byte 8 ... and LSB
 Byte 9 ... RTTY: 0=tx off, 1=txon
 Byte 10 to the end ... FFT spectrum data, beginning at 0 Hz to 4kHz with 
            a resolution of 10 Hz
*/		
		return;


	case 5:	// IQ data for a constellation display
		return;
	
	case 6: //received RTTY characters
		return;
	}

	return;


	buffptr = GetBuff();

	if (buffptr == 0)
	{
		return;			// No buffers, so ignore
	}
	
	buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "%s", TNC->RXBuffer);

	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

}

extern char LOC[7];

void SendMode(struct TNCINFO * TNC)
{
	unsigned char Msg[221] = "";
	int ret;

	Msg[0] = 16;
	Msg[1] = TNC->HSModemInfo->Mode;

	TNC->destaddr.sin_port = htons(TNC->TCPPort + 1);		// Data Port
	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	ret = sendto(TNC->TCPSock, (char *)&Msg, 221,  0, (struct sockaddr *)&TNC->destaddr, sizeof(struct sockaddr));

	return;
}

void SendPoll(struct TNCINFO * TNC)
{
	struct BroadcastMsg PollMsg = {0x3c, 100, 100, 0, 50, 50, 1, 0, 0, 9};
	int ret;

	strcpy(&PollMsg.captureDevice[0], TNC->HSModemInfo->Capture);
	strcpy(&PollMsg.playbackDevice[0], TNC->HSModemInfo->Playback);
//	strcpy(&PollMsg.playbackDevice[0], "CABLE Input (VB-Audio Virtual Cable)");

	strcpy(&PollMsg.Callsign[0], TNC->NodeCall);
	strcpy(&PollMsg.Locator[0], LOC);
	strcpy(&PollMsg.Name[0], "1234567890");

	TNC->destaddr.sin_port = htons(TNC->TCPPort);		// Command Port
	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	ret = sendto(TNC->TCPSock, (char *)&PollMsg, 260,  0, (struct sockaddr *)&TNC->destaddr, sizeof(struct sockaddr));

	return;
}
/*
	unsigned char Type;
	unsigned char Info;		// 0 - First, 1 - Continuation 2 Last 3  - Only
	char filename[50];
	unsigned short CRC;		// of filename = transfer id
	unsigned char Size[3];	// Big endian
	unsigned char Data[163];
*/

unsigned short int compute_crc(unsigned char *buf,int len);

int HSMODEMSendSingleData(struct TNCINFO * TNC, UCHAR * FN, UCHAR * data, int txlen)
{
	struct FileHeader Msg;
	unsigned short int crc;
	int ret, fragLen = txlen;
	struct HSMODEMINFO * Modem = TNC->HSModemInfo;
	struct HSFILEINFO * Info = Modem->File;

	char Seq[60] = "";

	sprintf(Seq, "%04X%s", Modem->Seq++, FN);

	crc = compute_crc(Seq, 60);

	crc ^= 0xffff;

	memset(&Msg, 0, sizeof(struct FileHeader));

	Msg.Type = 5;			// Binary Data
	Msg.Info = 3;			// Only Fragment

	if (txlen > 163)
	{
		// Need to send as multiple fragments

		fragLen = 164;
		Info->txData = malloc(txlen + 512);
		memcpy(Info->txData, data, txlen);
		Info->txSize = txlen;
		Info->txLeft = txlen - 164;
		Msg.Info = 0;			// First Fragment
	}

	strcpy(Msg.filename, FN);
	memcpy(Msg.Data, data, txlen);
	memcpy(&Msg.CRC, &crc, 2);
	Msg.Size[0] = txlen >> 16;
	Msg.Size[1] = txlen >> 8;;
	Msg.Size[2] = txlen;

	TNC->destaddr.sin_port = htons(TNC->TCPPort + 1);		// Data Port
	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	ret = sendto(TNC->TCPSock, (char *)&Msg, 221,  0, (struct sockaddr *)&TNC->destaddr, sizeof(struct sockaddr));
	memset(&Msg, 0, sizeof(struct FileHeader));

	return ret;
}

int HSMODEMSendData(struct TNCINFO * TNC, UCHAR * data, int txlen)
{
	struct FileHeader Msg;

	memset(&Msg, 0, sizeof(struct FileHeader));

	Msg.Type = 5;			// Binary Data
	Msg.Info = 3;			// Only Fragment


	return 0;
}

void HSMODEMCheckRX(struct TNCINFO * TNC)
{
	int  Len = 0;
	unsigned char Buff[2000];
		
	struct sockaddr_in rxaddr;
	int addrlen = sizeof(struct sockaddr_in);

	Len = recvfrom(TNC->TCPSock, Buff, 2000, 0, (struct sockaddr *)&rxaddr, &addrlen);

	while (1)
	{
		if (Len == -1)
		{
//			Debugprintf("%d", GetLastError());
			Len = 0;
			return;
		}
		TNC->RXLen = Len;
		HSMODEMProcessTNCMessage(TNC, Buff, Len);

		Len = recvfrom(TNC->TCPSock, Buff, 2000, 0, (struct sockaddr *)&rxaddr, &addrlen);

	}
	return;
		
	return;
}

int HSMODEMWriteCommBlock(struct TNCINFO * TNC)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, TNC->TXBuffer, TNC->TXLen);

	return 0;
}


int HSMODEMSendCommand(struct TNCINFO * TNC, UCHAR * data)
{
	if (TNC->hDevice)
		return WriteCOMBlock(TNC->hDevice, data, (int)strlen(data));

	return 0;
}

	