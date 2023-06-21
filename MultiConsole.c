 // Mail and Chat Server for BPQ32 Packet Switch
//
//	Console Window Module

#include "bpqmail.h"

extern BOOL WINE;

char ClassName[]="CONSOLEWINDOW";

char SYSOPCall[50];

struct UserInfo * user;

struct ConsoleInfo BBSConsole;
struct ConsoleInfo ChatConsole;
struct ConsoleInfo * ConsHeader[2] = {&BBSConsole, &ChatConsole};

struct ConsoleInfo * InitHeader;

extern struct SEM ChatSemaphore;
extern int	NumberofStreams;
extern ConnectionInfo Connections[];

BOOL Bells;
BOOL FlashOnBell;		// Flash instead of Beep
BOOL StripLF;

BOOL WarnWrap;
BOOL FlashOnConnect;
BOOL WrapInput;
BOOL CloseWindowOnBye;

char RTFHeader[4000];
int RTFHddrLen = 0;

RECT ConsoleRect;
	
char chatMsg[] = "\rSysop wants to chat to you\r";
char endChatMsg[] = "Sysop ended chat\r";

//CIRCUIT * Console;
HWND hConsole;
//RECT ConsoleRect;

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(64,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 17 
		RGB(64,64,0), RGB(64,64,128), RGB(64,64,192), RGB(64,64,255),
		RGB(64,128,0), RGB(64,128,128), RGB(64,128,192), RGB(64,128,255),
		RGB(64,192,0), RGB(64,192,128), RGB(64,192,192), RGB(64,192,255),
		RGB(64,255,0), RGB(64,255,128), RGB(64,255,192), RGB(64,255,255),

		RGB(128,0,0), RGB(128,0,128), RGB(128,0,192), RGB(128,0,255),				// 33
		RGB(128,64,0), RGB(128,64,128), RGB(128,64,192), RGB(128,64,255),
		RGB(128,128,0), RGB(128,128,128), RGB(128,128,192), RGB(128,128,255),
		RGB(128,192,0), RGB(128,192,128), RGB(128,192,192), RGB(128,192,255),
		RGB(128,255,0), RGB(128,255,128), RGB(128,255,192), RGB(128,255,255),

		RGB(192,0,0), RGB(192,0,128), RGB(192,0,192), RGB(192,0,255),				// 49
		RGB(192,64,0), RGB(192,64,128), RGB(192,64,192), RGB(192,64,255),
		RGB(192,128,0), RGB(192,128,128), RGB(192,128,192), RGB(192,128,255),
		RGB(192,192,0), RGB(192,192,128), RGB(192,192,192), RGB(192,192,255),
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,2552,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 49
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,2552,255)
};


#define InputBoxHeight 25
static LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
void MoveWindows(struct ConsoleInfo * Cinfo);
VOID CloseConsoleSupport(struct ConsoleInfo * Cinfo);
VOID AddLinetoWindow(struct ConsoleInfo * Cinfo, char * Line);
VOID DoRefresh(struct ConsoleInfo * Cinfo);

#define BGCOLOUR RGB(236,233,216)


HMENU trayMenu = 0, hBBSUSERCHAT = 0;

BOOL CreateConsole(int Stream)
{
	WNDCLASS  wc = {0};
	HBRUSH bgBrush;
	HMENU hMenu,hActionMenu;
	char RTFColours[3000];
	struct ConsoleInfo * Cinfo;
	int i, n;
	
	Cinfo = &BBSConsole;
	
	InitHeader = Cinfo;

	if (Cinfo->hConsole)
	{
		ShowWindow(Cinfo->hConsole, SW_SHOWNORMAL);
		SetForegroundWindow(Cinfo->hConsole);
		return FALSE;							// Already open
	}

	memset(Cinfo, 0, sizeof(struct ConsoleInfo));

	Cinfo->BPQStream = Stream;

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = ConsWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hConsole = CreateDialog(hInst,ClassName,0,NULL);
	
	if (!hConsole)
        return (FALSE);
		
	Cinfo->Bells = Bells;		
	Cinfo->FlashOnBell = FlashOnBell;
	Cinfo->StripLF = StripLF;
	Cinfo->CloseWindowOnBye = CloseWindowOnBye;
	Cinfo->WarnWrap = WarnWrap;
	Cinfo->WrapInput= WrapInput;
	Cinfo->FlashOnConnect = FlashOnConnect;
	
	Cinfo->ConsoleRect = ConsoleRect;

	Cinfo->readbuff = zalloc(1000);
	Cinfo->readbufflen = 1000;

	hMenu=GetMenu(hConsole);
	Cinfo->hMenu = hMenu;

	CheckMenuItem(hMenu,BPQBELLS, (Cinfo->Bells) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,BPQFLASHONBELL, (Cinfo->FlashOnBell) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,BPQStripLF, (Cinfo->StripLF) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WARNINPUT, (Cinfo->WarnWrap) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WRAPTEXT, (Cinfo->WrapInput) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_Flash, (Cinfo->FlashOnConnect) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_CLOSEWINDOW, (Cinfo->CloseWindowOnBye) ? MF_CHECKED : MF_UNCHECKED);


	hActionMenu=GetSubMenu(hMenu,2);
	hBBSUSERCHAT=GetSubMenu(hActionMenu,0);

	DrawMenuBar(hWnd);	

	if (trayMenu == 0)
	{
		trayMenu = CreatePopupMenu();
		AppendMenu(trayMenu,MF_STRING,40000,"Copy");
	}

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

	strcpy(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fprq1 FixedSys;}}");
//	strcpy(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset204\\fprq1 FixedSys;}}");
	strcat(RTFHeader, RTFColours);
	strcat(RTFHeader, "\\viewkind4\\uc1\\pard\\f0");

	RTFHddrLen = strlen(RTFHeader);

	// Create a Rich Text Control 

	Cinfo->SendHeader = TRUE;
	Cinfo->Finished = TRUE;
	Cinfo->CurrentColour = 1;

	LoadLibrary("riched20.dll");

	Cinfo->hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
		WS_CHILD |  WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_VSCROLL | ES_READONLY,
		6,145,290,130, hConsole, NULL, hInst, NULL);

	// Register for Mouse Events for Copy/Paste
	
	SendMessage(Cinfo->hwndOutput, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
	SendMessage(Cinfo->hwndOutput, EM_EXLIMITTEXT, 0, MAXLINES * LINELEN);

	Cinfo->hwndInput = GetDlgItem(hConsole, 118); 
 
	// Set our own WndProcs for the controls. 

	Cinfo->wpOrigInputProc = (WNDPROC) SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, (LONG) InputProc); 

	if (cfgMinToTray)
		if (Stream == -1)
			AddTrayMenuItem(hConsole, "BBS Console");
		else
			AddTrayMenuItem(hConsole, "Chat Console");

	if (Stream == -1)
		SetWindowText(hConsole, "BBS Console");
	else
		SetWindowText(hConsole, "Chat Console");

	ShowWindow(hConsole, SW_SHOWNORMAL);

	if (Cinfo->ConsoleRect.right < 100 || Cinfo->ConsoleRect.bottom < 100)
	{
		GetWindowRect(hConsole,	&Cinfo->ConsoleRect);
	}

	MoveWindow(hConsole, Cinfo->ConsoleRect.left, Cinfo->ConsoleRect.top, 
		Cinfo->ConsoleRect.right-Cinfo->ConsoleRect.left,
		Cinfo->ConsoleRect.bottom-Cinfo->ConsoleRect.top, TRUE);

	Cinfo->hConsole = hConsole;

	MoveWindows(Cinfo);

	Cinfo->Console = zalloc(sizeof(CIRCUIT));

	Cinfo->Console->Active = TRUE;
	Cinfo->Console->BPQStream = Stream;

	strcpy(Cinfo->Console->Callsign, SYSOPCall);

	user = LookupCall(SYSOPCall);

	if (user == NULL)
	{
		user = AllocateUserRecord(SYSOPCall);

		if (user == NULL) return 0; //		Cant happen??
	
		user->Temp = zalloc(sizeof (struct TempUserInfo));
	}

	time((time_t *)&user->TimeLastConnected);
	user->Total.ConnectsIn++;

	Cinfo->Console->UserPointer = user;
	Cinfo->Console->lastmsg = user->lastmsg;
	Cinfo->Console->paclen=236;
	Cinfo->Console->sysop = TRUE;

	Cinfo->Console->PageLen = user->PageLen;
	Cinfo->Console->Paging = (user->PageLen > 0);

	nodeprintf(Cinfo->Console, BBSSID, "BPQ-", Ver[0], Ver[1], Ver[2], Ver[3], "B", "", "", "", "F");

	if (user->Name[0] == 0)
	{
		Cinfo->Console->Flags |= GETTINGUSER;
		SendUnbuffered(-1, NewUserPrompt, strlen(NewUserPrompt));
	}
	else
	{
		if (Stream == -2)
		{
			if(ChatApplMask == 0)
			{
				BBSputs(Cinfo->Console, "Chat Node is disabled\r");
				SendPrompt(Cinfo->Console, user);
				return TRUE;
			}
		}
		else
			SendWelcomeMsg(-1, Cinfo->Console, user);
	}
	return TRUE;

}


VOID CloseConsole(int Stream)
{
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader[0]; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->Console)
		{
			if (Cinfo->BPQStream == Stream)
			{
				CloseConsoleSupport(Cinfo);
				return;
			}
		}
	}
}



VOID CloseConsoleSupport(struct ConsoleInfo * Cinfo)
{
	GetWindowRect(Cinfo->hConsole,	&ConsoleRect);	
	
	if (Cinfo->CloseWindowOnBye)
	{
//		PostMessage(hConsole, WM_DESTROY, 0, 0);
		DestroyWindow(Cinfo->hConsole);
	}
}

void MoveWindows(struct ConsoleInfo * Cinfo)
{
	RECT rcClient;
	int ClientWidth;

	GetClientRect(Cinfo->hConsole, &rcClient); 

	if (rcClient.bottom == 0)		// Minimised
		return;

	Cinfo->ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(Cinfo->hwndOutput,2, 2, ClientWidth-4, Cinfo->ClientHeight-InputBoxHeight-4, TRUE);
	MoveWindow(Cinfo->hwndInput,2, Cinfo->ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);

	GetClientRect(Cinfo->hwndOutput, &rcClient); 

	Cinfo->ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	Cinfo->WarnLen = ClientWidth/8 - 1;
	Cinfo->WrapLen = Cinfo->WarnLen;
	Cinfo->maxlinelen = Cinfo->WarnLen;

}

LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	int i;
	struct ConsoleInfo * Cinfo;
	ConnectionInfo * conn;

    UCHAR tchBuffer[100000];
	UCHAR * buf = tchBuffer;
    TEXTMETRIC tm; 
    int y;  
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 

	for (Cinfo = ConsHeader[0]; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hConsole == hWnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = InitHeader;
	
	switch (message) {

	case WM_CTLCOLOREDIT:
		
		if (Cinfo->Scrolled)
		{
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LONG)GetStockObject(LTGRAY_BRUSH);
		}
		return (DefWindowProc(hWnd, message, wParam, lParam));


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
	
	case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
            lpmis->itemHeight = 15; 
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
					
					SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM) tchBuffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

					if ((Cinfo->BPQStream == -2) && (tchBuffer[0] == 0x1b))
					{
						SetTextColor(lpdis->hDC,  Colours[tchBuffer[1] - 10]);
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

		SetFocus(Cinfo->hwndInput);
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		if (wmId >= BBSUSERCHAT && wmId < BBSUSERCHAT + 63)
		{
			// Chat to user

			conn=&Connections[wmId-BBSUSERCHAT];
		
			if (conn->Active)
			{
				conn->BBSFlags |= SYSOPCHAT;
				Cinfo->Console->SysopChatStream = conn;
				SendUnbuffered(conn->BPQStream, chatMsg, strlen(chatMsg));


//				Disconnect(conn->BPQStream);
			}
		}

		switch (wmId) {

		case ENDUSERCHAT:

			if (Cinfo->Console->SysopChatStream)
			{
				SendUnbuffered(Cinfo->Console->SysopChatStream->BPQStream, endChatMsg, strlen(endChatMsg));
				Cinfo->Console->SysopChatStream->BBSFlags &= ~SYSOPCHAT;
				SendPrompt(Cinfo->Console->SysopChatStream, Cinfo->Console->SysopChatStream->UserPointer);
				SendPrompt(Cinfo->Console, Cinfo->Console->UserPointer);
				Cinfo->Console->SysopChatStream = 0;
			}

			break;


		case 40000:
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

		case BPQBELLS:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->Bells, BPQBELLS);
			Bells = Cinfo->Bells;
			break;

		case BPQFLASHONBELL:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->FlashOnBell, BPQFLASHONBELL);
			FlashOnBell = Cinfo->FlashOnBell;
			break;

		case BPQStripLF:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->StripLF, BPQStripLF);
			StripLF = Cinfo->StripLF;
			break;

		case IDM_WARNINPUT:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->WarnWrap, IDM_WARNINPUT);
			WarnWrap = Cinfo->WarnWrap;
			break;


		case IDM_WRAPTEXT:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->WrapInput, IDM_WRAPTEXT);
			Cinfo->WrapInput = WrapInput;
			break;

		case IDM_Flash:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->FlashOnConnect, IDM_Flash);
			FlashOnConnect = Cinfo->FlashOnConnect;
			break;

		case IDM_CLOSEWINDOW:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->CloseWindowOnBye, IDM_CLOSEWINDOW);
			CloseWindowOnBye = Cinfo->CloseWindowOnBye;
			break;

		case BPQCLEAROUT:

			for (i = 0; i < MAXLINES; i++)
			{
				Cinfo->OutputScreen[i][0] = 0;
			}

			Cinfo->CurrentLine = 0;
			DoRefresh(Cinfo);
			break;


			SendMessage(Cinfo->hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYOUT:
		
			CopyRichTextToClipboard(Cinfo->hwndOutput);
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

			Cinfo->Height = lprc->bottom-lprc->top;
			Cinfo->Width = lprc->right-lprc->left;

			MoveWindows(Cinfo);
			
			return TRUE;

		case WM_SIZE:
	
			MoveWindows(Cinfo);		
			return TRUE;
		
		case WM_CLOSE:

	
			if (Cinfo->Console->SysopChatStream)
			{
				SendUnbuffered(Cinfo->Console->SysopChatStream->BPQStream, endChatMsg, strlen(endChatMsg));
				Cinfo->Console->SysopChatStream->BBSFlags &= ~SYSOPCHAT;
				SendPrompt(Cinfo->Console->SysopChatStream, Cinfo->Console->SysopChatStream->UserPointer);
				SendPrompt(Cinfo->Console, Cinfo->Console->UserPointer);
				Cinfo->Console->SysopChatStream = 0;
			}

			CloseConsoleSupport(Cinfo);
			
			return (DefWindowProc(hWnd, message, wParam, lParam));

		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&ConsoleRect);	// For save soutine
	
            SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, 
                (LONG) Cinfo->wpOrigInputProc); 
         
			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);

			if (Cinfo->Console && Cinfo->Console->Active)
			{
				ClearQueue(Cinfo->Console);
		
				Cinfo->Console->Active = FALSE;
				RefreshMainWindow();

				{
					SendUnbuffered(Cinfo->Console->BPQStream, SignoffMsg, strlen(SignoffMsg));
					if (Cinfo->Console->lastmsg > user->lastmsg)
					{
						user->lastmsg = Cinfo->Console->lastmsg;
						SaveUserDatabase();
					}
				}
			}

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

			free(Cinfo->readbuff);
			Cinfo->readbufflen = 0;

			free(Cinfo->Console);
			Cinfo->Console = 0;
			Cinfo->hConsole = NULL;

			break;


			
	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)hBBSUSERCHAT)
		{
			// Set up Chat Menu

			CIRCUIT * conn;
			char MenuLine[30];
			int n;

			for (n = 0; n <= NumberofStreams-1; n++)
			{
				conn=&Connections[n];

				RemoveMenu(hBBSUSERCHAT, BBSUSERCHAT + n, MF_BYCOMMAND);

				if (conn->Active)
				{
					sprintf_s(MenuLine, 30, "%d %s", conn->BPQStream, conn->Callsign);
					AppendMenu(hBBSUSERCHAT, MF_STRING, BBSUSERCHAT + n, MenuLine);
				}
			}
			return TRUE;
		}
		break;



		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}


LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	int i;
	unsigned int TextLen;
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader[0]; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hwndInput == hwnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = InitHeader;

 
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
		
		if(Cinfo->WarnWrap || Cinfo->WrapInput)
		{
			TextLen = SendMessage(Cinfo->hwndInput,WM_GETTEXTLENGTH, 0, 0);

			if (Cinfo->WarnWrap)
				if (TextLen == Cinfo->WarnLen) Beep(220, 150);

			if (Cinfo->WrapInput)
				if ((wParam == 0x20) && (TextLen > Cinfo->WrapLen))
					wParam = 13;		// Replace space with Enter

		}

		if (wParam == 13)
		{
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

			if (Cinfo->BPQStream == -2)
			{
				char Msg[INPUTLEN+4];
				Msg[0] = 0x1b;
				Msg[1] = 11;
				memcpy(&Msg[2], Cinfo->kbbuf, Cinfo->kbptr+1);

				WritetoConsoleWindow(Cinfo->BPQStream, Msg, Cinfo->kbptr+3);

			}
			else
				WritetoConsoleWindow(Cinfo->BPQStream, Cinfo->kbbuf, Cinfo->kbptr+1);

			if (Cinfo->Scrolled)
			{
				POINT Point;
				Point.x = 0;
				Point.y = 25000;					// Should be plenty for any font

				SendMessage(Cinfo->hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
				Cinfo->Scrolled = FALSE;
			}

			DoRefresh(Cinfo);

			if (Cinfo->Console->SysopChatStream)
				SendUnbuffered(Cinfo->Console->SysopChatStream->BPQStream, &Cinfo->kbbuf[0], Cinfo->kbptr+1);
			else
				ProcessLine(Cinfo->Console, user, &Cinfo->kbbuf[0], Cinfo->kbptr+1);

			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
	
			Cinfo->kbbuf[0]=0x1a;
			Cinfo->kbbuf[1]=13;

			ProcessLine(Cinfo->Console, user, &Cinfo->kbbuf[0], 2);


			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
 
	}

    return CallWindowProc(Cinfo->wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 



int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len);

int WritetoConsoleWindow(int Stream, char * Msg, int len)
{
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader[0]; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->Console)
		{
			if (Cinfo->BPQStream == Stream)
			{
				WritetoConsoleWindowSupport(Cinfo, Msg, len);
				DoRefresh(Cinfo);
				return 0;
			}
		}
	}
	return 0;
}

int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len)
{
	char * ptr1, * ptr2;

	if (len + Cinfo->PartLinePtr > Cinfo->readbufflen)
	{
		Cinfo->readbufflen += len + Cinfo->PartLinePtr;
		Cinfo->readbuff = realloc(Cinfo->readbuff, Cinfo->readbufflen);
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

	if (Cinfo->Bells)
	{
		do {

			ptr2=memchr(ptr1,7,len);
			
			if (ptr2)
			{
				*(ptr2)=32;

				if (Cinfo->FlashOnBell)
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

int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
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

		if (OpenClipboard(MainWnd))
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


void CopyToClipboard(HWND hWnd)
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


DWORD CALLBACK EditStreamCallback(struct ConsoleInfo * Cinfo, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	int ReqLen = cb;
	int i;
	int Line;

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
	strcat(lpBuff, Cinfo->OutputScreen[Line]);
	strcat(lpBuff, "\\line");

	if (Cinfo->Index == MAXLINES)
	{
		Cinfo->Finished = TRUE;
		strcat(lpBuff, "}");
		i = 10;
	}
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

VOID AddLinetoWindow(struct ConsoleInfo * Cinfo, char * Line)
{
	int Len = strlen(Line);
	char * ptr1 = Line;
	char * ptr2;
	int l, Index;
	char LineCopy[LINELEN * 2];

	if (Len > 199)			// Console can't handle long lines
	{
		Line[198] = 13;
		Line[199] = 0;
		Len = 199;
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


/*
#define XBITMAP 80 
#define YBITMAP 20 
 
#define BUFFER MAX_PATH 
 
HBITMAP hbmpPencil, hbmpCrayon, hbmpMarker, hbmpPen, hbmpFork; 
HBITMAP hbmpPicture, hbmpOld; 
 
void AddItem(HWND hwnd, LPSTR lpstr, HBITMAP hbmp) 
{ 
    int nItem; 
 
    nItem = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)lpstr); 
    SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)hbmp); 
} 
 
DWORD APIENTRY DlgDrawProc( 
        HWND hDlg,            // window handle to dialog box 
        UINT message,         // type of message 
        UINT wParam,          // message-specific information 
        LONG lParam) 
{ 
    int nItem; 
    TCHAR tchBuffer[BUFFER]; 
    HBITMAP hbmp; 
    HWND hListBox; 
    TEXTMETRIC tm; 
    int y; 
    HDC hdcMem; 
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 
    RECT rcBitmap;
	HRESULT hr; 
	size_t * pcch;
 
    switch (message) 
    { 
 
        case WM_INITDIALOG: 
 
            // Load bitmaps. 
 
            hbmpPencil = LoadBitmap(hinst, MAKEINTRESOURCE(700)); 
            hbmpCrayon = LoadBitmap(hinst, MAKEINTRESOURCE(701)); 
            hbmpMarker = LoadBitmap(hinst, MAKEINTRESOURCE(702)); 
            hbmpPen = LoadBitmap(hinst, MAKEINTRESOURCE(703)); 
            hbmpFork = LoadBitmap(hinst, MAKEINTRESOURCE(704)); 
 
            // Retrieve list box handle. 
 
            hListBox = GetDlgItem(hDlg, IDL_STUFF); 
 
            // Initialize the list box text and associate a bitmap 
            // with each list box item. 
 
            AddItem(hListBox, "pencil", hbmpPencil); 
            AddItem(hListBox, "crayon", hbmpCrayon); 
            AddItem(hListBox, "marker", hbmpMarker); 
            AddItem(hListBox, "pen",    hbmpPen); 
            AddItem(hListBox, "fork",   hbmpFork); 
 
            SetFocus(hListBox); 
            SendMessage(hListBox, LB_SETCURSEL, 0, 0); 
            return TRUE; 
 
        case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
            lpmis->itemHeight = 20; 
            return TRUE; 
 
        case WM_DRAWITEM: 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If there are no list box items, skip this message. 
 
            if (lpdis->itemID == -1) 
            { 
                break; 
            } 
 
            // Draw the bitmap and text for the list box item. Draw a 
            // rectangle around the bitmap if it is selected. 
 
            switch (lpdis->itemAction) 
            { 
                case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 
                    // Display the bitmap associated with the item. 
 
                    hbmpPicture =(HBITMAP)SendMessage(lpdis->hwndItem, 
                        LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0); 
 
                    hdcMem = CreateCompatibleDC(lpdis->hDC); 
                    hbmpOld = SelectObject(hdcMem, hbmpPicture); 
 
                    BitBlt(lpdis->hDC, 
                        lpdis->rcItem.left, lpdis->rcItem.top, 
                        lpdis->rcItem.right - lpdis->rcItem.left, 
                        lpdis->rcItem.bottom - lpdis->rcItem.top, 
                        hdcMem, 0, 0, SRCCOPY); 
 
                    // Display the text associated with the item. 
 
                    SendMessage(lpdis->hwndItem, LB_GETTEXT, 
                        lpdis->itemID, (LPARAM) tchBuffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
                        tm.tmHeight) / 2;
						
                    hr = StringCchLength(tchBuffer, BUFFER, pcch);
                    if (FAILED(hr))
                    {
                        // TODO: Handle error.
                    }
 
                    TextOut(lpdis->hDC, 
                        XBITMAP + 6, 
                        y, 
                        tchBuffer, 
                        pcch); 						
 
                    SelectObject(hdcMem, hbmpOld); 
                    DeleteDC(hdcMem); 
 
                    // Is the item selected? 
 
                    if (lpdis->itemState & ODS_SELECTED) 
                    { 
                        // Set RECT coordinates to surround only the 
                        // bitmap. 
 
                        rcBitmap.left = lpdis->rcItem.left; 
                        rcBitmap.top = lpdis->rcItem.top; 
                        rcBitmap.right = lpdis->rcItem.left + XBITMAP; 
                        rcBitmap.bottom = lpdis->rcItem.top + YBITMAP; 
 
                        // Draw a rectangle around bitmap to indicate 
                        // the selection. 
 
                        DrawFocusRect(lpdis->hDC, &rcBitmap); 
                    } 
                    break; 
 
                case ODA_FOCUS: 
 
                    // Do not process focus changes. The focus caret 
                    // (outline rectangle) indicates the selection. 
                    // The IDOK button indicates the final 
                    // selection. 
 
                    break; 
            } 
            return TRUE; 
 
        case WM_COMMAND: 
 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                    // Get the selected item's text. 
 
                    nItem = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                       LB_GETCURSEL, 0, (LPARAM) 0); 
                       hbmp = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                            LB_GETITEMDATA, nItem, 0); 
 
                    // If the item is not the correct answer, tell the 
                    // user to try again. 
                    //
                    // If the item is the correct answer, congratulate 
                    // the user and destroy the dialog box. 
 
                    if (hbmp != hbmpFork) 
                    { 
                        MessageBox(hDlg, "Try again!", "Oops", MB_OK); 
                        return FALSE; 
                    } 
                    else 
                    { 
                        MessageBox(hDlg, "You're right!", 
                            "Congratulations.", MB_OK); 
 
                      // Fall through. 
                    } 
 
                case IDCANCEL: 
 
                    // Destroy the dialog box. 
 
                    EndDialog(hDlg, TRUE); 
                    return TRUE; 
 
                default: 
 
                    return FALSE; 
            } 
 
        case WM_DESTROY: 
 
            // Free any resources used by the bitmaps. 
 
            DeleteObject(hbmpPencil); 
            DeleteObject(hbmpCrayon); 
            DeleteObject(hbmpMarker); 
            DeleteObject(hbmpPen); 
            DeleteObject(hbmpFork); 
 
            return TRUE; 
 
        default: 
            return FALSE; 
 
    } 
    return FALSE; 
} 
*/
