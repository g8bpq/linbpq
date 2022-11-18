// Chat Server for BPQ32 Packet Switch
//
//
// Based on MailChat Version 1.4.48.1


#define _CRT_SECURE_NO_DEPRECATE 

#include "BPQChat.h"
#include <new.h>

#define MaxSockets 64

extern ChatCIRCUIT ChatConnections[MaxSockets+1];
extern int	NumberofChatStreams;

extern char ChatConfigName[MAX_PATH];
extern char Session[20];

extern struct SEM ChatSemaphore;
extern struct SEM AllocSemaphore;
extern struct SEM ConSemaphore;
extern struct SEM OutputSEM;

extern char OtherNodesList[1000];
extern int MaxChatStreams;

extern char Position[81];
extern char PopupText[260];
extern int PopupMode;
extern int Bells, FlashOnBell, StripLF, WarnWrap, WrapInput, FlashOnConnect, CloseWindowOnBye;

extern char Version[32];
extern char ConsoleSize[32];
extern char MonitorSize[32];
extern char DebugSize[32];
extern char WindowSize[32];

extern int RunningConnectScript;

INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int GetMultiLineDialog(HWND hDialog, int DLGItem);
BOOL ProcessChatConnectScript(ChatCIRCUIT * conn, char * Buffer, int len);
VOID WriteMiniDump();

int Connected(int Stream)
{
	int n;
	ChatCIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, paclen, maxframe, l4window;
	char ConnectedMsg[] = "*** CONNECTED    ";
	char Msg[100];
	LINK    *link;
	KNOWNNODE *node;

	for (n = 0; n < NumberofChatStreams; n++)
	{
  		conn = &ChatConnections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect
		
				if (conn->rtcflags == p_linkini)
				{
					conn->paclen = 236;

					// Run first line of connect script

					ProcessChatConnectScript(conn, ConnectedMsg, 15);
					return 0;

//					nprintf(conn, "c %s\r", conn->u.link->call);
				}
				return 0;
			}
	
			memset(conn, 0, sizeof(ChatCIRCUIT));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			conn->Secure_Session = GetConnectionInfo(Stream, callsign,
				&port, &conn->SessType, &paclen, &maxframe, &l4window);

			conn->paclen = paclen;

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = zalloc(sizeof(struct UserInfo));

			strcpy(user->Call, callsign);

			conn->UserPointer = user;

			n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt

			ChatWriteLogLine(conn, '|',Msg, n, LOG_CHAT);
			conn->Flags |= CHATMODE;

			nodeprintf(conn, ChatSID, Ver[0], Ver[1], Ver[2], Ver[3]);

			// See if from a defined node
				
			for (link = link_hd; link; link = link->next)
			{
				if (matchi(conn->Callsign, link->call))
				{
					conn->rtcflags = p_linkwait;
					return 0;						// Wait for *RTL
				}
			}

			// See if from a previously known node

			// I'm not sure this is safe. If it really is from a node the *RTL will be rejected
			// Actually this protects against repeated attempts from a node that isn't configured. Maybe leave as is

			node = knownnode_find(conn->Callsign);

			if (node)
			{
				// A node is trying to link, but we don't have it defined - close

				Logprintf(LOG_CHAT, conn, '!', "Node %s connected, but is not defined as a Node - closing",
					conn->Callsign);

				nodeprintf(conn, "Node %s does not have %s defined as a node to link to - closing.\r",
					OurNode, conn->Callsign);

				ChatFlush(conn);
				Sleep(500);
				conn->rtcflags = p_nil;
				Disconnect(conn->BPQStream);

				return 0;
			}

			if (user->Name[0] == 0)
			{
				char * Name = lookupuser(user->Call);

				if (Name)
				{
					if (strlen(Name) > 17)
						Name[17] = 0;

					strcpy(user->Name, Name);
					free(Name);
				}
				else
				{
					conn->Flags |= GETTINGUSER;
					nputs(conn, NewUserPrompt);
					return TRUE;
				}
			}

			SendWelcomeMsg(Stream, conn, user);
			RefreshMainWindow();
			ChatFlush(conn);
			
			return 0;
		}
	}

	return 0;
}

int Disconnected (int Stream)
{
	struct UserInfo * user = NULL;
	ChatCIRCUIT * conn;
	int n;
	char Msg[255];
	int len;
	struct _EXCEPTION_POINTERS exinfo;

	for (n = 0; n <= NumberofChatStreams-1; n++)
	{
		conn=&ChatConnections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
			{
				return 0;
			}

			ChatClearQueue(conn);

			conn->Active = FALSE;

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK && conn->u.link)
				{
					// if running connect script, clear script active

					if (conn->u.link->flags & p_linkini)
					{
						RunningConnectScript = 0;
						conn->u.link->scriptRunning = 0;
					}

					len = sprintf_s(Msg, sizeof(Msg), "Chat Node %s Disconnected", conn->u.link->call);
					ChatWriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try {link_drop(conn);} My__except_Routine("link_drop");

				}
				else
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat User %s Disconnected", conn->Callsign);
					ChatWriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try
					{
						logout(conn);
					}
					#define EXCEPTMSG "logout"
					#include "StdExcept.c"
					}
				}

				conn->Flags = 0;
				conn->u.link = NULL;
				conn->UserPointer = NULL;	
			}

//			RefreshMainWindow();
			return 0;
		}
	}
	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	UINT MsgLen;
	int n;
	ChatCIRCUIT * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char Buffer[10000];
	int Written;

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn = &ChatConnections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen + 1000 > 10000)	// Shouldnt have lines longer  than this in text mode
					conn->InputLen = 0;				// discard	
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				if (conn->DebugHandle)				// Receiving a Compressed Message
					WriteFile(conn->DebugHandle, &conn->InputBuffer[conn->InputLen],
						InputLen, &Written, NULL);

				conn->Watchdog = 900;				// 15 Minutes

				conn->InputLen += InputLen;

				{

			loop:

				if (conn->InputLen == 1 && conn->InputBuffer[0] == 0)		// Single Null
				{
					conn->InputLen = 0;

					if (conn->u.user->circuit && conn->u.user->circuit->rtcflags & p_user)	// Local User
						conn->u.user->lastmsgtime = time(NULL);

					return 0;
				}

				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);

				if (ptr)	//  CR in buffer
				{
					user = conn->UserPointer;
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single meg in buffer

						__try
						{
							if (conn->rtcflags == p_linkini)		// Chat Connect
								ProcessChatConnectScript(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							conn->InputBuffer[conn->InputLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", conn->InputBuffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}
						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						memcpy(Buffer, conn->InputBuffer, MsgLen);
						__try
						{
							if (conn->rtcflags == p_linkini)
								ProcessChatConnectScript(conn, Buffer, MsgLen);
							else
								ProcessLine(conn, user, Buffer, MsgLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							Buffer[MsgLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", Buffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}

						if (*ptr == 0 || *ptr == '\n')
						{
							/// CR LF or CR Null

							ptr++;
							conn->InputLen--;
						}

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= MsgLen;

						goto loop;

					}
				}
				else
				{
					// no cr - testing.. 

//					Debugprintf("Test");
				}
				}
			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}

int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}


VOID SendWelcomeMsg(int Stream, ChatCIRCUIT * conn, struct UserInfo * user)
{
		if (!rtloginu (conn, TRUE))
		{
			// Already connected - close
			
			ChatFlush(conn);
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;

}

VOID SendPrompt(ChatCIRCUIT * conn, struct UserInfo * user)
{
	nodeprintf(conn, "de %s>\r", OurNode);
}

VOID ProcessLine(ChatCIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char seps[] = " \t\r";
	struct _EXCEPTION_POINTERS exinfo;

	{
		GetSemaphore(&ChatSemaphore, 0);

		__try 
		{
			ProcessChatLine(conn, user, Buffer, len);
		}
			#define EXCEPTMSG "ProcessChatLine"
			#include "StdExcept.c"

			FreeSemaphore(&ChatSemaphore);
	
			if (conn->BPQStream <  0)
				CloseConsole(conn->BPQStream);
			else
				Disconnect(conn->BPQStream);	

			return;
		}
		FreeSemaphore(&ChatSemaphore);
		return;
	}

	//	Send if possible

	ChatFlush(conn);
}


VOID SendUnbuffered(int stream, char * msg, int len)
{
	if (stream < 0)
		WritetoConsoleWindow(stream, msg, len);
	else
		SendMsg(stream, msg, len);
}


void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ChatCIRCUIT * conn;
	struct ConsoleInfo * Cons;

	int n;

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn = &ChatConnections[n];
		
		if (conn->Active == TRUE)
			ChatFlush(conn);
	}

	for (Cons = ConsHeader[0]; Cons; Cons = Cons->next)
	{
		if (Cons->Console)
			ChatFlush(Cons->Console);
	}
}


void ChatFlush(ChatCIRCUIT * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	//	BOOL Paging;				// Set if user wants paging
	//	int LinesSent;				// Count when paging
	//	int PageLen;				// Lines per page


	if (conn->OutputQueue == NULL)
	{
		// Nothing to send. If Close after Flush is set, disconnect

		if (conn->CloseAfterFlush)
		{
			conn->CloseAfterFlush--;
			
			if (conn->CloseAfterFlush)
				return;

			Disconnect(conn->BPQStream);
		}

		return;						// Nothing to send
	}
	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 4)
			return;						// Busy

		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		GetSemaphore(&OutputSEM, 0);

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		FreeSemaphore(&OutputSEM);

		tosend-=len;	
		sent++;

		if (sent > 4)
			return;
	}

	// All Sent. Free buffers and reset pointers

	ChatClearQueue(conn);
}

VOID ChatClearQueue(ChatCIRCUIT * conn)
{
	GetSemaphore(&OutputSEM, 0);

	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;

	FreeSemaphore(&OutputSEM);
}

/*
char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *tm;
	static char Date[]="xx-xxx hh:mmZ";

	tm = gmtime(&Datim);
	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%02d-%3s %02d:%02dZ",
					tm->tm_mday, month[tm->tm_mon], tm->tm_hour, tm->tm_min);

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}
	
	return Date;
}
*/


VOID FreeList(char ** Hddr)
{
	VOID ** Save;
	
	if (Hddr)
	{
		Save = Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]);
			Hddr++;
		}	
		free(Save);
	}
}


#define LIBCONFIG_STATIC
#include "libconfig.h"


config_t cfg;
config_setting_t * group;

extern char ChatWelcomeMsg[1000];

VOID SaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}

VOID SaveStringValue(config_setting_t * group, char * name, char * value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, value);

}

int GetIntValue(config_setting_t * group, char * name, int Default)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return Default;
}


BOOL GetStringValue(config_setting_t * group, char * name, char * value)
{
	const char * str;
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
	{
		str =  config_setting_get_string (setting);
		strcpy(value, str);
		return TRUE;
	}
	return FALSE;
}

BOOL GetChatConfig(char * ConfigName)
{
	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	
	if(! config_read_file(&cfg, ConfigName))
	{
		fprintf(stderr, "%d - %s\n",
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

	group = config_lookup (&cfg, "Chat");

	if (group == NULL)
		return EXIT_FAILURE;

	ChatApplNum = GetIntValue(group, "ApplNum", 0);
	MaxChatStreams = GetIntValue(group, "MaxStreams", 0);
	GetStringValue(group, "OtherChatNodes", OtherNodesList);
	GetStringValue(group, "ChatWelcomeMsg", ChatWelcomeMsg);
	GetStringValue(group, "MapPosition", Position);
	GetStringValue(group, "MapPopup", PopupText);
	PopupMode = GetIntValue(group, "PopupMode", 0);

	Bells = GetIntValue(group, "Bells", 0);
	FlashOnBell = GetIntValue(group, "FlashOnBell",0 );
	StripLF = GetIntValue(group, "StripLF", 0);
	WarnWrap = GetIntValue(group, "WarnWrap", 0);
	WrapInput = GetIntValue(group, "WrapInput",0 );
	FlashOnConnect = GetIntValue(group, "FlashOnConnect", 0);
	CloseWindowOnBye = GetIntValue(group, "CloseWindowOnBye", 0);

	GetStringValue(group, "ConsoleSize", ConsoleSize);
	GetStringValue(group, "MonitorSize", MonitorSize);
	GetStringValue(group, "DebugSize", DebugSize);
	GetStringValue(group, "WindowSize", WindowSize);
	GetStringValue(group, "Version", Version);

	return EXIT_SUCCESS;
}



VOID SaveChatConfigFile(char * File)
{
	config_setting_t *root, *group;

	//	Get rid of old config before saving
	
	config_init(&cfg);

	root = config_root_setting(&cfg);

	group = config_setting_add(root, "Chat", CONFIG_TYPE_GROUP);

	SaveIntValue(group, "ApplNum", ChatApplNum);
	SaveIntValue(group, "MaxStreams", MaxChatStreams);
	SaveStringValue(group, "OtherChatNodes", OtherNodesList);
	SaveStringValue(group, "ChatWelcomeMsg", ChatWelcomeMsg);

	SaveStringValue(group, "MapPosition", Position);
	SaveStringValue(group, "MapPopup", PopupText);
	SaveIntValue(group, "PopupMode", PopupMode);

	SaveIntValue(group, "Bells", Bells);
	SaveIntValue(group, "FlashOnBell", FlashOnBell);
	SaveIntValue(group, "StripLF", StripLF);
	SaveIntValue(group, "WarnWrap", WarnWrap);
	SaveIntValue(group, "WrapInput", WrapInput);
	SaveIntValue(group, "FlashOnConnect", FlashOnConnect);
	SaveIntValue(group, "CloseWindowOnBye", CloseWindowOnBye);

	SaveStringValue(group, "ConsoleSize", ConsoleSize);
	SaveStringValue(group, "MonitorSize", MonitorSize);
	SaveStringValue(group, "DebugSize", DebugSize);
	SaveStringValue(group, "WindowSize",WindowSize );
	SaveStringValue(group, "Version",Version );

	if(! config_write_file(&cfg, File))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}
	config_destroy(&cfg);
}



VOID SaveChatConfig(HWND hDlg)
{
	BOOL OK1;
	HKEY hKey=0;
	int OldChatAppl;
	char * ptr1;
	char * Save, * Context;

	OldChatAppl = ChatApplNum;
	
	ChatApplNum = GetDlgItemInt(hDlg, ID_CHATAPPL, &OK1, FALSE);
	MaxChatStreams = GetDlgItemInt(hDlg, ID_STREAMS, &OK1, FALSE);

	if (ChatApplNum)	
	{
		ptr1=GetApplCall(ChatApplNum);

		if (ptr1 && (*ptr1 < 0x21))
		{
			MessageBox(NULL, "WARNING - There is no APPLCALL in BPQCFG matching the confgured ChatApplNum. Chat will not work",
				"BPQMailChat", MB_ICONINFORMATION);
		}
	}

	GetMultiLineDialog(hDlg, IDC_ChatNodes);
	
	// Show dialog box now - gives time for links to close
	
	// reinitialise other nodes list. rtlink messes with the string so pass copy

	node_close();

	if (ChatApplNum == OldChatAppl)
		wsprintf(InfoBoxText, "Configuration Changes Saved and Applied");
	else
		wsprintf(InfoBoxText, "Warning Program must be restarted to change Chat Appl Num");

	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	Sleep(2);
			
	// Dont call removelinks - they may still be attached to a circuit. Just clear header

	link_hd = NULL;

	// Set up other nodes list. rtlink messes with the string so pass copy
	
	Save = ptr1 = strtok_s(_strdup(OtherNodesList), "\r\n", &Context);

	while (ptr1 && ptr1[0])
	{
		rtlink(ptr1);
		ptr1 = strtok_s(NULL, "\r\n", &Context);
	}


//	if (strchr(ptr1, '|') == 0)		// No script

//	while (*ptr1)
//	{
//		if (*ptr1 == '\r')
//		{
//			while (*(ptr1+2) == '\r')			// Blank line
//				ptr1+=2;

//			*++ptr1 = 32;
//		}
//		*ptr2++=*ptr1++;
//	}

//	*ptr2++ = 0;


	free(Save);

	if (user_hd)			// Any Users?
		makelinks();		// Bring up links

	SaveChatConfigFile(ChatConfigName);				// Commit to file
	GetChatConfig(ChatConfigName);

}
