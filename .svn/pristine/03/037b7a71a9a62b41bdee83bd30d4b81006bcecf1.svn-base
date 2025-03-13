/*
Copyright 2001-2018 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without Fvoideven the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/	

// Mail and Chat Server for BPQ32 Packet Switch
//
// White Pages Database Support Routines

#include "bpqmail.h"

#define GetSemaphore(Semaphore,ID) _GetSemaphore(Semaphore, ID, __FILE__, __LINE__)
void _GetSemaphore(struct SEM * Semaphore, int ID, char * File, int Line);

int CurrentWPIndex;
char CurrentWPCall[10];

time_t LASTWPSendTime;


VOID DoWPUpdate(WPRec *WP, char Type, char * Name, char * HA, char * QTH, char * ZIP, time_t WPDate);
VOID Do_Save_WPRec(HWND hDlg);
VOID SaveInt64Value(config_setting_t * group, char * name, long long value);
VOID SaveIntValue(config_setting_t * group, char * name, int value);
VOID SaveStringValue(config_setting_t * group, char * name, char * value);
BOOL GetStringValue(config_setting_t * group, char * name, char * value, int maxlen);
void MQTTMessageEvent(void* message);

WPRec * AllocateWPRecord()
{
	WPRec * WP = zalloc(sizeof (WPRec));

	GetSemaphore(&AllocSemaphore, 0);

	WPRecPtr=realloc(WPRecPtr,(++NumberofWPrecs+1) * sizeof(void *));
	WPRecPtr[NumberofWPrecs]= WP;

	FreeSemaphore(&AllocSemaphore);

	return WP;
}

int BadCall(char * Call)
{
	if (_stricmp(Call, "RMS") == 0)
		return 1;

	if (_stricmp(Call, "SYSTEM") == 0)
		return 1;

	if (_stricmp(Call, "SWITCH") == 0)
		return 1;

	if (_stricmp(Call, "SYSOP") == 0)
		return 1;

	if (_memicmp(Call, "SMTP", 4) == 0)
		return 1;

	if (_memicmp(Call, "SMTP:", 5) == 0)
		return 1;

	if (_stricmp(Call, "AMPR") == 0)
		return 1;

	if (_stricmp(Call, "FILE") == 0)
		return 1;

	if (_memicmp(Call, "MCAST", 5) == 0)
		return 1;

	if (_memicmp(Call, "SYNC", 5) == 0)
		return 1;

	return 0;
}

extern config_t cfg;

VOID GetWPDatabase()
{
	WPRec WPRec;
	FILE * Handle;
	int ReadLen;
	WPRecP WP;
	char CfgName[MAX_PATH];
	long long val;
	config_t wpcfg;
	config_setting_t * group, * wpgroup;
	int i = 1;
	struct stat STAT;

	// If WP info is in main config file, use it

	group = config_lookup (&cfg, "WP");

	if (group)
	{
		// Set up control record

		WPRecPtr = malloc(sizeof(void *));
		WPRecPtr[0] = zalloc(sizeof(WPRec));
		NumberofWPrecs = 0;

		while (1)
		{
			char Key[16];
			char Record[1024]; 
			char * ptr, * ptr2;
			unsigned int n;

			sprintf(Key, "R%d", i++);

			GetStringValue(group, Key, Record, 1024);

			if (Record[0] == 0)			// End of List
				return;

			memset(&WPRec, 0, sizeof(WPRec));

			WP = &WPRec;

			ptr = Record;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->callsign[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->name[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) WP->Type = atoi(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) WP->changed = atoi(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) WP->seen = atoi(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->first_homebbs[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->secnd_homebbs[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->first_zip[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');
			if (ptr) strcpy(&WP->secnd_zip[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');

			if (ptr == NULL) continue;
			
			if (strlen(ptr) > 30)
				ptr[30] = 0;

			strcpy(&WP->first_qth[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');

			if (ptr == NULL) continue;

			if (strlen(ptr) > 30)
				ptr[30] = 0;

			strcpy(&WP->secnd_qth[0], ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');

			if (ptr) WP->last_modif = atol(ptr);

			ptr = ptr2;
			ptr2 = strlop(ptr, '|');

			if (ptr)
			{
				WP->last_seen = atol(ptr);

				// Check Call

				for (n = 1; n < strlen(WP->callsign); n++)		// skip first which may also be digit
				{
					if (isdigit(WP->callsign[n]))
					{
						// Has a digit. Check Last is not digit

						if (isalpha(WP->callsign[strlen(WP->callsign) - 1]))
						{
							WP = LookupWP(WPRec.callsign);
							if (WP == NULL)
								WP = AllocateWPRecord();

							memcpy(WP, &WPRec, sizeof(WPRec));
							goto WPOK;
						}
					}
				}
				Debugprintf("Bad WP Call %s", WP->callsign);
			}
WPOK:;
		}
		return;
	}

	// If text format exists use it

	strcpy(CfgName, WPDatabasePath);
	strlop(CfgName, '.');
	strcat(CfgName, ".cfg");

	if (stat(CfgName, &STAT) == -1)
		goto tryOld;

	config_init(&wpcfg);

	if (!config_read_file(&wpcfg, CfgName))
	{
		char Msg[256];
		sprintf(Msg, "Config File %s Line %d - %s\n", CfgName,
			config_error_line(&wpcfg), config_error_text(&wpcfg));

		printf("%s", Msg);
		config_destroy(&wpcfg);
		goto tryOld;
	}

	// Set up control record

	WPRecPtr = malloc(sizeof(void *));
	WPRecPtr[0] = zalloc(sizeof(WPRec));
	NumberofWPrecs = 0;

	while (1)
	{
		char Key[16];
		char Temp[128];

		sprintf(Key, "R%d", i++);

		wpgroup = config_lookup(&wpcfg, Key);

		if (wpgroup == NULL)			// End of List
		{
			config_destroy(&wpcfg);
			return;
		}

		memset(&WPRec, 0, sizeof(WPRec));

		GetStringValue(wpgroup, "c", WPRec.callsign, 6);
		GetStringValue(wpgroup, "n", WPRec.name, 12);

		WPRec.Type = GetIntValue(wpgroup, "T");
		WPRec.changed = GetIntValue(wpgroup, "ch");
		WPRec.seen = GetIntValue(wpgroup, "s");

		GetStringValue(wpgroup, "h", WPRec.first_homebbs, 40);
		GetStringValue(wpgroup, "sh", WPRec.secnd_homebbs, 40);
		GetStringValue(wpgroup, "z", WPRec.first_zip, 8);
		GetStringValue(wpgroup, "sz", WPRec.secnd_zip, 8);

		GetStringValue(wpgroup, "q", Temp, 30);
		Temp[30] = 0;
		strcpy(WPRec.first_qth, Temp);
	
		GetStringValue(wpgroup, "sq", Temp, 30);
		Temp[30] = 0;
		strcpy(WPRec.secnd_qth, Temp);

		val = GetIntValue(wpgroup, "m");
		WPRec.last_modif = val;
		val = GetIntValue(wpgroup, "ls");
		WPRec.last_seen = val;

		_strupr(WPRec.callsign);

		strlop(WPRec.callsign, ' ');

		if (strlen(WPRec.callsign) > 2)
		{
			if (strchr(WPRec.callsign, ':'))
				continue;

			if (BadCall(WPRec.callsign))
				continue;

			WP = LookupWP(WPRec.callsign);

			if (WP == NULL)
				WP = AllocateWPRecord();

			memcpy(WP, &WPRec, sizeof(WPRec));
		}
	}

tryOld:

	Handle = fopen(WPDatabasePath, "rb");

	if (Handle == NULL)
	{
		// Initialise a new File

		WPRecPtr = malloc(sizeof(void *));
		WPRecPtr[0] = malloc(sizeof(WPRec));
		memset(WPRecPtr[0], 0, sizeof(WPRec));
		NumberofWPrecs = 0;

		return;
	}


	// Get First Record

	ReadLen = fread(&WPRec, 1, sizeof(WPRec), Handle);

	if (ReadLen == 0)
	{
		// Duff file

		memset(&WPRec, 0, sizeof(WPRec));
	}

	// Set up control record

	WPRecPtr = malloc(sizeof(void *));
	WPRecPtr[0] = malloc(sizeof(WPRec));
	memcpy(WPRecPtr[0], &WPRec, sizeof(WPRec));

	NumberofWPrecs = 0;

Next:

	ReadLen = fread(&WPRec, 1, sizeof(WPRec), Handle);

	if (ReadLen == sizeof(WPRec))
	{
		_strupr(WPRec.callsign);

		strlop(WPRec.callsign, ' ');

		if (strlen(WPRec.callsign) > 2)
		{
			if (strchr(WPRec.callsign, ':'))
				goto Next;

			if (BadCall(WPRec.callsign))
				goto Next;

			WP = LookupWP(WPRec.callsign);

			if (WP == NULL)
				WP = AllocateWPRecord();

			memcpy(WP, &WPRec, sizeof(WPRec));
		}
		goto Next;
	}

	fclose(Handle);
	SaveWPDatabase();
}

VOID CopyWPDatabase()
{
	char Backup[MAX_PATH];
	char Orig[MAX_PATH];

	return;

	strcpy(Backup, WPDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(WPDatabasePath, Backup, FALSE);

	strcpy(Backup, WPDatabasePath);
	strlop(Backup, '.');
	strcat(Backup, ".cfg.bak");

	strcpy(Orig, WPDatabasePath);
	strlop(Orig, '.');
	strcat(Orig, ".cfg");
	CopyFile(Orig, Backup, FALSE);
}

VOID SaveWPDatabase()
{
//	SaveConfig(ConfigName);			// WP config is now in main config file

	int i;
	config_setting_t *root, *group;
	config_t cfg;
	char Key[16];
	WPRec * WP;
	char CfgName[MAX_PATH];
	long long val;

	memset((void *)&cfg, 0, sizeof(config_t));

	config_init(&cfg);

	root = config_root_setting(&cfg);

	for (i = 0; i <= NumberofWPrecs; i++)
	{
		WP = WPRecPtr[i];
		sprintf(Key, "R%d", i); 

		group = config_setting_add(root, Key, CONFIG_TYPE_GROUP);

		SaveStringValue(group, "c", &WP->callsign[0]);
		SaveStringValue(group, "n", &WP->name[0]);
		SaveIntValue(group, "T", WP->Type);
		SaveIntValue(group, "ch", WP->changed);
		SaveIntValue(group, "s", WP->seen);
		SaveStringValue(group, "h", &WP->first_homebbs[0]);
		SaveStringValue(group, "sh", &WP->secnd_homebbs[0]);
		SaveStringValue(group, "z", &WP->first_zip[0]);
		SaveStringValue(group, "sz", &WP->secnd_zip[0]);
		SaveStringValue(group, "q", &WP->first_qth[0]);
		SaveStringValue(group, "sq", &WP->secnd_qth[0]);
		val = WP->last_modif;
		SaveInt64Value(group, "m", val);
		val = WP->last_seen;
		SaveInt64Value(group, "ls", val);
	}

	strcpy(CfgName, WPDatabasePath);
	strlop(CfgName, '.');
	strcat(CfgName, ".cfg");

	Debugprintf("Saving WP Database to %s\n", CfgName);
	
	config_write_file(&cfg, CfgName);
	config_destroy(&cfg);

}

WPRec * LookupWP(char * Call)
{
	WPRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofWPrecs; i++)
	{
		ptr = WPRecPtr[i];

		if (_stricmp(ptr->callsign, Call) == 0) return ptr;
	}

	return NULL;
}

char * FormatWPDate(time_t Datim)
{
	struct tm *tm;
	static char Date[]="xx-xxx-xx ";

	tm = gmtime(&Datim);
	
	if (tm)
	{
		if (tm->tm_year >= 100)
			sprintf_s(Date, sizeof(Date), "%02d-%3s-%02d",
					tm->tm_mday, month[tm->tm_mon], tm->tm_year - 100);
		else
			sprintf_s(Date, sizeof(Date), "");
	}
	return Date;
}

#ifndef LINBPQ

int Do_WP_Sel_Changed(HWND hDlg)
{
	// Update WP display with newly selected rec

	WPRec *  WP;
	int Sel = SendDlgItemMessage(hDlg, IDC_WP, CB_GETCURSEL, 0, 0);
	char Type[] = " ";

	if (Sel == -1)
		SendDlgItemMessage(hDlg, IDC_WP, WM_GETTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentWPCall);
	else
		SendDlgItemMessage(hDlg, IDC_WP, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentWPCall);
	
	for (CurrentWPIndex = 1; CurrentWPIndex <= NumberofWPrecs; CurrentWPIndex++)
	{
		WP = WPRecPtr[CurrentWPIndex];

		if (_stricmp(WP->callsign, CurrentWPCall) == 0)
		{

			SetDlgItemText(hDlg, IDC_WPNAME, WP->name);
			SetDlgItemText(hDlg, IDC_HOMEBBS1, WP->first_homebbs);
			SetDlgItemText(hDlg, IDC_HOMEBBS2, WP->secnd_homebbs);
			SetDlgItemText(hDlg, IDC_QTH1, WP->first_qth);
			SetDlgItemText(hDlg, IDC_QTH2, WP->secnd_qth);
			SetDlgItemText(hDlg, IDC_ZIP1, WP->first_zip);
			SetDlgItemText(hDlg, IDC_ZIP2, WP->secnd_zip);
			Type[0] = WP->Type;
			SetDlgItemText(hDlg, IDC_TYPE, Type);
			SetDlgItemInt(hDlg, IDC_CHANGED, WP->changed, FALSE);
			SetDlgItemInt(hDlg, IDC_SEEN, WP->seen, FALSE);

			SetDlgItemText(hDlg, IDC_LASTSEEN, FormatWPDate(WP->last_seen));
			SetDlgItemText(hDlg, IDC_LASTMODIFIED, FormatWPDate(WP->last_modif));
	
			return 0;
		}
	}

	CurrentWPIndex = -1;

	return 0;
}

INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


VOID Do_Save_WPRec(HWND hDlg)
{
	WPRec * WP;
	char Type[] = " ";
	BOOL OK1;

	if (CurrentWPIndex == -1)
	{
		sprintf(InfoBoxText, "Please select a WP Record to save");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	WP = WPRecPtr[CurrentWPIndex];

	if (strcmp(CurrentWPCall, WP->callsign) != 0)
	{
		sprintf(InfoBoxText, "Inconsistancy detected - record not saved");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	GetDlgItemText(hDlg, IDC_WPNAME, WP->name, 13);
	GetDlgItemText(hDlg, IDC_HOMEBBS1, WP->first_homebbs, 41);
	GetDlgItemText(hDlg, IDC_HOMEBBS2, WP->secnd_homebbs, 41);
	GetDlgItemText(hDlg, IDC_QTH1, WP->first_qth, 31);
	GetDlgItemText(hDlg, IDC_QTH2, WP->secnd_qth, 31);
	GetDlgItemText(hDlg, IDC_ZIP1, WP->first_zip, 31);
	GetDlgItemText(hDlg, IDC_ZIP2, WP->secnd_zip, 31);
	WP->last_modif = time(NULL);
	WP->seen = GetDlgItemInt(hDlg, IDC_SEEN, &OK1, FALSE);

	WP->Type = 'U';
	WP->changed = 1;

	SaveWPDatabase();

	sprintf(InfoBoxText, "WP information saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);				
}

VOID Do_Delete_WPRec(HWND hDlg)
{
	WPRec * WP;
	int n;

	if (CurrentWPIndex == -1)
	{
		sprintf(InfoBoxText, "Please select a WP Record to delete");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	WP = WPRecPtr[CurrentWPIndex];

	if (strcmp(CurrentWPCall, WP->callsign) != 0)
	{
		sprintf(InfoBoxText, "Inconsistancy detected - record not deleted");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	for (n = CurrentWPIndex; n < NumberofWPrecs; n++)
	{
		WPRecPtr[n] = WPRecPtr[n+1];		// move down all following entries
	}
	
	NumberofWPrecs--;

	SendDlgItemMessage(hDlg, IDC_WP, CB_RESETCONTENT, 0, 0);

	for (n = 1; n <= NumberofWPrecs; n++)
	{
		SendDlgItemMessage(hDlg, IDC_WP, CB_ADDSTRING, 0, (LPARAM)WPRecPtr[n]->callsign);
	} 


	sprintf(InfoBoxText, "WP record for %s deleted", WP->callsign);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	free(WP);

	SaveWPDatabase();

	return;

}

INT_PTR CALLBACK WPEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, n;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (n = 1; n <= NumberofWPrecs; n++)
		{
			SendDlgItemMessage(hDlg, IDC_WP, CB_ADDSTRING, 0, (LPARAM)WPRecPtr[n]->callsign);
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

		case IDC_WP:

			// Msg Selection Changed

			Do_WP_Sel_Changed(hDlg);

			return TRUE;

		case IDC_SAVEWP:

			Do_Save_WPRec(hDlg);
			return TRUE;

		case IDC_DELETEWP:

			Do_Delete_WPRec(hDlg);
			return TRUE;
		}
		break;
	}
	
	return (INT_PTR)FALSE;
}
#endif

VOID GetWPBBSInfo(char * Rline)
{
	// Update WP with /I records for each R: Line

	// R:111206/1636Z 29130@N9PMO.#SEWI.WI.USA.NOAM [Racine, WI] FBB7.00i

	struct tm rtime;
	time_t RLineTime;
	int Age;
		
	WPRec * WP;
	char ATBBS[200];
	char Call[200];
	char QTH[200] = "";
	int RLen;

	char * ptr1; 
	char * ptr2;


	memset(&rtime, 0, sizeof(struct tm));

	if (Rline[10] == '/')
	{
		// Dodgy 4 char year
	
		sscanf(&Rline[2], "%04d%02d%02d/%02d%02d",
				&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);
				rtime.tm_year -= 1900;
				rtime.tm_mon--;
	}
	else if (Rline[8] == '/')
	{
		sscanf(&Rline[2], "%02d%02d%02d/%02d%02d",
			&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);

		if (rtime.tm_year < 90)
			rtime.tm_year += 100;		// Range 1990-2089
		rtime.tm_mon--;
	}

	// Otherwise leave date as zero, which should be rejected

	if ((RLineTime = mktime(&rtime)) != (time_t)-1 )
	{
		Age = (time(NULL) - RLineTime)/86400;

		if ( Age < -1)
			return;			// in the future
		
		if (Age > BidLifetime || Age > MaxAge)
			return;			// Too old
	}

	ptr1 = strchr(Rline, '@'); 
	ptr2 = strchr(Rline, '\r');

	if (!ptr1)
		return;			// Duff

	if (*++ptr1 == ':')
		ptr1++;			// Format 2


	if (ptr2 == NULL)
		return;			// No CR on end
	
	RLen = ptr2 - ptr1;

	if (RLen > 200)
		return;

	memcpy(ATBBS, ptr1, RLen);
	ATBBS[RLen] = 0;

	ptr2 = strchr(ATBBS,  ' ');

	if (ptr2)
		*ptr2 = 0;

	strcpy(Call, ATBBS);
	strlop(Call, '.');

	ptr2 = memchr(ptr1, '[', RLen);

	if (ptr2)
	{
		ptr1= memchr(ptr2, ']', RLen);
		if (ptr1)
			memcpy(QTH, ptr2 + 1, ptr1 - ptr2 - 1);
	}

	if (BadCall(Call))
		return;

	WP = LookupWP(Call);

	if (!WP)
	{
		// Not Found

		WP = AllocateWPRecord();

		strcpy(WP->callsign, Call);
		strcpy(WP->first_homebbs, ATBBS);
		strcpy(WP->secnd_homebbs, ATBBS);

		if (QTH[0])
		{
			strcpy(WP->first_qth, QTH);
			strcpy(WP->secnd_qth, QTH);
		}

		WP->last_modif = RLineTime;
		WP->last_seen = RLineTime;

		WP->Type = 'I';
		WP->changed = TRUE;

		return;
	}

	if (WP->last_modif >= RLineTime	|| WP->Type != 'I')
		return;

	// Update 2nd if changed

	if (strcmp(WP->secnd_homebbs , ATBBS) != 0)
	{
		strcpy(WP->secnd_homebbs, ATBBS);
		WP->last_modif = RLineTime;
	}

	if (QTH[0] && strcmp(WP->secnd_qth , QTH) != 0)
	{
		strcpy(WP->secnd_qth, QTH);
		WP->last_modif = RLineTime;
	}

	return;
}




VOID GetWPInfoFromRLine(char * From, char * FirstRLine, time_t RLineTime)
{
	/* The /G suffix denotes that the information in this line has been gathered by examining
	the header of a message to GUESS at which BBS the sender is registered. The HomeBBS of the User
	is assumed to be the BBS shown in the first R: header line. The date associated with this 
	information is the date shown on this R: header line.
	*/

	// R:930101/0000 1530@KA6FUB.#NOCAL.CA.USA.NOAM

	// R:930101/0000 @:KA6FUB.#NOCAL.CA.USA.NOAM #:1530

	// The FirstRLine pointer points to the message, so shouldnt be changed

	WPRec * WP;
	char ATBBS[200];
	int RLen;

	char * ptr1 = strchr(FirstRLine, '@'); 
	char * ptr2 = strchr(FirstRLine, '\r');

	if (BadCall(From))
		return;

	if (!ptr1)
		return;			// Duff

	if (*++ptr1 == ':')
		ptr1++;			// Format 2

	RLen = ptr2 - ptr1;

	if (RLen > 200)
		return;

	memcpy(ATBBS, ptr1, RLen);
	ATBBS[RLen] = 0;

	ptr2 = strchr(ATBBS,  ' ');

	if (ptr2)
		*ptr2 = 0;

	if (strlen(ATBBS) > 40)
		ATBBS[40] = 0;

	WP = LookupWP(From);

	if (!WP)
	{
		// Not Found

		WP = AllocateWPRecord();

		strcpy(WP->callsign, From);
		strcpy(WP->first_homebbs, ATBBS);
		strcpy(WP->secnd_homebbs, ATBBS);

		WP->last_modif = RLineTime;
		WP->last_seen = RLineTime;

		WP->Type = 'G';
		WP->changed = TRUE;

		return;
	}

	if (WP->last_modif >= RLineTime)
		return;

	// Update 2nd if changed

	if (strcmp(WP->secnd_homebbs , ATBBS) != 0)
	{
		strcpy(WP->secnd_homebbs, ATBBS);
		WP->last_modif = RLineTime;
	}

	return;
}

VOID ProcessWPMsg(char * MailBuffer, int Size, char * FirstRLine)
{
	char * ptr1 = MailBuffer;
	char * ptr2;
	WPRec * WP;
	char WPLine[200];
	int WPLen;

	ptr1[Size] = 0;

	ptr1 = FirstRLine;

	if (ptr1 == NULL)
		return;

	while (*ptr1)
	{
		ptr2 = strchr(ptr1, '\r');

		if (ptr2 == 0)	//  No CR in buffer
			return;

		WPLen =	ptr2 - ptr1;

		if (WPLen > 128)
			return;

		if ((memcmp(ptr1, "On ", 3)  == 0) && (WPLen < 200))
		{
			char * Date;
			char * Call;
			char * AT;
			char * HA;
			char * zip;
			char * ZIP;
			char * Name;
			char * QTH = NULL;
			char * Context;
			char seps[] = " \r";

			// Make copy of string, as strtok messes with it
			
			memcpy(WPLine, ptr1, WPLen);
			WPLine[WPLen] = 0;

			Date = strtok_s(WPLine+3, seps, &Context);
			Call = strtok_s(NULL, seps, &Context);
			AT = strtok_s(NULL, seps, &Context);
			HA = strtok_s(NULL, seps, &Context);
			zip = strtok_s(NULL, seps, &Context);
			ZIP = strtok_s(NULL, seps, &Context);
			Name = strtok_s(NULL, seps, &Context);
			QTH = strtok_s(NULL, "\r", &Context);			// QTH may have spaces

			if (Date == 0 || Call == 0 || AT == 0 || ZIP == 0 || Name == 0 || QTH == 0)
				return;

			if (strlen(HA) > 40)
				return;
			if (strlen(ZIP) > 8)
				return;
			if (strlen(Name) > 12)
				return;
			if (strlen(QTH) > 30)
				return;

			if (AT[0] == '@' && (QTH))
			{
				struct tm rtime;
				time_t WPDate;
				char Type;
				char * TypeString;

				if (memcmp(Name, "?", 2) == 0) Name = NULL;
				if (memcmp(ZIP, "?", 2) == 0) ZIP = NULL;
				if (memcmp(QTH, "?", 2) == 0) QTH = NULL;

				memset(&rtime, 0, sizeof(struct tm));
			
				sscanf(Date, "%02d%02d%02d",
				&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday);

				rtime.tm_year += 100;
				rtime.tm_mon--;

/*
This process freshens the database, following receipt of the new or changed information detailed above.

The update subroutine will first look for an entry in the database for the callsign which matches the received information.
If it does not exist then a completely new record will be created in the database and the information be used to fill what
fields it can, in both the active and the temporary components. The date will be then changed to the one associated with the
update information.

If the record does already exist, then the unknown fields of both the temporary and active fields will be filled in, and 
those fields already known in the temporary part will be replaced by the new information if the date new information is
younger than that already on file. The date will then be
adjusted such that it is consistent with the updated information.

If the new information is of the /U category, then the current fields will be replaced by the new information in both
the primary and secondary (Active and Temporary) parts of the record, as this information has been input directly from
the user. If the information was of another category then only the secondary (Temporary) part of the record will be
updated, so the Active or primary record will remain unchanged at this time.

If a field is changed, a flag giving the update request type is then validated. If the /U flag is already validated,
it will not be replaced. This flag will be used in case the WP update messages are validated.
*/
				if ((WPDate = mktime(&rtime)) != (time_t)-1 )
				{
					WPDate -= (time_t)_MYTIMEZONE;
					TypeString = strlop(Call, '/');
					
					if (strlen(Call) < 3 || strlen(Call) > 6)
						return;

					if (TypeString)
						Type = TypeString[0];
					else
						Type = 'G';

					if (strchr(Call, ':'))
						break;

					if (BadCall(Call))
						break;

					WP = LookupWP(Call);

					if (WP)
					{
						// Found, so update 

						DoWPUpdate(WP, Type, Name, HA, QTH, ZIP, WPDate);
					}
					else
					{
						WP = AllocateWPRecord();

						strcpy(WP->callsign, Call);
						if (Name) strcpy(WP->name, Name);
						strcpy(WP->first_homebbs, HA);
						strcpy(WP->secnd_homebbs, HA);

						if (QTH)
						{
							strcpy(WP->first_qth, QTH);
							strcpy(WP->secnd_qth, QTH);;
						}
						if (ZIP)
						{
							strcpy(WP->first_zip, ZIP);
							strcpy(WP->secnd_zip, ZIP);
						}

						WP->Type = Type;
						WP->last_modif = WPDate;
						WP->last_seen = WPDate;
						WP->changed = TRUE;
						WP->seen++;
					}
				}	
			}
		}
		
		ptr1 = ++ptr2;
		if (*ptr1 == '\n')
			ptr1++;
	}

	SaveWPDatabase();

	return;
}

VOID DoWPUpdate(WPRec * WP, char Type, char * Name, char * HA, char * QTH, char * ZIP, time_t WPDate)
{
	// First Update any unknown field

	if(Name)
		if (WP->name == NULL) {strcpy(WP->name, Name); WP->last_modif = WPDate; WP->changed = TRUE;}

	if (QTH)
	{
		if (WP->first_qth == NULL) {strcpy(WP->first_qth, QTH); WP->last_modif = WPDate; WP->changed = TRUE;}
		if (WP->secnd_qth == NULL) {strcpy(WP->secnd_qth, QTH); WP->last_modif = WPDate; WP->changed = TRUE;}
	}
	if (ZIP)
	{
		if (WP->first_zip == NULL) {strcpy(WP->first_zip, ZIP); WP->last_modif = WPDate; WP->changed = TRUE;}
		if (WP->secnd_zip == NULL) {strcpy(WP->secnd_zip, ZIP); WP->last_modif = WPDate; WP->changed = TRUE;}
	}
	
	WP->last_seen = WPDate;
	WP->seen++;

	// Now Update Temp Fields if update is newer than original

	if (WP->last_modif >= WPDate)
		return;

	if (Type == 'U')					// Definitive Update
	{
		if (strcmp(WP->first_homebbs , HA) != 0)
		{
			strcpy(WP->first_homebbs, HA); strcpy(WP->secnd_homebbs, HA); WP->last_modif = WPDate; WP->changed = TRUE;
		}

		 if (Name)
		 {
			 if (strcmp(WP->name , Name) != 0)
			 {
				strcpy(WP->name, Name); 
				WP->last_modif = WPDate; 
				WP->changed = TRUE;
			}
		 }

		if (QTH)
		{
			if (strcmp(WP->first_qth , QTH) != 0)
			{
				strcpy(WP->first_qth, QTH);
				strcpy(WP->secnd_qth, QTH);
				WP->last_modif = WPDate;
				WP->changed = TRUE;
			}
		}

		if (ZIP)
		{
			if (strcmp(WP->first_zip , ZIP) != 0)
			{
				strcpy(WP->first_zip, ZIP);
				strcpy(WP->secnd_zip, ZIP);
				WP->last_modif = WPDate;
				WP->changed = TRUE;
			}
		}

		WP->Type = Type;

		return;
	}

	// Non-Definitive - only update second copy

	if (strcmp(WP->secnd_homebbs , HA) != 0) {strcpy(WP->secnd_homebbs, HA); WP->last_modif = WPDate; WP->Type = Type;}

	if (Name)
		if (strcmp(WP->name , Name) != 0) {strcpy(WP->name, Name); WP->last_modif = WPDate; WP->Type = Type;}

	if (QTH)
		if (strcmp(WP->secnd_qth , QTH) != 0) {strcpy(WP->secnd_qth, QTH); WP->last_modif = WPDate; WP->Type = Type;}

	if (ZIP)
		if (strcmp(WP->secnd_zip , ZIP) != 0) {strcpy(WP->secnd_zip, ZIP); WP->last_modif = WPDate; WP->Type = Type;}
						
	return;
}

VOID UpdateWPWithUserInfo(struct UserInfo * user)
{
	WPRec * WP = LookupWP(user->Call);

	if (strchr(user->Call, ':'))
		return;

	if (BadCall(user->Call))
		return;

	if (!WP)
	{
		WP = AllocateWPRecord();
		strcpy(WP->callsign, user->Call);
	}

	// Update Record

	if (user->HomeBBS[0])
	{
		strcpy(WP->first_homebbs, user->HomeBBS);
		strcpy(WP->secnd_homebbs, user->HomeBBS);
	}

	if (user->Address[0])
	{
		char Temp[127];
		strcpy(Temp, user->Address);
		Temp[30] = 0;

		strcpy(WP->first_qth, Temp);
		strcpy(WP->secnd_qth, Temp);
	}

	if (user->ZIP[0])
	{
		strcpy(WP->first_zip, user->ZIP);
		strcpy(WP->secnd_zip, user->ZIP );
	}

	if (user->Name[0])
		strcpy(WP->name, user->Name);

	WP->last_modif = WP->last_seen = time(NULL);

	WP->changed = TRUE;
	WP->Type = 'U';

	SaveWPDatabase();

}

VOID DoWPLookup(ConnectionInfo * conn, struct UserInfo * user, char Type, char *Param)
{
	// Process the I call command

	WPRec * ptr = NULL;
	int i;
	char ATBBS[100];
	char * HA;
	char * Rest;

	_strupr(Param);
	Type = toupper(Type);

	switch (Type)
	{
	case 0:

		for (i=1; i <= NumberofWPrecs; i++)
		{
			ptr = WPRecPtr[i];

			if (wildcardcompare(ptr->callsign, Param))
			{
				nodeprintf(conn, "%s  %s %s %s %s\r", ptr->callsign, ptr->first_homebbs,
					ptr->name, ptr->first_zip, ptr->first_qth);
			}
		}
		
		return;

	case'@':			// AT BBS

		for (i=1; i <= NumberofWPrecs; i++)
		{
			ptr = WPRecPtr[i];

			strcpy(ATBBS, ptr->first_homebbs);
			strlop(ATBBS,'.');

			if (wildcardcompare(ATBBS, Param))
			{
				nodeprintf(conn, "%s  %s %s %s %s\r", ptr->callsign, ptr->first_homebbs, ptr->name, ptr->first_zip, ptr->first_qth);
			}
		}
		
	case'H':			// Hierarchic Element

		for (i=1; i <= NumberofWPrecs; i++)
		{
			ptr = WPRecPtr[i];

			strcpy(ATBBS, ptr->first_homebbs);

			Rest = strlop(ATBBS,'.');

			if (Rest == 0)
				continue;

			HA = strtok_s(Rest, ".", &Rest);

			while (HA)
			{
				if (wildcardcompare(HA, Param))
				{
					nodeprintf(conn, "%s  %s %s %s %s\r", ptr->callsign, ptr->first_homebbs, ptr->name, ptr->first_zip, ptr->first_qth);
				}
				
				HA = strtok_s(NULL, ".", &Rest);
			}
		}
		return;

	case'Z':			// ZIP

		for (i=1; i <= NumberofWPrecs; i++)
		{
			ptr = WPRecPtr[i];

			if (ptr->first_zip[0] == 0)
				continue;

			if (wildcardcompare(ptr->first_zip, Param))
			{
				nodeprintf(conn, "%s  %s %s %s %s\r", ptr->callsign, ptr->first_homebbs, ptr->name, ptr->first_zip, ptr->first_qth);
			}
		}
		return;
	}
	nodeprintf(conn, "Invalid I command option %c\r", Type);
}
/*
On 111120 N4ZKF/I @ N4ZKF.#NFL.FL.USA.NOAM zip 32118 Dave 32955
On 111120 F6IQF/I @ F6IQF.FRPA.FRA.EU zip ? ? f6iqf.dyndns.org
On 111121 W9JUN/I @ W9JUN.W9JUN.#SEIN.IN.US.NOAM zip 47250 Don NORTH VERNON, IN
On 111120 KR8ZY/U @ KR8ZY zip ? john ?
On 111120 N0DEC/G @ N0ARY.#NCA.CA.USA.NOAM zip ? ? ?

From: N0JAL
To: WP
Type/Status: B$
Date/Time: 22-Nov 10:15Z
Bid: 95F7N0JAL
Title: WP Update

R:111122/1500Z 35946@KD6PGI.OR.USA.NOAM BPQ1.0.4
R:111122/1020 16295@K7ZS.OR.USA.NOAM
R:111122/1015 38391@N0JAL.OR.USA.NOAM

On 111121 N0JAL @ N0JAL.OR.USA.NOAM zip ? Sai Damascus, Oregon CN85sj

*/

VOID UpdateWP()
{
	// If 2nd copy of info has been unchanged for 30 days, copy to active fields

	WPRec * ptr = NULL;
	int i;
	char * via = NULL;
	int MsgLen = 0;
	char MailBuffer[100100];
	char * Buffptr = MailBuffer;
	int WriteLen=0;
	char HDest[61] = "WP";
	char WPMsgType = 'P';
	time_t NOW = time(NULL);
	time_t UpdateLimit = NOW - (86400 * 30);	// 30 days
	LASTWPSendTime = NOW - (86400 * 5);		// 5 days max

	for (i=1; i <= NumberofWPrecs; i++)
	{
		ptr = WPRecPtr[i];

		// DO we have a new field, and if so is it different?

		if ((ptr->secnd_homebbs[0] && _stricmp(ptr->first_homebbs, ptr->secnd_homebbs))
			|| (ptr->secnd_qth[0] && _stricmp(ptr->first_qth, ptr->secnd_qth))
			|| (ptr->secnd_zip[0] && _stricmp(ptr->first_zip, ptr->secnd_zip)))
		{
			// Have new data

			if (ptr->last_modif < UpdateLimit)
			{
				// Stable for 30 days

				if (ptr->secnd_homebbs[0])
					strcpy(ptr->first_homebbs, ptr->secnd_homebbs);
				if (ptr->secnd_qth[0])
					strcpy(ptr->first_qth, ptr->secnd_qth);
				if (ptr->secnd_zip[0])
					strcpy(ptr->first_zip, ptr->secnd_zip);
				
				ptr->last_modif = NOW;
		
			}
		}
	}
}

int CreateWPMessage()
{
	// Include all messages with Type of U whach have changed since LASTWPSendTime

	WPRec * ptr = NULL;
	int i;
	struct tm *tm;
	struct MsgInfo * Msg;
	char * via = NULL;
	char BID[13];
	BIDRec * BIDRec;
	int MsgLen = 0;
	char MailBuffer[100100];
	char * Buffptr = MailBuffer;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	int WriteLen=0;
	char ** To = SendWPAddrs;

	LASTWPSendTime = time(NULL) - (86400 * 5);		// 5 days max

	for (i=1; i <= NumberofWPrecs; i++)
	{
		ptr = WPRecPtr[i];

//		if (ptr->last_modif > LASTWPSendTime && ptr->Type == 'U' && ptr->first_homebbs[0])
		if (ptr->changed && ptr->last_modif > LASTWPSendTime && ptr->first_homebbs[0])
		{
			tm = gmtime((time_t *)&ptr->last_modif);
			MsgLen += sprintf(Buffptr, "On %02d%02d%02d %s/%c @ %s zip %s %s %s\r\n", 
				tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
				ptr->callsign, ptr->Type, ptr->first_homebbs,
				(ptr->first_zip[0]) ? ptr->first_zip : "?",
				(ptr->name[0]) ? ptr->name : "?",
				(ptr->first_qth[0]) ? ptr->first_qth : "?");

			Buffptr = &MailBuffer[MsgLen];

			ptr->changed = FALSE;

			if (MsgLen > 100000)
				break;
		}
	}

	if (MsgLen == 0)
		return TRUE;

	while(To[0])
	{
		char TO[256];
		char * VIA;
		
		Msg = AllocateMsgRecord();

		// Set number here so they remain in sequence
		
		Msg->number = ++LatestMsg;
		Msg->length = MsgLen;
		MsgnotoMsg[Msg->number] = Msg;

		strcpy(Msg->from, BBSName);	

		strcpy(TO, To[0]);
			
		VIA = strlop(TO, '@');

		if (VIA)
		{
			if (strlen(VIA) > 40)
				VIA[40] = 0;
			strcpy(Msg->via, VIA);
		}
		strcpy(Msg->to, TO); 

		strcpy(Msg->title, "WP Update");

		Msg->type = (SendWPType) ? 'P' : 'B';
		Msg->status = 'N';
				
		sprintf_s(BID, sizeof(BID), "%d_%s", LatestMsg, BBSName);

		strcpy(Msg->bid, BID);

		Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->u.msgno = LOWORD(Msg->number);
		BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

		sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
		hFile = fopen(MsgFile, "wb");
	
		if (hFile)
		{
			fwrite(MailBuffer, 1, Msg->length, hFile);
			fclose(hFile);
		}

		MatchMessagetoBBSList(Msg, 0);

		BuildNNTPList(Msg);				// Build NNTP Groups list

#ifndef NOMQTT
		if (MQTT)
			MQTTMessageEvent(Msg);
#endif
		To++;
	}

	SaveMessageDatabase();
	SaveBIDDatabase();

	return TRUE;
}

VOID CreateWPReport()
{
	int i;
	char Line[200];
	int len;
	char File[100];
	FILE * hFile;
	WPRec * WP = NULL;

	sprintf_s(File, sizeof(File), "%s/wp.txt", BaseDir);
	
	hFile = fopen(File, "wb");

	if (hFile == NULL)
		return;

//	len = sprintf(Line, "    Call    Connects  Connects  Messages   Messages   Bytes      Bytes     Rejected  Rejected\r\n");
//	WriteFile(hFile, Line, len, &written, NULL);
//	len = sprintf(Line, "               In        Out     Rxed        Sent     Rxed       Sent         In         Out\r\n\r\n");
//	WriteFile(hFile, Line, len, &written, NULL);
		

	for (i=1; i <= NumberofWPrecs; i++)
	{
		WP = WPRecPtr[i];

		len = sprintf(Line, "%-7s,%c,%s,%s,%s,%s,%s,%s,%s,%d,%s,%s\r\n",
			WP->callsign, WP->Type, WP->first_homebbs, WP->first_qth, WP->first_zip, 
			WP->secnd_homebbs, WP->secnd_qth, WP->secnd_zip, WP->name, WP->changed,
			FormatWPDate((time_t)WP->last_modif),
			FormatWPDate((time_t)WP->last_seen));
		
		fwrite(Line, 1, len, hFile);
	}
	fclose(hFile);
}






