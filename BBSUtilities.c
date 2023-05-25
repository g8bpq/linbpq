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
//	Utility Routines

#include "bpqmail.h"
#ifdef WIN32
#include "Winspool.h"
#else
#include <sys/time.h>
#endif


BOOL Bells;
BOOL FlashOnBell;		// Flash instead of Beep
BOOL StripLF;

BOOL WarnWrap;
BOOL FlashOnConnect;
BOOL WrapInput;
BOOL CloseWindowOnBye;

RECT ConsoleRect;

BOOL OpenConsole;
BOOL OpenMon;

int reportNewMesageEvents = 0;


extern struct ConsoleInfo BBSConsole;

extern char LOC[7];

//#define BBSIDLETIME 120
//#define USERIDLETIME 300


#define BBSIDLETIME 900
#define USERIDLETIME 900

#ifdef LINBPQ
extern BPQVECSTRUC ** BPQHOSTVECPTR;
UCHAR * GetLogDirectory();
DllExport int APIENTRY SessionStateNoAck(int stream, int * state);
int RefreshWebMailIndex();
#else
__declspec(dllimport) BPQVECSTRUC ** BPQHOSTVECPTR;
typedef char * (WINAPI FAR *FARPROCZ)();
typedef int (WINAPI FAR *FARPROCX)();
FARPROCZ pGetLOC;
FARPROCX pRefreshWebMailIndex;

#endif

Dll BOOL APIENTRY APISendAPRSMessage(char * Text, char * ToCall);
VOID APIENTRY md5 (char *arg, unsigned char * checksum);
int APIENTRY GetRaw(int stream, char * msg, int * len, int * count);
void GetSemaphore(struct SEM * Semaphore, int ID);
void FreeSemaphore(struct SEM * Semaphore);
int EncryptPass(char * Pass, char * Encrypt);
VOID DecryptPass(char * Encrypt, unsigned char * Pass, unsigned int len);
void DeletetoRecycle(char * FN);
VOID DoImportCmd(CIRCUIT * conn, struct UserInfo * user, char * Arg1, char * Context);
VOID DoExportCmd(CIRCUIT * conn, struct UserInfo * user, char * Arg1, char * Context);
VOID TidyPrompts();
char * ReadMessageFileEx(struct MsgInfo * MsgRec);
char * APIENTRY GetBPQDirectory();
BOOL SendARQMail(CIRCUIT * conn);
int APIENTRY ChangeSessionIdletime(int Stream, int idletime);
int APIENTRY GetApplNum(int Stream);
VOID FormatTime(char * Time, time_t cTime);
BOOL CheckifPacket(char * Via);
char * APIENTRY GetVersionString();
void ListFiles(ConnectionInfo * conn, struct UserInfo * user, char * filename);
void ReadBBSFile(ConnectionInfo * conn, struct UserInfo * user, char * filename);
int GetCMSHash(char * Challenge, char * Password);
BOOL SendAMPRSMTP(CIRCUIT * conn);
VOID ProcessMCASTLine(ConnectionInfo * conn, struct UserInfo * user, char * Buffer, int MsgLen);
VOID MCastTimer();
VOID MCastConTimer(ConnectionInfo * conn);
int FindFreeBBSNumber();
VOID DoSetMsgNo(CIRCUIT * conn, struct UserInfo * user, char * Arg1, char * Context);
BOOL ProcessYAPPMessage(CIRCUIT * conn);
void YAPPSendFile(ConnectionInfo * conn, struct UserInfo * user, char * filename);
void YAPPSendData(ConnectionInfo * conn);
VOID CheckBBSNumber(int i);
struct UserInfo * FindAMPR();
VOID SaveInt64Value(config_setting_t * group, char * name, long long value);
VOID SaveIntValue(config_setting_t * group, char * name, int value);
VOID SaveStringValue(config_setting_t * group, char * name, char * value);
char *stristr (char *ch1, char *ch2);
BOOL CheckforMessagetoServer(struct MsgInfo * Msg);
void DoHousekeepingCmd(CIRCUIT * conn, struct UserInfo * user, char * Arg1, char * Context);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP, BOOL IncludeKilled);
void ListCategories(ConnectionInfo * conn);
void RebuildNNTPList();
long long GetInt64Value(config_setting_t * group, char * name);
void ProcessSyncModeMessage(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len);
int ReformatSyncMessage(CIRCUIT * conn);
char * initMultipartUnpack(char ** Input);
char * FormatSYNCMessage(CIRCUIT * conn, struct MsgInfo * Msg);
int decode_quoted_printable(char *ptr, int len);
void decodeblock( unsigned char in[4], unsigned char out[3]);
int encode_quoted_printable(char *s, char * out, int Len);
int32_t Encode(char * in, char * out, int32_t inlen, BOOL B1Protocol, int Compress);
int APIENTRY ChangeSessionCallsign(int Stream, unsigned char * AXCall);

config_t cfg;
config_setting_t * group;

extern ULONG BBSApplMask;

//static int SEMCLASHES = 0;

char SecureMsg[80] = "";			// CMS Secure Signon Response

int	NumberofStreams;

extern char VersionStringWithBuild[50];

#define MaxSockets 64

extern struct SEM OutputSEM;

extern ConnectionInfo Connections[MaxSockets+1];

extern struct UserInfo ** UserRecPtr;
extern int NumberofUsers;

extern struct UserInfo * BBSChain;					// Chain of users that are BBSes

extern struct MsgInfo ** MsgHddrPtr;
extern int NumberofMessages;

extern int FirstMessageIndextoForward;					// Lowest Message wirh a forward bit set - limits search

extern char UserDatabaseName[MAX_PATH];
extern char UserDatabasePath[MAX_PATH];

extern char MsgDatabasePath[MAX_PATH];
extern char MsgDatabaseName[MAX_PATH];

extern char BIDDatabasePath[MAX_PATH];
extern char BIDDatabaseName[MAX_PATH];

extern char WPDatabasePath[MAX_PATH];
extern char WPDatabaseName[MAX_PATH];

extern char BadWordsPath[MAX_PATH];
extern char BadWordsName[MAX_PATH];

extern char BaseDir[MAX_PATH];
extern char BaseDirRaw[MAX_PATH];			// As set in registry - may contain %NAME%
extern char ProperBaseDir[MAX_PATH];		// BPQ Directory/BPQMailChat


extern char MailDir[MAX_PATH];

extern BIDRec ** BIDRecPtr;
extern int NumberofBIDs;

extern BIDRec ** TempBIDRecPtr;
extern int NumberofTempBIDs;

extern WPRec ** WPRecPtr;
extern int NumberofWPrecs;

extern char ** BadWords;
extern int NumberofBadWords;
extern char * BadFile;

extern int LatestMsg;
extern struct SEM MsgNoSemaphore;					// For locking updates to LatestMsg
extern int HighestBBSNumber;

extern int MaxMsgno;
extern int BidLifetime;
extern int MaxAge;
extern int MaintInterval;
extern int MaintTime;

extern int ProgramErrors;

extern BOOL MonBBS;
extern BOOL MonCHAT;
extern BOOL MonTCP;

BOOL SendNewUserMessage = TRUE;
BOOL AllowAnon = FALSE;
BOOL UserCantKillT = FALSE;

typedef int (WINAPI FAR *FARPROCX)();
FARPROCX pRunEventProgram;

int RunEventProgram(char * Program, char * Param);


extern BOOL EventsEnabled;

#define BPQHOSTSTREAMS	64

// Although externally streams are numbered 1 to 64, internally offsets are 0 - 63

extern BPQVECSTRUC BPQHOSTVECTOR[BPQHOSTSTREAMS + 5];

#ifdef LINBPQ
extern BPQVECSTRUC ** BPQHOSTVECPTR;
extern char WL2KModes [54][18];
#else
__declspec(dllimport) BPQVECSTRUC ** BPQHOSTVECPTR;


char WL2KModes [54][18] = {
	"Packet 1200", "Packet 2400", "Packet 4800", "Packet 9600", "Packet 19200", "Packet 38400", "High Speed Packet", "", "", "", "",
	"", "Pactor 1", "", "", "Pactor 2", "", "Pactor 3", "", "", "Pactor 4", // 10 - 20
	"Winmor 500", "Winmor 1600", "", "", "", "", "", "", "",				// 21 - 29
	"Robust Packet", "", "", "", "", "", "", "", "", "",					// 30 - 39
	"ARDOP 200", "ARDOP 500", "ARDOP 1000", "ARDOP 2000", "ARDOP 2000 FM", "", "", "", "", "",	// 40 - 49
	"VARA", "VARA FM", "VARA FM WIDE", "VARA 500"};
#endif





FILE * LogHandle[4] = {NULL, NULL, NULL, NULL};

time_t LastLogTime[4] = {0, 0, 0, 0};

char FilesNames[4][100] = {"", "", "", ""};

char * Logs[4] = {"BBS", "CHAT", "TCP", "DEBUG"};


BOOL OpenLogfile(int Flags)
{
	UCHAR FN[MAX_PATH];
	time_t LT;
	struct tm * tm;

	LT = time(NULL);
	tm = gmtime(&LT);	

	sprintf(FN,"%s/logs/log_%02d%02d%02d_%s.txt", GetLogDirectory(), tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, Logs[Flags]);

	LogHandle[Flags] = fopen(FN, "ab");
		
#ifndef WIN32

	if (strcmp(FN, &FilesNames[Flags][0]))
	{
		UCHAR SYMLINK[MAX_PATH];

		sprintf(SYMLINK,"%s/logLatest_%s.txt", GetBPQDirectory(), Logs[Flags]);
		unlink(SYMLINK); 
		strcpy(&FilesNames[Flags][0], FN);
		symlink(FN, SYMLINK);
	}

#endif

	return (LogHandle[Flags] != NULL);
}

typedef  int (WINAPI FAR *FARPROCX)();

extern FARPROCX pDllBPQTRACE;

struct SEM LogSEM = {0, 0};

void WriteLogLine(CIRCUIT * conn, int Flag, char * Msg, int MsgLen, int Flags)
{
	char CRLF[2] = {0x0d,0x0a};
	struct tm * tm;
	char Stamp[20];
	time_t LT;
//	struct _EXCEPTION_POINTERS exinfo;

	// Write to Node BPQTRACE system

	if ((Flags == LOG_BBS || Flags == LOG_DEBUG_X) && MsgLen < 250)
	{
		MESSAGE Monframe;
		memset(&Monframe, 0, sizeof(Monframe));

		Monframe.PORT = 64;	
		Monframe.LENGTH = 12 + MsgLen;
		Monframe.DEST[0] = 1;			// Plain Text Monitor

		memcpy(&Monframe.DEST[1], Msg, MsgLen);
		Monframe.DEST[1 + MsgLen] = 0;

		time(&Monframe.Timestamp);
#ifdef LINBPQ
		GetSemaphore(&Semaphore, 88);
		BPQTRACE(&Monframe, FALSE);
		FreeSemaphore(&Semaphore);
#else
		if (pDllBPQTRACE)
			pDllBPQTRACE(&Monframe, FALSE);
#endif
	}
#ifndef LINBPQ
	__try
	{
#endif



#ifndef LINBPQ

	if (hMonitor)
	{
		if (Flags == LOG_TCP && MonTCP)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);
			WritetoMonitorWindow(Msg, MsgLen);
			WritetoMonitorWindow(CRLF , 1);
		}
		else if (Flags == LOG_CHAT && MonCHAT)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);

			if (conn && conn->Callsign[0])
			{
				char call[20];
				sprintf(call, "%s          ", conn->Callsign);
				WritetoMonitorWindow(call, 10);
			}
			else
				WritetoMonitorWindow("          ", 10);

			WritetoMonitorWindow(Msg, MsgLen);
			if (Msg[MsgLen-1] != '\r')
				WritetoMonitorWindow(CRLF , 1);
		}
		else if (Flags == LOG_BBS  && MonBBS)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);
			if (conn && conn->Callsign[0])
			{
				char call[20];
				sprintf(call, "%s          ", conn->Callsign);
				WritetoMonitorWindow(call, 10);
			}
			else
				WritetoMonitorWindow("          ", 10);

			WritetoMonitorWindow(Msg, MsgLen);
			WritetoMonitorWindow(CRLF , 1);
		}
		else if (Flags == LOG_DEBUG_X)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);
			WritetoMonitorWindow(Msg, MsgLen);
			WritetoMonitorWindow(CRLF , 1);
		}
	}
#endif

	if (Flags == LOG_TCP && !LogTCP)
		return;
	if (Flags == LOG_BBS && !LogBBS)
		return;
	if (Flags == LOG_CHAT && !LogCHAT)
		return;

	GetSemaphore(&LogSEM, 0);

	if (LogHandle[Flags] == NULL)
		OpenLogfile(Flags);

	if (LogHandle[Flags] == NULL) 
	{
		FreeSemaphore(&LogSEM);
		return;
	}
	LT = time(NULL);
	tm = gmtime(&LT);	
	
	sprintf(Stamp,"%02d%02d%02d %02d:%02d:%02d %c",
				tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, Flag);

	fwrite(Stamp, 1, strlen(Stamp), LogHandle[Flags]);

	if (conn && conn->Callsign[0])
	{
		char call[20];
		sprintf(call, "%s          ", conn->Callsign);
		fwrite(call, 1, 10, LogHandle[Flags]);
	}
	else
		fwrite("          ", 1, 10, LogHandle[Flags]);

	fwrite(Msg, 1, MsgLen, LogHandle[Flags]);

	if (Flags == LOG_CHAT && Msg[MsgLen-1] == '\r')
		fwrite(&CRLF[1], 1, 1, LogHandle[Flags]);
	else
		fwrite(CRLF, 1, 2, LogHandle[Flags]);

	// Don't close/reopen logs every time

//	if ((LT - LastLogTime[Flags]) > 60)
	{
		LastLogTime[Flags] = LT;
		fclose(LogHandle[Flags]);
		LogHandle[Flags] = NULL;
	}
	FreeSemaphore(&LogSEM);
	
#ifndef LINBPQ
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
}

int	CriticalErrorHandler(char * error)
{
	Debugprintf("Critical Error %s", error);
	ProgramErrors = 25;
	CheckProgramErrors();				// Force close
	return 0;
}

BOOL CheckForTooManyErrors(ConnectionInfo * conn)
{
	conn->ErrorCount++;

	if (conn->ErrorCount > 4)
	{
		BBSputs(conn, "Too many errors - closing\r");
		conn->CloseAfterFlush = 20;
		return TRUE;
	}
	return FALSE;
}





VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[16384];
	va_list(arglist);
	int Len;

	va_start(arglist, format);
	Len = vsprintf(Mess, format, arglist);
#ifndef LINBPQ
	WriteLogLine(NULL, '!',Mess, Len, LOG_DEBUG_X);
#endif
	//	#ifdef _DEBUG 
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

//	#endif
	return;
}

VOID __cdecl Logprintf(int LogMode, CIRCUIT * conn, int InOut, const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf(Mess, format, arglist);
	WriteLogLine(conn, InOut, Mess, Len, LogMode);

	return;
}

struct MsgInfo * GetMsgFromNumber(int msgno)
{
	if (msgno < 1 || msgno > 999999)
		return NULL;
	
	return MsgnotoMsg[msgno];
}
		
struct UserInfo * AllocateUserRecord(char * Call)
{
	struct UserInfo * User = zalloc(sizeof (struct UserInfo));
		
	strcpy(User->Call, Call);
	User->Length = sizeof (struct UserInfo);

	GetSemaphore(&AllocSemaphore, 0);

	UserRecPtr=realloc(UserRecPtr,(++NumberofUsers+1) * sizeof(void *));
	UserRecPtr[NumberofUsers]= User;

	FreeSemaphore(&AllocSemaphore);

	return User;
}

struct MsgInfo * AllocateMsgRecord()
{
	struct MsgInfo * Msg = zalloc(sizeof (struct MsgInfo));

	GetSemaphore(&AllocSemaphore, 0);

	MsgHddrPtr=realloc(MsgHddrPtr,(++NumberofMessages+1) * sizeof(void *));
	MsgHddrPtr[NumberofMessages] = Msg;

	FreeSemaphore(&AllocSemaphore);

	return Msg;
}

BIDRec * AllocateBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));
	
	GetSemaphore(&AllocSemaphore, 0);

	BIDRecPtr = realloc(BIDRecPtr,(++NumberofBIDs+1) * sizeof(void *));
	BIDRecPtr[NumberofBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

BIDRec * AllocateTempBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));

	GetSemaphore(&AllocSemaphore, 0);

	TempBIDRecPtr=realloc(TempBIDRecPtr,(++NumberofTempBIDs+1) * sizeof(void *));
	TempBIDRecPtr[NumberofTempBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

struct UserInfo * LookupCall(char * Call)
{
	struct UserInfo * ptr = NULL;
	int i;

	for (i=1; i <= NumberofUsers; i++)
	{
		ptr = UserRecPtr[i];

		if (_stricmp(ptr->Call, Call) == 0) return ptr;

	}

	return NULL;
}

int GetNetInt(char * Line)
{
	char temp[1024];
	char * ptr = strlop(Line, ',');
	int n = atoi(Line);
	if (ptr == NULL)
		Line[0] = 0;
	else
	{	
		strcpy(temp, ptr);
		strcpy(Line, temp);
	}
	return n;
}

VOID GetUserDatabase()
{
	struct UserInfo UserRec;

	FILE * Handle;
	size_t ReadLen;
	struct UserInfo * user;
	time_t UserLimit = time(NULL) - (UserLifetime * 86400); // Oldest user to keep
	int i;

	// See if user config is in main config

	group = config_lookup (&cfg, "BBSUsers");

	if (group)
	{
		// We have User config in the main config file. so use that

		int index = 0;
		char * stats;
		struct MsgStats * Stats;
		char * ptr, * ptr2;

		config_setting_t * entry =  config_setting_get_elem (group, index++);

		// Initialise a new File

		UserRecPtr = malloc(sizeof(void *));
		UserRecPtr[0] = malloc(sizeof (struct UserInfo));
		memset(UserRecPtr[0], 0, sizeof (struct UserInfo));
		UserRecPtr[0]->Length = sizeof (struct UserInfo);

		NumberofUsers = 0;

		while (entry)
		{
			char call[16];

			// entry->name is call, will have * in front if a call stating woth number

			if (entry->name[0] == '*')
				strcpy(call, &entry->name[1]);
			else
				strcpy(call, entry->name);

			user = AllocateUserRecord(call);

			ptr = entry->value.sval;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->Name, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->Address, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->HomeBBS, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->QRA, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->pass, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->ZIP, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) strcpy(user->CMSPass, ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->lastmsg = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->flags = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->PageLen = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->BBSNumber = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->RMSSSIDBits = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->WebSeqNo = atoi(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) user->TimeLastConnected = atol(ptr);
			ptr = ptr2;

			ptr2 = strlop(ptr, '^');
			if (ptr) Stats = &user->Total;
			stats = ptr;

			if (Stats == NULL)
			{
				NumberofUsers--;
				free(user);
				entry =  config_setting_get_elem (group, index++);
				continue;
			}

			Stats->ConnectsIn = GetNetInt(stats);
			Stats->ConnectsOut = GetNetInt(stats);
			Stats->MsgsReceived[0] = GetNetInt(stats);
			Stats->MsgsReceived[1] = GetNetInt(stats);
			Stats->MsgsReceived[2] = GetNetInt(stats);
			Stats->MsgsReceived[3] = GetNetInt(stats);
			Stats->MsgsSent[0] = GetNetInt(stats);
			Stats->MsgsSent[1] = GetNetInt(stats);
			Stats->MsgsSent[2] = GetNetInt(stats);
			Stats->MsgsSent[3] = GetNetInt(stats);
			Stats->MsgsRejectedIn[0] = GetNetInt(stats);
			Stats->MsgsRejectedIn[1] = GetNetInt(stats);
			Stats->MsgsRejectedIn[2] = GetNetInt(stats);
			Stats->MsgsRejectedIn[3] = GetNetInt(stats);
			Stats->MsgsRejectedOut[0] = GetNetInt(stats);
			Stats->MsgsRejectedOut[1] = GetNetInt(stats);
			Stats->MsgsRejectedOut[2] = GetNetInt(stats);
			Stats->MsgsRejectedOut[3] = GetNetInt(stats);
			Stats->BytesForwardedIn[0] = GetNetInt(stats);
			Stats->BytesForwardedIn[1] = GetNetInt(stats);
			Stats->BytesForwardedIn[2] = GetNetInt(stats);
			Stats->BytesForwardedIn[3] = GetNetInt(stats);
			Stats->BytesForwardedOut[0] = GetNetInt(stats);
			Stats->BytesForwardedOut[1] = GetNetInt(stats);
			Stats->BytesForwardedOut[2] = GetNetInt(stats);
			Stats->BytesForwardedOut[3] = GetNetInt(stats);

			Stats = &user->Last;
			stats = ptr2;

			if (Stats == NULL)
			{
				NumberofUsers--;
				free(user);
				entry =  config_setting_get_elem (group, index++);
				continue;
			}

			Stats->ConnectsIn = GetNetInt(stats);
			Stats->ConnectsOut = GetNetInt(stats);
			Stats->MsgsReceived[0] = GetNetInt(stats);
			Stats->MsgsReceived[1] = GetNetInt(stats);
			Stats->MsgsReceived[2] = GetNetInt(stats);
			Stats->MsgsReceived[3] = GetNetInt(stats);
			Stats->MsgsSent[0] = GetNetInt(stats);
			Stats->MsgsSent[1] = GetNetInt(stats);
			Stats->MsgsSent[2] = GetNetInt(stats);
			Stats->MsgsSent[3] = GetNetInt(stats);
			Stats->MsgsRejectedIn[0] = GetNetInt(stats);
			Stats->MsgsRejectedIn[1] = GetNetInt(stats);
			Stats->MsgsRejectedIn[2] = GetNetInt(stats);
			Stats->MsgsRejectedIn[3] = GetNetInt(stats);
			Stats->MsgsRejectedOut[0] = GetNetInt(stats);
			Stats->MsgsRejectedOut[1] = GetNetInt(stats);
			Stats->MsgsRejectedOut[2] = GetNetInt(stats);
			Stats->MsgsRejectedOut[3] = GetNetInt(stats);
			Stats->BytesForwardedIn[0] = GetNetInt(stats);
			Stats->BytesForwardedIn[1] = GetNetInt(stats);
			Stats->BytesForwardedIn[2] = GetNetInt(stats);
			Stats->BytesForwardedIn[3] = GetNetInt(stats);
			Stats->BytesForwardedOut[0] = GetNetInt(stats);
			Stats->BytesForwardedOut[1] = GetNetInt(stats);
			Stats->BytesForwardedOut[2] = GetNetInt(stats);
			Stats->BytesForwardedOut[3] = GetNetInt(stats);


			if ((user->flags & F_BBS) == 0)		// Not BBS - Check Age
			{
				if (UserLifetime && user->TimeLastConnected)	// Dont delete manually added Users that havent yet connected
				{
					if (user->TimeLastConnected < UserLimit)
					{
						// Too Old - ignore

						NumberofUsers--;
						free(user);
						entry =  config_setting_get_elem (group, index++);
						continue;
					}
				}
			}
			user->Temp = zalloc(sizeof (struct TempUserInfo));

			if (user->lastmsg < 0 || user->lastmsg > LatestMsg)
				user->lastmsg = LatestMsg;


			entry =  config_setting_get_elem (group, index++);
		}
	}
	else
	{
		Handle = fopen(UserDatabasePath, "rb");

		if (Handle == NULL)
		{
			// Initialise a new File

			UserRecPtr=malloc(sizeof(void *));
			UserRecPtr[0]= malloc(sizeof (struct UserInfo));
			memset(UserRecPtr[0], 0, sizeof (struct UserInfo));
			UserRecPtr[0]->Length = sizeof (struct UserInfo);

			NumberofUsers = 0;

			return;
		}


		// Get First Record

		ReadLen = fread(&UserRec, 1, (int)sizeof (UserRec), Handle);

		if (ReadLen == 0)
		{
			// Duff file

			memset(&UserRec, 0, sizeof (struct UserInfo));
			UserRec.Length = sizeof (struct UserInfo);
		}
		else
		{
			// See if format has changed

			if (UserRec.Length == 0)
			{
				// Old format without a Length field

				struct OldUserInfo * OldRec = (struct OldUserInfo *)&UserRec;
				int Users = OldRec->ConnectsIn;		// User Count in control record
				char Backup1[MAX_PATH];

				//  Create a backup in case reversion is needed and Reposition to first User record

				fclose(Handle);

				strcpy(Backup1, UserDatabasePath);
				strcat(Backup1, ".oldformat");

				CopyFile(UserDatabasePath, Backup1, FALSE);	 // Copy to .bak

				Handle = fopen(UserDatabasePath, "rb");

				ReadLen = fread(&UserRec, 1, (int)sizeof (struct OldUserInfo), Handle);	// Skip Control Record

				// Set up control record

				UserRecPtr=malloc(sizeof(void *));
				UserRecPtr[0]= malloc(sizeof (struct UserInfo));
				memcpy(UserRecPtr[0], &UserRec,  sizeof (UserRec));
				UserRecPtr[0]->Length = sizeof (UserRec);

				NumberofUsers = 0;

OldNext:

				ReadLen = fread(&UserRec, 1, (int)sizeof (struct OldUserInfo), Handle);

				if (ReadLen > 0)
				{
					if (OldRec->Call[0] < '0')
						goto OldNext;					// Blank record

					user = AllocateUserRecord(OldRec->Call);
					user->Temp = zalloc(sizeof (struct TempUserInfo));

					// Copy info from Old record

					user->lastmsg = OldRec->lastmsg;
					user->Total.ConnectsIn = OldRec->ConnectsIn;
					user->TimeLastConnected = OldRec->TimeLastConnected;
					user->flags = OldRec->flags;
					user->PageLen = OldRec->PageLen;
					user->BBSNumber = OldRec->BBSNumber;
					memcpy(user->Name, OldRec->Name, 18);
					memcpy(user->Address, OldRec->Address, 61);
					user->Total.MsgsReceived[0] = OldRec->MsgsReceived;
					user->Total.MsgsSent[0] = OldRec->MsgsSent;
					user->Total.MsgsRejectedIn[0] = OldRec->MsgsRejectedIn;			// Messages we reject
					user->Total.MsgsRejectedOut[0] = OldRec->MsgsRejectedOut;		// Messages Rejectd by other end
					user->Total.BytesForwardedIn[0] = OldRec->BytesForwardedIn;
					user->Total.BytesForwardedOut[0] = OldRec->BytesForwardedOut;
					user->Total.ConnectsOut = OldRec->ConnectsOut;			// Forwarding Connects Out
					user->RMSSSIDBits = OldRec->RMSSSIDBits;			// SSID's to poll in RMS
					memcpy(user->HomeBBS, OldRec->HomeBBS, 41);
					memcpy(user->QRA, OldRec->QRA, 7);
					memcpy(user->pass, OldRec->pass, 13);
					memcpy(user->ZIP, OldRec->ZIP, 9);

					//	Read any forwarding info, even if not a BBS.
					//	This allows a BBS to be temporarily set as a
					//	normal user without loosing forwarding info

					SetupForwardingStruct(user);

					if (user->flags & F_BBS)
					{
						// Defined as BBS - allocate and initialise forwarding structure

						// Add to BBS Chain;

						user->BBSNext = BBSChain;
						BBSChain = user;

						// Save Highest BBS Number

						if (user->BBSNumber > HighestBBSNumber) HighestBBSNumber = user->BBSNumber;
					}
					goto OldNext;
				}

				SortBBSChain();
				fclose(Handle);	

				return;
			}
		}

		// Set up control record

		UserRecPtr=malloc(sizeof(void *));
		UserRecPtr[0]= malloc(sizeof (struct UserInfo));
		memcpy(UserRecPtr[0], &UserRec,  sizeof (UserRec));
		UserRecPtr[0]->Length = sizeof (UserRec);

		NumberofUsers = 0;

Next:

		ReadLen = fread(&UserRec, 1, (int)sizeof (UserRec), Handle);

		if (ReadLen > 0)
		{
			if (UserRec.Call[0] < '0')
				goto Next;					// Blank record

			if (UserRec.TimeLastConnected == 0)
				UserRec.TimeLastConnected = UserRec.xTimeLastConnected;

			if ((UserRec.flags & F_BBS) == 0)		// Not BBS - Check Age
				if (UserLifetime)					// if limit set
					if (UserRec.TimeLastConnected)	// Dont delete manually added Users that havent yet connected
						if (UserRec.TimeLastConnected < UserLimit)
							goto Next;			// Too Old - ignore

			user = AllocateUserRecord(UserRec.Call);
			memcpy(user, &UserRec,  sizeof (UserRec));
			user->Temp = zalloc(sizeof (struct TempUserInfo));

			user->ForwardingInfo = NULL;	// In case left behind on crash
			user->BBSNext = NULL;
			user->POP3Locked = FALSE;

			if (user->lastmsg < 0 || user->lastmsg > LatestMsg)
				user->lastmsg = LatestMsg;

			goto Next;
		}
		fclose(Handle);
	}

	// Setting up BBS struct has been moved until all user record
	//	have been read so we can fix corrupt BBSNUmber

	for (i=1; i <= NumberofUsers; i++)
	{
		user = UserRecPtr[i];

		//	Read any forwarding info, even if not a BBS.
		//	This allows a BBS to be temporarily set as a
		//	normal user without loosing forwarding info

		SetupForwardingStruct(user);

		if (user->flags & F_BBS)
		{
			// Add to BBS Chain;

			if (user->BBSNumber == NBBBS)				// Fix corrupt records
			{
				user->BBSNumber = FindFreeBBSNumber();
				if (user->BBSNumber == 0)
					user->BBSNumber = NBBBS;			// cant really do much else
			}

			user->BBSNext = BBSChain;
			BBSChain = user;

//			Logprintf(LOG_BBS, NULL, '?', "BBS %s BBSNumber %d", user->Call, user->BBSNumber);

			// Save Highest BBS Number

			if (user->BBSNumber > HighestBBSNumber)
				HighestBBSNumber = user->BBSNumber;
		}
	}

	// Check for dulicate BBS numbers
	
	for (i=1; i <= NumberofUsers; i++)
	{
		user = UserRecPtr[i];

		if (user->flags & F_BBS)
		{
			if (user->BBSNumber == 0)
				user->BBSNumber = FindFreeBBSNumber();
			
			CheckBBSNumber(user->BBSNumber);
		}
	}

	SortBBSChain();
}

VOID CopyUserDatabase()
{
	return;			// User config now in main config file
/*
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];

	// Keep 4 Generations

	strcpy(Backup2, UserDatabasePath);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, UserDatabasePath);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, UserDatabasePath);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, UserDatabasePath);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		//Move .bak to .bak.1

	CopyFile(UserDatabasePath, Backup1, FALSE);	 // Copy to .bak
*/
}

VOID CopyConfigFile(char * ConfigName)
{
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];

	// Keep 4 Generations

	strcpy(Backup2, ConfigName);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, ConfigName);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, ConfigName);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, ConfigName);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		// Move .bak to .bak.1

	CopyFile(ConfigName, Backup1, FALSE);	// Copy to .bak
}



VOID SaveUserDatabase()
{
	SaveConfig(ConfigName);			// User config is now in main config file
	GetConfig(ConfigName);

/*
	FILE * Handle;
	size_t WriteLen;
	int i;

	Handle = fopen(UserDatabasePath, "wb");

	UserRecPtr[0]->Total.ConnectsIn = NumberofUsers;

	for (i=0; i <= NumberofUsers; i++)
	{
		WriteLen = fwrite(UserRecPtr[i], 1, (int)sizeof (struct UserInfo), Handle);
	}

	fclose(Handle);
*/
	return;
}

VOID GetMessageDatabase()
{
	struct MsgInfo MsgRec;
	FILE * Handle;
	size_t ReadLen;
	struct MsgInfo * Msg;
	char * MsgBytes;
	int FileRecsize = sizeof(struct MsgInfo);	// May be changed if reformating
	BOOL Reformatting = FALSE;
	char HEX[3] = "";
	int n;

	// See if Message Database is in main config

	group = config_lookup (&cfg, "MSGS");

//	group = 0;

	if (group)
	{
		// We have User config in the main config file. so use that

		int index = 0;
		char * ptr, * ptr2;
		config_setting_t * entry =  config_setting_get_elem (group, index++);

		// Initialise a new File

		MsgHddrPtr=malloc(sizeof(void *));
		MsgHddrPtr[0]= zalloc(sizeof (MsgRec));
		NumberofMessages = 0;
		MsgHddrPtr[0]->status = 2;

		if (entry)
		{
			// First Record has current message number

			ptr = entry->value.sval;
			ptr2 = strlop(ptr, '|');
			ptr2 = strlop(ptr2, '|');
			if (ptr2)
				LatestMsg = atoi(ptr2);
		}

		entry =  config_setting_get_elem (group, index++);

		while (entry)
		{
			// entry->name is MsgNo with 'R' in front

			ptr = entry->value.sval;
			ptr2 = strlop(ptr, '|');

			memset(&MsgRec, 0, sizeof(struct MsgInfo));

			MsgRec.number = atoi(&entry->name[1]);
			MsgRec.type = ptr[0];

			ptr = ptr2;

			if (ptr == NULL)
			{
				entry =  config_setting_get_elem (group, index++);
				continue;
			}

			ptr2 = strlop(ptr, '|');
			MsgRec.status = ptr[0];

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.length = atoi(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.datereceived = atol(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.bbsfrom, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.via, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.from, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.to, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.bid, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.B2Flags = atoi(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.datecreated = atol(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.datechanged = atol(ptr);

			ptr = ptr2;
			if (ptr) ptr2 = strlop(ptr, '|');

			if (ptr == NULL)
			{
				entry =  config_setting_get_elem (group, index++);
				continue;
			}

			if (ptr[0])
			{
				char String[50] = "00000000000000000000";
				String[20] = 0;
				memcpy(String, ptr, strlen(ptr));
				for (n = 0; n < NBMASK; n++)
				{
					memcpy(HEX, &String[n * 2], 2);
					MsgRec.fbbs[n] = (UCHAR)strtol(HEX, 0, 16);
				}
			}

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');

			if (ptr == NULL)
			{
				entry =  config_setting_get_elem (group, index++);
				continue;
			}

			if (ptr[0])
			{
				char String[50] = "00000000000000000000";
				String[20] = 0;
				memcpy(String, ptr, strlen(ptr));
				for (n = 0; n < NBMASK; n++)
				{
					memcpy(HEX, &String[n * 2], 2);
					MsgRec.forw[n] = (UCHAR)strtol(HEX, 0, 16);
				}
			}

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(MsgRec.emailfrom, ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) MsgRec.UTF8 = atoi(ptr);

			ptr = ptr2;

			if (ptr) 
			{
				strcpy(MsgRec.title, ptr);

				MsgBytes = ReadMessageFileEx(&MsgRec);

				if (MsgBytes)
				{
					free(MsgBytes);
					Msg = AllocateMsgRecord();
					memcpy(Msg, &MsgRec, sizeof (MsgRec));

					MsgnotoMsg[Msg->number] = Msg;

					// Fix Corrupted NTS Messages

					if (Msg->type == 'N')
						Msg->type = 'T';

					// Look for corrupt FROM address (ending in @)

					strlop(Msg->from, '@');

					BuildNNTPList(Msg);				// Build NNTP Groups list

					// If any forward bits are set, increment count on corresponding BBS record.

					if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
					{
						if (FirstMessageIndextoForward == 0)
							FirstMessageIndextoForward = NumberofMessages;			// limit search
					}
				}
			}
			entry =  config_setting_get_elem (group, index++);
		}

		if (FirstMessageIndextoForward == 0)
			FirstMessageIndextoForward = NumberofMessages;			// limit search

		return;
	}

	Handle = fopen(MsgDatabasePath, "rb");

	if (Handle == NULL)
	{
		// Initialise a new File

		MsgHddrPtr=malloc(sizeof(void *));
		MsgHddrPtr[0]= zalloc(sizeof (MsgRec));
		NumberofMessages = 0;
		MsgHddrPtr[0]->status = 2;

		return;
	}

	// Get First Record
		
	ReadLen = fread(&MsgRec, 1, FileRecsize, Handle); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&MsgRec, 0, sizeof (MsgRec));
		MsgRec.status = 2;
	}

	// Set up control record

	MsgHddrPtr=malloc(sizeof(void *));
	MsgHddrPtr[0]= malloc(sizeof (MsgRec));
	memcpy(MsgHddrPtr[0], &MsgRec,  sizeof (MsgRec));

	LatestMsg=MsgHddrPtr[0]->length;

	NumberofMessages = 0;

	if (MsgRec.status == 1)		// Used as file format version
								// 0 = original, 1 = Extra email from addr, 2 = More BBS's.
	{
		char Backup1[MAX_PATH];

			//  Create a backup in case reversion is needed and Reposition to first User record

			fclose(Handle);

			strcpy(Backup1, MsgDatabasePath);
			strcat(Backup1, ".oldformat");

			CopyFile(MsgDatabasePath, Backup1, FALSE);	 // Copy to .oldformat

			Handle = fopen(MsgDatabasePath, "rb");

			FileRecsize = sizeof(struct OldMsgInfo);
			
			ReadLen = fread(&MsgRec, 1, FileRecsize, Handle); 

			MsgHddrPtr[0]->status = 2;
	}

Next: 

	ReadLen = fread(&MsgRec, 1, FileRecsize, Handle); 

	if (ReadLen > 0)
	{
		// Validate Header

		if (FileRecsize == sizeof(struct MsgInfo))
		{
			if (MsgRec.type == 0 || MsgRec.number == 0)
				goto Next;

			MsgBytes = ReadMessageFileEx(&MsgRec);

			if (MsgBytes)
			{
	//			MsgRec.length = strlen(MsgBytes);
				free(MsgBytes);
			}
			else
				goto Next;

			Msg = AllocateMsgRecord();

			memcpy(Msg, &MsgRec, +sizeof (MsgRec));
		}
		else
		{
			// Resizing - record from file is an OldRecInfo
			
			struct OldMsgInfo * OldMessage = (struct OldMsgInfo *) &MsgRec;

			if (OldMessage->type == 0)
				goto Next;

			if (OldMessage->number > 99999 || OldMessage->number < 1)
				goto Next;

			Msg = AllocateMsgRecord();


			Msg->B2Flags = OldMessage->B2Flags;
			memcpy(Msg->bbsfrom, OldMessage->bbsfrom, 7);
			memcpy(Msg->bid, OldMessage->bid, 13);
			Msg->datechanged = OldMessage->datechanged;
			Msg->datecreated = OldMessage->datecreated;
			Msg->datereceived = OldMessage->datereceived;
			memcpy(Msg->emailfrom, OldMessage->emailfrom, 41);
			memcpy(Msg->fbbs , OldMessage->fbbs, 10);
			memcpy(Msg->forw , OldMessage->forw, 10);
			memcpy(Msg->from, OldMessage->from, 7);
			Msg->length = OldMessage->length;
			Msg->nntpnum = OldMessage->nntpnum;
			Msg->number = OldMessage->number;
			Msg->status = OldMessage->status;
			memcpy(Msg->title, OldMessage->title, 61);
			memcpy(Msg->to, OldMessage->to, 7);
			Msg->type = OldMessage->type;
			memcpy(Msg->via, OldMessage->via, 41);
		}

		MsgnotoMsg[Msg->number] = Msg;

		// Fix Corrupted NTS Messages

		if (Msg->type == 'N')
			Msg->type = 'T';

		// Look for corrupt FROM address (ending in @)

		strlop(Msg->from, '@');

		// Move Dates if first run with new format

		if (Msg->datecreated == 0)
			Msg->datecreated = Msg->xdatecreated;

		if (Msg->datereceived == 0)
			Msg->datereceived = Msg->xdatereceived;

		if (Msg->datechanged == 0)
			Msg->datechanged = Msg->xdatechanged;

		BuildNNTPList(Msg);				// Build NNTP Groups list

		Msg->Locked = 0;				// In case left locked
		Msg->Defered = 0;				// In case left set.

		// If any forward bits are set, increment count on corresponding BBS record.

		if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		{
			if (FirstMessageIndextoForward == 0)
				FirstMessageIndextoForward = NumberofMessages;			// limit search
		}

		goto Next;
	}

	if (FirstMessageIndextoForward == 0)
		FirstMessageIndextoForward = NumberofMessages;			// limit search

	fclose(Handle);
}

VOID CopyMessageDatabase()
{
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];

//	return;

	// Keep 4 Generations

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, MsgDatabasePath);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, MsgDatabasePath);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		//Move .bak to .bak.1

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak");

	CopyFile(MsgDatabasePath, Backup2, FALSE);	// Copy to .bak

}

VOID SaveMessageDatabase()
{
	FILE * Handle;
	size_t WriteLen;
	int i;
	char Key[16];
	struct MsgInfo *Msg;
//	char CfgName[MAX_PATH];
	char HEXString1[64];
	char HEXString2[64];
	int n;
//	char * CfgBuffer;
	char Cfg[1024];
//	int CfgLen = 0;
//	FILE * hFile;

//	SaveConfig(ConfigName);		// Message Headers now in main config
//	return;

#ifdef LINBPQ
	RefreshWebMailIndex();
#else
	if (pRefreshWebMailIndex)
		pRefreshWebMailIndex();
#endif

	Handle = fopen(MsgDatabasePath, "wb");

	if (Handle == NULL)
	{
		CriticalErrorHandler("Failed to open message database");
		return;
	}

	MsgHddrPtr[0]->status = 2;
	MsgHddrPtr[0]->number = NumberofMessages;
	MsgHddrPtr[0]->length = LatestMsg;

	for (i=0; i <= NumberofMessages; i++)
	{
		WriteLen = fwrite(MsgHddrPtr[i], 1, sizeof (struct MsgInfo), Handle);
	
		if (WriteLen != sizeof(struct MsgInfo))
		{
			CriticalErrorHandler("Failed to write message database record");
			return;
		}
	}

	if (fclose(Handle) != 0)
		CriticalErrorHandler("Failed to close message database");

	for (i = 1; i <= NumberofMessages; i++)
	{
		Msg = MsgHddrPtr[i];

		for (n = 0; n < NBMASK; n++)
			sprintf(&HEXString1[n * 2], "%02X", Msg->fbbs[n]);

		n = 39;
		while (n >=0 && HEXString1[n] == '0')
			HEXString1[n--] = 0;

		for (n = 0; n < NBMASK; n++)
			sprintf(&HEXString2[n * 2], "%02X", Msg->forw[n]);

		n = 39;
		while (n >= 0 && HEXString2[n] == '0')
			HEXString2[n--] = 0;
		
		sprintf(Key, "R%d:\r\n", i);

		n = sprintf(Cfg, "%c|%c|%d|%d|%lld|%s|%s|%s|%s|%s|%d|%lld|%lld|%s|%s|%s|%d|%s", Msg->type, Msg->status,
		Msg->number, Msg->length, Msg->datereceived, &Msg->bbsfrom[0], &Msg->via[0], &Msg->from[0],
		&Msg->to[0], &Msg->bid[0], Msg->B2Flags, Msg->datecreated, Msg->datechanged, HEXString1, HEXString2, 
		&Msg->emailfrom[0], Msg->UTF8, &Msg->title[0]);
	}

	return;
}

VOID GetBIDDatabase()
{
	BIDRec BIDRec;
	FILE * Handle;
	size_t ReadLen;
	BIDRecP BID;
	int index = 0;
	char * ptr, * ptr2;

	// If BID info is in main config file, use it

	group = config_lookup (&cfg, "BIDS");

	if (group)
	{
		config_setting_t * entry =  config_setting_get_elem (group, index++);
	
		BIDRecPtr=malloc(sizeof(void *));
		BIDRecPtr[0]= malloc(sizeof (BIDRec));
		memset(BIDRecPtr[0], 0, sizeof (BIDRec));
		NumberofBIDs = 0;

		while (entry)
		{
			// entry->name is Bid with 'R' in front

			ptr = entry->value.sval;
			ptr2 = strlop(ptr, '|');

			if (ptr && ptr2)
			{
				BID = AllocateBIDRecord();
				strcpy(BID->BID, &entry->name[1]);
				BID->mode = atoi(ptr);
				BID->u.timestamp = atoi(ptr2);

				if (BID->u.timestamp == 0) 	
					BID->u.timestamp = LOWORD(time(NULL)/86400);

			}
			entry =  config_setting_get_elem (group, index++);
		}
		return;
	}

	Handle = fopen(BIDDatabasePath, "rb");

	if (Handle == NULL)
	{
		// Initialise a new File

		BIDRecPtr=malloc(sizeof(void *));
		BIDRecPtr[0]= malloc(sizeof (BIDRec));
		memset(BIDRecPtr[0], 0, sizeof (BIDRec));
		NumberofBIDs = 0;

		return;
	}


	// Get First Record
		
	ReadLen = fread(&BIDRec, 1, sizeof (BIDRec), Handle); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&BIDRec, 0, sizeof (BIDRec));
	}

	// Set up control record

	BIDRecPtr = malloc(sizeof(void *));
	BIDRecPtr[0] = malloc(sizeof (BIDRec));
	memcpy(BIDRecPtr[0], &BIDRec,  sizeof (BIDRec));

	NumberofBIDs = 0;

Next:

	ReadLen = fread(&BIDRec, 1, sizeof (BIDRec), Handle); 

	if (ReadLen > 0)
	{
		BID = AllocateBIDRecord();
		memcpy(BID, &BIDRec,  sizeof (BIDRec));

		if (BID->u.timestamp == 0) 	
			BID->u.timestamp = LOWORD(time(NULL)/86400);

		goto Next;
	}

	fclose(Handle);
}

VOID CopyBIDDatabase()
{
	char Backup[MAX_PATH];

//	return;


	strcpy(Backup, BIDDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(BIDDatabasePath, Backup, FALSE);
}

VOID SaveBIDDatabase()
{
	FILE * Handle;
	size_t WriteLen;
	int i;

//	return;					// Bids are now in main config and are saved when message is saved

	Handle = fopen(BIDDatabasePath, "wb");

	BIDRecPtr[0]->u.msgno = NumberofBIDs;			// First Record has file size

	for (i=0; i <= NumberofBIDs; i++)
	{
		WriteLen = fwrite(BIDRecPtr[i], 1, sizeof (BIDRec), Handle);
	}

	fclose(Handle);

	return;
}

BIDRec * LookupBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofBIDs; i++)
	{
		ptr = BIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0)
			return ptr;
	}

	return NULL;
}

BIDRec * LookupTempBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofTempBIDs; i++)
	{
		ptr = TempBIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0) return ptr;
	}

	return NULL;
}

VOID RemoveTempBIDS(CIRCUIT * conn)
{
	// Remove any Temp BID records for conn. Called when connection closes - Msgs will be complete or failed
	
	if (NumberofTempBIDs == 0)
		return;
	else
	{
		BIDRec * ptr = NULL;
		BIDRec ** NewTempBIDRecPtr = zalloc((NumberofTempBIDs+1) * sizeof(void *));
		int i = 0, n;

		GetSemaphore(&AllocSemaphore, 0);

		for (n = 1; n <= NumberofTempBIDs; n++)
		{
			ptr = TempBIDRecPtr[n];

			if (ptr)
			{
				if (ptr->u.conn == conn)
					// Remove this entry 
					free(ptr);
				else
					NewTempBIDRecPtr[++i] = ptr;
			}
		}

		NumberofTempBIDs = i;

		free(TempBIDRecPtr);

		TempBIDRecPtr = NewTempBIDRecPtr;
		FreeSemaphore(&AllocSemaphore);
	}

}

VOID GetBadWordFile()
{
	FILE * Handle;
	DWORD FileSize;
	char * ptr1, * ptr2;
	struct stat STAT;

	if (stat(BadWordsPath, &STAT) == -1)
		return;

	FileSize = STAT.st_size;

	Handle = fopen(BadWordsPath, "rb");

	if (Handle == NULL)
		return;

	// Release old info in case a re-read

	if (BadWords) free(BadWords);
	if (BadFile) free(BadFile);

	BadWords = NULL;
	BadFile = NULL;
	NumberofBadWords = 0;

	BadFile = malloc(FileSize+1);

	fread(BadFile, 1, FileSize, Handle); 

	fclose(Handle);

	BadFile[FileSize]=0;

	_strlwr(BadFile);								// Compares are case-insensitive

	ptr1 = BadFile;

	while (ptr1)
	{
		if (*ptr1 == '\n') ptr1++;

		ptr2 = strtok_s(NULL, "\r\n", &ptr1);
		if (ptr2)
		{
			if (*ptr2 != '#')
			{
				BadWords = realloc(BadWords,(++NumberofBadWords+1) * sizeof(void *));
				BadWords[NumberofBadWords] = ptr2;
			}
		}
		else
			break;
	}
}

BOOL CheckBadWord(char * Word, char * Msg)
{
	char * ptr1 = Msg, * ptr2;
	size_t len = strlen(Word);

	while (*ptr1)					// Stop at end
	{
		ptr2 = strstr(ptr1, Word);

		if (ptr2 == NULL)
			return FALSE;				// OK

		// Only bad if it ia not part of a longer word

		if ((ptr2 == Msg) || !(isalpha(*(ptr2 - 1))))	// No alpha before
			if (!(isalpha(*(ptr2 + len))))			// No alpha after
				return TRUE;					// Bad word
	
		// Keep searching

		ptr1 = ptr2 + len;
	}

	return FALSE;					// OK
}

BOOL CheckBadWords(char * Msg)
{
	char * dupMsg = _strlwr(_strdup(Msg));
	int i;

	for (i = 1; i <= NumberofBadWords; i++)
	{
		if (CheckBadWord(BadWords[i], dupMsg))
		{
			free(dupMsg);
			return TRUE;			// Bad
		}
	}

	free(dupMsg);
	return FALSE;					// OK

}

VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user)
{
	if (user->flags & F_Expert)
		ExpandAndSendMessage(conn, ExpertWelcomeMsg, LOG_BBS);
	else if (conn->NewUser)
		ExpandAndSendMessage(conn, NewWelcomeMsg, LOG_BBS);
	else
		ExpandAndSendMessage(conn, WelcomeMsg, LOG_BBS);

	if (user->HomeBBS[0] == 0 && !DontNeedHomeBBS)
		BBSputs(conn, "Please enter your Home BBS using the Home command.\rYou may also enter your QTH and ZIP/Postcode using qth and zip commands.\r");

//	if (user->flags & F_Temp_B2_BBS)
//		nodeprintf(conn, "%s CMS >\r", BBSName);
//	else
		SendPrompt(conn, user);
}

VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user)
{
	if (user->Temp->ListSuspended)
		return;						// Dont send prompt if pausing a listing
		
	if (user->flags & F_Expert)
		ExpandAndSendMessage(conn, ExpertPrompt, LOG_BBS);
	else if (conn->NewUser)
		ExpandAndSendMessage(conn, NewPrompt, LOG_BBS);
	else
		ExpandAndSendMessage(conn, Prompt, LOG_BBS);

//	if (user->flags & F_Expert)
//		nodeprintf(conn, "%s\r", ExpertPrompt);
//	else if (conn->NewUser)
//		nodeprintf(conn, "%s\r", NewPrompt);
//	else
//		nodeprintf(conn, "%s\r", Prompt);
}



VOID * _zalloc(size_t len)
{
	// ?? malloc and clear

	void * ptr;

	ptr=malloc(len);
	memset(ptr, 0, len);

	return ptr;
}

BOOL isAMPRMsg(char * Addr)
{
	// See if message is addressed to ampr.org and is either
	// for us or we have SendAMPRDirect (ie don't need RMS or SMTP to send it)

	size_t toLen = strlen(Addr);

	if (_memicmp(&Addr[toLen - 8], "ampr.org", 8) == 0)
	{
		// message is for ampr.org

		char toCall[48];
		char * via;

		strcpy(toCall, _strupr(Addr));

		via = strlop(toCall, '@');

		if (_stricmp(via, AMPRDomain) == 0)
		{
			// message is for us.
			
			return TRUE;
		}
		
		if (SendAMPRDirect)
		{
			// We want to send ampr mail direct to host. Queue to BBS AMPR

			if (FindAMPR())
			{
				// We have bbs AMPR
				
				return TRUE;
			}
		}
	}
	return FALSE;
}

struct UserInfo * FindAMPR()
{
	struct UserInfo * bbs;
	
	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		if (strcmp(bbs->Call, "AMPR") == 0)
			return bbs;
	}
	
	return NULL;
}

struct UserInfo * FindRMS()
{
	struct UserInfo * bbs;
	
	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		if (strcmp(bbs->Call, "RMS") == 0)
			return bbs;
	}
	
	return NULL;
}

struct UserInfo * FindBBS(char * Name)
{
	struct UserInfo * bbs;
	
	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		if (strcmp(bbs->Call, Name) == 0)
			return bbs;
	}
	
	return NULL;
}

int CountConnectionsOnPort(int CheckPort)
{
	int n, Count = 0;
	CIRCUIT * conn;
	int port, sesstype, paclen, maxframe, l4window;
	char callsign[11];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active)
		{
			GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);
			if (port == CheckPort)
				Count++;
		}
	}

	return Count;
}


BOOL CheckRejFilters(char * From, char * To, char * ATBBS, char * BID, char Type)
{
	char ** Calls;

	if (Type == 'B' && FilterWPBulls && _stricmp(To, "WP") == 0)
		return TRUE;

	if (RejFrom && From)
	{
		Calls = RejFrom;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], From) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (RejTo && To)
	{
		Calls = RejTo;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], To) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (RejAt && ATBBS)
	{
		Calls = RejAt;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (RejBID && BID)
	{
		Calls = RejBID;

		while(Calls[0])
		{
			if (Calls[0][0] == '*')
			{
				if (stristr(BID, &Calls[0][1]))	
					return TRUE;
			}
			else
			{
				if (_stricmp(BID, Calls[0]) == 0)	
					return TRUE;
			}

			Calls++;
		}
	}
	return FALSE;		// Ok to accept
}

BOOL CheckValidCall(char * From)
{
	unsigned int i;

	if (DontCheckFromCall)
		return TRUE;
	
	if (strcmp(From, "SYSOP") == 0 || strcmp(From, "SYSTEM") == 0 || 
		strcmp(From, "IMPORT") == 0 || strcmp(From, "SMTP:") == 0 || strcmp(From, "RMS:") == 0)
		return TRUE;

	for (i = 1; i < strlen(From); i++)		// skip first which may also be digit
	{
		if (isdigit(From[i]))
		{
			// Has a digit. Check Last is not digit

			if (isalpha(From[strlen(From) - 1]))
				return TRUE;
		}
	}

	// No digit, return false

	return FALSE;
}

BOOL CheckHoldFilters(char * From, char * To, char * ATBBS, char * BID)
{
	char ** Calls;

	if (HoldFrom && From)
	{
		Calls = HoldFrom;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], From) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (HoldTo && To)
	{
		Calls = HoldTo;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], To) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (HoldAt && ATBBS)
	{
		Calls = HoldAt;

		while(Calls[0])
		{
			if (_stricmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (HoldBID && BID)
	{
		Calls = HoldBID;

		while(Calls[0])
		{
			if (Calls[0][0] == '*')
			{
				if (stristr(BID, &Calls[0][1]))	
					return TRUE;
			}
			else
			{
				if (_stricmp(BID, Calls[0]) == 0)	
					return TRUE;
			}

			Calls++;
		}
	}
	return FALSE;		// Ok to accept
}

BOOL CheckifLocalRMSUser(char * FullTo)
{
	struct UserInfo * user = LookupCall(FullTo);

	if (user)
		if (user->flags & F_POLLRMS)
			return TRUE;

	return FALSE;
		
}



int check_fwd_bit(char *mask, int bbsnumber)
{
	if (bbsnumber)
		return (mask[(bbsnumber - 1) / 8] & (1 << ((bbsnumber - 1) % 8)));
	else
		return 0;
}


void set_fwd_bit(char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] |= (1 << ((bbsnumber - 1) % 8));
}


void clear_fwd_bit (char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] &= (~(1 << ((bbsnumber - 1) % 8)));
}

VOID BBSputs(CIRCUIT * conn, char * buf)
{
	// Sends to user and logs

	WriteLogLine(conn, '>',buf,  (int)strlen(buf) -1, LOG_BBS);

	QueueMsg(conn, buf, (int)strlen(buf));
}

VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[1000];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf(Mess, format, arglist);

	QueueMsg(conn, Mess, len);

	WriteLogLine(conn, '>',Mess, len-1, LOG_BBS);

	return;
}

// nodeprintfEx add a LF if NEEFLF is set

VOID __cdecl nodeprintfEx(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[1000];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf(Mess, format, arglist);

	QueueMsg(conn, Mess, len);

	WriteLogLine(conn, '>',Mess, len-1, LOG_BBS);

	if (conn->BBSFlags & NEEDLF)
		QueueMsg(conn, "\r", 1);

	return;
}


int compare( const void *arg1, const void *arg2 );

VOID SortBBSChain()
{
	struct UserInfo * user;
	struct UserInfo * users[161]; 
	int i = 0, n;

	// Get array of addresses

	for (user = BBSChain; user; user = user->BBSNext)
	{
		users[i++] = user;
		if (i > 160) break;
	}

	qsort((void *)users, i, sizeof(void *), compare );

	BBSChain = NULL;

	// Rechain (backwards, as entries ate put on front of chain)

	for (n = i-1; n >= 0; n--)
	{
		users[n]->BBSNext = BBSChain;
		BBSChain = users[n];
	}
}

int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately call is at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}

int CountMessagesTo(struct UserInfo * user, int * Unread)
{
	int i, Msgs = 0;
	UCHAR * Call = user->Call;

	*Unread = 0;

	for (i = NumberofMessages; i > 0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->to, Call) == 0)
		{
			Msgs++;
			if (MsgHddrPtr[i]->status == 'N')
				*Unread = *Unread + 1;
		}
	}
	return(Msgs);
}



// Custimised message handling routines.
/*
	Variables - a subset of those used by FBB

 $C : Number of the next message.
 $I : First name of the connected user.
 $L : Number of the latest message.
 $N : Number of active messages
 $U : Callsign of the connected user.
 $W : Inserts a carriage return.
 $Z : Last message read by the user (L command).
 %X : Number of messages for the user.
 %x : Number of new messages for the user.
*/

VOID ExpandAndSendMessage(CIRCUIT * conn, char * Msg, int LOG)
{
	char NewMessage[10000];
	char * OldP = Msg;
	char * NewP = NewMessage;
	char * ptr, * pptr;
	size_t len;
	char Dollar[] = "$";
	char CR[] = "\r";
	char num[20];
	int Msgs = 0, Unread = 0;

	ptr = strchr(OldP, '$');

	while (ptr)
	{
		len = ptr - OldP;		// Chars before $
		memcpy(NewP, OldP, len);
		NewP += len;

		switch (*++ptr)
		{
		case 'I': // First name of the connected user.

			pptr = conn->UserPointer->Name;
			break;

		case 'L': // Number of the latest message.

			sprintf(num, "%d", LatestMsg);
			pptr = num;
			break;

		case 'N': // Number of active messages.

			sprintf(num, "%d", NumberofMessages);
			pptr = num;
			break;

		case 'U': // Callsign of the connected user.

			pptr = conn->UserPointer->Call;
			break;

		case 'W': // Inserts a carriage return.

			pptr = CR;
			break;

		case 'Z': // Last message read by the user (L command).

			sprintf(num, "%d", conn->UserPointer->lastmsg);
			pptr = num;
			break;

		case 'X': // Number of messages for the user.

			Msgs = CountMessagesTo(conn->UserPointer, &Unread);
			sprintf(num, "%d", Msgs);
			pptr = num;
			break;

		case 'x': // Number of new messages for the user.

			Msgs = CountMessagesTo(conn->UserPointer, &Unread);
			sprintf(num, "%d", Unread);
			pptr = num;
			break;

		case 'F': // Number of new messages to forward to this BBS.

			Msgs = CountMessagestoForward(conn->UserPointer);
			sprintf(num, "%d", Msgs);
			pptr = num;
			break;

		default:

			pptr = Dollar;		// Just Copy $
		}

		len = strlen(pptr);
		memcpy(NewP, pptr, len);
		NewP += len;

		OldP = ++ptr;
		ptr = strchr(OldP, '$');
	}

	strcpy(NewP, OldP);

	len = RemoveLF(NewMessage, (int)strlen(NewMessage));

	WriteLogLine(conn, '>', NewMessage,  (int)len, LOG);
	QueueMsg(conn, NewMessage, (int)len);
}

BOOL isdigits(char * string)
{
	// Returns TRUE id sting is decimal digits

	size_t i, n = strlen(string);
	
	for (i = 0; i < n; i++)
	{
		if (isdigit(string[i]) == FALSE) return FALSE;
	}
	return TRUE;
}

BOOL wildcardcompare(char * Target, char * Match)
{
	// Do a compare with string *string string* *string*

	// Strings should all be UC

	char Pattern[100];
	char * firststar;

	strcpy(Pattern, Match);
	firststar = strchr(Pattern,'*');

	if (firststar)
	{
		size_t Len = strlen(Pattern);

		if (Pattern[0] == '*' && Pattern[Len - 1] == '*')		// * at start and end
		{
			Pattern[Len - 1] = 0;
			return !(strstr(Target, &Pattern[1]) == NULL);
		}
		if (Pattern[0] == '*')		// * at start
		{
			// Compare the last len - 1 chars of Target

			size_t Targlen = strlen(Target);
			size_t Comparelen = Targlen - (Len - 1);

			if (Len == 1)			// Just *
				return TRUE;

			if (Comparelen < 0)	// Too Short
				return FALSE;

			return (memcmp(&Target[Comparelen], &Pattern[1], Len - 1) == 0);
		}

		// Must be * at end - compare first Len-1 char

		return (memcmp(Target, Pattern, Len - 1) == 0);
	}

	// No WildCards - straight strcmp
	return (strcmp(Target, Pattern) == 0);
}

#ifndef LINBPQ

PrintMessage(HDC hDC, struct MsgInfo * Msg);

PrintMessages(HWND hDlg, int Count, int * Indexes)
{
	int i, CurrentMsgIndex;
	char MsgnoText[10];
	int Msgno;
	struct MsgInfo * Msg;
	int Len = MAX_PATH;
	BOOL hResult;
    PRINTDLG pdx = {0};
	HDC hDC;

//	CHOOSEFONT cf; 
	LOGFONT lf; 
    HFONT hFont; 
 
 
    //  Initialize the PRINTDLG structure.

    pdx.lStructSize = sizeof(PRINTDLG);
    pdx.hwndOwner = hWnd;
    pdx.hDevMode = NULL;
    pdx.hDevNames = NULL;
    pdx.hDC = NULL;
    pdx.Flags = PD_RETURNDC | PD_COLLATE;
    pdx.nMinPage = 1;
    pdx.nMaxPage = 1000;
    pdx.nCopies = 1;
    pdx.hInstance = 0;
    pdx.lpPrintTemplateName = NULL;
    
    //  Invoke the Print property sheet.
    
    hResult = PrintDlg(&pdx);

	memset(&lf, 0, sizeof(LOGFONT));

 /*
 
	// Initialize members of the CHOOSEFONT structure.  
 
    cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner = (HWND)NULL; 
    cf.hDC = pdx.hDC; 
    cf.lpLogFont = &lf; 
    cf.iPointSize = 0; 
    cf.Flags = CF_PRINTERFONTS | CF_FIXEDPITCHONLY; 
    cf.rgbColors = RGB(0,0,0); 
    cf.lCustData = 0L; 
    cf.lpfnHook = (LPCFHOOKPROC)NULL; 
    cf.lpTemplateName = (LPSTR)NULL; 
    cf.hInstance = (HINSTANCE) NULL; 
    cf.lpszStyle = (LPSTR)NULL; 
    cf.nFontType = PRINTER_FONTTYPE; 
    cf.nSizeMin = 0; 
    cf.nSizeMax = 0; 
 
    // Display the CHOOSEFONT common-dialog box.  
 
    ChooseFont(&cf); 
 
    // Create a logical font based on the user's  
    // selection and return a handle identifying  
    // that font. 
*/

	lf.lfHeight =  -56;
	lf.lfWeight = 600;
	lf.lfOutPrecision = 3;
	lf.lfClipPrecision = 2;
	lf.lfQuality = 1;
	lf.lfPitchAndFamily = '1';
	strcpy (lf.lfFaceName, "Courier New");

    hFont = CreateFontIndirect(&lf); 

    if (hResult)
    {
        // User clicked the Print button, so use the DC and other information returned in the 
        // PRINTDLG structure to print the document.

		DOCINFO pdi;

		pdi.cbSize = sizeof(DOCINFO);
		pdi.lpszDocName = "BBS Message Print";
		pdi.lpszOutput = NULL;
		pdi.lpszDatatype = "RAW";
		pdi.fwType = 0;

		hDC = pdx.hDC;

		SelectObject(hDC, hFont);

		StartDoc(hDC, &pdi);
		StartPage(hDC);

		for (i = 0; i < Count; i++)
		{
			SendDlgItemMessage(hDlg, 0, LB_GETTEXT, Indexes[i], (LPARAM)(LPCTSTR)&MsgnoText);
	
			Msgno = atoi(MsgnoText);

			for (CurrentMsgIndex = 1; CurrentMsgIndex <= NumberofMessages; CurrentMsgIndex++)
			{
				Msg = MsgHddrPtr[CurrentMsgIndex];
	
				if (Msg->number == Msgno)
				{
					PrintMessage(hDC, Msg);
					break;
				}
			}
		}
		
		EndDoc(hDC);
	}

    if (pdx.hDevMode != NULL) 
        GlobalFree(pdx.hDevMode); 
    if (pdx.hDevNames != NULL) 
        GlobalFree(pdx.hDevNames); 

    if (pdx.hDC != NULL) 
        DeleteDC(pdx.hDC);

	return 0;
}

PrintMessage(HDC hDC, struct MsgInfo * Msg)
{
	int Len = MAX_PATH;
	char * MsgBytes;
	char * Save;
  	int Msglen;
 
	StartPage(hDC);

	Save = MsgBytes = ReadMessageFile(Msg->number);

	Msglen = Msg->length;

	if (MsgBytes)
	{
		char Hddr[1000];
		char FullTo[100];
		int HRes, VRes;
		char * ptr1, * ptr2;
		int LineLen;

		RECT Rect;

		if (_stricmp(Msg->to, "RMS") == 0)
			 sprintf(FullTo, "RMS:%s", Msg->via);
		else
		if (Msg->to[0] == 0)
			sprintf(FullTo, "smtp:%s", Msg->via);
		else
			strcpy(FullTo, Msg->to);


		sprintf(Hddr, "From: %s%s\r\nTo: %s\r\nType/Status: %c%c\r\nDate/Time: %s\r\nBid: %s\r\nTitle: %s\r\n\r\n",
			Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime((time_t)Msg->datecreated, FALSE), Msg->bid, Msg->title);


		if (Msg->B2Flags & B2Msg)
		{
			// Remove B2 Headers (up to the File: Line)
			
			char * ptr;
			ptr = strstr(MsgBytes, "Body:");
			if (ptr)
			{
				Msglen = atoi(ptr + 5);
				ptr = strstr(ptr, "\r\n\r\n");
			}
			if (ptr)
				MsgBytes = ptr + 4;
		}

		HRes = GetDeviceCaps(hDC, HORZRES) - 50;
		VRes = GetDeviceCaps(hDC, VERTRES) - 50;

		Rect.top = 50;
		Rect.left = 50;
		Rect.right = HRes;
		Rect.bottom = VRes;

		DrawText(hDC, Hddr, strlen(Hddr), &Rect, DT_CALCRECT | DT_WORDBREAK);
		DrawText(hDC, Hddr, strlen(Hddr), &Rect, DT_WORDBREAK);

		// process message a line at a time. When page is full, output a page break

		ptr1 = MsgBytes;
		ptr2 = ptr1;

		while (Msglen-- > 0)
		{	
			if (*ptr1++ == '\r')
			{
				// Output this line

				// First check if it will fit

				Rect.top = Rect.bottom;
				Rect.right = HRes;
				Rect.bottom = VRes;

				LineLen = ptr1 - ptr2 - 1;
			
				if (LineLen == 0)		// Blank line
					Rect.bottom = Rect.top + 40;
				else
					DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_CALCRECT | DT_WORDBREAK);

				if (Rect.bottom >= VRes)
				{
					EndPage(hDC);
					StartPage(hDC);

					Rect.top = 50;
					Rect.bottom = VRes;
					if (LineLen == 0)		// Blank line
						Rect.bottom = Rect.top + 40;
					else
						DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_CALCRECT | DT_WORDBREAK);
				}

				if (LineLen == 0)		// Blank line
					Rect.bottom = Rect.top + 40;
				else
					DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_WORDBREAK);

				if (*(ptr1) == '\n')
				{
					ptr1++;
					Msglen--;
				}

				ptr2 = ptr1;
			}
		}
	
		free(Save);
	
		EndPage(hDC);

		}
		return 0;
}

#endif


int ImportMessages(CIRCUIT * conn, char * FN, BOOL Nopopup)
{
	char FileName[MAX_PATH] = "Messages.in";
	int Files = 0;
	int WriteLen=0;
	FILE *in;
	CIRCUIT dummyconn;
	struct UserInfo User;	
	int Index = 0;
			
	char Buffer[100000];
	char *buf = Buffer;

	if (FN[0])			// Name supplled
		strcpy(FileName, FN);

	else
	{
#ifndef LINBPQ
		OPENFILENAME Ofn; 

		memset(&Ofn, 0, sizeof(Ofn));
 
		Ofn.lStructSize = sizeof(OPENFILENAME); 
		Ofn.hInstance = hInst;
		Ofn.hwndOwner = MainWnd; 
		Ofn.lpstrFilter = NULL; 
		Ofn.lpstrFile= FileName; 
		Ofn.nMaxFile = sizeof(FileName)/ sizeof(*FileName); 
		Ofn.lpstrFileTitle = NULL; 
		Ofn.nMaxFileTitle = 0; 
		Ofn.lpstrInitialDir = BaseDir; 
		Ofn.Flags = OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST; 
		Ofn.lpstrTitle = NULL;//; 

		if (!GetOpenFileName(&Ofn))
			return 0;
#endif
	}

	in = fopen(FileName, "rb");

	if (!(in))
	{
		char msg[500];
		sprintf_s(msg, sizeof(msg), "Failed to open %s", FileName);
		if (conn)
			nodeprintf(conn, "%s\r", msg);
#ifdef WIN32
		else
			if (Nopopup == FALSE)
				MessageBox(NULL, msg, "BPQMailChat", MB_OK);
#endif
		return 0;
	}

	memset(&dummyconn, 0, sizeof(CIRCUIT));
	memset(&User, 0, sizeof(struct UserInfo));

	if (conn == 0)
	{	
		conn = &dummyconn;

		dummyconn.UserPointer = &User;	// Was SYSOPCall, but I think that is wrong.
		strcpy(User.Call, "IMPORT");
		User.flags |= F_EMAIL;
		dummyconn.sysop = TRUE;
		dummyconn.BBSFlags = BBS;
		
		strcpy(dummyconn.Callsign, "IMPORT");
	}

	while(fgets(Buffer, 99999, in))
	{
		// First line should start SP/SB ?ST?

		char * From = NULL;
		char * BID = NULL;
		char * ATBBS = NULL;
		char seps[] = " \t\r";
		struct MsgInfo * Msg;
		char To[100]= "";
		int msglen;
		char * Context;
		char * Arg1, * Cmd;

NextMessage:

		From = NULL;
		BID = NULL;
		ATBBS = NULL;
		To[0]= 0;

		Sleep(100);

		strlop(Buffer, 10);
		strlop(Buffer, 13);				// Remove cr and/or lf

		if (Buffer[0] == 0)			//Blank Line
			continue;

		WriteLogLine(conn, '>', Buffer, (int)strlen(Buffer), LOG_BBS);

		if (dummyconn.sysop == 0)
		{
			nodeprintf(conn, "%s\r", Buffer);
			Flush(conn);
		}
 
		Cmd = strtok_s(Buffer, seps, &Context);

		if (Cmd == NULL)
		{
			fclose(in);
			return Files;
		}

		Arg1 = strtok_s(NULL, seps, &Context);

		if (Arg1 == NULL)
		{
			if (dummyconn.sysop)
				Debugprintf("Bad Import Line %s", Buffer);
			else
				nodeprintf(conn, "Bad Import Line %s\r", Buffer);

			fclose(in);
			return Files;
		}

		strcpy(To, Arg1);

		if (DecodeSendParams(conn, Context, &From, To, &ATBBS, &BID))
		{
			if (CreateMessage(conn, From, To, ATBBS, toupper(Cmd[1]), BID, NULL))
			{
				Msg = conn->TempMsg;

				// SP is Ok, read message;

				ClearQueue(conn);

				fgets(Buffer, 99999, in);
				strlop(Buffer, 10);
				strlop(Buffer, 13);				// Remove cr and/or lf
				if (strlen(Buffer) > 60)
					Buffer[60] = 0;

				strcpy(Msg->title, Buffer);

				// Read the lines
		
				conn->Flags |= GETTINGMESSAGE;

				Buffer[0] = 0;

				fgets(Buffer, 99999, in);

				while ((conn->Flags & GETTINGMESSAGE) && Buffer[0])
				{
					strlop(Buffer, 10);
					strlop(Buffer, 13);				// Remove cr and/or lf
					msglen = (int)strlen(Buffer);
					Buffer[msglen++] = 13;
					ProcessMsgLine(conn, conn->UserPointer,Buffer, msglen);
	
					Buffer[0] = 0;
					fgets(Buffer, 99999, in);
				}

				// Message completed (or off end of file)

				Files ++;

				ClearQueue(conn);
		
				if (Buffer[0])
					goto NextMessage;		// We have read the SP/SB line;
				else
				{
					fclose(in);
					return Files;
				}
			}
			else
			{
				// Create failed
			
				Flush(conn);
			}
		}

		// Search for next message 

		Buffer[0] = 0;
		fgets(Buffer, 99999, in);

		while (Buffer[0])
		{
			strlop(Buffer, 10);
			strlop(Buffer, 13);				// Remove cr and/or lf

			if (_stricmp(Buffer, "/EX") == 0)
			{
				// Found end

				Buffer[0] = 0;
				fgets(Buffer, 99999, in);
			
				if (dummyconn.sysop)
					ClearQueue(conn);
				else
					Flush(conn);

				if (Buffer[0])
					goto NextMessage;		// We have read the SP/SB line;
			}
			
			Buffer[0] = 0;
			fgets(Buffer, 99999, in);
		}
	}

	fclose(in);

	if (dummyconn.sysop)
		ClearQueue(conn);
	else
		Flush(conn);

	return Files;
}
char * ReadMessageFileEx(struct MsgInfo * MsgRec)
{
	// Sets Message Size from File Size

	int msgno = MsgRec->number;
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	char * MsgBytes;
	struct stat STAT;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, msgno);

	if (stat(MsgFile, &STAT) == -1)
		return NULL;

	FileSize = STAT.st_size;

	hFile = fopen(MsgFile, "rb");

	if (hFile == NULL)
		return NULL;

	MsgBytes=malloc(FileSize+1);

	fread(MsgBytes, 1, FileSize, hFile); 

	fclose(hFile);

	MsgBytes[FileSize]=0;
	MsgRec->length = FileSize;

	return MsgBytes;
}

char * ReadMessageFile(int msgno)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	char * MsgBytes;
	struct stat STAT;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, msgno);

	if (stat(MsgFile, &STAT) == -1)
		return NULL;

	FileSize = STAT.st_size;

	hFile = fopen(MsgFile, "rb");

	if (hFile == NULL)
		return NULL;

	MsgBytes = malloc(FileSize + 100);	// A bit of space for alias substitution on B2

	fread(MsgBytes, 1, FileSize, hFile); 

	fclose(hFile);

	MsgBytes[FileSize]=0;

	return MsgBytes;
}


int QueueMsg(ConnectionInfo * conn, char * msg, int len)
{
	// Add Message to queue for this connection

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	// Create or extend buffer

	GetSemaphore(&OutputSEM, 0);

	conn->OutputQueue=realloc(conn->OutputQueue, conn->OutputQueueLength + len);

	if (conn->OutputQueue == NULL)
	{
		// relloc failed - should never happen, but clean up

		CriticalErrorHandler("realloc failed to expand output queue");
		FreeSemaphore(&OutputSEM);
		return 0;
	}

	memcpy(&conn->OutputQueue[conn->OutputQueueLength], msg, len);
	conn->OutputQueueLength += len;
	FreeSemaphore(&OutputSEM);

	return len;
}

void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ConnectionInfo * conn;
	struct ConsoleInfo * Cons;

	int n;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
		{
			Flush(conn);

			// if an FLARQ mail has been sent see if queues have cleared

			if (conn->BBSFlags & YAPPTX)
			{
				YAPPSendData(conn);
			}
			else if (conn->OutputQueue == NULL && (conn->BBSFlags & ARQMAILACK))
			{
				int n = TXCount(conn->BPQStream);		// All Sent and Acked?
	
				if (n == 0)
				{
					struct MsgInfo * Msg = conn->FwdMsg;

					conn->ARQClearCount--;

					if (conn->ARQClearCount <= 0)
					{
						Logprintf(LOG_BBS, conn, '>', "ARQ Send Complete");

						// Mark mail as sent, and look for more
	
						clear_fwd_bit(Msg->fbbs, conn->UserPointer->BBSNumber);
						set_fwd_bit(Msg->forw, conn->UserPointer->BBSNumber);

						//  Only mark as forwarded if sent to all BBSs that should have it
			
						if (memcmp(Msg->fbbs, zeros, NBMASK) == 0)
						{
							Msg->status = 'F';			// Mark as forwarded
							Msg->datechanged=time(NULL);
						}
	
						conn->BBSFlags &= ~ARQMAILACK;
						conn->UserPointer->ForwardingInfo->MsgCount--;

						SaveMessageDatabase();
						SendARQMail(conn);				// See if any more - close if not
					}
				}
				else
					conn->ARQClearCount = 10;
			}
		}
	}
#ifndef LINBPQ
	for (Cons = ConsHeader[0]; Cons; Cons = Cons->next)
	{
		if (Cons->Console)
			Flush(Cons->Console);
	}
#endif
}


void Flush(CIRCUIT * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	//	BOOL Paging;				// Set if user wants paging
	//	int LinesSent;				// Count when paging
	//	int PageLen;				// Lines per page


	if (conn->OutputQueue == NULL)
	{
		// Nothing to send. If Close after Flush is set, disconnect

		if (conn->CloseAfterFlush)
		{
			conn->CloseAfterFlush--;
			
			if (conn->CloseAfterFlush)
				return;

			Disconnect(conn->BPQStream);
			conn->ErrorCount = 0;
		}

		return;						// Nothing to send
	}
	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 15)
			return;						// Busy

		if (conn->BBSFlags & SYSOPCHAT)		// Suspend queued output while sysop chatting
			return;

		if (conn->Paging && (conn->LinesSent >= conn->PageLen))
			return;

		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		GetSemaphore(&OutputSEM, 0);

		if (conn->Paging)
		{
			// look for CR chars in message to send. Increment LinesSent, and stop if at limit

			UCHAR * ptr1 = &conn->OutputQueue[conn->OutputGetPointer];
			UCHAR * ptr2;
			int lenleft = len;

			ptr2 = memchr(ptr1, 0x0d, len);

			while (ptr2)
			{
				conn->LinesSent++;
				ptr2++;
				lenleft = len - (int)(ptr2 - ptr1);

				if (conn->LinesSent >= conn->PageLen)
				{
					len = (int)(ptr2 - &conn->OutputQueue[conn->OutputGetPointer]);
					
					SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);
					conn->OutputGetPointer+=len;
					tosend-=len;
					SendUnbuffered(conn->BPQStream, "<A>bort, <CR> Continue..>", 25);
					FreeSemaphore(&OutputSEM);
					return;

				}
				ptr2 = memchr(ptr2, 0x0d, lenleft);
			}
		}

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		FreeSemaphore(&OutputSEM);

		tosend-=len;	
		sent++;

		if (sent > 15)
			return;
	}

	// All Sent. Free buffers and reset pointers

	conn->LinesSent = 0;

	ClearQueue(conn);
}

VOID ClearQueue(ConnectionInfo * conn)
{
	if (conn->OutputQueue == NULL)
		return;

	GetSemaphore(&OutputSEM, 0);
	
	free(conn->OutputQueue);

	conn->OutputQueue=NULL;
	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;

	FreeSemaphore(&OutputSEM);
}



VOID FlagAsKilled(struct MsgInfo * Msg, BOOL SaveDB)
{
	struct UserInfo * user;

	Msg->status='K';
	Msg->datechanged=time(NULL);

	// Remove any forwarding references

	if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
	{	
		for (user = BBSChain; user; user = user->BBSNext)
		{
			if (check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount--;
				clear_fwd_bit(Msg->fbbs, user->BBSNumber);
			}
		}
	}
	if (SaveDB)
		SaveMessageDatabase();
	RebuildNNTPList();
}

void DoDeliveredCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	struct MsgInfo * Msg;
	
	while (Arg1)
	{
		msgno = atoi(Arg1);
				
		if (msgno > 100000)
		{
			BBSputs(conn, "Message Number too high\r");
			return;
		}

		Msg = GetMsgFromNumber(msgno);

		if (Msg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			goto Next;
		}

		if (Msg->type != 'T')
		{
			nodeprintf(conn, "Message %d not an NTS Message\r", msgno);
			goto Next;
		}

		if (Msg->status == 'N')
			nodeprintf(conn, "Warning - Message has status N\r");

		Msg->status = 'D';
		Msg->datechanged=time(NULL);
		SaveMessageDatabase();

		nodeprintf(conn, "Message #%d Flagged as Delivered\r", msgno);
	Next:
		Arg1 = strtok_s(NULL, " \r", &Context);
	}

	return;
}

void DoUnholdCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;
	
	// Param is either ALL or a list of numbers

	if (Arg1 == NULL)
	{
		nodeprintf(conn, "No message number\r");
		return;
	}

	if (_stricmp(Arg1, "ALL") == 0)
	{
		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if (Msg->status == 'H')
			{
				if (Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
					Msg->status = '$';				// Has forwarding
				else
					Msg->status = 'N';
				
				nodeprintf(conn, "Message #%d Unheld\r", Msg->number);
			}
		}
		return;
	}

	while (Arg1)
	{
		msgno = atoi(Arg1);
		Msg = GetMsgFromNumber(msgno);
		
		if (Msg)
		{
			if (Msg->status == 'H')
			{
				if (Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
					Msg->status = '$';				// Has forwarding
				else
					Msg->status = 'N';

				nodeprintf(conn, "Message #%d Unheld\r", msgno);
			}
			else
			{
				nodeprintf(conn, "Message #%d was not held\r", msgno);
			}
		}
		else
				nodeprintf(conn, "Message #%d not found\r", msgno);
		
		Arg1 = strtok_s(NULL, " \r", &Context);
	}

	return;
}

void DoKillCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;
	
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just K

		while (Arg1)
		{
			msgno = atoi(Arg1);
			KillMessage(conn, user, msgno);

			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		SaveMessageDatabase();
		return;

	case 'M':					// Kill Mine

		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0) || (conn->sysop && _stricmp(Msg->to, "SYSOP") == 0 && user->flags & F_SYSOP_IN_LM))
			{
				if (Msg->type == 'P' && Msg->status == 'Y')
				{
					FlagAsKilled(Msg, FALSE);
					nodeprintf(conn, "Message #%d Killed\r", Msg->number);
				}
			}
		}

		SaveMessageDatabase();
		return;

	case 'H':					// Kill Held

		if (conn->sysop)
		{
			for (i=NumberofMessages; i>0; i--)
			{
				Msg = MsgHddrPtr[i];

				if (Msg->status == 'H')
				{
					FlagAsKilled(Msg, FALSE);
					nodeprintf(conn, "Message #%d Killed\r", Msg->number);
				}
			}
		}
		SaveMessageDatabase();
		return;

	case '>':			// K> - Kill to 

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesTo(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
			return;
		}

	case '<':

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesFrom(conn, user, Arg1) == 0);
					BBSputs(conn, "No Messages found\r");

					return;
		}
	}

	nodeprintf(conn, "*** Error: Invalid Kill option %c\r", Cmd[1]);

	return;

}

int KillMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;

	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->to, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}

	SaveMessageDatabase();
	return(Msgs);
}

int KillMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;


	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->from, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}

	SaveMessageDatabase();	
	return(Msgs);
}

BOOL OkToKillMessage(BOOL SYSOP, char * Call, struct MsgInfo * Msg)
{	
	if (SYSOP || (Msg->type == 'T' && UserCantKillT == FALSE))
		return TRUE;
	
	if (Msg->type == 'P')
		if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
			return TRUE;

	if (Msg->type == 'B')
		if (_stricmp(Msg->from, Call) == 0)
			return TRUE;

	return FALSE;
}

void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;

	Msg = GetMsgFromNumber(msgno);

	if (Msg == NULL || Msg->status == 'K')
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	if (OkToKillMessage(conn->sysop, user->Call, Msg))
	{
		FlagAsKilled(Msg, FALSE);
		nodeprintf(conn, "Message #%d Killed\r", msgno);
	}
	else
		nodeprintf(conn, "Not your message\r");
}


BOOL ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn, struct TempUserInfo * Temp)
{
	char FullFrom[80];
	char FullTo[80];

	strcpy(FullFrom, Msg->from);

	if ((_stricmp(Msg->from, "RMS:") == 0) || (_stricmp(Msg->from, "SMTP:") == 0) || 
		Temp->SendFullFrom || (_stricmp(Msg->emailfrom, "@winlink.org") == 0))
		strcat(FullFrom, Msg->emailfrom);

	if (_stricmp(Msg->to, "RMS") == 0)
	{
		sprintf(FullTo, "RMS:%s", Msg->via);
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s %-6s %-s\r",
				Msg->number, FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, FullTo, FullFrom, Msg->title);
	}
	else

	if (Msg->to[0] == 0 && Msg->via[0] != 0)
	{
		sprintf(FullTo, "smtp:%s", Msg->via);
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s %-6s %-s\r",
				Msg->number, FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, FullTo, FullFrom, Msg->title);
	}

	else
		if (Msg->via[0] != 0)
		{
			char Via[80];
			strcpy(Via, Msg->via);
			strlop(Via, '.');			// Only show first part of via
			nodeprintf(conn, "%-6d %s %c%c   %5d %-7s@%-6s %-6s %-s\r",
				Msg->number, FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Via, FullFrom, Msg->title);
		}
		else
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s        %-6s %-s\r",
				Msg->number, FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, FullFrom, Msg->title);
	
	//	if paging, stop two before page lengh. This lets us send the continue prompt, save status
	//	and exit without triggering the system paging code. We can then read a message then resume listing

	if (Temp->ListActive && conn->Paging)
	{
		Temp->LinesSent++;

		if ((Temp->LinesSent + 1) >= conn->PageLen)
		{
			nodeprintf(conn, "<A>bort, <R Msg(s)>, <CR> = Continue..>");
			Temp->LastListedInPagedMode = Msg->number;
			Temp->ListSuspended = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}

void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, BOOL Resuming, char * Context)
{
	struct  TempUserInfo * Temp = user->Temp;
	struct MsgInfo * Msg;

	// Allow compound selection, eg LTN or LFP

	// types P N T
	// Options LL LR L< L> L@ LM LC (L* used internally for just L, ie List New
	// Status N Y H F K D 

	// Allowing options in any order complicates paging. May be best to parse options once and restore if paging.

	Temp->ListActive = TRUE;
	Temp->LinesSent = 0;

	if (Resuming)
	{
		// Entered after a paging pause. Selection fields are already set up

		// We have reentered list command after a pause. The next message to list is in Temp->LastListedInPagedMode

//		Start = Temp->LastListedInPagedMode;
		Temp->ListSuspended = FALSE;
	}
	else
	{
		Temp->ListRangeEnd = LatestMsg;
		Temp->ListRangeStart = 1;
		Temp->LLCount = 0;
		Temp->SendFullFrom = 0;
		Temp->ListType = 0;
		Temp->ListStatus = 0;
		Temp->ListSelector = 0;
		Temp->UpdateLatest = 0;
		Temp->LastListParams[0] = 0;
		Temp->IncludeKilled = 1;			// SYSOP include Killed except LM

		//Analyse L params. 

		_strupr(Cmd);

		if (strcmp(Cmd, "LC") == 0)			// List Bull Categories
		{
			ListCategories(conn);
			return;
		}

		// if command is just L or LR start from last listed

		if (Arg1 == NULL)
		{
			if (strcmp(Cmd, "L") == 0 || strcmp(Cmd, "LR") == 0)
			{
				if (LatestMsg == conn->lastmsg)
				{
					BBSputs(conn, "No New Messages\r");
					return;
				}

				Temp->UpdateLatest = 1;
				Temp->ListRangeStart = conn->lastmsg;
			}
		}

		if (strchr(Cmd, 'V'))					// Verbose
			Temp->SendFullFrom = 'V';

		if (strchr(Cmd, 'R'))
			Temp->ListDirn = 'R';
		else
			Temp->ListDirn = '*';				// Default newest first

		Cmd++;					// skip L

		if (strchr(Cmd, 'T'))
			Temp->ListType = 'T';
		else if (strchr(Cmd, 'P'))
			Temp->ListType = 'P';
		else if (strchr(Cmd, 'B'))
			Temp->ListType = 'B';

		if (strchr(Cmd, 'N'))
			Temp->ListStatus = 'N';
		else if (strchr(Cmd, 'Y'))
			Temp->ListStatus = 'Y';
		else if (strchr(Cmd, 'F'))
			Temp->ListStatus = 'F';
		else if (strchr(Cmd, '$'))
			Temp->ListStatus = '$';
		else if (strchr(Cmd, 'H'))
			Temp->ListStatus = 'H';
		else if (strchr(Cmd, 'K'))
			Temp->ListStatus = 'K';
		else if (strchr(Cmd, 'D'))
			Temp->ListStatus = 'D';

		// H or K only by Sysop

		switch (Temp->ListStatus)
		{
		case 'K':
		case 'H':				// List Status

			if (conn->sysop)
				break;
			
			BBSputs(conn, "LH or LK can only be used by SYSOP\r");
			return;
		}

		if (strchr(Cmd, '<'))
			Temp->ListSelector = '<';
		else if (strchr(Cmd, '>'))
			Temp->ListSelector = '>';
		else if (strchr(Cmd, '@'))
			Temp->ListSelector = '@';
		else if (strchr(Cmd, 'M'))
		{
			Temp->ListSelector = 'M';
			Temp->IncludeKilled = FALSE;
		}

		// Param could be single number, number range or call
		
		if (Arg1)
		{
			if (strchr(Cmd, 'L'))		// List Last 
			{
				// Param is number

				if (Arg1)
					Temp->LLCount = atoi(Arg1);
			}
			else
			{
				// Range nnn-nnn or single value or callsign

				char * Arg2, * Arg3, * Range;
				char seps[] = " \t\r";
				UINT From=LatestMsg, To=0;

				Arg2 = strtok_s(NULL, seps, &Context);
				Arg3 = strtok_s(NULL, seps, &Context);

				if (Temp->ListSelector && Temp->ListSelector != 'M')
				{
					// < > or @ - first param is callsign

					strcpy(Temp->LastListParams, Arg1);

					// Just possible number range

					Arg1 = Arg2;
					Arg2 = Arg3;
					Arg3 = strtok_s(NULL, seps, &Context);			
				}

				if (Arg1)
				{
					Range = strchr(Arg1, '-');
			
					// A number could be a Numeric Bull Dest (eg 44)
					// I think this can only resaonably be >

					if (isdigits(Arg1))
						To = From = atoi(Arg1);

					if (Arg2)
						From = atoi(Arg2);
					else
					{
						if (Range)
						{
							Arg3 = strlop(Arg1, '-');

							To = atoi(Arg1);
							
							if (Arg3 && Arg3[0])
								From = atoi(Arg3);
							else
								From = LatestMsg;
						}
					}
					if (From > 100000 || To > 100000)
					{
						BBSputs(conn, "Message Number too high\r");
						return;
					}
					Temp->ListRangeStart = To;
					Temp->ListRangeEnd = From;
				}
			}
		}
	}

	// Run through all messages (either forwards or backwards) and list any that match all selection criteria

	while (1) 
	{
		if (Temp->ListDirn == 'R')
			Msg = GetMsgFromNumber(Temp->ListRangeStart);
		else
			Msg = GetMsgFromNumber(Temp->ListRangeEnd);


		if (Msg && CheckUserMsg(Msg, user->Call, conn->sysop, Temp->IncludeKilled))		// Check if user is allowed to list this message
		{
			// Check filters

			if (Temp->ListStatus && Temp->ListStatus != Msg->status)
				goto skip;

			if (Temp->ListType && Temp->ListType != Msg->type)
				goto skip;

			if (Temp->ListSelector == '<')
				if (_stricmp(Msg->from, Temp->LastListParams) != 0)
					goto skip;

			if (Temp->ListSelector == '>')
				if (_stricmp(Msg->to, Temp->LastListParams) != 0)
					goto skip;

			if (Temp->ListSelector == '@')
				if (_memicmp(Msg->via, Temp->LastListParams, strlen(Temp->LastListParams)) != 0 &&
					(_stricmp(Temp->LastListParams, "SMTP:") != 0 || Msg->to[0] != 0))
						goto skip;

			if (Temp->ListSelector == 'M')
				if (_stricmp(Msg->to, user->Call) != 0 &&
					(_stricmp(Msg->to, "SYSOP") != 0 || ((user->flags & F_SYSOP_IN_LM) == 0)))

					goto skip;

			if (ListMessage(Msg, conn, Temp))
			{
				if (Temp->ListDirn == 'R')
					Temp->ListRangeStart++;
				else
					Temp->ListRangeEnd--;

				return;			// Hit page limit
			}

			if (Temp->LLCount)
			{
				Temp->LLCount--;
				if (Temp->LLCount == 0)
					return;				// LL count reached
			}
skip:;	
		}

		if (Temp->ListRangeStart == Temp->ListRangeEnd)
		{
			// if using L or LR (list new) update last listed field
			
			if (Temp->UpdateLatest)
				conn->lastmsg = LatestMsg;

			return;
		}

		if (Temp->ListDirn == 'R')
			Temp->ListRangeStart++;
		else
			Temp->ListRangeEnd--;

		if (Temp->ListRangeStart > 100000 || Temp->ListRangeEnd < 0)		// Loop protection!
			return;		

	}

/*

	switch (Cmd[0])
	{

	case '*':					// Just L
	case 'R':				// LR = List Reverse

		if (Arg1)
		{
			// Range nnn-nnn or single value

			char * Arg2, * Arg3;
			char * Context;
			char seps[] = " -\t\r";
			UINT From=LatestMsg, To=0;
			char * Range = strchr(Arg1, '-');
			
			Arg2 = strtok_s(Arg1, seps, &Context);
			Arg3 = strtok_s(NULL, seps, &Context);

			if (Arg2)
				To = From = atoi(Arg2);

			if (Arg3)
				From = atoi(Arg3);
			else
				if (Range)
					From = LatestMsg;

			if (From > 100000 || To > 100000)
			{
				BBSputs(conn, "Message Number too high\r");
				return;
			}

			if (Cmd[1] == 'R')
			{
				if (Start)
					To = Start + 1;

				ListMessagesInRangeForwards(conn, user, user->Call, From, To, Temp->SendFullFrom);
			}
			else
			{
				if (Start)
					From = Start - 1;

				ListMessagesInRange(conn, user, user->Call, From, To, Temp->SendFullFrom);
			}
		}
		else

			if (LatestMsg == conn->lastmsg)
				BBSputs(conn, "No New Messages\r");
			else if (Cmd[1] == 'R')
				ListMessagesInRangeForwards(conn, user, user->Call, LatestMsg, conn->lastmsg + 1, SendFullFrom);
			else
				ListMessagesInRange(conn, user, user->Call, LatestMsg, conn->lastmsg + 1, SendFullFrom);

			conn->lastmsg = LatestMsg;

		return;


	case 'L':				// List Last

		if (Arg1)
		{
			int i = atoi(Arg1);
			int m = NumberofMessages;

			if (Resuming)
				i = Temp->LLCount;
			else
				Temp->LLCount = i;
				
			for (; i>0 && m != 0; i--)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (Start && MsgHddrPtr[m]->number >= Start)
					{
						m--;
						i++;
						continue;
					}

					Temp->LLCount--;
					
					if (ListMessage(MsgHddrPtr[m], conn, SendFullFrom))
						return;			// Hit page limit
					m--;
				}
			}
		}
		return;

	case 'M':			// LM - List Mine

		if (ListMessagesTo(conn, user, user->Call, SendFullFrom, Start) == 0)
			BBSputs(conn, "No Messages found\r");
		return;

	case '>':			// L> - List to 

		if (Arg1)
			if (ListMessagesTo(conn, user, Arg1, SendFullFrom, Start) == 0)
				BBSputs(conn, "No Messages found\r");
		
		
		return;

	case '<':

		if (Arg1)
			if (ListMessagesFrom(conn, user, Arg1, SendFullFrom, Start) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case '@':

		if (Arg1)
			if (ListMessagesAT(conn, user, Arg1, SendFullFrom, Start) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case 'N':
	case 'Y':
	case 'F':
	case '$':
	case 'D':			// Delivered NTS Traffic can be listed by anyone
		{
			int m = NumberofMessages;
				
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (Start && MsgHddrPtr[m]->number >= Start)
					{
						m--;
						continue;
					}
			
					if (Temp->ListType)
					{
						if (MsgHddrPtr[m]->status == Cmd[1] && MsgHddrPtr[m]->type == Temp->ListType)
							if (ListMessage(MsgHddrPtr[m], conn, SendFullFrom))
								return;			// Hit page limit
					}
					else
					{
						if (MsgHddrPtr[m]->status == toupper(Cmd[1]))
							if (ListMessage(MsgHddrPtr[m], conn, SendFullFrom))
								return;			// Hit page limit
					}
					m--;
				}
			}
		}
		return;

	case 'K':
	case 'H':				// List Status

		if (conn->sysop)
		{
			int i, Msgs = Start;

			for (i=NumberofMessages; i>0; i--)
			{
				if (Start && MsgHddrPtr[i]->number >= Start)
					continue;

				if (MsgHddrPtr[i]->status == toupper(Cmd[1]))
				{
					Msgs++;
					if (ListMessage(MsgHddrPtr[i], conn, SendFullFrom))
						return;			// Hit page limit

				}
			}

			if (Msgs == 0)
				BBSputs(conn, "No Messages found\r");
		}
		else
				BBSputs(conn, "LH or LK can only be used by SYSOP\r");

		return;

	case 'C':
	{
		struct NNTPRec * ptr = FirstNNTPRec;
		char Cat[100];
		char NextCat[100];
		int Line = 0;
		int Count;

		while (ptr)
		{
			// if the next name is the same, combine  counts
			
			strcpy(Cat, ptr->NewsGroup);
			strlop(Cat, '.');
			Count = ptr->Count;
		Catloop:
			if (ptr->Next)
			{
				strcpy(NextCat, ptr->Next->NewsGroup);
				strlop(NextCat, '.');
				if (strcmp(Cat, NextCat) == 0)
				{
					ptr = ptr->Next;
					Count += ptr->Count;
					goto Catloop;
				}
			}

			nodeprintf(conn, "%-6s %-3d", Cat, Count);
			Line += 10;
			if (Line > 80)
			{
				Line = 0;
				nodeprintf(conn, "\r");
			}
			
			ptr = ptr->Next;
		}

		if (Line)
			nodeprintf(conn, "\r\r");
		else
			nodeprintf(conn, "\r");

		return;
	}
	}
	
	// Could be P B or T if specified without a status

	switch (Temp->ListType)
	{
	case 'P':
	case 'B':
	case 'T':			// NTS Traffic can be listed by anyone
	{
			int m = NumberofMessages;
							
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (Start && MsgHddrPtr[m]->number >= Start)
					{
						m--;
						continue;
					}

					if (MsgHddrPtr[m]->type == Temp->ListType)
						if (ListMessage(MsgHddrPtr[m], conn, SendFullFrom))
							return;			// Hit page limit
					m--;
				}
			}

			return;
		}
	}

*/	
	nodeprintf(conn, "*** Error: Invalid List option %c\r", Cmd[1]);

}

void ListCategories(ConnectionInfo * conn)
{
	// list bull categories
	struct NNTPRec * ptr = FirstNNTPRec;
	char Cat[100];
	char NextCat[100];
	int Line = 0;
	int Count;

	while (ptr)
	{
		// if the next name is the same, combine  counts

		strcpy(Cat, ptr->NewsGroup);
		strlop(Cat, '.');
		Count = ptr->Count;
Catloop:
		if (ptr->Next)
		{
			strcpy(NextCat, ptr->Next->NewsGroup);
			strlop(NextCat, '.');
			if (strcmp(Cat, NextCat) == 0)
			{
				ptr = ptr->Next;
				Count += ptr->Count;
				goto Catloop;
			}
		}

		nodeprintf(conn, "%-6s %-3d", Cat, Count);
		Line += 10;
		if (Line > 80)
		{
			Line = 0;
			nodeprintf(conn, "\r");
		}

		ptr = ptr->Next;
	}

	if (Line)
		nodeprintf(conn, "\r\r");
	else
		nodeprintf(conn, "\r");

	return;
}

/*
int ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call, BOOL SendFullFrom, int Start)
{
	int i, Msgs = Start;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (Start && MsgHddrPtr[i]->number >= Start)
			continue;

		if ((_stricmp(MsgHddrPtr[i]->to, Call) == 0) ||
			((conn->sysop) && _stricmp(Call, SYSOPCall) == 0 &&
			_stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0 && (user->flags & F_SYSOP_IN_LM)))
		{
			Msgs++;
			if (ListMessage(MsgHddrPtr[i], conn, Temp->SendFullFrom))
				break;			// Hit page limit
		}
	}
	
	return(Msgs);
}

int ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call, BOOL SendFullFrom, int Start)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (Start && MsgHddrPtr[i]->number >= Start)
			continue;

		if (_stricmp(MsgHddrPtr[i]->from, Call) == 0)
		{
			Msgs++;
			if (ListMessage(MsgHddrPtr[i], conn, Temp->SendFullFrom))
				return Msgs;			// Hit page limit

		}
	}
	
	return(Msgs);
}

int ListMessagesAT(ConnectionInfo * conn, struct UserInfo * user, char * Call, BOOL SendFullFrom,int Start)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (Start && MsgHddrPtr[i]->number >= Start)
			continue;

		if (_memicmp(MsgHddrPtr[i]->via, Call, strlen(Call)) == 0 ||
			(_stricmp(Call, "SMTP:") == 0 && MsgHddrPtr[i]->to[0] == 0))
		{
			Msgs++;
			if (ListMessage(MsgHddrPtr[i], conn, Temp->SendFullFrom))
				break;			// Hit page limit
		}
	}
	
	return(Msgs);
}
*/
int GetUserMsg(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];

		if (SYSOP) return m;			// Sysop can list or read anything
		
		if (Msg->status != 'K')
		{
	
			if (Msg->status != 'H')
			{
				if (Msg->type == 'B' || Msg->type == 'T') return m;

				if (Msg->type == 'P')
				{
					if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
						return m;
				}
			}
		}

		m--;

	} while (m> 0);

	return 0;
}


BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP, BOOL IncludeKilled)
{
	// Return TRUE if user is allowed to read message
	
	if (Msg->status == 'K' && IncludeKilled == 0)
		return FALSE;
		
	if (SYSOP)
		return TRUE;			// Sysop can list or read anything

	if ((Msg->status != 'K') && (Msg->status != 'H'))
	{
		if (Msg->type == 'B' || Msg->type == 'T') return TRUE;

		if (Msg->type == 'P')
		{
			if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
				return TRUE;
		}
	}

	return FALSE;
}
/*
int GetUserMsgForwards(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];
		
		if (Msg->status != 'K')
		{
			if (SYSOP) return m;			// Sysop can list or read anything

			if (Msg->status != 'H')
			{
				if (Msg->type == 'B' || Msg->type == 'T') return m;

				if (Msg->type == 'P')
				{
					if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
						return m;
				}
			}
		}

		m++;

	} while (m <= NumberofMessages);

	return 0;

}


void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End, BOOL SendFullFrom)
{
	int m;
	struct MsgInfo * Msg;

	for (m = Start; m >= End; m--)
	{
		Msg = GetMsgFromNumber(m);

		if (Msg && CheckUserMsg(Msg, user->Call, conn->sysop))
			if (ListMessage(Msg, conn, Temp->SendFullFrom))
				return;			// Hit page limit

	}
}


void ListMessagesInRangeForwards(ConnectionInfo * conn, struct UserInfo * user, char * Call, int End, int Start, BOOL SendFullFrom)
{
	int m;
	struct MsgInfo * Msg;

	for (m = Start; m <= End; m++)
	{
		Msg = GetMsgFromNumber(m);

		if (Msg && CheckUserMsg(Msg, user->Call, conn->sysop))
			if (ListMessage(Msg, conn, Temp->SendFullFrom))
				return;			// Hit page limit
	}
}
*/

void DoReadCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;


	switch (toupper(Cmd[1]))
	{
	case 0:					// Just R

		while (Arg1)
		{
			msgno = atoi(Arg1);
			if (msgno > 100000)
			{
				BBSputs(conn, "Message Number too high\r");
				return;
			}

			ReadMessage(conn, user, msgno);
			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		return;

	case 'M':					// Read Mine (Unread Messages)

		if (toupper(Cmd[2]) == 'R')
		{
			for (i = 1; i <= NumberofMessages; i++)
			{
				Msg = MsgHddrPtr[i];

				if ((_stricmp(Msg->to, user->Call) == 0) || (conn->sysop && _stricmp(Msg->to, "SYSOP") == 0 && user->flags & F_SYSOP_IN_LM))
					if (Msg->status == 'N')
						ReadMessage(conn, user, Msg->number);
			}
		}
		else
		{
			for (i = NumberofMessages; i > 0; i--)
			{
				Msg = MsgHddrPtr[i];

				if ((_stricmp(Msg->to, user->Call) == 0) || (conn->sysop && _stricmp(Msg->to, "SYSOP") == 0 && user->flags & F_SYSOP_IN_LM))
					if (Msg->status == 'N')
						ReadMessage(conn, user, Msg->number);
			}
		}

		return;
	}

	nodeprintf(conn, "*** Error: Invalid Read option %c\r", Cmd[1]);

	return;
}

int RemoveLF(char * Message, int len)
{
	// Remove lf chars and nulls

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		while (*ptr1 == 0 && len)
		{
			ptr1++;
			len--;
		}
		
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



int RemoveNulls(char * Message, int len)
{
	// Remove nulls

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		while (*ptr1 == 0 && len)
		{
			ptr1++;
			len--;
		}
		
		*ptr2 = *ptr1;

		ptr1++;
		ptr2++;
	}

	return (int)(ptr2 - Message);
}

void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;
	char * MsgBytes, * Save;
	char FullTo[100];
	int Index = 0;

	Msg = GetMsgFromNumber(msgno);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	if (!CheckUserMsg(Msg, user->Call, conn->sysop, TRUE))
	{
		nodeprintf(conn, "Message %d not for you\r", msgno);
		return;
	}

	if (_stricmp(Msg->to, "RMS") == 0)
		 sprintf(FullTo, "RMS:%s", Msg->via);
	else
	if (Msg->to[0] == 0)
		sprintf(FullTo, "smtp:%s", Msg->via);
	else
		strcpy(FullTo, Msg->to);


	nodeprintf(conn, "From: %s%s\rTo: %s\rType/Status: %c%c\rDate/Time: %s\rBid: %s\rTitle: %s\r\r",
		Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime((time_t)Msg->datecreated, FALSE), Msg->bid, Msg->title);

	MsgBytes = Save = ReadMessageFile(msgno);

	if (Msg->type == 'P')
		Index = PMSG;
	else if (Msg->type == 'B')
		Index = BMSG;
	else if (Msg->type == 'T')
		Index = TMSG;

	if (MsgBytes)
	{
		int Length = Msg->length;

		if (Msg->B2Flags & B2Msg)
		{
			char * ptr;
	
			// if message has attachments, display them if plain text

			if (Msg->B2Flags & Attachments)
			{
				char * FileName[100];
				int FileLen[100];
				int Files = 0;
				int BodyLen, NewLen;
				int i;
				char *ptr2;		
				char Msg[512];
				int Len;
		
				ptr = MsgBytes;
	
				Len = sprintf(Msg, "Message has Attachments\r\r");
				QueueMsg(conn, Msg, Len);

				while(*ptr != 13)
				{
					ptr2 = strchr(ptr, 10);	// Find CR

					if (memcmp(ptr, "Body: ", 6) == 0)
					{
						BodyLen = atoi(&ptr[6]);
					}

					if (memcmp(ptr, "File: ", 6) == 0)
					{
						char * ptr1 = strchr(&ptr[6], ' ');	// Find Space

						FileLen[Files] = atoi(&ptr[6]);

						FileName[Files++] = &ptr1[1];
						*(ptr2 - 1) = 0;
					}
				
					ptr = ptr2;
					ptr++;
				}

				ptr += 2;			// Over Blank Line and Separator 

				NewLen = RemoveLF(ptr, BodyLen);

				QueueMsg(conn, ptr, NewLen);		// Display Body

				ptr += BodyLen + 2;		// to first file

				for (i = 0; i < Files; i++)
				{
					char Msg[512];
					int Len, n;
					char * p = ptr;
					char c;

					// Check if message is probably binary

					int BinCount = 0;

					NewLen = RemoveLF(ptr, FileLen[i]);		// Removes LF agter CR but not on its own

					for (n = 0; n < NewLen; n++)
					{
						c = *p;
						
						if (c == 10)
							*p = 13;

						if (c==0 || (c & 128))
							BinCount++;

						p++;

					}

					if (BinCount > NewLen/10)
					{
						// File is probably Binary

						Len = sprintf(Msg, "\rAttachment %s is a binary file\r", FileName[i]);
						QueueMsg(conn, Msg, Len);
					}
					else
					{
						Len = sprintf(Msg, "\rAttachment %s\r\r", FileName[i]);
						QueueMsg(conn, Msg, Len);

						user->Total.MsgsSent[Index] ++;
						user->Total.BytesForwardedOut[Index] += NewLen;

						QueueMsg(conn, ptr, NewLen);
					}
				
					ptr += FileLen[i];
					ptr +=2;				// Over separator
				}
				goto sendEOM;
			}
			
			// Remove B2 Headers (up to the File: Line)
			
			ptr = strstr(MsgBytes, "Body:");

			if (ptr)
			{
				MsgBytes = ptr;
				Length = (int)strlen(ptr);
			}
		}

		// Remove lf chars

		Length = RemoveLF(MsgBytes, Length);

		user->Total.MsgsSent[Index] ++;
		user->Total.BytesForwardedOut[Index] += Length;

		QueueMsg(conn, MsgBytes, Length);

sendEOM:

		free(Save);

		nodeprintf(conn, "\r\r[End of Message #%d from %s%s]\r", msgno, Msg->from, Msg->emailfrom);

		if ((_stricmp(Msg->to, user->Call) == 0) || ((conn->sysop) && (_stricmp(Msg->to, "SYSOP") == 0)))
		{
			if ((Msg->status != 'K') && (Msg->status != 'H') && (Msg->status != 'F') && (Msg->status != 'D'))
			{
				if (Msg->status != 'Y')
				{
					Msg->status = 'Y';
					Msg->datechanged=time(NULL);
					SaveMessageDatabase();
				}
			}
		}
	}
	else
	{
		nodeprintf(conn, "File for Message %d not found\r", msgno);
	}
}
 struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop)
 {
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	do
	{
		m = GetUserMsg(m, Call, sysop);

		if (m == 0)
			return NULL;

		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		m--;

	} while (m> 0);

	return NULL;

}


char * ReadInfoFile(char * File)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	char * MsgBytes;
	struct stat STAT;
	char * ptr1 = 0, * ptr2;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s", BaseDir, File);

	if (stat(MsgFile, &STAT) == -1)
		return NULL;

	FileSize = STAT.st_size;

	hFile = fopen(MsgFile, "rb");

	if (hFile == NULL)
		return NULL;

	MsgBytes=malloc(FileSize+1);

	fread(MsgBytes, 1, FileSize, hFile); 

	fclose(hFile);

	MsgBytes[FileSize] = 0;

	ptr1 = MsgBytes;

	// Replace LF or CRLF with CR

	// First remove cr from crlf

	while(ptr2 = strstr(ptr1, "\r\n"))
	{
		memmove(ptr2, ptr2 + 1, strlen(ptr2));
	}

	// Now replace lf with cr

	ptr1 = MsgBytes;

	while (*ptr1)
	{
		if (*ptr1 == '\n')
			*(ptr1) = '\r';

		ptr1++;
	}

	return MsgBytes;
}

char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *tm;
	static char Date[]="xx-xxx hh:mmZ";

	tm = gmtime(&Datim);
	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%02d-%3s %02d:%02dZ",
					tm->tm_mday, month[tm->tm_mon], tm->tm_hour, tm->tm_min);

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}
	
	return Date;
}

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char * To, char ** ATBBS, char ** BID);


BOOL DoSendCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY
	
	char * From = NULL;
	char * BID = NULL;
	char * ATBBS = NULL;
	char seps[] = " \t\r";
	struct MsgInfo * OldMsg;
	char OldTitle[62];
	char NewTitle[62];
	char To[100]= "";
	int msgno;

	if (Cmd[1] == 0) Cmd[1] ='P'; // Just S means SP

	switch (toupper(Cmd[1]))
	{
	case 'B':

		if (RefuseBulls)
		{
			nodeprintf(conn, "*** Error: This system doesn't allow sending Bulls\r");
			return FALSE;
		}

		if (user->flags & F_NOBULLS)
		{
			nodeprintf(conn, "*** Error: You are not allowed to send Bulls\r");
			return FALSE;
		}
		

	case 'P':
	case 'T':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		strcpy(To, Arg1);

		if (!DecodeSendParams(conn, Context, &From, To, &ATBBS, &BID))
			return FALSE;

		return CreateMessage(conn, From, To, ATBBS, toupper(Cmd[1]), BID, NULL);	

	case 'R':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		if (msgno > 100000)
		{
			BBSputs(conn, "Message Number too high\r");
			return FALSE;
		}

		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		Arg1=&OldMsg->from[0];

		strcpy(To, Arg1);

		if (_stricmp(Arg1, "SMTP:") == 0 || _stricmp(Arg1, "RMS:") == 0 || OldMsg->emailfrom)
		{
			// SMTP message. Need to get the real sender from the message

			sprintf(To, "%s%s", Arg1, OldMsg->emailfrom);
		}

		if (!DecodeSendParams(conn, "", &From, To, &ATBBS, &BID))
			return FALSE;

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 57) OldTitle[57] = 0;

		strcpy(NewTitle, "Re:");
		strcat(NewTitle, OldTitle);

		return CreateMessage(conn, From, To, ATBBS, 'P', BID, NewTitle);	

		return TRUE;

	case 'C':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		if (msgno > 100000)
		{
			BBSputs(conn, "Message Number too high\r");
			return FALSE;
		}

		Arg1 = strtok_s(NULL, seps, &Context);

		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		strcpy(To, Arg1);

		if (!DecodeSendParams(conn, Context, &From, To, &ATBBS, &BID))
			return FALSE;
	
		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 56) OldTitle[56] = 0;

		strcpy(NewTitle, "Fwd:");
		strcat(NewTitle, OldTitle);

		conn->CopyBuffer = ReadMessageFile(msgno);

		return CreateMessage(conn, From, To, ATBBS, 'P', BID, NewTitle);	
	}


	nodeprintf(conn, "*** Error: Invalid Send option %c\r", Cmd[1]);

	return FALSE;
}

char * CheckToAddress(CIRCUIT * conn, char * Addr)
{
	// Check one element of Multiple Address

	if (conn == NULL || !(conn->BBSFlags & BBS))
	{
		// if a normal user, check that TO and/or AT are known and warn if not.

		if (_stricmp(Addr, "SYSOP") == 0)
		{
			return _strdup(Addr);
		}

		if (SendBBStoSYSOPCall)
			if (_stricmp(Addr, BBSName) == 0)
				return _strdup(SYSOPCall);
	

		if (strchr(Addr, '@') == 0)
		{
			// No routing, if not a user and not known to forwarding or WP warn

			struct UserInfo * ToUser = LookupCall(Addr);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->HomeBBS[0])
				{
					char * NewAddr = malloc(250);
					if (conn)
						nodeprintf(conn, "Address %s - @%s added from HomeBBS\r", Addr, ToUser->HomeBBS);
					sprintf(NewAddr, "%s@%s", Addr, ToUser->HomeBBS);
					return NewAddr;
				}
			}
			else
			{
				WPRecP WP = LookupWP(Addr);

				if (WP)
				{
					char * NewAddr = malloc(250);

					if (conn)
						nodeprintf(conn, "Address %s - @%s added from WP\r", Addr, WP->first_homebbs);
					sprintf(NewAddr, "%s@%s", Addr, WP->first_homebbs);
					return NewAddr;
				}
			}
		}
	}

	// Check SMTP and RMS Addresses

	if ((_memicmp(Addr, "rms:", 4) == 0) || (_memicmp(Addr, "rms/", 4) == 0))
	{
		Addr[3] = ':';			// Replace RMS/ with RMS:
			
		if (conn && !FindRMS())
		{
			nodeprintf(conn, "*** Error - Forwarding via RMS is not configured on this BBS\r");
			return FALSE;
		}
	}
	else if ((_memicmp(Addr, "smtp:", 5) == 0) || (_memicmp(Addr, "smtp/", 5) == 0))
	{
		Addr[4] = ':';			// Replace smpt/ with smtp:

		if (ISP_Gateway_Enabled)
		{
			if (conn && (conn->UserPointer->flags & F_EMAIL) == 0)
			{
				nodeprintf(conn, "*** Error - You need to ask the SYSOP to allow you to use Internet Mail\r");
				return FALSE;
			}
		}
		else
		{
			if (conn)
				nodeprintf(conn, "*** Error - Sending mail to smtp addresses is disabled\r");
			return FALSE;
		}
	}

	return _strdup(Addr);
}


char Winlink[] = "WINLINK.ORG";

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char *To, char ** ATBBS, char ** BID)
{
	char * ptr;
	char seps[] = " \t\r";
	WPRecP WP;
	char * ToCopy = _strdup(To);
	int Len;

	conn->ToCount = 0;

	// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY

	// Having trailing ; will mess up parsing multiple addresses, so remove.

	while (To[strlen(To) - 1] == ';')
		To[strlen(To) - 1] = 0;

	if (strchr(Context, ';') || strchr(To, ';'))
	{
		// Multiple Addresses - put address list back together

		char * p;
		
		To[strlen(To)] = ' ';
		Context = To;

		while (p = strchr(Context, ';'))
		{
			// Multiple Addressees

			To = strtok_s(NULL, ";", &Context);
			Len = (int)strlen(To);
			conn->To = realloc(conn->To, (conn->ToCount+1) * sizeof(void *));
			if (conn->To[conn->ToCount] = CheckToAddress(conn, To))
				conn->ToCount++;
		}

		To = strtok_s(NULL, seps, &Context);

		Len = (int)strlen(To);
		conn->To=realloc(conn->To, (conn->ToCount+1) * sizeof(void *));
		if (conn->To[conn->ToCount] = CheckToAddress(conn, To))
			conn->ToCount++;
	}
	else
	{
		// Single Call

		// accept CALL!CALL for source routed message
		
		if (strchr(To, '@') == 0 && strchr(To, '!')) // Bang route without @
		{
			char * bang = strchr(To, '!');
			
			memmove(bang + 1, bang, strlen(bang));	// Move !call down one
			
			*ATBBS = strlop(To, '!');;
		}

		// Accept call@call (without spaces) - but check for smtp addresses

		if (_memicmp(To, "smtp:", 5) != 0 && _memicmp(To, "rms:", 4) != 0  && _memicmp(To, "rms/", 4) != 0)
		{
			ptr = strchr(To, '@');

			if (ptr)
			{
				// If looks like a valid email address, treat as such

				int tolen;
				*ATBBS = strlop(To, '@');

				strlop(To, '-');		// Cant have SSID on BBS Name

				tolen = (int)strlen(To);

				if (tolen > 6 || !CheckifPacket(*ATBBS))
				{
					// Probably Email address. Add smtp: or rms:

					if (FindRMS() || strchr(*ATBBS, '!')) // have RMS or source route
						sprintf(To, "rms:%s", ToCopy);
					else if (ISP_Gateway_Enabled)
						sprintf(To, "smtp:%s", ToCopy);
					else if (isAMPRMsg(ToCopy))
						sprintf(To, "rms:%s", ToCopy);

				}
			}
		}
	}

	free(ToCopy);

	// Look for Optional fields;

	ptr = strtok_s(NULL, seps, &Context);

	while (ptr)
	{
		if (strcmp(ptr, "@") == 0)
		{
			*ATBBS = _strupr(strtok_s(NULL, seps, &Context));
		}
		else if(strcmp(ptr, "<") == 0)
		{			
			*From = strtok_s(NULL, seps, &Context);
		}
		else if (ptr[0] == '$')
			*BID = &ptr[1];
		else
		{
			nodeprintf(conn, "*** Error: Invalid Format\r");
			return FALSE;
		}
		ptr = strtok_s(NULL, seps, &Context);
	}

	// Only allow < from a BBS

	if (*From)
	{
		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "*** < can only be used by a BBS\r");
			return FALSE;
		}
	}

	if (!*From)
		*From = conn->UserPointer->Call;

	if (!(conn->BBSFlags & BBS))
	{
		// if a normal user, check that TO and/or AT are known and warn if not.

		if (_stricmp(To, "SYSOP") == 0)
		{
			conn->LocalMsg = TRUE;
			return TRUE;
		}

		if (!*ATBBS && conn->ToCount == 0)
		{
			// No routing, if not a user and not known to forwarding or WP warn

			struct UserInfo * ToUser = LookupCall(To);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->flags & F_RMSREDIRECT)
				{
					// sent to Winlink
				
					*ATBBS = Winlink;
					nodeprintf(conn, "Redirecting to winlink.org\r", *ATBBS);
				}
				else if (ToUser->HomeBBS[0])
				{
					*ATBBS = ToUser->HomeBBS;
					nodeprintf(conn, "Address @%s added from HomeBBS\r", *ATBBS);
				}
				else
				{
					conn->LocalMsg = TRUE;
				}
			}
			else
			{
				conn->LocalMsg = FALSE;
				WP = LookupWP(To);

				if (WP)
				{
					*ATBBS = WP->first_homebbs;
					nodeprintf(conn, "Address @%s added from WP\r", *ATBBS);
				}
			}
		}
	}
	return TRUE;
}

BOOL CreateMessage(CIRCUIT * conn, char * From, char * ToCall, char * ATBBS, char MsgType, char * BID, char * Title)
{
	struct MsgInfo * Msg, * TestMsg;
	char * via = NULL;
	char * FromHA;

	// Create a temp msg header entry

	if (conn->ToCount)
	{
	}
	else
	{
		if (CheckRejFilters(From, ToCall, ATBBS, BID, MsgType))
		{	
			if ((conn->BBSFlags & BBS))
			{
				nodeprintf(conn, "NO - REJECTED\r");
				if (conn->BBSFlags & OUTWARDCONNECT)
					nodeprintf(conn, "F>\r");				// if Outward connect must be reverse forward
				else
					nodeprintf(conn, ">\r");
			}
			else
				nodeprintf(conn, "*** Error - Message Filters prevent sending this message\r");

			return FALSE;
		}
	}

	Msg = malloc(sizeof (struct MsgInfo));

	if (Msg == 0)
	{
		CriticalErrorHandler("malloc failed for new message header");
		return FALSE;
	}
	
	memset(Msg, 0, sizeof (struct MsgInfo));

	conn->TempMsg = Msg;

	Msg->type = MsgType;
	
	if (conn->UserPointer->flags & F_HOLDMAIL)
		Msg->status = 'H';
	else
		Msg->status = 'N';
	
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (BID)
	{
		BIDRec * TempBID;

		// If P Message, dont immediately reject on a Duplicate BID. Check if we still have the message
		//	If we do, reject  it. If not, accept it again. (do we need some loop protection ???)

		TempBID = LookupBID(BID);
	
		if (TempBID)
		{
			if (MsgType == 'B')
			{
				// Duplicate bid
	
				if ((conn->BBSFlags & BBS))
				{
					nodeprintf(conn, "NO - BID\r");
					if (conn->BBSFlags & OUTWARDCONNECT)
						nodeprintf(conn, "F>\r");				// if Outward connect must be reverse forward
					else
						nodeprintf(conn, ">\r");
				}
				else
					nodeprintf(conn, "*** Error- Duplicate BID\r");

				return FALSE;
			}

			TestMsg = GetMsgFromNumber(TempBID->u.msgno);
 
			// if the same TO we will assume the same message

			if (TestMsg && strcmp(TestMsg->to, ToCall) == 0)
			{
				// We have this message. If we have already forwarded it, we should accept it again

				if ((TestMsg->status == 'N') || (TestMsg->status == 'Y')|| (TestMsg->status == 'H'))
				{
					// Duplicate bid
	
					if ((conn->BBSFlags & BBS))
					{
						nodeprintf(conn, "NO - BID\r");
						if (conn->BBSFlags & OUTWARDCONNECT)
							nodeprintf(conn, "F>\r");				// if Outward connect must be reverse forward
						else
							nodeprintf(conn, ">\r");
					}
					else
						nodeprintf(conn, "*** Error- Duplicate BID\r");

					return FALSE;
				}
			}
		}
		
		if (strlen(BID) > 12) BID[12] = 0;
		strcpy(Msg->bid, BID);

		// Save BID in temp list in case we are offered it again before completion
			
		TempBID = AllocateTempBIDRecord();
		strcpy(TempBID->BID, BID);
		TempBID->u.conn = conn;

	}

	if (conn->ToCount)
	{
	}
	else
	{
		if (_memicmp(ToCall, "rms:", 4) == 0)
		{
			if (!FindRMS())
			{
				nodeprintf(conn, "*** Error - Forwarding via RMS is not configured on this BBS\r");
				return FALSE;
			}

			via=strlop(ToCall, ':');
			_strupr(ToCall);
		}
		else if (_memicmp(ToCall, "rms/", 4) == 0)
		{
			if (!FindRMS())
			{
				nodeprintf(conn, "*** Error - Forwarding via RMS is not configured on this BBS\r");
				return FALSE;
			}

			via=strlop(ToCall, '/');
			_strupr(ToCall);
		}
		else if (_memicmp(ToCall, "smtp:", 5) == 0)
		{
			if (ISP_Gateway_Enabled)
			{
				if ((conn->UserPointer->flags & F_EMAIL) == 0)
				{
					nodeprintf(conn, "*** Error - You need to ask the SYSOP to allow you to use Internet Mail\r");
					return FALSE;
				}
				via=strlop(ToCall, ':');
				ToCall[0] = 0;
			}
			else
			{
				nodeprintf(conn, "*** Error - Sending mail to smtp addresses is disabled\r");
				return FALSE;
			}
		}
		else
		{
			_strupr(ToCall);
			if (ATBBS)
				via=_strupr(ATBBS);
		}

		strlop(ToCall, '-');						// Remove any (illegal) ssid
		if (strlen(ToCall) > 6) ToCall[6] = 0;
	
		strcpy(Msg->to, ToCall);
		
		if (SendBBStoSYSOPCall)
			if (_stricmp(ToCall, BBSName) == 0)
				strcpy(Msg->to, SYSOPCall);

		if (via)
		{
			if (strlen(via) > 40) via[40] = 0;

			strcpy(Msg->via, via);
		}

	}		// End of Multiple Dests

	// Look for HA in From (even if we shouldn't be getting it!)

	FromHA = strlop(From, '@');


	strlop(From, '-');						// Remove any (illegal) ssid
	if (strlen(From) > 6) From[6] = 0;
	strcpy(Msg->from, From);

	if (FromHA)
	{
		if (strlen(FromHA) > 39) FromHA[39] = 0;
		Msg->emailfrom[0] = '@';
		strcpy(&Msg->emailfrom[1], _strupr(FromHA));
	}

	if (Title)					// Only used by SR and SC
	{
		strcpy(Msg->title, Title);
		conn->Flags |= GETTINGMESSAGE;

		// Create initial buffer of 10K. Expand if needed later

		conn->MailBuffer=malloc(10000);
		conn->MailBufferSize=10000;

		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");
		return TRUE;
	}

	if (conn->BBSFlags & FLARQMODE)
		return TRUE;

	if (!(conn->BBSFlags & FBBCompressed))
		conn->Flags |= GETTINGTITLE;

	if (!(conn->BBSFlags & BBS))
		nodeprintf(conn, "Enter Title (only):\r");
	else
		if (!(conn->BBSFlags & FBBForwarding))
			nodeprintf(conn, "OK\r");

	return TRUE;
}

VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int msglen)
{
		
	conn->Flags &= ~GETTINGTITLE;

	if (msglen == 1)
	{
		nodeprintf(conn, "*** Message Cancelled\r");
		SendPrompt(conn, user);
		return;
	}

	if (msglen > 60) msglen = 60;

	Buffer[msglen-1] = 0;

	strcpy(conn->TempMsg->title, Buffer);

	// Create initial buffer of 10K. Expand if needed later

	conn->MailBuffer=malloc(10000);
	conn->MailBufferSize=10000;

	if (conn->MailBuffer == NULL)
	{
		nodeprintf(conn, "Failed to create Message Buffer\r");
		return;
	}

	conn->Flags |= GETTINGMESSAGE;

	if (!conn->BBSFlags & BBS)
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");

}

VOID ProcessMsgLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int msglen)
{
	char * ptr2 = NULL;

	if (((msglen < 3) && (Buffer[0] == 0x1a)) || ((msglen == 4) && (_memicmp(Buffer, "/ex", 3) == 0)))
	{
		int Index = 0;
			
		if (conn->TempMsg->type == 'P')
			Index = PMSG;
		else if (conn->TempMsg->type == 'B')
			Index = BMSG;
		else if (conn->TempMsg->type == 'T')
			Index = TMSG;
		
		conn->Flags &= ~GETTINGMESSAGE;

		user->Total.MsgsReceived[Index]++;
		user->Total.BytesForwardedIn[Index] += conn->TempMsg->length;

		if (conn->ToCount)
		{
			// Multiple recipients

			struct MsgInfo * Msg = conn->TempMsg;
			int i;
			struct MsgInfo * SaveMsg = Msg;
			char * SaveBody = conn->MailBuffer;
			int SaveMsgLen = Msg->length; 
			BOOL SentToRMS = FALSE;
			int ToLen = 0;
			char * ToString = zalloc(conn->ToCount * 100);

			// If no BID provided, allocate one
			
			if (Msg->bid[0] == 0)
				sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg + 1, BBSName);

			for (i = 0; i < conn->ToCount; i++)
			{
				char * Addr = conn->To[i];
				char * Via;

				if (_memicmp (Addr, "SMTP:", 5) == 0)
				{
					// For Email

					conn->TempMsg = Msg = malloc(sizeof(struct MsgInfo));
					memcpy(Msg, SaveMsg, sizeof(struct MsgInfo));
	
					conn->MailBuffer = malloc(SaveMsgLen + 10);
					memcpy(conn->MailBuffer, SaveBody, SaveMsgLen);

					Msg->to[0] = 0;
					strcpy(Msg->via, &Addr[5]);

					CreateMessageFromBuffer(conn);
					continue;
				}

				if (_memicmp (Addr, "RMS:", 4) == 0)
				{
					// Add to B2 Message for RMS

					Addr+=4;
					
					Via = strlop(Addr, '@');
				
					if (Via && _stricmp(Via, "winlink.org") == 0)
					{
						if (CheckifLocalRMSUser(Addr))
						{
							// Local RMS - Leave Here
	
							Via = 0;							// Drop Through
							goto PktMsg;
						}
						else
						{
							ToLen = sprintf(ToString, "%sTo: %s\r\n", ToString, Addr);
							continue;
						}
					}

					ToLen = sprintf(ToString, "%sTo: %s@%s\r\n", ToString, Addr, Via);
					continue;
				}

				_strupr(Addr);
				
				Via = strlop(Addr, '@');

				if (Via && _stricmp(Via, "winlink.org") == 0)
				{
					if (CheckifLocalRMSUser(Addr))
					{
						// Local RMS - Leave Here

						Via = 0;							// Drop Through
					}
					else
					{
						ToLen = sprintf(ToString, "%sTo: %s\r\n", ToString, Addr);

						// Add to B2 Message for RMS

						continue;
					}
				}

			PktMsg:		
				
				conn->LocalMsg = FALSE;

				// Normal BBS Message

				if (_stricmp(Addr, "SYSOP") == 0)
					conn->LocalMsg = TRUE;
				else
				{
					struct UserInfo * ToUser = LookupCall(Addr);

					if (ToUser)
						conn->LocalMsg = TRUE;
				}

				conn->TempMsg = Msg = malloc(sizeof(struct MsgInfo));
				memcpy(Msg, SaveMsg, sizeof(struct MsgInfo));
	
				conn->MailBuffer = malloc(SaveMsgLen + 10);
				memcpy(conn->MailBuffer, SaveBody, SaveMsgLen);

				strcpy(Msg->to, Addr);

				if (Via)
				{
					Msg->bid[0] = 0;					// if we are forwarding it, we must change BID to be safe
					strcpy(Msg->via, Via);
				}

				CreateMessageFromBuffer(conn);
			}

			if (ToLen)
			{
				char * B2Hddr = zalloc(ToLen + 1000);
				int B2HddrLen;
				char DateString[80];
				struct tm * tm;
				time_t Date = time(NULL);
				char Type[16] = "Private";
					
				// Get Type
	
				if (conn->TempMsg->type == 'B')
					strcpy(Type, "Bulletin");
				else if (conn->TempMsg->type == 'T')
					strcpy(Type, "Traffic");

				tm = gmtime(&Date);	
	
				sprintf(DateString, "%04d/%02d/%02d %02d:%02d",
					tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

				conn->TempMsg = Msg = malloc(sizeof(struct MsgInfo));
				memcpy(Msg, SaveMsg, sizeof(struct MsgInfo));
	
				conn->MailBuffer = malloc(SaveMsgLen + 1000 + ToLen);

				Msg->B2Flags = B2Msg;

				B2HddrLen = sprintf(B2Hddr,
					"MID: %s\r\nDate: %s\r\nType: %s\r\nFrom: %s\r\n%sSubject: %s\r\nMbo: %s\r\nBody: %d\r\n\r\n",
					SaveMsg->bid, DateString, Type,
					SaveMsg->from, ToString, SaveMsg->title, BBSName, SaveMsgLen);

				memcpy(conn->MailBuffer, B2Hddr, B2HddrLen);
				memcpy(&conn->MailBuffer[B2HddrLen], SaveBody, SaveMsgLen);

				Msg->length += B2HddrLen;

				strcpy(Msg->to, "RMS");

				CreateMessageFromBuffer(conn);

				free(B2Hddr);
			}

			free(SaveMsg);
			free(SaveBody);
			conn->MailBuffer = NULL;
			conn->MailBufferSize=0;

			if (!(conn->BBSFlags & BBS))
				SendPrompt(conn, conn->UserPointer);
			else
				if (!(conn->BBSFlags & FBBForwarding))
				{
					if (conn->BBSFlags & OUTWARDCONNECT)
						BBSputs(conn, "F>\r");				// if Outward connect must be reverse forward
					else
						BBSputs(conn, ">\r");
				}

			/*
			// From a client - Create one copy with all RMS recipients, and another for each packet recipient	

			// Merge all RMS To: lines 

			ToLen = 0;
			ToString[0] = 0;

			for (i = 0; i < Recipients; i++)
			{
				if (LocalMsg[i])
					continue;						// For a local RMS user
				
				if (_stricmp(Via[i], "WINLINK.ORG") == 0 || _memicmp (&HddrTo[i][4], "SMTP:", 5) == 0 ||
					_stricmp(RecpTo[i], "RMS") == 0)
				{
					ToLen += strlen(HddrTo[i]);
					strcat(ToString, HddrTo[i]);
				}
			}

			if (ToLen)
			{
				conn->TempMsg = Msg = malloc(sizeof(struct MsgInfo));
				memcpy(Msg, SaveMsg, sizeof(struct MsgInfo));
	
				conn->MailBuffer = malloc(SaveMsgLen + 1000);
				memcpy(conn->MailBuffer, SaveBody, SaveMsgLen);


				memmove(&conn->MailBuffer[B2To + ToLen], &conn->MailBuffer[B2To], count);
				memcpy(&conn->MailBuffer[B2To], ToString, ToLen); 

				conn->TempMsg->length += ToLen;

				strcpy(Msg->to, "RMS");
				strcpy(Msg->via, "winlink.org");

				// Must Change the BID

				Msg->bid[0] = 0;

				CreateMessageFromBuffer(conn);
			}

			}

			free(ToString);

			for (i = 0; i < Recipients; i++)
			{
				// Only Process Non - RMS Dests or local RMS Users

				if (LocalMsg[i] == 0)
					if (_stricmp (Via[i], "WINLINK.ORG") == 0 ||
						_memicmp (&HddrTo[i][4], "SMTP:", 5) == 0 ||
						_stricmp(RecpTo[i], "RMS") == 0)		
					continue;

				conn->TempMsg = Msg = malloc(sizeof(struct MsgInfo));
				memcpy(Msg, SaveMsg, sizeof(struct MsgInfo));
	
				conn->MailBuffer = malloc(SaveMsgLen + 1000);
				memcpy(conn->MailBuffer, SaveBody, SaveMsgLen);

				// Add our To: 

				ToLen = strlen(HddrTo[i]);

				if (_memicmp(HddrTo[i], "CC", 2) == 0)	// Replace CC: with TO:
					memcpy(HddrTo[i], "To", 2);

				memmove(&conn->MailBuffer[B2To + ToLen], &conn->MailBuffer[B2To], count);
				memcpy(&conn->MailBuffer[B2To], HddrTo[i], ToLen); 

				conn->TempMsg->length += ToLen;

				strcpy(Msg->to, RecpTo[i]);
				strcpy(Msg->via, Via[i]);
				
				Msg->bid[0] = 0;

				CreateMessageFromBuffer(conn);
			}
			}	// End not from RMS

			free(SaveMsg);
			free(SaveBody);
			conn->MailBuffer = NULL;
			conn->MailBufferSize=0;

			SetupNextFBBMessage(conn);
			return;
	
			} My__except_Routine("Process Multiple Destinations");

			BBSputs(conn, "*** Program Error Processing Multiple Destinations\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
*/

			conn->ToCount = 0;

			return;
		}


		CreateMessageFromBuffer(conn);
		return;

	}

	Buffer[msglen++] = 0x0a;

	if ((conn->TempMsg->length + msglen) > conn->MailBufferSize)
	{
		conn->MailBufferSize += 10000;
		conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
		if (conn->MailBuffer == NULL)
		{
			nodeprintf(conn, "Failed to extend Message Buffer\r");

			conn->Flags &= ~GETTINGMESSAGE;
			return;
		}
	}

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, msglen);

	conn->TempMsg->length += msglen;
}

VOID CreateMessageFromBuffer(CIRCUIT * conn)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;
	char * ptr1, * ptr2 = NULL;
	char * ptr3, * ptr4;
	int FWDCount = 0;
	char OldMess[] = "\r\n\r\nOriginal Message:\r\n\r\n";
	time_t Age;
	int OurCount;
	char * HoldReason = "User has Hold Messages flag set";
	struct UserInfo * user;


#ifndef LINBPQ
	struct _EXCEPTION_POINTERS exinfo;
#endif

	// If doing SC, Append Old Message

	if (conn->CopyBuffer)
	{
		if ((conn->TempMsg->length + (int) strlen(conn->CopyBuffer) + 80 )> conn->MailBufferSize)
		{
			conn->MailBufferSize += (int)strlen(conn->CopyBuffer) + 80;
			conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
			if (conn->MailBuffer == NULL)
			{
				nodeprintf(conn, "Failed to extend Message Buffer\r");

				conn->Flags &= ~GETTINGMESSAGE;
				return;
			}
		}

		memcpy(&conn->MailBuffer[conn->TempMsg->length], OldMess, strlen(OldMess));

		conn->TempMsg->length += (int)strlen(OldMess);

		memcpy(&conn->MailBuffer[conn->TempMsg->length], conn->CopyBuffer, strlen(conn->CopyBuffer));

		conn->TempMsg->length += (int)strlen(conn->CopyBuffer);

		free(conn->CopyBuffer);
		conn->CopyBuffer = NULL;
	}
	
	// Allocate a message Record slot
	
	Msg = AllocateMsgRecord();
	memcpy(Msg, conn->TempMsg, sizeof(struct MsgInfo));

	free(conn->TempMsg);

	// Set number here so they remain in sequence
		
	GetSemaphore(&MsgNoSemaphore, 0);
	Msg->number = ++LatestMsg;
	FreeSemaphore(&MsgNoSemaphore);
	MsgnotoMsg[Msg->number] = Msg;

	if (Msg->status == 0)
		Msg->status = 'N';

	// Create BID if non supplied

	if (Msg->bid[0] == 0)
		sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	// if message body had R: lines, get date created from last (not very accurate, but best we can do)

	// Also check if we have had message before to detect loops

	ptr1 = conn->MailBuffer;
	OurCount = 0;

	// If it is a B2 Message, Must Skip B2 Header

	if (Msg->B2Flags & B2Msg)
	{
		ptr1 = strstr(ptr1, "\r\n\r\n");
		if (ptr1)
			ptr1 += 4;
		else
			ptr1 = conn->MailBuffer;
	}

nextline:

	if (memcmp(ptr1, "R:", 2) == 0)
	{
		// Is if ours?

		// BPQ RLINE Format R:090920/1041Z 6542@N4JOA.#WPBFL.FL.USA.NOAM BPQ1.0.2

		ptr3 = strchr(ptr1, '@');
		ptr4 = strchr(ptr1, '.');

		if (ptr3 && ptr4 && (ptr4 > ptr3))
		{
			if (memcmp(ptr3+1, BBSName, ptr4-ptr3-1) == 0)
				OurCount++;
		}

		GetWPBBSInfo(ptr1);		// Create WP /I record from R: Line

		// see if another

		ptr2 = ptr1;			// save
		ptr1 = strchr(ptr1, '\r');
		if (ptr1 == 0)
		{
			Debugprintf("Corrupt Message %s from %s - truncated within R: line", Msg->bid, Msg->from);
			return;
		}
		ptr1++;
		if (*ptr1 == '\n') ptr1++;

		goto nextline;
	}

	// ptr2 points to last R: line (if any)

	if (ptr2)
	{
		struct tm rtime;
		time_t result;

		memset(&rtime, 0, sizeof(struct tm));

		if (ptr2[10] == '/')
		{
			// Dodgy 4 char year

			sscanf(&ptr2[2], "%04d%02d%02d/%02d%02d",
				&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);
			rtime.tm_year -= 1900;
			rtime.tm_mon--;
		}
		else if (ptr2[8] == '/')
		{
			sscanf(&ptr2[2], "%02d%02d%02d/%02d%02d",
				&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);

			if (rtime.tm_year < 90)
				rtime.tm_year += 100;		// Range 1990-2089
			rtime.tm_mon--;
		}

		// Otherwise leave date as zero, which should be rejected

		//			result = _mkgmtime(&rtime);

		if ((result = mktime(&rtime)) != (time_t)-1 )
		{
			result -= (time_t)_MYTIMEZONE;

			Msg->datecreated =  result;	
			Age = (time(NULL) - result)/86400;

			if ( Age < -7)
			{
				Msg->status = 'H';
				HoldReason = "Suspect Date Sent";
			}
			else if (Age > BidLifetime || Age > MaxAge)
			{
				Msg->status = 'H';
				HoldReason = "Message too old";

			}
			else
				GetWPInfoFromRLine(Msg->from, ptr2, result);
		}
		else
		{
			// Can't decode R: Datestamp

			Msg->status = 'H';
			HoldReason = "Corrupt R: Line - can't determine age";
		}

		if (OurCount > 1)
		{
			// Message is looping 

			Msg->status = 'H';
			HoldReason = "Message may be looping";

		}
	}

	if (strcmp(Msg->to, "WP") == 0)
	{
		// If Reject WP Bulls is set, Kill message here.
		// It should only get here if B2 - otherwise it should be
		// rejected earlier

		if (Msg->type == 'B' && FilterWPBulls)
			Msg->status = 'K';

	}

	conn->MailBuffer[Msg->length] = 0;

	if (CheckBadWords(Msg->title) || CheckBadWords(conn->MailBuffer))
	{
		Msg->status = 'H';
		HoldReason = "Bad word in title or body";
	}

	if (CheckHoldFilters(Msg->from, Msg->to, Msg->via, Msg->bid))
	{
		Msg->status = 'H';
		HoldReason = "Matched Hold Filters";
	}

	if (CheckValidCall(Msg->from) == 0)
	{
		Msg->status = 'H';
		HoldReason = "Probable Invalid From Call";
	}

	// Process any WP Messages
	
	if (strcmp(Msg->to, "WP") == 0)
	{	
		if (Msg->status == 'N')
		{
			ProcessWPMsg(conn->MailBuffer, Msg->length, ptr2);

			if (Msg->type == 'P')			// Kill any processed private WP messages.
			{
				char VIA[80];

				strcpy(VIA, Msg->via);
				strlop(VIA, '.');

				if (strcmp(VIA, BBSName) == 0)
					Msg->status = 'K';
			}
		}
	}
	
	CreateMessageFile(conn, Msg);

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	if (Msg->length > MaxTXSize)
	{
		Msg->status = 'H';
		HoldReason = "Message too long";

		if (!(conn->BBSFlags & BBS))
			nodeprintf(conn, "*** Warning Message length exceeds sysop-defined maximum of %d - Message will be held\r", MaxTXSize);
	}

	// Check for message to internal server

	if (Msg->via[0] == 0 
		|| _stricmp(Msg->via, BBSName) == 0		// our BBS a
		|| _stricmp(Msg->via, AMPRDomain) == 0)	// our AMPR Address
	{
		if (CheckforMessagetoServer(Msg))
		{
			// Flag as killed and send prompt

			FlagAsKilled(Msg, TRUE);

			if (!(conn->BBSFlags & BBS))
			{
				nodeprintf(conn, "Message %d to Server Processed and Killed.\r", Msg->number);
				SendPrompt(conn, conn->UserPointer);
			}
			return;				// no need to process further
		}
	}

	if (Msg->to[0])
		FWDCount = MatchMessagetoBBSList(Msg, conn);
	else
	{
		// If addressed @winlink.org, and to a local user, Keep here.

		char * Call;
		char * AT;

		// smtp or rms - don't warn no route

		FWDCount = 1;

		Call = _strupr(_strdup(Msg->via));
		AT = strlop(Call, '@');

		if (AT && _stricmp(AT, "WINLINK.ORG") == 0)
		{
			struct UserInfo * user = LookupCall(Call);

			if (user)
			{
				if (user->flags & F_POLLRMS)
				{
					Logprintf(LOG_BBS, conn, '?', "SMTP Message @ winlink.org, but local RMS user - leave here");
					strcpy(Msg->to, Call);
					strcpy(Msg->via, AT);
					if (user->flags & F_BBS)	// User is a BBS, so set FWD bit so he can get it
						set_fwd_bit(Msg->fbbs, user->BBSNumber);

				}
			}
		}
		free(Call);
	}

	// Warn SYSOP if P or T forwarded in, and has nowhere to go

	if ((conn->BBSFlags & BBS) && Msg->type != 'B' && FWDCount == 0 && WarnNoRoute &&
		strcmp(Msg->to, "SYSOP") && strcmp(Msg->to, "WP"))
	{
		if (Msg->via[0])
		{	
			if (_stricmp(Msg->via, BBSName))		// Not for our BBS a
				if (_stricmp(Msg->via, AMPRDomain))	// Not for our AMPR Address
					SendWarningToSYSOP(Msg);
		}
		else
		{
			// No via - is it for a local user?

			if (LookupCall(Msg->to) == 0)
				SendWarningToSYSOP(Msg);
		}
	}

	if ((conn->BBSFlags & SYNCMODE) == 0)
	{
		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "Message: %d Bid:  %s Size: %d\r", Msg->number, Msg->bid, Msg->length);

			if (Msg->via[0])
			{
				if (_stricmp(Msg->via, BBSName))		// Not for our BBS a
					if (_stricmp(Msg->via, AMPRDomain))	// Not for our AMPR Address

						if (FWDCount ==  0 &&  Msg->to[0] != 0)		// unless smtp msg
							nodeprintf(conn, "@BBS specified, but no forwarding info is available - msg may not be delivered\r");
			}
			else
			{
				if (FWDCount ==  0 && conn->LocalMsg == 0 && Msg->type != 'B')
					// Not Local and no forward route
					nodeprintf(conn, "Message is not for a local user, and no forwarding info is available - msg may not be delivered\r");
			}
			if (conn->ToCount == 0)
				SendPrompt(conn, conn->UserPointer);
		}
		else
		{
			if (!(conn->BBSFlags & FBBForwarding))
			{
				if (conn->ToCount == 0)
					if (conn->BBSFlags & OUTWARDCONNECT)
						nodeprintf(conn, "F>\r");				// if Outward connect must be reverse forward
					else
						nodeprintf(conn, ">\r");
			}					
		}
	}

	if(Msg->to[0] == 0)
		SMTPMsgCreated=TRUE;

	if (Msg->status != 'H' && Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		Msg->status = '$';				// Has forwarding

	if (Msg->status == 'H')
	{
		int Length=0;
		char * MailBuffer = malloc(100);
		char Title[100];

		Length += sprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
		sprintf(Title, "Message %d Held - %s", Msg->number, HoldReason);
		SendMessageToSYSOP(Title, MailBuffer, Length);
	}

	BuildNNTPList(Msg);				// Build NNTP Groups list

	SaveMessageDatabase();
	SaveBIDDatabase();

	// If Event Notifications enabled report a new message event

	if (reportNewMesageEvents)
	{
		char msg[200];

		//12345 B 2053 TEST@ALL F6FBB 920325 This is the subject

		struct tm *tm = gmtime((time_t *)&Msg->datecreated);	

		sprintf_s(msg, sizeof(msg),"%-6d %c %6d %-13s %-6s %02d%02d%02d %s\r",
			Msg->number, Msg->type, Msg->length, Msg->to,
			Msg->from, tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, Msg->title);

#ifdef WIN32
		if (pRunEventProgram)
			pRunEventProgram("MailNewMsg.exe", msg);
#else
		{
			char prog[256];
			sprintf(prog, "%s/%s", BPQDirectory, "MailNewMsg");
			RunEventProgram(prog, msg);
		}
#endif
	}


	if (EnableUI)
#ifdef LINBPQ
		SendMsgUI(Msg);	
#else
		__try
	{
		SendMsgUI(Msg);
	}
	My__except_Routine("SendMsgUI");
#endif

	user = LookupCall(Msg->to);

	if (user && (user->flags & F_APRSMFOR))
	{
		char APRS[128];
		char Call[16];
		int SSID = user->flags >> 28;

		if (SSID)
			sprintf(Call, "%s-%d", Msg->to, SSID);
		else
			strcpy(Call, Msg->to);

		sprintf(APRS, "New BBS Message %s From %s", Msg->title, Msg->from);
		APISendAPRSMessage(APRS, Call);
	}

	return;
}

VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg)
{
	char MsgFile[MAX_PATH];
	FILE * hFile;
	size_t WriteLen=0;
	char Mess[255];
	int len;
	BOOL AutoImport = FALSE;

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	//	If title is "Batched messages for AutoImport from BBS xxxxxx and first line is S? and it is
	//  for this BBS, Import file and set message as Killed. May need to strip B2 Header and R: lines

	if (strcmp(Msg->to, BBSName) == 0 && strstr(Msg->title, "Batched messages for AutoImport from BBS "))
	{
		UCHAR * ptr = conn->MailBuffer;

		// If it is a B2 Message, Must Skip B2 Header

		if (Msg->B2Flags & B2Msg)
		{
			ptr = strstr(ptr, "\r\n\r\n");
			if (ptr)
				ptr += 4;
			else
				ptr = conn->MailBuffer;
		}

		if (memcmp(ptr, "R:", 2) == 0)
		{
			ptr = strstr(ptr, "\r\n\r\n");		//And remove R: Lines
			if (ptr)
				ptr += 4;
		}

		if (*(ptr) == 'S' && ptr[2] == ' ')
		{
			int HeaderLen = (int)(ptr - conn->MailBuffer);
			Msg->length -= HeaderLen;
			memmove(conn->MailBuffer, ptr, Msg->length);
			Msg->status = 'K';
			AutoImport = TRUE;
		}
	}

	hFile = fopen(MsgFile, "wb");

	if (hFile)
	{
		WriteLen = fwrite(conn->MailBuffer, 1, Msg->length, hFile);
		fclose(hFile);
	}

	if (AutoImport)
		ImportMessages(NULL, MsgFile, TRUE);

	free(conn->MailBuffer);
	conn->MailBufferSize=0;
	conn->MailBuffer=0;

	if (WriteLen != Msg->length)
	{
		len = sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
		QueueMsg(conn, Mess, len);
		Debugprintf(Mess);
	}
	return;
}




VOID SendUnbuffered(int stream, char * msg, int len)
{
#ifndef LINBPQ
	if (stream < 0)
		WritetoConsoleWindow(stream, msg, len);
	else
#endif
		SendMsg(stream, msg, len);
}

BOOL FindMessagestoForwardLoop(CIRCUIT * conn, char Type, int MaxLen);

BOOL FindMessagestoForward (CIRCUIT * conn)
{
	struct UserInfo * user = conn->UserPointer;

#ifndef LINBPQ

	struct _EXCEPTION_POINTERS exinfo;

	__try {
#endif

	// This is a hack so Winpack or WLE users can use forwarding
	// protocols to get their messages without being defined as a BBS

	// !!IMPORTANT Getting this wrong can see message repeatedly proposed !!
	// !! Anything sent using this must be killed if sent or rejected.

	// I'm not sure about this. I think I only need the PacLinkCalls
	// if from RMS Express, and it always sends an FW; line
	// Ah, no. What about WinPack ??
	// If from RMS Express must have Temp_B2 or BBS set or SID will
	// be rejected. So maybe just Temp_B2 && BBS = 0??
	// No, someone may have Temp_B2 set and not be using WLE ?? So what ??

	if ((user->flags & F_Temp_B2_BBS) && ((user->flags & F_BBS) == 0) || conn->RMSExpress || conn->PAT)
	{
		if (conn->PacLinkCalls == NULL)
		{
			// create a list with just the user call

			char * ptr1;

			conn->PacLinkCalls = zalloc(30);

			ptr1 = (char *)conn->PacLinkCalls;
			ptr1 += 16;		// Must be room for a null pointer on end (64 bit bug)
			strcpy(ptr1, user->Call);

			conn->PacLinkCalls[0] = ptr1;
		}
	}

	if (conn->SendT && FindMessagestoForwardLoop(conn, 'T', conn->MaxTLen))
	{
		conn->LastForwardType = 'T';
		return TRUE;
	}

	if (conn->LastForwardType == 'T')
		conn->NextMessagetoForward = FirstMessageIndextoForward;

	if (conn->SendP && FindMessagestoForwardLoop(conn, 'P', conn->MaxPLen))
	{
		conn->LastForwardType = 'P';
		return TRUE;
	}

	if (conn->LastForwardType == 'P')
		conn->NextMessagetoForward = FirstMessageIndextoForward;

	if (conn->SendB && FindMessagestoForwardLoop(conn, 'B', conn->MaxBLen))
	{
		conn->LastForwardType = 'B';
		return TRUE;
	}

	conn->LastForwardType = 0;
	return FALSE;
#ifndef LINBPQ
	} My__except_Routine("FindMessagestoForward");
#endif
	return FALSE;

}


BOOL FindMessagestoForwardLoop(CIRCUIT * conn, char Type, int MaxLen)
{
	// See if any messages are queued for this BBS

	int m;
	struct MsgInfo * Msg;
	struct UserInfo * user = conn->UserPointer;
	struct FBBHeaderLine * FBBHeader;
	BOOL Found = FALSE;
	char RLine[100];
	int TotalSize = 0;
	time_t NOW = time(NULL);

//	Debugprintf("FMTF entered Call %s Type %c Maxlen %d NextMsg = %d BBSNo = %d",
//		conn->Callsign, Type, MaxLen, conn->NextMessagetoForward, user->BBSNumber);

	if (conn->PacLinkCalls || (conn->UserPointer->flags & F_NTSMPS))	// Looking for all messages, so reset 
		conn->NextMessagetoForward = 1;

	conn->FBBIndex = 0;

	for (m = conn->NextMessagetoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		//	If an NTS MPS, see if anything matches

		if (Type == 'T' && (conn->UserPointer->flags & F_NTSMPS))
		{
			struct BBSForwardingInfo * ForwardingInfo = conn->UserPointer->ForwardingInfo;
			int depth;
				
			if (Msg->type == 'T' && Msg->status == 'N' && Msg->length <= MaxLen && ForwardingInfo)
			{
				depth = CheckBBSToForNTS(Msg, ForwardingInfo);

				if (depth > -1 && Msg->Locked == 0)
					goto Forwardit;
						
				depth = CheckBBSAtList(Msg, ForwardingInfo, Msg->via);

				if (depth && Msg->Locked == 0)
					goto Forwardit;

				depth = CheckBBSATListWildCarded(Msg, ForwardingInfo, Msg->via);

				if (depth > -1 && Msg->Locked == 0)
					goto Forwardit;
			}
		}

		// If forwarding to Paclink or RMS Express, look for any message matching the
		// requested call list with status 'N' (maybe should also be 'P' ??)

		if (conn->PacLinkCalls)
		{
			int index = 1;

			char * Call = conn->PacLinkCalls[0];

			while (Call)
			{
				if (Msg->type == Type && Msg->status == 'N')
				{
//				Debugprintf("Comparing RMS Call %s %s", Msg->to, Call);
				if (_stricmp(Msg->to, Call) == 0)
					if (Msg->status == 'N' && Msg->type == Type && Msg->length <= MaxLen) 
						goto Forwardit;
					else
						Debugprintf("Call Match but Wrong Type/Len %c %d", Msg->type, Msg->length);
				}
				Call = conn->PacLinkCalls[index++];
			}
//			continue;
		}

		if (Msg->type == Type && Msg->length <= MaxLen && (Msg->status != 'H')
			&& (Msg->status != 'D') && Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
		{
			// Message to be sent - do a consistancy check (State, etc)

		Forwardit:

			if (Msg->Defered > 0)			// = response received
			{
				Msg->Defered--;
				Debugprintf("Message %d deferred", Msg->number);
				continue;
			}

			if ((Msg->from[0] == 0) || (Msg->to[0] == 0))
			{
				int Length=0;
				char * MailBuffer = malloc(100);
				char Title[100];

				Length += sprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
				sprintf(Title, "Message %d Held - %s", Msg->number, "Missing From: or To: field");
				SendMessageToSYSOP(Title, MailBuffer, Length);
			
				Msg->status = 'H';
				continue;
			}

			conn->NextMessagetoForward = m + 1;			// So we don't offer again if defered

			Msg->Locked = 1;				// So other MPS can't pick it up

			// if FBB forwarding add to list, eise save pointer

			if (conn->BBSFlags & FBBForwarding)
			{
				struct tm *tm;
				time_t temp;

				FBBHeader = &conn->FBBHeaders[conn->FBBIndex++];

				FBBHeader->FwdMsg = Msg;
				FBBHeader->MsgType = Msg->type;
				FBBHeader->Size = Msg->length;
				TotalSize += Msg->length;
				strcpy(FBBHeader->From, Msg->from);
				strcpy(FBBHeader->To, Msg->to);
				strcpy(FBBHeader->ATBBS, Msg->via);
				strcpy(FBBHeader->BID, Msg->bid);

				// Set up R:Line, so se can add its length to the sise

				memcpy(&temp, &Msg->datereceived, sizeof(time_t));
				tm = gmtime(&temp);	

				FBBHeader->Size += sprintf_s(RLine, sizeof(RLine),"R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
					tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
					Msg->number, BBSName, HRoute, RlineVer);

				// If using B2 forwarding we need the message size and Compressed size for FC proposal

				if (conn->BBSFlags & FBBB2Mode) 
				{
					if (CreateB2Message(conn, FBBHeader, RLine) == FALSE)
					{
						char * MailBuffer = malloc(100);
						char Title[100];
						int Length;
						
						// Corrupt B2 Message
						
						Debugprintf("Corrupt B2 Message found - Message %d will be held", Msg->number);
						Msg->status = 'H';
						SaveMessageDatabase();

						conn->FBBIndex--;
						TotalSize -= Msg->length;
						memset(&conn->FBBHeaders[conn->FBBIndex], 0, sizeof(struct FBBHeaderLine));

						Length = sprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
						sprintf(Title, "Message %d Held - %s", Msg->number, "Corrupt B2 Message");
						SendMessageToSYSOP(Title, MailBuffer, Length);
			
						continue;
					}
				}

				if (conn->FBBIndex == 5  || TotalSize > user->ForwardingInfo->MaxFBBBlockSize)
					return TRUE;							// Got max number or too big

				Found = TRUE;								// Remember we have some
			}
			else
			{
				conn->FwdMsg = Msg;
				return TRUE;
			}
		}
	}

	return Found;
}

BOOL SeeifMessagestoForward (int BBSNumber, CIRCUIT * conn)
{
	// See if any messages are queued for this BBS

	// if Conn is not NULL, also check Msg Type

	int m;
	struct MsgInfo * Msg;

	for (m = FirstMessageIndextoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status != 'H') && (Msg->status != 'D') && Msg->type && check_fwd_bit(Msg->fbbs, BBSNumber))
		{
			if (conn)
			{
				char Type = Msg->type;

				if ((conn->SendB && Type == 'B') || (conn->SendP && Type == 'P') || (conn->SendT && Type == 'T'))
				{
//					Debugprintf("SeeifMessagestoForward BBSNo %d Msg %d", BBSNumber, Msg->number);
					return TRUE;
				}
			}
			else
			{
//				Debugprintf("SeeifMessagestoForward BBSNo %d Msg %d", BBSNumber, Msg->number);	
				return TRUE;
			}
		}
	}

	return FALSE;
}

int CountMessagestoForward (struct UserInfo * user)
{
	// See if any messages are queued for this BBS

	int m, n=0;
	struct MsgInfo * Msg;
	int BBSNumber = user->BBSNumber;
	int FirstMessage = FirstMessageIndextoForward;

	if ((user->flags & F_NTSMPS))
		FirstMessage = 1;

	for (m = FirstMessage; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status != 'H') && (Msg->status != 'D') && Msg->type && check_fwd_bit(Msg->fbbs, BBSNumber))
		{
			n++;
			continue;			// So we dont count twice in Flag set and NTS MPS
		}

		// if an NTS MPS, also check for any matches

		if (Msg->type == 'T' && (user->flags & F_NTSMPS))
		{
			struct BBSForwardingInfo * ForwardingInfo = user->ForwardingInfo;
			int depth;
				
			if (Msg->status == 'N' && ForwardingInfo)
			{
				depth = CheckBBSToForNTS(Msg, ForwardingInfo);

				if (depth > -1 && Msg->Locked == 0)
				{
					n++;
					continue;
				}						
				depth = CheckBBSAtList(Msg, ForwardingInfo, Msg->via);

				if (depth && Msg->Locked == 0)
				{
					n++;
					continue;
				}						

				depth = CheckBBSATListWildCarded(Msg, ForwardingInfo, Msg->via);

				if (depth > -1 && Msg->Locked == 0)
				{
					n++;
					continue;
				}
			}
		}
	}

	return n;
}

int ListMessagestoForward(CIRCUIT * conn, struct UserInfo * user)
{
	// See if any messages are queued for this BBS

	int m, n=0;
	struct MsgInfo * Msg;
	int BBSNumber = user->BBSNumber;
	int FirstMessage = FirstMessageIndextoForward;

	if ((user->flags & F_NTSMPS))
		FirstMessage = 1;

	for (m = FirstMessage; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status != 'H') && (Msg->status != 'D') && Msg->type && check_fwd_bit(Msg->fbbs, BBSNumber))
		{
			nodeprintf(conn, "%d %s\r", Msg->number, Msg->title);
			continue;			// So we dont count twice in Flag set and NTS MPS
		}

		// if an NTS MPS, also check for any matches

		if (Msg->type == 'T' && (user->flags & F_NTSMPS))
		{
			struct BBSForwardingInfo * ForwardingInfo = user->ForwardingInfo;
			int depth;
				
			if (Msg->status == 'N' && ForwardingInfo)
			{
				depth = CheckBBSToForNTS(Msg, ForwardingInfo);

				if (depth > -1 && Msg->Locked == 0)
				{
					nodeprintf(conn, "%d %s\r", Msg->number, Msg->title);
					continue;
				}						
				depth = CheckBBSAtList(Msg, ForwardingInfo, Msg->via);

				if (depth && Msg->Locked == 0)
				{
					nodeprintf(conn, "%d %s\r", Msg->number, Msg->title);
					continue;
				}						

				depth = CheckBBSATListWildCarded(Msg, ForwardingInfo, Msg->via);

				if (depth > -1 && Msg->Locked == 0)
				{
					nodeprintf(conn, "%d %s\r", Msg->number, Msg->title);
					continue;
				}
			}
		}
	}

	return n;
}

VOID SendWarningToSYSOP(struct MsgInfo * Msg)
{
	int Length=0;
	char * MailBuffer = malloc(100);
	char Title[100];

	Length += sprintf(MailBuffer, "Warning - Message %d has nowhere to go", Msg->number);
	sprintf(Title, "Warning - Message %d has nowhere to go", Msg->number);
	SendMessageToSYSOP(Title, MailBuffer, Length);
}



VOID SendMessageToSYSOP(char * Title, char * MailBuffer, int Length)
{
	struct MsgInfo * Msg = AllocateMsgRecord();
	BIDRec * BIDRec;

	char MsgFile[MAX_PATH];
	FILE * hFile;
	size_t WriteLen=0;

	Msg->length = Length;

	GetSemaphore(&MsgNoSemaphore, 0);
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;

	FreeSemaphore(&MsgNoSemaphore);
 
	strcpy(Msg->from, "SYSTEM");
	if (SendSYStoSYSOPCall)
		strcpy(Msg->to, SYSOPCall);
	else
		strcpy(Msg->to, "SYSOP");

	strcpy(Msg->title, Title);

	Msg->type = 'P';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	BIDRec = AllocateBIDRecord();
	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	hFile = fopen(MsgFile, "wb");

	if (hFile)
	{
		WriteLen = fwrite(MailBuffer, 1, Msg->length, hFile);
		fclose(hFile);
	}

	MatchMessagetoBBSList(Msg, NULL);
	free(MailBuffer);
}

VOID CheckBBSNumber(int i)
{
	// Make sure number is unique

	int Count = 0;
	struct UserInfo * user;

	for (user = BBSChain; user; user = user->BBSNext)
	{	
		if (user->BBSNumber == i)
		{
			Count++;

			if (Count > 1)
			{
				// Second with same number - Renumber this one

				user->BBSNumber = FindFreeBBSNumber();

				if (user->BBSNumber == 0)
					user->BBSNumber = NBBBS;			// cant really do much else

				Logprintf(LOG_BBS, NULL, '?', "Duplicate BBS Number found. BBS %s Old BBSNumber %d New BBS Number %d", user->Call, i, user->BBSNumber);

			}
		}
	}
}


int FindFreeBBSNumber()
{
	// returns the lowest number not used by any bbs or message.

	struct MsgInfo * Msg;
	struct UserInfo * user;
	int i, m;

	for (i = 1; i<= NBBBS; i++)
	{
		for (user = BBSChain; user; user = user->BBSNext)
		{
			if (user->BBSNumber == i)
				goto nexti;				// In use
		}

		// Not used by BBS - check messages

		for (m = 1; m <= NumberofMessages; m++)
		{
			Msg=MsgHddrPtr[m];

			if (check_fwd_bit(Msg->fbbs, i))
				goto nexti;				// In use

			if (check_fwd_bit(Msg->forw, i))
				goto nexti;				// In use
		}

		// Not in Use

		return i;
	
nexti:;

	}

	return 0;		// All used
}

BOOL SetupNewBBS(struct UserInfo * user)
{
	user->BBSNumber = FindFreeBBSNumber();

	if (user->BBSNumber == 0)
		return FALSE;

	user->BBSNext = BBSChain;
	BBSChain = user;

	SortBBSChain();

	ReinitializeFWDStruct(user);

	return TRUE;
}

VOID DeleteBBS(struct UserInfo * user)
{
	struct UserInfo * BBSRec, * PrevBBS = NULL;

#ifndef LINBPQ
	RemoveMenu(hFWDMenu, IDM_FORWARD_ALL + user->BBSNumber, MF_BYCOMMAND); 
#endif
	for (BBSRec = BBSChain; BBSRec; PrevBBS = BBSRec, BBSRec = BBSRec->BBSNext)
	{
		if (user == BBSRec)
		{
			if (PrevBBS == NULL)		// First in chain;
			{
				BBSChain = BBSRec->BBSNext;
				break;
			}
			PrevBBS->BBSNext = BBSRec->BBSNext;
			break;
		}
	}
}


VOID SetupFwdTimes(struct BBSForwardingInfo * ForwardingInfo);

VOID SetupForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;

	char Key[100] =  "BBSForwarding.";
	char Temp[100];

	HKEY hKey=0;
	char RegKey[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";

	int m;
	struct MsgInfo * Msg;

	ForwardingInfo = user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));

	if (UsingingRegConfig == 0)
	{
		//	Config from file

		if (isdigit(user->Call[0]) || user->Call[0] == '_')
			strcat(Key, "*");

		strcat(Key, user->Call);

		group = config_lookup (&cfg, Key);

		if (group == NULL)			// No info
			return;
		else
		{
			ForwardingInfo->TOCalls = GetMultiStringValue(group,  "TOCalls");
			ForwardingInfo->ConnectScript = GetMultiStringValue(group,  "ConnectScript");
			ForwardingInfo->ATCalls = GetMultiStringValue(group,  "ATCalls");
			ForwardingInfo->Haddresses = GetMultiStringValue(group,  "HRoutes");
			ForwardingInfo->HaddressesP = GetMultiStringValue(group,  "HRoutesP");
			ForwardingInfo->FWDTimes = GetMultiStringValue(group,  "FWDTimes");

			ForwardingInfo->Enabled = GetIntValue(group, "Enabled");
			ForwardingInfo->ReverseFlag = GetIntValue(group, "RequestReverse");
			ForwardingInfo->AllowBlocked = GetIntValue(group, "AllowBlocked");
			ForwardingInfo->AllowCompressed = GetIntValue(group, "AllowCompressed");
			ForwardingInfo->AllowB1 = GetIntValue(group, "UseB1Protocol");
			ForwardingInfo->AllowB2 = GetIntValue(group, "UseB2Protocol");
			ForwardingInfo->SendCTRLZ = GetIntValue(group, "SendCTRLZ");

			if (ForwardingInfo->AllowB1 || ForwardingInfo->AllowB2)
				ForwardingInfo->AllowCompressed = TRUE;

			if (ForwardingInfo->AllowCompressed)
				ForwardingInfo->AllowBlocked = TRUE;

			ForwardingInfo->PersonalOnly = GetIntValue(group, "FWDPersonalsOnly");
			ForwardingInfo->SendNew = GetIntValue(group, "FWDNewImmediately");
			ForwardingInfo->FwdInterval = GetIntValue(group, "FwdInterval");
			ForwardingInfo->RevFwdInterval = GetIntValue(group, "RevFWDInterval");
			ForwardingInfo->MaxFBBBlockSize = GetIntValue(group, "MaxFBBBlock");
			ForwardingInfo->ConTimeout = GetIntValue(group, "ConTimeout");

			if (ForwardingInfo->MaxFBBBlockSize == 0)
				ForwardingInfo->MaxFBBBlockSize = 10000;

			if (ForwardingInfo->FwdInterval == 0)
				ForwardingInfo->FwdInterval = 3600;

			if (ForwardingInfo->ConTimeout == 0)
				ForwardingInfo->ConTimeout = 120;

			GetStringValue(group, "BBSHA", Temp);
		
			if (Temp[0])
				ForwardingInfo->BBSHA = _strdup(Temp);
			else
				ForwardingInfo->BBSHA = _strdup("");
		}
	}
	else
	{
#ifndef	LINBPQ

		int retCode,Type,Vallen;

		strcat(RegKey, user->Call);
		retCode = RegOpenKeyEx (REGTREE, RegKey, 0, KEY_QUERY_VALUE, &hKey);

		if (retCode != ERROR_SUCCESS)
			return;
		else
		{
			ForwardingInfo->ConnectScript = RegGetMultiStringValue(hKey,  "Connect Script");
			ForwardingInfo->TOCalls = RegGetMultiStringValue(hKey,  "TOCalls");
			ForwardingInfo->ATCalls = RegGetMultiStringValue(hKey,  "ATCalls");
			ForwardingInfo->Haddresses = RegGetMultiStringValue(hKey,  "HRoutes");
			ForwardingInfo->HaddressesP = RegGetMultiStringValue(hKey,  "HRoutesP");
			ForwardingInfo->FWDTimes = RegGetMultiStringValue(hKey,  "FWD Times");

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "Enabled", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->Enabled,(ULONG *)&Vallen);
				
			Vallen=4;
			retCode += RegQueryValueEx(hKey, "RequestReverse", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->ReverseFlag,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "AllowCompressed", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowCompressed,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "Use B1 Protocol", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB1,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "Use B2 Protocol", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB2,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "SendCTRLZ", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->SendCTRLZ,(ULONG *)&Vallen);

			if (ForwardingInfo->AllowB1 || ForwardingInfo->AllowB2)
				ForwardingInfo->AllowCompressed = TRUE;
	
			Vallen=4;
			retCode += RegQueryValueEx(hKey, "FWD Personals Only", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->PersonalOnly,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey, "FWD New Immediately", 0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->SendNew,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"FWDInterval",0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->FwdInterval,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"RevFWDInterval",0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->RevFwdInterval,(ULONG *)&Vallen);

			RegQueryValueEx(hKey,"MaxFBBBlock",0,			
				(ULONG *)&Type,(UCHAR *)&ForwardingInfo->MaxFBBBlockSize,(ULONG *)&Vallen);

			if (ForwardingInfo->MaxFBBBlockSize == 0)
				ForwardingInfo->MaxFBBBlockSize = 10000;

			if (ForwardingInfo->FwdInterval == 0)
				ForwardingInfo->FwdInterval = 3600;

			Vallen=0;
			retCode = RegQueryValueEx(hKey,"BBSHA",0 , (ULONG *)&Type,NULL, (ULONG *)&Vallen);

			if (retCode != 0)
			{
				// No Key - Get from WP??
				
				WPRec * ptr = LookupWP(user->Call);

				if (ptr)
				{
					if (ptr->first_homebbs)
					{
						ForwardingInfo->BBSHA = _strdup(ptr->first_homebbs);
					}
				}
			}

			if (Vallen)
			{
				ForwardingInfo->BBSHA = malloc(Vallen);
				RegQueryValueEx(hKey, "BBSHA", 0, (ULONG *)&Type, ForwardingInfo->BBSHA,(ULONG *)&Vallen);
			}

			RegCloseKey(hKey);
		}
#endif
	}

	// Convert FWD Times and H Addresses

	if (ForwardingInfo->FWDTimes)
			SetupFwdTimes(ForwardingInfo);

	if (ForwardingInfo->Haddresses)
			SetupHAddreses(ForwardingInfo);

	if (ForwardingInfo->HaddressesP)
			SetupHAddresesP(ForwardingInfo);

	if (ForwardingInfo->BBSHA)
	{
			if (ForwardingInfo->BBSHA[0])
				SetupHAElements(ForwardingInfo);
			else
			{
				free(ForwardingInfo->BBSHA);
				ForwardingInfo->BBSHA = NULL;
			}
	}

	for (m = FirstMessageIndextoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		// If any forward bits are set, increment count on  BBS record.

		if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		{
			if (Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount++;
			}
		}
	}
}

VOID * GetMultiStringValue(config_setting_t * group, char * ValueName)
{
	char * ptr1;
	char * MultiString = NULL;
	const char * ptr;
	int Count = 0;
	char ** Value;
	config_setting_t *setting;
	char * Save;

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;

	setting = config_setting_get_member (group, ValueName);
	
	if (setting)
	{
		ptr =  config_setting_get_string (setting);

		Save = _strdup(ptr);			// DOnt want to change config string
		ptr = Save;
	
		while (ptr && strlen(ptr))
		{
			ptr1 = strchr(ptr, '|');
			
			if (ptr1)
				*(ptr1++) = 0;

			if (strlen(ptr))		// ignore null elements
			{
				Value = realloc(Value, (Count+2) * sizeof(void *));
				Value[Count++] = _strdup(ptr);
			}
			ptr = ptr1;
		}
		free(Save);
	}

	Value[Count] = NULL;
	return Value;
}


VOID * RegGetMultiStringValue(HKEY hKey, char * ValueName)
{
#ifdef LINBPQ
	return NULL;
#else
	int retCode,Type,Vallen;
	char * MultiString = NULL;
	int ptr, len;
	int Count = 0;
	char ** Value;

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values

	Value[0] = NULL;

	Vallen=0;


	retCode = RegQueryValueEx(hKey, ValueName, 0, (ULONG *)&Type, NULL, (ULONG *)&Vallen);

	if ((retCode != 0)  || (Vallen < 3))		// Two nulls means empty multistring
	{
		free(Value);
		return FALSE;
	}

	MultiString = malloc(Vallen);

	retCode = RegQueryValueEx(hKey, ValueName, 0,			
			(ULONG *)&Type,(UCHAR *)MultiString,(ULONG *)&Vallen);

	ptr=0;

	while (MultiString[ptr])
	{
		len=strlen(&MultiString[ptr]);

		Value = realloc(Value, (Count+2) * sizeof(void *));
		Value[Count++] = _strdup(&MultiString[ptr]);
		ptr+= (len + 1);
	}

	Value[Count] = NULL;

	free(MultiString);

	return Value;
#endif
}

VOID FreeForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;
	int i;


	ForwardingInfo = user->ForwardingInfo;

	FreeList(ForwardingInfo->TOCalls);
	FreeList(ForwardingInfo->ATCalls);
	FreeList(ForwardingInfo->Haddresses);
	FreeList(ForwardingInfo->HaddressesP);

	i=0;
	if(ForwardingInfo->HADDRS)
	{
		while(ForwardingInfo->HADDRS[i])
		{
			FreeList(ForwardingInfo->HADDRS[i]);
			i++;
		}
		free(ForwardingInfo->HADDRS);
		free(ForwardingInfo->HADDROffet);
	}

	i=0;
	if(ForwardingInfo->HADDRSP)
	{
		while(ForwardingInfo->HADDRSP[i])
		{
			FreeList(ForwardingInfo->HADDRSP[i]);
			i++;
		}
		free(ForwardingInfo->HADDRSP);
	}

	FreeList(ForwardingInfo->ConnectScript);
	FreeList(ForwardingInfo->FWDTimes);
	if (ForwardingInfo->FWDBands)
	{
		i=0;
		while(ForwardingInfo->FWDBands[i])
		{
			free(ForwardingInfo->FWDBands[i]);
			i++;
		}
		free(ForwardingInfo->FWDBands);
	}
	if (ForwardingInfo->BBSHAElements)
	{
		i=0;
		while(ForwardingInfo->BBSHAElements[i])
		{
			free(ForwardingInfo->BBSHAElements[i]);
			i++;
		}
		free(ForwardingInfo->BBSHAElements);
	}
	free(ForwardingInfo->BBSHA);

}

VOID FreeList(char ** Hddr)
{
	VOID ** Save;
	
	if (Hddr)
	{
		Save = (void **)Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]);
			Hddr++;
		}	
		free(Save);
	}
}


VOID ReinitializeFWDStruct(struct UserInfo * user)
{
	if (user->ForwardingInfo)
	{
		FreeForwardingStruct(user);
		free(user->ForwardingInfo); 
	}

	SetupForwardingStruct(user);

}

VOID SetupFwdTimes(struct BBSForwardingInfo * ForwardingInfo)
{
	char ** Times = ForwardingInfo->FWDTimes;
	int Start, End;
	int Count = 0;

	ForwardingInfo->FWDBands = zalloc(sizeof(struct FWDBAND));

	if (Times)
	{
		while(Times[0])
		{
			ForwardingInfo->FWDBands = realloc(ForwardingInfo->FWDBands, (Count+2)* sizeof(struct FWDBAND));
			ForwardingInfo->FWDBands[Count] = zalloc(sizeof(struct FWDBAND));

			Start = atoi(Times[0]);
			End = atoi(&Times[0][5]);

			ForwardingInfo->FWDBands[Count]->FWDStartBand =  (time_t)(Start / 100) * 3600 + (Start % 100) * 60; 
			ForwardingInfo->FWDBands[Count]->FWDEndBand =  (time_t)(End / 100) * 3600 + (End % 100) * 60 + 59; 

			Count++;
			Times++;
		}
		ForwardingInfo->FWDBands[Count] = NULL;
	}
}
void StartForwarding(int BBSNumber, char ** TempScript)
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;
	time_t NOW = time(NULL);


	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;

		if ((BBSNumber == 0) || (user->BBSNumber == BBSNumber))
			if (ForwardingInfo)
				if (ForwardingInfo->Enabled || BBSNumber)		// Menu Command overrides enable
					if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
						if (BBSNumber || SeeifMessagestoForward(user->BBSNumber, NULL) ||
							(ForwardingInfo->ReverseFlag && ((NOW - ForwardingInfo->LastReverseForward) >= ForwardingInfo->RevFwdInterval))) // Menu Command overrides Reverse
						{
							user->ForwardingInfo->ScriptIndex = -1;	// Incremented before being used
						
							// See if TempScript requested
							
							if (user->ForwardingInfo->TempConnectScript)
								FreeList(user->ForwardingInfo->TempConnectScript);

							user->ForwardingInfo->TempConnectScript = TempScript;
						
							if (ConnecttoBBS(user))
								ForwardingInfo->Forwarding = TRUE;
						}
	}

	return;
}

size_t fwritex(CIRCUIT * conn, void * _Str, size_t _Size, size_t _Count, FILE * _File)
{
	if (_File)
		return fwrite(_Str, _Size, _Count, _File);

	// Appending to MailBuffer

	memcpy(&conn->MailBuffer[conn->InputLen], _Str, _Count);
	conn->InputLen += (int)_Count;

	return _Count;
}


BOOL ForwardMessagestoFile(CIRCUIT * conn, char * FN)
{
	BOOL AddCRLF = FALSE;
	BOOL AutoImport = FALSE;
	FILE * Handle = NULL;
	char * Context;
	BOOL Email = FALSE;
	time_t now = time(NULL);
	char * param;

	FN = strtok_s(FN, " ,", &Context); 

	param = strtok_s(NULL, " ,", &Context); 

	if (param)
	{
		if (_stricmp(param, "ADDCRLF") == 0)
			AddCRLF = TRUE;

		if (_stricmp(param, "AutoImport") == 0)
			AutoImport = TRUE;

		param = strtok_s(NULL, " ,", &Context); 

		if (param)
		{
			if (_stricmp(param, "ADDCRLF") == 0)
				AddCRLF = TRUE;

			if (_stricmp(param, "AutoImport") == 0)
				AutoImport = TRUE;

		}
	}
	// If FN is an email address, write to a temp file, and send via rms or emali gateway
	
	if (strchr(FN, '@') || _memicmp(FN, "RMS:", 4) == 0)
	{
		Email = TRUE;
		AddCRLF =TRUE;
		conn->MailBuffer=malloc(100000);
		conn->MailBufferSize=100000;
		conn->InputLen = 0;
	}
	else
	{
		Handle = fopen(FN, "ab");

		if (Handle == NULL)
		{
			int err = GetLastError();
			Logprintf(LOG_BBS, conn, '!', "Failed to open Export File %s", FN);
			return FALSE;
		}
	}

	while (FindMessagestoForward(conn))
	{
		struct MsgInfo * Msg;
		struct tm * tm;
		time_t temp;
		char * MsgBytes = ReadMessageFile(conn->FwdMsg->number);
		int MsgLen;
		char * MsgPtr;
		char Line[256];
		int len;
		struct UserInfo * user = conn->UserPointer;
		int Index = 0;

		Msg = conn->FwdMsg;

		if (Email)
			if (conn->InputLen + Msg->length + 500 > conn->MailBufferSize)
				break;

		if (Msg->type == 'P')
			Index = PMSG;
		else if (Msg->type == 'B')
			Index = BMSG;
		else if (Msg->type == 'T')
			Index = TMSG;


		if (Msg->via[0])
			len = sprintf(Line, "S%c %s @ %s < %s $%s\r\n", Msg->type, Msg->to,
						Msg->via, Msg->from, Msg->bid);
		else
			len = sprintf(Line, "S%c %s < %s $%s\r\n", Msg->type, Msg->to, Msg->from, Msg->bid);
	
		fwritex(conn, Line, 1, len, Handle);

		len = sprintf(Line, "%s\r\n", Msg->title);
		fwritex(conn, Line, 1, len, Handle);
		
		if (MsgBytes == 0)
		{
			MsgBytes = _strdup("Message file not found\r\n");
			conn->FwdMsg->length = (int)strlen(MsgBytes);
		}

		MsgPtr = MsgBytes;
		MsgLen = conn->FwdMsg->length;

		// If a B2 Message, remove B2 Header

		if (conn->FwdMsg->B2Flags & B2Msg)
		{		
			// Remove all B2 Headers, and all but the first part.
					
			MsgPtr = strstr(MsgBytes, "Body:");
			
			if (MsgPtr)
			{
				MsgLen = atoi(&MsgPtr[5]);
				MsgPtr = strstr(MsgBytes, "\r\n\r\n");		// Blank Line after headers
	
				if (MsgPtr)
					MsgPtr +=4;
				else
					MsgPtr = MsgBytes;
			
			}
			else
				MsgPtr = MsgBytes;
		}

		memcpy(&temp, &Msg->datereceived, sizeof(time_t));
		tm = gmtime(&temp);	

		len = sprintf(Line, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
				tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
				conn->FwdMsg->number, BBSName, HRoute, RlineVer);

		fwritex(conn, Line, 1, len, Handle);

		if (memcmp(MsgPtr, "R:", 2) != 0)    // No R line, so must be our message - put blank line after header
			fwritex(conn, "\r\n", 1, 2, Handle);

		fwritex(conn, MsgPtr, 1, MsgLen, Handle);

		if (MsgPtr[MsgLen - 2] == '\r')
			fwritex(conn, "/EX\r\n", 1, 5, Handle);
		else
			fwritex(conn, "\r\n/EX\r\n", 1, 7, Handle);

		if (AddCRLF)
			fwritex(conn, "\r\n", 1, 2, Handle);

		free(MsgBytes);
			
		user->Total.MsgsSent[Index]++;
		user->Total.BytesForwardedOut[Index] += MsgLen;

		Msg->datechanged = now;
			
		clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
		set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);

		//  Only mark as forwarded if sent to all BBSs that should have it
			
		if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
		{
			conn->FwdMsg->status = 'F';			// Mark as forwarded
			conn->FwdMsg->datechanged=time(NULL);
		}

		conn->UserPointer->ForwardingInfo->MsgCount--;
	}

	if (Email)
	{
		struct MsgInfo * Msg;
		BIDRec * BIDRec;

		if (conn->InputLen == 0)
		{
			free(conn->MailBuffer);
			conn->MailBufferSize=0;
			conn->MailBuffer=0;

			return TRUE;
		}

		// Allocate a message Record slot

		Msg = AllocateMsgRecord();

		// Set number here so they remain in sequence
		
		GetSemaphore(&MsgNoSemaphore, 0);
		Msg->number = ++LatestMsg;
		FreeSemaphore(&MsgNoSemaphore);
		MsgnotoMsg[Msg->number] = Msg;

		Msg->type = 'P';
		Msg->status = 'N';
		Msg->datecreated = Msg->datechanged = Msg->datereceived = now;

		strcpy(Msg->from, BBSName);

		sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

		if (AutoImport)
			sprintf(Msg->title, "Batched messages for AutoImport from BBS %s",  BBSName);
		else
			sprintf(Msg->title, "Batched messages from BBS %s",  BBSName);

		Msg->length = conn->InputLen; 
		CreateMessageFile(conn, Msg);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->u.msgno = LOWORD(Msg->number);
		BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

		if (_memicmp(FN, "SMTP:", 5) == 0)
		{
			strcpy(Msg->via, &FN[5]);
			SMTPMsgCreated=TRUE;
		}
		else
		{
			strcpy(Msg->to, "RMS");
			if (_memicmp(FN, "RMS:", 4) == 0)
				strcpy(Msg->via, &FN[4]);
			else
				strcpy(Msg->via, FN);
		}

		MatchMessagetoBBSList(Msg, conn);

		SaveMessageDatabase();
		SaveBIDDatabase();
	}
	else
		fclose(Handle);

	SaveMessageDatabase();
	return TRUE;
}

BOOL ForwardMessagetoFile(struct MsgInfo * Msg, FILE * Handle)
{
	struct tm * tm;
	time_t temp;

	char * MsgBytes = ReadMessageFile(Msg->number);
	char * MsgPtr;
	char Line[256];
	int len;
	int MsgLen = Msg->length;

	if (Msg->via[0])
		len = sprintf(Line, "S%c %s @ %s < %s $%s\r\n", Msg->type, Msg->to,
			Msg->via, Msg->from, Msg->bid);
	else
		len = sprintf(Line, "S%c %s < %s $%s\r\n", Msg->type, Msg->to, Msg->from, Msg->bid);
	
	fwrite(Line, 1, len, Handle);

	len = sprintf(Line, "%s\r\n", Msg->title);
	fwrite(Line, 1, len, Handle);
		
	if (MsgBytes == 0)
	{
		MsgBytes = _strdup("Message file not found\r\n");
		MsgLen = (int)strlen(MsgBytes);
		}

	MsgPtr = MsgBytes;

	// If a B2 Message, remove B2 Header

	if (Msg->B2Flags & B2Msg)
	{		
		// Remove all B2 Headers, and all but the first part.
					
		MsgPtr = strstr(MsgBytes, "Body:");
			
		if (MsgPtr)
		{
			MsgLen = atoi(&MsgPtr[5]);
			
			MsgPtr= strstr(MsgBytes, "\r\n\r\n");		// Blank Line after headers
	
			if (MsgPtr)
				MsgPtr +=4;
			else
				MsgPtr = MsgBytes;
			
		}
		else
			MsgPtr = MsgBytes;
	}

	memcpy(&temp, &Msg->datereceived, sizeof(time_t));
	tm = gmtime(&temp);	

	len = sprintf(Line, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
			tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
			Msg->number, BBSName, HRoute, RlineVer);

	fwrite(Line, 1, len, Handle);

	if (memcmp(MsgPtr, "R:", 2) != 0)    // No R line, so must be our message - put blank line after header
		fwrite("\r\n", 1, 2, Handle);

	fwrite(MsgPtr, 1, MsgLen, Handle);

	if (MsgPtr[MsgLen - 2] == '\r')
		fwrite("/EX\r\n", 1, 5, Handle);
	else
		fwrite("\r\n/EX\r\n", 1, 7, Handle);

	free(MsgBytes);
		
	return TRUE;

}

BOOL ConnecttoBBS (struct UserInfo * user)
{
	int n, p;
	CIRCUIT * conn;
	struct	BBSForwardingInfo * ForwardingInfo = user->ForwardingInfo;

	for (n = NumberofStreams-1; n >= 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			p = conn->BPQStream;
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->BPQStream = p;

			// Can't set Active until Connected or Stuck Session detertor can clear session.
			// But must set Active before Connected() runs or will appear is Incoming Connect.
			// Connected() is semaphored, so get semaphore before ConnectUsingAppl
			// Probably better to semaphore lost session code instead


			strcpy(conn->Callsign, user->Call); 
			conn->BBSFlags |= (RunningConnectScript | OUTWARDCONNECT);
			conn->UserPointer = user;

			Logprintf(LOG_BBS, conn, '|', "Connecting to BBS %s", user->Call);

			ForwardingInfo->MoreLines = TRUE;
			
			GetSemaphore(&ConSemaphore, 1);
			conn->Active = TRUE;
			ConnectUsingAppl(conn->BPQStream, BBSApplMask);
			FreeSemaphore(&ConSemaphore);

			// If we are sending to a dump pms we may need to connect using the message sender's callsign.
			// But we wont know until we run the connect script, which is a bit late to change call. Could add
			// flag to forwarding config, but easier to look for SETCALLTOSENDER in the connect script.

			if (strstr(ForwardingInfo->ConnectScript[0], "SETCALLTOSENDER"))
			{
				conn->SendB = conn->SendP = conn->SendT = conn->DoReverse = TRUE;
				conn->MaxBLen = conn->MaxPLen = conn->MaxTLen = 99999999;
				
				if (FindMessagestoForward(conn) && conn->FwdMsg)
				{
					// We have a message to send

					struct MsgInfo * Msg;
					unsigned char AXCall[7];

					Msg = conn->FwdMsg;
					ConvToAX25(Msg->from, AXCall);
					ChangeSessionCallsign(p, AXCall);

					conn->BBSFlags |= TEXTFORWARDING | SETCALLTOSENDER | NEWPACCOM;
					conn->NextMessagetoForward = 0;		// was set by FindMessages
				}
				conn->SendB = conn->SendP = conn->SendT = conn->DoReverse = FALSE;
			}
#ifdef LINBPQ
			{
				BPQVECSTRUC * SESS;	
				SESS = &BPQHOSTVECTOR[conn->BPQStream - 1];

				if (SESS->HOSTSESSION == NULL)
				{
					Logprintf(LOG_BBS, NULL, '|', "No L4 Sessions for connect to BBS %s", user->Call);
					return FALSE;
				}

				SESS->HOSTSESSION->Secure_Session = 1;
			}
#endif

			strcpy(conn->Callsign, user->Call);

			//	Connected Event will trigger connect to remote system

			RefreshMainWindow();

			return TRUE;
		}
	}

	Logprintf(LOG_BBS, NULL, '|', "No Free Streams for connect to BBS %s", user->Call);

	return FALSE;
	
}

struct DelayParam
{
	struct UserInfo * User;
	CIRCUIT * conn;
	int Delay;
};

struct DelayParam DParam;		// Not 100% safe, but near enough

VOID ConnectDelayThread(struct DelayParam * DParam)
{
	struct UserInfo * User = DParam->User;
	int Delay = DParam->Delay;

	User->ForwardingInfo->Forwarding = TRUE;		// Minimize window for two connects

	Sleep(Delay);

	User->ForwardingInfo->Forwarding = TRUE;
	ConnecttoBBS(User);
	
	return;
}

VOID ConnectPauseThread(struct DelayParam * DParam)
{
	CIRCUIT * conn = DParam->conn;
	int Delay = DParam->Delay;
	char Msg[] = "Pause Ok\r    ";

	Sleep(Delay);

	ProcessBBSConnectScript(conn, Msg, 9);
	
	return;
}


/*
BOOL ProcessBBSConnectScriptInner(CIRCUIT * conn, char * Buffer, int len);


BOOL ProcessBBSConnectScript(CIRCUIT * conn, char * Buffer, int len)
{
	BOOL Ret;
	GetSemaphore(&ScriptSEM);
	Ret = ProcessBBSConnectScriptInner(conn, Buffer, len);
	FreeSemaphore(&ScriptSEM);

	return Ret;
}
*/

BOOL ProcessBBSConnectScript(CIRCUIT * conn, char * Buffer, int len)
{
	struct	BBSForwardingInfo * ForwardingInfo = conn->UserPointer->ForwardingInfo;
	char ** Scripts;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;
	char * ptr, * ptr2;

	WriteLogLine(conn, '<',Buffer, len-1, LOG_BBS);

	Buffer[len]=0;
	_strupr(Buffer);

	if (ForwardingInfo->TempConnectScript)
		Scripts = ForwardingInfo->TempConnectScript;
	else
		Scripts = ForwardingInfo->ConnectScript;	

	if (ForwardingInfo->ScriptIndex == -1)
	{
		// First Entry - if first line is TIMES, check and skip forward if necessary
	
		int n = 0;
		int Start, End;
		time_t now = time(NULL), StartSecs, EndSecs;
		char * Line;

		if (Localtime)
			now -= (time_t)_MYTIMEZONE; 

		now %= 86400;
		Line = Scripts[n];

		if (_memicmp(Line, "TIMES", 5) == 0)
		{
		NextBand:
			Start = atoi(&Line[6]);
			End = atoi(&Line[11]);

			StartSecs =  (time_t)(Start / 100) * 3600 + (Start % 100) * 60; 
			EndSecs =  (time_t)(End / 100) * 3600 + (End % 100) * 60 + 59;

			if ((StartSecs <= now) && (EndSecs >= now))
				goto InBand;	// In band

			// Look for next TIME
		NextLine:
			Line = Scripts[++n];

			if (Line == NULL)
			{
				// No more lines - Disconnect

				conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
				Disconnect(conn->BPQStream);
				return FALSE;
			}

			if (_memicmp(Line, "TIMES", 5) != 0)
				goto NextLine;
			else
				goto NextBand;
InBand:
			ForwardingInfo->ScriptIndex = n;	
		}

	}
	else
	{
		// Dont check first time through

		if (strcmp(Buffer, "*** CONNECTED  ") != 0)
		{
			if (Scripts[ForwardingInfo->ScriptIndex] == NULL ||
				_memicmp(Scripts[ForwardingInfo->ScriptIndex], "TIMES", 5) == 0	||		// Only Check until script is finished
				_memicmp(Scripts[ForwardingInfo->ScriptIndex], "ELSE", 4) == 0)			// Only Check until script is finished
			{
				ForwardingInfo->MoreLines = FALSE;
			}
			if (!ForwardingInfo->MoreLines)
				goto CheckForSID;
			}
	}

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") ||
		(strstr(Buffer, "DOWNLINK") && strstr(Buffer, "ATTEMPTING") == 0) ||
		strstr(Buffer, "SORRY") || strstr(Buffer, "INVALID") || strstr(Buffer, "RETRIED") ||
		strstr(Buffer, "NO CONNECTION TO") || strstr(Buffer, "ERROR - ") ||
		strstr(Buffer, "UNABLE TO CONNECT") ||  strstr(Buffer, "DISCONNECTED") ||
		strstr(Buffer, "FAILED TO CONNECT") ||	strstr(Buffer, "REJECTED"))
	{
		// Connect Failed

		char * Cmd = Scripts[++ForwardingInfo->ScriptIndex];
		int Delay = 1000;
	
		// Look for an alternative connect block (Starting with ELSE)

	ElseLoop:

		// Skip any comments
		
		while (Cmd && ((strcmp(Cmd, " ") == 0 || Cmd[0] == ';' || Cmd[0] == '#')))
			Cmd = Scripts[++ForwardingInfo->ScriptIndex];

		// TIMES terminates a script

		if (Cmd == 0 || _memicmp(Cmd, "TIMES", 5) == 0)			// Only Check until script is finished
		{
			conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
			Disconnect(conn->BPQStream);
			return FALSE;
		}

		if (_memicmp(Cmd, "ELSE", 4) != 0)
		{
			Cmd = Scripts[++ForwardingInfo->ScriptIndex];
			goto ElseLoop;
		}

		if (_memicmp(&Cmd[5], "DELAY", 5) == 0)
			Delay = atoi(&Cmd[10]) * 1000;
		else
			Delay = 1000;

		conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
		Disconnect(conn->BPQStream);

		DParam.Delay = Delay;
		DParam.User = conn->UserPointer;

		_beginthread((void (*)(void *))ConnectDelayThread, 0, &DParam);
		
		return FALSE;
	}

	// The pointer is only updated when we get the connect, so we can tell when the last line is acked
	// The first entry is always from Connected event, so don't have to worry about testing entry -1 below


	// NETROM to  KA node returns

	//c 1 milsw
	//WIRAC:N9PMO-2} Connected to MILSW
	//###CONNECTED TO NODE MILSW(N9ZXS) CHANNEL A
	//You have reached N9ZXS's KA-Node MILSW
	//ENTER COMMAND: B,C,J,N, or Help ?

	//C KB9PRF-7
	//###LINK MADE
	//###CONNECTED TO NODE KB9PRF-7(KB9PRF-4) CHANNEL A

	// Look for (Space)Connected so we aren't fooled by ###CONNECTED TO NODE, which is not
	// an indication of a connect.

	if (strstr(Buffer, " CONNECTED") || strstr(Buffer, "PACLEN") || strstr(Buffer, "IDLETIME") ||
		strstr(Buffer, "OK") || strstr(Buffer, "###LINK MADE") || strstr(Buffer, "VIRTUAL CIRCUIT ESTABLISHED"))
	{
		// If connected to SYNC, save IP address and port

		char * Cmd;

		if (strstr(Buffer, "*** CONNECTED TO SYNC"))
		{
			char * IPAddr = &Buffer[22];
			char * Port = strlop(IPAddr, ':');

			if (Port)
			{
				if (conn->SyncHost)
					free(conn->SyncHost);

				conn->SyncHost = _strdup(IPAddr);
				conn->SyncPort = atoi(Port);
			}
		}

		if (conn->SkipConn)
		{
			conn->SkipConn = FALSE;
			return TRUE;
		}

	LoopBack:

		Cmd = Scripts[++ForwardingInfo->ScriptIndex];

		// Only Check until script is finished
		
		if (Cmd && (strcmp(Cmd, " ") == 0 || Cmd[0] == ';' || Cmd[0] == '#'))
			goto LoopBack;			// Blank line 

		if (Cmd && _memicmp(Cmd, "TIMES", 5) != 0 && _memicmp(Cmd, "ELSE", 4) != 0)			// Only Check until script is finished
		{
			if (_memicmp(Cmd, "MSGTYPE", 7) == 0)
			{
				char * ptr;
				
				// Select Types to send. Only send types in param. Only reverse if R in param

				_strupr(Cmd);
				
				Logprintf(LOG_BBS, conn, '?', "Script %s", Cmd);

				conn->SendB = conn->SendP = conn->SendT = conn->DoReverse = FALSE;

				strcpy(conn->MSGTYPES, &Cmd[8]);

				if (strchr(&Cmd[8], 'R')) conn->DoReverse = TRUE;

				ptr = strchr(&Cmd[8], 'B');
	
				if (ptr)
				{
					conn->SendB = TRUE;
					conn->MaxBLen = atoi(++ptr);
					if (conn->MaxBLen == 0) conn->MaxBLen = 99999999;
				}

				ptr = strchr(&Cmd[8], 'T');
	
				if (ptr)
				{
					conn->SendT = TRUE;
					conn->MaxTLen = atoi(++ptr);
					if (conn->MaxTLen == 0) conn->MaxTLen = 99999999;
				}
				ptr = strchr(&Cmd[8], 'P');

				if (ptr)
				{
					conn->SendP = TRUE;
					conn->MaxPLen = atoi(++ptr);
					if (conn->MaxPLen == 0) conn->MaxPLen = 99999999;
				}

				// If nothing to do, terminate script

				if (conn->DoReverse || SeeifMessagestoForward(conn->UserPointer->BBSNumber, conn))
					goto LoopBack;

				Logprintf(LOG_BBS, conn, '?', "Nothing to do - quitting");
				conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
				Disconnect(conn->BPQStream);
				return FALSE;
			}

			if (_memicmp(Cmd, "INTERLOCK ", 10) == 0)
			{
				// Used to limit connects on a port to 1

				int Port;
				char Option[80];

				Logprintf(LOG_BBS, conn, '?', "Script %s", Cmd);

				sscanf(&Cmd[10], "%d %s", &Port, &Option[0]);

				if (CountConnectionsOnPort(Port))
				{								
					Logprintf(LOG_BBS, conn, '?', "Interlocked Port is busy - quitting");
					conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
					Disconnect(conn->BPQStream);
					return FALSE;
				}

				goto LoopBack;
			}

			if (_memicmp(Cmd, "RADIO AUTH", 10) == 0)
			{
				// Generate a Password to enable RADIO commands on a remote node
				char AuthCommand[80];

				_strupr(Cmd);
				strcpy(AuthCommand, Cmd);

				CreateOneTimePassword(&AuthCommand[11], &Cmd[11], 0); 

				nodeprintf(conn, "%s\r", AuthCommand);
				return TRUE;
			}

			if (_memicmp(Cmd, "SKIPCON", 7) == 0)
			{
				// Remote Node sends Connected in CTEXT - we need to swallow it

				conn->SkipConn = TRUE;
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "SendWL2KPM", 10) == 0|| _memicmp(Cmd, "SendWL2KFW", 10) == 0)
			{
				// Send ;FW: command

				conn->SendWL2KFW = TRUE;
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "SKIPPROMPT", 10) == 0)
			{
				// Remote Node sends > at end of CTEXT - we need to swallow it

				conn->SkipPrompt++;
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "TEXTFORWARDING", 10) == 0)
			{
				conn->BBSFlags |= TEXTFORWARDING;			
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "SETCALLTOSENDER", 15) == 0)
			{
				conn->BBSFlags |= TEXTFORWARDING | SETCALLTOSENDER;			
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "RADIOONLY", 9) == 0)
			{
				conn->BBSFlags |= WINLINKRO;			
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "SYNC", 4) == 0)
			{
				conn->BBSFlags |= SYNCMODE;			
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "NEEDLF", 6) == 0)
			{
				conn->BBSFlags |= NEEDLF;			
				goto CheckForEnd;
			}

			if (_memicmp(Cmd, "MCASTRX", 6) == 0)
			{
				conn->BBSFlags |= MCASTRX;	
				conn->MCastListenTime = atoi(&Cmd[7]) * 6; // Time to run session for *6 as value is mins put timer ticks 10 secs

				// send MCAST to Node
				
				nodeprintfEx(conn, "MCAST\r");
				return TRUE;
			}

			if (_memicmp(Cmd, "FLARQ", 5) == 0)
			{
				conn->BBSFlags |= FLARQMAIL;

		CheckForEnd:
				if (Scripts[ForwardingInfo->ScriptIndex + 1] == NULL ||
						memcmp(Scripts[ForwardingInfo->ScriptIndex +1], "TIMES", 5) == 0	||		// Only Check until script is finished
						memcmp(Scripts[ForwardingInfo->ScriptIndex + 1], "ELSE", 4) == 0)			// Only Check until script is finished
					ForwardingInfo->MoreLines = FALSE;
			
				goto LoopBack;
			}
			if (_memicmp(Cmd, "PAUSE", 5) == 0)
			{
				// Pause script

				Logprintf(LOG_BBS, conn, '?', "Script %s", Cmd);

				DParam.Delay = atoi(&Cmd[6]) * 1000;
				DParam.conn = conn;

				_beginthread((void (*)(void *))ConnectPauseThread, 0, &DParam);

				return TRUE;
			}

			if (_memicmp(Cmd, "FILE", 4) == 0)
			{
				if (Cmd[4] == 0)
				{
					// Missing Filename
					
					Logprintf(LOG_BBS, conn, '!', "Export file name missing");
				}
				else
					ForwardMessagestoFile(conn, &Cmd[5]);
				
				conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
				Disconnect(conn->BPQStream);
				return FALSE;
			}

			if (_memicmp(Cmd, "SMTP", 4) == 0)
			{
				conn->NextMessagetoForward = FirstMessageIndextoForward;
				conn->UserPointer->Total.ConnectsOut++;

				SendAMPRSMTP(conn);
				conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
				Disconnect(conn->BPQStream);
				return FALSE;
			}


			if (_memicmp(Cmd, "IMPORT", 6) == 0)
			{
				char * File, * Context;
				int Num;
				char * Temp = _strdup(Cmd);

				File = strtok_s(&Temp[6], " ", &Context);

				if (File && File[0]) 
				{
					Num = ImportMessages(NULL, File, TRUE);

					Logprintf(LOG_BBS, NULL, '|', "Imported %d Message(s) from %s", Num, File);

					if (Context && _stricmp(Context, "delete") == 0)
						DeleteFile(File);
				}
				free(Temp);

				if (Scripts[ForwardingInfo->ScriptIndex + 1] == NULL ||
						memcmp(Scripts[ForwardingInfo->ScriptIndex +1], "TIMES", 5) == 0	||		// Only Check until script is finished
						memcmp(Scripts[ForwardingInfo->ScriptIndex + 1], "ELSE", 4) == 0)			// Only Check until script is finished
				{
					conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
					Disconnect(conn->BPQStream);
					return FALSE;
				}
				goto LoopBack;
			}

			// Anything else is sent to Node

			// Replace \ with # so can send commands starting with #

			if (Cmd[0] == '\\')
			{
				Cmd[0] = '#';
				nodeprintfEx(conn, "%s\r", Cmd);
				Cmd[0] = '\\';						// Put \ back in script
			}
			else
				nodeprintfEx(conn, "%s\r", Cmd);
			
			return TRUE;
		}

		// End of script.

		ForwardingInfo->MoreLines = FALSE;

		if (conn->BBSFlags & MCASTRX)
		{
			// No session with Multicast, so no SID

			conn->BBSFlags &= ~RunningConnectScript;
			return TRUE;
		}

		if (conn->BBSFlags & FLARQMAIL)
		{
			// FLARQ doesnt send a prompt - Just send message(es)

			conn->UserPointer->Total.ConnectsOut++;
			conn->BBSFlags &= ~RunningConnectScript;
			ForwardingInfo->LastReverseForward = time(NULL);

			//	Update Paclen
					
			GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);
		
			if (paclen > 0)
				conn->paclen = paclen;

			SendARQMail(conn);
			return TRUE;
		}


		return TRUE;
	}

	ptr = strchr(Buffer, '}');

	if (ptr && ForwardingInfo->MoreLines) // Beware it could be part of ctext
	{
		// Could be respsonse to Node Command 

		ptr+=2;
		
		ptr2 = strchr(&ptr[0], ' ');

		if (ptr2)
		{
			if (_memicmp(ptr, Scripts[ForwardingInfo->ScriptIndex], ptr2-ptr) == 0)	// Reply to last sscript command
			{
				if (Scripts[ForwardingInfo->ScriptIndex+1] && _memicmp(Scripts[ForwardingInfo->ScriptIndex+1], "else", 4) == 0)
				{
					// stray match or misconfigured

					return TRUE;
				}

				ForwardingInfo->ScriptIndex++;
		
				if (Scripts[ForwardingInfo->ScriptIndex])
					if (_memicmp(Scripts[ForwardingInfo->ScriptIndex], "TIMES", 5) != 0)	
					nodeprintf(conn, "%s\r", Scripts[ForwardingInfo->ScriptIndex]);

				return TRUE;
			}
		}
	}

	// Not Success or Fail. If last line is still outstanding, wait fot Response
	//		else look for SID or Prompt

	if (conn->SkipPrompt && Buffer[len-2] == '>')
	{
		conn->SkipPrompt--;
		return TRUE;
	}

	if (ForwardingInfo->MoreLines)
		return TRUE;

	// No more steps, Look for SID or Prompt

CheckForSID:

	if (strstr(Buffer, "POSYNCHELLO"))			// RMS RELAY Sync process
	{
		conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
		conn->NextMessagetoForward = FirstMessageIndextoForward;
		conn->UserPointer->Total.ConnectsOut++;
		ForwardingInfo->LastReverseForward = time(NULL);

		ProcessLine(conn, 0, Buffer, len);
		return FALSE;
	}

	if (strstr(Buffer, "SORRY, NO"))			// URONODE
	{
		conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
		Disconnect(conn->BPQStream);
		return FALSE;
	}

	if (memcmp(Buffer, ";PQ: ", 5) == 0)
	{
		// Secure CMS challenge

		int Len;
		struct UserInfo * User = conn->UserPointer;
		char * Pass = User->CMSPass;
		int Response ;
		char RespString[12];
		char ConnectingCall[10];

#ifdef LINBPQ
		BPQVECSTRUC * SESS = &BPQHOSTVECTOR[0];
#else
		BPQVECSTRUC * SESS = (BPQVECSTRUC *)BPQHOSTVECPTR;
#endif

		SESS += conn->BPQStream - 1;

		ConvFromAX25(SESS->HOSTSESSION->L4USER, ConnectingCall);

		strlop(ConnectingCall, ' ');

		if (Pass[0] == 0)
		{
			Pass = User->pass;		// Old Way
			if (Pass[0] == 0)
			{
				strlop(ConnectingCall, '-');
				User = LookupCall(ConnectingCall);
				if (User)
					Pass = User->CMSPass;
			}
		}

		// 

		Response = GetCMSHash(&Buffer[5], Pass);

		sprintf(RespString, "%010d", Response);

		Len = sprintf(conn->SecureMsg, ";PR: %s\r", &RespString[2]);

		// Save challengs in case needed for FW lines

		strcpy(conn->PQChallenge, &Buffer[5]);

		return FALSE;
	}


	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// Update PACLEN

		GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

		if (paclen > 0)
			conn->paclen = paclen;

		
		Parse_SID(conn, &Buffer[1], len-4);
			
		if (conn->BBSFlags & FBBForwarding)
		{
			conn->FBBIndex = 0;		// ready for first block;
			memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			conn->FBBChecksum = 0;
		}

		return TRUE;
	}

	if (memcmp(Buffer, "[PAKET ", 7) == 0)
	{
		conn->BBSFlags |= BBS;
		conn->BBSFlags |= MBLFORWARDING;
	}

	if (Buffer[len-2] == '>')
	{
		if (conn->SkipPrompt)
		{
			conn->SkipPrompt--;
			return TRUE;
		}

		conn->NextMessagetoForward = FirstMessageIndextoForward;
		conn->UserPointer->Total.ConnectsOut++;
		conn->BBSFlags &= ~RunningConnectScript;
		ForwardingInfo->LastReverseForward = time(NULL);

		if (memcmp(Buffer, "[AEA PK", 7) == 0 || (conn->BBSFlags & TEXTFORWARDING))
		{
			// PK232. Don't send a SID, and switch to Text Mode

			conn->BBSFlags |= (BBS | TEXTFORWARDING);
			conn->Flags |= SENDTITLE;

			// Send Message. There is no mechanism for reverse forwarding

			if (FindMessagestoForward(conn) && conn->FwdMsg)
			{
				struct MsgInfo * Msg;
				
				// Send S line and wait for response - SB WANT @ USA < W8AAA $1029_N0XYZ 

				Msg = conn->FwdMsg;
		
				if ((conn->BBSFlags & SETCALLTOSENDER))
					nodeprintf(conn, "S%c %s @ %s \r", Msg->type, Msg->to,
						(Msg->via[0]) ? Msg->via : conn->UserPointer->Call);
				else
					nodeprintf(conn, "S%c %s @ %s < %s $%s\r", Msg->type, Msg->to,
						(Msg->via[0]) ? Msg->via : conn->UserPointer->Call, 
						Msg->from, Msg->bid);
			}
			else
			{
				conn->BBSFlags &= ~RunningConnectScript;	// so it doesn't get reentered
				Disconnect(conn->BPQStream);
				return FALSE;
			}
			
			return TRUE;
		}

		if (strcmp(conn->Callsign, "RMS") == 0 || conn->SendWL2KFW)
		{
			// Build a ;FW: line with all calls with PollRMS Set

			// According to Lee if you use secure login the first
			// must be the BBS call
			//	Actually I don't think we need the first,
			//	as that is implied

			//	If a secure password is available send the new 
			//	call|response format.

			//	I think this should use the session callsign, which 
			//	normally will be the BBS ApplCall, and not the BBS Name, 
			//	but coudl be changed by *** LINKED

			int i, s;
			char FWLine[10000] = ";FW: ";
			struct UserInfo * user;
			char RMSCall[20];
			char ConnectingCall[10];

#ifdef LINBPQ
			BPQVECSTRUC * SESS = &BPQHOSTVECTOR[0];
#else
			BPQVECSTRUC * SESS = (BPQVECSTRUC *)BPQHOSTVECPTR;
#endif

			SESS += conn->BPQStream - 1;

			ConvFromAX25(SESS->HOSTSESSION->L4USER, ConnectingCall);
			strlop(ConnectingCall, ' ');

			strcat (FWLine, ConnectingCall);
			
			for (i = 0; i <= NumberofUsers; i++)
			{
				user = UserRecPtr[i];

				if (user->flags & F_POLLRMS)
				{
					if (user->RMSSSIDBits == 0) user->RMSSSIDBits = 1;

					for (s = 0; s < 16; s++)
					{
						if (user->RMSSSIDBits & (1 << s))
						{
							if (s)
								sprintf(RMSCall, "%s-%d", user->Call, s);
							else
								sprintf(RMSCall, "%s", user->Call);

							// We added connectingcall at front
							
							if (strcmp(RMSCall, ConnectingCall) != 0)
							{
								strcat(FWLine, " ");
								strcat(FWLine, RMSCall);
					
								if (user->CMSPass[0])
								{
									int Response = GetCMSHash(conn->PQChallenge, user->CMSPass);
									char RespString[12];
	
									sprintf(RespString, "%010d", Response);
									strcat(FWLine, "|");
									strcat(FWLine, &RespString[2]);
								}
							}
						}
					}
				}
			}
			
			strcat(FWLine, "\r");	

			nodeprintf(conn, FWLine);
		}

		// Only declare B1 and B2 if other end did, and we are configued for it

		nodeprintfEx(conn, BBSSID, "BPQ-",
			Ver[0], Ver[1], Ver[2], Ver[3],
			(conn->BBSFlags & FBBCompressed) ? "B" : "", 
			(conn->BBSFlags & FBBB1Mode && !(conn->BBSFlags & FBBB2Mode)) ? "1" : "",
			(conn->BBSFlags & FBBB2Mode) ? "2" : "",
			(conn->BBSFlags & FBBForwarding) ? "F" : "", 
			(conn->BBSFlags & WINLINKRO) ? "" : "J"); 

		if (conn->SecureMsg[0])
		{
			struct UserInfo * user;
			BBSputs(conn, conn->SecureMsg);
			conn->SecureMsg[0] = 0;

			// Also send a Location Comment Line

			//; GM8BPQ-10 DE G8BPQ (IO92KX)<cr>
			//; WL2K DE GM8BPQ ()<cr>			(PAT)

			user = LookupCall(BBSName);

			if (LOC && LOC[0])
				nodeprintf(conn, "; WL2K DE %s (%s)\r", BBSName, LOC);
		}

		if (conn->BPQBBS && conn->MSGTYPES[0])

			// Send a ; MSGTYPES to control what he sends us

			nodeprintf(conn, "; MSGTYPES %s\r", conn->MSGTYPES);

		if (conn->BBSFlags & FBBForwarding)
		{
			if (!FBBDoForward(conn))				// Send proposal if anthing to forward
			{
				if (conn->DoReverse)
					FBBputs(conn, "FF\r");
				else
				{
					FBBputs(conn, "FQ\r");
					conn->CloseAfterFlush = 20;			// 2 Secs
				}
			}

			return TRUE;
		}

		return TRUE;
	}

	return TRUE;
}

VOID Parse_SID(CIRCUIT * conn, char * SID, int len)
{
	ChangeSessionIdletime(conn->BPQStream, BBSIDLETIME);		// Default Idletime for BBS Sessions

	// scan backwards for first '-'

	if (strstr(SID, "BPQCHATSERVER"))
	{
		Disconnect(conn->BPQStream);
		return;
	}

	if (strstr(SID, "RMS Ex") || strstr(SID, "Winlink Ex"))
	{
		conn->RMSExpress = TRUE;
		conn->Paclink = FALSE;
		conn->PAT = FALSE;

		// Set new RMS Users as RMS User

		if (conn->NewUser)
			conn->UserPointer->flags |= F_Temp_B2_BBS;
	}

	if (stristr(SID, "PAT"))
	{
		// Set new PAT Users as RMS User

		conn->RMSExpress = FALSE;
		conn->Paclink = FALSE;
		conn->PAT = TRUE;

		if (conn->NewUser)
			conn->UserPointer->flags |= F_Temp_B2_BBS;
	}
	if (strstr(SID, "Paclink"))
	{
		conn->RMSExpress = FALSE;
		conn->Paclink = TRUE;
	}

	if (strstr(SID, "WL2K-"))
	{
		conn->WL2K = TRUE;
		conn->BBSFlags |= WINLINKRO;
	}

	if (strstr(SID, "MFJ-"))
	{
		conn->BBSFlags |= MFJMODE;
	}

	if (_memicmp(SID, "OpenBCM", 7) == 0)
	{
		// We should really only do this on Telnet Connections, as OpenBCM flag is used to remove relnet transparency


		conn->OpenBCM = TRUE;
	}

	if (_memicmp(SID, "PMS-3.2", 7) == 0)
	{
		// Paccom TNC that doesn't send newline prompt ater receiving subject

		conn->BBSFlags |= NEWPACCOM;
	}

	// See if BPQ for selective forwarding 

	if (strstr(SID, "BPQ"))
		conn->BPQBBS = TRUE;

	while (len > 0)
	{
		switch (SID[len--])
		{
		case '-':

			len=0;
			break;

		case '$':

			conn->BBSFlags |= BBS | MBLFORWARDING;
			conn->Paging = FALSE;

			break;

		case 'F':			// FBB Blocked Forwarding

			// We now support blocked uncompressed. Not necessarily compatible with FBB		

			if ((conn->UserPointer->ForwardingInfo == NULL) && (conn->UserPointer->flags & F_PMS))
			{
				// We need to allocate a forwarding structure

					conn->UserPointer->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));
					conn->UserPointer->ForwardingInfo->AllowCompressed = TRUE;
					conn->UserPointer->ForwardingInfo->AllowBlocked = TRUE;
					conn->UserPointer->BBSNumber = NBBBS;
			}

			if (conn->UserPointer->ForwardingInfo->AllowBlocked)
			{
				conn->BBSFlags |= FBBForwarding | BBS;
				conn->BBSFlags &= ~MBLFORWARDING;
		
				conn->Paging = FALSE;

				if ((conn->UserPointer->ForwardingInfo == NULL) && (conn->UserPointer->flags & F_PMS))
				{
					// We need to allocate a forwarding structure

						conn->UserPointer->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));
						conn->UserPointer->ForwardingInfo->AllowCompressed = TRUE;
						conn->UserPointer->BBSNumber = NBBBS;
				}
			
				// Allocate a Header Block

				conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));
			}
			break;

		case 'J':

			// Suspected to be associated with Winlink Radio Only

			conn->BBSFlags &= ~WINLINKRO;
			break;

		case 'B':

			if (conn->UserPointer->ForwardingInfo->AllowCompressed)
			{
				conn->BBSFlags |= FBBCompressed;
				conn->DontSaveRestartData = FALSE;		// Allow restarts
				
				// Look for 1 or 2 or 12 as next 2 chars

				if (SID[len+2] == '1')
				{
					if (conn->UserPointer->ForwardingInfo->AllowB1 ||
						conn->UserPointer->ForwardingInfo->AllowB2)		// B2 implies B1
						conn->BBSFlags |= FBBB1Mode;

					if (SID[len+3] == '2')
						if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)

					break;
				}

				if (SID[len+2] == '2')
				{
					if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)
	
					if (conn->UserPointer->ForwardingInfo->AllowB1)
							conn->BBSFlags |= FBBB1Mode;				// B2 should allow fallback to B1 (but RMS doesnt!)

				}
				break;
			}

			break;
		}
	}

	// Only allow blocked non-binary to other BPQ Nodes

	if ((conn->BBSFlags & FBBForwarding) && ((conn->BBSFlags & FBBCompressed) == 0) && (conn->BPQBBS == 0))
	{
		// Switch back to MBL

		conn->BBSFlags |= MBLFORWARDING;
		conn->BBSFlags &= ~FBBForwarding;	// Turn off FBB Blocked
	}

	return;
}

VOID BBSSlowTimer()
{
	ConnectionInfo * conn;
	int n;

	// Called every 10 seconds

	MCastTimer();


	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
		{
			// Check for stuck BBS sessions (BBS session but no Node Session)

			int state;
				
			GetSemaphore(&ConSemaphore, 1);
			SessionStateNoAck(conn->BPQStream, &state);
			FreeSemaphore(&ConSemaphore);

			if (state == 0)		// No Node Session
			{
				// is it safe just to clear Active ??

				conn->InputMode = 0;		// So Disconnect wont save partial transfer	
				conn->BBSFlags = 0;
				Disconnected (conn->BPQStream);
				continue;
			}

			if (conn->BBSFlags & MCASTRX)
				MCastConTimer(conn);


			//	Check SIDTImers - used to detect failure to compete SID Handshake

			if (conn->SIDResponseTimer)
			{
				conn->SIDResponseTimer--;
				if (conn->SIDResponseTimer == 0)
				{
					// Disconnect Session

					Disconnect(conn->BPQStream);
				}
			}
		}
	}

	// Flush logs

	for (n = 0; n < 4; n++)
	{
		if (LogHandle[n])
		{
			time_t LT = time(NULL);
			if ((LT - LastLogTime[n]) > 30)
			{
				LastLogTime[n] = LT;
				fclose(LogHandle[n]);
				LogHandle[n] = NULL;
			}
		}
	}
}


VOID FWDTimerProc()
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;
	time_t NOW = time(NULL);

	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;
		ForwardingInfo->FwdTimer+=10;

		if (ForwardingInfo->FwdTimer >= ForwardingInfo->FwdInterval)
		{
			ForwardingInfo->FwdTimer=0;

			if (ForwardingInfo->FWDBands && ForwardingInfo->FWDBands[0])
			{
				// Check Timebands

				struct FWDBAND ** Bands = ForwardingInfo->FWDBands;
				int Count = 0;
				time_t now = time(NULL);
						
				if (Localtime)
					now -= (time_t)_MYTIMEZONE; 

				now %= 86400;		// Secs in day

				while(Bands[Count])
				{
					if ((Bands[Count]->FWDStartBand < now) && (Bands[Count]->FWDEndBand >= now))
						goto FWD;	// In band

				Count++;
				}
				continue;				// Out of bands
			}
		FWD:	
			if (ForwardingInfo->Enabled)
			{
				if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
				{
					//Temp Debug Code

//					Debugprintf("ReverseFlag = %d, Msgs to Forward Flag %d Msgs to Forward Count %d",
//						ForwardingInfo->ReverseFlag,
//						SeeifMessagestoForward(user->BBSNumber, NULL),
//						CountMessagestoForward(user));
						
					if (SeeifMessagestoForward(user->BBSNumber, NULL) ||
						(ForwardingInfo->ReverseFlag && ((NOW - ForwardingInfo->LastReverseForward) >= ForwardingInfo->RevFwdInterval)))

					{
						user->ForwardingInfo->ScriptIndex = -1;			 // Incremented before being used


						// remove any old TempScript

						if (user->ForwardingInfo->TempConnectScript)
						{
							FreeList(user->ForwardingInfo->TempConnectScript);
							user->ForwardingInfo->TempConnectScript = NULL;
						}

						if (ConnecttoBBS(user))
							ForwardingInfo->Forwarding = TRUE;					
					}
				}
			}
		}
	}
}

VOID * _zalloc_dbg(size_t len, int type, char * file, int line)
{
	// ?? malloc and clear

	void * ptr;

#ifdef WIN32
	ptr=_malloc_dbg(len, type, file, line);
#else
	ptr = malloc(len);
#endif
	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

struct MsgInfo * FindMessageByNumber(int msgno)
 {
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	do
	{
		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		if (Msg->number && Msg->number < msgno)		// sometimes get zero msg number
			return NULL;			// Not found

		m--;

	} while (m > 0);

	return NULL;
}

struct MsgInfo * FindMessageByBID(char * BID)
{
	int m = NumberofMessages;

	struct MsgInfo * Msg;

	while (m > 0)
	{
		Msg = MsgHddrPtr[m];

		if (strcmp(Msg->bid, BID) == 0)
			return Msg;

		m--;
	}

	return NULL;
}

VOID DecryptPass(char * Encrypt, unsigned char * Pass, unsigned int len)
{
	unsigned char hash[50];
	unsigned char key[100];
	unsigned int i, j = 0, val1, val2;
	unsigned char hostname[100]="";

	gethostname(hostname, 100);

	strcpy(key, hostname);
	strcat(key, ISPPOP3Name);

	md5(key, hash);
	memcpy(&hash[16], hash, 16);	// in case very long password

	// String is now encoded as hex pairs, but still need to decode old format

	for (i=0; i < len; i++)
	{
		if (Encrypt[i] < '0' || Encrypt[i] > 'F')
			goto OldFormat;
	}

	// Only '0' to 'F'

	for (i=0; i < len; i++)
	{
		val1 = Encrypt[i++];
		val1 -= '0';
		if (val1 > 9)
			val1 -= 7;

		val2 = Encrypt[i];
		val2 -= '0';
		if (val2 > 9)
			val2 -= 7;

		Pass[j] =  (val1 << 4) | val2;
		Pass[j] ^= hash[j];
		j++;
	}

	return;

OldFormat:

	for (i=0; i < len; i++)
	{
		Pass[i] =  Encrypt[i] ^ hash[i];
	}

	return;
}

int EncryptPass(char * Pass, char * Encrypt)
{
	unsigned char hash[50];
	unsigned char key[100];
	unsigned int i, val;
	unsigned char hostname[100];
	unsigned char extendedpass[100];
	unsigned int passlen;
	unsigned char * ptr;

	gethostname(hostname, 100);

	strcpy(key, hostname);
	strcat(key, ISPPOP3Name);

	md5(key, hash);
	memcpy(&hash[16], hash, 16);	// in case very long password

	// if password is less than 16 chars, extend with zeros

	passlen=(int)strlen(Pass);

	strcpy(extendedpass, Pass);

	if (passlen < 16)
	{
		for  (i=passlen+1; i <= 16; i++)
		{
			extendedpass[i] = 0;
		}

		passlen = 16;
	}

	ptr = Encrypt;
	Encrypt[0] = 0;

	for (i=0; i < passlen; i++)
	{
		val = extendedpass[i] ^ hash[i];
		ptr += sprintf(ptr, "%02X", val);
	}

	return passlen * 2;
}



VOID SaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}

VOID SaveInt64Value(config_setting_t * group, char * name, long long value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT64);
	if(setting)
		config_setting_set_int64(setting, value);
}

VOID SaveFloatValue(config_setting_t * group, char * name, double value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_FLOAT);
	if (setting)
		config_setting_set_float(setting, value);
}

VOID SaveStringValue(config_setting_t * group, char * name, char * value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, value);

}


VOID SaveOverride(config_setting_t * group, char * name, struct Override ** values)
{
	config_setting_t *setting;
	struct Override ** Calls;
	char Multi[10000];
	char * ptr = &Multi[1];

	*ptr = 0;

	if (values)
	{
		Calls = values;

		while(Calls[0])
		{
			ptr += sprintf(ptr, "%s, %d|", Calls[0]->Call, Calls[0]->Days);
			Calls++;
		}
		*(--ptr) = 0;
	}

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, &Multi[1]);

}


VOID SaveMultiStringValue(config_setting_t * group, char * name, char ** values)
{
	config_setting_t *setting;
	char ** Calls;
	char Multi[100000];
	char * ptr = &Multi[1];

	*ptr = 0;

	if (values)
	{
		Calls = values;

		while(Calls[0])
		{
			strcpy(ptr, Calls[0]);
			ptr += strlen(Calls[0]);
			*(ptr++) = '|';
			Calls++;
		}
		*(--ptr) = 0;
	}

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, &Multi[1]);

}

int configSaved = 0;

VOID SaveConfig(char * ConfigName)
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;
	config_setting_t *root, *group, *bbs;
	int i;
	char Size[80];
	struct BBSForwardingInfo DummyForwardingInfo;
	char Line[1024];

	if (configSaved == 0)
	{
		// only create backup once per run

		CopyConfigFile(ConfigName);
		configSaved = 1;
	}

	memset(&DummyForwardingInfo, 0, sizeof(struct BBSForwardingInfo));

	//	Get rid of old config before saving

	config_destroy(&cfg);

	memset((void *)&cfg, 0, sizeof(config_t));

	config_init(&cfg);

	root = config_root_setting(&cfg);

	group = config_setting_add(root, "main", CONFIG_TYPE_GROUP);

	SaveIntValue(group, "Streams", MaxStreams);
	SaveIntValue(group, "BBSApplNum", BBSApplNum);
	SaveStringValue(group, "BBSName", BBSName);
	SaveStringValue(group, "SYSOPCall", SYSOPCall);
	SaveStringValue(group, "H-Route", HRoute);
	SaveStringValue(group, "AMPRDomain", AMPRDomain);
	SaveIntValue(group, "EnableUI", EnableUI);
	SaveIntValue(group, "RefuseBulls", RefuseBulls);
	SaveIntValue(group, "OnlyKnown", OnlyKnown);
	SaveIntValue(group, "SendSYStoSYSOPCall", SendSYStoSYSOPCall);
	SaveIntValue(group, "SendBBStoSYSOPCall", SendBBStoSYSOPCall);
	SaveIntValue(group, "DontHoldNewUsers", DontHoldNewUsers);
	SaveIntValue(group, "DefaultNoWINLINK", DefaultNoWINLINK);
	SaveIntValue(group, "AllowAnon", AllowAnon);
	SaveIntValue(group, "DontNeedHomeBBS", DontNeedHomeBBS);
	SaveIntValue(group, "DontCheckFromCall", DontCheckFromCall);
	SaveIntValue(group, "UserCantKillT", UserCantKillT);

	SaveIntValue(group, "ForwardToMe", ForwardToMe);
	SaveIntValue(group, "SMTPPort", SMTPInPort);
	SaveIntValue(group, "POP3Port", POP3InPort);
	SaveIntValue(group, "NNTPPort", NNTPInPort);
	SaveIntValue(group, "RemoteEmail", RemoteEmail);
	SaveIntValue(group, "SendAMPRDirect", SendAMPRDirect);

	SaveIntValue(group, "MailForInterval", MailForInterval);
	SaveStringValue(group, "MailForText", MailForText);

	EncryptedPassLen = EncryptPass(ISPAccountPass, EncryptedISPAccountPass);

	SaveIntValue(group, "AuthenticateSMTP", SMTPAuthNeeded);

	SaveIntValue(group, "MulticastRX", MulticastRX);

	SaveIntValue(group, "SMTPGatewayEnabled", ISP_Gateway_Enabled);
	SaveIntValue(group, "ISPSMTPPort", ISPSMTPPort);
	SaveIntValue(group, "ISPPOP3Port", ISPPOP3Port);
	SaveIntValue(group, "POP3PollingInterval", ISPPOP3Interval);

	SaveStringValue(group, "MyDomain", MyDomain);
	SaveStringValue(group, "ISPSMTPName", ISPSMTPName);
	SaveStringValue(group, "ISPEHLOName", ISPEHLOName);
	SaveStringValue(group, "ISPPOP3Name", ISPPOP3Name);
	SaveStringValue(group, "ISPAccountName", ISPAccountName);
	SaveStringValue(group, "ISPAccountPass", EncryptedISPAccountPass);


	//	Save Window Sizes
	
#ifndef LINBPQ

	if (ConsoleRect.right)
	{
		sprintf(Size,"%d,%d,%d,%d",ConsoleRect.left, ConsoleRect.right,
			ConsoleRect.top, ConsoleRect.bottom);

		SaveStringValue(group, "ConsoleSize", Size);
	}
	
	sprintf(Size,"%d,%d,%d,%d,%d",MonitorRect.left,MonitorRect.right,MonitorRect.top,MonitorRect.bottom, hMonitor ? 1 : 0);
	SaveStringValue(group, "MonitorSize", Size);

	sprintf(Size,"%d,%d,%d,%d",MainRect.left,MainRect.right,MainRect.top,MainRect.bottom);
	SaveStringValue(group, "WindowSize", Size);

	SaveIntValue(group, "Bells", Bells);
	SaveIntValue(group, "FlashOnBell", FlashOnBell);
	SaveIntValue(group, "StripLF", StripLF);
	SaveIntValue(group, "WarnWrap", WarnWrap);
	SaveIntValue(group, "WrapInput", WrapInput);
	SaveIntValue(group, "FlashOnConnect", FlashOnConnect);
	SaveIntValue(group, "CloseWindowOnBye", CloseWindowOnBye);

#endif

	SaveIntValue(group, "Log_BBS", LogBBS);
	SaveIntValue(group, "Log_TCP", LogTCP);

	sprintf(Size,"%d,%d,%d,%d", Ver[0], Ver[1], Ver[2], Ver[3]);
	SaveStringValue(group, "Version", Size);

	// Save Welcome Messages and prompts

	SaveStringValue(group, "WelcomeMsg", WelcomeMsg);
	SaveStringValue(group, "NewUserWelcomeMsg", NewWelcomeMsg);
	SaveStringValue(group, "ExpertWelcomeMsg", ExpertWelcomeMsg);
	
	SaveStringValue(group, "Prompt", Prompt);
	SaveStringValue(group, "NewUserPrompt", NewPrompt);
	SaveStringValue(group, "ExpertPrompt", ExpertPrompt);
	SaveStringValue(group, "SignoffMsg", SignoffMsg);

	SaveMultiStringValue(group,  "RejFrom", RejFrom);
	SaveMultiStringValue(group,  "RejTo", RejTo);
	SaveMultiStringValue(group,  "RejAt", RejAt);
	SaveMultiStringValue(group,  "RejBID", RejBID);

	SaveMultiStringValue(group,  "HoldFrom", HoldFrom);
	SaveMultiStringValue(group,  "HoldTo", HoldTo);
	SaveMultiStringValue(group,  "HoldAt", HoldAt);
	SaveMultiStringValue(group,  "HoldBID", HoldBID);

	SaveIntValue(group, "SendWP", SendWP);
	SaveIntValue(group, "SendWPType", SendWPType);
	SaveIntValue(group, "FilterWPBulls", FilterWPBulls);
	SaveIntValue(group, "NoWPGuesses", NoWPGuesses);

	SaveStringValue(group, "SendWPTO", SendWPTO);
	SaveStringValue(group, "SendWPVIA", SendWPVIA);

	SaveMultiStringValue(group, "SendWPAddrs", SendWPAddrs); 

	// Save Forwarding Config

	// Interval and Max Sizes and Aliases are not user specific

	SaveIntValue(group, "MaxTXSize", MaxTXSize);
	SaveIntValue(group, "MaxRXSize", MaxRXSize);
	SaveIntValue(group, "ReaddressLocal", ReaddressLocal);
	SaveIntValue(group, "ReaddressReceived", ReaddressReceived);
	SaveIntValue(group, "WarnNoRoute", WarnNoRoute);
	SaveIntValue(group, "Localtime", Localtime);
	SaveIntValue(group, "SendPtoMultiple", SendPtoMultiple);

	SaveMultiStringValue(group, "FWDAliases", AliasText);

	bbs = config_setting_add(root, "BBSForwarding", CONFIG_TYPE_GROUP);

	for (i=1; i <= NumberofUsers; i++)
	{
		user = UserRecPtr[i];
		ForwardingInfo = user->ForwardingInfo;

		if (ForwardingInfo == NULL)
			continue;

		if (memcmp(ForwardingInfo, &DummyForwardingInfo, sizeof(struct BBSForwardingInfo)) == 0)
			continue;		// Ignore empty records;

		if (isdigit(user->Call[0]) || user->Call[0] == '_')
		{
			char Key[20] = "*";
			strcat (Key, user->Call); 
			group = config_setting_add(bbs, Key, CONFIG_TYPE_GROUP);
		}
		else
			group = config_setting_add(bbs, user->Call, CONFIG_TYPE_GROUP);

		SaveMultiStringValue(group, "TOCalls", ForwardingInfo->TOCalls);
		SaveMultiStringValue(group, "ConnectScript", ForwardingInfo->ConnectScript);
		SaveMultiStringValue(group, "ATCalls", ForwardingInfo->ATCalls);
		SaveMultiStringValue(group, "HRoutes", ForwardingInfo->Haddresses);
		SaveMultiStringValue(group, "HRoutesP", ForwardingInfo->HaddressesP);
		SaveMultiStringValue(group, "FWDTimes", ForwardingInfo->FWDTimes);
	
		SaveIntValue(group, "Enabled", ForwardingInfo->Enabled);
		SaveIntValue(group, "RequestReverse", ForwardingInfo->ReverseFlag);
		SaveIntValue(group, "AllowBlocked", ForwardingInfo->AllowBlocked);
		SaveIntValue(group, "AllowCompressed", ForwardingInfo->AllowCompressed);
		SaveIntValue(group, "UseB1Protocol", ForwardingInfo->AllowB1);
		SaveIntValue(group, "UseB2Protocol", ForwardingInfo->AllowB2);
		SaveIntValue(group, "SendCTRLZ", ForwardingInfo->SendCTRLZ);
				
		SaveIntValue(group, "FWDPersonalsOnly", ForwardingInfo->PersonalOnly);
		SaveIntValue(group, "FWDNewImmediately", ForwardingInfo->SendNew);
		SaveIntValue(group, "FwdInterval", ForwardingInfo->FwdInterval);
		SaveIntValue(group, "RevFWDInterval", ForwardingInfo->RevFwdInterval);
		SaveIntValue(group, "MaxFBBBlock", ForwardingInfo->MaxFBBBlockSize);
		SaveIntValue(group, "ConTimeout", ForwardingInfo->ConTimeout);

		SaveStringValue(group, "BBSHA", ForwardingInfo->BBSHA);
	}


	// Save Housekeeping config

	group = config_setting_add(root, "Housekeeping", CONFIG_TYPE_GROUP);

	SaveInt64Value(group, "LastHouseKeepingTime", LastHouseKeepingTime);
	SaveInt64Value(group, "LastTrafficTime", LastTrafficTime);
	SaveIntValue(group, "MaxMsgno", MaxMsgno);
	SaveIntValue(group, "BidLifetime", BidLifetime);
	SaveIntValue(group, "MaxAge", MaxAge);
	SaveIntValue(group, "LogLifetime", LogAge);
	SaveIntValue(group, "LogLifetime", LogAge);
	SaveIntValue(group, "MaintInterval", MaintInterval);
	SaveIntValue(group, "UserLifetime", UserLifetime);
	SaveIntValue(group, "MaintTime", MaintTime);
	SaveFloatValue(group, "PR", PR);
	SaveFloatValue(group, "PUR", PUR);
	SaveFloatValue(group, "PF", PF);
	SaveFloatValue(group, "PNF", PNF);
	SaveIntValue(group, "BF", BF);
	SaveIntValue(group, "BNF", BNF);
	SaveIntValue(group, "NTSD", NTSD);
	SaveIntValue(group, "NTSF", NTSF);
	SaveIntValue(group, "NTSU", NTSU);
//	SaveIntValue(group, "AP", AP);
//	SaveIntValue(group, "AB", AB);
	SaveIntValue(group, "DeletetoRecycleBin", DeletetoRecycleBin);
	SaveIntValue(group, "SuppressMaintEmail", SuppressMaintEmail);
	SaveIntValue(group, "MaintSaveReg", SaveRegDuringMaint);
	SaveIntValue(group, "OverrideUnsent", OverrideUnsent);
	SaveIntValue(group, "SendNonDeliveryMsgs", SendNonDeliveryMsgs);
	SaveIntValue(group, "GenerateTrafficReport", GenerateTrafficReport);

	SaveOverride(group, "LTFROM", LTFROM);
	SaveOverride(group, "LTTO", LTTO);
	SaveOverride(group, "LTAT", LTAT);

	// Save UI config

	for (i=1; i<=32; i++)
	{
		char Key[100];
			
		sprintf(Key, "UIPort%d", i);

		group = config_setting_add(root, Key, CONFIG_TYPE_GROUP);

		if (group)
		{
			SaveIntValue(group, "Enabled", UIEnabled[i]);
			SaveIntValue(group, "SendMF", UIMF[i]);
			SaveIntValue(group, "SendHDDR", UIHDDR[i]);
			SaveIntValue(group, "SendNull", UINull[i]);
	
			if (UIDigi[i])
				SaveStringValue(group, "Digis", UIDigi[i]);
		}
	}

	// Save User Config

	bbs = config_setting_add(root, "BBSUsers", CONFIG_TYPE_GROUP);

	for (i=1; i <= NumberofUsers; i++)
	{
		char stats[256], stats2[256];
		struct MsgStats * Stats;
		char Key[20] = "*";

		user = UserRecPtr[i];

		if (isdigit(user->Call[0]) || user->Call[0] == '_')
		{
			strcat (Key, user->Call); 
//			group = config_setting_add(bbs, Key, CONFIG_TYPE_GROUP);
		}
		else
		{
			strcpy(Key, user->Call);
//			group = config_setting_add(bbs, user->Call, CONFIG_TYPE_GROUP);
		}
		/*
		SaveStringValue(group, "Name", user->Name);
		SaveStringValue(group, "Address", user->Address);
		SaveStringValue(group, "HomeBBS", user->HomeBBS);
		SaveStringValue(group, "QRA", user->QRA);
		SaveStringValue(group, "pass", user->pass);
		SaveStringValue(group, "ZIP", user->ZIP);
		SaveStringValue(group, "CMSPass", user->CMSPass);

		SaveIntValue(group, "lastmsg", user->lastmsg);
		SaveIntValue(group, "flags", user->flags);
		SaveIntValue(group, "PageLen", user->PageLen);
		SaveIntValue(group, "BBSNumber", user->BBSNumber);
		SaveIntValue(group, "RMSSSIDBits", user->RMSSSIDBits);
		SaveIntValue(group, "WebSeqNo", user->WebSeqNo);
		
		SaveInt64Value(group, "TimeLastConnected", user->TimeLastConnected);
*/
		Stats = &user->Total;

//		sprintf(stats, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		sprintf(stats, "%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d",
			Stats->ConnectsIn, Stats->ConnectsOut,
			Stats->MsgsReceived[0], Stats->MsgsReceived[1], Stats->MsgsReceived[2], Stats->MsgsReceived[3], 
			Stats->MsgsSent[0], Stats->MsgsSent[1], Stats->MsgsSent[2], Stats->MsgsSent[3], 
			Stats->MsgsRejectedIn[0], Stats->MsgsRejectedIn[1], Stats->MsgsRejectedIn[2], Stats->MsgsRejectedIn[3], 
			Stats->MsgsRejectedOut[0], Stats->MsgsRejectedOut[1], Stats->MsgsRejectedOut[2], Stats->MsgsRejectedOut[3], 
			Stats->BytesForwardedIn[0], Stats->BytesForwardedIn[1], Stats->BytesForwardedIn[2], Stats->BytesForwardedIn[3], 
			Stats->BytesForwardedOut[0], Stats->BytesForwardedOut[1], Stats->BytesForwardedOut[2], Stats->BytesForwardedOut[3]);

//		SaveStringValue(group, "Totsl", stats);

		Stats = &user->Last;

		sprintf(stats2, "%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d,%.0d",
//		sprintf(stats2, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			Stats->ConnectsIn, Stats->ConnectsOut,
			Stats->MsgsReceived[0], Stats->MsgsReceived[1], Stats->MsgsReceived[2], Stats->MsgsReceived[3], 
			Stats->MsgsSent[0], Stats->MsgsSent[1], Stats->MsgsSent[2], Stats->MsgsSent[3], 
			Stats->MsgsRejectedIn[0], Stats->MsgsRejectedIn[1], Stats->MsgsRejectedIn[2], Stats->MsgsRejectedIn[3], 
			Stats->MsgsRejectedOut[0], Stats->MsgsRejectedOut[1], Stats->MsgsRejectedOut[2], Stats->MsgsRejectedOut[3], 
			Stats->BytesForwardedIn[0], Stats->BytesForwardedIn[1], Stats->BytesForwardedIn[2], Stats->BytesForwardedIn[3], 
			Stats->BytesForwardedOut[0], Stats->BytesForwardedOut[1], Stats->BytesForwardedOut[2], Stats->BytesForwardedOut[3]);

//		SaveStringValue(group, "Last", stats2);

		sprintf(Line,"%s^%s^%s^%s^%s^%s^%s^%d^%d^%d^%d^%d^%d^%lld^%s^%s",
			user->Name, user->Address, user->HomeBBS, user->QRA, user->pass, user->ZIP, user->CMSPass,
			user->lastmsg, user->flags, user->PageLen, user->BBSNumber, user->RMSSSIDBits, user->WebSeqNo,
			user->TimeLastConnected, stats, stats2);

		if (strlen(Line) < 10)
			continue;

		SaveStringValue(bbs, Key, Line);
	}

/*
	wp = config_setting_add(root, "WP", CONFIG_TYPE_GROUP);

	for (i = 0; i <= NumberofWPrecs; i++)
	{
		char WPString[1024];
		long long val1, val2;

		WP = WPRecPtr[i];
		val1 = WP->last_modif;
		val2 = WP->last_seen;

		sprintf(Key, "R%d", i);

		sprintf(WPString, "%s|%s|%d|%d|%d|%s|%s|%s|%s|%s|%s|%ld|%ld",
			&WP->callsign[0], &WP->name[0], WP->Type, WP->changed, WP->seen, &WP->first_homebbs[0],
			&WP->secnd_homebbs[0], &WP->first_zip[0], &WP->secnd_zip[0], &WP->first_qth[0], &WP->secnd_qth[0],
			val1, val2);

		SaveStringValue(wp, Key, WPString);
	}

	// Save Message Headers 

	msgs = config_setting_add(root, "MSGS", CONFIG_TYPE_GROUP);

	memset(MsgHddrPtr[0], 0, sizeof(struct MsgInfo));

	MsgHddrPtr[0]->type = 'X';
	MsgHddrPtr[0]->status = '2';
	MsgHddrPtr[0]->number = 0;
	MsgHddrPtr[0]->length = LatestMsg;


	for (i = 0; i <= NumberofMessages; i++)
	{
		Msg = MsgHddrPtr[i];

		for (n = 0; n < NBMASK; n++)
			sprintf(&HEXString1[n * 2], "%02X", Msg->fbbs[n]);

		n = 39;
		while (n >=0 && HEXString1[n] == '0')
			HEXString1[n--] = 0;

		for (n = 0; n < NBMASK; n++)
			sprintf(&HEXString2[n * 2], "%02X", Msg->forw[n]);

		n = 39;
		while (n >= 0 && HEXString2[n] == '0')
			HEXString2[n--] = 0;
		
		sprintf(Key, "R%d", Msg->number);

		n = sprintf(Line, "%c|%c|%d|%lld|%s|%s|%s|%s|%s|%d|%lld|%lld|%s|%s|%s|%d|%s", Msg->type, Msg->status,
		Msg->length, Msg->datereceived, &Msg->bbsfrom[0], &Msg->via[0], &Msg->from[0],
		&Msg->to[0], &Msg->bid[0], Msg->B2Flags, Msg->datecreated, Msg->datechanged, HEXString1, HEXString2, 
		&Msg->emailfrom[0], Msg->UTF8, &Msg->title[0]);

		SaveStringValue(msgs, Key, Line);
	}

	// Save Bids  

	msgs = config_setting_add(root, "BIDS", CONFIG_TYPE_GROUP);

	for (i=1; i <= NumberofBIDs; i++)
	{
		sprintf(Key, "R%s", BIDRecPtr[i]->BID);
		sprintf(Line, "%d|%d", BIDRecPtr[i]->mode, BIDRecPtr[i]->u.timestamp);
		SaveStringValue(msgs, Key, Line);
	}

#ifdef LINBPQ

	if(! config_write_file(&cfg,"/dev/shm/linmail.cfg.temp" ))
	{
		print("Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}

	CopyFile("/dev/shm/linmail.cfg.temp", ConfigName, FALSE);

#else
*/
	if(! config_write_file(&cfg, ConfigName))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}

//#endif

	config_destroy(&cfg);

/*

#ifndef LINBPQ

	//	Save a copy with current Date/Time Stamp for debugging

	{
		char Backup[MAX_PATH];
		time_t LT;
		struct tm * tm;

		LT = time(NULL);
		tm = gmtime(&LT);	

		sprintf(Backup,"%s.%02d%02d%02d%02d%02d.save", ConfigName, tm->tm_year-100, tm->tm_mon+1,
			tm->tm_mday, tm->tm_hour, tm->tm_min);

		CopyFile(ConfigName, Backup, FALSE);	// Copy to .bak
	}
#endif
*/
}

int GetIntValue(config_setting_t * group, char * name)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return 0;
}

long long GetInt64Value(config_setting_t * group, char * name)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int64 (setting);

	return 0;
}

double GetFloatValue(config_setting_t * group, char * name)
{
	config_setting_t *setting;
			
	setting = config_setting_get_member (group, name);

	if (setting)
	{
		return config_setting_get_float (setting);
	}
	return 0;
}

int GetIntValueWithDefault(config_setting_t * group, char * name, int Default)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return Default;
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

BOOL GetConfig(char * ConfigName)
{
	int i;
	char Size[80];
	config_setting_t *setting;
	const char * ptr;

	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	
	if(! config_read_file(&cfg, ConfigName))
	{
		char Msg[256];
		sprintf(Msg, "Config File Line %d - %s\n",
			config_error_line(&cfg), config_error_text(&cfg));
#ifdef WIN32
		MessageBox(NULL, Msg, "BPQMail", MB_ICONSTOP);
#else
		printf("%s", Msg);
#endif
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

#if IBCONFIG_VER_MINOR > 4
	config_set_option(&cfg, CONFIG_OPTION_AUTOCONVERT, 1);
#else
	config_set_auto_convert (&cfg, 1);
#endif

	group = config_lookup (&cfg, "main");

	if (group == NULL)
		return EXIT_FAILURE;

	SMTPInPort =  GetIntValue(group, "SMTPPort");
	POP3InPort =  GetIntValue(group, "POP3Port");
	NNTPInPort =  GetIntValue(group, "NNTPPort");
	RemoteEmail =  GetIntValue(group, "RemoteEmail");
	MaxStreams =  GetIntValue(group, "Streams");
	BBSApplNum =  GetIntValue(group, "BBSApplNum");
	EnableUI =  GetIntValue(group, "EnableUI");
	MailForInterval =  GetIntValue(group, "MailForInterval");
	RefuseBulls =  GetIntValue(group, "RefuseBulls");
	OnlyKnown =  GetIntValue(group, "OnlyKnown");
	SendSYStoSYSOPCall =  GetIntValue(group, "SendSYStoSYSOPCall");
	SendBBStoSYSOPCall =  GetIntValue(group, "SendBBStoSYSOPCall");
	DontHoldNewUsers =  GetIntValue(group, "DontHoldNewUsers");
	DefaultNoWINLINK =  GetIntValue(group, "DefaultNoWINLINK");
	ForwardToMe =  GetIntValue(group, "ForwardToMe");
	AllowAnon =  GetIntValue(group, "AllowAnon");
	UserCantKillT = GetIntValue(group, "UserCantKillT");

	DontNeedHomeBBS =  GetIntValue(group, "DontNeedHomeBBS");
	DontCheckFromCall =  GetIntValue(group, "DontCheckFromCall");
	MaxTXSize =  GetIntValue(group, "MaxTXSize");
	MaxRXSize =  GetIntValue(group, "MaxRXSize");
	ReaddressLocal =  GetIntValue(group, "ReaddressLocal");
	ReaddressReceived =  GetIntValue(group, "ReaddressReceived");
	WarnNoRoute =  GetIntValue(group, "WarnNoRoute");
	SendPtoMultiple =  GetIntValue(group, "SendPtoMultiple");
	Localtime =  GetIntValue(group, "Localtime");
	AliasText = GetMultiStringValue(group, "FWDAliases");
	GetStringValue(group, "BBSName", BBSName);
	GetStringValue(group, "MailForText", MailForText);
	GetStringValue(group, "SYSOPCall", SYSOPCall);
	GetStringValue(group, "H-Route", HRoute);
	GetStringValue(group, "AMPRDomain", AMPRDomain);
	SendAMPRDirect = GetIntValue(group, "SendAMPRDirect");
	ISP_Gateway_Enabled =  GetIntValue(group, "SMTPGatewayEnabled");
	ISPPOP3Interval =  GetIntValue(group, "POP3PollingInterval");
	GetStringValue(group, "MyDomain", MyDomain);
	GetStringValue(group, "ISPSMTPName", ISPSMTPName);
	GetStringValue(group, "ISPPOP3Name", ISPPOP3Name);
	ISPSMTPPort = GetIntValue(group, "ISPSMTPPort");
	ISPPOP3Port = GetIntValue(group, "ISPPOP3Port");
	GetStringValue(group, "ISPAccountName", ISPAccountName);
	GetStringValue(group, "ISPAccountPass", EncryptedISPAccountPass);
	GetStringValue(group, "ISPAccountName", ISPAccountName);

	sprintf(SignoffMsg, "73 de %s\r", BBSName);					// Default
	GetStringValue(group, "SignoffMsg", SignoffMsg);

	DecryptPass(EncryptedISPAccountPass, ISPAccountPass, (int)strlen(EncryptedISPAccountPass));

	SMTPAuthNeeded = GetIntValue(group, "AuthenticateSMTP");
	LogBBS = GetIntValue(group, "Log_BBS");
	LogTCP = GetIntValue(group, "Log_TCP");

	MulticastRX = GetIntValue(group, "MulticastRX");

#ifndef LINBPQ

	GetStringValue(group, "MonitorSize", Size);
	sscanf(Size,"%d,%d,%d,%d,%d",&MonitorRect.left,&MonitorRect.right,&MonitorRect.top,&MonitorRect.bottom,&OpenMon);
	
	GetStringValue(group, "WindowSize", Size);
	sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);

	Bells = GetIntValue(group, "Bells");

	FlashOnBell = GetIntValue(group, "FlashOnBell");			

	StripLF	 = GetIntValue(group, "StripLF");	
	CloseWindowOnBye = GetIntValue(group, "CloseWindowOnBye");			
	WarnWrap = GetIntValue(group, "WarnWrap");
	WrapInput = GetIntValue(group, "WrapInput");			
	FlashOnConnect = GetIntValue(group, "FlashOnConnect");			
	
	GetStringValue(group, "ConsoleSize", Size);
	sscanf(Size,"%d,%d,%d,%d,%d", &ConsoleRect.left, &ConsoleRect.right,
			&ConsoleRect.top, &ConsoleRect.bottom,&OpenConsole);

#endif

	// Get Welcome Messages

	setting = config_setting_get_member (group, "WelcomeMsg");

	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		WelcomeMsg = _strdup(ptr);
	}
	else
		WelcomeMsg = _strdup("Hello $I. Latest Message is $L, Last listed is $Z\r\n");


	setting = config_setting_get_member (group, "NewUserWelcomeMsg");
	
	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		NewWelcomeMsg = _strdup(ptr);
	}
	else
		NewWelcomeMsg = _strdup("Hello $I. Latest Message is $L, Last listed is $Z\r\n");


	setting = config_setting_get_member (group, "ExpertWelcomeMsg");
	
	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		ExpertWelcomeMsg = _strdup(ptr);
	}
	else
		ExpertWelcomeMsg = _strdup("");

	// Get Prompts

	setting = config_setting_get_member (group, "Prompt");
	
	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		Prompt = _strdup(ptr);
	}
	else
	{
		Prompt = malloc(20);
		sprintf(Prompt, "de %s>\r\n", BBSName);
	}

	setting = config_setting_get_member (group, "NewUserPrompt");
	
	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		NewPrompt = _strdup(ptr);
	}
	else
	{
		NewPrompt = malloc(20);
		sprintf(NewPrompt, "de %s>\r\n", BBSName);
	}

	setting = config_setting_get_member (group, "ExpertPrompt");
	
	if (setting && setting->value.sval[0])
	{
		ptr =  config_setting_get_string (setting);
		ExpertPrompt = _strdup(ptr);
	}
	else
	{
		ExpertPrompt = malloc(20);
		sprintf(ExpertPrompt, "de %s>\r\n", BBSName);
	}

	TidyPrompts();

	RejFrom = GetMultiStringValue(group,  "RejFrom");
	RejTo = GetMultiStringValue(group,  "RejTo");
	RejAt = GetMultiStringValue(group,  "RejAt");
	RejBID = GetMultiStringValue(group,  "RejBID");

	HoldFrom = GetMultiStringValue(group,  "HoldFrom");
	HoldTo = GetMultiStringValue(group,  "HoldTo");
	HoldAt = GetMultiStringValue(group,  "HoldAt");
	HoldBID = GetMultiStringValue(group,  "HoldBID");

	// Send WP Params
	
	SendWP = GetIntValue(group, "SendWP");
	SendWPType = GetIntValue(group, "SendWPType");

	GetStringValue(group, "SendWPTO", SendWPTO);
	GetStringValue(group, "SendWPVIA", SendWPVIA);

	SendWPAddrs = GetMultiStringValue(group,  "SendWPAddrs"); 

	FilterWPBulls = GetIntValue(group, "FilterWPBulls");
	NoWPGuesses = GetIntValue(group, "NoWPGuesses");

	if (SendWPAddrs[0] == NULL && SendWPTO[0])
	{
		// convert old format TO and VIA to entry in SendWPAddrs
	
		SendWPAddrs = realloc(SendWPAddrs, 8);		// Add entry

		if (SendWPVIA[0])
		{
			char WP[256];
	
			sprintf(WP, "%s@%s", SendWPTO, SendWPVIA);
			SendWPAddrs[0] = _strdup(WP);
		}
		else
			SendWPAddrs[0] = _strdup(SendWPTO);


		SendWPAddrs[1] = 0;

		SendWPTO[0] = 0;
		SendWPVIA[0] = 0;
	}

	GetStringValue(group, "Version", Size);
	sscanf(Size,"%d,%d,%d,%d", &LastVer[0], &LastVer[1], &LastVer[2], &LastVer[3]);

	for (i=1; i<=32; i++)
	{
		char Key[100];
			
		sprintf(Key, "UIPort%d", i);

		group = config_lookup (&cfg, Key);

		if (group)
		{
			UIEnabled[i] = GetIntValue(group, "Enabled");
			UIMF[i] = GetIntValueWithDefault(group, "SendMF", UIEnabled[i]);
			UIHDDR[i] = GetIntValueWithDefault(group, "SendHDDR", UIEnabled[i]);
			UINull[i] = GetIntValue(group, "SendNull");
			Size[0] = 0;
			GetStringValue(group, "Digis", Size);
			if (Size[0])
				UIDigi[i] = _strdup(Size);
		}
	}

	 group = config_lookup (&cfg, "Housekeeping");

	 if (group)
	 {
		 LastHouseKeepingTime = GetIntValue(group, "LastHouseKeepingTime");
		 LastTrafficTime = GetIntValue(group, "LastTrafficTime");
		 MaxMsgno = GetIntValue(group, "MaxMsgno");
		 LogAge = GetIntValue(group, "LogLifetime");
		 BidLifetime = GetIntValue(group, "BidLifetime");
		 MaxAge = GetIntValue(group, "MaxAge");
		 if (MaxAge == 0)
			 MaxAge = 30;
		 UserLifetime = GetIntValue(group, "UserLifetime");
		 MaintInterval = GetIntValue(group, "MaintInterval");

		 if (MaintInterval == 0)
			 MaintInterval = 24;

		 MaintTime = GetIntValue(group, "MaintTime");
	
		 PR = GetFloatValue(group, "PR");
		 PUR = GetFloatValue(group, "PUR");
		 PF = GetFloatValue(group, "PF");
		 PNF = GetFloatValue(group, "PNF");

		 BF = GetIntValue(group, "BF");
		 BNF = GetIntValue(group, "BNF");
		 NTSD = GetIntValue(group, "NTSD");
		 NTSU = GetIntValue(group, "NTSU");
		 NTSF = GetIntValue(group, "NTSF");
//		 AP = GetIntValue(group, "AP");
//		 AB = GetIntValue(group, "AB");
		 DeletetoRecycleBin = GetIntValue(group, "DeletetoRecycleBin");
		 SuppressMaintEmail = GetIntValue(group, "SuppressMaintEmail");
		 SaveRegDuringMaint = GetIntValue(group, "MaintSaveReg");
		 OverrideUnsent = GetIntValue(group, "OverrideUnsent");
		 SendNonDeliveryMsgs = GetIntValue(group, "SendNonDeliveryMsgs");
		 OverrideUnsent = GetIntValue(group, "OverrideUnsent");
		 GenerateTrafficReport = GetIntValueWithDefault(group, "GenerateTrafficReport", 1);

		 LTFROM = GetOverrides(group,  "LTFROM");
		 LTTO = GetOverrides(group,  "LTTO");
		 LTAT = GetOverrides(group,  "LTAT");
	}

	return EXIT_SUCCESS;
}


int Connected(int Stream)
{
	int n, Mask;
	CIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, paclen, maxframe, l4window;
	char ConnectedMsg[] = "*** CONNECTED    ";
	char Msg[100];
	char Title[100];
	int Freq = 0;
	int Mode = 0;
	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * Sess1 = NULL, * Sess2;	

	for (n = 0; n < NumberofStreams; n++)
	{
  		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect
		
				ChangeSessionIdletime(Stream, USERIDLETIME);		// Default Idletime for BBS Sessions
				conn->SendB = conn->SendP = conn->SendT = conn->DoReverse = TRUE;
				conn->MaxBLen = conn->MaxPLen = conn->MaxTLen = 99999999;
				conn->ErrorCount = 0;

				if (conn->BBSFlags & RunningConnectScript)
				{
					// BBS Outgoing Connect

					conn->paclen = 236;

					// Run first line of connect script

					ChangeSessionIdletime(Stream, BBSIDLETIME);		// Default Idletime for BBS Sessions
					ProcessBBSConnectScript(conn, ConnectedMsg, 15);
					return 0;
				}
			}

			// Incoming Connect

			// Try to find port, freq, mode, etc

#ifdef LINBPQ
			SESS = &BPQHOSTVECTOR[0];
#else
			SESS = (BPQVECSTRUC *)BPQHOSTVECPTR;
#endif
			SESS +=(Stream - 1);

			if (SESS)
				Sess1 = SESS->HOSTSESSION;

			if (Sess1)
			{
				Sess2 = Sess1->L4CROSSLINK;

				if (Sess2)
				{
					// See if L2 session - if so, get info from WL2K report line

					// if Session has report info, use it

					if (Sess2->Mode)
					{
						Freq = Sess2->Frequency;
						Mode = Sess2->Mode;
					}
					else if (Sess2->L4CIRCUITTYPE & L2LINK)	
					{
						LINKTABLE * LINK = Sess2->L4TARGET.LINK;
						PORTCONTROLX * PORT = LINK->LINKPORT;

						Freq = PORT->WL2KInfo.Freq;
						Mode = PORT->WL2KInfo.mode;
					}
					else
					{
						if (Sess2->RMSCall[0])
						{
							Freq = Sess2->Frequency;
							Mode = Sess2->Mode;
						}
					}
				}
			}
	
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;
			ChangeSessionIdletime(Stream, USERIDLETIME);	// Default Idletime for BBS Sessions

			conn->SendB = conn->SendP = conn->SendT = conn->DoReverse = TRUE;
			conn->MaxBLen = conn->MaxPLen = conn->MaxTLen = 99999999;
			conn->ErrorCount = 0;

			conn->Secure_Session = GetConnectionInfo(Stream, callsign,
				&port, &conn->SessType, &paclen, &maxframe, &l4window);

			strlop(callsign, ' ');		// Remove trailing spaces

			if (strcmp(&callsign[strlen(callsign) - 2], "-T") == 0)
				conn->RadioOnlyMode = 'T';
			else if (strcmp(&callsign[strlen(callsign) - 2], "-R") == 0)
				conn->RadioOnlyMode = 'R';
			else
				conn->RadioOnlyMode = 0;

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = LookupCall(callsign);

			if (user == NULL)
			{
				int Length=0;

				if (OnlyKnown)
				{
					// Unknown users not allowed

					n = sprintf_s(Msg, sizeof(Msg), "Incoming Connect from unknown user %s Rejected", callsign);
					WriteLogLine(conn, '|',Msg, n, LOG_BBS);

					Disconnect(Stream);
					return 0;
				}

				user = AllocateUserRecord(callsign);
				user->Temp = zalloc(sizeof (struct TempUserInfo));

				if (SendNewUserMessage)
				{
					char * MailBuffer = malloc(100);
					Length += sprintf(MailBuffer, "New User %s Connected to Mailbox on Port %d Freq %d Mode %d\r\n", callsign, port, Freq, Mode);

					sprintf(Title, "New User %s", callsign);

					SendMessageToSYSOP(Title, MailBuffer, Length);
				}

				if (user == NULL) return 0; //		Cant happen??

				if (!DontHoldNewUsers)
					user->flags |= F_HOLDMAIL;

				if (DefaultNoWINLINK)
					user->flags |= F_NOWINLINK;

				// Always set WLE User - can't see it doing any harm

				user->flags |= F_Temp_B2_BBS;

				conn->NewUser = TRUE;
			}

			user->TimeLastConnected = time(NULL);
			user->Total.ConnectsIn++;

			conn->UserPointer = user;

			conn->lastmsg = user->lastmsg;

			conn->NextMessagetoForward = FirstMessageIndextoForward;

			if (paclen == 0)
			{
				paclen = 236;
	
				if (conn->SessType & Sess_PACTOR)
					paclen = 100;
			}

			conn->paclen = paclen;

			//	Set SYSOP flag if user is defined as SYSOP and Host Session 
			
			if (((conn->SessType & Sess_BPQHOST) == Sess_BPQHOST) && (user->flags & F_SYSOP))
				conn->sysop = TRUE;

			if (conn->Secure_Session && (user->flags & F_SYSOP))
				conn->sysop = TRUE;

			Mask = 1 << (GetApplNum(Stream) - 1);

			if (user->flags & F_Excluded)
			{
				n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s Rejected by Exclude Flag", user->Call);
				WriteLogLine(conn, '|',Msg, n, LOG_BBS);
				Disconnect(Stream);
				return 0;
			}

			if (port)
				n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s on Port %d Freq %d Mode %s",
					user->Call,  port, Freq, WL2KModes[Mode]);
			else
				n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt (Unless Sync)

			if (user->ForwardingInfo && user->ForwardingInfo->ConTimeout)
				conn->SIDResponseTimer = user->ForwardingInfo->ConTimeout / 10;			// 10 sec ticks
			else
				conn->SIDResponseTimer =  12;				// Allow a couple of minutes for response to SID

			{
				BOOL B1 = FALSE, B2 = FALSE, BIN = FALSE, BLOCKED = FALSE;
				BOOL WL2KRO = FALSE;

				struct	BBSForwardingInfo * ForwardingInfo;

				if (conn->RadioOnlyMode == 'R')
					WL2KRO = 1;

				conn->PageLen = user->PageLen;
				conn->Paging = (user->PageLen > 0);

				if (user->flags & F_Temp_B2_BBS)
				{
					// An RMS Express user that needs a temporary BBS struct

					if (user->ForwardingInfo == NULL)
					{
						// we now save the Forwarding info if BBS flag is cleared,
						// so there may already be a ForwardingInfo

						user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));
					}

					if (user->BBSNumber == 0)
						user->BBSNumber = NBBBS;

					ForwardingInfo = user->ForwardingInfo;

					ForwardingInfo->AllowCompressed = TRUE;
					B1 = ForwardingInfo->AllowB1 = FALSE;
					B2 = ForwardingInfo->AllowB2 = TRUE;
					BLOCKED = ForwardingInfo->AllowBlocked = TRUE;
				}

				if (conn->NewUser)
				{
					BLOCKED = TRUE;
					BIN = TRUE;
					B2 = TRUE;
				}

				if (user->ForwardingInfo)
				{
					BLOCKED = user->ForwardingInfo->AllowBlocked;
					if (BLOCKED)
					{
						BIN = user->ForwardingInfo->AllowCompressed;
						B1 = user->ForwardingInfo->AllowB1;
						B2 = user->ForwardingInfo->AllowB2;
					}
				}

				WriteLogLine(conn, '|',Msg, n, LOG_BBS);

				if (conn->RadioOnlyMode)
					nodeprintf(conn,";WL2K-Radio/Internet_Network\r");

				if (!(conn->BBSFlags & SYNCMODE))
				{

					nodeprintf(conn, BBSSID, "BPQ-",
						Ver[0], Ver[1], Ver[2], Ver[3],
						BIN ? "B" : "", B1 ? "1" : "", B2 ? "2" : "",
						BLOCKED ? "FW": "", WL2KRO ? "" : "J");

					//				 if (user->flags & F_Temp_B2_BBS)
					//					 nodeprintf(conn,";PQ: 66427529\r");

					//			nodeprintf(conn,"[WL2K-BPQ.1.0.4.39-B2FWIHJM$]\r");
				}
			}

			if ((user->Name[0] == 0) & AllowAnon)
				strcpy(user->Name, user->Call);

			if (!(conn->BBSFlags & SYNCMODE))
			{
				if (user->Name[0] == 0)
				{
					conn->Flags |= GETTINGUSER;
					BBSputs(conn, NewUserPrompt);
				}
				else
					SendWelcomeMsg(Stream, conn, user);
			}
			else
			{
				// Seems to be a timing problem - see if this fixes it

				Sleep(500);
			}

			RefreshMainWindow();

			return 0;
		}
	}

	return 0;
}

int Disconnected (int Stream)
{
	struct UserInfo * user = NULL;
	CIRCUIT * conn;
	int n;
	char Msg[255];
	int len;
	char DiscMsg[] = "DISCONNECTED    ";

	for (n = 0; n <= NumberofStreams-1; n++)
	{
		conn=&Connections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
				return 0;

			// if still running connect script, reenter it to see if
			// there is an else

			if (conn->BBSFlags & RunningConnectScript)
			{
				// We need to see if we got as far as connnected,
				// as if we have we need to reset the connect script 
				// over the ELSE

				struct	BBSForwardingInfo * ForwardingInfo = conn->UserPointer->ForwardingInfo;
				char ** Scripts;

				if (ForwardingInfo->TempConnectScript)
					Scripts = ForwardingInfo->TempConnectScript;
				else
					Scripts = ForwardingInfo->ConnectScript;	

				// First see if any script left

				if (Scripts[ForwardingInfo->ScriptIndex])
				{
					if (ForwardingInfo->MoreLines == FALSE)
					{
						// Have reached end of script, so need to set back over ELSE
					
						ForwardingInfo->ScriptIndex--;
						ForwardingInfo->MoreLines = TRUE;
					}
					
			//	if (Scripts[ForwardingInfo->ScriptIndex] == NULL ||
			//	_memicmp(Scripts[ForwardingInfo->ScriptIndex], "TIMES", 5) == 0	||		// Only Check until script is finished
			//	_memicmp(Scripts[ForwardingInfo->ScriptIndex], "ELSE", 4) == 0)			// Only Check until script is finished
			
				
					ProcessBBSConnectScript(conn, DiscMsg, 15);
					return 0;
				}
			}

			// if sysop was chatting to user clear link
#ifndef LINBPQ
			if (conn->BBSFlags & SYSOPCHAT)
			{
				SendUnbuffered(-1, "User has disconnected\n", 23);
				BBSConsole.Console->SysopChatStream = 0;
			}
#endif
			ClearQueue(conn);

			if (conn->PacLinkCalls)
				free(conn->PacLinkCalls);

			if (conn->InputBuffer)
			{
				free(conn->InputBuffer);
				conn->InputBuffer = NULL;
				conn->InputBufferLen = 0;
			}

			if (conn->InputMode == 'B')
			{
				// Save partly received message for a restart
						
				if (conn->BBSFlags & FBBB1Mode)
					if (conn->Paclink == 0)			// Paclink doesn't do restarts
						if (strcmp(conn->Callsign, "RMS") != 0)	// Neither does RMS Packet.
							if (conn->DontSaveRestartData == FALSE)
								SaveFBBBinary(conn);		
			}

			conn->Active = FALSE;

			if (conn->FwdMsg)
				conn->FwdMsg->Locked = 0;	// Unlock

			RefreshMainWindow();

			RemoveTempBIDS(conn);

			len=sprintf_s(Msg, sizeof(Msg), "%s Disconnected", conn->Callsign);
			WriteLogLine(conn, '|',Msg, len, LOG_BBS);

			if (conn->FBBHeaders)
			{
				struct FBBHeaderLine * FBBHeader;
				int n;

				for (n = 0; n < 5; n++)
				{
					FBBHeader = &conn->FBBHeaders[n];
					
					if (FBBHeader->FwdMsg)
						FBBHeader->FwdMsg->Locked = 0;	// Unlock

				}

				free(conn->FBBHeaders);
				conn->FBBHeaders = NULL;
			}

			if (conn->UserPointer)
			{
				struct	BBSForwardingInfo * FWDInfo = conn->UserPointer->ForwardingInfo;

				if (FWDInfo)
				{
					FWDInfo->Forwarding = FALSE;

//					if (FWDInfo->UserCall[0])			// Will be set if RMS
//					{
//						FindNextRMSUser(FWDInfo);
//					}
//					else
						FWDInfo->FwdTimer = 0;
				}
			}
			
			conn->BBSFlags = 0;				// Clear ARQ Mode

			return 0;
		}
	}
	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	size_t MsgLen;
	int n;
	CIRCUIT * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char * Buffer;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			conn->SIDResponseTimer = 0;		// Got a message, so cancel timeout.

			do
			{ 
				// May have several messages per packet, or message split over packets

			OuterLoop:
				if (conn->InputLen + 1000 > conn->InputBufferLen )	// Shouldnt have lines longer  than this in text mode
				{
					conn->InputBufferLen += 1000;
					conn->InputBuffer = realloc(conn->InputBuffer, conn->InputBufferLen);
				}
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0 && conn->InputMode != 'Y')
					return 0;

				conn->InputLen += InputLen;

				if (conn->InputLen == 0) return 0;

				conn->Watchdog = 900;				// 15 Minutes

				if (conn->InputMode == 'Y')			// YAPP
				{
					if (ProcessYAPPMessage(conn))	// Returns TRUE if there could be more to process
						goto OuterLoop;

					return 0;
				}

				if (conn->InputMode == 'B')
				{
					// if in OpenBCM mode, remove FF transparency

					if (conn->OpenBCM)			// Telnet, so escape any 0xFF
					{
						unsigned char * ptr1 = conn->InputBuffer;
						unsigned char * ptr2;
						int Len;
						unsigned char c;

						// We can come through here again for the
						// same data as we wait for a full packet
						// So only check last InputLen bytes

						ptr1 += (conn->InputLen - InputLen);
						ptr2 = ptr1;
						Len = InputLen;

						while (Len--)
						{
							c = *(ptr1++);

							if (conn->InTelnetExcape)	// Last char was ff
							{
								conn->InTelnetExcape = FALSE;
								continue;
							}

							*(ptr2++) = c;

							if (c == 0xff)		// 
								conn->InTelnetExcape = TRUE;
						}

						conn->InputLen = (int)(ptr2 - conn->InputBuffer);
					}

					UnpackFBBBinary(conn);
					goto OuterLoop;
				}
				else
				{

			loop:

				if (conn->InputLen == 1 && conn->InputBuffer[0] == 0)		// Single Null
				{
					conn->InputLen = 0;
					return 0;
				}

				user = conn->UserPointer;

				if (conn->BBSFlags & (MCASTRX | SYNCMODE))
				{
					//	MCAST and SYNCMODE deliver full packets

					if (conn->BBSFlags & RunningConnectScript)
						ProcessBBSConnectScript(conn, conn->InputBuffer, conn->InputLen);
					else
						ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
					
					conn->InputLen=0;
					continue;
				}

				// This looks for CR, CRLF, LF or CR/Null and removes any LF or NULL,
				// but this relies on both arriving in same packet.
				// Need to check for LF and start of packet and ignore it
				// But what if client is only using LF??
				// (WLE sends SID with CRLF, other packets with CR only)

				// We don't get here on the data part of a binary transfer, so
				// don't need to worry about messing up binary data.

				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);
				ptr2 = memchr(conn->InputBuffer, '\n', conn->InputLen);

				if (ptr)
					conn->usingCR = 1;
				
				if ((ptr2 && ptr2 < ptr) || ptr == 0)		// LF before CR, or no CR
					ptr = ptr2;								// Use LF

				if (ptr)				// CR or LF in buffer
				{
					conn->lastLineEnd = *(ptr);

					*(ptr) = '\r';		// In case was LF
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single msg in buffer

						// if Length is 1 and Term is LF and normal line end is CR
						// this is from a split CRLF - Ignore it

						if (conn->InputLen == 1 && conn->lastLineEnd == 0x0a && conn->usingCR)
							Debugprintf("BPQMail split Line End Detected");
						else
						{
							if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						Buffer = malloc(MsgLen + 100);

						memcpy(Buffer, conn->InputBuffer, MsgLen);
					
						// if Length is 1 and Term is LF and normal line end is CR
						// this is from a split CRLF - Ignore it

						if (MsgLen == 1 && conn->lastLineEnd == 0x0a && conn->usingCR)
							Debugprintf("BPQMail split Line End Detected");
						else
						{
							if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, Buffer, (int)MsgLen);
							else
								ProcessLine(conn, user, Buffer, (int)MsgLen);
						}						
						free(Buffer);

						if (*ptr == 0 || *ptr == '\n')
						{
							/// CR LF or CR Null

							ptr++;
							conn->InputLen--;
						}

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= (int)MsgLen;

						goto loop;

					}
				}
				else
				{
					// Could be a YAPP Header


					if (conn->InputLen == 2 && conn->InputBuffer[0] == ENQ  && conn->InputBuffer[1] == 1)		// YAPP Send_Init
					{
						UCHAR YAPPRR[2];
						YAPPRR[0] = ACK;
						YAPPRR[1] = 1;

						conn->InputMode = 'Y';
						QueueMsg(conn, YAPPRR, 2);

						conn->InputLen = 0;
						return 0;
					}
				}
				}

			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}
int DoBBSMonitorData(int Stream)
{
//	UCHAR Buffer[1000];
	UCHAR buff[500];

	int len = 0,count=0;
	int stamp;
	
		do
		{ 
			stamp=GetRaw(Stream, buff,&len,&count);

			if (len == 0) return 0;

			SeeifBBSUIFrame((struct _MESSAGEX *)buff, len);	
		}
		
		while (count > 0);	
 		

	return 0;

}

VOID ProcessFLARQLine(ConnectionInfo * conn, struct UserInfo * user, char * Buffer, int MsgLen)
{
	Buffer[MsgLen] = 0;

	if (MsgLen == 1 && Buffer[0] == 13)
		return;

	if (strcmp(Buffer, "ARQ::ETX\r") == 0)
	{
		// Decode it. 

		UCHAR * ptr1, * ptr2, * ptr3;
		int len, linelen;
		struct MsgInfo * Msg = conn->TempMsg;
		time_t Date;
		char FullTo[100];
		char FullFrom[100];
		char ** RecpTo = NULL;				// May be several Recipients
		char ** HddrTo = NULL;				// May be several Recipients
		char ** Via = NULL;					// May be several Recipients
		int LocalMsg[1000]	;				// Set if Recipient is a local wl2k address

		int B2To;							// Offset to To: fields in B2 header
		int Recipients = 0;
		int RMSMsgs = 0, BBSMsgs = 0;

//		Msg->B2Flags |= B2Msg;
				

		ptr1 = conn->MailBuffer;
		len = Msg->length;
		ptr1[len] = 0;

		if (strstr(ptr1, "ARQ:ENCODING::"))
		{
			// a file, not a message. If is called  "BBSPOLL" do a reverse forward else Ignore for now

			_strupr(conn->MailBuffer);
			if (strstr(conn->MailBuffer, "BBSPOLL"))
			{
				SendARQMail(conn);
			}

			free(conn->MailBuffer);
			conn->MailBuffer = NULL;
			conn->MailBufferSize = 0;

			return;
		}
	Loop:
		ptr2 = strchr(ptr1, '\r');

		linelen = (int)(ptr2 - ptr1);

		if (_memicmp(ptr1, "From:", 5) == 0 && linelen > 6)			// Can have empty From:
		{
			char SaveFrom[100];
			char * FromHA;

			memcpy(FullFrom, ptr1, linelen);
			FullFrom[linelen] = 0;

			// B2 From may now contain an @BBS 

			strcpy(SaveFrom, FullFrom);
				
			FromHA = strlop(SaveFrom, '@');

			if (strlen(SaveFrom) > 12) SaveFrom[12] = 0;
			
			strcpy(Msg->from, &SaveFrom[6]);

			if (FromHA)
			{
				if (strlen(FromHA) > 39) FromHA[39] = 0;
				Msg->emailfrom[0] = '@';
				strcpy(&Msg->emailfrom[1], _strupr(FromHA));
			}

			// Remove any SSID

			ptr3 = strchr(Msg->from, '-');
				if (ptr3) *ptr3 = 0;
		
		}
		else if (_memicmp(ptr1, "To:", 3) == 0 || _memicmp(ptr1, "cc:", 3) == 0)
		{
			HddrTo=realloc(HddrTo, (Recipients+1) * sizeof(void *));
			HddrTo[Recipients] = zalloc(100);

			memset(FullTo, 0, 99);
			memcpy(FullTo, &ptr1[4], linelen-4);
			memcpy(HddrTo[Recipients], ptr1, linelen+2);
			LocalMsg[Recipients] = FALSE;

			_strupr(FullTo);

			B2To = (int)(ptr1 - conn->MailBuffer);

			if (_memicmp(FullTo, "RMS:", 4) == 0)
			{
				// remove RMS and add @winlink.org

				strcpy(FullTo, "RMS");
				strcpy(Msg->via, &FullTo[4]);
			}
			else
			{
				ptr3 = strchr(FullTo, '@');

				if (ptr3)
				{
					*ptr3++ = 0;
					strcpy(Msg->via, ptr3);
				}
				else
					Msg->via[0] = 0;
			}
		
			if (_memicmp(&ptr1[4], "SMTP:", 5) == 0)
			{
				// Airmail Sends MARS messages as SMTP
					
				if (CheckifPacket(Msg->via))
				{
					// Packet Message

					memmove(FullTo, &FullTo[5], strlen(FullTo) - 4);
					_strupr(FullTo);
					_strupr(Msg->via);
						
					// Update the saved to: line (remove the smtp:)

					strcpy(&HddrTo[Recipients][4], &HddrTo[Recipients][9]);
					BBSMsgs++;
					goto BBSMsg;
				}

				// If a winlink.org address we need to convert to call

				if (_stricmp(Msg->via, "winlink.org") == 0)
				{
					memmove(FullTo, &FullTo[5], strlen(FullTo) - 4);
					_strupr(FullTo);
					LocalMsg[Recipients] = CheckifLocalRMSUser(FullTo);
				}
				else
				{
					memcpy(Msg->via, &ptr1[9], linelen);
					Msg->via[linelen - 9] = 0;
					strcpy(FullTo,"RMS");
				}
//					FullTo[0] = 0;

		BBSMsg:		
				_strupr(FullTo);
				_strupr(Msg->via);
			}

			if (memcmp(FullTo, "RMS:", 4) == 0)
			{
				// remove RMS and add @winlink.org

				memmove(FullTo, &FullTo[4], strlen(FullTo) - 3);
				strcpy(Msg->via, "winlink.org");
				sprintf(HddrTo[Recipients], "To: %s\r\n", FullTo);
			}

			if (strcmp(Msg->via, "RMS") == 0)
			{
				// replace RMS with @winlink.org

				strcpy(Msg->via, "winlink.org");
				sprintf(HddrTo[Recipients], "To: %s@winlink.org\r\n", FullTo);
			}

			if (strlen(FullTo) > 6)
				FullTo[6] = 0;

			strlop(FullTo, '-');

			strcpy(Msg->to, FullTo);

			if (SendBBStoSYSOPCall)
				if (_stricmp(FullTo, BBSName) == 0)
					strcpy(Msg->to, SYSOPCall);

			if ((Msg->via[0] == 0 || strcmp(Msg->via, "BPQ") == 0 || strcmp(Msg->via, "BBS") == 0))
			{
				// No routing - check @BBS and WP

				struct UserInfo * ToUser = LookupCall(FullTo);

				Msg->via[0] = 0;				// In case BPQ and not found

				if (ToUser)
				{
					// Local User. If Home BBS is specified, use it

					if (ToUser->HomeBBS[0])
					{
						strcpy(Msg->via, ToUser->HomeBBS); 
					}
				}
				else
				{
					WPRecP WP = LookupWP(FullTo);

					if (WP)
					{
						strcpy(Msg->via, WP->first_homebbs);
			
					}
				}

				// Fix To: address in B2 Header

				if (Msg->via[0])
					sprintf(HddrTo[Recipients], "To: %s@%s\r\n", FullTo, Msg->via);
				else
					sprintf(HddrTo[Recipients], "To: %s\r\n", FullTo);

			}

			RecpTo=realloc(RecpTo, (Recipients+1) * sizeof(void *));
			RecpTo[Recipients] = zalloc(10);

			Via=realloc(Via, (Recipients+1) * sizeof(void *));
			Via[Recipients] = zalloc(50);

			strcpy(Via[Recipients], Msg->via);
			strcpy(RecpTo[Recipients++], FullTo);

			// Remove the To: Line from the buffer
			
		}
		else if (_memicmp(ptr1, "Type:", 4) == 0)
		{
			if (ptr1[6] == 'N')
				Msg->type = 'T';				// NTS
			else
				Msg->type = ptr1[6];
		}
		else if (_memicmp(ptr1, "Subject:", 8) == 0)
		{
			size_t Subjlen = ptr2 - &ptr1[9];
			if (Subjlen > 60) Subjlen = 60;
			memcpy(Msg->title, &ptr1[9], Subjlen);

			goto ProcessBody;
		}
//		else if (_memicmp(ptr1, "Body:", 4) == 0)
//		{
//			MsgLen = atoi(&ptr1[5]);
//			StartofMsg = ptr1;
//		}
		else if (_memicmp(ptr1, "File:", 5) == 0)
		{
			Msg->B2Flags |= Attachments;
		}
		else if (_memicmp(ptr1, "Date:", 5) == 0)
		{
			struct tm rtime;
			char seps[] = " ,\t\r";

			memset(&rtime, 0, sizeof(struct tm));

			// Date: 2009/07/25 10:08
	
			sscanf(&ptr1[5], "%04d/%02d/%02d %02d:%02d:%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec);

			sscanf(&ptr1[5], "%02d/%02d/%04d %02d:%02d:%02d",
					&rtime.tm_mday, &rtime.tm_mon, &rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec);

			rtime.tm_year -= 1900;

			Date = mktime(&rtime) - (time_t)_MYTIMEZONE;
	
			if (Date == (time_t)-1)
				Date = time(NULL);

			Msg->datecreated = Date;

		}

		if (linelen)			// Not Null line
		{
			ptr1 = ptr2 + 2;		// Skip cr
			goto Loop;
		}
	
		
		// Processed all headers
ProcessBody:

		ptr2 +=2;					// skip crlf

		Msg->length = (int)(&conn->MailBuffer[Msg->length] - ptr2);

		memmove(conn->MailBuffer, ptr2, Msg->length);

		CreateMessageFromBuffer(conn);

		conn->BBSFlags = 0;				// Clear ARQ Mode
		return;
	}

	// File away the data

	Buffer[MsgLen++] = 0x0a;			// BBS Msgs stored with crlf

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

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, MsgLen);

	conn->TempMsg->length += MsgLen;

	return;

	// Not sure what to do yet with files, but will process emails (using text style forwarding

/*
ARQ:FILE::flarqmail-1.eml
ARQ:EMAIL::
ARQ:SIZE::96
ARQ::STX
//FLARQ COMPOSER
Date: 16/01/2014 22:26:06
To: g8bpq
From: 
Subject: test message

Hello
Hello

ARQ::ETX
*/

	return;
}

VOID ProcessTextFwdLine(ConnectionInfo * conn, struct UserInfo * user, char * Buffer, int len)
{
	Buffer[len] = 0;
//	Debugprintf(Buffer);

	// With TNC2 body prompt is a single CR, so that shouldn't be ignored.

	// If thia causes problems with other TNC PMS implementations I'll have to revisit this

//	if (len == 1 && Buffer[0] == 13)
//		return;

	if (conn->Flags & SENDTITLE)
	{	
		// Waiting for Subject: prompt

		struct MsgInfo * Msg = conn->FwdMsg;
		
		nodeprintf(conn, "%s\r", Msg->title);
		
		conn->Flags &= ~SENDTITLE;
		conn->Flags |= SENDBODY;

		// New Paccom PMS (V3.2) doesn't prompt for body so drop through and send it
		if ((conn->BBSFlags & NEWPACCOM) == 0)
			return;

	}
	
	if (conn->Flags & SENDBODY)
	{
		// Waiting for Enter Message Prompt

		struct tm * tm;
		time_t temp;

		char * MsgBytes = ReadMessageFile(conn->FwdMsg->number);
		char * MsgPtr;
		int MsgLen;
		int Index = 0;

		if (MsgBytes == 0)
		{
			MsgBytes = _strdup("Message file not found\r");
			conn->FwdMsg->length = (int)strlen(MsgBytes);
		}

		MsgPtr = MsgBytes;
		MsgLen = conn->FwdMsg->length;

		// If a B2 Message, remove B2 Header

		if (conn->FwdMsg->B2Flags & B2Msg)
		{		
			// Remove all B2 Headers, and all but the first part.
					
			MsgPtr = strstr(MsgBytes, "Body:");
			
			if (MsgPtr)
			{
				MsgLen = atoi(&MsgPtr[5]);
				MsgPtr= strstr(MsgBytes, "\r\n\r\n");		// Blank Line after headers
	
				if (MsgPtr)
					MsgPtr +=4;
				else
					MsgPtr = MsgBytes;
			
			}
			else
				MsgPtr = MsgBytes;
		}

		memcpy(&temp, &conn->FwdMsg->datereceived, sizeof(time_t));
		tm = gmtime(&temp);	

		nodeprintf(conn, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r",
				tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
				conn->FwdMsg->number, BBSName, HRoute, RlineVer);

		if (memcmp(MsgPtr, "R:", 2) != 0)    // No R line, so must be our message - put blank line after header
			BBSputs(conn, "\r");

		MsgLen = RemoveLF(MsgPtr, MsgLen);

		QueueMsg(conn, MsgPtr, MsgLen);

		if (user->ForwardingInfo->SendCTRLZ)
			nodeprintf(conn, "\r\x1a");
		else
			nodeprintf(conn, "\r/ex\r");

		free(MsgBytes);
			
		conn->FBBMsgsSent = TRUE;

		
		if (conn->FwdMsg->type == 'P')
			Index = PMSG;
		else if (conn->FwdMsg->type == 'B')
			Index = BMSG;
		else if (conn->FwdMsg->type == 'T')
			Index = TMSG;

		user->Total.MsgsSent[Index]++;
		user->Total.BytesForwardedOut[Index] += MsgLen;
			
		conn->Flags &= ~SENDBODY;
		conn->Flags |= WAITPROMPT;

		return;
	}

	if (conn->Flags & WAITPROMPT)
	{
		if (Buffer[len-2] != '>')
			return;

		conn->Flags &= ~WAITPROMPT;

		clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
		set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);

		//  Only mark as forwarded if sent to all BBSs that should have it
			
		if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
		{
			conn->FwdMsg->status = 'F';			// Mark as forwarded
			conn->FwdMsg->datechanged=time(NULL);
		}

		SaveMessageDatabase();

		conn->UserPointer->ForwardingInfo->MsgCount--;

		// See if any more to forward

		if (FindMessagestoForward(conn) && conn->FwdMsg)
		{
			struct MsgInfo * Msg;

			// If we are using SETCALLTOSENDER make sure this message is from the same sender

#ifdef LINBPQ
			BPQVECSTRUC * SESS = &BPQHOSTVECTOR[0];
#else
			BPQVECSTRUC * SESS = (BPQVECSTRUC *)BPQHOSTVECPTR;
#endif
			unsigned char AXCall[7];

			Msg = conn->FwdMsg;	
			ConvToAX25(Msg->from, AXCall);
			if (memcmp(SESS[conn->BPQStream - 1].HOSTSESSION->L4USER, AXCall, 7) != 0)
			{
				Disconnect(conn->BPQStream);
				return;
			}

			// Send S line and wait for response - SB WANT @ USA < W8AAA $1029_N0XYZ 

			conn->Flags |= SENDTITLE;


			if ((conn->BBSFlags & SETCALLTOSENDER))
				nodeprintf(conn, "S%c %s @ %s \r", Msg->type, Msg->to,
						(Msg->via[0]) ? Msg->via : conn->UserPointer->Call);
			else
				nodeprintf(conn, "S%c %s @ %s < %s $%s\r", Msg->type, Msg->to,
						(Msg->via[0]) ? Msg->via : conn->UserPointer->Call, 
						Msg->from, Msg->bid);
		}
		else
		{
			Disconnect(conn->BPQStream);
		}
		return;
	}
}


#define N               2048    /* buffer size */
#define F               60      /* lookahead buffer size */
#define THRESHOLD       2
#define NIL             N       /* leaf of tree */

extern UCHAR * infile;

BOOL CheckforMIME(SocketConn * sockptr, char * Msg, char ** Body, int * MsgLen);


VOID ProcessLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;

	if (_memicmp(Buffer, "POSYNCLOGON", 11) == 0)
	{
		WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
		conn->BBSFlags |= SYNCMODE;
		conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));

		Sleep(500);

		BBSputs(conn, "OK\r");
		Flush(conn);
		return;
	}

	if (_memicmp(Buffer, "POSYNCHELLO", 11) == 0)
	{
		// This is first message received after connecting to SYNC
		// Save Callsign

		char Reply[32];
		conn->BBSFlags |= SYNCMODE;
		conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));

		sprintf(Reply, "POSYNCLOGON %s\r", BBSName);
		BBSputs(conn, Reply);
		return;
	}

	if (conn->BBSFlags & SYNCMODE)
	{
		ProcessSyncModeMessage(conn, user, Buffer, len);
		return;
	}

	WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);

	// A few messages should be trapped here and result in an immediate disconnect, whatever mode I think the session is in (it could be wrong)

	// *** Protocol Error
	// Already Connected
	// Invalid Command

	if (_memicmp(Buffer, "Already Connected", 17) == 0 ||
		_memicmp(Buffer, "Invalid Command", 15) == 0 ||
		_memicmp(Buffer, "*** Protocol Error", 18) == 0)
	{
		conn->BBSFlags |= DISCONNECTING;
		Disconnect(conn->BPQStream);
		return;
	}

	if (conn->BBSFlags & FBBForwarding)
	{
		ProcessFBBLine(conn, user, Buffer, len);
		return;
	}

	if (conn->BBSFlags & FLARQMODE)
	{
		ProcessFLARQLine(conn, user, Buffer, len);
		return;
	}

	if (conn->BBSFlags & MCASTRX)
	{
		ProcessMCASTLine(conn, user, Buffer, len);
		return;
	}


	if (conn->BBSFlags & TEXTFORWARDING)
	{
		ProcessTextFwdLine(conn, user, Buffer, len);
		return;
	}

	// if chatting to sysop pass message to BBS console

	if (conn->BBSFlags & SYSOPCHAT)
	{
		SendUnbuffered(-1, Buffer,len);
		return;
	}

	if (conn->Flags & GETTINGMESSAGE)
	{
		ProcessMsgLine(conn, user, Buffer, len);
		return;
	}
	if (conn->Flags & GETTINGTITLE)
	{
		ProcessMsgTitle(conn, user, Buffer, len);
		return;
	}

	if (conn->BBSFlags & MBLFORWARDING)
	{
		ProcessMBLLine(conn, user, Buffer, len);
		return;
	}

	if (conn->Flags & GETTINGUSER || conn->NewUser)		// Could be new user but dont need name
	{
		if (memcmp(Buffer, ";FW:", 4) == 0 || Buffer[0] == '[')
		{
			struct	BBSForwardingInfo * ForwardingInfo;
			
			conn->Flags &= ~GETTINGUSER;

			// New User is a BBS - create a temp struct for it

			if ((user->flags & (F_BBS | F_Temp_B2_BBS)) == 0)			// It could already be a BBS without a user name
			{
				// Not defined as BBS - allocate and initialise forwarding structure
		
				user->flags |= F_Temp_B2_BBS;

				// An RMS Express user that needs a temporary BBS struct

				ForwardingInfo = user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));

				ForwardingInfo->AllowCompressed = TRUE;
				ForwardingInfo->AllowBlocked = TRUE;
				conn->UserPointer->ForwardingInfo->AllowB2 = TRUE;
			}
			SaveUserDatabase();
		}
		else
		{
			if (conn->Flags & GETTINGUSER)
			{
				conn->Flags &= ~GETTINGUSER;
				if (len > 18)
					len = 18;

				memcpy(user->Name, Buffer, len-1);
				SendWelcomeMsg(conn->BPQStream, conn, user);
				SaveUserDatabase();
				UpdateWPWithUserInfo(user);
				return;
			}
		}
	}

	// Process Command

	if (conn->Paging && (conn->LinesSent >= conn->PageLen))
	{
		// Waiting for paging prompt

		if (len > 1)
		{
			if (_memicmp(Buffer, "Abort", 1) == 0)
			{
				ClearQueue(conn);
				conn->LinesSent = 0;

				nodeprintf(conn, AbortedMsg);

				if (conn->UserPointer->Temp->ListSuspended)
					nodeprintf(conn, "<A>bort, <R Msg(s)>, <CR> = Continue..>");

				SendPrompt(conn, user);
				return;
			}
		}

		conn->LinesSent = 0;
		return;
	}

	if (user->Temp->ListSuspended)
	{
		// Paging limit hit when listing. User may abort, continue, or read one or more messages

		ProcessSuspendedListCommand(conn, user, Buffer, len);
		return;
	}
	if (len == 1)
	{
		SendPrompt(conn, user);
		return;
	}

	Buffer[len] = 0;

	if (strstr(Buffer, "ARQ:FILE:"))
	{
		// Message from FLARQ

		conn->BBSFlags |= FLARQMODE;
		strcpy(conn->ARQFilename, &Buffer[10]);			// Will need name when we decide what to do with files

		// Create a Temp Messge Stucture

		CreateMessage(conn, conn->Callsign, "", "", 'P', NULL, NULL);

		Buffer[len++] = 0x0a;			// BBS Msgs stored with crlf

		if ((conn->TempMsg->length + len) > conn->MailBufferSize)
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

		memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, len);

		conn->TempMsg->length += len;

		return;
	}
	if (Buffer[0] == ';')			// WL2K Comment
	{
		if (memcmp(Buffer, ";FW:", 4) == 0)
		{
			// Paclink User Select (poll for list)
		
			char * ptr1,* ptr2, * ptr3;
			int index=0;

			// Convert string to Multistring
		
			Buffer[len-1] = 0;

			conn->PacLinkCalls = zalloc(len*3);

			ptr1 = &Buffer[5];
			ptr2 = (char *)conn->PacLinkCalls;
			ptr2 += (len * 2);
			strcpy(ptr2, ptr1);

			while (ptr2)
			{
				ptr3 = strlop(ptr2, ' ');

				if (strlen(ptr2))
					conn->PacLinkCalls[index++] = ptr2;
		
				ptr2 = ptr3;
			}
	
			return;	
		}

		if (memcmp(Buffer, ";FR:", 4) == 0)
		{
			// New Message from TriMode - Just igonre till I know what to do with it

			return;
		}

		// Ignore other ';' message

		return;
	}



	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// If a BBS, set BBS Flag

		if (user->flags & ( F_BBS | F_Temp_B2_BBS))
		{
			if (user->ForwardingInfo)
			{
				if (user->ForwardingInfo->Forwarding && ((conn->BBSFlags & OUTWARDCONNECT) == 0))
				{
					BBSputs(conn, "Already Connected\r");
					Flush(conn);
					Sleep(500);
					Disconnect(conn->BPQStream);
					return;
				}
			}

			if (user->ForwardingInfo)
			{
				user->ForwardingInfo->Forwarding = TRUE;
				user->ForwardingInfo->FwdTimer = 0;			// So we dont send to immediately
			}
		}

		if (user->flags & ( F_BBS | F_PMS | F_Temp_B2_BBS))
		{
			Parse_SID(conn, &Buffer[1], len-4);
			
			if (conn->BBSFlags & FBBForwarding)
			{
				conn->FBBIndex = 0;		// ready for first block;
				conn->FBBChecksum = 0;
				memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			}
			else
				FBBputs(conn, ">\r");

		}

		return;
	}

	Cmd = strtok_s(Buffer, seps, &Context);

	if (Cmd == NULL)
	{
		if (!CheckForTooManyErrors(conn))
			BBSputs(conn, "Invalid Command\r");

		SendPrompt(conn, user);
		return;
	}

	Arg1 = strtok_s(NULL, seps, &Context);
	CmdLen = (int)strlen(Cmd);

	// Check List first. If any other, save last listed to user record.

	if (_memicmp(Cmd, "L", 1) == 0 && _memicmp(Cmd, "LISTFILES", 3) != 0)
	{
		DoListCommand(conn, user, Cmd, Arg1, FALSE, Context);
		SendPrompt(conn, user);
		return;
	}

	if (conn->lastmsg > user->lastmsg)
	{
		user->lastmsg = conn->lastmsg;
		SaveUserDatabase();
	}

	if (_stricmp(Cmd, "SHOWRMSPOLL") == 0)
	{
		DoShowRMSCmd(conn, user, Arg1, Context);
		return;
	}

	if (_stricmp(Cmd, "AUTH") == 0)
	{
		DoAuthCmd(conn, user, Arg1, Context);
		return;
	}

	if (_memicmp(Cmd, "Abort", 1) == 0)
	{
		ClearQueue(conn);
		conn->LinesSent = 0;

		nodeprintf(conn, AbortedMsg);

		if (conn->UserPointer->Temp->ListSuspended)
			nodeprintf(conn, "<A>bort, <R Msg(s)>, <CR> = Continue..>");

		SendPrompt(conn, user);
		return;
	}
	if (_memicmp(Cmd, "Bye", CmdLen) == 0 || _stricmp(Cmd, "ELSE") == 0)
	{
		ExpandAndSendMessage(conn, SignoffMsg, LOG_BBS);
		Flush(conn);
		Sleep(1000);

		if (conn->BPQStream > 0)
			Disconnect(conn->BPQStream);
#ifndef LINBPQ
		else
			CloseConsole(conn->BPQStream);
#endif
		return;
	}

	if (_memicmp(Cmd, "Node", 4) == 0)
	{
		ExpandAndSendMessage(conn, SignoffMsg, LOG_BBS);
		Flush(conn);
		Sleep(1000);
			
		if (conn->BPQStream > 0)
			ReturntoNode(conn->BPQStream);
#ifndef LINBPQ
		else
			CloseConsole(conn->BPQStream);
#endif
		return;						
	}

	if (_memicmp(Cmd, "IDLETIME", 4) == 0)
	{
		DoSetIdleTime(conn, user, Arg1, Context);
		return;
	}

	if (_stricmp(Cmd, "SETNEXTMESSAGENUMBER") == 0)
	{
		DoSetMsgNo(conn, user, Arg1, Context);
		return;
	}

	if (strlen(Cmd) < 12 && _memicmp(Cmd, "D", 1) == 0)
	{
		DoDeliveredCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "K", 1) == 0)
	{
		DoKillCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}


	if (_memicmp(Cmd, "LISTFILES", 3) == 0 || _memicmp(Cmd, "FILES", 5) == 0)
	{
		ListFiles(conn, user, Arg1);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "READFILE", 4) == 0)
	{
		ReadBBSFile(conn, user, Arg1);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "REROUTEMSGS", 7) == 0)
	{
		if (conn->sysop == 0)
			nodeprintf(conn, "Reroute Messages needs SYSOP status\r");
		else
		{
			ReRouteMessages();
			nodeprintf(conn, "Ok\r");
		}
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "YAPP", 4) == 0)
	{
		YAPPSendFile(conn, user, Arg1);
		return;
	}

	if (_memicmp(Cmd, "UH", 2) == 0 && conn->sysop)
	{
		DoUnholdCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_stricmp(Cmd, "IMPORT") == 0)
	{
		DoImportCmd(conn, user, Arg1, Context);
		return;
	}

	if (_stricmp(Cmd, "EXPORT") == 0)
	{
		DoExportCmd(conn, user, Arg1, Context);
		return;
	}

	if (_memicmp(Cmd, "I", 1) == 0)
	{
		char * Save;
		char * MsgBytes;

		if (Arg1)
		{
			// User WP lookup

			DoWPLookup(conn, user, Cmd[1], Arg1);
			SendPrompt(conn, user);
			return;	
		}


		MsgBytes = Save = ReadInfoFile("info.txt");
		if (MsgBytes)
		{
			int Length;

			// Remove lf chars

			Length = RemoveLF(MsgBytes, (int)strlen(MsgBytes));

			QueueMsg(conn, MsgBytes, Length);
			free(Save);
		}
		else
			BBSputs(conn, "SYSOP has not created an INFO file\r");


		SendPrompt(conn, user);
		return;	
	}


	if (_memicmp(Cmd, "Name", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 17)
				Arg1[17] = 0;

			strcpy(user->Name, Arg1);
			UpdateWPWithUserInfo(user);

		}

		SendWelcomeMsg(conn->BPQStream, conn, user);
		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "OP", 2) == 0)
	{
		int Lines;
		
		// Paging Control. Param is number of lines per page

		if (Arg1)
		{
			Lines = atoi(Arg1);
			
			if (Lines)				// Sanity Check
			{
				if (Lines < 10)
				{
					nodeprintf(conn,"Page Length %d is too short\r", Lines);
					SendPrompt(conn, user);
					return;
				}
			}

			user->PageLen = Lines;
			conn->PageLen = Lines;
			conn->Paging = (Lines > 0);
			SaveUserDatabase();
		}
		
		nodeprintf(conn,"Page Length is %d\r", user->PageLen);
		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "QTH", CmdLen) == 0)
	{
		if (Arg1)
		{
			// QTH may contain spaces, so put back together, and just split at cr
			
			Arg1[strlen(Arg1)] = ' ';
			strtok_s(Arg1, "\r", &Context);

			if (strlen(Arg1) > 60)
				Arg1[60] = 0;

			strcpy(user->Address, Arg1);
			UpdateWPWithUserInfo(user);

		}

		nodeprintf(conn,"QTH is %s\r", user->Address);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "ZIP", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 8)
				Arg1[8] = 0;

			strcpy(user->ZIP, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
		}

		nodeprintf(conn,"ZIP is %s\r", user->ZIP);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "CMSPASS", 7) == 0)
	{
		if (Arg1 == 0)
		{
			nodeprintf(conn,"Must specify a password\r");
		}
		else
		{
			if (strlen(Arg1) > 15)
				Arg1[15] = 0;

			strcpy(user->CMSPass, Arg1);
			nodeprintf(conn,"CMS Password Set\r");
			SaveUserDatabase();
		}

		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "PASS", CmdLen) == 0)
	{
		if (Arg1 == 0)
		{
			nodeprintf(conn,"Must specify a password\r");
		}
		else
		{
			if (strlen(Arg1) > 12)
				Arg1[12] = 0;

			strcpy(user->pass, Arg1);
			nodeprintf(conn,"BBS Password Set\r");
			SaveUserDatabase();
		}

		SendPrompt(conn, user);

		return;
	}


	if (_memicmp(Cmd, "R", 1) == 0)
	{
		DoReadCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "S", 1) == 0)
	{
		if (!DoSendCommand(conn, user, Cmd, Arg1, Context))
			SendPrompt(conn, user);
		return;
	}

	if ((_memicmp(Cmd, "Help", CmdLen) == 0) || (_memicmp(Cmd, "?", 1) == 0))
	{
		char * Save;
		char * MsgBytes = Save = ReadInfoFile("help.txt");

		if (MsgBytes)
		{
			int Length;

			// Remove lf chars

			Length = RemoveLF(MsgBytes, (int)strlen(MsgBytes));

			QueueMsg(conn, MsgBytes, Length);
			free(Save);
		}
		else
		{
			BBSputs(conn, "A - Abort Output\r");
			BBSputs(conn, "B - Logoff\r");
			BBSputs(conn, "CMSPASS Password - Set CMS Password\r");
			BBSputs(conn, "D - Flag NTS Message(s) as Delivered - D num\r");
			BBSputs(conn, "HOMEBBS - Display or get HomeBBS\r");
			BBSputs(conn, "INFO - Display information about this BBS\r");
			BBSputs(conn, "I CALL - Lookup CALL in WP Allows *CALL CALL* *CALL* wildcards\r");
			BBSputs(conn, "I@ PARAM - Lookup @BBS in WP\r");
			BBSputs(conn, "IZ PARAM - Lookup Zip Codes in WP\r");
			BBSputs(conn, "IH PARAM - Lookup HA elements in WP - eg USA EU etc\r");

			BBSputs(conn, "K - Kill Message(s) - K num, KM (Kill my read messages)\r");
			BBSputs(conn, "L - List Message(s) - \r");
			BBSputs(conn, "   L = List New, LR = List New (Oldest first)\r");
			BBSputs(conn, "   LM = List Mine, L> Call, L< Call, L@ = List to, from or at\r");
			BBSputs(conn, "   LL num = List msg num, L num-num = List Range\r");
			BBSputs(conn, "   LN LY LH LK LF L$ LD = List Message with corresponding Status\r");
			BBSputs(conn, "   LB LP LT = List Mesaage with corresponding Type\r");
			BBSputs(conn, "   LC = List TO fields of all active bulletins\r");
			BBSputs(conn, "   You can combine most selections eg LMP, LMN LB< G8BPQ\r"); 
			BBSputs(conn, "LISTFILES or FILES - List files available for download\r");

			BBSputs(conn, "N Name - Set Name\r");
			BBSputs(conn, "NODE - Return to Node\r");
			BBSputs(conn, "OP n - Set Page Length (Output will pause every n lines)\r");
			BBSputs(conn, "PASS Password - Set BBS Password\r");
			BBSputs(conn, "POLLRMS - Manage Polling for messages from RMS \r");
			BBSputs(conn, "Q QTH - Set QTH\r");
			BBSputs(conn, "R - Read Message(s) - R num \r");
			BBSputs(conn, "                      RM (Read new messages to me), RMR (RM oldest first)\r");
			BBSputs(conn, "READ Name - Read File\r");

			BBSputs(conn, "S - Send Message - S or SP Send Personal, SB Send Bull, ST Send NTS,\r");
			BBSputs(conn, "                   SR Num - Send Reply, SC Num - Send Copy\r");
			BBSputs(conn, "X - Toggle Expert Mode\r");
			BBSputs(conn, "YAPP - Download file from BBS using YAPP protocol\r");
			if (conn->sysop)
			{
				BBSputs(conn, "DOHOUSEKEEPING - Run Housekeeping process\r");
				BBSputs(conn, "EU - Edit User Flags - Type EU for Help\r");
				BBSputs(conn, "EXPORT - Export messages to file - Type EXPORT for Help\r");
				BBSputs(conn, "FWD - Control Forwarding - Type FWD for Help\r");
				BBSputs(conn, "IMPORT - Import messages from file - Type IMPORT for Help\r");
				BBSputs(conn, "REROUTEMSGS - Rerun message routing process\r");
				BBSputs(conn, "SETNEXTMESSAGENUMBER - Sets next message number\r");
				BBSputs(conn, "SHOWRMSPOLL - Displays your RMS polling list\r");
				BBSputs(conn, "UH - Unhold Message(s) - UH ALL or UH num num num...\r");
			}
		}

		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "Ver", CmdLen) == 0)
	{
		nodeprintf(conn, "BBS Version %s\rNode Version %s\r", VersionStringWithBuild, GetVersionString());

		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "HOMEBBS", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 40) Arg1[40] = 0;

			strcpy(user->HomeBBS, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
	
			if (!strchr(Arg1, '.'))
				BBSputs(conn, "Please enter HA with HomeBBS eg g8bpq.gbr.eu - this will help message routing\r");
		}

		nodeprintf(conn,"HomeBBS is %s\r", user->HomeBBS);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if ((_memicmp(Cmd, "EDITUSER", 5) == 0) || (_memicmp(Cmd, "EU", 2) == 0))
	{
		DoEditUserCmd(conn, user, Arg1, Context);
		return;
	}

	if (_stricmp(Cmd, "POLLRMS") == 0)
	{
		DoPollRMSCmd(conn, user, Arg1, Context);
		return;
	}

	if (_stricmp(Cmd, "DOHOUSEKEEPING") == 0)
	{
		DoHousekeepingCmd(conn, user, Arg1, Context);
		return;
	}


	if (_stricmp(Cmd, "FWD") == 0)
	{
		DoFwdCmd(conn, user, Arg1, Context);
		return;
	}

	if (_memicmp(Cmd, "X", 1) == 0)
	{
		user->flags ^= F_Expert;

		if (user->flags & F_Expert)
			BBSputs(conn, "Expert Mode\r");
		else
			BBSputs(conn, "Expert Mode off\r");

		SaveUserDatabase();
		SendPrompt(conn, user);
		return;
	}

	if (conn->Flags == 0)
	{
		if (!CheckForTooManyErrors(conn))
			BBSputs(conn, "Invalid Command\r");

		SendPrompt(conn, user);
	}

	//	Send if possible

	Flush(conn);
}

VOID __cdecl nprintf(CIRCUIT * conn, const char * format, ...)
{
	// seems to be printf to a socket

	char buff[600];
	va_list(arglist);
	
	va_start(arglist, format);
	vsprintf(buff, format, arglist);

	BBSputs(conn, buff);
}

// Code to delete obsolete files from Mail folder

#ifdef WIN32

int DeleteRedundantMessages()
{
   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   char File[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
   int Msgno;

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, MailDir);
   strcat(szDir, "\\*.mes");



   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      return 0;
   } 
   
   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         OutputDebugString(ffd.cFileName);
      }
      else
      {
		 Msgno = atoi(&ffd.cFileName[2]);

		 if (MsgnotoMsg[Msgno] == 0)
		 {
			 sprintf(File, "%s/%s%c", MailDir, ffd.cFileName, 0);
			 Debugprintf("Tidy Mail - Delete %s\n", File);

//			 if (DeletetoRecycleBin)
				DeletetoRecycle(File);
//			 else
//				 DeleteFile(File);
		 }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   FindClose(hFind);
   return 0;
}

#else

#include <dirent.h>

int MsgFilter(const struct dirent * dir)
{
	return (strstr(dir->d_name, ".mes") != 0);
}

int DeleteRedundantMessages()
{
	struct dirent **namelist;
    int n;
	struct stat STAT;
	int Msgno = 0, res;
	char File[100];
     	
    n = scandir("Mail", &namelist, MsgFilter, alphasort);

	if (n < 0) 
		perror("scandir");
	else  
	{ 
		while(n--)
		{
			if (stat(namelist[n]->d_name, &STAT) == 0);
			{
				Msgno = atoi(&namelist[n]->d_name[2]);

				if (MsgnotoMsg[Msgno] == 0)
				{
					sprintf(File, "Mail/%s", namelist[n]->d_name);
					printf("Deleting %s\n", File);
					unlink(File);
				}
			}
			free(namelist[n]);
		}
		free(namelist);
    }
	return 0;
}
#endif

VOID TidyWelcomeMsg(char ** pPrompt)
{
	// Make sure Welcome Message doesn't ends with >

	char * Prompt = *pPrompt;

	int i = (int)strlen(Prompt) - 1;

	*pPrompt = realloc(Prompt, i + 5);	// In case we need to expand it

	Prompt = *pPrompt;

	while (Prompt[i] == 10 || Prompt[i] == 13)
	{
		Prompt[i--] = 0;
	}

	while (i >= 0 && Prompt[i] == '>')
		Prompt[i--] = 0;

	strcat(Prompt, "\r\n");
}

VOID TidyPrompt(char ** pPrompt)
{
	// Make sure prompt ends > CR LF

	char * Prompt = *pPrompt;

	int i = (int)strlen(Prompt) - 1;

	*pPrompt = realloc(Prompt, i + 5);	// In case we need to expand it

	Prompt = *pPrompt;

	while (Prompt[i] == 10 || Prompt[i] == 13)
	{
		Prompt[i--] = 0;
	}

	if (Prompt[i] != '>')
		strcat(Prompt, ">");

	strcat(Prompt, "\r\n");
}

VOID TidyPrompts()
{
	TidyPrompt(&Prompt);
	TidyPrompt(&NewPrompt);
	TidyPrompt(&ExpertPrompt);
}

BOOL SendARQMail(CIRCUIT * conn)
{
	conn->NextMessagetoForward = FirstMessageIndextoForward;

	// Send Message. There is no mechanism for reverse forwarding

	if (FindMessagestoForward(conn))
	{
		struct MsgInfo * Msg;
		char MsgHddr[512];
		int HddrLen;
		char TimeString[64];
		char * WholeMessage;

		char * MsgBytes = ReadMessageFile(conn->FwdMsg->number);
		int MsgLen;
		
		if (MsgBytes == 0)
		{
			MsgBytes = _strdup("Message file not found\r");
			conn->FwdMsg->length = (int)strlen(MsgBytes);
		}

		Msg = conn->FwdMsg;
		WholeMessage = malloc(Msg->length + 512);

		FormatTime(TimeString, (time_t)Msg->datecreated);

/*
ARQ:FILE::flarqmail-1.eml
ARQ:EMAIL::
ARQ:SIZE::96
ARQ::STX
//FLARQ COMPOSER
Date: 16/01/2014 22:26:06
To: g8bpq
From: 
Subject: test message

Hello
Hello

ARQ::ETX		
*/
		Logprintf(LOG_BBS, conn, '>', "ARQ Send Msg %d From %s To %s", Msg->number, Msg->from, Msg->to);

		HddrLen = sprintf(MsgHddr, "Date: %s\nTo: %s\nFrom: %s\nSubject %s\n\n",
			TimeString, Msg->to, Msg->from, Msg->title);
				
		MsgLen = sprintf(WholeMessage, "ARQ:FILE::Msg%s_%d\nARQ:EMAIL::\nARQ:SIZE::%d\nARQ::STX\n%s%s\nARQ::ETX\n",
			BBSName, Msg->number, (int)(HddrLen + strlen(MsgBytes)), MsgHddr, MsgBytes);

		WholeMessage[MsgLen] = 0;
		QueueMsg(conn,WholeMessage, MsgLen);

		free(WholeMessage);
		free(MsgBytes);

		// FLARQ doesn't ACK the message, so set flag to look for all acked
				
		conn->BBSFlags |= ARQMAILACK;
		conn->ARQClearCount = 10;		// To make sure clear isn't reported too soon

		return TRUE;
	}

	// Nothing to send - close

	Logprintf(LOG_BBS, conn, '>', "ARQ Send -  Nothing to Send - Closing");

	conn->CloseAfterFlush = 20;
	return FALSE;
}

char *stristr (char *ch1, char *ch2)
{
	char	*chN1, *chN2;
	char	*chNdx;
	char	*chRet				= NULL;

	chN1 = _strdup (ch1);
	chN2 = _strdup (ch2);
	if (chN1 && chN2)
	{
		chNdx = chN1;
		while (*chNdx)
		{
			*chNdx = (char) tolower (*chNdx);
			chNdx ++;
		}
		chNdx = chN2;
		while (*chNdx)
		{
			*chNdx = (char) tolower (*chNdx);
			chNdx ++;
		}

		chNdx = strstr (chN1, chN2);
		if (chNdx)
			chRet = ch1 + (chNdx - chN1);
	}
	free (chN1);
	free (chN2);
	return chRet;
}

#ifdef WIN32

void ListFiles(ConnectionInfo * conn, struct UserInfo * user, char * filename)
{

   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
 
   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, GetBPQDirectory());
   strcat(szDir, "\\BPQMailChat\\Files\\*.*");

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      nodeprintf(conn, "No Files\r");
	  return;
   } 
   
   // List all the files in the directory with some info about them.

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{}
		else
		{
			if (filename == NULL || stristr(ffd.cFileName, filename))
				nodeprintf(conn, "%s %d\r", ffd.cFileName, ffd.nFileSizeLow);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);
	
	FindClose(hFind);
}

#else

#include <dirent.h>

void ListFiles(ConnectionInfo * conn, struct UserInfo * user, char * filename)
{
	struct dirent **namelist;
    int n, i;
	struct stat STAT;
	time_t now = time(NULL);
	int Age = 0, res;
	char FN[256];
     	
    n = scandir("Files", &namelist, NULL, alphasort);

	if (n < 0) 
		perror("scandir");
	else  
	{ 
		for (i = 0; i < n; i++)
		{
			sprintf(FN, "Files/%s", namelist[i]->d_name);

			if (filename == NULL || stristr(namelist[i]->d_name, filename))
				if (FN[6] != '.' && stat(FN, &STAT) == 0)
					nodeprintf(conn, "%s %d\r", namelist[i]->d_name, STAT.st_size);
			
			free(namelist[i]);
		}
		free(namelist);
    }
	return;
}
#endif

void ReadBBSFile(ConnectionInfo * conn, struct UserInfo * user, char * filename)
{
	char * MsgBytes;
	
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	struct stat STAT;

	if (filename == NULL)
	{
		nodeprintf(conn, "Missing Filename\r");
		return;
	}

	if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\'))
	{
		nodeprintf(conn, "Invalid filename\r");
		return;
	}

	if (BaseDir[0])
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/Files/%s", BaseDir, filename);
	else
		sprintf_s(MsgFile, sizeof(MsgFile), "Files/%s", filename);

	if (stat(MsgFile, &STAT) != -1)
	{
		FileSize = STAT.st_size;

		hFile = fopen(MsgFile, "rb");

		if (hFile)
		{
			int Length;
	
			MsgBytes=malloc(FileSize+1);
			fread(MsgBytes, 1, FileSize, hFile); 
			fclose(hFile);

			MsgBytes[FileSize]=0;

			// Remove lf chars

			Length = RemoveLF(MsgBytes, (int)strlen(MsgBytes));

			QueueMsg(conn, MsgBytes, Length);
			free(MsgBytes);

			nodeprintf(conn, "\r\r[End of File %s]\r", filename);
			return;
		}
	}

	nodeprintf(conn, "File %s not found\r", filename);
}

VOID ProcessSuspendedListCommand(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	struct  TempUserInfo * Temp = user->Temp;

	Buffer[len] = 0;

	//	Command entered during listing pause. May be A R or C (or <CR>)

	if (Buffer[0] == 'A' || Buffer[0] == 'a')
	{
		// Abort

		Temp->ListActive = Temp->ListSuspended = FALSE;
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Buffer, "R ", 2) == 0)
	{
		// Read Message(es)

		int msgno;
		char * ptr;
		char * Context;

		ptr = strtok_s(&Buffer[2], " ", &Context);

		while (ptr)
		{
			msgno = atoi(ptr);
			ReadMessage(conn, user, msgno);

			ptr = strtok_s(NULL, " ", &Context);
		}

		nodeprintf(conn, "<A>bort, <R Msg(s)>, <CR> = Continue..>");
		return;
	}

	if (Buffer[0] == 'C' || Buffer[0] == 'c' || Buffer[0] == '\r' )
	{
		//	Resume Listing from where we left off

		DoListCommand(conn, user, Temp->LastListCommand, Temp->LastListParams, TRUE, "");
		SendPrompt(conn, user);
		return;
	}

	nodeprintf(conn, "<A>bort, <R Message>, <CR> = Continue..>");

}
/*
CreateMessageWithAttachments()
{
	int i;
	char * ptr, * ptr2, * ptr3, * ptr4;
	char Boundary[1000];
	BOOL Multipart = FALSE;
	BOOL ALT = FALSE;
	int Partlen;
	char * Save;
	BOOL Base64 = FALSE;
	BOOL QuotedP = FALSE;
	
	char FileName[100][250] = {""};
	int FileLen[100];
	char * FileBody[100];
	char * MallocSave[100];
	UCHAR * NewMsg;

	int Files = 0;

	ptr = Msg;

	if ((sockptr->MailSize + 2000) > sockptr->MailBufferSize)
	{
		sockptr->MailBufferSize += 2000;
		sockptr->MailBuffer = realloc(sockptr->MailBuffer, sockptr->MailBufferSize);
	
		if (sockptr->MailBuffer == NULL)
		{
			CriticalErrorHandler("Failed to extend Message Buffer");
			shutdown(sockptr->socket, 0);
			return FALSE;
		}
	}


	NewMsg = sockptr->MailBuffer + 1000;

	NewMsg += sprintf(NewMsg, "Body: %d\r\n", FileLen[0]);

	for (i = 1; i < Files; i++)
	{
		NewMsg += sprintf(NewMsg, "File: %d %s\r\n", FileLen[i], FileName[i]);
	}

	NewMsg += sprintf(NewMsg, "\r\n");

	for (i = 0; i < Files; i++)
	{
		memcpy(NewMsg, FileBody[i], FileLen[i]);
		NewMsg += FileLen[i];
		free(MallocSave[i]);
		NewMsg += sprintf(NewMsg, "\r\n");
	}

	*MsgLen = NewMsg - (sockptr->MailBuffer + 1000);
	*Body = sockptr->MailBuffer + 1000;

	return TRUE;		// B2 Message
}

*/
VOID CreateUserReport()
{
	struct UserInfo * User;
	int i;
	char Line[200];
	int len;
	char File[MAX_PATH];
	FILE * hFile;

	sprintf(File, "%s/UserList.csv", BaseDir);
	
	hFile = fopen(File, "wb");

	if (hFile == NULL)
	{
		Debugprintf("Failed to create UserList.csv");
		return;
	}
	
	for (i=1; i <= NumberofUsers; i++)
	{
		User = UserRecPtr[i];

		len = sprintf(Line, "%s,%d,%s,%x,%s,\"%s\",%x,%s,%s,%s\r\n",
			User->Call,
			User->lastmsg,
			FormatDateAndTime((time_t)User->TimeLastConnected, FALSE),
			User->flags,
			User->Name,
			User->Address,
			User->RMSSSIDBits,
			User->HomeBBS,
			User->QRA,
			User->ZIP
//	struct MsgStats Total;
//	struct MsgStats	Last;
			);
			fwrite(Line, 1, len, hFile);
	}

	fclose(hFile);
}

BOOL ProcessYAPPMessage(CIRCUIT * conn)
{
	int Len = conn->InputLen;
	UCHAR * Msg = conn->InputBuffer;
	int pktLen = Msg[1];
	char Reply[2] = {ACK};
	int NameLen, SizeLen, OptLen;
	char * ptr;
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	char Mess[255];
	int len;
	char * FN = &Msg[2];

	switch (Msg[0])
	{
	case ENQ: // YAPP Send_Init

		// Shouldn't occur in session. Reset state
				
		Mess[0] = ACK;
		Mess[1] = 1;
		QueueMsg(conn, Mess, 2);
		Flush(conn);
		conn->InputLen = 0;
		if (conn->MailBuffer)
		{
			free(conn->MailBuffer);
			conn->MailBufferSize=0;
			conn->MailBuffer=0;
		}
		return TRUE;

	case SOH:

		// HD Send_Hdr     SOH  len  (Filename)  NUL  (File Size in ASCII)  NUL (Opt) 

		// YAPPC has date/time in dos format

		if (Len < Msg[1] + 1)
			return 0;

		NameLen = (int)strlen(FN);
		strcpy(conn->ARQFilename, FN);
		ptr = &Msg[3 + NameLen];
		SizeLen = (int)strlen(ptr);
		FileSize = atoi(ptr);

		// Check file name for unsafe characters (.. / \)

		if (strstr(FN, "..") || strchr(FN, '/') || strchr(FN, '\\'))
		{
			Mess[0] = NAK;
			Mess[1] = 0;
			QueueMsg(conn, Mess, 2);
			Flush(conn);
			len = sprintf_s(Mess, sizeof(Mess), "YAPP File Name %s invalid\r", FN);
			QueueMsg(conn, Mess, len);
			SendPrompt(conn, conn->UserPointer);
			WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);

			conn->InputLen = 0;
			conn->InputMode = 0;

			return FALSE;
		}

		OptLen = pktLen - (NameLen + SizeLen + 2);

		conn->YAPPDate = 0;

		if (OptLen >= 8)		// We have a Date/Time for YAPPC
		{
			ptr = ptr + SizeLen + 1;
			conn->YAPPDate = strtol(ptr, NULL, 16);
		}

		// Check Size

		if (FileSize > MaxRXSize)
		{
			Mess[0] = NAK;
			Mess[1] = sprintf(&Mess[2], "YAPP File %s size %d larger than limit %d\r", conn->ARQFilename, FileSize, MaxRXSize);
			QueueMsg(conn, Mess, Mess[1] + 2);
	
			Flush(conn);

			len = sprintf_s(Mess, sizeof(Mess), "YAPP File %s size %d larger than limit %d\r", conn->ARQFilename, FileSize, MaxRXSize);
			QueueMsg(conn, Mess, len);
			SendPrompt(conn, conn->UserPointer);
			WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);

			conn->InputLen = 0;
			conn->InputMode = 0;

			return FALSE;
		}
		
		// Make sure file does not exist

	
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/Files/%s", BaseDir, conn->ARQFilename);
	
		hFile = fopen(MsgFile, "rb");

		if (hFile)
		{
			Mess[0] = NAK;
			Mess[1] = sprintf(&Mess[2], "YAPP File %s already exists\r", conn->ARQFilename);;
			QueueMsg(conn, Mess, Mess[1] + 2);
	
			Flush(conn);
	
			len = sprintf_s(Mess, sizeof(Mess), "YAPP File %s already exists\r", conn->ARQFilename);
			QueueMsg(conn, Mess, len);
			SendPrompt(conn, conn->UserPointer);
			WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);
			fclose(hFile);

			conn->InputLen = 0;
			conn->InputMode = 0;

			return FALSE;
		}


		conn->MailBufferSize = FileSize;
		conn->MailBuffer=malloc(FileSize);
		conn->YAPPLen = 0;

		if (conn->YAPPDate)			// If present use YAPPC
			Reply[1] = ACK;			//Receive_TPK
		else
			Reply[1] = 2;			//Rcv_File

		QueueMsg(conn, Reply, 2);

		len = sprintf_s(Mess, sizeof(Mess), "YAPP upload to %s started", conn->ARQFilename);
		WriteLogLine(conn, '!', Mess, len, LOG_BBS);

		conn->InputLen = 0;
		return FALSE;
		
	case STX:

		// Data Packet

		// Check we have it all

		if (conn->YAPPDate)			// If present use YAPPC so have checksum
		{
			if (pktLen > (Len - 3))		// -3 for header and checksum
				return 0;				// Wait for rest
		}
		else
		{
			if (pktLen > (Len - 2))		// -2 for header
				return 0;				// Wait for rest
		}

		// Save data and remove from buffer

		// if YAPPC check checksum

		if (conn->YAPPDate)
		{
			UCHAR Sum = 0;
			int i;
			UCHAR * uptr = &Msg[2];

			i = pktLen;

			while(i--)
				Sum += *(uptr++);

			if (Sum != *uptr)
			{
				// Checksum Error

				Mess[0] = CAN;
				Mess[1] = 0;
				QueueMsg(conn, Mess, 2);
				Flush(conn);
				len = sprintf_s(Mess, sizeof(Mess), "YAPPC Checksum Error\r");
				QueueMsg(conn, Mess, len);
				WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);
				conn->InputLen = 0;
				conn->InputMode = 0;
				return TRUE;
			}
		}

		if ((conn->YAPPLen) + pktLen > conn->MailBufferSize)
		{
			// Too Big ??

			Mess[0] = CAN;
			Mess[1] = 0;
			QueueMsg(conn, Mess, 2);
			Flush(conn);
			len = sprintf_s(Mess, sizeof(Mess), "YAPP Too much data received\r");
			QueueMsg(conn, Mess, len);
			WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);
			conn->InputLen = 0;
			conn->InputMode = 0;
			return TRUE;
		}


		memcpy(&conn->MailBuffer[conn->YAPPLen], &Msg[2], pktLen);
		conn->YAPPLen += pktLen;

		if (conn->YAPPDate)
			++pktLen;				// Add Checksum

		conn->InputLen -= (pktLen + 2);
		memmove(conn->InputBuffer, &conn->InputBuffer[pktLen + 2], conn->InputLen);

		return TRUE;

	case ETX:

		// End Data



		if (conn->YAPPLen == conn->MailBufferSize)
		{
			// All received

			int ret;
			DWORD Written = 0;

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/Files/%s", BaseDir, conn->ARQFilename);
	
#ifdef WIN32
			hFile = CreateFile(MsgFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
			{	
				ret = WriteFile(hFile, conn->MailBuffer, conn->YAPPLen, &Written, NULL);
 
				if (conn->YAPPDate)
				{
					FILETIME FileTime;
					struct tm TM;
					struct timeval times[2];
					time_t TT;
/*			
					The MS-DOS date. The date is a packed value with the following format.

					cant use DosDateTimeToFileTime on Linux
		
					Bits	Description
					0-4	Day of the month (1–31)
					5-8	Month (1 = January, 2 = February, and so on)
					9-15	Year offset from 1980 (add 1980 to get actual year)
					wFatTime
					The MS-DOS time. The time is a packed value with the following format.
					Bits	Description
					0-4	Second divided by 2
					5-10	Minute (0–59)
					11-15	Hour (0–23 on a 24-hour clock)
*/
					memset(&TM, 0, sizeof(TM));

					TM.tm_sec = (conn->YAPPDate & 0x1f) << 1;
					TM.tm_min = ((conn->YAPPDate >> 5) & 0x3f);
					TM.tm_hour =  ((conn->YAPPDate >> 11) & 0x1f);

					TM.tm_mday =  ((conn->YAPPDate >> 16) & 0x1f);
					TM.tm_mon =  ((conn->YAPPDate >> 21) & 0xf) - 1;
					TM.tm_year = ((conn->YAPPDate >> 25) & 0x7f) + 80;

					Debugprintf("%d %d %d %d %d %d", TM.tm_year, TM.tm_mon, TM.tm_mday, TM.tm_hour, TM.tm_min, TM.tm_sec);

					TT = mktime(&TM);
					times[0].tv_sec = times[1].tv_sec = 
					times[0].tv_usec = times[1].tv_usec = 0;

					DosDateTimeToFileTime((WORD)(conn->YAPPDate >> 16), (WORD)conn->YAPPDate & 0xFFFF, &FileTime);
					ret = SetFileTime(hFile, &FileTime, &FileTime, &FileTime);
					ret = GetLastError();

				}
				CloseHandle(hFile);
			}
#else

			hFile = fopen(MsgFile, "wb");
			if (hFile)
			{
				Written = fwrite(conn->MailBuffer, 1, conn->YAPPLen, hFile);
				fclose(hFile);

				if (conn->YAPPDate)
				{
					struct tm TM;
					struct timeval times[2];
/*			
					The MS-DOS date. The date is a packed value with the following format.

					cant use DosDateTimeToFileTime on Linux
		
					Bits	Description
					0-4	Day of the month (1–31)
					5-8	Month (1 = January, 2 = February, and so on)
					9-15	Year offset from 1980 (add 1980 to get actual year)
					wFatTime
					The MS-DOS time. The time is a packed value with the following format.
					Bits	Description
					0-4	Second divided by 2
					5-10	Minute (0–59)
					11-15	Hour (0–23 on a 24-hour clock)
*/
					memset(&TM, 0, sizeof(TM));

					TM.tm_sec = (conn->YAPPDate & 0x1f) << 1;
					TM.tm_min = ((conn->YAPPDate >> 5) & 0x3f);
					TM.tm_hour =  ((conn->YAPPDate >> 11) & 0x1f);

					TM.tm_mday =  ((conn->YAPPDate >> 16) & 0x1f);
					TM.tm_mon =  ((conn->YAPPDate >> 21) & 0xf) - 1;
					TM.tm_year = ((conn->YAPPDate >> 25) & 0x7f) + 80;

					Debugprintf("%d %d %d %d %d %d", TM.tm_year, TM.tm_mon, TM.tm_mday, TM.tm_hour, TM.tm_min, TM.tm_sec);

					times[0].tv_sec = times[1].tv_sec = mktime(&TM);
					times[0].tv_usec = times[1].tv_usec = 0;
				}
			}
#endif

			free(conn->MailBuffer);
			conn->MailBufferSize=0;
			conn->MailBuffer=0;

			if (Written != conn->YAPPLen)
			{
				Mess[0] = CAN;
				Mess[1] = 0;
				QueueMsg(conn, Mess, 2);
				Flush(conn);
				len = sprintf_s(Mess, sizeof(Mess), "Failed to save YAPP File\r");
				QueueMsg(conn, Mess, len);
				WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);
				conn->InputLen = 0;
				conn->InputMode = 0;
			}
		}

		Reply[1] = 3;		//Ack_EOF
		QueueMsg(conn, Reply, 2);
		Flush(conn);	
		conn->InputLen = 0;

		return TRUE;

	case EOT:

		// End Session

		Reply[1] = 4;		// Ack_EOT
		QueueMsg(conn, Reply, 2);
		Flush(conn);
		conn->InputLen = 0;
		conn->InputMode = 0;
	
		len = sprintf_s(Mess, sizeof(Mess), "YAPP file %s received\r", conn->ARQFilename);
		WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);
		QueueMsg(conn, Mess, len);
		SendPrompt(conn, conn->UserPointer);

		return TRUE;

	case CAN:

		// Abort

		Mess[0] = ACK;
		Mess[1] = 5;			// CAN Ack
		QueueMsg(conn, Mess, 2);
		Flush(conn);

		if (conn->MailBuffer)
		{
			free(conn->MailBuffer);
			conn->MailBufferSize=0;
			conn->MailBuffer=0;
		}

		// There may be a reason after the CAN

		len = Msg[1];

		if (len)
		{
			char * errormsg = &Msg[2];
			errormsg[len] = 0;
			nodeprintf(conn, "File Rejected - %s\r", errormsg);
		}
		else
			
			nodeprintf(conn, "File Rejected\r");


		len = sprintf_s(Mess, sizeof(Mess), "YAPP Transfer cancelled by Terminal\r");
		WriteLogLine(conn, '!', Mess, len - 1, LOG_BBS);

		conn->InputLen = 0;
		conn->InputMode = 0;
		conn->BBSFlags &= ~YAPPTX;

		return FALSE;

	case ACK:

		switch (Msg[1])
		{
		case 1:					// Rcv_Rdy

			// HD Send_Hdr     SOH  len  (Filename)  NUL  (File Size in ASCII)  NUL (Opt)
			
			len = (int)strlen(conn->ARQFilename) + 3;
		
			strcpy(&Mess[2], conn->ARQFilename);
			len += sprintf(&Mess[len], "%d", conn->MailBufferSize);
			len++;					// include null
			Mess[0] = SOH;
			Mess[1] = len - 2;

			QueueMsg(conn, Mess, len);
			Flush(conn);
			conn->InputLen = 0;

			return FALSE;

		case 2:

			//	Start sending message

			YAPPSendData(conn);
			conn->InputLen = 0;
			return FALSE;

		case 3:

			// ACK EOF - Send EOT

			
			Mess[0] = EOT;
			Mess[1] = 1;
			QueueMsg(conn, Mess, 2);
			Flush(conn);

			conn->InputLen = 0;
			return FALSE;
	
		case 4:

			// ACK EOT

			conn->InputMode = 0;
			conn->BBSFlags &= ~YAPPTX;

			conn->InputLen = 0;
			return FALSE;

		default:
			conn->InputLen = 0;
			return FALSE;



		}
	
	case NAK:

		// Either Reject or Restart

		// RE Resume       NAK  len  R  NULL  (File size in ASCII)  NULL

		if (conn->InputLen  > 2 && Msg[2] == 'R' && Msg[3] == 0)
		{
			int posn = atoi(&Msg[4]);
			
			conn->YAPPLen += posn;
			conn->MailBufferSize -= posn;

			YAPPSendData(conn);
			conn->InputLen = 0;
			return FALSE;

		}

		// There may be a reason after the ack

		len = Msg[1];

		if (len)
		{
			char * errormsg = &Msg[2];
			errormsg[len] = 0;
			nodeprintf(conn, "File Rejected - %s\r", errormsg);
		}
		else
			
			nodeprintf(conn, "File Rejected\r");

		conn->InputMode = 0;
		conn->BBSFlags &= ~YAPPTX;
		conn->InputLen = 0;
		SendPrompt(conn, conn->UserPointer);
		return FALSE;
	}

	nodeprintf(conn, "Unexpected message during YAPP Transfer. Transfer canncelled\r");

	conn->InputMode = 0;
	conn->BBSFlags &= ~YAPPTX;
	conn->InputLen = 0;
	SendPrompt(conn, conn->UserPointer);

	return FALSE;

}

void YAPPSendFile(ConnectionInfo * conn, struct UserInfo * user, char * filename)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	struct stat STAT;

	if (filename == NULL)
	{
		nodeprintf(conn, "Filename missing\r");
		SendPrompt(conn, user);
		return;
	}

	if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\'))
	{
		nodeprintf(conn, "Invalid filename\r");
		SendPrompt(conn, user);
		return;
	}

	if (BaseDir[0])
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/Files/%s", BaseDir, filename);
	else
		sprintf_s(MsgFile, sizeof(MsgFile), "Files/%s", filename);

	if (stat(MsgFile, &STAT) != -1)
	{
		FileSize = STAT.st_size;

		hFile = fopen(MsgFile, "rb");

		if (hFile)
		{	
			char Mess[255];
			strcpy(conn->ARQFilename, filename);
			conn->MailBuffer = malloc(FileSize);
			conn->MailBufferSize = FileSize;
			conn->YAPPLen = 0;
			fread(conn->MailBuffer, 1, FileSize, hFile); 
			fclose(hFile);
	
			Mess[0] = ENQ;
			Mess[1] = 1;

			QueueMsg(conn, Mess, 2);
			Flush(conn);

			conn->InputMode = 'Y';

			return;
		}
	}

	nodeprintf(conn, "File %s not found\r", filename);
	SendPrompt(conn, user);
}

void YAPPSendData(ConnectionInfo * conn)
{
	char Mess[258];

	conn->BBSFlags |= YAPPTX;

	while (TXCount(conn->BPQStream) < 15)
	{
		int Left = conn->MailBufferSize;

		if (Left == 0)
		{
			// Finished - send End Data

			Mess[0] = ETX;
			Mess[1] = 1;
			
			QueueMsg(conn, Mess, 2);
			Flush(conn);

			conn->BBSFlags &= ~YAPPTX;
			break;
		}

		if (Left > conn->paclen - 2)		// 2 byte header
			Left = conn->paclen -2;

		memcpy(&Mess[2], &conn->MailBuffer[conn->YAPPLen], Left);
		Mess[0] = STX;
		Mess[1] = Left;
				
		QueueMsg(conn, Mess, Left + 2);
		Flush(conn);

		conn->YAPPLen += Left;
		conn->MailBufferSize -= Left;
	}
}

char * AddUser(char * Call, char * password, BOOL BBSFlag)
{
	struct UserInfo * USER;

	strlop(Call, '-');

	if (strlen(Call) > 6)
		Call[6] = 0;

	_strupr(Call);
		
	if (Call[0] == 0 || LookupCall(Call))
	{
		return("User already exists\r\n");
	}

	USER = AllocateUserRecord(Call);
	USER->Temp = zalloc(sizeof (struct TempUserInfo));

	if (strlen(password) > 12)
		password[12] = 0;

	strcpy(USER->pass, password);

	if (BBSFlag)
	{
		if(SetupNewBBS(USER))
			USER->flags |= F_BBS;
		else
			printf("Cannot set user to be a BBS - you already have 160 BBS's defined\r\n");
	}

	SaveUserDatabase();
	UpdateWPWithUserInfo(USER);

	return("User added\r\n");
}

// Server Support Code

// For the moment only internal REQDIR and REQFIL.

// May add WPSERV and user implemented servers
/*
F6FBB BBS >
 SP REQDIR @ F6ABJ.FRA.EU
 Title of message :
 YAPP\*.ZIP @ F6FBB.FMLR.FRA.EU
 Text of message :
 /EX

 F6FBB BBS >
 SP REQFIL @ F6ABJ.FRA.EU
 Title of message :
 DEMOS\ESSAI.TXT @ F6FBB.FMLR.FRA.EU
 Text of message :
 /EX

 Note Text not used.

*/

VOID SendServerReply(char * Title, char * MailBuffer, int Length, char * To);

BOOL ProcessReqDir(struct MsgInfo * Msg)
{
	char * Buffer;
	int Len = 0;
	char * ptr;

	// Parse title - gives directory and return address

	// YAPP\*.ZIP @ F6FBB.FMLR.FRA.EU

	// At the moment we don't allow subdirectories but no harm handling here

	char Pattern[64];
	char * Address;
	char * filename = NULL; // ?? Pattern Match ??

#ifdef WIN32

   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;

#else

	#include <dirent.h>

	struct dirent **namelist;
    int n, i;
	struct stat STAT;
	int res;
	char FN[256];
     	
#endif

	strcpy(Pattern, Msg->title);

	ptr = strchr(Pattern, '@');

	if (ptr == NULL)

		// if we don't have return address no point
		// but could we default to sender??

		return FALSE;

	*ptr++ = 0;			// Terminate Path

	strlop(Pattern, ' ');

	while (*ptr == ' ')
		ptr++;			// accept with or without spaces round @

	Address = ptr;

	ptr = Buffer = malloc(MaxTXSize);

#ifdef WIN32

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, GetBPQDirectory());
   strcat(szDir, "\\BPQMailChat\\Files\\");
   strcat(szDir, Pattern);

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      Len = sprintf(Buffer, "No Files\r");
   } 
   else
   {
		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{}
			else	
			{
				if (filename == NULL || stristr(ffd.cFileName, filename))
					Len += sprintf(&Buffer[Len], "%s %d\r", ffd.cFileName, ffd.nFileSizeLow);
			}
		}
		while (FindNextFile(hFind, &ffd) != 0);
	
		FindClose(hFind);
	}

#else

    n = scandir("Files", &namelist, NULL, alphasort);

	if (n < 0) 
		perror("scandir");
	else  
	{ 
		for (i = 0; i < n; i++)
		{
			sprintf(FN, "Files/%s", namelist[i]->d_name);

			if (filename == NULL || stristr(namelist[i]->d_name, filename))
				if (FN[6] != '.' && stat(FN, &STAT) == 0)
					Len += sprintf(&Buffer[Len], "%s %d\r", namelist[i]->d_name, STAT.st_size);
			
			free(namelist[i]);
		}
		free(namelist);
    }

#endif

	// Build Message

	SendServerReply("REQDIR Reply", Buffer, Len, _strupr(Address));
	return TRUE;
}

/*
      '  Augment Message ID with the Message Pickup Station we're directing this message to.
        '
        Dim strAugmentedMessageID As String
        If GetMidRMS(MessageId) <> "" Then
            ' The MPS RMS is already set on the message ID
            strAugmentedMessageID = MessageId
            strMPS = GetMidRMS(MessageId)
            ' "@R" at the end of the MID means route message only via radio
            If GetMidForwarding(MessageId) = "" And (blnRadioOnly Or UploadThroughInternet()) Then
                strAugmentedMessageID &= "@" & strHFOnlyFlag
            End If
        ElseIf strMPS <> "" Then
            ' Add MPS to the message ID
            strAugmentedMessageID = MessageId & "@" & strMPS
            ' "@R" at the end of the MID means route message only via radio
            If blnRadioOnly Or UploadThroughInternet() Then
                strAugmentedMessageID &= "@" & strHFOnlyFlag
            End If
        Else
            strAugmentedMessageID = MessageId
        End If
  
*/

void ProcessSyncModeMessage(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	Buffer[len] = 0;

	if (conn->Flags & GETTINGSYNCMESSAGE)
	{
		// Data

		if ((conn->TempMsg->length + len) > conn->MailBufferSize)
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

		memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, len);

		conn->TempMsg->length += len;

		if (conn->TempMsg->length >= conn->SyncCompressedLen)
		{
			// Complete - decompress it

			conn->BBSFlags |= FBBCompressed;
			Decode(conn, 1);

			conn->Flags &= !GETTINGSYNCMESSAGE;

			BBSputs(conn, "OK\r");	
			return;
		}
		return;
	}

	if (conn->Flags & PROPOSINGSYNCMSG)
	{
		// Waiting for response to TR AddMessage

		if (strcmp(Buffer, "OK\r") == 0)
		{
			char Msg[256];
			int n;

			WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
	
			// Send the message, it has already been built

			conn->Flags &= !PROPOSINGSYNCMSG;
			conn->Flags |= SENDINGSYNCMSG;

			n = sprintf_s(Msg, sizeof(Msg), "Sending SYNC message %s", conn->FwdMsg->bid);
			WriteLogLine(conn, '|',Msg, n, LOG_BBS);

			QueueMsg(conn, conn->SyncMessage, conn->SyncCompressedLen);
			return;
		}

		if (strcmp(Buffer, "NO\r") == 0)
		{
			WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
	
			// Message Rejected - ? duplicate

			if (conn->FwdMsg)
			{
				// Zap the entry

				clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
				set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);
				conn->UserPointer->ForwardingInfo->MsgCount--;

				//  Only mark as forwarded if sent to all BBSs that should have it

				if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
				{
					conn->FwdMsg->status = 'F';			// Mark as forwarded
					conn->FwdMsg->datechanged=time(NULL);
				}

				conn->FwdMsg->Locked = 0;	// Unlock
			}
		}

		BBSputs(conn, "BYE\r");
		conn->CloseAfterFlush = 20;			// 2 Secs
		conn->Flags &= !PROPOSINGSYNCMSG;
		conn->BBSFlags &= ~SYNCMODE;
		return;
	}

	if (conn->Flags & SENDINGSYNCMSG)
	{
		if (strcmp(Buffer, "OK\r") == 0)
		{
			// Message Sent

			conn->Flags &= !SENDINGSYNCMSG;
			free(conn->SyncMessage);

			if (conn->FwdMsg)
			{
				char Msg[256];
				int n;

				n = sprintf_s(Msg, sizeof(Msg), "SYNC message %s Sent", conn->FwdMsg->bid);
				WriteLogLine(conn, '|',Msg, n, LOG_BBS);

				clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
				set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);
				conn->UserPointer->ForwardingInfo->MsgCount--;

				//  Only mark as forwarded if sent to all BBSs that should have it

				if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
				{
					conn->FwdMsg->status = 'F';			// Mark as forwarded
					conn->FwdMsg->datechanged=time(NULL);
				}

				conn->FwdMsg->Locked = 0;	// Unlock
			}

			// drop through to send any more
		}
		else
		{
			WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
	
			conn->Flags &= !SENDINGSYNCMSG;
			free(conn->SyncMessage);

			BBSputs(conn, "BYE\r");
			conn->CloseAfterFlush = 20;			// 2 Secs
			conn->BBSFlags &= ~SYNCMODE;

			return;
		}
	}

	if (strcmp(Buffer, "OK\r") == 0)
	{
		WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);

		// Send Message(?s) to RMS Relay SYNC

/*
<POSYNCLOGON G8BPQ
>OK
>TR AddMessage_V5JLSGH591JR 786 1219 522 True
<OK
.. message
<OK
?? repeat
>BYE*/
		if (FindMessagestoForward(conn) && conn->FwdMsg)
		{
			struct MsgInfo * Msg = conn->FwdMsg;
			char Buffer[128];
			char * Message;

			Message = FormatSYNCMessage(conn, Msg);

			// Need to compress it

			conn->SyncMessage = malloc(conn->SyncXMLLen + conn->SyncMsgLen + 4096);

			conn->SyncCompressedLen = Encode(Message, conn->SyncMessage, conn->SyncXMLLen + conn->SyncMsgLen, 0, 1);

			sprintf(Buffer, "TR AddMessage_%s %d %d %d True\r",		// The True on end indicates compressed
				Msg->bid, conn->SyncCompressedLen, conn->SyncXMLLen, conn->SyncMsgLen);

			free(Message);

			conn->Flags |= PROPOSINGSYNCMSG;

			BBSputs(conn, Buffer);
			return;
		}
				

		BBSputs(conn, "BYE\r");
		conn->CloseAfterFlush = 20;			// 2 Secs
		conn->BBSFlags &= ~SYNCMODE;
		return;
	}

	if (memcmp(Buffer, "TR ", 2) == 0) 
	{
		// Messages have TR_COMMAND_BID Compressed Len XML Len Bosy Len

		char * Command;
		char * BIDptr;

		BIDRec * BID;
		char *ptr1, *ptr2, *context;

		//		TR AddMessage_1145_G8BPQ 727 1202 440 True

		WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
		
		Command = strtok_s(&Buffer[3], "_", &context);
		BIDptr = strtok_s(NULL, " ", &context);
		ptr2 =  strtok_s(NULL, " ", &context);
		conn->SyncCompressedLen = atoi(ptr2);
		ptr2 =  strtok_s(NULL, " ", &context);
		conn->SyncXMLLen = atoi(ptr2);
		ptr2 =  strtok_s(NULL, " ", &context);
		conn->SyncMsgLen = atoi(ptr2);
		ptr2 =  strtok_s(NULL, " ", &context);

		// If addmessage need to check bid doesn't exist

		if (strcmp(Command, "AddMessage") == 0)
		{
			strlop(BIDptr, '@');			// sometimes has @CALL@R
			if (strlen(BIDptr) > 12)
				BIDptr[12] = 0;

			BID = LookupBID(BIDptr);

			if (BID)
			{
				BBSputs(conn, "Rejected - Duplicate BID\r");
				return;
			}
		}

		conn->TempMsg = zalloc(sizeof(struct MsgInfo));

		conn->Flags |= GETTINGSYNCMESSAGE;

		BBSputs(conn, "OK\r");
		return;
	}

	if (memcmp(Buffer, "BYE\r", 4) == 0)
	{
		WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
		conn->CloseAfterFlush = 20;			// 2 Secs
		conn->BBSFlags &= ~SYNCMODE;
		return;
	}

	if (memcmp(Buffer, "BBS\r", 4) == 0)
	{
		// Out of Sync

		WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
		conn->BBSFlags &= ~SYNCMODE;
		return;
	}

	WriteLogLine(conn, '<', Buffer, len-1, LOG_BBS);
	WriteLogLine(conn, '<', "Unexpected SYNC Message", 23, LOG_BBS);

	BBSputs(conn, "BYE\r");
	conn->CloseAfterFlush = 20;			// 2 Secs
	conn->BBSFlags &= ~SYNCMODE;
	return;	
}
BOOL ProcessReqFile(struct MsgInfo * Msg)
{
	char FN[128];
	char * Buffer;
	int Len = 0;
	char * ptr;
	struct stat STAT;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	int FileSize;
	char * MsgBytes;

	// Parse title - gives file and return address

	// DEMOS\ESSAI.TXT @ F6FBB.FMLR.FRA.EU

	// At the moment we don't allow subdirectories but no harm handling here

	char * Address;
	char * filename = NULL; // ?? Pattern Match ??

	strcpy(FN, Msg->title);

	ptr = strchr(FN, '@');

	if (ptr == NULL)

		// if we don't have return address no point
		// but could we default to sender??

		return FALSE;

	*ptr++ = 0;			// Terminate Path

	strlop(FN, ' ');

	while (*ptr == ' ')
		ptr++;			// accept with or without spaces round @

	Address = ptr;

	ptr = Buffer = malloc(MaxTXSize + 1);	// Allow terminating Null

	// Build Message

	if (FN == NULL)
	{
		Len = sprintf(Buffer, "Missing Filename\r");
	}
	else if (strstr(FN, "..") || strchr(FN, '/') || strchr(FN, '\\'))
	{
		Len = sprintf(Buffer,"Invalid filename %s\r", FN);
	}
	else
	{
		if (BaseDir[0])
			sprintf_s(MsgFile, sizeof(MsgFile), "%s/Files/%s", BaseDir, FN);
		else
			sprintf_s(MsgFile, sizeof(MsgFile), "Files/%s", FN);

		if (stat(MsgFile, &STAT) != -1)
		{
			FileSize = STAT.st_size;

			hFile = fopen(MsgFile, "rb");

			if (hFile)
			{
				int Length;

				if (FileSize > MaxTXSize)
					FileSize = MaxTXSize;		// Truncate to max size
	
				MsgBytes=malloc(FileSize+1);
				fread(MsgBytes, 1, FileSize, hFile); 
				fclose(hFile);

				MsgBytes[FileSize]=0;

				// Remove lf chars

				Length = RemoveLF(MsgBytes, (int)strlen(MsgBytes));

				Len = sprintf(Buffer, "%s", MsgBytes);
				free(MsgBytes);
			}
		}
		else
			Len = sprintf(Buffer, "File %s not found\r", FN);
	}

	SendServerReply("REQFIL Reply", Buffer, Len, _strupr(Address));
	return TRUE;
}

BOOL CheckforMessagetoServer(struct MsgInfo * Msg)
{
	if (_stricmp(Msg->to, "REQDIR") == 0)
		return ProcessReqDir(Msg);

	if (_stricmp(Msg->to, "REQFIL") == 0)
		return ProcessReqFile(Msg);

	return FALSE;
}

VOID SendServerReply(char * Title, char * MailBuffer, int Length, char * To)
{
	struct MsgInfo * Msg = AllocateMsgRecord();
	BIDRec * BIDRec;
	char * Via;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	size_t WriteLen=0;

	Msg->length = Length;

	GetSemaphore(&MsgNoSemaphore, 0);
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;

	FreeSemaphore(&MsgNoSemaphore);
 
	strcpy(Msg->from, BBSName);
	Via = strlop(To, '@');

	if (Via)
		strcpy(Msg->via, Via);

	strcpy(Msg->to, To);
	strcpy(Msg->title, Title);

	Msg->type = 'P';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	BIDRec = AllocateBIDRecord();
	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	hFile = fopen(MsgFile, "wb");

	if (hFile)
	{
		WriteLen = fwrite(MailBuffer, 1, Msg->length, hFile);
		fclose(hFile);
	}

	MatchMessagetoBBSList(Msg, NULL);
	free(MailBuffer);
}

void SendRequestSync(CIRCUIT * conn)
{
	// Only need XML Header

	char * Buffer = malloc(4096);
	int Len = 0;
	
	struct tm *tm;
	char Date[32];
	char MsgTime[32];
	time_t Time = time(NULL);

	char * Encoded;

	tm = gmtime(&Time);

	sprintf_s(Date, sizeof(Date), "%04d%02d%02d%02d%02d%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	sprintf_s(MsgTime, sizeof(Date), "%04d/%02d/%02d %02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	Len += sprintf(&Buffer[Len], "<?xml version=\"1.0\"?>\r\n");

	Len += sprintf(&Buffer[Len], "<sync_record>\r\n");
	Len += sprintf(&Buffer[Len], "  <po_sync>\r\n");
    Len += sprintf(&Buffer[Len], "    <transaction_type>request_sync</transaction_type>\r\n");
    Len += sprintf(&Buffer[Len], "    <timestamp>%s</timestamp>\r\n", Date);
    Len += sprintf(&Buffer[Len], "    <originating_station>%s</originating_station>\r\n", BBSName);
	Len += sprintf(&Buffer[Len], "  </po_sync>\r\n");
 	Len += sprintf(&Buffer[Len], "  <request_sync>\r\n");
 	Len += sprintf(&Buffer[Len], "    <callsign>BBSName</callsign>\r\n");
 	Len += sprintf(&Buffer[Len], "    <password></password>\r\n");
	Len += sprintf(&Buffer[Len], "    <ip_address>%s</ip_address>\r\n", conn->SyncHost);
	Len += sprintf(&Buffer[Len], "    <ip_port>%d</ip_port>\r\n", conn->SyncPort);
 	Len += sprintf(&Buffer[Len], "    <note></note>\r\n");
 	Len += sprintf(&Buffer[Len], "  </request_sync>\r\n");
 	Len += sprintf(&Buffer[Len], "</sync_record>\r\n");
 
/*
<?xml version="1.0"?>
<sync_record>
  <po_sync>
    <transaction_type>request_sync</transaction_type>
    <timestamp>20230205100652</timestamp>
    <originating_station>GI8BPQ</originating_station>
  </po_sync>
  <request_sync>
    <callsign>GI8BPQ</callsign>
    <password></password>
    <ip_address>127.0.0.1</ip_address>
    <ip_port>8780</ip_port>
    <note></note>
  </request_sync>
</sync_record>
*/

	// Need to compress it

	conn->SyncXMLLen = Len;
	conn->SyncMsgLen = 0;

	conn->SyncMessage = malloc(conn->SyncXMLLen + 4096);

	conn->SyncCompressedLen = Encode(Buffer, conn->SyncMessage, conn->SyncXMLLen, 0, 1);

	sprintf(Buffer, "TR RequestSync_%s_%d %d %d 0 True\r",		// The True on end indicates compressed
				50, conn->SyncCompressedLen, conn->SyncXMLLen);

	free(Buffer);

	conn->Flags |= REQUESTINGSYNC;

	BBSputs(conn, Buffer);
	return;
}


void ProcessSyncXML(CIRCUIT * conn, char * XML)
{
	// Process XML from RMS Relay Sync

	// All seem to start 

	//<?xml version="1.0"?>
	//<sync_record>
	// <po_sync>
	//  <transaction_type>

	char * Type = strstr(XML, "<transaction_type>");

	if (Type == NULL)
		return;

	Type += strlen("<transaction_type>");

	if (memcmp(Type, "rms_location", 12) == 0)
	{
		return;
	}


	if (memcmp(Type, "request_sync", 12) == 0)
	{
		char * Call;
		struct UserInfo * BBSREC;

		// This isn't requesting a poll, it is asking to be added as a sync partner

		Call = strstr(Type, "<callsign>");

		if (Call == NULL)
			return;
	
		Call += 10;
		strlop(Call, '<');
		BBSREC = FindBBS(Call);

		if (BBSREC == NULL)
			return;

		if (BBSREC->ForwardingInfo->Forwarding == 0)
			StartForwarding(BBSREC->BBSNumber, NULL);

		return;
	}

	if (memcmp(Type, "remove_message", 14) == 0)
	{
		char * MID = strstr(Type, "<MessageId>");
		struct MsgInfo * Msg;

		if (MID == NULL)
			return;
	
		MID += 11;
		strlop(MID, '<');

		strlop(MID, '@');			// sometimes has @CALL@R
		if (strlen(MID) > 12)
			MID[12] = 0;

		Msg = FindMessageByBID(MID);

		if (Msg == NULL)
			return;

		Logprintf(LOG_BBS, conn, '|', "Killing Msg %d %s", Msg->number, Msg->bid);

		FlagAsKilled(Msg, TRUE);
		return;
	}

	if (memcmp(Type, "delivered", 9) == 0)
	{
		char * MID = strstr(Type, "<MessageId>");
		struct MsgInfo * Msg;

		if (MID == NULL)
			return;
	
		MID += 11;
		strlop(MID, '<');

		strlop(MID, '@');			// sometimes has @CALL@R
		if (strlen(MID) > 12)
			MID[12] = 0;

		Msg = FindMessageByBID(MID);

		if (Msg == NULL)
			return;

		Logprintf(LOG_BBS, conn, '|', "Message Msg %d %s Delivered", Msg->number, Msg->bid);
		return;
	}

	Debugprintf(Type);
	return;

/*
<?xml version="1.0"?>
<sync_record>
  <po_sync>
    <transaction_type>request_sync</transaction_type>
    <timestamp>20230205100652</timestamp>
    <originating_station>GI8BPQ</originating_station>
  </po_sync>
  <request_sync>
    <callsign>GI8BPQ</callsign>
    <password></password>
    <ip_address>127.0.0.1</ip_address>
    <ip_port>8780</ip_port>
    <note></note>
  </request_sync>
</sync_record>
}

<sync_record>
  <po_sync>
    <transaction_type>delivered</transaction_type>
    <timestamp>20230205093113</timestamp>
    <originating_station>G8BPQ</originating_station>
  </po_sync>
  <delivered>
    <MessageId>10845_GM8BPB</MessageId>
    <Destination>G8BPQ</Destination>
    <ForwardedTo>G8BPQ</ForwardedTo>
    <DeliveredVia>3</DeliveredVia>
  </delivered>
</sync_record>

	Public Enum MessageDeliveryMethod
    '
    ' Method used to deliver a message.  None if the message hasn't been delivered.
    '
    Unspecified = -1
    None = 0
    Telnet = 1
    CMS = 2
    Radio = 3
    Email = 4
End Enum
*/
}

int ReformatSyncMessage(CIRCUIT * conn)
{
	// Message has been decompressed - reformat to look like a WLE message

	char * MsgBit;
	char *ptr1, *ptr2;
	int linelen;
	char FullFrom[80];
	char FullTo[80];
	char BID[80];
	time_t Date;
	char Mon[80];
	char Subject[80];
	int i = 0;
	char * Boundary;
	char * Input;
	char * via = NULL;
	char * NewMsg = conn->MailBuffer;
	char * SaveMsg = NewMsg;
	char DateString[80];
	struct tm * tm;
	char Type[16] = "Private";
	char * part[100] = {""};
	char * partname[100];
	int partLen[100];
	char xml[4096];

	// Message has an XML header then the message

	// The XML may have control info, so examine it.
 
	/*
	Date: Mon, 25 Oct 2021 10:22:00 -0000
	From: GM8BPQ
	Subject: Test
	To: 2E1BGT
	Message-ID: ALYJQJRXVQAO
	X-Source: GM8BPQ
	X-Relay: G8BPQ
	MIME-Version: 1.0
	MIME-Version: 1.0
	Content-Type: multipart/mixed; boundary="boundaryBSoxlw=="

	--boundaryBSoxlw==
	Content-Type: text/plain; charset="iso-8859-1"
	Content-Transfer-Encoding: quoted-printable

	Hello Hello

	--boundaryBSoxlw==--
	*/

	// I think the best way is to reformat as if from Winlink Express, then pass 
	//through the normal B2 code.

//	WriteLogLine(conn, '<', conn->MailBuffer, conn->TempMsg->length, LOG_BBS);

	// display the message  for testing

	conn->MailBuffer[conn->TempMsg->length] = 0;

//	OutputDebugString(conn->MailBuffer);
	memcpy(xml, conn->MailBuffer, conn->SyncXMLLen);
	xml[conn->SyncXMLLen] = 0;

	if (conn->SyncMsgLen == 0)
	{
		// No message, Just xml. Looks like a status report

		ProcessSyncXML(conn, xml);
		return 0;
	}
	
	MsgBit = &conn->MailBuffer[conn->SyncXMLLen];
	conn->TempMsg->length -= conn->SyncXMLLen;

	ptr1 = MsgBit;

Loop:

	ptr2 = strchr(ptr1, '\r');

	linelen = (int)(ptr2 - ptr1);

	if (_memicmp(ptr1, "From:", 5) == 0)
	{
		memcpy(FullFrom, &ptr1[6], linelen - 6);
		FullFrom[linelen - 6] = 0;
	}

	if (_memicmp(ptr1, "To:", 3) == 0)
	{
		memcpy(FullTo, &ptr1[4], linelen - 4);
		FullTo[linelen - 4] = 0;
	}

	else if (_memicmp(ptr1, "Subject:", 8) == 0)
	{
		memcpy(Subject, &ptr1[9], linelen - 9);
		Subject[linelen - 9] = 0;
	}

	else if (_memicmp(ptr1, "Message-ID", 10) == 0)
	{
		memcpy(BID, &ptr1[12], linelen - 12);
		BID[linelen - 12] = 0;
	}

	else if (_memicmp(ptr1, "Date:", 5) == 0)
	{
		struct tm rtime;
		char seps[] = " ,\t\r";

		memset(&rtime, 0, sizeof(struct tm));

		// Date: Mon, 25 Oct 2021 10:22:00 -0000

		sscanf(&ptr1[11], "%02d %s %04d %02d:%02d:%02d",
			&rtime.tm_mday, &Mon, &rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec);

		rtime.tm_year -= 1900;

		for (i = 0; i < 12; i++)
		{
			if (strcmp(Mon, month[i]) == 0)
				break;
		}

		rtime.tm_mon = i;

		Date = mktime(&rtime) - (time_t)_MYTIMEZONE;

		if (Date == (time_t)-1)
			Date = time(NULL);

	}

	if (linelen)			// Not Null line
	{
		ptr1 = ptr2 + 2;		// Skip crlf
		goto Loop;
	}

	// Unpack Body - seems to be multipart even if only one

	// Can't we just send the whole body through ??
	// No, Attachment format is different

	// Mbo: GM8BPQ
	// Body: 17
	// File: 1471 leadercoeffs.txt

	Input = MsgBit;
	Boundary = initMultipartUnpack(&Input);
	
	i = 0;

	if (Boundary)
	{
		// input should be start of part

		// Find End of part - ie -- Boundary + CRLF or --

		char * ptr, * saveptr;
		char * Msgptr;
		size_t BLen = strlen(Boundary);
		size_t Partlen;

		saveptr = Msgptr = ptr = Input;

		while(ptr)				// Just in case we run off end
		{
			if (*ptr == '-' && *(ptr+1) == '-')
			{
				if (memcmp(&ptr[2], Boundary, BLen) == 0)
				{
					// Found Boundary

					char * p1, *p2, *ptr3, *ptr4;
					int llen;
					int Base64 = 0;
					int QuotedP = 0;
					char * BoundaryStart = ptr;

					Partlen = ptr - Msgptr;

					ptr += (BLen + 2);			// End of Boundary

					if (*ptr == '-')			// Terminating Boundary
						Input = NULL;
					else
						Input = ptr + 2;

					// Will check for quoted printable

					p1 = Msgptr;
Loop2:
					p2 = strchr(p1, '\r');
					llen = (int)(p2 - p1);

					if (llen)
					{

						if (_memicmp(p1, "Content-Transfer-Encoding:", 26) == 0)
						{
							if (_memicmp(&p1[27], "base64", 6) == 0)
								Base64 = TRUE;
							else if (_memicmp(&p1[27], "quoted", 6) == 0)
								QuotedP = TRUE;
						}
						else if (_memicmp(p1, "Content-Disposition: ", 21) == 0)
						{
							ptr3 = strstr(&p1[21], "name");

							if (ptr3)
							{
								ptr3 += 5;
								if (*ptr3 == '"') ptr3++;
								ptr4 = strchr(ptr3, '"');
								if (ptr4) *ptr4 = 0;

								partname[i] = ptr3;
							}
						}

						if (llen)			// Not Null line
						{
							p1 = p2 + 2;		// Skip crlf
							goto Loop2;
						}
					}

					part[i] = strstr(p2, "\r\n");	// Over separator
			
					if (part[i])
					{
						part[i] += 2;
						partLen[i] = BoundaryStart - part[i] - 2;
						if (QuotedP)
							partLen[i] = decode_quoted_printable(part[i], partLen[i]);
						else if (Base64)
						{
							int Len = partLen[i], NewLen;
							char * ptr = part[i];
							char * ptr2 = part[i];

							// WLE sends base64 with embedded crlf, so remove them

							while (Len-- > 0)
							{
								if ((*ptr) != 10 && (*ptr) != 13)
									*(ptr2++) = *(ptr++);
								else
									ptr ++;
							}

							Len = ptr2 - part[i];
							ptr = part[i];
							ptr2 = part[i];
							
							while (Len > 0)
							{
								decodeblock(ptr, ptr2);
								ptr += 4;
								ptr2 += 3;
								Len -= 4;
							}

							NewLen = (int)(ptr2 - part[i]);

							if (*(ptr-1) == '=')
								NewLen--;

							if (*(ptr-2) == '=')
								NewLen--;

							partLen[i] = NewLen;
						}
					}
					Msgptr = ptr = Input;
					i++;
					continue;				}

				// See if more parts
			}
			ptr++;
		}
		ptr++;
	}


	// Build the message

	tm = gmtime(&Date);	

	sprintf(DateString, "%04d/%02d/%02d %02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	NewMsg += sprintf(NewMsg,
		"MID: %s\r\n"
		"Date: %s\r\n"
		"Type: %s\r\n"
		"From: %s\r\n",
			BID, DateString, Type, FullFrom);

//	if (ToCalls)
//	{
//		int i;

//		for (i = 0; i < Calls; i++)
//			NewMsg += sprintf(NewMsg, "To: %s\r\n",	ToCalls[i]);

//	}
//	else
	{
		NewMsg += sprintf(NewMsg, "To: %s\r\n",
			FullTo);
	}
//	if (WebMail->CC && WebMail->CC[0])
//		NewMsg += sprintf(NewMsg, "CC: %s\r\n", WebMail->CC);

	NewMsg += sprintf(NewMsg,
		"Subject: %s\r\n"
		"Mbo: %s\r\n",
		Subject, BBSName);

	// Write the Body: line and any File Lines

	NewMsg += sprintf(NewMsg, "Body: %d\r\n", partLen[0]);

	i = 1;

	while (part[i])
	{
		NewMsg += sprintf(NewMsg, "File: %d %s\r\n",
			partLen[i], partname[i]);

		i++;
	}

	NewMsg += sprintf(NewMsg, "\r\n");		// Blank Line to end header

	// Now add parts

	i = 0;

	while (part[i])
	{
		memmove(NewMsg, part[i], partLen[i]);
		NewMsg += partLen[i];
		i++;
		NewMsg += sprintf(NewMsg, "\r\n");		// Blank Line between attachments
	}

	conn->TempMsg->length = NewMsg - SaveMsg;
	conn->TempMsg->datereceived = conn->TempMsg->datechanged = time(NULL);
	conn->TempMsg->datecreated = Date;
	strcpy(conn->TempMsg->bid, BID);

	if (strlen(Subject) > 60)
		Subject[60] = 0;

	strcpy(conn->TempMsg->title, Subject);

	return TRUE;
}

char * FormatSYNCMessage(CIRCUIT * conn, struct MsgInfo * Msg)
{
	// First an XML Header

	char * Buffer = malloc(4096 + Msg->length);
	int Len = 0;
	
	struct tm *tm;
	char Date[32];
	char MsgTime[32];
	char Separator[33]="";
	time_t Time = time(NULL);
	char * MailBuffer;
	int BodyLen;
	char * Encoded;

	// Get the message - may need length in header

	MailBuffer = ReadMessageFile(Msg->number);

	BodyLen = Msg->length;

	// Remove any B2 Header

	if (Msg->B2Flags & B2Msg)
	{
		// Remove B2 Headers (up to the File: Line)
			
		char * ptr;
		ptr = strstr(MailBuffer, "Body:");
		if (ptr)
		{
			BodyLen = atoi(ptr + 5);
			ptr = strstr(ptr, "\r\n\r\n");
		}
		if (ptr)
		{
			memcpy(MailBuffer, ptr + 4, BodyLen);
			MailBuffer[BodyLen] = 0;
		}
	}

	// encode body as quoted printable;

	Encoded = malloc(Msg->length * 3);

	BodyLen = encode_quoted_printable(MailBuffer, Encoded, BodyLen);

	// Create multipart Boundary
	
	CreateOneTimePassword(&Separator[0], "Key", 0); 
	CreateOneTimePassword(&Separator[16], "Key", 1); 


	tm = gmtime(&Time);

	sprintf_s(Date, sizeof(Date), "%04d%02d%02d%02d%02d%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	tm = gmtime((time_t *)&Msg->datecreated);

	sprintf_s(MsgTime, sizeof(Date), "%04d/%02d/%02d %02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	Len += sprintf(&Buffer[Len], "<?xml version=\"1.0\"?>\r\n");

	Len += sprintf(&Buffer[Len], "<sync_record>\r\n");
	Len += sprintf(&Buffer[Len], "  <po_sync>\r\n");
    Len += sprintf(&Buffer[Len], "    <transaction_type>add_message</transaction_type>\r\n");
    Len += sprintf(&Buffer[Len], "    <timestamp>%s</timestamp>\r\n", Date);
    Len += sprintf(&Buffer[Len], "    <originating_station>%s</originating_station>\r\n", Msg->from);
	Len += sprintf(&Buffer[Len], "  </po_sync>\r\n");
 	Len += sprintf(&Buffer[Len], " <add_message>\r\n");
 	Len += sprintf(&Buffer[Len], "   <register_entry>\r\n");
	Len += sprintf(&Buffer[Len], "     <MessageId>%s</MessageId>\r\n", Msg->bid);
 	Len += sprintf(&Buffer[Len], "     <Time>%s</Time>\r\n", MsgTime);
 	Len += sprintf(&Buffer[Len], "     <Sender>%s</Sender>\r\n", Msg->from);
  	Len += sprintf(&Buffer[Len], "     <Source>%s</Source>\r\n", Msg->from);
	Len += sprintf(&Buffer[Len], "     <Precedence>2</Precedence>\r\n");
	Len += sprintf(&Buffer[Len], "     <Attachment>%s</Attachment>\r\n", (Msg->B2Flags & Attachments) ? "true" : "false");
	Len += sprintf(&Buffer[Len], "     <CSize>%d</CSize>\r\n", BodyLen);
	Len += sprintf(&Buffer[Len], "     <Subject>%s</Subject>\r\n", Msg->title);
	Len += sprintf(&Buffer[Len], "     <RMSOriginator></RMSOriginator>\r\n");
	Len += sprintf(&Buffer[Len], "     <RMSDestination></RMSDestination>\r\n");
	Len += sprintf(&Buffer[Len], "   </register_entry>\r\n");
	Len += sprintf(&Buffer[Len], "   <destinations>\r\n");
 	Len += sprintf(&Buffer[Len], "    <destination>\r\n");
 	Len += sprintf(&Buffer[Len], "      <MessageId>%s</MessageId>\r\n", Msg->bid);
	Len += sprintf(&Buffer[Len], "      <Priority>450443</Priority>\r\n");
	Len += sprintf(&Buffer[Len], "      <Destination>%s</Destination>\r\n", Msg->to);
 	Len += sprintf(&Buffer[Len], "      <ForwardedTo></ForwardedTo>\r\n");
 	Len += sprintf(&Buffer[Len], "      <DeliveredVia>0</DeliveredVia>\r\n");
	Len += sprintf(&Buffer[Len], "      <SentToCMS>False</SentToCMS>\r\n");
	Len += sprintf(&Buffer[Len], "      <SentViaRadio>False</SentViaRadio>\r\n");
	Len += sprintf(&Buffer[Len], "      <SentViaTelnet>False</SentViaTelnet>\r\n");
	Len += sprintf(&Buffer[Len], "      <LocalOnly>False</LocalOnly>\r\n");
	Len += sprintf(&Buffer[Len], "      </destination>\r\n");
	Len += sprintf(&Buffer[Len], "    </destinations>\r\n");
 	Len += sprintf(&Buffer[Len], "   <misc>\r\n");
	Len += sprintf(&Buffer[Len], "      <Local>True</Local>\r\n");
	Len += sprintf(&Buffer[Len], "      <LocalOnly>False</LocalOnly>\r\n");
	Len += sprintf(&Buffer[Len], "    </misc>\r\n");
	Len += sprintf(&Buffer[Len], "  </add_message>\r\n");
	Len += sprintf(&Buffer[Len], "</sync_record>\r\n");

//	Debugprintf(Buffer);

	conn->SyncXMLLen = Len;

	Len += sprintf(&Buffer[Len], "Date: Sat, 04 Feb 2023 11:19:00 +0000\r\n");
	Len += sprintf(&Buffer[Len], "From: %s\r\n", Msg->from);
	Len += sprintf(&Buffer[Len], "Subject: %s\r\n", Msg->title);
	Len += sprintf(&Buffer[Len], "To: %s\r\n", Msg->to);
	Len += sprintf(&Buffer[Len], "Message-ID: %s\r\n", Msg->bid);
//	Len += sprintf(&Buffer[Len], "X-Source: G8BPQ\r\n");
//	Len += sprintf(&Buffer[Len], "X-Location: 52.979167N, 1.125000W (GRID SQUARE)\r\n");
//	Len += sprintf(&Buffer[Len], "X-RMS-Originator: G8BPQ\r\n");
//	Len += sprintf(&Buffer[Len], "X-RMS-Path: G8BPQ@2023-02-04-11:19:29\r\n");
	Len += sprintf(&Buffer[Len], "X-Relay: %s\r\n", BBSName);

	Len += sprintf(&Buffer[Len], "MIME-Version: 1.0\r\n");
	Len += sprintf(&Buffer[Len], "Content-Type: multipart/mixed; boundary=\"%s\"\r\n", Separator);

	Len += sprintf(&Buffer[Len], "\r\n");		// Blank line before separator
	Len += sprintf(&Buffer[Len], "--%s\r\n", Separator);
	Len += sprintf(&Buffer[Len], "Content-Type: text/plain; charset=\"iso-8859-1\"\r\n");
	Len += sprintf(&Buffer[Len], "Content-Transfer-Encoding: quoted-printable\r\n");
	Len += sprintf(&Buffer[Len], "\r\n");		// Blank line before body

	Len += sprintf(&Buffer[Len], "%s\r\n", Encoded);	
	Len += sprintf(&Buffer[Len], "--%s--\r\n", Separator);

	conn->SyncMsgLen = Len - conn->SyncXMLLen;

	free(Encoded);
	free(MailBuffer);

	return Buffer;
}

int encode_quoted_printable(char *s, char * out, int Len)
{
  int n = 0;
  char * start = out;

  while(Len--)
  {
    if (n >= 73 && *s != 10 && *s != 13)
		{strcpy(out, "=\r\n"); n = 0; out +=3;}
    if (*s == 10 || *s == 13) {putchar(*s); n = 0;}
    else if (*s<32 || *s==61 || *s>126)
		out += sprintf(out, "=%02x", (unsigned char)*s);
    else if (*s != 32 || (*(s+1) != 10 && *(s+1) != 13))
		{*(out++) = *s; n++;}
    else n += printf("=20");

	s++;
  }
  *out = 0;

  return out - start;
}

int decode_quoted_printable(char *ptr, int len)
{
	// overwrite input with decoded version

	char * ptr2 = ptr;
	char * End = ptr + len;
	char * Start = ptr;

	while (ptr < End)
	{
		if ((*ptr) == '=')
		{
			char c = *(++ptr);
			char d;

			c = c - 48;
			if (c < 0)
			{
				// = CRLF as a soft break

				ptr += 2;
				continue;
			}
			if (c > 9) c -= 7;
			d  = *(++ptr);
			d = d - 48;
			if (d > 9) d -= 7;

			*(ptr2) = c << 4 | d;
			ptr2++;	
			ptr++;
		}
		else
			*ptr2++ = *ptr++;
	}
	return ptr2 - Start;
}
