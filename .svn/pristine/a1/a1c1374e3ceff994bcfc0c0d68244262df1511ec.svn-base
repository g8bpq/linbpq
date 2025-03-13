		
// Mail and Chat Server for BPQ32 Packet Switch
//
//	Configuration Module

#include "bpqmail.h"


#define C_PAGES 7

int CurrentPage=0;				// Page currently on show in tabbed Dialog

#define BBSPARAMS 0
#define ISPPARAMS 1
#define MAINTPARAMS 2
#define WELCOMEMSGS 3
#define PROMPTS 4
#define FILTERS 5
#define WPUPDATE 6

typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control


DLGTEMPLATE *apRes[C_PAGES];

} DLGHDR;

HWND hwndDlg;		// Config Dialog
HWND hwndDisplay;   // Current child dialog box

HWND hCheck[33];
HWND hNullCheck[33];
HWND hSendMF[33];
HWND hSendHDDR[33];
HWND hLabel[33];
HWND hUIBox[33];
HFONT hFont;
LOGFONT LFTTYFONT ;

char CurrentConfigCall[20];		// Current user or bbs
int CurrentConfigIndex;			// Index of current user record
int CurrentMsgIndex;			// Index of current Msg record
struct UserInfo * CurrentBBS;	// User Record of selected BBS iin Forwarding Config;

struct UserInfo * MsgBBSList[NBBBS+2] = {0}; // Sorted BBS List

char InfoBoxText[100];			// Text to display in Config Info Popup

char Filter_FROM[20];
char Filter_TO[20];
char Filter_VIA[60];				// Filters for Edit Message Dialog
char Filter_BID[16];				// Filters for Edit Message Dialog

extern char LTFROMString[2048];
extern char LTTOString[2048];
extern char LTATString[2048];
VOID * GetOverrideFromString(char * input);

extern time_t MaintClock;			// Time to run housekeeping

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);
VOID WINAPI OnTabbedDialogInit(HWND hwndDlg);
VOID SaveWPConfig(HWND hDlg);
PrintMessage(struct MsgInfo * Msg);
BOOL ForwardMessagetoFile(struct MsgInfo * Msg, FILE * Handle);
VOID TidyPrompts();
struct UserInfo * FindBBS(char * Name);

INT_PTR CALLBACK UIDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditMsgTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID SaveMAINTConfigFromDialog();
VOID TidyWelcomeMsg(char ** pPrompt);

// POP3 Password is encrypted by xor'ing it with an MD5 hash of the hostname and pop3 server name


double GetDlgItemFloat(HWND hDlg, int DlgItem, BOOL *lpTranslated, BOOL bSigned)
{
	char Num[32];
	double ret = 0.0;

	GetDlgItemText(hDlg, DlgItem, Num, 31);
	return atof(Num);
}


BOOL SetDlgItemFloat(HWND hDlg, int DlgItem, double Value, BOOL bSigned)
{
	char Num[32];

	sprintf(Num, "%.2f", Value);

	while (Num[strlen(Num) -1] == '0')
		Num[strlen(Num) -1] = 0;

	if (Num[strlen(Num) -1] == '.')
		Num[strlen(Num) -1] = 0;

	return SetDlgItemText(hDlg, DlgItem, Num);
}



int ww, wh, w, h, hpos, vpos;
int xmargin;
int ymargin;

INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT Rect;
	SCROLLINFO Sinfo;

	int ret;
		
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (message)
	{
	case WM_INITDIALOG:

		ShowScrollBar(hDlg, SB_BOTH, FALSE);		// Hide them till needed
		OnTabbedDialogInit(hDlg);

		// initialise scroll bars

		xmargin = 6;
		ymargin = 2 + GetSystemMetrics(SM_CYCAPTION);
		hpos = vpos = 0;

		return (INT_PTR)TRUE;
		
	case WM_SIZE:
		
		w = LOWORD(lParam);
		h = HIWORD(lParam);

		// If window is smaller than client area enable scroll bars

		ret = GetWindowRect(hwndDisplay, &Rect);
		ww = Rect.right - Rect.left;
		wh = Rect.bottom - Rect.top;

		if (ww <= w && (wh + ymargin) <= h)
		{
			ShowScrollBar(hDlg, SB_BOTH, FALSE);	// Hide them till needed
			MoveWindow(hwndDisplay, xmargin, ymargin, ww, wh, TRUE);
			hpos = vpos = 0;
			return TRUE;
		}

		ShowScrollBar(hDlg, SB_BOTH, TRUE);	

		Sinfo.cbSize = sizeof(SCROLLINFO);
		Sinfo.fMask = SIF_ALL;
		Sinfo.nMin = 0;
		Sinfo.nMax = ww + xmargin;
		Sinfo.nPage = w;
		Sinfo.nPos = hpos;
		SetScrollInfo(hDlg, SB_HORZ, &Sinfo, TRUE);

		Sinfo.cbSize = sizeof(SCROLLINFO);
		Sinfo.fMask = SIF_ALL;
		Sinfo.nMin = 0;
		Sinfo.nMax = wh + ymargin;
		Sinfo.nPage = h;
		Sinfo.nPos = hpos;
		SetScrollInfo(hDlg, SB_VERT, &Sinfo, TRUE);

		return TRUE;

	case WM_HSCROLL:

		switch (LOWORD(wParam))
		{
		case SB_PAGELEFT:

			hpos -= 20;
			if (hpos < 0)
				hpos = 0;

			goto UpdateHPos;

		case SB_LINELEFT:

			if (hpos)
				hpos --;

			goto UpdateHPos;

		case SB_PAGERIGHT:

			hpos += 20;
			goto UpdateHPos;

		case SB_LINERIGHT:

			hpos++;
			goto UpdateHPos;

		case SB_THUMBPOSITION:

			hpos = HIWORD(wParam);

UpdateHPos:
			// Need to update Scroll Bar

			Sinfo.cbSize = sizeof(SCROLLINFO);
			Sinfo.fMask = SIF_ALL;
			Sinfo.nMin = 0;
			Sinfo.nMax = ww + xmargin;
			Sinfo.nPage = w;
			Sinfo.nPos = hpos;
			SetScrollInfo(hDlg, SB_HORZ, &Sinfo, TRUE);

			// Move Client Window

			MoveWindow(hwndDisplay, xmargin - hpos , ymargin - vpos , ww, wh, TRUE);
			return TRUE;
		}

		return TRUE;

		
	case WM_VSCROLL:

		switch (LOWORD(wParam))
		{
		case SB_PAGEUP:

			vpos -= 20;
			if (vpos < 0)
				vpos = 0;

			goto UpdateVPos;

		case SB_LINEUP:

			if (vpos)
				vpos --;

			goto UpdateVPos;

		case SB_PAGEDOWN:

			vpos += 20;
			goto UpdateVPos;

		case SB_LINEDOWN:

			vpos++;
			goto UpdateVPos;

		case SB_THUMBPOSITION:

			vpos = HIWORD(wParam);

UpdateVPos:
			// Need to update Scroll Bar

			Sinfo.cbSize = sizeof(SCROLLINFO);
			Sinfo.fMask = SIF_ALL;
			Sinfo.nMin = 0;
			Sinfo.nMax = wh + ymargin;
			Sinfo.nPage = h;
			Sinfo.nPos = vpos;
			SetScrollInfo(hDlg, SB_VERT, &Sinfo, TRUE);

			// Move Client Window

			MoveWindow(hwndDisplay, xmargin - hpos , ymargin - vpos , ww, wh, TRUE);
			return TRUE;
		}

		return TRUE;


	case WM_NOTIFY:

		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:

			OnSelChanged(hDlg);

			// Check if scroll now needed

			ret = GetWindowRect(hwndDisplay, &Rect);
			ww = Rect.right - Rect.left;
			wh = Rect.bottom - Rect.top;

			if (ww <= w && (wh + 27) <= h)
			{
				ShowScrollBar(hDlg, SB_BOTH, FALSE);	// Hide them till needed
				return TRUE;
			}

			ShowScrollBar(hDlg, SB_BOTH, TRUE);	

			return TRUE;
			// More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
		}

		break;

	case WM_CTLCOLORDLG:

		return (LONG)bgBrush;

	case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	
	case WM_KEYUP:

		if (wParam == VK_TAB)
			return TRUE;


		break;



	case WM_COMMAND:

		switch (LOWORD(wParam))
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return FALSE;

}

INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
		
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, 5050, InfoBoxText);

		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case 0:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
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

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case IDC_UICONFIG:
			
			DialogBox(hInst, MAKEINTRESOURCE(IDD_UICONFIG), hWnd, UIDialogProc);
			return TRUE;

		case IDC_BBSSAVE:
			
			SaveBBSConfig();
			return TRUE;

		case IDC_ISPSAVE:
			
			SaveISPConfig();
			return TRUE;


		case IDM_MAINTSAVE:

			SaveMAINTConfigFromDialog();
			return TRUE;


		case IDM_MSGSAVE:

			SaveWelcomeMsgs();
			return TRUE;

		case IDM_PROMPTSAVE:

			SavePrompts();
			return TRUE;

		case IDC_FILTERSAVE:

			SaveFilters(hDlg);
			return TRUE;

		case IDC_WPSAVE:

			SaveWPConfig(hDlg);
			return TRUE;

		}
		break;

	}	
	return (INT_PTR)FALSE;
}


//The following function processes the WM_INITDIALOG message for the main dialog box. The function allocates the DLGHDR structure, loads the dialog template resources for the child dialog boxes, and creates the tab control.

//The size of each child dialog box is specified by the DLGTEMPLATE structure. The function examines the size of each dialog box and uses the macro for the TCM_ADJUSTRECT message to calculate an appropriate size for the tab control. Then it sizes the dialog box and positions the two buttons accordingly. This example sends TCM_ADJUSTRECT by using the TabCtrl_AdjustRect macro.

VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos;
	INITCOMMONCONTROLSEX init;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC=ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	tie.pszText = "BBS Params";
	TabCtrl_InsertItem(pHdr->hwndTab, 0, &tie);

	tie.pszText = "ISP Interface";
	TabCtrl_InsertItem(pHdr->hwndTab, 1, &tie);

 	tie.pszText = "Housekeeping";
	TabCtrl_InsertItem(pHdr->hwndTab, 2, &tie);

	tie.pszText = "Welcome Msgs";
	TabCtrl_InsertItem(pHdr->hwndTab, 3, &tie);

	tie.pszText = "Prompts";
	TabCtrl_InsertItem(pHdr->hwndTab, 4, &tie);

	tie.pszText = "Msg Filters";
	TabCtrl_InsertItem(pHdr->hwndTab, 5, &tie);

	tie.pszText = "WP Update";
	TabCtrl_InsertItem(pHdr->hwndTab, 6, &tie);

	// Lock the resources for the three child dialog boxes.

	pHdr->apRes[0] = DoLockDlgRes("BBS_CONFIG");
	pHdr->apRes[1] = DoLockDlgRes("ISP_CONFIG");
	pHdr->apRes[2] = DoLockDlgRes("MAINT");
	pHdr->apRes[3] = DoLockDlgRes("WELCOMEMSG");
	pHdr->apRes[4] = DoLockDlgRes("BBSPROMPTS");
	pHdr->apRes[5] = DoLockDlgRes("FILTERS");
	pHdr->apRes[6] = DoLockDlgRes("WPUPDATE");

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < C_PAGES-1; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char Nodes[1000]="";
	char Text[10000]="";
	char Line[80];
	struct Override ** Call;
	char Time[10];

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	// Fill in the controls

	switch (CurrentPage)
	{
	case BBSPARAMS:
		
		SetDlgItemText(pHdr->hwndDisplay, IDC_BBSCall, BBSName);
		SetDlgItemText(pHdr->hwndDisplay, IDC_SYSOPCALL, SYSOPCall);
		CheckDlgButton(pHdr->hwndDisplay, IDC_SYSTOSYSOPCALL, SendSYStoSYSOPCall);
		CheckDlgButton(pHdr->hwndDisplay, IDC_BBSTOSYSOPCALL, SendBBStoSYSOPCall);
		CheckDlgButton(pHdr->hwndDisplay, IDC_DONTHOLDNEW, DontHoldNewUsers);
		CheckDlgButton(pHdr->hwndDisplay, IDC_FORWARDTOBBS, ForwardToMe);
		CheckDlgButton(pHdr->hwndDisplay, IDC_NONAME, AllowAnon);
		CheckDlgButton(pHdr->hwndDisplay, IDC_USERRKILLT, !UserCantKillT);		// Note negative logic
		CheckDlgButton(pHdr->hwndDisplay, IDC_NOHOMEBBS, DontNeedHomeBBS);
		CheckDlgButton(pHdr->hwndDisplay, IDC_DONTCHECKFROM, DontCheckFromCall);
		CheckDlgButton(pHdr->hwndDisplay, IDC_DEFAULTNOWINLINK, DefaultNoWINLINK);

		
		
		SetDlgItemText(pHdr->hwndDisplay, IDC_HRoute, HRoute);
		SetDlgItemText(pHdr->hwndDisplay, IDC_BaseDir, BaseDirRaw);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSAppl, BBSApplNum, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSStreams, MaxStreams, FALSE);
		CheckDlgButton(pHdr->hwndDisplay, IDC_ENABLEUI, EnableUI);
		SetDlgItemInt(pHdr->hwndDisplay, MAILFOR_MINS, MailForInterval, FALSE);
		CheckDlgButton(pHdr->hwndDisplay, IDC_REFUSEBULLS, RefuseBulls);
		CheckDlgButton(pHdr->hwndDisplay, IDC_KNOWNUSERS, OnlyKnown);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Port, POP3InPort, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_NNTPPort, NNTPInPort, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_SMTPPort, SMTPInPort, FALSE);
		CheckDlgButton(pHdr->hwndDisplay, IDC_REMOTEEMAIL, RemoteEmail);
		SetDlgItemText(pHdr->hwndDisplay, IDC_AMPR, AMPRDomain);
		CheckDlgButton(pHdr->hwndDisplay, IDC_FORWARDAMPR, SendAMPRDirect);


		break;

	case ISPPARAMS:
		
		CheckDlgButton(pHdr->hwndDisplay, IDC_ISP_Gateway_Enabled, ISP_Gateway_Enabled);
 		
		SetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Timer, ISPPOP3Interval, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_MyMailDomain, MyDomain);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPSMTPName, ISPSMTPName);
		SetDlgItemText(pHdr->hwndDisplay, SMTP_EHELO, ISPEHLOName);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_ISPSMTPPort, ISPSMTPPort, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPPOP3Name, ISPPOP3Name);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_ISPPOP3Port, ISPPOP3Port, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountName, ISPAccountName);
		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountPass, ISPAccountPass);

		CheckDlgButton(pHdr->hwndDisplay, ISP_SMTP_AUTH, SMTPAuthNeeded);

		break;

	case MAINTPARAMS:

		SetDlgItemInt(pHdr->hwndDisplay, IDC_MAXMSG, MaxMsgno, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BIDLIFETIME, BidLifetime, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_USERLIFETIME, UserLifetime, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_LOGLIFETIME, LogAge, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_MAINTINTERVAL, MaintInterval, FALSE);
		sprintf(Time, "%04d", MaintTime);
		SetDlgItemText(pHdr->hwndDisplay, IDC_MAINTTIME, Time);

		SetDlgItemFloat(pHdr->hwndDisplay, IDM_PR, PR, FALSE);
		SetDlgItemFloat(pHdr->hwndDisplay, IDM_PUR, PUR, FALSE);
		SetDlgItemFloat(pHdr->hwndDisplay, IDM_PF, PF, FALSE);
		SetDlgItemFloat(pHdr->hwndDisplay, IDM_PNF, PNF, FALSE);

		SetDlgItemInt(pHdr->hwndDisplay, IDM_BF, BF, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_BNF, BNF, FALSE);

		SetDlgItemInt(pHdr->hwndDisplay, IDM_NTSD, NTSD, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_NTSF, NTSF, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_NTSU, NTSU, FALSE);

		CheckDlgButton(pHdr->hwndDisplay, IDC_DELETETORECYCLE, DeletetoRecycleBin);
		CheckDlgButton(pHdr->hwndDisplay, IDC_MAINTNOMAIL, SuppressMaintEmail);
		CheckDlgButton(pHdr->hwndDisplay, IDC_MAINTSAVEREG, SaveRegDuringMaint);
		CheckDlgButton(pHdr->hwndDisplay, IDC_OVERRIDEUNSENT, OverrideUnsent);
		CheckDlgButton(pHdr->hwndDisplay, IDC_MAINTNONDELIVERY , SendNonDeliveryMsgs);
		CheckDlgButton(pHdr->hwndDisplay, IDC_MAINTTRAFFIC, GenerateTrafficReport);

		if (LTFROM)
		{
			Call = LTFROM;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTFROM, Text);

		Text[0] = 0;

		if (LTTO)
		{
			Call = LTTO;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTTO, Text);

		Text[0] = 0;

		if (LTAT)
		{
			Call = LTAT;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTAT, Text);

		Text[0] = 0;

		break;


	case WELCOMEMSGS:

		SetDlgItemText(pHdr->hwndDisplay, IDM_USERMSG, WelcomeMsg);
		SetDlgItemText(pHdr->hwndDisplay, IDM_NEWUSERMSG, NewWelcomeMsg);
		SetDlgItemText(pHdr->hwndDisplay, IDM_EXPERTUSERMSG, ExpertWelcomeMsg);
		SetDlgItemText(pHdr->hwndDisplay, IDM_SIGNOFF, SignoffMsg);

		break;

	case PROMPTS:

		SetDlgItemText(pHdr->hwndDisplay, IDM_USERMSG, Prompt);
		SetDlgItemText(pHdr->hwndDisplay, IDM_NEWUSERMSG, NewPrompt);
		SetDlgItemText(pHdr->hwndDisplay, IDM_EXPERTUSERMSG, ExpertPrompt);

		break;
	case FILTERS:

		Text[0] = 0;

		if (RejFrom)
		{
			char ** Call = RejFrom;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_REJFROM, Text);

		Text[0] = 0;

		if (RejTo)
		{
			char ** Call = RejTo;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_REJTO, Text);

		Text[0] = 0;

		if (RejAt)
		{
			char ** Call = RejAt;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_REJAT, Text);

		Text[0] = 0;

		if (RejBID)
		{
			char ** Call = RejBID;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_REJBID, Text);

		Text[0] = 0;

		if (HoldFrom)
		{
			char ** Call = HoldFrom;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_HOLDFROM, Text);

		Text[0] = 0;

		if (HoldTo)
		{
			char ** Call = HoldTo;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_HOLDTO, Text);

		Text[0] = 0;

		if (HoldAt)
		{
			char ** Call = HoldAt;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}

		SetDlgItemText(pHdr->hwndDisplay, IDC_HOLDAT, Text);

		Text[0] = 0;

		if (HoldBID)
		{
			char ** Call = HoldBID;
			while(Call[0])
			{
				sprintf(Line, "%s\r\n", Call[0]);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_HOLDBID, Text);



		break;

	case WPUPDATE:

		if (SendWPAddrs)
		{
			char ** Calls = SendWPAddrs;
			char Text[10000]="";

			while(Calls[0])
			{
				strcat(Text, Calls[0]);
				strcat(Text, "\r\n");
				Calls++;
			}
			
			SetDlgItemText(hwndDisplay, IDC_WPTO, Text);
		}
		
		SendDlgItemMessage(hwndDisplay, IDC_WPTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "B");
		SendDlgItemMessage(hwndDisplay, IDC_WPTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "P");


		SendDlgItemMessage(hwndDisplay, IDC_WPTYPE, CB_SETCURSEL, SendWPType, 0);

		CheckDlgButton(hwndDisplay, IDC_SENDWP, SendWP);
//		CheckDlgButton(hwndDisplay, IDC_SENDWP, NoWPGuesses);
		CheckDlgButton(hwndDisplay, IDC_FILTERWPB, FilterWPBulls);

		break;
	}

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);
}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}

void SetForwardingPage(HWND hDlg, struct UserInfo * user)
{

	struct	BBSForwardingInfo * ForwardingInfo = user->ForwardingInfo;
	char ** Calls;
	char Text[100000]="";

	Calls = ForwardingInfo->TOCalls;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
	}
	SetDlgItemText(hDlg, IDC_TOCALLS, Text);

	Text[0] = 0;

	Calls = ForwardingInfo->ATCalls;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
	}

	SetDlgItemText(hDlg, IDC_ATCALLS, Text);

	Text[0] = 0;
	Calls = ForwardingInfo->Haddresses;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
			
	}

	SetDlgItemText(hDlg, IDC_HROUTES, Text);

	Text[0] = 0;
	Calls = ForwardingInfo->HaddressesP;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
			
	}

	SetDlgItemText(hDlg, IDC_HROUTESP, Text);

	Text[0] = 0;

	Calls = ForwardingInfo->FWDTimes;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
	}

	SetDlgItemText(hDlg, IDC_FWDTIMES, Text);

	Text[0] = 0;
	Calls = ForwardingInfo->ConnectScript;

	if (Calls)
	{
		while(Calls[0])
		{
			strcat(Text, Calls[0]);
			strcat(Text, "\r\n");
			Calls++;
		}
			
	}

	SetDlgItemText(hDlg, IDC_CALL, Text);

	if (ForwardingInfo->AllowB1 || ForwardingInfo->AllowB2)
		ForwardingInfo->AllowCompressed = TRUE;

	CheckDlgButton(hDlg, IDC_FWDENABLE, ForwardingInfo->Enabled);
	CheckDlgButton(hDlg, IDC_REVERSE, ForwardingInfo->ReverseFlag);
	CheckDlgButton(hDlg, IDC_BLOCKED, ForwardingInfo->AllowBlocked);
	CheckDlgButton(hDlg, IDC_ALLOWCOMP, ForwardingInfo->AllowCompressed);
	CheckDlgButton(hDlg, IDC_USEB1, ForwardingInfo->AllowB1);
	CheckDlgButton(hDlg, IDC_USEB2, ForwardingInfo->AllowB2);
	CheckDlgButton(hDlg, IDC_CTRLZ, ForwardingInfo->SendCTRLZ);
	CheckDlgButton(hDlg, IDC_PERSONALONLY, ForwardingInfo->PersonalOnly);
	CheckDlgButton(hDlg, IDC_SENDNEW, ForwardingInfo->SendNew);
	SetDlgItemInt(hDlg, IDC_FWDINT, ForwardingInfo->FwdInterval, FALSE);
	SetDlgItemInt(hDlg, IDC_REVFWDINT, ForwardingInfo->RevFwdInterval, FALSE);
	SetDlgItemInt(hDlg, IDC_MAXBLOCK, ForwardingInfo->MaxFBBBlockSize, FALSE);
	SetDlgItemInt(hDlg, IDC_CONTIMEOUT, ForwardingInfo->ConTimeout, FALSE);
	SetDlgItemText(hDlg, IDC_BBSHA, ForwardingInfo->BBSHA);

	SetFocus(GetDlgItem(hDlg, IDC_TOCALLS)); 
	return;
}

int Do_BBS_Sel_Changed(HWND hDlg)
{
	// Update BBS display with newly selected BBS

	struct UserInfo * user;

	int Sel = SendDlgItemMessage(hDlg, IDC_BBS, CB_GETCURSEL, 0, 0);

	SendDlgItemMessage(hDlg, IDC_BBS, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);

	for (user = BBSChain; user; user = user->BBSNext)
	{
		if (strcmp(user->Call, CurrentConfigCall) == 0)
		{
			CurrentBBS = user;
			SetForwardingPage(hDlg, user);			// moved to separate routine as also called from copy config
		}
	}
	return 0;

}
int Sel;

int Do_User_Sel_Changed(HWND hDlg)
{

	// Update BBS display with newly selected BBS

	struct UserInfo * user;

	Sel = SendDlgItemMessage(hDlg, IDC_USER, CB_GETCURSEL, 0, 0);

	if (Sel == -1)
		SendDlgItemMessage(hDlg, IDC_USER, WM_GETTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);
	else
		SendDlgItemMessage(hDlg, IDC_USER, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);

	for (CurrentConfigIndex = 1; CurrentConfigIndex <= NumberofUsers; CurrentConfigIndex++)
	{
		user = UserRecPtr[CurrentConfigIndex];

		if (_stricmp(user->Call, CurrentConfigCall) == 0)
		{
			struct tm *tm;
			char Date[80];
			char * Dateptr;
			int i, n, s;
			char SSID[10];

			int	ConnectsIn;
			int ConnectsOut;
			int MsgsReceived;
			int MsgsSent;
			int MsgsRejectedIn;
			int MsgsRejectedOut;
			int BytesForwardedIn;
			int BytesForwardedOut;
//			char MsgsIn[80];
//			char MsgsOut[80];
//			char BytesIn[80];
//			char BytesOut[80];
//			char RejIn[80];
//			char RejOut[80];

			i = 0;

			ConnectsIn = user->Total.ConnectsIn - user->Last.ConnectsIn;
			ConnectsOut = user->Total.ConnectsOut - user->Last.ConnectsOut;

			MsgsReceived = MsgsSent = MsgsRejectedIn = MsgsRejectedOut = BytesForwardedIn = BytesForwardedOut = 0;

			for (n = 0; n < 4; n++)
			{
				MsgsReceived +=	user->Total.MsgsReceived[n] - user->Last.MsgsReceived[n];	
				MsgsSent += user->Total.MsgsSent[n] - user->Last.MsgsSent[n];
				BytesForwardedIn += user->Total.BytesForwardedIn[n] - user->Last.BytesForwardedIn[n];
				BytesForwardedOut += user->Total.BytesForwardedOut[n] - user->Last.BytesForwardedOut[n];
				MsgsRejectedIn += user->Total.MsgsRejectedIn[n] - user->Last.MsgsRejectedIn[n];
				MsgsRejectedOut += user->Total.MsgsRejectedOut[n] - user->Last.MsgsRejectedOut[n];
			}



			SetDlgItemText(hDlg, IDC_NAME, user->Name);
			SetDlgItemText(hDlg, IDC_PASSWORD, user->pass);
			SetDlgItemText(hDlg, IDC_QTH, user->Address);
			SetDlgItemText(hDlg, IDC_UZIP, user->ZIP);
			SetDlgItemText(hDlg, IDC_HOMEBBS, user->HomeBBS);
			SetDlgItemText(hDlg, IDC_CMSPASS, user->CMSPass);
			
			SetDlgItemInt(hDlg, IDC_LASTLISTED, user->lastmsg, TRUE);

			CheckDlgButton(hDlg, IDC_SYSOP, (user->flags & F_SYSOP));
			CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
			CheckDlgButton(hDlg, IDC_PMSFLAG, (user->flags & F_PMS));
			CheckDlgButton(hDlg, IDC_EXPERT, (user->flags & F_Expert));
			CheckDlgButton(hDlg, IDC_EXCLUDED, (user->flags & F_Excluded));
			CheckDlgButton(hDlg, IDC_EMAIL, (user->flags & F_EMAIL));
			CheckDlgButton(hDlg, IDC_HOLDMAIL, (user->flags & F_HOLDMAIL));
			CheckDlgButton(hDlg, ALLOW_BULLS, (user->flags & F_NOBULLS) == 0);	// Node inverted flag
			CheckDlgButton(hDlg, IDC_NTSMPS, (user->flags & F_NTSMPS));
			CheckDlgButton(hDlg, IDC_APRSMFOR, (user->flags & F_APRSMFOR));
			CheckDlgButton(hDlg, IDC_POLLRMS, (user->flags & F_POLLRMS));
			CheckDlgButton(hDlg, IDC_SYSOP_IN_LM, (user->flags & F_SYSOP_IN_LM));
			CheckDlgButton(hDlg, RMS_EXPRESS_USER, (user->flags & F_Temp_B2_BBS));
			CheckDlgButton(hDlg, NO_WINLINKdotORG, (user->flags & F_NOWINLINK));
			CheckDlgButton(hDlg, IDC_RMSREDIRECT, (user->flags & F_RMSREDIRECT));
			
			EnableWindow(GetDlgItem(hDlg, IDC_SYSOP_IN_LM), user->flags & F_SYSOP);

			SetDlgItemInt(hDlg, CONN_IN, ConnectsIn, FALSE);
			SetDlgItemInt(hDlg, CONN_OUT, ConnectsOut, FALSE);
			SetDlgItemInt(hDlg, MSGS_IN, MsgsReceived, FALSE);
			SetDlgItemInt(hDlg, MSGS_OUT, MsgsSent, FALSE);
			SetDlgItemInt(hDlg, REJECTS_IN, MsgsRejectedIn, FALSE);
			SetDlgItemInt(hDlg, REJECTS_OUT, MsgsRejectedOut, FALSE);
			SetDlgItemInt(hDlg, BYTES_IN, BytesForwardedIn, FALSE);
			SetDlgItemInt(hDlg, BYTES_OUT, BytesForwardedOut, FALSE);

/*
			for (i = 0; i < 3; i++)
			{
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_RESETCONTENT,0 , 0);
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)" ");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"1");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"2");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"3");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"4");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"5");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"6");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"7");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"8");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"9");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"10");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"11");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"12");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"13");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"14");
			SendDlgItemMessage(hDlg, RMS_SSID1 + i, CB_ADDSTRING,0 , (LPARAM)"15");
			}
*/			
			SendDlgItemMessage(hDlg, RMS_SSID1, WM_SETTEXT, 0, (LPARAM)"-");
			SendDlgItemMessage(hDlg, RMS_SSID2, WM_SETTEXT, 0, (LPARAM)"-");
			SendDlgItemMessage(hDlg, RMS_SSID3, WM_SETTEXT, 0, (LPARAM)"-");

			i = 0;
			for (s = 0; s < 16; s++)
			{
				if (user->RMSSSIDBits & (1 << s))
				{
					if (s)
						sprintf(SSID, "%d", s);
					else
						SSID[0] = 0;

					SendDlgItemMessage(hDlg, RMS_SSID1 + i++, WM_SETTEXT, 0, (LPARAM)SSID);
//					SendDlgItemMessage(hDlg, RMS_SSID1 + i++, CB_SETCURSEL, s, 0);
					if (i == 3)
						break;
				}
			}

			tm = gmtime((time_t *)&user->TimeLastConnected);	
			Dateptr = asctime(tm);
			strcpy(Date, Dateptr);

			SetDlgItemText(hDlg, LASTCONNECT, Date);

			i = (user->flags >> 28);
			sprintf(SSID, "%d", i);

			if (i == 0)
				SSID[0] = 0;

			SendDlgItemMessage(hDlg, IDC_APRSSSID, WM_SETTEXT, 0, (LPARAM)SSID);

			return 0;
		}
	}

	// Typing in new user

	CurrentConfigIndex = -1;

	SetDlgItemText(hDlg, IDC_NAME, "");
	SetDlgItemText(hDlg, IDC_PASSWORD, "");
	SetDlgItemText(hDlg, IDC_CMSPASS, "");
	
	SetDlgItemText(hDlg, IDC_QTH, "");
	SetDlgItemText(hDlg, IDC_UZIP, "");
	SetDlgItemText(hDlg, IDC_HOMEBBS, "");
	SetDlgItemInt(hDlg, IDC_LASTLISTED, LatestMsg, FALSE);

	CheckDlgButton(hDlg, IDC_SYSOP, FALSE);
	CheckDlgButton(hDlg, IDC_BBSFLAG, FALSE);
	CheckDlgButton(hDlg, IDC_PMSFLAG, FALSE);
	CheckDlgButton(hDlg, IDC_EXPERT, FALSE);
	CheckDlgButton(hDlg, IDC_EXCLUDED, FALSE);

	return 0;
}

VOID Do_Add_User(HWND hDlg)
{
	struct UserInfo * user;
	int n;

	SendDlgItemMessage(hDlg, IDC_USER, WM_GETTEXT, 19, (LPARAM)(LPCTSTR)&CurrentConfigCall);
	
	if (LookupCall(CurrentConfigCall))
		sprintf(InfoBoxText, "User %s already exists", CurrentConfigCall);
	else if	((strlen(CurrentConfigCall) < 3) || (strlen(CurrentConfigCall) > MAXUSERNAMELEN))
		sprintf(InfoBoxText, "User %s is invalid", CurrentConfigCall);
	else
	{
		user = AllocateUserRecord(CurrentConfigCall);
		user->Temp = zalloc(sizeof (struct TempUserInfo));

		CurrentConfigIndex=NumberofUsers;
		Do_Save_User(hDlg, FALSE);
		sprintf(InfoBoxText, "User %s added and info saved", CurrentConfigCall);
	}	
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	SendDlgItemMessage(hDlg, IDC_USER, CB_RESETCONTENT, 0, 0);

	for (n = 1; n <= NumberofUsers; n++)
	{
		SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
	} 

	return;

}


VOID Do_Delete_User(HWND hDlg)
{
	struct UserInfo * user;
	int n;


	if (CurrentConfigIndex == -1)
	{
		sprintf(InfoBoxText, "Please select a user to delete");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	user = UserRecPtr[CurrentConfigIndex];

	if (_stricmp(CurrentConfigCall, user->Call) != 0)
	{
		sprintf(InfoBoxText, "Inconsistancy detected - user not deleted");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	for (n = CurrentConfigIndex; n < NumberofUsers; n++)
	{
		UserRecPtr[n] = UserRecPtr[n+1];		// move down all following entries
	}
	
	NumberofUsers--;

	SendDlgItemMessage(hDlg, IDC_USER, CB_RESETCONTENT, 0, 0);

	for (n = 1; n <= NumberofUsers; n++)
	{
		SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
	} 

	sprintf(InfoBoxText, "User %s deleted", user->Call);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	if (user->flags & F_BBS)	// was a BBS?
		DeleteBBS(user);
	
	free(user);

	// Position to same place in list

	SendDlgItemMessage(hDlg, IDC_USER, CB_SETCURSEL, Sel, 0);
	Do_User_Sel_Changed(hDlg);

	return;

}
VOID Do_Save_User(HWND hDlg, BOOL ShowBox)
{
	struct UserInfo * user;
	BOOL OK;
	char RMSSSID[10];
	unsigned int SSID;

	if (CurrentConfigIndex == -1)
	{
		sprintf(InfoBoxText, "Please select a user to save");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	user = UserRecPtr[CurrentConfigIndex];

	if (strcmp(CurrentConfigCall, user->Call) != 0)
	{
		sprintf(InfoBoxText, "Inconsistancy detected - information not saved");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	GetDlgItemText(hDlg, IDC_NAME, user->Name, 17);
	GetDlgItemText(hDlg, IDC_PASSWORD, user->pass, 12);
	GetDlgItemText(hDlg, IDC_QTH, user->Address, 60);
	GetDlgItemText(hDlg, IDC_UZIP, user->ZIP, 8);
	GetDlgItemText(hDlg, IDC_HOMEBBS, user->HomeBBS, 40);
	GetDlgItemText(hDlg, IDC_CMSPASS, user->CMSPass, 15);

	user->lastmsg = GetDlgItemInt(hDlg, IDC_LASTLISTED, &OK, FALSE);

	if (IsDlgButtonChecked(hDlg, IDC_BBSFLAG))
	{
		// If BBS Flag has changed, must set up or delete forwarding info

		if ((user->flags & F_BBS) == 0)
		{
			// New BBS

			if(SetupNewBBS(user))
			{
				user->flags |= F_BBS;
				user->flags &= ~F_Temp_B2_BBS;			// Clear RMS Express User
				CheckDlgButton(hDlg, RMS_EXPRESS_USER, (user->flags & F_Temp_B2_BBS));
			}
			else
			{
				// Failed - too many bbs's defined

				sprintf(InfoBoxText, "Cannot set user to be a BBS - you already have 80 BBS's defined");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				user->flags &= ~F_BBS;
				CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
			}
		}
	}
	else
	{
		if (user->flags & F_BBS)
		{
			//was a BBS

			user->flags &= ~F_BBS;
			DeleteBBS(user);
		}
	}
			
	if (IsDlgButtonChecked(hDlg, IDC_PMSFLAG))
		user->flags |= F_PMS; else user->flags &= ~F_PMS;

	if (IsDlgButtonChecked(hDlg, IDC_EXPERT))
		user->flags |= F_Expert; else user->flags &= ~F_Expert;

	if (IsDlgButtonChecked(hDlg, IDC_EXCLUDED))
		user->flags |= F_Excluded; else user->flags &= ~F_Excluded;

	if (IsDlgButtonChecked(hDlg, IDC_SYSOP))
		user->flags |= F_SYSOP; else user->flags &= ~F_SYSOP;

	if (IsDlgButtonChecked(hDlg, IDC_EMAIL))
		user->flags |= F_EMAIL; else user->flags &= ~F_EMAIL;

	if (IsDlgButtonChecked(hDlg, IDC_HOLDMAIL))
		user->flags |= F_HOLDMAIL; else user->flags &= ~F_HOLDMAIL;

	if (IsDlgButtonChecked(hDlg, ALLOW_BULLS) == 0)
		user->flags |= F_NOBULLS; else user->flags &= ~F_NOBULLS;		// Note flag inverted

	if (IsDlgButtonChecked(hDlg, IDC_APRSMFOR))
		user->flags |= F_APRSMFOR; else user->flags &= ~F_APRSMFOR;
	
	if (IsDlgButtonChecked(hDlg, IDC_NTSMPS))
		user->flags |= F_NTSMPS; else user->flags &= ~F_NTSMPS;
	
	if (IsDlgButtonChecked(hDlg, IDC_RMSREDIRECT))
		user->flags |= F_RMSREDIRECT; else user->flags &= ~F_RMSREDIRECT;

	if (IsDlgButtonChecked(hDlg, IDC_POLLRMS))
		user->flags |= F_POLLRMS; else user->flags &= ~F_POLLRMS;

	if (IsDlgButtonChecked(hDlg, IDC_SYSOP_IN_LM))
		user->flags |= F_SYSOP_IN_LM; else user->flags &= ~F_SYSOP_IN_LM;

	if (IsDlgButtonChecked(hDlg, RMS_EXPRESS_USER))
		user->flags |= F_Temp_B2_BBS; else user->flags &= ~F_Temp_B2_BBS;

	if (IsDlgButtonChecked(hDlg, NO_WINLINKdotORG))
		user->flags |= F_NOWINLINK; else user->flags &= ~F_NOWINLINK;

//	if (user->flags & F_BBS)
//		user->flags &= ~F_Temp_B2_BBS;		// Can't be both


	user->RMSSSIDBits = 0;

	SendDlgItemMessage(hDlg, RMS_SSID1, WM_GETTEXT, 3, (LPARAM)(LPCTSTR)&RMSSSID);

	if (RMSSSID[0] != '-')
	{
		SSID = atoi(RMSSSID);
		user->RMSSSIDBits |= (1 << (SSID));
	}
	SendDlgItemMessage(hDlg, RMS_SSID2, WM_GETTEXT, 3, (LPARAM)(LPCTSTR)&RMSSSID);
	
	if (RMSSSID[0] != '-')
	{
		SSID = atoi(RMSSSID);
		user->RMSSSIDBits |= (1 << (SSID));
	}

	SendDlgItemMessage(hDlg, RMS_SSID3, WM_GETTEXT, 3, (LPARAM)(LPCTSTR)&RMSSSID);

	if (RMSSSID[0] != '-')
	{
		SSID = atoi(RMSSSID);
		user->RMSSSIDBits |= (1 << (SSID));
	}

	SendDlgItemMessage(hDlg, IDC_APRSSSID, WM_GETTEXT, 3, (LPARAM)(LPCTSTR)&RMSSSID);

	SSID = atoi(RMSSSID);
	SSID &= 15;

	user->flags &= 0x0fffffff;
	user->flags |= (SSID << 28);

	SaveUserDatabase();

	UpdateWPWithUserInfo(user);


	if (ShowBox)
	{
		sprintf(InfoBoxText, "User information saved");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
	}				
}

int compare(const void *arg1, const void *arg2);

int Do_Msg_Sel_Changed(HWND hDlg)
{
	// Update Msg display with newly selected Msg

	struct MsgInfo * Msg;
	char MsgnoText[10];
	int Msgno;
	int Sel = SendDlgItemMessage(hDlg, 0, LB_GETCURSEL, 0, 0);
	char Size[10];

	if (Sel != -1)
	{
		SendDlgItemMessage(hDlg, 0, LB_GETTEXT, Sel, (LPARAM)(LPCTSTR)&MsgnoText);
		Msgno = atoi(MsgnoText);
	}
	
	for (CurrentMsgIndex = 1; CurrentMsgIndex <= NumberofMessages; CurrentMsgIndex++)
	{
		Msg = MsgHddrPtr[CurrentMsgIndex];

		if (Msg->number == Msgno)
		{
			struct UserInfo * USER;
			int i = 0, n;
			UINT State;

			SetDlgItemText(hDlg, 6001, Msg->from);
			SetDlgItemText(hDlg, 6002, Msg->bid);
			SetDlgItemText(hDlg, 6003, Msg->to);
			SetDlgItemText(hDlg, EMAILFROM, Msg->emailfrom);
			SetDlgItemText(hDlg, 6004, Msg->via);
			SetDlgItemText(hDlg, 6005, Msg->title);
			sprintf(Size, "%d", Msg->length);

			SetDlgItemText(hDlg, 6018, FormatDateAndTime((time_t)Msg->datecreated, FALSE));
			SetDlgItemText(hDlg, 6019, FormatDateAndTime((time_t)Msg->datereceived, FALSE));
			SetDlgItemText(hDlg, 6021, FormatDateAndTime((time_t)Msg->datechanged, FALSE));
			SetDlgItemText(hDlg, 6020, Size);

			if (Msg->type  == 'B')
				SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, 0, 0);
			else if (Msg->type  == 'P')
				SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, 1, 0);
			else
				SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, 2, 0);

			switch(Msg->status)
			{
			case 'N': Sel = MSGSTATUS_N; break;
			case 'Y': Sel = MSGSTATUS_Y; break;
			case 'F': Sel = MSGSTATUS_F; break;
			case 'K': Sel = MSGSTATUS_K; break;
			case 'H': Sel = MSGSTATUS_H; break;
			case 'D': Sel = MSGSTATUS_D; break;
			case '$': Sel = MSGSTATUS_$; break;
			}

			// Get a sorted list of BBS records

			for (n = 1; n <= NumberofUsers; n++)
			{
				USER = UserRecPtr[n];

				if ((USER->flags & F_BBS) && USER->BBSNumber)
					MsgBBSList[i++] = USER;
			}

			qsort((void *)MsgBBSList, i, 4, compare );

			SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_SETCURSEL, Sel, 0);

			for (n = 0; n <= NBBBS; n++)
			{
				State = BST_INDETERMINATE;

				USER = MsgBBSList[n];
				
				if (USER)
				{
					if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)	
						if (check_fwd_bit(Msg->fbbs, USER->BBSNumber))
							State = BST_UNCHECKED;
					if (memcmp(Msg->forw, zeros, NBMASK) != 0)	
						if (check_fwd_bit(Msg->forw, USER->BBSNumber))
							State = BST_CHECKED;
				
					SetDlgItemText(hDlg, n + 25, USER->Call);
				}
				else
					SetDlgItemText(hDlg, n + 25, "");

				CheckDlgButton(hDlg, n + 25, State);	
			}

			return 0;
		}
	}

	CurrentMsgIndex = -1;

	return 0;
}

VOID Do_Save_Msg(HWND hDlg)
{
	struct MsgInfo * Msg;
	struct UserInfo * user;

	char status[2];
	int i, n, BBSNumber;
	BOOL toforward, forwarded;

	if (CurrentMsgIndex == -1)
	{
		sprintf(InfoBoxText, "Please select a message to save");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	Msg = MsgHddrPtr[CurrentMsgIndex];

	GetDlgItemText(hDlg, 6001, Msg->from, 7);
	GetDlgItemText(hDlg, 6002, Msg->bid, 13);
	GetDlgItemText(hDlg, 6003, Msg->to, 7);
	GetDlgItemText(hDlg, 6004, Msg->via, 41);
	GetDlgItemText(hDlg, 6005, Msg->title, 61);
	GetDlgItemText(hDlg, EMAILFROM, Msg->emailfrom, 41);

	GetDlgItemText(hDlg, IDC_MSGTYPE, status, 2);
	Msg->type = status[0];

	// Check each BBS to for Farwardind State

	for (i = 0; i < NBBBS; i++)
	{
		n = IsDlgButtonChecked(hDlg, i + 25);

		user = MsgBBSList[i];

		if (user)
		{
			BBSNumber = user->BBSNumber;	

//			if (BBSNumber == 31)
//				n = n;

			toforward = check_fwd_bit(Msg->fbbs, BBSNumber);
			forwarded = check_fwd_bit(Msg->forw, BBSNumber);

			if (n == BST_INDETERMINATE)
			{
				if ((!toforward) && (!forwarded))
				{
					// No Change
					continue;
				}
				else
				{
					clear_fwd_bit(Msg->fbbs, BBSNumber);
					if (toforward)
						user->ForwardingInfo->MsgCount--;

					clear_fwd_bit(Msg->forw, BBSNumber);
				}
			}
			else if (n == BST_UNCHECKED)
			{
				if (toforward)
				{
					// No Change
					continue;
				}
				else
				{
					set_fwd_bit(Msg->fbbs, BBSNumber);
					user->ForwardingInfo->MsgCount++;
					clear_fwd_bit(Msg->forw, BBSNumber);
					if (FirstMessageIndextoForward > CurrentMsgIndex)
						FirstMessageIndextoForward = CurrentMsgIndex;
				}
			}
			else if (n == BST_CHECKED)
			{
				if (forwarded)
				{
					// No Change
					continue;
				}
				else
				{
					set_fwd_bit(Msg->forw, BBSNumber);
					clear_fwd_bit(Msg->fbbs, BBSNumber);
					if (toforward)
						user->ForwardingInfo->MsgCount--;
				}
			}
		}
	}

	GetDlgItemText(hDlg, IDC_MSGSTATUS, status, 2);
	
	if (Msg->status != status[0])
	{
		// Need to take action if killing message

		Msg->status = status[0];
		if (status[0] == 'K')
			FlagAsKilled(Msg, FALSE);					// Clear forwarding bits
	}

	sprintf(InfoBoxText, "Message Updated");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	Msg->datechanged=time(NULL);

	SaveMessageDatabase();		

	Do_Msg_Sel_Changed(hDlg);				// Refresh
}

VOID SaveBBSConfig()
{
	BOOL OK1,OK2,OK3,OK4;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	
	GetDlgItemText(hwndDisplay, IDC_BBSCall, BBSName, 50);
	GetDlgItemText(hwndDisplay, IDC_SYSOPCALL, SYSOPCall, 50);
	GetDlgItemText(hwndDisplay, IDC_HRoute, HRoute, 50);
	GetDlgItemText(hwndDisplay, IDC_BaseDir, BaseDirRaw, MAX_PATH -1);
	EnableUI = IsDlgButtonChecked(hwndDisplay, IDC_ENABLEUI);
	RefuseBulls = IsDlgButtonChecked(hwndDisplay, IDC_REFUSEBULLS);
	OnlyKnown = IsDlgButtonChecked(hwndDisplay, IDC_KNOWNUSERS);
	MailForInterval = GetDlgItemInt(hwndDisplay, MAILFOR_MINS, &OK1, FALSE);
	SendSYStoSYSOPCall = IsDlgButtonChecked(hwndDisplay, IDC_SYSTOSYSOPCALL);
	SendBBStoSYSOPCall = IsDlgButtonChecked(hwndDisplay, IDC_BBSTOSYSOPCALL);
	DontHoldNewUsers = IsDlgButtonChecked(hwndDisplay, IDC_DONTHOLDNEW);
	ForwardToMe = IsDlgButtonChecked(hwndDisplay, IDC_FORWARDTOBBS);
	DontNeedHomeBBS = IsDlgButtonChecked(hwndDisplay, IDC_NOHOMEBBS);
	DontCheckFromCall = IsDlgButtonChecked(hwndDisplay, IDC_DONTCHECKFROM);
	AllowAnon = IsDlgButtonChecked(hwndDisplay, IDC_NONAME);
	UserCantKillT = !IsDlgButtonChecked(hwndDisplay, IDC_USERRKILLT);	// Reverse logic
	DefaultNoWINLINK = IsDlgButtonChecked(hwndDisplay, IDC_DEFAULTNOWINLINK);

	BBSApplNum = GetDlgItemInt(hwndDisplay, IDC_BBSAppl, &OK1, FALSE);
	MaxStreams = GetDlgItemInt(hwndDisplay, IDC_BBSStreams, &OK2, FALSE);
	POP3InPort = GetDlgItemInt(hwndDisplay, IDC_POP3Port, &OK3, FALSE);
	SMTPInPort = GetDlgItemInt(hwndDisplay, IDC_SMTPPort, &OK4, FALSE);
	NNTPInPort = GetDlgItemInt(hwndDisplay, IDC_NNTPPort, &OK3, FALSE);

	GetDlgItemText(hwndDisplay, IDC_AMPR, AMPRDomain, 50);
	SendAMPRDirect= IsDlgButtonChecked(hwndDisplay, IDC_FORWARDAMPR);
	RemoteEmail = IsDlgButtonChecked(hwndDisplay, IDC_REMOTEEMAIL);

	strlop(BBSName, '-');
	strlop(SYSOPCall, '-');

	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Warning - Program must be restarted for changes to be effective");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}
	

VOID SaveISPConfig()
{
	BOOL OK1,OK2,OK3;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
		
	ISP_Gateway_Enabled = IsDlgButtonChecked(hwndDisplay, IDC_ISP_Gateway_Enabled);

	SMTPAuthNeeded = IsDlgButtonChecked(hwndDisplay, ISP_SMTP_AUTH);

	ISPPOP3Interval = GetDlgItemInt(hwndDisplay, IDC_POP3Timer, &OK1, FALSE);

	GetDlgItemText(hwndDisplay, IDC_MyMailDomain, MyDomain, 50);

	GetDlgItemText(hwndDisplay, IDC_ISPSMTPName, ISPSMTPName, 50);
	ISPSMTPPort = GetDlgItemInt(hwndDisplay, IDC_ISPSMTPPort, &OK2, FALSE);

	GetDlgItemText(hwndDisplay, SMTP_EHELO, ISPEHLOName, 50);

	GetDlgItemText(hwndDisplay, IDC_ISPPOP3Name, ISPPOP3Name, 50);
	ISPPOP3Port = GetDlgItemInt(hwndDisplay, IDC_ISPPOP3Port, &OK3, FALSE);

	GetDlgItemText(hwndDisplay, IDC_ISPAccountName, ISPAccountName, 50);
	GetDlgItemText(hwndDisplay, IDC_ISPAccountPass, ISPAccountPass, 50);

	EncryptedPassLen = EncryptPass(ISPAccountPass, EncryptedISPAccountPass);

	SaveConfig(ConfigName);
	GetConfig(ConfigName);


	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}

VOID SaveFWDConfig(HWND hDlg)
{
	int OK, n;
	char BBSHA[50];

	ReaddressLocal = IsDlgButtonChecked(hDlg, IDC_READDRESSLOCAL);
	ReaddressReceived = IsDlgButtonChecked(hDlg, IDC_READDRESSRXED);
	WarnNoRoute = IsDlgButtonChecked(hDlg, IDC_WARNNOROUTE);
	Localtime = IsDlgButtonChecked(hDlg, IDC_USELOCALTIME);
	MaxTXSize = GetDlgItemInt(hDlg, IDC_MAXSEND, &OK, FALSE);
	MaxRXSize = GetDlgItemInt(hDlg, IDC_MAXRECV, &OK, FALSE);
	MaxAge = GetDlgItemInt(hDlg, IDC_MAXAGE, &OK, FALSE);
	SendPtoMultiple = IsDlgButtonChecked(hDlg, IDC_MULTIP);
	FOURCHARCONT = IsDlgButtonChecked(hDlg, IDC_FOURCHARCONTINENT);

	
	// Reinitialise Aliases

	n = 0;

	if (Aliases)
	{
		while(Aliases[n])
		{
			free(Aliases[n]->Dest);
			free(Aliases[n]);
			n++;
		}

		free(Aliases);
		Aliases = NULL;
		FreeList(AliasText);
	}

	AliasText = GetMultiLineDialogParam(hDlg, IDC_ALIAS);
	SetupFwdAliases();

	if (CurrentBBS)
	{
		struct	BBSForwardingInfo * ForwardingInfo = CurrentBBS->ForwardingInfo;

		ForwardingInfo->ATCalls = GetMultiLineDialogParam(hDlg, IDC_ATCALLS);
		ForwardingInfo->TOCalls = GetMultiLineDialogParam(hDlg, IDC_TOCALLS);
		ForwardingInfo->Haddresses = GetMultiLineDialogParam(hDlg, IDC_HROUTES);
		ForwardingInfo->HaddressesP = GetMultiLineDialogParam(hDlg, IDC_HROUTESP);
		ForwardingInfo->ConnectScript = GetMultiLineDialogParam(hDlg, IDC_CALL);
		ForwardingInfo->FWDTimes = GetMultiLineDialogParam(hDlg, IDC_FWDTIMES);


		ForwardingInfo->Enabled = IsDlgButtonChecked(hDlg, IDC_FWDENABLE);
		ForwardingInfo->ReverseFlag = IsDlgButtonChecked(hDlg, IDC_REVERSE);
		ForwardingInfo->AllowB2 = IsDlgButtonChecked(hDlg, IDC_USEB2);
		ForwardingInfo->PersonalOnly = IsDlgButtonChecked(hDlg, IDC_PERSONALONLY);
		ForwardingInfo->SendNew = IsDlgButtonChecked(hDlg, IDC_SENDNEW);
		ForwardingInfo->AllowB1 = IsDlgButtonChecked(hDlg, IDC_USEB1);
		ForwardingInfo->SendCTRLZ = IsDlgButtonChecked(hDlg, IDC_CTRLZ);
		ForwardingInfo->AllowBlocked = IsDlgButtonChecked(hDlg, IDC_BLOCKED);
		ForwardingInfo->AllowCompressed = IsDlgButtonChecked(hDlg, IDC_ALLOWCOMP);
		ForwardingInfo->FwdInterval = GetDlgItemInt(hDlg, IDC_FWDINT, &OK, FALSE);
		ForwardingInfo->RevFwdInterval = GetDlgItemInt(hDlg, IDC_REVFWDINT, &OK, FALSE);
		ForwardingInfo->MaxFBBBlockSize = GetDlgItemInt(hDlg, IDC_MAXBLOCK, &OK, FALSE);
		ForwardingInfo->ConTimeout = GetDlgItemInt(hDlg, IDC_CONTIMEOUT,&OK , FALSE);

		GetDlgItemText(hDlg, IDC_BBSHA, BBSHA, 50);
		if (ForwardingInfo->BBSHA)
			free(ForwardingInfo->BBSHA);
		ForwardingInfo->BBSHA = _strdup(BBSHA);
	}

	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	if (CurrentBBS)
		ReinitializeFWDStruct(CurrentBBS);	

	sprintf(InfoBoxText, "Forwarding information saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}

VOID CopyFwdConfig(HWND hDlg)
{
	char FromBBS[11] = "";
	struct UserInfo * OldBBS;

	if (CurrentBBS == NULL)
	{
		sprintf(InfoBoxText, "Please select a BBS to copy to");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	// Get call to copy from 

	GetDlgItemText(hDlg, COPYFROMCALL, FromBBS, 10);

	OldBBS = FindBBS(FromBBS);

	if (OldBBS == NULL)
	{
		sprintf(InfoBoxText, "BBS %s not found", FromBBS);
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	// Set current info from OldBBS

	SetForwardingPage(hDlg, OldBBS);			// moved to separate routine as also called from copy config

//	sprintf(InfoBoxText, "Forwarding information saved");
//	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}


VOID SaveMAINTConfigFromDialog()
{
	BOOL OK1;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	MaxMsgno = GetDlgItemInt(hwndDisplay, IDC_MAXMSG, &OK1, FALSE);

	if (MaxMsgno > 99000) MaxMsgno = 99000;

	BidLifetime = GetDlgItemInt(hwndDisplay, IDC_BIDLIFETIME, &OK1, FALSE);
	LogAge = GetDlgItemInt(hwndDisplay, IDC_LOGLIFETIME, &OK1, FALSE);
	UserLifetime = GetDlgItemInt(hwndDisplay, IDC_USERLIFETIME, &OK1, FALSE);
	MaintInterval = GetDlgItemInt(hwndDisplay, IDC_MAINTINTERVAL, &OK1, FALSE);
	MaintTime = GetDlgItemInt(hwndDisplay, IDC_MAINTTIME, &OK1, FALSE);
	PR = GetDlgItemFloat(hwndDisplay, IDM_PR, &OK1, FALSE);
	PUR = GetDlgItemFloat(hwndDisplay, IDM_PUR, &OK1, FALSE);
	PF = GetDlgItemFloat(hwndDisplay, IDM_PF, &OK1, FALSE);
	PNF = GetDlgItemFloat(hwndDisplay, IDM_PNF, &OK1, FALSE);
	BF = GetDlgItemInt(hwndDisplay, IDM_BF, &OK1, FALSE);
	BNF = GetDlgItemInt(hwndDisplay, IDM_BNF, &OK1, FALSE);
	NTSD = GetDlgItemInt(hwndDisplay, IDM_NTSD, &OK1, FALSE);
	NTSF = GetDlgItemInt(hwndDisplay, IDM_NTSF, &OK1, FALSE);
	NTSU = GetDlgItemInt(hwndDisplay, IDM_NTSU, &OK1, FALSE);
	DeletetoRecycleBin = IsDlgButtonChecked(hwndDisplay, IDC_DELETETORECYCLE);
	SuppressMaintEmail = IsDlgButtonChecked(hwndDisplay, IDC_MAINTNOMAIL);
	SaveRegDuringMaint = IsDlgButtonChecked(hwndDisplay, IDC_MAINTSAVEREG);
	OverrideUnsent = IsDlgButtonChecked(hwndDisplay, IDC_OVERRIDEUNSENT);
	SendNonDeliveryMsgs = IsDlgButtonChecked(hwndDisplay, IDC_MAINTNONDELIVERY);

	GetDlgItemText(hwndDisplay, IDM_LTFROM, LTFROMString, 2048);
	LTFROM = GetOverrideFromString(LTFROMString);

	GetDlgItemText(hwndDisplay, IDM_LTTO, LTTOString, 2048);
	LTTO = GetOverrideFromString(LTTOString);

	GetDlgItemText(hwndDisplay, IDM_LTAT, LTATString, 2048);
	LTAT = GetOverrideFromString(LTATString);
 
	// Calulate time to run Housekeeping
	{
		struct tm *tm;
		time_t now;

		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

		MaintClock = _mkgmtime(tm);

		while (MaintClock < now)
			MaintClock += MaintInterval * 3600;

		Debugprintf("Maint Clock %d NOW %d Time to HouseKeeping %d", MaintClock, now, MaintClock - now);
	}
	
	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}

VOID SaveWelcomeMsgs()
{
	char Value[10000];

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	GetDlgItemText(hwndDisplay, IDM_USERMSG, Value, 10000);

	free(WelcomeMsg);
	WelcomeMsg = _strdup(Value);
	
	GetDlgItemText(hwndDisplay, IDM_NEWUSERMSG, Value, 10000);

	free(NewWelcomeMsg);
	NewWelcomeMsg = _strdup(Value);
	
	GetDlgItemText(hwndDisplay, IDM_EXPERTUSERMSG, Value, 10000);

	free(ExpertWelcomeMsg);
	ExpertWelcomeMsg = _strdup(Value);
	
	GetDlgItemText(hwndDisplay, IDM_SIGNOFF, SignoffMsg, 99);

	if (SignoffMsg[0])
		if (SignoffMsg[strlen(SignoffMsg) - 1] != 13)
			strcat(SignoffMsg, "\r");

	TidyWelcomeMsg(&WelcomeMsg);
	TidyWelcomeMsg(&NewWelcomeMsg);
	TidyWelcomeMsg(&ExpertWelcomeMsg);

		// redisplay, in case tidy has changed them

	SetDlgItemText(hwndDisplay, IDM_USERMSG, WelcomeMsg);
	SetDlgItemText(hwndDisplay, IDM_NEWUSERMSG, NewWelcomeMsg);
	SetDlgItemText(hwndDisplay, IDM_EXPERTUSERMSG, ExpertWelcomeMsg);


	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
}

VOID SavePrompts()
{
	char Value[10000];

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	GetDlgItemText(hwndDisplay, IDM_USERMSG, Value, 10000);

	free(Prompt);
	Prompt = _strdup(Value);
	
	GetDlgItemText(hwndDisplay, IDM_NEWUSERMSG, Value, 10000);

	free(NewPrompt);
	NewPrompt = _strdup(Value);
	
	GetDlgItemText(hwndDisplay, IDM_EXPERTUSERMSG, Value, 10000);

	free(ExpertPrompt);
	ExpertPrompt = _strdup(Value);
	
	TidyPrompts();

	// redisplay, in case tidy has changed them

	SetDlgItemText(hwndDisplay, IDM_USERMSG, Prompt);
	SetDlgItemText(hwndDisplay, IDM_NEWUSERMSG, NewPrompt);
	SetDlgItemText(hwndDisplay, IDM_EXPERTUSERMSG, ExpertPrompt);

	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
}


VOID SaveWPConfig(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	SendWP = IsDlgButtonChecked(hwndDisplay, IDC_SENDWP);
	SendWPType = SendDlgItemMessage(hwndDisplay, IDC_WPTYPE, CB_GETCURSEL, 0, 0);
	FilterWPBulls = IsDlgButtonChecked(hwndDisplay, IDC_FILTERWPB);

	SendWPAddrs = GetMultiLineDialogParam(hDlg, IDC_WPTO);

	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
}


VOID SaveFilters(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	RejFrom = GetMultiLineDialogParam(hDlg, IDC_REJFROM);
	RejTo = GetMultiLineDialogParam(hDlg, IDC_REJTO);
	RejAt = GetMultiLineDialogParam(hDlg, IDC_REJAT);
	RejBID = GetMultiLineDialogParam(hDlg, IDC_REJBID);

	HoldFrom = GetMultiLineDialogParam(hDlg, IDC_HOLDFROM);
	HoldTo = GetMultiLineDialogParam(hDlg, IDC_HOLDTO);
	HoldAt = GetMultiLineDialogParam(hDlg, IDC_HOLDAT);
	HoldBID = GetMultiLineDialogParam(hDlg, IDC_HOLDBID);

	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	sprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}


VOID * GetMultiLineDialogParam(HWND hDialog, int DLGItem)
{
	char Text[32768];
	char Val[32768];
	char * ptr1, * ptr2;
	char * MultiString = NULL;
	const char * ptr;
	int Count = 0;
	char ** Value;

	int Len = GetDlgItemText(hDialog, DLGItem, Text, 10000);

	// replace crlf with '|'

	if (Text[strlen(Text)-1] != '\n')			// no terminating crlf?
		strcat(Text, "\r\n");

	ptr1 = Text;
	ptr2 = Val;
		
	while (*ptr1)
	{
		if (*ptr1 == '\r')
		{
			while (*(ptr1+2) == '\r')			// Blank line
				ptr1+=2;

			*++ptr1 = '|';
		}
		*ptr2++= *ptr1++;
	}

	*ptr2++ = 0;

	Value = zalloc(4);				// always NULL entry on end even if no values
	Value[0] = NULL;

	ptr = Val;
	
	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, '|');
			
		if (ptr1)
			*(ptr1++) = 0;

		if (ptr[0] == 0)		// Just had a | (empty string)
			break;

		Value = realloc(Value, (Count+2) * sizeof(void *));
			
		Value[Count++] = _strdup(ptr);
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}

BOOL GetConfigFromRegistry()
{
	HKEY hKey=0;
	int retCode,Type,Vallen, i;
	char Size[80];
	char * ptr;

	// Get Config From Registry

	sprintf(BaseDirRaw, "%s/BPQMailChat", GetBPQDirectory());

	retCode = RegOpenKeyEx (REGTREE,
                 "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, KEY_ALL_ACCESS, &hKey);

	if (retCode != ERROR_SUCCESS)
		return FALSE;

	{
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"Streams",0,			
			(ULONG *)&Type,(UCHAR *)&MaxStreams,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"BBSApplNum",0,			
			(ULONG *)&Type,(UCHAR *)&BBSApplNum,(ULONG *)&Vallen);
		
		Vallen=4;
		RegQueryValueEx(hKey, "EnableUI", 0, &Type, (UCHAR *)&EnableUI, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey, "MailForInterval", 0, &Type, (UCHAR *)&MailForInterval, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey, "RefuseBulls", 0, &Type, (UCHAR *)&RefuseBulls, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"SendSYStoSYSOPCall",0,			
			(ULONG *)&Type,(UCHAR *)&SendSYStoSYSOPCall,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"SendBBStoSYSOPCall",0,			
			(ULONG *)&Type,(UCHAR *)&SendBBStoSYSOPCall,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"DontHoldNewUsers",0,			
			(ULONG *)&Type,(UCHAR *)&DontHoldNewUsers,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"ForwardToMe",0,			
			(ULONG *)&Type,(UCHAR *)&ForwardToMe,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"AllowAnon",0,			
			(ULONG *)&Type,(UCHAR *)&AllowAnon,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"DontNeedHomeBBS",0,			
			(ULONG *)&Type,(UCHAR *)&DontNeedHomeBBS,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"MaxTXSize",0,			
			(ULONG *)&Type,(UCHAR *)&MaxTXSize,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"MaxRXSize",0,			
			(ULONG *)&Type,(UCHAR *)&MaxRXSize,(ULONG *)&Vallen);

		AliasText = RegGetMultiStringValue(hKey,  "FWD Aliases");

		Vallen=4;
		RegQueryValueEx(hKey, "Readdress Local",0,			
			(ULONG *)&Type,(UCHAR *)&ReaddressLocal, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey, "Readdress Received",0,			
			(ULONG *)&Type,(UCHAR *)&ReaddressReceived, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey, "Warn No Route",0,			
			(ULONG *)&Type,(UCHAR *)&WarnNoRoute, &Vallen);

		Vallen=4;
		RegQueryValueEx(hKey, "Localtime",0,			
			(ULONG *)&Type,(UCHAR *)&Localtime, &Vallen);

		Vallen=100;
		retCode += RegQueryValueEx(hKey, "BBSName",0 , &Type, (UCHAR *)&BBSName, &Vallen);

		sprintf(SignoffMsg, "73 de %s\r", BBSName);	// Default

		Vallen=100;
		retCode += RegQueryValueEx(hKey, "MailForText",0 , &Type, (UCHAR *)&MailForText, &Vallen);

		Vallen=100;
		retCode += RegQueryValueEx(hKey,"SYSOPCall",0,			
			(ULONG *)&Type,(UCHAR *)&SYSOPCall,(ULONG *)&Vallen);
				
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"H-Route",0,			
			(ULONG *)&Type,(UCHAR *)&HRoute,(ULONG *)&Vallen);

		Vallen=MAX_PATH;
		retCode += RegQueryValueEx(hKey,"BaseDir",0,			
			(ULONG *)&Type,(UCHAR *)&BaseDirRaw,(ULONG *)&Vallen);

		ptr = &BaseDirRaw[strlen(BaseDirRaw) -1];

		if (*ptr == '\\' || *ptr == '/')
			*ptr = 0;

		ExpandEnvironmentStrings(BaseDirRaw, BaseDir, MAX_PATH);
		// Get length of Chatnodes String
				
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"SMTPPort",0,			
			(ULONG *)&Type,(UCHAR *)&SMTPInPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"POP3Port",0,			
			(ULONG *)&Type,(UCHAR *)&POP3InPort,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"NNTPPort",0,			
			(ULONG *)&Type,(UCHAR *)&NNTPInPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"SMTPGatewayEnabled",0,			
			(ULONG *)&Type,(UCHAR *)&ISP_Gateway_Enabled,(ULONG *)&Vallen);

		Vallen=4;

		Vallen=4;
		RegQueryValueEx(hKey,"RemoteEmail",0,			
			(ULONG *)&Type,(UCHAR *)&RemoteEmail,(ULONG *)&Vallen);

		Vallen=4;

		retCode += RegQueryValueEx(hKey,"POP3 Polling Interval",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Interval,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"MyDomain",0,			
			(ULONG *)&Type,(UCHAR *)&MyDomain,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPSMTPName",0,			
			(ULONG *)&Type,(UCHAR *)&ISPSMTPName,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPPOP3Name",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Name,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ISPSMTPPort",0,			
			(ULONG *)&Type,(UCHAR *)&ISPSMTPPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ISPPOP3Port",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Port,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPAccountName",0,			
			(ULONG *)&Type,(UCHAR *)&ISPAccountName,(ULONG *)&Vallen);

		EncryptedPassLen=50;
		retCode += RegQueryValueEx(hKey,"ISPAccountPass",0,			
			(ULONG *)&Type,(UCHAR *)&EncryptedISPAccountPass,(ULONG *)&EncryptedPassLen);

		DecryptPass(EncryptedISPAccountPass, ISPAccountPass, EncryptedPassLen);

		Vallen=4;
		RegQueryValueEx(hKey,"AuthenticateSMTP",0,			
			(ULONG *)&Type,(UCHAR *)&SMTPAuthNeeded,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_BBS",0,			
			(ULONG *)&Type,(UCHAR *)&LogBBS,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_TCP",0,			
			(ULONG *)&Type,(UCHAR *)&LogTCP,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_CHAT",0,			
			(ULONG *)&Type,(UCHAR *)&LogCHAT,(ULONG *)&Vallen);

		Vallen=80;
		RegQueryValueEx(hKey,"MonitorSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MonitorRect.left,&MonitorRect.right,&MonitorRect.top,&MonitorRect.bottom);

		Vallen=80;
		RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);

		// Get Welcome Messages

		Vallen=0;
		
		RegQueryValueEx(hKey,"WelcomeMsg",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			WelcomeMsg = malloc(Vallen);
			RegQueryValueEx(hKey,"WelcomeMsg",0, (ULONG *)&Type, WelcomeMsg, (ULONG *)&Vallen);
		}
		else
			WelcomeMsg = _strdup("Hello $I. Latest Message is $L, Last listed is $Z\r\n");

		RegQueryValueEx(hKey,"NewUserWelcomeMsg",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			NewWelcomeMsg = malloc(Vallen);
			RegQueryValueEx(hKey,"NewUserWelcomeMsg",0, (ULONG *)&Type, NewWelcomeMsg, (ULONG *)&Vallen);
		}
		else
			
			NewWelcomeMsg = _strdup("Hello $I. Latest Message is $L, Last listed is $Z\r\n");

		Vallen=0;
		
		RegQueryValueEx(hKey,"ExpertWelcomeMsg",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			ExpertWelcomeMsg = malloc(Vallen);
			RegQueryValueEx(hKey,"ExpertWelcomeMsg",0, (ULONG *)&Type, ExpertWelcomeMsg, (ULONG *)&Vallen);
		}
		else
			ExpertWelcomeMsg = _strdup("");

		Vallen = 99;
		RegQueryValueEx(hKey,"SignoffMsg",0, (ULONG *)&Type, &SignoffMsg[0], (ULONG *)&Vallen);

		// Get Prompts

		Vallen=0;
		
		RegQueryValueEx(hKey,"Prompt",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			Prompt = malloc(Vallen + 3);
			RegQueryValueEx(hKey,"Prompt",0, (ULONG *)&Type, Prompt, (ULONG *)&Vallen);
		}
		else
		{
			Prompt = malloc(20);
			sprintf(Prompt, "de %s>\r\n", BBSName);
		}

		RegQueryValueEx(hKey,"NewUserPrompt",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			NewPrompt = malloc(Vallen + 3);
			RegQueryValueEx(hKey,"NewUserPrompt",0, (ULONG *)&Type, NewPrompt, (ULONG *)&Vallen);
		}
		else
		{
			NewPrompt = malloc(20);
			sprintf(NewPrompt, "de %s>\r\n", BBSName);
		}

		RegQueryValueEx(hKey,"ExpertPrompt",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			ExpertPrompt = malloc(Vallen);
			RegQueryValueEx(hKey,"ExpertPrompt",0, (ULONG *)&Type, ExpertPrompt, (ULONG *)&Vallen);
		}
		else
		{
			ExpertPrompt = malloc(20);
			sprintf(ExpertPrompt, "de %s>\r\n", BBSName);
		}

		TidyPrompts();

		RegQueryValueEx(hKey,"NewUserWelcomeMsg",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			NewWelcomeMsg = malloc(Vallen);
			RegQueryValueEx(hKey,"NewUserWelcomeMsg",0, (ULONG *)&Type, NewWelcomeMsg, (ULONG *)&Vallen);
		}
		else
			
			NewWelcomeMsg = _strdup("Hello $I. Latest Message is $L, Last listed is $Z\r\n");

		Vallen=0;
		
		RegQueryValueEx(hKey,"ExpertWelcomeMsg",0,	(ULONG *)&Type, NULL, (ULONG *)&Vallen);

		if (Vallen)
		{
			ExpertWelcomeMsg = malloc(Vallen);
			RegQueryValueEx(hKey,"ExpertWelcomeMsg",0, (ULONG *)&Type, ExpertWelcomeMsg, (ULONG *)&Vallen);
		}
		else
			ExpertWelcomeMsg = _strdup("");

		Vallen=80;


		RejFrom = RegGetMultiStringValue(hKey,  "RejFrom");
		RejTo = RegGetMultiStringValue(hKey,  "RejTo");
		RejAt = RegGetMultiStringValue(hKey,  "RejAt");

		HoldFrom = RegGetMultiStringValue(hKey,  "HoldFrom");
		HoldTo = RegGetMultiStringValue(hKey,  "HoldTo");
		HoldAt = RegGetMultiStringValue(hKey,  "HoldAt");

		// Send WP Params

		Vallen=4;
		RegQueryValueEx(hKey, "SendWP", 0, &Type, (UCHAR *)&SendWP, &Vallen);

		Vallen=10;
		RegQueryValueEx(hKey,"SendWPTO",0, &Type, &SendWPTO[0],&Vallen);

		Vallen=80;
		RegQueryValueEx(hKey,"SendWPVIA",0,	&Type,&SendWPVIA[0],&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"SendWPType",0, &Type, (UCHAR *)&SendWPType, &Vallen);


		if (RegQueryValueEx(hKey,"Version",0, (ULONG *)&Type, (UCHAR *)&Size, (ULONG *)&Vallen) == 0)
			sscanf(Size,"%d,%d,%d,%d", &LastVer[0], &LastVer[1], &LastVer[2], &LastVer[3]);

/*
		if ((LastVer[3] != Ver[3]) || (LastVer[2] != Ver[2]) ||
			(LastVer[1] != Ver[1]) || (LastVer[0] != Ver[0]))
		{
			// New Version Detected

			if (LastVer[0] == 0)
			{
				// Pre Version Checking

				MessageBox(NULL, "WARNING - This seems to be the first time you have run this version.\r\n"
					"Forwarding has changed significantly. Please read the docs and make the necessary\r\n"
					"changes to Forwarding Config. The Software will try to fill in the BBS HA fields from the WP\r\n"
					"Database, but check them, and complete the  new 'Hierarchical Routes (Flood Bulls)' field.\r\n"
					"Network access has been  disabled by setting BBS Streams to zero to prevent messages\r\n"
					"being lost or incorrecly forwarded. Once you are happy with the forwarding config\r\n"
					"you can reset BBS Streams.",
						"BPQMailChat", MB_ICONINFORMATION);

				MaxStreams = 0;
				
				RegSetValueEx(hKey, "Streams", 0, REG_DWORD,(BYTE *)&MaxStreams, 4);

			}
		}
*/
		RegCloseKey(hKey);

		for (i=1; i<=32; i++)
		{
			int retCode;
			char Key[100];
			
			sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\UIPort%d", i);

			retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

			if (retCode == ERROR_SUCCESS)
			{
				Vallen=4;
				RegQueryValueEx(hKey,"Enabled",0,			
					(ULONG *)&Type,(UCHAR *)&UIEnabled[i],(ULONG *)&Vallen);

				UIMF[i] = UIEnabled[i];		// Defaults
				UIHDDR[i] = UIEnabled[i];

				Vallen=4;
				RegQueryValueEx(hKey,"SendMF",0,			
					(ULONG *)&Type,(UCHAR *)&UIMF[i],(ULONG *)&Vallen);
		
				Vallen=4;
				RegQueryValueEx(hKey,"SendHDDR",0,			
					(ULONG *)&Type,(UCHAR *)&UIHDDR[i],(ULONG *)&Vallen);

				Vallen=4;
				RegQueryValueEx(hKey,"SendNull",0,			
					(ULONG *)&Type,(UCHAR *)&UINull[i],(ULONG *)&Vallen);

				Vallen=0;
				RegQueryValueEx(hKey,"Digis",0,			
					(ULONG *)&Type, NULL, (ULONG *)&Vallen);

				if (Vallen)
				{
					UIDigi[i] = malloc(Vallen);
					RegQueryValueEx(hKey,"Digis",0,			
						(ULONG *)&Type, UIDigi[i], (ULONG *)&Vallen);
				}

			//	retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ,(BYTE *)UIDigi[i], strlen(UIDigi[i]));

				RegCloseKey(hKey);
			}
		}
	
		retCode += RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\Housekeeping",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);
		
		if (retCode == ERROR_SUCCESS)
		{
			Vallen=4;
			RegQueryValueEx(hKey,"LastHouseKeepingTime",0,			
			(ULONG *)&Type,(UCHAR *)&LastHouseKeepingTime,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"LastTrafficTime",0,			
			(ULONG *)&Type,(UCHAR *)&LastTrafficTime,(ULONG *)&Vallen);


			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaxMsgno",0,			
			(ULONG *)&Type,(UCHAR *)&MaxMsgno,(ULONG *)&Vallen);

			if (MaxMsgno > 99000) MaxMsgno = 99000;

			Vallen=4;
			RegQueryValueEx(hKey,"LogLifetime",0,			
			(ULONG *)&Type,(UCHAR *)&LogAge,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BidLifetime",0,			
			(ULONG *)&Type,(UCHAR *)&BidLifetime,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"UserLifetime",0,			
			(ULONG *)&Type,(UCHAR *)&UserLifetime,(ULONG *)&Vallen);

	
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaintInterval",0,			
			(ULONG *)&Type,(UCHAR *)&MaintInterval,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaintTime",0,			
			(ULONG *)&Type,(UCHAR *)&MaintTime,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PR",0,			
			(ULONG *)&Type,(UCHAR *)&PR,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PUR",0,			
			(ULONG *)&Type,(UCHAR *)&PUR,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PF",0,			
			(ULONG *)&Type,(UCHAR *)&PF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PNF",0,			
			(ULONG *)&Type,(UCHAR *)&PNF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BF",0,			
			(ULONG *)&Type,(UCHAR *)&BF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BNF",0,			
			(ULONG *)&Type,(UCHAR *)&BNF,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"NTSD",0,			
			(ULONG *)&Type,(UCHAR *)&NTSD,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"NTSU",0,			
			(ULONG *)&Type,(UCHAR *)&NTSU,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"NTSF",0,			
			(ULONG *)&Type,(UCHAR *)&NTSF,(ULONG *)&Vallen);

//			Vallen=4;
//			retCode += RegQueryValueEx(hKey, "AP", 0,			
//				(ULONG *)&Type,(UCHAR *)&AP,(ULONG *)&Vallen);
				
//			Vallen=4;
//			retCode += RegQueryValueEx(hKey, "AB", 0,			
//				(ULONG *)&Type,(UCHAR *)&AB,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey, "DeletetoRecycleBin", 0,			
				(ULONG *)&Type,(UCHAR *)&DeletetoRecycleBin,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey, "SuppressMaintEmail", 0,			
				(ULONG *)&Type,(UCHAR *)&SuppressMaintEmail,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey, "MaintSaveReg", 0,			
				(ULONG *)&Type,(UCHAR *)&SaveRegDuringMaint,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey, "OverrideUnsent", 0,			
				(ULONG *)&Type,(UCHAR *)&OverrideUnsent,(ULONG *)&Vallen);

			Vallen=4;
			RegQueryValueEx(hKey, "SendNonDeliveryMsgs", 0,			
				(ULONG *)&Type,(UCHAR *)&SendNonDeliveryMsgs,(ULONG *)&Vallen);

			LTFROM = RegGetOverrides(hKey, "LTFROM");
			LTTO = RegGetOverrides(hKey, "LTTO");
			LTAT = RegGetOverrides(hKey, "LTAT");
		}

		return TRUE;
	}
}


INT_PTR CALLBACK UserEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, n;
		
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (n = 1; n <= NumberofUsers; n++)
		{
			SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
		} 

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

		Command = LOWORD(wParam);

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;


		case IDC_USER:

			// User Selection Changed

			Do_User_Sel_Changed(hDlg);

			return TRUE;


		case IDC_ADDUSER:

			Do_Add_User(hDlg);
			return TRUE;

		case IDC_DELETEUSER:

			Do_Delete_User(hDlg);
			return TRUE;

		case IDC_SAVEUSER:

			Do_Save_User(hDlg, TRUE);
			return TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MsgEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, n;
	char msgno[20];
	struct MsgInfo * Msg;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (n = NumberofMessages; n >= 1; n--)
		{
			sprintf_s(msgno, sizeof(msgno), "%d", MsgHddrPtr[n]->number);
			SendDlgItemMessage(hDlg, 0, LB_ADDSTRING, 0, (LPARAM)msgno);
		} 

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "B");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "P");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "T");

		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "N");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "Y");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "F");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "K");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "H");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "D");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "$");

		CheckDlgButton(hDlg,205, BST_INDETERMINATE);
		CheckDlgButton(hDlg,206, BST_UNCHECKED);
		CheckDlgButton(hDlg,207, BST_CHECKED);

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

		Command = LOWORD(wParam);

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case 0:

			// Msg Selection Changed

			Do_Msg_Sel_Changed(hDlg);

			return TRUE;

		case IDC_EDITTEXT:

			if (CurrentMsgIndex == -1)
			{
				sprintf(InfoBoxText, "Please select a message to Edit");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			if (SendDlgItemMessage(hDlg, 0, LB_GETSELCOUNT, 0, 0) > 1)
			{
				sprintf(InfoBoxText, "Please select only one message");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			DialogBox(hInst, MAKEINTRESOURCE(IDD_EDITMSGTEXT), hDlg, EditMsgTextDialogProc);
			return TRUE;

		case IDC_SAVEMSG:

			if (SendDlgItemMessage(hDlg, 0, LB_GETSELCOUNT, 0, 0) > 1)
			{
				sprintf(InfoBoxText, "Please select only one message");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			Do_Save_Msg(hDlg);
			return TRUE;

		case IDC_EXPORT:
		{
			struct MsgInfo * Msg;
			char FileName[MAX_PATH] = "Export.out";
			OPENFILENAME Ofn; 
			int Count;
			int * Indexes;
			int i;
			char MsgnoText[10];
			int Msgno;

			if (CurrentMsgIndex == -1)
			{
				sprintf(InfoBoxText, "Please select a message to Export");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			Count = SendDlgItemMessage(hDlg, 0, LB_GETSELCOUNT, 0, 0);

			Indexes = malloc(Count * sizeof(void *));

			SendDlgItemMessage(hDlg, 0, LB_GETSELITEMS , Count, (LPARAM)&Indexes[0]);

			memset(&Ofn, 0, sizeof(Ofn));
 
			Ofn.lStructSize = sizeof(OPENFILENAME); 
			Ofn.hInstance = hInst;
			Ofn.hwndOwner = hDlg; 
			Ofn.lpstrFilter = NULL; 
			Ofn.lpstrFile= FileName; 
			Ofn.nMaxFile = sizeof(FileName)/ sizeof(*FileName); 
			Ofn.lpstrFileTitle = NULL; 
			Ofn.nMaxFileTitle = 0; 
			Ofn.lpstrInitialDir = (LPSTR)NULL; 
			Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 
			Ofn.lpstrTitle = NULL;//; 

			if (GetSaveFileName(&Ofn))
			{
				FILE * Handle = fopen(FileName, "ab");

				if (Handle == NULL) 
				{
					sprintf(InfoBoxText, "Failed to open Export File %s", FileName);
					DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
					return TRUE;
				}

//				SetFilePointer(Handle, 0, 0, FILE_END);

				for (i = 0; i < Count; i++)
				{
					Msg = MsgHddrPtr[Indexes[i]];
					SendDlgItemMessage(hDlg, 0, LB_GETTEXT, Indexes[i], (LPARAM)(LPCTSTR)&MsgnoText);
					Msgno = atoi(MsgnoText);

					for (CurrentMsgIndex = 1; CurrentMsgIndex <= NumberofMessages; CurrentMsgIndex++)
					{
						Msg = MsgHddrPtr[CurrentMsgIndex];

						if (Msg->number == Msgno)
						{
							ForwardMessagetoFile(Msg, Handle);
							break;
						}
					}
				}
				fclose(Handle);
			}

			free(Indexes);

			return TRUE;
		}

		case IDC_SAVETOFILE:
		{
			struct MsgInfo * Msg;
			char * MailBuffer;
			char FileName[MAX_PATH] = "";
			int Files = 0;
			int BodyLen;
			char * ptr;
			HANDLE hFile = INVALID_HANDLE_VALUE;
			int WriteLen=0;
			OPENFILENAME Ofn; 
			char Hddr[1000];
			char FullTo[100];

			if (CurrentMsgIndex == -1)
			{
				sprintf(InfoBoxText, "Please select a message to Save");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			if (SendDlgItemMessage(hDlg, 0, LB_GETSELCOUNT, 0, 0) > 1)
			{
				sprintf(InfoBoxText, "Please select only one message");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}


			Msg = MsgHddrPtr[CurrentMsgIndex];

			MailBuffer = ReadMessageFile(Msg->number);
			BodyLen = Msg->length;

			sprintf(FileName, "MSG%05d.txt", Msg->number); 

			ptr = MailBuffer;

			if (_stricmp(Msg->to, "RMS") == 0)
				 sprintf(FullTo, "RMS:%s", Msg->via);
			else
			if (Msg->to[0] == 0)
				sprintf(FullTo, "smtp:%s", Msg->via);
			else
				strcpy(FullTo, Msg->to);

			sprintf(Hddr, "From: %s%s\r\nTo: %s\r\nType/Status: %c%c\r\nDate/Time: %s\r\nBid: %s\r\nTitle: %s\r\n\r\n",
				Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime((time_t)Msg->datecreated, FALSE), Msg->bid, Msg->title);

			if (Msg->B2Flags & B2Msg)
			{
			// Remove B2 Headers (up to the File: Line)
			
				char * bptr;
				bptr = strstr(ptr, "Body:");
				if (bptr)
				{
					BodyLen = atoi(bptr + 5);
					bptr = strstr(bptr, "\r\n\r\n");

					if (bptr)
						ptr = bptr+4;
				}
			}

			memset(&Ofn, 0, sizeof(Ofn));
 
			Ofn.lStructSize = sizeof(OPENFILENAME); 
			Ofn.hInstance = hInst;
			Ofn.hwndOwner = hDlg; 
			Ofn.lpstrFilter = NULL; 
			Ofn.lpstrFile= FileName; 
			Ofn.nMaxFile = sizeof(FileName)/ sizeof(*FileName); 
			Ofn.lpstrFileTitle = NULL; 
			Ofn.nMaxFileTitle = 0; 
			Ofn.lpstrInitialDir = (LPSTR)NULL; 
			Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 
			Ofn.lpstrTitle = NULL;//; 

			if (GetSaveFileName(&Ofn))
			{
				hFile = CreateFile(FileName,
					GENERIC_WRITE, FILE_SHARE_READ, NULL,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

				if (hFile != INVALID_HANDLE_VALUE)
				{
					WriteFile(hFile, Hddr, strlen(Hddr), &WriteLen, NULL);
					WriteFile(hFile, ptr, BodyLen, &WriteLen, NULL);
					CloseHandle(hFile);
				}
			}
			return TRUE;
		}

		

		case IDC_PRINTMSG:
		{
			int Count;
			int * Indexes;

			if (CurrentMsgIndex == -1)
			{
				sprintf(InfoBoxText, "Please select a message to Print");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
				return TRUE;
			}

			Count = SendDlgItemMessage(hDlg, 0, LB_GETSELCOUNT, 0, 0);

			Indexes = malloc(Count * sizeof(void *));

			SendDlgItemMessage(hDlg, 0, LB_GETSELITEMS , Count, (LPARAM)&Indexes[0]);

			PrintMessages(hDlg, Count, Indexes);

			free(Indexes);
	
			return TRUE;
		}
		case FILTER_FROM:
		case FILTER_TO:
		case FILTER_VIA:
		case FILTER_BID:

			if (HIWORD(wParam) == 0x300)
			{
				GetDlgItemText(hDlg, FILTER_FROM, Filter_FROM, 10);
				GetDlgItemText(hDlg, FILTER_TO, Filter_TO, 10);
				GetDlgItemText(hDlg, FILTER_VIA, Filter_VIA, 50);
				GetDlgItemText(hDlg, FILTER_BID, Filter_BID, 14);

				SendDlgItemMessage(hDlg, 0, LB_RESETCONTENT, 0, 0);
				
				for (n = NumberofMessages; n >= 1; n--)
				{
					Msg = MsgHddrPtr[n];
					
					if ((!Filter_TO[0] || strstr(Msg->to, Filter_TO)) &&
						(!Filter_FROM[0] || strstr(Msg->from, Filter_FROM)) &&
						(!Filter_BID[0] || strstr(Msg->bid, Filter_BID)) &&
						(!Filter_VIA[0] || strstr(Msg->via, Filter_VIA)))
					{
						sprintf_s(msgno, sizeof(msgno), "%d", Msg->number);
						SendDlgItemMessage(hDlg, 0, LB_ADDSTRING, 0, (LPARAM)msgno);
					} 
				}
			}

			return TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}

char HRHelpMsg[] = 
"Please read the following carefully, as forwarding is handled rather differently from other BBS software\r\n" 
"Private Messages, and Bulls that have not reached their target area (eg a Bull sent to ALL@GBR from the\r\n"
"USA) are forwarded to only one define define BBS1 with HR EU and BBS2 with HR GBR.EU, a message for GBR.EU\r\n"
"will be sent to BBS2. Any other EU message (eg FRA.EU) would be sent to BBS2."
"\r\n\r\n"
"Bulls which have reached their target will be sent to ALL BBS's where the BBS HA matches all elements\r\n"
"of the HA of the message\r\n So if a BBS had\r\n"
"GBR.EU It would match messages sent to EU or GBR.EU, but not FRA.EU.\r\n"
"If you want to send only Bulls addressed to a lower level then add the number of levels to ignore after the string\r\n"
"So #23.GBR.EU,2 would match only Bulls for #23, and not GBR or EU\r\r"
"The software assumes an implied WW on the end of all aadresses, but only if there is something in the field\r\n"
"So you need an explicit WW to send to everyone. So a BBS with WW in the HA will get all Bulls, and any Personal\r\n"
"Messages that don't have a more explicit route via another BBS"
;

INT_PTR CALLBACK HRHelpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_HRTEXT, HRHelpMsg);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

#include <htmlhelp.h>

int scrolledx; scrolledy;

INT_PTR CALLBACK FwdEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
	struct UserInfo * user;
	RECT Rect;
	SCROLLINFO Sinfo;
	int deltax, deltay;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (user = BBSChain; user; user = user->BBSNext)
		{
			SendDlgItemMessage(hDlg, IDC_BBS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)user->Call);
		}  

		SetDlgItemInt(hDlg, IDC_MAXSEND, MaxTXSize, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXRECV, MaxRXSize, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXAGE, MaxAge, FALSE);

		if (Aliases)
		{
			char Text[100000] = "";
			int i=0;

			while(Aliases[i])
			{
				strcat(Text, Aliases[i]->Dest);
				strcat(Text, ":");
				strcat(Text, Aliases[i]->Alias);
				strcat(Text, "\r\n");
				i++;
			}
			SetDlgItemText(hDlg, IDC_ALIAS, Text);
		}


		CheckDlgButton(hDlg, IDC_READDRESSLOCAL, ReaddressLocal);
		CheckDlgButton(hDlg, IDC_READDRESSRXED, ReaddressReceived);
		CheckDlgButton(hDlg, IDC_WARNNOROUTE, WarnNoRoute);
		CheckDlgButton(hDlg, IDC_USELOCALTIME, Localtime);
		CheckDlgButton(hDlg, IDC_MULTIP, SendPtoMultiple);
		CheckDlgButton(hDlg, IDC_FOURCHARCONTINENT, FOURCHARCONT);

		CurrentBBS = NULL;

		ww = 0;
		wh = 0;

		ShowScrollBar(hDlg, SB_BOTH, FALSE);		// Hide them till needed

		xmargin = 6;
		ymargin = 2 + GetSystemMetrics(SM_CYCAPTION);
		scrolledx = scrolledy = 0;

		GetWindowRect(hDlg, &Rect);
		ww = Rect.right - Rect.left;
		wh = Rect.bottom - Rect.top;

		return (INT_PTR)TRUE;
		
	case WM_SIZE:
		
		w = LOWORD(lParam);
		h = HIWORD(lParam);

		// If window is smaller than client area enable scroll bars

		if (w >= ww && (h + ymargin) >= wh)
		{
			ShowScrollBar(hDlg, SB_BOTH, FALSE);	// Hide them till needed
//			MoveWindow(hwndDisplay, xmargin, ymargin, ww, wh, TRUE);
			ScrollWindow(hDlg, scrolledx, scrolledy, 0, 0);
			scrolledx = scrolledy = 0;
			return TRUE;
		}

		ShowScrollBar(hDlg, SB_BOTH, TRUE);	

		Sinfo.cbSize = sizeof(SCROLLINFO);
		Sinfo.fMask = SIF_ALL;
		Sinfo.nMin = 0;
		Sinfo.nMax = ww + xmargin;
		Sinfo.nPage = w;
		Sinfo.nPos = hpos;
		SetScrollInfo(hDlg, SB_HORZ, &Sinfo, TRUE);

		Sinfo.cbSize = sizeof(SCROLLINFO);
		Sinfo.fMask = SIF_ALL;
		Sinfo.nMin = 0;
		Sinfo.nMax = wh + ymargin;
		Sinfo.nPage = h;
		Sinfo.nPos = hpos;
		SetScrollInfo(hDlg, SB_VERT, &Sinfo, TRUE);
		
		return TRUE;

	case WM_HSCROLL:

		switch (LOWORD(wParam))
		{
		case SB_PAGELEFT:

			goto UpdateHPos;

		case SB_LINELEFT:

			goto UpdateHPos;

		case SB_PAGERIGHT:

			goto UpdateHPos;

		case SB_LINERIGHT:

			hpos++;
			goto UpdateHPos;

		case SB_THUMBPOSITION:

			deltax = hpos - HIWORD(wParam);
			
			ScrollWindow(hDlg, deltax, 0, 0, 0);
			scrolledx -= deltax;

			hpos = hpos -= deltax;

UpdateHPos:
			// Need to update Scroll Bar

			Sinfo.cbSize = sizeof(SCROLLINFO);
			Sinfo.fMask = SIF_ALL;
			Sinfo.nMin = 0;
			Sinfo.nMax = ww + xmargin;
			Sinfo.nPage = w;
			Sinfo.nPos = hpos;
			SetScrollInfo(hDlg, SB_HORZ, &Sinfo, TRUE);

			// Move Client Window

			return TRUE;
		}

		return TRUE;

		
	case WM_VSCROLL:

		switch (LOWORD(wParam))
		{
		case SB_PAGEUP:

			goto UpdateVPos;

		case SB_LINEUP:

			goto UpdateVPos;

		case SB_PAGEDOWN:

			goto UpdateVPos;

		case SB_LINEDOWN:

			goto UpdateVPos;

		case SB_THUMBPOSITION:

			deltay = vpos - HIWORD(wParam);
			
			ScrollWindow(hDlg,0,  deltay, 0, 0);
			scrolledy -= deltay;

			vpos = vpos -= deltay;

UpdateVPos:
			// Need to update Scroll Bar

			Sinfo.cbSize = sizeof(SCROLLINFO);
			Sinfo.fMask = SIF_ALL;
			Sinfo.nMin = 0;
			Sinfo.nMax = wh + ymargin;
			Sinfo.nPage = h;
			Sinfo.nPos = vpos;
			SetScrollInfo(hDlg, SB_VERT, &Sinfo, TRUE);
			return TRUE;
		}

		return TRUE;



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

		Command = LOWORD(wParam);

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case IDC_BBS:

			// BBS Selection Changed

			Do_BBS_Sel_Changed(hDlg);

			return TRUE;

		case IDC_HRHELP:
			
			ShellExecute(hDlg,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQ Mail and Chat Server Mail Forwarding.htm",
				"", NULL, SW_SHOWNORMAL); 

			return TRUE;

		case IDC_FWDSAVE:
			
			SaveFWDConfig(hDlg);
			return TRUE;

		case COPYCONFIG:
			
			CopyFwdConfig(hDlg);
			return TRUE;


		}
		break;
	}
	
	return (INT_PTR)FALSE;
}



int CreateDialogLine(HWND hWnd, int i, int row)
{
	char PortNo[60];
	char PortDesc[31];

	// Only allow UI on ax.25 ports

	struct _EXTPORTDATA * PORTVEC;

	PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntryFromSlot(i);

	if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
		if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
			if (PORTVEC->PORTCONTROL.UICAPABLE == 0)
				return FALSE;

	GetPortDescription(i, PortDesc);
	sprintf(PortNo, "Port %2d %30s", GetPortNumber(i), PortDesc);
	
	hCheck[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 10,row+5,14,14, hWnd, NULL, hInst, NULL);

	Button_SetCheck(hCheck[i], UIEnabled[i]);

	hLabel[i] = CreateWindow(WC_STATIC , PortNo,  WS_CHILD | WS_VISIBLE,
                 30,row+5,300,22, hWnd, NULL, hInst, NULL);
	
	SendMessage(hLabel[i], WM_SETFONT,(WPARAM) hFont, 0);


	hUIBox[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "", WS_CHILD | WS_BORDER | WS_VISIBLE | ES_UPPERCASE,
                 315,row,200,22, hWnd, NULL, hInst, NULL);

	SendMessage(hUIBox[i], WM_SETFONT,(WPARAM) hFont, 0);
	SetWindowText(hUIBox[i], UIDigi[i]);

	hSendMF[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 550,row+4,14,14, hWnd, NULL, hInst, NULL);

	hSendHDDR[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 610,row+4,14,14, hWnd, NULL, hInst, NULL);

	hNullCheck[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 670,row+4,14,14, hWnd, NULL, hInst, NULL);

	Button_SetCheck(hSendMF[i], UIMF[i]);
	Button_SetCheck(hSendHDDR[i], UIHDDR[i]);
	Button_SetCheck(hNullCheck[i], UINull[i]);

	return TRUE;
}



DoUICheck(int i)
{
	return TRUE;
}
DoUIBox(int i)
{
	return TRUE;
}

GetUIConfig()
{
	int Num = GetNumberofPorts();
	int i, Len;

	Free_UI();

	for (i=1; i<=Num; i++)
	{
		UIEnabled[i] =  Button_GetCheck(hCheck[i]);
		UINull[i] =  Button_GetCheck(hNullCheck[i]);
		UIMF[i] =  Button_GetCheck(hSendMF[i]);
		UIHDDR[i] =  Button_GetCheck(hSendHDDR[i]);
	
		Len = GetWindowTextLength(hUIBox[i]);
	
		UIDigi[i] = malloc(Len+1);
		GetWindowText(hUIBox[i], UIDigi[i], Len+1);
	}

	return TRUE;
}

INT_PTR CALLBACK UIDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, i;
	RECT Rect;
	int Row = 80;

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_MAILFOR, MailForText); 

		for (i = 1; i <= GetNumberofPorts(); i++)
		{
			if (CreateDialogLine(hDlg, i, Row))
				Row += 30;

		}

		GetWindowRect(hDlg, &Rect);      
		SetWindowPos(hDlg, HWND_TOP, Rect.left, Rect.top, 800, Row+100, 0);
		SetWindowPos(GetDlgItem(hDlg, IDOK), NULL, 300, Row+20, 70, 30, 0);
		SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL, 400, Row+20, 80, 30, 0);


		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
			case IDCANCEL:

				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
		
			case IDOK:

				GetDlgItemText(hDlg, IDC_MAILFOR, MailForText, 99); 
				GetUIConfig();

				SaveConfig(ConfigName);
	
				sprintf(InfoBoxText, "Configuration Saved");
				DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;

			case 0:

				for (i = 1; i <= 32; i++)
				{
					if (lParam == (LPARAM)hCheck[i])
					{
						DoUICheck(i);
						break;
					}
					else if (lParam == (LPARAM)hUIBox[i])
					{
						DoUIBox(i);
						return TRUE;
					
					}
				}
		}

		break;
		}
	
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK EditMsgTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct MsgInfo * Msg;
	char * MsgBytes;
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		HWND hWndEdit = GetDlgItem(hDlg, IDC_MESSAGE); 

		Msg = MsgHddrPtr[CurrentMsgIndex];

		MsgBytes = ReadMessageFile(Msg->number);

		// See if Multipart

		if (Msg->B2Flags & Attachments)
			EnableWindow(GetDlgItem(hDlg, IDC_SAVEATTACHMENTS), TRUE);

		if (MsgBytes)
		{
			SetDlgItemText(hDlg, IDC_MESSAGE, MsgBytes); 
			SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

			free (MsgBytes);
		}
		return TRUE; 
	}

	case WM_SIZING:
	{
		HWND hWndEdit = GetDlgItem(hDlg, IDC_MESSAGE); 

		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndEdit, 5, 50, Width-20, Height - 95, TRUE);

		return TRUE;
	}

	case WM_ACTIVATE:

		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:

		if (Cmd == IDC_SAVEATTACHMENTS)
		{
			struct MsgInfo * Msg;
			char * MailBuffer;
			char FileName[100][MAX_PATH] = {""};
			int FileLen[100];
			int Files = 0;
			int BodyLen;
			int i;
			char * ptr;

			HANDLE hFile = INVALID_HANDLE_VALUE;
			int WriteLen=0;

			Msg = MsgHddrPtr[CurrentMsgIndex];

			MailBuffer = ReadMessageFile(Msg->number);

			ptr = MailBuffer;

			while(*ptr != 13)
			{
				char * ptr2 = strchr(ptr, 10);	// Find CR

				if (memcmp(ptr, "Body: ", 6) == 0)
				{
					BodyLen = atoi(&ptr[6]);
				}

				if (memcmp(ptr, "File: ", 6) == 0)
				{
					char * ptr1 = strchr(&ptr[6], ' ');	// Find Space

					FileLen[Files] = atoi(&ptr[6]);

					memcpy(FileName[Files++], &ptr1[1], (ptr2-ptr1 - 2));
				}
				
				ptr = ptr2;
				ptr++;
			}

			ptr += 4;			// Over Blank Line and Separator
			ptr += BodyLen;		// to first file

			for (i = 0; i < Files; i++)
			{
				OPENFILENAME Ofn; 
				memset(&Ofn, 0, sizeof(Ofn));
 
				Ofn.lStructSize = sizeof(OPENFILENAME); 
				Ofn.hInstance = hInst;
				Ofn.hwndOwner = hDlg; 
				Ofn.lpstrFilter = NULL; 
				Ofn.lpstrFile= FileName[i]; 
				Ofn.nMaxFile = sizeof(FileName[i])/ sizeof(*FileName[i]); 
				Ofn.lpstrFileTitle = NULL; 
				Ofn.nMaxFileTitle = 0; 
				Ofn.lpstrInitialDir = (LPSTR)NULL; 
				Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 
				Ofn.lpstrTitle = NULL;//; 

				if (GetSaveFileName(&Ofn))
				{
					hFile = CreateFile(FileName[i],
						GENERIC_WRITE, FILE_SHARE_READ, NULL,
						CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

					if (hFile != INVALID_HANDLE_VALUE)
					{
						WriteFile(hFile, ptr, FileLen[i], &WriteLen, NULL);
						CloseHandle(hFile);
					}
				}

				ptr += FileLen[i];
				ptr +=2;				// Over separator - I don't think there should be one
			}
		}
				
		if (Cmd == IDSAVE)
		{
			struct MsgInfo * Msg;
			char * via = NULL;
			int MsgLen;
			char * MailBuffer;
			char MsgFile[MAX_PATH];
			HANDLE hFile = INVALID_HANDLE_VALUE;
			int WriteLen=0;

			Msg = MsgHddrPtr[CurrentMsgIndex];

			if (Msg->B2Flags & Attachments)
			{
				MessageBox(NULL, "It isn't safe to save messages with attachments", "BPQMail", MB_ICONERROR);
				return TRUE;
			}


			MsgLen = SendDlgItemMessage(hDlg, IDC_MESSAGE, WM_GETTEXTLENGTH, 0 ,0);

			if (MsgLen)
			{
				MailBuffer = malloc(MsgLen+1);
				GetDlgItemText(hDlg, IDC_MESSAGE, MailBuffer, MsgLen+1);
			}

			Msg->datechanged = time(NULL);
			Msg->length = MsgLen;

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
			hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, MailBuffer, Msg->length, &WriteLen, NULL);
				CloseHandle(hFile);
			}

			free(MailBuffer);

			EndDialog(hDlg, LOWORD(wParam));

			return TRUE;
		}

		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}
