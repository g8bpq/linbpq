
// Program to start or stop a Software TNC on a remote host

// Should work with ARDOP, WINMOR or VARA

// Version 1. 0. 0. 1 June 2013

// Version 1. 0. 2. 1 April 2018

// Updated to support running programs with command line parameters
// Add option to KILL by program name 

// Version 1. 0. 3. 1 April 2019

//	Add remote rigcontrol feature

// Version 1. 0. 3. 2 Feb 2020

//	Fix Rigcontol

// Version 1. 0. 3. 3 Dec 2020

//	Set working directory when starting TNC

// Version 1. 0. 3. 4 Jan 2021

//	Add trace window

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <Psapi.h>

#define BPQICON 400

WSADATA WsaData;            // receives data from WSAStartup

#define WSA_READ WM_USER + 1

HINSTANCE hInst; 
char AppName[] = "WinmorControl";
char Title[80] = "WinmorControl";

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
VOID WINAPI txCompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
VOID CATThread();

int TimerHandle = 0;

LOGFONT LFTTYFONT ;

HFONT hFont;

struct sockaddr_in sinx; 
struct sockaddr rx;
SOCKET sock;
int udpport = 8500;
int addrlen = sizeof(struct sockaddr_in);

BOOL MinimizetoTray=FALSE;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
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

	char * ptr;
	
	if (buf == NULL) return NULL;		// Protect
	
	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}


BOOL KillOldTNC(char * Path)
{
	HANDLE hProc;
	char ExeName[256] = "";
	DWORD Pid = 0;

    DWORD Processes[1024], Needed, Count;
    unsigned int i;

    if (!EnumProcesses(Processes, sizeof(Processes), &Needed))
        return FALSE;
    
    // Calculate how many process identifiers were returned.

    Count = Needed / sizeof(DWORD);

    for (i = 0; i < Count; i++)
    {
        if (Processes[i] != 0)
        {
			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Processes[i]);
	
			if (hProc)
			{
				GetModuleFileNameEx(hProc, 0,  ExeName, 255);

				if (_memicmp(ExeName, Path, strlen(ExeName)) == 0)
				{
					Debugprintf("Killing Pid %d %s", Processes[i], ExeName);
					TerminateProcess(hProc, 0);
					CloseHandle(hProc);
					return TRUE;
				}
				CloseHandle(hProc);
			}
		}
	}

	return FALSE;
}

KillTNC(int PID)
{
	HANDLE hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);

	Debugprintf("KillTNC Called Pid %d", PID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	return 0;
}

RestartTNC(char * Path)
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	int i, n = 0;
	char workingDirectory[256];

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	Debugprintf("RestartTNC Called for %s", Path);

	strcpy(workingDirectory, Path);

	i = strlen(Path);

	while (i--)
	{
		if (workingDirectory[i] == '\\' || workingDirectory[i] == '/')
		{
			workingDirectory[i] = 0; 
			break;
		}
	}


	while (KillOldTNC(Path) && n++ < 100)
	{
		Sleep(100);
	}

	if (CreateProcess(NULL, Path, NULL, NULL, FALSE,0 ,NULL ,workingDirectory, &SInfo, &PInfo))
		Debugprintf("Restart TNC OK");
	else
		Debugprintf("Restart TNC Failed %d ", GetLastError());
}
char * RigPort = NULL;
char * RigSpeed = NULL;
HANDLE RigHandle = 0;
int RigType = 0;			// Flag for possible RTS/DTR

#define RTS 1
#define DTR 2

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	if (lpCmdLine[0])
	{
		// Port Name and Speed for Remote CAT

		RigPort = _strdup(lpCmdLine);
		RigSpeed = strlop(RigPort, ':');
	}

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
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

	// Fill in window class structure with parameters that describe
    // the main window.
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

//		wc.lpszMenuName =  MAKEINTRESOURCE(BPQMENU) ;
 
        wc.lpszClassName = AppName;

        // Register the window class and return success/failure code.

		return RegisterClass(&wc);
      
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

VOID COMSetDTR(HANDLE fd)
{
	EscapeCommFunction(fd, SETDTR);
}

VOID COMClearDTR(HANDLE fd)
{
	EscapeCommFunction(fd, CLRDTR);
}

VOID COMSetRTS(HANDLE fd)
{
	EscapeCommFunction(fd, SETRTS);
}

VOID COMClearRTS(HANDLE fd)
{
	EscapeCommFunction(fd, CLRRTS);
}





OVERLAPPED txOverlapped;

HANDLE txEvent;

char RXBuffer[1024];
int RXLen;

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	BOOL	ret;

	// only try to read number of bytes in queue

	ret = ClearCommError(fd, &dwErrorFlags, &ComStat);

	if (ret == 0)
	{
		int Err = GetLastError();
		return 0;
	}


	dwLength = min((DWORD) MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL) ;

		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(fd, &dwErrorFlags, &ComStat ) ;
		}
	}

   return dwLength;
}


BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;
	int Err, txLength;

	Err = WaitForSingleObject(txEvent, 1000);

	if (Err == WAIT_TIMEOUT)
	{
		Debugprintf("TX Event Wait Timout");
	}
	else
	{
		ResetEvent(txEvent);
		Err =  GetOverlappedResult(RigHandle, &txOverlapped, &txLength, FALSE);
	}
		
	memset(&txOverlapped, 0, sizeof(OVERLAPPED));
	txOverlapped.hEvent = txEvent;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
	               &BytesWritten, &txOverlapped);

	if (!fWriteStat)
	{
		Err = GetLastError();

		if (Err != ERROR_IO_PENDING)
		{
			ClearCommError(fd, &ErrorFlags, &ComStat);
			return FALSE;
		}
	}
	return TRUE;
}


HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	sprintf( szPort, "\\\\.\\%s", pPort);
	
	// open COMM device

	fd = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                  NULL );


	if (fd == (HANDLE) -1)
	{
		char Msg[80];
		
		sprintf(Msg, "%s could not be opened - Error %d", pPort, GetLastError());
		MessageBox(NULL, Msg, "WinmorControl", MB_OK);

		return FALSE;
	}

	// setup device buffers

	SetupComm(fd, 4096, 4096);

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 20;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500 ;

	SetCommTimeouts(fd, &CommTimeOuts);

   dcb.DCBlength = sizeof(DCB);

   GetCommState(fd, &dcb);

   dcb.BaudRate = speed;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = TWOSTOPBITS;
   dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE ;

	// setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

	fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		if (SetDTR)
			EscapeCommFunction(fd, SETDTR);
		else
			EscapeCommFunction(fd, CLRDTR);
	
		if (SetRTS)
			EscapeCommFunction(fd, SETRTS);
		else
			EscapeCommFunction(fd, CLRRTS);
	}
	else
	{
		sprintf(buf,"%s Setup Failed %d ", pPort, GetLastError());
		OutputDebugString(buf);
		CloseHandle(fd);
		return 0;
	}

	txEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Start Read Thread

	_beginthread(CATThread, 0, 0);

	return fd;
}



VOID CloseCOMPort(HANDLE fd)
{
	SetCommMask(fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	CloseHandle(fd);
}

HWND hMonitor;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Msg[255];
	int ret, err;


	hInst = hInstance; // Store instance handle in our global variable

	WSAStartup(MAKEWORD(2, 0), &WsaData);

	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 420,275,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
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

	
	hMonitor = CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			2,2,400,230, hWnd, NULL, hInstance, NULL);



	ShowWindow(hWnd, nCmdShow);

	sock = socket(AF_INET,SOCK_DGRAM,0);

	if (sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		sprintf(Msg, "Failed to create UDP socket - error code = %d", err);
		MessageBox(NULL, Msg, "WinmorControl", MB_OK);
		return FALSE; 
	}
	
	if (WSAAsyncSelect(sock, hWnd, WSA_READ, FD_READ) > 0)
	{
		wsprintf(Msg, TEXT("WSAAsyncSelect failed Error %d"), WSAGetLastError());

		MessageBox(hWnd, Msg, TEXT("WinmorControl"), MB_OK);

		closesocket(sock);
		
		return FALSE;

	}

//	ioctlsocket (sock, FIONBIO, &param);
 
	setsockopt (sock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	
	sinx.sin_port = htons(udpport);

	ret = bind(sock, (struct sockaddr *) &sinx, sizeof(sinx));

	if (ret != 0)
	{
		//	Bind Failed

		err = WSAGetLastError();
		sprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
		MessageBox(NULL, Msg, "WinmorControl", MB_OK);

		return FALSE;
	}

	if (RigPort)
	{
		int Speed = 9600;

		if (RigSpeed)
			Speed = atoi(RigSpeed);

		RigHandle = OpenCOMPort(RigPort, Speed, FALSE, FALSE, FALSE, TWOSTOPBITS);

		if (RigHandle)
		{
			if (RigType == RTS)
				COMClearRTS(RigHandle);
			else
				COMSetRTS(RigHandle);

			if (RigType == DTR)
				COMClearDTR(RigHandle);
			else
				COMSetDTR(RigHandle);
		}
		else
			return FALSE;			// Open Failed
	}

	TimerHandle = SetTimer(hWnd, 1, 100, NULL);

	return TRUE;
}
	
void Trace(char * Msg)
{	
	int index = SendMessage(hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Msg);

	if (index > 1200)					
	do
		index=index=SendMessage(hMonitor, LB_DELETESTRING, 0, 0);
	while (index > 1000);

	if (index > -1)
		index=SendMessage(hMonitor, LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	char Msg[256];
	int len;

	switch (message) 
	{ 
		case WSA_READ: // Notification on data socket
	
			len = recvfrom(sock, Msg, 256, 0, &rx, &addrlen);

			if (len <= 0)
				return 0;

			Msg[len] = 0;
		
			
			if (_memicmp(Msg, "REMOTE:", 7) == 0)
			{
				Trace(Msg);
				RestartTNC(&Msg[7]);
			}
			else
			if (_memicmp(Msg, "KILL ", 5) == 0)
			{
				Trace(Msg);
				KillTNC(atoi(&Msg[5]));
			}
			else 
			if (_memicmp(Msg, "KILLBYNAME ", 11) == 0)
			{
				Trace(Msg);
				KillOldTNC(&Msg[11]);
			}
			else
			{
				// Anything else is Rig Control
	
				len = WriteCOMBlock(RigHandle, Msg, len);
			}

			break;

		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

	
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


		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}

BOOL EndCATThread = FALSE;

VOID CATThread()
{
	DWORD dwLength = 0;
	int Length, ret;
	HANDLE Event;
	OVERLAPPED Overlapped;
	
	EndCATThread = FALSE;

	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	memset(&Overlapped, 0, sizeof(OVERLAPPED));
	Overlapped.hEvent = Event;

	ReadFile(RigHandle, RXBuffer, 1024, &RXLen, &Overlapped);
		
	while (EndCATThread == FALSE)
	{
		ret = WaitForSingleObject(Event, 10000);

		if (ret == WAIT_TIMEOUT)
		{
			if (EndCATThread)
			{
				CancelIo(RigHandle);
				CloseHandle(RigHandle);
				RigHandle = INVALID_HANDLE_VALUE;	
				CloseHandle(Event);
				EndCATThread = FALSE;
				return;
			}
			continue;	
		}
		ResetEvent(Event);

		ret =  GetOverlappedResult(RigHandle, &Overlapped, &Length, FALSE);

		if (ret && Length)
		{
			// got something so send to BPQ

			sendto(sock, RXBuffer, Length,  0, &rx, sizeof(struct sockaddr));
		}
		
		memset(&Overlapped, 0, sizeof(OVERLAPPED));
		Overlapped.hEvent = Event;

		ReadFile(RigHandle, RXBuffer, 1024, &RXLen, &Overlapped);
	}
}

