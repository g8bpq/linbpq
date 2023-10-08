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



// General C Routines common to bpq32 and linbpq. Mainly moved from BPQ32.c

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma data_seg("_BPQDATA")

#include "CHeaders.h"
#include "tncinfo.h"
#include "configstructs.h"

extern struct CONFIGTABLE xxcfg;

#define LIBCONFIG_STATIC
#include "libconfig.h"

#ifndef LINBPQ

//#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.

#include "commctrl.h"
#include "Commdlg.h"

#endif

struct TNCINFO * TNCInfo[70];		// Records are Malloc'd

extern int ReportTimer;

Dll VOID APIENTRY Send_AX(UCHAR * Block, DWORD Len, UCHAR Port);
TRANSPORTENTRY * SetupSessionFromHost(PBPQVECSTRUC HOST, UINT ApplMask);
int Check_Timer();
VOID SENDUIMESSAGE(struct DATAMESSAGE * Msg);
DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID APIENTRY md5 (char *arg, unsigned char * checksum);
VOID COMSetDTR(HANDLE fd);
VOID COMClearDTR(HANDLE fd);
VOID COMSetRTS(HANDLE fd);
VOID COMClearRTS(HANDLE fd);

VOID WriteMiniDump();
void printStack(void);
char * FormatMH(PMHSTRUC MH, char Format);
void WriteConnectLog(char * fromCall, char * toCall, UCHAR * Mode);
void SendDataToPktMap(char *Msg);

extern BOOL LogAllConnects;
extern BOOL M0LTEMap;


extern VOID * ENDBUFFERPOOL;


//	Read/Write length field in a buffer header

//	Needed for Big/LittleEndian and ARM5 (unaligned operation problem) portability


VOID PutLengthinBuffer(PDATAMESSAGE buff, USHORT datalen)
{
	if (datalen <= sizeof(void *) + 4)
		datalen = sizeof(void *) + 4;		// Protect

	memcpy(&buff->LENGTH, &datalen, 2);
}

int GetLengthfromBuffer(PDATAMESSAGE buff)
{
	USHORT Length;

	memcpy(&Length, &buff->LENGTH, 2);
	return Length;
}

BOOL CheckQHeadder(UINT * Q)
{
#ifdef WIN32
	UINT Test;

	__try
	{
		Test = *Q;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("Invalid Q Header %p", Q);
		printStack();
		return FALSE;
	}
#endif
	return TRUE;
}

// Get buffer from Queue


VOID * _Q_REM(VOID **PQ, char * File, int Line)
{
	void ** Q;
	void ** first;
	VOID * next;
	PMESSAGE Test;

	//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (Semaphore.Flag == 0)
		Debugprintf("Q_REM called without semaphore from %s Line %d", File, Line);

	if (CheckQHeadder((UINT *) Q) == 0)
		return(0);

	first = Q[0];

	if (first == 0)
		return (0);			// Empty

	next = first[0];			// Address of next buffer

	Q[0] = next;

	// Make sure guard zone is zeros

	Test = (PMESSAGE)first;

	if (Test->GuardZone != 0)
	{
		Debugprintf("Q_REM %p GUARD ZONE CORRUPT %x Called from %s Line %d", first, Test->GuardZone, File, Line);
		printStack();
	}

	return first;
}

// Non=pool version (for IPGateway)

VOID * _Q_REM_NP(VOID *PQ, char * File, int Line)
{
	void ** Q;
	void ** first;
	void * next;

	//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)
		return(0);

	first = Q[0];

	if (first == 0) return (0);			// Empty

	next = first[0];			// Address of next buffer

	Q[0] = next;

	return first;
}

// Return Buffer to Free Queue

extern VOID * BUFFERPOOL;
extern void ** Bufferlist[1000];
void printStack(void);

void _CheckGuardZone(char * File, int Line)
{
	int n = 0, i, offset = 0;
	PMESSAGE Test;
	UINT CodeDump[8];
	unsigned char * ptr;

	n = NUMBEROFBUFFERS;

	while (n--)
	{
		Test = (PMESSAGE)Bufferlist[n];
	
		if (Test && Test->GuardZone)
		{
			Debugprintf("CheckGuardZone %p GUARD ZONE CORRUPT %d Called from %s Line %d", Test, Test->Process, File, Line);

			offset = 0;
			ptr = (unsigned char *)Test;

			while (offset < 400)
			{
				memcpy(CodeDump, &ptr[offset], 32);
	
				for (i = 0; i < 8; i++)
					CodeDump[i] = htonl(CodeDump[i]);
	
				Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
					&ptr[offset], CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

				offset += 32;
			}
			WriteMiniDump();
#ifdef MDIKERNEL
			CloseAllNeeded = 1;
#endif
		}

	}
}

UINT _ReleaseBuffer(VOID *pBUFF, char * File, int Line)
{
	void ** pointer, ** BUFF = pBUFF;
	int n = 0;
	void ** debug;
	PMESSAGE Test;
	UINT CodeDump[16];
	int i;
	unsigned int rev;

	if (Semaphore.Flag == 0)
		Debugprintf("ReleaseBuffer called without semaphore from %s Line %d", File, Line);

	// Make sure address is within pool

	if ((uintptr_t)BUFF < (uintptr_t)BUFFERPOOL || (uintptr_t)BUFF > (uintptr_t)ENDBUFFERPOOL)
	{
		// Not pointing to a buffer . debug points to the buffer that this is chained from

		// Dump first chunk and source tag

		memcpy(CodeDump, BUFF, 64);

		Debugprintf("Releasebuffer Buffer not in pool from %s Line %d, ptr %p prev %d", File, Line, BUFF, 0);

		for (i = 0; i < 16; i++)
		{
			rev = (CodeDump[i] & 0xff) << 24;
			rev |= (CodeDump[i] & 0xff00) << 8;
			rev |= (CodeDump[i] & 0xff0000) >> 8;
			rev |= (CodeDump[i] & 0xff000000) >> 24;

			CodeDump[i] = rev;
		}

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			Bufferlist[n], CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

		Debugprintf("         %08x %08x %08x %08x %08x %08x %08x %08x",
			CodeDump[8], CodeDump[9], CodeDump[10], CodeDump[11], CodeDump[12], CodeDump[13], CodeDump[14], CodeDump[15]);


		return 0;
	}

	Test = (PMESSAGE)pBUFF;

	if (Test->GuardZone != 0)
	{
		Debugprintf("_ReleaseBuffer %p GUARD ZONE CORRUPT %x Called from %s Line %d", pBUFF, Test->GuardZone, File, Line);
	}

	while (n <= NUMBEROFBUFFERS)
	{
		if (BUFF == Bufferlist[n++])
			goto BOK1;
	}

	Debugprintf("ReleaseBuffer %X not in Pool called from %s Line %d", BUFF, File, Line);
	printStack();

	return 0;

BOK1:

	n = 0;

	// validate free Queue

	pointer = FREE_Q;
	debug = &FREE_Q;

	while (pointer)
	{
		// Validate pointer to make sure it is in pool - it may be a duff address if Q is corrupt 

		Test = (PMESSAGE)pointer;

		if (Test->GuardZone || (uintptr_t)pointer < (uintptr_t)BUFFERPOOL || (uintptr_t)pointer > (uintptr_t)ENDBUFFERPOOL)
		{
			// Not pointing to a buffer . debug points to the buffer that this is chained from

			// Dump first chunk and source tag

			memcpy(CodeDump, debug, 64);

			Debugprintf("Releasebuffer Pool Corruption n = %d, ptr %p prev %p", n, pointer, debug);
	
			for (i = 0; i < 16; i++)
			{
				rev = (CodeDump[i] & 0xff) << 24;
				rev |= (CodeDump[i] & 0xff00) << 8;
				rev |= (CodeDump[i] & 0xff0000) >> 8;
				rev |= (CodeDump[i] & 0xff000000) >> 24;

				CodeDump[i] = rev;
			}

			Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
				Bufferlist[n], CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

			Debugprintf("         %08x %08x %08x %08x %08x %08x %08x %08x",
				CodeDump[8], CodeDump[9], CodeDump[10], CodeDump[11], CodeDump[12], CodeDump[13], CodeDump[14], CodeDump[15]);
					
			if (debug[400])
				Debugprintf("         %s", &debug[400]);

		}

		// See if already on free Queue
	
		if (pointer == BUFF)
		{
			Debugprintf("Trying to free buffer %p when already on FREE_Q called from %s Line %d", BUFF, File, Line);
//			WriteMiniDump();
			return 0;
		}

//		if (pointer[0] && pointer == pointer[0])
//		{
//			Debugprintf("Buffer chained to itself");
//			return 0;
//		}

		debug = pointer;
		pointer = pointer[0];
		n++;

		if (n > 1000)
		{
			Debugprintf("Loop searching free chain - pointer = %p %p", debug, pointer);
			return 0;
		}
	}

	pointer = FREE_Q;

	*BUFF = pointer;

	FREE_Q = BUFF;

	QCOUNT++;

	return 0;
}

int _C_Q_ADD(VOID *PQ, VOID *PBUFF, char * File, int Line)
{
	void ** Q;
	void ** BUFF = PBUFF;
	void ** next;
	PMESSAGE Test;


	int n = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (Semaphore.Flag == 0)
		Debugprintf("C_Q_ADD called without semaphore from %s Line %d", File, Line);

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	// Make sure guard zone is zeros

	Test = (PMESSAGE)PBUFF;

	if (Test->GuardZone != 0)
	{
		Debugprintf("C_Q_ADD %p GUARD ZONE CORRUPT %x Called from %s Line %d", PBUFF, Test->GuardZone, File, Line);
	}

	Test = (PMESSAGE)Q;



	// Make sure address is within pool

	while (n <= NUMBEROFBUFFERS)
	{
		if (BUFF == Bufferlist[n++])
			goto BOK2;
	}

	Debugprintf("C_Q_ADD %X not in Pool called from %s Line %d", BUFF, File, Line);
	printStack();

	return 0;

BOK2:

	BUFF[0] = 0;						// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=BUFF;				// New one on front
		return(0);
	}

	next = Q[0];

	while (next[0] != 0)
	{
		next = next[0];			// Chain to end of queue
	}
	next[0] = BUFF;					// New one on end

	return(0);
}

// Non-pool version

int C_Q_ADD_NP(VOID *PQ, VOID *PBUFF)
{
	void ** Q;
	void ** BUFF = PBUFF;
	void ** next;
	int n = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=BUFF;				// New one on front
//		memcpy(PQ, &BUFF, 4);
		return 0;
	}
	next = Q[0];

	while (next[0] != 0)
		next=next[0];				// Chain to end of queue

	next[0] = BUFF;					// New one on end

	return(0);
}


int C_Q_COUNT(VOID *PQ)
{
	void ** Q;
	int count = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	//	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER

	while (*Q)
	{
		count++;
		if ((count + QCOUNT) > MAXBUFFS)
		{
 			Debugprintf("C_Q_COUNT Detected corrupt Q %p len %d", PQ, count);
			return count;
		}
		Q = *Q;
	}

	return count;
}

VOID * _GetBuff(char * File, int Line)
{
	UINT * Temp;
	MESSAGE * Msg;
	char * fptr = 0;
	unsigned char * byteaddr;

	Temp = Q_REM(&FREE_Q);

//	FindLostBuffers();

	if (Semaphore.Flag == 0)
		Debugprintf("GetBuff called without semaphore from %s Line %d", File, Line);

	if (Temp)
	{
		QCOUNT--;

		if (QCOUNT < MINBUFFCOUNT)
			MINBUFFCOUNT = QCOUNT;

		Msg = (MESSAGE *)Temp;
		fptr = File + (int)strlen(File);
		while (*fptr != '\\' && *fptr != '/')
			fptr--;
		fptr++;

		// Buffer Length is BUFFLEN, but buffers are allocated 512
		// So add file info in gap between

		byteaddr = (unsigned char *)Msg;


		memset(&byteaddr[0], 0, 64);		// simplify debugging lost buffers
		memset(&byteaddr[400], 0, 64);		// simplify debugging lost buffers
		sprintf(&byteaddr[400], "%s %d", fptr, Line);

		Msg->Process = (short)GetCurrentProcessId();
		Msg->Linkptr = NULL;
	}
	else
		Debugprintf("Warning - Getbuff returned NULL");

	return Temp;
}

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;

	if (buf == NULL) return NULL;		// Protect

	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

VOID DISPLAYCIRCUIT(TRANSPORTENTRY * L4, char * Buffer)
{
	UCHAR Type = L4->L4CIRCUITTYPE;
	struct PORTCONTROL * PORT;
	struct _LINKTABLE * LINK;
	BPQVECSTRUC * VEC;
	struct DEST_LIST * DEST;

	char Normcall[20] = "";			// Could be alias:call
	char Normcall2[11] = "";
	char Alias[11] = "";

	Buffer[0] = 0;

	switch (Type)
	{
	case PACTOR+UPLINK:

		PORT = L4->L4TARGET.PORT;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		if (PORT)
			sprintf(Buffer, "%s %d/%d(%s)", "TNC Uplink Port", PORT->PORTNUMBER, L4->KAMSESSION, Normcall);

		return;


	case PACTOR+DOWNLINK:

		PORT = L4->L4TARGET.PORT;

		if (PORT)
			sprintf(Buffer, "%s %d/%d", "Attached to Port", PORT->PORTNUMBER, L4->KAMSESSION);
		return;


	case L2LINK+UPLINK:

		LINK = L4->L4TARGET.LINK;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		if (LINK &&LINK->LINKPORT)
			sprintf(Buffer, "%s %d(%s)", "Uplink", LINK->LINKPORT->PORTNUMBER, Normcall);

		return;

	case L2LINK+DOWNLINK:

		LINK = L4->L4TARGET.LINK;

		if (LINK == NULL)
			return;

		ConvFromAX25(LINK->OURCALL, Normcall);
		strlop(Normcall, ' ');

		ConvFromAX25(LINK->LINKCALL, Normcall2);
		strlop(Normcall2, ' ');

		sprintf(Buffer, "%s %d(%s %s)", "Downlink", LINK->LINKPORT->PORTNUMBER, Normcall, Normcall2);
		return;

	case BPQHOST + UPLINK:
	case BPQHOST + DOWNLINK:

		// if the call has a Level 4 address display ALIAS:CALL, else just Call

		if (FindDestination(L4->L4USER, &DEST))
			Normcall[DecodeNodeName(DEST->DEST_CALL, Normcall)] = 0;		// null terminate
		else
			Normcall[ConvFromAX25(L4->L4USER, Normcall)] = 0;

		VEC = L4->L4TARGET.HOST;
		sprintf(Buffer, "%s%02d(%s)", "Host", (int)(VEC - BPQHOSTVECTOR) + 1, Normcall);
		return;

	case SESSION + DOWNLINK:
	case SESSION + UPLINK:

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		DEST = L4->L4TARGET.DEST;

		if (DEST == NULL)
			return;

		ConvFromAX25(DEST->DEST_CALL, Normcall2);
		strlop(Normcall2, ' ');

		memcpy(Alias, DEST->DEST_ALIAS, 6);
		strlop(Alias, ' ');

		sprintf(Buffer, "Circuit(%s:%s %s)", Alias, Normcall2, Normcall);

		return;
	}
}

VOID CheckForDetach(struct TNCINFO * TNC, int Stream, struct STREAMINFO * STREAM,
			VOID TidyCloseProc(), VOID ForcedCloseProc(), VOID CloseComplete())
{
	void ** buffptr;

	if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
	{
		// Node has disconnected - clear any connection

 		if (STREAM->Disconnecting)
		{
			// Already detected the detach, and have started to close

			STREAM->DisconnectingTimeout--;

			if (STREAM->DisconnectingTimeout)
				return;							// Give it a bit longer

			// Close has timed out - force a disc, and clear

			ForcedCloseProc(TNC, Stream);		// Send Tidy Disconnect

			goto NotConnected;
		}

		// New Disconnect

		Debugprintf("New Disconnect Port %d Q %x", TNC->Port, STREAM->BPQtoPACTOR_Q);

		if (STREAM->Connected || STREAM->Connecting)
		{
			char logmsg[120];
			time_t Duration;

			// Need to do a tidy close

			STREAM->Connecting = FALSE;
			STREAM->Disconnecting = TRUE;
			STREAM->DisconnectingTimeout = 300;			// 30 Secs

			if (Stream == 0)
				SetWindowText(TNC->xIDC_TNCSTATE, "Disconnecting");

			// Create a traffic record

			if (STREAM->Connected && STREAM->ConnectTime)
			{
				Duration = time(NULL) - STREAM->ConnectTime;

				if (Duration == 0)
					Duration = 1;				// Or will get divide by zero error 

				sprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
					TNC->Port, STREAM->RemoteCall,
					STREAM->BytesTXed, (int)(STREAM->BytesTXed/Duration),
					STREAM->BytesRXed, (int)(STREAM->BytesRXed/Duration), (int)Duration);

				Debugprintf(logmsg);

				STREAM->ConnectTime = 0;
			}

			if (STREAM->BPQtoPACTOR_Q)					// Still data to send?
				return;									// Will close when all acked

//			if (STREAM->FramesOutstanding && TNC->Hardware == H_UZ7HO)
//				return;									// Will close when all acked

			TidyCloseProc(TNC, Stream);					// Send Tidy Disconnect

			return;
		}

		// Not connected
NotConnected:

		STREAM->Disconnecting = FALSE;
		STREAM->Attached = FALSE;
		STREAM->Connecting = FALSE;
		STREAM->Connected = FALSE;

		if (Stream == 0)
			SetWindowText(TNC->xIDC_TNCSTATE, "Free");

		STREAM->FramesQueued = 0;
		STREAM->FramesOutstanding = 0;

		CloseComplete(TNC, Stream);

		if (TNC->DefaultRXFreq && TNC->RXRadio)
		{
			char Msg[128];

			sprintf(Msg, "R%d %f", TNC->RXRadio, TNC->DefaultRXFreq);
			Rig_Command( (TRANSPORTENTRY *) -1, Msg);
		}

		if (TNC->DefaultTXFreq && TNC->TXRadio && TNC->TXRadio != TNC->RXRadio)
		{
			char Msg[128];

			sprintf(Msg, "R%d %f", TNC->TXRadio, TNC->DefaultTXFreq);
			Rig_Command( (TRANSPORTENTRY *) -1, Msg);
		}

		while(STREAM->BPQtoPACTOR_Q)
		{
			buffptr=Q_REM(&STREAM->BPQtoPACTOR_Q);
			ReleaseBuffer(buffptr);
		}

		while(STREAM->PACTORtoBPQ_Q)
		{
			buffptr=Q_REM(&STREAM->PACTORtoBPQ_Q);
			ReleaseBuffer(buffptr);
		}
	}
}

char * CheckAppl(struct TNCINFO * TNC, char * Appl)
{
	APPLCALLS * APPL;
	BPQVECSTRUC * PORTVEC;
	int Allocated = 0, Available = 0;
	int App, Stream;
	struct TNCINFO * APPLTNC;

//	Debugprintf("Checking if %s is running", Appl);

	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (_memicmp(APPL->APPLCMD, Appl, 12) == 0)
		{
			int _APPLMASK = 1 << App;

			// If App has an alias, assume it is running , unless a CMS alias - then check CMS

			if (APPL->APPLHASALIAS)
			{
				if (_memicmp(APPL->APPLCMD, "RELAY ", 6) == 0)
					return APPL->APPLCALL_TEXT;			// Assume people using RELAY know what they are doing

				if (APPL->APPLPORT && (_memicmp(APPL->APPLCMD, "RMS ", 4) == 0))
				{
					APPLTNC = TNCInfo[APPL->APPLPORT];
					{
						if (APPLTNC)
						{
							if (APPLTNC->TCPInfo && !APPLTNC->TCPInfo->CMSOK && !APPLTNC->TCPInfo->FallbacktoRelay)
								return NULL;
						}
					}
				}
				return APPL->APPLCALL_TEXT;
			}

			// See if App is running

			PORTVEC = &BPQHOSTVECTOR[0];

			for (Stream = 0; Stream < 64; Stream++)
			{
				if (PORTVEC->HOSTAPPLMASK & _APPLMASK)
				{
					Allocated++;

					if (PORTVEC->HOSTSESSION == 0 && (PORTVEC->HOSTFLAGS & 3) == 0)
					{
						// Free and no outstanding report

						return APPL->APPLCALL_TEXT;		// Running
					}
				}
				PORTVEC++;
			}
		}
	}

	return NULL;			// Not Running
}

VOID SetApplPorts()
{
	// If any appl has an alias, get port number

	struct APPLCONFIG * App;
	APPLCALLS * APPL;

	char C[80];
	char Port[80];
	char Call[80];

	int i, n;

	App = &xxcfg.C_APPL[0];

	for (i=0; i < NumberofAppls; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLHASALIAS)
		{
			n = sscanf(App->CommandAlias, "%s %s %s", &C[0], &Port[0], &Call[0]);
			if (n == 3)
				APPL->APPLPORT = atoi(Port);
		}
		App++;
	}
}


char Modenames[19][10] = {"WINMOR", "SCS", "KAM", "AEA", "HAL", "TELNET", "TRK",
	"V4", "UZ7HO", "MPSK", "FLDIGI", "UIARQ", "ARDOP", "VARA",
	"SERIAL", "KISSHF", "WINRPR", "HSMODEM", "FREEDATA"};

BOOL ProcessIncommingConnect(struct TNCINFO * TNC, char * Call, int Stream, BOOL SENDCTEXT)
{
	return ProcessIncommingConnectEx(TNC, Call, Stream, SENDCTEXT, FALSE);
}

BOOL ProcessIncommingConnectEx(struct TNCINFO * TNC, char * Call, int Stream, BOOL SENDCTEXT, BOOL AllowTR)
{
	TRANSPORTENTRY * Session;
	int Index = 0;
	PMSGWITHLEN buffptr;
	int Totallen = 0;
	UCHAR * ptr;
	struct PORTCONTROL * PORT = TNC->PortRecord;
	

	// Stop Scanner

	if (Stream == 0 || TNC->Hardware == H_UZ7HO)
	{
		char Msg[80];

		sprintf(Msg, "%d SCANSTOP", TNC->Port);

		Rig_Command( (TRANSPORTENTRY *) -1, Msg);

		UpdateMH(TNC, Call, '+', 'I');
	}

	Session = L4TABLE;

	// Find a free Circuit Entry

	while (Index < MAXCIRCUITS)
	{
		if (Session->L4USER[0] == 0)
			break;

		Session++;
		Index++;
	}

	if (Index == MAXCIRCUITS)
		return FALSE;					// Tables Full

	memset(Session, 0, sizeof(TRANSPORTENTRY));

	memcpy(TNC->Streams[Stream].RemoteCall, Call, 9);	// Save Text Callsign

	if (AllowTR)
		ConvToAX25Ex(Call, Session->L4USER);				// Allow -T and -R SSID's for MPS
	else
		ConvToAX25(Call, Session->L4USER);
	ConvToAX25(MYNODECALL, Session->L4MYCALL);
	Session->CIRCUITINDEX = Index;
	Session->CIRCUITID = NEXTID;
	NEXTID++;
	if (NEXTID == 0) NEXTID++;		// Keep non-zero

	TNC->PortRecord->ATTACHEDSESSIONS[Stream] = Session;
	TNC->Streams[Stream].Attached = TRUE;

	Session->L4TARGET.EXTPORT = TNC->PortRecord;

	Session->L4CIRCUITTYPE = UPLINK+PACTOR;
	Session->L4WINDOW = L4DEFAULTWINDOW;
	Session->L4STATE = 5;
	Session->SESSIONT1 = L4T1;
	Session->SESSPACLEN = TNC->PortRecord->PORTCONTROL.PORTPACLEN;
	Session->KAMSESSION = Stream;

	TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

	if (LogAllConnects)
	{
		if (TNC->TargetCall[0])
			WriteConnectLog(Call, TNC->TargetCall, Modenames[TNC->Hardware - 1]);
		else
			WriteConnectLog(Call, MYNODECALL, Modenames[TNC->Hardware - 1]);
	}

	if (SENDCTEXT == 0)
		return TRUE;

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
	else
		return TRUE;

	while (Totallen > 0)
	{
		int sendLen = TNC->PortRecord->ATTACHEDSESSIONS[Stream]->SESSPACLEN;

		if (sendLen == 0)
			sendLen = 80;

		if (Totallen < sendLen)
			sendLen = Totallen;
			
		buffptr = (PMSGWITHLEN)GetBuff();
		if (buffptr == 0) return TRUE;			// No buffers

		buffptr->Len = sendLen;
		memcpy(&buffptr->Data[0], ptr, sendLen);
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		Totallen -= sendLen;
		ptr += sendLen;
	}
	return TRUE;
}

char * Config;
static char * ptr1, * ptr2;

BOOL ReadConfigFile(int Port, int ProcLine())
{
	char buf[256],errbuf[256];

	if (TNCInfo[Port])					// If restarting, free old config
		free(TNCInfo[Port]);

	TNCInfo[Port] = NULL;

	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		if (strlen(Config) == 0)
		{
			// Empty Config File - OK for most types

			struct TNCINFO * TNC = TNCInfo[Port] = zalloc(sizeof(struct TNCINFO));

			TNC->InitScript = malloc(2);
			TNC->InitScript[0] = 0;

			return TRUE;
		}

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1 + 1);
			buf[ptr2 - ptr1 + 1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error

			if (!ProcLine(buf, Port))
			{
				WritetoConsoleLocal("\n");
				WritetoConsoleLocal("Bad config record ");
				WritetoConsoleLocal(errbuf);
			}
		}
	}
	else
	{
		sprintf(buf," ** Error - No Configuration info in bpq32.cfg");
		WritetoConsoleLocal(buf);
	}

	return (TRUE);
}
int GetLine(char * buf)
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
VOID DigiToMultiplePorts(struct PORTCONTROL * PORTVEC, PMESSAGE Msg)
{
	USHORT Mask=PORTVEC->DIGIMASK;
	int i;

	for (i=1; i<=NUMBEROFPORTS; i++)
	{
		if (Mask & 1)
		{
			// Block includes the Msg Header (7/11 bytes), Len Does not!

			Msg->PORT = i;
			Send_AX((UCHAR *)&Msg, Msg->LENGTH - MSGHDDRLEN, i);
			Mask>>=1;
		}
	}
}

int CompareAlias(struct DEST_LIST ** a, struct DEST_LIST ** b)
{
	return memcmp(a[0]->DEST_ALIAS, b[0]->DEST_ALIAS, 6); 
	/* strcmp functions works exactly as expected from comparison function */
}


int CompareNode(struct DEST_LIST ** a, struct DEST_LIST ** b)
{
	return memcmp(a[0]->DEST_CALL, b[0]->DEST_CALL, 7);
}

DllExport int APIENTRY CountFramesQueuedOnStream(int Stream)
{
	BPQVECSTRUC * PORTVEC = &BPQHOSTVECTOR[Stream-1];		// API counts from 1
	TRANSPORTENTRY * L4 = PORTVEC->HOSTSESSION;

	int Count = 0;

	if (L4)
	{
		if (L4->L4CROSSLINK)		// CONNECTED?
			Count = CountFramesQueuedOnSession(L4->L4CROSSLINK);
		else
			Count = CountFramesQueuedOnSession(L4);
	}
	return Count;
}

DllExport int APIENTRY ChangeSessionCallsign(int Stream, unsigned char * AXCall)
{
	// Equivalent to "*** linked to" command

	memcpy(BPQHOSTVECTOR[Stream-1].HOSTSESSION->L4USER, AXCall, 7);
	return (0);
}

DllExport int APIENTRY ChangeSessionPaclen(int Stream, int Paclen)
{
	BPQHOSTVECTOR[Stream-1].HOSTSESSION->SESSPACLEN = Paclen;
	return (0);
}

DllExport int APIENTRY ChangeSessionIdletime(int Stream, int idletime)
{
	if (BPQHOSTVECTOR[Stream-1].HOSTSESSION)
		BPQHOSTVECTOR[Stream-1].HOSTSESSION->L4LIMIT = idletime;
	return (0);
}

DllExport int APIENTRY Get_APPLMASK(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLMASK;
}
DllExport int APIENTRY GetStreamPID(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].STREAMOWNER;
}

DllExport int APIENTRY GetApplFlags(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLFLAGS;
}

DllExport int APIENTRY GetApplNum(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLNUM;
}

DllExport int APIENTRY GetApplMask(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLMASK;
}

DllExport BOOL APIENTRY GetAllocationState(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTFLAGS & 0x80;
}

VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);

extern int InitDone;
extern int SemHeldByAPI;
extern char pgm[256];		// Uninitialised so per process
extern int BPQHOSTAPI();


VOID POSTSTATECHANGE(BPQVECSTRUC * SESS)
{
	//	Post a message if requested
#ifndef LINBPQ
	if (SESS->HOSTHANDLE)
		PostMessage(SESS->HOSTHANDLE, BPQMsg, SESS->HOSTSTREAM, 4);
#endif
	return;
}


DllExport int APIENTRY SessionControl(int stream, int command, int Mask)
{
	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return (0);

	SESS = &BPQHOSTVECTOR[stream];

	//	Send Session Control command (BPQHOST function 6)
	//;	CL=0 CONNECT USING APPL MASK IN DL
	//;	CL=1, CONNECT. CL=2 - DISCONNECT. CL=3 RETURN TO NODE

	if 	(command > 1)
	{
		// Disconnect

		if (SESS->HOSTSESSION == 0)
		{
			SESS->HOSTFLAGS |= 1;		// State Change
			POSTSTATECHANGE(SESS);
			return 0;					// NOT CONNECTED
		}

		if (command == 3)
			SESS->HOSTFLAGS |= 0x20;	// Set Stay

		SESS->HOSTFLAGS |= 0x40;		// SET 'DISC REQ' FLAG

		return 0;
	}

	// 0 or 1 - connect

	if (SESS->HOSTSESSION)				// ALREADY CONNECTED
	{
		SESS->HOSTFLAGS |= 1;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;
	}

	//	SET UP A SESSION FOR THE CONSOLE

	SESS->HOSTFLAGS |= 0x80;			// SET ALLOCATED BIT

	if (command == 1)					// Zero is mask supplied by caller
		Mask = SESS->HOSTAPPLMASK;		// SO WE GET CORRECT CALLSIGN

	L4 = SetupSessionFromHost(SESS, Mask);

	if (L4 == 0)						// tables Full
	{
		SESS->HOSTFLAGS |= 3;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;
	}

	SESS->HOSTSESSION = L4;
	L4->L4CIRCUITTYPE = BPQHOST | UPLINK;
 	L4->Secure_Session = AuthorisedProgram;	// Secure Host Session

	SESS->HOSTFLAGS |= 1;		// State Change
	POSTSTATECHANGE(SESS);
	return 0;					// ALREADY CONNECTED
}

int FindFreeStreamEx(int GetSem);

int FindFreeStreamNoSem()
{
	return FindFreeStreamEx(0);
}

DllExport int APIENTRY FindFreeStream()
{
	return FindFreeStreamEx(1);
}

int FindFreeStreamEx(int GetSem)
{
	int stream, n;
	BPQVECSTRUC * PORTVEC;

//	Returns number of first unused BPQHOST stream. If none available,
//	returns 255. See API function 13.

	// if init has not yet been run, wait.

	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == -1)			// Init failed
		exit(0);

	if (GetSem)
		GetSemaphore(&Semaphore, 9);

	stream = 0;
	n = 64;

	while (n--)
	{
		PORTVEC = &BPQHOSTVECTOR[stream++];
		if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
		{
			PORTVEC->STREAMOWNER=GetCurrentProcessId();
			PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
			memcpy(&PORTVEC->PgmName[0], pgm, 31);
			if (GetSem)
				FreeSemaphore(&Semaphore);
			return stream;
		}
	}

	if (GetSem)
		FreeSemaphore(&Semaphore);

	return 255;
}

DllExport int APIENTRY AllocateStream(int stream)
{
//	Allocate stream. If stream is already allocated, return nonzero.
//	Otherwise allocate stream, and return zero.

	BPQVECSTRUC * PORTVEC = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
	{
		PORTVEC->STREAMOWNER=GetCurrentProcessId();
		PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&PORTVEC->PgmName[0], pgm, 31);
		FreeSemaphore(&Semaphore);
		return 0;
	}

	return 1;				// Already allocated
}


DllExport int APIENTRY DeallocateStream(int stream)
{
	BPQVECSTRUC * PORTVEC;
	UINT * monbuff;
	BOOL GotSem = Semaphore.Flag;

//	Release stream.

	stream--;

	if (stream < 0 || stream > 63)
		return (0);

	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->STREAMOWNER=0;
	PORTVEC->PgmName[0] = 0;
	PORTVEC->HOSTAPPLFLAGS=0;
	PORTVEC->HOSTAPPLMASK=0;
	PORTVEC->HOSTHANDLE=0;

	// Clear Trace Queue

	if (PORTVEC->HOSTSESSION)
		SessionControl(stream + 1, 2, 0);

	if (GotSem == 0)
		GetSemaphore(&Semaphore, 0);

	while (PORTVEC->HOSTTRACEQ)
	{
		monbuff = Q_REM((void *)&PORTVEC->HOSTTRACEQ);
		ReleaseBuffer(monbuff);
	}

	if (GotSem == 0)
		FreeSemaphore(&Semaphore);

	PORTVEC->HOSTFLAGS &= 0x60;			// Clear Allocated. Must leave any DISC Pending bits

	return(0);
}
DllExport int APIENTRY SessionState(int stream, int * state, int * change)
{
	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough

	GetSemaphore(&Semaphore, 20);

	//	CX = 0 if stream disconnected or CX = 1 if stream connected
	//	DX = 0 if no change of state since last read, or DX = 1 if
	//	       the connected/disconnected state has changed since
	//	       last read (ie. delta-stream status).

	//	HOSTFLAGS = Bit 80 = Allocated
	//		  Bit 40 = Disc Request
	//		  Bit 20 = Stay Flag
	//		  Bit 02 and 01 State Change Bits

	if ((HOST->HOSTFLAGS & 3) == 0)
		// No Chaange
		*change = 0;
	else
		*change = 1;

	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;

	HOST->HOSTFLAGS &= 0xFC;		// Clear Change Bitd

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport int APIENTRY SessionStateNoAck(int stream, int * state)
{
	//	Get current Session State. Dont ACK any change
	//	See BPQHOST function 4

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough

	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;

	return 0;
}

DllExport int APIENTRY SendMsg(int stream, char * msg, int len)
{
	//	Send message to stream (BPQHOST Function 2)

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	PDATAMESSAGE MSG;

	Check_Timer();

	if (len > 256)
		return 0;						// IGNORE

	if (stream == 0)
	{
		// Send UNPROTO - SEND FRAME TO ALL RADIO PORTS

		//	COPY DATA TO A BUFFER IN OUR SEGMENTS - SIMPLFIES THINGS LATER

		if (QCOUNT < 50)
			return 0;					// Dont want to run out

		GetSemaphore(&Semaphore, 10);

		if ((MSG = GetBuff()) == 0)
		{
			FreeSemaphore(&Semaphore);
			return 0;
		}

		MSG->PID = 0xF0;				// Normal Data PID

		memcpy(&MSG->L2DATA[0], msg, len);
		MSG->LENGTH = (len + MSGHDDRLEN + 1);

		SENDUIMESSAGE(MSG);
		ReleaseBuffer(MSG);
		FreeSemaphore(&Semaphore);
		return 0;
	}

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;

	GetSemaphore(&Semaphore, 22);

	SESS->HOSTFLAGS |= 0x80;		// SET ALLOCATED BIT

	if (QCOUNT < 40)				// PLENTY FREE?
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	// Dont allow massive queues to form

	if (QCOUNT < 100)
	{
		int n = CountFramesQueuedOnStream(stream + 1);

		if (n > 100)
		{
			Debugprintf("Stream %d QCOUNT %d Q Len %d - discarding", stream, QCOUNT, n);
			FreeSemaphore(&Semaphore);
			return 1;
		}
	}

	if ((MSG = GetBuff()) == 0)
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	MSG->PID = 0xF0;				// Normal Data PID

	memcpy(&MSG->L2DATA[0], msg, len);
	MSG->LENGTH = len + MSGHDDRLEN + 1;

	//	IF CONNECTED, PASS MESSAGE TO TARGET CIRCUIT - FLOW CONTROL AND
	//	DELAYED DISC ONLY WORK ON ONE SIDE

	Partner = L4->L4CROSSLINK;

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	if (Partner && Partner->L4STATE > 4)	// Partner and link up
	{
		//	Connected

		Partner->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT
		C_Q_ADD(&Partner->L4TX_Q, MSG);
		PostDataAvailable(Partner);
	}
	else
		C_Q_ADD(&L4->L4RX_Q, MSG);

	FreeSemaphore(&Semaphore);
	return 0;
}
DllExport int APIENTRY SendRaw(int port, char * msg, int len)
{
	struct PORTCONTROL * PORT;
	MESSAGE * MSG;

	Check_Timer();

	//	Send Raw (KISS mode) frame to port (BPQHOST function 10)

	if (len > (MAXDATA - (MSGHDDRLEN + 8)))
		return 0;

	if (QCOUNT < 50)
		return 1;

	//	GET A BUFFER

	PORT = GetPortTableEntryFromSlot(port);

	if (PORT == 0)
		return 0;

	GetSemaphore(&Semaphore, 24);

	MSG = GetBuff();

	if (MSG == 0)
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	memcpy(MSG->DEST, msg, len);

	MSG->LENGTH = len + MSGHDDRLEN;

	if (PORT->PROTOCOL == 10)		 // PACTOR/WINMOR Style
	{
		//	Pactor Style. Probably will only be used for Tracker uneless we do APRS over V4 or WINMOR

		EXTPORTDATA * EXTPORT = (EXTPORTDATA *) PORT;

		C_Q_ADD(&EXTPORT->UI_Q,	MSG);

		FreeSemaphore(&Semaphore);
		return 0;
	}

	MSG->PORT = PORT->PORTNUMBER;

	PUT_ON_PORT_Q(PORT, MSG);

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport time_t APIENTRY GetRaw(int stream, char * msg, int * len, int * count)
{
	time_t Stamp;
	BPQVECSTRUC * SESS;
	PMESSAGE MSG;
	int Msglen;

	Check_Timer();

	*len = 0;
	*count = 0;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];

	GetSemaphore(&Semaphore, 26);

	if (SESS->HOSTTRACEQ == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	MSG = Q_REM((void *)&SESS->HOSTTRACEQ);

	Msglen = MSG->LENGTH;

	if (Msglen < 0 || Msglen > 350)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Stamp = MSG->Timestamp;

	memcpy(msg, MSG, Msglen);

	*len = Msglen;

	ReleaseBuffer(MSG);

	*count = C_Q_COUNT(&SESS->HOSTTRACEQ);
	FreeSemaphore(&Semaphore);

	return Stamp;
}

DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count )
{
//	Get message from stream. Returns length, and count of frames
//	still waiting to be collected. (BPQHOST function 3)
//	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
//		in CX.  BX returns the number of outstanding frames still to
//		be received (ie. after this one) or zero if no more frames
//		(ie. this is last one).
//

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	PDATAMESSAGE MSG;
	int Msglen;

	Check_Timer();

	*len = 0;
	*count = 0;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;


	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 25);

	if (L4 == 0 || L4->L4TX_Q == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	if(L4->L4CROSSLINK)
		L4->L4CROSSLINK->L4KILLTIMER = 0;

	MSG = Q_REM((void *)&L4->L4TX_Q);

	Msglen = MSG->LENGTH - (MSGHDDRLEN + 1);	// Dont want PID

	if (Msglen < 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	if (Msglen > 256)
		Msglen = 256;

	memcpy(msg, &MSG->L2DATA[0], Msglen);

	*len = Msglen;

	ReleaseBuffer(MSG);

	*count = C_Q_COUNT(&L4->L4TX_Q);
	FreeSemaphore(&Semaphore);

	return 0;
}


DllExport int APIENTRY RXCount(int stream)
{
//	Returns count of packets waiting on stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	return C_Q_COUNT(&L4->L4TX_Q);
}

DllExport int APIENTRY TXCount(int stream)
{
//	Returns number of packets on TX queue for stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	L4 = L4->L4CROSSLINK;

	if (L4 == 0)
		return 0;			// NOTHING ro Q on

	return (CountFramesQueuedOnSession(L4));
}

DllExport int APIENTRY MONCount(int stream)
{
//	Returns number of monitor frames available
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];

	return C_Q_COUNT(&SESS->HOSTTRACEQ);
}


DllExport int APIENTRY GetCallsign(int stream, char * callsign)
{
	//	Returns call connected on stream (BPQHOST function 8 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    ";
	UCHAR * AXCall = NULL;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 26);

	if (L4 == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			if (LINK)
				AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED

				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			if (EXTPORT)
				AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (L4->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				if (DEST)
					AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		if (AXCall)
			ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport int APIENTRY GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window)
{
	// Return the Secure Session Flag rather than not connected

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    ";
	UCHAR * AXCall;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 27);

	if (L4 == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	// Return the Secure Session Flag rather than not connected

	//		AL = Radio port on which channel is connected (or zero)
	//		AH = SESSION TYPE BITS
	//		EBX = L2 paclen for the radio port
	//		ECX = L2 maxframe for the radio port
	//		EDX = L4 window size (if L4 circuit, or zero) or -1 if not connected
	//		ES:DI = CALLSIGN

	*port = 0;
	*sesstype = 0;
	*paclen = 0;
	*maxframe = 0;
	*l4window = 0;
	if (L4->SESSPACLEN)
		*paclen = L4->SESSPACLEN;
	else
		*paclen = 256;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		*l4window = Partner->L4WINDOW;
		*sesstype = Partner->L4CIRCUITTYPE;

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			//	EXTRACT PORT AND MAXFRAME

			*port = LINK->LINKPORT->PORTNUMBER;
			*maxframe = LINK->LINKWINDOW;
			*l4window = 0;

			AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED

				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			*port = EXTPORT->PORTCONTROL.PORTNUMBER;
			AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (L4->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore(&Semaphore);

	if (Partner)
		return Partner->Secure_Session;

	return 0;
}


DllExport int APIENTRY SetAppl(int stream, int flags, int mask)
{
//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in EDX (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.


	BPQVECSTRUC * PORTVEC;
	stream--;

	if (stream < 0 || stream > 63)
		return (0);

	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->HOSTAPPLFLAGS = flags;
	PORTVEC->HOSTAPPLMASK = mask;

	// If either is non-zero, set allocated and Process. This gets round problem with
	// stations that don't call allocate stream

	if (flags || mask)
	{
		if ((PORTVEC->HOSTFLAGS & 128) == 0)	// Not allocated
		{
			PORTVEC->STREAMOWNER=GetCurrentProcessId();
			memcpy(&PORTVEC->PgmName[0], pgm, 31);
			PORTVEC->HOSTFLAGS = 128;				 // SET ALLOCATED BIT, clear others
		}
	}

	return (0);
}

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntry(int portslot)		// Kept for Legacy apps
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

// Proc below renamed to avoid confusion with GetPortTableEntryFromPortNum

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

int CanPortDigi(int Port)
{
	struct PORTCONTROL * PORTVEC = GetPortTableEntryFromPortNum(Port);
	struct TNCINFO * TNC;

	if (PORTVEC == NULL)
		return FALSE;

	TNC = PORTVEC->TNC;

	if (TNC == NULL)
		return TRUE;

	if (TNC->Hardware == H_SCS || TNC->Hardware == H_TRK || TNC->Hardware == H_TRKM || TNC->Hardware == H_WINRPR)
		return FALSE;

	return TRUE;
}

struct PORTCONTROL * APIENTRY GetPortTableEntryFromPortNum(int portnum)
{
	struct PORTCONTROL * PORTVEC = PORTTABLE;

	do
	{
		if (PORTVEC->PORTNUMBER == portnum)
			return PORTVEC;

		PORTVEC=PORTVEC->PORTPOINTER;
	}
	while (PORTVEC);

	return NULL;
}

DllExport UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	memcpy(Desc, PORTVEC->PORTDESCRIPTION, 30);
	Desc[30]=0;

	return 0;
}

// Standard serial port handling routines, used by lots of modules.

int OpenCOMMPort(struct TNCINFO * conn, char * Port, int Speed, BOOL Quiet)
{
	if (conn->WEB_COMMSSTATE == NULL)
		conn->WEB_COMMSSTATE = zalloc(100);

	if (Port == NULL)
		return (FALSE);

	conn->hDevice = OpenCOMPort(Port, Speed, TRUE, TRUE, Quiet, 0);

	if (conn->hDevice == 0)
	{
		sprintf(conn->WEB_COMMSSTATE,"%s Open failed - Error %d", Port, GetLastError());
		if (conn->xIDC_COMMSSTATE)
			SetWindowText(conn->xIDC_COMMSSTATE, conn->WEB_COMMSSTATE);

		return (FALSE);
	}

	sprintf(conn->WEB_COMMSSTATE,"%s Open", Port);
	
	if (conn->xIDC_COMMSSTATE)
		SetWindowText(conn->xIDC_COMMSSTATE, conn->WEB_COMMSSTATE);

	return TRUE;
}



#ifdef WIN32

HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	int	Err;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	// if Port Name starts COM, convert to \\.\COM or ports above 10 wont work

	if (_memicmp(pPort, "COM", 3) == 0)
	{
		char * pp = (char *)pPort;
		int p = atoi(&pp[3]);
		sprintf( szPort, "\\\\.\\COM%d", p);
	}
	else
		strcpy(szPort, pPort);

	// open COMM device

	fd = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL );

	if (fd == (HANDLE) -1)
	{
		if (Quiet == 0)
		{
			Debugprintf("%s could not be opened %d", pPort, GetLastError());
		}
		return (FALSE);
	}

	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096 ) ;

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500 ;
	SetCommTimeouts(fd, &CommTimeOuts ) ;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState(fd, &dcb ) ;

   dcb.BaudRate = speed;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = TWOSTOPBITS;
   dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE ;

	// setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

   fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		if (SetDTR)
			EscapeCommFunction(fd, SETDTR);
		else
			EscapeCommFunction(fd, CLRDTR);
	
		if (SetRTS)
			EscapeCommFunction(fd, SETRTS);
		else
			EscapeCommFunction(fd, CLRRTS);
	}
	else
	{
		sprintf(buf,"%s Setup Failed %d ", pPort, GetLastError());

		WritetoConsoleLocal(buf);
		OutputDebugString(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;

}

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL Error;
	return ReadCOMBlockEx(fd, Block, MaxLength, &Error);
}

// version to pass read error back to caller

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error)
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	BOOL	ret;

	if (fd == NULL)
		return 0;

	// only try to read number of bytes in queue

	ret = ClearCommError(fd, &dwErrorFlags, &ComStat);

	if (ret == 0)
	{
		int Err = GetLastError();
		*Error = TRUE;
		return 0;
	}


	dwLength = min((DWORD) MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL) ;

		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(fd, &dwErrorFlags, &ComStat ) ;
		}
	}

	*Error = FALSE;

   return dwLength;
}


BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;
	DWORD Mask = 0;
	int Err;

	Err = GetCommModemStatus(fd, &Mask);

	if ((Mask & MS_CTS_ON) == 0)		// trap com0com other end not open
		return TRUE;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
	                       &BytesWritten, NULL );

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
	{
		int Err = GetLastError();
		ClearCommError(fd, &ErrorFlags, &ComStat);
		return FALSE;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	if (fd == NULL)
		return;

	SetCommMask(fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	CloseHandle(fd);
	fd = NULL;
}


VOID COMSetDTR(HANDLE fd)
{
	EscapeCommFunction(fd, SETDTR);
}

VOID COMClearDTR(HANDLE fd)
{
	EscapeCommFunction(fd, CLRDTR);
}

VOID COMSetRTS(HANDLE fd)
{
	EscapeCommFunction(fd, SETRTS);
}

VOID COMClearRTS(HANDLE fd)
{
	EscapeCommFunction(fd, CLRRTS);
}


#else

static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};


HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char Port[256];
	char buf[100];

	//	Linux Version.

	int fd;
	int hwflag = 0;
	u_long param=1;
	struct termios term;
	struct speed_struct *s;

	if ((UINT)pPort < 256)
		sprintf(Port, "%s/com%d", BPQDirectory, (int)pPort);
	else
		strcpy(Port, pPort);

	if ((fd = open(Port, O_RDWR | O_NDELAY)) == -1)
	{
		if (Quiet == 0)
		{
			perror("Com Open Failed");
			sprintf(buf," %s could not be opened \n", Port);
			WritetoConsoleLocal(buf);
			Debugprintf(buf);
		}
		return 0;
	}

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)
		if (s->user_speed == speed)
			break;

   if (s->user_speed == -1)
   {
	   fprintf(stderr, "tty_speed: invalid speed %d\n", speed);
	   return FALSE;
   }

   if (tcgetattr(fd, &term) == -1)
   {
	   perror("tty_speed: tcgetattr");
	   return FALSE;
   }

   	cfmakeraw(&term);
	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	Debugprintf("LinBPQ Port %s fd %d", Port, fd);

	if (SetDTR)
	{
		COMSetDTR(fd);
	}
	else
	{
		COMClearDTR(fd);
	}

	if (SetRTS)
	{
		COMSetRTS(fd);
	}
	else
	{
		COMClearRTS(fd);
	}
	return fd;
}

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL Error;
	return ReadCOMBlockEx(fd, Block, MaxLength, &Error);
}

// version to pass read error back to caller

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error)
{
	int Length;

	if (fd == 0)
	{
		*Error = 1;
		return 0;
	}

	errno = 22222;		// to catch zero read (?? file closed ??)

	Length = read(fd, Block, MaxLength);

	*Error = 0;

	if (Length == 0 && errno == 22222)	// seems to be result of unpluging USB
	{
//		printf("KISS read returned zero len and no errno\n");
		*Error = 1;
		return 0;
	}

	if (Length < 0)
	{
		if (errno != 11 && errno != 35)					// Would Block
		{
			perror("read");
			printf("Handle %d Errno %d Len %d\n", fd, errno, Length);
			*Error = errno;
		}
		return 0;
	}

	return Length;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	//	Some systems seem to have a very small max write size
	
	int ToSend = BytesToWrite;
	int Sent = 0, ret;

	while (ToSend)
	{
		ret = write(fd, &Block[Sent], ToSend);

		if (ret >= ToSend)
			return TRUE;

		if (ret == -1)
		{
			if (errno != 11 && errno != 35)					// Would Block
				return FALSE;
	
			usleep(10000);
			ret = 0;
		}
						
		Sent += ret;
		ToSend -= ret;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	if (fd == 0)
		return;

	close(fd);
	fd = 0;
}

VOID COMSetDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMClearDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMSetRTS(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMClearRTS(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}

#endif


int MaxNodes;
int MaxRoutes;
int NodeLen;
int RouteLen;
struct DEST_LIST * Dests;
struct ROUTE * Routes;

FILE *file;

int DoRoutes()
{
	char digis[30] = "";
	int count, len;
	char Normcall[10], Portcall[10];
	char line[80];

	for (count=0; count<MaxRoutes; count++)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
		{
			len=ConvFromAX25(Routes->NEIGHBOUR_CALL,Normcall);
			Normcall[len]=0;

			if (Routes->NEIGHBOUR_DIGI1[0] != 0)
			{
				memcpy(digis," VIA ",5);

				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
				Portcall[len]=0;
				strcpy(&digis[5],Portcall);

				if (Routes->NEIGHBOUR_DIGI2[0] != 0)
				{
					len=ConvFromAX25(Routes->NEIGHBOUR_DIGI2,Portcall);
					Portcall[len]=0;
					strcat(digis," ");
					strcat(digis,Portcall);
				}
			}
			else
				digis[0] = 0;

			len=sprintf(line,
					"ROUTE ADD %s %d %d %s %d %d %d %d %d\n",
					Normcall,
					Routes->NEIGHBOUR_PORT,
					Routes->NEIGHBOUR_QUAL, digis,
					Routes->NBOUR_MAXFRAME,
					Routes->NBOUR_FRACK,
					Routes->NBOUR_PACLEN,
					Routes->INP3Node | (Routes->NoKeepAlive << 2),
					Routes->OtherendsRouteQual);

					fputs(line, file);
		}

		Routes+=1;
	}

	return (0);
}

int DoNodes()
{
	int count, len, cursor, i;
	char Normcall[10], Portcall[10];
	char line[80];
	char Alias[7];

	Dests-=1;

	for (count=0; count<MaxNodes; count++)
	{
		Dests+=1;

		if (Dests->NRROUTE[0].ROUT_NEIGHBOUR == 0)
			continue;

		{
			len=ConvFromAX25(Dests->DEST_CALL,Normcall);
			Normcall[len]=0;

			memcpy(Alias,Dests->DEST_ALIAS,6);

			Alias[6]=0;

			for (i=0;i<6;i++)
			{
				if (Alias[i] == ' ')
					Alias[i] = 0;
			}

			cursor=sprintf(line,"NODE ADD %s:%s ", Alias,Normcall);

			if (Dests->NRROUTE[0].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[0].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[0].ROUT_QUALITY);

				cursor+=len;

				if (Dests->NRROUTE[0].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

			if (Dests->NRROUTE[1].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[1].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[1].ROUT_QUALITY);

				cursor+=len;

				if (Dests->NRROUTE[1].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

		if (Dests->NRROUTE[2].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[2].ROUT_NEIGHBOUR->INP3Node == 0)
		{
			len=ConvFromAX25(
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;

			len=sprintf(&line[cursor],"%s %d %d ",
				Portcall,
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->NRROUTE[2].ROUT_QUALITY);

			cursor+=len;

			if (Dests->NRROUTE[2].ROUT_OBSCOUNT > 127)
			{
				len=sprintf(&line[cursor],"! ");
				cursor+=len;
			}
		}

		if (cursor > 30)
		{
			line[cursor++]='\n';
			line[cursor++]=0;
			fputs(line, file);
		}
		}
	}
	return (0);
}

void SaveMH()
{
	char FN[250];
	struct PORTCONTROL * PORT = PORTTABLE;
	FILE *file;
	
	if (BPQDirectory[0] == 0)
	{
		strcpy(FN, "MHSave.txt");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"MHSave.txt");
	}

	if ((file = fopen(FN, "w")) == NULL)
		return;

	while (PORT)
	{	
		int Port = 0;
		char * ptr;
	
		MHSTRUC * MH = PORT->PORTMHEARD;

		int count = MHENTRIES;
		int n;
		char Normcall[20];
		char From[10];
		char DigiList[100];
		char * Output;
		int len;
		char Digi = 0;


		// Note that the MHDIGIS field may contain rubbish. You have to check End of Address bit to find
		// how many digis there are
	
		if (MH == NULL)
			continue;
		
		fprintf(file, "Port:%d\n", PORT->PORTNUMBER);
	
		while (count--)
		{
			if (MH->MHCALL[0] == 0)
				break;

			Digi = 0;
		
			len = ConvFromAX25(MH->MHCALL, Normcall);
			Normcall[len] = 0;

			n = 8;					// Max number of digi-peaters

			ptr = &MH->MHCALL[6];	// End of Address bit

			Output = &DigiList[0];

			if ((*ptr & 1) == 0)
			{
				// at least one digi

				strcpy(Output, "via ");
				Output += 4;
		
				while ((*ptr & 1) == 0)
				{
					//	MORE TO COME
	
					From[ConvFromAX25(ptr + 1, From)] = 0;
					Output += sprintf((char *)Output, "%s", From);
	
					ptr += 7;
					n--;

					if (n == 0)
						break;

					// See if digi actioned - put a * on last actioned

					if (*ptr & 0x80)
					{
						if (*ptr & 1)						// if last address, must need *
						{
							*(Output++) = '*';
							Digi = '*';
						}

						else
							if ((ptr[7] & 0x80) == 0)		// Repeased by next?
							{
								*(Output++) = '*';			// No, so need *
								Digi = '*';
							}
					

					}
					*(Output++) = ',';
				}		
				*(--Output) = 0;							// remove last comma
			}
			else 
				*(Output) = 0;

			// if we used a digi set * on call and display via string


			if (Digi)
				Normcall[len++] = Digi;
			else
				DigiList[0] = 0;	// Dont show list if not used

			Normcall[len++] = 0;

			ptr = FormatMH(MH, 'U');

			ptr[15] = 0;
		
			if (MH->MHDIGI)
				fprintf(file, "%d %6d %-10s%c %s %s|%s|%s\n", (int)MH->MHTIME, MH->MHCOUNT, Normcall, MH->MHDIGI, ptr, DigiList, MH->MHLocator, MH->MHFreq);
			else
				fprintf(file, "%d %6d %-10s%c %s %s|%s|%s\n", (int)MH->MHTIME, MH->MHCOUNT, Normcall, ' ', ptr, DigiList, MH->MHLocator, MH->MHFreq);

			MH++;
		}
		PORT = PORT->PORTPOINTER;
	}

	fclose(file);

	return;
}


int APIENTRY SaveNodes ()
{
	char FN[250];

	Routes = NEIGHBOURS;
	RouteLen = ROUTE_LEN;
	MaxRoutes = MAXNEIGHBOURS;

	Dests = DESTS;
	NodeLen = DEST_LIST_LEN;
	MaxNodes = MAXDESTS;

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"BPQNODES.dat");
	}

	if ((file = fopen(FN, "w")) == NULL)
		return FALSE;

	DoRoutes();
	DoNodes();

	fclose(file);

	return (0);
}

DllExport int APIENTRY ClearNodes ()
{
	char FN[250];

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"BPQNODES.dat");
	}

	if ((file = fopen(FN, "w")) == NULL)
		return FALSE;

	fclose(file);

	return (0);
}
char * FormatUptime(int Uptime)
 {
	struct tm * TM;
	static char UPTime[50];
	time_t szClock = Uptime * 60;

	TM = gmtime(&szClock);

	sprintf(UPTime, "Uptime (Days Hours Mins)     %.2d:%.2d:%.2d\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min);

	return UPTime;
 }

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


char * FormatMH(PMHSTRUC MH, char Format)
{
	struct tm * TM;
	static char MHTime[50];
	time_t szClock;
	char LOC[7];

	memcpy(LOC, MH->MHLocator, 6);
	LOC[6] = 0;

	if (Format == 'U' || Format =='L')
		szClock = MH->MHTIME;
	else
		szClock = time(NULL) - MH->MHTIME;

	if (Format == 'L')
		TM = localtime(&szClock);
	else
		TM = gmtime(&szClock);

	if (Format == 'U' || Format =='L')
		sprintf(MHTime, "%s %02d %.2d:%.2d:%.2d  %s %s",
			month[TM->tm_mon], TM->tm_mday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);
	else
		sprintf(MHTime, "%.2d:%.2d:%.2d:%.2d  %s %s",
			TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);

	return MHTime;

}


Dll VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset)
{
	// Create a time dependent One Time Password from the KeyPhrase
	// TimeOffset is used when checking to allow for slight variation in clocks

	time_t NOW = time(NULL);
	UCHAR Hash[16];
	char Key[1000];
	int i, chr;

	NOW = NOW/30 + TimeOffset;				// Only Change every 30 secs

	sprintf(Key, "%s%x", KeyPhrase, (int)NOW);

	md5(Key, Hash);

	for (i=0; i<16; i++)
	{
		chr = (Hash[i] & 31);
		if (chr > 9) chr += 7;

		Password[i] = chr + 48;
	}

	Password[16] = 0;
	return;
}

Dll BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase)
{
	char CheckPassword[17];
	int Offsets[10] = {0, -1, 1, -2, 2, -3, 3, -4, 4};
	int i, Pass;

	if (strlen(Password) < 16)
		Pass = atoi(Password);

	for (i = 0; i < 9; i++)
	{
		CreateOneTimePassword(CheckPassword, KeyPhrase, Offsets[i]);

		if (strlen(Password) < 16)
		{
			// Using a numeric extract

			long long Val;

			memcpy(&Val, CheckPassword, 8);
			Val = Val %= 1000000;

			if (Pass == Val)
				return TRUE;
		}
		else
			if (memcmp(Password, CheckPassword, 16) == 0)
				return TRUE;
	}

	return FALSE;
}


DllExport BOOL ConvToAX25Ex(unsigned char * callsign, unsigned char * ax25call)
{
	// Allows SSID's of 'T and 'R'
	
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			
			if (callsign[i+1] == 'T')
			{
				ax25call[6]=0x42;
				return TRUE;
			}

			if (callsign[i+1] == 'R')
			{
				ax25call[6]=0x44;
				return TRUE;
			}
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ' || callsign[i] == ',')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}

		ax25call[i] = callsign[i] << 1;
	}

	//
	//	Too many chars
	//

	return (FALSE);
}


DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call)
{
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ' || callsign[i] == ',')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}

		ax25call[i] = callsign[i] << 1;
	}

	//
	//	Too many chars
	//

	return (FALSE);
}


DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall)
{
	int in,out=0;
	unsigned char chr;

	memset(outcall,0x20,10);

	for (in=0;in<6;in++)
	{
		chr=incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr=incall[6];				// ssid

	if (chr == 0x42)
	{
		outcall[out++]='-';
		outcall[out++]='T';
		return out;
	}

	if (chr == 0x44)
	{
		outcall[out++]='-';
		outcall[out++]='R';
		return out;
	}

	chr >>= 1;
	chr	&= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}
	return (out);
}

unsigned short int compute_crc(unsigned char *buf, int txlen);

SOCKADDR_IN reportdest = {0};

SOCKET ReportSocket = 0;

SOCKADDR_IN Chatreportdest = {0};

extern char LOCATOR[];			// Locator for Reporting - may be Maidenhead or LAT:LON
extern char MAPCOMMENT[];		// Locator for Reporting - may be Maidenhead or LAT:LON
extern char LOC[7];				// Maidenhead Locator for Reporting
extern char ReportDest[7];


VOID SendReportMsg(char * buff, int txlen)
{
 	unsigned short int crc = compute_crc(buff, txlen);

	crc ^= 0xffff;

	buff[txlen++] = (crc&0xff);
	buff[txlen++] = (crc>>8);

	sendto(ReportSocket, buff, txlen, 0, (struct sockaddr *)&reportdest, sizeof(reportdest));

}
VOID SendLocation()
{
	MESSAGE AXMSG = {0};
	PMESSAGE AXPTR = &AXMSG;
	char Msg[512];
	int Len;

	Len = sprintf(Msg, "%s %s<br>%s", LOCATOR, VersionString, MAPCOMMENT);

#ifdef LINBPQ
	Len = sprintf(Msg, "%s L%s<br>%s", LOCATOR, VersionString, MAPCOMMENT);
#endif
#ifdef MACBPQ
	Len = sprintf(Msg, "%s M%s<br>%s", LOCATOR, VersionString, MAPCOMMENT);
#endif
#ifdef FREEBSD
	Len = sprintf(Msg, "%s F%s<br>%s", LOCATOR, VersionString, MAPCOMMENT);
#endif

	if (Len > 256)
		Len = 256;

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendReportMsg((char *)&AXMSG.DEST, Len + 16);

	printf("M0LTEMap %d\n", M0LTEMap);

	if (M0LTEMap)
		SendDataToPktMap("");

	return;

}




VOID SendMH(struct TNCINFO * TNC, char * call, char * freq, char * LOC, char * Mode)
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[100];
	int Len;

	if (ReportSocket == 0 || LOCATOR[0] == 0)
		return;

	Len = sprintf(Msg, "MH %s,%s,%s,%s", call, freq, LOC, Mode);

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	if (TNC->PortRecord->PORTCONTROL.PORTCALL[0])
		memcpy(AXPTR->ORIGIN, TNC->PortRecord->PORTCONTROL.PORTCALL, 7);
	else
		memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;

	return;

}

time_t TimeLastNRRouteSent = 0;

char NRRouteMessage[256];
int NRRouteLen = 0;


VOID SendNETROMRoute(struct PORTCONTROL * PORT, unsigned char * axcall)
{
	//	Called to update Link Map when a NODES Broadcast is received
	//  Batch to reduce Load

	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[300];
	int Len;
	char Call[10];
	char Report[16];
	time_t Now = time(NULL);
	int NeedSend = FALSE;


	if (ReportSocket == 0 || LOCATOR[0] == 0)
		return;

	Call[ConvFromAX25(axcall, Call)] = 0;

	sprintf(Report, "%s,%d,", Call, PORT->PORTTYPE);

	if (Now - TimeLastNRRouteSent > 60)
		NeedSend = TRUE;
	
	if (strstr(NRRouteMessage, Report) == 0)	//  reported recently
		strcat(NRRouteMessage, Report);
		
	if (strlen(NRRouteMessage) > 230 || NeedSend)
	{
		Len = sprintf(Msg, "LINK %s", NRRouteMessage);

		// Block includes the Msg Header (7 bytes), Len Does not!

		memcpy(AXPTR->DEST, ReportDest, 7);
		memcpy(AXPTR->ORIGIN, MYCALL, 7);
		AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
		AXPTR->DEST[6] |= 0x80;			// set Command Bit

		AXPTR->ORIGIN[6] |= 1;			// Set End of Call
		AXPTR->CTL = 3;		//UI
		AXPTR->PID = 0xf0;
		memcpy(AXPTR->L2DATA, Msg, Len);

		SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;

		TimeLastNRRouteSent = Now;
		NRRouteMessage[0] = 0;
	}

	return;

}

DllExport char * APIENTRY GetApplCall(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCALL_TEXT);
}
DllExport char * APIENTRY GetApplAlias(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLALIAS_TEXT);
}

DllExport int32_t APIENTRY GetApplQual(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return 0;

	return (APPLCALLTABLE[Appl-1].APPLQUAL);
}

char * GetApplCallFromName(char * App)
{
	int i;
	char PaddedAppl[13] = "            ";

	memcpy(PaddedAppl, App, (int)strlen(App));

	for (i = 0; i < NumberofAppls; i++)
	{
		if (memcmp(&APPLCALLTABLE[i].APPLCMD, PaddedAppl, 12) == 0)
			return &APPLCALLTABLE[i].APPLCALL_TEXT[0];
	}
	return NULL;
}


DllExport char * APIENTRY GetApplName(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCMD);
}

DllExport int APIENTRY GetNumberofPorts()
{
	return (NUMBEROFPORTS);
}

DllExport int APIENTRY GetPortNumber(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC->PORTNUMBER;

}

DllExport char * APIENTRY GetVersionString()
{
//	return ((char *)&VersionStringWithBuild);
	return ((char *)&VersionString);
}

#ifdef MACBPQ

//Fiddle till I find a better solution

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060
int __sync_lock_test_and_set(int * ptr, int val)
{
	*ptr = val;
	return 0;
}
#endif // __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MACBPQ



void GetSemaphore(struct SEM * Semaphore, int ID)
{
	//
	//	Wait for it to be free
	//

	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
	}

loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

#ifdef WIN32

	{
		if (InterlockedExchange(&Semaphore->Flag, 1) != 0) // Failed to get it
			goto loop1;		// try again;;
	}

#else

	if (__sync_lock_test_and_set(&Semaphore->Flag, 1) != 0)

		// Failed to get it
		goto loop1;		// try again;

#endif

	//Ok. got it

	Semaphore->Gets++;
	Semaphore->SemProcessID = GetCurrentProcessId();
	Semaphore->SemThreadID = GetCurrentThreadId();
	SemHeldByAPI = ID;

	return;
}

void FreeSemaphore(struct SEM * Semaphore)
{
	if (Semaphore->Flag == 0)
		Debugprintf("Free Semaphore Called when Sem not held");

	Semaphore->Rels++;
	Semaphore->Flag = 0;

	return;
}

#ifdef WIN32

#include "DbgHelp.h"
/*
USHORT WINAPI RtlCaptureStackBackTrace(
  __in       ULONG FramesToSkip,
  __in       ULONG FramesToCapture,
  __out      PVOID *BackTrace,
  __out_opt  PULONG BackTraceHash
);
*/
#endif

void printStack(void)
{
#ifdef WIN32
#ifdef _DEBUG					// So we can use on 98/2K

     unsigned int   i;
     void         * stack[ 100 ];
     unsigned short frames;
     SYMBOL_INFO  * symbol;
     HANDLE         process;

	 Debugprintf("Stack Backtrace");

     process = GetCurrentProcess();

     SymInitialize( process, NULL, TRUE );

     frames               = RtlCaptureStackBackTrace( 0, 60, stack, NULL );
     symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
     symbol->MaxNameLen   = 255;
     symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

     for( i = 0; i < frames; i++ )
     {
         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

         Debugprintf( "%i: %s - %p", frames - i - 1, symbol->Name, symbol->Address );
     }

     free(symbol);

#endif
#endif
}

pthread_t ResolveUpdateThreadId = 0;

char NodeMapServer[80] = "update.g8bpq.net";
char ChatMapServer[80] = "chatupdate.g8bpq.net";

VOID ResolveUpdateThread(void * Unused)
{
	struct hostent * HostEnt1;
	struct hostent * HostEnt2;

	ResolveUpdateThreadId = GetCurrentThreadId();

	while (TRUE)
	{
		if (pthread_equal(ResolveUpdateThreadId, GetCurrentThreadId()) == FALSE)
		{
			Debugprintf("Resolve Update thread %x redundant - closing", GetCurrentThreadId());
			return;
		}

		//	Resolve name to address

		Debugprintf("Resolving %s", NodeMapServer);
		HostEnt1 = gethostbyname (NodeMapServer);
//		HostEnt1 = gethostbyname ("192.168.1.64");

		if (HostEnt1)
			memcpy(&reportdest.sin_addr.s_addr,HostEnt1->h_addr,4);

		Debugprintf("Resolving %s", ChatMapServer);
		HostEnt2 = gethostbyname (ChatMapServer);
//		HostEnt2 = gethostbyname ("192.168.1.64");

		if (HostEnt2)
			memcpy(&Chatreportdest.sin_addr.s_addr,HostEnt2->h_addr,4);

		if (HostEnt1 && HostEnt2)
		{
			Sleep(1000 * 60 * 30);
			continue;
		}

		Debugprintf("Resolve Failed for update.g8bpq.net or chatmap.g8bpq.net");
		Sleep(1000 * 60 * 5);
	}
}


VOID OpenReportingSockets()
{
	u_long param=1;
	BOOL bcopt=TRUE;

	if (LOCATOR[0])
	{
		// Enable Node Map Reports

		ReportTimer = 600;

		ReportSocket = socket(AF_INET,SOCK_DGRAM,0);

		if (ReportSocket == INVALID_SOCKET)
		{
			Debugprintf("Failed to create Reporting socket");
			ReportSocket = 0;
  		 	return;
		}

		ioctlsocket (ReportSocket, FIONBIO, &param);
		setsockopt (ReportSocket, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt,4);

		reportdest.sin_family = AF_INET;
		reportdest.sin_port = htons(81);
		ConvToAX25("DUMMY-1", ReportDest);
	}

	// Set up Chat Report even if no LOCATOR	reportdest.sin_family = AF_INET;
	// Socket must be opened in MailChat Process

	Chatreportdest.sin_family = AF_INET;
	Chatreportdest.sin_port = htons(81);

	_beginthread(ResolveUpdateThread, 0, NULL);
}

VOID WriteMiniDumpThread();

time_t lastMiniDump = 0;

void WriteMiniDump()
{
#ifdef WIN32

	_beginthread(WriteMiniDumpThread, 0, 0);
	Sleep(3000);
}

VOID WriteMiniDumpThread()
{
	HANDLE hFile;
	BOOL ret;
	char FN[256];
	struct tm * TM;
	time_t Now = time(NULL);

	if (lastMiniDump == Now)		// Not more than one per second
	{
		Debugprintf("minidump suppressed");
		return;
	}

	lastMiniDump = Now;

	TM = gmtime(&Now);

	sprintf(FN, "%s/Logs/MiniDump%d%02d%02d%02d%02d%02d.dmp", BPQDirectory,
		TM->tm_year + 1900, TM->tm_mon +1, TM->tm_mday, TM->tm_hour, TM->tm_min, TM->tm_sec);

	hFile = CreateFile(FN, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		// Create the minidump

		ret = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, MiniDumpNormal, 0, 0, 0 );

		if(!ret)
			Debugprintf("MiniDumpWriteDump failed. Error: %u", GetLastError());
		else
			Debugprintf("Minidump %s created.", FN);
			CloseHandle(hFile);
	}
#endif
}

// UI Util Code

#pragma pack(1)

typedef struct _MESSAGEX
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGEX * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;
	UCHAR	DATA[256];
	UCHAR	PADDING[56];			// In case he have Digis

}MESSAGEX, *PMESSAGEX;

#pragma pack()


int PortNum[MaxBPQPortNo + 1] = {0};	// Tab nunber to port

char * UIUIDigi[MaxBPQPortNo + 1]= {0};
char * UIUIDigiAX[MaxBPQPortNo + 1] = {0};		// ax.25 version of digistring
int UIUIDigiLen[MaxBPQPortNo + 1] = {0};			// Length of AX string

char UIUIDEST[MaxBPQPortNo + 1][11] = {0};		// Dest for Beacons

char UIAXDEST[MaxBPQPortNo + 1][7] = {0};


UCHAR FN[MaxBPQPortNo + 1][256];			// Filename
int Interval[MaxBPQPortNo + 1];			// Beacon Interval (Mins)
int MinCounter[MaxBPQPortNo + 1];			// Interval Countdown

BOOL SendFromFile[MaxBPQPortNo + 1];
char Message[MaxBPQPortNo + 1][1000];		// Beacon Text

VOID SendUIBeacon(int Port);

BOOL RunUI = TRUE;

VOID UIThread(void * Unused)
{
	int Port, MaxPorts = GetNumberofPorts();

	Sleep(60000);

	while (RunUI)
	{
		int sleepInterval = 60000;

		for (Port = 1; Port <= MaxPorts; Port++)
		{
			if (MinCounter[Port])
			{
				MinCounter[Port]--;

				if (MinCounter[Port] == 0)
				{
					MinCounter[Port] = Interval[Port];
					SendUIBeacon(Port);

					// pause beteen beacons but adjust sleep interval to suit

					Sleep(10000);
					sleepInterval -= 10000;
				}
			}
		}

		while (sleepInterval <= 0)		// just in case we have a crazy config
			sleepInterval += 60000;

		Sleep(sleepInterval);
	}
}

int UIRemoveLF(char * Message, int len)
{
	// Remove lf chars

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;
	}

	return (int)(ptr2 - Message);
}




VOID UISend_AX_Datagram(UCHAR * Msg, DWORD Len, UCHAR Port, UCHAR * HWADDR, BOOL Queue)
{
	MESSAGEX AXMSG;
	PMESSAGEX AXPTR = &AXMSG;
	int DataLen = Len;
	struct PORTCONTROL * PORT = GetPortTableEntryFromSlot(Port);

	// Block includes the Msg Header (7 or 11 bytes), Len Does not!

	memcpy(AXPTR->DEST, HWADDR, 7);

	// Get BCALL or PORTCALL if set

	if (PORT && PORT->PORTBCALL[0])
		memcpy(AXPTR->ORIGIN, PORT->PORTBCALL, 7);
	else if (PORT && PORT->PORTCALL[0])
		memcpy(AXPTR->ORIGIN, PORT->PORTCALL, 7);
	else
		memcpy(AXPTR->ORIGIN, MYCALL, 7);

	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	if (UIUIDigi[Port])
	{
		// This port has a digi string

		int DigiLen = UIUIDigiLen[Port];
		UCHAR * ptr;

		memcpy(&AXPTR->CTL, UIUIDigiAX[Port], DigiLen);
		
		ptr = (UCHAR *)AXPTR;
		ptr += DigiLen;
		AXPTR = (PMESSAGEX)ptr;

		Len += DigiLen;
	}

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->DATA, Msg, DataLen);

//	if (Queue)
//		QueueRaw(Port, &AXMSG, Len + 16);
//	else
		SendRaw(Port, (char *)&AXMSG.DEST, Len + 16);

	return;

}



VOID SendUIBeacon(int Port)
{
	char UIMessage[1024];
	int Len = (int)strlen(Message[Port]);
	int Index = 0;

	if (SendFromFile[Port])
	{
		FILE * hFile;

		hFile = fopen(FN[Port], "rb");
	
		if (hFile == 0)
			return;

		Len = (int)fread(UIMessage, 1, 1024, hFile); 
		
		fclose(hFile);

	}
	else
		strcpy(UIMessage, Message[Port]);

	Len =  UIRemoveLF(UIMessage, Len);

	while (Len > 256)
	{
		UISend_AX_Datagram(&UIMessage[Index], 256, Port, UIAXDEST[Port], TRUE);
		Index += 256;
		Len -= 256;
		Sleep(2000);
	}
	UISend_AX_Datagram(&UIMessage[Index], Len, Port, UIAXDEST[Port], TRUE);
}

#ifndef LINBPQ

typedef struct tag_dlghdr
{
	HWND hwndTab; // tab control
	HWND hwndDisplay; // current child dialog box
	RECT rcDisplay; // display rectangle for the tab control

	DLGTEMPLATE *apRes[MaxBPQPortNo + 1];

} DLGHDR;

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);

#endif

HWND hwndDlg;
int PageCount;
int CurrentPage=0;				// Page currently on show in tabbed Dialog


VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);

#define ICC_STANDARD_CLASSES   0x00004000

HWND hwndDisplay;

#define ID_TEST                         102
#define IDD_DIAGLOG1                    103
#define IDC_FROMFILE                    1022
#define IDC_EDIT1                       1054
#define IDC_FILENAME                    1054
#define IDC_EDIT2                       1055
#define IDC_MESSAGE                     1055
#define IDC_EDIT3                       1056
#define IDC_INTERVAL                    1056
#define IDC_EDIT4                       1057
#define IDC_UIDEST                      1057
#define IDC_FILE                        1058
#define IDC_TAB1                        1059
#define IDC_UIDIGIS                     1059
#define IDC_PORTNAME                    1060

extern HKEY REGTREE;
HBRUSH bgBrush; 

VOID SetupUI(int Port)
{
	char DigiString[100], * DigiLeft;

	ConvToAX25(UIUIDEST[Port], &UIAXDEST[Port][0]);

	UIUIDigiLen[Port] = 0;

	if (UIUIDigi[Port])
	{
		UIUIDigiAX[Port] = zalloc(100);
		strcpy(DigiString, UIUIDigi[Port]);
		DigiLeft = strlop(DigiString,',');

		while(DigiString[0])
		{
			ConvToAX25(DigiString, &UIUIDigiAX[Port][UIUIDigiLen[Port]]);
			UIUIDigiLen[Port] += 7;

			if (DigiLeft)
			{
				memmove(DigiString, DigiLeft, (int)strlen(DigiLeft) + 1);
				DigiLeft = strlop(DigiString,',');
			}
			else
				DigiString[0] = 0;
		}
	}
}

#ifndef LINBPQ

VOID SaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}

VOID SaveStringValue(config_setting_t * group, char * name, char * value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, value);

}

#endif

config_t cfg;

VOID SaveUIConfig()
{
	config_setting_t *root, *group, *UIGroup;
	int Port, MaxPort = GetNumberofPorts();
	char ConfigName[256];

	if (BPQDirectory[0] == 0)
	{
		strcpy(ConfigName,"UIUtil.cfg");
	}
	else
	{
		strcpy(ConfigName,BPQDirectory);
		strcat(ConfigName,"/");
		strcat(ConfigName,"UIUtil.cfg");
	}

	//	Get rid of old config before saving
	
	config_init(&cfg);

	root = config_root_setting(&cfg);

	group = config_setting_add(root, "main", CONFIG_TYPE_GROUP);

	UIGroup = config_setting_add(group, "UIUtil", CONFIG_TYPE_GROUP);

	for (Port = 1; Port <= MaxPort; Port++)
	{
		char Key[20];
		
		sprintf(Key, "Port%d", Port); 
		group = config_setting_add(UIGroup, Key, CONFIG_TYPE_GROUP);

		SaveStringValue(group, "UIDEST", &UIUIDEST[Port][0]);
		SaveStringValue(group, "FileName", &FN[Port][0]);
		SaveStringValue(group, "Message", &Message[Port][0]);
		SaveStringValue(group, "Digis", UIUIDigi[Port]);
	
		SaveIntValue(group, "Interval", Interval[Port]);
		SaveIntValue(group, "SendFromFile", SendFromFile[Port]);
	}

	if(!config_write_file(&cfg, ConfigName))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}

	config_destroy(&cfg);
}

int GetRegConfig();

VOID GetUIConfig()
{
	char Key[100];
	char CfgFN[256];
	char Digis[100];
	struct stat STAT;

	config_t cfg;
	config_setting_t *group;
	int Port, MaxPort = GetNumberofPorts();

	memset((void *)&cfg, 0, sizeof(config_t));

	config_init(&cfg);

	if (BPQDirectory[0] == 0)
	{
		strcpy(CfgFN,"UIUtil.cfg");
	}
	else
	{
		strcpy(CfgFN,BPQDirectory);
		strcat(CfgFN,"/");
		strcat(CfgFN,"UIUtil.cfg");
	}

	if (stat(CfgFN, &STAT) == -1)
	{
		// No file. If Windows try to read from registy

#ifndef LINBPQ 
		GetRegConfig();
#else
		Debugprintf("UIUtil Config File not found\n");
#endif
		return;
	}

	if(!config_read_file(&cfg, CfgFN))
	{
		fprintf(stderr, "UI Util Config Error Line %d - %s\n", config_error_line(&cfg), config_error_text(&cfg));

		config_destroy(&cfg);
		return;
	}

	group = config_lookup(&cfg, "main");

	if (group)
	{
		for (Port = 1; Port <= MaxPort; Port++)
		{	
			sprintf(Key, "main.UIUtil.Port%d", Port); 

			group = config_lookup (&cfg, Key);

			if (group)
			{
				GetStringValue(group, "UIDEST", &UIUIDEST[Port][0]);
				GetStringValue(group, "FileName", &FN[Port][0]);
				GetStringValue(group, "Message", &Message[Port][0]);
				GetStringValue(group, "Digis", Digis);
				UIUIDigi[Port] = _strdup(Digis);
	
				Interval[Port] = GetIntValue(group, "Interval");
				MinCounter[Port] = Interval[Port];

				SendFromFile[Port] = GetIntValue(group, "SendFromFile");

				SetupUI(Port);
			}
		}
	}


	_beginthread(UIThread, 0, NULL);

}

#ifndef LINBPQ

int GetIntValue(config_setting_t * group, char * name)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return 0;
}

BOOL GetStringValue(config_setting_t * group, char * name, char * value)
{
	const char * str;
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
	{
		str =  config_setting_get_string (setting);
		strcpy(value, str);
		return TRUE;
	}
	value[0] = 0;
	return FALSE;
}

int GetRegConfig()
{
	int retCode, Vallen, Type, i;
	char Key[80];
	char Size[80];
	HKEY hKey;
	RECT Rect;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil");
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

		RegCloseKey(hKey);
	}

	for (i=1; i<=32; i++)
	{
		wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", i);

		retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

		if (retCode == ERROR_SUCCESS)
		{	
			Vallen=0;
			RegQueryValueEx(hKey,"Digis",0,			
				(ULONG *)&Type, NULL, (ULONG *)&Vallen);

			if (Vallen)
			{
				UIUIDigi[i] = malloc(Vallen);
				RegQueryValueEx(hKey,"Digis",0,			
					(ULONG *)&Type, UIUIDigi[i], (ULONG *)&Vallen);
			}

			Vallen=4;
			retCode = RegQueryValueEx(hKey, "Interval", 0,			
				(ULONG *)&Type, (UCHAR *)&Interval[i], (ULONG *)&Vallen);

			MinCounter[i] = Interval[i];

			Vallen=4;
			retCode = RegQueryValueEx(hKey, "SendFromFile", 0,			
				(ULONG *)&Type, (UCHAR *)&SendFromFile[i], (ULONG *)&Vallen);


			Vallen=10;
			retCode = RegQueryValueEx(hKey, "UIDEST", 0, &Type, &UIUIDEST[i][0], &Vallen);

			Vallen=255;
			retCode = RegQueryValueEx(hKey, "FileName", 0, &Type, &FN[i][0], &Vallen);

			Vallen=999;
			retCode = RegQueryValueEx(hKey, "Message", 0, &Type, &Message[i][0], &Vallen);

			SetupUI(i);

			RegCloseKey(hKey);
		}
	}

	SaveUIConfig();

	return TRUE;
}

INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

	int retCode, disp;
	char Key[80];
	HKEY hKey;
	BOOL OK;
	OPENFILENAME ofn;
	char Digis[100];

	int Port = PortNum[CurrentPage];


	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{
			case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = &FN[Port][0];
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = BPQDirectory;

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0]);

			break;


		case IDOK:

			GetDlgItemText(hDlg, IDC_UIDEST, &UIUIDEST[Port][0], 10);

			if (UIUIDigi[Port])
			{
				free(UIUIDigi[Port]);
				UIUIDigi[Port] = NULL;
			}

			if (UIUIDigiAX[Port])
			{
				free(UIUIDigiAX[Port]);
				UIUIDigiAX[Port] = NULL;
			}

			GetDlgItemText(hDlg, IDC_UIDIGIS, Digis, 99); 
		
			UIUIDigi[Port] = _strdup(Digis);
		
			GetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0], 255); 
			GetDlgItemText(hDlg, IDC_MESSAGE, &Message[Port][0], 1000); 
	
			Interval[Port] = GetDlgItemInt(hDlg, IDC_INTERVAL, &OK, FALSE); 

			MinCounter[Port] = Interval[Port];

			SendFromFile[Port] = IsDlgButtonChecked(hDlg, IDC_FROMFILE);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", PortNum[CurrentPage]);

			retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIUIDEST[Port][0], (int)strlen(&UIUIDEST[Port][0]));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], (int)strlen(&FN[Port][0]));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], (int)strlen(&Message[Port][0]));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
				retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, Digis, (int)strlen(Digis));

				RegCloseKey(hKey);
			}

			SetupUI(Port);

			SaveUIConfig();

			return (INT_PTR)TRUE;


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case ID_TEST:

			SendUIBeacon(Port);
			return TRUE;

		}
		break;

	}	
	return (INT_PTR)FALSE;
}



VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos, tab = 0;
	INITCOMMONCONTROLSEX init;

	char PortNo[60];
	struct _EXTPORTDATA * PORTVEC;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

#define GWL_USERDATA        (-21)

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC = ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInstance, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	for (i = 1; i <= NUMBEROFPORTS; i++)
	{
		// Only allow UI on ax.25 ports

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntryFromSlot(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				if (PORTVEC->PORTCONTROL.UICAPABLE == 0)
					continue;

		wsprintf(PortNo, "Port %2d", GetPortNumber(i));
		PortNum[tab] = i;

		tie.pszText = PortNo;
		TabCtrl_InsertItem(pHdr->hwndTab, tab, &tie);
	
		pHdr->apRes[tab++] = DoLockDlgRes("PORTPAGE");
	}

	PageCount = tab;

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < PageCount; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(hInstance, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInstance, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char PortDesc[40];
	int Port;

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInstance, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	Port = PortNum[CurrentPage];
	// Fill in the controls

	GetPortDescription(PortNum[CurrentPage], PortDesc);

	SetDlgItemText(hwndDisplay, IDC_PORTNAME, PortDesc);

	CheckDlgButton(hwndDisplay, IDC_FROMFILE, SendFromFile[Port]);

	SetDlgItemInt(hwndDisplay, IDC_INTERVAL, Interval[Port], FALSE);

	SetDlgItemText(hwndDisplay, IDC_UIDEST, &UIUIDEST[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_UIDIGIS, UIUIDigi[Port]);



	SetDlgItemText(hwndDisplay, IDC_FILENAME, &FN[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_MESSAGE, &Message[Port][0]);

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}


//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}



LRESULT CALLBACK UIWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HKEY hKey=0;

	switch (message) { 

	case WM_INITDIALOG:
		OnTabbedDialogInit(hWnd);
		return (INT_PTR)TRUE;

	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hWnd);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;


		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LONG)bgBrush;
		}

		case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) {

		case IDOK:

			return TRUE;

		default:

			return 0;
		}


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
		case SC_RESTORE:

			return (DefWindowProc(hWnd, message, wParam, lParam));

		case  SC_MINIMIZE: 
			
			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));
						
			break;
		
		default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_CLOSE:
			return(DestroyWindow(hWnd));

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}

#endif

extern struct DATAMESSAGE * REPLYBUFFER;
char * __cdecl Cmdprintf(TRANSPORTENTRY * Session, char * Bufferptr, const char * format, ...);
void GetPortCTEXT(TRANSPORTENTRY * Session, char * Bufferptr, char * CmdTail, CMDX * CMD)
{
	char FN[250];
	FILE *hFile;
	struct stat STAT;
	struct PORTCONTROL * PORT = PORTTABLE;
	char PortList[256] = "";

	while (PORT)
	{
		if (PORT->CTEXT)
		{
			free(PORT->CTEXT);
			PORT->CTEXT = 0;
		}

		if (BPQDirectory[0] == 0)
			sprintf(FN, "Port%dCTEXT.txt", PORT->PORTNUMBER);
		else
			sprintf(FN, "%s/Port%dCTEXT.txt", BPQDirectory, PORT->PORTNUMBER);

		if (stat(FN, &STAT) == -1)
		{
			PORT = PORT->PORTPOINTER;
			continue;
		}

		hFile = fopen(FN, "rb");

		if (hFile)
		{
			char * ptr;
			
			PORT->CTEXT = zalloc(STAT.st_size + 1);
			fread(PORT->CTEXT , 1, STAT.st_size, hFile); 
			fclose(hFile);
			
			// convert CRLF or LF to CR
	
			while (ptr = strstr(PORT->CTEXT, "\r\n"))
				memmove(ptr, ptr + 1, strlen(ptr));

			// Now has LF

			while (ptr = strchr(PORT->CTEXT, '\n'))
				*ptr = '\r';


			sprintf(PortList, "%s,%d", PortList, PORT->PORTNUMBER);
		}

		PORT = PORT->PORTPOINTER;
	}

	if (Session)
	{	
		Bufferptr = Cmdprintf(Session, Bufferptr, "CTEXT Read for ports %s\r", &PortList[1]);
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
	}
	else
		Debugprintf("CTEXT Read for ports %s\r", &PortList[1]);
}

SOCKET OpenHTTPSock(char * Host)
{
	SOCKET sock = 0;
	struct sockaddr_in destaddr;
	struct sockaddr_in sinx; 
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
		
	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(80);

	//	Resolve name to address

	HostEnt = gethostbyname (Host);
		 
	if (!HostEnt)
	{
		err = WSAGetLastError();

		Debugprintf("Resolve Failed for %s %d %x", "api.winlink.org", err, err);
		return 0 ;			// Resolve failed
	}
	
	memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	
	//   Allocate a Socket entry

	sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return 0; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (struct sockaddr *) &sinx, addrlen) != 0 )
  	 	return FALSE; 

	if (connect(sock,(struct sockaddr *) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();
		closesocket(sock);
		return 0;
	}

	return sock;
}

static char HeaderTemplate[] = "POST %s HTTP/1.1\r\n"
	"Accept: application/json\r\n"
//	"Accept-Encoding: gzip,deflate,gzip, deflate\r\n"
	"Content-Type: application/json\r\n"
	"Host: %s:%d\r\n"
	"Content-Length: %d\r\n"
	//r\nUser-Agent: BPQ32(G8BPQ)\r\n"
//	"Expect: 100-continue\r\n"
	"\r\n";


VOID SendWebRequest(SOCKET sock, char * Host, char * Request, char * Params, int Len, char * Return)
{
	int InputLen = 0;
	int inptr = 0;
	char Buffer[4096];
	char Header[256];
	char * ptr, * ptr1;
	int Sent;

	sprintf(Header, HeaderTemplate, Request, Host, 80, Len, Params);
	Sent = send(sock, Header, (int)strlen(Header), 0);
	Sent = send(sock, Params, (int)strlen(Params), 0);

	if (Sent == -1)
	{
		int Err = WSAGetLastError();
		Debugprintf("Error %d from Web Update send()", Err);
		return;
	}

	while (InputLen != -1)
	{
		InputLen = recv(sock, &Buffer[inptr], 4096 - inptr, 0);

		if (InputLen == -1 || InputLen == 0)
		{
			int Err = WSAGetLastError();
			Debugprintf("Error %d from Web Update recv()", Err);
			return;
		}

		//	As we are using a persistant connection, can't look for close. Check
		//	for complete message

		inptr += InputLen;

		Buffer[inptr] = 0;
		
		ptr = strstr(Buffer, "\r\n\r\n");

		if (ptr)
		{
			// got header

			int Hddrlen = (int)(ptr - Buffer);
					
			ptr1 = strstr(Buffer, "Content-Length:");

			if (ptr1)
			{
				// Have content length

				int ContentLen = atoi(ptr1 + 16);

				if (ContentLen + Hddrlen + 4 == inptr)
				{
					// got whole response

					if (strstr(Buffer, " 200 OK"))
					{
						if (Return)
						{
							memcpy(Return, ptr + 4, ContentLen); 
							Return[ContentLen] = 0;
						}
						else
							Debugprintf("Map Database update ok");
					
					}
					else
					{
						strlop(Buffer, 13);
						Debugprintf("Map Update Params - %s", Params);
						Debugprintf("Map Update failed - %s", Buffer);
					}
					return;
				}
			}
			else
			{
				ptr1 = strstr(_strlwr(Buffer), "transfer-encoding:");
				
				if (ptr1)
				{
					// Just accept anything until I've sorted things with Lee
					Debugprintf("%s", ptr1);
					Debugprintf("Web Database update ok");
					return;
				}
			}
		}
	}
}

// https://packetnodes.spots.radio/api/NodeData/{callsign}

//SendHTTPRequest(sock, "/account/exists", Message, Len, Response);

#include "kiss.h"

extern char MYALIASLOPPED[10];
extern int MasterPort[MAXBPQPORTS+1];

void SendDataToPktMap(char *Msg)
{
	SOCKET sock;
	char Return[256];
	char Request[64];
	char Params[50000];
	struct PORTCONTROL * PORT = PORTTABLE;
	struct PORTCONTROL * SAVEPORT;
	struct ROUTE * Routes = NEIGHBOURS;
	int MaxRoutes = MAXNEIGHBOURS;
		
	int PortNo;
	int Active;
	uint64_t Freq;
	int Baud;
	int Bitrate;
	char * Mode;
	char * Use;
	char * Type;
	char * Modulation;

	char locked[] = " ! ";
	int Percent = 0;
	int Port = 0;
	char Normcall[10];
	char Copy[20];

	char * ptr = Params;

	printf("Sending to new map\n");

	sprintf(Request, "/api/NodeData/%s", MYNODECALL);

//	https://packetnodes.spots.radio/swagger/index.html

	// This builds the request and sends it

	// Minimum header seems to be

	//  "nodeAlias": "BPQ",
	//  "location": {"locator": "IO68VL"},
	//  "software": {"name": "BPQ32","version": "6.0.24.3"},

	ptr += sprintf(ptr, "{\"nodeAlias\": \"%s\",\r\n", MYALIASLOPPED);

	if (strlen(LOCATOR) == 6)
		ptr += sprintf(ptr, "\"location\": {\"locator\": \"%s\"},\r\n", LOCATOR);
	else
	{
		// Lat Lon

		double myLat, myLon;
		char LocCopy[80];
		char * context;

		strcpy(LocCopy, LOCATOR);

		myLat = atof(strtok_s(LocCopy, ",:; ", &context));
		myLon = atof(context);

		ptr += sprintf(ptr, "\"location\": {\"coords\": {\"lat\": %f, \"lon\": %f}},\r\n",
			myLat, myLon);

	}

#ifdef LINBPQ
	ptr += sprintf(ptr, "\"software\": {\"name\": \"LINBPQ\",\"version\": \"%s\"},\r\n", VersionString);
#else
	ptr += sprintf(ptr, "\"software\": {\"name\": \"BPQ32\",\"version\": \"%s\"},\r\n", VersionString);
#endif
	ptr += sprintf(ptr, "\"source\": \"ReportedByNode\",\r\n");

	//Ports

	ptr += sprintf(ptr, "\"ports\": [");

	// Get active ports

	while (PORT)
	{
		PortNo = PORT->PORTNUMBER;

		if (PORT->Hide) 
		{
			PORT = PORT->PORTPOINTER;
			continue;
		}

		if (PORT->SendtoM0LTEMap == 0)
		{
			PORT = PORT->PORTPOINTER;
			continue;
		}

		// Try to get port status - may not be possible with some

		if (PORT->PortStopped)
		{
			PORT = PORT->PORTPOINTER;
			continue;
		}

		Active = 0;
		Freq = 0;
		Baud = 0;
		Mode = "ax.25";
		Use = "";
		Type = "RF";
		Bitrate = 0;
		Modulation = "FSK";

		if (PORT->PORTTYPE == 0)
		{
			struct KISSINFO * KISS = (struct KISSINFO *)PORT;
			NPASYINFO Port;

			SAVEPORT = PORT;

			if (KISS->FIRSTPORT && KISS->FIRSTPORT != KISS)
			{
				// Not first port on device

				PORT = (struct PORTCONTROL *)KISS->FIRSTPORT;
				Port = KISSInfo[PortNo];
			}

			Port = KISSInfo[PORT->PORTNUMBER];

			if (Port)
			{
				// KISS like - see if connected 

				if (PORT->PORTIPADDR.s_addr || PORT->KISSSLAVE)
				{
					// KISS over UDP or TCP

					if (PORT->KISSTCP)
					{
						if (Port->Connected)
							Active = 1;
					}
					else
						Active = 1;		// UDP - Cant tell
				}
				else
					if (Port->idComDev)			// Serial port Open
						Active = 1;
				
				PORT = SAVEPORT;
			}		
		}
		else if (PORT->PORTTYPE == 14)		// Loopback 
			Active = 0;

		else if (PORT->PORTTYPE == 16)		// External
		{
			if (PORT->PROTOCOL == 10)		// 'HF' Port
			{
				struct TNCINFO * TNC = TNCInfo[PortNo];
				struct AGWINFO * AGW;

				if (TNC == NULL)
				{
					PORT = PORT->PORTPOINTER;
					continue;
				}

				if (TNC->RIG)
					Freq = TNC->RIG->RigFreq * 1000000;

				switch (TNC->Hardware)				// Hardware Type
				{
				case H_KAM:
				case H_AEA:
				case H_HAL:
				case H_SERIAL:

					// Serial

					if (TNC->hDevice)
						Active = 1;

					break;

				case H_SCS:
				case H_TRK:
				case H_WINRPR:
			
					if (TNC->HostMode)
						Active = 1;

					break;


				case H_UZ7HO:

					if (TNCInfo[MasterPort[PortNo]]->CONNECTED)
						Active = 1;

					// Try to get mode and frequency

					AGW = TNC->AGWInfo;

					if (AGW && AGW->isQTSM)
					{
						if (AGW->ModemName[0])
						{
							char * ptr1, * ptr2, *Context;

							strcpy(Copy, AGW->ModemName);
							ptr1 = strtok_s(Copy, " ", & Context);
							ptr2 = strtok_s(NULL, " ", & Context);

							if (Context)
							{
								Modulation = Copy;

								if (strstr(ptr1, "BPSK") || strstr(ptr1, "AFSK"))
								{
									Baud = Bitrate = atoi(Context);
								}
								else if (strstr(ptr1, "QPSK"))
								{
									Modulation = "QPSK";
									Bitrate = atoi(Context);
									Baud = Bitrate /2;
								}
							}
						}
					}

					break;

				case H_WINMOR:
				case H_V4:

				case H_MPSK:
				case H_FLDIGI:
				case H_UIARQ:
				case H_ARDOP:
				case H_VARA:
				case H_KISSHF:
				case H_FREEDATA:

					// TCP

					Mode = Modenames[TNC->Hardware];

					if (TNC->CONNECTED)
						Active = 1;

					break;

				case H_TELNET:

					Active = 1;
					Type = "Internet";
					Mode = "";
				}
			}
			else
			{
				// External but not HF - AXIP, BPQETHER VKISS, ??

				struct _EXTPORTDATA * EXTPORT = (struct _EXTPORTDATA *)PORT;
				Type = "Internet";
				Active = 1;
			}
		}

		if (Active)
		{
			ptr += sprintf(ptr, "{\"id\": \"%d\",\"linkType\": \"%s\","
			"\"freq\": \"%lld\",\"mode\": \"%s\",\"modulation\": \"%s\","
			"\"baud\": \"%d\",\"bitrate\": \"%d\",\"usage\": \"%s\",\"comment\": \"%s\"},\r\n",
			PortNo, Type, 
			Freq, Mode, Modulation,
			Baud, Bitrate, "Access", PORT->PORTDESCRIPTION);
		}
	
		PORT = PORT->PORTPOINTER;
	}

	ptr -= 3;
	ptr += sprintf(ptr, "],\r\n");

	//	Neighbours

	ptr += sprintf(ptr, "\"neighbours\": [\r\n");

	while (MaxRoutes--)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
			if (Routes->NEIGHBOUR_LINK && Routes->NEIGHBOUR_LINK->L2STATE >= 5)
			{
				ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall);
				strlop(Normcall, ' ');

				ptr += sprintf(ptr, 
				"{\"node\": \"%s\", \"port\": \"%d\", \"quality\":  \"%d\"},\r\n",
				Normcall, Routes->NEIGHBOUR_PORT, Routes->NEIGHBOUR_QUAL);
			}

		Routes++;
	}

	ptr -= 3;
	ptr += sprintf(ptr, "]}");

/*
{
  "nodeAlias": "BPQ",
  "location": {"locator": "IO92KX"},
  "software": {"name": "BPQ32","version": "6.0.24.11 Debug Build "},
  "contact": "G8BPQ",
  "sysopComment": "Testing",
  "source": "ReportedByNode"
}

 "ports": [
    {
      "id": "string",
      "linkType": "RF",
      "freq": 0,
      "mode": "string",
      "modulation": "string",
      "baud": 0,
      "bitrate": 0,
      "usage": "Access",
      "comment": "string"
    }
  ],



*/
	//  "contact": "string",
	//  "neighbours": [{"node": "G7TAJ","port": "30"}]

	sock = OpenHTTPSock("packetnodes.spots.radio");

	if (sock == 0)
		return;

	SendWebRequest(sock, "packetnodes.spots.radio", Request, Params, strlen(Params), Return);
	closesocket(sock);
}

//	="{\"neighbours\": [{\"node\": \"G7TAJ\",\"port\": \"30\"}]}";

//'POST' \
//  'https://packetnodes.spots.radio/api/NodeData/GM8BPQ' \
//  -H 'accept: */*' \
//  -H 'Content-Type: application/json' \
//  -d '{
//  "nodeAlias": "BPQ",
//  "location": {"locator": "IO68VL"},
//  "software": {"name": "BPQ32","version": "6.0.24.3"},
//  "contact": "string",
//  "neighbours": [{"node": "G7TAJ","port": "30"}]
//}'









