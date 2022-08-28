
#define MAXSTACK 20
#define INPUTLEN 512

#define MAXLINES 1000
#define LINELEN 200


#define BPQICON                         2
#define IDR_MENU1                       101
#define BPQMENU                         101
#define BPQCONNECT                      102
#define BPQDISCONNECT                   103
#define IDD_FONT                        105

#define ID_WARNWRAP		415
#define ID_WRAP			416
#define ID_FLASHONBELL	417

#define IDC_FONTWIDTH                   1008
#define IDC_FONTNAME                    1009
#define IDC_CODEPAGE                    1010
#define IDC_CHARSET                     1011
#define IDC_FONTSIZE                    1012
#define BPQMTX                          1164
#define BPQMCOM                         1165
#define BPQCOPYMON                      1166
#define BPQCOPYOUT                      1167
#define BPQCLEARMON                     1168
#define BPQCLEAROUT                     1169
#define BPQBELLS                        1170
#define BPQCHAT                         1171
#define BPQHELP                         1172
#define BPQStripLF                      1173
#define BPQLogOutput                    1174
#define BPQLogMonitor                   1175
#define BPQSendDisconnected             1176
#define BPQMNODES                       1177
#define MONCOLOUR                       1178
#define CHATTERM                        1179
#define IDM_CLOSEWINDOW					1180
#define MONITORAPRS						1181
#define MON_UI_ONLY                     40006
#define StopALLMon                      40007

#define IDR_MAINFRAME_MENU              191
#define TERM_MENU						192
#define MON_MENU						193
#define IDI_SIGMA_MAIN_ICON             104
#define IDI_SYSTEM_INFO                 106

#define RTFCOPY							30000
#define ID_INFORMATION_SYSTEMINFORMATION 30001
#define ID_HELP_ABOUT                   30002
#define ID_WINDOWS_CASCADE              30003
#define ID_FILE_EXIT                    30004
#define ID_WINDOWS_TILE                 30005
#define ID_NEWWINDOW                    30006
#define ID_WINDOWS_RESTORE				30007
#define ID_SETUP_FONT                   30008
#define ID_ACTION_RESETWINDOWSPLIT      30009

#define BPQBASE 40100

#define IDM_FIRSTCHILD   50000 // used in structure when creating mdi client area for the main frame 

//	Port monitoring flags use BPQBASE -> BPQBASE+100
struct ConsoleInfo 
{
	struct ConsoleInfo * next;
	int BPQStream;
	BOOL Active;
	int Incoming;
	WNDPROC wpOrigInputProc; 
	HWND hConsole;
	HWND hwndInput;
	HWND hwndOutput;
	HMENU hMenu;		// handle of menu 
	RECT ConsoleRect;
	RECT OutputRect;
	int CharWidth;

	int Height, Width, Top, Left;

	int ClientHeight, ClientWidth;
	char kbbuf[INPUTLEN];
	int kbptr;

	int readbufflen;		// Current Length
	char * readbuff;		// Malloc'ed
	char * KbdStack[MAXSTACK];

	int StackIndex;

//	BOOL Bells;
//	BOOL FlashOnBell;		// Flash instead of Beep
	BOOL StripLF;

//	BOOL WarnWrap;
//	BOOL FlashOnConnect;
//	BOOL WrapInput;
//	BOOL CloseWindowOnBye;

	unsigned int WrapLen;
	int WarnLen;
	int maxlinelen;

	int PartLinePtr;
	int PartLineIndex;		// Listbox index of (last) incomplete line

	DWORD dwCharX;      // average width of characters 
	DWORD dwCharY;      // height of characters 
	DWORD dwClientX;    // width of client area 
	DWORD dwClientY;    // height of client area 
	DWORD dwLineLen;    // line length 
	int nCaretPosX; // horizontal position of caret 
	int nCaretPosY; // vertical position of caret 

	COLORREF FGColour;		// Text Colour
	COLORREF BGColour;		// Background Colour
	COLORREF DefaultColour;	// Default Text Colour

	int CurrentLine;				// Line we are writing to in circular buffer.

	int Index;
	BOOL SendHeader;
	BOOL Finished;

	char OutputScreen[MAXLINES][LINELEN];

	int Colourvalue[MAXLINES];
	int LineLen[MAXLINES];

	int CurrentColour;
	int Thumb;
	int FirstTime;
	BOOL Scrolled;				// Set if scrolled back
	int RTFHeight;				// Height of RTF control in pixels 

	BOOL CONNECTED;
	int SlowTimer;
	BOOL Minimized;
	BOOL NeedRefresh;

};

