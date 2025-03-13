// Mail and Chat Server for BPQ32 Packet Switch
//
//	Utility Routines



#include "BPQChat.h"

#include "Winspool.h"

int LogAge = 7;

int SEMCLASHES = 0;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[65536];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	ChatWriteLogLine(NULL, '!',Mess, Len, LOG_DEBUGx);
//	#ifdef _DEBUG 
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
//	#endif
	return;
}

VOID __cdecl Logprintf(int LogMode, ChatCIRCUIT * conn, int InOut, const char * format, ...)
{
	char Mess[65536];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	ChatWriteLogLine(conn, InOut, Mess, Len, LogMode);

	return;
}


void _GetSemaphore(struct SEM * Semaphore, int ID, char * File, int Line)
{
	//
	//	Wait for it to be free
	//
	
	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
//		Debugprintf("MailChat Semaphore Clash");
	}
loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	mov ebx, Semaphore
	xchg [ebx],eax		// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, we've got the semaphore
;
	}

	Semaphore->Line = Line;
	strcpy(Semaphore->File, File);

	return;
}
void FreeSemaphore(struct SEM * Semaphore)
{
	Semaphore->Flag=0;

	return; 
}

VOID __cdecl nodeprintf(ChatCIRCUIT * conn, const char * format, ...)
{
	char Mess[65536];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf_s(Mess, sizeof(Mess), format, arglist);

	ChatQueueMsg(conn, Mess, len);

	ChatWriteLogLine(conn, '>',Mess, len-1, LOG_CHAT);

	return;
}


// Costimised message handling routines.
/*
	Variables - a subset of those used by FBB

 $C : Number of the next message.
 $I : First name of the connected user.
 $L : Number of the latest message.
 $N : Number of active messages.
 $U : Callsign of the connected user.
 $W : Inserts a carriage return.
 $Z : Last message read by the user (L command).
 %X : Number of messages for the user.
 %x : Number of new messages for the user.
*/

VOID ExpandAndSendMessage(ChatCIRCUIT * conn, char * Msg, int LOG)
{
	char NewMessage[10000];
	char * OldP = Msg;
	char * NewP = NewMessage;
	char * ptr, * pptr;
	int len;
	char Dollar[] = "$";
	char CR[] = "\r";

	ptr = strchr(OldP, '$');

	while (ptr)
	{
		len = ptr - OldP;		// Chars before $
		memcpy(NewP, OldP, len);
		NewP += len;

		switch (*++ptr)
		{

		case 'I': // First name of the connected user.
			pptr = conn->UserPointer->Name;
			break;


		case 'W': // Inserts a carriage return.

			pptr = CR;
			break;

		default:

			pptr = Dollar;		// Just Copy $
		}

		len = strlen(pptr);
		memcpy(NewP, pptr, len);
		NewP += len;

		OldP = ++ptr;
		ptr = strchr(OldP, '$');
	}

	strcpy(NewP, OldP);

	len = RemoveLF(NewMessage, strlen(NewMessage));

	ChatWriteLogLine(conn, '>', NewMessage,  len, LOG);
	ChatQueueMsg(conn, NewMessage, len);
}

BOOL isdigits(char * string)
{
	// Returns TRUE id sting is decimal digits

	int i, n = strlen(string);
	
	for (i = 0; i < n; i++)
	{
		if (isdigit(string[i]) == FALSE) return FALSE;
	}
	return TRUE;
}

BOOL wildcardcompare(char * Target, char * Match)
{
	// Do a compare with string *string string* *string*

	// Strings should all be UC

	char Pattern[100];
	char * firststar;

	strcpy(Pattern, Match);
	firststar = strchr(Pattern,'*');

	if (firststar)
	{
		int Len = strlen(Pattern);

		if (Pattern[0] == '*' && Pattern[Len - 1] == '*')		// * at start and end
		{
			Pattern[Len - 1] = 0;
			return (BOOL)(strstr(Target, &Pattern[1]));
		}
		if (Pattern[0] == '*')		// * at start
		{
			// Compare the last len - 1 chars of Target

			int Targlen = strlen(Target);
			int Comparelen = Targlen - (Len - 1);

			if (Len == 1)			// Just *
				return TRUE;

			if (Comparelen < 0)	// Too Short
				return FALSE;

			return (memcmp(&Target[Comparelen], &Pattern[1], Len - 1) == 0);
		}

		// Must be * at end - compare first Len-1 char

		return (memcmp(Target, Pattern, Len - 1) == 0);

	}

	// No WildCards - straight strcmp
	return (strcmp(Target, Pattern) == 0);
}

int DeleteLogFiles()
{
   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   char File[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   LARGE_INTEGER ft;
   time_t now = time(NULL);
   int Age;

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, GetLogDirectory());
   strcat(szDir, "\\logs\\Log_*.txt");

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      return dwError;
   } 
   
   // List all the files in the directory with some info about them.

   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         OutputDebugString(ffd.cFileName);
      }
      else
      {
         ft.HighPart = ffd.ftCreationTime.dwHighDateTime;
         ft.LowPart = ffd.ftCreationTime.dwLowDateTime;

		 ft.QuadPart -=  116444736000000000;
		 ft.QuadPart /= 10000000;

		 Age = (now - ft.LowPart) / 86400; 

		 if (Age > LogAge)
		 {
			 sprintf(File, "%s/logs/%s%c", GetLogDirectory(), ffd.cFileName, 0);
			DeleteFile(File);
		 }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   dwError = GetLastError();

   FindClose(hFind);
   return dwError;
}

int RemoveLF(char * Message, int len)
{
	// Remove lf chars

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;
	}

	return (ptr2 - Message);
}

char * ReadInfoFile(char * File)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	char * MsgBytes;
	struct stat STAT;
	char * ptr1 = 0, * ptr2;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s", GetBPQDirectory(), File);

	if (stat(MsgFile, &STAT) == -1)
		return NULL;

	FileSize = STAT.st_size;

	hFile = fopen(MsgFile, "rb");

	if (hFile == NULL)
		return NULL;

	MsgBytes=malloc(FileSize+1);

	fread(MsgBytes, 1, FileSize, hFile); 

	fclose(hFile);

	MsgBytes[FileSize]=0;

	// Replace LF or CRLF with CR

	// First remove cr from crlf

	ptr1 = MsgBytes;

	while(ptr2 = strstr(ptr1, "\r\n"))
	{
		memmove(ptr2, ptr2 + 1, strlen(ptr2));
	}

	// Now replace lf with cr

	ptr1 = MsgBytes;

	while (*ptr1)
	{
		if (*ptr1 == '\n')
			*(ptr1) = '\r';

		ptr1++;
	}

	return MsgBytes;
}
