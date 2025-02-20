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



// Monitor Code - from moncode.asm

// Modified for AGW form monitor

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE 

#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma data_seg("_BPQDATA")
				
#include "cheaders.h"
#include "tncinfo.h"

//	MSGFLAG contains CMD/RESPONSE BITS

#define	CMDBIT	4		// CURRENT MESSAGE IS A COMMAND
#define	RESP 2		// CURRENT MSG IS RESPONSE
#define	VER1 1 		// CURRENT MSG IS VERSION 1


#define	UI	3
#define	SABM 0x2F
#define	DISC 0x43
#define	DM	0x0F
#define	UA	0x63
#define	FRMR 0x87
#define	RR	1
#define	RNR	5
#define	REJ	9

#define	PFBIT 0x10		// POLL/FINAL BIT IN CONTROL BYTE

#define	NETROM_PID 0xCF
#define	IP_PID 0xCC
#define	ARP_PID 0xCD

#define	NODES_SIG	0xFF

UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, int msglen);

static UCHAR * DISPLAY_NETROM(MESSAGE * ADJBUFFER, UCHAR * Output, int MsgLen, int DoNodes);
static UCHAR * DISPLAYIPDATAGRAM(IPMSG * IP, UCHAR * Output, int MsgLen);
static UCHAR * DISPLAYARPDATAGRAM(UCHAR * Datagram, UCHAR * Output);


int InternalAGWDecodeFrame(MESSAGE * msg, char * buffer, time_t Stamp, int * FrameType, int useLocalTime, int DoNodes)
{
	UCHAR * ptr;
	int n;
	MESSAGE * ADJBUFFER;
	ptrdiff_t Work;
	UCHAR CTL;
	BOOL PF = 0;
	char CRCHAR[3] = "  ";
	char PFCHAR[3] = "  ";
	int MSGFLAG = 0;		//CR and V1 flags
	char * Output = buffer;
	char From[10], To[10];
	BOOL Info = 0;
	BOOL FRMRFLAG = 0;
	BOOL XIDFLAG = 0;
	BOOL TESTFLAG = 0;
	size_t MsgLen = msg->LENGTH;

	struct tm * TM;

	if (useLocalTime)
		TM = localtime(&Stamp);
	else
		TM = gmtime(&Stamp);

	//	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED

	n = 8;						// MAX DIGIS
	ptr = &msg->ORIGIN[6];	// End of Address bit

	while ((*ptr & 1) == 0)
	{
		//	MORE TO COME
	
		ptr += 7;
		n--;

		if (n == 0)
		{
			return 0;						// Corrupt - no end of address bit
		}
	}

	// Reached End of digis

	Work = ptr - &msg->ORIGIN[6];			// Work is length of digis

	MsgLen -= Work;

	ADJBUFFER = (MESSAGE *)((UCHAR *)msg + Work);			// ADJBUFFER points to CTL, etc. allowing for digis

	CTL = ADJBUFFER->CTL;

	if (CTL & PFBIT)
		PF = TRUE;

	CTL &= ~PFBIT;

	*FrameType = CTL;

	Output += sprintf((char *)Output, " %d:Fm ", msg->PORT & 0x7f);		// Mask TX bit

	From[ConvFromAX25(msg->ORIGIN, From)] = 0;
	To[ConvFromAX25(msg->DEST, To)] = 0;

	Output += sprintf((char *)Output, "%s To %s", From, To);

	//	Display any Digi-Peaters   

	n = 8;					// Max number of digi-peaters
	ptr = &msg->ORIGIN[6];	// End of Address bit

	while ((*ptr & 1) == 0)
	{
		//	MORE TO COME

		From[ConvFromAX25(ptr + 1, From)] = 0;

		if (n == 8)
			Output += sprintf((char *)Output, " Via %s", From);	// Send via on first
		else
			Output += sprintf((char *)Output, ",%s", From);
	
		ptr += 7;
		n--;

		if (n == 0)
			break;

		// See if digi actioned - put a * on last actioned

		if (*ptr & 0x80)
		{
			if (*ptr & 1)						// if last address, must need *
				*(Output++) = '*';
			else
				if ((ptr[7] & 0x80) == 0)		// Repeased by next?
					*(Output++) = '*';			// No, so need *
		}
	}		
	
	*(Output++) = ' ';
	
	// Set up CR and PF

	CRCHAR[0] = 0;
	PFCHAR[0] = 0;

	if (msg->DEST[6] & 0x80)
	{
		if (msg->ORIGIN[6] & 0x80)			//	Both set, assume V1
			MSGFLAG |= VER1;
		else
		{
			MSGFLAG |= CMDBIT;
			CRCHAR[0] = ' ';
			CRCHAR[1] = 'C';
			if (PF)							// If FP set
			{
				PFCHAR[0] = ' ';
				PFCHAR[1] = 'P';
			}
		}
	}
	else
	{
		if (msg->ORIGIN[6] & 0x80)			//	Only Origin Set
		{
			MSGFLAG |= RESP;
			CRCHAR[0] = ' ';
			CRCHAR[1] = 'R';
			if (PF)							// If FP set
			{
				PFCHAR[0] = ' ';
				PFCHAR[1] = 'F';
			}
		}
		else
			MSGFLAG |= VER1;				// Neither, assume V1
	}

	if ((CTL & 1) == 0)						// I frame
	{
		int NS = (CTL >> 1) & 7;			// ISOLATE RECEIVED N(S)
		int NR = (CTL >> 5) & 7;

		Info = 1;

		Output += sprintf((char *)Output, "<I%s%s S%d R%d>", CRCHAR, PFCHAR, NS, NR);
	}
	else if (CTL == 3)
	{
		//	Un-numbered Information Frame 
		//UI pid=F0 Len=20 >

		Output += sprintf((char *)Output, "<UI pid=%02X Len=%d>", ADJBUFFER->PID, (int)MsgLen - 23);
		Info = 1;
	}
	else if (CTL & 2)
	{
		// UN Numbered
				
		char SUP[5] = "??";

		switch (CTL)
		{
		case SABM:

			strcpy(SUP, "C");
			break;

		case DISC:

			strcpy(SUP, "D");
			break;

		case DM:

			strcpy(SUP, "DM");
			break;
		
		case UA:

			strcpy(SUP, "UA");
			break;
		

		case FRMR:

			strcpy(SUP, "FRMR");
			FRMRFLAG = 1;
			break;
		}

		Output += sprintf((char *)Output, "<%s%s%s>", SUP, CRCHAR, PFCHAR);
	}
	else
	{
		// Super

		int NR = (CTL >> 5) & 7;
		char SUP[4] = "??";

		switch (CTL & 0x0F)
		{
		case RR:

			strcpy(SUP, "RR");
			break;

		case RNR:

			strcpy(SUP, "RNR");
			break;

		case REJ:

			strcpy(SUP, "REJ");
			break;
		}

		Output += sprintf((char *)Output, "<%s%s%s R%d>", SUP, CRCHAR, PFCHAR, NR);

	}

	Output += sprintf((char *)Output, "[%02d:%02d:%02d]", TM->tm_hour, TM->tm_min, TM->tm_sec);


	if (FRMRFLAG)
		Output += sprintf((char *)Output, "%02X %02X %02X", ADJBUFFER->PID, ADJBUFFER->L2DATA[0], ADJBUFFER->L2DATA[1]); 

	if (Info)
	{
		// We have an info frame

		switch (ADJBUFFER->PID)
		{
		case 0xF0:		// Normal Data
		{
			char Infofield[257];
			char * ptr1 = Infofield;
			char * ptr2 = ADJBUFFER->L2DATA;
			UCHAR C;
			size_t len;

			MsgLen = MsgLen - 23;

			if (MsgLen < 0 || MsgLen > 257)
				return 0;				// Duff

			while (MsgLen--)
			{
				C = *(ptr2++);

				// Convert to printable

				C &= 0x7F;

				if (C == 13 || C == 10 || C > 31)
					*(ptr1++) = C;	
			}

			len = ptr1 - Infofield;
	
//			Output[0] = ':';
			Output[0] = 13;
			memcpy(&Output[1], Infofield, len);
			Output += (len + 1);

			break;
		}
		case NETROM_PID:
			
			Output = DISPLAY_NETROM(ADJBUFFER, Output,(int) MsgLen, DoNodes);
			break;

		case IP_PID:

			Output += sprintf((char *)Output, " <IP>\r");
			Output = DISPLAYIPDATAGRAM((IPMSG *)&ADJBUFFER->L2DATA[0], Output, (int)MsgLen);
			break;

		case ARP_PID:
			
			Output = DISPLAYARPDATAGRAM(&ADJBUFFER->L2DATA[0], Output);
			break;

		case 8:					// Fragmented IP

			Output += sprintf((char *)Output, "<Fragmented IP>");
			break;	
		}
	}

	if (Output == NULL)
		return 0;

	if (Output[-1] != 13)
		Output += sprintf((char *)Output, "\r");

	return (int)(Output - buffer);

}
//      Display NET/ROM data                                                 

UCHAR * DISPLAY_NETROM(MESSAGE * ADJBUFFER, UCHAR * Output, int MsgLen, int DoNodes)
{
	char Alias[7]= "";
	char Dest[10];
	char Node[10];
	UCHAR TTL, Index, ID, TXNO, RXNO, OpCode, Flags, Window;
	UCHAR * ptr = &ADJBUFFER->L2DATA[0];

 	if (ADJBUFFER->L2DATA[0] == NODES_SIG)
	{
		// Display NODES

		if (DoNodes == 0)
			return NULL;

		// If an INP3 RIF (type <> UI) decode as such
	
		if (ADJBUFFER->CTL != 3)		// UI
			return DisplayINP3RIF(&ADJBUFFER->L2DATA[1], Output, MsgLen - 24);

		memcpy(Alias, ++ptr, 6);

		ptr += 6;
	
		Output += sprintf((char *)Output, "\rFF %s (NetRom Routing)\r", Alias);

		MsgLen -= 30;					//Header, mnemonic and signature length

		while(MsgLen > 20)				// Entries are 21 bytes
		{
			Dest[ConvFromAX25(ptr, Dest)] = 0;
			ptr +=7;
			memcpy(Alias, ptr, 6);
			ptr +=6;
			strlop(Alias, ' ');
			Node[ConvFromAX25(ptr, Node)] = 0;
			ptr +=7;

			Output += sprintf((char *)Output, "  %s:%s via %s qlty=%d\r", Alias, Dest, Node, ptr[0]);
			ptr++;
			MsgLen -= 21;
		}
		return Output;
	}

	//	Display normal NET/ROM transmissions 

	Output += sprintf((char *)Output, " NET/ROM\r  ");

	Dest[ConvFromAX25(ptr, Dest)] = 0;
	ptr +=7;
	Node[ConvFromAX25(ptr, Node)] = 0;
	ptr +=7;

	TTL = *(ptr++);
	Index = *(ptr++);
	ID = *(ptr++);
	TXNO = *(ptr++);
	RXNO = *(ptr++);
	OpCode = Flags = *(ptr++);

	OpCode &= 15;				// Remove Flags

	Output += sprintf((char *)Output, "%s to %s ttl %d cct=%02X%02X ", Dest, Node, TTL, Index, ID );
	MsgLen -= 20;

	switch (OpCode)
	{
	case L4CREQ:

		Window = *(ptr++);
		Dest[ConvFromAX25(ptr, Dest)] = 0;
		ptr +=7;
		Node[ConvFromAX25(ptr, Node)] = 0;
		ptr +=7;

		Output += sprintf((char *)Output, "<CON REQ> w=%d %s at %s", Window, Dest, Node);

		if (MsgLen > 38)				// BPQ Extended Params
		{
			short Timeout = (SHORT)*ptr;
			Output += sprintf((char *)Output, " t/o %d", Timeout);
		}

		return Output;

	case L4CACK:

		if (Flags & L4BUSY)				// BUSY RETURNED
			return Output + sprintf((char *)Output, " <CON NAK> - BUSY");

		return Output + sprintf((char *)Output, " <CON ACK> w=%d my cct=%02X%02X", ptr[1], TXNO, RXNO);

	case L4DREQ:

		return Output + sprintf((char *)Output, " <DISC REQ>");

	case L4DACK:

		return Output + sprintf((char *)Output, " <DISC ACK>");

	case L4INFO:
		{
			char Infofield[257];
			char * ptr1 = Infofield;
			UCHAR C;
			size_t len;
			
			Output += sprintf((char *)Output, " <INFO S%d R%d>", TXNO, RXNO);
			
			if (Flags & L4BUSY)
				*(Output++) = 'B';

			if (Flags & L4NAK)
				*(Output++) = 'N';

			if (Flags & L4MORE)
				*(Output++) = 'M';
	
			MsgLen = MsgLen - 23;

			if (MsgLen < 0 || MsgLen > 257)
				return Output;				// Duff

			while (MsgLen--)
			{
				C = *(ptr++);

				// Convert to printable

				C &= 0x7F;

				if (C == 13 || C == 10 || C > 31)
					*(ptr1++) = C;	
			}

			len = ptr1 - Infofield;
	
			Output[0] = ':';
			Output[1] = 13;
			memcpy(&Output[2], Infofield, len);
			Output += (len + 2);
		}
		
		return Output;

	case L4IACK:

		Output += sprintf((char *)Output, " <INFO ACK R%d> ", RXNO);
	
		if (Flags & L4BUSY)
			*(Output++) = 'B';

		if (Flags & L4NAK)
			*(Output++) = 'N';

		if (Flags & L4MORE)
			*(Output++) = 'M';

		return Output;


	case 0:

		//	OPcode zero is used for several things

		if (Index == 0x0c && ID == 0x0c)	// IP	
		{
//			Output =  L3IP(Output);
			return Output;
		}
	
		if (Index == 0 && ID == 1)			// NRR	
		{
			Output += sprintf((char *)Output, " <Record Route>\r");

			MsgLen -= 23;

			while (MsgLen > 6)
			{
				Dest[ConvFromAX25(ptr, Dest)] = 0;

				if (ptr[7] & 0x80)
					Output += sprintf((char *)Output, "%s* ", Dest);
				else
					Output += sprintf((char *)Output, "%s ", Dest);

				ptr +=8;
				MsgLen -= 8;
			}

			return Output;
		}
	}

	Output += sprintf((char *)Output, " <???\?>");
	return Output;
}

/*
	
	PUBLIC	L3IP
L3IP:
;
;	TCP/IP DATAGRAM
;
	mov	EBX,OFFSET IP_MSG
	call	NORMSTR
;
	INC	ESI			; NOW POINTING TO IP HEADER

*/
UCHAR * DISPLAYIPDATAGRAM(IPMSG * IP, UCHAR * Output, int MsgLen)
{
	UCHAR * ptr;
		
	ptr = (UCHAR *)&IP->IPSOURCE;
	Output += sprintf((char *)Output, "%d.%d.%d.%d>", ptr[0], ptr[1], ptr[2], ptr[3]);

	ptr = (UCHAR *)&IP->IPDEST;
	Output += sprintf((char *)Output, "%d.%d.%d.%d LEN:%d ", ptr[0], ptr[1], ptr[2], ptr[3], htons(IP->IPLENGTH));

/*
	MOV	AL,IPPROTOCOL[ESI]
	CMP AL,6
	JNE @F
	
	MOV EBX, OFFSET TCP
	CALL NORMSTR
	JMP ADD_CR
@@:

	CMP AL,1
	JNE @F
	
	MOV EBX, OFFSET ICMP
	CALL NORMSTR
	JMP ADD_CR
@@:

	CALL	DISPLAY_BYTE_1		; DISPLAY PROTOCOL TYPE

;	mov	AL,CR
;	call	PUTCHAR
;
;	MOV	ECX,39			; TESTING
;IPLOOP:
;	lodsb
;	CALL	BYTE_TO_HEX
;
;	LOOP	IPLOOP

	JMP	ADD_CR


*/
	return Output;
}



UCHAR * DISPLAYARPDATAGRAM(UCHAR * Datagram, UCHAR * Output)
{
	UCHAR * ptr = Datagram;
	UCHAR Dest[10];
	
	if (ptr[7] == 1)		// Request
		return Output + sprintf((char *)Output, " < ARP Request who has %d.%d.%d.%d? Tell %d.%d.%d.%d",
			ptr[26], ptr[27], ptr[28], ptr[29], ptr[15], ptr[16], ptr[17], ptr[18]);

	// Response

	Dest[ConvFromAX25(&ptr[8], Dest)] = 0;

	return Output + sprintf((char *)Output, " < ARP Rreply %d.%d.%d.%d? is at %s",
			ptr[15], ptr[16], ptr[17], ptr[18], "??");

}
