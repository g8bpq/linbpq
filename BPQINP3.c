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

extern int DEBUGINP3;

int NegativePercent = 120;			// if time is 10% worse send negative info
int PositivePercent = 80;			// if time is 20% better send positive info
int NegativeDelay = 10;				// Seconds between checks for negative info - should be quite shourt
int PositiveDelay = 300;

time_t SENDRIFTIME = 0;
int RIFInterval = 60;

VOID SendNegativeInfo();
VOID SortRoutes(struct DEST_LIST * Dest);
VOID SendRTTMsg(struct ROUTE * Route);
VOID TCPNETROMSend(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame);
void NETROMCloseTCP(struct ROUTE * Route);

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
	UCHAR ID[6];
	UCHAR Space1;
	UCHAR TXTIME[10];
	UCHAR Space2;
	UCHAR SMOOTHEDRTT[10];
	UCHAR Space3;
	UCHAR LASTRTT[10];
	UCHAR Space4;
	UCHAR RTTID[10];
	UCHAR Space5;
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
VOID SendRIFToNewNeighbour(struct ROUTE * Route);
VOID DecayNETROMRoutes(struct ROUTE * Route);
VOID DeleteINP3Routes(struct ROUTE * Route);
BOOL L2SETUPCROSSLINKEX(PROUTE ROUTE, int Retries);

//#define NOINP3

struct _RTTMSG RTTMsg = {""};

//struct ROUTE DummyRoute = {"","",""};

int RIPTimerCount = 0;				// 1 sec to 10 sec counter
int PosTimerCount = 0;
int NegTimerCount = 0;

// Timer Runs every 10 Secs

extern int MAXRTT;			// 90 secs
extern int MaxHops;

extern int RTTInterval;			// 4 Minutes
int RTTRetries = 2;
int RTTTimeout = 6;				// 1 Min (Horizon is 1 min)

uint32_t RTTID = 1;

VOID InitialiseRTT()
{
	UCHAR temp[256] = "";

	SENDRIFTIME = time(NULL);

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
	Route->NeighbourSRTT = 0;
	Route->localport = 0;
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
			//	We need to keep the entry with a 60000 rtt so
			//	we can send it. Remove when all gone 

			//	How do we indicate is is dead - Maybe the 60000 is enough!

			// If we are cleaning up after a sabm on an existing link (frmr or other end reloaded) then we don't need to tell anyone - the routes should be reestablished very quickly

			if (DEBUGINP3) Debugprintf("Deleting First INP3 Route to %s", Call2);

			Dest->INP3ROUTE[0].STT = 60000;		// leave hops so we can check if we need to send

			if (DEBUGINP3) Debugprintf("Was the only INP3 route");

			if (Dest->DEST_ROUTE == 4)			// we were using it
				Dest->DEST_ROUTE = 0;

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

	NegTimerCount = NegativeDelay;
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
	uint32_t RTT;
	uint32_t OrigTime;

	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Route->Timeout = 0;			// Got Response
	
	sscanf(&Buff->L4DATA[6], "%u", &OrigTime);
	RTT = GetTickCountINP3() - OrigTime;		// We work internally in mS

	if (RTT > 60000 || RTT < 0)
		return;					// Ignore if more than 60 secs (why ??)

	if (RTT == 0)
		RTT = 1;				// Don't allow a Node TT of zero

	if (DEBUGINP3) Debugprintf("INP3 RTT reply from %s - SRTT was %d, Current RTT %d", Normcall, Route->SRTT, RTT);

	Route->RTT = RTT;

	if (Route->SRTT == 0)
		Route->SRTT = RTT;
	else
		Route->SRTT = ((Route->SRTT * 80)/100) + ((RTT * 20)/100);

	Route->RTTIncrement = Route->SRTT / 2;		// Half for one way time.

	if (Route->RTTIncrement == 0)
		Route->RTTIncrement = 1;

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

	ConvFromAX25(axcall, call);

	// We need to detect unreachable here 

	if (rtt >= 60000 || hops > 30)	// I use 255, Paula uses 31 hops for unreachable 
	{
		// node is unreachable. I need propagate it to other neighbours.

		if (DEBUGINP3) Debugprintf("INP3 Node %s is unreachable via", call);

		if (FindDestination(axcall, &Dest))
		{
			if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == Route)		// Best route
			{
				Dest->INP3ROUTE[0].STT = 60000;		// Will be removed once reported. leave hops so we can check if we need to send

				if (Dest->DEST_ROUTE == 4)			// we were using it
					Dest->DEST_ROUTE = 0;

				NegTimerCount = 0;			// Send negative info asap
				return;
			}

			if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == Route)
			{
				if (DEBUGINP3) Debugprintf("Deleting 2nd INP3 Route to %s", call);
				memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));

				return;
			}

			if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == Route)
			{
				if (DEBUGINP3) Debugprintf("Deleting 3rd INP3 Route to %s", call);
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
				return;
			}
		}

		// Not found or not in table - ignore

		return;
	}

	if (hops > MaxHops)
	{
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - Hop Count too high", call, hops, rtt);
		return;
	}

	if (rtt > MAXRTT)
	{
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - rtt too high", call, hops, rtt);
		return;
	}


	if (FindDestination(axcall, &Dest))
		goto Found;

	if (Dest == NULL)
	{
		if (DEBUGINP3) Debugprintf("INP3 Table Full - discarding");
		return;	// Table Full
	}
	
	// Adding New Node

	if (Dest->RouteLastTT)
		free(Dest->RouteLastTT);

	memset(Dest, 0, sizeof(struct DEST_LIST));


	memcpy(Dest->DEST_CALL, axcall, 7);
	memcpy(Dest->DEST_ALIAS, alias, 6);

//	Set up First Route

	Dest->RouteLastTT = (uint16_t *)zalloc(MAXNEIGHBOURS * sizeof(uint16_t));
	Dest->INP3ROUTE[0].Hops = hops;
	Dest->INP3ROUTE[0].STT = rtt;
	Dest->RouteLastTT[Route->recNum] = 0; 

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
	if (DEBUGINP3) Debugprintf("INP3 Updating Node %s Hops %d TT %d", call, hops, rtt);

	if (alias[0] > ' ')
		memcpy(Dest->DEST_ALIAS, alias, 6);

	// See if we are known to it, it not add

	ROUTEPTR = &Dest->INP3ROUTE[0];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[0] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[1];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[1] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[2];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[2] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
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
			if (i == 0)
				Dest->RouteLastTT[Route->recNum] = 0;
			SortRoutes(Dest);
			return;
		}
		ROUTEPTR++;
	}

	if (DEBUGINP3) Debugprintf("INP3 All entries in use - see if this is better than existing");

	// Full, see if this is better

	// Note that wont replace any netrom routes with INP3 ones unless we add pseudo rtt values to netrom entries

	if (Dest->INP3ROUTE[0].STT > rtt)
	{
		// We are better. Move others down and add on front

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 0");

		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[0], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[1].STT > rtt)
	{
		// We are better. Move  2nd down and add

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 1");
		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[1], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[2].STT > rtt)
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
	ROUTEPTR->STT = rtt;
	ROUTEPTR->ROUT_NEIGHBOUR = Route;

	return;
}

	
struct INP3_DEST_ROUTE_ENTRY Temp;


VOID SortRoutes(struct DEST_LIST * Dest)
{
	 char Call1[10], Call2[10], Call3[10];

	 // force route re-evaluation

	 Dest->DEST_ROUTE = 0;

	// May now be out of order

	if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == 0)
	{
		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		if (DEBUGINP3) Debugprintf("INP3 1 route %d %s",  Dest->INP3ROUTE[0].STT, Call1);
		return;						// Only One, so cant be out of order
	}
	if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == 0)
	{
		// Only 2

		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	
		if (DEBUGINP3) Debugprintf("INP3 2 routes %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2);

		if (Dest->INP3ROUTE[0].STT <= Dest->INP3ROUTE[1].STT)
			return;

		// Swap one and two

		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));

		Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	
		if (DEBUGINP3) Debugprintf("INP3 2 routes %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2);
		return;
	}

	// Have 3 Entries


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2, Dest->INP3ROUTE[2].STT, Call3);
		
	// In order?

	if (Dest->INP3ROUTE[0].STT <= Dest->INP3ROUTE[1].STT && Dest->INP3ROUTE[1].STT <= Dest->INP3ROUTE[2].STT)// In order?
		return;

	// If second is better that first swap

	if (Dest->INP3ROUTE[0].STT > Dest->INP3ROUTE[1].STT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2, Dest->INP3ROUTE[2].STT, Call3);

	// if 3 is better than 2 swap them. As two is worse than one. three will then be worst

	if (Dest->INP3ROUTE[1].STT > Dest->INP3ROUTE[2].STT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[2], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2, Dest->INP3ROUTE[2].STT, Call3);

	// 3 is now slowest. 2 could still be better than 1


	if (Dest->INP3ROUTE[0].STT > Dest->INP3ROUTE[1].STT)
	{
		memcpy(&Temp, &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(Dest->INP3ROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(Dest->INP3ROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(Dest->INP3ROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  Dest->INP3ROUTE[0].STT, Call1, Dest->INP3ROUTE[1].STT, Call2, Dest->INP3ROUTE[2].STT, Call3);

	if (Dest->INP3ROUTE[0].STT <= Dest->INP3ROUTE[1].STT && Dest->INP3ROUTE[1].STT <= Dest->INP3ROUTE[2].STT)// In order?
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
		ROUTEPTR->STT = rtt;

		SortRoutes(Dest);
		return;
	}

	if (rtt == 60000)
	{
		ROUTEPTR->STT = rtt;
		ROUTEPTR->Hops = hops;

		SortRoutes(Dest);
		return;

	}

	ROUTEPTR->STT = rtt;
	ROUTEPTR->Hops = hops;
	
	SortRoutes(Dest);
	return;
}

VOID ProcessRTTMsg(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Buff, int Len, int Port)
{
	uint32_t OtherRTT;
	uint32_t Dummy;
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

	// Check TTL

	if (Buff->L3TTL < 2)
	{
		ReleaseBuffer(Buff);
		return;
	}

	Buff->L3TTL--;

	if (Route->NEIGHBOUR_LINK->LINKPORT && (Route->NEIGHBOUR_LINK->LINKPORT->ALLOWINP3 || Route->NEIGHBOUR_LINK->LINKPORT->ENABLEINP3))
		Route->INP3Node = 1;

	if (Route->INP3Node == 0)
	{
		if (DEBUGINP3) Debugprintf("Ignoring RTT Msg from %s - not using INP3", Normcall);
		ReleaseBuffer(Buff);
		return;						// We don't want to use INP3
	}

	// Basic Validation - look for spaces in the right place

	if ((RTTMsg->Space1 | RTTMsg->Space2 | RTTMsg->Space3 | RTTMsg->Space4 | RTTMsg->Space5) != ' ')
	{
		Debugprintf("Corrupt INP3 RTT Message %s", &Buff->L4DATA[0]);
	}
	else
	{
		// Extract other end's SRTT

		// Get SWVERSION to see if other end is old (Buggy) BPQ

		if (memcmp(RTTMsg->SWVERSION, "BPQ32001 ", 9) == 0)
			Route->OldBPQ = 1;
		else
			Route->OldBPQ = 0;

		sscanf(&Buff->L4DATA[6], "%u %u", &Dummy, &OtherRTT);

		if (OtherRTT < 60000)		// Don't save suspect values
			Route->NeighbourSRTT = OtherRTT;

		if (DEBUGINP3) Debugprintf("INP3 RTT Msg from %s remote SRTT %u", Normcall, OtherRTT);

	}

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
	uint32_t sendTime;
	int n;

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

	// Windows GetTickCount wraps every 54 days or so. INP3 doesn't care, so long as the edge
	// case where timer wraps between sending msg and getting response is ignored 
	// For platform independence use GetTickCountINP3() and map as appropriate

	sendTime = GetTickCountINP3();	// 10mS units

	sprintf(Stamp, "%10u %10d %10d %10d ", sendTime, Route->SRTT, Route->RTT, RTTID++);

	n = strlen(Stamp);

	if (n != 44)
	{
		Debugprintf("Trying to send corrupt RTT message %s", Stamp);
		return;
	}
	
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

int BuildRIF(UCHAR * RIF, UCHAR * Call, UCHAR * Alias, int Hops, int RTT, char * Dest)
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
		if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s:%s %d %d to %s", AliasCopy, Normcall, Hops, RTT, Dest);
		return RIFLen;
	}

	RIF[10] = 0;

	if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s %d %d to %s", Normcall, Hops, RTT, Dest);
	
	return (11);
}


VOID SendOurRIF(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg;
	int RIFLen;
	int totLen = 1;
	int App;
	APPLCALLS * APPL;
	int sendTT = Route->RTTIncrement;
	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 Sending Our Call and Applcalls to %s ", Normcall);

	if (Route->OldBPQ)	// old bpq bug - send mS not 10 mS units
		sendTT *= 10;

	Msg = GetBuff();
	if (Msg == 0)
		return;

	Msg->L3SRCE[0] = 0xff;

	// send a RIF for our Node and all our APPLCalls

	RIFLen = BuildRIF(&Msg->L3SRCE[totLen], MYCALL, MYALIASTEXT, 1, sendTT, Normcall);
	totLen += RIFLen;

	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			RIFLen = BuildRIF(&Msg->L3SRCE[totLen], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, sendTT, Normcall);
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

				if (Route->Stopped)
				{
					Route++;
					continue;
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

				if (Route->INP3Node && ((Route->TCPPort == 0 || strcmp(Route->TCPHost, "0.0.0.0") != 0))) 
				{
					Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
					if (DEBUGINP3) Debugprintf("INP3 Activating link to %s", Normcall);
				}

				L2SETUPCROSSLINKEX(Route, 2);		// Only try SABM/XID twice
				Route->NeighbourSRTT = 0;			// just in case!
				Route->BCTimer = 0;

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
							if (DEBUGINP3) Debugprintf("BPQ32 INP3 Neighbour %s Lost (No Response to RTT)", Call);

							DecayNETROMRoutes(Route);
							DeleteINP3Routes(Route);

							Route->Status = 0;	// Down

							// close the link

							if (Route->TCPPort == 0)	// NetromTCP doesn't have a real link
							{
								Route->NEIGHBOUR_LINK->KILLTIMER = 0;
								Route->NEIGHBOUR_LINK->L2TIMER = 1;		// TO FORCE DISC
								Route->NEIGHBOUR_LINK->L2STATE = 4;		// DISCONNECTING
							}
							else
							{
								// but we should reset the TCP connection

								NETROMCloseTCP(Route);
							}
						}

						Route->BCTimer = 5;		// Wait a while before retrying
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

VOID SendRIFToOtherNeighbours(struct DEST_LIST * Dest, UCHAR * alias, struct INP3_DEST_ROUTE_ENTRY * Entry, int Negative, int portNum)
{
	UCHAR * axcall = Dest->DEST_CALL;
	struct ROUTE * Routes = NEIGHBOURS;
	struct _L3MESSAGEBUFFER * Msg;
	int count, MaxRoutes = MAXNEIGHBOURS;
	char NodeCall[10];
	char destCall[10];

	int sendHops, sendTT, lastTT;

	// if portNum is set sending a periodic refresh. Just sent to this port

	NodeCall[ConvFromAX25(axcall, NodeCall)] = 0;

	for (count = 0; count < MaxRoutes; count++)
	{
		if (Routes->INP3Node && Routes->Status && Routes != Entry->ROUT_NEIGHBOUR)
		{	
			// as the value sent will be different for each link, we need to check if change is enough here

			sendHops = Entry->Hops + 1;
			if (Entry->STT < 60000)
				sendTT = Entry->STT + Routes->RTTIncrement;
			else
				sendTT = 60000;

			lastTT = Dest->RouteLastTT[Routes->recNum];

			destCall[ConvFromAX25(Routes->NEIGHBOUR_CALL, destCall)] = 0;

			if (!portNum)
			{ 
				if (Negative)
				{
					// only send if significantly worse

					if (sendTT < (lastTT * NegativePercent) / 100)
					{
						Routes+=1;
						continue;
					}
				}
				else
				{
					// Send if significantly better

					if (sendTT > (lastTT * PositivePercent) / 100)
					{
						Routes+=1;
						continue;
					}
				}

			}

			if (DEBUGINP3) Debugprintf("INP3 SendRIFToOtherNeighbours  need to send %s to %s", NodeCall, destCall);

			// Don't send if Node is the Neighbour we are sending to

			if (memcmp(Routes->NEIGHBOUR_CALL, axcall, 7) == 0)
			{
				if (DEBUGINP3) Debugprintf("INP3 SendRIFToOtherNeighbours Don't send %s to itself", NodeCall);
				Dest->RouteLastTT[Routes->recNum] = sendTT;		// But update or we will keep re-entering
				Routes+=1;
				continue;
			}

			if (portNum && Routes->NEIGHBOUR_PORT != portNum)
			{
				Routes+=1;
				continue;
			}

			if (portNum)
				Routes->Status &= ~SentOurRIF;

			Dest->RouteLastTT[Routes->recNum] = sendTT;

			// send, but only if within their constraints

			// Does it make any sense to send a node with hopcount of say 2 which was received from a node with
			// maxhops 2. The next hop (with hopcount of 3 or above) will get it but won't be able to reply. 

			if ((Routes->RemoteMAXHOPS == 0 || Routes->RemoteMAXHOPS >= sendHops) && 
				(Routes->RemoteMAXRTT == 0 || Routes->RemoteMAXRTT >= sendTT  || sendTT == 60000))
			{
				if (sendTT == 60000)
					sendHops = 31;

				if (DEBUGINP3)
				{
					if (portNum)
						Debugprintf("INP3 %s Timer Refresh Sending to port %d", NodeCall, portNum);
					else
						Debugprintf("INP3 %s Old TT %d New %d  Sufficent change so sending ", NodeCall, lastTT, sendTT);
				}

				Msg = Routes->Msg;

				if (Msg == NULL) 
				{
					if (DEBUGINP3) Debugprintf("INP3 Building RIF to send to %s", destCall);
					Msg = Routes->Msg = CreateRIFHeader(Routes);
				}

				if (Msg)
				{
					if (Routes->OldBPQ)	// old bpq bug - send mS not 10 mS units
						sendTT *= 10;

					Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], axcall, alias, sendHops, sendTT, destCall);

					if (Msg->LENGTH > 250 - 15)
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

VOID SendRIFToNewNeighbour(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	struct _L3MESSAGEBUFFER * Msg;
	int sendHops, sendTT;
	char Normcall[10];

	if (Route->NEIGHBOUR_LINK == 0)		// shouldn't happen but to be safe..
		return;

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

			sendTT = Entry->STT + Entry->ROUT_NEIGHBOUR->RTTIncrement;
			Dest->RouteLastTT[Entry->ROUT_NEIGHBOUR->recNum] = sendTT;

			if ((Route->RemoteMAXHOPS == 0 || Route->RemoteMAXHOPS >= Entry->Hops || Entry->Hops > 30) && 
				(Route->RemoteMAXRTT == 0 || Route->RemoteMAXRTT >= Entry->STT || Entry->STT == 60000))
			{
				Msg = Route->Msg;

				if (Msg == NULL) 
					Msg = Route->Msg = CreateRIFHeader(Route);

				if (Msg == NULL) 
					return;

				if (Route->OldBPQ)	// old bpq bug - send mS not 10 mS units
					sendTT *= 10;

				Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT, Normcall);

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
	struct ROUTE * Route = NEIGHBOURS;
	int count, MaxRoutes = MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		// Make sure we've sent our local calls

		if ((Route->Status & GotRTTRequest) && (Route->Status & GotRTTResponse) && ((Route->Status & SentOurRIF) == 0))
		{	
			Route->Status |= SentOurRIF;
			SendOurRIF(Route);
			SendRIFToNewNeighbour(Route);
		}
		
		if (Route->Msg)
		{
			char Normcall[10];

			Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
			if (DEBUGINP3) Debugprintf("INP3 Flushing RIF to  %s", Normcall); 
			SendRIF(Route, Route->Msg);
			Route->Msg = NULL;
		}
		Route+=1;
	}
}

VOID SendNegativeInfo()
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	char call[11]="";

	Dest--;

	// Send RIF for any Dests that have got worse
	
	// ?? Should we send to one Neighbour at a time, or do all in parallel ??

	// The spec says send Negative info as soon as possible so I'll try building them in parallel
	// That will mean building several packets in parallel


	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)		// unused entry
			continue;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR == 0)
			continue;

		SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, TRUE, FALSE);
			
		if (Entry->STT >= 60000)
		{
			// It is dead, and we have reported it if necessary, so remove if no NETROM Routes

			// Wrong. We may have other INP3 routes. Move them up. This will delete first if only one

			// I think I need to set lastTT on all routes.

			if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == 0)			// No other INP3 routes
			{	
				if (DEBUGINP3) Debugprintf("Was the only INP3 route");
				memset(&Dest->INP3ROUTE[0], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
			}
			else
			{
				memset(Dest->RouteLastTT, 0, MAXNEIGHBOURS * sizeof(uint16_t));	// So next scan will check if rtt has increaced enough to need a RIF
				memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
				NegTimerCount = 0;			// Send negative info again asap to send new best
			}

			if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == 0 && Dest->NRROUTE[0].ROUT_NEIGHBOUR == 0)		// No INP3 and no Netrom Routes
			{
				char call[11]="";
				ConvFromAX25(Dest->DEST_CALL, call);
				if (DEBUGINP3) Debugprintf("INP3 No INP3 and no Netrom Routes left - Deleting Node %s", call);
				REMOVENODE(Dest);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry	
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

		if (Dest->DEST_CALL[0] == 0)		// unused entry
			continue;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR)
			SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, FALSE, FALSE);
	}
}

VOID SendNewInfo()
{
	int i;
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have just been added

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->INP3FLAGS & NewNode)
		{
			char call[10];
			ConvFromAX25(Dest->DEST_CALL, call);
			if (DEBUGINP3) Debugprintf("INP3 Sending New Node %s", call);
			Dest->INP3FLAGS &= ~NewNode;
			
			Entry = &Dest->INP3ROUTE[0];

			SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, TRUE, FALSE);	// Send as negative so will always be worse than zero
		}
	}
}

// Refresh RIF entries for each route. Shouldn't be necessary, but add for now

int routeCount = 0;
struct ROUTE * Route = NULL;

VOID sendAlltoOneNeigbour(struct ROUTE * Route)
{
	char Call[10];
	struct DEST_LIST * Dest =  DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	int i;
	struct _L3MESSAGEBUFFER * Msg;
	int sendHops, sendTT, lastTT;
	APPLCALLS * APPL;
	int App;

	Call[ConvFromAX25(Route->NEIGHBOUR_CALL, Call)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 Manual send RIF to %s", Call); 

	// send a RIF for our Node and all our APPLCalls

	Msg = Route->Msg;

	if (Msg == NULL) 
		Msg = Route->Msg = CreateRIFHeader(Route);

	if (Msg == 0)
		return;
				
	if (Route->OldBPQ)
		Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], MYCALL, MYALIASTEXT, 1, Route->RTTIncrement * 10, Call);
	else
		Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], MYCALL, MYALIASTEXT, 1, Route->RTTIncrement, Call);

	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			if (Route->OldBPQ)
				Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, Route->RTTIncrement * 10, Call);
			else
				Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, Route->RTTIncrement, Call);
			
		}
	}

	// Send all dests that have this route as their best inp3 route

	Dest--;

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR == 0)
			continue;

		if (Entry->ROUT_NEIGHBOUR && Route->INP3Node && Route->Status && Route != Entry->ROUT_NEIGHBOUR)	// Dont send to originator of route
		{
			// as the value sent will be different for each link, we need to check if change is enough here

			// Don't send if Node is the Neighbour we are sending to

			if (memcmp(Route->NEIGHBOUR_CALL, Dest->DEST_CALL, 7) == 0)
			{
				if (DEBUGINP3) Debugprintf("INP3 Timer RIF Don't send %s to itself", Call);
				Route++;
				continue;
			}

			sendHops = Entry->Hops + 1;
			sendTT = Entry->STT + Entry->ROUT_NEIGHBOUR->RTTIncrement;
			lastTT = Dest->RouteLastTT[Entry->ROUT_NEIGHBOUR->recNum];

			Dest->RouteLastTT[Entry->ROUT_NEIGHBOUR->recNum] = sendTT;

			// send, but only if within their constraints

			if ((Route->RemoteMAXHOPS == 0 || Route->RemoteMAXHOPS >= Entry->Hops) &&  (Route->RemoteMAXRTT == 0 || Route->RemoteMAXRTT >= sendTT))
			{	
				Msg = Route->Msg;

				if (Msg == NULL) 
					Msg = Route->Msg = CreateRIFHeader(Route);

				if (Msg)
				{
					if (Route->OldBPQ)
						sendTT *= 10;
					
					Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT, Call);

					if (Msg->LENGTH > 250 - 15)
					{
						SendRIF(Route, Msg);
						Route->Msg = NULL;
					}
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


VOID SendAllInfo()
{
	time_t Now = time(NULL);

	if (routeCount == 0)			// Not sending
	{
		if (RIFInterval == 0 || (Now - SENDRIFTIME) < RIFInterval)	// Time for new send?
			return;

		Route = NEIGHBOURS;
	}

	// Build RIF

	while (Route->INP3Node == 0)
	{
		Route++;
		routeCount++;

		if (routeCount == MAXNEIGHBOURS)
		{
			//cycle finished

			SENDRIFTIME = Now;
			routeCount = 0;
			return;
		}
	}

	sendAlltoOneNeigbour(Route);

	Route++;
	routeCount++;

	if (routeCount == MAXNEIGHBOURS)
	{
		//cycle finished

		SENDRIFTIME = Now;
		routeCount = 0;
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

	SendNewInfo();					// Need to send to set up last sent time

	if (NegTimerCount == 0)
	{
		NegTimerCount = NegativeDelay;
		SendNegativeInfo();
	}
	else
		NegTimerCount--;

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;

		SendRIPTimer();
		SendAllInfo();					// Timer Driven refresh
	}
	else
		RIPTimerCount--;

	if (PosTimerCount == 0)
	{
		PosTimerCount = PositiveDelay;
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

