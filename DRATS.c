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

// DRATS support code

#define _CRT_SECURE_NO_DEPRECATE

#include "CHeaders.h"

#include "bpq32.h"
#include "telnetserver.h"


/*
The header is the first 23 bytes of of the frame. The payload is the rest of the frame.

Byte 1 is a "magic" number. It is 0xDD if the payload is zlib compressed before being yencoded.
Bytes 2 and 3 is a 16 bit sequence number.
Byte 4 is a session number.
Byte 5 is a type. Still don't know the types.
Bytes 6 and 7, 16 bits, is the checksum of the data after any compression.
Bytes 8 and 9, 16 bits, is the length of the data.
bytes 10-18, 8 bits, are the source call sign.
bytes 19-25, 8 bits, are the destination call sign.
If a call sign is less than 8 characters, it is padded to fill the space with the "~" tilde character.

Frame types: (Some from sessions/chat.py)

0 - T_DEF
1 - T_PING_REQ - Ping request (Used in Test frame)
2 - T_PING_RSP - Ping response
3 - T_PING_ERS - Ping error status?
4 - T_STATUS - Status frame
5 - File Transfer
8 - Used in Test frame
254 - Apparently a warm up frame.

	T_DEF = 0
    T_PNG_REQ = 1
    T_PNG_RSP = 2
    T_PNG_ERQ = 3
    T_PNG_ERS = 4
    T_STATUS  = 5
For a station status frame, the first byte of the message is an ASCII station status value.

'0' - Unknown
'1' - Online
'2' - Unattended
'9' - Offline
*/

#define	T_DEF 0
#define T_PNG_REQ 1
#define T_PNG_RSP 2
#define T_PNG_ERQ 3
#define T_PNG_ERS 4
#define T_STATUS 5

#pragma pack(1)

// shorts are big-endian

struct DRATSHeader
{
	unsigned char Magic;
	unsigned short Seq;
	unsigned char Sessno;
	unsigned char Type;
	unsigned short CheckSum;
	unsigned short Length;
	char CallFrom[8];
	char CallTo[8];
	unsigned char Message[2048];
};

#pragma pack()

struct DRATSSession
{
	struct ConnectionInfo * sockptr;
	unsigned int Seq;
	unsigned int Sessno;
	char CallFrom[8];
	char CallTo[8];
	int Stream;					// BPQ Stream
	int StreamState;
	struct DRATSQueue * Queue;
	struct DRATSSession * Next; 
};

struct DRATSQueue
{
	// Queue of messages to be sent to node from background (ie not under semaphore)

	int Stream;
	int Len;
	unsigned char * Msg;
	struct DRATSQueue * Next;
};


struct DRATSSession * DRATSSessions = NULL; 


char peer0_2[] = { /* Packet 17 */
0x5b, 0x53, 0x4f, 0x42, 0x5d, 0xdd, 0x3d, 0x40, 
0x3d, 0x40, 0x01, 0x05, 0x45, 0x78, 0x3d, 0x40, 
0x18, 0x47, 0x38, 0x42, 0x50, 0x51, 0x7e, 0x7e, 
0x7e, 0x43, 0x51, 0x43, 0x51, 0x43, 0x51, 0x7e, 
0x7e, 0x78, 0xda, 0x33, 0xf4, 0xcf, 0xcb, 0xc9, 
0xcc, 0x4b, 0x55, 0xd0, 0x70, 0xd1, 0x0d, 0x72, 
0x0c, 0x09, 0xd6, 0x04, 0x3d, 0x40, 0x2a, 0x8c, 
0x04, 0xb3, 0x5b, 0x45, 0x4f, 0x42, 0x5d };


void processDRATSFrame(unsigned char * Message, int Len, struct ConnectionInfo * sockptr);

int testDRATS()
{
//	processDRATSFrame(peer0_1, sizeof(peer0_1), 0);
//	processDRATSFrame(peer0_2, sizeof(peer0_2), 0);
//	processDRATSFrame(peer1_1, sizeof(peer1_1), 0);
//	processDRATSFrame(peer1_20, sizeof(peer1_20));
//	processDRATSFrame(peer0_20, sizeof(peer0_20));
//	processDRATSFrame(peer1_21, sizeof(peer1_21));

	return 0;
}


extern char pgm[256];	
extern char TextVerstring[50];

int HeaderLen = offsetof(struct DRATSHeader, Message);

int doinflate(unsigned char * source, unsigned char * dest, int Len, int destlen, int * outLen);
int dratscrc(unsigned char *ptr, int count);
int FindFreeStreamNoSem();
void sendDRATSFrame(struct ConnectionInfo * sockptr, struct DRATSHeader * Header);
int yEncode(unsigned char * in, unsigned char * out, int len, unsigned char * Banned);


int AllocateDRATSStream(struct DRATSSession * Sess)
{
	int Stream;

	strcpy(pgm, "DRATS");

	Stream = FindFreeStreamNoSem();

	strcpy(pgm, "bpq32.exe");

	if (Stream == 255) return 0;

	if (memcmp(Sess->CallTo, "NODE", 4) == 0)
	{
		//  Just connect to command level on switch
	}

	return Stream;
}

void ProcessDRATSPayload(struct DRATSHeader * Header, struct DRATSSession * Sess)
{
	struct DRATSQueue * QEntry;
	BPQVECSTRUC * HOST;

	if (Sess->Stream == 0)
	{
		Sess->Stream = AllocateDRATSStream(Sess);
	}

	if (Sess->StreamState == 0)
	{
		unsigned char AXCall[10];

		Connect(Sess->Stream);				// Connect
		ConvToAX25(Sess->CallFrom, AXCall);
		ChangeSessionCallsign(Sess->Stream, AXCall);		// Prevent triggering incoming connect code

		// Clear State Changed bits (cant use SessionState under semaphore)

		HOST = &BPQHOSTVECTOR[Sess->Stream -1]; // API counts from 1
		HOST->HOSTFLAGS &= 0xFC;		  // Clear Change Bits
		Sess->StreamState = 1;
	}

	strcat(Header->Message, "\r");

	// Need to Queue to Background as we can't use SendMsg under semaphore

	QEntry = zalloc(sizeof(struct DRATSQueue));
	QEntry->Len = strlen(Header->Message);
	QEntry->Msg = malloc(QEntry->Len);
	memcpy(QEntry->Msg, Header->Message, QEntry->Len);

	// Add to queue

	if (Sess->Queue)
	{
		struct DRATSQueue * End = Sess->Queue;

		// Add on end
		while (End->Next)
			End = End->Next;

		End->Next = QEntry;
	}
	else
		Sess->Queue = QEntry;

}

// Called under semaphore


void processDRATSFrame(unsigned char * Message, int Len, struct ConnectionInfo * sockptr)
{
	unsigned char * Payload;
	unsigned char * ptr;
	unsigned char dest[2048];
	struct DRATSHeader * Header;
	int outLen;
	struct DRATSSession * Sess = DRATSSessions;
	unsigned short crc, savecrc;
	char CallFrom[10] = "";
	char CallTo[10] = "";

	Message[Len] = 0;
	Debugprintf(Message);

	Payload = strstr(Message, "[SOB]");
	
	if (Payload == 0)
		return;

	ptr = strstr(Message, "[EOB]");

	if (ptr == 0)
		return;

	ptr[0] = 0;

	Payload += 5;

	Header = (struct DRATSHeader *)Payload;

	// Undo = transparency

	ptr = Payload;

	while (ptr = strchr(ptr, '='))
	{
		memmove(ptr, ptr + 1, Len);
		ptr[0] -= 64;
		ptr++;
	}

	// Check CRC

	savecrc = htons(Header->CheckSum);
	Header->CheckSum = 0;

	crc = dratscrc(Payload, htons(Header->Length) + HeaderLen);

	if (crc != savecrc)
	{
		Debugprintf(" DRARS CRC Error %x %x", crc, savecrc);		// Good CRC
		return;
	}

	Header->Length = htons(Header->Length);		// convert to machine order

	if (Header->Magic == 0xdd)		// Zlib compressed
	{
		doinflate(Header->Message, dest,  Header->Length, 2048, &outLen);
		memcpy(Header->Message, dest, outLen + 1);
		Header->Length = outLen;
	}
	Debugprintf(Header->Message);

	// Look for a matching From/To/Session

	memcpy(CallFrom, Header->CallFrom, 8);
	memcpy(CallTo, Header->CallTo, 8);

	strlop(CallFrom, '~');
	strlop(CallTo, '~');

	if (Header->Type == T_STATUS)
	{
		// Status frame ?? What to do with it ??

		return;
	}

	if (Header->Type == T_PNG_REQ)
	{
		// "Ping Request"

		// if to "NODE" reply to it

		if (strcmp(CallTo, "NODE") == 0)
		{
			// Reuse incoming message

			strcpy(Header->CallFrom, CallTo);
			strcpy(Header->CallTo, CallFrom);
			Header->Type = T_PNG_RSP;
			Header->Length = sprintf(Header->Message, "Running BPQ32 Version %s", TextVerstring);

			sendDRATSFrame(sockptr, Header);
			return;
		}

		// Not to us - do we route it ??

		return;
	}

	if (Header->Type == T_PNG_RSP)
	{
		// Reponse is PNG_RSP then Status - 1Online (D-RATS)
		// "Running D-RATS 0.3.9  (Windows 8->10 (6, 2, 9200, 2, ''))"
		// "Running D-RATS 0.3.10 beta 4  (Linux - Raspbian GNU/Linux 9)"

		return;
	}

	if (Header->Type != T_DEF)
	{
		return;
	}

	// ?? Normal Data

	if (strcmp(CallTo, "NODE") != 0)
	{
		// Not not Node - should we route it ??

		return;
	}

	while (Sess)
	{
		if (Sess->Sessno == Header->Sessno && memcmp(Sess->CallFrom, CallFrom, 8) == 0
			&& memcmp(Sess->CallTo, CallTo, 8) == 0 && Sess->sockptr == sockptr)
		{
			ProcessDRATSPayload(Header, Sess);
			return;
		}
		Sess = Sess->Next;
	}

	// Allocate a new one

	Sess = zalloc(sizeof(struct DRATSSession));

	Sess->Sessno = Header->Sessno;
	memcpy(Sess->CallFrom, CallFrom, 8);
	memcpy(Sess->CallTo, CallTo, 8);
	Sess->sockptr = sockptr;

	if (DRATSSessions)
	{
		// Add to front of Chain

		Sess->Next = DRATSSessions;
	}
	
	DRATSSessions = Sess;	

	ProcessDRATSPayload(Header, Sess);
	return;

}

void DRATSPoll()
{
	struct DRATSSession * Sess = DRATSSessions;
	int Stream, state, change;
	int count;
	struct DRATSHeader Header;
	struct DRATSQueue * QEntry;
	struct DRATSQueue * Save;

	while (Sess)
	{
		Stream = Sess->Stream;	
		SessionState(Stream, &state, &change);
	
		if (change == 1)
		{
			if (state == 1)
			{
				// Connected - do we need anything ??
			}
			else
			{
				// Send a disconnected message

				char From[10] = "~~~~~~~~~";
				char To[10] = "~~~~~~~~~";
		
				Sess->StreamState = 0;

				Header.Length = sprintf(Header.Message, "*** Disconnected from Node");


				memcpy(To, Sess->CallFrom, strlen(Sess->CallFrom));
				memcpy(From, Sess->CallTo, strlen(Sess->CallTo));

				memcpy(Header.CallFrom, From, 8);
				memcpy(Header.CallTo, To, 8);

				Header.Magic = 0x22;
				Header.Type = 0;
				Header.Seq = 0;
				Header.Sessno = Sess->Sessno;

				sendDRATSFrame(Sess->sockptr, &Header);


			}
		}

		do
		{ 
			int Len;
			
			GetMsg(Stream, (char *)Header.Message, &Len, &count);
			Header.Length = Len;
	
			if (Header.Length)
			{
				char From[10] = "~~~~~~~~~";
				char To[10] = "~~~~~~~~~";

				memcpy(To, Sess->CallFrom, strlen(Sess->CallFrom));
				memcpy(From, Sess->CallTo, strlen(Sess->CallTo));

				memcpy(Header.CallFrom, From, 8);
				memcpy(Header.CallTo, To, 8);

				Header.Magic = 0x22;
				Header.Type = 0;
				Header.Seq = 0;
				Header.Sessno = Sess->Sessno;

				sendDRATSFrame(Sess->sockptr, &Header);
			}
		}
		while (count > 0);

		// See if anything to send to node

		QEntry = Sess->Queue;

		while (QEntry)
		{
			SendMsg(Sess->Stream, QEntry->Msg, QEntry->Len);
			Save = QEntry;	
			QEntry = QEntry->Next;
			free(Save->Msg);
			free(Save);
		}

		Sess->Queue = 0;
		Sess = Sess->Next;
	}
}

unsigned char BANNED[] = {'=', 0x11, 0x13, 0x1A, 0xFD, 0xFE, 0xFF, 0};


void sendDRATSFrame(struct ConnectionInfo * sockptr, struct DRATSHeader * Header)
{
	unsigned short crc;
	int len;
	unsigned char out[2048] = "[SOB]";
	int packetLen = Header->Length + HeaderLen;

	// Length is in host order

	Header->Length = htons(Header->Length);

	Header->CheckSum = 0;

	crc = dratscrc((unsigned char *)Header, packetLen);
	Header->CheckSum = htons(crc);

	len = yEncode((unsigned char *)Header, out + 5, packetLen, BANNED);

	memcpy(&out[len + 5], "[EOB]", 5);
	Debugprintf(out);
	send(sockptr->socket, out, len + 10, 0);
}

void DRATSConnectionLost(struct ConnectionInfo * sockptr)
{
	// Disconnect any sessions, then free Stream and Sess record
	
	struct DRATSSession * Sess = DRATSSessions;
	struct DRATSSession * Save = 0;
	BPQVECSTRUC * HOST;

	while (Sess)
	{
		if (Sess->sockptr == sockptr)
		{
			if (Sess->StreamState == 1)	// COnnected
			{
				Disconnect(Sess->Stream);
				HOST = &BPQHOSTVECTOR[Sess->Stream -1]; // API counts from 1
				HOST->HOSTFLAGS &= 0xFC;		  // Clear Change Bits
			}
			DeallocateStream(Sess->Stream);

			// We must unhook from chain

			if (Save)
				Save->Next = Sess->Next;
			else
				DRATSSessions = Sess->Next;

			// Should really Free any Queue, but unlikely to be any
			
			free(Sess);

			if (Save)
				Sess = Save->Next;
			else
				Sess = DRATSSessions;
		}
		else
		{
			Save = Sess;
			Sess = Sess->Next;
		}
	}
}




#ifdef WIN32
#define ZEXPORT __stdcall
#endif

#include "zlib.h"


int doinflate(unsigned char * source, unsigned char * dest, int Len, int destlen, int * outLen)
{
    int ret;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

	strm.avail_in = Len;
	strm.next_in = source;

	strm.avail_out = destlen;
	strm.next_out = dest;

	ret = inflate(&strm, Z_NO_FLUSH);

	inflateEnd(&strm);

	dest[strm.total_out] = 0;

	*outLen = strm.total_out;

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

// No idea what this CRC is, but it works! (converted from DRATS python code)

int update_crc(int c, int crc)
{
	int i;
	int v;

	for (i = 0; i < 8; i++)
	{
		if ((c & 0x80))
			v = 1;
		else
			v = 0;

		if (crc & 0x8000)
		{
			crc <<= 1;
			crc += v;
			crc ^= 0x1021;
		}
		else
		{
			crc <<= 1;
			crc += v;
		}

		c <<= 1;
	}

	crc &= 0xFFFF;
	return crc;

}

int dratscrc(unsigned char *ptr, int count)
{
	int i;	
	int checksum = 0;
    
	for (i = 0; i < count; i++)
        checksum = update_crc(ptr[i], checksum);

    checksum = update_crc(0, checksum);
    checksum = update_crc(0, checksum);
    return checksum;
}

#define OFFSET 64

int yEncode(unsigned char * in, unsigned char * out, int len, unsigned char * Banned)
{
	unsigned char * ptr = out;
	unsigned char c;

	while (len--)
	{
		c = *(in++);

		if (strchr(&Banned[0], c))
		{
			*(out++) = '=';
			*(out++) = (c + OFFSET) & 0xFF;
		}
		else
			*(out++) = c;
	}

	return (out - ptr);
}




