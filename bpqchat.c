// Chat Server for BPQ32 Packet Switch
//
//
// Based on MailChat Version 1.4.48.1

// Add Configurable SYSOP Call

// Sept 2013

// If user tries to reconnect to same node, disconnect old session

// Changed to match changed user rec format in kernel

// Fix initialisation of Signoff Message

// Jan 2014 

// Add UTF-8 support

// ????

// Get command from node, and allow topic to be selected on command line
// Validate HTML Pages


//	Oct 2014 1.0.5.1
//	Fix occasional crash in terminal part line processing

// October 2015 1.0.6.1
//	Recompiled for compatibility with WebMail

// Feb 2016 1.0.7.1
//	Send Chat Map INFO on Startup 
//	Add Welcome Message to Web Config

// July 2017 1.0.8.1
//	Fix Keepalives in Linux version

// November 2018 1.0.9.1
//	Recompiled for Web Interface changes in Node

// January 2019 1.0.10.1
//	Move Config from Registry to File BPQChatServer 1.cfg

// January 2019 1.0.10.2
//	Fix errors in chat config file names and locations

// December 2020 1.0.11.1

//	Send INFO to Map every hour
//	Remove CMD_TO_APPL flag from Applflags

// October 2021 1.0.11.2
//	Recompiled for Web Interface changes in Node

// February 2022 1.0.12.3 Renumbered to 6.0.23.1 to match Node
//	Add option for user defined help file
//  Add Connect Scripts
//	Improve connect timeouts and add link validation polls

//  Version 6.0.24.1 ??

//	Restore CMD_TO_APPL flag to Applflags (13)
//	Check for and remove names set to *RTL (


#include "BPQChat.h"
#include "Dbghelp.h"

#define CHAT
#include "Versions.h"

#define LIBCONFIG_STATIC
#include "libconfig.h"

#include "GetVersion.h"

#define MAX_LOADSTRING 100

BOOL WINE = FALSE;
UINT BPQMsg;

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default
char * REGTREETEXT = "HKEY_LOCAL_MACHINE";

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

int LastVer[4] = {0, 0, 0, 0};					// In case we need to do somthing the first time a version is run

HWND MainWnd;
HWND hWndSess;
RECT MainRect;
HMENU hActionMenu;
static HMENU hMenu;
HMENU hDisMenu;									// Disconnect Menu Handle
HMENU hFWDMenu;									// Forward Menu Handle

int SessX, SessY, SessWidth;					// Params for Session Window

char szBuff[80];

#define MaxSockets 64

extern ChatCIRCUIT ChatConnections[MaxSockets+1];

struct SEM ChatSemaphore = {0, 0};

struct SEM AllocSemaphore = {0, 0};

struct SEM ConSemaphore = {0, 0};

struct SEM OutputSEM = {0, 0};

struct UserInfo ** UserRecPtr=NULL;
int NumberofUsers=0;

struct UserInfo * BBSChain = NULL;					// Chain of users that are BBSes

struct MsgInfo ** MsgHddrPtr=NULL;
int NumberofMessages=0;

int FirstMessageIndextoForward=0;					// Lowest Message wirh a forward bit set - limits search

BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;

char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\r";
char disMsg[] = "Disconnected by SYSOP\r\r";

char LoginMsg[]="user:";

char BlankCall[]="         ";

char ChatSYSOPCall[50];

ULONG BBSApplMask;
ULONG ChatApplMask;

int BBSApplNum=0;
int ChatApplNum=0;

extern int NumberofChatStreams;

//int	StartStream=0;

extern int MaxChatStreams;

char ChatSID[]="[BPQChatServer-%d.%d.%d.%d]\r";

extern char ChatWelcomeMsg[1000];

char NewUserPrompt[100]="Please enter your Name\r>\r";

char ChatSignoffMsg[100];

char AbortedMsg[100]="\rOutput aborted\r";

char BaseDir[MAX_PATH];

char RlineVer[50];

BOOL KISSOnly = FALSE;

UCHAR * OtherNodes=NULL;
char OtherNodesList[1000];

char BPQDirectory[260];

extern char Position[81];
extern char PopupText[260];
extern int PopupMode;

int Bells, FlashOnBell, StripLF, WarnWrap, WrapInput, FlashOnConnect, CloseWindowOnBye;

char Version[32];
char ConsoleSize[32];
char MonitorSize[32];
char DebugSize[32];
char WindowSize[32];

extern config_setting_t * group;

int ProgramErrors = 0;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				RegisterMainWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChatColourDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID SaveWindowConfig();
void ChatWriteLogLine(ChatCIRCUIT * conn, int Flag, char * Msg, int MsgLen, int Flags);
char * lookupuser(char * call);
BOOL GetChatConfig(char * ConfigName);
BOOL SaveChatConfigFile(char * ConfigName);
BOOL GetConfigFromRegistry();
VOID SaveStringValue(config_setting_t * group, char * name, char * value);
VOID SaveIntValue(config_setting_t * group, char * name, int value);
VOID SaveChatConfig(HWND hDlg);
BOOL CreateChatPipeThread();

struct _EXCEPTION_POINTERS exinfox;
	
CONTEXT ContextRecord;
EXCEPTION_RECORD ExceptionRecord;

DWORD Stack[16];

BOOL Restarting = FALSE;

int Dump_Process_State(struct _EXCEPTION_POINTERS * exinfo, char * Msg)
{
	unsigned int SPPtr;
	unsigned int SPVal;

	memcpy(&ContextRecord, exinfo->ContextRecord, sizeof(ContextRecord));
	memcpy(&ExceptionRecord, exinfo->ExceptionRecord, sizeof(ExceptionRecord));
		
	SPPtr = ContextRecord.Esp;

	Debugprintf("BPQChat *** Program Error %x at %x in %s",
	ExceptionRecord.ExceptionCode, ExceptionRecord.ExceptionAddress, Msg);	


	__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

	}

	Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
		ContextRecord.Eax, ContextRecord.Ebx, ContextRecord.Ecx,
		ContextRecord.Edx, ContextRecord.Esi, ContextRecord.Edi, SPVal);
		
	Debugprintf("Stack:");

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

}



void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	Logprintf(LOG_DEBUGx, NULL, '!', "*** Error **** C Run Time Invalid Parameter Handler Called");

	if (expression && function && file)
	{
		Logprintf(LOG_DEBUGx, NULL, '!', "Expression = %S", expression);
		Logprintf(LOG_DEBUGx, NULL, '!', "Function %S", function);
		Logprintf(LOG_DEBUGx, NULL, '!', "File %S Line %d", file, line);
	}
}

char ChatConfigName[MAX_PATH];
char Session[20] = "Server 1";

BOOL UsingingRegConfig = FALSE;

// If program gets too many program errors, it will restart itself  and shut down

VOID CheckProgramErrors()
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	char ProgName[256];

	if (Restarting)
		exit(0);				// Make sure can't loop in restarting

	ProgramErrors++;

	if (ProgramErrors > 25)
	{
		char Command[30];
		
		Restarting = TRUE;

		Logprintf(LOG_DEBUGx, NULL, '!', "Too Many Program Errors - Closing");

		if (cfgMinToTray)
		{
			DeleteTrayMenuItem(MainWnd);
			if (ConsHeader[0]->hConsole)
				DeleteTrayMenuItem(ConsHeader[0]->hConsole);
			if (ConsHeader[1]->hConsole)
				DeleteTrayMenuItem(ConsHeader[1]->hConsole);
			if (hMonitor)
				DeleteTrayMenuItem(hMonitor);
			if (hDebug)
				DeleteTrayMenuItem(hDebug);
		}

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		GetModuleFileName(NULL, ProgName, 256);

		Debugprintf("Attempting to Restart %s", ProgName);

		wsprintf(Command, "WAIT %s", Session);

		CreateProcess(ProgName, Command, NULL, NULL, FALSE, 0, NULL, NULL, &SInfo, &PInfo);
					
		exit(0);
	}
}

VOID WriteMiniDump()
{
#ifdef WIN32

	HANDLE hFile;
	BOOL ret;
	char FN[256];

	sprintf(FN, "%s/Logs/MiniDump%x.dmp", GetBPQDirectory(), time(NULL));

	hFile = CreateFile(FN, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		// Create the minidump

		ret = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, MiniDumpNormal, 0, 0, 0 );

		if(!ret)
			Debugprintf("MiniDumpWriteDump failed. Error: %u", GetLastError());
		else
			Debugprintf("Minidump %s created.", FN);
			CloseHandle(hFile);
	}
#endif
}



HKEY OpenReg()
{
	char Key[100];
	HKEY hKey = 0;
	int disp;

	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\Chat\\%s", Session);

	RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	return hKey;
}

int regGetIntValue(char * Key, int Default)
{
	HKEY hKey = OpenReg();
	int Type, Vallen = 4;
	int Val = Default;

	RegQueryValueEx(hKey ,Key, 0, &Type, (UCHAR *)&Val, (ULONG *)&Vallen);
	RegCloseKey(hKey);

	return Val;
}

BOOL regGetStringValue(char * Key, char * Value, int Len)
{
	HKEY hKey = OpenReg();
	int Type, Vallen = Len;
	int ret;

	ret = RegQueryValueEx(hKey ,Key, 0, &Type, (UCHAR *)Value, (ULONG *)&Vallen);
	if (ret != ERROR_SUCCESS)
		return FALSE;

	RegCloseKey(hKey);
	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	struct _EXCEPTION_POINTERS exinfo;
	_invalid_parameter_handler oldHandler, newHandler;
	char Msg[100];
	int i = 60;
	char * ptr;

	int nCodePage = GetACP(); 

	if (strstr(lpCmdLine, "WAIT"))				// If AutoRestart then Delay 60 Secs
	{	
		hWnd = CreateWindow("STATIC", "Chat Restarting after Failure - Please Wait", 0,
		CW_USEDEFAULT, 100, 550, 70,
		NULL, NULL, hInstance, NULL);

		ShowWindow(hWnd, nCmdShow);

		while (i-- > 0)
		{
			wsprintf(Msg, "Chat Restarting after Failure - Please Wait %d secs.", i);
			SetWindowText(hWnd, Msg);
			
			Sleep(1000);
		}

		DestroyWindow(hWnd);
	}

	ptr = strstr(_strupr(lpCmdLine), "SERVER");

	if (ptr)
		strcpy(Session, ptr);

	_strlwr(&Session[1]);

	strcpy(BPQDirectory, GetBPQDirectory());

	__try {

	// Trap CRT Errors

	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BPQMailChat, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BPQMailChat));

	// Main message loop:

	Logprintf(LOG_CHAT, NULL, '!', "Program Starting");

	} My__except_Routine("Init");

	while (GetMessage(&msg, NULL, 0, 0))
	{
		__try
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		#define EXCEPTMSG "GetMessageLoop"
		#include "StdExcept.c"

		CheckProgramErrors();
		}
	}

	__try
	{
	for (n = 0; n < NumberofChatStreams; n++)
	{
		BPQStream=ChatConnections[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}

	CloseConsole(-2);

	ClearChatLinkStatus();
	SendChatLinkStatus();

	hWnd = CreateWindow("STATIC", "Chat Closing - Please Wait", 0,
				150, 200, 350, 40, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	Sleep(1000);				// A bit of time for links to close

	SendChatLinkStatus();		// Send again to reduce chance of being missed

	Sleep(2000);

	DestroyWindow(hWnd);

	if (ConsHeader[0]->hConsole)
		DestroyWindow(ConsHeader[0]->hConsole);
	if (ConsHeader[1]->hConsole)
		DestroyWindow(ConsHeader[1]->hConsole);
	if (hMonitor)
		DestroyWindow(hMonitor);
	if (hDebug)
		DestroyWindow(hDebug);

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		if (ConsHeader[0]->hConsole)
			DeleteTrayMenuItem(ConsHeader[0]->hConsole);
		if (ConsHeader[1]->hConsole)
			DeleteTrayMenuItem(ConsHeader[1]->hConsole);
		if (hMonitor)
			DeleteTrayMenuItem(hMonitor);
		if (hDebug)
			DeleteTrayMenuItem(hDebug);
	}


	FreeChatMemory();

	n = 0;

	_CrtDumpMemoryLeaks();

	}
	My__except_Routine("Close Processing");

	SaveWindowConfig();

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	SaveChatConfigFile(ChatConfigName);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
//
#define BGCOLOUR RGB(236,233,216)
//#define BGCOLOUR RGB(245,245,245)

HBRUSH bgBrush;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(BPQICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(BPQICON));

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	char Title[80];
	WSADATA WsaData;
	HMENU hTopMenu;		// handle of menu 
	HKEY hKey=0;
	int retCode;
	RECT InitRect;
	RECT SessRect;
	struct _EXCEPTION_POINTERS exinfo;
	struct stat STAT;

	REGTREE = GetRegistryKey();
	REGTREETEXT = GetRegistryKeyText();

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE =TRUE;
		Debugprintf("Running under WINE");
	}

	if (Session[7] == '1')
		sprintf(ChatConfigName, "%s/chatconfig.cfg", GetBPQDirectory());
	else
		sprintf(ChatConfigName, "%s/chatconfig%s.cfg", GetBPQDirectory(), Session);

	// Set up file and directory names

	strcpy(BaseDir, GetBPQDirectory());
		
	strcpy(RtUsr, BaseDir);
	strcat(RtUsr, "/ChatUsers.txt");

	strcpy(RtUsrTemp, BaseDir);
	strcat(RtUsrTemp, "/ChatUsers.tmp");

	strcpy(RtKnown, BaseDir);
	strcat(RtKnown, "/RTKnown.txt");

//	Debugprintf("Users %s", RtUsr);

	UsingingRegConfig = FALSE;

	//	if config file exists use it else try to get from Registry

	if (stat(ChatConfigName, &STAT) == -1)
	{
		UsingingRegConfig = TRUE;
	
		if (GetConfigFromRegistry())
		{
			SaveChatConfigFile(ChatConfigName);
		}
		else
		{
			int retCode;
			char msg[80];

			sprintf(msg, "No configuration found - Dummy Config created");

			retCode = MessageBox(NULL, msg, "BPQChat", MB_OKCANCEL);

			if (retCode == IDCANCEL)
				return FALSE;

			SaveChatConfigFile(ChatConfigName);
		}
	}

	GetChatConfig(ChatConfigName);

	// Get Window Sizes 
	
	sscanf(WindowSize,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);
	sscanf(DebugSize, "%d,%d,%d,%d", &DebugRect.left, &DebugRect.right, &DebugRect.top, &DebugRect.bottom);
	sscanf(MonitorSize, "%d,%d,%d,%d", &MonitorRect.left, &MonitorRect.right, &MonitorRect.top, &MonitorRect.bottom);
	sscanf(ConsoleSize, "%d,%d,%d,%d", &ConsoleRect.left, &ConsoleRect.right, &ConsoleRect.top, &ConsoleRect.bottom);

	hInst = hInstance;

	hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	MainWnd=hWnd;

	if (MainRect.right < 100 || MainRect.bottom < 100)
	{
		GetWindowRect(hWnd,	&MainRect);
	}

	MoveWindow(hWnd,MainRect.left,MainRect.top, MainRect.right-MainRect.left, MainRect.bottom-MainRect.top, TRUE);

	GetVersionInfo(NULL);

	wsprintf(Title,"G8BPQ Chat Server Version %s %s", VersionString, Session);

	SetWindowText(hWnd,Title);

	hWndSess = GetDlgItem(hWnd, 100); 

	GetWindowRect(hWnd,	&InitRect);
	GetWindowRect(hWndSess, &SessRect);

	SessX = SessRect.left - InitRect.left ;
	SessY = SessRect.top -InitRect.top;
	SessWidth = SessRect.right - SessRect.left;

   	// Get handles fou updating menu items

	hTopMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hTopMenu,0);

//	hFWDMenu=GetSubMenu(hActionMenu,0);
	hMenu=GetSubMenu(hActionMenu,0);
	hDisMenu=GetSubMenu(hActionMenu,1);

   CheckTimer();

 	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
		{
			ShowWindow(hWnd, SW_HIDE);
		}
		else
		{
			ShowWindow(hWnd, nCmdShow);
		}
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

   WSAStartup(MAKEWORD(2, 0), &WsaData);

   __try {
    
   return Initialise();

   }My__except_Routine("Initialise");

   return FALSE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int state,change, i;
	ChatCIRCUIT * conn;
	struct _EXCEPTION_POINTERS exinfo;


	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
		{
			//	Dont trap error at this level - let Node error handler pick it up
//			__try
//			{
				DoReceivedData(wParam);
//			}
//			My__except_Routine("DoReceivedData")
			return 0;
		}
		if (lParam & BPQStateChange)
		{
			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			__try
			{
				SessionState(wParam, &state, &change);
		
				if (change == 1)
				{
					if (state == 1) // Connected	
					{
						GetSemaphore(&ConSemaphore, 0);
						__try {Connected(wParam);}
						My__except_Routine("Connected");
						FreeSemaphore(&ConSemaphore);
					}
					else
					{
						GetSemaphore(&ConSemaphore, 0);
						__try{Disconnected(wParam);}
						My__except_Routine("Disconnected");
						FreeSemaphore(&ConSemaphore);
					}
				}
			}
			My__except_Routine("DoStateChange");

		}

		return 0;
	}


	switch (message)
	{

	case WM_KEYUP:	

		switch (wParam)
		{	
		case VK_F3:
			CreateConsole(-2);
			return 0;

		case VK_F4:
			CreateMonitor();
			return 0;

		case VK_F5:
			CreateDebugWindow();
			return 0;

		case VK_TAB:
			return TRUE;

		break;



		}
		return 0;
 			
	case WM_TIMER:

		if (wParam == 1)		// Slow = 10 secs
		{
			__try
			{
				RefreshMainWindow();
				CheckTimer();
				ChatTimer();
			}
			My__except_Routine("Slow Timer");
		}
		else
			__try
			{
				TrytoSend();
			}
			My__except_Routine("TrytoSend");
		
		return (0);

	
	case WM_CTLCOLORDLG:
        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)hActionMenu)
		{
			if (IsClipboardFormatAvailable(CF_TEXT))
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_GRAYED );
			
			return TRUE;
		}

		if (wParam == (WPARAM)hDisMenu)
		{
			// Set up Disconnect Menu

			ChatCIRCUIT * conn;
			char MenuLine[30];
			int n;

			for (n = 0; n <= NumberofChatStreams-1; n++)
			{
				conn=&ChatConnections[n];

				RemoveMenu(hDisMenu, IDM_DISCONNECT + n, MF_BYCOMMAND);

				if (conn->Active)
				{
					sprintf_s(MenuLine, 30, "%d %s", conn->BPQStream, conn->Callsign);
					AppendMenu(hDisMenu, MF_STRING, IDM_DISCONNECT + n, MenuLine);
				}
			}
			return TRUE;
		}
		break;


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmEvent == LBN_DBLCLK)

				break;

		if (wmId >= IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&ChatConnections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				Disconnect(conn->BPQStream);
			}
		}


		switch (wmId)
		{

		case IDM_LOGCHAT:

			ToggleParam(hMenu, hWnd, &LogCHAT, IDM_LOGCHAT);
			break;


		case IDM_CHATCONSOLE:

			CreateConsole(-2);
			break;

		case IDM_MONITOR:

			CreateMonitor();
			break;

		case IDM_DEBUG:
			CreateDebugWindow();
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case ID_HELP_ONLINEHELP:

			ShellExecute(hWnd,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQ%20Mail%20and%20Chat%20Server.htm",
				"", NULL, SW_SHOWNORMAL); 
		
			break;

		case IDM_CONFIG:
			i = DialogBox(hInst, MAKEINTRESOURCE(CHAT_CONFIG), hWnd, ConfigWndProc);
			i = WSAGetLastError();
			break;

			
		case IDM_EDITCHATCOLOURS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CHATCOLCONFIG), hWnd, ChatColourDialogProc);
			break;

		case ID_ACTIONS_UPDATECHATMAPINFO:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_UPDATECHATMAP), hWnd, ChatMapDialogProc);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

  
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	
	case WM_SIZING:
	{
		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;

		MoveWindow(hWndSess, 0, 30, SessWidth, Height - 100, TRUE);
			
		return TRUE;
	}


	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:

		GetWindowRect(MainWnd,	&MainRect);	// For save soutine
		if (ConsHeader[1]->hConsole)
			GetWindowRect(ConsHeader[1]->hConsole, &ConsoleRect);	// For save soutine
		if (hMonitor)
			GetWindowRect(hMonitor,	&MonitorRect);	// For save soutine
		if (hDebug)
			GetWindowRect(hDebug,	&DebugRect);	// For save soutine

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Msg[500];
	int len;

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_MAPPOSITION, Position);
		SetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText);

		SendDlgItemMessage(hDlg, IDC_CLICK, BM_SETCHECK, PopupMode , PopupMode);
		SendDlgItemMessage(hDlg, IDC_HOVER, BM_SETCHECK, !PopupMode , !PopupMode);

		return TRUE; 

	case WM_COMMAND:

		switch LOWORD(wParam)
		{
		case IDSENDTOMAP:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			PopupMode = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
		
//			if (AXIPPort)
			{
				len = wsprintf(Msg, "INFO %s|%s|%d|\r", Position, PopupText, PopupMode);

				if (len < 256)
					Send_MON_Datagram(Msg, len);
			}
			
		break;

		case IDOK:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			PopupMode = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
		
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		
		case IDC_MAPHELP:
			
			ShellExecute(hDlg,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQChatMap.htm",
				"", NULL, SW_SHOWNORMAL); 

			return TRUE;
		}
	}
	return FALSE;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

int RefreshMainWindow()
{
	char msg[80];
	ChatCIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0, SMTPMsgs = 0;
	time_t now;
	struct tm * tm;
	char tim[20];

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn=&ChatConnections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (conn->Flags & CHATLINK)
			{
				if (conn->u.link->supportsPolls)
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d %4d",
						"Chat Link", conn->u.link->alias, conn->BPQStream, 
						"", conn->OutputQueueLength - conn->OutputGetPointer, conn->u.link->RTT);
				else
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					"Chat Link", conn->u.link->alias, conn->BPQStream,
					"", conn->OutputQueueLength - conn->OutputGetPointer);

			}
			else
			if ((conn->Flags & CHATMODE)  && conn->topic)
			{
				i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					conn->UserPointer->Name, conn->u.user->call, conn->BPQStream,
					conn->topic->topic->name, conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in");
				else
				{
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
						conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
						"CHAT", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	SetDlgItemInt(hWnd, IDC_MSGS, NumberofMessages, FALSE);

	n = 0;


	SetDlgItemInt(hWnd, IDC_SYSOPMSGS, SYSOPMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_HELD, HeldMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_SMTP, SMTPMsgs, FALSE);

	SetDlgItemInt(hWnd, IDC_CHATSEM, ChatSemaphore.Clashes, FALSE);

	now = time(NULL);

	tm = gmtime(&now);	
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_UTC, tim);

	tm = localtime(&now);
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_LOCAL, tim);


	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET sock;


BOOL GetConfigFromRegistry()
{
	ChatApplNum = regGetIntValue("ApplNum", 0);
	MaxChatStreams = regGetIntValue("MaxStreams", 0);
	
	regGetStringValue("MapPosition", Position, 80);
	regGetStringValue("MapPopup", PopupText, 250);
	PopupMode = regGetIntValue("PopupMode", 0);

	regGetStringValue("ConsoleSize", ConsoleSize, 32);
	regGetStringValue("MonitorSize", MonitorSize, 32);
	regGetStringValue("DebugSize", DebugSize, 32); 
	regGetStringValue("WindowSize", WindowSize, 32); 
	regGetStringValue("Version", Version, 32 ); 
	Bells = regGetIntValue("Bells", 0);
	FlashOnBell = regGetIntValue("FlashOnBell", 0);
	StripLF = regGetIntValue("StripLF", 0);
	WarnWrap = regGetIntValue("WarnWrap", 0);
	WrapInput = regGetIntValue("WrapInput", 0);
	FlashOnConnect = regGetIntValue("FlashOnConnect", 0);
	CloseWindowOnBye = regGetIntValue("CloseWindowOnBye", 0);

	return regGetStringValue("OtherChatNodes", OtherNodesList, 999);		// Return false if not there
}


BOOL Initialise()
{
	int i;
	ChatCIRCUIT * conn;

	//	Register message for posting by BPQDLL

	CheckTimer();				// Make sure init is complete

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

Retry:

	if (ChatApplNum)
	{
		char * ptr1 = GetApplCall(ChatApplNum);
		char * Save;

		if (*ptr1 > 0x20)
		{
			char * Context;
			
			memcpy(OurNode, ptr1, 10);
			strlop(OurNode, ' ');
			strcpy(ChatSYSOPCall, OurNode);
			strlop(ChatSYSOPCall, '-');

			ptr1=GetApplAlias(ChatApplNum);
			memcpy(OurAlias, ptr1,10);
			strlop(OurAlias, ' ');

			ChatApplMask = 1<<(ChatApplNum-1);
		
			// Set up other nodes list. rtlink messes with the string so pass copy

			// New format can have connect scrips and separates lines with crlf, not space


			if (strchr(OtherNodesList, '\r'))	// Has connect script entries
			{
				Save = ptr1 = strtok_s(_strdup(OtherNodesList), "\r\n", &Context);

				while (ptr1 && ptr1[0])
				{
					rtlink(ptr1);
					ptr1 = strtok_s(NULL, "\r\n", &Context);
					}
			}
			else
			{
				Save = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

				while (ptr1)
				{
					rtlink(ptr1);			
					ptr1 = strtok_s(NULL, " ,\r", &Context);
				}
			}
			free(Save);
			SetupChat();
		}
	}
	else
	{
		DialogBox(hInst, MAKEINTRESOURCE(CHAT_CONFIG), hWnd, ConfigWndProc);
		goto Retry;
	}

	// Allocate Streams

	for (i=0; i < MaxChatStreams; i++)
	{
		conn = &ChatConnections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofChatStreams++;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, 3, ChatApplMask);
		Disconnect(conn->BPQStream);
	}


	if (cfgMinToTray)
	{
		char Text[80];
		wsprintf(Text, "Chat Server %s ", Session);
		AddTrayMenuItem(MainWnd, Text);
	}
	
	SetTimer(hWnd,1,10000,NULL);	// Slow Timer (10 Secs)
	SetTimer(hWnd,2,100,NULL);		// Send to Node (100 ms)

	CheckMenuItem(hMenu,IDM_LOGCHAT, (LogCHAT) ? MF_CHECKED : MF_UNCHECKED);

	sprintf(ChatSignoffMsg, "73 de %s\r", ChatSYSOPCall);

	if (ChatWelcomeMsg[0] == 0)
		strcpy(ChatWelcomeMsg, "G8BPQ Chat Server.$WType /h for command summary.$WBringing up links to other nodes.$W"
			"This may take a minute or two.$WThe /p command shows what nodes are linked.$W");

	RefreshMainWindow();

	DeleteLogFiles();

	CreateChatPipeThread();

	return TRUE;
}

int	CriticalErrorHandler(char * error)
{
	Debugprintf("Critical Error %s", error);
	ProgramErrors = 25;
	CheckProgramErrors();				// Force close
	return 0;
}


VOID SaveWindowConfig()
{
	wsprintf(MonitorSize,"%d,%d,%d,%d",MonitorRect.left,MonitorRect.right,MonitorRect.top,MonitorRect.bottom);
	wsprintf(DebugSize,"%d,%d,%d,%d",DebugRect.left,DebugRect.right,DebugRect.top,DebugRect.bottom);
	wsprintf(WindowSize,"%d,%d,%d,%d",MainRect.left,MainRect.right,MainRect.top,MainRect.bottom);

	wsprintf(Version,"%d,%d,%d,%d", Ver[0], Ver[1], Ver[2], Ver[3]);

	if (ConsoleRect.right)
		wsprintf(ConsoleSize,"%d,%d,%d,%d",ConsoleRect.left, ConsoleRect.right, ConsoleRect.top, ConsoleRect.bottom);

	return;
}

char InfoBoxText[100];


INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
		
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, 5050, InfoBoxText);

		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case 0:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}


int GetMultiLineDialog(HWND hDialog, int DLGItem)
{
	char Text[10000];
	
	GetDlgItemText(hDialog, DLGItem, Text, 10000);

	// We can now have connect scripts for each node, so leave cr/lf

	if (Text[strlen(Text)-1] != '\n')			// no terminating crlf?
		strcat(Text, "\r\n");

	strcpy(OtherNodesList, Text);
	return TRUE;
}

INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
	char Text[10000] = "";
	char * ptr1, *ptr2, *Context;

	switch (message)
	{

	case WM_INITDIALOG:

		if (strchr(OtherNodesList, 13))		// New format with connect scripts?
		{
			strcpy(Text, OtherNodesList);
		/*
			ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), "\r", &Context);

			while (ptr1 && ptr1[0])
			{
				strcat(Text, ptr1);	
				strcat(Text, "\r\n");	
				ptr1 = strtok_s(NULL, "\r", &Context);

				if (*(ptr1) == 10)
					ptr1++;
			}
		*/
		}
		else
		{
			ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

			while (ptr1)
			{
				strcat(Text, ptr1);	
				strcat(Text, "\r\n");	
				ptr1 = strtok_s(NULL, " ,\r", &Context);
			}
			free(ptr2);
		}
		SetDlgItemInt(hDlg, ID_CHATAPPL, ChatApplNum, FALSE);
		SetDlgItemInt(hDlg, ID_STREAMS, MaxChatStreams, FALSE);
		SetDlgItemText(hDlg, ID_CHATNODES, Text);

		// Replace $W in  Welcome Message with cr lf

		ptr2 = ptr1 = _strdup(ChatWelcomeMsg);

	scan:

		ptr1 = strstr(ptr1, "$W");
    
		if (ptr1)
		{    
			*(ptr1++)=13;			// put in cr
			*(ptr1++)=10;			// put in lf

			goto scan;
		} 
	
		SetDlgItemText(hDlg, IDM_CHATUSERMSG, ptr2);

		free(ptr2);

		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case SAVEWELCOME:

			GetDlgItemText(hDlg, IDM_CHATUSERMSG, ChatWelcomeMsg, 999);

			// Replace cr lf in string with $W

			ptr1 = ChatWelcomeMsg;

		scan2:

			ptr1 = strstr(ptr1, "\r\n");
    
			if (ptr1)
			{    
				*(ptr1++)='$';			// put in cr
				*(ptr1++)='W';			// put in lf

				goto scan2;
			} 

			SaveChatConfigFile(ChatConfigName);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case SAVENODES:
			
			SaveChatConfig(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;

	}	
	return (INT_PTR)FALSE;
}



