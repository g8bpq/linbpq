
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

// Version 2.2.1 June 2012

//		Add UIOnly Monitor Option

//	Version 2.2.2.1 Nov 2014
//		Fix possible crash on processing part line


//	Version 2.2.3.1	Dec 2014
//		Remove "Enable Chat" option

//	Version 2.2.4.1	Dec 2015
//		Add Port Names to Monitor Config Menu


//	Add Chat Terminal Mode

//	Version 2.2.5.1	Sep 2019

//	Recompile for change to Port Struct

//	Version 2.2.5.2	May 2023
//	Changes for compatibility with 63 port BPQ32

#define _USE_32BIT_TIME_T
#define _CRT_SECURE_NO_DEPRECATE

#define pthread_t DWORD
#include "asmstrucs.h"

#include <windows.h>
#include <winstdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <htmlhelp.h>
#include <Richedit.h> 

#include "bpqterminal.h"

#define BPQBASE WM_USER
//
//	Port monitoring flags use BPQBASE -> BPQBASE+100

#include "bpq32.h"	// BPQ32 API Defines
#define BPQTerm
#include "Versions.h"
#include "GetVersion.h"

#define BGCOLOUR RGB(236,233,216)

HBRUSH bgBrush;

UINT BPQMsg;

HKEY FAR WINAPI GetRegistryKey();
char * FAR WINAPI GetRegistryKeyText();

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default

HINSTANCE hInst; 
char AppName[] = "BPQTerm 32";
char ClassName[] = "BPQMAINWINDOW";
char Title[80];

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
int NewLine();

int	ProcessBuff(char * readbuff,int len);

int EnableConnectMenu(HWND hWnd);
int EnableDisconnectMenu(HWND hWnd);
int	DisableConnectMenu(HWND hWnd);
int DisableDisconnectMenu(HWND hWnd);
int	ToggleAutoConnect(HWND hWnd);
int DoReceivedData(HWND hWnd);
int	DoStateChange(HWND hWnd);
int CopyScreentoBuffer(char * buff);
int DoMonData(HWND hWnd);
int TogglePort(HWND hWnd, int Item, uint64_t mask);
int ToggleMTX(HWND hWnd);
int ToggleMCOM(HWND hWnd);
int ToggleMUI(HWND hWnd);
int ToggleParam(HWND hWnd, BOOL * Param, int Item);
int ToggleChat(HWND hWnd);
void MoveWindows();
void CopyListToClipboard(HWND hWnd);
void CopyRichTextToClipboard(HWND hWnd);
BOOL OpenMonitorLogfile();
void WriteMonitorLine(char * Msg, int MsgLen);
VOID WritetoOutputWindow(struct RTFTerm * OPData, char * Msg, int len);
VOID DoRefresh(struct RTFTerm * OPData);
INT_PTR CALLBACK FontConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,2552,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,2552,255)
};



LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;


WNDPROC wpOrigInputProc; 
WNDPROC wpOrigMonProc; 
WNDPROC wpOrigSplitProc; 

HWND hwndInput;
HWND hwndOutput;
HWND hwndMon;
HWND hwndSplit;

HMENU trayMenu;

int xsize,ysize;		// Screen size at startup

double Split=0.5;
int SplitPos=300;
#define SplitBarHeight 5
#define InputBoxHeight 25
RECT Rect;
RECT MonRect;
RECT OutputRect;
RECT SplitRect;

int OutputBoxHeight;

int Height, Width, LastY;

int maxlinelen = 80;
int CharWidth = 8;

int ClientHeight, ClientWidth;


#define MAXLINES 1000
#define LINELEN 200

char RTFHeader[4000];

int RTFHddrLen;

typedef struct RTFTerm
{
	int CurrentLine;				// Line we are writing to in circular buffer.

	int Index;
	BOOL SendHeader;
	BOOL Finished;

	char OutputScreen[MAXLINES][LINELEN];

	int Colourvalue[MAXLINES];
	int LineLen[MAXLINES];
	int CharWidth;
	int CurrentColour;
	int Thumb;
	int FirstTime;
	int RTFHeight;				// Height of RTF control in pixels
	BOOL Scrolled;				// Window is scrolled back

};

char FontName[100] = "FixedSys";
int FontSize = 20;
int FontWidth = 8;
int CodePage = 437;
int CharSet = 0;


struct RTFTerm OutputData;

int CurrentHost = 0;
int CfgNo = 0;

SOCKET sock;

BOOL MonData = FALSE;

char Key[80];
uint64_t portmask=0;
int mtxparam=1;
int mcomparam=1;
int monUI = 0;

char kbbuf[160];
int kbptr=0;
char readbuff[100000];				// for stupid bbs programs

int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
int applmask = 0;
int applflags = 2;				// Message to Application
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

HANDLE 	MonHandle=INVALID_HANDLE_VALUE;

HCURSOR DragCursor;
HCURSOR	Cursor;

CONNECTED=FALSE;
AUTOCONNECT=TRUE;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
//BOOL StartMinimized = FALSE;

HWND MainWnd, hWnd;

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;

int TimerHandle = 0;

int SlowTimer;

BOOL WINE;

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


VOID CALLBACK TimerProc(

    HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime) 	// current system time	
{
	// entered every 10 secs

	CheckTimer();

	if (!CONNECTED)
		return;

	if (!ChatMode)
		return;

	SlowTimer++;

	if (SlowTimer > 50)				// About 9 mins
	{
		SlowTimer = 0;
		SendMsg(Stream, "\0", 1);
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
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

/*	PCHAR p1, p2, p3;

	p1 = strtok(lpCmdLine, " ,\t\n\r");
	p2 = strtok(NULL, " ,\t\n\r");
	p3 = strtok(NULL, " ,\t\n\r");

	if (nCmdShow == SW_SHOWMINIMIZED)
		StartMinimized = TRUE;

	if (p1)
	{
		if (strlen(p1) > 5)
		{
			if (_stricmp(p1, "StartMinimized") == 0)
				StartMinimized = TRUE;

			if (p2) Sessno = atoi(p2);
			if (p3) sscanf(p3,"%x", &applflags);
		}
		else 
		{		
			if (p2)
			{
				if (strlen(p2) > 5)
				{
					if (_stricmp(p2, "StartMinimized") == 0)
						StartMinimized = TRUE;
		
					if (p1) Sessno = atoi(p1);
					if (p3) sscanf(p3,"%x", &applflags);
				}
			}
			else
			{
				if (p3)
				{
					if (strlen(p3) > 5)
					{
						if (_stricmp(p3, "StartMinimized") == 0)
							StartMinimized = TRUE;

						if (p1) Sessno = atoi(p1);
						if (p2) sscanf(p2,"%x", &applflags);
					}
				}
			}
		}
	}
*/
	sscanf(lpCmdLine,"%d %x",&Sessno, &applflags);

	if (!InitApplication(hInstance)) 
			return (FALSE);

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

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
		retCode = RegSetValueEx(hKey,"AutoConnect",0,REG_DWORD,(BYTE *)&AUTOCONNECT,4);
		retCode = RegSetValueEx(hKey,"PortMask",0,REG_BINARY,(BYTE *)&portmask, 8);
		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
		retCode = RegSetValueEx(hKey,"SendDisconnected",0,REG_DWORD,(BYTE *)&SendDisconnected,4);
		retCode = RegSetValueEx(hKey,"MTX",0,REG_DWORD,(BYTE *)&mtxparam,4);
		retCode = RegSetValueEx(hKey,"MCOM",0,REG_DWORD,(BYTE *)&mcomparam,4);
		retCode = RegSetValueEx(hKey,"MUIONLY",0,REG_DWORD,(BYTE *)&monUI,4);
		retCode = RegSetValueEx(hKey,"Split",0,REG_BINARY,(BYTE *)&Split,sizeof(Split));
		retCode = RegSetValueEx(hKey,"MONColour",0,REG_DWORD,(BYTE *)&MonitorColour,4);
		retCode = RegSetValueEx(hKey,"MonitorNODES",0,REG_DWORD,(BYTE *)&MonitorNODES,4);

		wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		RegCloseKey(hKey);
	}

	KillTimer(NULL, TimerHandle);

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
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( BPQICON ) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	return (RegisterClass(&wc));

}

HMENU hMenu, hPopMenu1, hPopMenu2, hPopMenu3;		// handle of menu 

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
		n += wsprintf(&RTFColours[n], "\\red%d\\green%d\\blue%d;", GetRValue(Colour), GetGValue(Colour),GetBValue(Colour));
	}

	RTFColours[n++] = '}';
	RTFColours[n] = 0;

//	strcpy(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fmodern\\fprq1\\fcharset204 ;}}");
	wsprintf(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fprq1\\cpg%d\\fcharset%d %s;}}", CodePage, CharSet, FontName);
	strcat(RTFHeader, RTFColours);
	wsprintf(Temp, "\\viewkind4\\uc1\\pard\\f0\\fs%d", FontSize);
	strcat(RTFHeader, Temp);

	RTFHddrLen = strlen(RTFHeader);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i, n;
	uint64_t tempmask=0x0;
	char msg[50];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	TEXTMETRIC tm; 
	HDC dc;
	struct RTFTerm * OPData;
	struct PORTCONTROL * PORT;

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
	wpOrigSplitProc = (WNDPROC)SetWindowLong(hwndSplit, GWL_WNDPROC, (LONG)SplitProc);

	// Get config from Registry 

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQTerminal\\Session%d",Sessno);

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MUIONLY",0,			
			(ULONG *)&Type,(UCHAR *)&monUI,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);
		
		Vallen=8;
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

	wsprintf(Title,"BPQTerminal Version %s - using stream %d", VersionString, Stream);

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

	hMenu=GetMenu(hWnd);

//	if (applmask & 0x01)	CheckMenuItem(hMenu,BPQAPPL1,MF_CHECKED);
//	if (applmask & 0x02)	CheckMenuItem(hMenu,BPQCHAT,MF_CHECKED);
//	if (applmask & 0x04)	CheckMenuItem(hMenu,BPQAPPL3,MF_CHECKED);
//	if (applmask & 0x08)	CheckMenuItem(hMenu,BPQAPPL4,MF_CHECKED);
//	if (applmask & 0x10)	CheckMenuItem(hMenu,BPQAPPL5,MF_CHECKED);
//	if (applmask & 0x20)	CheckMenuItem(hMenu,BPQAPPL6,MF_CHECKED);
//	if (applmask & 0x40)	CheckMenuItem(hMenu,BPQAPPL7,MF_CHECKED);
//	if (applmask & 0x80)	CheckMenuItem(hMenu,BPQAPPL8,MF_CHECKED);

// CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
// MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
// MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
//		0x40 = Send Keepalives

//	if (applflags & 0x01)	CheckMenuItem(hMenu,BPQFLAGS1,MF_CHECKED);
//	if (applflags & 0x02)	CheckMenuItem(hMenu,BPQFLAGS2,MF_CHECKED);
//	if (applflags & 0x04)	CheckMenuItem(hMenu,BPQFLAGS3,MF_CHECKED);
//	if (applflags & 0x40)	CheckMenuItem(hMenu,BPQFLAGS4,MF_CHECKED);


	hPopMenu1=GetSubMenu(hMenu,1);

	for (n=1;n <= GetNumberofPorts();n++)
	{
		PORT = GetPortTableEntryFromSlot(n);
		
		i = PORT->PORTNUMBER;

		sprintf(msg,"Port %d %s ",i, PORT->PORTDESCRIPTION);

		if (tempmask & ((uint64_t)1<<(i-1)))
		{
			AppendMenu(hPopMenu1,MF_STRING | MF_CHECKED,BPQBASE + i,msg);
			portmask |= ((uint64_t)1 <<( i-1));
		}
		else
			AppendMenu(hPopMenu1,MF_STRING | MF_UNCHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	CheckMenuItem(hMenu, BPQMCOM, (mcomparam) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, MUIONLY, (monUI) ? MF_CHECKED : MF_UNCHECKED);
	
	if (AUTOCONNECT)
		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_UNCHECKED);

  	if (Bells & 1)
		CheckMenuItem(hMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQBELLS, MF_UNCHECKED);

  	if (SendDisconnected & 1)
		CheckMenuItem(hMenu,BPQSendDisconnected, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQSendDisconnected, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hMenu,BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hMenu,MONCOLOUR, (MonitorColour) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	if (portmask) applflags |= 0x80;

	SetAppl(Stream, applflags, applmask);

	SetTraceOptions64(portmask, mtxparam, mcomparam, monUI);
	
	DragCursor = LoadCursor(hInstance, "IDC_DragSize");
	Cursor = LoadCursor(NULL, IDC_ARROW);

	GetCallsign(Stream, callsign);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
	
		wsprintf(Title,"BPQTerminal Stream %d",Stream);
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

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	UCHAR Buffer[1000];
	UCHAR * buf = Buffer;
    TEXTMETRIC tm; 
    int i, y;  
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 


	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(hWnd);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(hWnd);
				
		if (lParam & BPQStateChange)
			DoStateChange(hWnd);

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

	case WM_NOTIFY:
	{
		const MSGFILTER * pF = (MSGFILTER *)lParam;
		POINT pos;
		CHARRANGE Range;

		if(pF->nmhdr.hwndFrom == hwndOutput)
		{
			if(pF->msg == WM_VSCROLL)
			{
//				OutputData.Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
				DoRefresh(&OutputData);
//				return TRUE;		
			}

			if(pF->msg == WM_KEYUP)
			{
				if (pF->wParam == VK_PRIOR || pF->wParam == VK_NEXT)
				{
//					OutputData.Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
					DoRefresh(&OutputData);
				}
			}

			if(pF->msg == WM_RBUTTONDOWN)
			{
				// Only allow popup if something is selected

				SendMessage(hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
				if (Range.cpMin == Range.cpMax)
					return TRUE;

				GetCursorPos(&pos);
				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return TRUE;
			}
		}
		break;
	}



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
		
		if (wmId > BPQBASE && wmId < BPQBASE + 64)
		{
			TogglePort(hWnd,wmId,(uint64_t)1 << (wmId - (BPQBASE + 1)));
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

			DialogBox(hInst, MAKEINTRESOURCE(IDD_FONT), hWnd, FontConfigWndProc);
			break;

        
		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;

		case MUIONLY:

			ToggleMUI(hWnd);
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

		case BPQBELLS:

			ToggleParam(hWnd, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hWnd, &StripLF, BPQStripLF);
			break;

		case BPQLogOutput:

			ToggleParam(hWnd, &LogOutput, BPQLogOutput);
			break;

		case CHATTERM:

			ToggleParam(hWnd, &ChatMode, CHATTERM);
			break;

			
		case BPQSendDisconnected:

			ToggleParam(hWnd, &SendDisconnected, BPQSendDisconnected);
			break;

		case BPQMNODES:

			ToggleParam(hWnd, &MonitorNODES, BPQMNODES);
			break;

		case MONCOLOUR:

			ToggleParam(hWnd, &MonitorColour, MONCOLOUR);
			break;

		case BPQLogMonitor:

			ToggleParam(hWnd, &LogMonitor, BPQLogMonitor);
			break;

		case BPQCLEARMON:

			SendMessage(hwndMon,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCLEAROUT:

			for (i = 0; i < MAXLINES; i++)
			{
				OutputData.OutputScreen[i][0] = 0;
			}

			OutputData.CurrentLine = 0;
			DoRefresh(&OutputData);

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

static char * KbdStack[20];

int StackIndex=0;
 
LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	char DisplayLine[200] = "\x1b\xb";

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
			
			kbptr=SendMessage(hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);

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
 
    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

//int FirstItem=-1, LastItem=-1, SELECTING=FALSE, MOVED=FALSE, LastSelected=0;

LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 
 
    return CallWindowProc(wpOrigMonProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int xPos, yPos;
	
	switch (uMsg) { 

		case WM_LBUTTONDOWN:

			SetCapture(hwnd);
			return 0;

		case WM_LBUTTONUP:

			xPos = LOWORD(lParam);  // horizontal position of cursor 
			yPos = lParam >> 16;  // vertical position of cursor 

			SplitPos = SplitPos + yPos - 5;

			if (SplitPos < 0) SplitPos = 0;

			Split=SplitPos;
			Split=Split/Height;

			ReleaseCapture();
			MoveWindows();
			DoRefresh(&OutputData);
			return 0;

		case WM_MOUSEMOVE:

 			// Used to size split between Monitor and Output Windows

			SetCursor(DragCursor);
			break;
	}
    return CallWindowProc(wpOrigSplitProc, hwnd, uMsg, wParam, lParam); 
} 




DoStateChange(HWND hWnd)
{
	int port, sesstype, paclen, maxframe, l4window;

	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.
	
	SessionState(Stream, &state, &change);
		
	if (change == 1)
	{
		if (state == 1)
		{
			// Connected
			
			CONNECTED = TRUE;
			SlowTimer = 0;

	//		GetCallsign(Stream, callsign);
			
			GetConnectionInfo(Stream, callsign,
										 &port, &sesstype, &paclen,
										 &maxframe, &l4window);

				
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Connected to %s",VersionString,Stream,callsign);
			SetWindowText(hWnd,Title);
			DisableConnectMenu(hWnd);
			EnableDisconnectMenu(hWnd);

		}
		else
		{	
			CONNECTED=FALSE;
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Disconnected",VersionString,Stream);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);

			if (SendDisconnected)
			{
				WritetoOutputWindow(&OutputData, "*** Disconnected\r", 17);		
				DoRefresh(&OutputData);
			}
		}
	}

	return (0);

}
		
DoReceivedData(HWND hWnd)
{
	char Msg[300];

	if (RXCount(Stream) > 0)
	{
		do {

			GetMsg(Stream, &Msg[0],&len,&count);
			WritetoOutputWindow(&OutputData, Msg, len);
			DoRefresh(&OutputData);

			SlowTimer = 0;

		} while (count > 0);
	}
	return (0);
}
DWORD CALLBACK EditStreamCallback(DWORD_PTR Cookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	struct RTFTerm * OPData	= (struct RTFTerm *)Cookie;
	int ReqLen = cb;
	int i;
	int Line;

//	if (cb != 4092)
//		return 0;

	if (OPData->SendHeader)
	{
		// Return header

		memcpy(lpBuff, RTFHeader, RTFHddrLen);
		*pcb = RTFHddrLen;
		OPData->SendHeader = FALSE;
		OPData->Finished = FALSE;
		OPData->Index = 0;
		return 0;
	}

	if (OPData->Finished)
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
	Line = OPData->Index++ + OPData->CurrentLine - MAXLINES;

	if (Line <0)
		Line = Line + MAXLINES;

	wsprintf(lpBuff, "\\cf%d ", OPData->Colourvalue[Line]);
	strcat(lpBuff, OPData->OutputScreen[Line]);
	strcat(lpBuff, "\\line");

	if (OPData->Index == MAXLINES)
	{
		OPData->Finished = TRUE;
		strcat(lpBuff, "}");
		i = 10;
	}
	}
	*pcb = strlen(lpBuff);
	return 0;
}

VOID ForceRefresh(struct RTFTerm * OPData)
{
	OPData->Thumb = 25000;
	DoRefresh(OPData);
	OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
}

VOID DoRefresh(struct RTFTerm * OPData)
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
		if (OPData->CurrentLine < 0)
			OPData->CurrentLine = MAXLINES - 1;
		
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
			AddLinetoWindow(OPData, ptr1);
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

			AddLinetoWindow(OPData, ptr1);

			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > maxlinelen)
				goto foldloop;
						
			AddLinetoWindow(OPData, ptr1);						
			ptr1 = saveptr1;
					
		}
		else
			AddLinetoWindow(OPData, ptr1);

			
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


DoMonData(HWND hWnd)
{
	char * ptr1, * ptr2;
	int index;
	time_t stamp;
	int len;
	unsigned char buffer[1024] = "\x1b\xb", monbuff[512];

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
					*(ptr2++)=0;

					index=SendMessage(hwndMon,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );

					if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

					if (index > 1200)

					do{

						index=SendMessage(hwndMon,LB_DELETESTRING, 0, 0);
					
						} while (index > 1000);

					SendMessage(hwndMon,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

					len-=(ptr2-ptr1);

					ptr1=ptr2;
					
					goto lineloop;
				}
			}
			
		} while (count > 0);
	}
	return (0);
}





int DisableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_GRAYED);

	return (0);
}	
int DisableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_GRAYED);
	return (0);
}	

int	EnableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_ENABLED);
	return (0);
}	

int EnableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_ENABLED);

    return (0);
}

int ToggleAutoConnect(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	AUTOCONNECT = !AUTOCONNECT;
	
	if (AUTOCONNECT)

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_UNCHECKED);

    return (0);
  
}

CopyScreentoBuffer(char * buff)
{

	return (0);
}
	
int TogglePort(HWND hWnd, int Item, uint64_t mask)
{
	portmask ^= mask;
	
	if (portmask & mask)
		CheckMenuItem(hMenu,Item,MF_CHECKED);
	else
		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	if (portmask)
		applflags |= 0x80;
	else
		applflags &= 0x7f;

	SetAppl(Stream,applflags,applmask);

	SetTraceOptions64(portmask, mtxparam, mcomparam, monUI);

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	SetTraceOptions64(portmask, mtxparam, mcomparam, monUI);

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	
	CheckMenuItem(hMenu, BPQMCOM, (mcomparam) ? MF_CHECKED : MF_UNCHECKED);
	SetTraceOptions64(portmask, mtxparam, mcomparam, monUI);

    return (0);
  
}
int ToggleMUI(HWND hWnd)
{
	monUI = monUI ^ 1;
	
	CheckMenuItem(hMenu, MUIONLY, (monUI) ? MF_CHECKED : MF_UNCHECKED);
	SetTraceOptions64(portmask, mtxparam, mcomparam, monUI);

    return (0);
  
}
int ToggleParam(HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
}

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

void CopyRichTextToClipboard(HWND hWnd)
{
	int len=0;
	HGLOBAL	hMem;
	char * ptr;

	// Copy Rich Text to Clipboard
	
	len = SendMessage(hwndOutput, WM_GETTEXTLENGTH, 0, 0);
	
	hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len + 1);

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);

		if (OpenClipboard(MainWnd))
		{
			len = SendMessage(hwndOutput, WM_GETTEXT  , len, (LPARAM)ptr);

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
	}
	else
		GlobalFree(hMem);
}


void CopyListToClipboard(HWND hWnd)
{
	int i,n, len=0;
	HGLOBAL	hMem;
	char * ptr;
	//
	//	Copy List Box to clipboard
	//
	
	n = SendMessage(hWnd, LB_GETCOUNT, 0, 0);		
	
	for (i=0; i<n; i++)
	{
		len+=SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
	}

	hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len+n+n+1);
	

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);
	
		if (OpenClipboard(MainWnd))
		{
			//			CopyScreentoBuffer(GlobalLock(hMem));
			
			for (i=0; i<n; i++)
			{
				ptr+=SendMessage(hWnd, LB_GETTEXT, i, (LPARAM) ptr);
				*(ptr++)=13;
				*(ptr++)=10;
			}

			*(ptr)=0;					// end of data

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
			else
				GlobalFree(hMem);		
	}
}

BOOL OpenMonitorLogfile()
{
	UCHAR * BPQDirectory=GetBPQDirectory();
	UCHAR FN[MAX_PATH];

	if (BPQDirectory[0] == 0)
		wsprintf(FN,"BPQTerm_%d.log", Stream);
	else
		wsprintf(FN,"%s\\BPQTerm_%d.log", BPQDirectory, Stream);

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

			OutputData.CharWidth = FontWidth;
			OutputData.FirstTime = FALSE; 

			SetupRTFHddr();

			Point.x = 0;
			Point.y = 25000;					// Should be plenty for any font

			SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
			OutputData.Scrolled = FALSE;

			GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
			OutputData.RTFHeight = Max;

			DoRefresh(&OutputData);
			EndDialog(hDlg, LOWORD(wParam));

		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return FALSE;

}


