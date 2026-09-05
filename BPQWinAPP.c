
// Version 1. 0. 2. 1 October 2010

//	Add Delay on start option, and dynamically load bpq32

// Version 1. 0. 3. 1 October 2011

//		Call CloseBPQ32 on exit

// Version 2.0.1.1 July 2002

//		Add try/except round main loop

// Version 2.0.1.2 July 2026

//		Call RunBPQ32Background() every 10mS


#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <time.h>
#include "DbgHelp.h"
#include "winstdint.h"

//#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#include "bpq32.h"

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

SYMBOL_INFO  * symbol;
HANDLE         process;

void printStack(void)
{
#ifdef WIN32
//#ifdef _DEBUG					// So we can use on 98/2K

    unsigned int   i;
    void         * stack[ 100 ];
    unsigned short frames;

	DWORD  dwDisplacement;
	IMAGEHLP_LINE64 line;
	char * fptr;
	Debugprintf("Stack Backtrace");

     process = GetCurrentProcess();

	 SymSetOptions(SYMOPT_LOAD_LINES);
     SymInitialize( process, NULL, TRUE );

     frames = RtlCaptureStackBackTrace(0, 60, stack, NULL );

     for( i = 0; i < frames; i++ )
     {
		 SymGetLineFromAddr64(process, (DWORD64)stack[i], &dwDisplacement, &line);
         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

		fptr = line.FileName + (int)strlen(line.FileName);	// remove path
		while (*fptr != '\\' && *fptr != '/')
			fptr--;
		
		fptr++;


		Debugprintf( "%i: %s - %s Line %d Addr %p", frames - i - 1, symbol->Name, fptr, line.LineNumber, symbol->Address );
	}

	free(symbol);

//#endif
#endif
}

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep)
{
    Debugprintf("in filter.");
	   printStack();
    if (code == EXCEPTION_ACCESS_VIOLATION)
    {
        Debugprintf("caught AV as expected.");
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        Debugprintf("didn't catch AV, unexpected.");
        return EXCEPTION_CONTINUE_SEARCH;
    }
}



LONG WINAPI UnhandledExcepFilter(PEXCEPTION_POINTERS pExcepPointers)
{
	DWORD  dwDisplacement;
	IMAGEHLP_LINE64 line;
	char * fptr;

	if(pExcepPointers->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C)
		 return EXCEPTION_CONTINUE_EXECUTION;

	SymFromAddr( process, ((DWORD)pExcepPointers->ExceptionRecord->ExceptionAddress), 0, symbol );

	Debugprintf("Program error trapped\r\n");

	SymGetLineFromAddr64(process, (DWORD)pExcepPointers->ExceptionRecord->ExceptionAddress, &dwDisplacement, &line);
	fptr = line.FileName + (int)strlen(line.FileName);	// remove path
	while (*fptr != '\\' && *fptr != '/')
		fptr--;
		
	fptr++;

	Debugprintf("In Procedure %s - %s Line %d Addr %p\r\n", symbol->Name, fptr, line.LineNumber, symbol->Address );
	printStack();

	MessageBox(NULL,"Program Error - program closing. See Debug Log for details","BPQ32",MB_ICONSTOP);
	RealCloseAllPrograms();
	Sleep(1000);
	CloseDebugLog();
    exit(0);
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
	int Running = 1;

	Debugprintf("BPQ32.exe %s Entered", lpCmdLine);

	 process = GetCurrentProcess();

	 SymSetOptions(SYMOPT_LOAD_LINES);
     SymInitialize( process, NULL, TRUE );

	 symbol = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
     symbol->MaxNameLen = 255;
     symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

	AddVectoredExceptionHandler(1, UnhandledExcepFilter);

	if (_stricmp(lpCmdLine, "Wait") == 0)				// If AutoRestart then Delay 5 Secs				
		Sleep(5000);

//	GetAPI();

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	RunBPQ32Background();

	// Main message loop:

//	__try 
	{
		while(Running)
		{
			Sleep(10);

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg); 
			    DispatchMessage(&msg); 

				if (msg.message == WM_QUIT)
					Running = 0;
			}

			RunBPQ32Background();
		}
	}
	
//	#define EXCEPTMSG "BPQ32.exe Main Loop"
//	#include "StdExcept.c"
//	}

	Debugprintf("BPQ32.exe exiting %d", msg.message);

	KillTimer(NULL,TimerHandle);

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process
	CloseDebugLog();
	
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


