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
//	Implements the DOS register based API. 

//	Called via an assmbler glue that puts registers into C variables.

#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#include "time.h"
#include "stdio.h"
#include <fcntl.h>		

#include "compatbits.h"

#include "CHeaders.h"

extern QCOUNT; 
extern BPQVECSTRUC BPQHOSTVECTOR[];
extern int MAJORVERSION;
extern int MINORVERSION;
extern char pgm[256];						// Uninitialised so per process

VOID PostDataAvailable(TRANSPORTENTRY * Session);
DllExport int APIENTRY SendMsg(int stream, char * msg, int len);
DllExport int APIENTRY AllocateStream(int stream);
DllExport int APIENTRY SendRaw(int port, char * msg, int len);
DllExport time_t APIENTRY GetRaw(int stream, char * msg, int * len, int * count);
VOID SENDNODESMSG();

int BTLENGTH;
char BTEXTFLD[256];
int REALTIMETICKS;

VOID CHOSTAPI(ULONG * pEAX, ULONG * pEBX, ULONG * pECX, ULONG * pEDX, VOID ** pESI, VOID ** pEDI)
{
	ULONG EAX = *pEAX;
	ULONG EBX = *pEBX;
	ULONG ECX = *pECX;
	ULONG EDX = *pEDX;
	VOID * ESI = *pESI;
	VOID * EDI = *pEDI;

	int Command;
	int Stream;
	int n;
	int Temp;
	PBPQVECSTRUC HostVec;
	TRANSPORTENTRY * Session;

/*
;	COMMANDS SUPPORTED ARE
;
;	AH = 0	Get node/switch version number and description.  On return
;		AH = major version number and AL = minor version number,
;		and user's buffer pointed to by ES:ESI is set to the text
;		string normally output by the USERS command, eg:
;		"G8BPQ Packet Switch Version 4.01 Dev".  CX is set to the
;		length of the text string.
;
;
;	AH = 1	Set application mask to value in DL (or even DX if 16
;		applications are ever to be supported).
;
;		Set application flag(s) to value in CL (or CX).
;		whether user gets connected/disconnected messages issued
;		by the node etc.
;
;
;	AH = 2	Send frame in ES:ESI (length CX)
;
;
;	AH = 3	Receive frame into buffer at ES:ESI, length of frame returned
;		in CX.  BX returns the number of outstanding frames still to
;		be received (ie. after this one) or zero if no more frames
;		(ie. this is last one).
;
;
;
;	AH = 4	Get stream status.  Returns:
;
;		CX = 0 if stream disconnected or CX = 1 if stream connected
;		DX = 0 if no change of state since last read, or DX = 1 if
;		       the connected/disconnected state has changed since
;		       last read (ie. delta-stream status).
;
;
;
;	AH = 6	Session control.
;
;		CX = 0 Conneect - _APPLMASK in DL
;		CX = 1 connect
;		CX = 2 disconnect
;		CX = 3 return user to node
;
;
;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		BX = L2 paclen for the radio port
;		CX = L2 maxframe for the radio port
;		DX = L4 window size (if L4 circuit, or zero)
;		ES:EDI = CALLSIGN

;AH = 9		Fetch node/application callsign & alias.  AL = application
;		number:
;
;		0 = node
;		1 = BBS
;		2 = HOST
;		3 = SYSOP etc. etc.
;
;		Returns string with alias & callsign or application name in
;		user's buffer pointed to by ES:ESI length CX.  For example:
;
;		"WORCS:G8TIC"  or "TICPMS:G8TIC-10".
;
;
;	AH = 10	Unproto transmit frame.  Data pointed to by ES:ESI, of
;		length CX, is transmitted as a HDLC frame on the radio
;		port (not stream) in AL.
;
;
;	AH = 11 Get Trace (RAW Data) Frame into ES:EDI,
;			 Length to CX, Timestamp to AX
;
;
;	AH = 12 Update Switch. At the moment only Beacon Text may be updated
;		DX = Function
;		     1=update BT. ES:ESI, Len CX = Text
;		     2=kick off nodes broadcast
;
;	AH = 14 Internal Interface for IP Router
;
;		Send frame - to NETROM L3 if DL=0
;			     to L2 Session if DL<>0
;
;
; 	AH = 15 Get interval timer
;

*/

	Command = (EAX & 0xFFFF) >> 8;

	Stream = (EAX & 0xFF);
	n = Stream - 1;				// API Numbers Streams 1-64 

	if (n < 0 || n > 63)
		n = 64;

	HostVec = &BPQHOSTVECTOR[n];
	Session = HostVec->HOSTSESSION;

	switch (Command)
	{
	case 0:					// Check Loaded/Get Version
	
		EAX = ('P' << 8) | 'B';
		EBX =  ('Q' << 8) | ' ';
	
		EDX = (MAJORVERSION << 8) | MINORVERSION; 
		break;

	case 1:					// Set Appl mAsk
	
		HostVec->HOSTAPPLMASK = EDX;	// APPL MASK
		HostVec->HOSTAPPLFLAGS = (UCHAR)ECX;	// APPL FLAGS
	
		// If either is non-zero, set allocated and Process. This gets round problem with
		// stations that don't call allocate stream
	
		if (ECX || EBX)
		{
			HostVec->HOSTFLAGS |= 0x80;		// SET ALLOCATED BIT	
			HostVec->STREAMOWNER = GetCurrentProcessId();
	
			//	Set Program Name

			memcpy(&HostVec->PgmName, pgm, 31);
		}
		break;

	case 2:							// Send Frame

		//	ES:ESI = MESSAGE, CX = LENGTH, BX = VECTOR

		EAX = SendMsg(Stream, ESI, ECX);
		break;
	
	case 3:

	//	AH = 3	Receive frame into buffer at ES:EDI, length of frame returned
	//		in CX.  BX returns the number of outstanding frames still to
	//		be received (ie. after this one) or zero if no more frames
	//		(ie. this is last one).

		EAX = GetMsg(Stream, EDI, &ECX, &EBX);
		break;

	case 4:

	//	AH = 4	Get stream status.  Returns:
	//	CX = 0 if stream disconnected or CX = 1 if stream connected
	//	DX = 0 if no change of state since last read, or DX = 1 if
	//	       the connected/disconnected state has changed since
	//	       last read (ie. delta-stream status).

		ECX = EDX = 0;

		if (HostVec->HOSTFLAGS & 3)		//STATE CHANGE BITS
			EDX = 1;

		if (Session)
			ECX = 1;
		
		break;
		
	case 5:

	//	AH = 5	Ack stream status change

		HostVec->HOSTFLAGS &= 0xFC;		// Clear Chnage Bits
		break;

	case 6:

	//	AH = 6	Session control.

	//	CX = 0 Conneect - APPLMASK in DL
	//	CX = 1 connect
	//	CX = 2 disconnect
	//	CX = 3 return user to node

		SessionControl(Stream, ECX, EDX);
		break;

	case 7:

	//	AH = 7	Get buffer counts for stream.  Returns:

	//	AX = number of status change messages to be received
	//	BX = number of frames queued for receive
	//	CX = number of un-acked frames to be sent
	//	DX = number of buffers left in node
	//	SI = number of trace frames queued for receive


	ECX = 0;				// unacked frames
	EDX = QCOUNT;

	ESI = (void *)MONCount(Stream);
	EBX = RXCount(Stream);
	ECX = TXCount(Stream);

	EAX = 0;				// Is this right ???

	break;

	case 8:

	//	AH = 8		Port control/information.  Called with a stream number
	//		in AL returns:
	//
	//	AL = Radio port on which channel is connected (or zero)
	//	AH = SESSION TYPE BITS
	//	BX = L2 paclen for the radio port
	//	CX = L2 maxframe for the radio port
	//	DX = L4 window size (if L4 circuit, or zero)
	//	ES:EDI = CALLSIGN


		GetConnectionInfo(Stream, EDI, &EAX, &Temp, &EBX, &ECX, &EDX); // Return the Secure Session Flag rather than not connected
		EAX |= Temp <<8;

		break;


	case 9:

		// Not Implemented

		break;

	case 10:

	//	AH = 10	Unproto transmit frame.  Data pointed to by ES:ESI, of
	//	length CX, is transmitted as a HDLC frame on the radio
	//	port (not stream) in AL.

		EAX = SendRaw(EAX, ESI, ECX);
		return;

	case 11:

	//	AH = 11 Get Trace (RAW Data) Frame into ES:EDI,
	//	 Length to CX, Timestamp to AX

		EAX =  GetRaw(Stream, EDI, &ECX, &EBX);
		break;

	case 12:

	// Update Switch

		if (EDX == 2)
		{
			SENDNODESMSG();
			break;
		}
		if (EDX == 2)
		{
			//	UPDATE BT

			BTLENGTH = ECX;
			memcpy(BTEXTFLD, ESI, ECX + 7);
		}

		break;

	case 13:

		// BPQALLOC

		//	AL = 0 = Find Free
		//	AL != 0 Alloc or Release

		if (EAX == 0)
		{
			EAX = FindFreeStream();
			break;
		}

		if (ECX == 1)			// Allocate
		{
			 EAX = AllocateStream(Stream);
			 break;
		}
		
		DeallocateStream(Stream);
		break;

	case 14:

	//	AH = 14 Internal Interface for IP Router

	//	Send frame - to NETROM L3 if DL=0
	//	             to L2 Session if DL<>0

		break;			// Shouldn't be needed

	case 15:

		// GETTIME

		EAX = REALTIMETICKS;
		EBX = 0;
	
#ifdef EXCLUDEBITS
	
		EBX = (ULONG)ExcludeList;

#endif
		break;

	}

	*pEAX = EAX;
	*pEBX = EBX;
	*pECX = ECX;
	*pEDX = EDX;
	*pESI = ESI;
	*pEDI = EDI;

	return;
}	

