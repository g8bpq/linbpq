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

#ifdef WIN32
//#include "C:\Program Files (x86)\GnuWin32\include\iconv.h"
#else
#include <iconv.h>
#endif

extern char NodeTail[];
extern char BBSName[10];

extern char LTFROMString[2048];
extern char LTTOString[2048];
extern char LTATString[2048];

//static UCHAR BPQDirectory[260];

extern ConnectionInfo Connections[];

extern int NumberofStreams;
extern time_t MaintClock;						// Time to run housekeeping

extern int SMTPMsgs;

extern int ChatApplNum;
extern int MaxChatStreams;

extern char Position[81];
extern char PopupText[251];
extern int PopupMode;
extern int reportMailEvents;

#define MaxCMS	10				// Numbr of addresses we can keep - currently 4 are used.

struct UserInfo * BBSLIST[NBBBS + 1];

int MaxBBS = 0;

#define MAIL
#include "httpconnectioninfo.h"

struct TCPINFO * TCP;

VOID ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int * RLen);
static struct HTTPConnectionInfo * FindSession(char * Key);
VOID ProcessUserUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgFwdUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key);
VOID ProcessConfUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessUIUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendUserSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFWDSelectPage(char * Reply, int * ReplyLen, char * Key);
int EncryptPass(char * Pass, char * Encrypt);
VOID ProcessFWDUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key);
VOID SendUIPage(char * Reply, int * ReplyLen, char * Key);
VOID GetParam(char * input, char * key, char * value);
BOOL GetConfig(char * ConfigName);
VOID ProcessDisUser(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY SessionControl(int stream, int command, int param);
int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key);
VOID ProcessMsgUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgAction(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY GetNumberofPorts();
int APIENTRY GetPortNumber(int portslot);
UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc);
struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key);
VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key);
VOID SaveWelcome(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID GetMallocedParam(char * input, char * key, char ** value);
VOID SaveMessageText(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveHousekeeping(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID SaveWP(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
int SendWPDetails(WPRec * WP, char * Reply, char * Key);
int SendUserDetails(struct HTTPConnectionInfo * Session, char * Reply, char * Key);
int SetupNodeMenu(char * Buff);
VOID SendFwdSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFwdDetails(struct UserInfo * User, char * Reply, int * ReplyLen, char * Key);
VOID SetMultiStringValue(char ** values, char * Multi);
VOID SendFwdMainPage(char * Reply, int * ReplyLen, char * Key);
VOID SaveFwdCommon(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveFwdDetails(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
char **	SeparateMultiString(char * MultiString, BOOL NoToUpper);
VOID TidyPrompts();
char * GetTemplateFromFile(int Version, char * FN);
VOID FormatTime(char * Time, time_t cTime);
struct MsgInfo * GetMsgFromNumber(int msgno);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP);
BOOL OkToKillMessage(BOOL SYSOP, char * Call, struct MsgInfo * Msg);
int MulticastStatusHTML(char * Reply);
void ProcessWebMailMessage(struct HTTPConnectionInfo * Session, char * Key, BOOL LOCAL, char * Method, char * NodeURL, char * input, char * Reply, int * RLen, int InputLen);
int SendWebMailHeader(char * Reply, char * Key, struct HTTPConnectionInfo * Session);
struct UserInfo * FindBBS(char * Name);
void ReleaseWebMailStruct(WebMailInfo * WebMail);
VOID TidyWelcomeMsg(char ** pPrompt);
int MailAPIProcessHTTPMessage(struct HTTPConnectionInfo * Session, char * response, char * Method, char * URL, char * request, BOOL LOCAL, char * Param, char * Token);
void UndoTransparency(char * input);
int GetMessageSlotFromMessageNumber(int msgno);

char UNC[] = "";
char CHKD[] = "checked=checked ";
char sel[] = "selected";

char Sent[] = "#98FFA0";
char ToSend[] = "#FFFF00";
char NotThisOne[] = "#FFFFFF";

static char PassError[] = "<p align=center>Sorry, User or Password is invalid - please try again</p>";

static char BusyError[] = "<p align=center>Sorry, No sessions available - please try later</p>";

extern char WebMailSignon[];

char MailSignon[] = "<html><head><title>BPQ32 Mail Server Access</title></head><body background=\"/background.jpg\">"
	"<h3 align=center>BPQ32 Mail Server %s Access</h3>"
	"<h3 align=center>Please enter Callsign and Password to access the BBS</h3>"
	"<form method=post action=/Mail/Signon?Mail>"
	"<table align=center  bgcolor=white>"
	"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
	"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
	"<p align=center><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></form>";


char MailPage[] = "<html><head><title>%s's BBS Web Server</title>"
	"<style type=\"text/css\">"
	"input.btn:active {background:black;color:white;} "
	"submit.btn:active {background:black;color:white;} "
	"</style>"
	"</head>"
	"<body background=\"/background.jpg\"><h3 align=center>BPQ32 BBS %s</h3><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Mail/Status?%s>Status</a></td>"
	"<td><a href=/Mail/Conf?%s>Configuration</a></td>"
	"<td><a href=/Mail/Users?%s>Users</a></td>"
	"<td><a href=/Mail/Msgs?%s>Messages</a></td>"
	"<td><a href=/Mail/FWD?%s>Forwarding</a></td>"
	"<td><a href=/Mail/Wel?%s>Welcome Msgs & Prompts</a></td>"
	"<td><a href=/Mail/HK?%s>Housekeeping</a></td>"
	"<td><a href=/Mail/WP?%s>WP Update</a></td>"
	"<td><a href=/Webmail>WebMail</a></td>"
	"<td><a href=/>Node Menu</a></td>"
	"</tr></table>";

char RefreshMainPage[] = "<html><head>"
	"<meta http-equiv=refresh content=10>"
	"<style type=\"text/css\">"
	"input.btn:active {background:black;color:white;} "
	"submit.btn:active {background:black;color:white;} "
	"</style>"
	"<title>%s's BBS Web Server</title></head>"
	"<body background=\"/background.jpg\"><h3 align=center>BPQ32 BBS %s</h3><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Mail/Status?%s>Status</a></td>"
	"<td><a href=/Mail/Conf?%s>Configuration</a></td>"
	"<td><a href=/Mail/Users?%s>Users</a></td>"
	"<td><a href=/Mail/Msgs?%s>Messages</a></td>"
	"<td><a href=/Mail/FWD?%s>Forwarding</a></td>"
	"<td><a href=/Mail/Wel?%s>Welcome Msgs & Prompts</a></td>"
	"<td><a href=/Mail/HK?%s>Housekeeping</a></td>"
	"<td><a href=/Mail/WP?%s>WP Update</a></td>"
	"<td><a href=/Webmail>WebMail</a></td>"
	"<td><a href=/>Node Menu</a></td>"
	"</tr></table>";

char StatusPage [] = 

"<form style=\"font-family: monospace; text-align: center\"  method=post action=/Mail/DisSession?%s>"
"<br>User&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Callsign&nbsp;&nbsp; Stream &nbsp;Queue &nbsp;Sent &nbsp;Rxed<br>"
"<select style=\"font-family: monospace;\" tabindex=1 size=10 name=call>";

char StreamEnd[] = 
"</select><br><input name=Disconnect value=Disconnect type=submit class='btn'><br><br>";

char StatusTail[] = 
"Msgs&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <input readonly=readonly value=%d size=3><br>"
"Sysop Msgs <input readonly=readonly value=%d size=3><br>"
"Held Msgs&nbsp; <input readonly=readonly value=%d size=3><br>"
"SMTP Msgs&nbsp; <input readonly=readonly value=%d size=3><br></form>";


char UIHddr [] = "<form style=\"font-family: monospace;\" align=center method=post"
" action=/Mail/UI?%s> Mailfor Header <input size=40 value=\"%s\" name=MailFor><br>"
"&nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp;"
"&nbsp;&nbsp;&nbsp; (use \\r to insert newline in message)<br><br>"
"Enable Port&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp;"
"&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; Path&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp;"
"&nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; Send: MailFor Headers Empty Mailfor<br><br>";

char UILine[] = "<input %sname=En%d type=checkbox> %s <input size=40 value=\"%s\" name=Path%d>"
" <input %sname=SndMF%d type=checkbox>"
"&nbsp;&nbsp;&nbsp;&nbsp;<input %sname=SndHDDR%d type=checkbox>"
"&nbsp; &nbsp; &nbsp; &nbsp; <input %sname=SndNull%d type=checkbox><br>";

char UITail[] = "<br><br><input name=Update value=Update type=submit class='btn'> "
"<input name=Cancel value=Cancel type=submit class='btn'><br></form>";

char FWDSelectHddr[] = 
	"<form style=\"font-family: monospace; text-align: center;\" method=post action=/Mail/FWDSel?%s>"
	"Max Size to Send &nbsp;&nbsp; <input value=%d size=3 name=MaxTX><br>"
	"Max Size to Receive <input value=%d size=3 name=MaxRX><br>"
	"Warn if no route for P or T <input %sname=WarnNoRoute type=checkbox><br>"
	"Use Local Time&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
	"<input %sname=LocalTime type=checkbox><br><br>"
	"Aliases &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; Select BBS<br>"
	"<textarea rows=10 cols=20 name=Aliases>%s</textarea> &nbsp<select tabindex=1 size=10 name=call>";

char FWDSelectTail[] =
	"</select><br>&nbsp;&nbsp; <input name=Save value=Save type=submit class='btn'>&nbsp;<input "
	"name=Cancel value=Cancel type=submit class='btn'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
	"&nbsp; <input name=Select value=Select type=submit class='btn'></form>";

char UserSelectHddr[] = 
	"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/Users?%s>"
	"Please Select User<br><br><select tabindex=1 size=10 name=call>";

char UserSelectLine[] = "<option value=%s>%s</option>";

char StatusLine[] = "<option value=%d>%s</option>";

char UserSelectTail[] = "</select><br><br>"
	"<input size=6 value=\"\" name=NewCall>"
	"<input type=submit class='btn' value=\"Add User\" name=Adduser><br>"
	"<input type=submit class='btn' value=Select> "
	"<input type=submit class='btn' value=Cancel name=Cancel><br></form>";

char UserUpdateHddr[] =
	"<h3 align=center>Update User %s</h3>"
	"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/Users?%s>";

char UserUpdateLine[] = "<option value=%s>%s</option>";

//<option value="G8BPQ">G8BPQ</option>
//<input checked="checked" name=%s type="checkbox"><br>


char FWDUpdate[] = 
"<h3 align=center>Update Forwarding for BBS %s</h3>"
"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/FWD?%s"
" name=Test>&nbsp;&nbsp;&nbsp;&nbsp;"
"TO &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"AT&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"TIMES&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; Connect Script<br>"
"<textarea wrap=hard rows=10 cols=10 name=TO>%s</textarea>"
" <textarea wrap=hard rows=10 cols=10 name=AT>%s</textarea>"
" <textarea wrap=hard rows=10 cols=10 name=Times>%s</textarea>"
" <textarea wrap=hard rows=10 cols=20 name=FWD>%s</textarea><br>"
"<textarea wrap=hard rows=10 cols=30 name=HRB>%s</textarea>"
" <textarea wrap=hard rows=10 cols=30 name=HRP>%s</textarea><br><br>"
"Enable Forwarding&nbsp;<input %sname=EnF type=checkbox> Interval"
"<input value=%d size=3 name=Interval>(Secs) Request Reverse"
"<input %sname=EnR type=checkbox> Interval <input value=%d size=3 "
"name=RInterval>(Secs)<br>"
"Send new messages without waiting for poll timer<input %sname=NoWait type=checkbox><br>"
"BBS HA <input value=%s size=60 name=BBSHA> FBB Max Block <input "
"value=%d size=3 name=FBBBlock><br>"
"Send Personal Mail Only <input %sname=Personal type=checkbox>&nbsp;"
"Allow Binary&nbsp; <input %sname=Bin type=checkbox>&nbsp;&nbsp; Use B1 "
"Protocol <input %sname=B1 type=checkbox>&nbsp; Use B2 Protocol<input "
"%sname=B2 type=checkbox><br><br>"
"<input name=Submit value=Update type=submit class='btn'> <input name=Fwd value=\"Start Forwarding\" type=submit class='btn'> "
"<input name=Cancel value=Cancel type=submit class='btn'></form><br></body></html>";

static char MailDetailPage[] = 
"<html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
"<title>MsgEdit</title></head><body><h4>Message %d</h4>"
"<form style=\"font-family: monospace;\" method=post action=/Mail/Msg?%s name=Msgs>"
"From&nbsp; <input style=\"font-family: monospace;\" size=10 name=From value=%s> Sent&nbsp;&nbsp;&nbsp;"
"&nbsp; &nbsp; &nbsp; <input readonly=readonly size=12 name=Sent value=\"%s\">&nbsp;"
"Type &nbsp;&nbsp;&nbsp;&nbsp;<select tabindex=1 size=1 name=Type>"
"<option %s value=B>B</option>"
"<option %s value=P>P</option>"
"<option %s value=T>T</option>"
"</select><br>"
"To&nbsp; &nbsp; <input style=\"font-family: monospace;\" size=10 name=To value=%s>"
" Received&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input readonly=readonly size=12 name=RX value=\"%s\">&nbsp;"
"Status &nbsp;&nbsp;<select tabindex=1 size=1 name=Status>"
"<option %s value=N>N</option>"
"<option %s value=Y>Y</option>"
"<option %s value=F>F</option>"
"<option %s value=K>K</option>"
"<option %s value=H>H</option>"
"<option %s value=D>D</option>"
"<option %s value=$>$</option>"
"</select><br>"
"BID&nbsp;&nbsp; <input style=\"width:100px; font-family: monospace; \" name=BID value=\"%s\"> Last Changed <input readonly=readonly size=12 name=LastChange value=\"%s\">&nbsp;"
"Size&nbsp; <input readonly=readonly size=5 name=Size value=%d><br><br>"
"%s"		// Email from Line
"&nbsp;VIA&nbsp; <input style=\"width:360px;\" name=VIA value=%s><br>"
"Title&nbsp; <input style=\"width:360px;\" name=Title value=\"%s\"> <br><br>"
"<span align = center><input onclick=editmsg(\"EditM?%s?%d\") value=\"Edit Text\" type=button class='btn'> "
"<input onclick=save(this.form) value=Save type=button class='btn'> "
"<td><a href=/Mail/SaveMessage?%s><button type=button class='btn'>Save Message</button></a></td>"
"<td><a href=/Mail/SaveAttachment?%s><button type=button class='btn' %s>Save Attachment</button></a></td>"
//"<input onclick=doit(\"SavetoFile\") value=\"Save to File\" type=button class='btn'> "
"<input onclick=doit(\"Print\") value=Print type=button class='btn'> "
"<input onclick=doit(\"Export\") value=Export type=button class='btn'></span><br><br>"
"Green = Sent, Yellow = Queued"
"<table style=\"text-align: left; width: 490px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>";

char MailDetailTail[] = "</table></form>";

char Welcome[] = "<form style=\"font-family: monospace; text-align: center;\"" 
"method=post action=/Mail/Welcome?%s>"
"Normal User Welcome<br>"
"<textarea cols=80 rows=3 name=NUWelcome>%s</textarea><br>"
"New User Welcome<br>"
"<textarea cols=80 rows=3 name=NewWelcome>%s</textarea><br>"
"Expert User Welcome<br>"
"<textarea cols=80 rows=3 name=ExWelcome>%s</textarea><br>"
"Normal User Prompt<br>"
"<textarea cols=80 rows=3 name=NUPrompt>%s</textarea><br>"
"New User Prompt<br>"
"<textarea cols=80 rows=3 name=NewPrompt>%s</textarea><br>"
"Expert User Prompt<br>"
"<textarea cols=80 rows=1 name=ExPrompt>%s</textarea><br>"
"Signoff<br>"
"<textarea cols=80 rows=1 name=Bye>%s</textarea><br><br>"
"$U:Callsign of the user&nbsp; $I:First name of the user $X:Messages for user $x:Unread messages<br>"
"$L:Number of the latest message $N:Number of active messages. $Z:Last message read by user<br><br>"
"<input name=Save value=Save type=submit class='btn'> <inputcname=Cancel value=Cancel type=submit class='btn'></form>";

static char MsgEditPage[] = "<html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
"<title></title></head><body>"
"<form style=\"font-family: monospace;  text-align: center;\"method=post action=EMSave?%s>"
"<textarea cols=90 rows=33 name=Msg>%s</textarea><br><br>"
"<input name=Save value=Save type=submit class='btn'><input name=Cancel value=Cancel type=submit class='btn'><br></form>";

static char WPDetail[] = "<form style=\"font-family: monospace;\" method=post action=/Mail/WP?%s>"
"<br><table style=\"text-align: left; width: 431px;\" border=0 cellpadding=2 cellspacing=2><tbody>"
 
"<tr><td>Call</td><td><input readonly=readonly size=10 value=\"%s\"></td></tr>"
"<tr><td>Name</td><td><input size=30 name=Name value=\"%s\"></td></tr>"
"<tr><td>Home BBS 1</td><td><input size=40 name=Home1 value=%s></td></tr>"
"<tr><td>Home BBS 2</td><td><input size=40 name=Home2 value=%s></td></tr>"
"<tr><td>QTH 1</td><td><input size=40 name=QTH1 value=\"%s\"></td></tr>"
"<tr><td>QTH 2</td><td><input size=40 name=QTH2 value=\"%s\"></td></tr>"
"<tr><td>ZIP 1<br></td><td><input size=10 name=ZIP1 value=\"%s\"></td></tr>"
"<tr><td>ZIP 2<br></td><td><input size=10 name=ZIP2 value=\"%s\"></td></tr>"
"<tr><td>Last Seen<br></td><td><input size=15 name=Seen value=\"%s\"></td></tr>"
"<tr><td>Last Modified<br></td><td><input size=15 name=Modif value=\"%s\"></td></tr>"
"<tr><td>Type<br></td><td><input size=4 name=Type value=%c></td></tr>"
"<tr><td>Changed<br></td><td><input size=4 name=Changed value=%d></td></tr>"
"<tr><td>Seen<br></td><td><input size=4 name=Seen value=%d></td></tr></tbody></table>"
"<br><input onclick=save(this.form) value=Save type=button class='btn'> "
"<input onclick=del(this.form) value=Delete type=button class='btn'> "
"<input name=Cancel value=Cancel type=submit class='btn'></form>";


static char LostSession[] = "<html><body>"
"<form style=\"font-family: monospace; text-align: center;\" method=post action=/Mail/Lost?%s>"
"Sorry, Session had been lost<br><br>&nbsp;&nbsp;&nbsp;&nbsp;"
"<input name=Submit value=Restart type=submit class='btn'> <input type=submit class='btn' value=Exit name=Cancel><br></form>";


char * MsgEditTemplate = NULL;
char * HousekeepingTemplate = NULL;
char * ConfigTemplate = NULL;
char * WPTemplate = NULL;
char * UserListTemplate = NULL;
char * UserDetailTemplate = NULL;
char * FwdTemplate = NULL;
char * FwdDetailTemplate = NULL;
char * WebMailTemplate = NULL;
char * WebMailMsgTemplate = NULL;
char * jsTemplate = NULL;


#ifdef LINBPQ
UCHAR * GetBPQDirectory();
#endif

static int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately call is at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}

int SendHeader(char * Reply, char * Key)
{
	return sprintf(Reply, MailPage, BBSName, BBSName, Key, Key, Key, Key, Key, Key, Key, Key);
}


void ConvertTitletoUTF8(WebMailInfo * WebMail, char * Title, char * UTF8Title, int Len)
{
	Len = strlen(Title);

	if (WebIsUTF8(Title, Len) == FALSE)
	{
		int code = TrytoGuessCode(Title, Len);

		if (code == 437)
			Len = Convert437toUTF8(Title, Len, UTF8Title);
		else if (code == 1251)
			Len = Convert1251toUTF8(Title, Len, UTF8Title);
		else
			Len = Convert1252toUTF8(Title, Len, UTF8Title);

		UTF8Title[Len] = 0;
	}
	else
		strcpy(UTF8Title, Title);
}

BOOL GotFirstMessage = 0;

void ProcessMailHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen, int InputLen, char * Token)
{
	char * Context = 0, * NodeURL;
	int ReplyLen;
	BOOL LOCAL = FALSE;
	char * Key;
	char Appl = 'M';

	if (URL[0] == 0 || Method == NULL)
		return;

	if (strstr(input, "Host: 127.0.0.1"))
		LOCAL = TRUE;

	if (Session->TNC == (void *)1)					// Re-using an address as a flag
		LOCAL = TRUE;

	NodeURL = strtok_s(URL, "?", &Context);

	Key = Session->Key;

	if (_memicmp(URL, "/WebMail", 8) == 0)
	{
		// Pass All Webmail messages to Webmail
			
		ProcessWebMailMessage(Session, Context, LOCAL, Method, NodeURL, input, Reply, RLen, InputLen);
		return;

	}


	if (_memicmp(URL, "/Mail/API/v1/", 13) == 0)
	{
		*RLen = MailAPIProcessHTTPMessage(Session, Reply, Method, URL, input, LOCAL, Context, Token);
		return;
	}

	// There is a problem if Mail is reloaded without reloading the node

	if (GotFirstMessage == 0)
	{
		if (_stricmp(NodeURL, "/Mail/Header") ==  0 || _stricmp(NodeURL, "/Mail/Lost") == 0)
		{
			*RLen = SendHeader(Reply, Session->Key);
		}
		else
		{
			*RLen = sprintf(Reply, "<html><script>window.location.href = '/Mail/Header?%s';</script>", Session->Key);
		}
		
		GotFirstMessage = 1;
		return;
	}

	
	if (strcmp(Method, "POST") == 0)
	{	
		if (_stricmp(NodeURL, "/Mail/Header") == 0)
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}


		if (_stricmp(NodeURL, "/Mail/Config") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			ProcessConfUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/UI") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			ProcessUIUpdate(Session, input, Reply, RLen, Key);
			return ;
		}
		if (_stricmp(NodeURL, "/Mail/FwdCommon") == 0)
		{
			SaveFwdCommon(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/DisSession") == 0)
		{
			ProcessDisUser(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/UserDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				Session->User = LookupCall(param+4);
				if (Session->User)
				{
					* RLen = SendUserDetails(Session, Reply, Key); 
					return;
				}
			}
		}


		if (_stricmp(NodeURL, "/Mail/UserSave") == 0)
		{
			ProcessUserUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/MsgDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				int Msgno = atoi(param + 4);
				struct MsgInfo * Msg = FindMessageByNumber(Msgno);

				Session->Msg = Msg;				// Save current Message
	
				* RLen = SendMessageDetails(Msg, Reply, Key); 
				return;
			}
		}

		if (_stricmp(NodeURL, "/Mail/MsgSave") == 0)
		{
			ProcessMsgUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/EMSave") == 0)
		{
			//	Save Message Text

			SaveMessageText(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgAction") == 0)
		{
			ProcessMsgAction(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgFwdUpdate") == 0)
		{
			ProcessMsgFwdUpdate(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/Welcome") == 0)
		{
			SaveWelcome(Session, input, Reply, RLen, Key);
			return;
		}
		if (_stricmp(NodeURL, "/Mail/HK") == 0)
		{
			SaveHousekeeping(Session, input, Reply, RLen, Key);
			return;
		}
		if (_stricmp(NodeURL, "/Mail/WPDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				WPRec * WP = LookupWP(param+4);
				Session->WP = WP;				// Save current Message
	
				* RLen = SendWPDetails(WP, Reply, Key); 
				return;
			}
		}
		if (_stricmp(NodeURL, "/Mail/WPSave") == 0)
		{
			SaveWP(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgInfo.txt") == 0)
		{
			int n, len = 0;
			char  * FF = "", *FT = "", *FB = "", *FV = "";
			char * param, * ptr1, *ptr2;
			struct MsgInfo * Msg;
			char UCto[80];
			char UCfrom[80];
			char UCvia[80];
			char UCbid[80];

			// Get filter string

			param = strstr(input, "\r\n\r\n");	// End of headers
			

			if (param)
 			{
				ptr1 = param + 4;
				ptr2 = strchr(ptr1, '|');
				if (ptr2){*(ptr2++) = 0; FF = ptr1; ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FT = ptr1;ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FV = ptr1;ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FB = ptr1;ptr1 = ptr2;}
			}

			if (FT[0])
				_strupr(FT);
			if (FF[0])
				_strupr(FF);
			if (FV[0])
				_strupr(FV);
			if (FB[0])
				_strupr(FB);

			for (n = NumberofMessages; n >= 1; n--)
			{
				Msg = MsgHddrPtr[n];

				strcpy(UCto, Msg->to);
				strcpy(UCfrom, Msg->from);
				strcpy(UCvia, Msg->via);
				strcpy(UCbid, Msg->bid);

				_strupr(UCto);
				_strupr(UCfrom);
				_strupr(UCvia);
				_strupr(UCbid);

				if ((!FT[0] || strstr(UCto, FT)) &&
					(!FF[0] || strstr(UCfrom, FF)) &&
					(!FB[0] || strstr(UCbid, FB)) &&
					(!FV[0] || strstr(UCvia, FV)))
				{
					len += sprintf(&Reply[len], "%d|", Msg->number);
				}
			} 
			*RLen = len;
			return;
		}
		
		if (_stricmp(NodeURL, "/Mail/UserList.txt") == 0)
		{
			SendUserSelectPage(Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/FwdList.txt") == 0)
		{
			SendFwdSelectPage(Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/FwdDetails") == 0)
		{
			char * param;
			
			param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				Session->User = LookupCall(param+4);
				if (Session->User)
				{
					SendFwdDetails(Session->User, Reply, RLen, Key); 
					return;
				}
			}
		}

		if (_stricmp(NodeURL, "/Mail/FWDSave") == 0)
		{
			SaveFwdDetails(Session, input, Reply, RLen, Key);
			return ;
		}

		// End of POST section
	}

	if (strstr(NodeURL, "webscript.js"))
	{
		if (jsTemplate)
			free(jsTemplate);
		
		jsTemplate = GetTemplateFromFile(1, "webscript.js");

		ReplyLen = sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
			"Cache-Control: max-age=900\r\nContent-Type: text/javascript\r\n\r\n%s", (int)strlen(jsTemplate), jsTemplate);
		*RLen = ReplyLen;
		return;
	}


	if (_stricmp(NodeURL, "/Mail/Header") == 0)
	{
		*RLen = SendHeader(Reply, Session->Key);
 		return;
	}
	
	if (_stricmp(NodeURL, "/Mail/all.html") == 0)
	{
		*RLen = SendHeader(Reply, Session->Key);
 		return;
	}

	if (_stricmp(NodeURL, "/Mail/Status") == 0 ||
		_stricmp(NodeURL, "/Mail/DisSession") == 0)		// Sent as POST by refresh timer for some reason
	{
		SendStatusPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Conf") == 0)
	{
		if (ConfigTemplate)
			free(ConfigTemplate);

		ConfigTemplate = GetTemplateFromFile(7, "MainConfig.txt");

		SendConfigPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/FWD") == 0)
	{
		if (FwdTemplate)
			free(FwdTemplate);

		FwdTemplate = GetTemplateFromFile(4, "FwdPage.txt");

		if (FwdDetailTemplate)
			free(FwdDetailTemplate);

		FwdDetailTemplate = GetTemplateFromFile(3, "FwdDetail.txt");

		SendFwdMainPage(Reply, RLen, Key);
		return;
	}
	if (_stricmp(NodeURL, "/Mail/Wel") == 0)
	{
		SendWelcomePage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Users") == 0)
	{
		if (UserListTemplate)
			free(UserListTemplate);

		UserListTemplate = GetTemplateFromFile(4, "UserPage.txt");

		if (UserDetailTemplate)
			free(UserDetailTemplate);

		UserDetailTemplate = GetTemplateFromFile(4, "UserDetail.txt");

		*RLen = sprintf(Reply, UserListTemplate, Key, Key, BBSName,
			Key, Key, Key, Key, Key, Key, Key, Key);
	
		return;
	}

	if (_stricmp(NodeURL, "/Mail/SaveMessage") == 0)
	{
		struct MsgInfo * Msg = Session->Msg;
		char * MailBuffer;

		int Files = 0;
		int BodyLen;
		char * ptr;
		int WriteLen=0;
		char Hddr[1000];
		char FullTo[100];

		MailBuffer = ReadMessageFile(Msg->number);
		BodyLen = Msg->length;

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

		ptr[BodyLen] = 0;

		sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Disposition: attachment; filename=\"SavedMsg%05d.txt\" \r\n\r\n",
			(int)(strlen(Hddr) + strlen(ptr)), Msg->number);	
		strcat(Reply, Hddr);
		strcat(Reply, ptr);

		*RLen = (int)strlen(Reply);

		free(MailBuffer);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/SaveAttachment") == 0)
	{
		struct MsgInfo * Msg = Session->Msg;
		char * MailBuffer;

		int Files = 0, i;
		int BodyLen;
		char * ptr;
		int WriteLen=0;
		char FileName[100][MAX_PATH] = {""};
		int FileLen[100];
		char Noatt[] = "Message has no attachments";


		MailBuffer = ReadMessageFile(Msg->number);
		BodyLen = Msg->length;

		if ((Msg->B2Flags & Attachments) == 0)
		{
			sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",
				(int)strlen(Noatt), Noatt);
			*RLen = (int)strlen(Reply);

			free(MailBuffer);
			return;
		}
		
		ptr = MailBuffer;

		while(ptr && *ptr != 13)
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

		if (Files == 0)
		{
			sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",
				(int)strlen(Noatt), Noatt);
			*RLen = (int)strlen(Reply);
			free(MailBuffer);
			return;
		}

		*RLen = 0;

		//	For now only handle first

		i = 0;

//		for (i = 0; i < Files; i++)
		{
			int Len = sprintf(&Reply[*RLen], "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Disposition: attachment; filename=\"%s\" \r\n\r\n",
				FileLen[i], FileName[i]);

			memcpy(&Reply[Len + *RLen], ptr, FileLen[i]);
	
			*RLen += (Len + FileLen[i]);

			ptr += FileLen[i];
			ptr +=2;				// Over separator - I don't think there should be one
		}

		free(MailBuffer);
		return;
	}


	if (_stricmp(NodeURL, "/Mail/Msgs") == 0)
	{
		struct UserInfo * USER = NULL;
		int PageLen;

		if (MsgEditTemplate)
			free(MsgEditTemplate);

		MsgEditTemplate = GetTemplateFromFile(2, "MsgPage.txt");

		// Refresh BBS No to BBS list

		MaxBBS = 0;

		for (USER = BBSChain; USER; USER = USER->BBSNext)
		{
			int n = USER->BBSNumber;
			BBSLIST[n] = USER;
			if (n > MaxBBS)
				MaxBBS = n;
		}

		PageLen = 334 + (MaxBBS / 8) * 24;

		if (MsgEditTemplate)
		{
			int len =sprintf(Reply, MsgEditTemplate, PageLen, PageLen, PageLen - 97, Key, Key, Key, Key, Key, 
				BBSName, Key, Key, Key, Key, Key, Key, Key, Key);
			*RLen = len;
			return;
		}




	}

	if (_stricmp(NodeURL, "/Mail/EditM") == 0)
	{
		// Edit Message

		char * MsgBytes;
	
		MsgBytes = ReadMessageFile(Session->Msg->number);

		// See if Multipart

//		if (Msg->B2Flags & Attachments)
//			EnableWindow(GetDlgItem(hDlg, IDC_SAVEATTACHMENTS), TRUE);

		if (MsgBytes)
		{
			*RLen = sprintf(Reply, MsgEditPage, Key, MsgBytes);
			free (MsgBytes);
		}
		else
			*RLen = sprintf(Reply, MsgEditPage, Key, "Message Not Found");

		return;
	}

	if (_stricmp(NodeURL, "/Mail/HK") == 0)
	{
		if (HousekeepingTemplate)
			free(HousekeepingTemplate);

		HousekeepingTemplate = GetTemplateFromFile(2, "Housekeeping.txt");

		SendHouseKeeping(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/WP") == 0)
	{
		if (WPTemplate)
			free(WPTemplate);

		WPTemplate = GetTemplateFromFile(1, "WP.txt");

		if (WPTemplate)
		{
			int len =sprintf(Reply, WPTemplate, Key, Key, Key, Key,
				BBSName, Key, Key, Key, Key, Key, Key, Key, Key);
			*RLen = len;
			return;
		}

		return;
	}

	if (_stricmp(NodeURL, "/Mail/WPInfo.txt") == 0)
	{
		int i = 0, n, len = 0;
		WPRec * WP[10000]; 

		// Get array of addresses

		for (n = 1; n <= NumberofWPrecs; n++)
		{
			WP[i++] = WPRecPtr[n];
			if (i > 9999) break;
		}

		qsort((void *)WP, i, sizeof(void *), compare);

		for (i=0; i < NumberofWPrecs; i++)
		{
			len += sprintf(&Reply[len], "%s|", WP[i]->callsign);
		}

		*RLen = len;
		return;
	}


	ReplyLen = sprintf(Reply, MailSignon, BBSName, BBSName);
	*RLen = ReplyLen;

}

int SendWPDetails(WPRec * WP, char * Reply, char * Key)
{
	int len = 0;
	char D1[80], D2[80];
	
	if (WP)
	{
		strcpy(D1, FormatDateAndTime(WP->last_modif, FALSE));
		strcpy(D2, FormatDateAndTime(WP->last_seen, FALSE));

		len = sprintf(Reply, WPDetail, Key, WP->callsign, WP->name,
			WP->first_homebbs, WP->secnd_homebbs,
			WP->first_qth, WP->secnd_qth,
			WP->first_zip, WP->secnd_zip, D1, D2,
			WP->Type,
			WP->changed, 
			WP->seen);
	}
	return(len);	
}
VOID SaveWP(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	WPRec * WP = Session->WP;
	char * input, * ptr1, * ptr2;
	int n;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}

		if (strcmp(input + 4, "Delete") == 0)
		{			
			for (n = 1; n <= NumberofWPrecs; n++)
			{
				if (Session->WP == WPRecPtr[n])
					break;
			}
		
			if (n <= NumberofWPrecs)
			{
				WP = Session->WP;

				for (n = n; n < NumberofWPrecs; n++)
				{
					WPRecPtr[n] = WPRecPtr[n+1];		// move down all following entries
				}
	
				NumberofWPrecs--;
	
				free(WP);
			
				SaveWPDatabase();

				Session->WP = WPRecPtr[1];
			}
			*RLen = SendWPDetails(Session->WP, Reply, Session->Key);
 			return;
		}
	}
	if (input && WP)
	{
		ptr1 = input + 4;
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 12) ptr1[12] = 0;strcpy(WP->name, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 40) ptr1[40] = 0;strcpy(WP->first_homebbs, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 40) ptr1[40] = 0;strcpy(WP->secnd_homebbs, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 30) ptr1[30] = 0;strcpy(WP->first_qth, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 30) ptr1[30] = 0;strcpy(WP->secnd_qth, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 8) ptr1[8] = 0;strcpy(WP->first_zip, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 8) ptr1[8] = 0;strcpy(WP->secnd_zip, ptr1);ptr1 = ptr2;}

	//	GetParam(input, "BBSCall=", BBSName);


/*
	GetDlgItemText(hDlg, IDC_WPNAME, WP->name, 13);
	GetDlgItemText(hDlg, IDC_HOMEBBS1, WP->first_homebbs, 41);
	GetDlgItemText(hDlg, IDC_HOMEBBS2, WP->first_homebbs, 41);
	GetDlgItemText(hDlg, IDC_QTH1, WP->first_qth, 31);
	GetDlgItemText(hDlg, IDC_QTH2, WP->secnd_qth, 31);
	GetDlgItemText(hDlg, IDC_ZIP1, WP->first_zip, 31);
	GetDlgItemText(hDlg, IDC_ZIP2, WP->secnd_zip, 31);
		WP->seen = GetDlgItemInt(hDlg, IDC_SEEN, &OK1, FALSE);
*/
	
		WP->last_modif = time(NULL);
		WP->Type = 'U';
		WP->changed = 1;

		SaveWPDatabase();

		*RLen = SendWPDetails(WP, Reply, Key);
	}
}


int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key)
{	
	int BBSNo = 1, x, y, len = 0;
	char D1[80], D2[80], D3[80];
	struct UserInfo * USER;
	int i = 0, n;
	struct UserInfo * bbs[NBBBS+2] = {0}; 

	if (Msg)
	{
		char EmailFromLine[256] = "";

		strcpy(D1, FormatDateAndTime((time_t)Msg->datecreated, FALSE));
		strcpy(D2, FormatDateAndTime((time_t)Msg->datereceived, FALSE));
		strcpy(D3, FormatDateAndTime((time_t)Msg->datechanged, FALSE));

//		if (Msg->emailfrom[0])
			sprintf(EmailFromLine, "Email From <input style=\"width:320px;\" name=EFROM value=%s><br>", Msg->emailfrom);

		len = sprintf(Reply, MailDetailPage, Msg->number, Key,
			Msg->from, D1, 
			(Msg->type == 'B')?sel:"",
			(Msg->type == 'P')?sel:"",
			(Msg->type == 'T')?sel:"",
			Msg->to, D2,
			(Msg->status == 'N')?sel:"",
			(Msg->status == 'Y')?sel:"",
			(Msg->status == 'F')?sel:"",
			(Msg->status == 'K')?sel:"",
			(Msg->status == 'H')?sel:"",
			(Msg->status == 'D')?sel:"",
			(Msg->status == '$')?sel:"",
			Msg->bid, D3, Msg->length, EmailFromLine, Msg->via, Msg->title,
			Key, Msg->number, Key, Key,
			(Msg->B2Flags & Attachments)?"":"disabled");

		// Get a sorted list of BBS records

		for (n = 1; n <= NumberofUsers; n++)
		{
			USER = UserRecPtr[n];

			if ((USER->flags & F_BBS))
				if (USER->BBSNumber)
					bbs[i++] = USER;
		}

		qsort((void *)bbs, i, sizeof(void *), compare );

		n = 0;
		
		for (y = 0; y < NBBBS/8; y++)
		{
			len += sprintf(&Reply[len],"<tr>");
			for (x= 0; x < 8; x++)
			{
				char * Colour  = NotThisOne;	

				if (bbs[n])
				{
					if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)	
						if (check_fwd_bit(Msg->fbbs, bbs[n]->BBSNumber))
								Colour = ToSend;
					if (memcmp(Msg->forw, zeros, NBMASK) != 0)	
						if (check_fwd_bit(Msg->forw, bbs[n]->BBSNumber))
								Colour = Sent;
				
					len += sprintf(&Reply[len],"<td style=\"background-color: %s;\"onclick=ck(\"%d\")>%s</td>",
						Colour, bbs[n]->BBSNumber, bbs[n]->Call);
				}
				else
					len += sprintf(&Reply[len], "<td>&nbsp;</td>");

				n++;
			}
			len += sprintf(&Reply[len],"</tr>");
			if (n > i)
				break;
		}
		len += sprintf(&Reply[len], "%s", MailDetailTail);
	}
	return(len);	
}

char ** GetMultiStringInput(char * input, char * key)
{
	char MultiString[16384] = "";

	GetParam(input, key, MultiString);

	if (MultiString[0] == 0)
		return NULL;

	return SeparateMultiString(MultiString, TRUE);
}

char **	SeparateMultiString(char * MultiString, BOOL NoToUpper)
{
	char * ptr1 = MultiString;
	char * ptr2 = NULL;
	char * DecodedString;
	char ** Value;
	int Count = 0;
	char c;
	char * ptr;

	ptr2 = zalloc(strlen(MultiString) + 1);
	DecodedString = ptr2;

	// Input has crlf or lf - replace with |

	while (*ptr1)
	{
		c = *(ptr1++);

		if (c == 13)
			continue;

		if (c == 10)
		{
			*ptr2++ = '|';
		}
		else
			*(ptr2++) = c;
	}

	// Convert to string array

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;

	ptr = DecodedString;

	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, '|');
			
		if (ptr1)
			*(ptr1++) = 0;

		if (strlen(ptr))
		{
			Value = realloc(Value, (Count+2) * sizeof(void *));
			if (_memicmp(ptr, "file ", 5) == 0 || NoToUpper)
				Value[Count++] = _strdup(ptr);
			else
				Value[Count++] = _strupr(_strdup(ptr));
		}
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}

VOID GetMallocedParam(char * input, char * key, char ** value)
{
	char Param[32768] = "";

	GetParam(input, key, Param);

	if (Param[0])
	{
		free(*value);
		*value = _strdup(Param);
	}
}

VOID GetParam(char * input, char * key, char * value)
{
	char * ptr = strstr(input, key);
	char Param[32768];
	char * ptr1, * ptr2;
	char c;

	if (ptr)
	{
		ptr2 = strchr(ptr, '&');
		if (ptr2) *ptr2 = 0;
		strcpy(Param, ptr + strlen(key));
		if (ptr2) *ptr2 = '&';					// Restore string

		// Undo any % transparency

		ptr1 = Param;
		ptr2 = Param;

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

				*(ptr2++) = m * 16 + n;
			}
			else if (c == '+')
				*(ptr2++) = ' ';
			else
				*(ptr2++) = c;

			c = *(ptr1++);
		}

		*(ptr2++) = 0;

		strcpy(value, Param);
	}
}

VOID GetCheckBox(char * input, char * key, int * value)
{
	char * ptr = strstr(input, key);
	if (ptr)
		*value = 1;
	else
		*value = 0;
}

	
VOID * GetOverrideFromString(char * input)
{
	char * ptr1;
	char * MultiString = NULL;
	char * ptr = input;
	int Count = 0;
	struct Override ** Value;
	char * Val;

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;
	
	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, 13);
			
		if (ptr1)
		{
			*(ptr1) = 0;
			ptr1 += 2;
		}
		Value = realloc(Value, (Count+2) * sizeof(void *));
		Value[Count] = zalloc(sizeof(struct Override));
		Val = strlop(ptr, ',');
		if (Val == NULL)
			break;

		Value[Count]->Call = _strupr(_strdup(ptr));
		Value[Count++]->Days = atoi(Val);
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}




VOID SaveHousekeeping(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	char Temp[80];
	struct tm *tm;
	time_t now;
	
	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "RunNow="))
		{
			DoHouseKeeping(FALSE);
			SendHouseKeeping(Reply, RLen, Key);
			return;
		}
		if (strstr(input, "Cancel=Cancel"))
		{
			SendHouseKeeping(Reply, RLen, Key);
 			return;
		}

		GetParam(input, "MTTime=", Temp);
		MaintTime = atoi(Temp);
		GetParam(input, "MTInt=", Temp);
		MaintInterval = atoi(Temp);
		GetParam(input, "MAXMSG=", Temp);
		MaxMsgno = atoi(Temp);
		GetParam(input, "BIDLife=", Temp);
		BidLifetime= atoi(Temp);
		GetParam(input, "MaxAge=", Temp);
		MaxAge = atoi(Temp);
		GetParam(input, "LogLife=", Temp);
		LogAge = atoi(Temp);
		GetParam(input, "UserLife=", Temp);
		UserLifetime= atoi(Temp);

		GetCheckBox(input, "Deltobin=", &DeletetoRecycleBin);
		GetCheckBox(input, "SendND=", &SendNonDeliveryMsgs);
		GetCheckBox(input, "NoMail=", &SuppressMaintEmail);
		GetCheckBox(input, "GenTraffic=", &GenerateTrafficReport);
		GetCheckBox(input, "OvUnsent=", &OverrideUnsent);

		GetParam(input, "PR=", Temp);
		PR = atof(Temp);
		GetParam(input, "PUR=", Temp);
		PUR = atof(Temp);
		GetParam(input, "PF=", Temp);
		PF = atof(Temp);
		GetParam(input, "PUF=", Temp);
		PNF = atof(Temp);
		GetParam(input, "BF=", Temp);
		BF = atoi(Temp);
		GetParam(input, "BUF=", Temp);
		BNF = atoi(Temp);

		GetParam(input, "NTSD=", Temp);
		NTSD = atoi(Temp);

		GetParam(input, "NTSF=", Temp);
		NTSF = atoi(Temp);

		GetParam(input, "NTSU=", Temp);
		NTSU = atoi(Temp);

		GetParam(input, "From=", LTFROMString);
		LTFROM = GetOverrideFromString(LTFROMString);

		GetParam(input, "To=", LTTOString);
		LTTO = GetOverrideFromString(LTTOString);

		GetParam(input, "At=", LTATString);
		LTAT = GetOverrideFromString(LTATString);
 
		SaveConfig(ConfigName);
		GetConfig(ConfigName);

		// Calulate time to run Housekeeping
	
		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

//		MaintClock = _mkgmtime(tm);
		MaintClock = mktime(tm) - (time_t)_MYTIMEZONE;

		while (MaintClock < now)
			MaintClock += MaintInterval * 3600;

		Debugprintf("Maint Clock %d NOW %d Time to HouseKeeping %d", MaintClock, now, MaintClock - now);
	}
	SendHouseKeeping(Reply, RLen, Key);
	return;
}







VOID SaveWelcome(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}
		
		GetMallocedParam(input, "NUWelcome=", &WelcomeMsg);
		GetMallocedParam(input, "NewWelcome=", &NewWelcomeMsg);
		GetMallocedParam(input, "ExWelcome=", &ExpertWelcomeMsg);

		TidyWelcomeMsg(&WelcomeMsg);
		TidyWelcomeMsg(&NewWelcomeMsg);
		TidyWelcomeMsg(&ExpertWelcomeMsg);

		GetMallocedParam(input, "NUPrompt=", &Prompt);
		GetMallocedParam(input, "NewPrompt=", &NewPrompt);
		GetMallocedParam(input, "ExPrompt=", &ExpertPrompt);
		TidyPrompts();

		GetParam(input, "Bye=", &SignoffMsg[0]);
		if (SignoffMsg[0])
		{
			if (SignoffMsg[strlen(SignoffMsg) - 1] == 10)
				SignoffMsg[strlen(SignoffMsg) - 1] = 0;

			if (SignoffMsg[strlen(SignoffMsg) - 1] != 13)
				strcat(SignoffMsg, "\r");
		}

		if (SignoffMsg[0] == 13)
			SignoffMsg[0] = 0;
	}
	
	SendWelcomePage(Reply, RLen, Key);
	return;
}

VOID ProcessConfUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;
	char Temp[80];

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}

		if (strstr(input, "ConfigUI=Config+UI"))
		{
			SendUIPage(Reply, RLen, Key);
			return;
		}
	
		GetParam(input, "BBSCall=", BBSName);
		_strupr(BBSName);
		strlop(BBSName, '-');
		GetParam(input, "SYSOPCall=", SYSOPCall);
		_strupr(SYSOPCall);
		strlop(SYSOPCall, '-');
		GetParam(input, "HRoute=", HRoute);
		_strupr(HRoute);
		GetParam(input, "ApplNum=", Temp);
		BBSApplNum = atoi(Temp);
		GetParam(input, "Streams=", Temp);
		MaxStreams = atoi(Temp);

		GetCheckBox(input, "SysToSYSOP=", &SendSYStoSYSOPCall);
		GetCheckBox(input, "BBSToSYSOP=", &SendBBStoSYSOPCall);
		GetCheckBox(input, "RefuseBulls=", &RefuseBulls);
		GetCheckBox(input, "EnUI=", &EnableUI);

		GetParam(input, "UIInterval=", Temp);
		MailForInterval = atoi(Temp);

		GetCheckBox(input, "DontHold=", &DontHoldNewUsers);
		GetCheckBox(input, "DefaultNoWinlink=", &DefaultNoWINLINK);
		GetCheckBox(input, "DontNeedName=", &AllowAnon);
		GetCheckBox(input, "DontNeedHomeBBS=", &DontNeedHomeBBS);
		GetCheckBox(input, "DontCheckFromCall=", &DontCheckFromCall);
		GetCheckBox(input, "UserCantKillT=", &UserCantKillT);
		UserCantKillT = !UserCantKillT;	// Reverse Logic
		GetCheckBox(input, "FWDtoMe=", &ForwardToMe);
		GetCheckBox(input, "OnlyKnown=", &OnlyKnown);
		GetCheckBox(input, "Events=", &reportMailEvents);

		GetParam(input, "POP3Port=", Temp);
		POP3InPort = atoi(Temp);

		GetParam(input, "SMTPPort=", Temp);
		SMTPInPort = atoi(Temp);

		GetParam(input, "NNTPPort=", Temp);
		NNTPInPort = atoi(Temp);

		GetCheckBox(input, "EnRemote=", &RemoteEmail);

		GetCheckBox(input, "EnISP=", &ISP_Gateway_Enabled);
		GetCheckBox(input, "SendAMPR=", &SendAMPRDirect);

		GetParam(input, "AMPRDomain=", AMPRDomain);

		GetParam(input, "ISPDomain=", MyDomain);
		GetParam(input, "SMTPServer=", ISPSMTPName);
		GetParam(input, "ISPEHLOName=", ISPEHLOName);
			
		GetParam(input, "ISPSMTPPort=", Temp);
		ISPSMTPPort = atoi(Temp);
	
		GetParam(input, "POP3Server=", ISPPOP3Name);

		GetParam(input, "ISPPOP3Port=", Temp);	
		ISPPOP3Port = atoi(Temp);
	
		GetParam(input, "ISPAccount=", ISPAccountName);
	
		GetParam(input, "ISPPassword=", ISPAccountPass);
		EncryptedPassLen = EncryptPass(ISPAccountPass, EncryptedISPAccountPass);
		
		GetParam(input, "PollInterval=", Temp);
		ISPPOP3Interval = atoi(Temp);

		GetCheckBox(input, "ISPAuth=", &SMTPAuthNeeded);

		GetCheckBox(input, "EnWP=", &SendWP);
		GetCheckBox(input, "RejWFBulls=", &FilterWPBulls);

		if (strstr(input, "Type=TypeB"))
			SendWPType = 0;

		if (strstr(input, "Type=TypeP"))
			SendWPType = 1;

		SendWPAddrs = GetMultiStringInput(input, "WPTO=");

		RejFrom = GetMultiStringInput(input, "Rfrom=");
		RejTo = GetMultiStringInput(input, "Rto=");
		RejAt = GetMultiStringInput(input, "Rat=");
		RejBID = GetMultiStringInput(input, "RBID=");
		HoldFrom = GetMultiStringInput(input, "Hfrom=");
		HoldTo = GetMultiStringInput(input, "Hto=");
		HoldAt = GetMultiStringInput(input, "Hat=");
		HoldBID = GetMultiStringInput(input, "HBID=");

		// Look for fbb style filters

		input = strstr(input, "&Action=");

		// delete old list

		while(Filters && Filters->Next)
		{
			FBBFilter * next = Filters->Next;
			free(Filters);
			Filters = next;
		}

		free(Filters);
		Filters = NULL;

		UndoTransparency(input);

		while (input)
		{
			// extract and validate before saving

			FBBFilter Filter;
			FBBFilter * PFilter;

			memset(&Filter, 0, sizeof(FBBFilter));

			Filter.Action = toupper(input[8]);

			input = strstr(input, "&Type=");
			
			if (Filter.Action == 'H' || Filter.Action == 'R' || Filter.Action == 'A')
			{
				Filter.Type = toupper(input[6]);
				input = strstr(input, "&From=");
				memcpy(Filter.From, &input[6], 10);
				input = strstr(input, "&TO=");
				strlop(Filter.From, '&');
				_strupr(Filter.From);
				memcpy(Filter.TO, &input[4], 10);
				input = strstr(input, "&AT=");
				strlop(Filter.TO, '&');
				_strupr(Filter.TO);
				memcpy(Filter.AT, &input[4], 10);
				input = strstr(input, "&BID=");
				strlop(Filter.AT, '&');
				_strupr(Filter.AT);
				memcpy(Filter.BID, &input[5], 10);
				input = strstr(input, "&MaxLen=");
				strlop(Filter.BID, '&');
				_strupr(Filter.BID);
				Filter.MaxLen = atoi(&input[8]);

				if (Filter.Type == '&') Filter.Type = '*';
				if (Filter.From[0] == 0) strcpy(Filter.From, "*");
				if (Filter.TO[0] == 0) strcpy(Filter.TO, "*");
				if (Filter.AT[0] == 0) strcpy(Filter.AT, "*");
				if (Filter.BID[0] == 0) strcpy(Filter.BID, "*");

				// add to list

				PFilter = zalloc(sizeof(FBBFilter));

				memcpy(PFilter, &Filter, sizeof(FBBFilter));

				if (Filters == 0)
					Filters = PFilter;
				else
				{
					FBBFilter * p = Filters;

					while (p->Next)
						p = p->Next;

					p->Next = PFilter;
				}
			}

			input = strstr(input, "&Action=");
		}

		SaveConfig(ConfigName);
		GetConfig(ConfigName);
	}
	
	SendConfigPage(Reply, RLen, Key);
	return;
}



VOID ProcessUIUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0, i;
	char * input;
	struct UserInfo * USER = NULL;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
			return;
		}

		GetParam(input, "MailFor=", &MailForText[0]);

		for (i = 1; i <= GetNumberofPorts(); i++)
		{
			char EnKey[10];
			char DigiKey[10];
			char MFKey[12];
			char HDDRKey[12];
			char NullKey[12];
			char Temp[100];

			sprintf(EnKey, "En%d=", i);
			sprintf(DigiKey, "Path%d=", i);
			sprintf(MFKey, "SndMF%d=", i);
			sprintf(HDDRKey, "SndHDDR%d=", i);
			sprintf(NullKey, "SndNull%d=", i);

			GetCheckBox(input, EnKey, &UIEnabled[i]);
			GetParam(input, DigiKey, Temp);
			if (UIDigi[i])
				free (UIDigi[i]);
			UIDigi[i] = _strdup(Temp);
			GetCheckBox(input, MFKey, &UIMF[i]);
			GetCheckBox(input, HDDRKey, &UIHDDR[i]);
			GetCheckBox(input, NullKey, &UINull[i]);
		}

		SaveConfig(ConfigName);
		GetConfig(ConfigName);
	}

	SendUIPage(Reply, RLen, Key);
	return;
}

VOID ProcessDisUser(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	char * input;
	char * ptr;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		ptr = strstr(input, "call=");
		if (ptr)
		{
			int Stream = atoi(ptr + 5);
			Disconnect(Stream);
		}
	}	
	SendStatusPage(Reply, RLen, Rest);
}



VOID SaveFwdCommon(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;

	char Temp[80];
	int Mask = 0;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		int n;
		GetParam(input, "MaxTX=", Temp);
		MaxTXSize = atoi(Temp);
		GetParam(input, "MaxRX=", Temp);
		MaxRXSize = atoi(Temp);
		GetParam(input, "MaxAge=", Temp);
		MaxAge = atoi(Temp);
		GetCheckBox(input, "WarnNoRoute=", &WarnNoRoute);
		GetCheckBox(input, "LocalTime=", &Localtime);
		GetCheckBox(input, "SendPtoMultiple=", &SendPtoMultiple);
		GetCheckBox(input, "FourCharCont=", &FOURCHARCONT);

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
	
		AliasText = GetMultiStringInput(input, "Aliases=");

		if (AliasText)
		{
			n = 0;

			while (AliasText[n])
			{
				_strupr(AliasText[n]);
				n++;
			}
		}
		SetupFwdAliases();
	}
	
	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	SendFwdMainPage(Reply, RLen, Session->Key);
}

char * GetNextParam(char ** next)
{
	char * ptr1 = *next;
	char * ptr2 = strchr(ptr1, '|');
	if (ptr2)
	{
		*(ptr2++) = 0;
		*next = ptr2;
	}
	return ptr1;
}

VOID SaveFwdDetails(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = Session->User;
	struct BBSForwardingInfo * FWDInfo = USER->ForwardingInfo;
	char * ptr1, *ptr2;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "StartForward"))
		{
			StartForwarding(USER->BBSNumber, NULL);
			SendFwdDetails(Session->User, Reply, RLen, Session->Key);
			return;
		}

		if (strstr(input, "CopyForward"))
		{
			struct UserInfo * OldBBS;

			// Get call to copy from 

			ptr2 = input + 4;
			ptr1 = GetNextParam(&ptr2);		// Call
			_strupr(ptr2);

			OldBBS = FindBBS(ptr2);

			if (OldBBS == NULL)
			{

				*RLen = sprintf(Reply, "<h3 align=center>Copy From BBS %s not found</h3>", ptr2);
				return;
			}

			// Set current info from OldBBS
//
//			SetForwardingPage(hDlg, OldBBS);			// moved to separate routine as also called from copy config

			SendFwdDetails(OldBBS, Reply, RLen, Session->Key);
			return;
		}
		// Fwd update

		ptr2 = input + 4;
		ptr1 = GetNextParam(&ptr2);		// TO
		FWDInfo->TOCalls = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// AT
		FWDInfo->ATCalls = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// TIMES
		FWDInfo->FWDTimes = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// FWD SCRIPT
		FWDInfo->ConnectScript = SeparateMultiString(ptr1, TRUE);

		ptr1 = GetNextParam(&ptr2);		// HRB
		FWDInfo->Haddresses = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// HRP
		FWDInfo->HaddressesP = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// BBSHA
		if (FWDInfo->BBSHA)
			free(FWDInfo->BBSHA);

		FWDInfo->BBSHA = _strdup(_strupr(ptr1));

		ptr1 = GetNextParam(&ptr2);		// EnF
		if (strcmp(ptr1, "true") == 0) FWDInfo->Enabled = TRUE; else FWDInfo->Enabled = FALSE;

		ptr1 = GetNextParam(&ptr2);		// Interval
		FWDInfo->FwdInterval = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// EnR
		if (strcmp(ptr1, "true") == 0) FWDInfo->ReverseFlag = TRUE; else FWDInfo->ReverseFlag = FALSE;

		ptr1 = GetNextParam(&ptr2);		// RInterval
		FWDInfo->RevFwdInterval = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->SendNew = TRUE; else FWDInfo->SendNew = FALSE;

		ptr1 = GetNextParam(&ptr2);		// Blocked
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowBlocked = TRUE; else FWDInfo->AllowBlocked = FALSE;

		ptr1 = GetNextParam(&ptr2);		// FBB Block
		FWDInfo->MaxFBBBlockSize = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// Personals
		if (strcmp(ptr1, "true") == 0) FWDInfo->PersonalOnly = TRUE; else FWDInfo->PersonalOnly = FALSE;
		ptr1 = GetNextParam(&ptr2);		// Binary
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowCompressed = TRUE; else FWDInfo->AllowCompressed = FALSE;
		ptr1 = GetNextParam(&ptr2);		// B1
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB1 = TRUE; else FWDInfo->AllowB1 = FALSE;
		ptr1 = GetNextParam(&ptr2);		// B2
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB2 = TRUE; else FWDInfo->AllowB2 = FALSE;
		ptr1 = GetNextParam(&ptr2);		// CTRLZ
		if (strcmp(ptr1, "true") == 0) FWDInfo->SendCTRLZ = TRUE; else FWDInfo->SendCTRLZ = FALSE;
		ptr1 = GetNextParam(&ptr2);		// Connect Timeout
		FWDInfo->ConTimeout = atoi(ptr1);

		SaveConfig(ConfigName);
		GetConfig(ConfigName);

		ReinitializeFWDStruct(Session->User);
	
		SendFwdDetails(Session->User, Reply, RLen, Session->Key);
	}
}



VOID ProcessUserUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = Session->User;
	int SSID, Mask = 0;
	char * ptr1, *ptr2;
	int skipRMSExUser = 0;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
			return;
		}

		if (strstr(input, "Delete"))
		{
			int n;

			for (n = 1; n <= NumberofUsers; n++)
			{
				if (Session->User == UserRecPtr[n])
					break;
			}
		
			if (n <= NumberofUsers)
			{
				USER = Session->User;

				for (n = n; n < NumberofUsers; n++)
				{
					UserRecPtr[n] = UserRecPtr[n+1];		// move down all following entries
				}
	
				NumberofUsers--;

				if (USER->flags & F_BBS)	// was a BBS?
					DeleteBBS(USER);
	
				free(USER);
			
				SaveUserDatabase();

				Session->User = UserRecPtr[1];

				SendUserSelectPage(Reply, RLen, Session->Key);
				return;
			}
		}

		if (strstr(input, "Add="))
		{
			char * Call;

			Call = input + 8;
			strlop(Call, '-');

			if (strlen(Call) > 6)
				Call[6] = 0;

			_strupr(Call);
		
			if (Call[0] == 0 || LookupCall(Call))
			{
				// Null or exists

				SendUserSelectPage(Reply, RLen, Session->Key);
				return;
			}

			USER = AllocateUserRecord(Call);
			USER->Temp = zalloc(sizeof (struct TempUserInfo));
		
			SendUserSelectPage(Reply, RLen, Session->Key);
			return;

		}

		// User update

		ptr2 = input + 4;
		ptr1 = GetNextParam(&ptr2);		// BBS

		// If BBS Flag has changed, must set up or delete forwarding info

		if (strcmp(ptr1, "true") == 0)
		{
			if ((USER->flags & F_BBS) == 0)
			{
				// New BBS

				if(SetupNewBBS(USER))
				{
					USER->flags |= F_BBS;
					USER->flags &= ~F_Temp_B2_BBS;		// Clear RMS Express User
					skipRMSExUser = 1;					// Dont read old value
				}
				else
				{
					// Failed - too many bbs's defined

					//sprintf(InfoBoxText, "Cannot set user to be a BBS - you already have 80 BBS's defined");
					//DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
					USER->flags &= ~F_BBS;
					//CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
				}
			}
		}
		else
		{
			if (USER->flags & F_BBS)
			{
				//was a BBS

				USER->flags &= ~F_BBS;
				DeleteBBS(USER);
			}
		}

		ptr1 = GetNextParam(&ptr2);		// Permit Email
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_EMAIL; else USER->flags &= ~F_EMAIL;

		ptr1 = GetNextParam(&ptr2);		// PMS
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_PMS; else USER->flags &= ~F_PMS;

		ptr1 = GetNextParam(&ptr2);		// RMS EX User
		if (strcmp(ptr1, "true") == 0 && !skipRMSExUser) USER->flags |= F_Temp_B2_BBS; else USER->flags &= ~F_Temp_B2_BBS;
		ptr1 = GetNextParam(&ptr2);		// SYSOP
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_SYSOP; else USER->flags &= ~F_SYSOP;
		ptr1 = GetNextParam(&ptr2);		// PollRMS
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_POLLRMS; else USER->flags &= ~F_POLLRMS;
		ptr1 = GetNextParam(&ptr2);		// Expert
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_Expert; else USER->flags &= ~F_Expert;

		ptr1 = GetNextParam(&ptr2);		// SSID1
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID2
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID3
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID4
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		Session->User->RMSSSIDBits = Mask;

		ptr1 = GetNextParam(&ptr2);		// Excluded
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_Excluded; else USER->flags &= ~F_Excluded;
		ptr1 = GetNextParam(&ptr2);		// Hold
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_HOLDMAIL; else USER->flags &= ~F_HOLDMAIL;
		ptr1 = GetNextParam(&ptr2);		// SYSOP gets LM
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_SYSOP_IN_LM; else USER->flags &= ~F_SYSOP_IN_LM;
		ptr1 = GetNextParam(&ptr2);		// Dont add winlink.org
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_NOWINLINK; else USER->flags &= ~F_NOWINLINK;
		ptr1 = GetNextParam(&ptr2);		// Allow Bulls
		if (strcmp(ptr1, "true") == 0) USER->flags &= ~F_NOBULLS; else USER->flags |= F_NOBULLS;	// Inverted flag
		ptr1 = GetNextParam(&ptr2);		// NTS Message Pickup Station
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_NTSMPS; else USER->flags &= ~F_NTSMPS;
		ptr1 = GetNextParam(&ptr2);		// APRS Mail For
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_RMSREDIRECT; else USER->flags &= ~F_RMSREDIRECT;
		ptr1 = GetNextParam(&ptr2);		// Redirect to RMS

		if (strcmp(ptr1, "true") == 0) USER->flags |= F_APRSMFOR; else USER->flags &= ~F_APRSMFOR;
	
		ptr1 = GetNextParam(&ptr2);		// APRS SSID
		SSID = atoi(ptr1);
		SSID &= 15;
		USER->flags &= 0x0fffffff;
		USER->flags |= (SSID << 28);


		ptr1 = GetNextParam(&ptr2);		// Last Listed
		USER->lastmsg = atoi(ptr1);
		ptr1 = GetNextParam(&ptr2);		// Name
		memcpy(USER->Name, ptr1, 17);
		ptr1 = GetNextParam(&ptr2);		// Pass
		memcpy(USER->pass, ptr1, 12);		
		ptr1 = GetNextParam(&ptr2);		// CMS Pass
		if (memcmp("****************", ptr1, strlen(ptr1) != 0))
		{
			memcpy(USER->CMSPass, ptr1, 15);
		}
		
		ptr1 = GetNextParam(&ptr2);		// QTH
		memcpy(USER->Address, ptr1, 60);
		ptr1 = GetNextParam(&ptr2);		// ZIP
		memcpy(USER->ZIP, ptr1, 8);
		ptr1 = GetNextParam(&ptr2);		// HomeBBS
		memcpy(USER->HomeBBS, ptr1, 40);
		_strupr(USER->HomeBBS);

		SaveUserDatabase();
		UpdateWPWithUserInfo(USER);

		*RLen = SendUserDetails(Session, Reply, Session->Key);
	}
}

VOID ProcessMsgAction(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr1;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		ptr1 = input + 4;
		*RLen = SendMessageDetails(Msg, Reply, Session->Key);
	}
}

VOID SaveMessageText(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr, * ptr1, * ptr2, *input;
	char c;
	int MsgLen, WriteLen;
	char MsgFile[256];
	FILE * hFile;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = sprintf(Reply, "%s", "<html><script>window.close();</script></html>");
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

				*(ptr2++) = m * 16 + n;
			}
			else if (c == '+')
				*(ptr2++) = ' ';
			else
				*(ptr2++) = c;

			c = *(ptr1++);
		}

		*(ptr2++) = 0;

		MsgLen = (int)strlen(input + 8);

		Msg->datechanged = time(NULL);
		Msg->length = MsgLen;

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
			hFile = fopen(MsgFile, "wb");
	
			if (hFile)
			{
				WriteLen = (int)fwrite(input + 8, 1, Msg->length, hFile); 
				fclose(hFile);
			}

			if (WriteLen != Msg->length)
			{
				char Mess[80];
				sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
				CriticalErrorHandler(Mess);

				return;
			}

			SaveMessageDatabase();

			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"Message Saved\");window.close();</script></html>");

		}
	}
	return;

}


VOID ProcessMsgUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr1, * ptr2;
	char OldStatus = Msg->status;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		ptr1 = input + 4;
		ptr2 = strchr(ptr1, '|');
		if (ptr2)
		{
			*(ptr2++) = 0;
			strcpy(Msg->from, ptr1);
			ptr1 = ptr2;
		}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->to, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->bid, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->emailfrom, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->via, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->title, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->type = *ptr1;ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->status = *ptr1;ptr1 = ptr2;}

		if (Msg->status != OldStatus)
		{
			// Need to take action if killing message

			if (Msg->status == 'K')
				FlagAsKilled(Msg, FALSE);					// Clear forwarding bits
		}

		Msg->datechanged = time(NULL);
		SaveMessageDatabase();
	}

	*RLen = SendMessageDetails(Msg, Reply, Session->Key);
}




VOID ProcessMsgFwdUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct UserInfo * User;
	struct MsgInfo * Msg = Session->Msg;
	BOOL toforward, forwarded;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		BBSNumber = atoi(input + 4);
		User = BBSLIST[BBSNumber];

		if (User == NULL)
			return;
		
		toforward = check_fwd_bit(Msg->fbbs, BBSNumber);
		forwarded = check_fwd_bit(Msg->forw, BBSNumber);

		if (forwarded)
		{
			// Changing to not this BBS

			clear_fwd_bit(Msg->forw, BBSNumber);
		}
		else if (toforward)
		{
			// Change to Forwarded

			clear_fwd_bit(Msg->fbbs, BBSNumber);
			User->ForwardingInfo->MsgCount--;
			set_fwd_bit(Msg->forw, BBSNumber);
		}
		else
		{
			// Change to to forward
			
			set_fwd_bit(Msg->fbbs, BBSNumber);
			User->ForwardingInfo->MsgCount++;
			clear_fwd_bit(Msg->forw, BBSNumber);
			if (FirstMessageIndextoForward > GetMessageSlotFromMessageNumber(Msg->number))
				FirstMessageIndextoForward = GetMessageSlotFromMessageNumber(Msg->number);

		}
		*RLen = SendMessageDetails(Msg, Reply, Session->Key);
	}
	SaveMessageDatabase();
}




VOID SetMultiStringValue(char ** values, char * Multi)
{
	char ** Calls;
	char * ptr = &Multi[0];

	*ptr = 0;

	if (values)
	{
		Calls = values;

		while(Calls[0])
		{
			strcpy(ptr, Calls[0]);
			ptr += strlen(Calls[0]);
			*(ptr++) = '\r';
			*(ptr++) = '\n';
			Calls++;
		}
		*(ptr) = 0;
	}
}



VOID SendFwdDetails(struct UserInfo * User, char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	struct BBSForwardingInfo * FWDInfo = User->ForwardingInfo;
	char TO[2048] = "";
	char AT[2048] = "";
	char TIMES[2048] = "";
	char FWD[100000] = "";
	char HRB[2048] = "";
	char HRP[2048] = "";

	SetMultiStringValue(FWDInfo->TOCalls, TO);
	SetMultiStringValue(FWDInfo->ATCalls, AT);
	SetMultiStringValue(FWDInfo->FWDTimes, TIMES);
	SetMultiStringValue(FWDInfo->ConnectScript, FWD);
	SetMultiStringValue(FWDInfo->Haddresses, HRB);
	SetMultiStringValue(FWDInfo->HaddressesP, HRP);

	if (FwdDetailTemplate == NULL)
		FwdDetailTemplate = GetTemplateFromFile(3, "FwdDetail.txt");
		
	Len = sprintf(Reply, FwdDetailTemplate, User->Call,
		CountMessagestoForward (User), Key,
		TO, AT, TIMES , FWD, HRB, HRP, 
		(FWDInfo->BBSHA) ? FWDInfo->BBSHA : "", 
		(FWDInfo->Enabled) ? CHKD  : UNC,
		FWDInfo->FwdInterval,
		(FWDInfo->ReverseFlag) ? CHKD  : UNC,
		FWDInfo->RevFwdInterval, 
		(FWDInfo->SendNew) ? CHKD  : UNC,
		(FWDInfo->AllowBlocked) ? CHKD  : UNC,
		FWDInfo->MaxFBBBlockSize,
		(FWDInfo->PersonalOnly) ? CHKD  : UNC,
		(FWDInfo->AllowCompressed) ? CHKD  : UNC,
		(FWDInfo->AllowB1) ? CHKD  : UNC,
		(FWDInfo->AllowB2) ? CHKD  : UNC,
		(FWDInfo->SendCTRLZ) ? CHKD  : UNC,
		FWDInfo->ConTimeout);

	*ReplyLen = Len;

}

VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len, i;

	char HF[2048] = "";
	char HT[2048] = "";
	char HA[2048] = "";
	char HB[2048] = "";
	char RF[2048] = "";
	char RT[2048] = "";
	char RA[2048] = "";
	char RB[2048] = "";
	char WPTO[10000] = "";

	char FBBFilters[100000] = "";

	
	char * ptr = FBBFilters;
	FBBFilter * Filter = Filters;

	SetMultiStringValue(RejFrom, RF);
	SetMultiStringValue(RejTo, RT);
	SetMultiStringValue(RejAt, RA);
	SetMultiStringValue(RejBID, RB);
	SetMultiStringValue(HoldFrom, HF);
	SetMultiStringValue(HoldTo, HT);
	SetMultiStringValue(HoldAt, HA);
	SetMultiStringValue(HoldBID, HB);
	SetMultiStringValue(SendWPAddrs, WPTO);

	// set up FB style fiters
	
	ptr += sprintf(ptr, 
		"<table><tr><th>Action</th><th>Type</th><th>From</th><th>To</th><th>@BBS</th><th>Bid</th><th>Max Size</th></tr>");

	while(Filter)
	{
		ptr += sprintf(ptr, "<tr>"	
		"<td><input type=text name=Action style=\"text-transform: uppercase\"maxlength=2 size=2 value=%c></td>"
		"<td><input type=text name=Type style=\"text-transform: uppercase\"maxlength=2 size=2 value=%c></td>"
		"<td><input type=text name=From style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=TO style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=AT style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=BID style=\"text-transform: uppercase\" maxlength=13 size=13 value=%s></td>"
		"<td><input type=text name=MaxLen maxlength=6 size=6 value=%d></td></tr>",
			Filter->Action, Filter->Type, Filter->From, Filter->TO, Filter->AT, Filter->BID, Filter->MaxLen);

		Filter = Filter->Next;
	}

	//	Add a few blank entries for input

	for (i = 0; i < 5; i++)
	{
		ptr += sprintf(ptr, "<tr>"
		"<td><input type=text name=Action style=\"text-transform: uppercase\"maxlength=2 size=2 value=%c></td>"
		"<td><input type=text name=Type style=\"text-transform: uppercase\"maxlength=2 size=2 value=%c></td>"
		"<td><input type=text name=From style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=TO style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=AT style=\"text-transform: uppercase\" maxlength=7 size=7 value=%s></td>"
		"<td><input type=text name=BID style=\"text-transform: uppercase\" maxlength=13 size=13 value=%s></td>"
		"<td><input type=text name=MaxLen maxlength=6 size=6 value=%d></td></tr>", ' ', ' ', "", "", "", "", 0);
	}

	ptr += sprintf(ptr, "</table>");

	Debugprintf("%d", strlen(FBBFilters));

	Len = sprintf(Reply, ConfigTemplate,
		BBSName, Key, Key, Key, Key, Key, Key, Key, Key, Key,
		BBSName, SYSOPCall, HRoute,
		(SendBBStoSYSOPCall) ? CHKD  : UNC,
		BBSApplNum, MaxStreams,
		(SendSYStoSYSOPCall) ? CHKD  : UNC,
		(RefuseBulls) ? CHKD  : UNC,
		(EnableUI) ? CHKD  : UNC,
		MailForInterval,
		(DontHoldNewUsers) ? CHKD  : UNC,
		(DefaultNoWINLINK) ? CHKD  : UNC,
		(AllowAnon) ? CHKD  : UNC, 
		(DontNeedHomeBBS) ? CHKD  : UNC, 
		(DontCheckFromCall) ? CHKD  : UNC, 
		(UserCantKillT) ? UNC : CHKD,		// Reverse logic
		(ForwardToMe) ? CHKD  : UNC,
		(OnlyKnown) ? CHKD  : UNC,
		(reportMailEvents) ? CHKD  : UNC,
		POP3InPort, SMTPInPort, NNTPInPort,
		(RemoteEmail) ? CHKD  : UNC,
		AMPRDomain,
		(SendAMPRDirect) ? CHKD  : UNC,
		(ISP_Gateway_Enabled) ? CHKD  : UNC,
		MyDomain, ISPSMTPName, ISPSMTPPort, ISPEHLOName, ISPPOP3Name, ISPPOP3Port,
		ISPAccountName, ISPAccountPass, ISPPOP3Interval,
		(SMTPAuthNeeded) ? CHKD  : UNC,
		(SendWP) ? CHKD  : UNC,
		(FilterWPBulls) ? CHKD  : UNC,
		(SendWPType == 0) ? CHKD  : UNC,
		(SendWPType == 1) ? CHKD  : UNC,
		WPTO,
		RF, RT, RA, RB, HF, HT, HA, HB, FBBFilters);

	*ReplyLen = Len;
}
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key)
{
	char FromList[1000]= "", ToList[1000]= "", AtList[1000] = "";
	char Line[80];
	struct Override ** Call;

	if (LTFROM)
	{
		Call = LTFROM;
		while(Call[0])
		{
			sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
			strcat(FromList, Line);
			Call++;
		}
	}
		if (LTTO)
		{
			Call = LTTO;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(ToList, Line);
				Call++;
			}
		}

		if (LTAT)
		{
			Call = LTAT;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(AtList, Line);
				Call++;
			}
		}

		*ReplyLen = sprintf(Reply, HousekeepingTemplate, 
			 BBSName, Key, Key, Key, Key, Key, Key, Key, Key, Key,
			MaintTime, MaintInterval, MaxMsgno, BidLifetime, LogAge, UserLifetime,
			(DeletetoRecycleBin) ? CHKD  : UNC,
			(SendNonDeliveryMsgs) ? CHKD  : UNC,
			(SuppressMaintEmail) ? CHKD  : UNC,
			(GenerateTrafficReport) ? CHKD  : UNC,
			PR, PUR, PF, PNF, BF, BNF, NTSD, NTSF, NTSU,
			FromList, ToList, AtList,
			(OverrideUnsent) ? CHKD  : UNC);

		return;

}


VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;

	Len = SendHeader(Reply, Key);
		
	Len += sprintf(&Reply[Len], Welcome, Key, WelcomeMsg, NewWelcomeMsg, ExpertWelcomeMsg,
			Prompt, NewPrompt, ExpertPrompt, SignoffMsg);
	*ReplyLen = Len;
}

VOID SendFwdMainPage(char * Reply, int * RLen, char * Key)
{
	char ALIASES[16384];

	SetMultiStringValue(AliasText, ALIASES);

	*RLen = sprintf(Reply, FwdTemplate, Key, Key, BBSName,
		Key, Key, Key, Key, Key, Key, Key, Key,
		Key, MaxTXSize, MaxRXSize, MaxAge,
		(WarnNoRoute) ? CHKD  : UNC, 
		(Localtime) ? CHKD  : UNC,
		(SendPtoMultiple) ? CHKD  : UNC,
		(FOURCHARCONT) ? CHKD  : UNC,
		ALIASES);
}


char TenSpaces[] = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

VOID SendUIPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len, i;

	Len = SendHeader(Reply, Key);
	Len += sprintf(&Reply[Len], UIHddr, Key, MailForText);

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		char PortNo[512];
		char PortDesc[31];
		int n;

		// Only allow UI on ax.25 ports

		struct _EXTPORTDATA * PORTVEC;

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntryFromSlot(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				if (PORTVEC->PORTCONTROL.UICAPABLE == 0)
					continue;


		GetPortDescription(i, PortDesc);
		n = sprintf(PortNo, "Port&nbsp;%2d %s", GetPortNumber(i), PortDesc);

		while (PortNo[--n] == ' ');

		PortNo[n + 1] = 0;
	
		while (n++ < 38)
			strcat(PortNo, "&nbsp;");

		Len += sprintf(&Reply[Len], UILine,
				(UIEnabled[i])?CHKD:UNC, i,
				 PortNo,
				 (UIDigi[i])?UIDigi[i]:"", i, 
			 	 (UIMF[i])?CHKD:UNC, i,
				 (UIHDDR[i])?CHKD:UNC, i,
				 (UINull[i])?CHKD:UNC, i);
	}

	Len += sprintf(&Reply[Len], UITail, Key);

	*ReplyLen = Len;
}

void ConvertSpaceTonbsp(char * msg)
{
	// Replace any space with &nbsp;
		
	char * ptr;

	while (ptr = strchr(msg, ' '))
	{
		memmove(ptr + 5, ptr, strlen(ptr) + 1);
		memcpy(ptr, "&nbsp;", 6);
	}
}

VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	char msg[1024];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0; 
	char Name[80];

	SMTPMsgs = 0;

	Len = sprintf(Reply, RefreshMainPage, BBSName, BBSName, Key, Key, Key, Key, Key, Key, Key, Key);

	Len += sprintf(&Reply[Len], StatusPage, Key);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\r\n");
		}
		else
		{
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in\r\n");
				else
				{
					strcpy(Name, conn->UserPointer->Name);
					Name[9] = 0;

					i=sprintf_s(msg, sizeof(msg), "%-12s  %-9s  %3d  %6d%6d%6d\r\n",
						Name,
						conn->UserPointer->Call,
						conn->BPQStream,
						conn->OutputQueueLength - conn->OutputGetPointer, conn->bytesSent, conn->bytesRxed);
				}
			}
		}

		ConvertSpaceTonbsp(msg);
		Len += sprintf(&Reply[Len], StatusLine, conn->BPQStream, msg);
	}

	n = 0;

	for (i=1; i <= NumberofMessages; i++)
	{
		if (MsgHddrPtr[i]->status == 'N')
		{
			if (_stricmp(MsgHddrPtr[i]->to, SYSOPCall) == 0  || _stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)
				SYSOPMsgs++;
			else
			if (MsgHddrPtr[i]->to[0] == 0)
				SMTPMsgs++;
		}
		else
		{
			if (MsgHddrPtr[i]->status == 'H')
				HeldMsgs++;
		}
	}

	Len += sprintf(&Reply[Len], StreamEnd, 
		NumberofMessages, SYSOPMsgs, HeldMsgs, SMTPMsgs);

	// If there are any active multicast transfers, display them.

	Len += MulticastStatusHTML(&Reply[Len]);

	Len += sprintf(&Reply[Len], StatusTail, 
		NumberofMessages, SYSOPMsgs, HeldMsgs, SMTPMsgs);

	*ReplyLen = Len;
}

VOID SendFwdSelectPage(char * Reply, int * ReplyLen, char * Key)
{
	struct UserInfo * USER;
	int i = 0;
	int Len = 0;

	for (USER = BBSChain; USER; USER = USER->BBSNext)
	{
		Len += sprintf(&Reply[Len], "%s|", USER->Call);
	}

	*ReplyLen = Len;
}

VOID SendUserSelectPage(char * Reply, int * ReplyLen, char * Key)
{
	struct UserInfo * USER;
	int i = 0, n;
	int Len = 0;
	struct UserInfo * users[10000]; 

	// Get array of addresses

	for (n = 1; n <= NumberofUsers; n++)
	{
		users[i++] = UserRecPtr[n];
		if (i > 9999) break;
	}

	qsort((void *)users, i, sizeof(void *), compare );
		
	for (n = 0; n < NumberofUsers; n++)
	{
		USER = users[n];
		Len += sprintf(&Reply[Len], "%s|", USER->Call);
	}
	*ReplyLen = Len;
}

int SendUserDetails(struct HTTPConnectionInfo * Session, char * Reply, char * Key)
{
	char SSID[16][16] = {""};
	char ASSID[16];
	int i, n, s, Len;
	struct UserInfo * User = Session->User;
	unsigned int flags = User->flags;
	int RMSSSIDBits = Session->User->RMSSSIDBits;
	char HiddenPass[20] = "";

	int	ConnectsIn;
	int ConnectsOut;
	int MsgsReceived;
	int MsgsSent;
	int MsgsRejectedIn;
	int MsgsRejectedOut;
	int BytesForwardedIn;
	int BytesForwardedOut;
//	char MsgsIn[80];
//	char MsgsOut[80];
//	char BytesIn[80];
//	char BytesOut[80];
//	char RejIn[80];
//	char RejOut[80];

	i = 0;

	ConnectsIn = User->Total.ConnectsIn - User->Last.ConnectsIn;
	ConnectsOut = User->Total.ConnectsOut - User->Last.ConnectsOut;

	MsgsReceived = MsgsSent = MsgsRejectedIn = MsgsRejectedOut = BytesForwardedIn = BytesForwardedOut = 0;

	for (n = 0; n < 4; n++)
	{
		MsgsReceived +=	User->Total.MsgsReceived[n] - User->Last.MsgsReceived[n];	
		MsgsSent += User->Total.MsgsSent[n] - User->Last.MsgsSent[n];
		BytesForwardedIn += User->Total.BytesForwardedIn[n] - User->Last.BytesForwardedIn[n];
		BytesForwardedOut += User->Total.BytesForwardedOut[n] - User->Last.BytesForwardedOut[n];
		MsgsRejectedIn += User->Total.MsgsRejectedIn[n] - User->Last.MsgsRejectedIn[n];
		MsgsRejectedOut += User->Total.MsgsRejectedOut[n] - User->Last.MsgsRejectedOut[n];
	}

	
	for (s = 0; s < 16; s++)
	{
		if (RMSSSIDBits & (1 << s))
		{
			if (s)
				sprintf(&SSID[i++][0], "%d", s);
			else
				SSID[i++][0] = 0;
		}
	}

	memset(HiddenPass, '*', strlen(User->CMSPass));

	i = (flags >> 28);
	sprintf(ASSID, "%d", i);

	if (i == 0)
		ASSID[0] = 0;

	if (!UserDetailTemplate)
		UserDetailTemplate = GetTemplateFromFile(4, "UserDetail.txt");

	Len = sprintf(Reply, UserDetailTemplate, Key, User->Call,
		(flags & F_BBS)?CHKD:UNC,
		(flags & F_EMAIL)?CHKD:UNC,
		(flags & F_PMS)?CHKD:UNC,
		(flags & F_Temp_B2_BBS)?CHKD:UNC,
		(flags & F_SYSOP)?CHKD:UNC,
		(flags & F_POLLRMS)?CHKD:UNC,
		(flags & F_Expert)?CHKD:UNC,
		SSID[0], SSID[1], SSID[2], SSID[3],
		(flags & F_Excluded)?CHKD:UNC,
		(flags & F_HOLDMAIL)?CHKD:UNC,
		(flags & F_SYSOP_IN_LM)?CHKD:UNC,
		(flags & F_NOWINLINK)?CHKD:UNC,
		(flags & F_NOBULLS)?UNC:CHKD,		// Inverted flag
		(flags & F_NTSMPS)?CHKD:UNC,
		(flags & F_RMSREDIRECT)?CHKD:UNC,
		(flags & F_APRSMFOR)?CHKD:UNC, ASSID,

		ConnectsIn, MsgsReceived, MsgsRejectedIn,
		ConnectsOut, MsgsSent, MsgsRejectedOut,
		BytesForwardedIn, FormatDateAndTime((time_t)User->TimeLastConnected, FALSE), 
		BytesForwardedOut, User->lastmsg,
		User->Name,
		User->pass,
		HiddenPass,
		User->Address,
		User->ZIP,
		User->HomeBBS);

	return Len;
}

#ifdef WIN32

int ProcessWebmailWebSock(char * MsgPtr, char * OutBuffer);

static char PipeFileName[] = "\\\\.\\pipe\\BPQMailWebPipe";

// Constants

static DWORD WINAPI InstanceThread(LPVOID lpvParam)

// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
	BOOL fSuccess = FALSE;
	HANDLE hPipe  = NULL;
	char Buffer[250000];
	char OutBuffer[250000];
	char * MsgPtr;
	int InputLen = 0;
	int OutputLen = 0;
	struct HTTPConnectionInfo Session;
	char URL[100001];
	char * Context, * Method;
	int n;
	char token[16]= "";

	char * ptr;

	// The thread's parameter is a handle to a pipe object instance. 

	hPipe = (HANDLE) lpvParam; 

	// First block is the HTTPConnectionInfo record, rest is request

	n = ReadFile(hPipe, &Session, sizeof (struct HTTPConnectionInfo), &n, NULL);

	// Get the data

	fSuccess = ReadFile(hPipe, Buffer, 250000, &InputLen, NULL);

	if (!fSuccess || InputLen == 0)
	{   
		if (GetLastError() == ERROR_BROKEN_PIPE)
			Debugprintf("InstanceThread: client disconnected.", GetLastError()); 
		else
			Debugprintf("InstanceThread ReadFile failed, GLE=%d.", GetLastError()); 

		return 1;
	}

	Buffer[InputLen] = 0;

	MsgPtr = &Buffer[0];

	if (memcmp(MsgPtr,  "WMRefresh", 9) == 0)
	{
		OutputLen = ProcessWebmailWebSock(MsgPtr, OutBuffer);
	}
	else
	{
		// look for auth header	
		
		const char * auth_header = "Authorization: Bearer ";
		char * token_begin = strstr(MsgPtr, auth_header);
		int Flags = 0;

		// Node Flags isn't currently used

		if (token_begin)
		{
			// Using Auth Header

			// Extract the token from the request (assuming it's present in the request headers)

			token_begin += strlen(auth_header); // Move to the beginning of the token
			strncpy(token, token_begin, 13);
			token[13] = '\0'; // Null-terminate the token
		}
	}

	strcpy(URL, MsgPtr);



	ptr = strstr(URL, " HTTP");

	if (ptr)
		*ptr = 0;

	Method = strtok_s(URL, " ", &Context);

	ProcessMailHTTPMessage(&Session, Method, Context, MsgPtr, OutBuffer, &OutputLen, InputLen, token);


	WriteFile(hPipe, &Session, sizeof (struct HTTPConnectionInfo), &n, NULL);
	WriteFile(hPipe, OutBuffer, OutputLen, &cbWritten, NULL); 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe);

	return 1;
}

static DWORD WINAPI PipeThreadProc(LPVOID lpvParam)
{
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and this loop is free to wait for the
// next client connect request. It is an infinite loop.
 
	for (;;) 
	{ 
      hPipe = CreateNamedPipe( 
          PipeFileName,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_BYTE |       // message type pipe 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          4096,                  // output buffer size 
          4096,                  // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          Debugprintf("CreateNamedPipe failed, GLE=%d.\n", GetLastError()); 
          return -1;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
	  {
         // Create a thread for this client. 
   
		 hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID 

         if (hThread == NULL) 
         {
            Debugprintf("CreateThread failed, GLE=%d.\n", GetLastError()); 
            return -1;
         }
         else CloseHandle(hThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 
   } 

   return 0; 
} 

BOOL CreatePipeThread()
{
	DWORD ThreadId;
	CreateThread(NULL, 0, PipeThreadProc, 0, 0, &ThreadId);
	return TRUE;
}

#endif

char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

VOID FormatTime(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);
}







