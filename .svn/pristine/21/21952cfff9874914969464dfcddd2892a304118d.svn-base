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
// FBB Forwarding Routines

#include "bpqmail.h"

#define GetSemaphore(Semaphore,ID) _GetSemaphore(Semaphore, ID, __FILE__, __LINE__)
void _GetSemaphore(struct SEM * Semaphore, int ID, char * File, int Line);


void DeleteRestartData(CIRCUIT * conn);

int32_t Encode(char * in, char * out, int32_t inlen, BOOL B1Protocol, int Compress);
void MQTTMessageEvent(void* message);

int MaxRXSize = 99999;
int MaxTXSize = 99999;

struct FBBRestartData ** RestartData = NULL;
int RestartCount = 0;

struct B2RestartData ** B2RestartRecs = NULL;
int B2RestartCount = 0;

extern char ProperBaseDir[];

char RestartDir[MAX_PATH] = "";

void GetRestartData()
{
	int i;
	struct FBBRestartData Restart;
	struct FBBRestartData * RestartRec;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	int FileSize;
	struct stat STAT;
	size_t ReadLen = 0;
	time_t Age;

	strcpy(RestartDir, MailDir);
	strcat(RestartDir, "/Restart");

	// Make sure RestartDir exists

#ifdef WIN32
	CreateDirectory(RestartDir, NULL);		// Just in case
#else
	mkdir(RestartDir, S_IRWXU | S_IRWXG | S_IRWXO);
	chmod(RestartDir, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

	// look for restart files. These will be numbered from 1 up

	for (i = 1; 1; i++)
	{
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/%d", RestartDir, i);
						
		if (stat(MsgFile, &STAT) == -1)
			break;
	
		FileSize = STAT.st_size;

		Age = time(NULL) - STAT.st_ctime;

		if (Age > 86400 * 2)		// Max 2 days
			continue;

		hFile = fopen(MsgFile, "rb");

		if (hFile == NULL)
			break;

		// Read Restart Record

		fread(&Restart, 1, sizeof(struct FBBRestartData), hFile); 

		if ((Restart.MailBufferSize + sizeof(struct FBBRestartData)) != FileSize)			// Duff file
		{
			fclose(hFile);
			break;
		}

		RestartRec = zalloc(sizeof (struct FBBRestartData));

		GetSemaphore(&AllocSemaphore, 0);

		RestartData = realloc(RestartData,(++RestartCount+1) * sizeof(void *));
		RestartData[RestartCount] = RestartRec;

		FreeSemaphore(&AllocSemaphore);

		memcpy(RestartRec, &Restart, sizeof(struct FBBRestartData));
		RestartRec->MailBuffer = malloc(RestartRec->MailBufferSize);
		ReadLen = fread(RestartRec->MailBuffer, 1, RestartRec->MailBufferSize, hFile); 
		
		Logprintf(LOG_BBS, 0, '?', "Restart Data for %s %s Len %d Loaded", RestartRec->Call, RestartRec->bid, RestartRec->length);
		fclose(hFile);
	}
}


void SaveRestartData()
{
	// Save restart data to file so we can reload on restart
	// Restart data has pointers to buffers so we must save copy of data and reconstitue on restart

	// Delete and resave all restart data to keep restart directory clean

	int i, n = 1;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	size_t WriteLen=0;
	struct FBBRestartData * RestartRec = NULL;
	struct stat STAT;
	time_t NOW = time(NULL);


	for (i = 1; 1; i++)
	{
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/%d", RestartDir, i);
						
		if (stat(MsgFile, &STAT) == -1)
			break;

		DeleteFile(MsgFile);
	}

	for (i = 1; i <= RestartCount; i++)
	{
		RestartRec = RestartData[i];

		if (RestartRec == 0)
			return;				// Shouldn't happen!

		if ((NOW - RestartRec->TimeCreated) > 86400 * 2)	// Max 2 days
			continue;

		sprintf_s(MsgFile, sizeof(MsgFile), "%s/%d", RestartDir, n++);

		hFile = fopen(MsgFile, "wb");

		if (hFile)
		{
			WriteLen = fwrite(RestartRec, 1, sizeof(struct FBBRestartData), hFile);		// Save Header	
			WriteLen = fwrite(RestartRec->MailBuffer, 1, RestartRec->MailBufferSize, hFile); // Save Data
			fclose(hFile);
		}
	}
}
VOID FBBputs(CIRCUIT * conn, char * buf)
{
	// Sends to user and logs

	int len = (int)strlen(buf);
	
	WriteLogLine(conn, '>', buf, len -1, LOG_BBS);

	QueueMsg(conn, buf, len);

	if (conn->BBSFlags & NEEDLF)
		QueueMsg(conn, "\n", 1);
}


VOID ProcessFBBLine(CIRCUIT * conn, struct UserInfo * user, UCHAR* Buffer, int len)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;
	int Index = 0;			// Message Type Index for Stats
	char * ptr;
	char * Context;
	char seps[] = " \r";
	int RestartPtr;
	char * Respptr;
	BOOL AllRejected = TRUE;
	char * MPS;
	char * ROChar;

	if (conn->Flags & GETTINGMESSAGE)
	{
		ProcessMsgLine(conn, user, Buffer, len);
		if (conn->Flags & GETTINGMESSAGE)

			// Still going
			return;

		SetupNextFBBMessage(conn);
		return;
	}

	if (conn->Flags & GETTINGTITLE)
	{
		ProcessMsgTitle(conn, user, Buffer, len);
		return;
	}

	// Should be FA FB F> FS FF FQ

	if (Buffer[0] == ';')			// winlink comment or BPQ Type Select
	{
		if (memcmp(Buffer, "; MSGTYPES", 7) == 0)
		{
			char * ptr;
			
			conn->SendB = conn->SendP = conn->SendT = FALSE;

			ptr = strchr(&Buffer[10], 'B');
	
			if (ptr)
			{
				conn->SendB = TRUE;
				conn->MaxBLen = atoi(++ptr);
				if (conn->MaxBLen == 0) conn->MaxBLen = 99999999;
			}

			ptr = strchr(&Buffer[10], 'T');
	
			if (ptr)
			{
				conn->SendT = TRUE;
				conn->MaxTLen = atoi(++ptr);
				if (conn->MaxTLen == 0) conn->MaxTLen = 99999999;
			}
			ptr = strchr(&Buffer[10], 'P');

			if (ptr)
			{
				conn->SendP = TRUE;
				conn->MaxPLen = atoi(++ptr);
				if (conn->MaxPLen == 0) conn->MaxPLen = 99999999;
			}
			return;
		}

		// Other ; Line - Ignore

		return;
	}

	if (Buffer[0] != 'F')
	{
		if (strstr(Buffer, "*** Profanity detected") || strstr(Buffer, "*** Unknown message sender"))
		{
			// Winlink Check - hold message

			if (conn->FBBMsgsSent)
				HoldSentMessages(conn, user);
		}

		if (conn->BBSFlags & DISCONNECTING)
			return;				// Ignore if disconnect aleady started

		BBSputs(conn, "*** Protocol Error - Line should start with 'F'\r");
		Flush(conn);
		Sleep(500);
		conn->BBSFlags |= DISCONNECTING;
		Disconnect(conn->BPQStream);

		return;
	}

	switch (Buffer[1])
	{
	case 'F':

		// Request Reverse

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);
		
		if (!FBBDoForward(conn))				// Send proposal if anthing to forward
		{
			FBBputs(conn, "FQ\r");

			conn->BBSFlags |= DISCONNECTING;

			// LinFBB needs a Disconnect Here

			if (conn->BPQBBS)
				return;							// BPQ will close when it sees FQ. Close collisions aren't good!

			if ((conn->SessType & Sess_PACTOR) == 0)
				conn->CloseAfterFlush = 20;			// 2 Secs
			else
				conn->CloseAfterFlush = 20;			// PACTOR/WINMOR drivers support deferred disc so 5 secs should be enough
		}
		return;

	case 'S':

		//	Proposal response

		Respptr=&Buffer[2];

		for (i=0; i < conn->FBBIndex; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->MsgType == 'P')
				Index = PMSG;
			else if (FBBHeader->MsgType == 'B')
				Index = BMSG;
			else if (FBBHeader->MsgType == 'T')
				Index = TMSG;

			Respptr++;

			if (*Respptr == 'E')
			{
				// Rejected 

				Logprintf(LOG_BBS, conn, '?', "Proposal %d Rejected by far end", i + 1);
			}
			
			if ((*Respptr == '-') || (*Respptr == 'N') || (*Respptr == 'R') || (*Respptr == 'E'))				// Not wanted
			{
				user->Total.MsgsRejectedOut[Index]++;
				
				// Zap the entry

				if (conn->Paclink || conn->RMSExpress || conn->PAT)			// Not using Bit Masks
				{
					// Kill Messages sent to paclink/RMS Express unless BBS FWD bit set

					// What if WLE retrieves P message that is queued to differnet BBS?
					// if we dont kill it will be offered again

					if (FBBHeader->FwdMsg->type == 'P' || (check_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber) == 0))
						FlagAsKilled(FBBHeader->FwdMsg, FALSE);
				}
				
				clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);
				set_fwd_bit(FBBHeader->FwdMsg->forw, user->BBSNumber);

				FBBHeader->FwdMsg->Locked = 0;	// Unlock

				// Shouldn't we set P messages as Forwarded
				// (or will check above have killed it if it is P with other FWD bits set)
				// Maybe better to be safe !!

				if (FBBHeader->FwdMsg->type == 'P' || memcmp(FBBHeader->FwdMsg->fbbs, zeros, NBMASK) == 0)
				{
					FBBHeader->FwdMsg->status = 'F';			// Mark as forwarded
					FBBHeader->FwdMsg->datechanged=time(NULL);
				}
				
				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));

				conn->UserPointer->ForwardingInfo->MsgCount--;

				SaveMessageDatabase();
				continue;
			}

			// FBB uses H for HOLD, but I've never seen it. RMS Express sends H for Defer.


			if (*Respptr == '=' || *Respptr == 'L' || (*Respptr == 'H' && conn->RMSExpress))	// Defer
			{
				// Remove entry from forwarding block

				FBBHeader->FwdMsg->Defered = 4;		// Don't retry for the next few forward cycles 
				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));
				continue;
			}

			conn->RestartFrom = 0;		// Assume Restart from
	
			if ((*Respptr == '!') || (*Respptr == 'A'))
			{
				// Restart 

				char Num[10];
				char *numptr=&Num[0];

				Respptr++;

				while (isdigit(*Respptr))
				{
					*(numptr++) = *(Respptr++);
				}
				*numptr = 0;

				conn->RestartFrom = atoi(Num);

				*(--Respptr) = '+';  // So can drop through
			}

			// FBB uses H for HOLD, but I've never seen it. RMS Express sends H for Defer. RMS use trapped above

			if ((*Respptr == '+') || (*Respptr == 'Y') || (*Respptr == 'H'))	
			{
				struct tm * tm;
				time_t now;
				char * MsgBytes;

				conn->FBBMsgsSent = TRUE;		// Messages to flag as complete when next command received
				AllRejected = FALSE;

				if (conn->BBSFlags & FBBForwarding)
				{
					if (conn->BBSFlags & FBBB2Mode)
						SendCompressedB2(conn, FBBHeader);
					else
						SendCompressed(conn, FBBHeader->FwdMsg);
				}
				else
				{
					nodeprintf(conn, "%s\r\n", FBBHeader->FwdMsg->title);

					MsgBytes = ReadMessageFile(FBBHeader->FwdMsg->number);

					if (MsgBytes == 0)
					{
						MsgBytes = _strdup("Message file not found\r\n");
						FBBHeader->FwdMsg->length = (int)strlen(MsgBytes);
					}

					now = time(NULL);

					tm = gmtime(&now);	
	
					nodeprintf(conn, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
						tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
						FBBHeader->FwdMsg->number, BBSName, HRoute, RlineVer);

					if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message - put blank line after header
						BBSputs(conn, "\r\n");

					QueueMsg(conn, MsgBytes, FBBHeader->FwdMsg->length);
					free(MsgBytes);

					user->Total.MsgsSent[Index]++;
					user->Total.BytesForwardedOut[Index] += FBBHeader->FwdMsg->length;
			
					nodeprintf(conn, "%c\r\n", 26);
				}
				continue;
			}
			BBSputs(conn, "*** Protocol Error - Invalid Proposal Response'\r");
		}

		conn->FBBIndex = 0;		// ready for next block;
		conn->FBBChecksum = 0;


		if (AllRejected && (conn->RMSExpress || conn->PAT))
		{
			// RMS Express and PAT don't send FF or proposal after rejecting all messages

			FBBputs(conn, "FF\r");
		}

		return;

	case 'Q':

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);

		conn->BBSFlags |= DISCONNECTING;

		Disconnect(conn->BPQStream);
		return;

	case 'A':			// Proposal
	case 'B':			// Proposal

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);			// Mark previously sent messages

		if (conn->DoReverse == FALSE)				// Dont accept messages
			return;

		// Accumulate checksum

		for (i=0; i< len; i++)
		{
			conn->FBBChecksum+=Buffer[i];
		}

		// Parse Header

		// Find free line

		for (i = 0; i < 5; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->Format == 0)
				break;
		}

		if (i == 5)
		{
			BBSputs(conn, "*** Protocol Error - Too Many Proposals\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
		}

		//FA P GM8BPQ G8BPQ G8BPQ 2209_GM8BPQ 8

		FBBHeader->Format = Buffer[1];

		ptr = strtok_s(&Buffer[3], seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) != 1) goto badparam;

		FBBHeader->MsgType = *ptr;

		if (FBBHeader->MsgType == 'P')
			Index = PMSG;
		else if (FBBHeader->MsgType == 'B')
			Index = BMSG;
		else if (FBBHeader->MsgType == 'T')
			Index = TMSG;


		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;
		strlop(ptr, '-');						// Remove any (illegal) ssid

		if (strlen(ptr) > 6 ) goto badparam;

		strcpy(FBBHeader->From, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 40 ) goto badparam;

		strcpy(FBBHeader->ATBBS, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 6)
		{
			// Temp fix - reject instead of breaking connection
		
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			Logprintf(LOG_BBS, conn, '?', "Message Rejected as TO field too long");

			user->Total.MsgsRejectedIn[Index]++;
			return;
		}

		strlop(ptr, '-');						// Remove any (illegal) ssid

		strcpy(FBBHeader->To, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 12 ) goto badparam;

		strcpy(FBBHeader->BID, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		FBBHeader->Size = atoi(ptr);

		goto ok;

badparam:

		BBSputs(conn, "*** Protocol Error - Proposal format error\r");
		Flush(conn);
		conn->CloseAfterFlush = 20;			// 2 Secs
		return;

ok:

		// Check Filters

		if (CheckRejFilters(FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->BID, FBBHeader->MsgType, FBBHeader->Size))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			Logprintf(LOG_BBS, conn, '?', "Message Rejected by Filters");

			user->Total.MsgsRejectedIn[Index]++;
		}

		// If P Message, dont immediately reject on a Duplicate BID. Check if we still have the message
		//	If we do, reject  it. If not, accept it again. (do we need some loop protection ???)

		else if (DoWeWantIt(conn, FBBHeader) == FALSE)
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			user->Total.MsgsRejectedIn[Index]++;
		}
		else if ((RestartPtr = LookupRestart(conn, FBBHeader)) > 0)
		{
			conn->FBBReplyIndex += sprintf(&conn->FBBReplyChars[conn->FBBReplyIndex], "!%d", RestartPtr);
		}
		else if (LookupTempBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '=';
		}
		else
		{

			//	Save BID in temp list in case we are offered it again before completion
			
			BIDRec * TempBID = AllocateTempBIDRecord();
			strcpy(TempBID->BID, FBBHeader->BID);
			TempBID->u.conn = conn;

			conn->FBBReplyChars[conn->FBBReplyIndex++] = '+';
		}

		FBBHeader->B2Message = FALSE;

		return;

	case 'C':			// B2 Proposal

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);			// Mark previously sent messages

		if (conn->DoReverse == FALSE)				// Dont accept messages
			return;

		// Accumulate checksum

		for (i=0; i< len; i++)
		{
			conn->FBBChecksum+=Buffer[i];
		}

		// Parse Header

		// Find free line

		for (i = 0; i < 5; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->Format == 0)
				break;
		}

		if (i == 5)
		{
			BBSputs(conn, "*** Protocol Error - Too Many Proposals\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
		}


		// FC EM A3EDD4P00P55 377 281 0
		
		/*
		
			FC Proposal code. Requires B2 SID feature.
			Type Message type ( 1 or 2 alphanumeric characters

			CM WinLink 2000 Control message
			EM Encapsulated Message
			ID Unique Message Identifier (max length 12 characters)
			U-Size Uncompressed size of message
			C-size Compressed size of message

		*/
		FBBHeader->Format = Buffer[1];

		ptr = strtok_s(&Buffer[3], seps, &Context);
		if (ptr == NULL) goto badparam2;
		if (strlen(ptr) != 2) goto badparam2;
		FBBHeader->MsgType = 'P'; //ptr[0];

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam2;

		// Relay In RO mode adds @MPS@R to the MID. Don't know why (yet!)

		MPS = strlop(ptr, '@');
		if (MPS)
			ROChar = strlop(MPS, '@');

		if (strlen(ptr) > 12 ) goto badparam;
		strcpy(FBBHeader->BID, ptr);

		ptr = strtok_s(NULL, seps, &Context);
		if (ptr == NULL) goto badparam2;
		FBBHeader->Size = atoi(ptr);

		ptr = strtok_s(NULL, seps, &Context);
		if (ptr == NULL) goto badparam2;
		FBBHeader->CSize = atoi(ptr);
		FBBHeader->B2Message = TRUE;

		// If using BPQ Extensions (From To AT in proposal) Check Filters

		Buffer[len - 1] = 0;

		if (conn->BPQBBS)
		{
			char * From = strtok_s(NULL, seps, &Context);
			char * ATBBS = strtok_s(NULL, seps, &Context);
			char * To = strtok_s(NULL, seps, &Context);
			char * Type = strtok_s(NULL, seps, &Context);

			if (From && To && ATBBS && Type && CheckRejFilters(From, To, ATBBS, NULL, *Type, FBBHeader->Size))
			{
				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
				conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
				user->Total.MsgsRejectedIn[Index]++;
				Logprintf(LOG_BBS, conn, '?', "Message Rejected by Filters");

				return;
			}
		}
		goto ok2;

badparam2:

		BBSputs(conn, "*** Protocol Error - Proposal format error\r");
		Flush(conn);
		conn->CloseAfterFlush = 20;			// 2 Secs
		return;

ok2:
		if (LookupBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			Logprintf(LOG_BBS, conn, '?', "Message Rejected by BID Check");
			user->Total.MsgsRejectedIn[Index]++;

		}
		else if (FBBHeader->Size > MaxRXSize)
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			Logprintf(LOG_BBS, conn, '?', "Message Rejected by Size Limit");
			user->Total.MsgsRejectedIn[Index]++;

		}
		else if ((RestartPtr = LookupRestart(conn, FBBHeader)) > 0)
		{
			conn->FBBReplyIndex += sprintf(&conn->FBBReplyChars[conn->FBBReplyIndex], "!%d", RestartPtr);
		}

		else if (LookupTempBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '=';
		}
		else
		{
			//	Save BID in temp list in case we are offered it again before completion
			
			BIDRec * TempBID = AllocateTempBIDRecord();
			strcpy(TempBID->BID, FBBHeader->BID);
			TempBID->u.conn = conn;

			conn->FBBReplyChars[conn->FBBReplyIndex++] = 'Y';
		}

		return;

	case '>':

		// Optional Checksum

		if (conn->DoReverse == FALSE)				// Dont accept messages
		{
			Logprintf(LOG_BBS, conn, '?', "Reverse Forwarding not allowed");
			Disconnect(conn->BPQStream);
			return;
		}

		if (len > 3)
		{
			int sum;
			
			sscanf(&Buffer[3], "%x", &sum);

			conn->FBBChecksum+=sum;

			if (conn->FBBChecksum)
			{
				BBSputs(conn, "*** Proposal Checksum Error\r");
				Flush(conn);
				conn->CloseAfterFlush = 20;			// 2 Secs
				return;
			}
		}

		// Return "FS ", followed by +-= for each proposal

		conn->FBBReplyChars[conn->FBBReplyIndex] = 0;
		conn->FBBReplyIndex = 0;

		nodeprintfEx(conn, "FS %s\r", conn->FBBReplyChars);

		// if all rejected, send proposals or prompt, else set up for first message

		FBBHeader = &conn->FBBHeaders[0];

		if (FBBHeader->MsgType == 0)
		{
			conn->FBBIndex = 0;						// ready for first block;
			memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			conn->FBBChecksum = 0;

			if (!FBBDoForward(conn))				// Send proposal if anthing to forward
			{
				conn->InputMode = 0;
				
				if (conn->DoReverse)
					FBBputs(conn, "FF\r");
				else
				{
					FBBputs(conn, "FQ\r");
					conn->CloseAfterFlush = 20;			// 2 Secs
				}
			}
		}
		else
		{
			if (conn->BBSFlags & FBBForwarding)	
			{	
				conn->InputMode = 'B';
			}

			CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID, NULL);
		}

		return;

	}

	return;
}

VOID HoldSentMessages(CIRCUIT * conn, struct UserInfo * user)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;

	conn->FBBMsgsSent = FALSE;

	for (i=0; i < 5; i++)
	{
		FBBHeader = &conn->FBBHeaders[i];
				
		if (FBBHeader && FBBHeader->MsgType)				// Not a zapped entry
		{
			int Length=0;
			char * MailBuffer = malloc(100);
			char Title[100];

			Length += sprintf(MailBuffer, "Message %d Held\r\n", FBBHeader->FwdMsg->number);
			sprintf(Title, "Message %d Held - Rejected by Winlink", FBBHeader->FwdMsg->number);
			SendMessageToSYSOP(Title, MailBuffer, Length);

			FBBHeader->FwdMsg->status = 'H';				// Mark as Held
		}
	}
	memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
	SaveMessageDatabase();
}



VOID FlagSentMessages(CIRCUIT * conn, struct UserInfo * user)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;

	// Called if FBB command received after sending a block of messages . Flag as as sent.

	conn->FBBMsgsSent = FALSE;

	for (i=0; i < 5; i++)
	{
		FBBHeader = &conn->FBBHeaders[i];
				
		if (FBBHeader && FBBHeader->MsgType)				// Not a zapped entry
		{
			if ((conn->Paclink || conn->RMSExpress || conn->PAT) &&
//				((conn->UserPointer->flags & F_NTSMPS) == 0) &&
				(FBBHeader->FwdMsg->type == 'P'))
			{
				// Kill Messages sent to paclink/RMS Express unless BBS FWD bit set

				if (check_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber) == 0)
				{
					FlagAsKilled(FBBHeader->FwdMsg, FALSE);
					continue;
				}
			}

			clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);
			set_fwd_bit(FBBHeader->FwdMsg->forw, user->BBSNumber);

			//  Only mark as forwarded if sent to all BBSs that should have it
			
			if (memcmp(FBBHeader->FwdMsg->fbbs, zeros, NBMASK) == 0)
			{
				FBBHeader->FwdMsg->status = 'F';			// Mark as forwarded
				FBBHeader->FwdMsg->datechanged=time(NULL);
			}

#ifndef NOMQTT
		if (MQTT)
			MQTTMessageEvent(FBBHeader->FwdMsg);
#endif

			FBBHeader->FwdMsg->Locked = 0;	// Unlock
			conn->UserPointer->ForwardingInfo->MsgCount--;
		}
	}
	memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
	SaveMessageDatabase();
}


VOID SetupNextFBBMessage(CIRCUIT * conn)
{	
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block

	memmove(&conn->FBBHeaders[0], &conn->FBBHeaders[1], 4 * sizeof(struct FBBHeaderLine));
	
	memset(&conn->FBBHeaders[4], 0, sizeof(struct FBBHeaderLine));

	FBBHeader = &conn->FBBHeaders[0];

	if (FBBHeader->MsgType == 0)
	{
		conn->FBBIndex = 0;		// ready for next block;
		memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));

		conn->FBBChecksum = 0;
		conn->InputMode = 0;

		if (!FBBDoForward(conn))				// Send proposal if anthing to forward
		{
			conn->InputMode = 0;
			FBBputs(conn, "FF\r");
		}
	}
	else
	{
		if (conn->BBSFlags & FBBForwarding)
			conn->InputMode = 'B';

		CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID, NULL);
	}
}

BOOL FBBDoForward(CIRCUIT * conn)
{
	int i;
	char proposal[100];
	int proplen;

	if (FindMessagestoForward(conn))
	{
		// Send Proposal Block

		struct FBBHeaderLine * FBBHeader;

		for (i=0; i < conn->FBBIndex; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (conn->BBSFlags & FBBB2Mode)

				if (conn->BPQBBS)
					
					// Add From and To Header for Filters

					proplen = sprintf(proposal, "FC EM %s %d %d %s %s %s %c\r", 
						FBBHeader->BID,
						FBBHeader->Size,
						FBBHeader->CSize,
						FBBHeader->From,
						(FBBHeader->ATBBS[0]) ? FBBHeader->ATBBS : conn->UserPointer->Call, 
						FBBHeader->To,
						FBBHeader->MsgType);

				else

					// FC EM A3EDD4P00P55 377 281 0

					proplen = sprintf(proposal, "FC EM %s %d %d %d\r", 
						FBBHeader->BID,
						FBBHeader->Size,
						FBBHeader->CSize, 0);

			else
				proplen = sprintf(proposal, "%s %c %s %s %s %s %d\r", 
					(conn->BBSFlags & FBBCompressed) ? "FA" : "FB",
					FBBHeader->MsgType,
					FBBHeader->From,
					(FBBHeader->ATBBS[0]) ? FBBHeader->ATBBS : conn->UserPointer->Call, 
					FBBHeader->To, 
					FBBHeader->BID,
					FBBHeader->Size);

			// Accumulate checksum

			while(proplen > 0)
			{
				conn->FBBChecksum+=proposal[--proplen];
			}

			FBBputs(conn, proposal);
		}

		conn->FBBChecksum = - conn->FBBChecksum;

		nodeprintfEx(conn, "F> %02X\r", conn->FBBChecksum);
	
		return TRUE;
	}

	return FALSE;
}

VOID UnpackFBBBinary(CIRCUIT * conn)
{
	int MsgLen, i, offset, n;
	UCHAR * ptr;

loop:

	if (conn->CloseAfterFlush)	// Failed (or complete), so discard rest of input
	{
		conn->InputLen = 0;
		return;
	}


	ptr = conn->InputBuffer;

	if (conn->InputLen < 2)
		return;							//  All formats need at least two bytes

	switch (*ptr)
	{
	case 1:			// Header

		MsgLen = ptr[1] + 2;

		if (conn->InputLen < MsgLen)
			return;						// Wait for more

		if (strlen(&ptr[2]) > 60)
		{
			memcpy(conn->TempMsg->title, &ptr[2], 60);
			conn->TempMsg->title[60] = 0;
			Debugprintf("FBB Subject too long - truncated, %s", &ptr[2]); 
		}
		else
			strcpy(conn->TempMsg->title, &ptr[2]);

		offset = atoi(ptr+3+strlen(&ptr[2]));

		ptr += MsgLen;

		memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

		conn->InputLen -= MsgLen;

		conn->FBBChecksum = 0;

		if (offset)
		{
			struct FBBRestartData * RestartRec;

			// Trying to restart - make sure we have restart data

			for (i = 1; i <= RestartCount; i++)
			{
				RestartRec = RestartData[i];
		
				if ((strcmp(RestartRec->Call, conn->UserPointer->Call) == 0)
					&& (strcmp(RestartRec->bid, conn->TempMsg->bid) == 0))
				{
					if (RestartRec->length <= offset)
					{
						conn->TempMsg->length = RestartRec->length;
						conn->MailBuffer = RestartRec->MailBuffer;
						conn->MailBufferSize = RestartRec->MailBufferSize;

						// FBB Seems to insert  6 Byte message
						// It looks like the original csum and length - perhaps a a consistancy check

						// But Airmail Sends the Restart Data in the next packet, move the check code.

						conn->NeedRestartHeader = TRUE;

						goto GotRestart;
					}
					else
					{
						BBSputs(conn, "*** Trying to restart from invalid position.\r");
						Flush(conn);
						conn->CloseAfterFlush = 20;			// 2 Secs

						return;
					}

					// Remove Restart info

					for (n = i; n < RestartCount; n++)
					{
						RestartData[n] = RestartData[n+1];		// move down all following entries
					}
					RestartCount--;
					SaveRestartData();
				}
			}

			// No Restart Data

			BBSputs(conn, "*** Trying to restart, but no restart data.\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
		}
	
		// Create initial buffer of 10K. Expand if needed later

		if (conn->MailBufferSize == 0)
		{
			// Dont allocate if restarting

			conn->MailBuffer=malloc(10000);
			conn->MailBufferSize=10000;
		}

	GotRestart:

		if (conn->MailBuffer == NULL)
		{
			BBSputs(conn, "*** Failed to create Message Buffer\r");
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
		}

		goto loop;



	case 2:			// Data Block

		if (ptr[1] == 0)
			MsgLen = 256;
		else
			MsgLen = ptr[1];

		if (conn->InputLen < (MsgLen + 2))
			return;						// Wait for more

		// If waiting for Restart Header, see if it has arrived

		if (conn->NeedRestartHeader)
		{
			conn->NeedRestartHeader = FALSE;

			if (MsgLen == 6)
			{
				ptr = conn->InputBuffer+2;
				conn->InputLen -=8;
								
				for (i=0; i<6; i++)
				{
					conn->FBBChecksum+=ptr[0];
					ptr++;
				}
				memmove(conn->InputBuffer, ptr, conn->InputLen);
			}
			else
			{
				BBSputs(conn, "*** Restart Header Missing.\r");
				Flush(conn);
				conn->CloseAfterFlush = 20;			// 2 Secs
			}

			goto loop;

		}
		// Process it

		ptr+=2;

		for (i=0; i< MsgLen; i++)
		{
			conn->FBBChecksum+=ptr[i];
		}

		ptr-=2;

		if ((conn->TempMsg->length + MsgLen) > conn->MailBufferSize)
		{
			conn->MailBufferSize += 10000;
			conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
			if (conn->MailBuffer == NULL)
			{
				BBSputs(conn, "*** Failed to extend Message Buffer\r");
				conn->CloseAfterFlush = 20;			// 2 Secs

				return;
			}
		}

		memcpy(&conn->MailBuffer[conn->TempMsg->length], &ptr[2], MsgLen);

		conn->TempMsg->length += MsgLen;

		MsgLen +=2;

		ptr += MsgLen;

		memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

		conn->InputLen -= MsgLen;

		goto loop;


	case 4:				// EOM

		// Process EOM

		conn->FBBChecksum+=ptr[1];

		if (conn->FBBChecksum == 0)
		{
#ifndef LINBPQ
			__try 
			{
#endif
				conn->InputMode = 0;		//  So we won't save Restart data if decode fails
				DeleteRestartData(conn);
				Decode(conn, 0);			// Setup Next Message will reset InputMode if needed
#ifndef LINBPQ
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				BBSputs(conn, "*** Program Error Decoding Message\r");
				Flush(conn);
				conn->CloseAfterFlush = 20;			// 2 Secs
				return;
			}
#endif
		}

		else
		{
			BBSputs(conn, "*** Message Checksum Error\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
	
			//	Don't allow restart, as saved data is probably duff

			conn->DontSaveRestartData = TRUE;
			return;
		}
		ptr += 2;

		memmove(conn->InputBuffer, ptr, conn->InputLen-2);

		conn->InputLen -= 2;

		goto loop;
	
	default:

		BBSputs(conn, "*** Protocol Error - Invalid Binary Message Format (Invalid Block Type)\r");
		Flush(conn);

		if (conn->CloseAfterFlush == 0)
		{
			// Dont do it more than once

			conn->CloseAfterFlush = 20;			// 2 Secs

			//	Don't allow restart, as saved data is probably duff

			//	Actually all but the last block is probably OK, but maybe
			//  not worth the risk of restarting

			// Actually I think it is

			if (conn->TempMsg->length > 256)
			{
				conn->TempMsg->length -= 256;
				conn->DontSaveRestartData = FALSE;
			}	
			else
				conn->DontSaveRestartData = TRUE;
		}
		return;
	}
}

VOID SendCompressed(CIRCUIT * conn, struct MsgInfo * FwdMsg)
{
	struct tm * tm;
	char * MsgBytes, * Save;
	UCHAR * Compressed, * Compressedptr;
	UCHAR * UnCompressed;
	char * Title;
	UCHAR * Output, * Outputptr;
	int i, OrigLen, MsgLen, CompLen, DataOffset;
	char Rline[80];
	int RLineLen;
	int Index;
	time_t temp;

	if (FwdMsg->type == 'P')
		Index = PMSG;
	else if (FwdMsg->type == 'B')
		Index = BMSG;
	else if (FwdMsg->type == 'T')
		Index = TMSG;

	MsgBytes = Save = ReadMessageFile(FwdMsg->number);

	if (MsgBytes == 0)
	{
		MsgBytes = _strdup("Message file not found\r\n");
		FwdMsg->length = (int)strlen(MsgBytes);
	}

	OrigLen = FwdMsg->length;

	Title = FwdMsg->title;

	Compressed = Compressedptr = zalloc(2 * OrigLen + 200);
	Output = Outputptr = zalloc(2 * OrigLen + 200);

	*Outputptr++ = 1;
	*Outputptr++ = (int)strlen(Title) + 8;
	strcpy(Outputptr, Title);
	Outputptr += strlen(Title) +1;
	sprintf(Outputptr, "%6d", conn->RestartFrom);
	Outputptr += 7;

	DataOffset = (int)(Outputptr - Output);	// Used if restarting

	memcpy(&temp, &FwdMsg->datereceived, sizeof(time_t));
	tm = gmtime(&temp);	
	
	sprintf(Rline, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
		tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		FwdMsg->number, BBSName, HRoute, RlineVer);

	if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message
		strcat(Rline, "\r\n");

	RLineLen = (int)strlen(Rline);

	MsgLen = OrigLen + RLineLen;
	
	UnCompressed = zalloc(MsgLen+10);
	
	strcpy(UnCompressed, Rline);

	// If a B2 Message, Remove B2 Header

	if (FwdMsg->B2Flags & B2Msg)
	{
		char * ptr;
		int BodyLen = OrigLen;				
		
		// Remove all B2 Headers, and all but the first part.
					
		ptr = strstr(MsgBytes, "Body:");
			
		if (ptr)
		{
			BodyLen = atoi(&ptr[5]);
			ptr= strstr(MsgBytes, "\r\n\r\n");		// Blank Line after headers
	
			if (ptr)
				ptr +=4;
			else
				ptr = MsgBytes;
			
		}
		else
			ptr = MsgBytes;

		if (memcmp(ptr, "R:", 2) == 0)    // Already have RLines, so remove blank line after new R:line
			RLineLen -= 2;

		memcpy(&UnCompressed[RLineLen], ptr, BodyLen);

		MsgLen = BodyLen + RLineLen;
	}
	else  // Not B2 Message
	{
		memcpy(&UnCompressed[RLineLen], MsgBytes, OrigLen);
	}

	CompLen = Encode(UnCompressed, Compressed, MsgLen, conn->BBSFlags & FBBB1Mode, conn->BBSFlags & FBBCompressed);

	conn->FBBChecksum = 0;

	// If restarting, send the checksum and length as a single record, then data from the restart point
	// The count includes the header, so adjust count and pointers

	if (conn->RestartFrom)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 6;

		for (i=0; i< 6; i++)
		{
			conn->FBBChecksum+=Compressed[i];
			*Outputptr++ = Compressed[i];
		}

		for (i=conn->RestartFrom; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		
		Compressedptr += conn->RestartFrom;
		CompLen -= conn->RestartFrom;
	}
	else
	{
		for (i=0; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
	}

	while (CompLen > 250)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 250;

		memcpy(Outputptr, Compressedptr, 250);
		Outputptr += 250;
		Compressedptr += 250;
		CompLen -= 250;
	}

	*Outputptr++ = 2;
	*Outputptr++ = CompLen;

	memcpy(Outputptr, Compressedptr, CompLen);

	Outputptr += CompLen;

	*Outputptr++ = 4;
	conn->FBBChecksum = - conn->FBBChecksum;
	*Outputptr++ = conn->FBBChecksum;

	if (conn->OpenBCM)			// Telnet, so escape any 0xFF
	{
		unsigned char * ptr1 = Output;
		unsigned char * ptr2 = Compressed;	// Reuse Compressed buffer
		size_t Len = Outputptr - Output;
		unsigned char c;

		while (Len--)
		{
			c = *(ptr1++);
			*(ptr2++) = c;
			if (c == 0xff)		// FF becodes FFFF
				*(ptr2++) = c;
		}

		QueueMsg(conn, Compressed, (int)(ptr2 - Compressed));
	}
	else
		QueueMsg(conn, Output, (int)(Outputptr - Output));

	free(Save);
	free(Compressed);
	free(UnCompressed);
	free(Output);
			
}

BOOL CreateB2Message(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader, char * Rline)
{
	char * MsgBytes;
	UCHAR * Compressed;
	UCHAR * UnCompressed;
	int OrigLen, MsgLen, B2HddrLen, CompLen;
	char Date[20];
	struct tm * tm;
	char B2From[80];
	char B2To[80];
	struct MsgInfo * Msg = FBBHeader->FwdMsg;
	struct UserInfo * FromUser;
	int BodyLineToBody;
	int RlineLen = (int)strlen(Rline) ;
	char * TypeString;
#ifndef LINBPQ	
	struct _EXCEPTION_POINTERS exinfo;

	__try {
#endif

	if (Msg == NULL)
		Debugprintf("Msg  = NULL");


	MsgBytes = ReadMessageFile(Msg->number);

	if (MsgBytes == 0)
	{
		Debugprintf("B2 Message - Message File not found");
		return FALSE;
	}

	UnCompressed = zalloc(Msg->length + 2000);

	if (UnCompressed == NULL)
		Debugprintf("B2 Message - zalloc for %d failed", Msg->length + 2000);

	OrigLen = Msg->length;

	// If a B2 Message  add R:line at start of Body, but otherwise leave intact.
	// Unless a message to Paclink, when we must remove any HA from the TO address
	// Or to a CMS, when we remove HA from From or Reply-to

	if (Msg->B2Flags & B2Msg)
	{
		char * ptr, *ptr2;
		int BodyLen;
		int BodyLineLen;
		int Index;

		MsgLen = OrigLen + RlineLen;

		if (conn->Paclink)
		{
			// Remove any HA on the TO address
		
			ptr = strstr(MsgBytes, "To:");
			if (ptr)
			{
				ptr2 = strstr(ptr, "\r\n");
				if (ptr2)
				{
					while (ptr < ptr2)
					{
						if (*ptr == '.' || *ptr == '@')
						{
							memset(ptr, ' ', ptr2 - ptr);
							break;
						}
						ptr++;
					}
				}
			}
		}

		if (conn->WL2K)
		{
			// Remove any HA on the From or Reply-To address
		
			ptr = strstr(MsgBytes, "From:");
			if (ptr == NULL)
				ptr = strstr(MsgBytes, "Reply-To:");

			if (ptr)
			{
				ptr2 = strstr(ptr, "\r\n");
				if (ptr2)
				{
					while (ptr < ptr2)
					{
						if (*ptr == '.' || *ptr == '@')
						{
							memset(ptr, ' ', ptr2 - ptr);
							break;
						}
						ptr++;
					}
				}
			}
		}


		// Add R: Line at start of body. Will Need to Update Body Length

		ptr = strstr(MsgBytes, "Body:");

		if (ptr == 0)
		{
			Debugprintf("B2 Messages without Body: Line");
			return FALSE;
		}
		ptr2 = strstr(ptr, "\r\n");

		Index = (int)(ptr - MsgBytes);		// Bytes Before Body: line

		if (Index <= 0 || Index > MsgLen)
		{
			Debugprintf("B2 Message Body: line position invalid - %d", Index);
			return FALSE;
		}

		// If message to saildocs adding an R: line will mess up the message processing, so add as an X header

		if (strstr(MsgBytes, "To: query@saildocs.com"))
		{
			int x_Len;

			memcpy(UnCompressed, MsgBytes, Index);	// Up to Old Body;
			x_Len = sprintf(&UnCompressed[Index], "x-R: %s", &Rline[2]);
			MsgLen = OrigLen + x_Len;
			Index +=x_Len;
			goto copyRest;
		}

		BodyLen = atoi(&ptr[5]);
	
		if (BodyLen < 0 || BodyLen > MsgLen)
		{
			Debugprintf("B2 Message Length from Body: line invalid - Msg len %d From Body %d", MsgLen, BodyLen);
			return FALSE;
		}

		BodyLineLen = (int)(ptr2 - ptr) + 2;
		MsgLen -= BodyLineLen;		// Length of Body Line may change

		ptr = strstr(ptr2, "\r\n\r\n");	// Blank line before Body

		if (ptr == 0)
		{
			Debugprintf("B2 Message - No Blank Line before Body");
			return FALSE;
		}

		ptr += 4;
	
		ptr2 += 2;					// Line Following Original Body: Line

		BodyLineToBody = (int)(ptr - ptr2);

		if (memcmp(ptr, "R:", 2) != 0)    // No R line, so must be our message
		{
			strcat(Rline, "\r\n");
			RlineLen += 2;
			MsgLen += 2;
		}
		BodyLen += RlineLen;

		memcpy(UnCompressed, MsgBytes, Index);	// Up to Old Body;
		BodyLineLen = sprintf(&UnCompressed[Index], "Body: %d\r\n", BodyLen);

		MsgLen += BodyLineLen;		// Length of Body Line may have changed
		Index += BodyLineLen;

		if (BodyLineToBody < 0 || BodyLineToBody > 1000)
		{
			Debugprintf("B2 Message - Body too far from Body Line - %d", BodyLineToBody);
			return FALSE;
		}
		memcpy(&UnCompressed[Index], ptr2, BodyLineToBody); // Stuff Between Body: Line and Body

		Index += BodyLineToBody;

		memcpy(&UnCompressed[Index], Rline, RlineLen);
		Index += RlineLen;

copyRest:

		memcpy(&UnCompressed[Index], ptr, MsgLen - Index);		// Rest of Message

		FBBHeader->Size = MsgLen;

		Compressed = zalloc(2 * MsgLen + 200);
#ifndef LINBPQ
		__try {
#endif
		CompLen = Encode(UnCompressed, Compressed, MsgLen, TRUE, conn->BBSFlags & FBBCompressed);

		FBBHeader->CompressedMsg = Compressed;
		FBBHeader->CSize = CompLen;

		free(UnCompressed);
		return TRUE;
#ifndef LINBPQ
		} My__except_Routine("Encode B2Message");
#endif
		return FALSE;
	}

	
	if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message
	{
		strcat(Rline, "\r\n");
		RlineLen += 2;
	}

	MsgLen = OrigLen + RlineLen;

//	if (conn->RestartFrom == 0)
//	{
//		// save time first sent, or checksum will be wrong when we restart
//		
//		FwdMsg->datechanged=time(NULL);
//	}

	tm = gmtime((time_t *)&Msg->datechanged);	
	
	sprintf(Date, "%04d/%02d/%02d %02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	// We create the B2 Header
/*
	MID: XR88I1J160EB
	Date: 2009/07/25 18:17
	Type: Private
	From: SMTP:john.wiseman@ntlworld.com
	To: G8BPQ
	Subject: RE: RMS Test Message
	Mbo: SMTP
	Body: 213

*/
	if (strcmp(Msg->to, "RMS") == 0)		// Address is in via
		strcpy(B2To, Msg->via);
	else
		if (Msg->via[0] && (!conn->Paclink))
			sprintf(B2To, "%s@%s", Msg->to, Msg->via);
		else
			strcpy(B2To, Msg->to);

	// Try to create a full from: addrsss so RMS Express can reply
	
	strcpy(B2From, Msg->from);

	Logprintf(LOG_BBS, conn, '?', "B2 From %s", B2From);

	if (strcmp(conn->Callsign, "RMS") != 0 && conn->WL2K == 0)		// if going to RMS - just send calll
	{
		if (_stricmp(Msg->from, "SMTP:") == 0)		// Address is in via
			strcpy(B2From, Msg->emailfrom);
		else
		{
			FromUser = LookupCall(Msg->from);

			if (FromUser)
			{
				Logprintf(LOG_BBS, conn, '?', "B2 From - Local User");

				if (FromUser->HomeBBS[0])
					sprintf(B2From, "%s@%s", Msg->from, FromUser->HomeBBS);
				else
					sprintf(B2From, "%s@%s", Msg->from, BBSName);
			}
			else
			{
				WPRecP WP = LookupWP(Msg->from);

				Logprintf(LOG_BBS, conn, '?', "B2 From - not local User");

				if (WP)
					sprintf(B2From, "%s@%s", Msg->from, WP->first_homebbs);
			}
		}
	}

	Logprintf(LOG_BBS, conn, '?', "B2 From Finally %s", B2From);

	if (Msg->type == 'P')
		TypeString = "Private" ;
	else if (Msg->type == 'B')
		TypeString = "Bulletin";
	else if (Msg->type == 'T')
		TypeString = "Traffic";

	B2HddrLen = sprintf(UnCompressed,
			"MID: %s\r\nDate: %s\r\nType: %s\r\nFrom: %s\r\nTo: %s\r\nSubject: %s\r\nMbo: %s\r\n"
			"Content-Type: text/plain\r\nContent-Transfer-Encoding: 8bit\r\nBody: %d\r\n\r\n",
			Msg->bid, Date, TypeString, B2From, B2To, Msg->title, BBSName, MsgLen);


	memcpy(&UnCompressed[B2HddrLen], Rline, RlineLen);
	memcpy(&UnCompressed[B2HddrLen + RlineLen], MsgBytes, OrigLen);		// Rest of Message

	MsgLen += B2HddrLen;

	FBBHeader->Size = MsgLen;

	Compressed = zalloc(2 * MsgLen + 200);

	CompLen = Encode(UnCompressed, Compressed, MsgLen, TRUE, conn->BBSFlags & FBBCompressed);

	FBBHeader->CompressedMsg = Compressed;
	FBBHeader->CSize = CompLen;

	free(UnCompressed);

	return TRUE;  
#ifndef LINBPQ
	} My__except_Routine("CreateB2Message");
#endif
	return FALSE;

}

VOID SendCompressedB2(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader)
{
	UCHAR * Compressed, * Compressedptr;
	UCHAR * Output, * Outputptr;
	int i, CompLen;
	int Index;

	if (FBBHeader->FwdMsg->type == 'P')
		Index = PMSG;
	else if (FBBHeader->FwdMsg->type == 'B')
		Index = BMSG;
	else if (FBBHeader->FwdMsg->type == 'T')
		Index = TMSG;

	Compressed = Compressedptr = FBBHeader->CompressedMsg;

	Output = Outputptr = zalloc(FBBHeader->CSize + 10000);

	*Outputptr++ = 1;
	*Outputptr++ = (int)strlen(FBBHeader->FwdMsg->title) + 8;
	strcpy(Outputptr, FBBHeader->FwdMsg->title);
	Outputptr += strlen(FBBHeader->FwdMsg->title) +1;
	sprintf(Outputptr, "%06d", conn->RestartFrom);
	Outputptr += 7;

	CompLen = FBBHeader->CSize;

	conn->FBBChecksum = 0;

	// If restarting, send the checksum and length as a single record, then data from the restart point
	// The count includes the header, so adjust count and pointers

	if (conn->RestartFrom)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 6;

		for (i=0; i< 6; i++)
		{
			conn->FBBChecksum+=Compressed[i];
			*Outputptr++ = Compressed[i];
		}
		
		for (i=conn->RestartFrom; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		
		Compressedptr += conn->RestartFrom;
		CompLen -= conn->RestartFrom;
	}
	else
	{
		for (i=0; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		conn->UserPointer->Total.MsgsSent[Index]++;
		conn->UserPointer->Total.BytesForwardedOut[Index] += FBBHeader->FwdMsg->length;

	}

	while (CompLen > 256)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 0;

		memcpy(Outputptr, Compressedptr, 256);
		Outputptr += 256;
		Compressedptr += 256;
		CompLen -= 256;
	}

	*Outputptr++ = 2;
	*Outputptr++ = CompLen;

	memcpy(Outputptr, Compressedptr, CompLen);

	Outputptr += CompLen;

	*Outputptr++ = 4;
	conn->FBBChecksum = - conn->FBBChecksum;
	*Outputptr++ = conn->FBBChecksum;

	if (conn->OpenBCM)			// Telnet, so escape any 0xFF
	{
		unsigned char * ptr1 = Output;
		unsigned char * ptr2 = Compressed;	// Reuse Compressed buffer
		int Len = (int)(Outputptr - Output);
		unsigned char c;

		while (Len--)
		{
			c = *(ptr1++);
			*(ptr2++) = c;
			if (c == 0xff)		// FF becodes FFFF
				*(ptr2++) = c;
		}

		QueueMsg(conn, Compressed, (int)(ptr2 - Compressed));
	}
	else
		QueueMsg(conn, Output, (int)(Outputptr - Output));

	free(Compressed);
	free(Output);		
}

// Restart Routines.

VOID SaveFBBBinary(CIRCUIT * conn)
{
	// Disconnected during binary transfer

	char Msg[120];
	int i, len;
	struct FBBRestartData * RestartRec = NULL;

	if (conn->TempMsg == NULL)
		return;
	
	if (conn->TempMsg->length < 256)
		return;							// Not worth it.

	// If we already have a restart record, reuse it

	for (i = 1; i <= RestartCount; i++)
	{
		RestartRec = RestartData[i];
		
		if ((strcmp(RestartRec->Call, conn->UserPointer->Call) == 0)
			&& (strcmp(RestartRec->bid, conn->TempMsg->bid) == 0))
		{
			// Found it, so reuse

			//	If we have more data, reset retry count

			if (RestartRec->length < conn->TempMsg->length)
				RestartRec->Count = 0;;

			break;
		}
	}

	if (RestartRec == NULL)
	{
		RestartRec = zalloc(sizeof (struct FBBRestartData));

		GetSemaphore(&AllocSemaphore, 0);

		RestartData=realloc(RestartData,(++RestartCount+1) * sizeof(void *));
		RestartData[RestartCount] = RestartRec;

		FreeSemaphore(&AllocSemaphore);
		RestartRec->TimeCreated = time(NULL);
	}

	strcpy(RestartRec->Call, conn->UserPointer->Call);
	RestartRec->length = conn->TempMsg->length;
	strcpy(RestartRec->bid, conn->TempMsg->bid);
	RestartRec->MailBuffer = conn->MailBuffer;
	RestartRec->MailBufferSize = conn->MailBufferSize;

	len = sprintf_s(Msg, sizeof(Msg), "Disconnect received from %s during Binary Transfer - %d Bytes Saved for restart",
		conn->Callsign, conn->TempMsg->length);

	SaveRestartData();

	WriteLogLine(conn, '|',Msg, len, LOG_BBS);
}

void DeleteRestartData(CIRCUIT * conn)
{
	struct FBBRestartData * RestartRec = NULL;
	int i, n;

	if (conn->TempMsg == NULL)
		return;
	
	for (i = 1; i <= RestartCount; i++)
	{
		RestartRec = RestartData[i];
		
		if ((strcmp(RestartRec->Call, conn->UserPointer->Call) == 0)
			&& (strcmp(RestartRec->bid, conn->TempMsg->bid) == 0))
		{
			// Remove restrt data

			for (n = i; n < RestartCount; n++)
			{
				RestartData[n] = RestartData[n+1];		// move down all following entries
			}
				
			RestartCount--;
			SaveRestartData();
			return;
		}
	}
}


BOOL LookupRestart(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader)
{
	int i, n;

	struct FBBRestartData * RestartRec;

	if ((conn->BBSFlags & FBBB1Mode) == 0)
		return FALSE;						// Only B1 & B2 support restart

	for (i = 1; i <= RestartCount; i++)
	{
		RestartRec = RestartData[i];
		
		if ((strcmp(RestartRec->Call, conn->UserPointer->Call) == 0)
			&& (strcmp(RestartRec->bid, FBBHeader->BID) == 0))
		{
			char Msg[120];
			int len;

			RestartRec->Count++;

			if (RestartRec->Count > 3)
			{
				len = sprintf_s(Msg, sizeof(Msg), "Too many restarts for %s - Requesting restart from beginning",
					FBBHeader->BID);
				
				WriteLogLine(conn, '|',Msg, len, LOG_BBS);

				// Remove restrt data

				for (n = i; n < RestartCount; n++)
				{
					RestartData[n] = RestartData[n+1];		// move down all following entries
				}
				
				RestartCount--;
				SaveRestartData();
				return FALSE;
			}

			len = sprintf_s(Msg, sizeof(Msg), "Restart Data found for %s - Requesting restart from %d",
				FBBHeader->BID, RestartRec->length);

			WriteLogLine(conn, '|',Msg, len, LOG_BBS);

			return (RestartRec->length);
		}
	}

	return FALSE;				// Not Found
}



BOOL DoWeWantIt(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader)
{
	struct MsgInfo * Msg;
	BIDRec * BID;
	int m;

	if (RefuseBulls && FBBHeader->MsgType == 'B')
	{
		Logprintf(LOG_BBS, conn, '?', "Message Rejected by RefuseBulls");
		return FALSE;
	}
	if (FBBHeader->Size > MaxRXSize)
	{
		Logprintf(LOG_BBS, conn, '?', "Message Rejected by Size Check");
		return FALSE;
	}
	
	BID = LookupBID(FBBHeader->BID);
	
	if (BID)
	{
		if (FBBHeader->MsgType == 'B')
		{
			Logprintf(LOG_BBS, conn, '?', "Message Rejected by BID Check");
			return FALSE;
		}

		// Treat P messages to SYSOP@WW as Bulls

		if (strcmp(FBBHeader->To, "SYSOP") == 0 && strcmp(FBBHeader->ATBBS, "WW") == 0)
		{
			Logprintf(LOG_BBS, conn, '?', "Message Rejected by BID Check");
			return FALSE;
		}

		m = NumberofMessages;
		
		while (m > 0)
		{
			Msg = MsgHddrPtr[m];
 
			if (Msg->number == BID->u.msgno)
			{
				// if the same TO we will assume the same message

				if (strcmp(Msg->to, FBBHeader->To) == 0)
				{
					// We have this message. If we have already forwarded it, we should accept it again

					if ((Msg->status == 'N') || (Msg->status == 'Y')|| (Msg->status == 'H'))
					{
						Logprintf(LOG_BBS, conn, '?', "Message Rejected by BID Check");
						return FALSE;			// Dont want it
					}
					else
						return TRUE;			// Get it again
				}

				// Same number. but different message (why?) Accept for now

				return TRUE; 
			}

			m--;
		}

		return TRUE; // A personal Message we have had before, but don't still have.
	}
	else
	{
		// We don't know the BID

		return TRUE;	// We want it
	}
}




