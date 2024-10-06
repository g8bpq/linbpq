/*
Copyright 2001-2022 John Wiseman G8BPQ

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


//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#define DllImport

#include "CHeaders.h"
#include <stdlib.h>

#include "tncinfo.h"
#include "time.h"
#include "bpq32.h"
#include "telnetserver.h"

// This is needed to link with a lib built from source

#ifdef WIN32
#define ZEXPORT __stdcall
#endif

#include "zlib.h"

#define CKernel
#include "httpconnectioninfo.h"

extern int MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS, L3FRAMES;
extern int NUMBEROFNODES, MAXDESTS, L4CONNECTSOUT, L4CONNECTSIN, L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES;
extern int STATSTIME;
extern  TRANSPORTENTRY * L4TABLE;
extern BPQVECSTRUC BPQHOSTVECTOR[];
extern BOOL APRSApplConnected;  
extern char VersionString[];
VOID FormatTime3(char * Time, time_t cTime);
DllExport int APIENTRY Get_APPLMASK(int Stream);
VOID SaveUIConfig();
int ProcessNodeSignon(SOCKET sock, struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, int LOCAL);
VOID SetupUI(int Port);
VOID SendUIBeacon(int Port);
VOID GetParam(char * input, char * key, char * value);
VOID ARDOPAbort(struct TNCINFO * TNC);
VOID WriteMiniDump();
BOOL KillTNC(struct TNCINFO * TNC);
BOOL RestartTNC(struct TNCINFO * TNC);
int GetAISPageInfo(char * Buffer, int ais, int adsb);
int GetAPRSPageInfo(char * Buffer, double N, double S, double W, double E, int aprs, int ais, int adsb);
unsigned char * Compressit(unsigned char * In, int Len, int * OutLen);
char * stristr (char *ch1, char *ch2);
int GetAPRSIcon(unsigned char * _REPLYBUFFER, char * NodeURL);
char * GetStandardPage(char * FN, int * Len);
BOOL SHA1PasswordHash(char * String, char * Hash);
char * byte_base64_encode(char *str, int len);
int APIProcessHTTPMessage(char * response, char * Method, char * URL, char * request,	BOOL LOCAL, BOOL COOKIE);

extern struct ROUTE * NEIGHBOURS;
extern int  ROUTE_LEN;
extern int  MAXNEIGHBOURS;

extern struct DEST_LIST * DESTS;				// NODE LIST
extern int  DEST_LIST_LEN;
extern int  MAXDESTS;			// MAX NODES IN SYSTEM

extern struct _LINKTABLE * LINKS;
extern int	LINK_TABLE_LEN; 
extern int	MAXLINKS;
extern char * RigWebPage;
extern COLORREF Colours[256];

extern BOOL IncludesMail;
extern BOOL IncludesChat;

extern BOOL APRSWeb;  
extern BOOL RigActive;

extern HKEY REGTREE;

extern BOOL APRSActive;

extern UCHAR LogDirectory[];

extern struct RIGPORTINFO * PORTInfo[34];
extern int NumberofPorts;

extern UCHAR ConfigDirectory[260];

char * strlop(char * buf, char delim);
VOID sendandcheck(SOCKET sock, const char * Buffer, int Len);
int CompareNode(const void *a, const void *b);
int CompareAlias(const void *a, const void *b);
void ProcessMailHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen, int InputLen);
void ProcessChatHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen);
struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
int SetupNodeMenu(char * Buff, int SYSOP);
int StatusProc(char * Buff);
int ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL);
int ProcessChatSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, int LOCAL);
VOID APRSProcessHTTPMessage(SOCKET sock, char * MsgPtr, BOOL LOCAL, BOOL COOKIE);


static struct HTTPConnectionInfo * SessionList;	// active term mode sessions

char Mycall[10];

char MAILPipeFileName[] = "\\\\.\\pipe\\BPQMAILWebPipe";
char CHATPipeFileName[] = "\\\\.\\pipe\\BPQCHATWebPipe";

char Index[] = "<html><head><title>%s's BPQ32 Web Server</title></head><body><P align=center>"
"<table border=2 cellpadding=2 cellspacing=2 bgcolor=white>"
"<tr><td align=center><a href=/Node/NodeMenu.html>Node Pages</a></td>"
"<td align=center><a href=/aprs>APRS Pages</a></td></tr></table></body></html>";

char IndexNoAPRS[] = "<meta http-equiv=\"refresh\" content=\"0;url=/Node/NodeIndex.html\">"
"<html><head></head><body></body></html>";

//char APRSBit[] = "<td><a href=../aprs>APRS Pages</a></td>";

//char MailBit[] = "<td><a href=../Mail/Header>Mail Mgmt</a></td>"
//				 "<td><a href=/Webmail>WebMail</a></td>";
//char ChatBit[] = "<td><a href=../Chat/Header>Chat Mgmt</a></td>";

char Tail[] = "</body></html>";

char RouteHddr[] = "<h2 align=center>Routes</h2><table align=center border=2 style=font-family:monospace bgcolor=white>"
"<tr><th>Port</th><th>Call</th><th>Quality</th><th>Node Count</th><th>Frame Count</th><th>Retries</th><th>Percent</th><th>Maxframe</th><th>Frack</th><th>Last Heard</th><th>Queued</th><th>Rem Qual</th></tr>";

char RouteLine[] = "<tr><td>%s%d</td><td>%s%c</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d%</td><td>%d</td><td>%d</td><td>%02d:%02d</td><td>%d</td><td>%d</td></tr>";
char xNodeHddr[] = "<align=center><form align=center method=get action=/Node/Nodes.html>"
"<table align=center  bgcolor=white>"
"<tr><td><input type=submit class='btn' name=a value=\"Nodes Sorted by Alias\"></td><td>"
"<input type=submit class='btn' name=c value=\"Nodes Sorted by Call\"></td><td>"
"<input type=submit class='btn' name=t value=\"Nodes with traffic\"></td></tr></form></table>"
"<h2 align=center>Nodes %s</h2><table style=font-family:monospace align=center border=2 bgcolor=white><tr>";

char NodeHddr[] = "<center><form method=get action=/Node/Nodes.html>"
"<input type=submit class='btn' name=a value=\"Nodes Sorted by Alias\">"
"<input type=submit class='btn' name=c value=\"Nodes Sorted by Call\">"
"<input type=submit class='btn' name=t value=\"Nodes with traffic\"></form></center>"
"<h2 align=center>Nodes %s</h2><table style=font-family:monospace align=center border=2 bgcolor=white><tr>";

char NodeLine[] = "<td><a href=NodeDetail?%s>%s:%s</td>";


char StatsHddr[] = "<h2 align=center>Node Stats</h2><table align=center cellpadding=2 bgcolor=white>"
"<col width=250 /><col width=80 /><col width=80 /><col width=80 /><col width=80 /><col width=80 />";

char PortStatsHddr[] = "<h2 align=center>Stats for Port %d</h2><table align=center border=2 cellpadding=2 bgcolor=white>";

char PortStatsLine[] = "<tr><td> %s </td><td> %d </td></tr>";


char Beacons[] = "<h2 align=center>Beacon Configuration for Port %d</h2><h3 align=center>You need to be signed in to save changes</h3><table align=center border=2 cellpadding=2 bgcolor=white>"
"<form method=post action=BeaconAction>"
"<table align=center  bgcolor=white>"
"<tr><td>Send Interval (Minutes)</td><td><input type=text name=Every tabindex=1 size=5 value=%d></td></tr>" 
"<tr><td>To</td><td><input name=Dest style=\"text-transform:uppercase;\" tabindex=2 size=5 value=%s></td></tr>"  
"<tr><td>Path</td><td><input type=text name=Path style=\"text-transform:uppercase;\" size=50 maxlength=50 value=%s></td></tr>"
"<tr><td>Send From File</td><td><input type=text name=File size=50 maxlength=50  value=%s></td></tr>"
"<tr><td>Text</td><td><textarea name=\"Text\" cols=40 rows=5>%s</textarea></td></tr>"
"</table>" 
"<input type=hidden name=Port value=%d>"

"<p align=center><input type=submit class='btn' value=Save><input type=submit class='btn' value=Test name=Test>"
"</form>";


char LinkHddr[] = "<h2 align=center>Links</h2><table align=center border=2 bgcolor=white>"
"<tr><th>Far Call</th><th>Our Call</th><th>Port</th><th>ax.25 state</th><th>Link Type</th><th>ax.25 Version</th></tr>";

char LinkLine[] = "<tr><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td align=center >%d</td></tr>";

char UserHddr[] = "<h2 align=center>Sessions</h2><table align=center border=2 cellpadding=2 bgcolor=white>";

char UserLine[] = "<tr><td>%s</td><td>%s</td><td>%s</td></tr>";

char TermSignon[] = "<html><head><title>BPQ32 Node %s Terminal Access</title></head><body background=\"/background.jpg\">"
"<h2 align=center>BPQ32 Node %s Terminal Access</h2>"
"<h3 align=center>Please enter username and password to access the node</h3>"
"<form method=post action=TermSignon>"
"<table align=center  bgcolor=white>"
"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
"<p align=center><input type=submit class='btn' value=Submit><input type=submit class='btn' value=Cancel name=Cancel>"
"<input type=hidden name=Appl value=\"%s\"  id=Pass></form>";


char PassError[] = "<p align=center>Sorry, User or Password is invalid - please try again</p>";

char BusyError[] = "<p align=center>Sorry, No sessions available - please try later</p>";

char LostSession[] = "<html><body>Sorry, Session had been lost - refresh page to sign in again";
char NoSessions[] = "<html><body>Sorry, No Sessions available - refresh page to try again";

char TermPage[] = "<!DOCTYPE html><html><meta http-equiv=Content-Type content='text/html; charset=UTF-8' />"
"<head><title>BPQ32 Node %s</title></head>"
"<script>function resize(){"
"var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0];"
"x=w.innerWidth;"
"y=w.innerHeight;"
"var txt=document.getElementById('txt');"
"txt.style.height = y - 150 + 'px';}</script>"
"<body onload='resize()' onresize='resize()'>"
"<h3 align=center>BPQ32 Node %s</h3>"
"<form method=post action=/Node/TermClose?%s>"
"<p align=center><input type=submit class='btn' value='Close and return to Node Page' /></form>"
"<iframe style='display:block;' id=txt frameborder=2 marginwidth=0  marginheight=0 src=OutputScreen.html?%s width=100%%></iframe>"
"<iframe style='display:block;' frameborder=0 marginwidth=0 marginheight=3 src=InputLine.html?%s width=100%% height=45px></iframe>"
"</body>";

char TermOutput[] = "<!DOCTYPE html><html><head>"
"<meta http-equiv=cache-control content=no-cache>"
"<meta http-equiv=pragma content=no-cache>"
"<meta http-equiv=expires content=0>" 
"<meta http-equiv=refresh content=2>"
"<script type=\"text/javascript\">\r\n"
"function ScrollOutput()\r\n"
"{window.scrollBy(0,document.body.scrollHeight)}</script>"
"</head><body id=Text>"
"<div style=\"font-family:monospace;%s>\"";


// font-family:monospace;background-color:black;color:lawngreen;font-size:12px

char TermOutputTail[] = "</div><script type=\"text/javascript\">\r\nsetTimeout(ScrollOutput, 1)</script></body></html>";

/*
char InputLine[] = "<html><head></head><body onload='resize()' onresize='resize()'>"
"<form name=inputform method=post action=/TermInput?%s>"
"<script>document.inputform.input.focus();"
"function resize(){"
"var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0];"
"x=w.innerWidth;y=w.innerHeight;"
"var inp=document.getElementById('inp');"
"inp.style.width =  x + 'px';}</script>"
"<input id=inp type=text width=100%% name=input /></form>";
*/
char InputLine[] = "<!DOCTYPE html><html><head></head><body onload='resize()' onresize='resize()'>"
"<form name=inputform method=post action=/TermInput?%s>"
"<input style=\"font-family:monospace;%s>\" id=inp type=text text width=100%% name=input />"
"<script>document.inputform.input.focus();"
"function resize(){"
"var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0];"
"x=w.innerWidth;y=w.innerHeight;"
"var inp=document.getElementById('inp');"
"inp.style.width=x-20+'px';}</script></form>";

static char NodeSignon[] = "<html><head><title>BPQ32 Node SYSOP Access</title></head><body background=\"/background.jpg\">"
"<h3 align=center>BPQ32 Node %s SYSOP Access</h3>"
"<h3 align=center>This page sets Cookies. Don't continue if you object to this</h3>"
"<h3 align=center>Please enter Callsign and Password to access the Node</h3>"
"<form method=post action=/Node/Signon?Node>"
"<table align=center  bgcolor=white>"
"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
"<p align=center><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></form>";


static char MailSignon[] = "<html><head><title>BPQ32 Mail Server Access</title></head><body background=\"/background.jpg\">"
"<h3 align=center>BPQ32 Mail Server %s Access</h3>"
"<h3 align=center>Please enter Callsign and Password to access the BBS</h3>"
"<form method=post action=/Mail/Signon?Mail>"
"<table align=center  bgcolor=white>"
"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
"<p align=center><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></form>";

static char ChatSignon[] = "<html><head><title>BPQ32 Chat Server Access</title></head><body background=\"/background.jpg\">"
"<h3 align=center>BPQ32 Chat Server %s Access</h3>"
"<h3 align=center>Please enter Callsign and Password to access the Chat Server</h3>"
"<form method=post action=/Chat/Signon?Chat>"
"<table align=center  bgcolor=white>"
"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
"<p align=center><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></form>";


static char MailLostSession[] = "<html><body>"
"<form style=\"font-family: monospace; text-align: center;\" method=post action=/Mail/Lost?%s>"
"Sorry, Session had been lost<br><br>&nbsp;&nbsp;&nbsp;&nbsp;"
"<input name=Submit value=Restart type=submit class='btn'> <input type=submit class='btn' value=Exit name=Cancel><br></form>";


static char ConfigEditPage[] = "<html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
"<title>Edit Config</title></head><body background=/background.jpg>"
"<form style=\"font-family: monospace;  text-align: center;\"method=post action=CFGSave?%s>"
"<textarea cols=100 rows=25 name=Msg>%s</textarea><br><br>"
"<input name=Save value=Save type=submit class='btn'><input name=Cancel value=Cancel type=submit class='btn'><br></form>";

static char EXCEPTMSG[80] = "";


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




VOID PollSession(struct HTTPConnectionInfo * Session)
{
	int state, change;
	int count, len;
	char Msg[400] = "";
	char Formatted[8192];
	char * ptr1, * ptr2;
	char c;
	int Line;

	// Poll Node

	SessionState(Session->Stream, &state, &change);

	if (change == 1)
	{
		int Line = Session->LastLine++;
		free(Session->ScreenLines[Line]);

		if (state == 1)// Connected
			Session->ScreenLines[Line] = _strdup("*** Connected<br>\r\n");
		else
			Session->ScreenLines[Line] = _strdup("*** Disconnected<br>\r\n");

		if (Line == 99)
			Session->LastLine = 0;

		Session->Changed = TRUE;
	}

	if (RXCount(Session->Stream) > 0)
	{
		int realLen = 0;

		do
		{
			GetMsg(Session->Stream, &Msg[0], &len, &count);

			// replace cr with <br> and space with &nbsp;


			ptr1 = Msg;
			ptr2 = &Formatted[0];

			if (Session->PartLine)
			{
				// Last line was incomplete - append to it

				realLen = Session->PartLine;

				Line = Session->LastLine - 1;

				if (Line < 0)
					Line = 99;

				strcpy(Formatted, Session->ScreenLines[Line]);
				ptr2 += strlen(Formatted);

				Session->LastLine = Line;
				Session->PartLine = FALSE;
			}

			while (len--)
			{
				c = *(ptr1++);
				realLen++;

				if (c == 13)
				{
					int LineLen;

					strcpy(ptr2, "<br>\r\n");

					// Write to screen

					Line = Session->LastLine++;
					free(Session->ScreenLines[Line]);

					LineLen = (int)strlen(Formatted);

					// if line starts with a colour code, process it

					if (Formatted[0] ==  0x1b && LineLen > 1)
					{
						int ColourCode = Formatted[1] - 10;
						COLORREF Colour = Colours[ColourCode];
						char ColString[30];

						memmove(&Formatted[20], &Formatted[2], LineLen);
						sprintf(ColString, "<font color=#%02X%02X%02X>",  GetRValue(Colour), GetGValue(Colour), GetBValue(Colour));
						memcpy(Formatted, ColString, 20);
						strcat(Formatted, "</font>");
						LineLen =+ 28;
					}

					Session->ScreenLineLen[Line] = LineLen;
					Session->ScreenLines[Line] = _strdup(Formatted);

					if (Line == 99)
						Session->LastLine = 0;

					ptr2 = &Formatted[0];
					realLen = 0;

				}
				else if (c == 32)
				{
					memcpy(ptr2, "&nbsp;", 6);
					ptr2 += 6;

					// Make sure line isn't too long
					// but beware of spaces expanded to &nbsp; - count chars in line

					if ((realLen) > 100)
					{
						strcpy(ptr2, "<br>\r\n");

						Line = Session->LastLine++;
						free(Session->ScreenLines[Line]);

						Session->ScreenLines[Line] = _strdup(Formatted);

						if (Line == 99)
							Session->LastLine = 0;

						ptr2 = &Formatted[0];
						realLen = 0;
					}
				}
				else if (c == '>')
				{
					memcpy(ptr2, "&gt;", 4);
					ptr2 += 4;
				}
				else if (c == '<')
				{
					memcpy(ptr2, "&lt;", 4);
					ptr2 += 4;
				}
				else
					*(ptr2++) = c;

			}

			*ptr2 = 0;

			if (ptr2 != &Formatted[0])
			{
				// Incomplete line

				// Save to screen

				Line = Session->LastLine++;
				free(Session->ScreenLines[Line]);

				Session->ScreenLines[Line] = _strdup(Formatted);

				if (Line == 99)
					Session->LastLine = 0;

				Session->PartLine = realLen;
			}

			//	strcat(Session->ScreenBuffer, Formatted);
			Session->Changed = TRUE;

		} while (count > 0);
	}
}


VOID HTTPTimer()
{
	// Run every tick. Check for status change and data available

	struct HTTPConnectionInfo * Session = SessionList;	// active term mode sessions
	struct HTTPConnectionInfo * PreviousSession = NULL;

//	inf();

	while (Session)
	{
		Session->KillTimer++;

		if (Session->Key[0] != 'T')
		{
			PreviousSession = Session;
			Session = Session->Next;
			continue;
		}

		if (Session->KillTimer > 3000)		// Around 5 mins
		{
			int i;
			int Stream = Session->Stream;

			for (i = 0; i < 100; i++)
			{
				free(Session->ScreenLines[i]);
			}

			SessionControl(Stream, 2, 0);
			SessionState(Stream, &i, &i);
			DeallocateStream(Stream);

			if (PreviousSession)
				PreviousSession->Next = Session->Next;		// Remove from chain
			else
				SessionList = Session->Next;

			free(Session);

			break;
		}

		PollSession(Session);

		//		if (Session->ResponseTimer == 0 && Session->Changed)
		//			Debugprintf("Data to send but no outstanding GET");

		if (Session->ResponseTimer)
		{
			Session->ResponseTimer--;

			if (Session->ResponseTimer == 0 || Session->Changed)
			{
				SOCKET sock = Session->sock;
				char _REPLYBUFFER[100000];
				int ReplyLen;
				char Header[256];
				int HeaderLen;
				int Last = Session->LastLine;
				int n;
				struct TNCINFO * TNC = Session->TNC;
				struct TCPINFO * TCP = 0;
				
				if (TNC)
					TCP = TNC->TCPInfo;

				if (TCP && TCP->WebTermCSS)	
					sprintf(_REPLYBUFFER, TermOutput, TCP->WebTermCSS);
				else
					sprintf(_REPLYBUFFER, TermOutput, "");

				for (n = Last;;)
				{
					if ((strlen(Session->ScreenLines[n]) + strlen(_REPLYBUFFER)) < 99999)
						strcat(_REPLYBUFFER, Session->ScreenLines[n]);

					if (n == 99)
						n = -1;

					if (++n == Last)
						break;
				}

				ReplyLen = (int)strlen(_REPLYBUFFER);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", TermOutputTail);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen);
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);

				Session->ResponseTimer = Session->Changed = 0;
			}
		}
		PreviousSession = Session;
		Session = Session->Next;
	}
}

struct HTTPConnectionInfo * AllocateSession(SOCKET sock, char Mode)
{
	time_t KeyVal;
	struct HTTPConnectionInfo * Session = zalloc(sizeof(struct HTTPConnectionInfo));
	int i;

	if (Session == NULL)
		return NULL;

	if (Mode == 'T')
	{
		// Terminal

		for (i = 0; i < 20; i++)
			Session->ScreenLines[i] = _strdup("Scroll to end<br>");

		for (i = 20; i < 100; i++)
			Session->ScreenLines[i] = _strdup("<br>\r\n");

		Session->Stream = FindFreeStream();

		if (Session->Stream == 0)
			return NULL;

		SessionControl(Session->Stream, 1, 0);
	}

	KeyVal = (int)sock * time(NULL);

	sprintf(Session->Key, "%c%012X", Mode, (int)KeyVal);

	if (SessionList)
		Session->Next = SessionList;

	SessionList = Session;

	return Session;
}

struct HTTPConnectionInfo * FindSession(char * Key)
{
	struct HTTPConnectionInfo * Session = SessionList;

	while (Session)
	{
		if (strcmp(Session->Key, Key) == 0)
			return Session;

		Session = Session->Next;
	}

	return NULL;
}

void ProcessTermInput(SOCKET sock, char * MsgPtr, int MsgLen, char * Key)
{
	char _REPLYBUFFER[2048];
	int ReplyLen;
	char Header[256];
	int HeaderLen;
	int State;
	struct HTTPConnectionInfo * Session = FindSession(Key);
	int Stream;
	int maxlen = 1000;


	if (Session == NULL)
	{
		ReplyLen = sprintf(_REPLYBUFFER, "%s", LostSession);
	}
	else
	{
		char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
		char * end = &MsgPtr[MsgLen];
		int Line = Session->LastLine++;
		char * ptr1, * ptr2;
		char c;
		UCHAR hex;

		int msglen = end - input;

		struct TNCINFO * TNC = Session->TNC;
		struct TCPINFO * TCP = 0;
				
		if (TNC)
			TCP = TNC->TCPInfo;

		if (TCP && TCP->WebTermCSS)
			maxlen -= strlen(TCP->WebTermCSS);

		if (MsgLen > maxlen)
		{
			Session->KillTimer = 99999; // close session
			return;
		}


		if (TCP && TCP->WebTermCSS)	
			ReplyLen = sprintf(_REPLYBUFFER, InputLine, Key, TCP->WebTermCSS);
		else
			ReplyLen = sprintf(_REPLYBUFFER, InputLine, Key, "");


		Stream = Session->Stream;

		input += 10;
		ptr1 = ptr2 = input;

		// Convert any %xx constructs

		while (ptr1 != end)
		{
			c = *(ptr1++);
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

		end = ptr2;

		*ptr2 = 0;

		strcat(input, "<br>\r\n");

		free(Session->ScreenLines[Line]);

		Session->ScreenLines[Line] = _strdup(input);

		if (Line == 99)
			Session->LastLine = 0;

		*end++ = 13;
		*end = 0;

		SessionStateNoAck(Stream, &State);

		if (State == 0)
		{
			char AXCall[10];
			SessionControl(Stream, 1, 0);
			if (BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION == NULL)
			{
				//No L4 sessions free

				ReplyLen = sprintf(_REPLYBUFFER, "%s", NoSessions);
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);                                                                                                                                                                                                                                        				
				send(sock, Tail, (int)strlen(Tail), 0);
				return;
			}

			ConvToAX25(Session->HTTPCall, AXCall);
			ChangeSessionCallsign(Stream, AXCall);
			if (Session->USER)
				BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION->Secure_Session = Session->USER->Secure;
			else
				Debugprintf("HTTP Term Session->USER is NULL");
		}

		SendMsg(Stream, input, (int)(end - input));
		Session->Changed = TRUE;
		Session->KillTimer = 0;
	}

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);
}


void ProcessTermClose(SOCKET sock, char * MsgPtr, int MsgLen, char * Key, int LOCAL)
{
	char _REPLYBUFFER[8192];
	int ReplyLen = sprintf(_REPLYBUFFER, InputLine, Key, "");
	char Header[256];
	int HeaderLen;
	struct HTTPConnectionInfo * Session = FindSession(Key);

	if (Session)
	{
		Session->KillTimer = 99999;
	}

	ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
		"\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);
}

int ProcessTermSignon(struct TNCINFO * TNC, SOCKET sock, char * MsgPtr, int MsgLen,  int LOCAL)
{
	char _REPLYBUFFER[8192];
	int ReplyLen;
	char Header[256];
	int HeaderLen;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Context, * Appl;
	char NoApp[] = "";
	struct TCPINFO * TCP = TNC->TCPInfo;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
			goto Sendit;
		}
		user = strtok_s(&input[9], "&", &Context);
		password = strtok_s(NULL, "=", &Context);
		password = strtok_s(NULL, "&", &Context);

		Appl = strtok_s(NULL, "=", &Context);
		Appl = strtok_s(NULL, "&", &Context);

		if (Appl == 0)
			Appl = NoApp;

		if (password == NULL)
		{
			ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Appl);
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PassError);
			goto Sendit;
		}

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if ((strcmp(password, USER->Password) == 0) &&
				((_stricmp(user, USER->UserName) == 0 ) || (_stricmp(USER->UserName, "ANON") == 0)))
			{
				// ok

				struct HTTPConnectionInfo * Session = AllocateSession(sock, 'T');

				if (Session)
				{
					char AXCall[10];
					ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, Session->Key, Session->Key, Session->Key);
					if (_stricmp(USER->UserName, "ANON") == 0)
						strcpy(Session->HTTPCall, _strupr(user));
					else
						strcpy(Session->HTTPCall, USER->Callsign);
					ConvToAX25(Session->HTTPCall, AXCall);
					ChangeSessionCallsign(Session->Stream, AXCall);
					BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION->Secure_Session = USER->Secure;
					Session->USER = USER;

					if (USER->Appl[0])
						SendMsg(Session->Stream, USER->Appl, (int)strlen(USER->Appl));
				}
				else
				{
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);

					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", BusyError);
				}
				break;
			}
		}

		if (i == TCP->NumberofUsers)
		{
			//   Not found

			ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Appl);
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PassError);
		}

	}

Sendit:

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);

	return 1;
}

char * LookupKey(char * Key)
{
	if (strcmp(Key, "##MY_CALLSIGN##") == 0)
	{
		char Mycall[10];
		memcpy(Mycall, &MYNODECALL, 10);
		strlop(Mycall, ' ');

		return _strdup(Mycall);
	}
	return NULL;
}


int ProcessSpecialPage(char * Buffer, int FileSize)
{
	// replaces ##xxx### constructs with the requested data

	char * NewMessage = malloc(100000);
	char * ptr1 = Buffer, * ptr2, * ptr3, * ptr4, * NewPtr = NewMessage;
	int PrevLen;
	int BytesLeft = FileSize;
	int NewFileSize = FileSize;
	char * StripPtr = ptr1;

	// strip comments blocks

	while (ptr4 = strstr(ptr1, "<!--"))
	{
		ptr2 = strstr(ptr4, "-->");
		if (ptr2)
		{
			PrevLen = (int)(ptr4 - ptr1);
			memcpy(StripPtr, ptr1, PrevLen);
			StripPtr += PrevLen;
			ptr1 = ptr2 + 3;
			BytesLeft = (int)(FileSize - (ptr1 - Buffer));
		}
	}

	memcpy(StripPtr, ptr1, BytesLeft);
	StripPtr += BytesLeft;

	BytesLeft = (int)(StripPtr - Buffer);

	FileSize = BytesLeft;
	NewFileSize = FileSize;
	ptr1 = Buffer;
	ptr1[FileSize] = 0;

loop:
	ptr2 = strstr(ptr1, "##");

	if (ptr2)
	{
		PrevLen = (int)(ptr2 - ptr1);			// Bytes before special text

		ptr3 = strstr(ptr2+2, "##");

		if (ptr3)
		{
			char Key[80] = "";
			int KeyLen;
			char * NewText;
			int NewTextLen;

			ptr3 += 2;
			KeyLen = (int)(ptr3 - ptr2);

			if (KeyLen < 80)
				memcpy(Key, ptr2, KeyLen);

			NewText = LookupKey(Key);

			if (NewText)
			{
				NewTextLen = (int)strlen(NewText);
				NewFileSize = NewFileSize + NewTextLen - KeyLen;					
				//	NewMessage = realloc(NewMessage, NewFileSize);

				memcpy(NewPtr, ptr1, PrevLen);
				NewPtr += PrevLen;
				memcpy(NewPtr, NewText, NewTextLen);
				NewPtr += NewTextLen;

				free(NewText);
				NewText = NULL;
			}
			else
			{
				// Key not found, so just leave

				memcpy(NewPtr, ptr1, PrevLen + KeyLen);
				NewPtr += (PrevLen + KeyLen);
			}

			ptr1 = ptr3;			// Continue scan from here
			BytesLeft = (int)(Buffer + FileSize - ptr3);
		}
		else		// Unmatched ##
		{
			memcpy(NewPtr, ptr1, PrevLen + 2);
			NewPtr += (PrevLen + 2);
			ptr1 = ptr2 + 2;
		}
		goto loop;
	}

	// Copy Rest

	memcpy(NewPtr, ptr1, BytesLeft);
	NewMessage[NewFileSize] = 0;

	strcpy(Buffer, NewMessage);
	free(NewMessage);

	return NewFileSize;
}

int SendMessageFile(SOCKET sock, char * FN, BOOL OnlyifExists, int allowDeflate)
{
	int FileSize = 0, Sent, Loops = 0;
	char * MsgBytes;
	char MsgFile[512];
	FILE * hFile;
	int ReadLen;
	BOOL Special = FALSE;
	int Len;
	int HeaderLen;
	char Header[256];
	char TimeString[64];
	char FileTimeString[64];
	struct stat STAT;
	char * ptr;
	char * Compressed = 0;
	char Encoding[] = "Content-Encoding: deflate\r\n";
	char Type[64] = "Content-Type: text/html\r\n";
 
#ifdef WIN32

	struct _EXCEPTION_POINTERS exinfo;
	strcpy(EXCEPTMSG, "SendMessageFile");

	__try {
#endif

		UndoTransparency(FN);

		if (strstr(FN, ".."))
		{
			FN[0] = '/';
			FN[1] = 0;
		}

		if (strlen(FN) > 256)
		{
			FN[256] = 0;
			Debugprintf("HTTP File Name too long %s", FN);
		}

		if (strcmp(FN, "/") == 0)
			if (APRSActive)
				sprintf(MsgFile, "%s/HTML/index.html", BPQDirectory);
			else
				sprintf(MsgFile, "%s/HTML/indexnoaprs.html", BPQDirectory);
		else
			sprintf(MsgFile, "%s/HTML%s", BPQDirectory, FN);


		// First see if file exists so we can override standard ones in code

		if (stat(MsgFile, &STAT) == 0 && (hFile = fopen(MsgFile, "rb")))
		{
			FileSize = STAT.st_size;

			MsgBytes = zalloc(FileSize + 1);

			ReadLen = (int)fread(MsgBytes, 1, FileSize, hFile);

			fclose(hFile);

			//	ft.QuadPart -=  116444736000000000;
			//	ft.QuadPart /= 10000000;

			//	ctime = ft.LowPart;

			FormatTime3(FileTimeString, STAT.st_ctime);
		}
		else
		{
			// See if it is a hard coded file

			MsgBytes = GetStandardPage(&FN[1], &FileSize);

			if (MsgBytes)
			{
				if (FileSize == 0)
					FileSize = strlen(MsgBytes);

				FormatTime3(FileTimeString, 0);
			}
			else
			{
				if (OnlyifExists)					// Set if we dont want an error response if missing
					return -1;

				Len = sprintf(Header, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
				send(sock, Header, Len, 0);
				return 0;
			}
		}

		// if HTML file, look for ##...## substitutions

		if ((strcmp(FN, "/") == 0 || strstr(FN, "htm" ) || strstr(FN, "HTM")) &&  strstr(MsgBytes, "##" ))
		{
			FileSize = ProcessSpecialPage(MsgBytes, FileSize); 
			FormatTime3(FileTimeString, time(NULL));

		}

		FormatTime3(TimeString, time(NULL));

		ptr = FN;

		while (strchr(ptr, '.'))
		{
			ptr = strchr(ptr, '.');
			++ptr;
		}

		if (_stricmp(ptr, "js") == 0)
			strcpy(Type, "Content-Type: text/javascript\r\n");

		if (_stricmp(ptr, "pdf") == 0)
			strcpy(Type, "Content-Type: application/pdf\r\n");

		if (allowDeflate)
		{
			Compressed = Compressit(MsgBytes, FileSize, &FileSize);
		} 
		else
		{
			Encoding[0] = 0;
			Compressed = MsgBytes;
		}

		if (_stricmp(ptr, "jpg") == 0 || _stricmp(ptr, "jpeg") == 0 || _stricmp(ptr, "png") == 0 || _stricmp(ptr, "gif") == 0 || _stricmp(ptr, "ico") == 0)
			strcpy(Type, "Content-Type: image\r\n");

		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
				"Date: %s\r\n"
				"Last-Modified: %s\r\n"
				"%s%s"
				"\r\n", FileSize, TimeString, FileTimeString, Type, Encoding);
	
		send(sock, Header, HeaderLen, 0);

		Sent = send(sock, Compressed, FileSize, 0);

		while (Sent != FileSize && Loops++ < 3000)				// 100 secs max
		{	
			if (Sent > 0)					// something sent
			{
//				Debugprintf("%d out of %d sent", Sent, FileSize);
				FileSize -= Sent;
				memmove(Compressed, &Compressed[Sent], FileSize);
			}

			Sleep(30);
			Sent = send(sock, Compressed, FileSize, 0);
		}

//		Debugprintf("%d out of %d sent %d loops", Sent, FileSize, Loops);


		free (MsgBytes);
		if (allowDeflate)
			free (Compressed);

#ifdef WIN32
	}
#include "StdExcept.c"
	Debugprintf("Sending FIle %s", FN);
}
#endif

return 0;
}

VOID sendandcheck(SOCKET sock, const char * Buffer, int Len)
{
	int Loops = 0;
	int Sent = send(sock, Buffer, Len, 0);
	char * Copy = NULL;

	while (Sent != Len && Loops++ < 300)					// 10 secs max
	{	
		//		Debugprintf("%d out of %d sent %d Loops", Sent, Len, Loops);

		if (Copy == NULL)
		{
			Copy = malloc(Len);
			memcpy(Copy, Buffer, Len);
		}

		if (Sent > 0)					// something sent
		{
			Len -= Sent;
			memmove(Copy, &Copy[Sent], Len);
		}		

		Sleep(30);
		Sent = send(sock, Copy, Len, 0);
	}

	if (Copy)
		free(Copy);

	return;
}

int RefreshTermWindow(struct TCPINFO * TCP, struct HTTPConnectionInfo * Session, char * _REPLYBUFFER)
{
	char Msg[400] = "";
	int HeaderLen, ReplyLen;
	char Header[256];

	PollSession(Session);			// See if anything received 

	if (Session->Changed)
	{
		int Last = Session->LastLine;
		int n;
	
		if (TCP && TCP->WebTermCSS)	
			sprintf(_REPLYBUFFER, TermOutput, TCP->WebTermCSS);
		else
			sprintf(_REPLYBUFFER, TermOutput, "");

		for (n = Last;;)
		{
			strcat(_REPLYBUFFER, Session->ScreenLines[n]);

			if (n == 99)
				n = -1;

			if (++n == Last)
				break;
		}

		Session->Changed = 0;

		ReplyLen = (int)strlen(_REPLYBUFFER);
		ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", TermOutputTail);

		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen);
		sendandcheck(Session->sock, Header, HeaderLen);
		sendandcheck(Session->sock, _REPLYBUFFER, ReplyLen);

		return 1;
	}
	else
		return 0;
}	

int SetupNodeMenu(char * Buff, int LOCAL)
{
	int Len = 0, i;
	struct TNCINFO * TNC;
	int top = 0, left = 0;

	char NodeMenuHeader[] = "<html id=body><head><title>%s's BPQ32 Web Server</title>"
	"<style type=\"text/css\">"
	// The container <div> - needed to position the dropdown content
	".dropdown {position: relative; display: inline-block;}"
	// Dropdown Content (Hidden by Default)
	".dropdown-content {display: none; position: absolute; left: -100px; background-color: #f1f1f1;"
	" min-width: 120px; border: 1px solid; padding: 4px; z-index: 1;}"
	// Links inside the dropdown
	".dropdown-content a {color: black; padding: 2px 2px; display: block;}"
	// Change color of dropdown links on hover 
	".dropdown-content a:hover {background-color: #ddd}"
	// Show the dropdown menu (use JS to add this class to the .dropdown-content container when the user clicks on the dropdown button)
	".show {display:block;}"
	"input.btn:active {background:black;color:white;} "
	"submit.btn:active {background:black;color:white;} "
	"</style>"

	"<script>\r\n"


// Close the dropdown menu if the user clicks outside of it
	"window.onclick = function(event) {console.log(event.target.id);"
	" if (event.target.id == 'body') {"
	"  var dropdowns = document.getElementsByClassName('dropdown-content');"
	"  var i;\r\n"
	" for (i = 0; i < dropdowns.length; i++) {"
    "  var openDropdown = dropdowns[i];"
    "  if (openDropdown.classList.contains('show')) {"
	"   openDropdown.classList.remove('show');"
    "  }}}}\r\n"

	"function myShow() {document.getElementById('myDropdown').classList.add('show');}"
//	"function myHide() {"
//	" if (event.target.matches('.HTMLBodyElement'))"
//	"{document.getElementById('myDropdown').classList.remove('show');}}"


	"function dev_win(URL,w,h,top,left){"
		"var ww = \"width=\" + w;"
		"var xx = \",height=\" + h;"
		"var yy = \",top=\" + top;"
		"var zz = \",left=\" + left;"
		"var param = \"toolbar=no, location=no, directories=no, status=no, "
		"menubar=no, scrollbars=no, resizable=no, titlebar=no, toobar=no, \" + ww + xx + yy + zz;"
		"window.open(URL,\"_blank\",param);"
		"}\r\n"
	
	"function open_win(){";

	char NodeMenuLine[] = "dev_win(\"/Node/Port?%d\",%d,%d,%d,%d);";

	char NodeMenuRest[] = "}</script></head>"
		"<body id=body background=\"/background.jpg\"><h1 align=center>BPQ32 Node %s</h1><P>"
		"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
		"<td><a href=/Node/Routes.html>Routes</a></td>"
		"<td><a href=/Node/Nodes.html>Nodes</a></td>"
		"<td><a href=/Node/Ports.html>Ports</a></td>"
		"<td><a href=/Node/Links.html>Links</a></td>"
		"<td><a href=/Node/Users.html>Users</a></td>"
		"<td><a href=/Node/Stats.html>Stats</a></td>"
		"<td><a href=/Node/Terminal.html>Terminal</a></td>%s%s%s%s%s%s";

	char DriverBit[] = "<td><a href=\"javascript:open_win();\">Driver Windows</a></td>"
		"<td><a href=javascript:dev_win(\"/Node/Streams\",820,700,200,200);>Stream Status</a></td>";

	char APRSBit[] = "<td><a href=../aprs>APRS Pages</a></td>";

	char MailBit[] = "<td><a href=../Mail/Header>Mail Mgmt</a></td>"
		"<td><a href=/Webmail>WebMail</a></td>";

	char ChatBit[] = "<td><a href=../Chat/Header>Chat Mgmt</a></td>";
	char SigninBit[] = "<td><a href=/Node/Signon.html>SYSOP Signin</a></td>";

	char NodeTail[] = 
		"<td><a href=/Node/EditCfg.html>Edit Config</a></td>"
		"<td><div onmouseover=myShow() class='dropdown'>"
		"<button class=\"dropbtn\">View Logs</button>"
		"<div id=\"myDropdown\" class=\"dropdown-content\">"
		"<form id = doDate form action='/node/ShowLog.html'><label>"
		"Select Date: <input type='date' name='date' id=e>"
		"<script>"
		"document.getElementById('e').value = new Date().toISOString().substring(0, 10);"
		"</script></label>"
		"<input type=submit class='btn' name='BBS' value='BBS Log'></br>"
		"<input type=submit class='btn' name='Debug' value='BBS Debug Log'></br>"
		"<input type=submit class='btn' name='Telnet' value='Telnet Log'></br>"
		"<input type=submit class='btn' name='CMS' value='CMS Log'></br>"
		"<input type=submit class='btn' name='Chat' value='Chat Log'></br>"
		"</form></div>"
		"</div>"		
		"</td></tr></table>";


	Len = sprintf(Buff, NodeMenuHeader, Mycall);

	for (i=1; i <= MAXBPQPORTS; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC->WebWindowProc)
		{
			Len += sprintf(&Buff[Len], NodeMenuLine, i, TNC->WebWinX, TNC->WebWinY, top, left);
			top += 22;
			left += 22;
		}
	}

	Len += sprintf(&Buff[Len], NodeMenuRest, Mycall,
		DriverBit,
		(APRSWeb)?APRSBit:"",
		(IncludesMail)?MailBit:"", (IncludesChat)?ChatBit:"", (LOCAL)?"":SigninBit, NodeTail);

	return Len;
}

VOID SaveConfigFile(SOCKET sock , char * MsgPtr, char * Rest, int LOCAL)
{
	int ReplyLen = 0;
	char * ptr, * ptr1, * ptr2, *input;
	char c;
	int MsgLen, WriteLen = 0;
	char inputname[250]="bpq32.cfg";
	FILE *fp1;
	char Header[256];
	int HeaderLen;
	char Reply[4096];
	char Mess[256];
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];
	struct stat STAT;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(Reply, LOCAL);
			//			ReplyLen = sprintf(Reply, "%s", "<html><script>window.close();</script></html>");
			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);
			send(sock, Reply, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);
			return;
		}

		ptr = strstr(input, "&Save=");

		if (ptr)
		{
			*ptr = 0;

			// Undo any % transparency

			ptr1 = ptr2 = input + 8;

			c = *(ptr1++);

			while (c)
			{
				if (c == '%')
				{
					int n;
					int m = *(ptr1++) - '0';
					if (m > 9) m = m - 7;
					n = *(ptr1++) - '0';
					if (n > 9) n = n - 7;

					c  = m * 16 + n;
				}
				else if (c == '+')
					c = ' ';

#ifndef WIN32
				if (c != 13)				// Strip CR if Linux
#endif
					*(ptr2++) = c;

				c = *(ptr1++);

			}

			*(ptr2++) = 0;

			MsgLen = (int)strlen(input + 8);

			if (ConfigDirectory[0] == 0)
			{
				strcpy(inputname, "bpq32.cfg");
			}
			else
			{
				strcpy(inputname,ConfigDirectory);
				strcat(inputname,"/");
				strcat(inputname, "bpq32.cfg");
			}

			// Make a backup of the config

			// Keep 4 Generations

			strcpy(Backup2, inputname);
			strcat(Backup2, ".bak.3");

			strcpy(Backup1, inputname);
			strcat(Backup1, ".bak.2");

			DeleteFile(Backup2);			// Remove old .bak.3
			MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

			strcpy(Backup2, inputname);
			strcat(Backup2, ".bak.1");

			MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

			strcpy(Backup1, inputname);
			strcat(Backup1, ".bak");

			MoveFile(Backup1, Backup2);		// Move .bak to .bak.1

			CopyFile(inputname, Backup1, FALSE);	// Copy to .bak

			// Get length to compare with new length

			stat(inputname, &STAT);

			fp1 = fopen(inputname, "wb");

			if (fp1)
			{
				WriteLen = (int)fwrite(input + 8, 1, MsgLen, fp1);
				fclose(fp1);
			}

			if (WriteLen != MsgLen)
				sprintf_s(Mess, sizeof(Mess), "Failed to write Config File");
			else
				sprintf_s(Mess, sizeof(Mess), "Configuration Saved, Orig Length %d New Length %d", 
				STAT.st_size, MsgLen);
		}

		ReplyLen = sprintf(Reply, "<html><script>alert(\"%s\");window.close();</script></html>", Mess);
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
		send(sock, Header, HeaderLen, 0);
		send(sock, Reply, ReplyLen, 0);
		send(sock, Tail, (int)strlen(Tail), 0);
	}
	return;
}

// Compress using deflate. Caller must free output buffer after use

unsigned char * Compressit(unsigned char * In, int Len, int * OutLen)
{
	z_stream defstream;
	int maxSize;
	unsigned char * Out;

	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;

	defstream.avail_in = Len; // size of input
	defstream.next_in = (Bytef *)In; // input char array

	deflateInit(&defstream, Z_BEST_COMPRESSION);
	maxSize = deflateBound(&defstream, Len);

	Out = malloc(maxSize);

	defstream.avail_out = maxSize; // size of output
	defstream.next_out = (Bytef *)Out; // output char array

	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);

	*OutLen = defstream.total_out;

	return Out;
}


int InnerProcessHTTPMessage(struct ConnectionInfo * conn)
{
	struct TCPINFO * TCP = conn->TNC->TCPInfo;
	SOCKET sock = conn->socket;
	char * MsgPtr = conn->InputBuffer;
	int MsgLen = conn->InputLen;
	int InputLen = 0;
	int OutputLen = 0;
	int Bufferlen;
	struct HTTPConnectionInfo CI;
	struct HTTPConnectionInfo * sockptr = &CI;
	struct HTTPConnectionInfo * Session = NULL;

	char URL[100000];
	char * ptr;
	char * encPtr = 0;
	int allowDeflate = 0;
	char * Compressed = 0;
	char * HostPtr = 0;

	char * Context, * Method, * NodeURL = 0, * Key;
	char _REPLYBUFFER[250000];
	char Reply[250000];

	int ReplyLen = 0;
	char Header[256];
	int HeaderLen;
	char TimeString[64];
	BOOL LOCAL = FALSE;
	BOOL COOKIE = FALSE;
	int Len;
	char * WebSock = 0;

	char PortsHddr[] = "<h2 align=center>Ports</h2><table align=center border=2 bgcolor=white>"
		"<tr><th>Port</th><th>Driver</th><th>ID</th><th>Beacons</th><th>Driver Window</th><th>Stats Graph</th></tr>";

//	char PortLine[] = "<tr><td>%d</td><td><a href=PortStats?%d&%s>&nbsp;%s</a></td><td>%s</td></tr>";

	char PortLineWithBeacon[] = "<tr><td>%d</td><td><a href=PortStats?%d&%s>&nbsp;%s</a></td><td>%s</td>"
		"<td><a href=PortBeacons?%d>&nbsp;Beacons</a><td> </td></td><td>%s</td></tr>\r\n";

	char SessionPortLine[] = "<tr><td>%d</td><td>%s</td><td>%s</td><td> </td>"
		"<td> </td><td>%s</td></tr>\r\n";

	char PortLineWithDriver[] = "<tr><td>%d</td><td>%s</td><td>%s</td><td> </td>"
		"<td><a href=\"javascript:dev_win('/Node/Port?%d',%d,%d,%d,%d);\">Driver Window</a></td><td>%s</td></tr>\r\n";


	char PortLineWithBeaconAndDriver[] = "<tr><td>%d</td><td>%s</td><td>%s</td>"
		"<td><a href=PortBeacons?%d>&nbsp;Beacons</a></td>"
		"<td><a href=\"javascript:dev_win('/Node/Port?%d',%d,%d,%d,%d);\">Driver Window</a></td><td>%s</td></tr>\r\n";

	char RigControlLine[] = "<tr><td>%d</td><td>%s</td><td>%s</td><td> </td>"
		"<td><a href=\"javascript:dev_win('/Node/RigControl.html',%d,%d,%d,%d);\">Rig Control</a></td></tr>\r\n";


	char Encoding[] = "Content-Encoding: deflate\r\n";

#ifdef WIN32xx

	struct _EXCEPTION_POINTERS exinfo;
	strcpy(EXCEPTMSG, "ProcessHTTPMessage");

	__try {
#endif

		Len = (int)strlen(MsgPtr);
		if (Len > 100000)
			return 0; 

		strcpy(URL, MsgPtr);

		HostPtr = strstr(MsgPtr, "Host: ");

		WebSock = strstr(MsgPtr, "Upgrade: websocket");
		
		if (HostPtr)
		{
			uint32_t Host;
			char Hostname[32]= "";
			struct LOCALNET * LocalNet = conn->TNC->TCPInfo->LocalNets;
			
			HostPtr += 6;
			memcpy(Hostname, HostPtr, 31);
			strlop(Hostname, ':');
			Host = inet_addr(Hostname);

			if (strcmp(Hostname, "127.0.0.1") == 0)
				LOCAL = TRUE;
			else
			{
				if (conn->sin.sin_family != AF_INET6)
				{
					while(LocalNet)
					{
						uint32_t MaskedHost = conn->sin.sin_addr.s_addr & LocalNet->Mask;
						if (MaskedHost == LocalNet->Network)
						{				
							LOCAL = 1;
							break;
						}
						LocalNet = LocalNet->Next;
					}
				}
			}
		}

		encPtr = stristr(MsgPtr, "Accept-Encoding:");

		if (encPtr && stristr(encPtr, "deflate"))
			allowDeflate = 1;
		else
			Encoding[0] = 0;

		ptr = strstr(MsgPtr, "BPQSessionCookie=N");

		if (ptr)
		{
			COOKIE = TRUE;
			Key = ptr + 17;
			ptr = strchr(Key, ',');
			if (ptr)
			{
				*ptr = 0;
				Session = FindSession(Key);
				*ptr = ',';
			}
			else
			{
				ptr = strchr(Key, 13);
				if (ptr)
				{
					*ptr = 0;
					Session = FindSession(Key);
					*ptr = 13;
				}
			}
		}

		if (WebSock)
		{
			// Websock connection request - Reply and remember state.

			char KeyMsg[128];
			char Webx[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";	// Fixed UID
			char Hash[64] = "";
			char * Hash64;		// base 64 version
			char * ptr;

			//Sec-WebSocket-Key: l622yZS3n+zI+hR6SVWkPw==

			char ReplyMsg[] = 
				"HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: %s\r\n"
//				"Sec-WebSocket-Protocol: chat\r\n"
				"\r\n";

			ptr = strstr(MsgPtr, "Sec-WebSocket-Key:");

			if (ptr)
			{
				ptr += 18;
				while (*ptr == ' ')
					ptr++;

				memcpy(KeyMsg, ptr, 40);
				strlop(KeyMsg, 13);
				strlop(KeyMsg, ' ');
				strcat(KeyMsg, Webx);

				SHA1PasswordHash(KeyMsg, Hash);
				Hash64 = byte_base64_encode(Hash, 20);

				conn->WebSocks = 1;
				strlop(&URL[4], ' ');
				strcpy(conn->WebURL, &URL[4]);

				ReplyLen = sprintf(Reply, ReplyMsg, Hash64);
			
				free(Hash64);
				goto Returnit;

			}
		}


		ptr = strstr(URL, " HTTP");

		if (ptr)
			*ptr = 0;

		Method = strtok_s(URL, " ", &Context);

		memcpy(Mycall, &MYNODECALL, 10);
		strlop(Mycall, ' ');


		// Look for API messages

		if (_memicmp(Context, "/api/", 5) == 0 || _stricmp(Context, "/api") == 0)
		{
			char * Compressed;
			ReplyLen = APIProcessHTTPMessage(_REPLYBUFFER, Method, Context, MsgPtr, LOCAL, COOKIE);
				
			if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
			{
				// Full Message - just send it

				sendandcheck(sock, _REPLYBUFFER, ReplyLen);

				return 0;
			}

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: application/json\r\n"
				"Connection: close\r\n"
				"%s\r\n", ReplyLen, Encoding);

			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}


		// APRS process internally

		if (_memicmp(Context, "/APRS/", 6) == 0 || _stricmp(Context, "/APRS") == 0)
		{
			APRSProcessHTTPMessage(sock, MsgPtr, LOCAL, COOKIE);
			return 0;
		}


		if (_stricmp(Context, "/Node/Signon?Node") == 0)
		{
			char * IContext;

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			ProcessNodeSignon(sock, TCP, MsgPtr, Key, Reply, &Session, LOCAL);
			return 0;

		}

		// If for Mail or Chat, check for a session, and send login screen if necessary

		// Moved here to simplify operation with both internal and external clients

		if (_memicmp(Context, "/Mail/", 6) == 0)
		{
			int RLen = 0;
			char Appl;
			char * input;
			char * IContext;

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			if (_stricmp(NodeURL, "/Mail/Signon") == 0)
			{
				ReplyLen = ProcessMailSignon(TCP, MsgPtr, Key, Reply, &Session, FALSE, LOCAL);

				if (ReplyLen)
				{
					goto Returnit;
				}

#ifdef LINBPQ
				strcpy(Context, "/Mail/Header");
#else
				strcpy(MsgPtr, "POST /Mail/Header");
#endif
				goto doHeader;
			}

			if (_stricmp(NodeURL, "/Mail/Lost") == 0)
			{
				input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

				if (input && strstr(input, "Cancel=Exit"))
				{
					ReplyLen = SetupNodeMenu(Reply, LOCAL);
					RLen = ReplyLen;
					goto Returnit;
				}
				if (Key)
					Appl = Key[0];

				Key = 0;
			}

			if (Key == 0 || Key[0] == 0)
			{
				// No Session 

				// if not local send a signon screen, else create a user session

				if (LOCAL || COOKIE)
				{
					Session = AllocateSession(sock, 'M');

					if (Session)
					{
						strcpy(Context, "/Mail/Header");
						goto doHeader;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					RLen = ReplyLen;

					goto Returnit;

				}

				ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);

				RLen = ReplyLen;
				goto Returnit;
			}

			Session = FindSession(Key);

	
			if (Session == NULL && _memicmp(Context, "/Mail/API/", 10) != 0)
			{
				ReplyLen = sprintf(Reply, MailLostSession, Key);
				RLen = ReplyLen;
				goto Returnit;
			}
		}

		if (_memicmp(Context, "/Chat/", 6) == 0)
		{
			int RLen = 0;
			char Appl;
			char * input;
			char * IContext;


			HostPtr = strstr(MsgPtr, "Host: ");

			if (HostPtr)
			{
				uint32_t Host;
				char Hostname[32]= "";
				struct LOCALNET * LocalNet = conn->TNC->TCPInfo->LocalNets;

				HostPtr += 6;
				memcpy(Hostname, HostPtr, 31);
				strlop(Hostname, ':');
				Host = inet_addr(Hostname);

				if (strcmp(Hostname, "127.0.0.1") == 0)
					LOCAL = TRUE;
				else while(LocalNet)
				{
					uint32_t MaskedHost = Host & LocalNet->Mask;
					if (MaskedHost == LocalNet->Network)
					{				
						char * rest;
						LOCAL = 1;
						rest = strchr(HostPtr, 13);
						if (rest)
						{
							memmove(HostPtr + 9, rest, strlen(rest) + 1);
							memcpy(HostPtr, "127.0.0.1", 9);
							break;
						}
					}
					LocalNet = LocalNet->Next;
				}
			}

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			if (_stricmp(NodeURL, "/Chat/Signon") == 0)
			{
				ReplyLen = ProcessChatSignon(TCP, MsgPtr, Key, Reply, &Session, LOCAL);

				if (ReplyLen)
				{
					goto Returnit;
				}

#ifdef LINBPQ
				strcpy(Context, "/Chat/Header");
#else
				strcpy(MsgPtr, "POST /Chat/Header");
#endif
				goto doHeader;
			}

			if (_stricmp(NodeURL, "/Chat/Lost") == 0)
			{
				input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

				if (input && strstr(input, "Cancel=Exit"))
				{
					ReplyLen = SetupNodeMenu(Reply, LOCAL);
					RLen = ReplyLen;
					goto Returnit;
				}
				if (Key)
					Appl = Key[0];

				Key = 0;
			}

			if (Key == 0 || Key[0] == 0)
			{
				// No Session 

				// if not local send a signon screen, else create a user session

				if (LOCAL || COOKIE)
				{
					Session = AllocateSession(sock, 'C');

					if (Session)
					{
						strcpy(Context, "/Chat/Header");
						goto doHeader;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					RLen = ReplyLen;

					goto Returnit;

				}

				ReplyLen = sprintf(Reply, ChatSignon, Mycall, Mycall);

				RLen = ReplyLen;
				goto Returnit;
			}

			Session = FindSession(Key);

			if (Session == NULL)
			{
				int Sent, Loops = 0;
				ReplyLen = sprintf(Reply, MailLostSession, Key);
				RLen = ReplyLen;
Returnit:
				if (memcmp(Reply, "HTTP", 4) == 0)
				{
					// Full Header provided by appl - just send it

					// Send may block

					Sent = send(sock, Reply, ReplyLen, 0);

					while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
					{	
						//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

						if (Sent > 0)					// something sent
						{
							InputLen -= Sent;
							memmove(Reply, &Reply[Sent], ReplyLen);
						}

						Sleep(30);
						Sent = send(sock, Reply, ReplyLen, 0);
					}

					return 0;
				}

				if (NodeURL && _memicmp(NodeURL, "/mail/api/", 10) != 0)
				{
					// Add tail

					strcpy(&Reply[ReplyLen], Tail);
					ReplyLen += strlen(Tail);
				}

				// compress if allowed
				
				if (allowDeflate)
					Compressed = Compressit(Reply, ReplyLen, &ReplyLen);
				else
					Compressed = Reply;

				if (NodeURL && _memicmp(NodeURL, "/mail/api/", 10) == 0)
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\n"
						"Content-Length: %d\r\n"
						"Content-Type: application/json\r\n"
						"Connection: close\r\n"
						"%s\r\n", ReplyLen, Encoding);
				else
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n%s\r\n", ReplyLen, Encoding);
				
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, Compressed, ReplyLen);

				if (allowDeflate)
					free (Compressed);

				return 0;
			}
		}

doHeader:

#ifdef LINBPQ

		if ((_memicmp(Context, "/MAIL/", 6) == 0) || (_memicmp(Context, "/WebMail", 8) == 0))
		{
			char _REPLYBUFFER[250000];
			struct HTTPConnectionInfo Dummy = {0};
			int Sent, Loops = 0;

			ReplyLen = 0;

			if (Session == 0)
				Session = &Dummy;

			Session->TNC = (void *)LOCAL;		// TNC only used for Web Terminal Sessions

			ProcessMailHTTPMessage(Session, Method, Context, MsgPtr, _REPLYBUFFER, &ReplyLen, MsgLen);

			if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
			{
				// Full Header provided by appl - just send it

				// Send may block

				Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);

				while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
				{	
					//				Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

					if (Sent > 0)					// something sent
					{
						InputLen -= Sent;
						memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
					}	

					Sleep(30);
					Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);
				}
				return 0;
			}

			// Add tail

			strcpy(&_REPLYBUFFER[ReplyLen], Tail);
			ReplyLen += strlen(Tail);

			// compress if allowed
				
			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = Reply;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;


/*

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);

			// Send may block

			Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);

			if (Sent == -1)
				return 0;

			while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
			{	
				//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

				if (Sent > 0)					// something sent
				{
					InputLen -= Sent;
					memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
				}

				Sleep(30);
				Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);
			}

			send(sock, Tail, (int)strlen(Tail), 0);
			return 0;
*/
		}

		if (_memicmp(Context, "/CHAT/", 6) == 0)
		{
			char _REPLYBUFFER[100000];

			ReplyLen = 0;

			ProcessChatHTTPMessage(Session, Method, Context, MsgPtr, _REPLYBUFFER, &ReplyLen);

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);
			send(sock, _REPLYBUFFER, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);

			return 0;

		}


		/*
		Sent = send(sock, _REPLYBUFFER, InputLen, 0);

		while (Sent != InputLen && Loops++ < 3000)					// 100 secs max
		{	
		//					Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

		if (Sent > 0)					// something sent
		{
		InputLen -= Sent;
		memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], InputLen);
		}

		Sleep(30);
		Sent = send(sock, _REPLYBUFFER, InputLen, 0);
		}
		return 0;
		}
		*/
#else

		// Pass to MailChat if active

		if ((_memicmp(Context, "/MAIL/", 6) == 0) || (_memicmp(Context, "/WebMail", 8) == 0))
		{
			// If for Mail, Pass to Mail Server via Named Pipe

			HANDLE hPipe;

			hPipe = CreateFile(MAILPipeFileName, GENERIC_READ | GENERIC_WRITE,
				0,                    // exclusive access
				NULL,                 // no security attrs
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				NULL );

			if (hPipe == (HANDLE)-1)
			{
				InputLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 28\r\n\r\nMail Data is not available\r\n");
				send(sock, Reply, InputLen, 0);
			}
			else
			{
				//			int Sent;
				int Loops = 0;
				struct HTTPConnectionInfo Dummy = {0};

				if (Session == 0)
					Session = &Dummy;

				Session->TNC = LOCAL;		// TNC is only used on Web Terminal Sessions so can reuse as LOCAL flag

				WriteFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				WriteFile(hPipe, MsgPtr, MsgLen, &InputLen, NULL);


				ReadFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				ReadFile(hPipe, Reply, 250000, &ReplyLen, NULL);
				if (ReplyLen <= 0)
				{
					InputLen = GetLastError();
				}

				CloseHandle(hPipe);
				goto Returnit;
			}
			return 0;
		}

		if (_memicmp(Context, "/CHAT/", 6) == 0)
		{
			HANDLE hPipe;

			hPipe = CreateFile(CHATPipeFileName, GENERIC_READ | GENERIC_WRITE,
				0,                    // exclusive access
				NULL,                 // no security attrs
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				NULL );

			if (hPipe == (HANDLE)-1)
			{
				InputLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 28\r\n\r\nChat Data is not available\r\n");
				send(sock, Reply, InputLen, 0);
			}
			else
			{
				//			int Sent;
				int Loops = 0;

				WriteFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				WriteFile(hPipe, MsgPtr, MsgLen, &InputLen, NULL);


				ReadFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				ReadFile(hPipe, Reply, 100000, &ReplyLen, NULL);
				if (ReplyLen <= 0)
				{
					InputLen = GetLastError();
				}

				CloseHandle(hPipe);
				goto Returnit;
			}
			return 0;
		}

#endif

		NodeURL = strtok_s(NULL, "?", &Context);

		if (NodeURL == NULL)
			return 0;

		if (strcmp(Method, "POST") == 0)
		{
			if (_stricmp(NodeURL, "/Node/freqOffset") == 0)
			{
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int port = atoi(Context);

				if (input == 0)
					return 1;

				input += 4;
	
				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];
					char value[6];

					if (TNC == 0)
						return 1;

					TNC->TXOffset = atoi(input);
#ifdef WIN32
					sprintf(value, "%d", TNC->TXOffset);
					MySetWindowText(TNC->xIDC_TXTUNEVAL, value);
					SendMessage(TNC->xIDC_TXTUNE, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) TNC->TXOffset);  // min. & max. positions

#endif
				}
				return 1;
			}

			if (_stricmp(NodeURL, "/Node/PortAction") == 0)
			{
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int port = atoi(Context);

				if (input == 0)
					return 1;

				input += 4;

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC == 0)
						return 1;

					if (LOCAL == FALSE && COOKIE == FALSE)
						return 1;

					if (strcmp(input, "Abort") == 0)
					{
						if (TNC->ForcedCloseProc)
							TNC->ForcedCloseProc(TNC, 0);
					}
					else if (strcmp(input, "Kill") == 0)
					{
						TNC->DontRestart = TRUE;
						KillTNC(TNC);
					}
					else if (strcmp(input, "KillRestart") == 0)
					{
						TNC->DontRestart = FALSE;
						KillTNC(TNC);
						RestartTNC(TNC);

					}
				}
				return 1;
			}

			if (_stricmp(NodeURL, "/TermInput") == 0)
			{
				ProcessTermInput(sock, MsgPtr, MsgLen, Context);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/TermSignon") == 0)
			{
				ProcessTermSignon(conn->TNC, sock, MsgPtr, MsgLen, LOCAL);
			}

			if (_stricmp(NodeURL, "/Node/Signon") == 0)
			{
				ProcessNodeSignon(sock, TCP, MsgPtr, Key, Reply, &Session, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/TermClose") == 0)
			{
				ProcessTermClose(sock, MsgPtr, MsgLen, Context, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/BeaconAction") == 0)
			{
				char Header[256];
				int HeaderLen;
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int Port;
				char Param[100];
#ifndef LINBPQ
				int retCode, disp;
				char Key[80];
				HKEY hKey;
#endif
				struct PORTCONTROL * PORT;
				int Slot = 0;


				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return 1;
				}

				if (strstr(input, "Cancel=Cancel"))
				{
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return 1;
				}

				GetParam(input, "Port=", &Param[0]);
				Port = atoi(&Param[0]);
				PORT = GetPortTableEntryFromPortNum(Port); // Need slot not number
				if (PORT)
					Slot = PORT->PortSlot;

				GetParam(input, "Every=", &Param[0]);
				Interval[Slot] = atoi(&Param[0]);

				GetParam(input, "Dest=", &Param[0]);
				_strupr(Param);
				strcpy(UIUIDEST[Slot], &Param[0]);

				GetParam(input, "Path=", &Param[0]);
				_strupr(Param);
				if (UIUIDigi[Slot])
					free(UIUIDigi[Slot]);
				UIUIDigi[Slot] = _strdup(&Param[0]);

				GetParam(input, "File=", &Param[0]);
				strcpy(FN[Slot], &Param[0]);
				GetParam(input, "Text=", &Param[0]);
				strcpy(Message[Slot], &Param[0]);

				MinCounter[Slot] = Interval[Slot];

				SendFromFile[Slot] = 0;

				if (FN[Slot][0])
					SendFromFile[Slot] = 1;

				SetupUI(Slot);

#ifdef LINBPQ
				SaveUIConfig();
#else
				SaveUIConfig();

				wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", Port);

				retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

				if (retCode == ERROR_SUCCESS)
				{
					retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIUIDEST[Port][0], strlen(&UIUIDEST[Port][0]));
					retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
					retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
					retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
					retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
					retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, UIUIDigi[Port], strlen(UIUIDigi[Port]));

					RegCloseKey(hKey);
				}
#endif
				if (strstr(input, "Test=Test"))
					SendUIBeacon(Slot);


				ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], Beacons, Port,		
					Interval[Slot], &UIUIDEST[Slot][0], &UIUIDigi[Slot][0], &FN[Slot][0], &Message[Slot][0], Port);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);
				send(sock, Tail, (int)strlen(Tail), 0);

				return 1;
			}

			if (_stricmp(NodeURL, "/Node/CfgSave") == 0)
			{
				//	Save Config File

				SaveConfigFile(sock, MsgPtr, Key, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/LogAction") == 0)
			{
				ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);
				send(sock, Tail, (int)strlen(Tail), 0);

				return 1;
			}


			if (_stricmp(NodeURL, "/Node/ARDOPAbort") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC && TNC->ForcedCloseProc)
						TNC->ForcedCloseProc(TNC, 0);


					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);


					ReplyLen = sprintf(Reply, "<html><script>alert(\"%s\");window.close();</script></html>", "Ok");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					//				goto SendResp;

					//				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + strlen(Tail));
					//				send(sock, Header, HeaderLen, 0);
					//				send(sock, _REPLYBUFFER, ReplyLen, 0);
					//				send(sock, Tail, strlen(Tail), 0);

					return 1;
				}

			}

			send(sock, _REPLYBUFFER, InputLen, 0);
			return 0;
		}

		if (_stricmp(NodeURL, "/") == 0 || _stricmp(NodeURL, "/Index.html") == 0)
		{		
			// Send if present, else use default

			Bufferlen = SendMessageFile(sock, NodeURL, TRUE, allowDeflate);		// return -1 if not found

			if (Bufferlen != -1)
				return 0;						// We've sent it
			else
			{	
				if (APRSApplConnected)
					ReplyLen = sprintf(_REPLYBUFFER, Index, Mycall, Mycall);
				else
					ReplyLen = sprintf(_REPLYBUFFER, IndexNoAPRS, Mycall, Mycall);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);
				send(sock, Tail, (int)strlen(Tail), 0);

				return 0;
			}
		}

		else if (_stricmp(NodeURL, "/NodeMenu.html") == 0 || _stricmp(NodeURL, "/Node/NodeMenu.html") == 0)
		{		
			// Send if present, else use default

			char Menu[] = "/NodeMenu.html";

			Bufferlen = SendMessageFile(sock, Menu, TRUE, allowDeflate);		// return -1 if not found

			if (Bufferlen != -1)
				return 0;											// We've sent it
		}

		else if (_memicmp(NodeURL, "/aisdata.txt", 12) == 0)
		{
			char * Compressed;
			ReplyLen = GetAISPageInfo(_REPLYBUFFER, 1, 1);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}

		else if (_memicmp(NodeURL, "/aprsdata.txt", 13) == 0)
		{
			char * Compressed;
			char * ptr;
			double N, S, W, E;
			int aprs = 1, ais = 1, adsb = 1;

			ptr = &NodeURL[14];
			
			N = atof(ptr);
			ptr = strlop(ptr, '|');
			S = atof(ptr);
			ptr = strlop(ptr, '|');
			W = atof(ptr);
			ptr = strlop(ptr, '|');
			E = atof(ptr);
			ptr = strlop(ptr, '|');	
			if (ptr)
			{
			aprs = atoi(ptr);
			ptr = strlop(ptr, '|');		
			ais = atoi(ptr);
			ptr = strlop(ptr, '|');		
			adsb = atoi(ptr);
			}
			ReplyLen = GetAPRSPageInfo(_REPLYBUFFER, N, S, W, E, aprs, ais, adsb);

			if (ReplyLen < 240000)
				ReplyLen += GetAISPageInfo(&_REPLYBUFFER[ReplyLen], ais, adsb);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}

		else if (_memicmp(NodeURL, "/portstats.txt", 15) == 0)
		{
			char * Compressed;
			char * ptr;
			int port;
			struct PORTCONTROL * PORT;

			ptr = &NodeURL[15];
			
			port = atoi(ptr);

			PORT = GetPortTableEntryFromPortNum(port);

			ReplyLen = 0;

			if (PORT && PORT->TX)
			{
				// We send the last 24 hours worth of data. Buffer is cyclic so oldest byte is at StatsPointer

				int first = PORT->StatsPointer;
				int firstlen = 1440 - first;

				memcpy(&_REPLYBUFFER[0], &PORT->TX[first], firstlen);
				memcpy(&_REPLYBUFFER[firstlen], PORT->TX, first);

				memcpy(&_REPLYBUFFER[1440], &PORT->BUSY[first], firstlen);
				memcpy(&_REPLYBUFFER[1440 + firstlen], PORT->BUSY, first);

				ReplyLen = 2880;
			}

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}


		else if (_memicmp(NodeURL, "/Icon", 5) == 0 && _memicmp(&NodeURL[10], ".png", 4) == 0)
		{
			// APRS internal Icon

			char * Compressed;
				
			ReplyLen = GetAPRSIcon(_REPLYBUFFER, NodeURL);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;

		}

		else if (_memicmp(NodeURL, "/NODE/", 6))
		{
			// Not Node, See if a local file

			Bufferlen = SendMessageFile(sock, NodeURL, FALSE, allowDeflate);		// Send error if not found
			return 0;
		}

		// Node URL

		{

			ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);

			if (_stricmp(NodeURL, "/Node/webproc.css") == 0)
			{
				char WebprocCSS[] =
					".dropbtn {position: relative; border: 1px solid black;padding:1px;}\r\n"
					".dropdown {position: relative; display: inline-block;}\r\n"
					".dropdown-content {display: none; position: absolute;background-color: #ccc; "
					"min-width: 160px; box-shadow: 0px 8px 16px 0px rgba(0,0,00.2); z-index: 1;}\r\n"
					".dropdown-content a {color: black; padding: 1px 1px;text-decoration:none;display:block;}"
					".dropdown-content a:hover {background-color: #dddfff;}\r\n"
					".dropdown:hover .dropdown-content {display: block;}\r\n"
					".dropdown:hover .dropbtn {background-color: #ddd;}\r\n"
					"input.btn:active {background:black;color:white;}\r\n"
					"submit.btn:active {background:black;color:white;}\r\n";
				ReplyLen = sprintf(_REPLYBUFFER, "%s", WebprocCSS);
			}

			else if (_stricmp(NodeURL, "/Node/Killandrestart") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					KillTNC(TNC);
					TNC->DontRestart = FALSE;
					RestartTNC(TNC);

					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);

				}
			}

			else if (_stricmp(NodeURL, "/Node/Port") == 0 || _stricmp(NodeURL, "/Node/ARDOPAbort") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);
				}

			}

			else if (_stricmp(NodeURL, "/Node/Streams") == 0)
			{
				ReplyLen = StatusProc(_REPLYBUFFER);
			}

			else if (_stricmp(NodeURL, "/Node/Stats.html") == 0)
			{
				struct tm * TM;
				char UPTime[50];
				time_t szClock = STATSTIME * 60;

				TM = gmtime(&szClock);

				sprintf(UPTime, "%.2d:%.2d:%.2d", TM->tm_yday, TM->tm_hour, TM->tm_min);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", StatsHddr);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%s</td></tr>",
					"Version", VersionString);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%s</td></tr>",
					"Uptime (Days Hours Mins)", UPTime);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td></tr>",
					"Semaphore: Get-Rel/Clashes", Semaphore.Gets - Semaphore.Rels, Semaphore.Clashes);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td align=right><td align=right>%d</td></tr>",
					"Buffers: Max/Cur/Min/Out/Wait", MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td></tr>",
					"Known Nodes/Max Nodes", NUMBEROFNODES, MAXDESTS);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td></tr>",
					"L4 Connects Sent/Rxed ", L4CONNECTSOUT, L4CONNECTSIN);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td align=right><td align=right>%d</td></tr>",
					"L4 Frames TX/RX/Resent/Reseq", L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%s</td><td align=right>%d</td></tr>",
					"L3 Frames Relayed", L3FRAMES);

			}

			else if (_stricmp(NodeURL, "/Node/RigControl.html") == 0)
			{
				char Test[] =
					"<html><meta http-equiv=expires content=0>\r\n"
					"<head><title>Rigcontrol</title></head>\r\n"
					"<script type = \"text/javascript\">\r\n"
					"var ws;"
					"function WebSocketTest()"
					"{"
					" if (\"WebSocket\" in window)"
					" {"
					"   // Let us open a web socket. Get address from URL\r\n"
					"	var text = window.location.href;"
					"	var result = text.substring(7);"
					"	var myArray = result.split('/', 1);"
					"   ws = new WebSocket('ws://' + myArray[0] + '/RIGCTL');\r\n"
					
					"   ws.onopen = function() {\r\n"
			
					"   // Web Socket is connected\r\n"

					"	const div = document.getElementById('div');\r\n"
					"	div.innerHTML = 'Websock Connected'\r\n"
					"    };\r\n"
					
					"   ws.onmessage = function (evt)"
					"   {"
					"     var received_msg = evt.data;\r\n"
					"	  const div = document.getElementById('div');\r\n"
					"	  div.innerHTML = received_msg\r\n"
					"     };"

					"   ws.onclose = function()"
					"   {"

					"    // websocket is closed.\r\n"
					"	 const div = document.getElementById('div');\r\n"
					"	 div.innerHTML = 'Websock Connection Lost'\r\n"
					"    };"
					" }"
					" else"
					" {"
					"  // The browser doesn't support WebSocket\r\n"
					"	const div = document.getElementById('div');\r\n"
					"	div.innerHTML = 'WebSocket not supported by your Browser - RigControl Page not availible'\r\n"
					" }"
					"}"
					"function PTT(p)"
					"{"
					"  ws.send(p);"
					"}" 
					"</script>\r\n"
					"</head>\r\n"
					"<body height: 600px; onload=WebSocketTest()>\r\n"
					"<div id = 'div'>Waiting for data...</div>\r\n"
					"</body></html>\r\n";

		
				char NoRigCtl[] =
					"<html><meta http-equiv=expires content=0>\r\n"
					"<head><title>Rigcontrol</title></head>\r\n"
					"</head>\r\n"
					"<body height: 600px>\r\n"
					"<div id = 'div'>RigControl Not Configured...</div>\r\n"
					"</body></html>\r\n";

				if (RigWebPage)
					ReplyLen = sprintf(_REPLYBUFFER, "%s", Test);
				else
					ReplyLen = sprintf(_REPLYBUFFER, "%s", NoRigCtl);
			}

			else if (_stricmp(NodeURL, "/Node/ShowLog.html") == 0)
			{
				char ShowLogPage[] =
					"<html><script>"
					"function myResize() {"
					" var h = document.getElementById('outer').clientHeight;"
					" var offsets = document.getElementById('log').getBoundingClientRect();"
					" document.getElementById('log').style.height = h - offsets.top;}"
					"</script>"
					"<head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
					"<title>Log Display</title></head>"
					"<body style=\"margin: 4;\" background=/background.jpg onload='myResize()' onresize='myResize()'>"
					"<div id=outer style=\"width: 100%%; height: 100%%;\">"
					"<form id = form><input name=input value=Back type=submit class='btn'>"
//					"<form id = doDate><input type=date value=Date name='date'><input type='submit'>"
					"</form>"
					"<textarea id=log style=\"box-sizing: border-box; overflow: auto; white-space: pre; width: 100%%; height: auto\" name=Msg>%s</textarea>"
					"</div>";

				char * _REPLYBUFFER;
				int ReplyLen;
				char Header[256];
				int HeaderLen;
				char * CfgBytes;
				int CfgLen;
				char inputname[250];
				FILE *fp1;
				struct stat STAT;
				char DummyKey[] = "DummyKey";
				time_t T;
				struct tm * tm;
				char Name[64] = "";

				T = time(NULL);
				tm = gmtime(&T);

				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					char _REPLYBUFFER[4096];	
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (COOKIE == FALSE)
					Key = DummyKey;

				if (memcmp(Context, "date=", 5) == 0)
				{
					memset(tm, 0, sizeof(struct tm));
					tm->tm_year = atoi(&Context[5]) - 1900;
					tm->tm_mon = atoi(&Context[10]) - 1;
					tm->tm_mday = atoi(&Context[13]);
				}



				if (strcmp(Context, "input=Back") == 0)
				{
					ReplyLen = SetupNodeMenu(Reply, LOCAL);
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (LogDirectory[0] == 0)
				{
					strcpy(inputname, "logs/");
				}
				else
				{
					strcpy(inputname,LogDirectory);
					strcat(inputname,"/");
					strcat(inputname, "/logs/");
				}

				if (strstr(Context, "CMS"))
				{
					sprintf(Name, "CMSAccess_%04d%02d%02d.log",
						tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Debug"))
				{
					sprintf(Name, "log_%02d%02d%02d_DEBUG.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "BBS"))
				{
					sprintf(Name, "log_%02d%02d%02d_BBS.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Chat"))
				{
					sprintf(Name, "log_%02d%02d%02d_CHAT.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Telnet"))
				{
					sprintf(Name, "Telnet_%02d%02d%02d.log",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}

				strcat(inputname, Name);

				if (stat(inputname, &STAT) == -1)
				{
					CfgBytes = malloc(256);
					sprintf(CfgBytes, "Log %s not found", inputname);
					CfgLen = strlen(CfgBytes);
				}
				else
				{
					fp1 = fopen(inputname, "rb");

					if (fp1 == 0)
					{
						CfgBytes = malloc(256);
						sprintf(CfgBytes, "Log %s not found", inputname);
						CfgLen = strlen(CfgBytes);
					}
					else
					{	
						CfgLen = STAT.st_size;

						CfgBytes = malloc(CfgLen + 1);

						CfgLen = (int)fread(CfgBytes, 1, CfgLen, fp1);
						CfgBytes[CfgLen] = 0;
					}		
				}

				_REPLYBUFFER = malloc(CfgLen + 1000);

				ReplyLen = sprintf(_REPLYBUFFER, ShowLogPage, CfgBytes);
				free (CfgBytes);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);
				sendandcheck(sock, Tail, (int)strlen(Tail));
				free (_REPLYBUFFER);

				return 1;
			}

			else if (_stricmp(NodeURL, "/Node/EditCfg.html") == 0)
			{
				char * _REPLYBUFFER;
				int ReplyLen;
				char Header[256];
				int HeaderLen;
				char * CfgBytes;
				int CfgLen;
				char inputname[250]="bpq32.cfg";
				FILE *fp1;
				struct stat STAT;
				char DummyKey[] = "DummyKey";

				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					char _REPLYBUFFER[4096];	
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (COOKIE ==FALSE)
					Key = DummyKey;

				if (ConfigDirectory[0] == 0)
				{
					strcpy(inputname, "bpq32.cfg");
				}
				else
				{
					strcpy(inputname,ConfigDirectory);
					strcat(inputname,"/");
					strcat(inputname, "bpq32.cfg");
				}


				if (stat(inputname, &STAT) == -1)
				{
					CfgBytes = _strdup("Config File not found");
				}
				else
				{
					fp1 = fopen(inputname, "rb");

					if (fp1 == 0)
					{
						CfgBytes = _strdup("Config File not found");
					}
					else
					{	
						CfgLen = STAT.st_size;

						CfgBytes = malloc(CfgLen + 1);

						CfgLen = (int)fread(CfgBytes, 1, CfgLen, fp1);
						CfgBytes[CfgLen] = 0;
					}		
				}

				_REPLYBUFFER = malloc(CfgLen + 1000);

				ReplyLen = sprintf(_REPLYBUFFER, ConfigEditPage, Key, CfgBytes);
				free (CfgBytes);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + (int)strlen(Tail));
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);
				sendandcheck(sock, Tail, (int)strlen(Tail));
				free (_REPLYBUFFER);

				return 1;
			}



			if (_stricmp(NodeURL, "/Node/PortBeacons") == 0)
			{
				char * PortChar = strtok_s(NULL, "&", &Context);
				int PortNo = atoi(PortChar);
				struct PORTCONTROL * PORT;
				int PortSlot = 0;

				PORT = GetPortTableEntryFromPortNum(PortNo); // Need slot not number
				if (PORT)
					PortSlot = PORT->PortSlot;


				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], Beacons, PortNo,
					Interval[PortSlot], &UIUIDEST[PortSlot][0], &UIUIDigi[PortSlot][0], &FN[PortSlot][0], &Message[PortSlot][0], PortNo);
			}



			if (_stricmp(NodeURL, "/Node/PortStats") == 0)
			{
				struct _EXTPORTDATA * Port;

				char * PortChar = strtok_s(NULL, "&", &Context);
				int PortNo = atoi(PortChar);
				int Protocol;
				int PortType;

				//		char PORTTYPE;	// H/W TYPE
				// 0 = ASYNC, 2 = PC120, 4 = DRSI
				// 6 = TOSH, 8 = QUAD, 10 = RLC100
				// 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

#define KISS 0
#define NETROM 2
#define HDLC 6
#define L2 8
#define WINMOR 10


				// char PROTOCOL;	// PORT PROTOCOL
				// 0 = KISS, 2 = NETROM, 4 = BPQKISS
				//; 6 = HDLC, 8 = L2


				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsHddr, PortNo);

				Port = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(PortNo);

				if (Port == NULL)
				{
					ReplyLen = sprintf(_REPLYBUFFER, "Invalid Port");
					goto SendResp;
				}

				Protocol = Port->PORTCONTROL.PROTOCOL;
				PortType = Port->PORTCONTROL.PROTOCOL;

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Digied", Port->PORTCONTROL.L2DIGIED);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Heard", Port->PORTCONTROL.L2FRAMES);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Received", Port->PORTCONTROL.L2FRAMESFORUS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Sent", Port->PORTCONTROL.L2FRAMESSENT);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Timeouts", Port->PORTCONTROL.L2TIMEOUTS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "REJ Frames Received", Port->PORTCONTROL.L2REJCOUNT);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX out of Seq", Port->PORTCONTROL.L2OUTOFSEQ);
				//		ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Resequenced", Port->PORTCONTROL.L2RESEQ);
				if (Protocol == HDLC)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Underrun", Port->PORTCONTROL.L2URUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX Overruns", Port->PORTCONTROL.L2ORUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Frames abandoned", Port->PORTCONTROL.L1DISCARD);
				}
				else if ((Protocol == KISS && Port->PORTCONTROL.KISSFLAGS) || Protocol == NETROM)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Poll Timeout", Port->PORTCONTROL.L2URUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "FRMRs Sent", Port->PORTCONTROL.L2FRMRTX);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "FRMRs Received", Port->PORTCONTROL.L2FRMRRX);

				//		DB	'Link Active %   '
				//		DD	AVSENDING

			}

			if (_stricmp(NodeURL, "/Node/Ports.html") == 0)
			{
				struct _EXTPORTDATA * ExtPort;
				struct PORTCONTROL * Port;

				int count;
				char DLL[20];
				char StatsURL[64];

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PortsHddr);

				for (count = 1; count <= NUMBEROFPORTS; count++)
				{
					Port = GetPortTableEntryFromSlot(count);
					ExtPort = (struct _EXTPORTDATA *)Port;

					// see if has a stats page

					if (Port->AVACTIVE)
						sprintf(StatsURL, "<a href=/PortStats.html?%d>&nbsp;Stats Graph</a>", Port->PORTNUMBER);
					else
						StatsURL[0] = 0;

					if (Port->PORTTYPE == 0x10)
					{	
						strcpy(DLL, ExtPort->PORT_DLL_NAME);
						strlop(DLL, '.');
					}
					else if (Port->PORTTYPE == 0)
						strcpy(DLL, "ASYNC");

					else if (Port->PORTTYPE == 22)
						strcpy(DLL, "I2C");

					else if (Port->PORTTYPE == 14)
						strcpy(DLL, "INTERNAL");

					else if (Port->PORTTYPE > 0 && Port->PORTTYPE < 14)
						strcpy(DLL, "HDLC");


					if (Port->TNC && Port->TNC->WebWindowProc)		// Has a Window
					{
						if (Port->UICAPABLE)
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithBeaconAndDriver, Port->PORTNUMBER, DLL,
							Port->PORTDESCRIPTION, Port->PORTNUMBER, Port->PORTNUMBER, Port->TNC->WebWinX, Port->TNC->WebWinY, 200, 200, StatsURL);
						else
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithDriver, Port->PORTNUMBER, DLL,
							Port->PORTDESCRIPTION, Port->PORTNUMBER, Port->TNC->WebWinX, Port->TNC->WebWinY, 200, 200, StatsURL);

						continue;
					}

					if (Port->PORTTYPE == 16 && Port->PROTOCOL == 10 && Port->UICAPABLE == 0)		// EXTERNAL, Pactor/WINMO
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], SessionPortLine, Port->PORTNUMBER, DLL,
						Port->PORTDESCRIPTION, Port->PORTNUMBER, StatsURL);
					else
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithBeacon, Port->PORTNUMBER, Port->PORTNUMBER,
						DLL, DLL, Port->PORTDESCRIPTION, Port->PORTNUMBER, StatsURL);
				}

				if (RigActive)
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RigControlLine, 64, "Rig Control", "Rig Control", 600, 350, 200, 200);

			}

			if (_stricmp(NodeURL, "/Node/Nodes.html") == 0)
			{
				struct DEST_LIST * Dests = DESTS;
				int count, i;
				char Normcall[10];
				char Alias[10];
				int Width = 5;
				int x = 0, n = 0;
				struct DEST_LIST * List[1000];
				char Param = 0;

				if (Context)
				{
					_strupr(Context);
					Param = Context[0];
				}

				for (count = 0; count < MAXDESTS; count++)
				{
					if (Dests->DEST_CALL[0] != 0)
					{
						if (Param != 'T' || Dests->DEST_COUNT)
							List[n++] = Dests;

						if (n > 999)
							break;
					}

					Dests++;
				}

				if (n > 1)
				{
					if (Param == 'C') 
						qsort(List, n, sizeof(void *), CompareNode);
					else
						qsort(List, n, sizeof(void *), CompareAlias);
				}

				Alias[6] = 0; 

				if (Param == 'T')
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "with traffic");
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<td>Call</td><td>Frames</td><td>RTT</td><td>BPQ?</td><td>Hops</td></tr><tr>");
				}
				else if (Param == 'C') 
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "sorted by Call");
				else
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "sorted by Alias");

				for (i = 0; i < n; i++)
				{
					int len = ConvFromAX25(List[i]->DEST_CALL, Normcall);
					Normcall[len]=0;

					memcpy(Alias, List[i]->DEST_ALIAS, 6);
					strlop(Alias, ' ');

					if (Param == 'T')
					{
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<td>%s:%s</td><td align=center>%d</td><td align=center>%d</td><td align=center>%c</td><td align=center>%.0d</td></tr><tr>",
							Normcall, Alias, List[i]->DEST_COUNT, List[i]->DEST_RTT /16,
							(List[i]->DEST_STATE & 0x40)? 'B':' ', (List[i]->DEST_STATE & 63));

					}
					else
					{
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeLine, Normcall, Alias, Normcall);

						if (++x == Width)
						{
							x = 0;
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tr><tr>");
						}
					}
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tr>");
			}

			if (_stricmp(NodeURL, "/Node/NodeDetail") == 0)
			{
				UCHAR AXCall[8];
				struct DEST_LIST * Dest = DESTS;
				struct NR_DEST_ROUTE_ENTRY * NRRoute;
				struct ROUTE * Neighbour;
				char Normcall[10];
				int i, len, count, Active;
				char Alias[7];

				Alias[6] = 0;

				_strupr(Context);

				ConvToAX25(Context, AXCall);

				for (count = 0; count < MAXDESTS; count++)
				{
					if (CompareCalls(Dest->DEST_CALL, AXCall))
					{
						break;
					}
					Dest++;
				}

				if (count == MAXDESTS)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<h3 align = center>Call %s not found</h3>", Context);
					goto SendResp;
				}

				memcpy(Alias, Dest->DEST_ALIAS, 6);
				strlop(Alias, ' ');

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen],
					"<h3 align=center>Info for Node %s:%s</h3><p style=font-family:monospace align=center>", Alias, Context);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<table border=1 bgcolor=white><tr><td>Frames</td><td>RTT</td><td>BPQ?</td><td>Hops</td></tr>");	

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td align=center>%d</td><td align=center>%d</td><td align=center>%c</td><td align=center>%.0d</td></tr></table>",
					Dest->DEST_COUNT, Dest->DEST_RTT /16,
					(Dest->DEST_STATE & 0x40)? 'B':' ', (Dest->DEST_STATE & 63));

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<h3 align=center>Neighbours</h3>");

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], 
					"<table border=1 style=font-family:monospace align=center bgcolor=white>"
					"<tr><td> </td><td> Qual </td><td> Obs </td><td> Port </td><td> Call </td></tr>");	

				NRRoute = &Dest->NRROUTE[0];

				Active = Dest->DEST_ROUTE;

				for (i = 1; i < 4; i++)
				{
					Neighbour = NRRoute->ROUT_NEIGHBOUR;

					if (Neighbour)
					{
						len = ConvFromAX25(Neighbour->NEIGHBOUR_CALL, Normcall);
						Normcall[len] = 0;

						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td>%c&nbsp;</td><td>%d</td><td>%d</td><td>%d</td><td>%s</td></tr>",
							(Active == i)?'>':' ',NRRoute->ROUT_QUALITY, NRRoute->ROUT_OBSCOUNT, Neighbour->NEIGHBOUR_PORT, Normcall);
					}
					NRRoute++;
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</table>");

				goto SendResp;

			}
			/*

			MOV	ESI,OFFSET32 NODEROUTEHDDR
			MOV	ECX,11
			REP MOVSB

			LEA	ESI,DEST_CALL[EBX]
			CALL	DECODENODENAME		; CONVERT TO ALIAS:CALL
			REP MOVSB

			CMP	DEST_RTT[EBX],0
			JE SHORT @f			; TIMER NOT SET - DEST PROBABLY NOT USED

			MOVSB				; ADD SPACE
			CALL	DORTT

			@@:
			MOV	AL,CR
			STOSB

			MOV	ECX,3
			MOV	DH,DEST_ROUTE[EBX]	; CURRENT ACTIVE ROUTE
			MOV	DL,1

			push ebx

			PUBLIC	CMDN110
			CMDN110:

			MOV	ESI,ROUT1_NEIGHBOUR[EBX]
			CMP	ESI,0
			JE CMDN199


			MOV	AX,'  '
			CMP	DH,DL
			JNE SHORT CMDN112			; NOT CURRENT DEST
			MOV	AX,' >'

			CMDN112:

			STOSW

			PUSH	ECX

			MOV	AL,ROUT1_QUALITY[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOV	AL,ROUT1_OBSCOUNT[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb


			PUSH	EDI
			CALL	CONVFROMAX25		; CONVERT TO CALL
			POP	EDI

			MOV	ESI,OFFSET32 NORMCALL
			REP MOVSB

			MOV	AL,CR
			STOSB

			ADD	EBX,ROUTEVECLEN
			INC	DL			; ROUTE NUMBER

			POP	ECX
			DEC	ECX
			JNZ	CMDN110

			PUBLIC	CMDN199
			CMDN199:

			POP	EBX

			; DISPLAY  INP3 ROUTES

			MOV	ECX,3
			MOV	DL,4

			PUBLIC	CMDNINP3
			CMDNINP3:

			MOV	ESI,INPROUT1_NEIGHBOUR[EBX]
			CMP	ESI,0
			JE CMDNINPEND

			MOV	AX,'  '
			CMP	DH,DL
			JNE SHORT @F			; NOT CURRENT DEST
			MOV	AX,' >'

			@@:

			STOSW

			PUSH	ECX

			MOV	AL, Hops1[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOVZX	EAX, SRTT1[EBX]

			MOV	EDX,0
			MOV ECX, 100
			DIV ECX	
			CALL	CONV_5DIGITS
			MOV	AL,'.'
			STOSB
			MOV EAX, EDX
			CALL	PRINTNUM
			MOV	AL,'s'
			STOSB
			MOV	AL,' '
			STOSB

			MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			PUSH	EDI
			CALL	CONVFROMAX25		; CONVERT TO CALL
			POP	EDI

			MOV	ESI,OFFSET32 NORMCALL
			REP MOVSB


			MOV	AL,CR
			STOSB

			ADD	EBX,INPROUTEVECLEN
			INC	DL			; ROUTE NUMBER

			POP	ECX
			LOOP	CMDNINP3

			CMDNINPEND:

			ret

			*/


			if (_stricmp(NodeURL, "/Node/Routes.html") == 0)
			{
				struct ROUTE * Routes = NEIGHBOURS;
				int MaxRoutes = MAXNEIGHBOURS;
				int count;
				char Normcall[10];
				char locked;
				int NodeCount;
				int Percent = 0;
				int Iframes, Retries;
				char Active[10];
				int Queued;

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", RouteHddr);

				for (count=0; count<MaxRoutes; count++)
				{
					if (Routes->NEIGHBOUR_CALL[0] != 0)
					{
						int len = ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall);
						Normcall[len]=0;

						if ((Routes->NEIGHBOUR_FLAG & 1) == 1)
							locked = '!';
						else
							locked = ' ';

						NodeCount = COUNTNODES(Routes);

						if (Routes->NEIGHBOUR_LINK)
							Queued = COUNT_AT_L2(Routes->NEIGHBOUR_LINK);
						else
							Queued = 0;

						Iframes = Routes->NBOUR_IFRAMES;
						Retries = Routes->NBOUR_RETRIES;

						if (Routes->NEIGHBOUR_LINK && Routes->NEIGHBOUR_LINK->L2STATE >= 5)
							strcpy(Active, ">");
						else
							strcpy(Active, "&nbsp;");

						if (Iframes)
							Percent = (Retries * 100) / Iframes;
						else
							Percent = 0;

						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RouteLine, Active, Routes->NEIGHBOUR_PORT, Normcall, locked, 
							Routes->NEIGHBOUR_QUAL,	NodeCount, Iframes, Retries, Percent, Routes->NBOUR_MAXFRAME, Routes->NBOUR_FRACK,
							Routes->NEIGHBOUR_TIME >> 8, Routes->NEIGHBOUR_TIME & 0xff, Queued, Routes->OtherendsRouteQual);
					}
					Routes+=1;
				}
			}

			if (_stricmp(NodeURL, "/Node/Links.html") == 0)
			{
				struct _LINKTABLE * Links = LINKS;
				int MaxLinks = MAXLINKS;
				int count;
				char Normcall1[10];
				char Normcall2[10];
				char State[12] = "", Type[12] = "Uplink";
				int axState;
				int cctType;

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", LinkHddr);

				for (count=0; count<MaxLinks; count++)
				{
					if (Links->LINKCALL[0] != 0)
					{
						int len = ConvFromAX25(Links->LINKCALL, Normcall1);
						Normcall1[len] = 0;

						len = ConvFromAX25(Links->OURCALL, Normcall2);
						Normcall2[len] = 0;

						axState = Links->L2STATE;

						if (axState == 2)
							strcpy(State, "Connecting");
						else if (axState == 3)
							strcpy(State, "FRMR");
						else if (axState == 4)
							strcpy(State, "Closing");
						else if (axState == 5)
							strcpy(State, "Active");
						else if (axState == 6)
							strcpy(State, "REJ Sent");

						cctType = Links->LINKTYPE;

						if (cctType == 1)
							strcpy(Type, "Uplink");
						else if (cctType == 2)
							strcpy(Type, "Downlink");
						else if (cctType == 3)
							strcpy(Type, "Node-Node");

						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], LinkLine, Normcall1, Normcall2, Links->LINKPORT->PORTNUMBER, 
							State, Type, 2 - Links->VER1FLAG );

						Links+=1;
					}
				}
			}

			if (_stricmp(NodeURL, "/Node/Users.html") == 0)
			{
				TRANSPORTENTRY * L4 = L4TABLE;
				TRANSPORTENTRY * Partner;
				int MaxLinks = MAXLINKS;
				int count;
				char State[12] = "", Type[12] = "Uplink";
				char LHS[50] = "", MID[10] = "", RHS[50] = "";

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", UserHddr);

				for (count=0; count < MAXCIRCUITS; count++)
				{
					if (L4->L4USER[0])
					{
						RHS[0] = MID[0] = 0;

						if ((L4->L4CIRCUITTYPE & UPLINK) == 0)   //SHORT CMDS10A		; YES
						{
							// IF DOWNLINK, ONLY DISPLAY IF NO CROSSLINK

							if (L4->L4CROSSLINK == 0)	//jne 	CMDS60			; WILL PROCESS FROM OTHER END
							{
								// ITS A DOWNLINK WITH NO PARTNER - MUST BE A CLOSING SESSION
								// DISPLAY TO THE RIGHT FOR NOW

								strcpy(LHS, "(Closing) ");
								DISPLAYCIRCUIT(L4, RHS);
								goto CMDS50;
							}
							else
								goto CMDS60;		// WILL PROCESS FROM OTHER END
						}

						if (L4->L4CROSSLINK == 0)
						{
							// Single Entry

							DISPLAYCIRCUIT(L4, LHS);
						}
						else
						{
							DISPLAYCIRCUIT(L4, LHS);

							Partner = L4->L4CROSSLINK;

							if (Partner->L4STATE == 5)
								strcpy(MID, "<-->");
							else
								strcpy(MID, "<~~>");

							DISPLAYCIRCUIT(Partner, RHS);
						}
CMDS50:
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], UserLine, LHS, MID, RHS);
					}
CMDS60:			
					L4++;	
				}
			}
			/*
			PUBLIC	CMDUXX_1
			CMDUXX_1:
			push	EBX
			push	ESI
			PUSH	ECX
			push	EDI

			call	_FINDDESTINATION
			pop	EDI

			jz SHORT NODE_FOUND

			push	EDI			; NET/ROM not found
			call	CONVFROMAX25		; CONVERT TO CALL
			pop	EDI
			mov	ESI,OFFSET32 NORMCALL
			rep movsb

			jmp	SHORT END_CMDUXX

			PUBLIC	NODE_FOUND
			NODE_FOUND:

			lea	ESI,DEST_CALL[EBX]
			call	DECODENODENAME

			REP 	MOVSB

			PUBLIC	END_CMDUXX
			END_CMDUXX:

			POP	ECX
			pop	ESI
			pop	EBX
			ret

			}}}
			*/

			else if (_stricmp(NodeURL, "/Node/Terminal.html") == 0)
			{
				if (COOKIE && Session)
				{
					// Already signed in as sysop

					struct UserRec * USER = Session->USER;

					struct HTTPConnectionInfo * NewSession = AllocateSession(sock, 'T');

					if (NewSession)
					{
						char AXCall[10];
						ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, NewSession->Key, NewSession->Key, NewSession->Key);
						strcpy(NewSession->HTTPCall, USER->Callsign);
						ConvToAX25(NewSession->HTTPCall, AXCall);
						ChangeSessionCallsign(NewSession->Stream, AXCall);
						BPQHOSTVECTOR[NewSession->Stream -1].HOSTSESSION->Secure_Session = USER->Secure;
						Session->USER = USER;
						NewSession->TNC = conn->TNC;


						//			if (Appl[0])
						//			{
						//				strcat(Appl, "\r");
						//				SendMsg(Session->Stream, Appl, strlen(Appl));
						//			}

					}
					else
					{
						ReplyLen = SetupNodeMenu(_REPLYBUFFER, LOCAL);
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", BusyError);
					}
				}
				else if (LOCAL)
				{
					// connected to 127.0.0.1 so sign in using node call

					struct HTTPConnectionInfo * NewSession = AllocateSession(sock, 'T');

					if (NewSession)
					{
						ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, NewSession->Key, NewSession->Key, NewSession->Key);
						strcpy(NewSession->HTTPCall, MYNODECALL);
						ChangeSessionCallsign(NewSession->Stream, MYCALL);
						BPQHOSTVECTOR[NewSession->Stream -1].HOSTSESSION->Secure_Session = TRUE;
						NewSession->TNC = conn->TNC;
					}
				}
				else
					ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Context);
			}

			else if (_stricmp(NodeURL, "/Node/Signon.html") == 0)
			{
				ReplyLen = sprintf(_REPLYBUFFER, NodeSignon, Mycall, Mycall, Context);
			}

			else if (_stricmp(NodeURL, "/Node/Drivers") == 0)
			{
				int Bufferlen = SendMessageFile(sock, "/Drivers.htm", TRUE, allowDeflate);		// return -1 if not found

				if (Bufferlen != -1)
					return 0;											// We've sent it
			}

			else if (_stricmp(NodeURL, "/Node/OutputScreen.html") == 0)
			{
				struct HTTPConnectionInfo * Session = FindSession(Context);

				if (Session == NULL)
				{
					ReplyLen = sprintf(_REPLYBUFFER, "%s", LostSession);
				}
				else
				{
					Session->sock = sock;				// socket to reply on
					ReplyLen = RefreshTermWindow(TCP, Session, _REPLYBUFFER);

					if (ReplyLen == 0)		// Nothing new
					{
						//				Debugprintf("GET with no data avail - response held");
						Session->ResponseTimer = 1200;		// Delay response for up to a minute
					}
					else
					{
						//				Debugprintf("GET - outpur sent, timer was %d, set to zero", Session->ResponseTimer);
						Session->ResponseTimer = 0;
					}

					Session->KillTimer = 0;
					return 0;				// Refresh has sent any available output
				}
			}

			else if (_stricmp(NodeURL, "/Node/InputLine.html") == 0)
			{
				struct TNCINFO * TNC = conn->TNC;
				struct TCPINFO * TCP = 0;

				if (TNC)
					TCP = TNC->TCPInfo;

				if (TCP && TCP->WebTermCSS)	
					ReplyLen = sprintf(_REPLYBUFFER, InputLine, Context, TCP->WebTermCSS);
				else
					ReplyLen = sprintf(_REPLYBUFFER, InputLine, Context, "");

			}

			else if (_stricmp(NodeURL, "/Node/PTT") == 0)
			{
				struct TNCINFO * TNC = conn->TNC;
				int x = atoi(Context);
			}


SendResp:

			FormatTime3(TimeString, time(NULL));

			strcpy(&_REPLYBUFFER[ReplyLen], Tail);
			ReplyLen += (int)strlen(Tail);


			if (allowDeflate)
			{
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			} 
			else
			{
				Encoding[0] = 0;
				Compressed = _REPLYBUFFER;
			}

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
				"Date: %s\r\n%s\r\n", ReplyLen, TimeString, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);
		}
		return 0;

#ifdef WIN32xx
	}
#include "StdExcept.c"
}
return 0;
#endif
}

void ProcessHTTPMessage(void * conn)
{
	// conn is a malloc'ed copy to handle reused connections, so need to free it

	InnerProcessHTTPMessage((struct ConnectionInfo *)conn);
	free(conn);
	return;
}

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID FormatTime3(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

}

// Sun, 06 Nov 1994 08:49:37 GMT

int StatusProc(char * Buff)
{
	int i;
	char callsign[12] = "";
	char flag[3];
	UINT Mask, MaskCopy;
	int Flags;
	int AppNumber;
	int OneBits;
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<head><title>Stream Status</title></head><body>");

	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; font-family: monospace; align=center \" border=1 cellpadding=1 cellspacing=0>");
	Len += sprintf(&Buff[Len], "<tr><th>&nbsp;&nbsp;&nbsp;</th><th>&nbsp;RX&nbsp;&nbsp;</th><th>&nbsp;TX&nbsp;&nbsp;</th>");
	Len += sprintf(&Buff[Len], "<th>&nbsp;MON&nbsp;</th><th>&nbsp;App&nbsp;</th><th>&nbsp;Flg&nbsp;</th>");
	Len += sprintf(&Buff[Len], "<th>Callsign&nbsp;&nbsp;</th><th width=200px>Program</th>");
	Len += sprintf(&Buff[Len], "<th>&nbsp;&nbsp;&nbsp;</th><th>&nbsp;RX&nbsp;&nbsp;</th><th>&nbsp;TX&nbsp;&nbsp;</th>");
	Len += sprintf(&Buff[Len], "<th>&nbsp;MON&nbsp;</th><th>&nbsp;App&nbsp;</th><th>&nbsp;Flg&nbsp;</th>");
	Len += sprintf(&Buff[Len], "<th>Callsign&nbsp;&nbsp;</th><th width=200px>Program</th></tr><tr>");

	for (i=1;i<65; i++)
	{		
		callsign[0]=0;

		if (GetAllocationState(i))

			strcpy(flag,"*");
		else
			strcpy(flag," ");

		GetCallsign(i,callsign);

		Mask = MaskCopy = Get_APPLMASK(i);

		// if only one bit set, convert to number

		AppNumber = 0;
		OneBits = 0;

		while (MaskCopy)
		{
			if (MaskCopy & 1)
				OneBits++;

			AppNumber++;
			MaskCopy = MaskCopy >> 1;
		}

		Flags=GetApplFlags(i);

		if (OneBits > 1)
			Len += sprintf(&Buff[Len], "<td>%d%s</td><td>%d</td><td>%d</td><td>%d</td><td>%x</td>"
			"<td>%x</td><td>%s</td><td>%s</td>", 
			i, flag, RXCount(i), TXCount(i), MONCount(i), Mask, Flags, callsign, BPQHOSTVECTOR[i-1].PgmName);

		else
			Len += sprintf(&Buff[Len], "<td>%d%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>"
			"<td>%x</td><td>%s</td><td>%s</td>", 
			i, flag, RXCount(i), TXCount(i), MONCount(i), AppNumber, Flags, callsign, BPQHOSTVECTOR[i-1].PgmName);

		if ((i & 1) == 0)
			Len += sprintf(&Buff[Len], "</tr><tr>");

	}

	Len += sprintf(&Buff[Len], "</tr></table>");
	return Len;
}

int ProcessNodeSignon(SOCKET sock, struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;
	char Header[256];
	int HeaderLen;
	struct HTTPConnectionInfo *Sess;


	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen =  SetupNodeMenu(Reply, LOCAL);

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
				"\r\n", (int)(ReplyLen + strlen(Tail)));	
			send(sock, Header, HeaderLen, 0);
			send(sock, Reply, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);
		}
		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && USER->Secure)
				{
					// ok

					Sess = *Session = AllocateSession(sock, 'N');
					Sess->USER = USER;

					ReplyLen =  SetupNodeMenu(Reply, LOCAL);

					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
						"Set-Cookie: BPQSessionCookie=%s; Path = /\r\n\r\n", (int)(ReplyLen + strlen(Tail)), Sess->Key);	
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, NodeSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", (int)(ReplyLen + strlen(Tail)));	
	send(sock, Header, HeaderLen, 0);
	send(sock, Reply, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);

	return 0;


	return ReplyLen;
}




int ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;
	struct HTTPConnectionInfo * NewSession;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(Reply, LOCAL);
			return ReplyLen;
		}
		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && (USER->Secure || WebMail))
				{
					// ok

					NewSession = AllocateSession(Appl[0], 'M');

					*Session = NewSession;

					if (NewSession)
					{

						ReplyLen = 0;
						strcpy(NewSession->Callsign, USER->Callsign);
					}
					else
					{
						ReplyLen =  SetupNodeMenu(Reply, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	return ReplyLen;
}


int ProcessChatSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, struct HTTPConnectionInfo ** Session, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(Reply, LOCAL);
			return ReplyLen;
		}

		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;


		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && USER->Secure)
				{
					// ok

					*Session = AllocateSession(Appl[0], 'C');

					if (Session)
					{
						ReplyLen = 0;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, ChatSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	return ReplyLen;

}

#define SHA1_HASH_LEN 20

/*

Copyright (C) 1998, 2009
Paul E. Jones <paulej@packetizer.com>

Freeware Public License (FPL)

This software is licensed as "freeware."  Permission to distribute
this software in source and binary forms, including incorporation 
into other products, is hereby granted without a fee.  THIS SOFTWARE 
IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESSED OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE.  THE AUTHOR SHALL NOT BE HELD 
LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF THIS SOFTWARE, EITHER 
DIRECTLY OR INDIRECTLY, INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA 
OR DATA BEING RENDERED INACCURATE.
*/

/*  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved
 *
 *****************************************************************************
 *  $Id: sha1.h 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in the SHA1Context, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

/* 
 *  This structure will hold context information for the hashing
 *  operation
 */
typedef struct SHA1Context
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */

    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */

    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */

    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

/*
 *  Function Prototypes
 */
void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *, const unsigned char *, unsigned);

#endif
 
BOOL SHA1PasswordHash(char * lpszPassword, char * Hash)
{
	SHA1Context sha; 
	int i;

	SHA1Reset(&sha);
	SHA1Input(&sha, lpszPassword, strlen(lpszPassword));
	SHA1Result(&sha);

	// swap byte order if little endian
	
	for (i = 0; i < 5; i++)
		sha.Message_Digest[i] = htonl(sha.Message_Digest[i]);

	memcpy(Hash, &sha.Message_Digest[0], 20);

    return TRUE;
}

int BuildRigCtlPage(char * _REPLYBUFFER)
{
	int ReplyLen;

	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;
	int p, i;

	char Page[] =
		"<html><meta http-equiv=expires content=0>\r\n"
		//					"<meta http-equiv=refresh content=5>\r\n"
		"<head><title>Rigcontrol</title></head>\r\n"
		"<style type=text/css>form{margin:0px; padding:0px; display:inline;}</style>"
		"<body height: 580px;><h3>Rigcontrol</h3>\r\n"
		"<table style=\"text-align: left; width: 580px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2><tr>\r\n"
		"<th width=90px>Radio</th>\r\n"
		"<th width=90px>Freq</th>\r\n"
		"<th width=90px>Mode</th>\r\n"
		"<th>ST</th>\r\n"
		"<th>Ports</th>\r\n"
		"<th hidden width=10px>Action</th>\r\n"
		"</tr>";
	char RigLine[] =
		"<tr>\r\n"
		"  <td>%s</td>\r\n"
		"  <td>%s</td>\r\n"
		"  <td>%s/1</td>\r\n"
		"  <td>%c%c</td>\r\n"
		"  <td>%s</td>\r\n"
		"  <td hidden width=10px><input onclick=PTT('R%d') type=submit class='btn' value='PTT'></td>\r\n"
		"  </tr>\r\n";
	char Tail[] =		
		"</table>\r\n"
		"</body></html>\r\n";

	ReplyLen = sprintf(_REPLYBUFFER, "%s", Page);

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RigLine, RIG->WEB_Label, RIG->WEB_FREQ, RIG->WEB_MODE, RIG->WEB_SCAN, RIG->WEB_PTT, RIG->WEB_PORTS, RIG->Interlock);
		}
	}

	ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", Tail);
	return ReplyLen;
}


void SendRigWebPage()
{
	int i, n;
	struct ConnectionInfo * sockptr;
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;

	for (i = 0; i < 33; i++)
	{
		TNC = TNCInfo[i];

		if (TNC && TNC->Hardware == H_TELNET)
		{
			TCP = TNC->TCPInfo;

			if (TCP)
			{
				for (n = 0; n <= TCP->MaxSessions; n++)
				{
					sockptr = TNC->Streams[n].ConnectionInfo;

					if (sockptr->SocketActive)
					{
						if (sockptr->HTTPMode && sockptr->WebSocks  && strcmp(sockptr->WebURL, "RIGCTL") == 0)
						{
							char RigMsg[8192];
							int RigMsgLen = strlen(RigWebPage);
							char* ptr;

							RigMsg[0] = 0x81;		// Fin, Data
							RigMsg[1] = 126;		// Unmasked, Extended Len
							RigMsg[2] = RigMsgLen >> 8;
							RigMsg[3] = RigMsgLen & 0xff;
							strcpy(&RigMsg[4], RigWebPage);

							// If secure session enable PTT button

							if (sockptr->WebSecure)
							{
								while (ptr = strstr(RigMsg, "hidden"))
									memcpy(ptr, "      ", 6);
							}

							send(sockptr->socket, RigMsg, RigMsgLen + 4, 0);
						}
					}
				}
			}
		}
	}
}

// Webmail web socket code

int ProcessWebmailWebSock(char * MsgPtr, char * OutBuffer);

void ProcessWebmailWebSockThread(void * conn)
{
	// conn is a malloc'ed copy to handle reused connections, so need to free it

	struct ConnectionInfo * sockptr = (struct ConnectionInfo *)conn;
	char * URL = sockptr->WebURL;
	int Loops = 0;
	int Sent;
	struct HTTPConnectionInfo Dummy = {0};
	int ReplyLen = 0;
	int InputLen = 0;

#ifdef LINBPQ

	char _REPLYBUFFER[250000];

	ReplyLen = ProcessWebmailWebSock(URL, _REPLYBUFFER);

	// Send may block

	Sent = send(sockptr->socket, _REPLYBUFFER, ReplyLen, 0);

	while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
	{	
		if (Sent > 0)					// something sent
		{
			ReplyLen -= Sent;
			memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
		}	

		Sleep(30);
		Sent = send(sockptr->socket, _REPLYBUFFER, ReplyLen, 0);
	}
	
#else
	// Send URL to BPQMail via Pipe. Just need a dummy session, as URL contains session key

	HANDLE hPipe;
	char Reply[250000];



	hPipe = CreateFile(MAILPipeFileName, GENERIC_READ | GENERIC_WRITE,
		0,                    // exclusive access
		NULL,                 // no security attrs
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 
		NULL );

	if (hPipe == (HANDLE)-1)
	{
		free(conn);
		return;
	}

	WriteFile(hPipe, &Dummy, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
	WriteFile(hPipe, URL, strlen(URL), &InputLen, NULL);

	ReadFile(hPipe, &Dummy, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
	ReadFile(hPipe, Reply, 250000, &ReplyLen, NULL);

	if (ReplyLen <= 0)
	{
		InputLen = GetLastError();
	}

	CloseHandle(hPipe);

	// ?? do we need a thread to handle write which may block

	Sent = send(sockptr->socket, Reply, ReplyLen, 0);

	while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
	{	
		//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

		if (Sent > 0)					// something sent
		{
			InputLen -= Sent;
			memmove(Reply, &Reply[Sent], ReplyLen);
		}

		Sleep(30);
		Sent = send(sockptr->socket, Reply, ReplyLen, 0);
	}
#endif
	free(conn);
	return;
}

/*
 *  sha1.c
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved
 *
 *****************************************************************************
 *  $Id: sha1.c 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This file implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The Secure Hashing Standard, which uses the Secure Hashing
 *      Algorithm (SHA), produces a 160-bit message digest for a
 *      given data stream.  In theory, it is highly improbable that
 *      two messages will produce the same message digest.  Therefore,
 *      this algorithm can serve as a means of providing a "fingerprint"
 *      for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code was
 *      written with the expectation that the processor has at least
 *      a 32-bit machine word size.  If the machine word size is larger,
 *      the code should still function properly.  One caveat to that
 *      is that the input functions taking characters and character
 *      arrays assume that only 8 bits of information are stored in each
 *      character.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long. Although SHA-1 allows a message digest to be generated for
 *      messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is a
 *      multiple of the size of an 8-bit character.
 *
 */

/*
 *  Define the circular shift macro
 */
#define SHA1CircularShift(bits,word) \
                ((((word) << (bits)) & 0xFFFFFFFF) | \
                ((word) >> (32-(bits))))

/* Function prototypes */
void SHA1ProcessMessageBlock(SHA1Context *);
void SHA1PadMessage(SHA1Context *);

/*  
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1Reset(SHA1Context *context)
{
    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Message_Digest[0]      = 0x67452301;
    context->Message_Digest[1]      = 0xEFCDAB89;
    context->Message_Digest[2]      = 0x98BADCFE;
    context->Message_Digest[3]      = 0x10325476;
    context->Message_Digest[4]      = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;
}

/*  
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array within the SHA1Context provided
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *
 *  Returns:
 *      1 if successful, 0 if it failed.
 *
 *  Comments:
 *
 */
int SHA1Result(SHA1Context *context)
{

    if (context->Corrupted)
    {
        return 0;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        context->Computed = 1;
    }

    return 1;
}

/*  
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA-1 context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1Input(     SHA1Context         *context,
                    const unsigned char *message_array,
                    unsigned            length)
{
    if (!length)
    {
        return;
    }

    if (context->Computed || context->Corrupted)
    {
        context->Corrupted = 1;
        return;
    }

    while(length-- && !context->Corrupted)
    {
        context->Message_Block[context->Message_Block_Index++] =
                                                (*message_array & 0xFF);

        context->Length_Low += 8;
        /* Force it to 32 bits */
        context->Length_Low &= 0xFFFFFFFF;
        if (context->Length_Low == 0)
        {
            context->Length_High++;
            /* Force it to 32 bits */
            context->Length_High &= 0xFFFFFFFF;
            if (context->Length_High == 0)
            {
                /* Message is too long */
                context->Corrupted = 1;
            }
        }

        if (context->Message_Block_Index == 64)
        {
            SHA1ProcessMessageBlock(context);
        }

        message_array++;
    }
}

/*  
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *      Many of the variable names in the SHAContext, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *         
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const unsigned K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int         t;                  /* Loop counter                 */
    unsigned    temp;               /* Temporary word value         */
    unsigned    W[80];              /* Word sequence                */
    unsigned    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = ((unsigned) context->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Message_Digest[0];
    B = context->Message_Digest[1];
    C = context->Message_Digest[2];
    D = context->Message_Digest[3];
    E = context->Message_Digest[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Message_Digest[0] =
                        (context->Message_Digest[0] + A) & 0xFFFFFFFF;
    context->Message_Digest[1] =
                        (context->Message_Digest[1] + B) & 0xFFFFFFFF;
    context->Message_Digest[2] =
                        (context->Message_Digest[2] + C) & 0xFFFFFFFF;
    context->Message_Digest[3] =
                        (context->Message_Digest[3] + D) & 0xFFFFFFFF;
    context->Message_Digest[4] =
                        (context->Message_Digest[4] + E) & 0xFFFFFFFF;

    context->Message_Block_Index = 0;
}

/*  
 *  SHA1PadMessage
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call SHA1ProcessMessageBlock()
 *      appropriately.  When it returns, it can be assumed that the
 *      message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = (context->Length_High >> 24) & 0xFF;
    context->Message_Block[57] = (context->Length_High >> 16) & 0xFF;
    context->Message_Block[58] = (context->Length_High >> 8) & 0xFF;
    context->Message_Block[59] = (context->Length_High) & 0xFF;
    context->Message_Block[60] = (context->Length_Low >> 24) & 0xFF;
    context->Message_Block[61] = (context->Length_Low >> 16) & 0xFF;
    context->Message_Block[62] = (context->Length_Low >> 8) & 0xFF;
    context->Message_Block[63] = (context->Length_Low) & 0xFF;

    SHA1ProcessMessageBlock(context);
}





