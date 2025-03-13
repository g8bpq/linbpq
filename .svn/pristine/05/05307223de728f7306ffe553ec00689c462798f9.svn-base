/*
Copyright 2001-2018 John Wiseman G8BPQ

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

// Mail and Chat Server for BPQ32 Packet Switch
//
//	Support for FLAMP compatible Mulitcast

#include "bpqmail.h"

void decodeblock( unsigned char in[4], unsigned char out[3]);  // Base64 Decode

time_t MulticastMaxAge = 48 * 60 * 60;		// 48 Hours in secs

struct MSESSION * MSessions = NULL;

#ifndef LINBPQ

#include "AFXRES.h"

HWND hMCMonitor = NULL;
HWND MCList;

static HMENU hMCMenu;		// handle of menu 

static char MCClassName[]="BPQMCWINDOW";

RECT MCMonitorRect;

static int Height, Width, LastY;

#define BGCOLOUR RGB(236,233,216)

void MCMoveWindows()
{
	RECT rcClient;
	int ClientWidth, ClientHeight;

	GetClientRect(hMCMonitor, &rcClient); 

	if (rcClient.bottom == 0)		// Minimised
		return;

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(MCList, 0, 0, rcClient.right, rcClient.bottom, TRUE);
}

void CopyMCToClipboard(HWND hWnd);

LRESULT CALLBACK MCWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	struct MSESSION * Sess = MSessions;
	struct MSESSION * Temp;
	
	switch (message)
	{ 
	
	case WM_ACTIVATE:

		break;

	case WM_CLOSE:
		if (wParam)				// Used by Close All Programs.
			return 0;
			
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{

		case ID_EDIT_COPY:

			CopyMCToClipboard(hMCMonitor);
			return 0;;

		case ID_EDIT_CLEAR:
	
			while (Sess)
			{
				ListView_DeleteItem(MCList, Sess->Index);

				if (Sess->FileName)
					free(Sess->FileName);

				if (Sess->OrigTimeStamp)
					free(Sess->OrigTimeStamp);

				if (Sess->Message)
					free(Sess->Message);

				if (Sess->BlockList)
					free(Sess->BlockList);

				if (Sess->ID)
					free(Sess->ID);

				Temp = Sess;
				Sess = Sess->Next;
			}

			MSessions = NULL;
			return 0;
		
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

			MCMoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&MonitorRect);	// For save soutine         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			hMCMonitor = NULL;

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}


static void MoveMCWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	GetWindowRect(hMCMonitor, &rcMain);
	GetClientRect(hMCMonitor, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

//	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
//	MoveWindow(hwndOutput,2, 2, ClientWidth-4, ClientHeight-4, TRUE);
//	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);
}




HWND CreateMCListView (HWND hwndParent) 
{
    INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	HWND hList;
	LV_COLUMN Column;
	LOGFONT lf; 
    HFONT hFont; 
	int n = 0;
 
	memset(&lf, 0, sizeof(LOGFONT));

	lf.lfHeight = 12;
	lf.lfWidth = 8;
	lf.lfPitchAndFamily = FIXED_PITCH;
	strcpy (lf.lfFaceName, "FIXEDSYS");

    hFont = CreateFontIndirect(&lf); 
	
	icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the list-view window in report view with label editing enabled.
    
	hList = CreateWindow(WC_LISTVIEW, 
                                     "Messages",
                                     WS_CHILD | LVS_REPORT | LVS_EDITLABELS,
                                     0, 0, 100, 100,
                                     hwndParent,
                                     (HMENU)NULL,
                                     hInst,
                                     NULL); 

	SendMessage(hList, WM_SETFONT,(WPARAM) hFont, 0);


	ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);

	Column.cx=45;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="ID";

	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 
	Column.cx=95;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="From";

	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=140;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="FileName";

	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=50;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Size";

	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 
	
	Column.cx=40;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="%";
	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=55;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Time";
	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=55;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Age";
	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=20;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="C";

	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM)&Column); 

	Column.cx=430;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="BlockList";
	SendMessage(hList, LVM_INSERTCOLUMN, n++, (LPARAM) &Column); 

	ShowWindow(hList, SW_SHOWNORMAL);
	UpdateWindow(hList);

    return (hList);
}

#define OurSetItemText(hwndLV, i, iSubItem_, pszText_) \
{ LV_ITEM _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  _ms_lvi.pszText = pszText_;\
  SNDMSG((hwndLV), LVM_SETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);\
}

void RefreshMCLine(struct MSESSION * MSession)
{
	LV_ITEM Item;
	LVFINDINFO Finfo;
	int ret, n, pcent;
	char Time[80];
	char Agestring[80];
	char Size[16] = "??";
	char Percent[16] = "??";
	char Key[16];

	char BlockList[101] = "";
	struct tm * TM;
	time_t Age;

	if (MCList == 0)
		return;

	sprintf(Key, "%04X", MSession->Key);

	Age = time(NULL) - MSession->LastUpdated;

//	if (LocalTime)
//		TM = localtime(&MSession->LastUpdated);
//	else
		TM = gmtime(&Age);

	sprintf(Agestring, "%.2d:%.2d",
			TM->tm_hour, TM->tm_min);

	TM = gmtime(&MSession->Created);

	sprintf(Time, "%.2d:%.2d",
			TM->tm_hour, TM->tm_min);


	Finfo.flags = LVFI_STRING;
	Finfo.psz = Key;
	Finfo.vkDirection = VK_DOWN;
	ret = SendMessage(MCList, LVM_FINDITEM, (WPARAM)-1, (LPARAM) &Finfo);

	if (ret == -1)
	{
		n = ListView_GetItemCount(MCList);
		MSession->Index = n;
	}
	else
		MSession->Index = ret;

	Item.mask=LVIF_TEXT;
	Item.iItem = MSession->Index;
	Item.iSubItem = 0;
	Item.pszText = Key;

	ret = SendMessage(MCList, LVM_SETITEMTEXT, (WPARAM)MSession->Index, (LPARAM) &Item);

	if (ret == 0)
		MSession->Index = ListView_InsertItem(MCList, &Item);	 
			
	sprintf(Size, "%d", MSession->MessageLen);
	
	if (MSession->MessageLen)
	{
		int i;
		
		pcent = (MSession->BlocksReceived * 100) / MSession->BlockCount;
		sprintf(Percent, "%d", pcent);

		// Flag received blocks. Normalise to 50 wide 

		memset(BlockList, '.', 50);

		for (i = 0; i < 50; i++)
		{
			int posn = (i * MSession->BlockCount) / 50;
			if (MSession->BlockList[posn] == 1)
				BlockList[i] = 'Y';
		}
	}

	n = 0;

	OurSetItemText(MCList, MSession->Index, n++, Key);
	if (MSession->ID)
		OurSetItemText(MCList, MSession->Index, n++, MSession->ID)
	else
		OurSetItemText(MCList, MSession->Index, n++, " ");

	OurSetItemText(MCList, MSession->Index, n++, MSession->FileName);
	OurSetItemText(MCList, MSession->Index, n++, Size);
	OurSetItemText(MCList, MSession->Index, n++, Percent);
	OurSetItemText(MCList, MSession->Index, n++, Time);
	OurSetItemText(MCList, MSession->Index, n++, Agestring);

	if (MSession->Completed)
		OurSetItemText(MCList, MSession->Index, n++, "Y")
	else
		OurSetItemText(MCList, MSession->Index, n++, " ");

	OurSetItemText(MCList, MSession->Index, n++, BlockList);
}


BOOL CreateMulticastConsole()
{
    WNDCLASS  wc;
	HBRUSH bgBrush;
    RECT rcClient;

	if (hMCMonitor)
	{
		ShowWindow(hMCMonitor, SW_SHOWNORMAL);
		SetForegroundWindow(hMCMonitor);
		return FALSE;							// Already open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = MCWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = MCClassName; 

	RegisterClass(&wc);

	hMCMonitor=CreateDialog(hInst, MCClassName, 0, NULL);
	
    if (!hMCMonitor)
        return (FALSE);

	hMCMenu=GetMenu(hMCMonitor);

//	CheckMenuItem(hMenu,MONBBS, MonBBS ? MF_CHECKED : MF_UNCHECKED);
//	CheckMenuItem(hMenu,MONCHAT, MonCHAT ? MF_CHECKED : MF_UNCHECKED);
//	CheckMenuItem(hMenu,MONTCP, MonTCP ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hMCMonitor);	

	// Create List View
			
	GetClientRect (hMCMonitor, &rcClient); 

	MCList = CreateMCListView(hMCMonitor);

	MoveWindow(MCList, 0, 0, rcClient.right, rcClient.bottom, TRUE);

	if (cfgMinToTray)
		AddTrayMenuItem(hMCMonitor, "Mail Multicast Monitor");

	ShowWindow(hMCMonitor, SW_SHOWNORMAL);

	if (MCMonitorRect.right < 100 || MCMonitorRect.bottom < 100)
	{
		GetWindowRect(hMCMonitor, &MCMonitorRect);
	}

	MoveWindow(hMCMonitor, MCMonitorRect.left, MCMonitorRect.top,
		MCMonitorRect.right-MCMonitorRect.left, MCMonitorRect.bottom-MCMonitorRect.top, TRUE);

	MoveMCWindows();

	return TRUE;

}
void CopyMCToClipboard(HWND hWnd)
{
	int i,n, len=0;
	char * Buffer;
	HGLOBAL	hMem;
	char * ptr;

	char Time[80];
	char Agestring[80];
	char From[16];
	char Size[16];
	char Percent[16];
	char FileName[128];
	char Key[16];
	char Complete[2];

	char BlockList[128];

	n = ListView_GetItemCount(MCList);
	
	Buffer = malloc((n + 1) * 200);

	len = sprintf(Buffer, "ID   From       FileName          Size  %%  Time  Age     Blocklist\r\n");

	for (i=0; i<n; i++)
	{
		// Get Items
		
		ListView_GetItemText(MCList, i, 0, Key, 8);
		ListView_GetItemText(MCList, i, 1, From, 15);
		ListView_GetItemText(MCList, i, 2, FileName, 128);
		ListView_GetItemText(MCList, i, 3, Size, 16);
		ListView_GetItemText(MCList, i, 4, Percent, 16);
		ListView_GetItemText(MCList, i, 5, Time, 80);
		ListView_GetItemText(MCList, i, 6, Agestring, 80);
		ListView_GetItemText(MCList, i, 7, Complete, 2);
		ListView_GetItemText(MCList, i, 8, BlockList, 100);

		// Add line to buffer

		len += sprintf(&Buffer[len], "%4s %-10s %-16s %5s%4s %-6s%-6s%-2s%50s\r\n",
			Key, From, FileName, Size, Percent, Time, Agestring, Complete, BlockList);
	}
	
	hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len+1);
	
	if (hMem != 0)
	{
		ptr = GlobalLock(hMem);
	
		if (OpenClipboard(MainWnd))
		{
			strcpy(ptr, Buffer);
			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
			else
				GlobalFree(hMem);		
	}
	free(Buffer);
}




#else

// LinBPQ

void RefreshMCLine(struct MSESSION * MSession)
{
}

#endif

static unsigned int crcval = 0xFFFF;

static void update(char c)
{
	int i;
	
	crcval ^= c & 255;
    for (i = 0; i < 8; ++i)
	{
        if (crcval & 1)
            crcval = (crcval >> 1) ^ 0xA001;
        else
            crcval = (crcval >> 1);
    }
}
	
static unsigned int CalcCRC(UCHAR * ptr, int Len)
{
	int i;
	
	crcval = 0xFFFF;
	for (i = 0; i < Len; i++)
	{
		update(*ptr++);
	}
	return crcval;
}

struct MSESSION * FindMSession(unsigned int Key)
{
	struct MSESSION * Sess = MSessions;
	struct MSESSION * LastSess = NULL;

	while (Sess)
	{
		if (Sess->Key == Key)
			return Sess;

		LastSess = Sess;
		Sess = Sess->Next;
	}

	//	 Not found

	Sess = zalloc(sizeof(struct MSESSION)); 

	if (Sess == NULL)
		return NULL;

	Sess->Key = Key;

	Sess->Created = time(NULL);

	if (LastSess)
		LastSess->Next = Sess;
	else
		MSessions = Sess;

	return Sess;
}

#include "LzmaLib.h"

#define LZMA_STR "\1LZMA"

UCHAR * LZUncompress(UCHAR * Decoded, size_t Len, size_t * NewLen)
{
	unsigned char * buf;
	unsigned char inprops[LZMA_PROPS_SIZE];
	size_t inlen;
	int r;
	
	size_t rlen = 0;
	size_t outlen;

	memcpy(&rlen, &Decoded[5], 4);

	outlen = ntohl(rlen);
	*NewLen = outlen;

	buf = malloc(outlen);

	if (outlen > 1 << 25) 
	{
		Debugprintf("Refusing to decompress data (> 32 MiB)");				
		return NULL;
	}


	memcpy(inprops, Decoded + strlen(LZMA_STR) + sizeof(int), LZMA_PROPS_SIZE);

	inlen = Len - strlen(LZMA_STR) - sizeof(int) - LZMA_PROPS_SIZE;

	if ((r = LzmaUncompress(buf, &outlen, (const unsigned char*)Decoded + Len - inlen, &inlen,
			inprops, LZMA_PROPS_SIZE)) != SZ_OK)
	{			
		Debugprintf("Lzma Uncompress failed: %s", LZMA_ERRORS[r]);
		return NULL;
	}
	else
	{
		return buf;
	}
}

void decodeblock128(unsigned char in[8], unsigned char out[7] )
{   
	out[0] = (unsigned char) (in[0] << 1 | in[1] >> 6);
	out[1] = (unsigned char) (in[1] << 2 | in[2] >> 5);
	out[2] = (unsigned char) (in[2] << 3 | in[3] >> 4);
	out[3] = (unsigned char) (in[3] << 4 | in[4] >> 3);
	out[4] = (unsigned char) in[4] << 5 | in[5] >> 2;
	out[5] = (unsigned char) in[5] << 6 | in[6] >> 1;
	out[6] = (unsigned char) in[6] << 7 | in[7];
}


void SaveMulticastMessage(struct MSESSION * MSession)
{
	UCHAR * Decoded = NULL;			// Output from Basexxx decode
	UCHAR * Uncompressed = NULL;
	size_t DecodedLen;				// Length of decoded message
	size_t UncompressedLen;			// Length of decompressed message
	int ExpectedLen;				// From front of Base128 or Base256 message
	int HddrLen;					// Length of Expected Len Header

	if (MSession->FileName == NULL)
		return;						// Need Name
		
	MSession->Completed = TRUE;		// So we don't get it again

	// If compresses and encoded, decode and decompress

	if (memcmp(MSession->Message, "[b64:start]", 11) == 0)
	{
		UCHAR * ptr1 = &MSession->Message[11];
		UCHAR * ptr2 = malloc(MSession->MessageLen);	// Must get smaller
		
		int Len = MSession->MessageLen - 21;			// Header and Trailer
		
		Decoded = ptr2;
	
		// Decode Base64 encoding

		while (Len > 0)
		{
			decodeblock(ptr1, ptr2);
			ptr1 += 4;
			ptr2 += 3;
			Len -= 4;
		}

		DecodedLen = (int)(ptr2 - Decoded);
		Uncompressed = LZUncompress(Decoded, DecodedLen, &UncompressedLen);
	}
	else if (memcmp(MSession->Message, "[b128:start]", 12) == 0)
	{
		UCHAR * ptr1 = &MSession->Message[12];
		UCHAR * ptr2 = malloc(MSession->MessageLen);	// Must get smaller
		UCHAR ch;
		UCHAR * Intermed;

		int Len = MSession->MessageLen - 23;			// Header and Trailer
		
		Intermed = ptr2;
	
		// Decode Base128 encoding

		// First remove transparency (as in base256)


		// Extract decoded msg len
		
		ExpectedLen = atoi(ptr1);

		ptr1 = strchr(ptr1, 10);
		ptr1++;

		HddrLen = (int)(ptr1 - &MSession->Message[12]);

		if (ExpectedLen == 0 || ExpectedLen > Len || ptr1 == (UCHAR *)1)
		{
			Debugprintf("MCAST Missing Length Field");
			return;
		}
					
		Len -= HddrLen;;

		while (Len > 0)
		{
			ch = *(ptr1++);
			Len --;

			if (ch == ':')
			{
				ch = *(ptr1++);
				Len--;
				
				switch (ch)
				{
					case ':' : *(ptr2++) =  ':';  break;
					case '0' : *(ptr2++) = 0x00; break;
					case '1' : *(ptr2++) = 0x01; break;
					case '2' : *(ptr2++) = 0x02; break;
					case '3' : *(ptr2++) = 0x03; break;
					case '4' : *(ptr2++) = 0x04; break;
					case '5' : *(ptr2++) = 0x05; break;
					case '6' : *(ptr2++) = 0x06; break;
					case '7' : *(ptr2++) = 0x07; break;
					case '8' : *(ptr2++) = 0x08; break;
					case '9' : *(ptr2++) = 0x09; break;
					case 'A' : *(ptr2++) = '\n'; break;
					case 'B' : *(ptr2++) = '\r'; break;
					case 'C' : *(ptr2++) = '^';  break;
					case 'D' : *(ptr2++) = 0x7F; break;
					case 'E' : *(ptr2++) = 0xFF; break;
				}
			}
			else
				*(ptr2++) = ch;
		}


		Len = ptr2 - Intermed;

		ptr1 = Intermed;
		ptr2 = malloc(MSession->MessageLen);	// Must get smaller
		Decoded = ptr2;

		while (Len > 0)
		{
			decodeblock128(ptr1, ptr2);
			ptr1 += 8;
			ptr2 += 7;
			Len -= 8;
		}

		DecodedLen = ptr2 - Decoded;
		Uncompressed = LZUncompress(Decoded, DecodedLen, &UncompressedLen);
	}
	else if (memcmp(MSession->Message, "[b256:start]", 12) == 0)
	{
		UCHAR * ptr1 = &MSession->Message[12];
		UCHAR * ptr2 = malloc(MSession->MessageLen);	// Must get smaller
		UCHAR ch;

		int Len = MSession->MessageLen - 23;			// Header and Trailer
		
		Decoded = ptr2;
	
		// Decode Base256 encoding

		// Extract decoded msg len
		
		ExpectedLen = atoi(ptr1);

		ptr1 = strchr(ptr1, 10);
		ptr1++;

		HddrLen = ptr1 - &MSession->Message[12];

		if (ExpectedLen == 0 || ExpectedLen > Len || ptr1 == (UCHAR *)1)
		{
			Debugprintf("MCAST Missing Length Field");
			return;
		}
					
		Len -= HddrLen;;

		while (Len > 0)
		{
			ch = *(ptr1++);
			Len --;

			if (ch == ':')
			{
				ch = *(ptr1++);
				Len--;
				
				switch (ch)
				{
					case ':' : *(ptr2++) =  ':';  break;
					case '0' : *(ptr2++) = 0x00; break;
					case '1' : *(ptr2++) = 0x01; break;
					case '2' : *(ptr2++) = 0x02; break;
					case '3' : *(ptr2++) = 0x03; break;
					case '4' : *(ptr2++) = 0x04; break;
					case '5' : *(ptr2++) = 0x05; break;
					case '6' : *(ptr2++) = 0x06; break;
					case '7' : *(ptr2++) = 0x07; break;
					case '8' : *(ptr2++) = 0x08; break;
					case '9' : *(ptr2++) = 0x09; break;
					case 'A' : *(ptr2++) = '\n'; break;
					case 'B' : *(ptr2++) = '\r'; break;
					case 'C' : *(ptr2++) = '^';  break;
					case 'D' : *(ptr2++) = 0x7F; break;
					case 'E' : *(ptr2++) = 0xFF; break;
				}
			}
			else
				*(ptr2++) = ch;
		}

		DecodedLen = ptr2 - Decoded;
		Uncompressed = LZUncompress(Decoded, DecodedLen, &UncompressedLen);
	}
	else
	{
		// Plain Text

		UncompressedLen = MSession->MessageLen;
		Uncompressed = MSession->Message;

		MSession->Message = NULL;		// So we dont try to free again
	}

	if (Decoded)
		free(Decoded);

	if (Uncompressed)
	{
		// Write it away and free it

		char MsgFile[MAX_PATH];
		FILE * hFile;
		int WriteLen=0;
		UCHAR * ptr1 = Uncompressed;

		// Make Sure MCAST directory exists

		sprintf_s(MsgFile, sizeof(MsgFile), "%s/MCAST", MailDir);

#ifdef WIN32
		CreateDirectory(MsgFile, NULL);		// Just in case
#else
		mkdir(MsgFile, S_IRWXU | S_IRWXG | S_IRWXO);
		chmod(MsgFile, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

		sprintf_s(MsgFile, sizeof(MsgFile), "%s/MCAST/%s", MailDir, MSession->FileName);

		hFile = fopen(MsgFile, "wb");
			
		if (hFile)
		{
			WriteLen = (int)fwrite(Uncompressed, 1, UncompressedLen, hFile);
			fclose(hFile);
		}


		// if it looks like an export file (Starts SP SB or ST) and ends /ex
		// import and delete it.
		
		if (*(ptr1) == 'S' && ptr1[2] == ' ')
			if (_memicmp(&ptr1[UncompressedLen - 5], "/EX", 3) == 0)
				ImportMessages(NULL, MsgFile, TRUE);

		free (Uncompressed);
	}
}

VOID ProcessMCASTLine(ConnectionInfo * conn, struct UserInfo * user, char * Buffer, int MsgLen)
{
	char Opcode[80];
	unsigned int checksum, len = 0;
	char * data;
	int headerlen = 0;
	unsigned int crcval;
	int n;
	unsigned int Key;
	struct MSESSION * MSession;

	if (MsgLen == 1 && Buffer[0] == 13)
		return;

	MsgLen --;			// Remove the CR we added

	Buffer[MsgLen] = 0;

	if (MsgLen == 1 && Buffer[0] == 13)
		return;

//	return;

	n = sscanf(&Buffer[1], "%s %04d %04X", Opcode, &len, &checksum);

	if (n != 3)
		return;

	data = strchr(Buffer, '>');

	if (data)
		headerlen = (int)(++data - Buffer);

	if (headerlen + len != MsgLen)
		return;

	crcval = CalcCRC(data, len);
	
	if (checksum != crcval)
		return;

	// Extract Session Key

	sscanf(&data[1], "%04X", &Key);

	MSession = FindMSession(Key);

	if (MSession == 0)
		return;					// ?? couldn't allocate

	MSession->LastUpdated = time(NULL);

	if (MSession->Completed)
		return;					// We already have it all

	if (strcmp(Opcode, "ID") == 0)
	{
		strlop(&data[6], ' ');
		MSession->ID = _strdup(&data[6]);

		return;
	}

	if (strcmp(Opcode, "PROG") == 0)
	{
		// Ignore for now
		return;
	}

	if (strcmp(Opcode, "FILE") == 0)
	{
		//		<FILE 34 2A1A>{80BC}20141108142542:debug_log.txt

		char * FN = strchr(&data[6], ':');

		if (FN)
		{
			*(FN++) = 0;

			MSession->FileName = _strdup(FN);
			MSession->OrigTimeStamp = _strdup(&data[6]);
		}

		// We could get whole message without getting the Name,
		// so check

		if (MSession->BlockCount && MSession->BlocksReceived == MSession->BlockCount)
		{
			// We have the whole message. Decode and Save

			if (MSession->MessageLen)				// Also need length
				SaveMulticastMessage(MSession);
		}

		RefreshMCLine(MSession);
		return;
	}

	if (strcmp(Opcode, "SIZE") == 0)
	{
		// SIZE 14 2995>{80BC}465 8 64

		int a, b, c, n = sscanf(&data[6], "%d %d %d", &a, &b, &c);
		
		if (n == 3)
		{
			// We may already have some (or even all) the message if we
			// missed the SIZE block first time round

			if (MSession->Message)
			{
				// Already have at least part of it

				if (MSession->BlockSize	!= c)
				{
					// We based blocksize on last packet, so need to sort out mess

					// Find where we put the block, and move it

					UCHAR * OldLoc;

					MSession->Message = realloc(MSession->Message, a);
					MSession->BlockList = realloc(MSession->BlockList, b);

					OldLoc = &MSession->Message[(MSession->BlockCount - 1) * MSession->BlockSize];

					memmove(&MSession->Message[(MSession->BlockCount - 1) * c], OldLoc, MSession->BlockSize);
		
					MSession->BlockSize = c;	
				}

				if (MSession->BlockCount < b)
				{
					// Dont have it all, so need to extend ;

					MSession->Message = realloc(MSession->Message, a);
					MSession->BlockList = realloc(MSession->BlockList, b);
				}
			}

			MSession->MessageLen = a;
			MSession->BlockCount = b;
			MSession->BlockSize	= c;

			if (MSession->Message == NULL)
			{
				MSession->Message = zalloc(b * c);
				MSession->BlockList = zalloc(b);
			}

			// We might have it all now

			if (MSession->BlocksReceived == MSession->BlockCount)
			{
				// We have the whole message. Decode and Save

				SaveMulticastMessage(MSession);
			}
		}

		RefreshMCLine(MSession);
		return;
	}

	if (strcmp(Opcode, "DATA") == 0)
	{
		//	<DATA 72 B21B>{80BC:1}[b256:start]401
		
		int Blockno = atoi(&data[6]);
		char * dataptr = strchr(&data[6], '}');

		if (dataptr == 0)
			return;

		dataptr++;

		// What should we do if we don't have Filename or Size??

		// If we assume this isn't the last block, then we can get
		// the block size from this message. This is pretty save, but
		// I guess as we will only get one last block, if we subsequently
		// get an earlier one that is bigger, we can recalculate the position
		// of this block and move it.

		if (MSession->MessageLen == 0)
		{
			// Haven't received SIZE Message yet. Guess the blocksize

			int blocksize = MsgLen - (int)(dataptr - Buffer);

			if (MSession->BlockSize == 0)
			{
				MSession->BlockSize = blocksize;
			}
			else
			{
				if (MSession->BlockSize < blocksize)
				{
					// We based blocksize on last packet, so need to sort out mess

					// Find where we put the block, and move it
					
					UCHAR * OldLoc = &MSession->Message[(MSession->BlockCount - 1) * MSession->BlockSize];
					memmove(&MSession->Message[(MSession->BlockCount - 1) * blocksize], OldLoc, MSession->BlockSize);
		
					MSession->BlockSize = blocksize;	
				}
			}

			// We need to realloc Message and Blocklist if this is a later block

			if (MSession->BlockCount < Blockno)
			{
				MSession->Message = realloc(MSession->Message, Blockno * MSession->BlockSize);
				MSession->BlockList = realloc(MSession->BlockList, Blockno);
			
				memset(&MSession->BlockList[MSession->BlockCount], 0, Blockno - MSession->BlockCount);		
				MSession->BlockCount = Blockno;

			}

		}
		if (Blockno == 0 || Blockno > MSession->BlockCount)
			return;

		Blockno--;

		if (MSession->BlockList[Blockno] == 1)
		{
			// Already have this block

			return;
		}


		memcpy(&MSession->Message[Blockno * MSession->BlockSize], dataptr, MSession->BlockSize);
		MSession->BlockList[Blockno] = 1;

		MSession->BlocksReceived++;

		if (MSession->BlocksReceived == MSession->BlockCount && MSession->MessageLen)
		{
			// We have the whole message. Decode and Save

			SaveMulticastMessage(MSession);
		}

		RefreshMCLine(MSession);

		return;
	}

	MsgLen++;

/*

QST DE GM8BPQ

<PROG 18 A519>{80BC}FLAMP 2.2.03
<FILE 34 2A1A>{80BC}20141108142542:debug_log.txt
<ID 22 9BF3>{80BC}GM8BPQ Skigersta
<SIZE 14 2995>{80BC}465 8 64
<DATA 72 B21B>{80BC:1}[b256:start]401
:1LZMA:0:0:6-]:0:0:0:4:0$--:7<DC3>m?8-<EM><DC1>v\-E<DLE>-Y-[---
<DATA 72 AE86>{80BC:2}rS-)N{j--o--ZMPX-<DLE>,-l-yD----<EM><EM>--E--;-o:6-|;<US>--f---q----0<---%<RS>--
<DATA 72 A08F>{80BC:3}*N-?N--<US>*:Cf{<FS>:9--z-J<CAN>:9-HMd:8-------Q--D---_-a----:C<EM>$;A-j---(:
<DATA 72 538A>{80BC:4}D<DC1>Wb<SI>--K<DC1>---Qq-uj-_<SUB>--<RS><SO>;i------T-\>-{:6---~-ij~-,-(-O--2--+
<SI>-:8
<DATA 72 35E6>{80BC:5}p---:7G<DC1><DC2>f:E<GS>-5o->x---4<GS>--K--:3-\:E---gouu<DC1>H<SYN>-3'----:A!.:7
--N:0S
<DATA 72 CA70>{80BC:6}.---/-~#<SUB>.-:D:7zg~--m--:8-'---Y<US>%-?--ze<DC1>\-ho:5-}-:C<ETB>:A:1u-1-O-<SI>
-
<DATA 72 8C88>{80BC:7}<NAK>9p-<US>42-w--<ETB>G:2G:3--g--O---n-<NAK><RS>---c-#----DF-!~--:D-A--|-e-------
<DATA 25 E6D6>{80BC:8}B{--:0
[b256:end]
<CNTL 10 7451>{80BC:EOF}
<CNTL 10 D45D>{80BC:EOT}

DE GM8BPQ K

*/

	return; 
/*
	if (strcmp(Buffer, "ARQ::ETX\r") == 0)
	{
		// Decode it. 

		UCHAR * ptr1, * ptr2, * ptr3;
		int len, linelen;
		struct MsgInfo * Msg = conn->TempMsg;
		time_t Date;
		char FullTo[100];
		char FullFrom[100];
		char ** RecpTo = NULL;				// May be several Recipients
		char ** HddrTo = NULL;				// May be several Recipients
		char ** Via = NULL;					// May be several Recipients
		int LocalMsg[1000]	;				// Set if Recipient is a local wl2k address

		int B2To;							// Offset to To: fields in B2 header
		int Recipients = 0;
		int RMSMsgs = 0, BBSMsgs = 0;

//		Msg->B2Flags |= B2Msg;
				

		ptr1 = conn->MailBuffer;
		len = Msg->length;
		ptr1[len] = 0;

		if (strstr(ptr1, "ARQ:ENCODING::"))
		{
			// a file, not a message. If is called  "BBSPOLL" do a reverse forward else Ignore for now

			_strupr(conn->MailBuffer);
			if (strstr(conn->MailBuffer, "BBSPOLL"))
			{
				SendARQMail(conn);
			}

			free(conn->MailBuffer);
			conn->MailBuffer = NULL;
			conn->MailBufferSize = 0;

			return;
		}
	Loop:
		ptr2 = strchr(ptr1, '\r');

		linelen = ptr2 - ptr1;

		if (_memicmp(ptr1, "From:", 5) == 0 && linelen > 6)			// Can have empty From:
		{
			char SaveFrom[100];
			char * FromHA;

			memcpy(FullFrom, ptr1, linelen);
			FullFrom[linelen] = 0;

			// B2 From may now contain an @BBS 

			strcpy(SaveFrom, FullFrom);
				
			FromHA = strlop(SaveFrom, '@');

			if (strlen(SaveFrom) > 12) SaveFrom[12] = 0;
			
			strcpy(Msg->from, &SaveFrom[6]);

			if (FromHA)
			{
				if (strlen(FromHA) > 39) FromHA[39] = 0;
				Msg->emailfrom[0] = '@';
				strcpy(&Msg->emailfrom[1], _strupr(FromHA));
			}

			// Remove any SSID

			ptr3 = strchr(Msg->from, '-');
				if (ptr3) *ptr3 = 0;
		
		}
		else if (_memicmp(ptr1, "To:", 3) == 0 || _memicmp(ptr1, "cc:", 3) == 0)
		{
			HddrTo=realloc(HddrTo, (Recipients+1) * sizeof(void *));
			HddrTo[Recipients] = zalloc(100);

			memset(FullTo, 0, 99);
			memcpy(FullTo, &ptr1[4], linelen-4);
			memcpy(HddrTo[Recipients], ptr1, linelen+2);
			LocalMsg[Recipients] = FALSE;

			_strupr(FullTo);

			B2To = ptr1 - conn->MailBuffer;

			if (_memicmp(FullTo, "RMS:", 4) == 0)
			{
				// remove RMS and add @winlink.org

				strcpy(FullTo, "RMS");
				strcpy(Msg->via, &FullTo[4]);
			}
			else
			{
				ptr3 = strchr(FullTo, '@');

				if (ptr3)
				{
					*ptr3++ = 0;
					strcpy(Msg->via, ptr3);
				}
				else
					Msg->via[0] = 0;
			}
		
			if (_memicmp(&ptr1[4], "SMTP:", 5) == 0)
			{
				// Airmail Sends MARS messages as SMTP
					
				if (CheckifPacket(Msg->via))
				{
					// Packet Message

					memmove(FullTo, &FullTo[5], strlen(FullTo) - 4);
					_strupr(FullTo);
					_strupr(Msg->via);
						
					// Update the saved to: line (remove the smtp:)

					strcpy(&HddrTo[Recipients][4], &HddrTo[Recipients][9]);
					BBSMsgs++;
					goto BBSMsg;
				}

				// If a winlink.org address we need to convert to call

				if (_stricmp(Msg->via, "winlink.org") == 0)
				{
					memmove(FullTo, &FullTo[5], strlen(FullTo) - 4);
					_strupr(FullTo);
					LocalMsg[Recipients] = CheckifLocalRMSUser(FullTo);
				}
				else
				{
					memcpy(Msg->via, &ptr1[9], linelen);
					Msg->via[linelen - 9] = 0;
					strcpy(FullTo,"RMS");
				}
//					FullTo[0] = 0;

		BBSMsg:		
				_strupr(FullTo);
				_strupr(Msg->via);
			}

			if (memcmp(FullTo, "RMS:", 4) == 0)
			{
				// remove RMS and add @winlink.org

				memmove(FullTo, &FullTo[4], strlen(FullTo) - 3);
				strcpy(Msg->via, "winlink.org");
				sprintf(HddrTo[Recipients], "To: %s\r\n", FullTo);
			}

			if (strcmp(Msg->via, "RMS") == 0)
			{
				// replace RMS with @winlink.org

				strcpy(Msg->via, "winlink.org");
				sprintf(HddrTo[Recipients], "To: %s@winlink.org\r\n", FullTo);
			}

			if (strlen(FullTo) > 6)
				FullTo[6] = 0;

			strlop(FullTo, '-');

			strcpy(Msg->to, FullTo);

			if (SendBBStoSYSOPCall)
				if (_stricmp(FullTo, BBSName) == 0)
					strcpy(Msg->to, SYSOPCall);

			if ((Msg->via[0] == 0 || strcmp(Msg->via, "BPQ") == 0 || strcmp(Msg->via, "BBS") == 0))
			{
				// No routing - check @BBS and WP

				struct UserInfo * ToUser = LookupCall(FullTo);

				Msg->via[0] = 0;				// In case BPQ and not found

				if (ToUser)
				{
					// Local User. If Home BBS is specified, use it

					if (ToUser->HomeBBS[0])
					{
						strcpy(Msg->via, ToUser->HomeBBS); 
					}
				}
				else
				{
					WPRecP WP = LookupWP(FullTo);

					if (WP)
					{
						strcpy(Msg->via, WP->first_homebbs);
			
					}
				}

				// Fix To: address in B2 Header

				if (Msg->via[0])
					sprintf(HddrTo[Recipients], "To: %s@%s\r\n", FullTo, Msg->via);
				else
					sprintf(HddrTo[Recipients], "To: %s\r\n", FullTo);

			}

			RecpTo=realloc(RecpTo, (Recipients+1) * sizeof(void *));
			RecpTo[Recipients] = zalloc(10);

			Via=realloc(Via, (Recipients+1) * sizeof(void *));
			Via[Recipients] = zalloc(50);

			strcpy(Via[Recipients], Msg->via);
			strcpy(RecpTo[Recipients++], FullTo);

			// Remove the To: Line from the buffer
			
		}
		else if (_memicmp(ptr1, "Type:", 4) == 0)
		{
			if (ptr1[6] == 'N')
				Msg->type = 'T';				// NTS
			else
				Msg->type = ptr1[6];
		}
		else if (_memicmp(ptr1, "Subject:", 8) == 0)
		{
			int Subjlen = ptr2 - &ptr1[9];
			if (Subjlen > 60) Subjlen = 60;
			memcpy(Msg->title, &ptr1[9], Subjlen);

			goto ProcessBody;
		}
//		else if (_memicmp(ptr1, "Body:", 4) == 0)
//		{
//			MsgLen = atoi(&ptr1[5]);
//			StartofMsg = ptr1;
//		}
		else if (_memicmp(ptr1, "File:", 5) == 0)
		{
			Msg->B2Flags |= Attachments;
		}
		else if (_memicmp(ptr1, "Date:", 5) == 0)
		{
			struct tm rtime;
			char seps[] = " ,\t\r";

			memset(&rtime, 0, sizeof(struct tm));

			// Date: 2009/07/25 10:08
	
			sscanf(&ptr1[5], "%04d/%02d/%02d %02d:%02d:%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec);

			sscanf(&ptr1[5], "%02d/%02d/%04d %02d:%02d:%02d",
					&rtime.tm_mday, &rtime.tm_mon, &rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec);

			rtime.tm_year -= 1900;

			Date = mktime(&rtime) - (time_t)_MYTIMEZONE;
	
			if (Date == (time_t)-1)
				Date = time(NULL);

			Msg->datecreated = Date;

		}

		if (linelen)			// Not Null line
		{
			ptr1 = ptr2 + 2;		// Skip cr
			goto Loop;
		}
	
		
		// Processed all headers
ProcessBody:

		ptr2 +=2;					// skip crlf

		Msg->length = &conn->MailBuffer[Msg->length] - ptr2;

		memmove(conn->MailBuffer, ptr2, Msg->length);

		CreateMessageFromBuffer(conn);

		conn->BBSFlags = 0;				// Clear ARQ Mode
		return;
	}

	// File away the data

	Buffer[MsgLen++] = 0x0a;			// BBS Msgs stored with crlf

	if ((conn->TempMsg->length + MsgLen) > conn->MailBufferSize)
	{
		conn->MailBufferSize += 10000;
		conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
		if (conn->MailBuffer == NULL)
		{
			BBSputs(conn, "*** Failed to extend Message Buffer\r");
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
		}
	}

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, MsgLen);

	conn->TempMsg->length += MsgLen;

	return;

	// Not sure what to do yet with files, but will process emails (using text style forwarding
*/
/*
ARQ:FILE::flarqmail-1.eml
ARQ:EMAIL::
ARQ:SIZE::96
ARQ::STX
//FLARQ COMPOSER
Date: 16/01/2014 22:26:06
To: g8bpq
From: 
Subject: test message

Hello
Hello

ARQ::ETX
*/

	return;
}

VOID MCastConTimer(ConnectionInfo * conn)
{
	conn->MCastListenTime--;

	if (conn->MCastListenTime == 0)
		Disconnect(conn->BPQStream);
}

VOID MCastTimer()
{
	struct MSESSION * Sess = MSessions;
	struct MSESSION * Prev = NULL;

	time_t Now = time(NULL);

	while (Sess)
	{
		if (Sess->Completed == FALSE)
			RefreshMCLine(Sess);

		if (Now - Sess->LastUpdated > MulticastMaxAge)
		{
			// remove from list

#ifndef LINBPQ
			ListView_DeleteItem(MCList, Sess->Index);
#endif
			if (Prev)
				Prev->Next = Sess->Next;
			else
				MSessions = Sess->Next;

			if (Sess->FileName)
				free(Sess->FileName);

			if (Sess->OrigTimeStamp)
				free(Sess->OrigTimeStamp);

			if (Sess->Message)
				free(Sess->Message);

			if (Sess->BlockList)
				free(Sess->BlockList);

			if (Sess->ID)
				free(Sess->ID);

			free(Sess);

			return;				// Saves messing with chain

		}
		Prev = Sess;
		Sess = Sess->Next;
	}
}

int MulticastStatusHTML(char * Reply)
{
	char StatusPage [] = 
		"<pre>ID    From      FileName        Size  %%  Time   Age   Blocklist"
		"                                                   "
		"\r\n<textarea cols=110 rows=6 name=MC>";

	char StatusTail [] = "</textarea><br><br>";
	int Len = 0;
	char Unknown[] = "???";

	struct MSESSION * Sess = MSessions;

	if (Sess ==NULL)
		return 0;

	Len = sprintf(Reply, "%s", StatusPage);

	while (Sess)
	{
		char Percent[16] = "???";
		char BlockList[51] = "";
		int i;
		char Time[80];
		char Agestring[80];
		struct tm * TM;
		time_t Age;
		char * ID = Unknown;
		char * FileName = Unknown;

		Age = time(NULL) - Sess->LastUpdated;

		TM = gmtime(&Age);

		sprintf(Agestring, "%.2d:%.2d", TM->tm_hour, TM->tm_min);

		TM = gmtime(&Sess->Created);

		sprintf(Time, "%.2d:%.2d", TM->tm_hour, TM->tm_min);

		if (Sess->MessageLen && Sess->BlockCount)
		{
			int pcent;
		
			pcent = (Sess->BlocksReceived * 100) / Sess->BlockCount;
			sprintf(Percent, "%d", pcent);
		}

		// Flag received blocks. Normalise to 50 wide 

		memset(BlockList, '.', 50);

		if (Sess->BlockList)
		{
			for (i = 0; i < 50; i++)
			{
				int posn = (i * Sess->BlockCount) / 50;
				if (Sess->BlockList[posn] == 1)
					BlockList[i] = 'Y';
			}
		}
		if (Sess->FileName)
			FileName = Sess->FileName;

		if (Sess->ID)
			ID = Sess->ID;

		Len += sprintf(&Reply[Len], "%04X %-10s%-15s%5d %-3s %s %s %s\r\n",
			Sess->Key, ID, FileName,
			Sess->MessageLen, Percent, Time, Agestring, BlockList);
			
		Sess = Sess->Next;
	}

	Len += sprintf(&Reply[Len], "%s", StatusTail);

	return Len;
}
