// Mail and Chat Server for BPQ32 Packet Switch
//
//	Debug Window(s) Module

#include "BPQChat.h"

static char ClassName[]="BPQDEBUGWINDOW";


static WNDPROC wpOrigInputProc; 
static WNDPROC wpOrigOutputProc; 

HWND hDebug;
static HWND hwndInput;
static HWND hwndOutput;

static HMENU hMenu;		// handle of menu 

#define InputBoxHeight 25

RECT DebugRect;


int Height, Width, LastY;

static char readbuff[1024];

static BOOL Bells = TRUE;
static BOOL StripLF = TRUE;
static BOOL MonBBS = TRUE;
static BOOL MonCHAT = TRUE;
static BOOL MonTCP = TRUE;

static int PartLinePtr=0;
static int PartLineIndex=0;		// Listbox index of (last) incomplete line


static LRESULT CALLBACK MonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static void MoveWindows();

#define BGCOLOUR RGB(236,233,216)

extern char DebugSize[32];

BOOL CreateDebugWindow()
{
    WNDCLASS  wc;
	HBRUSH bgBrush;
	char Text[80];

	if (hDebug)
	{
		ShowWindow(hDebug, SW_SHOWNORMAL);
		SetForegroundWindow(hDebug);
		return FALSE;							// Alreaqy open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = MonWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hDebug=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hDebug)
        return (FALSE);

	wsprintf(Text, "Chat %s Debug", Session);
	SetWindowText(hDebug, Text);

	hMenu=GetMenu(hDebug);

	if (Bells & 1)
		CheckMenuItem(hMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQBELLS, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hMenu,MONBBS, MonBBS ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,MONCHAT, MonCHAT ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,MONTCP, MonTCP ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	// Retrieve the handlse to the edit controls. 

	hwndOutput = GetDlgItem(hDebug, 122); 
 
	// Set our own WndProcs for the controls. 

	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);

	if (cfgMinToTray)
	{
		AddTrayMenuItem(hDebug, Text);
	}
	ShowWindow(hDebug, SW_SHOWNORMAL);

	if (DebugRect.right < 100 || DebugRect.bottom < 100)
	{
		GetWindowRect(hDebug,	&DebugRect);
	}

	MoveWindow(hDebug,DebugRect.left,DebugRect.top, DebugRect.right-DebugRect.left, DebugRect.bottom-DebugRect.top, TRUE);

	MoveWindows();

	return TRUE;

}


static void MoveWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	GetWindowRect(hDebug, &rcMain);
	GetClientRect(hDebug, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

//	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, 2, ClientWidth-4, ClientHeight-4, TRUE);
//	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);
}


static LRESULT CALLBACK MonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	
	switch (message) { 

		case WM_ACTIVATE:

			SetFocus(hwndInput);
			break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

		case MONBBS:

			ToggleParam(hMenu, hWnd, &MonBBS, MONBBS);
			break;

		case MONCHAT:

			ToggleParam(hMenu, hWnd, &MonCHAT, MONCHAT);
			break;

		case MONTCP:

			ToggleParam(hMenu, hWnd, &MonTCP, MONTCP);
			break;


		case BPQCLEAROUT:

			SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(hwndOutput);
			break;



		//case BPQHELP:

		//	HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
		//	break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

			MoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&DebugRect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			hDebug = NULL;
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}



LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 

	return CallWindowProc(wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

VOID ClearDebugWindow()
{
	SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
}

VOID WritetoDebugWindow(char * Msg, int len)
{
	char * ptr1, * ptr2;
	int index;

	if (len ==0)
		return;


	if (PartLinePtr != 0)
		SendMessage(hwndOutput,LB_DELETESTRING,PartLineIndex,(LPARAM)(LPCTSTR) 0 );		

	memcpy(&readbuff[PartLinePtr], Msg, len);
		
	len=len+PartLinePtr;

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

	if (PartLinePtr > 300)
		PartLinePtr = 0;

	if (len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			PartLinePtr=len;
			memmove(readbuff,ptr1,len);
			PartLineIndex=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) PartLineIndex, MAKELPARAM(FALSE, 0));

			return;

		}

		*(ptr2++)=0;

		index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
					
	//		if (LogOutput) WriteMonitorLine(ptr1, ptr2 - ptr1);

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

		if (index > 1200)
						
		do{

			index=SendMessage(hwndOutput,LB_DELETESTRING, 0, 0);
			
			} while (index > 1000);

		SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

		goto lineloop;
	}
	return;
}

/*static int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
}
*/
static  void CopyToClipboard(HWND hWnd)
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


