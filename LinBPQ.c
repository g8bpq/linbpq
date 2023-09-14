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

// Control Routine for LinBPQ 

#define _CRT_SECURE_NO_DEPRECATE

#include "CHeaders.h"
#include "bpqmail.h"
#ifdef WIN32
#include <Iphlpapi.h>
//#include "C:\Program Files (x86)\GnuWin32\include\iconv.h"
#else
#include <iconv.h>
#ifndef MACBPQ
#ifndef FREEBSD
#include <sys/prctl.h>
#endif
#endif
#endif

#include "time.h"

#define Connect(stream) SessionControl(stream,1,0)
#define Disconnect(stream) SessionControl(stream,2,0)
#define ReturntoNode(stream) SessionControl(stream,3,0)
#define ConnectUsingAppl(stream, appl) SessionControl(stream, 0, appl)

BOOL APIENTRY Rig_Init();

void GetSemaphore(struct SEM * Semaphore, int ID);
void FreeSemaphore(struct SEM * Semaphore);
VOID CopyConfigFile(char * ConfigName);
VOID SendMailForThread(VOID * Param);
VOID GetUIConfig();
Dll BOOL APIENTRY Init_IP();
VOID OpenReportingSockets();
VOID SetupNTSAliases(char * FN);
int DeleteRedundantMessages();
BOOL InitializeTNCEmulator();
VOID FindLostBuffers();
VOID IPClose();
DllExport BOOL APIENTRY Rig_Close();
Dll BOOL APIENTRY Poll_IP();
BOOL Rig_Poll();
BOOL Rig_Poll();
VOID CheckWL2KReportTimer();
VOID TNCTimer();
VOID SendLocation();
int ChatPollStreams();
void ChatTrytoSend();
VOID BBSSlowTimer();
int GetHTMLForms();
char * AddUser(char * Call, char * password, BOOL BBSFlag);
VOID SaveChatConfigFile(char * ConfigName);
VOID SaveMH();
int upnpClose();
void SaveAIS();
void initAIS();
void DRATSPoll();

extern uint64_t timeLoadedMS;

BOOL IncludesMail = FALSE;
BOOL IncludesChat = FALSE;

BOOL RunMail = FALSE;
BOOL RunChat = FALSE;
BOOL needAIS= FALSE;
BOOL needADSB = FALSE;

int CloseOnError = 0;

VOID Poll_AGW();
BOOL AGWAPIInit();
int AGWAPITerminate();

BOOL AGWActive = FALSE;

extern int AGWPort;

BOOL RigActive = FALSE;

extern ULONG ChatApplMask;
extern char Verstring[];

extern char SignoffMsg[];
extern char AbortedMsg[];
extern char InfoBoxText[];			// Text to display in Config Info Popup

extern int LastVer[4];				// In case we need to do somthing the first time a version is run

extern HWND MainWnd;
extern char BaseDir[];
extern char BaseDirRaw[];
extern char MailDir[];
extern char WPDatabasePath[];
extern char RlineVer[50];

extern BOOL LogBBS;
extern BOOL LogCHAT;
extern BOOL LogTCP;
extern BOOL ForwardToMe;

extern int LatestMsg;
extern char BBSName[];
extern char SYSOPCall[];
extern char BBSSID[];
extern char NewUserPrompt[];

extern int	NumberofStreams;
extern int MaxStreams;
extern ULONG BBSApplMask;
extern int BBSApplNum;
extern int ChatApplNum;
extern int MaxChatStreams;

extern int NUMBEROFTNCPORTS;

extern int EnableUI;

extern  BOOL AUTOSAVEMH;

extern FILE * LogHandle[4];

#define MaxSockets 64

extern ConnectionInfo Connections[MaxSockets+1];

time_t LastTrafficTime;
extern int MaintTime;

#define LOG_BBS 0
#define LOG_CHAT 1
#define LOG_TCP 2
#define LOG_DEBUG_X 3

int _MYTIMEZONE = 0;

// flags equates

#define F_Excluded   0x0001
#define F_LOC        0x0002
#define F_Expert     0x0004
#define F_SYSOP      0x0008
#define F_BBS        0x0010
#define F_PAG        0x0020
#define F_GST        0x0040
#define F_MOD        0x0080
#define F_PRV        0x0100
#define F_UNP        0x0200
#define F_NEW        0x0400
#define F_PMS        0x0800
#define F_EMAIL      0x1000
#define F_HOLDMAIL   0x2000
#define F_POLLRMS	 0x4000
#define F_SYSOP_IN_LM 0x8000
#define F_Temp_B2_BBS 0x00010000

/* #define F_PWD        0x1000 */


UCHAR BPQDirectory[260];
UCHAR LogDirectory[260];

BOOL GetConfig(char * ConfigName);
VOID DecryptPass(char * Encrypt, unsigned char * Pass, unsigned int len);
int EncryptPass(char * Pass, char * Encrypt);
int APIENTRY FindFreeStream();
int PollStreams();
int APIENTRY SetAppl(int stream, int flags, int mask);
int APIENTRY SessionState(int stream, int * state, int * change);
int APIENTRY SessionControl(int stream, int command, int Mask);

BOOL ChatInit();
VOID CloseChat();
VOID CloseTNCEmulator();

static config_t cfg;
static config_setting_t * group;

BOOL MonBBS = TRUE;
BOOL MonCHAT = TRUE;
BOOL MonTCP = TRUE;

BOOL LogBBS = TRUE;
BOOL LogCHAT = TRUE;
BOOL LogTCP = TRUE;

extern BOOL LogAPRSIS;

BOOL UIEnabled[33];
BOOL UINull[33];
char * UIDigi[33];

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

extern char NTSAliasesPath[MAX_PATH];
extern char NTSAliasesName[MAX_PATH];

extern char BaseDir[MAX_PATH];
extern char BaseDirRaw[MAX_PATH];			// As set in registry - may contain %NAME%
extern char ProperBaseDir[MAX_PATH];		// BPQ Directory/BPQMailChat

extern char MailDir[MAX_PATH];

extern time_t MaintClock;						// Time to run housekeeping

#ifdef WIN32
BOOL KEEPGOING = 30;					// 5 secs to shut down
#else
BOOL KEEPGOING = 50;					// 5 secs to shut down
#endif
BOOL Restarting = FALSE;
BOOL CLOSING = FALSE;

int ProgramErrors;
int Slowtimer = 0;

#define REPORTINTERVAL 15 * 549;	// Magic Ticks Per Minute for PC's nominal 100 ms timer
int ReportTimer = 0;

// Console Terminal Support

struct ConTermS 
{
	int BPQStream;
	BOOL Active;
	int Incoming;

	char kbbuf[INPUTLEN];
	int kbptr;

	char * KbdStack[MAXSTACK];
	int StackIndex;

	BOOL CONNECTED;
	int SlowTimer;
};

struct ConTermS ConTerm = {0, 0};


VOID CheckProgramErrors()
{
	if (Restarting)
		exit(0);				// Make sure can't loop in restarting

	ProgramErrors++;

	if (ProgramErrors > 25)
	{
		Restarting = TRUE;

		Logprintf(LOG_DEBUG_X, NULL, '!', "Too Many Program Errors - Closing");

/*
		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL;
		SInfo.lpDesktop=NULL;
		SInfo.lpTitle=NULL;
		SInfo.dwFlags=0;
		SInfo.cbReserved2=0;
  		SInfo.lpReserved2=NULL;

		GetModuleFileName(NULL, ProgName, 256);

		Debugprintf("Attempting to Restart %s", ProgName);

		CreateProcess(ProgName, "MailChat.exe WAIT", NULL, NULL, FALSE, 0, NULL, NULL, &SInfo, &PInfo);
*/
		exit(0);
	}
}

#ifdef WIN32

BOOL CtrlHandler(DWORD fdwCtrlType)
{
  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
      printf( "Ctrl-C event\n\n" );
	  CLOSING = TRUE;
      Beep( 750, 300 );
      return( TRUE );

    // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:

	  CLOSING = TRUE;
     printf( "Ctrl-Close event\n\n" );
	 Sleep(20000);
       Beep( 750, 300 );
	   return( TRUE );

    // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
      Beep( 900, 200 );
      printf( "Ctrl-Break event\n\n" );
	  CLOSING = TRUE;
      Beep( 750, 300 );
     return FALSE;

    case CTRL_LOGOFF_EVENT:
      Beep( 1000, 200 );
      printf( "Ctrl-Logoff event\n\n" );
      return FALSE;

    case CTRL_SHUTDOWN_EVENT:
      Beep( 750, 500 );
      printf( "Ctrl-Shutdown event\n\n" );
	  CLOSING = TRUE;
      Beep( 750, 300 );
    return FALSE;

    default:
      return FALSE;
  }
}

#else

// Linux Signal Handlers

static void sigterm_handler(int sig)
{
	syslog(LOG_INFO, "terminating on SIGTERM\n");
	CLOSING = TRUE;
}

static void sigint_handler(int sig)
{
	printf("terminating on SIGINT\n");
	CLOSING = TRUE;
}


static void sigusr1_handler(int sig)
{
	signal(SIGUSR1, sigusr1_handler);
}

#endif


#ifndef WIN32

BOOL CopyFile(char * In, char * Out, BOOL Failifexists)
{
	FILE * Handle;
	DWORD FileSize;
	char * Buffer;
	struct stat STAT;

	if (stat(In, &STAT) == -1)
		return FALSE;

	FileSize = STAT.st_size;

	Handle = fopen(In, "rb");

	if (Handle == NULL)
		return FALSE;

	Buffer = malloc(FileSize+1);

	FileSize = fread(Buffer, 1, STAT.st_size, Handle);

	fclose(Handle);

	if (FileSize != STAT.st_size)
	{
		free(Buffer);
		return FALSE;
	}

	Handle = fopen(Out, "wb");

	if (Handle == NULL)
	{
		free(Buffer);
		return FALSE;
	}

	FileSize = fwrite(Buffer, 1, STAT.st_size, Handle);

	fclose(Handle);
	free(Buffer);

	return TRUE;
}
#endif

int RefreshMainWindow()
{
	return 0;
}

int LastSemGets = 0;

extern int SemHeldByAPI;

VOID MonitorThread(void * x)
{
	// Thread to detect stuck semaphore

	do
	{
		if ((Semaphore.Gets == LastSemGets) && Semaphore.Flag)
		{
			// It is stuck - try to release

			Debugprintf ("Semaphore locked - Process ID = %d, Held By %d",
				Semaphore.SemProcessID, SemHeldByAPI);

			Semaphore.Flag = 0;
		}

		LastSemGets = Semaphore.Gets;

		Sleep(30000);
//		Debugprintf("Monitor Thread Still going %d %d %d %x %d", LastSemGets, Semaphore.Gets, Semaphore.Flag, Semaphore.SemThreadID, SemHeldByAPI);

	} 
	while (TRUE);
}




VOID TIMERINTERRUPT();

BOOL Start();
VOID INITIALISEPORTS();
Dll BOOL APIENTRY Init_APRS();
VOID APRSClose();
Dll VOID APIENTRY Poll_APRS();
VOID HTTPTimer();


#define CKernel
#include "Versions.h"

extern struct SEM Semaphore;

int SemHeldByAPI = 0;
BOOL IGateEnabled = TRUE;
BOOL APRSActive = FALSE;
BOOL ReconfigFlag = FALSE;
BOOL APRSReconfigFlag = FALSE;
BOOL RigReconfigFlag = FALSE;

BOOL IPActive = FALSE;
extern BOOL IPRequired;

extern struct WL2KInfo * WL2KReports;

int InitDone;
char pgm[256] = "LINBPQ";

char SESSIONHDDR[80] = "";
int SESSHDDRLEN = 0;


// Next 3 should be uninitialised so they are local to each process

UCHAR MCOM;
UCHAR MUIONLY;
UCHAR MTX;
uint64_t MMASK;


UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

int SAVEPORT = 0;

VOID SetApplPorts();

char VersionString[50] = Verstring;
char VersionStringWithBuild[50]=Verstring;
int Ver[4] = {Vers};
char TextVerstring[50] = Verstring;

extern UCHAR PWLen;
extern char PWTEXT[];
extern int ISPort;

extern char ChatConfigName[250];

BOOL EventsEnabled = 0;

UCHAR * GetBPQDirectory()
{
	return BPQDirectory;
}
UCHAR * GetLogDirectory()
{
	return LogDirectory;
}
extern int POP3Timer;

// Console Terminal Stuff

#ifndef WIN32

#define _getch getchar

/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */

#include <stdio.h>
#include <sys/select.h>
#include <termios.h>

int _kbhit() {
    static const int STDIN = 0;
    static int initialized = 0;

    if (! initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
 
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

#endif

void ConTermInput(char * Msg)
{
	int i;

	if (ConTerm.BPQStream == 0)
	{
		ConTerm.BPQStream = FindFreeStream();

		if (ConTerm.BPQStream == 255)
		{
			ConTerm.BPQStream = 0;
			printf("No Free Streams\n");
			return;
		}
	}
	
	if (!ConTerm.CONNECTED)
		SessionControl(ConTerm.BPQStream, 1, 0);

	ConTerm.StackIndex = 0;

	// Stack it

	if (ConTerm.KbdStack[19])
		free(ConTerm.KbdStack[19]);

	for (i = 18; i >= 0; i--)
	{
		ConTerm.KbdStack[i+1] = ConTerm.KbdStack[i];
	}

	ConTerm.KbdStack[0] = _strdup(ConTerm.kbbuf);

	ConTerm.kbbuf[ConTerm.kbptr]=13;

	SendMsg(ConTerm.BPQStream, ConTerm.kbbuf, ConTerm.kbptr+1);
}

void ConTermPoll()
{
	int port, sesstype, paclen, maxframe, l4window, len;
	int state, change, InputLen, count;
	char callsign[11] = "";
	char Msg[300];

	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.
	
	SessionState(ConTerm.BPQStream, &state, &change);
		
	if (change == 1)
	{
		if (state == 1)
		{
			// Connected

			ConTerm.CONNECTED = TRUE;
			ConTerm.SlowTimer = 0;
		}
		else
		{
			ConTerm.CONNECTED = FALSE;
			printf("*** Disconnected\n");		
		}
	}

	GetMsg(ConTerm.BPQStream, Msg, &InputLen, &count);

	if (InputLen)
	{
		char * ptr = Msg;
		char * ptr2 = ptr;
		Msg[InputLen] = 0;
		
		while (ptr)
		{
			ptr2 = strlop(ptr, 13);

		// Replace CR with CRLF

			printf(ptr);

			if (ptr2)
				printf("\r\n");

			ptr = ptr2;
		}
	}

	if (_kbhit())
	{
        unsigned char c = _getch();

		if (c == 0xe0)
		{
			// Cursor control 

			c = _getch();

			if (c == 75)		// cursor left
				c = 8;
		}

#ifdef WIN32
		printf("%c", c);
#endif
		if (c == 8)
		{
			if (ConTerm.kbptr)
				ConTerm.kbptr--;
			printf(" \b");		// Already echoed bs - clear typed char from screen
			return;
		}

		if (c == 13  || c == 10)
		{
			 ConTermInput(ConTerm.kbbuf);
			 ConTerm.kbptr = 0;
			 return;
		}

		ConTerm.kbbuf[ConTerm.kbptr++] = c;
		fflush(NULL);

	}

	return;

}
		

int Redirected = 0;

int main(int argc, char * argv[])
{
	int i;
	struct UserInfo * user = NULL;
	ConnectionInfo * conn;
	struct stat STAT;
	PEXTPORTDATA PORTVEC;
	UCHAR LogDir[260];

#ifdef WIN32

	WSADATA       WsaData;            // receives data from WSAStartup
	HWND hWnd = GetForegroundWindow();

	WSAStartup(MAKEWORD(2, 0), &WsaData);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	// disable the [x] button.

	if (hWnd != NULL)
	{
		HMENU hMenu = GetSystemMenu(hWnd, 0);
		if (hMenu != NULL)
		{
			DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
			DrawMenuBar(hWnd);
		}
	}

#else
	setlinebuf(stdout);
	struct sigaction act;
 	openlog("LINBPQ", LOG_PID, LOG_DAEMON);
#ifndef MACBPQ
#ifndef FREEBSD
	prctl(PR_SET_DUMPABLE, 1);					// Enable Core Dumps even with setcap
#endif
#endif

	// Disable Console Terminal if stdout redirected

//	printf("STDOUT %d\n",isatty(STDOUT_FILENO));
//	printf("STDIN %d\n",isatty(STDIN_FILENO));

	if (!isatty(STDOUT_FILENO) || !isatty(STDIN_FILENO))
		Redirected = 1;

	 timeLoadedMS = GetTickCount();

	 printf("Loaded at %llu ms\r\n", timeLoadedMS);

#endif

	printf("G8BPQ AX25 Packet Switch System Version %s %s\n", TextVerstring, Datestring);
	printf("%s\n", VerCopyright);

	if (argc > 1 && _stricmp(argv[1], "-v") == 0)
		return 0;

	sprintf(RlineVer, "LinBPQ%d.%d.%d", Ver[0], Ver[1], Ver[2]);


	Debugprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);

#ifndef MACBPQ
	_MYTIMEZONE = _timezone;
#endif

	if (_MYTIMEZONE < -86400 || _MYTIMEZONE > 86400)
		_MYTIMEZONE = 0;

#ifdef WIN32
	GetCurrentDirectory(256, BPQDirectory);
	GetCurrentDirectory(256, LogDirectory);
#else
	getcwd(BPQDirectory, 256);
	getcwd(LogDirectory, 256);
#endif
	Consoleprintf("Current Directory is %s\n", BPQDirectory);

	for (i = 1; i < argc; i++)
	{
		if (_memicmp(argv[i], "logdir=", 7) == 0)
		{
			strcpy(LogDirectory, &argv[i][7]);
			break;
		}
	}


	// Make sure logs directory exists

	sprintf(LogDir, "%s/logs", LogDirectory);

#ifdef WIN32
	CreateDirectory(LogDir, NULL);
#else
	mkdir(LogDir, S_IRWXU | S_IRWXG | S_IRWXO);
	chmod(LogDir, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

	if (!ProcessConfig())
	{
			WritetoConsoleLocal("Configuration File Error\n");
			return (0);
	}

	SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for Linux (", TextVerstring);

#ifdef MACBPQ
	SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for MAC (", TextVerstring);
#endif
#ifdef FREEBSD
	SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for FreeBSD (", TextVerstring);
#endif


	GetSemaphore(&Semaphore, 0);

	if (Start() != 0)
	{
		FreeSemaphore(&Semaphore);
		return (0);
	}

	for (i=0;PWTEXT[i] > 0x20;i++); //Scan for cr or null

	PWLen=i;

	SetApplPorts();

	GetUIConfig();

	INITIALISEPORTS();

	if (IPRequired)	IPActive = Init_IP();

	APRSActive = Init_APRS();

	if (ISPort == 0)
		IGateEnabled = 0;

	if (needAIS)
		initAIS();

	RigActive = Rig_Init();

	FreeSemaphore(&Semaphore);

	OpenReportingSockets();

	initUTF8();

	InitDone = TRUE;

	Debugprintf("Monitor Thread ID %x", _beginthread(MonitorThread, 0, 0));


#ifdef WIN32
#else
	openlog("LINBPQ", LOG_PID, LOG_DAEMON);
 
	memset (&act, '\0', sizeof(act));
 
	act.sa_handler = &sigint_handler; 
	if (sigaction(SIGINT, &act, NULL) < 0) 
		perror ("SIGINT");

	act.sa_handler = &sigterm_handler; 
	if (sigaction(SIGTERM, &act, NULL) < 0) 
		perror ("sigaction");

	act.sa_handler = SIG_IGN; 
	if (sigaction(SIGHUP, &act, NULL) < 0) 
		perror ("SIGHUP");

	if (sigaction(SIGPIPE, &act, NULL) < 0) 
		perror ("SIGPIPE");

#endif

	for (i = 1; i < argc; i++)
	{
		if (_stricmp(argv[i], "chat") == 0)
			IncludesChat = TRUE;
	}

	if (IncludesChat) 
	{
		RunChat = TRUE;

		printf("Starting Chat\n");

		sprintf (ChatConfigName, "%s/chatconfig.cfg", BPQDirectory);
		printf("Config File is %s\n", ChatConfigName);

		if (stat(ChatConfigName, &STAT) == -1)
		{
			printf("Chat Config File not found - creating a default config\n");
			ChatApplNum = 2;
			MaxChatStreams = 10;
			SaveChatConfigFile(ChatConfigName);
		}

		if (GetChatConfig(ChatConfigName) == EXIT_FAILURE)
		{
			printf("Chat Config File seems corrupt - check before continuing\n");
			return -1;
		}

		if (ChatApplNum)
		{
			if (ChatInit() == 0)
			{
				printf("Chat Init Failed\n");
				RunChat = 0;
			}
			else
			{
				printf("Chat Started\n");
			}
		}
		else
		{
			printf("Chat APPLNUM not defined\n");
			RunChat = 0;
		}
		CopyConfigFile(ChatConfigName);
	}

	// Start Mail if requested by command line or config

	for (i = 1; i < argc; i++)
	{
		if (_stricmp(argv[i], "mail") == 0)
			IncludesMail = TRUE;
	}


	if (IncludesMail)
	{
		RunMail = TRUE;

		printf("Starting Mail\n");

		sprintf (ConfigName, "%s/linmail.cfg", BPQDirectory);
		printf("Config File is %s\n", ConfigName);

		if (stat(ConfigName, &STAT) == -1)
		{
			printf("Config File not found - creating a default config\n");
			strcpy(BBSName, MYNODECALL);
			strlop(BBSName, '-');
			strlop(BBSName, ' ');
			BBSApplNum = 1;
			MaxStreams = 10;
			SaveConfig(ConfigName);
		}

		if (GetConfig(ConfigName) == EXIT_FAILURE)
		{
			printf("BBS Config File seems corrupt - check before continuing\n");
			return -1;
		}

		printf("Config Processed\n");

		BBSApplMask = 1<<(BBSApplNum-1);

	// See if we need to warn of possible problem with BaseDir moved by installer

	sprintf(BaseDir, "%s", BPQDirectory);


	// Set up file and directory names

	strcpy(UserDatabasePath, BaseDir);
	strcat(UserDatabasePath, "/");
	strcat(UserDatabasePath, UserDatabaseName);

	strcpy(MsgDatabasePath, BaseDir);
	strcat(MsgDatabasePath, "/");
	strcat(MsgDatabasePath, MsgDatabaseName);

	strcpy(BIDDatabasePath, BaseDir);
	strcat(BIDDatabasePath, "/");
	strcat(BIDDatabasePath, BIDDatabaseName);

	strcpy(WPDatabasePath, BaseDir);
	strcat(WPDatabasePath, "/");
	strcat(WPDatabasePath, WPDatabaseName);

	strcpy(BadWordsPath, BaseDir);
	strcat(BadWordsPath, "/");
	strcat(BadWordsPath, BadWordsName);

	strcpy(NTSAliasesPath, BaseDir);
	strcat(NTSAliasesPath, "/");
	strcat(NTSAliasesPath, NTSAliasesName);

	strcpy(MailDir, BaseDir);
	strcat(MailDir, "/");
	strcat(MailDir, "Mail");

#ifdef WIN32
	CreateDirectory(MailDir, NULL);		// Just in case
#else
	mkdir(MailDir, S_IRWXU | S_IRWXG | S_IRWXO);
	chmod(MailDir, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

	// Make backup copies of Databases

//	CopyConfigFile(ConfigName);

	CopyBIDDatabase();
	CopyMessageDatabase();
	CopyUserDatabase();
	CopyWPDatabase();

	SetupMyHA();
	SetupFwdAliases();
	SetupNTSAliases(NTSAliasesPath);

	GetWPDatabase();

	GetMessageDatabase();
	GetUserDatabase();
	GetBIDDatabase();
	GetBadWordFile();
	GetHTMLForms();

	// Make sure there is a user record for the BBS, with BBS bit set.

	user = LookupCall(BBSName);

	if (user == NULL)
	{
		user = AllocateUserRecord(BBSName);
		user->Temp = zalloc(sizeof (struct TempUserInfo));
	}

	if ((user->flags & F_BBS) == 0)
	{
		// Not Defined as a BBS

		if(SetupNewBBS(user))
			user->flags |= F_BBS;
	}

	// if forwarding AMPR mail make sure User/BBS AMPR exists

	if (SendAMPRDirect)
	{
		BOOL NeedSave = FALSE;
		
		user = LookupCall("AMPR");
		
		if (user == NULL)
		{
			user = AllocateUserRecord("AMPR");
			user->Temp = zalloc(sizeof (struct TempUserInfo));
			NeedSave = TRUE;
		}

		if ((user->flags & F_BBS) == 0)
		{
			// Not Defined as a BBS

			if (SetupNewBBS(user))
				user->flags |= F_BBS;
			NeedSave = TRUE;
		}

		if (NeedSave)
			SaveUserDatabase();
	}


	// Make sure SYSOPCALL is set

	if (SYSOPCall[0] == 0)
		strcpy(SYSOPCall, BBSName);

	// See if just want to add user (mainly for setup scripts)

	if (argc == 5 && _stricmp(argv[1], "--adduser") == 0)
	{
		BOOL isBBS = FALSE;
		char * response;

		if (_stricmp(argv[4], "TRUE") == 0)
			isBBS = TRUE;

		printf("Adding User %s\r\n", argv[2]);
		response = AddUser(argv[2], argv[3], isBBS);
		printf("%s", response);
		exit(0);
	}
	// Allocate Streams

	strcpy(pgm, "BBS");

	for (i=0; i < MaxStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofStreams++;

//		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, (i == 0 && EnableUI) ? 0x82 : 2, BBSApplMask);
		Disconnect(conn->BPQStream);
	}

	strcpy(pgm, "LINBPQ");

	Debugprintf("POP3 Debug Before Init TCP Timer = %d", POP3Timer);

	InitialiseTCP();
	Debugprintf("POP3 Debug Before Init NNTP Timer = %d", POP3Timer);
	InitialiseNNTP();

	SetupListenSet();		// Master set of listening sockets

	if (EnableUI || MailForInterval)
		SetupUIInterface();

	if (MailForInterval)
		_beginthread(SendMailForThread, 0, 0);


	// Calulate time to run Housekeeping
	{
		struct tm *tm;
		time_t now;

		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

		MaintClock = mktime(tm) - (time_t)_MYTIMEZONE;

		while (MaintClock < now)
			MaintClock += MaintInterval * 3600;

		Debugprintf("Maint Clock %lld NOW %lld Time to HouseKeeping %lld", (long long)MaintClock, (long long)now, (long long)(MaintClock - now));

		if (LastHouseKeepingTime)
		{
			if ((now - LastHouseKeepingTime) > MaintInterval * 3600)
			{
				DoHouseKeeping(FALSE);
			}
		}
		for (i = 1; i < argc; i++)
		{
			if (_stricmp(argv[i], "tidymail") == 0)
				DeleteRedundantMessages();

			if (_stricmp(argv[i], "nohomebbs") == 0)
				DontNeedHomeBBS = TRUE;
		}

		printf("Mail Started\n");
		Logprintf(LOG_BBS, NULL, '!', "Mail Starting");

	}
	}

	Debugprintf("POP3 Debug After Mail Init Timer = %d", POP3Timer);

	if (NUMBEROFTNCPORTS)
		InitializeTNCEmulator();

	AGWActive = AGWAPIInit();

#ifndef WIN32

	for (i = 1; i < argc; i++)
	{
		if (_stricmp(argv[i], "daemon") == 0)
		{
	
			// Convert to daemon
	
			pid_t pid, sid;

		    /* Fork off the parent process */
			pid = fork();

			if (pid < 0)
				exit(EXIT_FAILURE);

			if (pid > 0)
				exit(EXIT_SUCCESS);

			/* Change the file mode mask */

			umask(0);

			/* Create a new SID for the child process */

			sid = setsid();

			if (sid < 0)
				exit(EXIT_FAILURE);

			 /* Change the current working directory */

			if ((chdir("/")) < 0)
				exit(EXIT_FAILURE);

			/* Close out the standard file descriptors */

			printf("Entering daemon mode\n");

			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			break;
		}
	}
#endif

	while (KEEPGOING)
	{
		Sleep(100);
		GetSemaphore(&Semaphore, 2);

		if (QCOUNT < 10)
		{
			if (CLOSING == FALSE)
				FindLostBuffers();
			CLOSING = TRUE;
		}

		if (CLOSING)
		{
			if (RunChat)
			{
				CloseChat();
				RunChat = FALSE;
			}

			if (RunMail)
			{
				int BPQStream, n;

				RunMail = FALSE;

				for (n = 0; n < NumberofStreams; n++)
				{
					BPQStream = Connections[n].BPQStream;

					if (BPQStream)
					{
						SetAppl(BPQStream, 0, 0);
						Disconnect(BPQStream);
						DeallocateStream(BPQStream);
					}
				}

//				SaveUserDatabase();
				SaveMessageDatabase();
				SaveBIDDatabase();
				SaveConfig(ConfigName);
			}

			KEEPGOING--;					// Give time for links to close
			setbuf(stdout, NULL);
			printf("Closing... %d  \r", KEEPGOING);
		}


		if (RigReconfigFlag)
		{
			RigReconfigFlag = FALSE;
			Rig_Close();
			Sleep(2000);				// Allow CATPTT threads to close
			RigActive = Rig_Init();

			Consoleprintf("Rigcontrol Reconfiguration Complete");	
		}

		if (APRSReconfigFlag)
		{
			APRSReconfigFlag = FALSE;
			APRSClose();				
			APRSActive = Init_APRS();

			Consoleprintf("APRS Reconfiguration Complete");	
		}

		if (ReconfigFlag)
		{
			int i;
			BPQVECSTRUC * HOSTVEC;
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;

			ReconfigFlag = FALSE;

//			SetupBPQDirectory();

			WritetoConsoleLocal("Reconfiguring ...\n\n");
			OutputDebugString("BPQ32 Reconfiguring ...\n");


			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR)
					{
//						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
//						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
//						CloseDriverWindow(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}
				}
				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);
				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;
			}

			IPClose();
			APRSClose();
			Rig_Close();
			CloseTNCEmulator();

			if (AGWActive)
				AGWAPITerminate();

			WL2KReports = NULL;

//			Sleep(2000);

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);

			Start();

			INITIALISEPORTS();

			SetApplPorts();

			GetUIConfig();

			FreeConfig();

			for (i=1; i<68; i++)			// Include Telnet, APRS, IP Vec
			{
				HOSTVEC=&BPQHOSTVECTOR[i-1];

				HOSTVEC->HOSTTRACEQ=0;

				if (HOSTVEC->HOSTSESSION !=0)
				{
					// Had a connection

					HOSTVEC->HOSTSESSION=0;
					HOSTVEC->HOSTFLAGS |=3;	// Disconnected

//					PostMessage(HOSTVEC->HOSTHANDLE, BPQMsg, i, 4);
				}
			}

			OpenReportingSockets();

			WritetoConsoleLocal("\n\nReconfiguration Complete\n");

			if (IPRequired)	IPActive = Init_IP();

			APRSActive = Init_APRS();

			if (ISPort == 0)
				IGateEnabled = 0;

			RigActive = Rig_Init();

			if (NUMBEROFTNCPORTS)
			{
				FreeSemaphore(&Semaphore);
				InitializeTNCEmulator();
				GetSemaphore(&Semaphore, 2);
			}

			FreeSemaphore(&Semaphore);
			AGWActive = AGWAPIInit();
			GetSemaphore(&Semaphore, 2);

			OutputDebugString("BPQ32 Reconfiguration Complete\n");
		}

		if (IPActive) Poll_IP();
		if (RigActive) Rig_Poll();
		if (APRSActive) Poll_APRS();
		CheckWL2KReportTimer();

		TIMERINTERRUPT();

		FreeSemaphore(&Semaphore);

		if (Redirected == 0)
			ConTermPoll();

		if (NUMBEROFTNCPORTS)
			TNCTimer();

		if (AGWActive)
			Poll_AGW();

		DRATSPoll();

		HTTPTimer();

		if (ReportTimer)
		{
			ReportTimer--;

			if (ReportTimer == 0)
			{
				ReportTimer = REPORTINTERVAL;
				SendLocation();
			}
		}

		Slowtimer++;

		if (RunChat)
		{
			ChatPollStreams();
			ChatTrytoSend();

			if (Slowtimer > 100)		// 10 secs
			{
				ChatTimer();
			}
		}

		if (RunMail)
		{
			PollStreams();

			if (Slowtimer > 100)		// 10 secs
			{
				time_t NOW = time(NULL);
				struct tm * tm;

				TCPTimer();
				FWDTimerProc();
				BBSSlowTimer();

				if (MaintClock < NOW)
				{
					while (MaintClock < NOW)		// in case large time step
						MaintClock += MaintInterval * 3600;

					Debugprintf("|Enter HouseKeeping");
					DoHouseKeeping(FALSE);
				}

				tm = gmtime(&NOW);

				if (tm->tm_wday == 0)		// Sunday
				{
					if (GenerateTrafficReport && (LastTrafficTime + 86400) < NOW)
					{
						LastTrafficTime = NOW;
						CreateBBSTrafficReport();
					}
				}
			}
			TCPFastTimer();
			TrytoSend();
		}

		if (Slowtimer > 100)
			Slowtimer = 0;
	}

	printf("Closing Ports\n");

	CloseTNCEmulator();

	if (AGWActive)
		AGWAPITerminate();

	if (needAIS)
		SaveAIS();

	// Close Ports

	PORTVEC=(PEXTPORTDATA)PORTTABLE;

	for (i=0;i<NUMBEROFPORTS;i++)
	{
		if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
		{
			if (PORTVEC->PORT_EXT_ADDR)
			{
				PORTVEC->PORT_EXT_ADDR(5, PORTVEC->PORTCONTROL.PORTNUMBER, NULL);
			}
		}
		PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;
	}

	if (AUTOSAVE)
		SaveNodes();

	if (AUTOSAVEMH)
		SaveMH();

	if (IPActive)
		IPClose();

	if (RunMail)
		FreeWebMailMallocs();

	upnpClose();

	// Close any open logs

	for (i = 0; i < 4; i++)
	{
		if (LogHandle[i])
			fclose(LogHandle[i]);
	}

	return 0;
}

int APIENTRY WritetoConsole(char * buff)
{
	return WritetoConsoleLocal(buff);
}

int WritetoConsoleLocal(char * buff)
{
	return printf("%s", buff);
}

#ifdef WIN32
void * VCOMExtInit(struct PORTCONTROL *  PortEntry);
void * V4ExtInit(EXTPORTDATA * PortEntry);
#endif
//UINT SoundModemExtInit(EXTPORTDATA * PortEntry);
//UINT BaycomExtInit(EXTPORTDATA * PortEntry);

void * AEAExtInit(struct PORTCONTROL *  PortEntry);
void * MPSKExtInit(EXTPORTDATA * PortEntry);
void * HALExtInit(struct PORTCONTROL *  PortEntry);

void * AGWExtInit(struct PORTCONTROL *  PortEntry);
void * KAMExtInit(struct PORTCONTROL *  PortEntry);
void * WinmorExtInit(EXTPORTDATA * PortEntry);
void * SCSExtInit(struct PORTCONTROL *  PortEntry);
void * TrackerExtInit(EXTPORTDATA * PortEntry);
void * TrackerMExtInit(EXTPORTDATA * PortEntry);

void * TelnetExtInit(EXTPORTDATA * PortEntry);
void * UZ7HOExtInit(EXTPORTDATA * PortEntry);
void * FLDigiExtInit(EXTPORTDATA * PortEntry);
void * ETHERExtInit(struct PORTCONTROL *  PortEntry);
void * AXIPExtInit(struct PORTCONTROL *  PortEntry);
void * ARDOPExtInit(EXTPORTDATA * PortEntry);
void * VARAExtInit(EXTPORTDATA * PortEntry);
void * SerialExtInit(EXTPORTDATA * PortEntry);
void * WinRPRExtInit(EXTPORTDATA * PortEntry);
void * HSMODEMExtInit(EXTPORTDATA * PortEntry);
void * FreeDataExtInit(EXTPORTDATA * PortEntry);
void * KISSHFExtInit(EXTPORTDATA * PortEntry);

void * InitializeExtDriver(PEXTPORTDATA PORTVEC)
{
	// Only works with built in drivers

	UCHAR Value[20];

	strcpy(Value,PORTVEC->PORT_DLL_NAME);

	_strupr(Value);

#ifndef FREEBSD
#ifndef MACBPQ
	if (strstr(Value, "BPQETHER"))
		return ETHERExtInit;
#endif
#endif
	if (strstr(Value, "BPQAXIP"))
		return AXIPExtInit;

	if (strstr(Value, "BPQTOAGW"))
		return AGWExtInit;

	if (strstr(Value, "AEAPACTOR"))
		return AEAExtInit;

	if (strstr(Value, "HALDRIVER"))
		return HALExtInit;

#ifdef WIN32

	if (strstr(Value, "BPQVKISS"))
		return VCOMExtInit;

	if (strstr(Value, "V4"))
		return V4ExtInit;

#endif
/*
	if (strstr(Value, "SOUNDMODEM"))
		return (UINT) SoundModemExtInit;

	if (strstr(Value, "BAYCOM"))
		return (UINT) BaycomExtInit;
*/
	if (strstr(Value, "MULTIPSK"))
		return MPSKExtInit;

	if (strstr(Value, "KAMPACTOR"))
		return KAMExtInit;

	if (strstr(Value, "WINMOR"))
		return WinmorExtInit;

	if (strstr(Value, "SCSPACTOR"))
		return SCSExtInit;

	if (strstr(Value, "SCSTRACKER"))
		return TrackerExtInit;

	if (strstr(Value, "TRKMULTI"))
		return TrackerMExtInit;

	if (strstr(Value, "UZ7HO"))
		return UZ7HOExtInit;

	if (strstr(Value, "FLDIGI"))
		return FLDigiExtInit;

	if (strstr(Value, "TELNET"))
		return TelnetExtInit;

	if (strstr(Value, "ARDOP"))
		return ARDOPExtInit;

	if (strstr(Value, "VARA"))
		return VARAExtInit;

	if (strstr(Value, "KISSHF"))
		return KISSHFExtInit;

	if (strstr(Value, "SERIAL"))
		return SerialExtInit;

	if (strstr(Value, "WINRPR"))
		return WinRPRExtInit;

	if (strstr(Value, "HSMODEM"))
		return HSMODEMExtInit;

	if (strstr(Value, "FREEDATA"))
		return FreeDataExtInit;

	return(0);
}

int APIENTRY Restart()
{
	CLOSING = TRUE;
	return TRUE;
}

int APIENTRY Reboot()
{
	// Run sudo shutdown -r -f
#ifdef WIN32
	STARTUPINFO  SInfo;
    PROCESS_INFORMATION PInfo;
	char Cmd[] = "shutdown -r -f";


	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL;
	SInfo.lpDesktop=NULL;
	SInfo.lpTitle=NULL;
	SInfo.dwFlags=0;
	SInfo.cbReserved2=0;
  	SInfo.lpReserved2=NULL;

	return CreateProcess(NULL, Cmd, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);
	return 0;
#else
	
	char * arg_list[] = {NULL, NULL, NULL, NULL, NULL};
	pid_t child_pid;
	char * Context;
	signal(SIGCHLD, SIG_IGN); // Silently (and portably) reap children. 

	arg_list[0] = "sudo";
	arg_list[1] = "shutdown";
	arg_list[2] = "now";
	arg_list[3] = "-r";

	//	Fork and Exec shutdown

	// Duplicate this process.  

	child_pid = fork(); 

	if (child_pid == -1) 
	{    				
		printf ("Reboot fork() Failed\n"); 
		return 0;
	}

	if (child_pid == 0) 
 	{    				
		execvp (arg_list[0], arg_list); 
        
		/* The execvp  function returns only if an error occurs.  */ 

		printf ("Failed to run shutdown\n"); 
		exit(0);			// Kill the new process
	}
	return TRUE;								 
#endif

}

int APIENTRY Reconfig()
{
	if (!ProcessConfig())
	{
		return (0);
	}
	SaveNodes();
	WritetoConsoleLocal("Nodes Saved\n");
	ReconfigFlag=TRUE;
	WritetoConsoleLocal("Reconfig requested ... Waiting for Timer Poll\n");
	return 1;
}

int APRSWriteLog(char * msg);

VOID MonitorAPRSIS(char * Msg, size_t MsgLen, BOOL TX)
{
	char Line[300];
	char Copy[300];
	int Len;
	struct tm * TM;
	time_t NOW;

	if (LogAPRSIS == 0)
		return;

	if (MsgLen > 250)
		return;

	// Mustn't change Msg

	memcpy(Copy, Msg, MsgLen);
	Copy[MsgLen] = 0;

	NOW = time(NULL);
	TM = gmtime(&NOW);

	Len = sprintf_s(Line, 299, "%02d:%02d:%02d%c %s", TM->tm_hour, TM->tm_min, TM->tm_sec, (TX)? 'T': 'R', Copy);

	APRSWriteLog(Line);

}

struct TNCINFO * TNC;

#ifndef WIN32

#include <time.h>
#include <sys/time.h>

#ifndef MACBPQ
#ifdef __MACH__

#include <mach/mach_time.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0



int clock_gettime(int clk_id, struct timespec *t){
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    t->tv_sec = seconds;
    t->tv_nsec = nseconds;
    return 0;
}
#endif
#endif


uint64_t GetTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}



void SetWindowText(HWND hWnd, char * lpString)
{
	return;
};

BOOL SetDlgItemText(HWND hWnd, int item, char * lpString)
{
	return 0;
};

#endif

int GetListeningPortsPID(int Port)
{
#ifdef WIN32

	MIB_TCPTABLE_OWNER_PID * TcpTable = NULL;
	PMIB_TCPROW_OWNER_PID Row;
	int dwSize = 0;
	unsigned int n;

	// Get PID of process for this TCP Port

	// Get Length of table
	
	GetExtendedTcpTable(TcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);

	TcpTable = malloc(dwSize);
	GetExtendedTcpTable(TcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);

	for (n = 0; n < TcpTable->dwNumEntries; n++)
	{
		Row = &TcpTable->table[n];
		
		if (Row->dwLocalPort == Port && Row->dwState == MIB_TCP_STATE_LISTEN)
		{
			return Row->dwOwningPid;
			break;
		}
	}
#endif
	return 0;			// Not found
}



VOID Check_Timer()
{
}

VOID POSTDATAAVAIL(){};

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(6425,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 21
		RGB(64,64,0), RGB(64,64,128), RGB(64,64,192), RGB(64,64,255),
		RGB(64,128,0), RGB(64,128,128), RGB(64,128,192), RGB(64,128,255),
		RGB(64,192,0), RGB(64,192,128), RGB(64,192,192), RGB(64,192,255),
		RGB(64,255,0), RGB(64,255,128), RGB(64,255,192), RGB(64,255,255),

		RGB(128,0,0), RGB(128,0,128), RGB(128,0,192), RGB(128,0,255),				// 41
		RGB(128,64,0), RGB(128,64,128), RGB(128,64,192), RGB(128,64,255),
		RGB(128,128,0), RGB(128,128,128), RGB(128,128,192), RGB(128,128,255),
		RGB(128,192,0), RGB(128,192,128), RGB(128,192,192), RGB(128,192,255),
		RGB(128,255,0), RGB(128,255,128), RGB(128,255,192), RGB(128,255,255),

		RGB(192,0,0), RGB(192,0,128), RGB(192,0,192), RGB(192,0,255),				// 61
		RGB(192,64,0), RGB(192,64,128), RGB(192,64,192), RGB(192,64,255),
		RGB(192,128,0), RGB(192,128,128), RGB(192,128,192), RGB(192,128,255),
		RGB(192,192,0), RGB(192,192,128), RGB(192,192,192), RGB(192,192,255),
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,255,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,255,255)
};


//VOID SendRPBeacon(struct TNCINFO * TNC)
//{
//}

int PollStreams()
{
	int state,change;
	ConnectionInfo * conn;
	int n;
	struct UserInfo * user = NULL;
	char ConnectedMsg[] = "*** CONNECTED    ";

	for (n = 0; n < NumberofStreams; n++)
	{
  		conn = &Connections[n];

		DoReceivedData(conn->BPQStream);
		DoBBSMonitorData(conn->BPQStream);

		SessionState(conn->BPQStream, &state, &change);

		if (change == 1)
		{
			if (state == 1) // Connected
			{
				GetSemaphore(&ConSemaphore, 0);
				Connected(conn->BPQStream);
				FreeSemaphore(&ConSemaphore);
			}
			else
			{
				GetSemaphore(&ConSemaphore, 0);
				Disconnected(conn->BPQStream);
				FreeSemaphore(&ConSemaphore);
			}
		}
	}

	return 0;
}


VOID CloseConsole(int Stream)
{
}

#ifndef WIN32

int V4ProcessReceivedData(struct TNCINFO * TNC)
{
	return 0;
}
#endif

#ifdef FREEBSD

char * gcvt(double _Val, int _NumOfDigits, char * _DstBuf)
{
	sprintf(_DstBuf, "%f", _Val);
	return _DstBuf;
}

#endif





