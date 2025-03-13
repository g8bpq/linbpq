/*
Copyright 2001-2018 John Wiseman G8BPQ

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

// Mail and Chat Server for BPQ32 Packet Switch
//
// White Pages Database Support Routines

#include "bpqmail.h"

VOID __cdecl Debugprintf(const char * format, ...);
VOID ReleaseSock(SOCKET sock);
void MQTTMessageEvent(void* message);

#define GetSemaphore(Semaphore,ID) _GetSemaphore(Semaphore, ID, __FILE__, __LINE__)
void _GetSemaphore(struct SEM * Semaphore, int ID, char * File, int Line);

struct NNTPRec * FirstNNTPRec = NULL;

//int NumberofNNTPRecs=0;

SOCKET nntpsock;

extern SocketConn * Sockets;		// Chain of active sockets

int NNTPInPort = 0;

char *day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID ReleaseNNTPSock(SOCKET sock);

int NNTPSendSock(SocketConn * sockptr, char * msg)
{
	int len = (int)strlen(msg);
	char * newmsg = malloc(len+10);

 	WriteLogLine(NULL, '>',msg,  len, LOG_TCP);

	strcpy(newmsg, msg);

	strcat(newmsg, "\r\n");

	len+=2;

	// Attempt to fix Thunderbird - Queue all and send at end

	if ((sockptr->SendSize + len) > sockptr->SendBufferSize)
	{
		sockptr->SendBufferSize += (10000 + len);
		sockptr->SendBuffer = realloc(sockptr->SendBuffer, sockptr->SendBufferSize);
	}

	memcpy(&sockptr->SendBuffer[sockptr->SendSize], newmsg, len);
	sockptr->SendSize += len;
	free (newmsg);
	return len;
}

void NNTPFlush(SocketConn * sockptr)
{	
	int sent;
	
	sent = send(sockptr->socket, sockptr->SendBuffer, sockptr->SendSize, 0);
		
	if (sent < sockptr->SendSize)
	{
		int error, remains;

		// Not all could be sent - queue rest

		if (sent == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK)
				sent=0;

			//	What else??
		}

		remains = sockptr->SendSize - sent;

		sockptr->SendBufferSize += (10000 + remains);
		sockptr->SendBuffer = malloc(sockptr->SendBufferSize);

		memmove(sockptr->SendBuffer, &sockptr->SendBuffer[sent], remains);

		sockptr->SendSize = remains;
		sockptr->SendPtr = 0;

		return;
	}
		
	free(sockptr->SendBuffer);
	sockptr->SendBuffer = NULL;
	sockptr->SendSize = 0;
	sockptr->SendBufferSize = 0;
	return;
}

VOID __cdecl NNTPsockprintf(SocketConn * sockptr, const char * format, ...)
{
	// printf to a socket

	char buff[1000];
	va_list(arglist);
	
	va_start(arglist, format);
	vsprintf(buff, format, arglist);

	NNTPSendSock(sockptr, buff);
}

struct NNTPRec * LookupNNTP(char * Group)
{
	struct NNTPRec * ptr = FirstNNTPRec;

	while (ptr)
	{
		if (_stricmp(ptr->NewsGroup, Group) == 0)
			return ptr;

		ptr = ptr->Next;
	}

	return NULL;
}


VOID BuildNNTPList(struct MsgInfo * Msg)
{
	struct NNTPRec * REC;
	struct NNTPRec * OLDREC;
	struct NNTPRec * PREVREC = 0;

	char FullGroup[100];
					
	if (Msg->type != 'B' || Msg->status == 'K' || Msg->status == 'H')
		return;

	sprintf(FullGroup, "%s.%s", Msg->to, Msg->via);

	REC = LookupNNTP(FullGroup);

	if (REC == NULL)
	{
		// New Group. Allocate a record, and put at correct place in chain (alpha order)

		GetSemaphore(&AllocSemaphore, 0);

		REC = zalloc(sizeof (struct NNTPRec));
		OLDREC = FirstNNTPRec;

		if (OLDREC == 0)					// First record
		{
			FirstNNTPRec = REC;
			goto DoneIt;
		}
		else
		{
			// Follow chain till we find one with a later name

			while(OLDREC)
			{
				if (strcmp(OLDREC->NewsGroup, FullGroup) > 0)
				{
					// chain in here

					REC->Next = OLDREC;
					if (PREVREC)
						PREVREC->Next = REC;
					else
						FirstNNTPRec = REC;
					goto DoneIt;
						
				}
				else
				{
					PREVREC = OLDREC;
					OLDREC = OLDREC->Next;
				}
			}

			// Run off end - chain to PREVREC

			PREVREC->Next = REC;
		}
DoneIt:
		strcpy(REC->NewsGroup, FullGroup);
		REC->FirstMsg = Msg->number;
		REC->DateCreated = (time_t)Msg->datecreated;

		FreeSemaphore(&AllocSemaphore);
	}

	REC->LastMsg = Msg->number;
	REC->Count++;
}

void RebuildNNTPList()
{
	struct NNTPRec * NNTPREC = FirstNNTPRec;
	struct NNTPRec * SaveNNTPREC;
	struct MsgInfo * Msg;
	int i;

	// Free old list

	while (NNTPREC)
	{
		SaveNNTPREC = NNTPREC->Next;
		free(NNTPREC);
		NNTPREC = SaveNNTPREC;
	}

	FirstNNTPRec = NULL;

	for (i = 1; i <= NumberofMessages; i++)
	{
		Msg = MsgHddrPtr[i];
		BuildNNTPList(Msg);
	}
}



char * GetPathFromHeaders(char * MsgBytes)
{
	char * Path = zalloc(10000);
	char * ptr1;

	ptr1 = MsgBytes;

nextline:

	if (memcmp(ptr1, "R:", 2) == 0)
	{		
		char * ptr4 = strchr(ptr1, '\r');
		char * ptr5 = strchr(ptr1, '.');
		ptr1 = strchr(ptr1, '@'); 

		if (!ptr1)
			return Path;

		if (*++ptr1 == ':')
			ptr1++;			// Format 2

		*(ptr5) = 0;
		
		strcat(Path, "|");
		strcat(Path, ptr1);

		*(ptr5) = '.';

		ptr1 = ptr4;

		ptr1++;
		if (*ptr1 == '\n') ptr1++;

		goto nextline;
	}

	return Path;
}

char * FormatNNTPDateAndTime(time_t Datim)
{
	struct tm *tm;
	static char Date[30];

	// Fri, 19 Nov 82 16:14:55 GMT
	// A#asctime gives Wed Jan 02 02:03:55 1980\n\0.

	tm = gmtime(&Datim);


	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%s, %02d %3s %02d %02d:%02d:%02d Z",
			day[tm->tm_wday], tm->tm_mday, month[tm->tm_mon], tm->tm_year - 100, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return Date;
}

VOID InitialiseNNTP()

{
	if (NNTPInPort)
		nntpsock = CreateListeningSocket(NNTPInPort);
}

int CreateNNTPMessage(char * From, char * To, char * MsgTitle, time_t Date, char * MsgBody, int MsgLen)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;
	char * Via;

	// Allocate a message Record slot

	Msg = AllocateMsgRecord();
		
	// Set number here so they remain in sequence
		
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;
	Msg->length = MsgLen;

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	Msg->type = 'B';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (Date)
		Msg->datecreated = Date;

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);


	TidyString(To);
	Via = strlop(To, '@');

	if (strlen(To) > 6) To[6]=0;

	strcpy(Msg->to, To);
	strcpy(Msg->from, From);
	strcpy(Msg->title, MsgTitle);
	strcpy(Msg->via, Via);

	// Set up forwarding bitmap

	MatchMessagetoBBSList(Msg, 0);

	if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		Msg->status = '$';				// Has forwarding

	BuildNNTPList(Msg);				// Build NNTP Groups list

#ifndef NOMQTT
	if (MQTT)
		MQTTMessageEvent(Msg);
#endif


	return CreateSMTPMessageFile(MsgBody, Msg);
		
}



VOID ProcessNNTPServerMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;
	time_t Date = 0;

	sock=sockptr->socket;

	WriteLogLine(NULL, '<',Buffer, Len-2, LOG_TCP);

	if (sockptr->Flags == GETTINGMESSAGE)
	{
		if(memcmp(Buffer, ".\r\n", 3) == 0)
		{
			// File Message

			char * ptr1, * ptr2;
			int linelen, MsgLen;
			char MsgFrom[62], MsgTo[62], Msgtitle[62];

			// Scan headers for From: To: and Subject: Line (Headers end at blank line)

			ptr1 = sockptr->MailBuffer;
		Loop:
			ptr2 = strchr(ptr1, '\r');

			if (ptr2 == NULL)
			{
				NNTPSendSock(sockptr, "500 Eh");
				return;
			}

			linelen = (int)(ptr2 - ptr1);

			// From: "John Wiseman" <john.wiseman@ntlworld.com>
			// To: <G8BPQ@g8bpq.org.uk>
			//<To: <gm8bpq+g8bpq@googlemail.com>


			if (_memicmp(ptr1, "From:", 5) == 0)
			{
				if (linelen > 65) linelen = 65;
				memcpy(MsgFrom, ptr1, linelen);
				MsgFrom[linelen]=0;
			}
			else
			if (_memicmp(ptr1, "Newsgroups:", 3) == 0)
			{
				char * sep = strchr(ptr1, '.');
				
				if (sep)
					*(sep) = '@';

				if (linelen > 63) linelen = 63;
				memcpy(MsgTo, &ptr1[11], linelen-11);
				MsgTo[linelen-11]=0;
			}
			else
			if (_memicmp(ptr1, "Subject:", 8) == 0)
			{
				if (linelen > 68) linelen = 68;
				memcpy(Msgtitle, &ptr1[9], linelen-9);
				Msgtitle[linelen-9]=0;
			}
			else
			if (_memicmp(ptr1, "Date:", 5) == 0)
			{
				struct tm rtime;
				char * Context;
				char seps[] = " ,\t\r";
				char Offset[10] = "";
				int i, HH, MM;

				memset(&rtime, 0, sizeof(struct tm));

				// Date: Tue, 9 Jun 2009 20:54:55 +0100

				ptr1 = strtok_s(&ptr1[5], seps, &Context);	// Skip Day
				ptr1 = strtok_s(NULL, seps, &Context);		// Day

				rtime.tm_mday = atoi(ptr1);

				ptr1 = strtok_s(NULL, seps, &Context);		// Month

				for (i=0; i < 12; i++)
				{
					if (strcmp(month[i], ptr1) == 0)
					{
						rtime.tm_mon = i;
						break;
					}
				}
		
				sscanf(Context, "%04d %02d:%02d:%02d%s",
					&rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec, Offset);

				rtime.tm_year -= 1900;

				Date = mktime(&rtime) - (time_t)_MYTIMEZONE;
				
				if (Date == (time_t)-1)
					Date = 0;
				else
				{
					if ((Offset[0] == '+') || (Offset[0] == '-'))
					{
						MM = atoi(&Offset[3]);
						Offset[3] = 0;
						HH = atoi(&Offset[1]);
						MM = MM + (60 * HH);

						if (Offset[0] == '+')
							Date -= (60*MM);
						else
							Date += (60*MM);


					}
				}
			}

			if (linelen)			// Not Null line
			{
				ptr1 = ptr2 + 2;		// Skip crlf
				goto Loop;
			}

			ptr1 = sockptr->MailBuffer;

			MsgLen = sockptr->MailSize - (int)(ptr2 - ptr1);

			ptr1 = strchr(MsgFrom, '<');
			
			if (ptr1)
			{
				char * ptr3 = strchr(ptr1, '@');
				ptr1++;
				if (ptr3)
					*(ptr3) = 0;
			}
			else
				ptr1 = MsgFrom;

			CreateNNTPMessage(_strupr(ptr1), _strupr(MsgTo), Msgtitle, Date, ptr2, MsgLen);

			free(sockptr->MailBuffer);
			sockptr->MailBufferSize=0;
			sockptr->MailBuffer=0;
			sockptr->MailSize = 0;

			sockptr->Flags &= ~GETTINGMESSAGE;

			NNTPSendSock(sockptr, "240 OK");

			return;
		}

		if ((sockptr->MailSize + Len) > sockptr->MailBufferSize)
		{
			sockptr->MailBufferSize += 10000;
			sockptr->MailBuffer = realloc(sockptr->MailBuffer, sockptr->MailBufferSize);
	
			if (sockptr->MailBuffer == NULL)
			{
				CriticalErrorHandler("Failed to extend Message Buffer");
				shutdown(sock, 0);
				return;
			}
		}

		memcpy(&sockptr->MailBuffer[sockptr->MailSize], Buffer, Len);
		sockptr->MailSize += Len;

		return;
	}

	Buffer[Len-2] = 0;

	if(_memicmp(Buffer, "AUTHINFO USER", 13) == 0)
	{
		if (Len > 22) Buffer[22]=0;
		strcpy(sockptr->CallSign, &Buffer[14]);
		sockptr->State = GettingPass;
		NNTPsockprintf(sockptr, "381 More authentication information required");
		return;
	}

	if (sockptr->State == GettingUser)
	{
		NNTPsockprintf(sockptr, "480 Authentication required");
		return;
	}

	if (sockptr->State == GettingPass)
	{
		struct UserInfo * user = NULL;

		if(_memicmp(Buffer, "AUTHINFO PASS", 13) == 0)
		{
			user = LookupCall(sockptr->CallSign);

			if (user)
			{
				if (strcmp(user->pass, &Buffer[14]) == 0)
				{
					NNTPsockprintf(sockptr, "281 Authentication accepted");
	
					sockptr->State = Authenticated;
					sockptr->POP3User = user;
					return;
				}
			}
			NNTPSendSock(sockptr, "482 Authentication rejected");
			sockptr->State = GettingUser;
			return;
		}

		NNTPsockprintf(sockptr, "480 Authentication required");
		return;
	}


	if(_memicmp(Buffer, "GROUP", 5) == 0)
	{
		struct NNTPRec * REC = FirstNNTPRec;

		while (REC)
		{
			if (_stricmp(REC->NewsGroup, &Buffer[6]) == 0)
			{
				NNTPsockprintf(sockptr, "211 %d %d %d %s", REC->Count, REC->FirstMsg, REC->LastMsg, REC->NewsGroup);
				sockptr->NNTPNum = 0;
				sockptr->NNTPGroup = REC;
				return;
			}
			REC =REC->Next;
		}
	
		NNTPsockprintf(sockptr, "411 no such news group");
		return;
	}

	if(_memicmp(Buffer, "LISTGROUP", 9) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo ;

		// Either currently selected, or a param follows

		if (REC == NULL && Buffer[10] == 0)
		{
			NNTPsockprintf(sockptr, "412 No Group Selected");
			return;
		}

		if (Buffer[10] == 0)
			goto GotGroup;
		
		REC = FirstNNTPRec;

		while(REC)
		{
			if (_stricmp(REC->NewsGroup, &Buffer[10]) == 0)
			{
			GotGroup:

				NNTPsockprintf(sockptr, "211 Article Numbers Follows");
				sockptr->NNTPNum = 0;
				sockptr->NNTPGroup = REC;

				for (MsgNo = REC->FirstMsg; MsgNo <= REC->LastMsg; MsgNo++)
				{
					Msg=MsgnotoMsg[MsgNo];

					if (Msg)
					{
						char FullGroup[100];
						sprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
						if (_stricmp(FullGroup, REC->NewsGroup) == 0)
						{
							NNTPsockprintf(sockptr, "%d", MsgNo);
						}
					}
				}
				NNTPSendSock(sockptr,".");
				return;
			}
			REC = REC->Next;
		}
		NNTPsockprintf(sockptr, "411 no such news group");
		return;
	}

	if(_memicmp(Buffer, "MODE READER", 11) == 0)
	{
		NNTPSendSock(sockptr, "200 Hello");
		return;
	}

	if(_memicmp(Buffer, "LIST",4) == 0)
	{
		struct NNTPRec * REC = FirstNNTPRec;

		NNTPSendSock(sockptr, "215 list of newsgroups follows");
	
		while (REC)
		{
			NNTPsockprintf(sockptr, "%s %d %d y", REC->NewsGroup, REC->LastMsg, REC->FirstMsg);
			REC = REC->Next;
		}

		NNTPSendSock(sockptr,".");
		return;
	}

	//NEWGROUPS YYMMDD HHMMSS [GMT] [<distributions>]
	
	if(_memicmp(Buffer, "NEWGROUPS", 9) == 0)
	{
		struct NNTPRec * REC = FirstNNTPRec;
		struct tm rtime;
		char Offset[20] = "";
		time_t Time;

		memset(&rtime, 0, sizeof(struct tm));

		sscanf(&Buffer[10], "%02d%02d%02d %02d%02d%02d %s",
			&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday,
			&rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec, Offset);

		rtime.tm_year+=100;
		rtime.tm_mon--;

		if (_stricmp(Offset, "GMT") == 0)
			Time = mktime(&rtime) - (time_t)_MYTIMEZONE;
		else
			Time = mktime(&rtime);
		
		NNTPSendSock(sockptr, "231 list of new newsgroups follows");

		while(REC)
		{
			if (REC->DateCreated > Time)
				NNTPsockprintf(sockptr, "%s %d %d y", REC->NewsGroup, REC->LastMsg, REC->FirstMsg);
			REC = REC->Next;
		}

		NNTPSendSock(sockptr,".");
		return;
	}

	if(_memicmp(Buffer, "HEAD",4) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo = atoi(&Buffer[5]);

		if (REC == NULL)
		{
			NNTPSendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		if (MsgNo == 0)
		{
			MsgNo = sockptr->NNTPNum;

			if (MsgNo == 0)
			{
				NNTPSendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgNo;
		}

		Msg=MsgnotoMsg[MsgNo];

		if (Msg)
		{
			NNTPsockprintf(sockptr, "221 %d <%s>", MsgNo, Msg->bid);

			NNTPsockprintf(sockptr, "From: %s", Msg->from);
			NNTPsockprintf(sockptr, "Date: %s", FormatNNTPDateAndTime((time_t)Msg->datecreated));
			NNTPsockprintf(sockptr, "Newsgroups: %s.s", Msg->to, Msg->via);
			NNTPsockprintf(sockptr, "Subject: %s", Msg->title);
			NNTPsockprintf(sockptr, "Message-ID: <%s>", Msg->bid);
			NNTPsockprintf(sockptr, "Path: %s", BBSName);

			NNTPSendSock(sockptr,".");
			return;
		}

		NNTPSendSock(sockptr,"423 No such article in this newsgroup");
		return;
	}

	if(_memicmp(Buffer, "ARTICLE", 7) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo = atoi(&Buffer[8]);
		char * msgbytes;
		char * Path;

		if (REC == NULL)
		{
			NNTPSendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		if (MsgNo == 0)
		{
			MsgNo = sockptr->NNTPNum;

			if (MsgNo == 0)
			{
				NNTPSendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgNo;
		}

		Msg=MsgnotoMsg[MsgNo];

		if (Msg)
		{
			NNTPsockprintf(sockptr, "220 %d <%s>", MsgNo, Msg->bid);
			msgbytes = ReadMessageFile(Msg->number);

			Path = GetPathFromHeaders(msgbytes);

			NNTPsockprintf(sockptr, "From: %s", Msg->from);
			NNTPsockprintf(sockptr, "Date: %s", FormatNNTPDateAndTime((time_t)Msg->datecreated));
			NNTPsockprintf(sockptr, "Newsgroups: %s.%s", Msg->to, Msg->via);
			NNTPsockprintf(sockptr, "Subject: %s", Msg->title);
			NNTPsockprintf(sockptr, "Message-ID: <%s>", Msg->bid);
			NNTPsockprintf(sockptr, "Path: %s", &Path[1]);

			NNTPSendSock(sockptr,"");


			NNTPSendSock(sockptr,msgbytes);
			NNTPSendSock(sockptr,"");

			NNTPSendSock(sockptr,".");

			free(msgbytes);
			free(Path);

			return;
			
		}
		NNTPSendSock(sockptr,"423 No such article in this newsgroup");
		return;
	}

	if(_memicmp(Buffer, "BODY", 4) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo = atoi(&Buffer[8]);
		char * msgbytes;
		char * Path;

		if (REC == NULL)
		{
			NNTPSendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		if (MsgNo == 0)
		{
			MsgNo = sockptr->NNTPNum;

			if (MsgNo == 0)
			{
				NNTPSendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgNo;
		}

		Msg=MsgnotoMsg[MsgNo];

		if (Msg)
		{
			NNTPsockprintf(sockptr, "222 %d <%s>", MsgNo, Msg->bid);
			msgbytes = ReadMessageFile(Msg->number);

			Path = GetPathFromHeaders(msgbytes);

			NNTPSendSock(sockptr,msgbytes);
			NNTPSendSock(sockptr,"");

			NNTPSendSock(sockptr,".");

			free(msgbytes);
			free(Path);

			return;
			
		}
		NNTPSendSock(sockptr,"423 No such article in this newsgroup");
		return;
	}

	if(_memicmp(Buffer, "XHDR",4) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgStart, MsgEnd, MsgNo, fields;
		char Header[100];

		if (REC == NULL)
		{
			NNTPSendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		// XHDR subject nnnn-nnnn

		fields = sscanf(&Buffer[5], "%s %d-%d", &Header[0], &MsgStart, &MsgEnd);

		if (fields > 1)
			MsgNo = MsgStart;

		if (fields == 2)
			MsgEnd = MsgStart;

		if (MsgNo == 0)
		{
			MsgStart = MsgEnd = sockptr->NNTPNum;

			if (MsgStart == 0)
			{
				NNTPSendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgEnd;
		}

		NNTPsockprintf(sockptr, "221 ");

		for (MsgNo = MsgStart; MsgNo <= MsgEnd; MsgNo++)
		{
			Msg=MsgnotoMsg[MsgNo];

			if (Msg)
			{
				char FullGroup[100];
				sprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
				if (_stricmp(FullGroup, REC->NewsGroup) == 0)
				{
					if (_stricmp(Header, "subject") == 0)
						NNTPsockprintf(sockptr, "%d Subject: %s", MsgNo, Msg->title);
					else if (_stricmp(Header, "from") == 0)
						NNTPsockprintf(sockptr, "%d From: %s", MsgNo, Msg->from);
					else if (_stricmp(Header, "date") == 0)
						NNTPsockprintf(sockptr, "%d Date: %s", MsgNo, FormatNNTPDateAndTime((time_t)Msg->datecreated));
					else if (_stricmp(Header, "message-id") == 0)
						NNTPsockprintf(sockptr, "%d Message-ID: <%s>",  MsgNo, Msg->bid);
					else if (_stricmp(Header, "lines") == 0)
						NNTPsockprintf(sockptr, "%d Lines: %d",  MsgNo, Msg->length);
				}
			}
		}

		NNTPSendSock(sockptr,".");
		return;

	}

	if(_memicmp(Buffer, "XOVER", 5) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgStart, MsgEnd, MsgNo, fields;

		if (REC == NULL)
		{
			NNTPSendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		fields = sscanf(&Buffer[6], "%d-%d", &MsgStart, &MsgEnd);

		if (fields > 0)
			MsgNo = MsgStart;

		if (fields == 1)
			MsgEnd = MsgStart;

		if (MsgNo == 0)
		{
			MsgStart = MsgEnd = sockptr->NNTPNum;

			if (MsgStart == 0)
			{
				NNTPSendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgEnd;
		}

		NNTPsockprintf(sockptr, "224 ");

		for (MsgNo = MsgStart; MsgNo <= MsgEnd; MsgNo++)
		{
			Msg=MsgnotoMsg[MsgNo];

			if (Msg)
			{
				char FullGroup[100];
				sprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
				if (_stricmp(FullGroup, REC->NewsGroup) == 0)
				{
					 // subject, author, date, message-id, references, byte count, and line count. 
					NNTPsockprintf(sockptr, "%d\t%s\t%s\t%s\t%s\t%s\t%d\t%d",
						MsgNo, Msg->title, Msg->from, FormatNNTPDateAndTime((time_t)Msg->datecreated), Msg->bid,
						"", Msg->length, Msg->length);
				}
			}
		}

		NNTPSendSock(sockptr,".");
		return;

	}


 /*
 240 article posted ok
   340 send article to be posted. End with <CR-LF>.<CR-LF>
   440 posting not allowed
   441 posting failed
*/
	if(_memicmp(Buffer, "POST", 4) == 0)
	{
		if (sockptr->State != Authenticated)
		{
			NNTPsockprintf(sockptr, "480 Authentication required");
			return;
		}		

		sockptr->MailBuffer=malloc(10000);
		sockptr->MailBufferSize=10000;

		if (sockptr->MailBuffer == NULL)
		{
			CriticalErrorHandler("Failed to create POP3 Message Buffer");
			NNTPSendSock(sockptr, "QUIT");
			sockptr->State = WaitingForQUITResponse;
			shutdown(sock, 0);

			return;
		}
	
		sockptr->Flags |= GETTINGMESSAGE;
		
		NNTPSendSock(sockptr, "340 OK");
		return;
	}



	if(_memicmp(Buffer, "QUIT", 4) == 0)
	{
		NNTPSendSock(sockptr, "205 OK");
		Sleep(500);
		shutdown(sock, 0);
		return;
	}

/*	if(memcmp(Buffer, "RSET\r\n", 6) == 0)
	{
		NNTPSendSock(sockptr, "250 Ok");
		sockptr->State = 0;
		sockptr->Recipients;
		return;
	}
*/

	if(memcmp(Buffer, "DATE", 4) == 0)
	{
		//This command returns a one-line response code of 111 followed by the
		//GMT date and time on the server in the form YYYYMMDDhhmmss.
	    //  111 YYYYMMDDhhmmss

		struct tm *tm;
		char Date[32];
		time_t Time = time(NULL);

		tm = gmtime(&Time);

		if (tm)
		{
			sprintf_s(Date, sizeof(Date), "111 %04d%02d%02d%02d%02d%02d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

			NNTPSendSock(sockptr, Date);
		}
		else
			NNTPSendSock(sockptr, "500 command not recognized");

		return;
	}

	NNTPSendSock(sockptr, "500 command not recognized");

	return;
}




int NNTP_Read(SocketConn * sockptr, SOCKET sock)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (sockptr->InputLen > 1000)	// Shouldnt have lines longer  than this in text mode
	{
		sockptr->InputLen=0;
	}
				
	InputLen=recv(sock, &sockptr->TCPBuffer[sockptr->InputLen], 1000, 0);

	if (InputLen <= 0)
	{
		int x = WSAGetLastError();

		closesocket(sock);
		ReleaseSock(sock);

		return 0;					// Does this mean closed?
	}

	sockptr->InputLen += InputLen;

loop:
	
	ptr = memchr(sockptr->TCPBuffer, '\n', sockptr->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &sockptr->TCPBuffer[sockptr->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			ProcessNNTPServerMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);
			sockptr->InputLen=0;	
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = sockptr->InputLen - (int)(ptr2-ptr);

			memcpy(Buffer, sockptr->TCPBuffer, MsgLen);

			ProcessNNTPServerMessage(sockptr, Buffer, MsgLen);

			memmove(sockptr->TCPBuffer, ptr, sockptr->InputLen-MsgLen);

			sockptr->InputLen -= MsgLen;

			goto loop;

		}
	}
	
	NNTPFlush(sockptr);

	return 0;
}


int NNTP_Accept(SOCKET SocketId)
{
	int addrlen;
	SocketConn * sockptr;
	u_long param = 1;

	SOCKET sock;

	addrlen=sizeof(struct sockaddr);

		//   Allocate a Socket entry

	sockptr=zalloc(sizeof(SocketConn)+100);

	sockptr->Next = Sockets;
	Sockets=sockptr;

	sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

	if (sock == INVALID_SOCKET)
	{
		Logprintf(LOG_TCP, NULL, '|', "NNTP accept() failed Error %d", WSAGetLastError());

		// get rid of socket record

		Sockets = sockptr->Next;
		free(sockptr);

		return FALSE;
	}


	sockptr->Type = NNTPServer;

	ioctl(sock, FIONBIO, &param);
	sockptr->socket = sock;
	sockptr->State = 0;
	
	NNTPSendSock(sockptr, "200 BPQMail NNTP Server ready");	
	Logprintf(LOG_TCP, NULL, '|', "Incoming NNTP Connect Socket = %d", sock);

	NNTPFlush(sockptr);

	return 0;
}
/*
int NNTP_Data(int sock, int error, int eventcode)
{
	SocketConn * sockptr;

	//	Find Connection Record

	sockptr=Sockets;
		
	while (sockptr)
	{
		if (sockptr->socket == sock)
		{
			switch (eventcode)
			{
				case FD_READ:

					return NNTP_Read(sockptr,sock);

				case FD_WRITE:

					// Either Just connected, or flow contorl cleared

					if (sockptr->SendBuffer)
						// Data Queued

						SendFromQueue(sockptr);
					else
					{
						NNTPSendSock(sockptr, "200 BPQMail NNTP Server ready");	
//						sockptr->State = GettingUser;
					}
					
					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					closesocket(sock);
					ReleaseNNTPSock(sock);
					return 0;
				}
			return 0;
		}
		else
			sockptr=sockptr->Next;
	}

	return 0;
}
*/
VOID ReleaseNNTPSock(SOCKET sock)
{
	// remove and free the socket record

	SocketConn * sockptr, * lastptr;

	sockptr=Sockets;
	lastptr=NULL;
		
	while (sockptr)
	{
		if (sockptr->socket == sock)
		{
			if (lastptr)
				lastptr->Next=sockptr->Next;
			else
				Sockets=sockptr->Next;

			free(sockptr);
			return;
		}
		else
		{
			lastptr=sockptr;
			sockptr=sockptr->Next;
		}
	}

	return;
}

VOID SendFromQueue(SocketConn * sockptr)
{
	int bytestosend = sockptr->SendSize - sockptr->SendPtr;
	int bytessent;

	Debugprintf("TCP - Sending %d bytes from buffer", bytestosend); 

	bytessent = send(sockptr->socket, &sockptr->SendBuffer[sockptr->SendPtr], bytestosend, 0);

	if (bytessent == bytestosend)
	{
		//	All Sent

		free(sockptr->SendBuffer);
		sockptr->SendBuffer = NULL;
	}
	else
	{
		sockptr->SendPtr += bytessent;
	}

	return;
}
