
// Program to Convert RTS changes on Virtual COM Port to hamlib PTT commands


// Version 1. 0. 0. 1 December 2020

// Version 1. 0. 2. 1 April 2018


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <Psapi.h>

#define LIBCONFIG_STATIC
#include "libconfig.h"


#include "BPQRemotePTTRes.h"

#define BPQICON 400

WSADATA WsaData;            // receives data from WSAStartup

#define WSA_READ WM_USER + 1

HINSTANCE hInst; 
char AppName[] = "BPQRemotePTT";
char Title[80] = "BPQRemotePTT";

TCHAR szTitle[]="GPSMuxPC";						// The title bar text
TCHAR szWindowClass[]="GPSMAINWINDOW";			// the main window class name


// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
VOID WINAPI txCompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
VOID CATThread();
VOID runTimer();

int TimerHandle = 0;

LOGFONT LFTTYFONT ;

HFONT hFont;

struct sockaddr_in sinx; 
struct sockaddr rx;

int addrlen = sizeof(struct sockaddr_in);

int HAMLIBPORT;		// Port Number for HAMLIB (rigctld) Emulator
char * HAMLIBHOST[128];

BOOL MinimizetoTray=FALSE;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	OutputDebugString(Mess);

	return;
}


char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

char * RigPort = NULL;
char * RigSpeed = NULL;
HANDLE RigHandle = 0;
int RigType = 0;			// Flag for possible RTS/DTR


char BPQHostIP[128];

char PTTCATPort[4][16];
HANDLE PTTCATHandle[4];
short HamLibPort[4];
SOCKET HamLibSock[4];				// rigctld socket

HWND comWnd[4];
HWND portWnd[4];
HWND stateWnd[4];
int stateCtl[4];

struct sockaddr_in remoteDest[4];

int RealMux[4];		// BPQ Virtual or Real

int EndPTTCATThread = 0;
	
char ConfigName[256] = "BPQRemotePTT.cfg";

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
	return FALSE;
}

int GetIntValue(config_setting_t * group, char * name)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return 0;
}

VOID SaveStringValue(config_setting_t * group, char * name, char * value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, value);

}


VOID SaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}
VOID GetConfig()
{
	config_setting_t *group;
	config_t cfg;

	memset((void *)&cfg, 0, sizeof(config_t));

	config_init(&cfg);

	if(!config_read_file(&cfg, ConfigName))
	{
		fprintf(stderr, "Config read error line %d - %s\n", config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return;
	}

	group = config_lookup(&cfg, "main");

	if (group == NULL)
	{
		config_destroy(&cfg);
		return;
	}


	GetStringValue(group, "BPQHostIP", BPQHostIP);
	
	GetStringValue(group, "COM1", PTTCATPort[0]);
	GetStringValue(group, "COM2", PTTCATPort[1]);
	GetStringValue(group, "COM3", PTTCATPort[2]);
	GetStringValue(group, "COM4", PTTCATPort[3]);

	HamLibPort[0] = GetIntValue(group, "HamLibPort1");
	HamLibPort[1] = GetIntValue(group, "HamLibPort2");
	HamLibPort[2] = GetIntValue(group, "HamLibPort3");
	HamLibPort[3] = GetIntValue(group, "HamLibPort4");

	config_destroy(&cfg);
}

VOID SaveConfig()
{
	config_setting_t *root, *group;
	config_t cfg;

	//	Get rid of old config before saving
	
	config_init(&cfg);

	root = config_root_setting(&cfg);

	group = config_setting_add(root, "main", CONFIG_TYPE_GROUP);

	SaveStringValue(group, "BPQHostIP", BPQHostIP);
	
	SaveStringValue(group, "COM1", PTTCATPort[0]);
	SaveStringValue(group, "COM2", PTTCATPort[1]);
	SaveStringValue(group, "COM3", PTTCATPort[2]);
	SaveStringValue(group, "COM4", PTTCATPort[3]);

	SaveIntValue(group, "HamLibPort1", HamLibPort[0]);
	SaveIntValue(group, "HamLibPort2", HamLibPort[1]);
	SaveIntValue(group, "HamLibPort3", HamLibPort[2]);
	SaveIntValue(group, "HamLibPort4", HamLibPort[3]);

	if(!config_write_file(&cfg, ConfigName))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}

	config_destroy(&cfg);
}




int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	if (lpCmdLine[0])
	{
		// Port Name and Speed for Remote CAT

		RigPort = _strdup(lpCmdLine);
		RigSpeed = strlop(RigPort, ':');
	}

	MyRegisterClass(hInstance);

	GetConfig();

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL, TimerHandle);

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

#define BGCOLOUR RGB(236,233,216)
HBRUSH bgBrush;
HBRUSH RedBrush;
HBRUSH GreenBrush;

HWND hWnd;

BOOL InitApplication(HINSTANCE hInstance)
{
	return TRUE;
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window 
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HFONT FAR PASCAL MyCreateFont( void ) 
{ 
    CHOOSEFONT cf; 
    LOGFONT lf; 
    HFONT hfont; 
 
    // Initialize members of the CHOOSEFONT structure.  
 
    cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner = (HWND)NULL; 
    cf.hDC = (HDC)NULL; 
    cf.lpLogFont = &lf; 
    cf.iPointSize = 0; 
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY; 
    cf.rgbColors = RGB(0,0,0); 
    cf.lCustData = 0L; 
    cf.lpfnHook = (LPCFHOOKPROC)NULL; 
    cf.lpTemplateName = (LPSTR)NULL; 
    cf.hInstance = (HINSTANCE) NULL; 
    cf.lpszStyle = (LPSTR)NULL; 
    cf.nFontType = SCREEN_FONTTYPE; 
    cf.nSizeMin = 0; 
    cf.nSizeMax = 0; 
 
    // Display the CHOOSEFONT common-dialog box.  
 
    ChooseFont(&cf); 
 
    // Create a logical font based on the user's  
    // selection and return a handle identifying  
    // that font.  
 
    hfont = CreateFontIndirect(cf.lpLogFont); 
    return (hfont); 
}

VOID Rig_PTT(int n, BOOL PTTState)
{

	char Msg[16];
	int Len = sprintf(Msg, "T %d\n", PTTState);

	if (HamLibSock[n])
	{
		send(HamLibSock[n], Msg, Len, 0);

		if (PTTState)
			SetDlgItemText(hWnd, stateCtl[n], "PTT");
		else
			SetDlgItemText(hWnd, stateCtl[n], "");
	}
}



VOID PTTCATThread()
{
	DWORD dwLength = 0;
	int Length, ret, i;
	UCHAR * ptr1;
	UCHAR * ptr2;
	UCHAR c;
	UCHAR Block[4][80];
	UCHAR CurrentState[4] = {0};
#define RTS 2
#define DTR 4
	HANDLE Event;
	DWORD EvtMask[4];
	OVERLAPPED Overlapped[4];
	char Port[32];
	int PIndex = 0;
	int HIndex = 0;
	int rc;

	EndPTTCATThread = FALSE;

	while (PIndex < 4 && PTTCATPort[PIndex][0])
	{
		RealMux[HIndex] = 0;

		sprintf(Port, "\\\\.\\pipe\\BPQ%s", PTTCATPort[PIndex]);

		PTTCATHandle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
                  0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

		if (PTTCATHandle[HIndex] == INVALID_HANDLE_VALUE)
		{
			int Err = GetLastError();
//			Consoleprintf("PTTMUX port BPQCOM%s Open failed code %d", RIG->PTTCATPort[PIndex], Err);

			// See if real com port

			sprintf(Port, "\\\\.\\\\%s", PTTCATPort[PIndex]);

			PTTCATHandle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

			RealMux[HIndex] = 1;

			if (PTTCATHandle[HIndex] == INVALID_HANDLE_VALUE)
			{
				int Err = GetLastError();
				PTTCATHandle[HIndex] = 0;
				Debugprintf("PTTMUX port COM%s Open failed code %d", PTTCATPort[PIndex], Err);
			}
			else
			{
				rc = SetCommMask(PTTCATHandle[HIndex], EV_CTS | EV_DSR);		// Request notifications
				HIndex++;
			}
		}
		else
			HIndex++;

		PIndex++;

	}

	if (PIndex == 0)
		return;				// No ports

	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (i = 0; i < HIndex; i ++)
	{
		memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
		Overlapped[i].hEvent = Event;

		if (RealMux[i])
		{
			// Request Interface change notifications

			rc = WaitCommEvent(PTTCATHandle[i], &EvtMask[i], &Overlapped[i]);
			rc = GetLastError();
 
		}
		else
		{
			// Prime a read on each PTTCATHandle

			ReadFile(PTTCATHandle[i], Block[i], 80, &Length, &Overlapped[i]);
		}
	}
		
	while (EndPTTCATThread == FALSE)
	{

WaitAgain:

		ret = WaitForSingleObject(Event, 1000);

		if (ret == WAIT_TIMEOUT)
		{
			if (EndPTTCATThread)
			{
				for (i = 0; i < HIndex; i ++)
				{
					CancelIo(PTTCATHandle[i]);
					CloseHandle(PTTCATHandle[i]);
					PTTCATHandle[i] = INVALID_HANDLE_VALUE;
				}
				CloseHandle(Event);
				return;
			}
			goto WaitAgain;
		}

		ResetEvent(Event);

		// See which request(s) have completed

		for (i = 0; i < HIndex; i ++)
		{
			ret =  GetOverlappedResult(PTTCATHandle[i], &Overlapped[i], &Length, FALSE);

			if (ret)
			{
				if (RealMux[i])
				{
					// Request Interface change notifications

					DWORD Mask;

					GetCommModemStatus(PTTCATHandle[i], &Mask);

					if (Mask & MS_CTS_ON)
						Rig_PTT(i, TRUE);
					else
						Rig_PTT(i, FALSE);

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;
					WaitCommEvent(PTTCATHandle[i], &EvtMask[i], &Overlapped[i]);

				}
				else
				{

					ptr1 = Block[i];
					ptr2 = Block[i];

					while (Length > 0)
					{
						c = *(ptr1++);

						Length--;

						if (c == 0xff)
						{
							c = *(ptr1++);
							Length--;

							if (c == 0xff)			// ff ff means ff
							{
								Length--;
							}
							else
							{
								// This is connection / RTS/DTR statua from other end
								// Convert to CAT Command

								if (c == CurrentState[i])
									continue;

								if (c & RTS)
									Rig_PTT(i, TRUE);
								else
									Rig_PTT(i, FALSE);

								CurrentState[i] = c;
								continue;
							}
						}
					}

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;

					ReadFile(PTTCATHandle[i], Block[i], 80, &Length, &Overlapped[i]);
				}
			}
		}
	}
	EndPTTCATThread = FALSE;
}

char ClassName[]="BPQMAIL";

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS  wc;

	bgBrush = CreateSolidBrush(BGCOLOUR);
	RedBrush = CreateSolidBrush(RGB(255,0,0));
	GreenBrush = CreateSolidBrush(RGB(0,255,0));
//	BlueBrush = CreateSolidBrush(RGB(0,0,255));

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = NULL; //LoadIcon( hInstance, MAKEINTRESOURCE(IDI_GPSMUXPC));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;//"MENU_1";	
	wc.lpszClassName = szWindowClass; 

//	RegisterClass(&wc);

 //   wc.lpfnWndProc = TraceWndProc;       
 // 	wc.lpszClassName = TraceClassName;

//	RegisterClass(&wc);

//    wc.lpfnWndProc = ConfigWndProc;       
//  	wc.lpszClassName = ConfigClassName;

	return (RegisterClass(&wc));

}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	u_long param=1;
	BOOL bcopt=TRUE;
	int ret;
	WNDCLASS  wc = {0};
	int n = 0;


	hInst = hInstance; // Store instance handle in our global variable

	WSAStartup(MAKEWORD(2, 0), &WsaData);


   hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

  if (!hWnd)
   {
      ret=GetLastError();
	  return FALSE;
   }
		
	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        OEM_CHARSET ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
   lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
//	hFont = MyCreateFont();

	SetWindowText(hWnd,Title);

	comWnd[0] = GetDlgItem(hWnd, IDC_COM1);
	comWnd[1] = GetDlgItem(hWnd, IDC_COM2);
	comWnd[2] = GetDlgItem(hWnd, IDC_COM3);
	comWnd[3] = GetDlgItem(hWnd, IDC_COM4);

	portWnd[0] = GetDlgItem(hWnd, IDC_HAMLIBPORT1);
	portWnd[1] = GetDlgItem(hWnd, IDC_HAMLIBPORT2);
	portWnd[2] = GetDlgItem(hWnd, IDC_HAMLIBPORT3);
	portWnd[3] = GetDlgItem(hWnd, IDC_HAMLIBPORT4);

	stateWnd[0] = GetDlgItem(hWnd, IDC_STATE1);
	stateWnd[1] = GetDlgItem(hWnd, IDC_STATE2);
	stateWnd[2] = GetDlgItem(hWnd, IDC_STATE3);
	stateWnd[3] = GetDlgItem(hWnd, IDC_STATE4);


	stateCtl[0] = IDC_STATE1;
	stateCtl[1] = IDC_STATE2;
	stateCtl[2] = IDC_STATE3;
	stateCtl[3] = IDC_STATE4;

	ShowWindow(hWnd, nCmdShow);

	SetDlgItemText(hWnd, IDC_BPQHOST, BPQHostIP);	

	SetDlgItemText(hWnd, IDC_COM1, PTTCATPort[0]);			
	SetDlgItemText(hWnd, IDC_COM2, PTTCATPort[1]);			
	SetDlgItemText(hWnd, IDC_COM3, PTTCATPort[2]);			
	SetDlgItemText(hWnd, IDC_COM4, PTTCATPort[3]);	

	SetDlgItemInt(hWnd, IDC_HAMLIBPORT1, HamLibPort[0], 0);
	SetDlgItemInt(hWnd, IDC_HAMLIBPORT2, HamLibPort[1], 0);
	SetDlgItemInt(hWnd, IDC_HAMLIBPORT3, HamLibPort[2], 0);
	SetDlgItemInt(hWnd, IDC_HAMLIBPORT4, HamLibPort[3], 0);

//	ioctlsocket (sock, FIONBIO, &param);
 
	if (RigPort)
	{
		int Speed = 9600;

		if (RigSpeed)
			Speed = atoi(RigSpeed);
	}

	TimerHandle = SetTimer(hWnd, 1, 10000, NULL);

	runTimer();					// Open Hamlib connections

	_beginthread(PTTCATThread, 0);

	return TRUE;
}

VOID ConnecttoHAMLIB(int n);

VOID runTimer()
{
	int n;

	for (n = 0; n < 4; n++)
	{
		if (HamLibPort[n] && HamLibSock[n] == 0)
		{
			// try to connect

			ConnecttoHAMLIB(n);

		}
	}
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{ 
	case WM_TIMER:

		runTimer();
		break;

	case WM_CTLCOLOREDIT:
	{
		HDC hdc = (HDC) wParam;   // handle to display context 
		HWND hwnd = (HWND) lParam; // handle to control window
		int n;

		for (n = 0; n < 4; n++)
		{
			if (hwnd == comWnd[n] && PTTCATPort[n][0])
				if (PTTCATHandle[n])
					return (INT_PTR)GreenBrush;
				else
					return (INT_PTR)RedBrush;
				
			if (hwnd == portWnd[n] && HamLibPort[n])
				if (HamLibSock[n])
					return (INT_PTR)GreenBrush;
				else
					return (INT_PTR)RedBrush;
		}

		//return (INT_PTR)RedBrush;

		return (DefWindowProc(hWnd, message, wParam, lParam));
	}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case  SC_MINIMIZE: 

			if (MinimizetoTray)

				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));


			break;


		default:

			return (DefWindowProc(hWnd, message, wParam, lParam));
		}

	case WM_CLOSE:

		PostQuitMessage(0);
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
			int OK;

		case IDC_TEST1:

			Rig_PTT(0, 1);
			Sleep(1500);
			Rig_PTT(0, 0);
			break;

		case IDC_TEST2:

			Rig_PTT(1, 1);
			Sleep(1500);
			Rig_PTT(1, 0);
			break;

		case IDC_TEST3:

			Rig_PTT(2, 1);
			Sleep(1500);
			Rig_PTT(2, 0);
			break;

		case IDC_TEST4:

			Rig_PTT(3, 1);
			Sleep(1500);
			Rig_PTT(3, 0);
			break;

		case IDOK:

			GetDlgItemText(hWnd, IDC_BPQHOST, BPQHostIP, 127);	

			GetDlgItemText(hWnd, IDC_COM1, PTTCATPort[0], 15);			
			GetDlgItemText(hWnd, IDC_COM2, PTTCATPort[1], 15);			
			GetDlgItemText(hWnd, IDC_COM3, PTTCATPort[2], 15);			
			GetDlgItemText(hWnd, IDC_COM4, PTTCATPort[3], 15);	

			HamLibPort[0] = GetDlgItemInt(hWnd, IDC_HAMLIBPORT1, &OK, 0);
			HamLibPort[1] = GetDlgItemInt(hWnd, IDC_HAMLIBPORT2, &OK, 0);
			HamLibPort[2] = GetDlgItemInt(hWnd, IDC_HAMLIBPORT3, &OK, 0);
			HamLibPort[3] = GetDlgItemInt(hWnd, IDC_HAMLIBPORT4, &OK, 0);

			SaveConfig();

	//		EndPTTCATThread = 1;

	//		Sleep(1100);

	//		_beginthread(PTTCATThread, 0);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
	}

	return (DefWindowProc(hWnd, message, wParam, lParam));

}

void HAMLIBProcessMessage(int n)
{
	char RXBuffer[256];

	int InputLen = recv(HamLibSock[n], RXBuffer, 256, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		if (HamLibSock[n])
			closesocket(HamLibSock[n]);
		
		HamLibSock[n] = 0;
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

		return;					
	}
}


VOID HAMLIBThread(int n);

VOID ConnecttoHAMLIB(int n)
{
	_beginthread(HAMLIBThread, 0, n);
	return ;
}

VOID HAMLIBThread(int n)
{
	// Opens sockets and looks for data
	
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;

	if (HamLibSock[n])
		closesocket(HamLibSock[n]);

		// Param is IPADDR:PORT. Only Allow numeric addresses 
	
	if (HamLibPort[n] == 0)
		return;

	remoteDest[n].sin_family = AF_INET;
	remoteDest[n].sin_addr.s_addr = inet_addr(BPQHostIP);
	remoteDest[n].sin_port = htons(HamLibPort[n]);
	
	HamLibSock[n] = 0;
	HamLibSock[n] = socket(AF_INET,SOCK_STREAM,0);

	if (HamLibSock[n] == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for HAMLIB socket - error code = %d\r\n", WSAGetLastError());
		Debugprintf(Msg);

  	 	return; 
	}

	setsockopt(HamLibSock[n], SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(HamLibSock[n], IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	if (connect(HamLibSock[n],(LPSOCKADDR)&remoteDest[n],sizeof(remoteDest[n])) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		err=WSAGetLastError();
   		i=sprintf(Msg, "Connect Failed for HAMLIB socket %d - error code = %d\r\n", n, err);
		Debugprintf(Msg);
		
		closesocket(HamLibSock[n]);

		HamLibSock[n] = 0;

		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

		return;
	}

	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

	ret = GetLastError();

	while (HamLibSock[n])
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(HamLibSock[n],&readfs);
		FD_SET(HamLibSock[n],&errorfs);
		
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;
		
		ret = select((int)HamLibSock[n] + 1, &readfs, NULL, &errorfs, &timeout);

		if (ret == SOCKET_ERROR)
		{
			Debugprintf("HAMLIB Select failed %d ", WSAGetLastError());
			goto Lost;
		}

		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(HamLibSock[n], &readfs))
			{
				HAMLIBProcessMessage(n);
			}

			if (FD_ISSET(HamLibSock[n], &errorfs))
			{
Lost:	
				sprintf(Msg, "HAMLIB Connection lost for Port %d\r\n", n);
				Debugprintf(Msg);

				closesocket(HamLibSock[n]);
				HamLibSock[n] = 0;
				RedrawWindow(hWnd, NULL, NULL, 0);

				return;
			}
			continue;
		}
		else
		{
		}
	}
	sprintf(Msg, "HAMLIB Thread Terminated Port %d\r\n", n);
	Debugprintf(Msg);
}
