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


#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE

#include <time.h>

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02


#include "kernelresource.h"
#include "CHeaders.h"
#include "tncinfo.h"
#ifndef LINBPQ
#include <commctrl.h>
#endif
//#include <stdlib.h>
#include "bpq32.h"
#include "adif.h"


HANDLE hInstance;
extern HBRUSH bgBrush;
extern HWND ClientWnd, FrameWnd;
extern int OffsetH, OffsetW;

extern HMENU hMainFrameMenu;
extern HMENU hBaseMenu;
extern HANDLE hInstance;

extern HKEY REGTREE;

extern int Ver[];


int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);

char * GetChallengeResponse(char * Call, char *  ChallengeString);

VOID __cdecl Debugprintf(const char * format, ...);
int FromLOC(char * Locator, double * pLat, double * pLon);
BOOL ToLOC(double Lat, double Lon , char * Locator);

int GetPosnFromAPRS(char * Call, double * Lat, double * Lon);
char * stristr (char *ch1, char *ch2);


static RECT Rect;

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

int Winmor_Socket_Data(int sock, int error, int eventcode);

struct WL2KInfo * WL2KReports;

int WL2KTimer = 0;

int ModetoBaud[31] = {0,0,0,0,0,0,0,0,0,0,0,			// 0 = 10
					  200,600,3200,600,3200,3200,		// 11 - 16
					  0,0,0,0,0,0,0,0,0,0,0,0,0,600};	// 17 - 30

extern char HFCTEXT[];
extern int HFCTEXTLEN;


extern char WL2KCall[10];
extern char WL2KLoc[7];


VOID MoveWindows(struct TNCINFO * TNC)
{
#ifndef LINBPQ
	RECT rcClient;
	int ClientHeight, ClientWidth;

	GetClientRect(TNC->hDlg, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	if (TNC->hMonitor)
		MoveWindow(TNC->hMonitor,2 , TNC->RigControlRow + 3, ClientWidth-4, ClientHeight - (TNC->RigControlRow + 3), TRUE);
#endif
}

char * Config;
static char * ptr1, * ptr2;

#ifndef LINBPQ

LRESULT CALLBACK PacWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	MINMAXINFO * mmi;

	int i;
	struct TNCINFO * TNC;

	HKEY hKey;
	char Key[80];
	int retCode, disp;

	for (i=0; i<41; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;
		
		if (TNC->hDlg == hWnd)
			break;
	}

	if (TNC == NULL)
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	switch (message) { 

	case WM_CREATE:

		break;

	case WM_PAINT:

//			hdc = BeginPaint (hWnd, &ps);
			
//			SelectObject( hdc, hFont) ;
			
//			EndPaint (hWnd, &ps);
//
//			wParam = hdc;
	
			break;        


	case WM_GETMINMAXINFO:

 		if (TNC->ClientHeight)
		{
			mmi = (MINMAXINFO *)lParam;
			mmi->ptMaxSize.x = TNC->ClientWidth;
			mmi->ptMaxSize.y = TNC->ClientHeight;
			mmi->ptMaxTrackSize.x = TNC->ClientWidth;
			mmi->ptMaxTrackSize.y = TNC->ClientHeight;
		}

		break;


	case WM_MDIACTIVATE:
	{
			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);

			if (TNC->hMenu)
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)TNC->hMenu, "Actions");
			
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);

//			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) TNC->hMenu, (LPARAM) TNC->hWndMenu);
		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		 }
			 
		// call DrawMenuBar after the menu items are set
		DrawMenuBar(FrameWnd);

		return DefMDIChildProc(hWnd, message, wParam, lParam);
	}



	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)TNC->hMenu)
		{
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, " TNC") || strstr(TNC->ProgramPath, "ARDOP")
					 || strstr(TNC->ProgramPath, "VARA") || stristr(TNC->ProgramPath, "FREEDATA"))
				{
					EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_ENABLED);
		
					break;
				}
			}
			EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_GRAYED);
		}
			
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case WINMOR_KILL:

			TNC->DontRestart = TRUE;
			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			TNC->DontRestart = FALSE;
			KillTNC(TNC);
			RestartTNC(TNC);
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			TNC->RestartAfterFailure = !TNC->RestartAfterFailure;
			CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
			retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				RegSetValueEx(hKey,"TNC->RestartAfterFailure",0,REG_DWORD,(BYTE *)&TNC->RestartAfterFailure, 4);
				RegCloseKey(hKey);
			}
			break;

		case ARDOP_ABORT:

			if (TNC->ForcedCloseProc)
				TNC->ForcedCloseProc(TNC, 0);

			break;
		}
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SIZING:
	case WM_SIZE:

		MoveWindows(TNC);
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{ 

		case SC_RESTORE:

			TNC->Minimized = FALSE;
			break;

		case SC_MINIMIZE: 

			TNC->Minimized = TRUE;
			break;
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_CTLCOLORDLG:
		return (LONG)bgBrush;

		 
	case WM_CTLCOLORSTATIC:
	{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)bgBrush;
	}

	case WM_HSCROLL:
	{ 
		char value[16];

		switch (LOWORD(wParam))
		{     
		case TB_ENDTRACK:
		case TB_THUMBTRACK:

			TNC->TXOffset = SendMessage(TNC->xIDC_TXTUNE, TBM_GETPOS, 0, 0); 
			sprintf(value, "%d", TNC->TXOffset);
			MySetWindowText(TNC->xIDC_TXTUNEVAL, value);

            break;
		}

		default: 
			break; 
	}
	case WM_DESTROY:
		
		break;
	}
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}
#endif

BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow, WNDPROC WndProc, int Width, int Height, VOID ForcedCloseProc())
{
#ifdef LINBPQ
	return FALSE;
#else
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;
	HANDLE hDlg = 0;
	static int LP = 1235;

	if (TNC->hDlg)
	{
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(TNC->hDlg);
		return FALSE;							// Already open
	}

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    wc.lpfnWndProc = WndProc;                                      
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 
	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

//	if (TNC->Hardware == H_WINMOR || TNC->Hardware == H_TELNET ||TNC->Hardware == H_ARDOP ||
//			TNC->Hardware == H_V4 || TNC->Hardware == H_FLDIGI || TNC->Hardware == H_UIARQ || TNC->Hardware == H_VARA)
	if (TNC->PortRecord)
		sprintf(Title, "%s Status - Port %d %s", WindowTitle, TNC->Port, TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
	else
		sprintf(Title, "Rigcontrol");
	
	if (TNC->Hardware == H_MPSK)
		sprintf(Title, "Rigcontrol for MultiPSK Port %d", TNC->Port);

	TNC->hDlg = hDlg =  CreateMDIWindow(ClassName, Title, 0,
		  0, 0, Width, Height, ClientWnd, hInstance, ++LP);
	
	//	CreateDialog(hInstance,ClassName,0,NULL);
	
	Rect.top = 100;
	Rect.left = 20;
	Rect.right = Width + 20;
	Rect.bottom = Height + 100;


	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
		{
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &TNC->Minimized);

			if (Rect.top < - 500 || Rect.left < - 500)
			{
				Rect.left = 0;
				Rect.top = 0;
				Rect.right = 600;
				Rect.bottom = 400;
			}

			if (Rect.top < OffsetH)
			{
				int Error = OffsetH - Rect.top;
				Rect.top += Error;
				Rect.bottom += Error;
			}
		}

		if (TNC->Hardware == H_WINMOR || TNC->Hardware == H_ARDOP|| TNC->Hardware == H_VARA)	
			retCode = RegQueryValueEx(hKey,"TNC->RestartAfterFailure",0,			
				(ULONG *)&Type,(UCHAR *)&TNC->RestartAfterFailure,(ULONG *)&Vallen);

		RegCloseKey(hKey);
	}

	Top = Rect.top;
	Left = Rect.left;

//	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left - (OffsetW /2), Top - OffsetH, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);
	
	if (TNC->Minimized)
		ShowWindow(hDlg, SW_SHOWMINIMIZED);
	else
		ShowWindow(hDlg, SW_RESTORE);

	TNC->RigControlRow = RigControlRow;

	SetWindowText(TNC->xIDC_TNCSTATE, "Free");

	TNC->ForcedCloseProc = ForcedCloseProc;

	return TRUE;
#endif
}


// WL2K Reporting Code.

static SOCKADDR_IN sinx; 


VOID SendReporttoWL2KThread(void * unused);
VOID SendHTTPReporttoWL2KThread(void * unused);

VOID CheckWL2KReportTimer()
{
	if (WL2KReports == NULL)
		return;					// Shouldn't happen!

	WL2KTimer--;

	if (WL2KTimer != 0)
		return;

#ifdef WIN32
	WL2KTimer = 2 * 32910;			// Every 2 Hours - PC Tick is a bit slow 
#else
	WL2KTimer = 2 * 36000;			// Every 2 Hours 
#endif

	if (CheckAppl(NULL, "RMS         ") == NULL)
		if (CheckAppl(NULL, "RELAY       ") == NULL)
			return;

	_beginthread(SendHTTPReporttoWL2KThread, 0, 0);

	return;
}

static char HeaderTemplate[] = "POST %s HTTP/1.1\r\n"
	"Accept: application/json\r\n"
//	"Accept-Encoding: gzip,deflate,gzip, deflate\r\n"
	"Content-Type: application/json\r\n"
	"Host: %s:%d\r\n"
	"Content-Length: %d\r\n"
	//r\nUser-Agent: BPQ32(G8BPQ)\r\n"
//	"Expect: 100-continue\r\n"
	"\r\n{%s}";

char Missing[] = "** Missing **";

VOID GetJSONValue(char * _REPLYBUFFER, char * Name, char * Value)
{
	char * ptr1, * ptr2;

	strcpy(Value, Missing);

	ptr1 = strstr(_REPLYBUFFER, Name);

	if (ptr1 == 0)
		return;

	ptr1 += (strlen(Name) + 1);

	ptr2 = strchr(ptr1, '"');
			
	if (ptr2)
	{
		size_t ValLen = ptr2 - ptr1;
		memcpy(Value, ptr1, ValLen);
		Value[ValLen] = 0;
	}

	return;
}


// Send Winlink Session Record

extern char LOC[7];
extern char TextVerstring[50];

double Distance(double laa, double loa, double lah, double loh, BOOL KM);
double Bearing(double lat2, double lon2, double lat1, double lon1);
VOID SendHTTPRequest(SOCKET sock, char * Request, char * Params, int Len, char * Return);
SOCKET OpenWL2KHTTPSock();



struct WL2KMode
{
	int Mode;
	char * WL2KString;
	char * ADIFString;
	char * BPQString;
};

struct WL2KMode WL2KModeList[] =
{
	{0,"Packet 1200"},
	{1,"Packet 2400"},
	{2, "Packet 4800"},
	{3, "Packet 9600"},
	{4, "Packet 19200"},
	{5, "Packet 38400"},
	{11, "Pactor 1"},
	{12, "Pactor 1,2"},
	{13, "Pactor 1,2,3"},
	{14, "Pactor 2"},
	{15, "Pactor 2,3"},
	{16, "Pactor 3"},
	{17, "Pactor 1,2,3,4"},
	{18, "Pactor 2,3,4"},
	{19, "Pactor 3,4"},
	{20, "Pactor 4"},
	{21, "WINMOR 500"},
	{22, "WINMOR 1600"},
	{30, "Robust Packet"},
	{40, "ARDOP 200"},
	{41, "ARDOP 500"},
	{42, "ARDOP 1000"},
	{43, "ARDOP 2000"},
	{44, "ARDOP 2000 FM"},
	{50, "VARA"},
	{51, "VARA FM"},
	{52, "VARA FM WIDE"},
	{53, "VARA 500"}
};

char WL2KModes [55][18] = {
	"Packet 1200", "Packet 2400", "Packet 4800", "Packet 9600", "Packet 19200", "Packet 38400", "High Speed Packet", "", "", "", "",
	"Pactor 1", "Pactor", "Pactor", "Pactor 2", "Pactor", "Pactor 3", "Pactor", "Pactor", "Pactor", "Pactor 4", // 11 - 20
	"Winmor 500", "Winmor 1600", "", "", "", "", "", "", "",				// 21 - 29
	"Robust Packet", "", "", "", "", "", "", "", "", "",					// 30 - 39
	"ARDOP 200", "ARDOP 500", "ARDOP 1000", "ARDOP 2000", "ARDOP 2000 FM", "", "", "", "", "",	// 40 - 49
	"VARA", "VARA FM", "VARA FM WIDE", "VARA 500", "VARA 2750"};


VOID SendWL2KSessionRecordThread(void * param)
{
	SOCKET sock;
	char Message[512];

	strcpy(Message, param);
	free(param);

	Debugprintf("Sending %s", Message);

	sock = OpenWL2KHTTPSock();

	if (sock)
	{
		SendHTTPRequest(sock, "/session/add", (char *)Message, (int)strlen(Message), NULL);
		closesocket(sock);
	}

	return;
}

VOID SendWL2KRegisterHybridThread(void * param)
{
	SOCKET sock;
	char Message[512];

	strcpy(Message, param);
	free(param);

	Debugprintf("Sending %s", Message);

	sock = OpenWL2KHTTPSock();

	if (sock)
	{
		SendHTTPRequest(sock, "/radioNetwork/params/add", (char *)Message, (int)strlen(Message), NULL);
		closesocket(sock);
	}

	return;
}

VOID SendWL2KRegisterHybrid(struct TNCINFO * TNC)
{
	char Message[512];
	char Date[80] ;
	int Len;
	struct TCPINFO * TCP = TNC->TCPInfo;
	time_t T;
	struct tm * tm;
	char Call[10];

	if (TCP == NULL || TCP->GatewayLoc[0] == 0)
		return;

	strcpy(Call, TCP->GatewayCall);
	strlop(Call, '-');

	T = time(NULL);
	tm = gmtime(&T);

	//2021-10-31-14=35=29

	sprintf(Date, "%04d-%02d-%02d-%02d:%02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	
// "Callsign":"String","Password":"String","Param":"String","Value":"String","Key":"String"

	Len = sprintf(Message, "\"Callsign\":\"%s\",\"Password\":\"%s\",\"Param\":\"RMSRelayVersion\",\"Value\":\"%s|%s|*HARMNNNN|%s|%s|\"",
		Call, TCP->SecureCMSPassword, Date, "3.1.11.2",
		TCP->HybridServiceCode, TCP->GatewayLoc);

	SendWL2KRegisterHybridThread(_strdup(Message));

	Len = sprintf(Message, "\"Callsign\":\"%s\",\"Password\":\"%s\",\"Param\":\"CoLocatedRMS\",\"Value\":\"%s\"",
		Call, TCP->SecureCMSPassword, TCP->HybridCoLocatedRMS);

	SendWL2KRegisterHybridThread(_strdup(Message));

	Len = sprintf(Message, "\"Callsign\":\"%s\",\"Password\":\"%s\",\"Param\":\"AllowFreq\",\"Value\":\"%s\"",
		Call, TCP->SecureCMSPassword, TCP->HybridFrequencies);

	SendWL2KRegisterHybridThread(_strdup(Message));

	return;
}

BOOL NoSessionAccount = FALSE;
BOOL SessionAccountChecked = FALSE;

BOOL SendWL2KSessionRecord(ADIF * ADIF, int BytesSent, int BytesReceived)
{
/*
The API is /session/add https://api.winlink.org/json/metadata?op=SessionAdd

The important parameters are (others can be omitted):

Application (gateway program name)
Server (gateway callsign)
ServerGrid
Client (client callsign)
ClientGrid
Mode (Pactor, winmor, vara, etc)
Frequency
MessagesSent
MessagesReceived
BytesSent 
BytesReceived 
HoldingSeconds (duration of connection)
IdTag (random alphanumeric, 12 chars)

"Application":"RMS Trimode",
"Version":"1.3.25.0",
"Cms":"CMS-A",
"Server":"AB4NX",
"ServerGrid":"EM73WT",
"Client":"VE2SCA","ClientGrid":"",
"Sid":"","Mode":"WINMOR16",
"Frequency":10145000,
"Kilometers":0,
"Degrees":0,
"LastCommand":"&gt;",
"MessagesSent":0,
"MessagesReceived":0,
"BytesSent":179,
"BytesReceived":0,
"HoldingSeconds":126,
"IdTag":"ATK9S3QGL2E1"}
*/
	time_t T;

	char Message[4096] = "";
	char * MessagePtr;
	int MessageLen;
	int Dist = 0;
	int intBearing = 0;

	double Lat, Lon;
	double myLat, myLon;

	char Tag[32];

	SOCKET sock;
	char Response[1024];
	int Len;

	// Only report if the CMSCall has a WL2KAccount

	if (NoSessionAccount)
		return TRUE;

	if (!SessionAccountChecked)
	{
		// only check once

		sock = OpenWL2KHTTPSock();

		if (sock)
		{
			SessionAccountChecked = TRUE;

			Len = sprintf(Message, "\"Callsign\":\"%s\"", ADIF->CMSCall);

			SendHTTPRequest(sock, "/account/exists", Message, Len, Response);
			closesocket(sock);

			if (strstr(Response, "false"))
			{
				WritetoConsole("WL2K Traffic Reporting disabled - Gateway ");
				WritetoConsole(ADIF->CMSCall);
				WritetoConsole(" does not have a Winlink Account\r\n");
				Debugprintf("WL2K Traffic Reporting disabled - Gateway %s does not have a Winlink Account", ADIF->CMSCall);
				NoSessionAccount = TRUE;
				return TRUE;
			}
		}
	}

	if (ADIF == NULL || ADIF->LOC[0] == 0 || ADIF->Call[0] == 0)
		return TRUE;

	if (ADIF->StartTime == 0 || ADIF->ServerSID[0] == 0 || ADIF->CMSCall[0] == 0)
		return TRUE;

	T = time(NULL);

	// Extract Info we need

	// Distance and Bearing

	if (LOC[0] && ADIF->LOC[0])
	{
		if (FromLOC(LOC, &myLat, &myLon) == 0)  	// Basic checks on LOCs
			return TRUE;
		if (FromLOC(ADIF->LOC, &Lat, &Lon) == 0)
			return TRUE;

		Dist = (int)Distance(myLat, myLon, Lat, Lon, TRUE);
		intBearing = (int)Bearing(Lat, Lon, myLat, myLon);
	}

	MessageLen = sprintf(Message, "\"Application\":\"%s\",", "BPQ32");
	MessageLen += sprintf(&Message[MessageLen], "\"Version\":\"%s\",", TextVerstring);
	MessageLen += sprintf(&Message[MessageLen], "\"Cms\":\"%s\",", "CMS");
	MessageLen += sprintf(&Message[MessageLen], "\"Server\":\"%s\",", ADIF->CMSCall);
	MessageLen += sprintf(&Message[MessageLen], "\"ServerGrid\":\"%s\",", LOC);
	MessageLen += sprintf(&Message[MessageLen], "\"Client\":\"%s\",", ADIF->Call);
	MessageLen += sprintf(&Message[MessageLen], "\"ClientGrid\":\"%s\",", ADIF->LOC);
	MessageLen += sprintf(&Message[MessageLen], "\"Sid\":\"%s\",", ADIF->UserSID);
	MessageLen += sprintf(&Message[MessageLen], "\"Mode\":\"%s\",", WL2KModes[ADIF->Mode]);
	MessageLen += sprintf(&Message[MessageLen], "\"Frequency\":%lld,", ADIF->Freq);
	MessageLen += sprintf(&Message[MessageLen], "\"Kilometers\":%d,", Dist);
	MessageLen += sprintf(&Message[MessageLen], "\"Degrees\":%d,", intBearing);
	MessageLen += sprintf(&Message[MessageLen], "\"LastCommand\":\"%s\",", ADIF->Termination);
	MessageLen += sprintf(&Message[MessageLen], "\"MessagesSent\":%d,", ADIF->Sent);
	MessageLen += sprintf(&Message[MessageLen], "\"MessagesReceived\":%d,", ADIF->Received);
	MessageLen += sprintf(&Message[MessageLen], "\"BytesSent\":%d,", BytesSent);
	MessageLen += sprintf(&Message[MessageLen], "\"BytesReceived\":%d,", BytesReceived);
	MessageLen += sprintf(&Message[MessageLen], "\"HoldingSeconds\":%d,", (int)(T - ADIF->StartTime));
	sprintf(Tag, "%012X", (int)T * (rand() + 1));
	MessageLen += sprintf(&Message[MessageLen], "\"IdTag\":\"%s\"", Tag);

	MessagePtr = _strdup(Message);
	_beginthread(SendWL2KSessionRecordThread, 0, (void *)MessagePtr);

	return TRUE;
}

char APIKey[] = ",\"Key\":\"0D0C7AD6B38C45A7A9534E67111C38A7\"";


VOID SendHTTPRequest(SOCKET sock, char * Request, char * Params, int Len, char * Return)
{
	int InputLen = 0;
	int inptr = 0;
	char Buffer[2048];
	char Header[2048];
	char * ptr, * ptr1;
	int Sent;

	strcat(Params, APIKey);
	Len += (int)strlen(APIKey);

	sprintf(Header, HeaderTemplate, Request, "api.winlink.org", 80, Len + 2, Params);
	Sent = send(sock, Header, (int)strlen(Header), 0);

	if (Sent == -1)
	{
		int Err = WSAGetLastError();
		Debugprintf("Error %d from WL2K Update send()", Err);
		return;
	}

	while (InputLen != -1)
	{
		InputLen = recv(sock, &Buffer[inptr], 2048 - inptr, 0);

		if (InputLen == -1 || InputLen == 0)
		{
			int Err = WSAGetLastError();
			Debugprintf("Error %d from WL2K Update recv()", Err);
			return;
		}

		//	As we are using a persistant connection, can't look for close. Check
		//	for complete message

		inptr += InputLen;

		Buffer[inptr] = 0;
		
		ptr = strstr(Buffer, "\r\n\r\n");

		if (ptr)
		{
			// got header

			int Hddrlen = (int)(ptr - Buffer);
					
			ptr1 = strstr(Buffer, "Content-Length:");

			if (ptr1)
			{
				// Have content length

				int ContentLen = atoi(ptr1 + 16);

				if (ContentLen + Hddrlen + 4 == inptr)
				{
					// got whole response

					if (strstr(Buffer, " 200 OK"))
					{
						if (Return)
						{
							memcpy(Return, ptr + 4, ContentLen); 
							Return[ContentLen] = 0;
						}
						else
							Debugprintf("WL2K Database update ok");
					
					}
					else
					{
						strlop(Buffer, 13);
						Debugprintf("WL2K Update Params - %s", Params);
						Debugprintf("WL2K Update failed - %s", Buffer);
					}
					return;
				}
			}
			else
			{
				ptr1 = strstr(_strlwr(Buffer), "transfer-encoding:");
				
				if (ptr1)
				{
					// Just accept anything until I've sorted things with Lee
					Debugprintf("%s", ptr1);
					Debugprintf("WL2K Database update ok");
					return;
				}
			}
		}
	}
}

BOOL WL2KAccountChecked = FALSE;
BOOL NoWL2KAccount = FALSE;

VOID SendHTTPReporttoWL2KThread(void * unused)
{
	// Uses HTTP/JSON Interface

	struct WL2KInfo * WL2KReport = WL2KReports;
	char * LastHost = NULL;
	char * LastRMSCall = NULL;
	char Message[512];
	int LastSocket = 0;
	SOCKET sock = 0;
	struct sockaddr_in destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	int Len;

	// Send all reports in list

	char Response[1024];

	// Only report if the CMSCall has a WL2KAccount

	if (NoWL2KAccount)
		return;

	if (!WL2KAccountChecked)
	{
		// only check once

		sock = OpenWL2KHTTPSock();

		if (sock)
		{
			WL2KAccountChecked = TRUE;

			Len = sprintf(Message, "\"Callsign\":\"%s\"",
				WL2KReport->BaseCall);

			SendHTTPRequest(sock, "/account/exists", Message, Len, Response);
			closesocket(sock);

			if (strstr(Response, "false"))
			{
				WritetoConsole("WL2K Reporting disabled - Gateway ");
				WritetoConsole(WL2KReport->BaseCall);
				WritetoConsole(" does not have a Winlink Account\r\n");
				NoWL2KAccount = TRUE;
				return;
			}
		}
	}

	while (WL2KReport)
	{
		// Resolve Name if needed

		if (LastHost && strcmp(LastHost, WL2KReport->Host) == 0)		// Same host?
			goto SameHost;

		// New Host - Connect to it
	
		LastHost = WL2KReport->Host;
	
		destaddr.sin_family = AF_INET; 
		destaddr.sin_addr.s_addr = inet_addr(WL2KReport->Host);
		destaddr.sin_port = htons(WL2KReport->WL2KPort);

		if (destaddr.sin_addr.s_addr == INADDR_NONE)
		{
			//	Resolve name to address

			Debugprintf("Resolving %s", WL2KReport->Host);
			HostEnt = gethostbyname (WL2KReport->Host);
		 
			if (!HostEnt)
			{
				err = WSAGetLastError();

				Debugprintf("Resolve Failed for %s %d %x", WL2KReport->Host, err, err);
				return;			// Resolve failed
			}
	
			memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
		}

		//   Allocate a Socket entry

		if (sock)
			closesocket(sock);

		sock = socket(AF_INET, SOCK_STREAM, 0);

		if (sock == INVALID_SOCKET)
  	 		return; 

//		ioctlsocket(sock, FIONBIO, &param);
 
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt, 4);

		destaddr.sin_family = AF_INET;

		if (sock == INVALID_SOCKET)
		{
			sock = 0;
			return; 
		}

		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

		// Connect to Host

		if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
		{
			err=WSAGetLastError();

			//
			//	Connect failed
			//

			Debugprintf("Connect Failed");
			closesocket(sock);
			sock = 0;
			break;
		}

	SameHost:

		Len = sprintf(Message,
				"\"Callsign\":\"%s\","
				"\"BaseCallsign\":\"%s\","
				"\"GridSquare\":\"%s\","
				"\"Frequency\":%lld,"
				"\"Mode\":%d,"
				"\"Baud\":%d,"
				"\"Power\":%d,"
				"\"Height\":%d,"
				"\"Gain\":%d,"
				"\"Direction\":%d,"
				"\"Hours\":\"%s\","
				"\"ServiceCode\":\"%s\"",

				WL2KReport->RMSCall, WL2KReport->BaseCall, WL2KReport->GridSquare,
				WL2KReport->Freq, WL2KReport->mode, WL2KReport->baud, WL2KReport->power,
				WL2KReport->height, WL2KReport->gain, WL2KReport->direction,
				WL2KReport->Times, WL2KReport->ServiceCode);

		Debugprintf("Sending %s", Message);

		SendHTTPRequest(sock, "/channel/add", Message, Len, NULL);
	
		
		//	Send Version Message


		if (LastRMSCall == NULL || strcmp(WL2KReport->RMSCall, LastRMSCall) != 0)
		{
			int Len;
			
			LastRMSCall = WL2KReport->RMSCall;

	//	"Callsign":"String","Program":"String","Version":"String","Comments":"String"
		
			Len = sprintf(Message, "\"Callsign\":\"%s\",\"Program\":\"BPQ32\","
				"\"Version\":\"%d.%d.%d.%d\",\"Comments\":\"Test Comment\"",
				WL2KReport->RMSCall, Ver[0], Ver[1], Ver[2], Ver[3]);

			Debugprintf("Sending %s", Message);

			SendHTTPRequest(sock, "/version/add", Message, Len, NULL);
		}

		WL2KReport = WL2KReport->Next;
	}

	Sleep(100);
	closesocket(sock);
	sock = 0;

}

struct WL2KInfo * DecodeWL2KReportLine(char *  buf)
{
	//06'<callsign>', '<base callsign>', '<grid square>', <frequency>, <mode>, <baud>, <power>,
	// <antenna height>, <antenna gain>, <antenna direction>, '<hours>', <group reference>, '<service code>'

	 // WL2KREPORT  service, api.winlink.org, 80, GM8BPQ, IO68VL, 00-23, 144800000, PKT1200, 10, 20, 5, 0, BPQTEST
	
	char * Context;
	char * p_cmd;
	char * param;
	char errbuf[256];
	struct WL2KInfo * WL2KReport = zalloc(sizeof(struct WL2KInfo));
	char * ptr;
	char Param[8][256];
	char * ptr1, * ptr2;
	int n = 0;

	memset(Param, 0, 2048);

	strcpy(errbuf, buf); 

	p_cmd = strtok_s(&buf[10], ", \t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;
	
	strcpy(WL2KReport->ServiceCode, p_cmd);

	// Can default Host and Port, so cant use strtok for them
	
	ptr1 = Context;

	while (ptr1 && *ptr1 && n < 2)
	{
		while(ptr1 && *ptr1 && *ptr1 == ' ')
			ptr1++;	
		
		ptr2 = strchr(ptr1, ',');
		if (ptr2) *ptr2++ = 0;

		strcpy(&Param[n][0], ptr1);
		strlop(Param[n++], ' ');
		ptr1 = ptr2;

	}

	if (n < 2)
		goto BadLine;

	if (Param[1][0] == 0)
		WL2KReport->WL2KPort = 80;			// HTTP Interface
	else
		WL2KReport->WL2KPort = atoi(&Param[1][0]);

	if (Param[0][0] == 0)
		WL2KReport->Host = _strdup("api.winlink.org");
	else
	{
		_strlwr(&Param[0][0]);

		if (strstr(&Param[0][0], "winlink.org"))
		{
			WL2KReport->WL2KPort = 80;		// HTTP Interface
			WL2KReport->Host = _strdup("api.winlink.org");
		}
		else
			WL2KReport->Host = _strdup(&Param[0][0]);
	}

	Context = ptr1;

	p_cmd = strtok_s(NULL, ", \t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;

	if (WL2KReport->WL2KPort == 0) goto BadLine;

	strcpy(WL2KReport->RMSCall, p_cmd);
	strcpy(WL2KReport->BaseCall, p_cmd);
	strlop(WL2KReport->BaseCall, '-');					// Remove any SSID
	
	strcpy(WL2KCall, WL2KReport->BaseCall);				// For SYSOP Update

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
	if (p_cmd == NULL) goto BadLine;
	if (strlen(p_cmd) != 6) goto BadLine;

	strcpy(WL2KReport->GridSquare, p_cmd);
	strcpy(WL2KLoc, p_cmd);

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;
	if (strlen(p_cmd) > 79) goto BadLine;
	
	// Convert any : in times to comma

	ptr = strchr(p_cmd, ':');

	while (ptr)
	{
		*ptr = ',';
		ptr = strchr(p_cmd, ':');
	}

	strcpy(WL2KReport->Times, p_cmd);

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;

	WL2KReport->Freq = strtoll(p_cmd, NULL, 10);

	if (WL2KReport->Freq == 0)	// Invalid
		goto BadLine;					

	param = strtok_s(NULL, " ,\t\n\r", &Context);

	// Mode Designator - one of

	// PKTnnnnnn
	// WINMOR500
	// WINMOR1600
	// ROBUST
	// P1 P12 P123 P1234 etc

	if (memcmp(param, "PKT", 3) == 0)
	{
		int Speed, Mode;

		Speed = atoi(&param[3]);

		 WL2KReport->baud = Speed;
			
		 if (Speed <= 1200)
			 Mode = 0;					// 1200
		 else if (Speed <= 2400)
			 Mode = 1;					// 2400
		 else if (Speed <= 4800)
			 Mode = 2;					// 4800
		 else if (Speed <= 9600)
			 Mode = 3;					// 9600
		 else if (Speed <= 19200)
			 Mode = 4;					// 19200
		 else if (Speed <= 38400)
			 Mode = 5;					// 38400
		 else
			 Mode = 6;					// >38400

		WL2KReport->mode = Mode;
	}
	else if (_stricmp(param, "WINMOR500") == 0)
		WL2KReport->mode = 21;
	else if (_stricmp(param, "WINMOR1600") == 0)
		WL2KReport->mode = 22;
	else if (_stricmp(param, "ROBUST") == 0)
	{
		WL2KReport->mode = 30;
		WL2KReport->baud = 600;
	}
	else if (_stricmp(param, "ARDOP200") == 0)
		WL2KReport->mode = 40;
	else if (_stricmp(param, "ARDOP500") == 0)
		WL2KReport->mode = 41;
	else if (_stricmp(param, "ARDOP1000") == 0)
		WL2KReport->mode = 42;
	else if (_stricmp(param, "ARDOP2000") == 0)
		WL2KReport->mode = 43;
	else if (_stricmp(param, "ARDOP2000FM") == 0)
		WL2KReport->mode = 44;
	else if (_stricmp(param, "P1") == 0)
		WL2KReport->mode = 11;
	else if (_stricmp(param, "P12") == 0)
		WL2KReport->mode = 12;
	else if (_stricmp(param, "P123") == 0)
		WL2KReport->mode = 13;
	else if (_stricmp(param, "P2") == 0)
		WL2KReport->mode = 14;
	else if (_stricmp(param, "P23") == 0)
		WL2KReport->mode = 15;
	else if (_stricmp(param, "P3") == 0)
		WL2KReport->mode = 16;
	else if (_stricmp(param, "P1234") == 0)
		WL2KReport->mode = 17;
	else if (_stricmp(param, "P234") == 0)
		WL2KReport->mode = 18;
	else if (_stricmp(param, "P34") == 0)
		WL2KReport->mode = 19;
	else if (_stricmp(param, "P4") == 0)
		WL2KReport->mode = 20;
	else if (_stricmp(param, "VARA") == 0)
		WL2KReport->mode = 50;
	else if (_stricmp(param, "VARA2300") == 0)
		WL2KReport->mode = 50;
	else if (_stricmp(param, "VARAFM") == 0)
		WL2KReport->mode = 51;
	else if (_stricmp(param, "VARAFM12") == 0)
		WL2KReport->mode = 51;
	else if (_stricmp(param, "VARAFM96") == 0)
		WL2KReport->mode = 52;
	else if (_stricmp(param, "VARA500") == 0)
		WL2KReport->mode = 53;
	else if (_stricmp(param, "VARA2750") == 0)
		WL2KReport->mode = 54;
	else
		goto BadLine;
	
	param = strtok_s(NULL, " ,\t\n\r", &Context);

	// Optional Params

	WL2KReport->power = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->height = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->gain = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->direction = (param)? atoi(param) : 0;

	WL2KTimer = 60;

	WL2KReport->Next = WL2KReports;
	WL2KReports = WL2KReport;

	return WL2KReport;

BadLine:

	WritetoConsole(" Bad config record ");
	WritetoConsole(errbuf);
	WritetoConsole("\r\n");

	return 0;
}

VOID UpdateMHSupport(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction, char * Loc, BOOL Report, BOOL Digis);

VOID UpdateMHwithDigis(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction)
{
	UpdateMHSupport(TNC, Call, Mode, Direction, NULL, TRUE, TRUE);
}
VOID UpdateMHEx(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction, char * LOC, BOOL Report)
{
	UpdateMHSupport(TNC, Call, Mode, Direction, LOC, Report, FALSE);
}

VOID UpdateMH(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction)
{
	UpdateMHSupport(TNC, Call, Mode, Direction, NULL, TRUE, FALSE);
}

VOID UpdateMHSupport(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction, char * Loc, BOOL Report, BOOL Digis)
{
	PMHSTRUC MH = TNC->PortRecord->PORTCONTROL.PORTMHEARD;
	PMHSTRUC MHBASE = MH;
	UCHAR AXCall[72] = "";
	int i;
	char * LOC, *  LOCEND;
	char ReportMode[20];
	char NoLOC[7] = "";
	double Freq;
	char ReportFreq[350] = "";
	int OldCount = 0;
	char ReportCall[16];

	if (MH == 0) return;

	if (Digis)
	{
		// Call is an ax.25 digi string not a text call

		memcpy(AXCall, Call, 7 * 9);
		ReportCall[ConvFromAX25(Call, ReportCall)] = 0;

		// if this is a UI frame with a locator or APRS position
		// we could derive a position from it

	}
	else
	{
		strcpy(ReportCall, Call);
		ConvToAX25(Call, AXCall);
		AXCall[6] |= 1;					// Set End of address
	}

	// Adjust freq to centre

//	if (Mode != ' ' && TNC->RIG->Valchar[0])
	if (TNC->RIG->Valchar[0])
	{
		if (TNC->Hardware == H_UZ7HO)	
		{
			// See if we have Center Freq Info
			if (TNC->AGWInfo->CenterFreq)
			{
				Freq = atof(TNC->RIG->Valchar) + ((TNC->AGWInfo->CenterFreq * 1.0) / 1000000.0);
			}
#ifdef WIN32
			else if (TNC->AGWInfo->hFreq)
			{
				char Centre[16];
				double ModemFreq;

				SendMessage(TNC->AGWInfo->hFreq, WM_GETTEXT, 15, (LPARAM)Centre);

				ModemFreq = atof(Centre);

				Freq = atof(TNC->RIG->Valchar) + (ModemFreq / 1000000);
			}
#endif	
			else
				Freq = atof(TNC->RIG->Valchar) + 0.0015;		// Assume 1500
		}
		else

			// Not UZ7HO or Linux
		
			Freq = atof(TNC->RIG->Valchar) + 0.0015;

		_gcvt(Freq, 9, ReportFreq);
	}

	if (TNC->Hardware == H_ARDOP)	
	{
		LOC = memchr(Call, '[', 20);

		if (LOC)
		{
			LOCEND = memchr(Call, ']', 30);
			if (LOCEND)
			{
				LOC--;
				*(LOC++) = 0;
				*(LOCEND) = 0;
				LOC++;
				if (strlen(LOC) != 6 && strlen(LOC) != 0)
				{
					Debugprintf("Corrupt LOC %s %s", Call, LOC);
					LOC = NoLOC;
				}
				goto NOLOC;
			}		
		}
	}

	else if (TNC->Hardware != H_WINMOR)			// Only WINMOR has a locator
	{
		LOC = NoLOC;
		goto NOLOC;
	}

 	LOC = memchr(Call, '(', 20);

	if (LOC)
	{
		LOCEND = memchr(Call, ')', 30);
		if (LOCEND)
		{
			LOC--;
			*(LOC++) = 0;
			*(LOCEND) = 0;
			LOC++;
			if (strlen(LOC) != 6 && strlen(LOC) != 0)
			{
				Debugprintf("Corrupt LOC %s %s", Call, LOC);
				LOC = NoLOC;
			}
		}		
	}
	else
		LOC = NoLOC;

NOLOC:

	if (Loc)
		LOC = Loc;		// Supplied Locator overrides

	for (i = 0; i < MHENTRIES; i++)
	{
		if (Mode == ' ' || Mode == '*')			// Packet
		{
			if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) && MH->MHDIGI == Mode)) // Spare or our entry
			{
				OldCount = MH->MHCOUNT;
				goto DoMove;
			}
		}
		else
		{
			if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) &&
				MH->MHDIGI == Mode && strcmp(MH->MHFreq, ReportFreq) == 0)) // Spare or our entry
			{
				OldCount = MH->MHCOUNT;
				goto DoMove;
			}
		}
		MH++;
	}

	//	TABLE FULL AND ENTRY NOT FOUND - MOVE DOWN ONE, AND ADD TO TOP

	i = MHENTRIES - 1;
		
	// Move others down and add at front
DoMove:

	if (i != 0)				// First
		memmove(MHBASE + 1, MHBASE, i * sizeof(MHSTRUC));

//	memcpy (MHBASE->MHCALL, Buffer->ORIGIN, 7 * 9);	
	memcpy (MHBASE->MHCALL, AXCall, 7 * 9);	// Save Digis
	MHBASE->MHDIGI = Mode;
	MHBASE->MHTIME = time(NULL);
	MHBASE->MHCOUNT = ++OldCount;

	memcpy(MHBASE->MHLocator, LOC, 6);
	strcpy(MHBASE->MHFreq, ReportFreq);

	// Report to NodeMap

	if (Report == FALSE)
		return;

	if (Mode == '*')
		return;							// Digi'ed Packet
	
	if (Mode == ' ') 					// Packet Data
	{
		if (TNC->PktUpdateMap == 1)
			Mode = '!';
		else	
			return;
	}
			
	ReportMode[0] = TNC->Hardware + '@';
	ReportMode[1] = Mode;
	if (TNC->Hardware == H_HAL)
		ReportMode[2] = TNC->CurrentMode; 
	else
		ReportMode[2] = (TNC->RIG->CurrentBandWidth) ? TNC->RIG->CurrentBandWidth : '?';
	ReportMode[3] = Direction;
	ReportMode[4] = 0;

	// If no position see if we have an APRS posn

	if (LOC[0] == 0)
	{
		double Lat, Lon;

		if (GetPosnFromAPRS(ReportCall, &Lat, &Lon) && Lat != 0.0)
		{
			ToLOC(Lat, Lon, LOC);
		}
	}

 	SendMH(TNC, ReportCall, ReportFreq, LOC, ReportMode);

	return;
}

VOID CloseDriverWindow(int port)
{
#ifndef LINBPQ

	struct TNCINFO * TNC;

	TNC = TNCInfo[port];
	if (TNC == NULL)
		return;

	if (TNC->hDlg == NULL)
		return;

	PostMessage(TNC->hDlg, WM_CLOSE,0,0);
//	DestroyWindow(TNC->hDlg);

	TNC->hDlg = NULL;
#endif
	return;
}

VOID SaveWindowPos(int port)
{
#ifndef LINBPQ

	struct TNCINFO * TNC;
	char Key[80];

	TNC = TNCInfo[port];

	if (TNC == NULL)
		return;

	if (TNC->hDlg == NULL)
		return;
	
	sprintf(Key, "PACTOR\\PORT%d", port);

	SaveMDIWindowPos(TNC->hDlg, Key, "Size", TNC->Minimized);

#endif
	return;
}

VOID ShowTraffic(struct TNCINFO * TNC)
{
	char Status[80];

	sprintf(Status, "RX %d TX %d ACKED %d ", 
		TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);
#ifndef LINBPQ
	SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
#endif
}

BOOL InterlockedCheckBusy(struct TNCINFO * ThisTNC)
{
	// See if this port, or any interlocked ports are reporting channel busy

	struct TNCINFO * TNC;
	int i;
	int rxInterlock = ThisTNC->RXRadio;
	int txInterlock = ThisTNC->TXRadio;

	if (ThisTNC->Busy)
		return TRUE;				// Our port is busy
		
	if (rxInterlock == 0 && txInterlock == 0)
		return ThisTNC->Busy;		// No Interlock

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
	
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

		if (rxInterlock == TNC->RXRadio || txInterlock == TNC->TXRadio)	// Same Group	
			if (TNC->Busy)
				return TRUE;				// Interlocked port is busy

	}
	return FALSE;					// None Busy
}

char ChallengeResponse[13];

char * GetChallengeResponse(char * Call, char *  ChallengeString)
{
	// Generates a response to the CMS challenge string...

	long long Challenge = _atoi64(ChallengeString);
	long long CallSum = 0;
	long long Mask;
	long long Response;
	long long XX = 1065484730;

	char CallCopy[10];
	UINT i;


	if (Challenge == 0)
		return "000000000000";

// Calculate Mask from Callsign

	memcpy(CallCopy, Call, 10);
	strlop(CallCopy, '-');
	strlop(CallCopy, ' ');

	for (i = 0; i < strlen(CallCopy); i++)
	{
		CallSum += CallCopy[i];
	}
	
	Mask = CallSum + CallSum * 4963 + CallSum * 782386;

	Response = (Challenge % 930249781);
	Response ^= Mask;

	sprintf(ChallengeResponse, "%012lld", Response);

	return ChallengeResponse;
}

SOCKET OpenWL2KHTTPSock()
{
	SOCKET sock = 0;
	struct sockaddr_in destaddr;
	struct sockaddr_in sinx; 
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
		
	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(80);

	//	Resolve name to address

	HostEnt = gethostbyname ("api.winlink.org");
		 
	if (!HostEnt)
	{
		err = WSAGetLastError();

		Debugprintf("Resolve Failed for %s %d %x", "api.winlink.org", err, err);
		return 0 ;			// Resolve failed
	}
	
	memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	
	//   Allocate a Socket entry

	sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return 0; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (struct sockaddr *) &sinx, addrlen) != 0 )
  	 	return FALSE; 

	if (connect(sock,(struct sockaddr *) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();
		closesocket(sock);
		return 0;
	}

	return sock;
}

BOOL GetWL2KSYSOPInfo(char * Call, char * _REPLYBUFFER)
{
	SOCKET sock = 0;
	int Len;
	char Message[1000];
		
	sock = OpenWL2KHTTPSock();

	if (sock == 0)
		return 0;
	
	// {"Callsign":"String"}
			
	Len = sprintf(Message, "\"Callsign\":\"%s\"", Call);
		
	SendHTTPRequest(sock, "/sysop/get", Message, Len, _REPLYBUFFER);

	closesocket(sock);

	return _REPLYBUFFER[0];
}

BOOL UpdateWL2KSYSOPInfo(char * Call, char * SQL)
{
	SOCKET sock = 0;
	struct sockaddr_in destaddr;
	struct sockaddr_in sinx; 
	int len = 100;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[1000];
	char SendBuffer[1000];
		
	destaddr.sin_family = AF_INET; 
	destaddr.sin_addr.s_addr = inet_addr("api.winlink.org");
	destaddr.sin_port = htons(80);

	HostEnt = gethostbyname ("api.winlink.org");
		 
	if (!HostEnt)
	{
		err = WSAGetLastError();

		Debugprintf("Resolve Failed for %s %d %x", "api.winlink.org", err, err);
		return 0 ;			// Resolve failed
	}
	
	memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	

	//   Allocate a Socket entry

	sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return 0; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (struct sockaddr *) &sinx, addrlen) != 0 )
  	 	return FALSE; 

	if (connect(sock,(struct sockaddr *) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();
		closesocket(sock);
		return 0;
	}

	len = recv(sock, &Buffer[0], len, 0);

	len = sprintf(SendBuffer, "02%07d%-12s%s%s", (int)strlen(SQL), Call, GetChallengeResponse(Call, Buffer), SQL);

	send(sock, SendBuffer, len, 0);

	len = 1000;

	len = recv(sock, &Buffer[0], len, 0);

	Buffer[len] = 0;
	Debugprintf(Buffer);

	closesocket(sock);

	return TRUE;

}
// http://server.winlink.org:8085/csv/reply/ChannelList?Modes=40,41,42,43,44&ServiceCodes=BPQTEST,PUBLIC

// Process config lines that are common to a number of HF modes

static char ** SeparateMultiString(char * MultiString)
{
	char ** Value;
	int Count = 0;
	char * ptr, * ptr1;

	// Convert to string array

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;

	strlop(MultiString, 13);
	ptr = MultiString;

	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, '|');
			
		if (ptr1)
			*(ptr1++) = 0;

		if (strlen(ptr))
		{
			Value = realloc(Value, (Count+2) * sizeof(void *));
			Value[Count++] = _strdup(ptr);
		}
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}




extern int nextDummyInterlock;

int standardParams(struct TNCINFO * TNC, char * buf)
{
	if (_memicmp(buf, "WL2KREPORT", 10) == 0)
		TNC->WL2K = DecodeWL2KReportLine(buf);
	else if (_memicmp(buf, "SESSIONTIMELIMIT", 16) == 0)
		TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit = atoi(&buf[17]) * 60;
	else if (_memicmp(buf, "BUSYHOLD", 8) == 0)		// Hold Time for Busy Detect
		TNC->BusyHold = atoi(&buf[8]);
	else if (_memicmp(buf, "BUSYWAIT", 8) == 0)		// Wait time before failing connect if busy
		TNC->BusyWait = atoi(&buf[8]);
	else if (_memicmp(buf, "AUTOSTARTDELAY", 14) == 0) // Time to wait for TNC to start
		TNC->AutoStartDelay = atoi(&buf[15]);
	else if (_memicmp(buf, "DEFAULTRADIOCOMMAND", 19) == 0)
		TNC->DefaultRadioCmd = _strdup(&buf[20]);
	else if (_memicmp(buf, "MYCALLS", 7) == 0)
	{
		TNC->LISTENCALLS = _strdup(&buf[8]);
		strlop(TNC->LISTENCALLS, '\r');
	}
	else if (_memicmp(buf, "NRNEIGHBOUR", 11) == 0)
		TNC->NRNeighbour = _strdup(&buf[12]);
	else if (_memicmp(buf, "MAXCONREQ", 9) == 0)		// Hold Time for Busy Detect
		TNC->MaxConReq = atoi(&buf[9]);

	else if (_memicmp(buf, "FREQUENCY", 9) == 0)
		TNC->Frequency = _strdup(&buf[10]);
	else if (_memicmp(buf, "SendTandRtoRelay", 16) == 0)
		TNC->SendTandRtoRelay = atoi(&buf[17]);
	else if (_memicmp(buf, "Radio", 5) == 0)		// Rig Control RADIO for TX amd RX (Equiv to INTERLOCK)
		TNC->RXRadio = TNC->TXRadio = atoi(&buf[6]);
	else if (_memicmp(buf, "TXRadio", 7) == 0)		// Rig Control RADIO for TX
		TNC->TXRadio = atoi(&buf[8]);
	else if (_memicmp(buf, "RXRadio", 7) == 0)		// Rig Control RADIO for RXFRETRIES
		TNC->RXRadio = atoi(&buf[8]);
	else if (_memicmp(buf, "TXFreq", 6) == 0)		// For PTT Sets Freq mode
		TNC->TXFreq = strtoll(&buf[7], NULL, 10);
	else if (_memicmp(buf, "DefaultTXFreq", 13) == 0)	// Set at end of session
		TNC->DefaultTXFreq = atof(&buf[14]);
	else if (_memicmp(buf, "DefaultRXFreq", 13) == 0)	// Set at end of session
		TNC->DefaultRXFreq = atof(&buf[14]);
	else if (_memicmp(buf, "ActiveTXFreq", 12) == 0)	// Set at start of session
		TNC->ActiveTXFreq = atof(&buf[13]);
	else if (_memicmp(buf, "ActiveRXFreq", 12) == 0)	// Set at start of session
		TNC->ActiveRXFreq = atof(&buf[13]);
	else if (_memicmp(buf, "DisconnectScript", 16) == 0)	// Set at start of session
		TNC->DisconnectScript = SeparateMultiString(&buf[17]);
	else if (_memicmp(buf, "PTTONHEX", 8) == 0)
	{
		// Hex String to use for PTT on for this port

		char * ptr1 = &buf[9];
		char * ptr2 = TNC->PTTOn;
		int i, j, len;

		_strupr(ptr1);

		TNC->PTTOnLen = len = strlen(ptr1) / 2;

		if (len < 240)
		{
			while ((len--) > 0)
			{
				i = *(ptr1++);
				i -= '0';
				if (i > 9)
					i -= 7;

				j = i << 4;

				i = *(ptr1++);
				i -= '0';
				if (i > 9)
					i -= 7;

				*(ptr2++) = j | i;
			}
		}
	}
	else if (_memicmp(buf, "PTTOFFHEX", 9) == 0)
	{
		// Hex String to use for PTT off

		char * ptr = &buf[10];
		char * ptr2 = TNC->PTTOff;
		int i, j, len;

		_strupr(ptr);

		TNC->PTTOffLen = len = strlen(ptr) / 2;

		if (len < 240)
		{
			while ((len--) > 0)
			{
				i = *(ptr++);
				i -= '0';
				if (i > 9)
					i -= 7;

				j = i << 4;

				i = *(ptr++);
				i -= '0';
				if (i > 9)
					i -= 7;

				*(ptr2++) = j | i;
			}
		}
	}
	else
		return FALSE;

	return TRUE;
}

void DecodePTTString(struct TNCINFO * TNC, char * ptr)
{
	if (_stricmp(ptr, "CI-V") == 0)
		TNC->PTTMode = PTTCI_V;
	else if (_stricmp(ptr, "CAT") == 0)
		TNC->PTTMode = PTTCI_V;
	else if (_stricmp(ptr, "RTS") == 0)
		TNC->PTTMode = PTTRTS;
	else if (_stricmp(ptr, "DTR") == 0)
		TNC->PTTMode = PTTDTR;
	else if (_stricmp(ptr, "DTRRTS") == 0)
		TNC->PTTMode = PTTDTR | PTTRTS;
	else if (_stricmp(ptr, "CM108") == 0)
		TNC->PTTMode = PTTCM108;
	else if (_stricmp(ptr, "HAMLIB") == 0)
		TNC->PTTMode = PTTHAMLIB;
	else if (_stricmp(ptr, "FLRIG") == 0)
		TNC->PTTMode = PTTFLRIG;
}

extern SOCKET ReportSocket;
extern char LOCATOR[80];
extern char ReportDest[7];
extern int NumberofPorts;
extern struct RIGPORTINFO * PORTInfo[34];		// Records are Malloc'd

time_t LastModeReportTime;
time_t LastFreqReportTime;

VOID SendReportMsg(char * buff, int txlen);

void sendModeReport()
{
	// if TNC is connected send mode and frequencies to Node Map as a MODE record
	// Are we better sending scan info as a separate record ??

	// MODE Port, HWType, Interlock 

	struct PORTCONTROL * PORT = PORTTABLE;

	struct TNCINFO * TNC;
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[300] = "MODE ";
	int i, Len = 5;

	if ((CurrentSecs - LastModeReportTime) < 900)	// Every 15 Mins
		return;

	LastModeReportTime = CurrentSecs;

	for (i = 0; i < NUMBEROFPORTS; i++)
	{	
		if (PORT->PROTOCOL == 10)
		{
			PEXTPORTDATA PORTVEC = (PEXTPORTDATA)PORT;
			TNC = TNCInfo[PORT->PORTNUMBER];
			PORT = PORT->PORTPOINTER;

			if (TNC == NULL)
				continue;

			if (TNC->CONNECTED == 0 && TNC->TNCOK == 0)
				continue;

			if (ReportSocket == 0 || LOCATOR[0] == 0)
				continue;

			if (TNC->Frequency)
				Len += sprintf(&Msg[Len], "%d,%d,%d,%.6f/", TNC->Port, TNC->Hardware, TNC->RXRadio, atof(TNC->Frequency));
			else
				Len += sprintf(&Msg[Len], "%d,%d,%d/", TNC->Port, TNC->Hardware, TNC->RXRadio);

			if (Len > 240)
				break;
		}
		else
			PORT = PORT->PORTPOINTER;
	}

	if (Len == 5)
		return;			// Nothing to send
	
	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;
}

void sendFreqReport(char * From)
{
	// Send info from rig control or Port Frequency info to Node Map for Mode page.

	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[300] = "FREQ ";
	int i, Len = 5, p;

	struct RIGPORTINFO * RIGPORT;
	struct RIGINFO * RIG;
	struct TimeScan * Band;	
	struct PORTCONTROL * PORT = PORTTABLE;
	struct TNCINFO * TNC;

	if ((CurrentSecs - LastFreqReportTime) < 7200)	// Every 2 Hours
		return;

	LastFreqReportTime = CurrentSecs;

	for (p = 0; p < NumberofPorts; p++)
	{
		RIGPORT = PORTInfo[p];

		for (i = 0; i < RIGPORT->ConfiguredRigs; i++)
		{
			int j = 1, k = 0;

			RIG = &RIGPORT->Rigs[i];

			if (RIG->reportFreqs)
			{
				Len += sprintf(&Msg[Len], "%d/00:00/%s,\\|",RIG->Interlock,RIG->reportFreqs); 
			}
			else
			{
				if (RIG->TimeBands)
				{
					Len += sprintf(&Msg[Len], "%d/",RIG->Interlock); 
					while (RIG->TimeBands[j])
					{
						Band = RIG->TimeBands[j];
						k = 0;

						if (Band->Scanlist[0])
						{
							Len += sprintf(&Msg[Len], "%02d:%02d/", Band->Start / 3600, Band->Start % 3600); 

							while (Band->Scanlist[k])
							{
								Len += sprintf(&Msg[Len],"%.0f,", Band->Scanlist[k]->Freq + RIG->rxOffset);
								k++;
							}
							Len += sprintf(&Msg[Len], "\\"); 
						}
						j++;
					}
					Len += sprintf(&Msg[Len], "|"); 
				}
			}
		}
	}

	// Look for Port freq info

	for (i = 0; i < NUMBEROFPORTS; i++)
	{	
		if (PORT->PROTOCOL == 10)
		{
			PEXTPORTDATA PORTVEC = (PEXTPORTDATA)PORT;
			TNC = TNCInfo[PORT->PORTNUMBER];
			PORT = PORT->PORTPOINTER;

			if (TNC == NULL)
				continue;

			if (TNC->Frequency == NULL)
				continue;

			if (TNC->RIG->TimeBands && TNC->RIG->TimeBands[1]->Scanlist)
				continue;					// Have freq info from Rigcontrol
			
			if (TNC->RXRadio == 0)		// Replace with dummy
				TNC->RXRadio = nextDummyInterlock++;

			// Use negative port no instead of interlock group

			Len += sprintf(&Msg[Len], "%d/00:00/%.0f|", TNC->RXRadio, atof(TNC->Frequency) * 1000000.0);
		}
		else
			PORT = PORT->PORTPOINTER;
	}

	if (Len == 5)
		return;			// Nothing to send

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;
}


