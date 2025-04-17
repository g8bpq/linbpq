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

	Paula (G8PZT)'s Remote Host Protocol interface. 
	For now only sufficient support for WhatsPac
	
 	
*/
#define _CRT_SECURE_NO_DEPRECATE

#include "cheaders.h"
#include "bpq32.h"
#include "telnetserver.h"

int FindFreeStreamNoSem();
DllExport int APIENTRY DeallocateStream(int stream);
int SendMsgNoSem(int stream, char * msg, int len);

static void GetJSONValue(char * _REPLYBUFFER, char * Name, char * Value, int Len);
static int GetJSONInt(char * _REPLYBUFFER, char * Name);

// Generally Can have multiple RHP connections and each can have multiple RHF Sessions

struct RHPSessionInfo
{  
	struct ConnectionInfo * sockptr;
	SOCKET Socket;				// Websocks Socket
    int BPQStream;
	int Handle;				// RHP session ID
	int Seq;
    char Local[12];
    char Remote[12];
    BOOL Connecting;		// Set while waiting for connection to complete
    BOOL Listening; 
	BOOL Connected;
	int Busy;
};

struct RHPConnectionInfo
{
    SOCKET socket;
	struct RHPSessionInfo ** RHPSessions;
	int NumberofRHPSessions;
};

// Struct passed by beginhread

struct RHPParamBlock
{
	unsigned char * Msg;
	int Len;
	SOCKET Socket;
	struct ConnectionInfo * sockptr;
};



//struct RHPConnectionInfo ** RHPSockets = NULL;
//int NumberofRHPConnections = 0;

struct RHPSessionInfo ** RHPSessions;
int NumberofRHPSessions;

char ErrCodes[18][24] = 
{
	"Ok",
	"Unspecified",
	"Bad or missing type",
	"Invalid handle",
	"No memory",
	"Bad or missing mode",
	"Invalid local address",
	"Invalid remote address" ,
	"Bad or missing family" ,
	"Duplicate socket"   ,
	"No such port"    ,
	"Invalid protocol"      , 
	"Bad parameter" ,
	"No buffers"  ,
	"Unauthorised"  ,
	"No Route"  ,
	"Operation not supported"};




extern char pgm[256];	

SOCKET agwsock;

extern int SemHeldByAPI;

char szBuff[80];

int WhatsPacConfigured = 1;


int RHPPaclen = 236;


int processRHCPOpen(struct ConnectionInfo * sockptr, SOCKET Socket, char * Msg, char * ReplyBuffer);
int processRHCPSend(SOCKET Socket, char * Msg, char * ReplyBuffer);
int processRHCPClose(SOCKET Socket, char * Msg, char * ReplyBuffer);
int processRHCPStatus(SOCKET Socket, char * Msg, char * ReplyBuffer);



void SendWebSockMessage(SOCKET socket, char * Msg, int Len)
{
	int Loops = 0;
	int Sent;
	int TxLen;
	char * OutBuffer = Msg;

	// WebSock Encode. Buffer has 10 bytes on front for header but header len depends on Msg len

	if (Len < 126)
	{
		// Two Byte Header

		OutBuffer[8] = 0x81;		// Fin, Data
		OutBuffer[9] = Len;

		TxLen = Len + 2;
		OutBuffer = &Msg[8];
	}
	else if (Len < 65536)
	{
		OutBuffer[6] = 0x81;		// Fin, Data
		OutBuffer[7] = 126;			// Unmasked, Extended Len 16
		OutBuffer[8] = Len >> 8;
		OutBuffer[9] = Len & 0xff;
		TxLen = Len + 4;
		OutBuffer = &Msg[6];
	}
	else
	{
		OutBuffer[0] = 0x81;		// Fin, Data
		OutBuffer[1] = 127;			// Unmasked, Extended Len 64 bits
		// Len is 32 bits, so pad with zeros
		OutBuffer[2] = 0;
		OutBuffer[3] = 0;
		OutBuffer[4] = 0;
		OutBuffer[5] = 0;
		OutBuffer[6] = (Len >> 24) & 0xff;
		OutBuffer[7] = (Len >> 16) & 0xff;
		OutBuffer[8] = (Len >> 8) & 0xff;
		OutBuffer[9] = Len & 0xff;

		TxLen = Len +  + 10;
		OutBuffer = &Msg[0];
	}

	// Send may block

	Sent = send(socket, OutBuffer, TxLen, 0);

	while (Sent != TxLen && Loops++ < 3000)					// 100 secs max
	{	
		if (Sent > 0)					// something sent
		{
			TxLen -= Sent;
			memmove(OutBuffer, &OutBuffer[Sent], TxLen);
		}	

		Sleep(30);
		Sent = send(socket, OutBuffer, TxLen, 0);
		if (Sent == -1)
			break;
	}

	free(Msg);
	return;
}

void ProcessRHPWebSock(struct ConnectionInfo * sockptr, SOCKET Socket, char * Msg, int MsgLen);

void RHPThread(void * Params)
{
	// Params and data buffer are malloced blocks so free when done with it

	struct RHPParamBlock * Block = (struct RHPParamBlock *)Params;

	ProcessRHPWebSock(Block->sockptr, Block->Socket, Block->Msg, Block->Len);

	free(Block->Msg);
	free(Params);
}

int RHPProcessHTTPMessage(void * conn, char * response, char * Method, char * URL, char * request, BOOL LOCAL, BOOL COOKIE)
{
	// RHP messages can be sent over Websocks or normal http but I think WhatsPac only uses WebSocks
	return 0;
}

void ProcessRHPWebSock(struct ConnectionInfo * sockptr, SOCKET Socket, char * Msg, int MsgLen)
{
	int Loops = 0;
	int InputLen = 0;
	int Len;

	char Value[16];
	char * OutBuffer = malloc(250000);

//	struct RHPConnectionInfo * RHPSocket = NULL;
//	int n;

	Msg[MsgLen] = 0;

	// Find Connection Record. If none, create one

	// I dont think I need connection records

/*
	for (n = 0; n < NumberofRHPConnections; n++)
	{
		if (RHPSockets[n]->socket == socket)
		{
			RHPSocket = RHPSockets[n];
			break;
		}
	}

	if (RHPSocket == 0)
	{
		// See if there is an old one we can reuse

		for (n = 0; n < NumberofRHPConnections; n++)
		{
			if (RHPSockets[n]-Socket  == -1)
			{
				RHPSocket = RHPSockets[n];
				RHP

				break;
			}
		}

		if (RHPSocket == 0)

		NumberofRHPConnections;
		RHPSockets = realloc(RHPSockets, sizeof(void *) * (NumberofRHPConnections + 1));
		RHPSocket = RHPSockets[NumberofRHPConnections] = zalloc(sizeof (struct RHPConnectionInfo));
		NumberofRHPConnections++;
		RHPSocket->socket = socket;
	}
*/

//	{"type":"open","id":5,"pfam":"ax25","mode":"stream","port":"1","local":"G8BPQ","remote":"G8BPQ-2","flags":128}
//	{"type": "openReply", "id": 82, "handle": 1, "errCode": 0, "errText": "Ok"}
//	{"seqno": 0, "type": "status", "handle": 1, "flags": 0}
//	("seqno": 1, "type": "close", "handle": 1}
//	{"id":40,"type":"close","handle":1}

//	{"seqno": 0, "type": "status", "handle": 1, "flags": 2}.~.
//	{"seqno": 1, "type": "recv", "handle": 1, "data": "Welcome to G8BPQ's Test Switch in Nottingham \rType ? for list of available commands.\r"}.

//	{"type": "status", "handle": 0}. XRouter will reply with {"type": "statusReply", "handle": 0, "errcode": 12, "errtext": "invalid handle"}. It
//	{type: 'keepalive'} if there has been no other activity for nearly 3 minutes. Replies with {"type": "keepaliveReply"}

	GetJSONValue(Msg, "\"type\":", Value, 15);

	if (_stricmp(Value, "open") == 0)
	{
		Len = processRHCPOpen(sockptr, Socket, Msg, &OutBuffer[10]);		// Space at front for WebSock Header
		if (Len)
			SendWebSockMessage(Socket, OutBuffer, Len);
		return;
	}

	if (_stricmp(Value, "send") == 0)
	{
		Len = processRHCPSend(Socket, Msg, &OutBuffer[10]);		// Space at front for WebSock Header
		SendWebSockMessage(Socket, OutBuffer, Len);
		return;
	}
	
	if (_stricmp(Value, "close") == 0)
	{
		Len = processRHCPClose(Socket, Msg, &OutBuffer[10]);		// Space at front for WebSock Header
		SendWebSockMessage(Socket, OutBuffer, Len);
		return;
	}

	if (_stricmp(Value, "status") == 0)
	{
		Len = processRHCPStatus(Socket, Msg, &OutBuffer[10]);		// Space at front for WebSock Header
		SendWebSockMessage(Socket, OutBuffer, Len);
		return;
	}
	
	if (_stricmp(Value, "keepalive") == 0)
	{
		Len = sprintf(&OutBuffer[10], "{\"type\": \"keepaliveReply\"}"); // Space at front for WebSock Header
		SendWebSockMessage(Socket, OutBuffer, Len);
		return;
	}

	Debugprintf("Unrecognised RHP Message - %s", Msg);
}

void ProcessRHPWebSockClosed(SOCKET socket)
{
	// Close any connections on this scoket and delete socket entry

	struct RHPSessionInfo * RHPSession = 0;
	int n;

	// Find and close any Sessions

	for (n = 0; n < NumberofRHPSessions; n++)
	{
		if (RHPSessions[n]->Socket == socket)
		{
			RHPSession = RHPSessions[n];

			if (RHPSession->BPQStream)
			{
				Disconnect(RHPSession->BPQStream);
				DeallocateStream(RHPSession->BPQStream);

				RHPSession->BPQStream = 0;

			}

			RHPSession->Connecting = 0;

			// We can't send a close to RHP endpont as socket has gone

			RHPSession->Connected = 0;
		}
	}
}




int processRHCPOpen(struct ConnectionInfo * sockptr, SOCKET Socket, char * Msg, char * ReplyBuffer)
{
	//{"type":"open","id":5,"pfam":"ax25","mode":"stream","port":"1","local":"G8BPQ","remote":"G8BPQ-2","flags":128}

	struct RHPSessionInfo * RHPSession = 0;

	char * Value = malloc(strlen(Msg));	// Will always be long enough
	int ID;

	char pfam[16];
	char Mode[16];
	int Port;
	char Local[16];
	char Remote[16];
	int flags;
	int Handle = 1;
	int Stream;
	unsigned char AXCall[10];

	int n;

	// ID seems to be used for control commands like open. SeqNo for data within a session (i Think!

	ID = GetJSONInt(Msg, "\"id\":");
	GetJSONValue(Msg, "\"pfam\":", pfam, 15);
	GetJSONValue(Msg, "\"mode\":", Mode, 15);
	Port = GetJSONInt(Msg, "\"port\":");
	GetJSONValue(Msg, "\"local\":", Local, 15);
	GetJSONValue(Msg, "\"remote\":", Remote, 15);
	flags = GetJSONInt(Msg, "\"flags\":");

	if (_stricmp(pfam, "ax25") != 0)
		return sprintf(ReplyBuffer, "{\"type\": \"openReply\", \"id\": %d, \"handle\": %d, \"errCode\": 12, \"errText\": \"Bad parameter\"}", ID, 0);

	if (_stricmp(Mode, "stream") == 0)
	{
		{
			// Allocate a RHP Session

			// See if there is an old one we can reuse

			for (n = 0; n < NumberofRHPSessions; n++)
			{
				if (RHPSessions[n]->BPQStream == 0)
				{
					RHPSession = RHPSessions[n];
					Handle = n + 1;
					Stream = RHPSessions[n]->BPQStream;

					break;
				}
			}

			if (RHPSession == 0)
			{
				RHPSessions = realloc(RHPSessions, sizeof(void *) * (NumberofRHPSessions + 1));
				RHPSession = RHPSessions[NumberofRHPSessions] = zalloc(sizeof (struct RHPSessionInfo));
				NumberofRHPSessions++;

				Handle = NumberofRHPSessions;
			}

			strcpy(pgm, "RHP");
			Stream = FindFreeStream();
			strcpy(pgm, "bpq32.exe");

			if (Stream == 255)
				return sprintf(ReplyBuffer, "{\"type\": \"openReply\", \"id\": %d, \"handle\": %d, \"errCode\": 12, \"errText\": \"Bad parameter\"}", ID, 0);

			RHPSession->BPQStream = Stream;
			RHPSession->Handle = Handle;
			RHPSession->Connecting = TRUE;
			RHPSession->Socket = Socket;
			RHPSession->sockptr = sockptr;

			strcpy(RHPSession->Local, Local);
			strcpy(RHPSession->Remote, Remote);

			Connect(Stream);

			ConvToAX25(Local, AXCall);
			ChangeSessionCallsign(Stream, AXCall);

			return sprintf(ReplyBuffer, "{\"type\": \"openReply\", \"id\": %d, \"handle\": %d, \"errCode\": 0, \"errText\": \"Ok\"}", ID, Handle);
		}
	}
	return sprintf(ReplyBuffer, "{\"type\": \"openReply\", \"id\": %d, \"handle\": %d, \"errCode\": 12, \"errText\": \"Bad parameter\"}", ID, 0);
}

int processRHCPSend(SOCKET Socket, char * Msg, char * ReplyBuffer)
{
	// {"type":"send","handle":1,"data":";;;;;;\r","id":70}

	struct RHPSessionInfo * RHPSession;

	int ID;
	char * Data;
	char * ptr;
	unsigned char * uptr;
	int c;
	int Len;
	unsigned int HexCode1;
	unsigned int HexCode2;

	int n;

	int Handle = 1;

	Data = malloc(strlen(Msg));

	ID = GetJSONInt(Msg, "\"id\":");
	Handle = GetJSONInt(Msg, "\"handle\":");
	GetJSONValue(Msg, "\"data\":", Data, strlen(Msg) - 1);

	if (Handle < 1 || Handle > NumberofRHPSessions)
	{
		free(Data);
		return sprintf(ReplyBuffer, "{\"type\": \"sendReply\", \"id\": %d, \"handle\": %d, \"errCode\": 3, \"errtext\": \"Invalid handle\"}", ID, Handle);
	}

	RHPSession = RHPSessions[Handle - 1];

	// Look for \ escapes, Can now also get \u00c3

	ptr = Data;
	Len = strlen(Data);				// in case no escapes

	while (ptr = strchr(ptr, '\\'))
	{
		c = ptr[1];

		switch (c)
		{
		case 'r':

			*ptr = 13;
			memmove(ptr + 1, ptr + 2, strlen(ptr + 1));
			break;	

		case 'u':

			HexCode1 = HexCode2 = 0;
			
			n = toupper(ptr[2]) - '0';
			if (n > 9) n = n - 7;
			HexCode1 |= n << 4;

			n = toupper(ptr[3]) - '0';
			if (n > 9) n = n - 7;
			HexCode1 |= n;

			n = toupper(ptr[4]) - '0';
			if (n > 9) n = n - 7;
			HexCode2 |= n << 4;

			n = toupper(ptr[5]) - '0';
			if (n > 9) n = n - 7;
			HexCode2 |= n;

			if (HexCode1 == 0 || HexCode1 == 0xC2)
			{
				uptr = ptr;
				*uptr = HexCode2;
			}
			else if (HexCode1 == 0xc2)
			{
				uptr = ptr;
				*uptr = HexCode2 + 0x40;
			}

			memmove(ptr + 1, ptr + 6, strlen(ptr + 5));		
			break;

			
		case '\\':

			*ptr = '\\';
			memmove(ptr + 1, ptr + 2, strlen(ptr + 1));
			break;	

		case '"':

			*ptr = '"';
			memmove(ptr + 1, ptr + 2, strlen(ptr + 1));
			break;	
		}
		ptr++;
		Len = ptr - Data;
	}

	ptr = Data;

	while (Len > RHPPaclen)
	{
		SendMsg(RHPSession->BPQStream, ptr, RHPPaclen);
		Len -= RHPPaclen;
		ptr += RHPPaclen;
	}

	SendMsg(RHPSession->BPQStream, ptr, Len);

	free(Data);
	return sprintf(ReplyBuffer, "{\"type\": \"sendReply\", \"id\": %d, \"handle\": %d, \"errCode\": 0, \"errText\": \"Ok\", \"status\": %d}", ID, Handle, 2);
}


int processRHCPClose(SOCKET Socket, char * Msg, char * ReplyBuffer)
{

	// {"id":70,"type":"close","handle":1}


	struct RHPSessionInfo * RHPSession;

	int ID;
	int Handle = 1;

	char * OutBuffer = malloc(256);

	ID = GetJSONInt(Msg, "\"id\":");
	Handle = GetJSONInt(Msg, "\"handle\":");

	if (Handle < 1 || Handle > NumberofRHPSessions)
		return sprintf(ReplyBuffer, "{\"id\": %d, \"type\": \"closeReply\", \"handle\": %d, \"errcode\": 3, \"errtext\": \"Invalid handle\"}", ID, Handle);


	RHPSession = RHPSessions[Handle - 1];
	Disconnect(RHPSession->BPQStream);
	RHPSession->Connected = 0;
	RHPSession->Connecting = 0;

	DeallocateStream(RHPSession->BPQStream);
	RHPSession->BPQStream = 0;

	return sprintf(ReplyBuffer, "{\"id\": %d, \"type\": \"closeReply\", \"handle\": %d, \"errcode\": 0, \"errtext\": \"Ok\"}", ID, Handle);
}

int processRHCPStatus(SOCKET Socket, char * Msg, char * ReplyBuffer)
{
	// {"type": "status", "handle": 0}. XRouter will reply with {"type": "statusReply", "handle": 0, "errcode": 3, "errtext": "invalid handle"}. It

	struct RHPSessionInfo * RHPSession;
	int Handle = 0;

	Handle = GetJSONInt(Msg, "\"handle\":");

	if (Handle < 1 || Handle > NumberofRHPSessions)
		return sprintf(ReplyBuffer, "{\"type\": \"statusReply\", \"handle\": %d, \"errcode\": 3, \"errtext\": \"Invalid handle\"}", Handle);

	RHPSession = RHPSessions[Handle - 1];

	return sprintf(ReplyBuffer, "{\"type\": \"status\", \"handle\": %d, \"flags\": 2}", RHPSession->Handle);

}

char toHex[] = "0123456789abcdef";

void RHPPoll()
{
	int Stream;
	int n;
	int state, change;
	int Len;
	char * RHPMsg;
	unsigned char Buffer[2048];			// Space to escape control chars
	int pktlen, count;

	struct RHPSessionInfo * RHPSession;

	for (n = 0; n < NumberofRHPSessions; n++)
	{
		RHPSession = RHPSessions[n];
		Stream = RHPSession->BPQStream;

		// See if connected state has changed

		SessionState(Stream, &state, &change);

		if (change == 1)
		{
			if (state == 1)
			{
				// Connected

				RHPSession->Seq = 0;
				RHPSession->Connecting = FALSE;
				RHPSession->Connected = TRUE;

				RHPMsg = malloc(256);
				Len = sprintf(&RHPMsg[10], "{\"seqno\": %d, \"type\": \"status\", \"handle\": %d, \"flags\": 2}", RHPSession->Seq++, RHPSession->Handle);
				SendWebSockMessage(RHPSession->Socket, RHPMsg, Len);

				// Send RHP CTEXT

				RHPMsg = malloc(256);
				Sleep(10);			// otherwise WhatsPac doesn't display connected
				Len = sprintf(&RHPMsg[10], "{\"seqno\": %d, \"type\": \"recv\", \"handle\": %d, \"data\": \"Connected to RHP Server\\r\"}", RHPSession->Seq++, RHPSession->Handle);
				SendWebSockMessage(RHPSession->Socket, RHPMsg, Len);
			}
			else
			{
				// Disconnected. Send Close to client

				RHPMsg = malloc(256);
				Len = sprintf(&RHPMsg[10], "{\"type\": \"close\", \"seqno\": %d, \"handle\": %d}", RHPSession->Seq++, RHPSession->Handle);
				SendWebSockMessage(RHPSession->Socket, RHPMsg, Len);

				RHPSession->Connected = 0;
				RHPSession->Connecting = 0;

				DeallocateStream(RHPSession->BPQStream);
				RHPSession->BPQStream = 0;
			}
		}
		do
		{ 
			GetMsg(Stream, Buffer, &pktlen, &count);

			if (pktlen > 0)
			{
				char * ptr = Buffer;
				unsigned char c;

				Buffer[pktlen] = 0;

				RHPSession->sockptr->LastSendTime = time(NULL);


				// Message is JSON so Convert CR to \r, \ to \\ " to \"

				// Looks like I need to escape everything not between 0x20 and 0x7f eg \u00c3


				while (c = *(ptr))
				{
					switch (c)
					{
					case 13:

						memmove(ptr + 2, ptr + 1, strlen(ptr) + 1);
						*(ptr++) = '\\';
						*(ptr++) = 'r';
						break;
		
					case '"':

						memmove(ptr + 2, ptr + 1, strlen(ptr) + 1);
						*(ptr++) = '\\';
						*(ptr++) = '"';
						break;
	
					case '\\':

						memmove(ptr + 2, ptr + 1, strlen(ptr) + 1);
						*(ptr++) = '\\';
						*(ptr++) = '\\';
						break;
				
					default:

						if (c > 127)
						{
							memmove(ptr + 6, ptr + 1, strlen(ptr) + 1);
							*(ptr++) = '\\';
							*(ptr++) = 'u';
							*(ptr++) = '0';
							*(ptr++) = '0';
							*(ptr++) = toHex[c >> 4];
							*(ptr++) = toHex[c & 15];
							break;
						}
						else	
							ptr++;
					}
				}

				RHPMsg = malloc(2048);

				Len = sprintf(&RHPMsg[10], "{\"seqno\": %d, \"type\": \"recv\", \"handle\": %d, \"data\": \"%s\"}", RHPSession->Seq++, RHPSession->Handle, Buffer);
				SendWebSockMessage(RHPSession->Socket, RHPMsg, Len);

			}
    
		}
		while (count > 0);
	}
}



static void GetJSONValue(char * _REPLYBUFFER, char * Name, char * Value, int Len)
{
	char * ptr1, * ptr2;

	Value[0] = 0;

	ptr1 = strstr(_REPLYBUFFER, Name);

	if (ptr1 == 0)
		return;

	ptr1 += (strlen(Name) + 1);

//	"data":"{\"t\":\"c\",\"n\":\"John\",\"c\":\"G8BPQ\",\"lm\":1737912636,\"le\":1737883907,\"led\":1737758451,\"v\":0.33,\"cc\":[{\"cid\":1,\"lp\":1737917257201,\"le\":1737913735726,\"led\":1737905249785},{\"cid\":0,\"lp\":1737324074107,\"le\":1737323831510,\"led\":1737322973662},{\"cid\":5,\"lp\":1737992107419,\"le\":1737931466510,\"led\":1737770056244}]}\r","id":28}

	// There may be escaped " in data stream

	ptr2 = strchr(ptr1, '"');

	while (*(ptr2 - 1) == '\\')
	{
		ptr2 = strchr(ptr2 + 2, '"');
	}

		
	if (ptr2)
	{
		size_t ValLen = ptr2 - ptr1;
		if (ValLen > Len)
			ValLen = Len;

		memcpy(Value, ptr1, ValLen);
		Value[ValLen] = 0;
	}

	return;
}


static int GetJSONInt(char * _REPLYBUFFER, char * Name)
{
	char * ptr1;

	ptr1 = strstr(_REPLYBUFFER, Name);

	if (ptr1 == 0)
		return 0;

	ptr1 += (strlen(Name));

	return atoi(ptr1);
}


