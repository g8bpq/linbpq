// basic JASON API to BPQ Node

// Authentication is via Telnet USER records or bbs records


#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include "cheaders.h"
#include <stdlib.h>
#include "bpqmail.h"
#include "httpconnectioninfo.h"

struct MsgInfo * GetMsgFromNumber(int msgno);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP);
char * doXMLTransparency(char * string);


// Constants
#define TOKEN_SIZE 32 // Length of the authentication token
#define TOKEN_EXPIRATION 7200 // Token expiration time in seconds (2 hours)

// Token data structure
typedef struct MailToken {
	char token[TOKEN_SIZE + 1];
	time_t expiration_time;
	struct UserInfo * User;
	char Call[10];
	int Auth;			// Security level of user

	struct MailToken* next;
} MailToken;

static MailToken * token_list = NULL;

typedef struct MailAPI
{
	char *URL;
	int URLLen;
	int (* APIRoutine)(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth);
	int Auth;
} MailAPI;

// Auth defines

#define AuthNone 0
#define AuthUser 1
#define AuthBBSUser 2
#define AuthSysop 4


static int verify_token(const char* token);
static void remove_expired_tokens();
static int  request_token(char * response);
static void add_token_to_list(MailToken* token);
static MailToken * find_token(const char* token); 

int sendMsgList(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth);
int sendFwdQueueLen(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth);
int sendFwdConfig(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth);

static struct MailAPI APIList[] =
{
	"/mail/api/v1/msgs", 17, sendMsgList, 0,
	"/mail/api/v1/FwdQLen", 20, sendFwdQueueLen, AuthSysop,
	"/mail/api/v1/FwdConfig", 22, sendFwdConfig, AuthSysop,
};

static int APICount = sizeof(APIList) / sizeof(struct MailAPI);

#ifndef WIN32
iconv_t * icu = NULL;
#endif

void APIConvertTitletoUTF8(char * Title, char * UTF8Title, int Len)
{
	if (WebIsUTF8(Title, (int)strlen(Title)) == FALSE)
	{
		// With Windows it is simple - convert using current codepage
		// I think the only reliable way is to convert to unicode and back

		int origlen = (int)strlen(Title) + 1;
#ifdef WIN32
		WCHAR BufferW[128];
		int wlen;
		int len = origlen;

		wlen = MultiByteToWideChar(CP_ACP, 0, Title, len, BufferW, origlen * 2); 
		len = WideCharToMultiByte(CP_UTF8, 0, BufferW, wlen, UTF8Title, origlen * 2, NULL, NULL); 
#else
		size_t left = Len - 1;
		size_t len = origlen;

		if (icu == NULL)
			icu = iconv_open("UTF-8//IGNORE", "CP1252");

		if (icu == (iconv_t)-1)
		{
			strcpy(UTF8Title, Title);
			icu = NULL;
			return;
		}

		char * orig = UTF8Title;

		iconv(icu, NULL, NULL, NULL, NULL);		// Reset State Machine
		iconv(icu, &Title, &len, (char ** __restrict__)&UTF8Title, &left);

#endif
	}
	else
		strcpy(UTF8Title, Title);
}

static MailToken * generate_token() 
{
	// Generate a random authentication token
	int i;

	MailToken * token = malloc(sizeof(MailToken));

	srand(time(NULL));

	for (i = 0; i < TOKEN_SIZE; i++)
	{
		token->token[i] = 'A' + rand() % 26; // Random uppercase alphabet character
	}
	token->token[TOKEN_SIZE] = '\0'; // Null-terminate the token
	token->expiration_time = time(NULL) + TOKEN_EXPIRATION; // Set token expiration time
	add_token_to_list(token);
	return token;
}

// Function to add the token to the token_list
static void add_token_to_list(MailToken * token)
{
	if (token_list == NULL)
	{
		token_list = token;
		token->next = NULL;
	}
	else
	{
		MailToken * current = token_list;
		
		while (current->next != NULL) 
			current = current->next;

		current->next = token;
		token->next = NULL;
	}
}

static void remove_expired_tokens()
{
	time_t current_time = time(NULL);
	MailToken* current_token = token_list;
	MailToken* prev_token = NULL;
	MailToken* next_token;

	while (current_token != NULL)
	{
		if (current_time > current_token->expiration_time) 
		{
			// Token has expired, remove it from the token list
			if (prev_token == NULL)
			{
				token_list = current_token->next;
			} else {
				prev_token->next = current_token->next;
			}
			next_token = current_token->next;
			free(current_token);
			current_token = next_token;
		} else {
			prev_token = current_token;
			current_token = current_token->next;
		}
	}
}

static MailToken * find_token(const char* token) 
{
	MailToken * current_token = token_list;

	while (current_token != NULL) 
	{
		if (strcmp(current_token->token, token) == 0) 
		{
			return current_token;
		}
		current_token = current_token->next;
	}
	return NULL;
}

static int send_http_response(char * response, const char* msg)
{
	return sprintf(response, "HTTP/1.1 %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", msg);
}


int MailAPIProcessHTTPMessage(struct HTTPConnectionInfo * Session, char * response, char * Method, char * URL, char * request, BOOL LOCAL, char  *Params, char * TokenString)
{
	char * pass = strlop(Params, '&');
	int Flags = 0, n;
	MailToken * Token;
	char Msg[64];
	struct UserInfo * User;
	int Auth = 0;

	if (LOCAL)
		Auth = AuthSysop;

	// Check if the request is for token generation

	if (strcmp(Method, "GET") != 0)
		return send_http_response(response, "403 (Bad Method)");

	if (_stricmp(URL, "/mail/api/v1/login") == 0)
	{
		// Key is in Session->Key
		
		// Signon may have been validated in Node. If Session->Callsign is set
		
		if (Session->Callsign[0] == 0)
		{
			// Try BBS logon
			
			User = LookupCall(Params);
			
			if (User)
			{
				// Check Password

				if (pass[0] == 0 || strcmp(User->pass, pass) != 0 || User->flags & F_Excluded)
					return send_http_response(response, "403 (Login Failed)");		
			
				strcpy(Session->Callsign, User->Call);
				Auth = AuthBBSUser;
				if (User->flags & F_SYSOP)
					Auth |= AuthSysop;



			}
		}
		else
		{
			User = LookupCall(Session->Callsign);

			if (User)
			{
				Auth = AuthUser;
				if (User->flags & F_SYSOP)
					Auth |= AuthSysop;
			}
		}

		n = sprintf_s(Msg, sizeof(Msg), "API Connect from %s", _strupr(Params));
		WriteLogLine(NULL, '|',Msg, n, LOG_BBS);

		Token = zalloc(sizeof(MailToken));

		strcpy(Token->token, Session->Key);
		strcpy(Token->Call, Session->Callsign);
		Token->Auth = Auth;

		Token->expiration_time = time(NULL) + TOKEN_EXPIRATION; // Set token expiration time
		add_token_to_list(Token);

		// Return Token

		sprintf(response, "{\"access_token\":\"%s\", \"expires_at\":%d, \"scope\":\"create\"}\r\n",
			Token->token, Token->expiration_time);

		return strlen(response);
	}

	// Find Token

	if (TokenString[0])					// Token form Auth Header
		Token = find_token(TokenString);
	else
		Token = find_token(Params);		// Token form URL

	if (Token != NULL)
	{
		// Check if the token has expired
	
		time_t current_time = time(NULL);
		if (current_time > Token->expiration_time)
		{
			// Token has expired, remove it from the token list
			remove_expired_tokens();
			Token = NULL;
		}
	}

	if (Token)
		Auth |= Token->Auth;

	// Determine the requested API endpoint

	for (n = 0; n < APICount; n++)
	{
		struct MailAPI * APIEntry;
		char * rest;
		
		APIEntry = &APIList[n];

		if (_memicmp(URL, APIEntry->URL, APIEntry->URLLen) == 0)
		{
			rest = &request[4 + APIEntry->URLLen];	// Anything following?

			if (rest[0] =='?')
			{
				//Key

				strlop(rest, ' ');
				strlop(rest, '&');

				Token = find_token(&rest[1]);

				if (Token)
				{
					strcpy(Session->Callsign, Token->Call);
					strcpy(Session->Key, Token->token);
				}
				else
					return send_http_response(response, "403 (Invalid Security Token)");
			}
			
			if (APIEntry->Auth)
			{
				// Check Level 

				if ((Auth & APIEntry->Auth) == 0)
					return send_http_response(response, "403 (Not Authorized)");
			}

			if (rest[0] == ' ' || rest[0] == '/' || rest[0] == '?')
			{
				return APIEntry->APIRoutine(Session, response, rest, Auth);
			}
		}

	}

	return send_http_response(response, "401 Invalid API Call");


	return 0;
}

int WebMailAPIProcessHTTPMessage(char * response, char * Method, char * URL, char * request, BOOL LOCAL, char  *Params)
{
	char * pass = strlop(Params, '&');
	int Flags = 0;
	MailToken * Token;


	// Check if the request is for token generation

	if (strcmp(Method, "GET") != 0)
		return send_http_response(response, "403 (Bad Method)");

	if (_stricmp(URL, "/mail/api/login") == 0)
	{
		// user is in Params and Password in pass

		struct UserInfo * User;
		char Msg[256];
		int n;

		User = LookupCall(Params);

		if (User)
		{
			// Check Password

			if (pass[0] == 0 || strcmp(User->pass, pass) != 0 || User->flags & F_Excluded)
				return send_http_response(response, "403 (Login Failed)");		
			
			n=sprintf_s(Msg, sizeof(Msg), "API Connect from %s", _strupr(Params));
			WriteLogLine(NULL, '|',Msg, n, LOG_BBS);

			Token = generate_token();
			add_token_to_list(Token);

			Token->User = User;

			strcpy(Token->Call, Params);

			// Return Token

			sprintf(response, "{\"access_token\":\"%s\", \"expires_in\":%d, \"scope\":\"create\"}\r\n",
					Token->token, Token->expiration_time);

			return strlen(response);

		}
	}

	return 0;
}

//	Unauthenticated users can only get bulls.
//	Authenticated users may read only that users messages or all messages depending on sysop status

int sendMsgList(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth)
{
 	struct UserInfo * User = LookupCall(Session->Callsign);
	int m;
	struct MsgInfo * Msg;
	char * ptr = response;
	int n = NumberofMessages; //LineCount;
	char Via[64];
	int Count = 0;
	struct UserInfo DummyUser = {""};
	ptr[0] = 0;

	if (User == 0)
		User=&DummyUser;

	n = sprintf(ptr,"{\"msgs\":[\r\n");
	ptr += n;

	for (m = LatestMsg; m >= 1; m--)
	{
		if (ptr > &response[244000])
			break;						// protect buffer

		Msg = GetMsgFromNumber(m);

		if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
			continue;					// Protect against corrupt messages
		
		if (Msg && CheckUserMsg(Msg, User->Call, Auth & AuthSysop))
		{
			char UTF8Title[4096];
			char  * EncodedTitle;
			
			// List if it is the right type

			ptr += sprintf(ptr, "{\r\n");

			strcpy(Via, Msg->via);
			strlop(Via, '.');

			// make sure title is HTML safe (no < > etc) and UTF 8 encoded

			EncodedTitle = doXMLTransparency(Msg->title);

			memset(UTF8Title, 0, 4096);		// In case convert fails part way through
			APIConvertTitletoUTF8(EncodedTitle, UTF8Title, 4095);

			ptr += sprintf(ptr, "\"id\": \"%d\",\r\n", Msg->number);
			ptr += sprintf(ptr, "\"mid\": \"%s\",\r\n", Msg->bid);
			ptr += sprintf(ptr, "\"rcvd\": \"%d\",\r\n", Msg->datecreated);
			ptr += sprintf(ptr, "\"type\": \"%c\",\r\n", Msg->type);
			ptr += sprintf(ptr, "\"status\": \"%c\",\r\n", Msg->status);
			ptr += sprintf(ptr, "\"to\": \"%s\",\r\n", Msg->to);
			ptr += sprintf(ptr, "\"from\": \"%s\",\r\n", Msg->from);
			ptr += sprintf(ptr, "\"size\": \"%d\",\r\n", Msg->length);
			ptr += sprintf(ptr, "\"subject\": \"%s\"\r\n", UTF8Title);

			free(EncodedTitle);
			
			ptr += sprintf(ptr, "},\r\n"); 

		}
	}

	if (response[n] == 0)		// No entries
	{
		response[strlen(response) - 2] = '\0';          // remove \r\n
		strcat(response, "]}\r\n");
	}
	else
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n]}\r\n");
	}
	return strlen(response);
}

int sendFwdQueueLen(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth)
{
	struct UserInfo * USER;
	char * ptr = response;
	int n;
	int i = 0;
	int Len = 0;

	n = sprintf(ptr,"{\"forwardqueuelength\":[\r\n");
	ptr += n;
	
	for (USER = BBSChain; USER; USER = USER->BBSNext)
	{
		int Count = CountMessagestoForward (USER);

		ptr += sprintf(ptr, "{");
		ptr += sprintf(ptr, "\"call\": \"%s\",", USER->Call);
		ptr += sprintf(ptr, "\"queueLength\": \"%d\"", Count);
		ptr += sprintf(ptr, "},\r\n");
	}

	if (response[n] == 0)		// No entries
	{
		response[strlen(response) - 2] = '\0';          // remove \r\n
		strcat(response, "]}\r\n");
	}
	else
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n]}\r\n");
	}
	return strlen(response);
}

VOID APIMultiStringValue(char ** values, char * Multi)
{
	char ** Calls;
	char * ptr = &Multi[0];

	*ptr = 0;

	if (values)
	{
		Calls = values;

		while(Calls[0])
		{
			ptr += sprintf(ptr, "\"%s\",", Calls[0]);
			Calls++;
		}
		if (ptr != &Multi[0])
			*(--ptr) = 0;
	}
}

char * APIConvTime(int ss)
{
	int hh, mm;
	static char timebuf[64];

	hh = ss / 3600;
	mm = (ss - (hh * 3600)) / 60;
	ss = ss % 60;

	sprintf(timebuf, "\"%02d:%02d:%02d\"", hh, mm, ss);

	return timebuf;
}


int sendFwdConfig(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth)
{
	struct UserInfo * USER;
	char * ptr = response;
	int n = 0;
	int i = 0;
	int Len = 0;

	response[n] = 0;

	n = sprintf(ptr, "{\r\n");
	ptr += n;

	for (USER = BBSChain; USER; USER = USER->BBSNext)
	{
		struct BBSForwardingInfo * FWDInfo = USER->ForwardingInfo;

		int Count = CountMessagestoForward (USER);

		char TO[2048] = "";
		char AT[2048] = "";
		char TIMES[2048] = "";
		char FWD[100000] = "";
		char HRB[2048] = "";
		char HRP[2048] = "";

		APIMultiStringValue(FWDInfo->TOCalls, TO);
		APIMultiStringValue(FWDInfo->ATCalls, AT);
		APIMultiStringValue(FWDInfo->FWDTimes, TIMES);
		APIMultiStringValue(FWDInfo->ConnectScript, FWD);
		APIMultiStringValue(FWDInfo->Haddresses, HRB);
		APIMultiStringValue(FWDInfo->HaddressesP, HRP);



		ptr += sprintf(ptr, " \"%s\": {\r\n", USER->Call);
		ptr += sprintf(ptr, "  \"queueLength\": %d,\r\n", Count);
		ptr += sprintf(ptr, "  \"to\": [%s],\r\n", TO);
		ptr += sprintf(ptr, "  \"at\": [%s],\r\n", AT);
		ptr += sprintf(ptr, "  \"hrp\": [%s],\r\n",HRP);
		ptr += sprintf(ptr, "  \"hrb\": [%s],\r\n",HRB);
		ptr += sprintf(ptr, "  \"times\": [%s],\r\n",TIMES);
		ptr += sprintf(ptr, "  \"connectScript\": [%s],\r\n",FWD);
		ptr += sprintf(ptr, "  \"bbsHa\": \"%s\",\r\n", (FWDInfo->BBSHA)?FWDInfo->BBSHA:"");
		ptr += sprintf(ptr, "  \"enableForwarding\": %s,\r\n", (FWDInfo->Enabled)?"true":"false");
		ptr += sprintf(ptr, "  \"forwardingInterval\": %s,\r\n", APIConvTime(FWDInfo->FwdInterval));
		ptr += sprintf(ptr, "  \"requestReverse\": %s,\r\n", (FWDInfo->ReverseFlag)?"true":"false");
		ptr += sprintf(ptr, "  \"reverseInterval\": %s,\r\n", APIConvTime(FWDInfo->RevFwdInterval));
		ptr += sprintf(ptr, "  \"sendNewMessagesWithoutWaiting\": %s,\r\n", (FWDInfo->SendNew)?"true":"false");
		ptr += sprintf(ptr, "  \"fbbBlocked\": %s,\r\n", (FWDInfo->AllowBlocked)?"true":"false");
		ptr += sprintf(ptr, "  \"maxBlock\": %d,\r\n", FWDInfo->MaxFBBBlockSize);
		ptr += sprintf(ptr, "  \"sendPersonalMailOnly\": %s,\r\n", (FWDInfo->PersonalOnly)?"true":"false");
		ptr += sprintf(ptr, "  \"allowBinary\": %s,\r\n", (FWDInfo->AllowCompressed)?"true":"false");
		ptr += sprintf(ptr, "  \"useB1Protocol\": %s,\r\n", (FWDInfo->AllowB1)?"true":"false");
		ptr += sprintf(ptr, "  \"useB2Protocol\": %s,\r\n", (FWDInfo->AllowB2)?"true":"false");
		ptr += sprintf(ptr, "  \"incomingConnectTimeout\": %s\r\n", APIConvTime(FWDInfo->ConTimeout));
		ptr += sprintf(ptr, " },\r\n");
	}

	if (response[n] == 0)		// No entries
	{
		strcpy(response, "{}\r\n");
	}
	else
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n}\r\n");
	}

	return strlen(response);
}



/*
{
  "GB7BEX": {
    "queueLength": 0,
    "forwardingConfig": {
      "to": [],
      "at": [
        "OARC",
        "GBR",
        "WW"
      ],
      "times": [],
      "connectScript": [
        "PAUSE 3",
        "INTERLOCK 3",
        "NC 3 !GB7BEX"
      ],
      "hierarchicalRoutes": [],
      "hr": [
        "#38.GBR.EURO"
      ],
      "bbsHa": "GB7BEX.#38.GBR.EURO",
      "enableForwarding": true,
      "forwardingInterval": "00:56:40",
      "requestReverse": false,
      "reverseInterval": "00:56:40",
      "sendNewMessagesWithoutWaiting": true,
      "fbbBlocked": true,
      "maxBlock": 1000,
      "sendPersonalMailOnly": false,
      "allowBinary": true,
      "useB1Protocol": false,
      "useB2Protocol": true,
      "sendCtrlZInsteadOfEx": false,
      "incomingConnectTimeout": "00:02:00"
    }
  },
  "GB7RDG": {
    "queueLength": 0,
    "forwardingConfig": {
      "to": [],
...
      "incomingConnectTimeout": "00:02:00"
    }
  }
}



# HELP packetmail_queue_length The number of messages in the packetmail queue
# TYPE packetmail_queue_length gauge
packetmail_queue_length{partner="DM4RW"} 0 1729090716916
packetmail_queue_length{partner="G8BPQ"} 3 1729090716916
packetmail_queue_length{partner="GB7BEX"} 0 1729090716916
packetmail_queue_length{partner="GB7BPQ"} 1 1729090716916
packetmail_queue_length{partner="GB7MNS"} 0 1729090716916
packetmail_queue_length{partner="GB7NOT"} 0 1729090716916
packetmail_queue_length{partner="GB7NWL"} 0 1729090716916
packetmail_queue_length{partner="GM8BPQ"} 0 1729090716916

*/


// Stuff send to  packetnodes.spots.radio/api/bbsdata/{bbsCall}
//https://nodes.ukpacketradio.network/swagger/index.html


/*
BbsData{
callsign*	[...]
time*	[...]
hroute*	[...]
peers	[...]
software*	[...]
version*	[...]
mailQueues	[...]
messages	[...]
latitude	[...]
longitude	[...]
locator	[...]
location	[...]
unroutable	[...]
}

[

{
    "callsign": "GE8PZT",
    "time": "2024-11-25T10:07:41+00:00",
    "hroute": ".#24.GBR.EU",
    "peers": [
      "GB7BBS",
      "VE2PKT",
      "GB7NXT",
      "VA2OM"
    ],
    "software": "XrLin",
    "version": "504a",
    "mailQueues": [],
    "messages": [
      {
        "to": "TECH@WW",
        "mid": "20539_GB7CIP",
        "rcvd": "2024-11-24T09:27:59+00:00",
        "routing": [
          "R:241124/0927Z @:GE8PZT.#24.GBR.EU [Lamanva] #:2315 XrLin504a",
 

      {
        "to": "TNC@WW",
        "mid": "37_PA2SNK",
        "rcvd": "2024-11-18T21:56:55+00:00",
        "routing": [
          "R:241118/2156Z @:GE8PZT.#24.GBR.EU [] #:2215 XrLin504a",
          "R:241118/2156Z 12456@VE2PKT.#TRV.QC.CAN.NOAM BPQ6.0.24",
          "R:241118/2130Z 51539@VE3KPG.#ECON.ON.CAN.NOAM BPQK6.0.23",
          "R:241118/2130Z 26087@VE3CGR.#SCON.ON.CAN.NOAM LinBPQ6.0.24",
          "R:241118/2130Z 37521@PA8F.#ZH1.NLD.EURO LinBPQ6.0.24",
          "R:241118/2129Z 48377@PI8LAP.#ZLD.NLD.EURO LinBPQ6.0.24",
          "R:241118/2129Z @:PD0LPM.FRL.EURO.NLD #:33044 [Joure] $:37_PA2SNK"
        ]
      }
    ],
    "latitude": 50.145832,
    "longitude": -5.125,
    "locator": "IO70KD",
    "location": "Lamanva",
    "unroutable": [
      {
        "type": "P",
        "at": "WW"
      },
      {
        "type": "P",
        "at": "G8PZT-2"
      },
      {
        "type": "P",
        "at": "g8pzt._24.gbr.eu"
      },
      {
        "type": "P",
        "at": "G8PZT.#24.GBR.EU"
      },
      {
        "type": "P",
        "at": "GE8PZT.#24.GBR.EU"
      },
      {
        "type": "P",
        "at": "G8PZT.#24.GBR.EURO"
      }
    ]
  },

*/


//	https://packetnodes.spots.radio/swagger/index.html

// 	   "unroutable": [{"type": "P","at": "WW"}, {"type": "P", "at": "G8PZT.#24.GBR.EURO"}]

char * ViaList[100000];			// Pointers to the Message Header field
char TypeList[100000];

int unroutableCount = 0;


void CheckifRoutable(struct MsgInfo * Msg)
{
	char NextBBS[64];
	int n;

	if (Msg->status == 'K')
		return;

	if (Msg->via[0] == 0)		// No routing
		return;

	strcpy(NextBBS, Msg->via);
	strlop(NextBBS, '.');

	if (strcmp(NextBBS, BBSName) == 0)	// via this BBS
		return;

	if ((memcmp(Msg->fbbs, zeros, NBMASK) != 0) || (memcmp(Msg->forw, zeros, NBMASK) != 0))	// Has Forwarding Info
		return;

	// See if we already have it

	for (n = 0; n < unroutableCount; n++)
	{
		if ((TypeList[n] == Msg->type) && strcmp(ViaList[n], Msg->via) == 0)
			return;

	}

	// Add to list

	TypeList[unroutableCount] = Msg->type;
	ViaList[unroutableCount] = Msg->via;

	unroutableCount++;
}


extern char LOC[7];


DllExport VOID WINAPI SendWebRequest(char * Host, char * Request, char * Params, char * Return);

#ifdef LINBPQ
extern double LatFromLOC;
extern double LonFromLOC;
#else
typedef int (WINAPI FAR *FARPROCX)();
extern FARPROCX pSendWebRequest;
extern FARPROCX pGetLatLon;
double LatFromLOC = 0;
double LonFromLOC = 0;
#endif

void SendBBSDataToPktMapThread(void * Param);

void SendBBSDataToPktMap()
{
	_beginthread(SendBBSDataToPktMapThread, 0, 0);
}

void SendBBSDataToPktMapThread(void * Param)
{
	char Request[64];
	char * Params;
	char * ptr;
	int paramLen;
	struct MsgInfo * Msg;

	struct UserInfo * ourBBSRec = LookupCall(BBSName);
	struct UserInfo * USER;
	char Time[64];
	struct tm * tm;
	time_t Date = time(NULL);
	char Peers[2048] = "[]";
	char MsgQueues[16000] = "[]";
	char * Messages = malloc(1000000);	
	char * Unroutables;
	int m;
	char * MsgBytes;
	char * Rlineptr;
	char * Rlineend;
	char * RLines;
	char * ptr1, * ptr2;
	int n;

#ifndef LINBPQ
	if (pSendWebRequest == 0)
		return;						// Old Version of bpq32.dll

	pGetLatLon(&LatFromLOC, &LonFromLOC);

#endif
	if (ourBBSRec == 0)
		return;		// Wot!!

	// Get peers and Mail Queues

	ptr = &Peers[1];
	ptr1 = &MsgQueues[1];

	for (USER = BBSChain; USER; USER = USER->BBSNext)
	{
		if (strcmp(USER->Call, BBSName) != 0)
		{
			int Bytes;
			
			int Count = CountMessagestoForward(USER);	
			
			ptr += sprintf(ptr, "\"%s\",", USER->Call);

			if (Count)
			{
				Bytes = CountBytestoForward(USER);

				ptr1 += sprintf(ptr1, "{\"peerCall\": \"%s\", \"numQueued\": %d, \"bytesQueued\": %d},",
					USER->Call, Count, Bytes);
			}
      }
	}

	if ((*ptr) != ']')		// Have some entries
	{
		ptr--;				// over trailing comms
		*(ptr++) = ']';
		*(ptr) = 0;
	}

	if ((*ptr1) != ']')		// Have some entries
	{
		ptr1--;				// over trailing comms
		*(ptr1++) = ']';
		*(ptr1) = 0;
	}

	// Get Messages

	strcpy(Messages, "[]");
	ptr = &Messages[1];

	for (m = LatestMsg; m >= 1; m--)
	{
		if (ptr > &Messages[999000])
			break;						// protect buffer

		Msg = GetMsgFromNumber(m);

		if (Msg == 0 || Msg->type == 0 || Msg->status == 0)
			continue;					// Protect against corrupt messages

		// Paula suggests including H and K but limit it to the last 30 days or the last 100 messages, whichever is the smaller.

//		if (Msg->status == 'K' || Msg->status == 'H')
//			continue;

		if ((Date - Msg->datereceived) > 30 * 86400)		// Too old
			continue;

		CheckifRoutable(Msg);

		tm = gmtime((time_t *)&Msg->datereceived);

		sprintf(Time, "%04d-%02d-%02dT%02d:%02d:%02d+00:00", 
			tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

		// Get Routing

		MsgBytes = ReadMessageFile(Msg->number);
		RLines = malloc(Msg->length * 2);				// Very unlikely to need so much but better safe..

		strcpy(RLines, "[]");

		ptr2 = &RLines[1];
		
		// Need to skip B2 header if B2 Message

		Rlineptr = MsgBytes;

		// If it is a B2 Message, Must Skip B2 Header

		if (Msg->B2Flags & B2Msg)
		{
			Rlineptr = strstr(Rlineptr, "\r\n\r\n");
			if (Rlineptr)
				Rlineptr += 4;
			else
				Rlineptr = MsgBytes;
		}

		// We have to process R: lines one at a time as we need to send each one as a separate string

		while (memcmp(Rlineptr, "R:", 2) == 0)
		{
			// Have R Lines

			Rlineend = strstr(Rlineptr, "\r\n");
			Rlineend[0] = 0;
			ptr2 += sprintf(ptr2, "\"%s\",", Rlineptr);

			Rlineptr = Rlineend + 2;		// over crlf
		}

		if ((*ptr2) == ']')		// no entries
			continue;

		ptr2--;				// over trailing comms
		*(ptr2++) = ']';
		*(ptr2) = 0;
	
		ptr += sprintf(ptr, "{\"to\": \"%s\", \"mid\": \"%s\", \"rcvd\": \"%s\", \"routing\": %s},",
			Msg->to, Msg->bid, Time, RLines);

		free(MsgBytes);
		free(RLines);

	}

	if ((*ptr) != ']')	// Have some entries?
	{
		ptr--;				// over trailing comms
		*(ptr++) = ']';
		*(ptr) = 0;
	}

	// Get unroutables

	Unroutables = malloc((unroutableCount + 1) * 100);

	strcpy(Unroutables, "[]");
	ptr = &Unroutables[1];


	for (n = 0; n < unroutableCount; n++)
	{
		ptr += sprintf(ptr, "{\"type\": \"%c\",\"at\": \"%s\"},", TypeList[n], ViaList[n]);
	}

	if ((*ptr) != ']')	// Have some entries?
	{
		ptr--;				// over trailing comms
		*(ptr++) = ']';
		*(ptr) = 0;
	}



	/*
char * ViaList[100000];			// Pointers to the Message Header field
char TypeList[100000];

int unroutableCount = 0;
	   "unroutable": [{"type": "P","at": "WW"}, {"type": "P", "at": "G8PZT.#24.GBR.EURO"}]
	*/


	tm = gmtime(&Date);

	sprintf(Time, "%04d-%02d-%02dT%02d:%02d:%02d+00:00", 
		tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);


	paramLen = strlen(Peers) + strlen(MsgQueues) + strlen(Messages) + strlen(Unroutables);

	Params = malloc(paramLen + 1000);

	if (Params == 0)
	{
		free(Messages);
		free(Unroutables);
		return;
	}

	ptr = Params;

	sprintf(Request, "/api/bbsdata/%s", BBSName);

	ptr += sprintf(ptr, "{\"callsign\": \"%s\",\r\n", BBSName);
	ptr += sprintf(ptr, "\"time\": \"%s\",\r\n", Time);
	ptr += sprintf(ptr, "\"hroute\": \"%s\",\r\n", HRoute);
	ptr += sprintf(ptr, "\"peers\": %s,\r\n", Peers);
#ifdef LINBPQ
	ptr += sprintf(ptr, "\"software\": \"%s\",\r\n", "linbpq");
#else
	ptr += sprintf(ptr, "\"software\": \"%s\",\r\n", "BPQMail");
#endif
	ptr += sprintf(ptr, "\"version\": \"%s\",\r\n", VersionString);
	ptr += sprintf(ptr, "\"mailQueues\": %s,\r\n", MsgQueues);
	ptr += sprintf(ptr, "\"messages\": %s,\r\n", Messages);
	ptr += sprintf(ptr, "\"latitude\": %1.6f,\r\n", LatFromLOC);
	ptr += sprintf(ptr, "\"longitude\": %.6f,\r\n", LonFromLOC);
	ptr += sprintf(ptr, "\"locator\": \"%s\",\r\n", LOC);
	ptr += sprintf(ptr, "\"location\": \"%s\",\r\n", ourBBSRec->Address);
	ptr += sprintf(ptr, "\"unroutable\": %s\r\n}\r\n", Unroutables);

#ifdef LINBPQ
	SendWebRequest("packetnodes.spots.radio", Request, Params, 0);
#else
	pSendWebRequest("packetnodes.spots.radio", Request, Params, 0);
#endif
	free(Messages);
	free(Unroutables);
	free(Params);
}

