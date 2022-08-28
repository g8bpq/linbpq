
// Version 2.0.1 November 2007

// Change resizing algorithm


// Version 2.0.2 January 2008

// Restore Checked state of Bells and AutoConnect Flags
// Call CheckTimer on startup (for new Initialisation Scheme for perl)

// Version 2.0.3 July 2008

// Display lines received without a terminaing CR


// Version 2.0.4 November 2008

// Add option to remove a Line Feed following a CR
// Add Logging Option

// Version 2.0.5 January 2009

// Add Start Minimized Option

// Version 2.0.6 June 2009

// Add Option to send *** Disconnnected on disconnect
// Add line wrap code
// Add option not to monitor NODES broadcasts

// Version 2.0.7 October 2009

// Add input buffer scrollback.
// Fix monitoring when PORTNUM specified

// Version 2.0.8 December 2009

// Fix use of numeric keypad 2 and 8 (were treated as up and down)

// Version 2.0.9 March 2010

// Add colour for monitor and BPQ Chat

// Version 2.0.0 October 2010

// Add Chat Terminal Mode (sends keepalives)

// Version 2.1.0 August 2011

//		Add Copy/Paste capability to output window.
//		Add Font Selection
//		Get Registry Tree from BPQ32.dll
//		Add Command to reset Monitor/Output window split

// Version 2.1.1 October 2011

//		Wrap overlong lines

// Version 2.1.2 Jan 2012

//		Call CloseBPQ32 on exit
//		Fix ClearOutputWindow
//		Save MonitorNodes flag

#define _CRT_SECURE_NO_DEPRECATE

#include "compatbits.h"
#include "AsmStrucs.h"
#include <htmlhelp.h>
#include <Richedit.h> 
#include <commctrl.h>
#define DllImport __declspec(dllimport)

#include "bpq32.h"	// BPQ32 API Defines
#define BPQTermMDI

#ifndef MDIKERNEL

#include "Versions.h"
#include "GetVersion.h"

#define BGCOLOUR RGB(236,233,216)

#endif

HBRUSH bgBrush;

#include "BpqTermMDI.h"


extern BOOL FrameMaximized;

char RTFHeader[4000];

int RTFHddrLen;

struct ConsoleInfo * ConsHeader = NULL;

struct ConsoleInfo MonWindow;

struct ConsoleInfo InitHeader;			// Dummy for before window is created

HKEY FAR WINAPI GetRegistryKey();
char * FAR WINAPI GetRegistryKeyText();

VOID __cdecl Debugprintf(const char * format, ...);

#ifndef MDIKERNEL

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
	return;
}
#else
extern HKEY REGTREE;
extern int OffsetH, OffsetW;

#endif

HINSTANCE hInst; 

char ClassName[] = "BPQMAINWINDOW";
char Title[80];

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

extern RECT FRect;	// Frame 

int NewLine();

int	ProcessBuff(char * readbuff,int len);

int EnableConnectMenu(HWND hWnd);
int EnableDisconnectMenu(HWND hWnd);
int	DisableConnectMenu(HWND hWnd);
int DisableDisconnectMenu(HWND hWnd);
int	ToggleRestoreWindows();
int ToggleAppl(HWND hWnd, int Item, int mask);
int DoReceivedData(int Stream);
int	DoStateChange(int Stream);
int ToggleFlags(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
int DoMonData(int Stream);
int TogglePort(HWND hWnd, int Item, int mask);
int ToggleMTX(HWND hWnd);
int ToggleMCOM(HWND hWnd);
int ToggleParam(HMENU hMenu, BOOL * Param, int Item);
int ToggleChat(HWND hWnd);
void MoveWindows(struct ConsoleInfo * Cinfo);
void CopyListToClipboard(HWND hWnd);
void CopyRichTextToClipboard(HWND hWnd);
BOOL OpenMonitorLogfile();
void WriteMonitorLine(char * Msg, int MsgLen);
int WritetoOutputWindow(struct ConsoleInfo * Cinfo, char * Msg, int len, BOOL Refresh);
VOID DoRefresh(struct ConsoleInfo * Cinfo);
INT_PTR CALLBACK FontConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
struct ConsoleInfo * CreateChildWindow(int Stream, BOOL DuringInit);
BOOL CreateMonitorWindow(char * MonSize);
VOID SaveMDIWindowPos(HWND hWnd, char * RegKey, char * Value, BOOL Minimized);
int ToggleMON_UI_ONLY(HWND hWnd);

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(64,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 21 
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



LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;


extern CMDX COMMANDS[];
extern int APPL1;

static HMENU trayMenu;

int xsize,ysize;		// Screen size at startup

double Split=0.5;
int SplitPos=300;
#define SplitBarHeight 5
#define InputBoxHeight 25
RECT MonRect;
RECT OutputRect;
RECT SplitRect;

int OutputBoxHeight;

int maxlinelen = 80;
int CharWidth = 8;

int ClientHeight, ClientWidth;


#define MAXLINES 1000
#define LINELEN 200

char RTFHeader[4000];

int RTFHddrLen;


char FontName[100] = "FixedSys";
int FontSize = 20;
int FontWidth = 8;
int CodePage = 1252;
int CharSet = 0;


int CurrentHost = 0;
int CfgNo = 0;

SOCKET sock;

BOOL MonData = FALSE;

static char Key[80];
int portmask=1;
int mtxparam=1;
int mcomparam=1;
int monUI=0;

char kbbuf[160];
int kbptr=0;
char readbuff[100000];				// for stupid bbs programs

int ptr=0;

int Stream, Stream2;				// Stream2 for SYSOP Chat session
int len,count;


static UINT APPLMASK = 0x80000000;
int applflags = 2;				// Message to Uset and Application
int Sessno = 0;

int PartLinePtr=0;
int PartLineIndex=0;		// Listbox index of (last) incomplete line

BOOL Bells = FALSE;
BOOL StripLF = FALSE;
BOOL LogMonitor = FALSE;
BOOL LogOutput = FALSE;
BOOL SendDisconnected = TRUE;
BOOL MonitorNODES = TRUE;
BOOL MonitorColour = TRUE;
BOOL ChatMode = FALSE;
BOOL RestoreWindows = TRUE;
BOOL MonitorAPRS = FALSE;
BOOL WarnWrap = TRUE;
BOOL WrapInput = TRUE;
BOOL FlashOnBell = FALSE;




HANDLE 	MonHandle=INVALID_HANDLE_VALUE;

HCURSOR DragCursor;
HCURSOR	Cursor;

AUTOCONNECT=TRUE;

LOGFONT LF;

HFONT hFont ;

BOOL MinimizetoTray;

VOID CALLBACK tTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

TIMERPROC tTimerFunc = (TIMERPROC) tTimerProc;

int tTimerHandle = 0;

BOOL WINE;


// HMENU hTermMenu;		// handle of menu 
extern HMENU hTermActMenu, hTermCfgMenu, hTermEdtMenu, hTermHlpMenu;
extern HMENU hMonActMenu, hMonCfgMenu, hMonEdtMenu, hMonHlpMenu;


extern HMENU hWndMenu, hMainFrameMenu, hBaseMenu;
extern HWND ClientWnd, FrameWnd;
extern HANDLE hInstance;

extern byte	MCOM;
extern char	MTX;
extern ULONG MMASK;
extern byte	MUIONLY;

HMENU hPopMenu1;

#define AUTO -1
#define CP437 437
#define CP1251 1251
#define CP1252 1252

int RXMode = AUTO; //CP1252;
int TXMode = CP_UTF8;

int APRSWriteLog(char * msg);

VOID MonitorAPRSIS(char * Msg, int MsgLen, BOOL TX)
{
	char Line[300];
	char Copy[300];

	int Len;
	struct tm * TM;
	time_t NOW;

	if (MonWindow.hConsole == NULL || MonitorAPRS == 0)
		return;

	if (MsgLen > 250)
		return;

	NOW = _time32(NULL);
	TM = gmtime(&NOW);

	// Mustn't change Msg

	memcpy(Copy, Msg, MsgLen);
	Copy[MsgLen] = 0;

	if (strchr(Copy, 13))
		Len = sprintf_s(Line, 299, "%02d:%02d:%02d%c %s", TM->tm_hour, TM->tm_min, TM->tm_sec, (TX)? 'T': 'R', Copy);
	else
		Len = sprintf_s(Line, 299, "%02d:%02d:%02d%c %s\r", TM->tm_hour, TM->tm_min, TM->tm_sec, (TX)? 'T': 'R', Copy);

	if (TX)
		MonWindow.CurrentColour = 0;
	else
		MonWindow.CurrentColour = 64;

	WritetoOutputWindow(&MonWindow, Line, Len, FALSE);
	MonWindow.NeedRefresh = TRUE;

	APRSWriteLog(Line);
}


VOID CALLBACK tTimerProc(HWND  hwnd, UINT  uMsg, UINT  idEvent, DWORD dwTime)
{
	// entered every 1 sec

	struct ConsoleInfo * Cinfo = &MonWindow;

	if (Cinfo->NeedRefresh)
	{
		Cinfo->NeedRefresh = FALSE;
		DoRefresh(Cinfo);
	}


	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->CONNECTED && ChatMode)
		{
			Cinfo->SlowTimer++;

			if (Cinfo->SlowTimer > 500)				// About 9 mins
			{
				Cinfo->SlowTimer = 0;
				SendMsg(Cinfo->BPQStream, "\0", 1);
			}
		}
	}
}

//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
//	This function initializes the application and processes the
//	message loop.
//

VOID SetupRTFHddr()
{
	int i, n;
	char RTFColours[3000];
	char Temp[1000];

	// Set up RTF Header, including Colours String;

	memcpy(RTFColours, "{\\colortbl ;", 12);
	n = 12;

	for (i = 1; i < 100; i++)
	{
		COLORREF Colour = Colours[i];
		n += sprintf(&RTFColours[n], "\\red%d\\green%d\\blue%d;", GetRValue(Colour), GetGValue(Colour),GetBValue(Colour));
	}

	RTFColours[n++] = '}';
	RTFColours[n] = 0;

//	strcpy(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fmodern\\fprq1\\fcharset204 ;}}");
	sprintf(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fprq1\\cpg%d\\fcharset%d %s;}}", CodePage, CharSet, FontName);
	strcat(RTFHeader, RTFColours);
	sprintf(Temp, "\\viewkind4\\uc1\\pard\\f0\\fs%d", FontSize);
	strcat(RTFHeader, Temp);

	RTFHddrLen = strlen(RTFHeader);
}

#ifdef MDIKERNEL

extern int SessHandle;

VOID CALLBACK SetupTermSessions(HWND hwnd, UINT  uMsg, UINT  idEvent,  DWORD  dwTime)
{
	int i, n, tempmask=0xffff;
	char msg[50];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	int Sessions = 0;
	struct PORTCONTROL * PORT;

	KillTimer(NULL,SessHandle);

	tTimerHandle = SetTimer(NULL, 0, 1000, tTimerFunc);


	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE = TRUE;
		Debugprintf("Running under WINE");
	}

	BPQMsg = RegisterWindowMessage(BPQWinMsg);
	SetupRTFHddr();

	trayMenu = CreatePopupMenu();
	AppendMenu(trayMenu, MF_STRING, RTFCOPY, "Copy");

	Stream = FindFreeStream();		// For Monitoring /Inbound
	
	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return;
	}

	BPQSetHandle(Stream, FrameWnd);

	// Get config from Registry 

	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\BPQTermMDI");

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Sessions",0,			
			(ULONG *)&Type,(UCHAR *)&Sessions,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MONUIONLY",0,			
			(ULONG *)&Type,(UCHAR *)&monUI,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&tempmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ChatMode",0,			
			(ULONG *)&Type,(UCHAR *)&ChatMode, (ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MONColour",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorColour,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MonitorNODES",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorNODES,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MonitorAPRS",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorAPRS,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SendDisconnected",0,			
			(ULONG *)&Type,(UCHAR *)&SendDisconnected,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WarnWrap",0,			
			(ULONG *)&Type,(UCHAR *)&WarnWrap,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WrapInput",0,			
			(ULONG *)&Type,(UCHAR *)&WrapInput,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"FlashOnBell",0,			
			(ULONG *)&Type,(UCHAR *)&FlashOnBell,(ULONG *)&Vallen);

		

		Vallen=99;
		retCode = RegQueryValueEx(hKey, "FontName", 0, &Type, FontName, &Vallen);

		if (FontName[0] == 0)
			strcpy(FontName, "FixedSys");

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CharSet", 0, &Type, (UCHAR *)&CharSet, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CodePage", 0, &Type, (UCHAR *)&CodePage, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontSize", 0, &Type, (UCHAR *)&FontSize, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontWidth", 0, &Type, (UCHAR *)&FontWidth, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "RestoreWindows", 0, &Type, (UCHAR *)&RestoreWindows, &Vallen);

		Vallen=80;
		RegQueryValueEx(hKey, "MonSize", 0 , &Type, &Size[0], &Vallen);
	}

//	hPopMenu1 = GetSubMenu(hMonCfgMenu, 1);

	portmask = tempmask;

	for (n=1; n <= GetNumberofPorts(); n++)
	{
		PORT = GetPortTableEntryFromSlot(n);
		
		i = PORT->PORTNUMBER;

		sprintf(msg,"Port %d %s ",i, PORT->PORTDESCRIPTION);

		if (tempmask & (1<<(i-1)))
			AppendMenu(hMonCfgMenu,MF_STRING | MF_CHECKED,BPQBASE + i,msg);
		else
			AppendMenu(hMonCfgMenu,MF_STRING | MF_UNCHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMonCfgMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMonCfgMenu,BPQMTX,MF_UNCHECKED);

	if (mcomparam & 1)
		CheckMenuItem(hMonCfgMenu,BPQMCOM,MF_CHECKED);
	else
		CheckMenuItem(hMonCfgMenu,BPQMCOM,MF_UNCHECKED);

	if (monUI & 1)
		CheckMenuItem(hMonCfgMenu,MON_UI_ONLY,MF_CHECKED);
	else
		CheckMenuItem(hMonCfgMenu,MON_UI_ONLY,MF_UNCHECKED);
	
	if (RestoreWindows)
	{
		CheckMenuItem(hMonCfgMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
		CheckMenuItem(hTermCfgMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
	}
	else
	{
		CheckMenuItem(hMonCfgMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
		CheckMenuItem(hTermCfgMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
	}

  	if (Bells & 1)
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_UNCHECKED);

  	if (SendDisconnected & 1)
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hTermCfgMenu, ID_WARNWRAP, (WarnWrap) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hTermCfgMenu, ID_WRAP, (WrapInput) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hTermCfgMenu, ID_FLASHONBELL, (FlashOnBell) ? MF_CHECKED : MF_UNCHECKED);


	CheckMenuItem(hMonCfgMenu,BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hMonCfgMenu,MONITORAPRS, (MonitorAPRS) ? MF_CHECKED : MF_UNCHECKED);


	DrawMenuBar(FrameWnd);	

	if (portmask) applflags |= 0x80;

	MCOM = mcomparam;
	MTX = mtxparam;
	MMASK = portmask;
	MUIONLY = monUI;

	if (COMMANDS[APPL1 + 31].String[0] == '*')		// No Host App
	{
		SetAppl(Stream, 0x80, 0);
	}
	else
	{
		SetAppl(Stream, applflags, APPLMASK);

		// Allocate another incoming stream
	
		Stream2 = FindFreeStream();;

		BPQSetHandle(Stream2, FrameWnd);
		SetAppl(Stream2, 2, APPLMASK);
	}

//	CreateMonitorWindow(Size);

	if (RestoreWindows)
	{
		struct ConsoleInfo * Cinfo;
		int i;

		for (i = 1; i <= Sessions; i++)
		{
			Cinfo = CreateChildWindow(0, TRUE);
		}
	}
}

SaveHostSessions()
{
	int Sessions = 0;
	struct ConsoleInfo * Cinfo;
	int i, disp;
	char Item[80];
	BOOL NeedPause = FALSE;
	HKEY hKey;

	// Save config
	
	RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS,  NULL, &hKey, &disp);

	RegSetValueEx(hKey,"ChatMode",0,REG_DWORD,(BYTE *)&ChatMode,4);
	RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&portmask,4);
	RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
	RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
	RegSetValueEx(hKey,"SendDisconnected",0,REG_DWORD,(BYTE *)&SendDisconnected,4);
	RegSetValueEx(hKey,"RestoreWindows",0,REG_DWORD,(BYTE *)&RestoreWindows,4);
	RegSetValueEx(hKey,"MTX",0,REG_DWORD,(BYTE *)&mtxparam,4);
	RegSetValueEx(hKey,"MCOM",0,REG_DWORD,(BYTE *)&mcomparam,4);
	RegSetValueEx(hKey,"MONUIONLY",0,REG_DWORD,(BYTE *)&monUI,4);
	RegSetValueEx(hKey,"MONColour",0,REG_DWORD,(BYTE *)&MonitorColour,4);
	RegSetValueEx(hKey,"MonitorNODES",0,REG_DWORD,(BYTE *)&MonitorNODES,4);
	RegSetValueEx(hKey,"MonitorAPRS",0,REG_DWORD,(BYTE *)&MonitorAPRS,4);
	RegSetValueEx(hKey,"WarnWrap",0,REG_DWORD,(BYTE *)&WarnWrap,4);
	RegSetValueEx(hKey,"WrapInput",0,REG_DWORD,(BYTE *)&WrapInput,4);
	RegSetValueEx(hKey,"FlashOnBell",0,REG_DWORD,(BYTE *)&FlashOnBell,4);

	// Close any sessions

	i = 0;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		i++;

		sprintf(Item, "SessSize%d", i);
		SaveMDIWindowPos(Cinfo->hConsole, "BPQTermMDI", Item, Cinfo->Minimized);
	}

	RegSetValueEx(hKey,"Sessions",0,REG_DWORD,(BYTE *)&i,4);

	RegCloseKey(hKey);

	SaveMDIWindowPos(MonWindow.hConsole, "BPQTermMDI", "MonSize", MonWindow.Minimized);
}

CloseHostSessions()
{
	struct ConsoleInfo * Cinfo;
	int i;
	BOOL NeedPause = FALSE;

	i = 0;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		i++;

		if (Cinfo->CONNECTED)
		{
			SessionControl(Cinfo->BPQStream, 2, 0);
			NeedPause = TRUE;
		}
	}

	if (NeedPause)
		Sleep(1500);
}

#else 

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	sscanf(lpCmdLine,"%d %x",&Sessno, &applflags);

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL, tTimerHandle);

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (msg.wParam);
}

//

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class 
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling either RegisterClass or 
//       the internal MyRegisterClass.
//

HMENU hTermCfgMenu, hMonMenu, hPopMenu1, hPopMenu2, hPopMenu3;		// handle of menu 

HINSTANCE	hInstance					= NULL;
TCHAR		gszSigmaFrameClassName[]	= TEXT("SigmaMdiFrame");
TCHAR		gszAppName[]				= TEXT("Sigma");
HWND		ghMDIClientArea				= NULL; //This stores the MDI client area window handle

HWND		FrameWnd				= NULL; // This will hold the main frame window handle
HMENU		ghMainFrameMenu				= NULL;

HANDLE		ghSystemInfoMutex			= NULL;
HMENU		ghSysInfoMenu				= NULL;



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	INITCOMMONCONTROLSEX dICC;
	BOOL bInitCommonControlLib;
	WNDCLASSEX wndclassMainFrame;
	int i, n, tempmask=0xffff;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	int Sessions = 0;

	hInstance = hInstance;
	hInst = hInstance; // Store instance handle in our global variable

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE = TRUE;
		Debugprintf("Running under WINE");
	}

	REGTREE = GetRegistryKey();

	MinimizetoTray=GetMinimizetoTrayFlag();

	dICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
	dICC.dwICC = ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES;

	// Initialize common control library
	
	bInitCommonControlLib = InitCommonControlsEx(&dICC);
	
	if(!bInitCommonControlLib)
	{
		MessageBox(NULL, TEXT("Unable to load Common Controls!"), gszAppName, MB_ICONERROR);
	}

	LoadLibrary("riched20.dll");

	SetupRTFHddr();

	wndclassMainFrame.cbSize		= sizeof(WNDCLASSEX);
	wndclassMainFrame.style			= CS_HREDRAW | CS_VREDRAW;
	wndclassMainFrame.lpfnWndProc	= FrameWndProc;
	wndclassMainFrame.cbClsExtra	= 0;
	wndclassMainFrame.cbWndExtra	= 0;
	wndclassMainFrame.hInstance		= hInstance;
    wndclassMainFrame.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
	wndclassMainFrame.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclassMainFrame.hbrBackground	= (HBRUSH) GetStockObject(GRAY_BRUSH);
	wndclassMainFrame.lpszMenuName	= NULL;
	wndclassMainFrame.lpszClassName	= gszSigmaFrameClassName;
	wndclassMainFrame.hIconSm		= NULL;
	
	//Register MainFrame window
	if(!RegisterClassEx(&wndclassMainFrame))
	{
		DWORD derror = GetLastError();
		MessageBox(NULL, TEXT("This program requires Windows NT!"), gszAppName, MB_ICONERROR);
		return 0;
	}

	ghMainFrameMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME_MENU));
	hTermCfgMenu = LoadMenu(hInstance, MAKEINTRESOURCE(TERM_MENU));
	hMonMenu = LoadMenu(hInstance, MAKEINTRESOURCE(MON_MENU));

	//Create the main MDI frame window

	FrameWnd = CreateWindow(gszSigmaFrameClassName, 
								gszAppName, 
								WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
								CW_USEDEFAULT,	// allows system choose an x position
								CW_USEDEFAULT,	// allows system choose a y position
								CW_USEDEFAULT,	// width, CW_USEDEFAULT allows system to choose height and width
								CW_USEDEFAULT,	// height, CW_USEDEFAULT ignores heights as this is set by setting
												// CW_USEDEFAULT in width above.
								NULL,			// handle to parent window
								ghMainFrameMenu, // handle to menu
								hInstance,	// handle to the instance of module
								NULL);		// Long pointer to a value to be passed to the window through the 
											// CREATESTRUCT structure passed in the lParam parameter the WM_CREATE message

	nCmdShow = SW_SHOW; // To show the window
	
	ShowWindow(FrameWnd,  nCmdShow);
	UpdateWindow(FrameWnd);

	trayMenu = CreatePopupMenu();
	AppendMenu(trayMenu, MF_STRING, RTFCOPY, "Copy");

	// Create font for input line

	LF.lfHeight = 12;
	LF.lfWidth = 8;
	LF.lfOutPrecision = OUT_DEFAULT_PRECIS;
	LF.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
	LF.lfQuality = DEFAULT_QUALITY;
	LF.lfPitchAndFamily = FIXED_PITCH;
	strcpy(LF.lfFaceName, "FIXEDSYS");

	hFont = CreateFontIndirect(&LF);

	CheckTimer();

	tTimerHandle = SetTimer(NULL, 0, 10000, lptTimerFunc);

	Stream=FindFreeStream();
	
	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	//
	//	Enable Async notification
	//
	
	BPQSetHandle(Stream, FrameWnd);

	GetVersionInfo(NULL);

	sprintf(Title,"BPQTerminal Version %s", VersionString);

	SetWindowText(FrameWnd, Title);
		
	// Get config from Registry 

	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\BPQTermMDI");

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Sessions",0,			
			(ULONG *)&Type,(UCHAR *)&Sessions,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&tempmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ChatMode",0,			
			(ULONG *)&Type,(UCHAR *)&ChatMode, (ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MONColour",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorColour,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MonitorNODES",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorNODES,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SendDisconnected",0,			
			(ULONG *)&Type,(UCHAR *)&SendDisconnected,(ULONG *)&Vallen);
	
		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

		Vallen=99;
		retCode = RegQueryValueEx(hKey, "FontName", 0, &Type, FontName, &Vallen);

		if (FontName[0] == 0)
			strcpy(FontName, "FixedSys");

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CharSet", 0, &Type, (UCHAR *)&CharSet, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CodePage", 0, &Type, (UCHAR *)&CodePage, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontSize", 0, &Type, (UCHAR *)&FontSize, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontWidth", 0, &Type, (UCHAR *)&FontWidth, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "RestoreWindows", 0, &Type, (UCHAR *)&RestoreWindows, &Vallen);

	}

//	OutputData.CharWidth = FontWidth;

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(FrameWnd,	&Rect);
	}

	MoveWindow(FrameWnd, Rect.left, Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	hPopMenu1 = GetSubMenu(hMonMenu, 1);

	for (n=1; n <= GetNumberofPorts(); n++)
	{
		i = GetPortNumber(n);

		sprintf(msg,"Port %d",i);

		if (tempmask & (1<<(i-1)))
		{
			AppendMenu(hPopMenu1,MF_STRING | MF_CHECKED,BPQBASE + i,msg);
			portmask |= (1<<(i-1));
		}
		else
			AppendMenu(hPopMenu1,MF_STRING | MF_UNCHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMonMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMonMenu,BPQMTX,MF_UNCHECKED);

	if (mcomparam & 1)
		CheckMenuItem(hMonMenu,BPQMCOM,MF_CHECKED);
	else
		CheckMenuItem(hMonMenu,BPQMCOM,MF_UNCHECKED);
	
	if (RestoreWindows)
	{
		CheckMenuItem(hMonMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
		CheckMenuItem(hTermCfgMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
	}
	else
	{
		CheckMenuItem(hMonMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
		CheckMenuItem(hTermCfgMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
	}

  	if (Bells & 1)
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_UNCHECKED);

  	if (SendDisconnected & 1)
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hMonMenu,BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hMonMenu,MONCOLOUR, (MonitorColour) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(FrameWnd);	

	if (portmask) applflags |= 0x80;

	SetAppl(Stream, applflags, APPLMASK);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
	
		sprintf(Title, "BPQTermMDI");
		AddTrayMenuItem(FrameWnd, Title);
	}

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(FrameWnd, SW_HIDE);
		else
			ShowWindow(FrameWnd, nCmdShow);
	else
		ShowWindow(FrameWnd, nCmdShow);

	UpdateWindow(FrameWnd);

	// Allocate another incoming stream

	Stream2 = FindFreeStream();
	
	if (Stream2 < 255)
	{
		BPQSetHandle(Stream2, FrameWnd);
		SetAppl(Stream2, 2, APPLMASK);
	}
		
	Vallen=80;
	RegQueryValueEx(hKey, "MonSize", 0 , &Type, &Size[0], &Vallen);

	CreateMonitorWindow(Size);

	if (RestoreWindows)
	{
		int OffsetH, OffsetW;
		struct ConsoleInfo * Cinfo;
		int i;
		char Item[20];

		GetWindowRect(FrameWnd, &Rect);
		OffsetH = Rect.bottom - Rect.top;
		OffsetW = Rect.right - Rect.left;
		GetClientRect(FrameWnd, &Rect);
		OffsetH -= Rect.bottom;
		OffsetW -= Rect.right;

		for (i = 1; i <= Sessions; i++)
		{
			Cinfo = CreateChildWindow(0);
			if (Cinfo)
			{
				sprintf(Item, "SessSize%d", i);

				Vallen=80;
				RegQueryValueEx(hKey, Item, 0 , &Type, &Size[0], &Vallen);

				sscanf(Size,"%d,%d,%d,%d", &Cinfo->Left, &Cinfo->Height, &Cinfo->Top, &Cinfo->Width);

				if (Cinfo->Height < 30)
				{
					//Was probably minimized. Set to a sensible size and minimize it
					
					MoveWindow(Cinfo->hConsole,  i * 50 , i * 50, (Rect.right * 2)/3, (Rect.bottom * 2 /3), TRUE);
					ShowWindow(Cinfo->hConsole, SW_SHOWMINIMIZED);
				}
				else
				{
					MoveWindow(Cinfo->hConsole, Cinfo->Left - OffsetW/2 + 4, Cinfo->Top - OffsetH, Cinfo->Width, Cinfo->Height, TRUE);
					MoveWindows(Cinfo);
				}
			}
		}

	}
	RegCloseKey(hKey);	

	return TRUE;
}

#endif

/*
BOOL InitInstancex(HINSTANCE hInstance, int nCmdShow)
{
	int i, n, tempmask=0xffff;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	TEXTMETRIC tm; 
	HDC dc;
	struct RTFTerm * OPData;

	hInst = hInstance; // Store instance handle in our global variable

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE = TRUE;
		Debugprintf("Running under WINE");
	}

	REGTREE = GetRegistryKey();

	MinimizetoTray=GetMinimizetoTrayFlag();

	// Create a dialog box as the main window

	hWnd=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hWnd)
        return (FALSE);

	MainWnd=hWnd;

	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hWnd, 118); 
//	hwndOutput = GetDlgItem(hWnd, 117); 
	hwndSplit = GetDlgItem(hWnd, 119); 
	hwndMon = GetDlgItem(hWnd, 116); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigMonProc = (WNDPROC)SetWindowLong(hwndMon, GWL_WNDPROC, (LONG)MonProc);

	// Get config from Registry 

	sprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQTerminal\\Session%d",Sessno);

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"APPLMASK",0,			
			(ULONG *)&Type,(UCHAR *)&APPLMASK,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&tempmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"AutoConnect",0,			
			(ULONG *)&Type,(UCHAR *)&AUTOCONNECT,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MONColour",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorColour,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MonitorNODES",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorNODES,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SendDisconnected",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);
	

	
		Vallen=8;
		retCode = RegQueryValueEx(hKey,"Split",0,			
			(ULONG *)&Type,(UCHAR *)&Split,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

		Vallen=99;
		retCode = RegQueryValueEx(hKey, "FontName", 0, &Type, FontName, &Vallen);

		if (FontName[0] == 0)
			strcpy(FontName, "FixedSys");

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CharSet", 0, &Type, (UCHAR *)&CharSet, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CodePage", 0, &Type, (UCHAR *)&CodePage, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontSize", 0, &Type, (UCHAR *)&FontSize, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "FontWidth", 0, &Type, (UCHAR *)&FontWidth, &Vallen);

		RegCloseKey(hKey);
	}

	OutputData.CharWidth = FontWidth;

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	Height = Rect.bottom-Rect.top;
	Width = Rect.right-Rect.left;

#pragma warning(push)
#pragma warning(disable:4244)
	SplitPos=Height*Split;
#pragma warning(pop)

	SetupRTFHddr();

	// Create a Rich Text Control 

	OPData = &OutputData;

	OPData->SendHeader = TRUE;
	OPData->Finished = TRUE;
	OPData->CurrentColour = 1;

	LoadLibrary("riched20.dll");

	hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
		WS_CHILD |  WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_VSCROLL | ES_READONLY,
		6,145,290,130, MainWnd, NULL, hInstance, NULL);

	// Register for Mouse Events for Copy/Paste
	
	SendMessage(hwndOutput, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
	SendMessage(hwndOutput, EM_EXLIMITTEXT, 0, MAXLINES * LINELEN);

	trayMenu = CreatePopupMenu();

	AppendMenu(trayMenu,MF_STRING,40000,"Copy");

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindows();

	Stream=FindFreeStream();
	
	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	//
	//	Enable Async notification
	//
	
	BPQSetHandle(Stream, hWnd);

	GetVersionInfo(NULL);

	sprintf(Title,"BPQTerminal Version %s - using stream %d", VersionString, Stream);

	SetWindowText(hWnd,Title);
		

//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in DL (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.
//
//		Top bit of flags controls monitoring


//	if (APPLMASK & 0x01)	CheckMenuItem(hMenu,BPQAPPL1,MF_CHECKED);
	if (APPLMASK & 0x02)	CheckMenuItem(hTermCfgMenu,BPQCHAT,MF_CHECKED);
//	if (APPLMASK & 0x04)	CheckMenuItem(hMenu,BPQAPPL3,MF_CHECKED);
//	if (APPLMASK & 0x08)	CheckMenuItem(hMenu,BPQAPPL4,MF_CHECKED);
//	if (APPLMASK & 0x10)	CheckMenuItem(hMenu,BPQAPPL5,MF_CHECKED);
//	if (APPLMASK & 0x20)	CheckMenuItem(hMenu,BPQAPPL6,MF_CHECKED);
//	if (APPLMASK & 0x40)	CheckMenuItem(hMenu,BPQAPPL7,MF_CHECKED);
//	if (APPLMASK & 0x80)	CheckMenuItem(hMenu,BPQAPPL8,MF_CHECKED);

// CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
// MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
// MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
//		0x40 = Send Keepalives

//	if (applflags & 0x01)	CheckMenuItem(hMenu,BPQFLAGS1,MF_CHECKED);
//	if (applflags & 0x02)	CheckMenuItem(hMenu,BPQFLAGS2,MF_CHECKED);
//	if (applflags & 0x04)	CheckMenuItem(hMenu,BPQFLAGS3,MF_CHECKED);
//	if (applflags & 0x40)	CheckMenuItem(hMenu,BPQFLAGS4,MF_CHECKED);


	hPopMenu1=GetSubMenu(hMonMenu,1);

	for (n=1;n <= GetNumberofPorts();n++)
	{
		i = GetPortNumber(n);

		sprintf(msg,"Port %d",i);

		if (tempmask & (1<<(i-1)))
		{
			AppendMenu(hPopMenu1,MF_STRING | MF_CHECKED,BPQBASE + i,msg);
			portmask |= (1<<(i-1));
		}
		else
			AppendMenu(hPopMenu1,MF_STRING | MF_UNCHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMonMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMonMenu,BPQMTX,MF_UNCHECKED);

	if (mcomparam & 1)
		CheckMenuItem(hMonMenu,BPQMCOM,MF_CHECKED);
	else
		CheckMenuItem(hMonMenu,BPQMCOM,MF_UNCHECKED);
	
	if (AUTOCONNECT)
		CheckMenuItem(hTermCfgMenu,BPQAUTOCONNECT,MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQAUTOCONNECT,MF_UNCHECKED);

  	if (Bells & 1)
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQBELLS, MF_UNCHECKED);

  	if (SendDisconnected & 1)
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQSendDisconnected, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hTermCfgMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hMonMenu,BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hMonMenu,MONCOLOUR, (MonitorColour) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	if (portmask) applflags |= 0x80;

	SetAppl(Stream, applflags, APPLMASK);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);
	


	DragCursor = LoadCursor(hInstance, "IDC_DragSize");
	Cursor = LoadCursor(NULL, IDC_ARROW);

	GetCallsign(Stream, callsign);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
	
		sprintf(Title,"BPQTerminal Stream %d",Stream);
		AddTrayMenuItem(hWnd, Title);
	}

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	CheckTimer();

	TimerHandle = SetTimer(NULL, 0, 10000, lpTimerFunc);

	dc = GetDC(hwndOutput);
	GetTextMetrics(dc, &tm);

	return (TRUE);
}
*/
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//


LRESULT CALLBACK MonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct ConsoleInfo * Cinfo = &MonWindow;
	LPRECT lprc;
	int wmId, wmEvent;
	int i;
	RECT Rect;

	switch(message)
	{
	case WM_CREATE:
		break;

	case WM_VSCROLL:
		break;

	case WM_NOTIFY:
	{
		const MSGFILTER * pF = (MSGFILTER *)lParam;
		POINT pos;
		CHARRANGE Range;

		if(pF->nmhdr.hwndFrom == Cinfo->hwndOutput)
		{
			if(pF->msg == WM_VSCROLL)
			{
//				int Command = LOWORD(pF->wParam);
//				int Pos = HIWORD(pF->wParam);
				
//				Cinfo->Thumb = SendMessage(Cinfo->hwndOutput, EM_GETTHUMB, 0, 0);

				DoRefresh(Cinfo);
				break;		
			}

			if(pF->msg == WM_KEYUP)
			{
				if (pF->wParam == VK_PRIOR || pF->wParam == VK_NEXT)
				{
//					Cinfo->Thumb = SendMessage(Cinfo->hwndOutput, EM_GETTHUMB, 0, 0);
					DoRefresh(Cinfo);
				}
			}
			
			if(pF->msg == WM_RBUTTONDOWN)
			{
				// Only allow popup if something is selected

				SendMessage(Cinfo->hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
				if (Range.cpMin == Range.cpMax)
					return TRUE;

				GetCursorPos(&pos);
				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return TRUE;
			}
		}
		break;
	}
	case WM_MDIACTIVATE:
		 
		if (lParam == (LPARAM) hWnd)
		{
			RECT Rect;
			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hMonCfgMenu, "Config");
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hMonEdtMenu, "Edit");
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hMonHlpMenu, "Help");

			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM)hWndMenu);

			// Check Window is visible

			GetWindowRect(FrameWnd, &FRect);

			if (GetWindowRect(hWnd, &Rect))
			{
				if (Rect.top > FRect.bottom || Rect.left > FRect.right)
					MoveWindow(hWnd, FRect.top + 100, FRect.left+ 100, 300, 200, 1);
			}
		}
		else
		{
			RemoveMenu(hBaseMenu, 3, MF_BYPOSITION);
			RemoveMenu(hBaseMenu, 2, MF_BYPOSITION);
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		}
		DrawMenuBar(FrameWnd);
	 
		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	

	case WM_SIZING:

		lprc = (LPRECT) lParam;

		Cinfo->Height = lprc->bottom-lprc->top;
		Cinfo->Width = lprc->right-lprc->left;
		Cinfo->Top = lprc->top;
		Cinfo->Left = lprc->left;

		MoveWindows(Cinfo);
			
		break;

	case WM_SIZE:

		MoveWindows(Cinfo);		

		// Drop through to Move


	case WM_MOVE:

		GetWindowRect(hWnd,	&Rect);

		Cinfo->Height = Rect.bottom-Rect.top;
		Cinfo->Width = Rect.right-Rect.left;
		Cinfo->Top = Rect.top;
		Cinfo->Left = Rect.left;

		// Male relative to Frame

		GetWindowRect(FrameWnd, &Rect);

		Cinfo->Top -= Rect.top ;
		Cinfo->Left -= Rect.left;

		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		
		if (wmId > BPQBASE && wmId < BPQBASE + 33)
		{
			TogglePort(hWnd, wmId, 0x1 << (wmId - (BPQBASE + 1)));
			break;
		}

		switch (wmId)
		{
		case StopALLMon:
			
			for (i=1; i <= GetNumberofPorts();i++)
			{
				CheckMenuItem(hMonCfgMenu, BPQBASE + GetPortNumber(i), MF_UNCHECKED);
			}
			portmask = 0;
			applflags &= 0x7f;

			SetAppl(Stream,applflags,APPLMASK);

			break;

		case RTFCOPY:
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			CHARRANGE Range;

			// Copy Rich Text Selection to Clipboard
	
			SendMessage(Cinfo->hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
	
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Range.cpMax - Range.cpMin + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
	
				if (OpenClipboard(Cinfo->hConsole))
				{
					len = SendMessage(Cinfo->hwndOutput, EM_GETSELTEXT  , 0, (WPARAM)ptr);

					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

			SetFocus(Cinfo->hwndInput);

			break;
		}

		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;
			
		case MON_UI_ONLY:

			ToggleMON_UI_ONLY(hWnd);
			break;
			
		case BPQMNODES:

			ToggleParam(hMonCfgMenu, &MonitorNODES, BPQMNODES);
			break;

		case MONITORAPRS:

			ToggleParam(hMonCfgMenu, &MonitorAPRS, MONITORAPRS);
			break;

		case MONCOLOUR:

			ToggleParam(hMonCfgMenu, &MonitorColour, MONCOLOUR);
			break;

		case BPQLogMonitor:

			ToggleParam(hMonCfgMenu, &LogMonitor, BPQLogMonitor);
			break;


		case BPQCLEARMON:

			for (i = 0; i < MAXLINES; i++)
			{
				Cinfo->OutputScreen[i][0] = 0;
			}

			Cinfo->CurrentLine = 0;
			DoRefresh(Cinfo);

			break;

		case BPQCOPYMON:

			CopyRichTextToClipboard(Cinfo->hwndOutput);
			break;

		case ID_WINDOWS_RESTORE:

			ToggleRestoreWindows();
			break;

		}
		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{
		case SC_RESTORE:

			Cinfo->Minimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			DoRefresh(Cinfo);

			break;

		case SC_MINIMIZE: 

			Cinfo->Minimized = TRUE;
			break;
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	}
	return DefMDIChildProc(hWnd, message, wParam, lParam); //Frame window calls DefFrameProc rather than DefWindowProc
}

struct ConsoleInfo * FontCinfo;


LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct ConsoleInfo * Cinfo;
	struct ConsoleInfo * last = NULL;

	LPRECT lprc;
	int wmId, wmEvent;
	int i;
	RECT Rect;
//	MINMAXINFO * mmi;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hConsole == hWnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = &InitHeader;


	switch(message)
	{

/*	case WM_GETMINMAXINFO:

		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 800;
		mmi->ptMaxSize.y = 600;
		mmi->ptMaxTrackSize.x = 800;
		mmi->ptMaxTrackSize.y = 600;
	
		return DefMDIChildProc(hWnd, message, wParam, lParam); 
*/

	case WM_CTLCOLOREDIT:
		
		if (Cinfo->Scrolled)
		{
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LONG)GetStockObject(LTGRAY_BRUSH);
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam); 

	case WM_NOTIFY:
	{
		const MSGFILTER * pF = (MSGFILTER *)lParam;
		POINT pos;
		CHARRANGE Range;

		if(pF->nmhdr.hwndFrom == Cinfo->hwndOutput)
		{
			if(pF->msg == WM_VSCROLL)
			{
//				int Command = LOWORD(pF->wParam);
//				int Pos = HIWORD(pF->wParam);
				
//				Cinfo->Thumb = SendMessage(Cinfo->hwndOutput, EM_GETTHUMB, 0, 0);

				DoRefresh(Cinfo);
				break;		
			}

			if(pF->msg == WM_KEYUP)
			{
				if (pF->wParam == VK_PRIOR || pF->wParam == VK_NEXT)
				{
//					Cinfo->Thumb = SendMessage(Cinfo->hwndOutput, EM_GETTHUMB, 0, 0);
					DoRefresh(Cinfo);
				}
			}
			
			if(pF->msg == WM_RBUTTONDOWN)
			{
				// Only allow popup if something is selected

				SendMessage(Cinfo->hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
				if (Range.cpMin == Range.cpMax)
					return TRUE;

				GetCursorPos(&pos);
				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return TRUE;
			}
		}
		break;
	}
	

    case WM_MDIACTIVATE:
	
		// Set the system info menu when getting activated
			
			if (lParam == (LPARAM) hWnd)
			{
				 // Activate

				 Cinfo->Active = TRUE;

				RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hTermActMenu, "Actions");
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hTermCfgMenu, "Config");
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hTermEdtMenu, "Edit");
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hTermHlpMenu, "Help");

				SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM)hWndMenu);

				if (Cinfo->CONNECTED)
				{
					EnableDisconnectMenu(hWnd);
					DisableConnectMenu(hWnd);
				}
				else
				{
					DisableDisconnectMenu(hWnd);
					EnableConnectMenu(hWnd);
				}		
				SetFocus(Cinfo->hwndInput);
			 }
			 else
			 {
			 	 // Deactivate
				 
				 Cinfo->Active = FALSE;
				 RemoveMenu(hBaseMenu, 4, MF_BYPOSITION);
				 RemoveMenu(hBaseMenu, 3, MF_BYPOSITION);
				 RemoveMenu(hBaseMenu, 2, MF_BYPOSITION);

				 SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
			 }
			 
			 DrawMenuBar(FrameWnd);
			 
	
		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);
		 	
	case WM_SIZING:

		lprc = (LPRECT) lParam;

		Cinfo->Height = lprc->bottom-lprc->top;
		Cinfo->Width = lprc->right-lprc->left;
		Cinfo->Top = lprc->top;
		Cinfo->Left = lprc->left;

		MoveWindows(Cinfo);
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);


	case WM_SIZE:

		MoveWindows(Cinfo);		

		// Drop through to Move


	case WM_MOVE:

		GetWindowRect(hWnd,	&Rect);

		Cinfo->Height = Rect.bottom-Rect.top;
		Cinfo->Width = Rect.right-Rect.left;
		Cinfo->Top = Rect.top;
		Cinfo->Left = Rect.left;

		// Male relative to Frame

		GetWindowRect(FrameWnd, &Rect);

		Cinfo->Top -= Rect.top ;
		Cinfo->Left -= Rect.left;

		return DefMDIChildProc(hWnd, message, wParam, lParam); 

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
		case RTFCOPY:
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			CHARRANGE Range;

			// Copy Rich Text Selection to Clipboard
	
			SendMessage(Cinfo->hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
	
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Range.cpMax - Range.cpMin + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
	
				if (OpenClipboard(Cinfo->hConsole))
				{
					len = SendMessage(Cinfo->hwndOutput, EM_GETSELTEXT  , 0, (WPARAM)ptr);

					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

		
			SetFocus(Cinfo->hwndInput);
			break;	
		
		}

		case BPQCONNECT:

			SessionControl(Cinfo->BPQStream, 1, 0);
			break;
			        
		case BPQDISCONNECT:
			
			SessionControl(Cinfo->BPQStream, 2, 0);
			break;


		case BPQHELP:

			ShellExecute(hWnd,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQTerminal.htm",
				"", NULL, SW_SHOWNORMAL); 
			break;

		case ID_WINDOWS_RESTORE:

			ToggleRestoreWindows();
			break;

		case ID_SETUP_FONT:

			FontCinfo = Cinfo;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_FONT), hWnd, FontConfigWndProc);
			i =  GetLastError();
			break;
			
		case BPQCLEAROUT:

			for (i = 0; i < MAXLINES; i++)
			{
				Cinfo->OutputScreen[i][0] = 0;
			}

			Cinfo->CurrentLine = 0;
			DoRefresh(Cinfo);

			break;


		case BPQCOPYOUT:
		
			CopyRichTextToClipboard(Cinfo->hwndOutput);
			break;

		case BPQBELLS:

			ToggleParam(hTermCfgMenu, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hTermCfgMenu, &StripLF, BPQStripLF);
			break;

		case BPQLogOutput:

			ToggleParam(hTermCfgMenu, &LogOutput, BPQLogOutput);
			break;

		case CHATTERM:

			ToggleParam(hTermCfgMenu, &ChatMode, CHATTERM);
			break;

		case BPQSendDisconnected:

			ToggleParam(hTermCfgMenu, &SendDisconnected, BPQSendDisconnected);
			break;

		case ID_WARNWRAP:

			ToggleParam(hTermCfgMenu, &WarnWrap, ID_WARNWRAP);
			break;

		case ID_WRAP:

			ToggleParam(hTermCfgMenu, &WrapInput, ID_WRAP);
			break;

		case ID_FLASHONBELL:

			ToggleParam(hTermCfgMenu, &FlashOnBell, ID_FLASHONBELL);
			break;
		}

		break;

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{
		case SC_RESTORE:

			Cinfo->Minimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			DoRefresh(Cinfo);

			break;

		case SC_MINIMIZE: 

			Cinfo->Minimized = TRUE;
			break;
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);


	case WM_CLOSE:
		break; // Go on to call DefMDIChildProc

	case WM_DESTROY:

		// Close session and release stream

#define GWL_WNDPROC         (-4)

		SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, (LONG) Cinfo->wpOrigInputProc); 
		
		SessionControl(Cinfo->BPQStream, 2, 0);

		if (Cinfo->Incoming == FALSE)
			DeallocateStream(Cinfo->BPQStream);

		// Free Scrollback

		for (i = 0; i < MAXSTACK ; i++)
		{
			if (Cinfo->KbdStack[i])
			{
				free(Cinfo->KbdStack[i]);
				Cinfo->KbdStack[i] = NULL;
			}
		}

		Sleep(500);

		if (Cinfo->readbuff)
			free(Cinfo->readbuff);

		// Remove from List


		for (Cinfo = ConsHeader; Cinfo; last = Cinfo, Cinfo = Cinfo->next)
		{
				if (Cinfo->hConsole == hWnd)
				{
					if (last)
					{
						last->next = Cinfo->next;
						free(Cinfo);
						return 0;
					}
					else
					{
						// First in list
				
						ConsHeader = Cinfo->next;
						free(Cinfo);
						return 0;
					}
				}
		}
	
		return DefMDIChildProc(hWnd, message, wParam, lParam);
	}

	return DefMDIChildProc(hWnd, message, wParam, lParam); 
}

#ifndef MDIKERNEL

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	UCHAR Buffer[1000];
	UCHAR * buf = Buffer; 
	CLIENTCREATESTRUCT MDIClientCreateStruct; // Structure to be used for MDI client area
	HWND m_hwndSystemInformation = 0;
	struct ConsoleInfo * Cinfo;
	int i, disp;
	HKEY hKey=0;
	char Size[80], Item[20];

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(wParam);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(wParam);
				
		if (lParam & BPQStateChange)
			DoStateChange(wParam);

		return (0);
	}
	
	switch (message) { 

		case WM_CREATE:
		// On creation of main frame, create the MDI client area
		MDIClientCreateStruct.hWindowMenu	= NULL;
		MDIClientCreateStruct.idFirstChild	= IDM_FIRSTCHILD;
		
		ghMDIClientArea = CreateWindow(TEXT("MDICLIENT"), // predefined value for MDI client area
									NULL, // no caption required
									WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
									0, // No need to give any x/y or height/width since this client
									   // will just be used to get client windows created, effectively
									   // in the main window we will be seeing the mainframe window client area itself.
									0, 
									0,
									0,
									hWnd,
									NULL,
									hInstance,
									(void *) &MDIClientCreateStruct);

		return 0;


	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_NEWWINDOW:

			CreateChildWindow(0);

			break;

		case ID_HELP_ABOUT:
			MessageBox(FrameWnd, TEXT("Sigma by Sharath C V"), TEXT("Sigma"), MB_OK);
			break;
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;
		case ID_WINDOWS_CASCADE:
			SendMessage(ghMDIClientArea, WM_MDICASCADE, 0, 0);
			return 0;
					
		case ID_WINDOWS_TILE:
			SendMessage(ghMDIClientArea, WM_MDITILE , MDITILE_HORIZONTAL, 0);
			return 0;

 
        // Handle MDI Window commands
            
		default:
        {
			if(LOWORD(wParam) >= IDM_FIRSTCHILD)
			{
				DefFrameProc(hWnd, ghMDIClientArea, message, wParam, lParam);
			}
			else 
			{
				HWND hChild = (HWND)SendMessage(ghMDIClientArea, WM_MDIGETACTIVE,0,0);

				if(hChild)
					SendMessage(hChild, WM_COMMAND, wParam, lParam);
            }
        }
  
			break;
		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case  SC_MINIMIZE: 

			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
		default:

			return (DefFrameProc(hWnd, ghMDIClientArea, message, wParam, lParam));

		}

		case WM_DESTROY:
		
			GetWindowRect(hWnd,	&Rect);	// For save soutine

  			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			// Save config
	
			RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS,  NULL, &hKey, &disp);

			RegSetValueEx(hKey,"ChatMode",0,REG_DWORD,(BYTE *)&ChatMode,4);
			RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&portmask,4);
			RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
			RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
			RegSetValueEx(hKey,"SendDisconnected",0,REG_DWORD,(BYTE *)&SendDisconnected,4);
			RegSetValueEx(hKey,"RestoreWindows",0,REG_DWORD,(BYTE *)&RestoreWindows,4);
			RegSetValueEx(hKey,"MTX",0,REG_DWORD,(BYTE *)&mtxparam,4);
			RegSetValueEx(hKey,"MCOM",0,REG_DWORD,(BYTE *)&mcomparam,4);
			RegSetValueEx(hKey,"MONColour",0,REG_DWORD,(BYTE *)&MonitorColour,4);
			RegSetValueEx(hKey,"MonitorNODES",0,REG_DWORD,(BYTE *)&MonitorNODES,4);

			sprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
			RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

			// Close any sessions

			i = 0;

			for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
			{
				i++;

				sprintf(Size, "%d,%d,%d,%d", Cinfo->Left, Cinfo->Height, Cinfo->Top, Cinfo->Width);
				sprintf(Item, "SessSize%d", i);
				RegSetValueEx(hKey, Item, 0, REG_SZ, (BYTE *)&Size, strlen(Size));

				SessionControl(Cinfo->BPQStream, 2, 0);
				DeallocateStream(Cinfo->BPQStream);
			}

			RegSetValueEx(hKey,"Sessions",0,REG_DWORD,(BYTE *)&i,4);

			sprintf(Size, "%d,%d,%d,%d", MonWindow.Left, MonWindow.Height, MonWindow.Top, MonWindow.Width);
			RegSetValueEx(hKey, "MonSize", 0, REG_SZ, (BYTE *)&Size, strlen(Size));

			RegCloseKey(hKey);

			break;

		default:
			return (DefFrameProc(hWnd, ghMDIClientArea, message, wParam, lParam));

	}
	return (0);
}

#endif

/*
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	UCHAR Buffer[1000];
	UCHAR * buf = Buffer;
    TEXTMETRIC tm; 
    int y;  
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 
	HWND m_hwndSystemInformation = 0;

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(wParam);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(wParam);
				
		if (lParam & BPQStateChange)
			DoStateChange(wParam);

		return (0);
	}
	
	switch (message) { 

	case WM_CTLCOLOREDIT:
		
		if (OutputData.Scrolled)
		{
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LONG)GetStockObject(LTGRAY_BRUSH);
		}
		return (DefWindowProc(hWnd, message, wParam, lParam));


        case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
  //          lpmis->itemHeight = 15; 
            return TRUE; 
 
        case WM_DRAWITEM: 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If there are no list box items, skip this message. 
 
            if (lpdis->itemID == -1) 
            { 
               return TRUE; 
            } 
 
            switch (lpdis->itemAction) 
            { 
				case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 
				  // if Chat Console, and message has a colour eacape, action it 
					
					SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM) Buffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm);

					if (lpdis->hwndItem == hwndOutput)
					{
						CharWidth = tm.tmAveCharWidth;
						maxlinelen = ClientWidth/CharWidth - 1;
					}
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

					if (Buffer[0] == 0x1b)
					{
						SetTextColor(lpdis->hDC,  Colours[Buffer[1] - 10]);
						buf += 2;
					}
//					SetBkColor(lpdis->hDC, 0);

                    TextOut(lpdis->hDC, 
                        6, 
                        y, 
                        buf, 
                        strlen(buf)); 						
 
 //					SetTextColor(lpdis->hDC, OldColour);

                    break; 
			}

			return TRUE;

		case WM_ACTIVATE:

			//fActive = LOWORD(wParam);           // activation flag 
			//fMinimized = (BOOL) HIWORD(wParam); // minimized flag 
			//	hwnd = (HWND) lParam;               // window handle 
 
			SetFocus(hwndInput);
			break;



	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		if (wmId > BPQBASE && wmId < BPQBASE + 32)
		{
			TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
			break;
		}
		
		switch (wmId) {

		case 40000:
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			CHARRANGE Range;

			// Copy Rich Text Selection to Clipboard
	
			SendMessage(hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
	
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Range.cpMax - Range.cpMin + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
	
				if (OpenClipboard(MainWnd))
				{
					len = SendMessage(hwndOutput, EM_GETSELTEXT  , 0, (WPARAM)ptr);

					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

			SetFocus(hwndInput);
			return TRUE;
		}
		
		case ID_SETUP_FONT:

	//		DialogBox(hInst, MAKEINTRESOURCE(IDD_FONT), hWnd, FontConfigWndProc);
			break;

        
		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;

		case BPQCONNECT:
		
			SessionControl(Stream, 1, 0);
			break;
			        
		case BPQDISCONNECT:
			
			SessionControl(Stream, 2, 0);
			break;


		case ID_ACTION_RESETWINDOWSPLIT:
			
			Split = 0.5;
			SplitPos=Height*Split;
			MoveWindows();

			break;
			
		case BPQAUTOCONNECT:

			ToggleAutoConnect(hWnd);
			break;
			
		case BPQAPPL1:

			ToggleAppl(hWnd,BPQAPPL1,0x1);
			break;

		case BPQAPPL2:

			ToggleAppl(hWnd,BPQAPPL2,0x2);
			break;

		case BPQAPPL3:

			ToggleAppl(hWnd,BPQAPPL3,0x4);
			break;

		case BPQAPPL4:

			ToggleAppl(hWnd,BPQAPPL4,0x8);
			break;

		case BPQAPPL5:

			ToggleAppl(hWnd,BPQAPPL5,0x10);
			break;

		case BPQAPPL6:

			ToggleAppl(hWnd,BPQAPPL6,0x20);
			break;

		case BPQAPPL7:

			ToggleAppl(hWnd,BPQAPPL7,0x40);
			break;

		case BPQAPPL8:

			ToggleAppl(hWnd,BPQAPPL8,0x80);
			break;

		case BPQFLAGS1:

			ToggleFlags(hWnd,BPQFLAGS1,0x01);
			break;

		case BPQFLAGS2:

			ToggleFlags(hWnd,BPQFLAGS2,0x02);
			break;

		case BPQFLAGS3:

			ToggleFlags(hWnd,BPQFLAGS3,0x04);
			break;

		case BPQFLAGS4:

			ToggleFlags(hWnd,BPQFLAGS4,0x40);
			break;

		case BPQBELLS:

			ToggleParam(hTermCfgMenu, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hTermCfgMenu, &StripLF, BPQStripLF);
			break;

		case BPQLogOutput:

			ToggleParam(hTermCfgMenu, &LogOutput, BPQLogOutput);
			break;

		case CHATTERM:

			ToggleParam(hTermCfgMenu, &ChatMode, CHATTERM);
			break;

			
		case BPQSendDisconnected:

			ToggleParam(hTermCfgMenu, &SendDisconnected, BPQSendDisconnected);
			break;

		case BPQMNODES:

			ToggleParam(hTermCfgMenu, &MonitorNODES, BPQMNODES);
			break;

		case MONCOLOUR:

			ToggleParam(hTermCfgMenu, &MonitorColour, MONCOLOUR);
			break;

		case BPQLogMonitor:

			ToggleParam(hTermCfgMenu, &LogMonitor, BPQLogMonitor);
			break;

		case BPQCHAT:

			ToggleChat(hWnd);
			break;

		case BPQCLEARMON:

			SendMessage(hwndMon,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCLEAROUT:

			for (i = 0; i < MAXLINES; i++)
			{
				Cinfo->OutputScreen[i][0] = 0;
			}

			Cinfo->CurrentLine = 0;
			DoRefresh(Cinfo);

			break;

		case BPQCOPYMON:

			CopyListToClipboard(hwndMon);
			break;

		case BPQCOPYOUT:
		
			CopyRichTextToClipboard(hwndOutput);
			break;

		case BPQHELP:

			ShellExecute(hWnd,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQTerminal.htm",
				"", NULL, SW_SHOWNORMAL); 
			break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case  SC_MINIMIZE: 

			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
		default:
		
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

			SplitPos=Height*Split;

			MoveWindows();
			
			return TRUE;

		case WM_SIZE:

			MoveWindows();
			return TRUE;

		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&Rect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         
			SessionControl(Stream, 2, 0);
			DeallocateStream(Stream);
			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

*/

//static char * KbdStack[20];

//int StackIndex=0;

LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	int i;
	unsigned int TextLen;
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hwndInput == hwnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = &InitHeader;

 
	if (uMsg == WM_KEYUP)
	{
		unsigned int i;
//		Debugprintf("5%x", LOBYTE(HIWORD(lParam)));

		if (LOBYTE(HIWORD(lParam)) == 0x48 && wParam == 0x26)
		{
			// Scroll up

			if (Cinfo->KbdStack[Cinfo->StackIndex] == NULL)
				return TRUE;

			SendMessage(Cinfo->hwndInput, WM_SETTEXT,0,(LPARAM)(LPCSTR) Cinfo->KbdStack[Cinfo->StackIndex]);
			
			for (i = 0; i < strlen(Cinfo->KbdStack[Cinfo->StackIndex]); i++)
			{
				SendMessage(Cinfo->hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(Cinfo->hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}

			Cinfo->StackIndex++;
			if (Cinfo->StackIndex == 20)
				Cinfo->StackIndex = 19;

			return TRUE;
		}

		if (LOBYTE(HIWORD(lParam)) == 0x50 && wParam == 0x28)
		{
			// Scroll up

			Cinfo->StackIndex--;
			if (Cinfo->StackIndex < 0)
				Cinfo->StackIndex = 0;

			if (Cinfo->KbdStack[Cinfo->StackIndex] == NULL)
				return TRUE;
			
			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) Cinfo->KbdStack[Cinfo->StackIndex]);

			for (i = 0; i < strlen(Cinfo->KbdStack[Cinfo->StackIndex]); i++)
			{
				SendMessage(Cinfo->hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(Cinfo->hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}
			
			return TRUE;
		}
	}
				

	if (uMsg == WM_CHAR) 
	{
		TextLen = SendMessage(Cinfo->hwndInput,WM_GETTEXTLENGTH, 0, 0);

		if (TextLen > INPUTLEN-10) Beep(220, 150);
		
		if(WarnWrap || WrapInput)
		{
			TextLen = SendMessage(Cinfo->hwndInput,WM_GETTEXTLENGTH, 0, 0);

			if (WarnWrap)
				if (TextLen == Cinfo->WarnLen) Beep(220, 150);

			if (WrapInput)
				if ((wParam == 0x20) && (TextLen > Cinfo->WrapLen))
					wParam = 13;		// Replace space with Enter

		}

		if (wParam == 13)
		{
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!Cinfo->CONNECTED)
			{
				if (Cinfo->Incoming)		// Incoming call window
					MessageBox(NULL, "Session is for Incoming Calls", "BPQTerm", MB_OK);
				else
					SessionControl(Cinfo->BPQStream, 1, 0);
			}

			Cinfo->kbptr=SendMessage(Cinfo->hwndInput, WM_GETTEXT, INPUTLEN-1, 
				(LPARAM) (LPCSTR)Cinfo->kbbuf);

			Cinfo->StackIndex = 0;

			// Stack it

			if (Cinfo->KbdStack[19])
				free(Cinfo->KbdStack[19]);

			for (i = 18; i >= 0; i--)
			{
				Cinfo->KbdStack[i+1] = Cinfo->KbdStack[i];
			}

			Cinfo->KbdStack[0] = _strdup(Cinfo->kbbuf);

			Cinfo->kbbuf[Cinfo->kbptr]=13;

			// Echo

			Cinfo->CurrentColour = 64;
			WritetoOutputWindow(Cinfo, Cinfo->kbbuf, Cinfo->kbptr+1, TRUE);
			Cinfo->CurrentColour = 1;

			if (Cinfo->Scrolled)
			{
				POINT Point;
				Point.x = 0;
				Point.y = 25000;					// Should be plenty for any font

				SendMessage(Cinfo->hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
				Cinfo->Scrolled = FALSE;
			}


			DoRefresh(Cinfo);
//			ProcessLine(Cinfo, &Cinfo->kbbuf[0], Cinfo->kbptr+1);
			SendMsg(Cinfo->BPQStream, &Cinfo->kbbuf[0], Cinfo->kbptr+1);

			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
	
			Cinfo->kbbuf[0]=0x1a;
			Cinfo->kbbuf[1]=13;

//			ProcessLine(Cinfo, &Cinfo->kbbuf[0], 2);
			SendMsg(Cinfo->BPQStream, &Cinfo->kbbuf[0], 2);

			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
 
	}

    return CallWindowProc(Cinfo->wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 



 
/*LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	char DisplayLine[200] = "\x1b\xb";
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader[0]; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hwndInput == hwnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = InitHeader;


	if (uMsg == WM_CTLCOLOREDIT)
	{
		HBRUSH Brush = CreateSolidBrush(RGB(0, 255, 0));
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);

		return (LONG)Brush;
	}

	
	if (uMsg == WM_KEYUP)
	{
		unsigned int i;

//		Debugprintf("5%x", LOBYTE(HIWORD(lParam)));

		if (LOBYTE(HIWORD(lParam)) == 0x48 && wParam == 0x26)
		{
			// Scroll up

			if (KbdStack[StackIndex] == NULL)
				return TRUE;

			SendMessage(hwndInput, WM_SETTEXT,0,(LPARAM)(LPCSTR) KbdStack[StackIndex]);
			
			for (i = 0; i < strlen(KbdStack[StackIndex]); i++)
			{
				SendMessage(hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}

			StackIndex++;
			if (StackIndex == 20)
				StackIndex = 19;

			return TRUE;
		}

		if (LOBYTE(HIWORD(lParam)) == 0x50 && wParam == 0x28)
		{
			// Scroll up

			StackIndex--;
			if (StackIndex < 0)
				StackIndex = 0;

			if (KbdStack[StackIndex] == NULL)
				return TRUE;
			
			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) KbdStack[StackIndex]);

			for (i = 0; i < strlen(KbdStack[StackIndex]); i++)
			{
				SendMessage(hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}
			
			return TRUE;
		}
	}



	if (uMsg == WM_CHAR) 
	{
		if (wParam == 13)
		{
			int i;

			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
			
			kbptr=SendMessage(Cinfo->hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);

			// Stack it

			StackIndex = 0;

			if (KbdStack[19])
				free(KbdStack[19]);

			for (i = 18; i >= 0; i--)
			{
				KbdStack[i+1] = KbdStack[i];
			}

			KbdStack[0] = _strdup(kbbuf);

			kbbuf[kbptr]=13;

			SlowTimer = 0;

			// Echo, with set Black escape

			memcpy(&DisplayLine[2], kbbuf, kbptr+1);

			if (OutputData.Scrolled)
			{
				POINT Point;
				Point.x = 0;
				Point.y = 25000;					// Should be plenty for any font

				SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
				OutputData.Scrolled = FALSE;
			}

			WritetoOutputWindow(&OutputData, DisplayLine, kbptr+3);
			DoRefresh(&OutputData);
		
		
			// Replace null with CR, and send to Node

			SendMsg(Stream, &kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

			SendMsg(Stream, &kbbuf[0], 2);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

        return 0; 
		}

	}
 
    return CallWindowProc(Cinfo->wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 
*/



DoStateChange(int Stream)
{
	int port, sesstype, paclen, maxframe, l4window, len;
	int state, change;
	struct ConsoleInfo * Cinfo = NULL;
	char callsign[11] = "";
	char Msg[80];

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->BPQStream == Stream)
			break;
	}

	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.
	
	SessionState(Stream, &state, &change);
		
	if (change == 1)
	{
		if (state == 1)
		{
			// Connected

			if (Cinfo == NULL)
			{
				// Incoming connect. Create a window for it
		
				Cinfo = CreateChildWindow(Stream, FALSE);
				Cinfo->Incoming = TRUE;
				Cinfo->Minimized = FALSE;
				SendMessage(ClientWnd, WM_MDIACTIVATE, (WPARAM)Cinfo->hConsole, 0);
			}

			Cinfo->CONNECTED = TRUE;
			Cinfo->SlowTimer = 0;

			GetConnectionInfo(Stream, callsign,
										 &port, &sesstype, &paclen,
										 &maxframe, &l4window);

			if (Cinfo->Incoming == TRUE)
			{
				char conMsg[] = "Send ^D to disconnect\r";
	
				SendMsg(Stream, conMsg, strlen(conMsg));

				len = sprintf(Msg, "*** Incoming Call from %s\r", callsign);
				WritetoOutputWindow(Cinfo, Msg, len, TRUE);

				PlaySound("IncomingCall", hInstance, SND_RESOURCE | SND_ASYNC);

				ShowWindow(FrameWnd, SW_SHOWNA);

				if (FrameMaximized == TRUE)
					PostMessage(FrameWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				else
					PostMessage(FrameWnd, WM_SYSCOMMAND, SW_SHOWNA, 0);

			}
				
			sprintf(Title,"Stream %d - Connected to %s", Stream, callsign);
			SetWindowText(Cinfo->hConsole, Title);
			DisableConnectMenu(FrameWnd);
			EnableDisconnectMenu(FrameWnd);

		}
		else
		{
			if (Cinfo == NULL)
				return 0;

			Cinfo->CONNECTED=FALSE;
			sprintf(Title,"Stream %d - Disconnected", Stream);
			SetWindowText(Cinfo->hConsole,Title);
			DisableDisconnectMenu(FrameWnd);
			EnableConnectMenu(FrameWnd);

			if (SendDisconnected)
			{
				WritetoOutputWindow(Cinfo, "*** Disconnected\r", 17, TRUE);		
			}
		}
	}

	return (0);

}
		
DoReceivedData(int Stream)
{
	char Msg[300];
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->BPQStream == Stream)
			break;
	}

	if (Cinfo == NULL)
		 return 0;


	if (RXCount(Cinfo->BPQStream) > 0)
	{
		do {

			GetMsg(Cinfo->BPQStream, &Msg[0],&len,&count);

			if (len == 1 && Msg[0] == 0)
				continue;

			if (GetApplNum(Cinfo->BPQStream) == 32)			// Host Appl
			{
				// Check for ^D to disconnect

				if (_memicmp(Msg, "^d\r", 3) == 0)
				{
					SessionControl(Cinfo->BPQStream, 2, 0);
				}
			}

			WritetoOutputWindow(Cinfo, Msg, len, FALSE);
	
			Cinfo->SlowTimer = 0;
		
			if (Cinfo->Active == FALSE)
				FlashWindow(Cinfo->hConsole, TRUE);
//				Beep(440,250);

		} while (count > 0);
		
		DoRefresh(Cinfo);

	}
	return (0);
}

VOID wcstoRTF(char * out, WCHAR * in)
{
	WCHAR * ptr1 = in;
	char * ptr2 = out;
	int val = *ptr1++;

	while (val)
	{
		// May be Code Page or Unicode
		{
			if (val > 255 || val < -255 )
				ptr2 += sprintf(ptr2, "\\u%d ", val);
			else if (val > 127 || val < -128)
				ptr2 += sprintf(ptr2, "\\'%02X", val);
			else
				*(ptr2++) = val;
		}
		val = *ptr1++;
	}
	*ptr2 = 0;
}

DWORD CALLBACK EditStreamCallback(struct ConsoleInfo * Cinfo, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	int ReqLen = cb;
	int i;
	int Line;
	int NewLen;
	int err;

	char LineB[12048];
	WCHAR LineW[12048];

//	if (cb != 4092)
//		return 0;

	if (Cinfo->SendHeader)
	{
		// Return header

		memcpy(lpBuff, RTFHeader, RTFHddrLen);
		*pcb = RTFHddrLen;
		Cinfo->SendHeader = FALSE;
		Cinfo->Finished = FALSE;
		Cinfo->Index = 0;
		return 0;
	}

	if (Cinfo->Finished)
	{
		*pcb = 0;
		return 0;
	}
	
/*
	if (BufferLen > cb)
	{
		memcpy(lpBuff, &Buffer[Offset], cb);
		BufferLen -= cb;
		Offset += cb;
		*pcb = cb;
		return 0;
	}

	memcpy(lpBuff, &Buffer[Offset], BufferLen);

    *pcb = BufferLen;
*/

	// Return 10 line at a time

	for (i = 0; i < 10; i++);
	{
		Line = Cinfo->Index++ + Cinfo->CurrentLine - MAXLINES;

		if (Line <0)
			Line = Line + MAXLINES;

		sprintf(lpBuff, "\\cf%d ", Cinfo->Colourvalue[Line]);
	
		// Handle UTF-8

		NewLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Cinfo->OutputScreen[Line], -1, LineW, 2000); 

		err =  GetLastError();

		if (err == ERROR_NO_UNICODE_TRANSLATION)
		{
			// Input isn't UTF 8. Juat use default 8 bit set#

			strcat(lpBuff, Cinfo->OutputScreen[Line]);
		}
		else
		{
			if (LineW[0] != 0)
			{
				wcstoRTF(LineB, LineW);
				strcat(lpBuff, LineB);
			}
		}
	}
	strcat(lpBuff, "\\line");

	if (Cinfo->Index == MAXLINES)
	{
		Cinfo->Finished = TRUE;
		strcat(lpBuff, "}");
		i = 10;
	}
	
	*pcb = strlen(lpBuff);
	return 0;
}


VOID DoRefresh(struct ConsoleInfo * Cinfo)
{
	EDITSTREAM es = {0};
	int Min, Max, Pos;
	POINT Point;
	SCROLLINFO ScrollInfo;
	int LoopTrap = 0;
	HWND hwndOutput = Cinfo->hwndOutput;

	if(WINE)
		Cinfo->Thumb = 30000;
	else
		Cinfo->Thumb = SendMessage(Cinfo->hwndOutput, EM_GETTHUMB, 0, 0);

	Pos = Cinfo->Thumb + Cinfo->ClientHeight;

	if ((Cinfo->Thumb + Cinfo->ClientHeight) > Cinfo->RTFHeight - 10)		// Don't bother writing to screen if scrolled back
	{
		es.pfnCallback = (EDITSTREAMCALLBACK)EditStreamCallback;
		es.dwCookie = (DWORD_PTR)Cinfo;
		Cinfo->SendHeader = TRUE;
		SendMessage(hwndOutput, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	}

	GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);
	ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_ALL;

	GetScrollInfo(hwndOutput, SB_VERT, &ScrollInfo);

//	Debugprintf("Pos %d Max %d Min %d nMax %d ClientH %d", Pos, Min, Max, ScrollInfo.nMax, Cinfo->ClientHeight);

	if (Cinfo->FirstTime == FALSE)
	{
		// RTF Controls don't immediately scroll to end - don't know why.
		
		Cinfo->FirstTime = TRUE;
		Point.x = 0;
		Point.y = 25000;					// Should be plenty for any font

		while (LoopTrap++ < 20)
		{
			SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		}

		GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
		Cinfo->RTFHeight = Max;
		Point.x = 0;
		Point.y = Cinfo->RTFHeight - ScrollInfo.nPage;
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		Cinfo->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
	}

	Point.x = 0;
	Point.y = Cinfo->RTFHeight - ScrollInfo.nPage;

	if (Cinfo->Thumb > (Point.y - 10))		// Don't Scroll if user has scrolled back 
	{
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);

		if (Cinfo->Scrolled)
		{
			Cinfo->Scrolled = FALSE;
			InvalidateRect(Cinfo->hwndInput, NULL, TRUE);
		}
		return;
	}

	if (!Cinfo->Scrolled)
	{
		Cinfo->Scrolled = TRUE;
		InvalidateRect(Cinfo->hwndInput, NULL, TRUE);
	}
}

/*VOID DoRefresh(struct RTFTerm * OPData)
{
	EDITSTREAM es = {0};
	int Min, Max, Pos;
	POINT Point;
	SCROLLINFO ScrollInfo;
	int LoopTrap = 0;

	if(WINE)
		OPData->Thumb = 30000;
	else
		OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);

	Pos = OPData->Thumb + OutputBoxHeight;

	if (Pos > OPData->RTFHeight - 10)		// Don't bother writing to screen if scrolled back
	{
		es.pfnCallback = EditStreamCallback;
		es.dwCookie = (DWORD_PTR)OPData;
		OPData->SendHeader = TRUE;
		SendMessage(hwndOutput, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	}
//	else
//		Debugprintf("Pos %d RTFHeight %d - Not refreshing", Pos, OPData->RTFHeight);

	GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);
	ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_ALL;

	GetScrollInfo(hwndOutput, SB_VERT, &ScrollInfo);

//	Debugprintf("Thumb %d Pos %d Min %d Max %d nMax %d ClientH %d RTFHeight %d",
//		OPData->Thumb, Pos, Min, Max, ScrollInfo.nMax, OutputBoxHeight, OPData->RTFHeight);

	if (OPData->FirstTime == FALSE)
	{
		// RTF Controls don't immediately scroll to end - don't know why.
		
		OPData->FirstTime = TRUE;
		Point.x = 0;
		Point.y = 25000;					// Should be plenty for any font

		while (LoopTrap++ < 20)
		{
			SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		}

		GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
		OPData->RTFHeight = Max;
		Point.x = 0;
		Point.y = OPData->RTFHeight - ScrollInfo.nPage;
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
	}

	Point.x = 0;
	Point.y = OPData->RTFHeight - ScrollInfo.nPage;

	if (OPData->Thumb > (Point.y - 10))		// Don't Scroll if user has scrolled back 
	{
//		Debugprintf("Scrolling to %d", Point.y);
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		if (OPData->Scrolled)
		{
			OPData->Scrolled = FALSE;
			InvalidateRect(hwndInput, NULL, TRUE);
		}
		return;
	}

//	Debugprintf("Thumb = %d Point.y = %d - Not Scrolling", OPData->Thumb, Point.y);

	if (!OPData->Scrolled)
	{
		OPData->Scrolled = TRUE;
		InvalidateRect(hwndInput, NULL, TRUE);
	}
}

*/
VOID AddLinetoWindow(struct ConsoleInfo * Cinfo, char * Line)
{
	int Len = strlen(Line);
	char * ptr1 = Line;
	char * ptr2;
	int l, Index;
	char LineCopy[LINELEN * 2];
	
	if (strlen(Line) > 200)
	{
		Line[199] = 0;
	}

	if (Line[0] ==  0x1b && Len > 1)
	{
		// Save Colour Char
		
		Cinfo->CurrentColour = Line[1] - 10;
		ptr1 +=2;
		Len -= 2;
	}

	strcpy(Cinfo->OutputScreen[Cinfo->CurrentLine], ptr1);

	// Look for chars we need to escape (\  { })

	ptr1 = Cinfo->OutputScreen[Cinfo->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '\\');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ++ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy Including found char
			Index += l;
			LineCopy[Index++] = '\\';
			Len++;
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, '\\');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(Cinfo->OutputScreen[Cinfo->CurrentLine], LineCopy);
	}

	ptr1 = Cinfo->OutputScreen[Cinfo->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '{');

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '{';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '{');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(Cinfo->OutputScreen[Cinfo->CurrentLine], LineCopy);
	}

	ptr1 = Cinfo->OutputScreen[Cinfo->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '}');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy 
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '}';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '}');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(Cinfo->OutputScreen[Cinfo->CurrentLine], LineCopy);
	}


	Cinfo->Colourvalue[Cinfo->CurrentLine] = Cinfo->CurrentColour;
	Cinfo->LineLen[Cinfo->CurrentLine++] = Len;
	if (Cinfo->CurrentLine >= MAXLINES) Cinfo->CurrentLine = 0;
}



int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len);

int WritetoOutputWindow(struct ConsoleInfo * Cinfo, char * Msg, int len, BOOL Refresh)
{
	WritetoConsoleWindowSupport(Cinfo, Msg, len);

	if (Cinfo->Minimized)
		return 0;

	if (Refresh)
		DoRefresh(Cinfo);
	else
		Cinfo->NeedRefresh = TRUE;

	return 0;
}

int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len)
{
	char * ptr1, * ptr2;

	if (len + Cinfo->PartLinePtr > Cinfo->readbufflen)
	{
		Cinfo->readbufflen += len + Cinfo->PartLinePtr;
		Cinfo->readbuff = realloc(Cinfo->readbuff, Cinfo->readbufflen + 100);
	}

	if (Cinfo->PartLinePtr != 0)
	{
		Cinfo->CurrentLine--;				// Overwrite part line in buffer
		if (Cinfo->CurrentLine < 0)
			Cinfo->CurrentLine = MAXLINES - 1;
		

		if (Msg[0] == 0x1b && len > 1) 
		{
			Msg += 2;		// Remove Colour Escape
			len -= 2;
		}
	}

	memcpy(&Cinfo->readbuff[Cinfo->PartLinePtr], Msg, len);
		
	len=len+Cinfo->PartLinePtr;

	ptr1=&Cinfo->readbuff[0];
	Cinfo->readbuff[len]=0;

	if (Bells)
	{
		do {

			ptr2=memchr(ptr1,7,len);
			
			if (ptr2)
			{
				*(ptr2)=32;

				if (FlashOnBell)
					FlashWindow(Cinfo->hConsole, TRUE);
				else
					Beep(440,250);
			}
	
		} while (ptr2);
	}

lineloop:

	if (len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			Cinfo->PartLinePtr=len;
			memmove(Cinfo->readbuff,ptr1,len);
			AddLinetoWindow(Cinfo, ptr1);
//			InvalidateRect(Cinfo->hwndOutput, NULL, FALSE);

			return (0);
		}

		*(ptr2++)=0;
						
		// If len is greater that screen with, fold

		if (Cinfo == &MonWindow)
		{
			if (LogMonitor)
				WriteMonitorLine(ptr1, ptr2 - ptr1);
		}
		else
			if (LogOutput)
				WriteMonitorLine(ptr1, ptr2 - ptr1);
					
		if ((ptr2 - ptr1) > Cinfo->maxlinelen)
		{
			char * ptr3;
			char * saveptr1 = ptr1;
			int linelen = ptr2 - ptr1;
			int foldlen;
			char save;
					
		foldloop:

			ptr3 = ptr1 + Cinfo->maxlinelen;
					
			while(*ptr3!= 0x20 && ptr3 > ptr1)
			{
				ptr3--;
			}
						
			foldlen = ptr3 - ptr1 ;

			if (foldlen == 0)
			{
				// No space before, so split at width

				foldlen = Cinfo->maxlinelen;
				ptr3 = ptr1 + Cinfo->maxlinelen;

			}
			else
			{
				ptr3++ ; // Omit space
				linelen--;
			}
			save = ptr1[foldlen];
			ptr1[foldlen] = 0;
			AddLinetoWindow(Cinfo, ptr1);
			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > Cinfo->maxlinelen)
				goto foldloop;
						
			AddLinetoWindow(Cinfo, ptr1);

			ptr1 = saveptr1;
		}
		else
			AddLinetoWindow(Cinfo, ptr1);

		Cinfo->PartLinePtr=0;

		len-=(ptr2-ptr1);

		ptr1=ptr2;

		if ((len > 0) && Cinfo->StripLF)
		{
			if (*ptr1 == 0x0a)					// Line Feed
			{
				ptr1++;
				len--;
			}
		}

		goto lineloop;
	}


	return (0);
}



/*
VOID AddLinetoWindow(struct RTFTerm * OPData, char * Line)
{
	int Len = strlen(Line);
	char * ptr1 = Line;
	char * ptr2;
	int l, Index;
	char LineCopy[LINELEN * 2];
	
	if (Line[0] ==  0x1b && Len > 1)
	{
		// Save Colour Char
		
		OPData->CurrentColour = Line[1] - 10;
		ptr1 +=2;
		Len -= 2;
	}

	strcpy(OPData->OutputScreen[OPData->CurrentLine], ptr1);

	// Look for chars we need to escape (\  { })

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '\\');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ++ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy Including found char
			Index += l;
			LineCopy[Index++] = '\\';
			Len++;
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, '\\');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '{');

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '{';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '{');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '}');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy 
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '}';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '}');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}


	OPData->Colourvalue[OPData->CurrentLine] = OPData->CurrentColour;
	OPData->LineLen[OPData->CurrentLine++] = Len;
	if (OPData->CurrentLine >= MAXLINES) OPData->CurrentLine = 0;
}


VOID WritetoOutputWindow(struct RTFTerm * OPData, char * Msg, int len)
{
	char * ptr1, * ptr2;

	if (PartLinePtr > LINELEN)
		Msg[len++] = 13;					// Force a newline

	if (PartLinePtr != 0)
	{
		OPData->CurrentLine--;				// Overwrite part line in buffer
		
		if (Msg[0] == 0x1b && len > 1) 
		{
			Msg += 2;		// Remove Colour Escape
			len -= 2;
		}
	}
	
	memcpy(&readbuff[PartLinePtr], Msg, len);
		
	len += PartLinePtr;

	ptr1=&readbuff[0];
	readbuff[len]=0;

	if (Bells)
	{
		do {
			ptr2=memchr(ptr1,7,len);

			if (ptr2)
			{
				*(ptr2)=32;
				Beep(440,250);
			}
		} while (ptr2);
	}

lineloop:

	if (len > 0)
	{
		//	copy text to buffer a line at a time	
	
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			PartLinePtr = len;
			memmove(readbuff,ptr1,len);
			AddLinetoWindow(Cinfo, ptr1);
			return;
		}
		
		*(ptr2++)=0;

		if (LogOutput) WriteMonitorLine(ptr1, ptr2 - ptr1);
					
		// If len is greater that screen with, fold

		if ((ptr2 - ptr1) > maxlinelen)
		{
			char * ptr3;
			char * saveptr1 = ptr1;
			int linelen = ptr2 - ptr1;
			int foldlen;
			char save;
					
		foldloop:

			ptr3 = ptr1 + maxlinelen;
					
			while(*ptr3!= 0x20 && ptr3 > ptr1)
			{
				ptr3--;
			}
			foldlen = ptr3 - ptr1 ;

			if (foldlen == 0)
			{
				// No space before, so split at width

				foldlen = maxlinelen;
				ptr3 = ptr1 + maxlinelen;

			}
			else
			{
				ptr3++ ; // Omit space
				linelen--;
			}
			save = ptr1[foldlen];
			ptr1[foldlen] = 0;

			AddLinetoWindow(Cinfo, ptr1);

			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > maxlinelen)
				goto foldloop;
						
			AddLinetoWindow(Cinfo, ptr1);						
			ptr1 = saveptr1;
					
		}
		else
			AddLinetoWindow(Cinfo, ptr1);

			
		PartLinePtr=0;

		len-=(ptr2-ptr1);

		ptr1=ptr2;

		if ((len > 0) && StripLF)
		{
			if (*ptr1 == 0x0a)					// Line Feed
			{
				ptr1++;
				len--;
						}
		}
		goto lineloop;
	}
}


*/
DoMonData(int Stream)
{
	char * ptr1, * ptr2;
	int stamp;
	int len;
	unsigned char buffer[1024] = "\x1b\xb", monbuff[512];
	struct ConsoleInfo * Cinfo;

	// Monitor data uses the first stream

	Cinfo = &MonWindow;

	if (MONCount(Stream) > 0)
	{
		do {
		
			stamp=GetRaw(Stream, monbuff,&len,&count);

			if (MonitorColour)
			{
				if (monbuff[4] & 0x80)		// TX
					buffer[1] = 91;
				else
					buffer[1] = 17;
			}

			// See if a NODES

			if (!MonitorNODES && monbuff[21] == 3 && monbuff[22] == 0xcf && monbuff[23] == 0xff)
				len = 0;
			else
			{
				len=DecodeFrame(monbuff,&buffer[2],stamp);
				len +=2;
			}

			ptr1=&buffer[0];

		lineloop:

			if (len > 0)
			{
				//	copy text to control a line at a time	

				ptr2=memchr(ptr1,13,len);
				
				if (ptr2 == 0)
				{
					// no newline. Move data to start of buffer and Save pointer

					memmove(buffer,ptr1,len);

					return (0);

				}
				else
				{
					ptr2++;

					WritetoOutputWindow(Cinfo, ptr1, ptr2 - ptr1, FALSE);

					len-=(ptr2-ptr1);

					ptr1=ptr2;
					
					goto lineloop;
				}
			}
			
		} while (count > 0);

		Cinfo->NeedRefresh = TRUE;
	}
	return (0);
}





int DisableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(FrameWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_GRAYED);

	return (0);
}	
int DisableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(FrameWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_GRAYED);
	return (0);
}	

int	EnableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(FrameWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_ENABLED);
	return (0);
}	

int EnableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(FrameWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_ENABLED);

    return (0);
}

int ToggleRestoreWindows()
{	
	RestoreWindows = !RestoreWindows;
	
	if (RestoreWindows)
	{
		CheckMenuItem(hMonCfgMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
		CheckMenuItem(hTermCfgMenu,ID_WINDOWS_RESTORE,MF_CHECKED);
	}
	else
	{
		CheckMenuItem(hMonCfgMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
		CheckMenuItem(hTermCfgMenu, ID_WINDOWS_RESTORE, MF_UNCHECKED);
	}
    return (0);
  
}
/*
int ToggleAppl(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	APPLMASK = APPLMASK ^ mask;
	
	if (APPLMASK & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetAppl(Stream,applflags,APPLMASK);

    return (0);
  
}

int ToggleFlags(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	applflags ^= mask;
	
	if (applflags & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

    return (0);
  
}

*/

static CopyScreentoBuffer(char * buff)
{
	return (0);
}
	
int TogglePort(HWND hWnd, int Item, int mask)
{
	portmask ^= mask;
	
	if (portmask & mask)
		CheckMenuItem(hMonCfgMenu,Item,MF_CHECKED);
	else
		CheckMenuItem(hMonCfgMenu,Item,MF_UNCHECKED);

	if (portmask)
		applflags |= 0x80;
	else
		applflags &= 0x7f;

	SetAppl(Stream,applflags,APPLMASK);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMonCfgMenu,BPQMTX,MF_CHECKED);
	
	else

		CheckMenuItem(hMonCfgMenu,BPQMTX,MF_UNCHECKED);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	
	if (mcomparam & 1)

		CheckMenuItem(hMonCfgMenu,BPQMCOM,MF_CHECKED);
	
	else

		CheckMenuItem(hMonCfgMenu,BPQMCOM,MF_UNCHECKED);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);

    return (0);
  
}
int ToggleMON_UI_ONLY(HWND hWnd)
{
	monUI = monUI ^ 1;
	
	if (monUI & 1)

		CheckMenuItem(hMonCfgMenu,MON_UI_ONLY,MF_CHECKED);
	
	else

		CheckMenuItem(hMonCfgMenu,MON_UI_ONLY,MF_UNCHECKED);

	SetTraceOptionsEx(portmask,mtxparam,mcomparam, monUI);

    return (0);
  
}
int ToggleParam(HMENU hMenu, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu, Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
}

int ToggleChat(HWND hWnd)
{	
	APPLMASK = APPLMASK ^ 02;
	
	if (APPLMASK & 02)

		CheckMenuItem(hTermCfgMenu,BPQCHAT,MF_CHECKED);
	
	else

		CheckMenuItem(hTermCfgMenu,BPQCHAT,MF_UNCHECKED);

	SetAppl(Stream,applflags,APPLMASK);

    return (0);
  
}
/*
void MoveWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	if (hWnd == 0)
		return;

	GetWindowRect(hWnd, &rcMain);
	GetClientRect(hWnd, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	if (ClientWidth == 0)		// Minimized
		return;

	OutputBoxHeight = ClientHeight - SplitPos - InputBoxHeight - SplitBarHeight - SplitBarHeight;

	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, SplitPos+SplitBarHeight, ClientWidth-4, OutputBoxHeight, TRUE);
	MoveWindow(hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);
	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);

	GetClientRect(hwndOutput, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	if (ClientWidth > 16)
		maxlinelen = ClientWidth/OutputData.CharWidth - 1;

	InvalidateRect(hWnd, NULL, TRUE);
}

*/
static void MoveWindows(struct ConsoleInfo * Cinfo)
{
	RECT rcClient= {0,0,0,0};
	int ClientWidth;

	GetClientRect(Cinfo->hConsole, &rcClient); 

	if (rcClient.bottom == 0)		// Minimised
		return;

	Cinfo->ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	if (Cinfo->hwndInput)
	{
		MoveWindow(Cinfo->hwndOutput,2, 2, ClientWidth-4, Cinfo->ClientHeight-InputBoxHeight-4, TRUE);
		MoveWindow(Cinfo->hwndInput,2, Cinfo->ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);
	}
	else
		MoveWindow(Cinfo->hwndOutput,2, 2, ClientWidth-4, Cinfo->ClientHeight-4, TRUE);

	GetClientRect(Cinfo->hwndOutput, &rcClient); 

	Cinfo->ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	Cinfo->WarnLen = ClientWidth/8 - 1;
	Cinfo->WrapLen = Cinfo->WarnLen;
	Cinfo->maxlinelen = Cinfo->WarnLen;

}

void CopyRichTextToClipboard(HWND hWnd)
{
	int len=0;
	HGLOBAL	hMem;
	char * ptr;

	// Copy Rich Text to Clipboard
	
	len = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
	
	hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len + 1);

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);

		if (OpenClipboard(FrameWnd))
		{
			len = SendMessage(hWnd, WM_GETTEXT  , len, (LPARAM)ptr);

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
	}
	else
		GlobalFree(hMem);
}

BOOL OpenMonitorLogfile()
{
	UCHAR * BPQDirectory=GetBPQDirectory();
	UCHAR FN[MAX_PATH];

	if (BPQDirectory[0] == 0)
		sprintf(FN,"BPQTerm.log");
	else
		sprintf(FN,"%s\\BPQTerm.log", BPQDirectory);

	MonHandle = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	SetFilePointer(MonHandle, 0, 0, FILE_END);

	return (MonHandle != INVALID_HANDLE_VALUE);
}

void WriteMonitorLine(char * Msg, int MsgLen)
{
	int cnt;
	char CRLF[2] = {0x0d,0x0a};

	if (MonHandle == INVALID_HANDLE_VALUE) OpenMonitorLogfile();

	if (MonHandle == INVALID_HANDLE_VALUE) return;

	WriteFile(MonHandle ,Msg , MsgLen, &cnt, NULL);
	WriteFile(MonHandle ,CRLF , 2, &cnt, NULL);
}

INT_PTR CALLBACK FontConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)

{
	POINT Point;
	int Min, Max;
	int retCode, disp;
	HKEY hKey=0;


	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_FONTNAME, FontName);
		SetDlgItemInt(hDlg, IDC_CHARSET, CharSet, FALSE);
		SetDlgItemInt(hDlg, IDC_CODEPAGE, CodePage, FALSE);
		SetDlgItemInt(hDlg, IDC_FONTSIZE, FontSize, FALSE);
		SetDlgItemInt(hDlg, IDC_FONTWIDTH, FontWidth, FALSE);

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
		switch (LOWORD(wParam))
		{
		case IDOK:

			GetDlgItemText(hDlg, IDC_FONTNAME, FontName, 99);
			CharSet = GetDlgItemInt(hDlg, IDC_CHARSET, NULL, FALSE);
			CodePage = GetDlgItemInt(hDlg, IDC_CODEPAGE, NULL, FALSE);
			FontSize = GetDlgItemInt(hDlg, IDC_FONTSIZE, NULL, FALSE);
			FontWidth = GetDlgItemInt(hDlg, IDC_FONTWIDTH, NULL, FALSE);

//			SaveStringValue("FontName", FontName);
//			SaveIntValue("CharSet", CharSet);
//			SaveIntValue("CodePage", CodePage);
//			SaveIntValue("FontSize", FontSize);
//			SaveIntValue("FontWidth", FontWidth);

			
			// Save Config
	
			retCode = RegCreateKeyEx(REGTREE,
                              Key,
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey,"FontWidth",0,REG_DWORD,(BYTE *)&FontWidth,4);
				retCode = RegSetValueEx(hKey,"FontSize",0,REG_DWORD,(BYTE *)&FontSize,4);
				retCode = RegSetValueEx(hKey,"CodePage",0,REG_DWORD,(BYTE *)&CodePage,4);
				retCode = RegSetValueEx(hKey,"CharSet",0,REG_DWORD,(BYTE *)&CharSet,4);
				retCode = RegSetValueEx(hKey,"FontName",0,REG_SZ,(BYTE *)&FontName, strlen(FontName));

				RegCloseKey(hKey);
			}

			FontCinfo->CharWidth = FontWidth;
			FontCinfo->FirstTime = FALSE; 

			SetupRTFHddr();

			Point.x = 0;
			Point.y = 25000;					// Should be plenty for any font

			SendMessage(FontCinfo->hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
			FontCinfo->Scrolled = FALSE;

			GetScrollRange(FontCinfo->hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
			FontCinfo->RTFHeight = Max;

			DoRefresh(FontCinfo);
			EndDialog(hDlg, LOWORD(wParam));

		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return FALSE;

}


struct ConsoleInfo * CreateChildWindow(int Stream, BOOL DuringInit)
{
    WNDCLASS  wc;
	HWND ChildWnd = 0;
	struct ConsoleInfo * Cinfo;
	struct ConsoleInfo * last = NULL;
	int i = 1;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80]="";
	char Item[80];
	RECT Rect= {100, 100, 800, 600};

	char Title[80];

	if (Stream == 0)
		Stream = FindFreeStream();

	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return (FALSE);
	}

	// Find end of session chain

	for (Cinfo = ConsHeader; Cinfo; last = Cinfo, i++, Cinfo = Cinfo->next);

	Cinfo =  malloc(sizeof(struct ConsoleInfo));
	
	memset(Cinfo, 0, sizeof(struct ConsoleInfo));

	if (last)
		last->next = Cinfo;
	else
		ConsHeader = Cinfo;

	Cinfo->BPQStream = Stream;
	BPQSetHandle(Cinfo->BPQStream, FrameWnd);

//	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	sprintf(Title, "Stream %d", Cinfo->BPQStream);

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.lpfnWndProc   = (WNDPROC)ChildWndProc;
	wc.cbClsExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = "ChildWnd"; 

	if(!RegisterClass(&wc))
	{
		// return if RegisterClassEx fails
		DWORD dw_LastError = GetLastError();
		if(ERROR_CLASS_ALREADY_EXISTS != dw_LastError)
		{
			// return if the error is other than "error class already exists" 
			
			Debugprintf("Reg Class Failed %d", dw_LastError);
//			return NULL;
		}
	}

	ChildWnd =  CreateMDIWindow("ChildWnd", Title, 0,
		  0,0,0,0, ClientWnd, hInstance, 1234);

	Cinfo->hConsole = ChildWnd;

	// return if its not possible to create the child window

	if(NULL == ChildWnd)
	{
		DWORD dw_LastError = GetLastError();			
		Debugprintf("Create Child Failed %d", dw_LastError);
		return NULL;
	}

	Cinfo->SendHeader = TRUE;
	Cinfo->Finished = TRUE;
	Cinfo->CurrentColour = 1;

	Cinfo->WarnLen = Cinfo->WrapLen = Cinfo->maxlinelen = 100; // In case doesn't get set up


	Cinfo->hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
		WS_CHILD |  WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_VSCROLL | ES_READONLY,
		2,2,500,300, ChildWnd, NULL, hInstance, NULL);

	// Register for Mouse Events for Copy/Paste
	
	SendMessage(Cinfo->hwndOutput, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
	SendMessage(Cinfo->hwndOutput, EM_EXLIMITTEXT, 0, MAXLINES * LINELEN);

	Cinfo->hwndInput = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "",
			WS_CHILD |  WS_VISIBLE  | ES_AUTOHSCROLL | ES_NOHIDESEL,
			2,200,290,20, ChildWnd, NULL, hInstance, NULL);

	SendMessage(Cinfo->hwndInput, WM_SETFONT, (WPARAM)hFont, 0);

	// Set our own WndProcs for the controls. 

	Cinfo->wpOrigInputProc = (WNDPROC) SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, (LONG) InputProc); 

	sprintf(Item, "SessSize%d", i);

	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	Vallen=80;
	RegQueryValueEx(hKey, Item, 0 , &Type, &Size[0], &Vallen);

	sscanf(Size,"%d,%d,%d,%d,%d", &Rect.left, &Rect.right, &Rect.top, &Rect.bottom, &Cinfo->Minimized);

	if (Rect.top < - 500 || Rect.left < - 500)
	{
		Rect.left = 100;
		Rect.top = 100;
		Rect.right = 600;
		Rect.bottom = 400;
	}

	if (Rect.top < OffsetH)			// Make sure not off top of MDI frame
	{
		int Error = OffsetH - Rect.top;
		Rect.top += Error;
		Rect.bottom += Error;
	}

	MoveWindow(Cinfo->hConsole, Rect.left - (OffsetW /2), Rect.top - OffsetH,
		Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

 	MoveWindows(Cinfo);

	SetFocus(Cinfo->hwndInput);

	if (Cinfo->Minimized && DuringInit)
		ShowWindow(ChildWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(ChildWnd, SW_RESTORE);

	return Cinfo;
}


BOOL CreateMonitorWindow(char * MonSize)
{
	WNDCLASSEX wndclassMainFrame;
	HWND ChildWnd = 0;
	struct ConsoleInfo * Cinfo = &MonWindow;
	RECT Rect = {0,0,0,0};
	char Size[80];
	HKEY hKey;
	int retCode,Type,Vallen;

	memset(Cinfo, 0, sizeof(struct ConsoleInfo));

	Cinfo->WarnLen = Cinfo->WrapLen = Cinfo->maxlinelen = 100; // In case doesn't get set up
	Cinfo->StripLF = TRUE;

	wndclassMainFrame.cbSize		= sizeof(WNDCLASSEX);
	wndclassMainFrame.style			= CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;;
	wndclassMainFrame.lpfnWndProc	= MonWndProc;
	wndclassMainFrame.cbClsExtra	= 0;
	wndclassMainFrame.cbWndExtra	= 0;
	wndclassMainFrame.hInstance		= hInstance;
	wndclassMainFrame.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(BPQICON));
	wndclassMainFrame.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclassMainFrame.hbrBackground	= (HBRUSH) GetStockObject(GRAY_BRUSH);
	wndclassMainFrame.lpszMenuName	= NULL;
	wndclassMainFrame.lpszClassName	= "MonWnd";
	wndclassMainFrame.hIconSm		= NULL;
	
	if(!RegisterClassEx(&wndclassMainFrame))
	{
		// return if RegisterClassEx fails
		DWORD dw_LastError = GetLastError();
		if(ERROR_CLASS_ALREADY_EXISTS != dw_LastError)
		{
			// return if the error is other than "error class already exists" 
			return 0;
		}
	}

	// Create Window

	if(NULL != ChildWnd)
	{
		return 0;
	}

	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\BPQTermMDI");

	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	Vallen=80;
	RegQueryValueEx(hKey, "MonSize", 0 , &Type, &Size[0], &Vallen);

	sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left, &Rect.right, &Rect.top, &Rect.bottom, &Cinfo->Minimized);

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		Rect.right = 400;
		Rect.bottom = 400;
	}

	if (Rect.top < OffsetH)			// Make sure not off top of MDI frame
	{
		int Error = OffsetH - Rect.top;
		Rect.top += Error;
		Rect.bottom += Error;
	}

	ChildWnd =  CreateMDIWindow("MonWnd", "Monitor", 0,
		   Rect.left,	Rect.top,
		   Rect.right - Rect.left,
		   Rect.bottom - Rect.top,
		   ClientWnd, hInstance, 1234);

	Cinfo->hConsole = ChildWnd;

	// return if its not possible to create the child window
	if(NULL == ChildWnd)
	{
		return 0;
	}

	
	Cinfo->SendHeader = TRUE;
	Cinfo->Finished = TRUE;
	Cinfo->CurrentColour = 1;


	Cinfo->hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
		WS_CHILD |  WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_VSCROLL | ES_READONLY,
		2,2,500,300, ChildWnd, NULL, hInstance, NULL);

	// Register for Mouse Events for Copy/Paste
	
	SendMessage(Cinfo->hwndOutput, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
	SendMessage(Cinfo->hwndOutput, EM_EXLIMITTEXT, 0, MAXLINES * LINELEN);

	Cinfo = &MonWindow;
	
//	MoveWindow(Cinfo->hConsole, Rect.left - (OffsetW /2), Rect.top - OffsetH,
//		Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

 	MoveWindows(Cinfo);

	if (Cinfo->Minimized)
		ShowWindow(ChildWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(ChildWnd, SW_RESTORE);

	return TRUE;
}
