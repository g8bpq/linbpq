
// Version 1. 0. 2. 1 October 2010

//	Add Delay on start option, and dynamically load bpq32

// Version 1. 0. 3. 1 October 2011

//		Call CloseBPQ32 on exit

// Version 2.0.1.1 July 2002

//		Add try/except round main loop

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <time.h>
#include "DbgHelp.h"

//#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#include "..\Include\bpq32.h"

VOID APIENTRY SetFrameWnd(HWND hWnd);

#define BPQICON 400

HINSTANCE hInst; 
char AppName[] = "BPQ32";
char Title[80] = "Program to hold BPQ32.dll in memory";


// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HWND FrameWnd;
int TimerHandle = 0;


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[8192];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	strcat_s(Mess, 999, "\r\n");
	OutputDebugString(Mess);
	return;
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
	BOOL bRet;
	struct _EXCEPTION_POINTERS exinfo;

	Debugprintf("BPQ32.exe %s Entered", lpCmdLine);

	if (_stricmp(lpCmdLine, "Wait") == 0)				// If AutoRestart then Delay 5 Secs				
		Sleep(5000);

//	GetAPI();

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:

	__try 
	{

	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
		    Debugprintf("GetMessage Returned -1 %d", GetLastError());
			break;
		}
	    else
		{
			TranslateMessage(&msg); 
	        DispatchMessage(&msg); 
	    }
	}

	}
	
	#define EXCEPTMSG "BPQ32.exe Main Loop"
	#include "StdExcept.c"
	}

	Debugprintf("BPQ32.exe exiting %d", msg.message);

	KillTimer(NULL,TimerHandle);

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (msg.wParam);
}

//


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


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wndclassMainFrame;

	hInst = hInstance; // Store instance handle in our global variable

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
	wndclassMainFrame.lpszClassName	= AppName;
	wndclassMainFrame.hIconSm		= NULL;
	

	if(!RegisterClassEx(&wndclassMainFrame))
	{
		Debugprintf("BPQ32.exe RC failed %d", GetLastError());
		return 0;
	}

	FrameWnd = CreateWindow(AppName, 
								"BPQ32", 
								WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
								CW_USEDEFAULT,	// allows system choose an x position
								CW_USEDEFAULT,	// allows system choose a y position
								CW_USEDEFAULT,	// width, CW_USEDEFAULT allows system to choose height and width
								CW_USEDEFAULT,	// height, CW_USEDEFAULT ignores heights as this is set by setting
												// CW_USEDEFAULT in width above.
								HWND_MESSAGE,	// Message only Window
								NULL, // handle to menu
								hInstance,	// handle to the instance of module
								NULL);		// Long pointer to a value to be passed to the window through the 
											// CREATESTRUCT structure passed in the lParam parameter the WM_CREATE message

	

	TimerHandle=SetTimer(FrameWnd,WM_TIMER,5000,NULL);

	CheckTimer();

	return (TRUE);

}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//


LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) { 

		case WM_TIMER:

			CheckTimer();
			return 0;

		case WM_CLOSE:
		
			PostQuitMessage(0);
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}


