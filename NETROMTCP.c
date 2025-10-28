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

/*
Netrom over TCP Support

This is intended for operation over radio links with an IP interface, eg New Packet Radio or possibly microwave links

To simplify interface to the rest of the oode dummy LINK and PORT records are created

Packet Format is Length (2 byte little endian) Call (10 bytes ASCII) NETROM L3/4 Packet, starting 0xcf (to detect framing errors).

A TCP message can contain multiple packets and/or partial packets

It uses the Telnet Server, with port defined in NETROMPORT

ROUTE definitions have an extra field, the TCP Port Number

*/

//#pragma data_seg("_BPQDATA")


#define _CRT_SECURE_NO_DEPRECATE 

#include "time.h"
#include "stdio.h"
#include <fcntl.h>					 
//#include "vmm.h"

#include "cheaders.h"
#include "asmstrucs.h"
#include "telnetserver.h"

#define	NETROM_PID 0xCF

void NETROMConnectionLost(struct ConnectionInfo * sockptr);
int DataSocket_ReadNETROM(struct ConnectionInfo * sockptr, SOCKET sock, struct NRTCPSTRUCT * Info, int portNo);
int NETROMTCPConnect(struct ROUTE * Route, struct ConnectionInfo * sockptr);
void NETROMConnected(struct ConnectionInfo * sockptr, SOCKET sock, struct NRTCPSTRUCT * Info);
VOID SendRTTMsg(struct ROUTE * Route);
BOOL FindNeighbour(UCHAR * Call, int Port, struct ROUTE ** REQROUTE);
VOID NETROMMSG(struct _LINKTABLE * LINK, L3MESSAGEBUFFER * L3MSG);
int BPQTRACE(MESSAGE * Msg, BOOL TOAPRS);
VOID L3LINKCLOSED(struct _LINKTABLE * LINK, int Reason);

struct NRTCPMsg
{
	short Length;
	char Call[10];
	unsigned char PID;
	char Packet[1024];
};

struct NRTCPSTRUCT 
{
	struct ConnectionInfo * sockptr;
	struct _LINKTABLE * LINK;				// Dummy Link Record for this ROUTE
	struct ROUTE * Route;					// May need backlink
	char Call[10];
};

struct NRTCPSTRUCT * NRTCPInfo[256] = {0};

// Do we want to use normal TCP server connections, which are limited, or our own. Let's try our own for now

struct ConnectionInfo * AllocateNRTCPRec()
{
	struct ConnectionInfo * sockptr = 0;
	struct NRTCPSTRUCT * Info;
	int i;

	for (i = 0; i < 255; i++)
	{
		if (NRTCPInfo[i] == 0)
		{
			// only allocate as many as needed

			Info = NRTCPInfo[i] = (struct NRTCPSTRUCT *)zalloc(sizeof(struct NRTCPSTRUCT));
			Info->sockptr = (struct ConnectionInfo *)zalloc(sizeof(struct ConnectionInfo));
			Info->LINK = (struct _LINKTABLE *)zalloc(sizeof(struct _LINKTABLE));
			Info->sockptr->Number = i;
		}
		else 
			Info = NRTCPInfo[i];

		sockptr = Info->sockptr;

		if (sockptr->SocketActive == FALSE)
		{
			sockptr->SocketActive = TRUE;
			sockptr->ConnectTime = sockptr->LastSendTime = time(NULL);

			Debugprintf("NRTCP Allocated %d", i);
			return sockptr;
		}
	}
	return 0;
}

void checkNRTCPSockets(int portNo)
{
	SOCKET sock;
	int Active = 0;
	SOCKET maxsock;
	int retval;
	int i;

	struct timeval timeout;
	fd_set readfd, writefd, exceptfd;

	struct ConnectionInfo * sockptr;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;				// poll

	maxsock = 0;

	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	FD_ZERO(&exceptfd);

	for (i = 0; i < 255; i++)
	{
		if (NRTCPInfo[i] == 0)
			break;					// only as many as have been used

		sockptr = NRTCPInfo[i]->sockptr;

		if (sockptr->SocketActive == 0)
			continue;

		if (sockptr->Connecting)
		{
				// look for complete or failed

			FD_SET(sockptr->socket, &writefd);
			FD_SET(sockptr->socket, &exceptfd);
		}
		else
		{
			FD_SET(sockptr->socket, &readfd);
			FD_SET(sockptr->socket, &exceptfd);
		}

		Active++;

		if (sockptr->socket > maxsock)
			maxsock = sockptr->socket;
	}

	if (Active)
	{
		retval = select((int)maxsock + 1, &readfd, &writefd, &exceptfd, &timeout);

		if (retval == -1)
		{				
			perror("data select");
			Debugprintf("NRTCP Select Error %d Active %d", WSAGetLastError(), Active);
		}
		else
		{
			if (retval)
			{
				// see who has data

				for (i = 0; i < 255; i++)
				{
					if (NRTCPInfo[i] == 0)
						break;
	
					sockptr = NRTCPInfo[i]->sockptr;

					if (sockptr->SocketActive == 0)
						continue;

					sock = sockptr->socket;

					if (FD_ISSET(sock, &writefd))
						NETROMConnected(sockptr, sock, NRTCPInfo[i]);
	
					if (FD_ISSET(sock, &readfd))
						DataSocket_ReadNETROM(sockptr, sock, NRTCPInfo[i], portNo);
				
					if (FD_ISSET(sock, &exceptfd))
						NETROMConnectionLost(sockptr);
				}
			}
		}
	}
}

int NETROMOpenConnection(struct ROUTE * Route)
{
	struct NRTCPSTRUCT * Info;
	struct ConnectionInfo * sockptr;

	Debugprintf("Opening NRTCP Connection");

	if (Route->TCPSession)
	{
		//	SESSION ALREADY EXISTS

		sockptr = Route->TCPSession->sockptr;
		
		if (sockptr->Connected || sockptr->Connecting)
			return TRUE;

		// previous connect failed
	}
	else
	{
		sockptr = AllocateNRTCPRec();

		if (sockptr == NULL)
			return 0;

		Info = Route->TCPSession = NRTCPInfo[sockptr->Number];
		memcpy(Info->Call, MYNETROMCALL, 10);
		Route->NEIGHBOUR_LINK = Info->LINK;

		Info->Route = Route;
		Info->LINK->NEIGHBOUR = Route;
		Info->LINK->LINKPORT = GetPortTableEntryFromPortNum(Route->NEIGHBOUR_PORT);
	}

	return NETROMTCPConnect(Route, sockptr);

}

void NETROMTCPResolve()
{
	struct ROUTE * Route = NEIGHBOURS;
	int n = MAXNEIGHBOURS;
	struct addrinfo hints, *res = 0;
	char PortString[20];
	int err;

	while (n--)
	{
		if (Route->TCPAddress)
		{
			// try to resolve host

			sprintf(PortString, "%d", Route->TCPPort);

			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
			hints.ai_socktype = SOCK_STREAM;
	
			getaddrinfo(Route->TCPHost, PortString, &hints, &res);

			err = WSAGetLastError();

			if (res)
			{
				Route->TCPAddress->ai_family = res->ai_family;
				Route->TCPAddress->ai_socktype = res->ai_socktype;
				Route->TCPAddress->ai_protocol = res->ai_protocol;
				Route->TCPAddress->ai_addrlen = res->ai_addrlen;
				memcpy(Route->TCPAddress->ai_addr, res->ai_addr, sizeof(struct sockaddr));
				freeaddrinfo(res);
			}
		}
		
		Route++;
	}
}

int NETROMTCPConnect(struct ROUTE * Route, struct ConnectionInfo * sockptr)
{
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	SOCKET sock;
	struct sockaddr_in sinx; 
	int addrlen=sizeof(sinx);
	char PortString[20];
	struct addrinfo * res = Route->TCPAddress;
	int Port = Route->TCPPort;

	sprintf(PortString, "%d", Port);

	// get host info, make socket, and connect it

	if (res->ai_family == 0)
	{
//		err = WSAGetLastError();
//		Debugprintf("Resolve HostName %s Failed - Error %d", Route->TCPHost, err);
		return FALSE;			// Resolve failed
	}

	sock = sockptr->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock == INVALID_SOCKET)
	{
		Debugprintf, ("Netrom over TCP Create Socket Failed");
		return FALSE;
	}

	ioctl(sock, FIONBIO, &param);

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);


	if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == 0)
	{
		//
		//	Connected successful
		//
		
		sockptr->Connected = TRUE;
		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == 10035 || err == 115 || err == 36)		//EWOULDBLOCK
		{
			//	Connect in Progress
			
			sockptr->Connecting = TRUE;
			return TRUE;
		}
		else
		{
			//	Connect failed

			closesocket(sockptr->socket);
	
			return FALSE;
		}
	}

	return FALSE;
}




void NETROMConnectionAccepted(struct ConnectionInfo * sockptr)
{
	// Not sure we can do much here until first message arrives with callsign

	sockptr->Connected = TRUE;
	Debugprintf("NRTCP Connection Accepted");
}

void NETROMConnected(struct ConnectionInfo * sockptr, SOCKET sock, struct NRTCPSTRUCT * Info)
{
	// Connection Complete

	Debugprintf("NRTCP Connected");

	sockptr->Connecting = FALSE;
	sockptr->Connected = TRUE;

	Info->LINK->L2STATE = 5;

	if (Info->Route->INP3Node)
		SendRTTMsg(Info->Route);
}

int DataSocket_ReadNETROM(struct ConnectionInfo * sockptr, SOCKET sock, struct NRTCPSTRUCT * Info, int portNo)
{
	int len=0, maxlen;
	struct NRTCPMsg * Msg;
	struct _L3MESSAGEBUFFER * L3Msg;
	struct ROUTE * Route;
	UCHAR axCall[7];
	PMESSAGE Buffer;
	
	ioctl(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len = maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len == 0)
	{
		// Failed or closed - clear connection

		NETROMConnectionLost(sockptr);
		return 0;
	}

	sockptr->InputLen += len;

	// Process data

checkLen:

	// See if we have a whole packet
	
	Msg = (struct NRTCPMsg *)&sockptr->InputBuffer[0];

	if (Msg->Length > sockptr->InputLen)		// if not got whole frame wait
		return 0;

	if (Info->Call[0] == 0)
	{
		// first packet - do we need to do anything?

		// This must be an incoming connection as Call is set before calling so need to find route record and set things up.

		Debugprintf("New NRTCP Connection from %s", Msg->Call);

		memcpy(Info->Call, Msg->Call, 10);

		ConvToAX25(Msg->Call, axCall);

		if (FindNeighbour(axCall, portNo, &Route))
		{
			Info->Route = Route;
			Route->NEIGHBOUR_LINK = Info->LINK;
			Route->NEIGHBOUR_PORT = portNo;
			Info->LINK->NEIGHBOUR = Route;
			Info->LINK->LINKPORT = GetPortTableEntryFromPortNum(portNo);
			Route->TCPSession = Info;
			Info->LINK->L2STATE = 5;
		
			if (Info->Route->INP3Node)
				SendRTTMsg(Info->Route);
		}
		else
		{
			Debugprintf("Neighbour %s port %d not found - closing connection", Msg->Call, portNo);
			closesocket(sockptr->socket);
			sockptr->SocketActive = FALSE;
			memset(sockptr, 0, sizeof(struct ConnectionInfo)); 
			Info->Call[0] = 0;
			return 0;
		}
	}


	if (memcmp(Info->Call, Msg->Call, 10) != 0)
	{
		// something wrong - maybe connection reused
	}

	// Format as if come from an ax.25 link

	L3Msg = GetBuff();

	if (L3Msg == 0)
		goto seeifMore;

	L3Msg->LENGTH = (Msg->Length - 12) + MSGHDDRLEN;
	L3Msg->Next = 0;
	L3Msg->Port = portNo;
	L3Msg->L3PID = NETROM_PID;
	memcpy(&L3Msg->L3SRCE, Msg->Packet, Msg->Length - 13);

	// Create a dummy L2 message so we can trace it

	Buffer = GetBuff();

	if (Buffer)
	{
		Buffer->CHAIN = 0;
		Buffer->CTL = 0;
		Buffer->PORT = portNo;

		ConvToAX25(Info->Call, Buffer->ORIGIN);
		ConvToAX25(MYNETROMCALL, Buffer->DEST);

		memcpy(Buffer->L2DATA, &L3Msg->L3SRCE[0], Msg->Length - 13);
		Buffer->ORIGIN[6] |= 1;					// Set end of calls
		Buffer->PID = NETROM_PID;
		Buffer->LENGTH = Msg->Length + 10;
		time(&Buffer->Timestamp);

		BPQTRACE(Buffer, FALSE);
		ReleaseBuffer(Buffer);
	}

	NETROMMSG(Info->LINK, L3Msg);

seeifMore:

	 sockptr->InputLen -= Msg->Length;

	 if (sockptr->InputLen > 0)
	 {
		 memmove(sockptr->InputBuffer, &sockptr->InputBuffer[Msg->Length], sockptr->InputLen);
		 goto checkLen;
	 }

	return 0;
}

VOID TCPNETROMSend(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame)
{
	struct NRTCPMsg Msg;
	unsigned char * Data = (unsigned char *)&Frame->L3SRCE[0];
	int DataLen = Frame->LENGTH - (MSGHDDRLEN + 1); // Not including PID
	int Ret;
	PMESSAGE Buffer;

	Msg.Length = DataLen + 13;				// include PID
	memcpy(Msg.Call, MYNETROMCALL, 10);
	Msg.PID = NETROM_PID;
	memcpy(Msg.Packet, Data, DataLen);

	if (Route->TCPSession == 0)
		return;

	Ret = send(Route->TCPSession->sockptr->socket, (char *)&Msg, DataLen + 13, 0);

	// Create a dummy L2 message so we can trace it

	Buffer = GetBuff();

	if (Buffer)
	{
		Buffer->CHAIN = 0;
		Buffer->CTL = 0;
		Buffer->PORT = Route->NEIGHBOUR_PORT | 128;		// TX Flag

		ConvToAX25(Route->TCPSession->Call, Buffer->DEST);
		ConvToAX25(MYNETROMCALL, Buffer->ORIGIN);

		memcpy(Buffer->L2DATA, &Frame->L3SRCE[0], DataLen);
		Buffer->ORIGIN[6] |= 1;					// Set end of calls
		Buffer->PID = NETROM_PID;
		Buffer->LENGTH = DataLen + 15 + MSGHDDRLEN;
		time(&Buffer->Timestamp);

		BPQTRACE(Buffer, FALSE);
		ReleaseBuffer(Buffer);
	}

}


void NETROMConnectionLost(struct ConnectionInfo * sockptr)
{
	struct NRTCPSTRUCT * Info = NRTCPInfo[sockptr->Number];
	struct ROUTE * Route;

	closesocket(sockptr->socket);

	// If there is an attached route (there should be) clear all connections	
	
	if (Info)
	{
		Route = Info->Route;

		if (sockptr->Connected)
			L3LINKCLOSED(Info->LINK, LINKLOST);

		if (sockptr->Connecting)
			L3LINKCLOSED(Info->LINK, SETUPFAILED);

		if (Route)
			Route->TCPSession = 0;

		Info->Call[0] = 0;
	}

	sockptr->SocketActive = FALSE;

	memset(sockptr, 0, sizeof(struct ConnectionInfo)); 
}

