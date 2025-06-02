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

#define _CRT_SECURE_NO_DEPRECATE 

#include "cheaders.h"
#include "bpqmail.h"

#define MAIL
#include "httpconnectioninfo.h"

#ifdef WIN32
//#include "C:\Program Files (x86)\GnuWin32\include\iconv.h"
#else
#include <iconv.h>
#include <dirent.h>
#endif

static struct HTTPConnectionInfo * FindSession(char * Key);
int APIENTRY SessionControl(int stream, int command, int param);
int SetupNodeMenu(char * Buff);
VOID SetMultiStringValue(char ** values, char * Multi);
char * GetTemplateFromFile(int Version, char * FN);
VOID FormatTime(char * Time, time_t cTime);
struct MsgInfo * GetMsgFromNumber(int msgno);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP);
BOOL OkToKillMessage(BOOL SYSOP, char * Call, struct MsgInfo * Msg);
int DisplayWebForm(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, char * FileName, char * XML, char * Reply, char * RawMessage, int RawLen);
struct HTTPConnectionInfo * AllocateWebMailSession();
VOID SaveNewMessage(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest, int InputLen);
void ConvertTitletoUTF8(WebMailInfo * WebMail, char * Title, char * UTF8Title, int Len);
char *stristr (char *ch1, char *ch2);
char * ReadTemplate(char * FormSet, char * DirName, char * FileName);
VOID DoStandardTemplateSubsitutions(struct HTTPConnectionInfo * Session, char * txtFile);
BOOL CheckifPacket(char * Via);
int GetHTMLFormSet(char * FormSet);
void ProcessFormInput(struct HTTPConnectionInfo * Session, char * input, char * Reply, int * RLen, int InputLen);
char * WebFindPart(char ** Msg, char * Boundary, int * PartLen, char * End);
struct HTTPConnectionInfo * FindWMSession(char * Key);
int SendWebMailHeaderEx(char * Reply, char * Key, struct HTTPConnectionInfo * Session, char * Alert);
char * BuildFormMessage(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, 	char * Keys[1000], char * Values[1000], int NumKeys);
char * FindXMLVariable(WebMailInfo * WebMail, char * Var);
int ReplyToFormsMessage(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, char * Reply, BOOL Reenter);
BOOL ParsetxtTemplate(struct HTTPConnectionInfo * Session, struct HtmlFormDir * Dir, char * FN, BOOL isReply);
VOID UpdateFormAction(char * Template, char * Key);
BOOL APIENTRY GetAPRSLatLon(double * PLat,  double * PLon);
BOOL APIENTRY GetAPRSLatLonString(char * PLat,  char * PLon);
void FreeWebMailFields(WebMailInfo * WebMail);
VOID BuildXMLAttachment(struct HTTPConnectionInfo * Session, char * Keys[1000], char * Values[1000], int NumKeys);
VOID SaveTemplateMessage(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID DownloadAttachments(struct HTTPConnectionInfo * Session, char * Reply, int * RLen, char * Rest);
VOID getAttachmentList(struct HTTPConnectionInfo * Session, char * Reply, int * RLen, char * Rest);
char * BuildB2Header(WebMailInfo * WebMail, struct MsgInfo * Msg, char ** ToCalls, int Calls);
VOID FormatTime2(char * Time, time_t cTime);
VOID ProcessSelectResponse(struct HTTPConnectionInfo * Session, char * URLParams);
VOID ProcessAskResponse(struct HTTPConnectionInfo * Session, char * URLParams);
char * CheckFile(struct HtmlFormDir * Dir, char * FN);
VOID GetPage(struct HTTPConnectionInfo * Session, char * NodeURL);
VOID SendTemplateSelectScreen(struct HTTPConnectionInfo * Session, char *URLParams, int InputLen);
BOOL isAMPRMsg(char * Addr);
char * doXMLTransparency(char * string);
Dll BOOL APIENTRY APISendAPRSMessage(char * Text, char * ToCall);
void SendMessageReadEvent(char * Call, struct MsgInfo * Msg);
void SendNewMessageEvent(char * call, struct MsgInfo * Msg);
void MQTTMessageEvent(void* message);

extern char NodeTail[];
extern char BBSName[10];

extern char LTFROMString[2048];
extern char LTTOString[2048];
extern char LTATString[2048];

 UCHAR BPQDirectory[260];

int LineCount = 35;					// Lines per page on message list

// Forms 

struct HtmlFormDir ** HtmlFormDirs = NULL;
int FormDirCount = 0;

struct HtmlForm
{
	char * FileName;
	BOOL HasInitial;
	BOOL HasViewer;
	BOOL HasReply;
	BOOL HasReplyViewer;
};

struct HtmlFormDir
{
	char * FormSet;
	char * DirName;
	struct HtmlForm ** Forms;
	int FormCount;
	struct HtmlFormDir ** Dirs;		// Nested Directories
	int DirCount;
};


char FormDirList[4][MAX_PATH] = {"Standard_Templates", "Standard Templates", "Local_Templates"};

static char PassError[] = "<p align=center>Sorry, User or Password is invalid - please try again</p>";
static char BusyError[] = "<p align=center>Sorry, No sessions available - please try later</p>";

extern char MailSignon[];

char WebMailSignon[] = "<html><head><title>BPQ32 Mail Server Access</title></head><body background='/background.jpg'>"
	"<h3 align=center>BPQ32 Mail Server %s Access</h3>"
	"<h3 align=center>Please enter Callsign and Password to access WebMail</h3>"
	"<form method=post action=/WebMail/Signon>"
	"<table align=center  bgcolor=white>"
	"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
	"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
	"<p align=center><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></form>";

static char MsgInputPage[] = "<html><head><meta content=\"text/html; charset=UTF-8\" http-equiv=\"content-type\">"
	"<title></title><script src='/WebMail/webscript.js'></script>"
	"<style type=\"text/css\">"
	"input.btn:active {background:black;color:white;} "
	"submit.btn:active {background:black;color:white;} "
	"</style>"
	"</head>"
	"<body background=/background.jpg onload='initialize(185)' onresize='initialize(185)'>"
	"<h3 align=center>Webmail Interface - Message Input Form</h3>"
	"<form align=center id=myform style=\"font-family: monospace; \" method=post action=/WebMail/EMSave\?%s enctype=multipart/form-data>"
	"<div style='text-align: center;'><div style='display: inline-block;'><span style='display:block; text-align: left;'>"
	"To &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input size=60 id='To' name='To' value='%s'>%s<br>"
	"Subject <input size=60 id='Subj' name='Subj' value=\"%s\"> &nbsp; &nbsp; &nbsp;"
//	"<button onclick='document.getElementById('getFile').click()'>Attach Files</button>"
//	"<input type='file' id='getFile' name='myFile[]' multiple style='display:none'><br>"
	"<label for='myfile'>Attachments </label><input type='file'   name='myFile[]' multiple>"
	"<br>Type &nbsp;&nbsp;&nbsp;"
	"<select tabindex=1 size=1 name=Type><option value=P>P</option>"
	"<option value=B>B</option><option value=T>T</option></select>"
	" BID <input name=BID><br><label for='myfile2'>Header &nbsp;</label>"
	"<input type='file' name='myFile2[]' style='width: 220px'>"
	"<label for='myfile3'> Footer </label><input type='file' name='myFile3[]'>"
	"</span></div>"
	"<textarea id='main' name=Msg style='overflow:auto;'>%s</textarea><br>"
	"<input name=Send value=Send type=submit class='btn'> <input name=Cancel value=Cancel type=submit class='btn'></div></form>";

static char CheckFormMsgPage[] = "<html><head><meta content=\"text/html; charset=UTF-8\" http-equiv=\"content-type\">"
	"<title></title><script src='/WebMail/webscript.js'></script></head>"
	"<body background=/background.jpg onload='initialize(210)' onresize='initialize(210)'>"
	"<h3 align=center>Webmail Forms Interface - Check Message</h3>"
	"<form align=center id=myform style=\"font-family: monospace; \"method=post action=/WebMail/FormMsgSave\?%s>"

	"<div style='text-align: center;'><div style='display: inline-block;'><span style='display:block; text-align: left;'>"
	"To &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input size=60 id='To' name='To' value='%s'><br>"
	"CC &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input size=60 id='CC' name='CC' value='%s'><br>"
	"Subject <input size=60 id='Subj' name='Subj' value='%s'>"
//"<input type='file' name='myFile' multiple>"
	"<br>Type &nbsp;&nbsp;&nbsp;"
	"<select tabindex=1 size=1 name=Type><option value=P %s>P</option>"
	"<option value=B %s>B</option><option value=T %s>T</option></select>"
	" BID <input name=BID value='%s'><br><br>"
	"</span></div>"

	"<textarea id='main' name=Msg style='overflow:auto;'>%s</textarea><br>"
	"<input name=Send value=Send type=submit class='btn'><input name=Cancel value=Cancel type=submit class='btn'></div></form>";


extern char * WebMailTemplate;
extern char * WebMailMsgTemplate;
extern char * jsTemplate;

static char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *longday[] = {"Sunday", "Monday", "Tusday", "Wednesday", "Thusday", "Friday", "Saturday"};

static struct HTTPConnectionInfo * WebSessionList = NULL;	// active WebMail sessions

#ifdef LINBPQ
UCHAR * GetBPQDirectory();
#endif

void UndoTransparency(char * input);

#ifndef LINBPQ

void UndoTransparency(char * input)
{
	char * ptr1, * ptr2;
	char c;
	int hex;

	if (input == NULL)
		return;

	ptr1 = ptr2 = input;

	// Convert any %xx constructs

	while (1)
	{
		c = *(ptr1++);

		if (c == 0)
			break;

		if (c == '%')
		{
			c = *(ptr1++);
			if(isdigit(c))
				hex = (c - '0') << 4;
			else
				hex = (tolower(c) - 'a' + 10) << 4;

			c = *(ptr1++);
			if(isdigit(c))
				hex += (c - '0');
			else
				hex += (tolower(c) - 'a' + 10);

			*(ptr2++) = hex;
		}
		else if (c == '+')
			*(ptr2++) = 32;
		else
			*(ptr2++) = c;
	}
	*ptr2 = 0;
}
#endif

void ReleaseWebMailStruct(WebMailInfo * WebMail)
{
	// release any malloc'ed resources

	if (WebMail == NULL)
		return;

	FreeWebMailFields(WebMail);
	free(WebMail);
	return;
}

VOID FreeWebMailMallocs()
{
	// called when closing. Not really needed, but simplifies tracking down real memory leaks

	struct HTTPConnectionInfo * Session, * SaveNext;
	int i;
	Session = WebSessionList;

	while (Session)
	{
		SaveNext = Session->Next;
			
		// Release amy malloc'ed resouces
			
		ReleaseWebMailStruct(Session->WebMail);
		free(Session);
		Session = SaveNext;
	}

	for (i = 0; i < FormDirCount; i++)
	{
		struct HtmlFormDir * Dir = HtmlFormDirs[i];

		int j;

		for (j = 0; j < Dir->FormCount; j++)
		{
			free(Dir->Forms[j]->FileName);
			free(Dir->Forms[j]);
		}

		if (Dir->DirCount)
		{
			struct HtmlFormDir * SubDir;

			int k, l;

			for (l = 0; l < Dir->DirCount; l++)
			{
				SubDir = Dir->Dirs[l];
		
				for (k = 0; k < Dir->Dirs[l]->FormCount; k++)
				{
					free(SubDir->Forms[k]->FileName);
					free(SubDir->Forms[k]);
				}
				free(SubDir->DirName);
				free(SubDir->Forms);
				free(SubDir->FormSet);

				free(Dir->Dirs[l]);
			}
		}
		free(Dir->DirName);
		free(Dir->Forms);
		free(Dir->FormSet);
		free(Dir);
	}

	free(HtmlFormDirs);
	return;
}

char * initMultipartUnpack(char ** Input)
{
	// Check if Multipart and return Boundary. Update Input to first part

	// look through header for Content-Type line, and if multipart
	// find boundary string.

	char * ptr, * ptr2;
	char Boundary[128];
	BOOL Multipart = FALSE;

	ptr = *Input;

	while(*ptr != 13)
	{
		ptr2 = strchr(ptr, 10);	// Find CR

		while(ptr2[1] == ' ' || ptr2[1] == 9)		// Whitespace - continuation line
			ptr2 = strchr(&ptr2[1], 10);	// Find CR
	
		if (_memicmp(ptr, "Content-Type: ", 14) == 0)
		{
			char Line[256] = "";
			char * ptr3;
			size_t len = ptr2-ptr-14;

			if (len >255)
				return NULL;

			memcpy(Line, &ptr[14], len);

			if (_memicmp(Line, "Multipart/", 10) == 0)
			{
				ptr3 = stristr(Line, "boundary");
				if (ptr3)
				{
					ptr3+=9;

					if ((*ptr3) == '"')
						ptr3++;

					strcpy(Boundary, ptr3);
					ptr3 = strchr(Boundary, '"');
					if (ptr3) *ptr3 = 0;
					ptr3 = strchr(Boundary, 13);			// CR
					if (ptr3) *ptr3 = 0;
					break;
				}
				else
					return NULL;						// Can't do anything without a boundary ??
			}
		}
		ptr = ptr2;
		ptr++;
	}

	// Find First part - there is a boundary before it

	ptr = strstr(ptr2, Boundary);

	// Next should be crlf then part

	ptr = strstr(ptr, "\r\n");

	if (ptr)
		ptr += 2;		// Over CRLF

	*Input = ptr;		// Return first part or NULL	
	return _strdup(Boundary);
}

BOOL unpackPart(char * Boundary, char ** Input, char ** Name, char ** Value, int * ValLen, char * End)
{
	// Format seems to be
/*
		------WebKitFormBoundaryABJaEbBWB5SuAHmq
		Content-Disposition: form-data; name="Subj"

		subj
		------WebKitFormBoundaryABJaEbBWB5SuAHmq
		Content-Disposition: form-data; name="myFile[]"; filename="exiftool.txt"
		Content-Type: text/plain

		c:\exiftool "-filename<filemodifydate" -d "%Y-%m-%d_%H-%M-%S-%%c.%%e" *
		------WebKitFormBoundaryABJaEbBWB5SuAHmq

		ie Header is terminated by \r\n\r\n
		Value is terminated by ------WebKitFormBoundary7XHZ1i7Jc8tOZJbw
*/
	// Find End of part - ie -- Boundary + CRLF or --

	char * ptr, * endptr, * saveptr;
	char * Msgptr;
	size_t BLen = strlen(Boundary);
	size_t Partlen;

	saveptr = Msgptr = ptr = *Input;

	while(ptr < End)				// Just in case we run off end
	{
		if (*ptr == '-' && *(ptr+1) == '-')
		{
			if (memcmp(&ptr[2], Boundary, BLen) == 0)
			{
				// Found Boundary

				Partlen = ptr - Msgptr;

				ptr += (BLen + 2);			// End of Boundary

				if (*ptr == '-')			// Terminating Boundary
					*Input = NULL;
				else
					*Input = ptr + 2;
		
				// Now unpack Part to Name and Value

				ptr = strstr(Msgptr, "name=");
	
				if (ptr)
				{
					size_t vallen;
			
					endptr = strstr(ptr, "\r\n\r\n");		// "/r/n/r/n
					if (endptr == NULL)
						return FALSE;						// Malformed

					vallen = Partlen - (endptr - saveptr) - 6;

					// name is surrounded by ""

					*(--endptr) = 0;		// Termanate name
			
					ptr += 6;				// to start of name string
					*Name = ptr;

					endptr +=5;				// to Value
					endptr[vallen] = 0;		// terminate to simplify string handling

					*Value = endptr;
					*ValLen = (int)vallen;
					return TRUE;
				}
			}
		}
		ptr ++;
	}
	return FALSE;
}

int SaveInputValue(WebMailInfo * WebMail, char * Name, char * Value, int ValLength)
{
	if (strcmp(Name, "Cancel") == 0)
		return FALSE;
	
	if (strcmp(Name, "To") == 0)
		WebMail->To = _strdup(Value);
	else if (strcmp(Name, "CC") == 0)
		WebMail->CC = _strdup(Value);
	else if (strcmp(Name, "Subj") == 0)
		WebMail->Subject = _strdup(Value);
	else if (strcmp(Name, "Type") == 0)
		WebMail->Type = Value[0];
	else if (strcmp(Name, "BID") == 0)
		WebMail->BID = _strdup(Value);
	else if (strcmp(Name, "Msg") == 0)
		WebMail->Body = _strdup(Value);

	else if (_memicmp(Name, "myFile[]", 8) == 0)
	{
		// Get File Name from param string - myFile[]"; filename="exiftool.txt" \r\nContent-Type: text/plain

		char * fn = strstr(Name, "filename=");
		char * endfn;
		if (fn)
		{
			fn += 10;

			endfn = strchr(fn, '"');
			if (endfn)
			{
				*endfn = 0;

				if (strlen(fn))
				{
					WebMail->FileName[WebMail->Files] = _strdup(fn);
					WebMail->FileBody[WebMail->Files] = malloc(ValLength);
					memcpy(WebMail->FileBody[WebMail->Files], Value, ValLength);
					WebMail->FileLen[WebMail->Files++] = ValLength;
				}
			}
		}
	}

	else if (_memicmp(Name, "myFile2[]", 8) == 0)
	{
		// Get File Name from param string - myFile[]"; filename="exiftool.txt" \r\nContent-Type: text/plain

		char * fn = strstr(Name, "filename=");
		char * endfn;
		if (fn)
		{
			fn += 10;

			endfn = strchr(fn, '"');
			if (endfn)
			{
				*endfn = 0;

				if (strlen(fn))
				{
					WebMail->Header = malloc(ValLength + 1);
					memcpy(WebMail->Header, Value, ValLength + 1);
					WebMail->HeaderLen = RemoveLF(WebMail->Header, ValLength);
				}
			}
		}
	}

	else if (_memicmp(Name, "myFile3[]", 8) == 0)
	{
		// Get File Name from param string - myFile[]"; filename="exiftool.txt" \r\nContent-Type: text/plain

		char * fn = strstr(Name, "filename=");
		char * endfn;
		if (fn)
		{
			fn += 10;

			endfn = strchr(fn, '"');
			if (endfn)
			{
				*endfn = 0;

				if (strlen(fn))
				{
					WebMail->Footer = malloc(ValLength + 1);
					memcpy(WebMail->Footer, Value, ValLength + 1);
					WebMail->FooterLen = RemoveLF(WebMail->Footer, ValLength);
				}
			}
		}
	}

	return TRUE;
}


struct HTTPConnectionInfo * AllocateWebMailSession()
{
	int KeyVal;
	struct HTTPConnectionInfo * Session, * SaveNext;
	time_t NOW = time(NULL);

	//	First see if any session records havent been used for a while

	Session = WebSessionList;

	while (Session)
	{
		if (NOW - Session->WebMailLastUsed > 1200)	// 20 Mins
		{
			SaveNext = Session->Next;
			
			// Release amy malloc'ed resouces
			
			ReleaseWebMailStruct(Session->WebMail);

			memset(Session, 0, sizeof(struct HTTPConnectionInfo));
	
			Session->Next = SaveNext;
			goto UseThis;
		}
		Session = Session->Next;
	}
	
	Session = zalloc(sizeof(struct HTTPConnectionInfo));
	
	if (Session == NULL)
		return NULL;

	if (WebSessionList)
		Session->Next = WebSessionList;

	WebSessionList = Session;

UseThis:

	Session->WebMail = zalloc(sizeof(WebMailInfo));

	KeyVal = ((rand() % 100) + 1);

	KeyVal *= (int)time(NULL);

	sprintf(Session->Key, "%c%08X", 'W', KeyVal);

	return Session;
}

struct HTTPConnectionInfo * FindWMSession(char * Key)
{
	struct HTTPConnectionInfo * Session = WebSessionList;

	while (Session)
	{
		if (strcmp(Session->Key, Key) == 0)
		{
			Session->WebMailLastUsed = time(NULL);
			return Session;
		}
		Session = Session->Next;
	}

	return NULL;
}


// Build list of available forms

VOID ProcessFormDir(char * FormSet, char * DirName, struct HtmlFormDir *** xxx, int * DirCount)
{
	struct HtmlFormDir * FormDir;
	struct HtmlFormDir ** FormDirs = *xxx;
	struct HtmlForm * Form;
	char Search[MAX_PATH];
	int count = *DirCount;

#ifdef WIN32
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
#else
	DIR *dir;
	struct dirent *entry;
	char name[256];
#endif

	FormDir = zalloc(sizeof (struct HtmlFormDir));

	FormDir->DirName = _strdup(DirName);
	FormDir->FormSet = _strdup(FormSet);
	FormDirs=realloc(FormDirs, (count + 1) * sizeof(void *));
	FormDirs[count++] = FormDir;
	
	*DirCount = count;
	*xxx = FormDirs;


	// Scan Directory for .txt files

	sprintf(Search, "%s/%s/%s/*", GetBPQDirectory(), FormSet, DirName);

	// Find the first file in the directory.

#ifdef WIN32

	hFind = FindFirstFile(Search, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
		return;

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			char Dir[MAX_PATH];

			if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
                continue;

			// Recurse in subdir

			sprintf(Dir, "%s/%s", DirName, ffd.cFileName);

			ProcessFormDir(FormSet, Dir, &FormDir->Dirs, &FormDir->DirCount);

			continue;

		}
	
		// Add to list

		Form = zalloc(sizeof (struct HtmlForm));

		Form->FileName = _strdup(ffd.cFileName);

		FormDir->Forms=realloc(FormDir->Forms, (FormDir->FormCount + 1) * sizeof(void *));
		FormDir->Forms[FormDir->FormCount++] = Form;       
	}

	while (FindNextFile(hFind, &ffd) != 0);
 
	FindClose(hFind);

#else

	sprintf(Search, "%s/%s/%s", GetBPQDirectory(), FormSet, DirName);

	if (!(dir = opendir(Search)))
	{
		Debugprintf("%s %d %d", "cant open forms dir", errno, dir);
        return ;
	}
    while ((entry = readdir(dir)) != NULL)
	{
        if (entry->d_type == DT_DIR)
		{
			char Dir[MAX_PATH];

			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

			// Recurse in subdir

			sprintf(Dir, "%s/%s", DirName, entry->d_name);

			ProcessFormDir(FormSet, Dir, &FormDir->Dirs, &FormDir->DirCount);
			continue;
		}

		// Add to list

		Form = zalloc(sizeof (struct HtmlForm));

		Form->FileName = _strdup(entry->d_name);

		FormDir->Forms=realloc(FormDir->Forms, (FormDir->FormCount + 1) * sizeof(void *));
		FormDir->Forms[FormDir->FormCount++] = Form;
    }
    closedir(dir);
#endif
	return;
}

int GetHTMLForms()
{
	int n = 0;

	while (FormDirList[n][0])
		GetHTMLFormSet(FormDirList[n++]);

	return 0;
}

int GetHTMLFormSet(char * FormSet)
{
	int i;

#ifdef WIN32

	WIN32_FIND_DATA ffd;
	char szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

   	sprintf(szDir, "%s/%s/*", BPQDirectory, FormSet);

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
		// Accept either 
		return 0;
	}

	// Scan all directories looking for file
	
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
                continue;

			// Add to Directory List

			ProcessFormDir(FormSet, ffd.cFileName, &HtmlFormDirs, &FormDirCount);
		}
	}
	
	while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);

#else

	DIR *dir;
	struct dirent *entry;
	char name[256];
	
   	sprintf(name, "%s/%s", BPQDirectory, FormSet);

	if (!(dir = opendir(name)))
	{
		Debugprintf("cant open forms dir %s %d %d", name, errno, dir);
	}
	else
	{
		while ((entry = readdir(dir)) != NULL)
		{
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;

				// Add to Directory List

				ProcessFormDir(FormSet, entry->d_name, &HtmlFormDirs, &FormDirCount);
			}
		}
		closedir(dir);
	}
#endif

	// List for testing

	return 0;

	Debugprintf("%d form dirs", FormDirCount);

	for (i = 0; i < FormDirCount; i++)
	{
		struct HtmlFormDir * Dir = HtmlFormDirs[i];

		int j;
		Debugprintf("%3d %s", Dir->FormCount, Dir->DirName);

		for (j = 0; j < Dir->FormCount; j++)
			Debugprintf("   %s", Dir->Forms[j]->FileName);

		if (Dir->DirCount)
		{
			int k, l;

			for (l = 0; l < Dir->DirCount; l++)
			{
				Debugprintf("Subdir %3d %s", Dir->Dirs[l]->DirCount, Dir->Dirs[l]->DirName);
				for (k = 0; k < Dir->Dirs[l]->FormCount; k++)
					Debugprintf("       %s", Dir->Dirs[l]->Forms[k]->FileName);
			}
		}
	}


	return 0;
}


static int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately call is at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}


int SendWebMailHeader(char * Reply, char * Key, struct HTTPConnectionInfo * Session)
{
	return SendWebMailHeaderEx(Reply, Key, Session, NULL);
}


int SendWebMailHeaderEx(char * Reply, char * Key, struct HTTPConnectionInfo * Session, char * Alert)
{
 	// Ex includes an alert string to be sent before message
	
	struct UserInfo * User = Session->User;
	char Messages[245000];
	int m;
	struct MsgInfo * Msg;
	char * ptr = Messages;
	int n = NumberofMessages; //LineCount;
	char Via[64];
	int Count = 0;

	Messages[0] = 0;

	if (Alert && Alert[0])
		ptr += sprintf(Messages, "<script>alert(\"%s\");window.location.href = '/Webmail/WebMail?%s';</script>", Alert, Key); 

	ptr += sprintf(ptr, "%s", "     #  Date  XX   Len To      @       From    Subject\r\n\r\n");

	for (m = LatestMsg; m >= 1; m--)
	{
		if (ptr > &Messages[244000])
			break;						// protect buffer

		Msg = GetMsgFromNumber(m);

		if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
			continue;					// Protect against corrupt messages
		
		if (Msg && CheckUserMsg(Msg, User->Call, User->flags & F_SYSOP))
		{
			char UTF8Title[4096];
			char  * EncodedTitle;
			
			// List if it is the right type and in the page range we want

			if (Session->WebMailTypes[0] && strchr(Session->WebMailTypes, Msg->type) == 0) 
				continue;

			// All Types or right Type. Check Mine Flag

			if (Session->WebMailMine)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->to) != 0 && strcmp(User->Call, Msg->from) != 0)
					continue;
			}

			if (Session->WebMailMyTX)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->from) != 0)
					continue;
			}

			if (Session->WebMailMyRX)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->to)!= 0)
					continue;
			}

			if (Count++ < Session->WebMailSkip)
				continue;

			strcpy(Via, Msg->via);
			strlop(Via, '.');

			// make sure title is HTML safe (no < > etc) and UTF 8 encoded

			EncodedTitle = doXMLTransparency(Msg->title);

			memset(UTF8Title, 0, 4096);		// In case convert fails part way through
			ConvertTitletoUTF8(Session->WebMail, EncodedTitle, UTF8Title, 4095);

			free(EncodedTitle);
			
			ptr += sprintf(ptr, "<a href=/WebMail/WM?%s&%d>%6d</a> %s %c%c %5d %-8s%-8s%-8s%s\r\n",
				Key, Msg->number, Msg->number,
				FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type,
				Msg->status, Msg->length, Msg->to, Via,
				Msg->from, UTF8Title);

			n--;

			if (n == 0)
				break;
		}
	}

	if (WebMailTemplate == NULL)
		WebMailTemplate = GetTemplateFromFile(6, "WebMailPage.txt");

	return sprintf(Reply, WebMailTemplate, BBSName, User->Call, Key, Key, Key, Key, Key, Key, Key, Key, Key, Key, Messages);
}

int ViewWebMailMessage(struct HTTPConnectionInfo * Session, char * Reply, int Number, BOOL DisplayHTML)
{
	char * Key = Session->Key;
	struct UserInfo * User = Session->User;
	WebMailInfo * WebMail = Session->WebMail;
	char * DisplayStyle;

	char Message[200000] = "";
	struct MsgInfo * Msg;
	char * ptr = Message;
	char * MsgBytes, * Save;
	int msgLen;

	char FullTo[100];
	char UTF8Title[4096];
	int Index;
	char * crcrptr;
	char DownLoad[256] = "";

	DisplayStyle = "textarea";			// Prevents interpretation of html and xml

	Msg = GetMsgFromNumber(Number);

	if (Msg == NULL)
	{
		ptr += sprintf(ptr, "Message %d not found\r\n", Number);
		return sprintf(Reply, WebMailTemplate, BBSName, User->Call, Key, Key, Key, Key, Key, Key, Key, Message);
	}

	// New Display so free any old values

	FreeWebMailFields(WebMail);	

	WebMail->CurrentMessageIndex = Number;


	if (!CheckUserMsg(Msg, User->Call, User->flags & F_SYSOP))
	{
		ptr += sprintf(ptr, "Message %d not for you\r", Number);
		return sprintf(Reply, WebMailTemplate, BBSName, User->Call, Key, Key, Key, Key, Key, Key, Key, Message);
	}

	if (_stricmp(Msg->to, "RMS") == 0)
		sprintf(FullTo, "RMS:%s", Msg->via);
	else
		if (Msg->to[0] == 0)
			sprintf(FullTo, "smtp:%s", Msg->via);
		else
			strcpy(FullTo, Msg->to);

	// make sure title is UTF 8 encoded

	memset(UTF8Title, 0, 4096);		// In case convert fails part way through
	ConvertTitletoUTF8(Session->WebMail, Msg->title, UTF8Title, 4095);

	// if a B2 message diplay B2 Header instead of a locally generated one

	if ((Msg->B2Flags & B2Msg) == 0)
	{
		ptr += sprintf(ptr, "From: %s%s\nTo: %s\nType/Status: %c%c\nDate/Time: %s\nBid: %s\nTitle: %s\n\n",
			Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime((time_t)Msg->datecreated, FALSE), Msg->bid, UTF8Title);
	}

	MsgBytes = Save = ReadMessageFile(Number);

	msgLen = Msg->length;

	if (Msg->type == 'P')
		Index = PMSG;
	else if (Msg->type == 'B')
		Index = BMSG;
	else 
		Index = TMSG;

	if (MsgBytes)
	{
		if (Msg->B2Flags & B2Msg)
		{
			char * ptr1;

			// if message has attachments, display them if plain text

			if (Msg->B2Flags & Attachments)
			{
				int BodyLen, NewLen;
				int i;
				char *ptr2, *attptr;	

				sprintf(DownLoad, "<td><a href=/WebMail/DL?%s&%d>Save Attachments</a></td>", Key, Msg->number);

				WebMail->Files = 0;

				ptr1 = MsgBytes;

				//				ptr += sprintf(ptr, "Message has Attachments\r\n\r\n");

				while(*ptr1 != 13)
				{
					ptr2 = strchr(ptr1, 10);	// Find CR

					if (memcmp(ptr1, "Body: ", 6) == 0)
					{
						BodyLen = atoi(&ptr1[6]);
					}

					if (memcmp(ptr1, "File: ", 6) == 0)
					{
						char * ptr3 = strchr(&ptr1[6], ' ');	// Find Space
						*(ptr2 - 1) = 0;

						WebMail->FileLen[WebMail->Files] = atoi(&ptr1[6]);
						WebMail->FileName[WebMail->Files++] = _strdup(&ptr3[1]);
						*(ptr2 - 1) = ' ';		// put space back
					}

					ptr1 = ptr2;
					ptr1++;
				}

				ptr1 += 2;			// Over Blank Line and Separator

				// ptr1 is pointing to body. Save for possible reply

				WebMail->Body = malloc(BodyLen + 2);
				memcpy(WebMail->Body, ptr1, BodyLen);
				WebMail->Body[BodyLen] = 0;

				*(ptr1 + BodyLen) = 0;

				ptr += sprintf(ptr, "%s", MsgBytes);		// B2 Header and Body

				ptr1 += BodyLen + 2;		// to first file

				// Save pointers to file

				attptr = ptr1; 

				for (i = 0; i < WebMail->Files; i++)
				{
					WebMail->FileBody[i] = malloc(WebMail->FileLen[i]);
					memcpy(WebMail->FileBody[i], attptr, WebMail->FileLen[i]);
					attptr += (WebMail->FileLen[i] + 2);
				}

				// if first (only??) attachment is XML and  filename
				// starts "RMS_Express_Form" process as HTML Form

				if (DisplayHTML && _memicmp(ptr1, "<?xml", 4) == 0 && 
					_memicmp(WebMail->FileName[0], "RMS_Express_Form_", 16) == 0)
				{
					int Len = DisplayWebForm(Session, Msg, WebMail->FileName[0], ptr1, Reply, MsgBytes, BodyLen + 32); // 32 for added "has attachments"
					free(MsgBytes);

					// Flag as read

					if ((_stricmp(Msg->to, User->Call) == 0) || ((User->flags & F_SYSOP) && (_stricmp(Msg->to, "SYSOP") == 0)))
					{
						if ((Msg->status != 'K') && (Msg->status != 'H') && (Msg->status != 'F') && (Msg->status != 'D'))
						{
							if (Msg->status != 'Y')
							{
								Msg->status = 'Y';
								Msg->datechanged=time(NULL);
								SaveMessageDatabase();
								SendMessageReadEvent(Session->Callsign, Msg);
							}
						}
					}

					return Len;
				}

				for (i = 0; i < WebMail->Files; i++)
				{
					int n;
					char * p = ptr1;
					char c;

					// Check if message is probably binary

					int BinCount = 0;

					NewLen = WebMail->FileLen[i];

					for (n = 0; n < NewLen; n++)
					{
						c = *p;

						if (c==0 || (c & 128))
							BinCount++;

						p++;

					}

					if (BinCount > NewLen/10)
					{
						// File is probably Binary

						ptr += sprintf(ptr, "\rAttachment %s is a binary file\r", WebMail->FileName[i]);
					}
					else
					{
						*(ptr1 + NewLen) = 0;
						ptr += sprintf(ptr, "\rAttachment %s\r\r", WebMail->FileName[i]);
						RemoveLF(ptr1, NewLen + 1);		// Removes LF after CR but not on its own

						ptr += sprintf(ptr, "%s\r\r", ptr1);

						User->Total.MsgsSent[Index] ++;
						User->Total.BytesForwardedOut[Index] += NewLen;
					}

					ptr1 += WebMail->FileLen[i];
					ptr1 +=2;				// Over separator
				}

				free(Save);

				ptr += sprintf(ptr, "\r\r[End of Message #%d from %s]\r", Number, Msg->from);

				RemoveLF(Message, (int)strlen(Message) + 1);		// Removes LF after CR but not on its own

				if ((_stricmp(Msg->to, User->Call) == 0) || ((User->flags & F_SYSOP) && (_stricmp(Msg->to, "SYSOP") == 0)))
				{
					if ((Msg->status != 'K') && (Msg->status != 'H') && (Msg->status != 'F') && (Msg->status != 'D'))
					{
						if (Msg->status != 'Y')
						{
							Msg->status = 'Y';
							Msg->datechanged=time(NULL);
							SaveMessageDatabase();
							SendMessageReadEvent(Session->Callsign, Msg);
						}
					}
				}

				if (DisplayHTML && stristr(Message, "</html>"))
					DisplayStyle = "div";				// Use div so HTML and XML are interpreted

				return sprintf(Reply, WebMailMsgTemplate, BBSName, User->Call, Msg->number, Msg->number, Key, Msg->number, Key, DownLoad, Key, Key, Key, DisplayStyle, Message, DisplayStyle);
			}

			// Remove B2 Headers (up to the File: Line)

			//		ptr1 = strstr(MsgBytes, "Body:");

			//		if (ptr1)
			//			MsgBytes = ptr1;
		}

		// Body  may have cr cr lf which causes double space

		crcrptr = strstr(MsgBytes, "\r\r\n");

		while (crcrptr)
		{
			*crcrptr = ' ';
			crcrptr = strstr(crcrptr, "\r\r\n");
		}

		// Remove lf chars

		msgLen = RemoveLF(MsgBytes, msgLen);

		User->Total.MsgsSent[Index] ++;
		//		User->Total.BytesForwardedOut[Index] += Length;

		// if body not UTF-8, convert it

		if (WebIsUTF8(MsgBytes, msgLen) == FALSE)
		{
			int code = TrytoGuessCode(MsgBytes, msgLen);

			UCHAR * UTF = malloc(msgLen * 3);

			if (code == 437)
				msgLen = Convert437toUTF8(MsgBytes, msgLen, UTF);
			else if (code == 1251)
				msgLen = Convert1251toUTF8(MsgBytes, msgLen, UTF);
			else
				msgLen = Convert1252toUTF8(MsgBytes, msgLen, UTF);
			
			free(MsgBytes);
			Save = MsgBytes = UTF;
	
			MsgBytes[msgLen] = 0;
		}

		//		ptr += sprintf(ptr, "%s", MsgBytes);

		memcpy(ptr, MsgBytes, msgLen);
		ptr += msgLen;
		ptr[0] = 0;

		free(Save);

		ptr += sprintf(ptr, "\r\r[End of Message #%d from %s]\r", Number, Msg->from);

		if ((_stricmp(Msg->to, User->Call) == 0) || ((User->flags & F_SYSOP) && (_stricmp(Msg->to, "SYSOP") == 0)))
		{
			if ((Msg->status != 'K') && (Msg->status != 'H') && (Msg->status != 'F') && (Msg->status != 'D'))
			{
				if (Msg->status != 'Y')
				{
					Msg->status = 'Y';
					Msg->datechanged=time(NULL);
					SaveMessageDatabase();
					SendMessageReadEvent(Session->Callsign, Msg);
				}
			}
		}
	}
	else
	{
		ptr += sprintf(ptr, "File for Message %d not found\r", Number);
	}

	if (DisplayHTML && stristr(Message, "</html>"))
		DisplayStyle = "div";				// Use div so HTML and XML are interpreted


	return sprintf(Reply, WebMailMsgTemplate, BBSName, User->Call, Msg->number, Msg->number, Key, Msg->number, Key, DownLoad, Key, Key, Key, DisplayStyle, Message, DisplayStyle);
}

int KillWebMailMessage(char * Reply, char * Key, struct UserInfo * User, int Number)
{
	struct MsgInfo * Msg;
	char Message[100] = "";

	Msg = GetMsgFromNumber(Number);

	if (Msg == NULL)
	{
		sprintf(Message, "Message %d not found", Number);
		goto returnit;
	}

	if (OkToKillMessage(User->flags & F_SYSOP, User->Call, Msg))
	{
		FlagAsKilled(Msg, TRUE);
		sprintf(Message, "Message #%d Killed\r", Number);
		goto returnit;
	}

	sprintf(Message, "Not your message\r");

returnit:
	return sprintf(Reply, WebMailMsgTemplate, BBSName, User->Call, Msg->number, Msg->number, Key, Msg->number, Key, "", Key, Key, Key, "div", Message, "div");
}

void freeKeys(KeyValues * Keys)
{
	while (Keys->Key)
	{
		free(Keys->Key);
		free(Keys->Value);
		Keys++;
	}
}

void FreeWebMailFields(WebMailInfo * WebMail)
{
	// release any malloc'ed resources

	int i;
	char * SaveReply;
	int * SaveRlen;

	if (WebMail == NULL)
		return;

	if (WebMail->txtFile)
		free(WebMail->txtFile);

	if (WebMail->txtFileName)
		free(WebMail->txtFileName);

	if (WebMail->InputHTMLName)
		free(WebMail->InputHTMLName);
	
	if (WebMail->DisplayHTMLName)
		free(WebMail->DisplayHTMLName);

	if (WebMail->ReplyHTMLName)
		free(WebMail->ReplyHTMLName);

	if (WebMail->To)
		free(WebMail->To);
	if (WebMail->CC)
		free(WebMail->CC);
	if (WebMail->Subject)
		free(WebMail->Subject);
	if (WebMail->BID)
		free(WebMail->BID);
	if (WebMail->Body)
		free(WebMail->Body);
	if (WebMail->XML)
		free(WebMail->XML);
	if (WebMail->XMLName)
		free(WebMail->XMLName);

	if (WebMail->OrigTo)
		free(WebMail->OrigTo);
	if (WebMail->OrigSubject)
		free(WebMail->OrigSubject);
	if (WebMail->OrigBID)
		free(WebMail->OrigBID);
	if (WebMail->OrigBody)
		free(WebMail->OrigBody);

	freeKeys(WebMail->txtKeys);
	freeKeys(WebMail->XMLKeys);

	for (i = 0; i < WebMail->Files; i++)
	{
		free(WebMail->FileBody[i]);
		free(WebMail->FileName[i]);
	}

	if (WebMail->Header)
		free(WebMail->Header);
	if (WebMail->Footer)
		free(WebMail->Footer);

	SaveReply = WebMail->Reply;
	SaveRlen = WebMail->RLen;

#ifndef WIN32
	if (WebMail->iconv_toUTF8)
		iconv_close(WebMail->iconv_toUTF8);
#endif

	memset(WebMail, 0, sizeof(WebMailInfo));

	WebMail->Reply = SaveReply;
	WebMail->RLen = SaveRlen;

	return;
}


void ProcessWebMailMessage(struct HTTPConnectionInfo * Session, char * Key, BOOL LOCAL, char * Method, char * NodeURL, char * input, char * Reply, int * RLen, int InputLen)
{
	char * URLParams = strlop(Key, '&');
	int ReplyLen;
	char Appl = 'M';

	// Webmail doesn't use the normal Mail Key.

	// webscript.js doesn't need a key

	if (_stricmp(NodeURL, "/WebMail/webscript.js") == 0)
	{
		if (jsTemplate)
			free(jsTemplate);
		
		jsTemplate = GetTemplateFromFile(2, "webscript.js");

		ReplyLen = sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
			"Cache-Control: max-age=60\r\nContent-Type: text/javascript\r\n\r\n%s", (int)strlen(jsTemplate), jsTemplate);
		*RLen = ReplyLen;
		return;
	}

	// Neither do js or file downloads

	// This could be a request for a Template file
	// WebMail/Local_Templates/My Forms/inc/logo_ad63.png
	// WebMail/Standard Templates/


	if (_memicmp(NodeURL, "/WebMail/Local", 14) == 0 || (_memicmp(NodeURL, "/WebMail/Standard", 17) == 0))
	{
		int FileSize;
		char * MsgBytes;
		char MsgFile[512];
		FILE * hFile;
		size_t ReadLen;
		char TimeString[64];
		char FileTimeString[64];
		struct stat STAT;
 		char * FN = &NodeURL[9];
		char * fileBit = FN;
		char * ext;
		char Type[64] = "Content-Type: text/html\r\n";

		UndoTransparency(FN);
		ext = strchr(FN, '.');

		sprintf(MsgFile, "%s/%s", BPQDirectory, FN);

		while (strchr(fileBit, '/'))
			fileBit = strlop(fileBit, '/');

		if (stat(MsgFile, &STAT) == -1)
		{
			*RLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			return;
		}

		hFile = fopen(MsgFile, "rb");
	
		if (hFile == 0)
		{
			*RLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			return;
		}

		FileSize = STAT.st_size;
		MsgBytes = malloc(FileSize + 1);
		ReadLen = fread(MsgBytes, 1, FileSize, hFile); 

		fclose(hFile);

		FormatTime2(FileTimeString, STAT.st_ctime);
		FormatTime2(TimeString, time(NULL));

		ext++;
		
		if (_stricmp(ext, "js") == 0)
			strcpy(Type, "Content-Type: text/javascript\r\n");
	
		if (_stricmp(ext, "css") == 0)
			strcpy(Type, "Content-Type: text/css\r\n");

		if (_stricmp(ext, "pdf") == 0)
			strcpy(Type, "Content-Type: application/pdf\r\n");

		if (_stricmp(ext, "jpg") == 0 || _stricmp(ext, "jpeg") == 0 || _stricmp(ext, "png") == 0 ||
			_stricmp(ext, "gif") == 0 || _stricmp(ext, "bmp") == 0 || _stricmp(ext, "ico") == 0)
			strcpy(Type, "Content-Type: image\r\n");

		// File may be binary so output header then copy in message

		*RLen = sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
				"%s"
				"Date: %s\r\n"
				"Last-Modified: %s\r\n" 
				"\r\n", FileSize, Type,TimeString, FileTimeString);

		memcpy(&Reply[*RLen], MsgBytes, FileSize);
		*RLen += FileSize;
		free (MsgBytes);
		return;
	}	

	// 

	if (_memicmp(NodeURL, "/WebMail/WMFile/", 16) == 0)
	{
		int FileSize;
		char * MsgBytes;
		char MsgFile[512];
		FILE * hFile;
		size_t ReadLen;
		char TimeString[64];
		char FileTimeString[64];
		struct stat STAT;
		char * FN = &NodeURL[16];
		char * fileBit = FN;
		char * ext;
		char Type[64] = "Content-Type: text/html\r\n";


		UndoTransparency(FN);
		ext = strchr(FN, '.');

		sprintf(MsgFile, "%s/%s", BPQDirectory, FN);

		while (strchr(fileBit, '/'))
			fileBit = strlop(fileBit, '/');

		if (stat(MsgFile, &STAT) == -1)
		{
			*RLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			return;
		}

		hFile = fopen(MsgFile, "rb");
	
		if (hFile == 0)
		{
			*RLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			return;
		}

		FileSize = STAT.st_size;
		MsgBytes = malloc(FileSize + 1);
		ReadLen = fread(MsgBytes, 1, FileSize, hFile); 

		fclose(hFile);

		FormatTime2(FileTimeString, STAT.st_ctime);
		FormatTime2(TimeString, time(NULL));

		ext++;
		
		if (_stricmp(ext, "js") == 0)
			strcpy(Type, "Content-Type: text/javascript\r\n");
	
		if (_stricmp(ext, "css") == 0)
			strcpy(Type, "Content-Type: text/css\r\n");

		if (_stricmp(ext, "pdf") == 0)
			strcpy(Type, "Content-Type: application/pdf\r\n");

		if (_stricmp(ext, "jpg") == 0 || _stricmp(ext, "jpeg") == 0 || _stricmp(ext, "png") == 0 ||
			_stricmp(ext, "gif") == 0 || _stricmp(ext, "bmp") == 0 || _stricmp(ext, "ico") == 0)
			strcpy(Type, "Content-Type: image\r\n");

		// File may be binary so output header then copy in message

		*RLen = sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
				"%s"
				"Date: %s\r\n"
				"Last-Modified: %s\r\n" 
				"\r\n", FileSize, Type,TimeString, FileTimeString);

		memcpy(&Reply[*RLen], MsgBytes, FileSize);
		*RLen += FileSize;
		free (MsgBytes);
		return;
	}	

	Session = NULL;

	if (Key && Key[0])
		Session = FindWMSession(Key);

	if (Session == NULL)
	{
		//	Lost Session
			
		if (LOCAL)
		{
			Session = AllocateWebMailSession();

			Key = Session->Key;

			if (SYSOPCall[0])
				Session->User = LookupCall(SYSOPCall);
			else
				Session->User = LookupCall(BBSName);
	
			if (Session->User)
			{
				strcpy(NodeURL, "/WebMail/WebMail");
				Session->WebMailSkip = 0;
				Session->WebMailLastUsed = time(NULL);
			}
		}
		else
		{
			//	Send Login Page unless Signon request

			if (_stricmp(NodeURL, "/WebMail/Signon") != 0 || strcmp(Method, "POST") != 0)
			{
				ReplyLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
				*RLen = ReplyLen;
				return;
			}
		}
	}

	if (strcmp(Method, "POST") == 0)
	{	
		if (_stricmp(NodeURL, "/WebMail/Signon") == 0)
		{
			char * msg = strstr(input, "\r\n\r\n");	// End of headers
			char * user, * password, * Key;
			char Msg[128];
			int n;

			if (msg)
			{
				struct UserInfo * User;

				if (strstr(msg, "Cancel=Cancel"))
				{
					*RLen = sprintf(Reply, "<html><script>window.location.href = '/';</script>");
					return;
				}
				// Webmail Gets Here with a dummy Session

				Session = AllocateWebMailSession();
				Session->WebMail->Reply = Reply;
				Session->WebMail->RLen = RLen;


				Key = Session->Key;
		
				user = strtok_s(&msg[9], "&", &Key);
				password = strtok_s(NULL, "=", &Key);
				password = Key;

				Session->User = User = LookupCall(user);

				if (User)
				{
					// Check Password

					if (password[0] && strcmp(User->pass, password) == 0)
					{
						// send Message Index

						Session->WebMailLastUsed = time(NULL);
						Session->WebMailSkip = 0;
						Session->WebMailMyTX = FALSE;
						Session->WebMailMyRX = FALSE;
						Session->WebMailMine = FALSE;

						if (WebMailTemplate)
						{
							free(WebMailTemplate);
							WebMailTemplate = NULL;
						}

						if (User->flags & F_Excluded)
						{
							n = sprintf_s(Msg, sizeof(Msg), "Webmail Connect from %s Rejected by Exclude Flag", _strupr(user));
							WriteLogLine(NULL, '|',Msg, n, LOG_BBS);
							ReplyLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
							*RLen = ReplyLen;
							return;
						}			

						*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 						n=sprintf_s(Msg, sizeof(Msg), "Webmail Connect from %s", _strupr(user));
						WriteLogLine(NULL, '|',Msg, n, LOG_BBS);

						return;
					}
					
				}

				//	Bad User or Pass

				ReplyLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
				*RLen = ReplyLen;
				return;
			}
		}

		Session->WebMail->Reply = Reply;
		Session->WebMail->RLen = RLen;

		if (_stricmp(NodeURL, "/WebMail/EMSave") == 0)
		{
			//	Save New Message

			SaveNewMessage(Session, input, Reply, RLen, Key, InputLen);
			return;
		}

		if (_stricmp(NodeURL, "/WebMail/Submit") == 0)
		{
			// Get the POST data from the page and place in message
			
			char * 	param = strstr(input, "\r\n\r\n");	// End of headers
			WebMailInfo * WebMail = Session->WebMail;

			if (WebMail == NULL)
				return;				// Can't proceed if we have no info on form

			ProcessFormInput(Session, input, Reply, RLen, InputLen);
			return;
		}

		if (_stricmp(NodeURL, "/WebMail/FormMsgSave") == 0)
		{
			//	Save New Message

			SaveTemplateMessage(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/WebMail/GetTemplates") == 0)
		{
			SendTemplateSelectScreen(Session, input, InputLen);
			return;
		}
	
		// End of POST section
	}

	if (_stricmp(NodeURL, "/WebMail/WMLogout") == 0)
	{
		Session->Key[0] = 0;
		Session->WebMailLastUsed = 0;
		ReplyLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
		*RLen = ReplyLen;
		return;
	}

	if ((_stricmp(NodeURL, "/WebMail/MailEntry") == 0) ||
		(_stricmp(NodeURL, "/WebMail") == 0) ||
		(_stricmp(NodeURL, "/WebMail/") == 0))
	{
		// Entry from Menu if signed in, continue. If not and Localhost 
		// signin as sysop.

		if (Session->User == NULL)
		{
			// Not yet signed in

			if (LOCAL)
			{
				// Webmail Gets Here with a dummy Session

				Session = AllocateWebMailSession();
				Session->WebMail->Reply = Reply;
				Session->WebMail->RLen = RLen;

				Key = Session->Key;

				if (SYSOPCall[0])
					Session->User = LookupCall(SYSOPCall);
				else
					Session->User = LookupCall(BBSName);

				if (Session->User)
				{
					strcpy(NodeURL, "/WebMail/WebMail");
					Session->WebMailSkip = 0;
					Session->WebMailLastUsed = time(NULL);
				}
			}
			else
			{
				//	Send Login Page

				ReplyLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
				*RLen = ReplyLen;
				return;
			}
		}
	}

	Session->WebMail->Reply = Reply;
	Session->WebMail->RLen = RLen;

	if (_stricmp(NodeURL, "/WebMail/WebMail") == 0)
	{
		if (WebMailTemplate)
		{
			free(WebMailTemplate);
			WebMailTemplate = NULL;
		}

		Session->WebMailSkip = 0;
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;

		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMAll") == 0)
	{
		Session->WebMailSkip = 0;
		Session->WebMailTypes[0] = 0;
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;

		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMB") == 0)
	{
		Session->WebMailSkip = 0;
		strcpy(Session->WebMailTypes, "B");
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;

		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}
		
	if (_stricmp(NodeURL, "/WebMail/WMP") == 0)
	{
		Session->WebMailSkip = 0;
		strcpy(Session->WebMailTypes, "P");
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;

		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMT") == 0)
	{
		Session->WebMailSkip = 0;
		strcpy(Session->WebMailTypes, "T");
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;

		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMMine") == 0)
	{
		Session->WebMailSkip = 0;
		Session->WebMailTypes[0] = 0;
		Session->WebMailMine = TRUE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = FALSE;
		
		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMtoMe") == 0)
	{
		Session->WebMailSkip = 0;
		Session->WebMailTypes[0] = 0;
		Session->WebMailMine = FALSE;
		Session->WebMailMyTX = FALSE;
		Session->WebMailMyRX = TRUE;
		
		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMfromMe") == 0)
	{
		Session->WebMailSkip = 0;
		Session->WebMailTypes[0] = 0;
		Session->WebMailMine = TRUE;
		Session->WebMailMyTX = TRUE;
		Session->WebMailMyRX = FALSE;
		
		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}


	if (_stricmp(NodeURL, "/WebMail/WMSame") == 0)
	{
		*RLen = SendWebMailHeader(Reply, Session->Key, Session);
 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/WMAuto") == 0)
	{
		// Auto Refresh Version of index page. Uses Web Sockets

		char Page[4096];

		char WebSockPage[] =
			"<!-- Version 6 8/11/2018 -->\r\n"
			"<!DOCTYPE html> \r\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\"> \r\n"
			"<head> \r\n"
			"<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/> \r\n"
			"<style type=\"text/css\">\r\n"
			"pre {margin-left: 4px;white-space: pre} \r\n"
			"#main{width:700px;position:absolute;left:0px;border:2px solid;background-color: #ffffff;}\r\n"
			"</style>\r\n"
			"<script src=\"/WebMail/webscript.js\"></script>\r\n"

			"<script type = \"text/javascript\">\r\n"
			"var ws;"
			"function Init()"
			"{"
			" if (\"WebSocket\" in window)"
			" {"
			"   // open a web socket. Get address from URL\r\n"
			"	var text = window.location.href;"
			"	var result = text.substring(7);"
			"	var myArray = result.split('/', 1);"
			"   ws = new WebSocket('ws://' + myArray[0] + '/WMRefresh&%s');\r\n"

			"   ws.onopen = function() {\r\n"

			"   // Web Socket is connected\r\n"

			"	const div = document.getElementById('main');\r\n"
			"	div.innerHTML = 'Websock Connected'\r\n"
			"   ws.send('WMRefresh&%s');"
			"    };\r\n"

			"   ws.onmessage = function (evt)"
			"   {"
			"     var received_msg = evt.data;\r\n"
			"	  const div = document.getElementById('main');\r\n"
			"	  div.innerHTML = received_msg\r\n"
			"     };"

			"   ws.onclose = function()"
			"   {"

			"    // websocket is closed.\r\n"
			"	 const div = document.getElementById('main');\r\n"
			"	 div.innerHTML = 'Websock Connection Lost';\r\n"
			"    };"
			"  initialize(120);"
			" }"
			" else"
			" {"
			"  // The browser doesn't support WebSocket\r\n"
			"	const div = document.getElementById('main');\r\n"
			"	div.innerHTML = 'WebSocket not supported by your Browser - AutoRefresh not availble'\r\n"
			" }"
			"}"

			"</script>\r\n"
			"<title>WebMail</title> \r\n"
			"</head>\r\n"

			"<body background=/background.jpg onload=Init() onresize=initialize(120)>\r\n"
			"<h3 align=center> %s Webmail Interface - User %s - Message List</h3>\r\n"
			"<table align=center border=1 cellpadding=2 bgcolor=white><tr>\r\n"
			"\r\n"
			"<td><a href=/WebMail/WMB?%s>Bulls</a></td>\r\n"
			"<td><a href=/WebMail/WMP?%s>Personal</a></td>\r\n"
			"<td><a href=/WebMail/WMT?%s>NTS</a></td>\r\n"
			"<td><a href=/WebMail/WMALL?%s>All Types</a></td>\r\n"
			"<td><a href=/WebMail/WMMine?%s>Mine</a></td>\r\n"
			"<td><a href=/WebMail/WMfromMe?%s>My Sent</a></td>\r\n"
			"<td><a href=/WebMail/WMtoMe?%s>My Rxed</a></td>\r\n"
			"<td><a href=/WebMail/WMAuto?%s>Auto Refresh</a></td>\r\n"
			"<td><a href=\"#\" onclick=\"newmsg('%s'); return false;\">Send Message</a></td>\r\n"
			"<td><a href=/WebMail/WMLogout?%s>Logout</a></td>\r\n"
			"<td><a href=/>Node Menu</a></td></tr></table>\r\n"
			"<br>\r\n"

			"<div align=left id=main style=overflow:scroll;>Waiting for data...</div>\r\n"
			"</body></html>\r\n";

		sprintf(Page, WebSockPage, Key, Key ,BBSName, Session->User->Call, Key, Key, Key, Key, Key, Key, Key, Key, Key, Key);

		*RLen = sprintf(Reply, "%s", Page);
		return;
	}
	

	if (memcmp(NodeURL, "/WebMail/QuoteOriginal/", 15) == 0)
	{
		// Reply to Message

		int n, len;
		struct MsgInfo * Msg;
		char Message[100] = "";
		char Title[100];
		char * MsgBytes, * Save, * NewBytes;
		char * ptr;
		char * ptr1, * ptr2;
		char * EncodedTitle;

		n = Session->WebMail->CurrentMessageIndex;
	
		Msg = GetMsgFromNumber(n);

		if (Msg == NULL)
		{
			sprintf(Message, "Message %d not found", n);
			*RLen = sprintf(Reply, "%s", Message);
			return;
		}

		Session->WebMail->Msg = Msg;

		if (stristr(Msg->title, "Re:") == 0)
			sprintf(Title, "Re:%s", Msg->title);
		else
			sprintf(Title, "%s", Msg->title);

		MsgBytes = Save = ReadMessageFile(n);


		ptr = NewBytes = malloc((Msg->length * 2) + 256);

		// Copy a line at a time with "> " in front of each

		ptr += sprintf(ptr, "%s", "\r\n\r\n\r\n\r\n\r\nOriginal Message\r\n\r\n> ");

		ptr1 = ptr2 = MsgBytes;
		len  = (int)strlen(MsgBytes);

		while (len-- > 0)
		{
			*ptr++ = *ptr1;
	
			if (*(ptr1) == '\n')
			{
				*ptr++ = '>';
				*ptr++ = ' ';
			}
		
			ptr1++;
		}

		*ptr++ = 0;

		EncodedTitle = doXMLTransparency(Msg->title);

		*RLen = sprintf(Reply, MsgInputPage, Key, Msg->from, "", EncodedTitle , NewBytes);

		free(EncodedTitle);

		free(MsgBytes);
		free(NewBytes);

		return;
	}



	if (memcmp(NodeURL, "/WebMail/Reply/", 15) == 0)
	{
		// Reply to Message

		int n = atoi(&NodeURL[15]);
		struct MsgInfo * Msg;
		char Message[100] = "";
		char Title[100];
		char * EncodedTitle;
				
		// Quote Original
		
		char Button[] = 
			" &nbsp; &nbsp; &nbsp;<script>function myfunc(){"
			"document.getElementById('myform').action = '/WebMail/QuoteOriginal' + '?%s';"
			" document.getElementById('myform').submit();}</script>"
			"<input type=button class='btn' onclick='myfunc()' "
			"value='Include Original Msg'>";
		
		char Temp[1024];
		char ReplyAddr[128];

		Msg = GetMsgFromNumber(n);

		if (Msg == NULL)
		{
			sprintf(Message, "Message %d not found", n);
			*RLen = sprintf(Reply, "%s", Message);
			return;
		}

		Session->WebMail->Msg = Msg;

		// See if the message was displayed in an HTML form with a reply template

		*RLen = ReplyToFormsMessage(Session, Msg, Reply, FALSE);

		// If couldn't build reply form use normal text reply

		if (*RLen)
			return;


		sprintf(Temp, Button, Key);

		if (stristr(Msg->title, "Re:") == 0)
			sprintf(Title, "Re:%s", Msg->title);
		else
			sprintf(Title, "%s", Msg->title);

		strcpy(ReplyAddr, Msg->from);
		strcat(ReplyAddr, Msg->emailfrom);

		EncodedTitle = doXMLTransparency(Msg->title);

		*RLen = sprintf(Reply, MsgInputPage, Key, Msg->from, Temp, EncodedTitle , "");

		free(EncodedTitle);
		return;
	}

	if (strcmp(NodeURL, "/WebMail/WM") == 0)
	{
		// Read Message

		int n = 0;
		
		if (URLParams)
			n = atoi(URLParams);

		if (WebMailMsgTemplate)
			free(WebMailMsgTemplate);

		WebMailMsgTemplate = GetTemplateFromFile(5, "WebMailMsg.txt");

		*RLen = ViewWebMailMessage(Session, Reply, n, TRUE);

 		return;
	}

	if (strcmp(NodeURL, "/WebMail/WMPrev") == 0)
	{
		// Read Previous Message

		int m;
		struct MsgInfo * Msg;
		struct UserInfo * User = Session->User;


		for (m = Session->WebMail->CurrentMessageIndex - 1; m >= 1; m--)
		{

			Msg = GetMsgFromNumber(m);
	
			if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
				continue;					// Protect against corrupt messages
		
			if (Msg && CheckUserMsg(Msg, User->Call, User->flags & F_SYSOP))
			{			
				// Display if it is the right type and in the page range we want

				if (Session->WebMailTypes[0] && strchr(Session->WebMailTypes, Msg->type) == 0) 
					continue;

				// All Types or right Type. Check Mine Flag

				if (Session->WebMailMine)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->to) != 0 && strcmp(User->Call, Msg->from) != 0)
						continue;
				}

				if (Session->WebMailMyTX)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->from) != 0)
						continue;
				}

				if (Session->WebMailMyRX)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->to) != 0)
						continue;
				}
				*RLen = ViewWebMailMessage(Session, Reply, m, TRUE);

				return;
			}
		}

		// No More

		*RLen = sprintf(Reply, "<html><script>alert(\"No More Messages\");window.location.href = '/Webmail/WebMail?%s';</script></html></script></html>", Session->Key);
		return;

	}

	if (strcmp(NodeURL, "/WebMail/WMNext") == 0)
	{
		// Read Previous Message

		int m;
		struct MsgInfo * Msg;
		struct UserInfo * User = Session->User;

		for (m = Session->WebMail->CurrentMessageIndex + 1; m <= LatestMsg; m++)
		{
			Msg = GetMsgFromNumber(m);

			if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
				continue;					// Protect against corrupt messages

			if (Msg && CheckUserMsg(Msg, User->Call, User->flags & F_SYSOP))
			{			
				// Display if it is the right type and in the page range we want

				if (Session->WebMailTypes[0] && strchr(Session->WebMailTypes, Msg->type) == 0) 
					continue;

				// All Types or right Type. Check Mine Flag

				if (Session->WebMailMine)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->to) != 0 && strcmp(User->Call, Msg->from) != 0)
						continue;
				}

				if (Session->WebMailMyTX)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->from) != 0)
						continue;
				}

				if (Session->WebMailMyRX)
				{
					// Only list if to or from me

					if (strcmp(User->Call, Msg->to) != 0)
						continue;
				}
				*RLen = ViewWebMailMessage(Session, Reply, m, TRUE);

				return;
			}
		}

		// No More 

		*RLen = sprintf(Reply, "<html><script>alert(\"No More Messages\");window.location.href = '/Webmail/WebMail?%s';</script></html></script></html>", Session->Key);
		return;

	}


	if (strcmp(NodeURL, "/WebMail/DisplayText") == 0)
	{
		// Read Message

		int n = 0;
		
		if (URLParams)
			n = atoi(URLParams);

		if (WebMailMsgTemplate)
			free(WebMailMsgTemplate);

		WebMailMsgTemplate = GetTemplateFromFile(5, "WebMailMsg.txt");

		*RLen = ViewWebMailMessage(Session, Reply, n, FALSE);

 		return;
	}
	if (memcmp(NodeURL, "/WebMail/WMDel/", 15) == 0)
	{
		// Kill  Message

		int n = atoi(&NodeURL[15]);

		*RLen = KillWebMailMessage(Reply, Session->Key, Session->User, n);

 		return;
	}

	if (_stricmp(NodeURL, "/WebMail/NewMsg") == 0)
	{
		// Add HTML Template Button if we have any HTML Form

		char Button[] = 
			" &nbsp; &nbsp; &nbsp;<script>function myfunc(){"
			"document.getElementById('myform').action = '/WebMail/GetTemplates' + '?%s';"
			" document.getElementById('myform').submit();}</script>"
			"<input type=button class='btn' onclick='myfunc()' "
			"value='Use Template'>";
			
		char Temp[1024];

		FreeWebMailFields(Session->WebMail);		// Tidy up for new message

		sprintf(Temp, Button, Key);

		if (FormDirCount == 0)
			*RLen = sprintf(Reply, MsgInputPage, Key, "", "", "", "");
		else
			*RLen = sprintf(Reply, MsgInputPage, Key, "", Temp, "", "");

		return;
	}

	if (_memicmp(NodeURL, "/WebMail/GetPage/", 17) == 0)
	{
		// Read and Parse Template File

		GetPage(Session, NodeURL);
		return;
	}

	if (_memicmp(NodeURL, "/WebMail/GetList/", 17) == 0)
	{
		// Send Select Template Popup

		char * SubDir;
		int DirNo = 0;
		int SubDirNo = 0;
		char popup[10000]; 

		char popuphddr[] = 
			
			"<html><body align=center background='/background.jpg'>"
			"<script>function myFunction() {var x = document.getElementById(\"mySelect\").value;"
			"var Key = \"%s\";"
			"var param = \"toolbar=yes,location=yes,directories=yes,status=yes,menubar,=scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes\";"
			"window.open(\"/WebMail/GetPage/\" + x + \"?\" + Key,\"_self\",param);"
			"}</script>"
			"<p align=center>"
			"Select Required Template from %s<br><br>"
			"<select size=15 id=\"mySelect\" onclick=\"myFunction()\">"
			"<option value=-1>No Page Selected";
			
		struct HtmlFormDir * Dir;
		int i;
		int len;

		SubDir = strlop(&NodeURL[17], ':');
		DirNo = atoi(&NodeURL[17]);

		if (DirNo == -1)
		{
			// User has gone back, then selected "No Folder Selected"

			// For now just stop crash

			DirNo = 0;
		}


		if (SubDir)
			SubDirNo = atoi(SubDir);

		Dir = HtmlFormDirs[DirNo];

		if (SubDir)
			len = sprintf(popup, popuphddr, Key, Dir->Dirs[SubDirNo]->DirName);
		else
			len = sprintf(popup, popuphddr, Key, Dir->DirName);

		if (SubDir)
		{
			for (i = 0; i < Dir->Dirs[SubDirNo]->FormCount; i++)
			{		
				char * Name = Dir->Dirs[SubDirNo]->Forms[i]->FileName;

				// We only send if there is a .txt file

				if (_stricmp(&Name[strlen(Name) - 4], ".txt") == 0)
					len += sprintf(&popup[len], " <option value=%d:%d,%d>%s", DirNo, SubDirNo, i, Name);
			}
		}
		else
		{
			for (i = 0; i < Dir->FormCount; i++)
			{
				char * Name = Dir->Forms[i]->FileName;

				// We only send if there is a .txt file

				if (_stricmp(&Name[strlen(Name) - 4], ".txt") == 0)
					len += sprintf(&popup[len], " <option value=%d,%d>%s", DirNo, i, Name);
			}
		}
		len += sprintf(&popup[len], "</select></p>");

		*RLen = sprintf(Reply, "%s", popup);
		return;
	}
	
	if (_stricmp(NodeURL, "/WebMail/DL") == 0)
	{
		getAttachmentList(Session, Reply, RLen, URLParams);
		return;
	}

	if (_stricmp(NodeURL, "/WebMail/GetDownLoad") == 0)
	{
		DownloadAttachments(Session, Reply, RLen, URLParams);
		return;
	}

	if (_stricmp(NodeURL, "/WebMail/DoSelect") == 0)
	{
		// User has selected item from Template <select> field

		ProcessSelectResponse(Session, URLParams);

		return;
	}


	// Unrecognised message - reset session

	*RLen = sprintf(Reply, WebMailSignon, BBSName, BBSName);
}


VOID SendTemplateSelectScreen(struct HTTPConnectionInfo * Session, char *Params, int InputLen)
{
	// Save any supplied message fields and Send HTML Template dropdown list

	char popuphddr[] = 
			
		"<html><body align=center background='/background.jpg'>"
		"<script>"
		"function myFunction(val) {var x = document.getElementById(val).value;"
		"var Key = \"%s\";"
		"var param = \"toolbar=yes,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes\";"
		"window.open(\"/WebMail/GetList/\" + x + \"?\" + Key,\"_self\",param);"
		"}"
		"</script>"
		"<p align=center>"
		" Select Required Template Folder from List<br><br>"
		"<table border=1 cellpadding=2 bgcolor=white>"
		"<tr>"
		"<th>Standard Templates</th>"
		"<th>Local Templates</th>"
		"</tr>"
		"<tr><td width=50%%><select size=15 id=\"Sel1\" onclick=\"myFunction('Sel1')\">";

	char NewGroup [] =
		"</select></td><td width=50%% align=center>"
		"<select size=15 id=Sel2 onclick=\"myFunction('Sel2')\">";

	char popup[10000];
	struct HtmlFormDir * Dir;
	char * LastGroup;
	char * Input = strstr(Params, "\r\n\r\n"); // To end of HTML header
	int i;
	int MsgLen = 0;
	char * Boundary;
	int len;
				
	WebMailInfo * WebMail = Session->WebMail;

	Input = Params;

	Boundary = initMultipartUnpack(&Input);

	if (Boundary == NULL)
		return;	// Can't work without one

	// Input points to start of part. Normally preceeded by \r\n which is Boundary Terminator. If preceeded by -- we have used last part

	while(Input && Input[-1] != '-')
	{
		char * Name, * Value;
		int ValLen;
		
		if (unpackPart(Boundary, &Input, &Name, &Value, &ValLen, Params + InputLen) == FALSE)
		{
//			ReportCorrupt(WebMail);
			free(Boundary);
			return;
		}
		SaveInputValue(WebMail, Name, Value, ValLen);
	}

	strlop(WebMail->BID, ' ');
	if (strlen(WebMail->BID) > 12)
		WebMail->BID[12] = 0;

	UndoTransparency(WebMail->To);
	UndoTransparency(WebMail->CC);
	UndoTransparency(WebMail->BID);
	UndoTransparency(WebMail->Subject);
	UndoTransparency(WebMail->Body);

	// Save values from message when Template requested (npt sure if we need these!

	WebMail->OrigTo = _strdup(WebMail->To);
	WebMail->OrigSubject = _strdup(WebMail->Subject);
	WebMail->OrigType = WebMail->Type;
	WebMail->OrigBID = _strdup(WebMail->BID);
	WebMail->OrigBody = _strdup(WebMail->Body);

	// Also to active fields in case not changed by form

	len = sprintf(popup, popuphddr, Session->Key);

	LastGroup = HtmlFormDirs[0]->FormSet;		// Save so we know when changes

	for (i = 0; i < FormDirCount; i++)
	{
		int n;
			
		Dir = HtmlFormDirs[i];

		if (strcmp(LastGroup, Dir->FormSet) != 0)
		{
			LastGroup = Dir->FormSet;
			len += sprintf(&popup[len], "%s", NewGroup);
		}

		len += sprintf(&popup[len], " <option value=%d>%s", i, Dir->DirName);
			
		// Recurse any Subdirs
			
		n = 0;
		while (n < Dir->DirCount)
		{
			len += sprintf(&popup[len], " <option value=%d:%d>%s", i, n, Dir->Dirs[n]->DirName);
			n++;
		}
	}
	len += sprintf(&popup[len], "%</select></td></tr></table></p>");

	*WebMail->RLen = sprintf(WebMail->Reply, "%s", popup);

	free(Boundary);
	return;
}

static char WinlinkAddr[] = "WINLINK.ORG";

VOID SaveNewMessage(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest, int InputLen)
{
	int i, ReplyLen = 0;
	struct MsgInfo * Msg;
	FILE * hFile;
	int Template=0;
	char * via = NULL;
	BIDRec * BIDRec;
	char MsgFile[MAX_PATH];
	size_t WriteLen=0;
	char * HDest;
	char * HDestCopy;
	char * HDestRest;
	char * Vptr = NULL;
	char * FileList = NULL;
	char Prompt[256] = "Message Saved";
	char OrigTo[256];
	WebMailInfo * WebMail = Session->WebMail;
	struct UserInfo * user;
	CIRCUIT  conn;

	// So we can have attachments input is now Content-Type: multipart/form-data;

	char * Input; 
	size_t MsgLen = 0;
	char * Boundary;		

	strcpy(conn.Callsign, Session->User->Call);

	Input = MsgPtr;

	Boundary = initMultipartUnpack(&Input);

	if (Boundary == NULL)
		return;	// Can't work without one

	// Input points to start of part. Normally preceeded by \r\n which is Boundary Terminator. If preceeded by -- we have used last part

	while(Input && Input[-1] != '-')
	{
		char * Name, * Value;
		int ValLen;

		if (unpackPart(Boundary, &Input, &Name, &Value, &ValLen, MsgPtr + InputLen) == FALSE)
		{
			//			ReportCorrupt(WebMail);
			free(Boundary);
			return;
		}
		if (SaveInputValue(WebMail, Name, Value, ValLen) == FALSE)
		{
			*RLen = sprintf(Reply, "<html><script>window.location.href = '/Webmail/WebMail?%s';</script>", Session->Key);
			return;
		}
	}


	if (WebMail->txtFileName)
	{
		// Processing Form Input

		SaveTemplateMessage(Session, MsgPtr, Reply, RLen, Rest);

		// Prevent re-entry

		free(WebMail->txtFileName);
		WebMail->txtFileName = NULL;

		return;
	}

	// If we aren't using a template then all the information is in the WebMail fields, as we haven't been here before.

	strlop(WebMail->BID, ' ');
	if (strlen(WebMail->BID) > 12)
		WebMail->BID[12] = 0;

	UndoTransparency(WebMail->BID);
	UndoTransparency(WebMail->To);
	UndoTransparency(WebMail->Subject);
	UndoTransparency(WebMail->Body);

	MsgLen = strlen(WebMail->Body);

	// We will need to mess about with To field. Create a copy so the original can go in B2 header if we use one

	if (WebMail->To[0] == 0)
	{
		*RLen = sprintf(Reply, "%s", "<html><script>alert(\"To: Call missing!\");window.history.back();</script></html>");
		FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
		return;
	}

	if (strlen(WebMail->To) > 255)
	{
		*RLen = sprintf(Reply, "%s", "<html><script>alert(\"To: Call too long!\");window.history.back();</script></html>");
		FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
		return;
	}

	HDest = _strdup(WebMail->To);

	if (strlen(WebMail->BID))
	{		
		if (LookupBID(WebMail->BID))
		{
			// Duplicate bid
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"Duplicate BID\");window.history.back();</script></html>");
			FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
			return;
		}
	}

	if (WebMail->Type == 'B')
	{
		if (RefuseBulls)
		{
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"This system doesn't allow sending Bulls\");window.history.back();script></html>");
			FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
			return;
		}

		if (Session->User->flags & F_NOBULLS)
		{
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"You are not allowed to send Bulls\");window.history.back();</script></html>");
			FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
			return;
		}
	}

	// ?? Can we just loop though the rest of the code to allow multiple dests ??

	HDestCopy = HDest;

	while (HDest && HDest[0])
	{
		HDestRest = strlop(HDest, ';');

		Msg = AllocateMsgRecord();

		// Set number here so they remain in sequence

		Msg->number = ++LatestMsg;
		MsgnotoMsg[Msg->number] = Msg;

		strcpy(Msg->from, Session->User->Call);

		if (_memicmp(HDest, "rms:", 4) == 0 || _memicmp(HDest, "rms/", 4) == 0)
		{
			Vptr=&HDest[4];
			strcpy(Msg->to, "RMS");
		}
		else if (_memicmp(HDest, "smtp:", 5) == 0)
		{
			if (ISP_Gateway_Enabled)
			{
				Vptr=&HDest[5];
				Msg->to[0] = 0;
			}
		}
		else if (strchr(HDest, '@'))
		{
			strcpy(OrigTo, HDest);

			Vptr = strlop(HDest, '@');

			if (Vptr)
			{
				// If looks like a valid email address, treat as such

				if (strlen(HDest) > 6 || !CheckifPacket(Vptr))
				{
					// Assume Email address

					Vptr = OrigTo;

					if (FindRMS() || strchr(Vptr, '!')) // have RMS or source route
						strcpy(Msg->to, "RMS");
					else if (ISP_Gateway_Enabled)
						Msg->to[0] = 0;
					else if (isAMPRMsg(OrigTo))
						strcpy(Msg->to, "RMS");		// Routing will redirect it
					else
					{		
						*RLen = sprintf(Reply, "%s", "<html><script>alert(\"This system doesn't allow Sending to Internet Email\");window.close();</script></html>");
						FreeWebMailFields(WebMail);		// We will reprocess message and attachments so reinitialise
						return;

					}
				}
				else
					strcpy(Msg->to, _strupr(HDest));
			}
		}
		else
		{
			// No @

			if (strlen(HDest) > 6)
				HDest[6] = 0;

			strcpy(Msg->to, _strupr(HDest));
		}

		if (SendBBStoSYSOPCall)
			if (_stricmp(HDest, BBSName) == 0)
				strcpy(Msg->to, SYSOPCall);

		if (Vptr)
		{
			if (strlen(Vptr) > 40)
				Vptr[40] = 0;

			strcpy(Msg->via, _strupr(Vptr));
		}
		else
		{
			// No via. If not local user try to add BBS 

			struct UserInfo * ToUser = LookupCall(Msg->to);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->flags & F_RMSREDIRECT)
				{
					// sent to Winlink
				
					strcpy(Msg->via, WinlinkAddr);
					sprintf(Prompt, "Redirecting to winlink.org\r");
				}
				else if (ToUser->HomeBBS[0])
				{
					strcpy(Msg->via, ToUser->HomeBBS);
					sprintf(Prompt, "%s added from HomeBBS. Message Saved", Msg->via);
				}
			}
			else
			{
				// Not local user - Check WP

				WPRecP WP = LookupWP(Msg->to);

				if (WP)
				{
					strcpy(Msg->via, WP->first_homebbs);
					sprintf(Prompt, "%s added from WP", Msg->via);
				}
			}
		}

		if (strlen(WebMail->Subject) > 60)
			WebMail->Subject[60] = 0;

		strcpy(Msg->title, WebMail->Subject);
		Msg->type = WebMail->Type;

		if (Session->User->flags & F_HOLDMAIL)
		{
			int Length=0;
			char * MailBuffer = malloc(100);
			char Title[100];

			Msg->status = 'H';

			Length = sprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
			sprintf(Title, "Message %d Held - %s", Msg->number, "User has Hold Messages flag set");
			SendMessageToSYSOP(Title, MailBuffer, Length);
		}
		else
			Msg->status = 'N';

		if (strlen(WebMail->BID) == 0)
			sprintf(Msg->bid, "%d_%s", LatestMsg, BBSName);
		else
			strcpy(Msg->bid, WebMail->BID);

		Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->u.msgno = LOWORD(Msg->number);
		BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

		Msg->length = (int)MsgLen + WebMail->HeaderLen + WebMail->FooterLen;

		sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);

		//	BuildFormMessage(Session, Msg);

		if (WebMail->Files)
		{
			// Send as B2

			char * B2Header = BuildB2Header(WebMail, Msg, NULL, 0);

			hFile = fopen(MsgFile, "wb");

			if (hFile)
			{
				WriteLen = fwrite(B2Header, 1, strlen(B2Header), hFile); 
				WriteLen += fwrite(WebMail->Header, 1, WebMail->HeaderLen, hFile); 
				WriteLen += fwrite(WebMail->Body, 1, MsgLen, hFile); 
				WriteLen += fwrite(WebMail->Footer, 1, WebMail->FooterLen, hFile); 
				WriteLen += fwrite("\r\n", 1, 2, hFile); 

				for (i = 0; i < WebMail->Files; i++)
				{
					WriteLen += fwrite(WebMail->FileBody[i], 1, WebMail->FileLen[i], hFile); 
					WriteLen += fwrite("\r\n", 1, 2, hFile); 
				}
				fclose(hFile);
				free(B2Header);

				Msg->length = (int)WriteLen;
			}
		}
		else
		{
			hFile = fopen(MsgFile, "wb");

			if (hFile)
			{
				WriteLen += fwrite(WebMail->Header, 1, WebMail->HeaderLen, hFile); 
				WriteLen += fwrite(WebMail->Body, 1, MsgLen, hFile); 
				WriteLen += fwrite(WebMail->Footer, 1, WebMail->FooterLen, hFile); 
				fclose(hFile);
			}
		}
		MatchMessagetoBBSList(Msg, &conn);

		BuildNNTPList(Msg);				// Build NNTP Groups list

		if (Msg->status != 'H' && Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
			Msg->status = '$';				// Has forwarding


		if (EnableUI)
			SendMsgUI(Msg);	

		user = LookupCall(Msg->to);

		// If Event Notifications enabled report a new message event

		SendNewMessageEvent(user->Call, Msg);

#ifndef NOMQTT
		if (MQTT)
			MQTTMessageEvent(Msg);
#endif

		if (user && (user->flags & F_APRSMFOR))
		{
			char APRS[128];
			char Call[16];
			int SSID = user->flags >> 28;

			if (SSID)
				sprintf(Call, "%s-%d", Msg->to, SSID);
			else
				strcpy(Call, Msg->to);

			sprintf(APRS, "New BBS Message %s From %s", Msg->title, Msg->from);
			APISendAPRSMessage(APRS, Call);
		}

		HDest = HDestRest;
	}

	*RLen = SendWebMailHeaderEx(Reply, Session->Key, Session, Prompt);

	SaveMessageDatabase();
	SaveBIDDatabase();
	FreeWebMailFields(WebMail);
	free(HDestCopy);

	return;
}






// RMS Express Forms Support

char * GetHTMLViewerTemplate(char * FN, struct HtmlFormDir ** FormDir)
{
	int i, j, k, l;

	// Seach list of forms for base file (without .html)

	for (i = 0; i < FormDirCount; i++)
	{
		struct HtmlFormDir * Dir = HtmlFormDirs[i];

		for (j = 0; j < Dir->FormCount; j++)
		{
			if (strcmp(FN, Dir->Forms[j]->FileName) == 0)
			{
				*FormDir = Dir; 
				return CheckFile(Dir, FN);
			}
		}

		if (Dir->DirCount)
		{
			for (l = 0; l < Dir->DirCount; l++)
			{
				struct HtmlFormDir * SDir = Dir->Dirs[l];

				if (SDir->DirCount)
				{
					struct HtmlFormDir * SSDir = SDir->Dirs[0];
					int x = 1;
				}

				for (k = 0; k < SDir->FormCount; k++)
				{
					if (_stricmp(FN, SDir->Forms[k]->FileName) == 0)
					{
						*FormDir = SDir; 
						return CheckFile(SDir, SDir->Forms[k]->FileName);
					}
				}
				if (SDir->DirCount)
				{
					struct HtmlFormDir * SSDir = SDir->Dirs[0];
					int x = 1;
				}
			}
		}
	}

	return NULL;
}
VOID GetReply(struct HTTPConnectionInfo * Session, char * NodeURL)
{
}

VOID GetPage(struct HTTPConnectionInfo * Session, char * NodeURL)
{
		// Read the HTML Template file and do any needed substitutions
	
	WebMailInfo * WebMail = Session->WebMail;
	KeyValues * txtKey = WebMail->txtKeys;

	int DirNo;
	char * ptr;
	int FileNo = 0;
	char * SubDir;
	int SubDirNo;
	int i;
	struct HtmlFormDir * Dir;
	char * Template;
	char * inptr;
	char FormDir[MAX_PATH] = "";
	char FN[MAX_PATH] = "";
	char * InputName = NULL;	// HTML to input message
	char * ReplyName = NULL;
	char * To = NULL;
	char * CC = NULL;
	char * BID = NULL;
	char Type = 0;
	char * Subject = NULL;
	char * MsgBody = NULL;
	char * varptr;
	char * endptr;
	size_t varlen, vallen = 0;
	char val[256]="";			// replacement text
	char var[100] = "\"";
	char * MsgBytes;
	char Submit[64];

	if (NodeURL == NULL)
	{
		//rentry after processing <select> or <ask>
			
		goto reEnter;
	}

	DirNo = atoi(&NodeURL[17]);
	ptr = strchr(&NodeURL[17], ',');
	Dir = HtmlFormDirs[DirNo];


	if (DirNo == -1)
	{
		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"No Page Selected. \");window.location.href = '/Webmail/NewMsg?%s';</script></html>", Session->Key);
		return;
	}

	SubDir = strlop(&NodeURL[17], ':');
	if (SubDir)
	{
		SubDirNo = atoi(SubDir);
		Dir = Dir->Dirs[SubDirNo];
	}

	sprintf(Submit, "/Webmail/Submit?%s", Session->Key);

	if (ptr)
		FileNo = atoi(ptr + 1);

	// First we read the .txt. then get name of input .html from it

	if (WebMail->txtFile)
		free(WebMail->txtFile);

	WebMail->txtFile = NULL;

	if (WebMail->txtFileName)
		free(WebMail->txtFileName);

	WebMail->txtFileName = _strdup(Dir->Forms[FileNo]->FileName);

	// Read the file here to simplify reentry if we do <select> or <ask> substitutions

	// if Dir not specified search all for Filename

	if (Dir == NULL)
	{
		for (i = 0; i < FormDirCount; i++)
		{
			int n;
			
			Dir = HtmlFormDirs[i];

			MsgBytes = CheckFile(Dir, WebMail->txtFileName);
			if (MsgBytes)
				goto gotFile;
			
			// Recurse any Subdirs
			
			n = 0;
			while (n < Dir->DirCount)
			{
				MsgBytes = CheckFile(Dir->Dirs[n], FN);
				if (MsgBytes)
				{
					Dir = Dir->Dirs[n];
					goto gotFile;
				}
				n++;
			}
		}
		return;
	}
	else
		MsgBytes = CheckFile(Dir, WebMail->txtFileName);

gotFile:

	WebMail->Dir = Dir;

	if (WebMail->txtFile)
		free(WebMail->txtFile);

	WebMail->txtFile = MsgBytes;

reEnter:

	if (ParsetxtTemplate(Session, Dir, WebMail->txtFileName, FALSE) == FALSE)
	{
		// Template has <select> or <ask> tags and value has been requested
			
		return;
	}

	if (WebMail->InputHTMLName == NULL)
	{
		// This is a plain text template without HTML 

		if (To == NULL &&WebMail->To && WebMail->To[0])
			To = WebMail->To;
		else
			To = "";

		if (CC == NULL && WebMail->CC && WebMail->CC[0])
			CC = WebMail->CC;
		else
			CC = "";
	

		if (Subject == NULL && WebMail->Subject && WebMail->Subject[0])
			Subject = WebMail->Subject;
		else
			Subject = "";

		if (BID == NULL && WebMail->BID && WebMail->BID[0])
			BID = WebMail->BID;
		else
			BID = "";

		if (Type == 0 && WebMail->Type) 
			Type = WebMail->Type;
		else
			Type = 'P';

		if (MsgBody == NULL)
			MsgBody = "";

		if (MsgBody[0] == 0 && WebMail->Body && WebMail->Body[0])
			MsgBody = WebMail->Body;

		*WebMail->RLen = sprintf(WebMail->Reply, CheckFormMsgPage, Session->Key, To, CC, Subject,
			(Type =='P') ? "Selected" : "", (Type =='B') ? "Selected" : "", (Type =='T') ? "Selected" : "",  BID, MsgBody);
		return;
	}

		Template = CheckFile(Dir, WebMail->InputHTMLName);

		if (Template == NULL)
		{
			*WebMail->RLen  = sprintf(WebMail->Reply, "%s", "HTML Template not found");
			return;
		}

		// I've going to update the template in situ, as I can't see a better way
		// of making sure all occurances of variables in any order are substituted.
		// The space allocated to Template is twice the size of the file
		// to allow for insertions

		// First find the Form Action string and replace with our URL. It should have
		// action="http://{FormServer}:{FormPort}" but some forms have localhost:8001 instead

		// Also remove the OnSubmit if it contains the standard popup about having to close browser
	
		UpdateFormAction(Template, Session->Key);

		// Search for "{var }" strings in form and replace with
		// corresponding variable

		// we run through the Template once for each variable

		while (txtKey->Key)
		{
			char Key[256] = "{";
		
			strcpy(&Key[1], &txtKey->Key[1]);
		
			ptr = strchr(Key, '>');
			if (ptr)
				*ptr = '}';

			inptr = Template;
			varptr = stristr(inptr, Key);

			while (varptr)
			{
				// Move the remaining message up/down the buffer to make space for substitution

				varlen = strlen(Key);
				if (txtKey->Value)
					vallen = strlen(txtKey->Value);
				else vallen = 0;

				endptr = varptr + varlen;
		
				memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
				memcpy(varptr, txtKey->Value, vallen);

				inptr = endptr + 1;
		
				varptr = stristr(inptr, Key);
			}
			txtKey++;
		}

		// Remove </body></html> from end as we add it on later

		ptr = stristr(Template, "</body></html>");
		
		if (ptr)
			*ptr = 0;
		
		*WebMail->RLen = sprintf(WebMail->Reply, "%s", Template);
		free(Template);	
		return;
}



char * xxReadTemplate(char * FormSet, char * DirName, char *FileName)
{
	int FileSize;
	char * MsgBytes;
	char MsgFile[265];
	size_t ReadLen;
	struct stat STAT;
	FILE * hFile;

#ifndef WIN32

	// Need to do case insensitive file search

	DIR *dir;
	struct dirent *entry;
	char name[256];

	sprintf(name, "%s/%s/%s", BPQDirectory, FormSet, DirName);

	if (!(dir = opendir(name)))
	{
		Debugprintf("cant open forms dir %s %d %d", name, errno, dir);
        return 0;
	}

    while ((entry = readdir(dir)) != NULL)
	{
        if (entry->d_type == DT_DIR)
                continue;
	
		if (stristr(entry->d_name, FileName))
		{
			sprintf(MsgFile, "%s/%s/%s/%s", GetBPQDirectory(), FormSet, DirName, entry->d_name);
		    closedir(dir);
			break;
		}
	}
    closedir(dir);

#else

	sprintf(MsgFile, "%s/%s/%s/%s", GetBPQDirectory(), FormSet, DirName, FileName);

#endif

	if (stat(MsgFile, &STAT) != -1)
	{
		hFile = fopen(MsgFile, "rb");
	
		if (hFile == 0)
		{
			MsgBytes = _strdup("File is missing");
			return MsgBytes;
		}

		FileSize = STAT.st_size;
		MsgBytes = malloc(FileSize * 2);		// Allow plenty of room for template substitution
		ReadLen = fread(MsgBytes, 1, FileSize, hFile); 
		MsgBytes[FileSize] = 0;
		fclose(hFile);

		return MsgBytes;
	}
	
	return NULL;
}

int	ReturnRawMessage(struct UserInfo * User, struct MsgInfo * Msg, char * Key, char * Reply, char * RawMessage, int len, char * ErrorString)
{
	char * ErrorMsg = malloc(len + 100);
	char * ptr;
	char DownLoad[256];

	sprintf(DownLoad, "<td><a href=/WebMail/DL?%s&%d>Save Attachments</a></td>", Key, Msg->number);

	RawMessage[strlen(RawMessage)] = '.'; // We null terminated file name 
	RawMessage[strlen(RawMessage)] = ' '; // We null terminated file name 	Len = XML - RawMessage; 

	RawMessage[len] = 0;

	// Body seems to have cr cr lf which causes double space

	ptr = strstr(RawMessage, "\r\r");

	while (ptr)
	{
		*ptr = ' ';
		ptr = strstr(ptr, "\r\r");
	}

	sprintf(ErrorMsg, ErrorString, RawMessage);

	len = sprintf(Reply, WebMailMsgTemplate, BBSName, User->Call, Msg->number, Msg->number, Key, Msg->number, Key, DownLoad, Key, Key, Key, "textarea", ErrorMsg, "textarea");
	free(ErrorMsg);
	return len;
}

char * FindXMLVariable(WebMailInfo * WebMail, char * Var)
{
	KeyValues * XMLKey = &WebMail->XMLKeys[0];

	while (XMLKey->Key)
	{
		if (_stricmp(Var, XMLKey->Key) == 0)
		{
			return XMLKey->Value;
		}

		XMLKey++;
	}
	return NULL;
}


BOOL ParseXML(WebMailInfo * WebMail, char * XMLOrig)
{
	char * XML = _strdup(XMLOrig);		// Get copy so we can mess about with it
	char * ptr1, * ptr2, * ptr3;
	KeyValues * XMLKeys = &WebMail->XMLKeys[0];

	// Extract Fields (stuff between < and >. Ignore Whitespace between fields

	// Add FormFolder Key with our folder

//	XMLKeys->Key = "FormFolder";
//	XMLKeys->Value = _strdup(FormDir);

//	XMLKeys++;

	ptr1 = strstr(XML, "<xml_file_version>");

	while (ptr1)
	{
		ptr2 = strchr(++ptr1, '>');

		if (ptr2 == NULL)
			goto quit;

		*ptr2++ = 0;

		ptr3 = strstr(ptr2, "</");	// end of value string
		if (ptr3 == NULL)
			goto quit;

		*ptr3++ = 0;

		XMLKeys->Key = _strdup(ptr1);
		XMLKeys->Value = _strdup(ptr2);

		XMLKeys++;

		ptr1 = strchr(ptr3, '<');

		if (_memicmp(ptr1, "</", 2) == 0)
		{
			// end of a parameter block. Find start of next block

			ptr1 = strchr(++ptr1, '<');
			ptr1 = strchr(++ptr1, '<');		// Skip start of next block
		}
	}




quit:
	free(XML);
	return TRUE;
}
/*
?xml version="1.0"?>
<RMS_Express_Form>
 <form_parameters>
 <xml_file_version>1,0</xml_file_version>
 <rms_express_version>6.0.16.38 Debug Build</rms_express_version>
 <submission_datetime>20181022105202</submission_datetime>
 <senders_callsign>G8BPQ</senders_callsign>
 <grid_square></grid_square>
 <display_form>Alaska_ARES_ICS213_Initial_Viewer.html</display_form>
 <reply_template>Alaska_ARES_ICS213_SendReply.txt</reply_template>
 </form_parameters>
<variables>
 <msgto>g8bpq</msgto>
 <msgcc></msgcc>
 <msgsender>G8BPQ</msgsender>
 <msgsubject></msgsubject>
 <msgbody></msgbody>
 <msgp2p>false</msgp2p>
 <msgisreply>false</msgisreply>
 <msgisforward>false</msgisforward>
 <msgisacknowledgement>false</msgisacknowledgement>
 <SeqNum>G8BPQ-1</SeqNum>
 <Priority>Routine</Priority>
 <HX></HX>
 <OrgStation></OrgStation>
 <Check></Check>
 <OrgLocation>Here</OrgLocation>
 <Time>11:51</Time>
 <Date>2018-10-22</Date>
 <Incident_Name>Test</Incident_Name>
 <To_Name>John</To_Name>
 <From_Name>John</From_Name>
 <Subjectline>Test</Subjectline>
 <DateTime>2018-10-22 11:51</DateTime>
 <Message></Message>
 <Approved_Name>Me</Approved_Name>
 <Approved_PosTitle>Me</Approved_PosTitle>
 <Submit>Submit</Submit>
</variables>
</RMS_Express_Form>
*/


char HTMLNotFoundMsg[] = " *** HTML Template for message not found - displaying raw content ***\r\n\r\n%s";
char VarNotFoundMsg[] = " *** Variable {%s} not found in message - displaying raw content ***\r\n\r\n%s";
char BadXMLMsg[] = " *** XML for Variable {%s} invalid - displaying raw content ***\r\n\r\n%s";
char BadTemplateMsg[] = " *** Template near \"%s\" invalid - displaying raw content ***\r\n\r\n%s";

int DisplayWebForm(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, char * FileName, char * XML, char * Reply, char * RawMessage, int RawLen)
{
	WebMailInfo * WebMail = Session->WebMail;
	struct UserInfo * User = Session->User;
	char * Key = Session->Key;
	int Len = 0;
	char * Form;
	char * SaveReply = Reply;
	char FN[MAX_PATH] = "";
	char * formptr, * varptr, * xmlptr;
	char * ptr = NULL, * endptr, * xmlend;
	size_t varlen, xmllen;
	char var[100] = "<";
	KeyValues * KeyValue; 
	struct HtmlFormDir * Dir;
	char FormDir[MAX_PATH];

	if (ParseXML(WebMail, XML))
		ptr = FindXMLVariable(WebMail, "display_form");

	if (ptr == NULL)		// ?? No Display Form Specified??
	{
		// Not found - just display as normal message

		return ReturnRawMessage(User, Msg, Key, Reply, RawMessage, (int)(XML - RawMessage), HTMLNotFoundMsg);
	}

	strcpy(FN, ptr);

	Form = GetHTMLViewerTemplate(FN, &Dir);

	sprintf(FormDir, "WMFile/%s/%s/", Dir->FormSet, Dir->DirName);



	if (Form == NULL)
	{
		// Not found - just display as normal message

		return ReturnRawMessage(User, Msg, Key, Reply, RawMessage, (int)(XML - RawMessage), HTMLNotFoundMsg);
	}

	formptr = Form;

	// Search for {var xxx} strings in form and replace with
	// corresponding variable in xml

	// Don't know why, but {MsgOriginalBody} is sent instead of {var MsgOriginalBody}

	// So is {FormFolder} instread of {var FormFolder}

	// As a fiddle replace {FormFolder} with {var Folder} and look for that

	while (varptr = stristr(Form, "{FormFolder}"))
	{
		memcpy(varptr, "{var ", 5);
	}

	varptr = stristr(Form, "{MsgOriginalBody}");
	{
		char * temp, * tempsave;
		char * xvar, * varsave, * ptr;

		if (varptr)
		{
			varptr++;
		
			endptr = strchr(varptr, '}');
			varlen = endptr - varptr;

			if (endptr == NULL || varlen > 99)
			{
				// corrupt template - display raw message
	
				char Err[256];

				varptr[20] = 0;
		
				sprintf(Err, BadTemplateMsg, varptr - 5, "%s");
				return ReturnRawMessage(User, Msg, Key, SaveReply, RawMessage, (int)(XML - RawMessage), Err);
			}
	
			memcpy(var + 1, varptr, varlen);
			var[++varlen] = '>';
			var[++varlen] = 0;

			xmlptr = stristr(XML, var);

			if (xmlptr)
			{
				xmlptr += varlen;
			
				xmlend = strstr(xmlptr, "</");
			
				if (xmlend == NULL)
				{
					// Bad XML - return error message

					char Err[256];

					// remove <> from var as it confuses html

					var[strlen(var) -1] = 0;
			
					sprintf(Err, BadXMLMsg, var + 1, "%s");
					return ReturnRawMessage(User, Msg, Key, SaveReply, RawMessage, (int)(XML - RawMessage), Err);
				}

				xmllen = xmlend - xmlptr;
			}
			else
			{	
				// Variable not found - return error message

				char Err[256];

				// remove <> from var as it confuses html

				var[strlen(var) -1] = 0;
			
				sprintf(Err, VarNotFoundMsg, var + 1, "%s");
				return ReturnRawMessage(User, Msg, Key, SaveReply, RawMessage, (int)(XML - RawMessage), Err);
			}

			// Ok, we have the position of the variable and the substitution text.
			// Copy message up to variable to Result, then copy value

			// We create a copy so we can rescan later.
			// We also need to replace CR or CRLF with <br> 

			xvar = varsave = malloc(xmllen * 2);

			ptr = xmlptr;

			while(ptr < xmlend)
			{
				while (*ptr == '\n')
					ptr++;

				if (*ptr  == '\r')
				{
					*ptr++;
					strcpy(xvar, "<br>");
					xvar += 4;
				}
				else
					*(xvar++) = *(ptr++);
			}
			*xvar = 0;

			temp = tempsave = malloc(strlen(Form) + strlen(XML));

			memcpy(temp, formptr, varptr - formptr - 1);	// omit "{"
			temp += (varptr - formptr - 1);

			strcpy(temp, varsave);
			temp += strlen(varsave);
			free(varsave);

			formptr = endptr + 1;
			strcpy(temp, formptr);	
			strcpy(Form, tempsave);
			free(tempsave);
		}				
	}

	formptr = Form;

	varptr = stristr(Form, "{var ");

	while (varptr)
	{
		varptr+= 5;
		
		endptr = strchr(varptr, '}');

		varlen = endptr - varptr;

		if (endptr == NULL || varlen > 99)
		{
			// corrupt template - display raw message
	
			char Err[256];

			varptr[20] = 0;
		
			sprintf(Err, BadTemplateMsg, varptr - 5, "%s");
			return ReturnRawMessage(User, Msg, Key, SaveReply, RawMessage, (int)(XML - RawMessage), Err);
		}
	
		memcpy(var, varptr, varlen);
		var[varlen] = 0;

		KeyValue = &WebMail->XMLKeys[0];

		while (KeyValue->Key)
		{
			if (_stricmp(var, "Folder") == 0)
			{
				// Local form folder, not senders

				xmllen = strlen(FormDir);

				// Ok, we have the position of the variable and the substitution text.
				// Copy message up to variable to Result, then copy value

				memcpy(Reply, formptr, varptr - formptr - 5);	// omit "{var "
				Reply += (varptr - formptr - 5);

				strcpy(Reply, FormDir);
				Reply += xmllen;
				break;
			}

			if (_stricmp(var, KeyValue->Key) == 0)
			{
				xmllen = strlen(KeyValue->Value);

				// Ok, we have the position of the variable and the substitution text.
				// Copy message up to variable to Result, then copy value

				memcpy(Reply, formptr, varptr - formptr - 5);	// omit "{var "
				Reply += (varptr - formptr - 5);

				strcpy(Reply, KeyValue->Value);
				Reply += xmllen;
				break;
			}

			KeyValue++;

			if (KeyValue->Key == NULL)
			{
				// Not found in XML

				char Err[256];
			
				sprintf(Err, VarNotFoundMsg, var, "%s");
				return ReturnRawMessage(User, Msg, Key, SaveReply, RawMessage, (int)(XML - RawMessage), Err);
			}

		}

		formptr = endptr + 1;

		varptr = stristr(endptr, "{var ");
	}	
	
	// copy remaining

	// Remove </body></html> as we add it later

	ptr = strstr(formptr, "</body>");

	if (ptr)
		*ptr = 0;

	strcpy(Reply, formptr);	

	// Add Webmail header between <Body> and form data

	ptr = stristr(SaveReply, "<body");

	if (ptr)
	{
		ptr = strchr(ptr, '>');
		if (ptr)
		{
			char * temp = malloc(strlen(SaveReply) + 1000);
			size_t len = ++ptr - SaveReply;
			memcpy(temp, SaveReply, len);

			sprintf(&temp[len],
				"<script>function Reply(Num, Key)"
				"{"
				"var param = \"toolbar=yes,location=yes,directories=yes,status=yes,menubar=yes,scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes\";"
				"window.open(\"/WebMail/Reply/\" + Num + \"?\" + Key,\"_self\",param);"
				"}</script>"
				"<h3 align=center> %s Webmail Interface - User %s - Message %d</h3>"
				"<table align=center border=1 cellpadding=2 bgcolor=white><tr>"
				"<td><a href=\"#\" onclick=\"Reply('%d' ,'%s'); return false;\">Reply</a></td>"
				"<td><a href=/WebMail/WMDel/%d?%s>Kill Message</a></td>"
				"<td><a href=/WebMail/DisplayText?%s&%d>Display as Text</a></td>"
				"<td><a href=/WebMail/WMNext?%s>Next</a></td>"
				"<td><a href=/WebMail/WMPrev?%s>Previous</a></td>"
				"<td><a href=/WebMail/WMSame?%s>Back to List</a></td>"
				"</tr></table>", BBSName, User->Call, Msg->number, Msg->number, Key, Msg->number, Key,  Key, Msg->number, Key, Key, Key);

			strcat(temp, ptr);

			strcpy(SaveReply, temp);
			free(temp);
		}
	}

	if (Form)
		free(Form);

	return (int)strlen(SaveReply);
}

char * BuildB2Header(WebMailInfo * WebMail, struct MsgInfo * Msg, char ** ToCalls, int Calls)
{
	// Create B2 Header
	
	char * NewMsg = malloc(100000);
	char * SaveMsg = NewMsg;
	char DateString[80];
	struct tm * tm;
	int n;
	char Type[16] = "Private";

	// Get Type
	
	if (Msg->type == 'B')
		strcpy(Type, "Bulletin");
	else if (Msg->type == 'T')
		strcpy(Type, "Traffic");

	tm = gmtime((time_t *)&Msg->datecreated);	
	
	sprintf(DateString, "%04d/%02d/%02d %02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	NewMsg += sprintf(NewMsg,
		"MID: %s\r\n"
		"Date: %s\r\n"
		"Type: %s\r\n"
		"From: %s\r\n",
			Msg->bid, DateString, Type, Msg->from);

	if (ToCalls)
	{
		int i;

		for (i = 0; i < Calls; i++)
			NewMsg += sprintf(NewMsg, "To: %s\r\n",	ToCalls[i]);
	}
	else
	{
		NewMsg += sprintf(NewMsg, "To: %s\r\n",
			WebMail->To);
	}
	if (WebMail->CC && WebMail->CC[0])
		NewMsg += sprintf(NewMsg, "CC: %s\r\n", WebMail->CC);

	NewMsg += sprintf(NewMsg,
		"Subject: %s\r\n"
		"Mbo: %s\r\n",
		Msg->title, BBSName);

	NewMsg += sprintf(NewMsg, "Body: %d\r\n", (int)strlen(WebMail->Body) + WebMail->HeaderLen + WebMail->FooterLen);

	Msg->B2Flags = B2Msg;

	if (WebMail->XML)
	{
		Msg->B2Flags |= Attachments;
		NewMsg += sprintf(NewMsg, "File: %d %s\r\n",
			WebMail->XMLLen, WebMail->XMLName);
	}

	for (n = 0; n < WebMail->Files; n++)
	{
		Msg->B2Flags |= Attachments;
		NewMsg += sprintf(NewMsg, "File: %d %s\r\n",
			WebMail->FileLen[n], WebMail->FileName[n]);
	}

	NewMsg += sprintf(NewMsg, "\r\n");		// Blank Line to end header

	return SaveMsg;
}

VOID WriteOneRecipient(struct MsgInfo * Msg, WebMailInfo * WebMail, int MsgLen, char ** ToCalls, int Calls, char * BID)
{
	FILE * hFile;
	char * via = NULL;
	BIDRec * BIDRec;
	char MsgFile[MAX_PATH];
	size_t WriteLen=0;
	char * B2Header;

	if (strlen(WebMail->Subject) > 60)
		WebMail->Subject[60] = 0;

	strcpy(Msg->title, WebMail->Subject);

	if (strlen(BID) == 0)
		sprintf_s(BID, 32, "%d_%s", LatestMsg, BBSName);

	strcpy(Msg->bid, BID);

	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	Msg->length = MsgLen;

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	// We write a B2 Header, Body and XML attachment if present

	B2Header = BuildB2Header(WebMail, Msg, ToCalls, Calls);

	hFile = fopen(MsgFile, "wb");
	
	if (hFile)
	{
		int i;
		
		WriteLen = fwrite(B2Header, 1, strlen(B2Header), hFile); 
		WriteLen += fwrite(WebMail->Body, 1, Msg->length, hFile); 
		WriteLen += fwrite("\r\n", 1, 2, hFile); 
		if (WebMail->XML)
		{
			WriteLen += fwrite(WebMail->XML, 1, WebMail->XMLLen, hFile); 
			WriteLen += fwrite("\r\n", 1, 2, hFile); 
		}
		// Do any attachments

		for (i = 0; i < WebMail->Files; i++)
		{
			WriteLen += fwrite(WebMail->FileBody[i], 1, WebMail->FileLen[i], hFile); 
			WriteLen += fwrite("\r\n", 1, 2, hFile); 
		}
		fclose(hFile);
	}

	free(B2Header);

	Msg->length = (int)WriteLen;

	MatchMessagetoBBSList(Msg, 0);

	if (Msg->status != 'H' && Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		Msg->status = '$';				// Has forwarding

	BuildNNTPList(Msg);				// Build NNTP Groups list

#ifndef NOMQTT
	if (MQTT)
		MQTTMessageEvent(Msg);
#endif

}


VOID SaveTemplateMessage(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	struct MsgInfo * Msg;
	char * ptr, *input, *context;
	size_t MsgLen;
	char Type;
	char * via = NULL;
	char BID[32];
	size_t WriteLen=0;
	char * Body = NULL;
	char * To = NULL;
	char * CC = NULL;
	char * HDest = NULL;
	char * Title = NULL;
	char * Vptr = NULL;
	char * Context;
	char Prompt[256] = "Message Saved";
	char OrigTo[256];
	WebMailInfo * WebMail = Session->WebMail;
	BOOL SendMsg = FALSE;
	BOOL SendReply = FALSE;

	char * RMSTo[1000] = {NULL};		// Winlink addressees
	char * PKTT0[1000] = {NULL};		// Packet addressees

	__int32 Recipients = 0;
	__int32 RMSMsgs = 0, BBSMsgs = 0;


	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input == NULL)
		return;
	
	if (strstr(input, "Cancel=Cancel"))
	{
		*RLen = sprintf(Reply, "<html><script>window.location.href = '/Webmail/WebMail?%s';</script>", Session->Key);
		return;
	}

	if (WebMail->txtFileName == NULL)
	{
		// No template, so user must have used back button

		*RLen = sprintf(Reply, "<html><script>alert(\"Template missing. Was back Button used? \");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
		return;
	}

	ptr = strtok_s(input + 4, "&", &Context);

	while (ptr)
	{
		char * val = strlop(ptr, '=');

		if (strcmp(ptr, "To") == 0)
			HDest = To = val;
		else if (strcmp(ptr, "CC") == 0)
			CC = val;
		else if (strcmp(ptr, "Subj") == 0)
			Title = val;
		else if (strcmp(ptr, "Type") == 0)
			Type = val[0];
		else if (strcmp(ptr, "BID") == 0)
			strcpy(BID, val);
		else if (strcmp(ptr, "Msg") == 0)
			Body = _strdup(val);

		ptr = strtok_s(NULL, "&", &Context);

	}
	strlop(BID, ' ');
	if (strlen(BID) > 12)
		BID[12] = 0;

	UndoTransparency(To);
	UndoTransparency(CC);
	UndoTransparency(BID);
	UndoTransparency(HDest);
	UndoTransparency(Title);
	UndoTransparency(Body);

	MsgLen = strlen(Body);

	// The user could have changed any of the input fields.

	if (To && To[0])
	{
		free (WebMail->To);
		WebMail->To = _strdup(To);
	}

	if (CC && CC[0])
	{
		free (WebMail->CC);
		WebMail->CC = _strdup(CC);
	}

	if (Title && Title[0])
	{
		free (WebMail->Subject);
		WebMail->Subject = _strdup(Title);
	}

	if (Body && Body[0])
	{
		free (WebMail->Body);
		WebMail->Body = _strdup(Body);
	}

	// We will put the supplied address in the B2 header
	
	// We may need to change the HDest to make sure message
	// is delivered to Internet or Packet as requested

	if (HDest == NULL || HDest[0] == 0)
	{
		*RLen = sprintf(Reply, "%s", "<html><script>alert(\"To: Call Missing\");window.history.back();</script></html>");
		return;
	}

	// Multiple TO fields could be more than 255 bytes long

//	if (strlen(HDest) > 255)
//	{
//		*RLen = sprintf(Reply, "%s", "<html><script>alert(\"To: Call too long!\");window.history.back();</script></html>");
//		return;
//	}

	if (strlen(BID))
	{		
		if (LookupBID(BID))
		{
			// Duplicate bid
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"Duplicate BID\");window.history.back();</script></html>");
			return;
		}
	}

	if (Type == 'B')
	{
		if (RefuseBulls)
		{
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"This system doesn't allow sending Bulls\");window.history.back();script></html>");
			return;
		}

		if (Session->User->flags & F_NOBULLS)
		{
			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"You are not allowed to send Bulls\");window.history.back();</script></html>");
			return;
		}
	}

	// We should be able to handle multiple recipients, with all Winlink addresses sent in one message and
	// multiple copies for packet addressess. For Winlink we should use multiple To@ lines in B2 Header

	ptr = strtok_s(HDest, " ;", &context);

	while (ptr && ptr[0])
	{
		int Winlink = 0;
		int AMPR = 0;
		
		char * dest = zalloc(256);
		char * via = NULL;

		strcpy(dest, ptr);

		// See if packet or Winlink

		// If Type=Winlink specified send plain call as @winlink.org 

		if (strchr(dest, '@') == 0 && WebMail->Winlink)
			strcat(dest, "@winlink.org");

		if (_memicmp(dest, "rms:", 4) == 0 || _memicmp(dest, "rms/", 4) == 0)
		{
			memcpy(dest, &dest[4], strlen(dest));
			Winlink = 1;
			RMSTo[RMSMsgs++] = dest;
			ptr = strtok_s(NULL, " ;", &context);
			continue;
		}

		else if (_memicmp(dest, "smtp:", 5) == 0)
		{
			if (ISP_Gateway_Enabled)
			{
				via = &dest[5];
				dest[0] = 0;
			}
		}
		else if (strchr(dest, '@'))
		{
			strcpy(OrigTo, dest);

			via = strlop(dest, '@');

			if (via)
			{
				// If looks like a valid email address, treat as such

				if (strlen(via) > 6 || !CheckifPacket(via))
				{
					// Assume Email address. See if to send via RMS or SMTP

					if (isAMPRMsg(OrigTo))
					{
						dest[strlen(dest)] = '@';		// Put back together
						memmove(&dest[1], dest, strlen(dest));
						dest[0] = 0;
						via = &dest[1];
						AMPR = 1;
					}
					else if (FindRMS() || strchr(via, '!')) // have RMS or source route
					{
						dest[strlen(dest)] = '@';		// Put back together
						Winlink = 1;
						RMSTo[RMSMsgs++] = dest;
						ptr = strtok_s(NULL, " ;", &context);
						continue;
					}
					else if (ISP_Gateway_Enabled)
					{
						dest[strlen(dest)] = '@';		// Put back together
						memmove(dest, &dest[1], strlen(dest));
						dest[0] = 0;
					}
					else
					{		
						*RLen = sprintf(Reply, "%s", "<html><script>alert(\"This system doesn't allow Sending to Internet Email\");window.close();</script></html>");
						return;

					}
				}
			}
		}
		else
		{
			// No @

			if (strlen(dest) > 6)
				dest[6] = 0;
		}

		// This isn't an RMS Message, so can queue now

		Msg = AllocateMsgRecord();
		
		// Set number here so they remain in sequence
		
		Msg->number = ++LatestMsg;
		MsgnotoMsg[Msg->number] = Msg;

		strcpy(Msg->title, WebMail->Subject);
		Msg->type = Type;
		Msg->status = 'N';


		strcpy(Msg->from, Session->User->Call);

		strcpy(Msg->to, _strupr(dest));
		
		if (SendBBStoSYSOPCall)
			if (_stricmp(dest, BBSName) == 0)
				strcpy(Msg->to, SYSOPCall);

		if (via)
		{
			if (strlen(via) > 40)
				via[40] = 0;

			strcpy(Msg->via, _strupr(via));
		}
		else
		{
			// No via. If not local user try to add BBS 

			struct UserInfo * ToUser = LookupCall(Msg->to);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->flags & F_RMSREDIRECT)
				{
					// sent to Winlink
				
					strcpy(Msg->via, WinlinkAddr);
					sprintf(Prompt, "Redirecting to winlink.org\r");
				}
				else if (ToUser->HomeBBS[0])
				{
					strcpy(Msg->via, ToUser->HomeBBS);
					sprintf(Prompt, "%s added from HomeBBS. Message Saved", Msg->via);
				}
			}
			else
			{
				// Not local user - Check WP

				WPRecP WP = LookupWP(Msg->to);

				if (WP)
				{
					strcpy(Msg->via, WP->first_homebbs);
					sprintf(Prompt, "%s added from WP", Msg->via);
				}
			}
		}		
		WriteOneRecipient(Msg, WebMail, MsgLen, NULL, 0, BID);
		ptr = strtok_s(NULL, " ;", &context);
		free(dest);
		BID[0] = 0;					// Can't use more than once
	}

	if (RMSMsgs)
	{
		// Write one message to all Winlink addresses

		int i;

		Msg = AllocateMsgRecord();
		
		// Set number here so they remain in sequence
		
		Msg->number = ++LatestMsg;
		MsgnotoMsg[Msg->number] = Msg;

		strcpy(Msg->title, WebMail->Subject);
		Msg->type = Type;
		Msg->status = 'N';

		strcpy(Msg->from, Session->User->Call);
		strcpy(Msg->to, "RMS");

		WriteOneRecipient(Msg, WebMail, MsgLen, RMSTo, RMSMsgs, BID);

		for (i = 0; i < RMSMsgs; i++)
			free(RMSTo[i]);

	}

	SaveMessageDatabase();
	SaveBIDDatabase();

	*RLen = SendWebMailHeaderEx(Reply, Session->Key, Session, Prompt);

	FreeWebMailFields(WebMail);			

	return;
}



VOID DoStandardTemplateSubsitutions(struct HTTPConnectionInfo * Session, char * txtFile)
{
	WebMailInfo * WebMail = Session->WebMail;
	struct UserInfo * User = Session->User;
	KeyValues * txtKey = WebMail->txtKeys;

	char * inptr, * varptr, * endptr;
	int varlen, vallen;

	while (txtKey->Key != NULL)
	{
		inptr = WebMail->txtFile;

		varptr = stristr(inptr, txtKey->Key);

		while (varptr)
		{
			// Move the remaining message up/down the buffer to make space for substitution

			varlen = (int)strlen(txtKey->Key);

			if (txtKey->Value)
				vallen = (int)strlen(txtKey->Value);
			else
				vallen = 0;

			endptr = varptr + varlen;
		
			memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
			memcpy(varptr, txtKey->Value, vallen);

			inptr = endptr + 1;
		
			varptr = stristr(inptr, txtKey->Key);
		}
		txtKey++;
	}
}



VOID BuildMessageFromHTMLInput(struct HTTPConnectionInfo * Session, char * Reply, int * RLen, char * Keys[1000], char * Values[1000], int NumKeys)
{
	int ReplyLen = 0;
	struct MsgInfo * Msg;
	int MsgLen;
	FILE * hFile;
	char Type = 'P';
	BIDRec * BIDRec;
	char * MailBuffer;
	char MsgFile[MAX_PATH];
	int WriteLen=0;
	char Prompt[256] = "Message Saved";
	char OrigTo[256];
	WebMailInfo * WebMail = Session->WebMail;
	char * HDest = _strdup(WebMail->To);
	char * Vptr = NULL;
	char BID[16] = "";

///	if (strlen(HDest) > 255)
///	{
//		*RLen = sprintf(Reply, "%s", "<html><script>alert(\"To: Call too long!\");</script></html>");
//		return;
//	}

	MsgLen = (int)strlen(WebMail->Body);	
	Msg = AllocateMsgRecord();
		
	// Set number here so they remain in sequence
		
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;

	strcpy(Msg->from, Session->User->Call);

	if (_memicmp(HDest, "rms:", 4) == 0 || _memicmp(HDest, "rms/", 4) == 0)
	{
		Vptr=&HDest[4];
		strcpy(Msg->to, "RMS");
	}
	else if (_memicmp(HDest, "smtp:", 5) == 0)
	{
		if (ISP_Gateway_Enabled)
		{
			Vptr=&HDest[5];
			Msg->to[0] = 0;
		}
	}
	else if (strchr(HDest, '@'))
	{
		strcpy(OrigTo, HDest);

		Vptr = strlop(HDest, '@');

		if (Vptr)
		{
			// If looks like a valid email address, treat as such

			if (strlen(HDest) > 6 || !CheckifPacket(Vptr))
			{
				// Assume Email address

				Vptr = OrigTo;

				if (FindRMS() || strchr(Vptr, '!')) // have RMS or source route
					strcpy(Msg->to, "RMS");
				else if (ISP_Gateway_Enabled)
					Msg->to[0] = 0;
				else if (isAMPRMsg(OrigTo))
					strcpy(Msg->to, "RMS");		// Routing will redirect it
				else
				{		
					*RLen = sprintf(Reply, "%s", "<html><script>alert(\"This system doesn't allow Sending to Internet Email\");window.close();</script></html>");
					return;
		
				}
			}
		}
	}

	else
	{	
		if (strlen(HDest) > 6)
			HDest[6] = 0;
		
		strcpy(Msg->to, _strupr(HDest));
	}

	if (SendBBStoSYSOPCall)
		if (_stricmp(HDest, BBSName) == 0)
			strcpy(Msg->to, SYSOPCall);

	if (Vptr)
	{
		if (strlen(Vptr) > 40)
			Vptr[40] = 0;

		strcpy(Msg->via, _strupr(Vptr));
	}
	else
	{
		// No via. If not local user try to add BBS 
	
		struct UserInfo * ToUser = LookupCall(Msg->to);

		if (ToUser)
		{
			// Local User. If Home BBS is specified, use it

			if (ToUser->flags & F_RMSREDIRECT)
			{
				// sent to Winlink
				
				strcpy(Msg->via, WinlinkAddr);
				sprintf(Prompt, "Redirecting to winlink.org\r");
			}
			else if (ToUser->HomeBBS[0])
			{
				strcpy(Msg->via, ToUser->HomeBBS);
				sprintf(Prompt, "%s added from HomeBBS", Msg->via);
			}
		}
		else
		{
			// Not local user - Check WP
			
			WPRecP WP = LookupWP(Msg->to);

			if (WP)
			{
				strcpy(Msg->via, WP->first_homebbs);
				sprintf(Prompt, "%s added from WP", Msg->via);
			}
		}
	}

	if (strlen(WebMail->Subject) > 60)
		WebMail->Subject[60] = 0;

	strcpy(Msg->title, WebMail->Subject);
	Msg->type = Type;
	Msg->status = 'N';

	if (strlen(BID) == 0)
		sprintf_s(BID, sizeof(BID), "%d_%s", LatestMsg, BBSName);

	strcpy(Msg->bid, BID);

	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	MailBuffer = malloc(MsgLen + 2000);		// Allow for a B2 Header if attachments

	Msg->length = MsgLen;

	BuildFormMessage(Session, Msg, Keys, Values, NumKeys);

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	hFile = fopen(MsgFile, "wb");
	
	if (hFile)
	{
		WriteLen = (int)fwrite(WebMail->Body, 1, Msg->length, hFile);
		fclose(hFile);
	}
	
	free(WebMail->Body);
	free(HDest);

	WebMail->Body = NULL;

	MatchMessagetoBBSList(Msg, 0);

	if (Msg->status != 'H' && Msg->type == 'B' && memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		Msg->status = '$';				// Has forwarding

	BuildNNTPList(Msg);				// Build NNTP Groups list

#ifndef NOMQTT
	if (MQTT)
		MQTTMessageEvent(Msg);
#endif


	SaveMessageDatabase();
	SaveBIDDatabase();

	*RLen = sprintf(Reply, "<html><script>alert(\"%s\");window.close();</script></html>", Prompt);

	return;
}









void ProcessFormInput(struct HTTPConnectionInfo * Session, char * input, char * Reply, int * RLen, int InputLen)
{
	// If there is no display html defined place data in a normal
	// input window, else build the Message body and XML attachment and send

	// I now think it is better to put data in a normal input window
	// even if there is a display form so user can view it before submission

	WebMailInfo * WebMail = Session->WebMail;

	char * info = strstr(input, "\r\n\r\n"); // To end of HTML header

	// look through header for Content-Type line, and if multipart
	// find boundary string.

	char * ptr, * saveptr, * ptr1, * ptr2, * inptr;
	char Boundary[1000];
	BOOL Multipart = FALSE;
	int Partlen;
	char ** Body = &info;
	int i;
	char * Keys[1000];
	char * Values[1000];
	char * saveForfree[1000];

	int NumKeys = 0;
	char * varptr;
	char * endptr;
	int varlen, vallen = 0;
	char *crcrptr;

	if (WebMail->txtFile == NULL)
	{
		// No template, so user must have used back button

		*RLen = sprintf(Reply, "<html><script>alert(\"Template missing. Was back Button used? \");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
		return;
	}

	ptr = input;

	while(*ptr != 13)
	{
		ptr2 = strchr(ptr, 10);	// Find CR

		while(ptr2[1] == ' ' || ptr2[1] == 9)		// Whitespace - continuation line
		{
			ptr2 = strchr(&ptr2[1], 10);	// Find CR
		}

//		Content-Type: multipart/mixed;
//	boundary="----=_NextPart_000_025B_01CAA004.84449180"
//		7.2.2 The Multipart/mixed (primary) subtype
//		7.2.3 The Multipart/alternative subtype


		if (_memicmp(ptr, "Content-Type: ", 14) == 0)
		{
			char Line[1000] = "";
			char lcLine[1000] = "";

			char * ptr3;

			memcpy(Line, &ptr[14], ptr2-ptr-14);
			memcpy(lcLine, &ptr[14], ptr2-ptr-14);
			_strlwr(lcLine);

			if (_memicmp(Line, "Multipart/", 10) == 0)
			{
				Multipart = TRUE;

	
				ptr3 = strstr(Line, "boundary");

				if (ptr3)
				{
					ptr3+=9;

					if ((*ptr3) == '"')
						ptr3++;

					strcpy(Boundary, ptr3);
					ptr3 = strchr(Boundary, '"');
					if (ptr3) *ptr3 = 0;
					ptr3 = strchr(Boundary, 13);			// CR
					if (ptr3) *ptr3 = 0;

				}
				else
					return;						// Can't do anything without a boundary ??
			}

		}

		ptr = ptr2;
		ptr++;

	}

	if (info == NULL)
		return;		// Wot!

	// Extract the Key/Value pairs from input data

	saveptr = ptr = WebFindPart(Body, Boundary, &Partlen, input + InputLen);

	if (ptr == NULL)
		return;			// Couldn't find separator

	// Now extract fields

	while (ptr)
	{
		char * endptr;
		char * val;
//		Debugprintf(ptr);
	
		// Format seems to be

		//Content-Disposition: form-data; name="FieldName"
		// crlf crlf
		// field value
		// crlf crlf

		// No, is actually

		// ------WebKitFormBoundary7XHZ1i7Jc8tOZJbw
		// Content-Disposition: form-data; name="State"
		//
		// UK
		// ------WebKitFormBoundary7XHZ1i7Jc8tOZJbw

		// ie Value is terminated by ------WebKitFormBoundary7XHZ1i7Jc8tOZJbw
		// But FindPart has returned length, so can use that
		// Be aware that Part and PartLen include the CRLF which is actually part of the Boundary string so should be removed.


		ptr = strstr(ptr, "name=");
	
		if (ptr)
		{
			endptr = strstr(ptr, "\"\r\n\r\n");		// "/r/n/r/n
			if (endptr)
			{
				*endptr = 0;
				ptr += 6;			// to start of name string
				val = endptr + 5;

				// val was Null Terminated by FindPart so can just use it. This assumes all fields are text,
				// which I think is safe enough here.

				saveptr[Partlen - 2] = 0;

				// Now have key value pair

				Keys[NumKeys] = ptr;
				Values[NumKeys] = val;
				saveForfree[NumKeys++] = saveptr;		// so we can free() when finished with it
			}
			else
				free(saveptr);
		}
		else
			free(saveptr);

		saveptr = ptr = WebFindPart(Body, Boundary, &Partlen, input + InputLen);
	}

	if (info == NULL)
		return;		// Wot!

	info += 4;

	// It looks like some standard variables can be used in <var subsitutions as well as fields from form. Put on end, 
	// so fields from form have priority


	saveForfree[NumKeys]  = 0;		// flag end of malloced items

	Keys[NumKeys] = "MsgTo";
	Values[NumKeys++]  = "";
	Keys[NumKeys] = "MsgCc";
	Values[NumKeys++] = "";
	Keys[NumKeys] = "MsgSender";
	Values[NumKeys++] = Session->User->Call;
	Keys[NumKeys] = "MsgSubject";
	Values[NumKeys++] = "";
	Keys[NumKeys] = "MsgBody";
//	if (WebMail->OrigBody)
//		txtKey++->Value = _strdup(WebMail->OrigBody);
//	else
		Values[NumKeys++] = "";

	Keys[NumKeys] = _strdup("MsgP2P");
	Values[NumKeys++] = _strdup("");

	Keys[NumKeys] = _strdup("MsgIsReply");
	if (WebMail->isReply)
		Values[NumKeys++] = "True";
	else
		Values[NumKeys++] = "True";

	Keys[NumKeys] = "MsgIsForward";
	Values[NumKeys++] = "False";
	Keys[NumKeys] = "MsgIsAcknowledgement";
	Values[NumKeys++] = "False";


	// Update Template with variables from the form
	
	// I've going to update the template in situ, as I can't see a better way
	// of making sure all occurances of variables in any order are substituted.
	// The space allocated to Template is twice the size of the file
	// to allow for insertions

	inptr = WebMail->txtFile;

	// Search for "<var>" strings in form and replace with
	// corresponding variable

	// we run through the Template once for each variable

	i = 0;

	while (i < NumKeys)
	{
		char Key[256];
		
		sprintf(Key, "<var %s>", Keys[i]);
		
		inptr = WebMail->txtFile;
		varptr = stristr(inptr, Key);

		while (varptr)
		{
			// Move the remaining message up/down the buffer to make space for substitution

			varlen = (int)strlen(Key);
			vallen = (int)strlen(Values[i]);

			endptr = varptr + varlen;
		
			memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
			memcpy(varptr, Values[i], vallen);

			inptr = endptr + 1;
		
			varptr = stristr(inptr, Key);
		}
		i++;
	}

	// We need to look for To:, CC: and Subject lines, and remove any other
	// Var: lines. Use everything following Msg: as the plain text body
	
	// Find start of Message body

	ptr = WebMail->txtFile;

	ptr1 = strchr(ptr, '\r');
		
	while (ptr1)
	{
		if (_memicmp(ptr, "Msg:", 4) == 0)
		{
			// Rest is message body. <var> substitutions have been done

			if (WebMail->Body)
				free(WebMail->Body);

			WebMail->Body = _strdup(ptr + 4);	// Remove Msg:
			break;
		}

		// Can now terminate lines

		*ptr1++ = 0;

		while (*ptr1 == '\r' || *ptr1 == '\n')
			*ptr1++ = 0;

		if (_memicmp(ptr, "To:", 3) == 0)
		{
			if (strlen(ptr) > 5)
				WebMail->To = _strdup(&ptr[3]);
		}
		else if (_memicmp(ptr, "CC:", 3) == 0)
		{
			if (strlen(ptr) > 5)
				WebMail->CC = _strdup(&ptr[3]);
		}
		else if (_memicmp(ptr, "Subj:", 5) == 0)
		{
			if (ptr[5] == ' ')		// May have space after :
				ptr++;
			if (strlen(ptr) > 6)
				WebMail->Subject = _strdup(&ptr[5]);
		}

		else if (_memicmp(ptr, "Subject:", 8) == 0)
		{
			if (ptr[8] == ' ')
				ptr++;
			if (strlen(ptr) > 9)
				WebMail->Subject = _strdup(&ptr[8]);
		}
		ptr = ptr1;
		ptr1 = strchr(ptr, '\r');
	}

	if (WebMail->Subject == NULL)
		 WebMail->Subject = _strdup("");

	if (WebMail->To == NULL)
		 WebMail->To = _strdup("");

	if (WebMail->CC == NULL)
		 WebMail->CC = _strdup("");

	// Replace var in Subject

	if (_memicmp(WebMail->Subject, "<var ", 5) == 0)
	{
		WebMail->Subject = realloc(WebMail->Subject, 512);		// Plenty of space
		i = 0;

		while (i < NumKeys)
		{
			char Key[256];
		
			sprintf(Key, "<var %s>", Keys[i]);
		
			inptr = WebMail->Subject;
			varptr = stristr(inptr, Key);

			while (varptr)
			{
				// Move the remaining message up/down the buffer to make space for substitution

				varlen = (int)strlen(Key);
				vallen = (int)strlen(Values[i]);

				endptr = varptr + varlen;
		
				memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
				memcpy(varptr, Values[i], vallen);

				inptr = endptr + 1;
		
				varptr = stristr(inptr, Key);
			}
			i++;
		}
	}



	// Build XML Attachment if Display Form is defined

	if (WebMail->DisplayHTMLName)
		BuildXMLAttachment(Session, Keys, Values, NumKeys);

	// if Reply, attach original message to Body;

	if (WebMail->isReply && WebMail->OrigBody)
	{
		char * NewBody = malloc(strlen(WebMail->Body) + strlen(WebMail->OrigBody) + 100);
		sprintf(NewBody, "%s\r\n%s", WebMail->Body, WebMail->OrigBody);
		free(WebMail->Body);
		WebMail->Body = NewBody;
	}

	// Display Message for user to check and send

	// fix any cr cr lf sequence
	
	crcrptr = strstr(WebMail->Body, "\r\r");

	while (crcrptr)
	{
		*crcrptr = ' ';
		crcrptr = strstr(crcrptr, "\r\r");
	}

	if (WebMail->BID == NULL)
		WebMail->BID = _strdup("");

	*RLen = sprintf(Reply, CheckFormMsgPage, Session->Key, WebMail->To, WebMail->CC, WebMail->Subject, "Selected", "", "", WebMail->BID, WebMail->Body);

	// Free the part strings 

	i = 0;

	while (saveForfree[i])
		free(saveForfree[i++]);
}

// XML Template Stuff

char XMLHeader [] = 
	"<?xml version=\"1.0\"?>\r\n"
	"<RMS_Express_Form>\r\n"
	" <form_parameters>\r\n"
	" <xml_file_version>%s</xml_file_version>\r\n"
	" <rms_express_version>%s</rms_express_version>\r\n"
	" <submission_datetime>%s</submission_datetime>\r\n"
	" <senders_callsign>%s</senders_callsign>\r\n"
	" <grid_square>%s</grid_square>\r\n"
	" <display_form>%s</display_form>\r\n"
	" <reply_template>%s</reply_template>\r\n"
	" </form_parameters>\r\n"
	"<variables>\r\n"
	" <msgto>%s</msgto>\r\n"
    " <msgcc>%s</msgcc>\r\n"
    " <msgsender>%s</msgsender>\r\n"
    " <msgsubject>%s</msgsubject>\r\n"
    " <msgbody>%s</msgbody>\r\n"
    " <msgp2p>%s</msgp2p>\r\n"
    " <msgisreply>%s</msgisreply>\r\n"
    " <msgisforward>%s</msgisforward>\r\n"
    " <msgisacknowledgement>%s</msgisacknowledgement>\r\n";


char XMLLine[] = " <%s>%s</%s>\r\n";

char XMLTrailer[] = "</variables>\r\n</RMS_Express_Form>\r\n";

char * doXMLTransparency(char * string)
{
	// Make sure string doesn't contain forbidden XML chars (<>"'&)

	char * newstring = malloc(5 * strlen(string) + 1);		// If len is zero still need null terminator

	char * in = string;
	char * out = newstring;
	char c;

	c = *(in++);

	while (c)
	{
		switch (c)
		{
		case '<':

			strcpy(out, "&lt;");
			out += 4;
			break;

		case '>':

			strcpy(out, "&gt;");
			out += 4;
			break;

		case '"':

			strcpy(out, "&quot;");
			out += 6;
			break;

		case '\'':

			strcpy(out, "&apos;");
			out += 6;
			break;

		case '&':

			strcpy(out, "&amp;");
			out += 5;
			break;

		default:

			*(out++) = c;
		}
		c = *(in++);
	}

	*(out++) = 0;
	return newstring;
}
 

VOID BuildXMLAttachment(struct HTTPConnectionInfo * Session, char * Keys[1000], char * Values[1000], int NumKeys)
{
	// Create XML Attachment for form

	WebMailInfo * WebMail = Session->WebMail;
	char XMLName[MAX_PATH];
	char * XMLPtr;
	char DateString[80];
	struct tm * tm;
	time_t NOW = time(NULL);

	int n;
	int TotalFileSize = 0;
		
	tm = gmtime(&NOW);	
	
	sprintf(DateString, "%04d%02d%02d%02d%02d%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	strcpy(XMLName, WebMail->DisplayHTMLName);
	XMLName[strlen(XMLName) - 5] = 0;	// remove .html

	WebMail->XMLName = malloc(MAX_PATH);
	WebMail->XML = XMLPtr = malloc(100000);
	WebMail->XMLLen = 0;

	sprintf(WebMail->XMLName, "RMS_Express_Form_%s.xml", XMLName);

	XMLPtr += sprintf(XMLPtr, XMLHeader,
		"1,0", VersionString,
		DateString,
		Session->User->Call,
		"", //Grid
		WebMail->DisplayHTMLName,
		WebMail->ReplyHTMLName,
		WebMail->To,
		WebMail->CC,
		Session->User->Call,
		WebMail->OrigSubject,
		"", //	WebMail->OrigBody,
		"False",			// P2P
		WebMail->isReply ? "True": "False",
		"False",			// Forward,
		"False");			// Ack

	// create XML lines for Key/Value Pairs

	for (n = 0; n < NumKeys; n++)
	{
		if (Values[n] == NULL)
			Values[n] = _strdup("");

		XMLPtr += sprintf(XMLPtr, XMLLine, Keys[n], Values[n], Keys[n]);
	}
	if (WebMail->isReply)
	{
		if (WebMail->OrigBody)
		{
			char * goodXML = doXMLTransparency(WebMail->OrigBody);
			XMLPtr += sprintf(XMLPtr, XMLLine, "MsgOriginalBody", goodXML, "MsgOriginalBody");
		}
		else
			XMLPtr += sprintf(XMLPtr, XMLLine, "MsgOriginalBody", "", "MsgOriginalBody");
	}
		
	XMLPtr += sprintf(XMLPtr, "%s", XMLTrailer);
	WebMail->XMLLen = (int)(XMLPtr - WebMail->XML);
}

char * BuildFormMessage(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, char * Keys[1000], char * Values[1000], int NumKeys)
{

	// Create B2 message with template body and xml attachment

	char * NewMsg = malloc(100000);
	char * SaveMsg = NewMsg;
	char * XMLPtr;

	char DateString[80];
	struct tm * tm;

	char * FileName[100];
	int FileLen[100];
	char * FileBody[100];
	int n, Files = 0;
	int TotalFileSize = 0;
	char Type[16] = "Private";

	WebMailInfo * WebMail = Session->WebMail;

	// Create a B2 Message

	tm = gmtime((time_t *)&Msg->datecreated);	
	
	sprintf(DateString, "%04d%02d%02d%02d%02d%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	if (WebMail->DisplayHTMLName)
	{
		char XMLName[MAX_PATH];
		
		strcpy(XMLName, WebMail->DisplayHTMLName);

		XMLName[strlen(XMLName) - 5] = 0;	// remove .html

		FileName[0] = malloc(MAX_PATH);
		FileBody[0] = malloc(100000);
		Files = 1;
		FileLen[0] = 0;

		sprintf(FileName[0], "RMS_Express_Form_%s.xml", XMLName);

		XMLPtr = FileBody[0];

		XMLPtr += sprintf(XMLPtr, XMLHeader,
			"1,0", VersionString,
			DateString,
			Session->User->Call,
			"", //Grid
			WebMail->DisplayHTMLName,
			WebMail->ReplyHTMLName,
			WebMail->OrigTo,
			"",		// CC
			Session->User->Call,
			WebMail->OrigSubject,
			WebMail->OrigBody,
			"false",		// P2P,
			"false",		//Reply
			"false",		//Forward,
			"false");			// Ack

		// create XML lines for Key/Value Pairs

		for (n = 0; n < NumKeys; n++)
		{
			if (Values[n] == NULL)
				Values[n] = _strdup("");

			XMLPtr += sprintf(XMLPtr, XMLLine, Keys[n], Values[n], Keys[n]);
		}
		XMLPtr += sprintf(XMLPtr, "%s", XMLTrailer);

		FileLen[0] = (int)(XMLPtr - FileBody[0]);

	}

	sprintf(DateString, "%04d/%02d/%02d %02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	
	// Get Type
	
	if (Msg->type == 'B')
		strcpy(Type, "Bulletin");
	else if (Msg->type == 'T')
		strcpy(Type, "NTS");

	// We put original To call in B2 Header

	NewMsg += sprintf(NewMsg,
		"MID: %s\r\nDate: %s\r\nType: %s\r\nFrom: %s\r\nTo: %s\r\nSubject: %s\r\nMbo: %s\r\n",
			Msg->bid, DateString, Type, Msg->from, WebMail->To, Msg->title, BBSName);
				

	NewMsg += sprintf(NewMsg, "Body: %d\r\n", (int)strlen(WebMail->Body));

	for (n = 0; n < Files; n++)
	{
		char * p = FileName[n], * q;

		// Remove any path

		q = strchr(p, '\\');
					
		while (q)
		{
			if (q)
				*q++ = 0;
			p = q;
			q = strchr(p, '\\');
		}

		NewMsg += sprintf(NewMsg, "File: %d %s\r\n", FileLen[n], p);
	}

	NewMsg += sprintf(NewMsg, "\r\n");
	strcpy(NewMsg, WebMail->Body);
	NewMsg += strlen(WebMail->Body);
	NewMsg += sprintf(NewMsg, "\r\n");

	for (n = 0; n < Files; n++)
	{
		memcpy(NewMsg, FileBody[n], FileLen[n]);
		NewMsg += FileLen[n];
		free(FileName[n]);
		free(FileBody[n]);
		NewMsg += sprintf(NewMsg, "\r\n");
	}

	Msg->length = (int)strlen(SaveMsg);
	Msg->B2Flags = B2Msg;
	
	if (Files)
		Msg->B2Flags |= Attachments;

	if (WebMail->Body)
		free(WebMail->Body);

	WebMail->Body = SaveMsg;

	return NULL;
}

VOID UpdateFormAction(char * Template, char * Key)
{
	char * inptr, * saveptr;
	char * varptr, * endptr;
	size_t varlen, vallen;
	char Submit[64];

	sprintf(Submit, "/Webmail/Submit?%s", Key);

	// First find the Form Action string and replace with our URL. It should have
	// action="http://{FormServer}:{FormPort}" but some forms have localhost:8001 instead

	// Also remove the OnSubmit if it contains the standard popup about having to close browser

	inptr = Template;
	saveptr = varptr = stristr(inptr, "<Form ");

	if (varptr == NULL)
		return;

	varptr = stristr(inptr, " Action=");
	if (varptr)
	{
		char delim = ' ';

		varptr += 8;		// to first char.

		if (*varptr == '\"' || *varptr == '\'')
			delim = *varptr++;

		endptr = strchr(varptr, delim);

		// Move the remaining message up/down the buffer to make space for substitution

		varlen = endptr - varptr;
		vallen = strlen(Submit);

		endptr = varptr + varlen;
		
		memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
		memcpy(varptr, Submit, vallen);
	}
		
	varptr = saveptr;

	saveptr = varptr = stristr(inptr, " onsubmit=");
		
	if (varptr)
	{
		char delim = ' ';

		varptr += 10;		// to first char.

		if (*varptr == '\"' || *varptr == '\'')
			delim = *varptr++;

		endptr = strchr(varptr, delim);

		// We want to remove onsubmit and delimiter 

		varptr = saveptr;
		endptr++;

		// Move the remaining message up/down the buffer to make space for substitution

		varlen = endptr - varptr;
		vallen = 0;

		endptr = varptr + varlen;
		
		memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
		memcpy(varptr, Submit, vallen);
	}

	// Some forms seem to have file://C:/RMS%20Express/Standard%20Templates/ instead of {FormFolder}

	varptr = saveptr;

	saveptr = varptr = stristr(inptr, "file://C:/RMS");
		
	if (varptr)
	{
		// We need the last part of the name

		char * start = varptr;
		char * endptr = strchr(varptr, '"');

		if (endptr)
			*(endptr) = 0;			// Null terminate for strlop

		while (strchr(varptr, '/'))
			varptr = strlop(varptr, '/');

		*(endptr) = '"';			// Put Back

		// We want to remove onsubmit and delimiter 

		varlen =  varptr - saveptr;

		endptr++;

		// Move the remaining message up/down the buffer to make space for substitution

		endptr = varptr + varlen;
		
		memmove(saveptr + 12, varptr, strlen(varptr) + varlen);		// copy null on end
		memcpy(saveptr, "{FormFolder}", 12);
	}
}


int ReplyToFormsMessage(struct HTTPConnectionInfo * Session, struct MsgInfo * Msg, char * Reply, BOOL Reenter)
{
	WebMailInfo * WebMail = Session->WebMail;
	KeyValues * txtKey = WebMail->XMLKeys;
	int Len, i;
	char * inptr, * ptr;
	char * varptr, * endptr;
	int varlen, vallen;
	struct HtmlFormDir * Dir;

	char * Template = FindXMLVariable(WebMail, "reply_template");

	if (Template == NULL || Template[0] == 0)

		return 0;					// No Template

	if (Reenter)
		goto reEnter;

	WebMail->isReply = TRUE;

	if (WebMail->OrigBody)
		free(WebMail->OrigBody);

	WebMail->OrigBody = _strdup(WebMail->Body);
	
	// Add "Re: " to Subject

	ptr = WebMail->Subject;

	WebMail->Subject = malloc(strlen(Msg->title) + 10);
	sprintf(WebMail->Subject, "Re: %s", Msg->title);

	// Set To: from From:

	WebMail->To = malloc(80);

	if (Msg->emailfrom[0])
		sprintf(WebMail->To, "%s%s", Msg->from, Msg->emailfrom);
	else
		sprintf(WebMail->To, "%s", Msg->from);


	
	if (ptr)
		free(ptr);

	WebMail->txtFileName = _strdup(Template);

	// Read the .txt file

	if (WebMail->txtFile)
		free(WebMail->txtFile);

	for (i = 0; i < FormDirCount; i++)
	{
		int n;
			
		Dir = HtmlFormDirs[i];

		WebMail->txtFile = CheckFile(Dir, WebMail->txtFileName);
		if (WebMail->txtFile)
			goto gotFile;
			
		// Recurse any Subdirs
			
		n = 0;
		while (n < Dir->DirCount)
		{
			WebMail->txtFile = CheckFile(Dir->Dirs[n], WebMail->txtFileName);
			if (WebMail->txtFile)
			{
				Dir = Dir->Dirs[n];
				goto gotFile;
			}
			n++;
		}
	}

	// Template Not Found

	// Missing template

		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Reply Template %s missing. Display as Text then Reply\");;window.history.back();</script></html>", WebMail->txtFileName);
		return *WebMail->RLen;


gotFile:
	WebMail->Dir = Dir;
reEnter:


	if (ParsetxtTemplate(Session, WebMail->Dir, WebMail->txtFileName, TRUE) == FALSE)
		return *WebMail->RLen;			// processing <select> or <ask>

	if (WebMail->InputHTMLName == NULL)
	{
		// This is a plain text template without HTML 
/*
		if (To == NULL)
			To = "";

		if (To[0] == 0 && WebMail->To && WebMail->To[0])
			To = WebMail->To;

		if (CC == NULL)
			CC = "";

		if (CC[0] == 0 && WebMail->CC && WebMail->CC[0])
			CC = WebMail->CC;

		if (Subject == NULL)
			Subject = "";

		if (Subject[0] == 0 && WebMail->Subject && WebMail->Subject[0])
			Subject = WebMail->Subject;
	
		if (MsgBody == NULL)
			MsgBody = "";

		if (MsgBody[0] == 0 && WebMail->Body && WebMail->Body[0])
			MsgBody = WebMail->Body;

		*WebMail->RLen = sprintf(WebMail->Reply, CheckFormMsgPage, Session->Key, To, CC, Subject, MsgBody);
	*/	
		return *WebMail->RLen;
	}

	Template = CheckFile(WebMail->Dir, WebMail->InputHTMLName);

	if (Template == NULL)
	{
			// Missing HTML

		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Reply HTML %s missing. Display as Text then Reply\");;window.history.back();</script></html>", WebMail->InputHTMLName);
		return *WebMail->RLen;
	}

	// I've going to update the template in situ, as I can't see a better way
	// of making sure all occurances of variables in any order are substituted.
	// The space allocated to Template is twice the size of the file
	// to allow for insertions

	UpdateFormAction(Template, Session->Key);		// Update "Submit" Action

	// Search for "{var }" strings in form and replace with
	// corresponding variable from XML

	while (txtKey->Key)
	{
		char Key[256] = "{var ";
		
		strcpy(&Key[5], txtKey->Key);
		strcat(Key, "}");

		inptr = Template;
		varptr = stristr(inptr, Key);

		while (varptr)
		{
			// Move the remaining message up/down the buffer to make space for substitution

			varlen = (int)strlen(Key);
			if (txtKey->Value)
				vallen = (int)strlen(txtKey->Value);
			else vallen = 0;

			endptr = varptr + varlen;
		
			memmove(varptr + vallen, endptr, strlen(endptr) + 1);		// copy null on end
			memcpy(varptr, txtKey->Value, vallen);

			inptr = endptr + 1;
		
			varptr = stristr(inptr, Key);
		}
		txtKey++;
	}

	// Remove </body></html> from end as we add it on later

	ptr = stristr(Template, "</body></html>");
		
	if (ptr)
		*ptr = 0;
		
	Len = sprintf(Reply, "%s", Template);
	free(Template);	
	return Len;
}

char * CheckFile(struct HtmlFormDir * Dir, char * FN)
{
	struct stat STAT;
	FILE * hFile;
	char MsgFile[MAX_PATH];
	char * MsgBytes;
	int ReadLen;
	int FileSize;

#ifndef WIN32

	// Need to do case insensitive file search

	DIR *dir;
	struct dirent *entry;
	char name[256];

	sprintf(name, "%s/%s/%s", BPQDirectory, Dir->FormSet, Dir->DirName);

	if (!(dir = opendir(name)))
	{
		Debugprintf("cant open forms dir %s %s %d", Dir->DirName, name, errno);
        return 0;
	}

    while ((entry = readdir(dir)) != NULL)
	{
        if (entry->d_type == DT_DIR)
                continue;
	
		if (stricmp(entry->d_name, FN) == 0)
		{
			sprintf(MsgFile, "%s/%s/%s/%s", GetBPQDirectory(), Dir->FormSet, Dir->DirName, entry->d_name);
			break;
		}
	}
    closedir(dir);

#else

	sprintf(MsgFile, "%s/%s/%s/%s", GetBPQDirectory(), Dir->FormSet, Dir->DirName, FN);

#endif

	if (stat(MsgFile, &STAT) != -1)
	{
		hFile = fopen(MsgFile, "rb");
	
		if (hFile == 0)
		{
			MsgBytes = _strdup("File is missing");
			return MsgBytes;
		}

		FileSize = STAT.st_size;
		MsgBytes = malloc(FileSize * 10);		// Allow plenty of room for template substitution
		ReadLen = (int)fread(MsgBytes, 1, FileSize, hFile);
		MsgBytes[FileSize] = 0;
		fclose(hFile);

		return MsgBytes;
	}
	return NULL;
}

BOOL DoSelectPrompt(struct HTTPConnectionInfo * Session, char * Select)
{
	// Send a Popup window to select value. Reply handling code will update template then reenter ParsetxtTemplate

	char popuphddr[] = 
			
		"<html><body align=center background='/background.jpg'>"
		"<script>"
		"function myFunction() {var x = document.getElementById('Sel').value;"
		"var Key = \"%s\";"
		"var param = \"toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=no,resizable=no,titlebar=no,toobar=no\";"
		"window.open(\"/WebMail/DoSelect\" + '?' + Key + '&' + x,'_self');"
		"}"
		"</script>"
		"<div align=center>%s<br><br>"
		"<table border=1 cellpadding=2 bgcolor=white>"
		"<tr><td><select size=%d id='Sel' size=10 onclick=myFunction()>";
	

	char popup[10000];
	int i, vars = 0;
	char * ptr, * ptr1;
	char * prompt;
	char * var[100];
	int len;

	WebMailInfo * WebMail = Session->WebMail;
		
	char * SelCopy = _strdup(Select + 8);		// Skip "<select "

	ptr = strchr(SelCopy, '>');
	
	if (ptr)
		*ptr = 0;

	ptr = SelCopy;

	if (*ptr == '"')
	{
		// String has " " round it

		ptr++;

		ptr1 = strchr(ptr, '"');
		if (ptr1 == NULL)
			goto returnDuff;
	
		*(ptr1++) = 0;
		prompt = ptr;
	}
	else
	{
		// Normal comma terminated

		ptr1 = strchr(ptr, ',');
		if (ptr1 == NULL)
			goto returnDuff;
	
		*(ptr1++) = 0;
		prompt = ptr;
	}

	ptr = ptr1;

	while (ptr && ptr[0])
	{
		if (*ptr == '"')
		{
			// String has " " round it

			ptr++;

			ptr1 = strchr(ptr, '"');
			if (ptr1 == NULL)
				goto returnDuff;
	
			*(ptr1++) = 0;
			while(ptr1 && *ptr1 == ',')
				ptr1++;
		}
		else
		{
			// Normal comma terminated

			ptr1 = strchr(ptr, ',');
			if (ptr1)	
				*(ptr1++) = 0;
		}

		var[vars++] = ptr;
		
		ptr = ptr1;
	}

	len = sprintf(popup, popuphddr, Session->Key, prompt, vars + 1);

	for (i = 0; i < vars; i++)
	{
		char * key = strlop(var[i], '=');

		if (key == NULL)
			key = var[i];

		len += sprintf(&popup[len], " <option value='%s'>%s", key, var[i]);
	}
	len += sprintf(&popup[len], "%s</select></td></tr></table><br><input onclick=window.history.back() value=Back type=button class='btn'></div>");

	*WebMail->RLen = sprintf(WebMail->Reply, "%s", popup);
	free(SelCopy);
	return TRUE;

returnDuff:
	*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Template <select> Corrupt.\");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
	free(SelCopy);
	return TRUE;

}

BOOL DoAskPrompt(struct HTTPConnectionInfo * Session, char * Select)
{
	return TRUE;
}

VOID ProcessSelectResponse(struct HTTPConnectionInfo * Session, char * URLParams)
{
	// User has entered a response for a Template <Select>. Update Template and re-renter ParsetxtTemplate

	WebMailInfo * WebMail = Session->WebMail;
	
	char * valptr, * varptr, * endptr;
	size_t varlen, vallen;

	char * Select = WebMail->txtFile;

	if (Select == 0)
	{
		// Missing template

		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Template missing. Was back Button used? \");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
		return;
	}

	Select = stristr(Select, "<Select ");

	if (Select == 0)
	{
		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Template Corrupt.\");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
		return;
	}

	varptr = Select;
	endptr = strchr(Select, '>');

	if (endptr == NULL)
	{
		*WebMail->RLen = sprintf(WebMail->Reply, "<html><script>alert(\"Template Corrupt.\");window.location.href = '/Webmail/WebMail?%s';</script></html>", Session->Key);
		return;
	}

	*endptr = 0;
	varlen = endptr - varptr;


	valptr = URLParams;

	// Move the remaining message up/down the buffer to make space for substitution

	vallen = strlen(valptr);

	endptr = varptr + varlen;
		
	memcpy(varptr, valptr, vallen);
	memmove(varptr + vallen, endptr + 1, strlen(endptr + 1) + 1);		// copy null on end

	if (WebMail->isReply)
		*WebMail->RLen = ReplyToFormsMessage(Session, Session->Msg, WebMail->Reply, TRUE);
	else
		GetPage(Session, NULL);

	return ;
}

BOOL ParsetxtTemplate(struct HTTPConnectionInfo * Session, struct HtmlFormDir * Dir, char * FN, BOOL isReply)
{
	WebMailInfo * WebMail = Session->WebMail;
	KeyValues * txtKey = WebMail->txtKeys;

	char * MsgBytes;

	char * txtFile;
	char * ptr, *ptr1;
	char * InputName = NULL;	// HTML to input message
	char * ReplyName = NULL;
	char * To = NULL;
	char * Subject = NULL;
	char * MsgBody = NULL;
	char * Select = NULL;
	char * Ask = NULL;

	char Date[16];
	char UDate[16];
	char DateTime[32];
	char UDateTime[32];
	char Day[16];
	char UDay[16];
	char UDTG[32];
	char Seq[16];
	char FormDir[MAX_PATH];
	double Lat;
	double Lon;
	char LatString[32], LonString[32], GPSString[32];
	BOOL GPSOK;
	
	struct tm * tm;
	time_t NOW;

	// Template is now read before entering here

	MsgBytes = WebMail->txtFile;

	// if Template uses <Select> or <Ask> get the values

	Select = stristr(MsgBytes, "<Select ");
	Ask = stristr(MsgBytes, "<Ask ");

	if (Select && Ask)		
	{
		// use whichever came first

		if (Select < Ask)
		{
			DoSelectPrompt(Session, Select);
			return FALSE;
		}
		else 
		{
			DoAskPrompt(Session, Ask);
			return FALSE;
		}
	}

	if (Select)
	{
		DoSelectPrompt(Session, Select);
		return FALSE;
	}

	if (Ask)
	{
		DoAskPrompt(Session, Ask);
		return FALSE;
	}

	NOW = time(NULL);
	tm = localtime(&NOW);

	sprintf(Date, "%04d-%02d-%02d",
		tm->tm_year + 1900,tm->tm_mon + 1, tm->tm_mday);
	
	sprintf(DateTime, "%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900,tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	strcpy(Day, longday[tm->tm_wday]);

	tm = gmtime(&NOW);				

	sprintf(UDate, "%04d-%02d-%02dZ",
		tm->tm_year + 1900,tm->tm_mon + 1, tm->tm_mday);

	sprintf(UDateTime, "%04d-%02d-%02d %02d:%02d:%02dZ",
		tm->tm_year + 100,tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	sprintf(UDTG, "%02d%02d%02dZ %s %04d",
		tm->tm_mday, tm->tm_hour, tm->tm_min, month[tm->tm_mon], tm->tm_year + 1900);

	strcpy(UDay, longday[tm->tm_wday]);

	sprintf(Seq, "%d", Session->User->WebSeqNo);
	sprintf(FormDir, "/WebMail/WMFile/%s/%s/", WebMail->Dir->FormSet, WebMail->Dir->DirName);	

	// Keep SeqNo at front
	
	txtKey->Key = _strdup("<SeqNum>");
	txtKey++->Value = _strdup(Seq);

	txtKey->Key = _strdup("<DateTime>");
	txtKey++->Value = _strdup(DateTime);
	txtKey->Key = _strdup("<UDateTime>");
	txtKey++->Value = _strdup(UDateTime);
	txtKey->Key = _strdup("<Date>");
	txtKey++->Value = _strdup(Date);
	txtKey->Key = _strdup("<UDate>");
	txtKey++->Value = _strdup(UDate);
	txtKey->Key = _strdup("<Time>");
	txtKey++->Value = _strdup(&DateTime[11]);
	txtKey->Key = _strdup("<UTime>");
	txtKey++->Value = _strdup(&UDateTime[11]);
	txtKey->Key = _strdup("<Day>");
	txtKey++->Value = _strdup(Day);
	txtKey->Key = _strdup("<UDay>");
	txtKey++->Value = _strdup(UDay);
	txtKey->Key = _strdup("<UDTG>");
	txtKey++->Value = _strdup(UDTG);

	// Try to get position from APRS

	GPSOK = GetAPRSLatLon(&Lat, &Lon);
	GPSOK = GetAPRSLatLonString(&LatString[1], &LonString[1]);
	memmove(LatString, &LatString[1], 2);
	memmove(LonString, &LonString[1], 3);
	LatString[2] = '-';
	LonString[3] = '-';
	sprintf(GPSString,"%s %s", LatString, LonString);

	txtKey->Key = _strdup("<GPS>");
	if (GPSOK)
		txtKey++->Value = _strdup(GPSString);
	else
		txtKey++->Value = _strdup("");
	
	txtKey->Key = _strdup("<Position>");
	txtKey++->Value = _strdup(GPSString);
	txtKey->Key = _strdup("<ProgramVersion>");
	txtKey++->Value = _strdup(VersionString);
	txtKey->Key = _strdup("<Callsign>");
	txtKey++->Value = _strdup(Session->User->Call);
	txtKey->Key = _strdup("<MsgTo>");
	txtKey++->Value = _strdup("");
	txtKey->Key = _strdup("<MsgCc>");
	txtKey++->Value = _strdup("");
	txtKey->Key = _strdup("<MsgSender>");
	txtKey++->Value = _strdup(Session->User->Call);
	txtKey->Key = _strdup("<MsgSubject>");
	txtKey++->Value = _strdup("");
	txtKey->Key = _strdup("<MsgBody>");
//	if (WebMail->OrigBody)
//		txtKey++->Value = _strdup(WebMail->OrigBody);
//	else
		txtKey++->Value = _strdup("");

	txtKey->Key = _strdup("<MsgP2P>");
	txtKey++->Value = _strdup("");

	if (isReply)
	{
		txtKey->Key = _strdup("<MsgIsReply>");
		txtKey++->Value = _strdup("True");
		txtKey->Key = _strdup("<MsgIsForward>");
		txtKey++->Value = _strdup("False");
		txtKey->Key = _strdup("<MsgIsAcknowledgement>");
		txtKey++->Value = _strdup("False");
		txtKey->Key = _strdup("<MsgOriginalSubject>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalSender>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalBody>");
		txtKey++->Value = _strdup(WebMail->Body);
		txtKey->Key = _strdup("<MsgOriginalID>");
		txtKey++->Value = _strdup(WebMail->Msg->bid);

		// Get Timestamp from Message

		tm = gmtime((time_t *)&WebMail->Msg->datecreated);				

		sprintf(Date, "%02d-%02d-%02d",
			tm->tm_year - 100,tm->tm_mon + 1, tm->tm_mday);
	
		sprintf(DateTime, "%02d-%02d-%02d %02d:%02d",
			tm->tm_year - 100,tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
	
		strcpy(Day, longday[tm->tm_wday]);
			tm = gmtime((time_t *)&WebMail->Msg->datecreated);				

		sprintf(UDate, "%02d-%02d-%02dZ",
			tm->tm_year - 100,tm->tm_mon + 1, tm->tm_mday);

		sprintf(UDateTime, "%02d-%02d-%02d %02d:%02dZ",
			tm->tm_year - 100,tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);

		sprintf(UDTG, "%02d%02d%02dZ %s %04d",
			tm->tm_mday, tm->tm_hour, tm->tm_min, month[tm->tm_mon], tm->tm_year + 1900);

		txtKey->Key = _strdup("<MsgOriginalDate>");
		txtKey++->Value = _strdup(UDate);
		txtKey->Key = _strdup("<MsgOriginalUtcDate>");
		txtKey++->Value = _strdup(UDate);
		txtKey->Key = _strdup("<MsgOriginalUtcTime>");
		txtKey++->Value = _strdup(&UDateTime[9]);
		txtKey->Key = _strdup("<MsgOriginalLocalDate>");
		txtKey++->Value = _strdup(Date);
		txtKey->Key = _strdup("<MsgOriginalLocalTime>");
		txtKey++->Value = _strdup(&UDateTime[9]);
		txtKey->Key = _strdup("<MsgOriginalDTG>");
		txtKey++->Value = _strdup(UDTG);
		txtKey->Key = _strdup("<MsgOriginalSize>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalAttachmentCount>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalXML>");
		txtKey++->Value = _strdup("");
	}
	else
	{
		txtKey->Key = _strdup("<MsgIsReply>");
		txtKey++->Value = _strdup("False");
		txtKey->Key = _strdup("<MsgIsForward>");
		txtKey++->Value = _strdup("False");
		txtKey->Key = _strdup("<MsgIsAcknowledgement>");
		txtKey++->Value = _strdup("False");
		txtKey->Key = _strdup("<MsgOriginalSubject>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalSender>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalBody>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalID>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalDate>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalUtcDate>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalUtcTime>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalLocalDate>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalLocalTime>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalDTG>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalSize>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalAttachmentCount>");
		txtKey++->Value = _strdup("");
		txtKey->Key = _strdup("<MsgOriginalXML>");
		txtKey++->Value = _strdup("");
	}

	txtKey->Key = _strdup("<FormFolder>");
	txtKey++->Value = _strdup(FormDir);		//Form Folder

	// Do standard Variable substitution on file

	DoStandardTemplateSubsitutions(Session, WebMail->txtFile);

	txtFile = _strdup(WebMail->txtFile);		// We chop up and modify bits of txtFile, so need copy

	// Scan template line by line extracting useful information

	ptr = txtFile;
	ptr1 = strchr(ptr, '\r');
		
	while (ptr1)
	{
		if (_memicmp(ptr, "Msg:", 4) == 0)
		{
			// Rest is message body. May need <var> substitutions

			if (WebMail->Body)
				free(WebMail->Body);

			WebMail->Body = _strdup(ptr + 4);
			break;
		}

		// Can now terminate lines

		*ptr1++ = 0;

		while (*ptr1 == '\r' || *ptr1 == '\n')
			*ptr1++ = 0;

		if (_memicmp(ptr, "Form:", 5) == 0)
		{
			InputName = &ptr[5];
	
			while (*InputName == ' ')		// Remove leading spaces
				InputName++;

			WebMail->InputHTMLName = _strdup(InputName);
			WebMail->DisplayHTMLName = strlop(WebMail->InputHTMLName, ',');

			if (WebMail->DisplayHTMLName)
			{
				while (*WebMail->DisplayHTMLName == ' ')		// Remove leading spaces
				WebMail->DisplayHTMLName++;
				WebMail->DisplayHTMLName = _strdup(WebMail->DisplayHTMLName);
			}
		}
		else if (_memicmp(ptr, "ReplyTemplate:",14) == 0)
		{
			char * end;

			ReplyName = &ptr[14];
	
			while (*ReplyName == ' ')
				ReplyName++;
			
			strlop(ReplyName, '\r');		// Terminate

			// Filename may have embedded spaces, so have to scan from end, not use strlop

			end = ReplyName + strlen(ReplyName) - 1;

			while (*end == ' ')
				*end-- = 0;

			WebMail->ReplyHTMLName = _strdup(ReplyName);
		}
		else if (_memicmp(ptr, "To:", 3) == 0)
		{
			if (strlen(ptr) > 5)
				WebMail->To = _strdup(&ptr[3]);
		}
		else if (_memicmp(ptr, "Subj:", 5) == 0)
		{
			if (ptr[5] == ' ')		// May have space after :
				ptr++;
			if (strlen(ptr) > 6)
				WebMail->Subject = _strdup(&ptr[5]);
		}

		else if (_memicmp(ptr, "Subject:", 8) == 0)
		{
			if (ptr[8] == ' ')
				ptr++;
			if (strlen(ptr) > 9)
				WebMail->Subject = _strdup(&ptr[8]);
		}
		else if (_memicmp(ptr, "Def:", 4) == 0)
		{
			// Def: MsgOriginalBody=<var MsgOriginalBody> 

			char * val = strlop(ptr, '=');

			if (val)
			{
				// Make Room for {} delimiters

				memmove(ptr, ptr + 1, strlen(ptr)); 
				ptr[3] = '<';
				ptr[strlen(ptr)] = '>';
				
				while (val[strlen(val) - 1] == ' ')
					val[strlen(val) - 1]  = 0;

				txtKey->Key = _strdup(&ptr[3]);
				txtKey++->Value = _strdup(val);		//Form Folder
			}
		}
		else if (_memicmp(ptr, "Type:", 5) == 0)
		{
			if (stristr(ptr, "Winlink"))
				WebMail->Winlink = TRUE;
			else if (stristr(ptr, "P2P"))
				WebMail->P2P = TRUE;
			else if (stristr(ptr, "Packet"))
				WebMail->Packet = TRUE;
		}
		else if (_memicmp(ptr, "SeqInc:", 7) == 0)
		{
			int SeqInc = 1;
			
			while (ptr[7] == ' ')
				ptr++;

			if (strlen(&ptr[7]))
				SeqInc = atoi(&ptr[7]);
	
			Session->User->WebSeqNo += SeqInc;
			sprintf(WebMail->txtKeys[0].Value, "%d", Session->User->WebSeqNo);
		}

		else if (_memicmp(ptr, "SeqSet:", 7) == 0)
		{
			int SeqSet = 0;
			
			while (ptr[7] == ' ')
				ptr++;

			if (strlen(&ptr[7]))
				SeqSet = atoi(&ptr[7]);
	
			Session->User->WebSeqNo = SeqSet;
			sprintf(WebMail->txtKeys[0].Value, "%d", Session->User->WebSeqNo);
		}

		// Attach:file1, file2, ... - If present, specifies one or more files to attach to the message.
		//Readonly: Yes | No - If Readonly is set to Yes, then the message is created by the form and it cannot be edited by the user.

		ptr = ptr1;
		ptr1 = strchr(ptr, '\r');
	}

	if (WebMail->ReplyHTMLName == NULL)
		WebMail->ReplyHTMLName = _strdup("");

	free(txtFile);

	return TRUE;
}

VOID FormatTime2(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

}

VOID DownloadAttachments(struct HTTPConnectionInfo * Session, char * Reply, int * RLen, char * Param)
{
	WebMailInfo * WebMail = Session->WebMail;
	char TimeString[64];
	char FileTimeString[64];
	int file = atoi(Param);

	file--;			// Sent at +1 in case no downloadable attachments

	if (file == -1)
	{
		// User has gone back, then selected "No file Selected"
		// Or no files

		*RLen = sprintf(Reply, "<html><script>window.history.back();</script></html>");
		return;
	}

	FormatTime2(FileTimeString, time(NULL));
	FormatTime2(TimeString, time(NULL));

	*RLen =	sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Disposition: attachment; filename=\"%s\"\r\n"
		"Date: %s\r\n"
		"Last-Modified: %s\r\n" 
		"\r\n", WebMail->FileLen[file], WebMail->FileName[file], TimeString, FileTimeString);

	memcpy(&Reply[*RLen], WebMail->FileBody[file], WebMail->FileLen[file]); 

	*RLen += WebMail->FileLen[file];
	return;
}

VOID getAttachmentList(struct HTTPConnectionInfo * Session, char * Reply, int * RLen, char * Rest)
{
	char popuphddr[] = 
			
		"<html><body align=center background='/background.jpg'>"
		"<script>"
		"function myFunction() {var x = document.getElementById('Sel').value;"
		"var Key = \"%s\";"
		"var param = \"toolbar=yes,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes\";"
		"window.open(\"/WebMail/GetDownLoad\" + '?' + Key + '&' + x,'_self', param);"
		"}"
		"</script>"
		"<div align=center>Note files over 100K long can't be downloaded<br><br>"
		"<table border=1 cellpadding=2 bgcolor=white>"
		"<tr><td><select size=%d id='Sel' size=10 onclick=myFunction()>";

	char popup[10000];
	int i;
	WebMailInfo * WebMail = Session->WebMail;
	int len;

	len = sprintf(popup, popuphddr, Session->Key, WebMail->Files);

	for (i = 0; i < WebMail->Files; i++)
	{
		if(WebMail->FileLen[i] < 100000)
			len += sprintf(&popup[len], " <option value=%d>%s (Len %d)", i + 1, WebMail->FileName[i], WebMail->FileLen[i]);
	}

	len += sprintf(&popup[len], "%</select></td></tr></table><br><input onclick=window.history.back() value=Back type=button class='btn'></div>");

	*RLen = sprintf(Reply, "%s", popup);
	return;
}


char * WebFindPart(char ** Msg, char * Boundary, int * PartLen, char * End)
{
	char * ptr = *Msg;
	char * Msgptr = *Msg;
	size_t BLen = strlen(Boundary);
	char * Part;

	while(ptr < End)				// Just in case we run off end
	{
		if (*ptr == '-' && *(ptr+1) == '-')
		{
			if (memcmp(&ptr[2], Boundary, BLen) == 0)
			{
				// Found Boundary

				size_t Partlen = ptr - Msgptr;
				Part = malloc(Partlen + 1);
				memcpy(Part, Msgptr, Partlen);
				Part[Partlen] = 0;

				*Msg = ptr + BLen + 4;
		
				*PartLen = (int)Partlen;

				return Part; 
			}
		}

		ptr ++;
	}
	return NULL;
}


int ProcessWebmailWebSock(char * MsgPtr, char * OutBuffer)
{
	int Len = 129;
	char * Key = strlop(MsgPtr, '&');

	struct HTTPConnectionInfo * Session;
	struct UserInfo * User;
	int m;
	struct MsgInfo * Msg;
	char * ptr = &OutBuffer[10];			// allow room for full payload length (64 bit)

	int n = NumberofMessages;
	char Via[64];
	int Count = 0;

	if (Key == 0)
		return 0;

	Session = FindWMSession(Key);

	if (Session == NULL)
		return 0;

	User = Session->User;

	// Outbuffer is 250000

//	ptr += sprintf(ptr, "<div align=left id=\"main\" style=\"overflow:scroll;\">\r\n"
	ptr += sprintf(ptr, "<pre align=left>");

	ptr += sprintf(ptr, "%s", "     #  Date  XX   Len To      @       From    Subject\r\n\r\n");

	for (m = LatestMsg; m >= 1; m--)
	{
		if (ptr > &OutBuffer[244000])
			break;						// protect buffer

		Msg = GetMsgFromNumber(m);

		if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
			continue;					// Protect against corrupt messages
		
		if (Msg && CheckUserMsg(Msg, User->Call, User->flags & F_SYSOP))
		{
			char UTF8Title[4096];
			char  * EncodedTitle;
			
			// List if it is the right type and in the page range we want

			if (Session->WebMailTypes[0] && strchr(Session->WebMailTypes, Msg->type) == 0) 
				continue;

			// All Types or right Type. Check Mine Flag

			if (Session->WebMailMine)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->to) != 0 && strcmp(User->Call, Msg->from) != 0)
					continue;
			}

			if (Session->WebMailMyTX)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->from) != 0)
					continue;
			}

			if (Session->WebMailMyRX)
			{
				// Only list if to or from me

				if (strcmp(User->Call, Msg->to) != 0)
					continue;
			}
			if (Count++ < Session->WebMailSkip)
				continue;

			strcpy(Via, Msg->via);
			strlop(Via, '.');

			// make sure title is HTML safe (no < > etc) and UTF 8 encoded

			EncodedTitle = doXMLTransparency(Msg->title);

			memset(UTF8Title, 0, 4096);		// In case convert fails part way through
			ConvertTitletoUTF8(Session->WebMail, EncodedTitle, UTF8Title, 4095);

			free(EncodedTitle);

			ptr += sprintf(ptr, "<a href=/WebMail/WM?%s&%d>%6d</a> %s %c%c %5d %-8s%-8s%-8s%s\r\n",
				Key, Msg->number, Msg->number,
				FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type,
				Msg->status, Msg->length, Msg->to, Via,
				Msg->from, UTF8Title);

			n--;

			if (n == 0)
				break;
		}
	}

	ptr += sprintf(&ptr[strlen(ptr)], "</pre> \r\n");

	Len = ptr - &OutBuffer[10];

	OutBuffer[0] = 0x81;		// Fin, Data

	if (Len < 126)
	{
		OutBuffer[1] = Len;
		memmove(&OutBuffer[2], &OutBuffer[10], Len);
		return Len + 2;
	}
	else if (Len < 65536)
	{
		OutBuffer[1] = 126;			// Unmasked, Extended Len 16
		OutBuffer[2] = Len >> 8;
		OutBuffer[3] = Len & 0xff;
		memmove(&OutBuffer[4], &OutBuffer[10], Len);
		return Len + 4;
	}
	else
	{
		OutBuffer[1] = 127;			// Unmasked, Extended Len 64 bits
		// Len is 32 bits, so pad with zeros
		OutBuffer[2] = 0;
		OutBuffer[3] = 0;
		OutBuffer[4] = 0;
		OutBuffer[5] = 0;
		OutBuffer[6] = (Len >> 24) & 0xff;
		OutBuffer[7] = (Len >> 16) & 0xff;
		OutBuffer[8] = (Len >> 8) & 0xff;
		OutBuffer[9] = Len & 0xff;

		return Len + 10;
	}
}




