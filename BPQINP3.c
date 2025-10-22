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
//	INP3 Suport Code for BPQ32 Switch
//

//	All code runs from the BPQ32 Received or Timer Routines under Semaphore.


#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#include "cheaders.h"

#include "time.h"
#include "stdio.h"
#include <fcntl.h>					 
//#include "vmm.h"

uint64_t INP3timeLoadedMS = 0;

extern int DEBUGINP3;

VOID SendNegativeInfo();
VOID SortRoutes(struct DEST_LIST * Dest);
VOID SendRTTMsg(struct ROUTE * Route);
VOID TCPNETROMSend(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame);

static VOID SendNetFrame(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame)
{
	// INP3 should only ever send over an active link, so just queue the message

	if (Route->TCPPort)			// NETROM over TCP
	{
		TCPNETROMSend(Route, Frame);
		ReleaseBuffer(Frame);
		return;
	}

	if (Route->NEIGHBOUR_LINK)
		C_Q_ADD(&Route->NEIGHBOUR_LINK->TX_Q, Frame);
	else
		ReleaseBuffer(Frame);
}


typedef struct _RTTMSG
{
	UCHAR ID[7];
	UCHAR TXTIME[11];
	UCHAR SMOOTHEDRTT[11];
	UCHAR LASTRTT[11];
	UCHAR POINTER[11];
	UCHAR ALIAS[7];
	UCHAR VERSION[12];
	UCHAR SWVERSION[9];
	UCHAR FLAGS[20];
	UCHAR PADDING[137];

} RTTMSG;

int COUNTNODES(struct ROUTE * ROUTE);

VOID __cdecl Debugprintf(const char * format, ...);

VOID SendINP3RIF(struct ROUTE * Route, UCHAR * Call, UCHAR * Alias, int Hops, int RTT);
VOID SendOurRIF(struct ROUTE * Route);
VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt);
VOID UpdateRoute(struct DEST_LIST * Dest, struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt);
VOID KillRoute(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR);
VOID AddHere(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt);
VOID SendRIPToNeighbour(struct ROUTE * Route);
VOID DecayNETROMRoutes(struct ROUTE * Route);
VOID DeleteINP3Routes(struct ROUTE * Route);
BOOL L2SETUPCROSSLINKEX(PROUTE ROUTE, int Retries);

//#define NOINP3

struct _RTTMSG RTTMsg = {""};

//struct ROUTE DummyRoute = {"","",""};

int RIPTimerCount = 0;				// 1 sec to 10 sec counter
int PosTimerCount = 0;				// 1 sec to 5 Mins counter

// Timer Runs every 10 Secs

extern int MAXRTT;			// 90 secs
extern int MaxHops;

extern int RTTInterval;			// 4 Minutes
int RTTRetries = 2;
int RTTTimeout = 6;				// 1 Min (Horizon is 1 min)

VOID InitialiseRTT()
{
	UCHAR temp[256] = "";

	memset(&RTTMsg, ' ', sizeof(struct _RTTMSG));
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memcpy(RTTMsg.VERSION, "LEVEL3_V2.1 ", 12);
	memcpy(RTTMsg.SWVERSION, "BPQ32002 ", 9);
	_snprintf(temp, sizeof(temp), "$M%d $N $H%d            ", MAXRTT, MaxHops); // trailing spaces extend to ensure padding if the length of characters for MAXRTT changes.
	memcpy(RTTMsg.FLAGS, temp, 20);                 // But still limit the actual characters copied.
	memcpy(RTTMsg.ALIAS, &MYALIASTEXT, 6);
	RTTMsg.ALIAS[6] = ' ';
}

VOID TellINP3LinkGone(struct ROUTE * Route)
{
	struct DEST_LIST * Dest =  DESTS;
	char call[11]="";

	ConvFromAX25(Route->NEIGHBOUR_CALL, call);
	Debugprintf("BPQ32 L2 Link to Neighbour %s lost", call);

	if (Route->NEIGHBOUR_LINK)
		Debugprintf("BPQ32 Neighbour_Link not cleared");

	// Link can have both NETROM and INP3 links

//	if (Route->INP3Node == 0)
		DecayNETROMRoutes(Route);
//	else
		DeleteINP3Routes(Route);
}

VOID DeleteINP3Routes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	char Call1[10];
	char Call2[10];

	Call1[ConvFromAX25(Route->NEIGHBOUR_CALL, Call1)] = 0;

	if (DEBUGINP3) Debugprintf("Deleting INP3 routes via %s", Call1);

	// Delete any INP3 Dest entries via this Route

	Route->SRTT = 0;
	Route->RTT = 0;
	Route->BCTimer = 0;
	Route->Status = 0;
	Route->Timeout = 0;

	Dest--;

	// Delete any Dest entries via this Route

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry

		if (Dest->NRROUTE[0].ROUT_OBSCOUNT >= 128)	 // Not if locked
			continue;

		Call2[ConvFromAX25(Dest->DEST_CALL, Call2)] = 0;

		if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == Route)
		{
			//	We are deleting the best INP3 route, so need to tell other nodes
			//	If this is the only one, we need to keep the entry with at 60000 rtt so
			//	we can send it. Remove when all gone

			//	How do we indicate is is dead - Maybe the 60000 is enough!

			// If we are cleaning up after a sabm on an existing link (frmr or other end reloaded) then we don't need to tell anyone - the routes should be reestablished very quickly

			if (DEBUGINP3) Debugprintf("Deleting First INP3 Route to %s", Call2);

			if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == 0)
			{

				// Only entry
				Dest->INP3ROUTE[0].SRTT = 60000;
				Dest->INP3ROUTE[0].Hops = 255;

				if (DEBUGINP3) Debugprintf("Was the only INP3 route");

				if (Dest->DEST_ROUTE == 4)			// we were using it
					Dest->DEST_ROUTE = 0;

				continue;
			}

			Dest->INP3ROUTE[1].LastRTT = Dest->INP3ROUTE[0].SRTT;		// So next scan will check if rtt has increaced enough to need a RIF
			memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
			memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
			memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));

			continue;
		}

		// If we aren't removing the best, we don't need to tell anyone.
		
		if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == Route)
		{
			if (DEBUGINP3) Debugprintf("Deleting 2nd INP3 Route to %s", Call2);
			memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
			memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));

			continue;
		}

		if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == Route)
		{
			if (DEBUGINP3) Debugprintf("Deleting 3rd INP3 Route to %s", Call2);
			memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
			continue;
		}
	}

	// I think we should send Negative info immediately

	SendNegativeInfo();
}

VOID DecayNETROMRoutes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DESTS;

	Dest--;

	// Decay any NETROM Dest entries via this Route. If OBS reaches zero, remove

	// OBSINIT is probably too many retries. Try decrementing by 2.

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry

		if (Dest->NRROUTE[0].ROUT_NEIGHBOUR == Route)
		{
			if (Dest->NRROUTE[0].ROUT_OBSCOUNT && Dest->NRROUTE[0].ROUT_OBSCOUNT < 128)	 // Not if locked
			{
				Dest->NRROUTE[0].ROUT_OBSCOUNT--;
				if (Dest->NRROUTE[0].ROUT_OBSCOUNT)
					Dest->NRROUTE[0].ROUT_OBSCOUNT--;

			}
			if (Dest->NRROUTE[0].ROUT_OBSCOUNT == 0)
			{
				// Route expired

				if (Dest->NRROUTE[1].ROUT_NEIGHBOUR == 0)			// No more Netrom Routes
				{
					if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == 0)			// Any INP3 ROutes?
					{
						// No More Routes - ZAP Dest

						REMOVENODE(Dest);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry	
						continue;
					}
					else
					{
						// Still have an INP3 Route - just zap this entry

						memset(&Dest->NRROUTE[0], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
						continue;

					}
				}

				memcpy(&Dest->NRROUTE[0], &Dest->NRROUTE[1], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memcpy(&Dest->NRROUTE[1], &Dest->NRROUTE[2], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

				continue;
			}
		}
		
		if (Dest->NRROUTE[1].ROUT_NEIGHBOUR == Route)
		{
			Dest->NRROUTE[1].ROUT_OBSCOUNT--;

			if (Dest->NRROUTE[1].ROUT_OBSCOUNT == 0)
			{
				memcpy(&Dest->NRROUTE[1], &Dest->NRROUTE[2], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

				continue;
			}
		}

		if (Dest->NRROUTE[2].ROUT_NEIGHBOUR == Route)
		{
			Dest->NRROUTE[2].ROUT_OBSCOUNT--;

			if (Dest->NRROUTE[2].ROUT_OBSCOUNT == 0)
			{
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
				continue;
			}
		}
	}
}


VOID TellINP3LinkSetupFailed(struct ROUTE * Route)
{
	// Attempt to activate Neighbour failed
	
//	char call[11]="";

//	ConvFromAX25(Route->NEIGHBOUR_CALL, call);
//	Debugprintf("BPQ32 L2 Link to Neighbour %s setup failed", call);


	if (Route->INP3Node == 0)
		DecayNETROMRoutes(Route);
	else
		DeleteINP3Routes(Route);
}

VOID ProcessRTTReply(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Buff)
{
	int RTT;
	unsigned int OrigTime;

	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Route->Timeout = 0;			// Got Response
	
	sscanf(&Buff->L4DATA[6], "%d", &OrigTime);
	RTT = (int)((GetTickCount() - INP3timeLoadedMS) / 10 - (OrigTime));		// We work internally in mS

	if (RTT > 60000)
		return;					// Ignore if more than 60 secs (why ??)

	Route->RTT = RTT;

	if (Route->SRTT == 0)
		Route->SRTT = RTT;
	else
		Route->SRTT = ((Route->SRTT * 80)/100) + ((RTT * 20)/100);

	Route->RTTIncrement = Route->SRTT / 2;		// Half for one way time.

	if ((Route->Status & GotRTTResponse) == 0)
	{
		// Link is just starting

		if (DEBUGINP3) Debugprintf("INP3 got first RTT reply from %s - Link is (Re)staring", Normcall);

		Route->Status |= GotRTTResponse;
	}

}

VOID ProcessINP3RIF(struct ROUTE * Route, UCHAR * ptr1, int msglen, int Port)
{
	unsigned char axcall[7];
	int hops;
	unsigned short rtt;
	int len;
	int opcode;
	char alias[6];
	UINT Stamp, HH, MM;
	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;
	if (DEBUGINP3) Debugprintf("Processing RIF from %s INP3Node %d Route SRTT %d", Normcall, Route->INP3Node, Route->SRTT);

	if (Route->SRTT == 0)
		if (DEBUGINP3) Debugprintf("INP3 Zero SRTT");


#ifdef NOINP3

	return;

#endif

	if (Route->INP3Node == 0)
		return;						// We don't want to use INP3

	// Update Timestamp on Route

	Stamp = time(NULL) % 86400;		// Secs into day
	HH = Stamp / 3600;

	Stamp -= HH * 3600;
	MM = Stamp  / 60;

	Route->NEIGHBOUR_TIME = 256 * HH + MM;

	while (msglen > 0)
	{
		if (msglen < 10)
		{
			if (DEBUGINP3) Debugprintf("Corrupt INP3 Message");
			return;
		}

		memset(alias, ' ', 6);	
		memcpy(axcall, ptr1, 7);

		if (axcall[0] < 0x60 || (axcall[0] & 1))		// Not valid ax25 callsign
			return;					// Corrupt RIF
	
		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		// rtt is value from remote node. Add our RTT to that node

		// if other end is old bpq then value is mS otherwise 10 mS unita

		if (Route->OldBPQ)
			rtt /= 10;

//		rtt += Route->SRTT;		// Don't do this - other end has added linkrtt

		msglen -= 10;

		while (*ptr1 && msglen > 0)
		{
			len = *ptr1;
			opcode = *(ptr1+1);

			if (len < 2 || len > msglen)
				return;				// Duff RIF

			if (opcode == 0)
			{
				if (len > 1 && len < 9)
					memcpy(alias, ptr1+2, len-2);
				else
				{
					if (DEBUGINP3) Debugprintf("Corrupt INP3 Message");
					return;
				}
			}
			ptr1+=len;
			msglen -=len;
		}

		ptr1++;
		msglen--;		// EOP

		UpdateNode(Route, axcall, alias, hops, rtt);
	}
	
	return;
}

VOID KillRoute(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR)
{
}


VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt)
{
	struct DEST_LIST * Dest;
	struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR;
	int i;
	char call[11]="";
	APPLCALLS * APPL;
	int App;
	char Normcall[10];
	Normcall[ConvFromAX25(axcall, Normcall)] = 0;


//	SEE IF any of OUR CALLs - DONT WANT TO PUT IT IN LIST!

	if (CompareCalls(axcall, NETROMCALL))
	{
		if (DEBUGINP3) Debugprintf("INP3 for our Nodecall - discarding");
		return;
	}

	if (CheckExcludeList(axcall) == 0)
	{
		if (DEBUGINP3) Debugprintf("INP3 excluded - discarding");
		return;
	}
	
	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (CompareCalls(axcall, APPL->APPLCALL))
		{
			if (DEBUGINP3) Debugprintf("INP3 for an APPLCALL - discarding");
			return;
		}
	}


	if (hops > MaxHops && hops < 255)
	{
		ConvFromAX25(axcall, call);
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - Hop Count too high", call, hops, rtt);
		return;
	}

	if (rtt > MAXRTT  && rtt < 60000)
	{
		ConvFromAX25(axcall, call);
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - rtt too high", call, hops, rtt);
		return;
	}

	if (rtt >= 60000)
	{
		if (DEBUGINP3) Debugprintf("INP3 RTT > 60000 - discarding");
		return;	
	}

	if (FindDestination(axcall, &Dest))
		goto Found;

	if (Dest == NULL)
	{
		if (DEBUGINP3) Debugprintf("INP3 Table Full - discarding");
		return;	// Table Full
	}
	
	

	memset(Dest, 0, sizeof(struct DEST_LIST));

	memcpy(Dest->DEST_CALL, axcall, 7);
	memcpy(Dest->DEST_ALIAS, alias, 6);

//	Set up First Route

	Dest->INP3ROUTE[0].Hops = hops;
	Dest->INP3ROUTE[0].SRTT = rtt;
	Dest->INP3ROUTE[0].LastRTT = 0;

	Dest->INP3FLAGS = NewNode;

	Dest->INP3ROUTE[0].ROUT_NEIGHBOUR = Route;

	NUMBEROFNODES++;

	ConvFromAX25(Dest->DEST_CALL, call);
	if (DEBUGINP3) Debugprintf("INP3 Adding New Node %s Hops %d RTT %d", call, hops, rtt);

	return;

Found:

	if (Dest->DEST_STATE & 0x80)	// Application Entry
	{
		if (DEBUGINP3) Debugprintf("INP3 Application Entry - discarding");
		return;	
	}

	// Update ALIAS

	ConvFromAX25(Dest->DEST_CALL, call);
	if (DEBUGINP3) Debugprintf("INP3 Updating Node %s Hops %d RTT %d", call, hops, rtt);

	if (alias[0] > ' ')
		memcpy(Dest->DEST_ALIAS, alias, 6);

	// See if we are known to it, it not add

	ROUTEPTR = &Dest->INP3ROUTE[0];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[0] - updating");
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[1];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[1] - updating");
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[2];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[2] - updating");
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	// Not in list. If any spare, add.
	// If full, see if this is better

	for (i = 0; i < 3; i++)
	{
		ROUTEPTR = &Dest->INP3ROUTE[i];
		
		if (ROUTEPTR->ROUT_NEIGHBOUR == NULL)
		{
			// Add here

			if (DEBUGINP3) Debugprintf("INP3 adding as route[%d]", i);
			AddHere(ROUTEPTR, Route, hops, rtt);
			SortRoutes(Dest);
			return;
		}
		ROUTEPTR++;
	}

	if (DEBUGINP3) Debugprintf("INP3 All entries in use - see if this is better than existing");

	// Full, see if this is better

	// Note that wont replace any netrom routes with INP3 ones unless we add pseudo rtt values to netrom entries

	if (Dest->INP3ROUTE[0].SRTT > rtt)
	{
		// We are better. Move others down and add on front

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 0");

		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[0], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[1].SRTT > rtt)
	{
		// We are better. Move  2nd down and add

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 1");
		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[1], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[2].SRTT > rtt)
	{
		// We are better. Add here

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 2");
		AddHere(&Dest->INP3ROUTE[2], Route, hops, rtt);
		return;
	}


	if (DEBUGINP3) Debugprintf("INP3 Worse that any existing route");


	// Worse than any - ignore

}

VOID AddHere(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt)
{
	ROUTEPTR->Hops = hops;
	ROUTEPTR->SRTT = rtt;
	ROUTEPTR->LastRTT = 0;
	ROUTEPTR->RTT = 0;
	ROUTEPTR->ROUT_NEIGHBOUR = Route;

	return;
}


/*	LEA	EDI,DEST_CALL[EBX]
	MOV	ECX,7
	REP MOVSB

	MOV	ECX,6			; ADD ALIAS
	MOV	ESI,OFFSET32 TEMPFIELD
	REP MOVSB

	POP	ESI
;
;	GET _NEIGHBOURS FOR THIS DESTINATION
;
	CALL	CONVTOAX25
	JNZ SHORT BADROUTE
;
	CALL	GETVALUE
	MOV	_SAVEPORT,AL		; SET PORT FOR _FINDNEIGHBOUR

	CALL	GETVALUE
	MOV	_ROUTEQUAL,AL
;
	MOV	ESI,OFFSET32 AX25CALL

	PUSH	EBX			; SAVE DEST
	CALL	_FINDNEIGHBOUR
	MOV	EAX,EBX			; ROUTE TO AX
	POP	EBX

	JZ SHORT NOTBADROUTE

	JMP SHORT BADROUTE

NOTBADROUTE:
;
;	UPDATE ROUTE LIST FOR THIS DEST
;
	MOV	ROUT1_NEIGHBOUR[EBX],EAX
	MOV	AL,_ROUTEQUAL
	MOV	ROUT1_QUALITY[EBX],AL
	MOV	ROUT1_OBSCOUNT[EBX],255	; LOCKED
;
	POP	EDI
	POP	EBX
	
	INC	_NUMBEROFNODES

	JMP	SENDOK
*/
	
struct INP3_DEST_ROUTE_ENTRY Temp;


VOID SortRoutes(struct DEST_LIST * Dest)
{
	 char Call1[10], Call2[10], Call3[10];


	// May now be out of order

	if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == 0)
	{
		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		if (DEBUGINP3) Debugprintf("INP3 1 route %d %s",  Dest->INP3ROUTE[0].SRTT, Call1);
		return;						// Only One, so cant be out of order
	}
	if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == 0)
	{
		// Only 2

		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	
		if (DEBUGINP3) Debugprintf("INP3 2 routes %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2);

		if (Dest->INP3ROUTE[0].SRTT <= Dest->INP3ROUTE[1].SRTT)
			return;

		// Swap one and two

		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));

		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	
		if (DEBUGINP3) Debugprintf("INP3 2 routes %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2);
		return;
	}

	// Have 3 Entries


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2, Dest->INP3ROUTE[2].SRTT, Call3);
		
	// In order?

	if (Dest->INP3ROUTE[0].SRTT <= Dest->INP3ROUTE[1].SRTT && Dest->INP3ROUTE[1].SRTT <= Dest->INP3ROUTE[2].SRTT)// In order?
		return;

	// If second is better that first swap

	if (Dest->INP3ROUTE[0].SRTT > Dest->INP3ROUTE[1].SRTT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2, Dest->INP3ROUTE[2].SRTT, Call3);

	// if 3 is better than 2 swap them. As two is worse than one. three will then be worst

	if (Dest->INP3ROUTE[1].SRTT > Dest->INP3ROUTE[2].SRTT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[2], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2, Dest->INP3ROUTE[2].SRTT, Call3);

	// 3 is now slowest. 2 could still be better than 1


	if (Dest->INP3ROUTE[0].SRTT > Dest->INP3ROUTE[1].SRTT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].SRTT, Call1, Dest->INP3ROUTE[1].SRTT, Call2, Dest->INP3ROUTE[2].SRTT, Call3);

	if (Dest->INP3ROUTE[0].SRTT <= Dest->INP3ROUTE[1].SRTT && Dest->INP3ROUTE[1].SRTT <= Dest->INP3ROUTE[2].SRTT)// In order?
		return;

	// Something went wrong

	if (DEBUGINP3) Debugprintf("INP3 Sort Failed");

}



VOID UpdateRoute(struct DEST_LIST * Dest, struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt)
{
	if (ROUTEPTR->Hops == 0)
	{
		// This is not a INP3 Route - Convert it

		ROUTEPTR->Hops = hops;
		ROUTEPTR->SRTT = rtt;

		SortRoutes(Dest);
		return;
	}

	if (rtt == 60000)
	{
		ROUTEPTR->SRTT = rtt;
		ROUTEPTR->Hops = hops;

		SortRoutes(Dest);
		return;

	}

	ROUTEPTR->SRTT = rtt;
	ROUTEPTR->Hops = hops;
	
	SortRoutes(Dest);
	return;
}

VOID ProcessRTTMsg(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Buff, int Len, int Port)
{
	int OtherRTT;
	int Dummy;
	char * ptr;
	struct _RTTMSG * RTTMsg = (struct _RTTMSG *)&Buff->L4DATA[0];
	char Normcall[10];

	if (Route->NEIGHBOUR_LINK == 0)
		return;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;

	// See if a reply to our message, or a new request

	if (memcmp(Buff->L3SRCE, MYCALL,7) == 0)
	{
		ProcessRTTReply(Route, Buff);
		ReleaseBuffer(Buff);
		return;
	}

	if (Route->NEIGHBOUR_LINK->LINKPORT && (Route->NEIGHBOUR_LINK->LINKPORT->ALLOWINP3 || Route->NEIGHBOUR_LINK->LINKPORT->ENABLEINP3))
		Route->INP3Node = 1;

	if (Route->INP3Node == 0)
	{
		if (DEBUGINP3) Debugprintf("Ignoring RTT Msg from %s - not using INP3", Normcall);
		ReleaseBuffer(Buff);
		return;						// We don't want to use INP3
	}

	// Extract other end's SRTT

	// Get SWVERSION to see if other end is old (Buggy) BPQ

	if (memcmp(RTTMsg->SWVERSION, "BPQ32001 ", 9) == 0)
		Route->OldBPQ = 1;
	else
		Route->OldBPQ = 0;

	sscanf(&Buff->L4DATA[6], "%d %d", &Dummy, &OtherRTT);

	Route->NeighbourSRTT = OtherRTT;

	// Look for $M and $H (MAXRTT MAXHOPS)

	ptr = strstr(RTTMsg->FLAGS, "$M");

	if (ptr)
		Route->RemoteMAXRTT = atoi(ptr + 2);

	ptr = strstr(RTTMsg->FLAGS, "$H");

	if (ptr)
		Route->RemoteMAXHOPS = atoi(ptr + 2);

	// Echo Back to sender

	SendNetFrame(Route, Buff);

	if ((Route->Status & GotRTTRequest) == 0)
	{
		// Link is just starting

		if (DEBUGINP3) Debugprintf("INP3 Processing first RTT frame from %s - link is (re)starting", Normcall);
		Route->Status |= GotRTTRequest;
	}
}


VOID SendRTTMsg(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg;
	char Stamp[50];
	char Normcall[10];
	unsigned char temp[256];
	uint64_t sendTime;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Msg = GetBuff();
	if (Msg == 0)
		return;

	Msg->Port = Route->NEIGHBOUR_PORT;
	Msg->L3PID = NRPID;

	memcpy(Msg->L3DEST, L3RTT, 7);
	memcpy(Msg->L3SRCE, MYCALL, 7);
	Msg->L3TTL = 2;
	Msg->L4ID = 0;
	Msg->L4INDEX = 0;
	Msg->L4RXNO = 0;
	Msg->L4TXNO = 0;
	Msg->L4FLAGS = L4INFO;

	// The timestamp can possibly exceed 10 digits. INP3 only works on differece between send and received, so base can be reset safely.

	sendTime = ((uint64_t)GetTickCount() - INP3timeLoadedMS) / 10;	// 10mS units

	if (sendTime > 9999999999)
		sendTime = INP3timeLoadedMS = 0;

	sprintf(Stamp, "%10llu %10d %10d %10d ", sendTime, Route->SRTT, Route->RTT, 0);
	
	memcpy(RTTMsg.TXTIME, Stamp, 44);

	// We now allow MAXRTT and MAXHOPS to be reconfigured so should update header each time

	sprintf(temp, "$M%d $N $H%d            ", MAXRTT, MaxHops); // trailing spaces extend to ensure padding if the length of characters for MAXRTT changes.
	memcpy(RTTMsg.FLAGS, temp, 20);                 // But still limit the actual characters copied.

	memcpy(Msg->L4DATA, &RTTMsg, 236);

	Msg->LENGTH = 256 + 1 + MSGHDDRLEN;

	Route->Timeout = RTTTimeout;

	SendNetFrame(Route, Msg);

	if (Route->Status & SentRTTRequest)
		return;	

	Route->Status |= SentRTTRequest;	

	if (DEBUGINP3) Debugprintf("INP3 Sending first RTT Msg to %s", Normcall);

}

VOID SendKeepAlive(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg = GetBuff();

	if (Msg == 0)
		return;

	Msg->L3PID = NRPID;

	memcpy(Msg->L3DEST, L3KEEP, 7);
	memcpy(Msg->L3SRCE, MYCALL, 7);
	Msg->L3TTL = 1;
	Msg->L4ID = 0;
	Msg->L4INDEX = 0;
	Msg->L4RXNO = 0;
	Msg->L4TXNO = 0;
	Msg->L4FLAGS = L4INFO;

//	Msg->L3MSG.L4DATA[0] = 'K';

	Msg->LENGTH = 20 + MSGHDDRLEN + 1;

	SendNetFrame(Route, Msg);
}

int BuildRIF(UCHAR * RIF, UCHAR * Call, UCHAR * Alias, int Hops, int RTT)
{
	int AliasLen;
	int RIFLen;
	UCHAR AliasCopy[10] = "";
	UCHAR * ptr;
	char Normcall[10];

	Normcall[ConvFromAX25(Call, Normcall)] = 0;

	if (RTT > 60000) RTT = 60000;	// Dont send more than 60000

	memcpy(&RIF[0], Call, 7);
	RIF[7] = Hops;
	RIF[8] = RTT >> 8;
	RIF[9] = RTT & 0xff;

	if (Alias)
	{
		// Need to null-terminate Alias
		
		memcpy(AliasCopy, Alias, 6);
		ptr = strchr(AliasCopy, ' ');

		if (ptr)
			*ptr = 0;

		AliasLen = (int)strlen(AliasCopy);

		RIF[10] = AliasLen+2;
		RIF[11] = 0;
		memcpy(&RIF[12], Alias, AliasLen);
		RIF[12+AliasLen] = 0;

		RIFLen = 13 + AliasLen;
		if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s:%s %d %d", AliasCopy, Normcall, Hops, RTT);
		return RIFLen;
	}

	RIF[10] = 0;

	if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s %d %d", Normcall, Hops, RTT);
	
	return (11);
}


VOID SendOurRIF(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg;
	int RIFLen;
	int totLen = 1;
	int App;
	APPLCALLS * APPL;
	char Normcall[10];

	Msg = GetBuff();
	if (Msg == 0)
		return;


	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;
	if (DEBUGINP3) Debugprintf("INP3 Sending Initial RIF to %s ", Normcall);


	Msg->L3SRCE[0] = 0xff;

	// send a RIF for our Node and all our APPLCalls

	if (Route->OldBPQ)
		RIFLen = BuildRIF(&Msg->L3SRCE[totLen], MYCALL, MYALIASTEXT, 1, Route->RTTIncrement * 10);
	else
		RIFLen = BuildRIF(&Msg->L3SRCE[totLen], MYCALL, MYALIASTEXT, 1, Route->RTTIncrement);

	totLen += RIFLen;

	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			if (Route->OldBPQ)
				RIFLen = BuildRIF(&Msg->L3SRCE[totLen], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, Route->RTTIncrement * 10);
			else
				RIFLen = BuildRIF(&Msg->L3SRCE[totLen], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, Route->RTTIncrement);
			
			totLen += RIFLen;
		}
	}

	Msg->L3PID = NRPID;
	Msg->LENGTH = totLen + 1 + MSGHDDRLEN;

	SendNetFrame(Route, Msg);
}

int SendRIPTimer()
{
	int count, nodes;
	struct ROUTE * Route = NEIGHBOURS;
	int MaxRoutes = MAXNEIGHBOURS;
	int INP3Delay;
	char Normcall[10];


	for (count=0; count<MaxRoutes; count++)
	{
		if (Route->NEIGHBOUR_CALL[0] != 0)
		{
			if (Route->NoKeepAlive)					// Keepalive Disabled
			{
				Route++;
				continue;
			}
			
			if (Route->NEIGHBOUR_LINK == 0 || Route->NEIGHBOUR_LINK->LINKPORT == 0)
			{
				if (Route->NEIGHBOUR_QUAL == 0)
				{
					Route++;
					continue;						// Qual zero is a locked out route
				}

				// Dont Activate if link has no nodes unless INP3

				if (Route->INP3Node == 0)
				{
					nodes = COUNTNODES(Route);
			
					if (nodes == 0)
					{
						Route++;
						continue;
					}
				}

				// Delay more if Locked - they could be retrying for a long time

				if (Route->ConnectionAttempts < 5)
					INP3Delay = 30;
				else
				{
					if ((Route->NEIGHBOUR_FLAG))	 // LOCKED ROUTE
						INP3Delay = 300;
					else
						INP3Delay = 120;
				}

				if (Route->LastConnectAttempt && (REALTIMETICKS - Route->LastConnectAttempt) < INP3Delay) 
				{
					Route++;
					continue;		// Not yet
				}

				// Try to activate link

				Route->ConnectionAttempts++;

				if (Route->INP3Node)
				{
					Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
					if (DEBUGINP3) Debugprintf("INP3 Activating link to %s", Normcall);
				}

				L2SETUPCROSSLINKEX(Route, 2);		// Only try SABM twice

				Route->LastConnectAttempt = REALTIMETICKS;
				
				if (Route->NEIGHBOUR_LINK == 0)
				{
					Route++;
					continue;						// No room for link
				}
			}

			if (Route->NEIGHBOUR_LINK->L2STATE != 5)	// Not up yet
			{
				Route++;
				continue;
			}

			if (Route->NEIGHBOUR_LINK->KILLTIMER > ((L4LIMIT - 60) * 3))	// IDLETIME - 1 Minute
			{
				SendKeepAlive(Route);
				Route->NEIGHBOUR_LINK->KILLTIMER = 0;		// Keep Open
			}

#ifdef NOINP3

			Route++;
			continue;

#endif
			if (Route->INP3Node)
			{
				if (Route->Timeout)
				{
					// Waiting for response

					Route->Timeout--;

					if (Route->Timeout)
					{
						Route++;
						continue;				// Wait
					}
					// No response Try again

					Route->Retries--;

					if (Route->Retries)
					{
						// More Left

						SendRTTMsg(Route);
					}
					else
					{
						// No Response - Kill all Nodes via this link

						if (Route->Status)
						{
							char Call [11] = "";

							ConvFromAX25(Route->NEIGHBOUR_CALL, Call);
							if (DEBUGINP3) Debugprintf("BPQ32 INP3 Neighbour %s Lost", Call);

							Route->Status = 0;	// Down
						}

						Route->BCTimer=5;		// Wait a while before retrying
					}
				}

				if (Route->BCTimer)
				{
					Route->BCTimer--;
				}
				else
				{
					Route->BCTimer = RTTInterval + rand() % 4;
					Route->Retries = RTTRetries;
					SendRTTMsg(Route);
				}
			}
		}

		Route++;
	}

	return (0);
}

// Create an Empty RIF

struct _L3MESSAGEBUFFER * CreateRIFHeader(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg = GetBuff();
	UCHAR AliasCopy[10] = "";

	if (Msg)
	{
		Msg->LENGTH = 1;
		Msg->L3SRCE[0] = 0xff;

		Msg->L3PID = NRPID;
	}
	return Msg;

}

VOID SendRIF(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Msg)
{
	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Msg->LENGTH += MSGHDDRLEN + 1;		// PID

	if (DEBUGINP3) Debugprintf("Sending INP3 RIF length %d to %s", Msg->LENGTH, Normcall);
	SendNetFrame(Route, Msg);
}

VOID SendRIPToOtherNeighbours(UCHAR * axcall, UCHAR * alias, struct INP3_DEST_ROUTE_ENTRY * Entry)
{
	struct ROUTE * Routes = NEIGHBOURS;
	struct _L3MESSAGEBUFFER * Msg;
	int count, MaxRoutes = MAXNEIGHBOURS;
	char Normcall[10];
	int sendHops, sendTT;

	Normcall[ConvFromAX25(axcall, Normcall)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 SendRIPToOtherNeighbours for %s", Normcall);

	for (count=0; count<MaxRoutes; count++)
	{
		if ((Routes->INP3Node) && 
			(Routes->Status) && 
			(Routes != Entry->ROUT_NEIGHBOUR))	// Dont send to originator of route
		{
			sendHops = Entry->Hops + 1;
			sendTT = Entry->SRTT +  Entry->ROUT_NEIGHBOUR->RTTIncrement;

		// send, but only if within their constraints

			if ((Routes->RemoteMAXHOPS == 0 || Routes->RemoteMAXHOPS >= Entry->Hops) && 
				(Routes->RemoteMAXRTT == 0 || Routes->RemoteMAXRTT >= Entry->SRTT  || Entry->SRTT == 60000))
			{

				Msg = Routes->Msg;

				if (Msg == NULL) 
				{
					Normcall[ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall)] = 0;
					if (DEBUGINP3) Debugprintf("INP3 Building RIF to send to %s", Normcall);
					Msg = Routes->Msg = CreateRIFHeader(Routes);
				}

				if (Msg)
				{
					if (Routes->OldBPQ)
						Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH],
							axcall, alias, sendHops, sendTT + 10);
					else
						Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH],
							axcall, alias, sendHops, sendTT);
					
					if (Msg->LENGTH > 250 - 15)
						//				if (Msg->LENGTH > Routes->NBOUR_PACLEN - 11)
					{
						SendRIF(Routes, Msg);
						Routes->Msg = NULL;
					}
				}
			}
		}
		Routes+=1;
	}
}

VOID SendRIPToNeighbour(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	struct _L3MESSAGEBUFFER * Msg;
	int sendHops, sendTT;

	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;
	if (DEBUGINP3) Debugprintf("INP3 Sending Our Table to %s ", Normcall);

	Dest--;

	// Send all entries not via this Neighbour - used when link starts

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];


		if (Entry->ROUT_NEIGHBOUR && Entry->Hops && Route != Entry->ROUT_NEIGHBOUR)	
		{
			// Best Route not via this neighbour - send, but only if within their constraints

			sendHops = Entry->Hops + 1;
			sendTT = Entry->SRTT +  Entry->ROUT_NEIGHBOUR->RTTIncrement;

			if ((Route->RemoteMAXHOPS == 0 || Route->RemoteMAXHOPS >= Entry->Hops) && 
				(Route->RemoteMAXRTT == 0 || Route->RemoteMAXRTT >= Entry->SRTT || Entry->SRTT == 60000))
			{
				Msg = Route->Msg;

				if (Msg == NULL) 
					Msg = Route->Msg = CreateRIFHeader(Route);

				if (Msg == NULL) 
					return;

				if (Route->OldBPQ)
					Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT * 10);	// old bpq bug - send mS not 10 mS units
				else
					Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT);

				if (Msg->LENGTH > 250 - 15)
				{
					SendRIF(Route, Msg);
					Route->Msg = NULL;
				}
			}
		}
	}
	if (Route->Msg)
	{
		SendRIF(Route, Route->Msg);
		Route->Msg = NULL;
	}
}

VOID FlushRIFs()
{
	struct ROUTE * Routes = NEIGHBOURS;
	int count, MaxRoutes = MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		// Make sure we've sent our local calls

		if ((Routes->Status & GotRTTRequest) && (Routes->Status & GotRTTResponse) && ((Routes->Status & SentOurRIF) == 0))
		{	
			Routes->Status |= SentOurRIF;
			SendOurRIF(Routes);
			SendRIPToNeighbour(Routes);
		}
		
		if (Routes->Msg)
		{
			char Normcall[10];

			Normcall[ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall)] = 0;
			SendRIF(Routes, Routes->Msg);
			Routes->Msg = NULL;
			if (DEBUGINP3) Debugprintf("INP3 Flushing RIF to  %s", Normcall); 
		}
		Routes+=1;
	}
}

VOID SendNegativeInfo()
{
	int i, Preload;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have got worse
	
	// ?? Should we send to one Neighbour at a time, or do all in parallel ??

	// The spec says send Negative info as soon as possible so I'll try building them in parallel
	// That will mean building several packets in parallel


	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->SRTT > Entry->LastRTT)
		{
			if (Entry->LastRTT)		// if zero haven't yet reported +ve info
			{
				if (Entry->LastRTT == 1)	// if 1, probably new, so send alias
					SendRIPToOtherNeighbours(Dest->DEST_CALL, Dest->DEST_ALIAS, Entry);
				else
					SendRIPToOtherNeighbours(Dest->DEST_CALL, 0, Entry);

				Preload = Entry->SRTT /10;
				if (Entry->SRTT < 60000)
					Entry->LastRTT = Entry->SRTT + Preload;	//10% Negative Preload
			}
		}
			
		if (Entry->SRTT >= 60000)
		{
			// It is dead, and we have reported it if necessary, so remove if no NETROM Routes

			if (Dest->NRROUTE[0].ROUT_NEIGHBOUR == 0)			// No more Netrom Routes
			{
				char call[11]="";
				ConvFromAX25(Dest->DEST_CALL, call);
				if (DEBUGINP3) Debugprintf("INP3 Deleting Node %s", call);
				REMOVENODE(Dest);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry	
			}
			else
			{
				// Have a NETROM route, so zap the INP3 one

				memset(Entry, 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
			}

			if (Dest->DEST_ROUTE == 4)			// we were using it
					Dest->DEST_ROUTE = 0;

		}
	}
}

VOID SendPositiveInfo()
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have got significantly better or are newly discovered

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];

		if (( (Entry->SRTT) && (Entry->LastRTT == 0) )|| 		// if zero haven't yet reported +ve info
			((((Entry->SRTT * 125) /100) < Entry->LastRTT) && // Better by 25%
			((Entry->LastRTT - Entry->SRTT) > 10)))			  // and 100ms
		{
			SendRIPToOtherNeighbours(Dest->DEST_CALL, 0, Entry);
			Dest->INP3ROUTE[0].LastRTT = (Dest->INP3ROUTE[0].SRTT * 11) / 10;	//10% Negative Preload
		}
	}
}

VOID SendNewInfo()
{
	int i;
	unsigned int NewRTT;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have just been added

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->INP3FLAGS & NewNode)
		{
			Dest->INP3FLAGS &= ~NewNode;
			
			Entry = &Dest->INP3ROUTE[0];

			SendRIPToOtherNeighbours(Dest->DEST_CALL, Dest->DEST_ALIAS, Entry);

			NewRTT = (Entry->SRTT * 11) / 10;
			Entry->LastRTT = NewRTT;	//10% Negative Preload
		}
	}
}


VOID INP3TIMER()
{
	if (RTTMsg.ID[0] == 0)
		InitialiseRTT();

	// Called once per second

#ifdef NOINP3

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;
		SendRIPTimer();
	}
	else
		RIPTimerCount--;

	return;

#endif

	SendNegativeInfo();					// Urgent

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;
		SendNewInfo();					// Not quite so urgent
		SendRIPTimer();
	}
	else
		RIPTimerCount--;

	if (PosTimerCount == 0)
	{
		PosTimerCount = 300;			// 5 mins
		SendPositiveInfo();
	}
	else
		PosTimerCount--;

	FlushRIFs();

}


UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, int msglen)
{
	char call[10];
	int calllen;
	int hops;
	unsigned short rtt;
	unsigned int len;
	unsigned int opcode;
	char alias[10] = "";
	UCHAR IP[6];
	int i;

	ptr2+=sprintf(ptr2, " INP3 RIF:\r Alias  Call  Hops  RTT\r");

	while (msglen > 0)
	{
		calllen = ConvFromAX25(ptr1, call);
		call[calllen] = 0;

		// Validate the call

		for (i = 0; i < calllen; i++)
		{
			if (!isupper(call[i]) && !isdigit(call[i]) && call[i] != '-')
			{
				ptr2+=sprintf(ptr2, " Corrupt RIF\r");
				return ptr2;
			}
		}

		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		IP[0] = 0;
		strcpy(alias, "      ");

		msglen -= 10;

		// Process optional fields

		while (*ptr1 && msglen > 0)			//  Have an option
		{
			len = *ptr1;
			opcode = *(ptr1+1);

			if (len < 2 || len > msglen)
			{
				ptr2+=sprintf(ptr2, " Corrupt RIF\r");
				return ptr2;
			}
			if (opcode == 0 && len < 9)
			{
				memcpy(&alias[6 - (len - 2)], ptr1+2, len-2);		// Right Justiify
			}
			else if (opcode == 1 && len < 8)
			{
				memcpy(IP, ptr1+2, len-2);
			}

			ptr1 += len;
			msglen -= len;
		}

		ptr2+=sprintf(ptr2, " %s:%s %d %4.2d\r", alias, call, hops, rtt);

		ptr1++;
		msglen--;		// Over EOP

	}
	return ptr2;
}

// Paula's conversion of rtt to Quality

int inp3_tt2qual (int tt, int hops) 
{
	int qual;

	if (tt >= 60000 || hops > 30) 
		return(0);
	
	qual = 254 - (tt/20);
	
	if (qual > 254 - hops)
		qual = 254 - hops;
	
	if (qual < 0) 
		qual=0;
	
	return(qual); 
} 

