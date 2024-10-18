// basic JASON API to BPQ Node

// Authentication is via Telnet USER records.


#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include "CHeaders.h"
#include <stdlib.h>
#include "bpqmail.h"
#include "httpconnectioninfo.h"

struct MsgInfo * GetMsgFromNumber(int msgno);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP);
char * doXMLTransparency(char * string);
void ConvertTitletoUTF8(WebMailInfo * WebMail, char * Title, char * UTF8Title, int Len);


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
	int (* APIRoutine)();
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
	"/mail/api/v1/msgs", 17, sendMsgList, AuthBBSUser | AuthSysop,
	"/mail/api/v1/FwdQLen", 20, sendFwdQueueLen, AuthBBSUser | AuthSysop,
	"/mail/api/v1/FwdConfig", 22, sendFwdConfig, AuthBBSUser | AuthSysop,
};

static int APICount = sizeof(APIList) / sizeof(struct MailAPI);


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

static int verify_token(const char* token)
{
	// Find the token in the token list
	MailToken * existing_token = find_token(token);

	if (existing_token != NULL)
	{
		// Check if the token has expired
		time_t current_time = time(NULL);
		if (current_time > existing_token->expiration_time)
		{
			// Token has expired, remove it from the token list
			remove_expired_tokens();
			return 0;
		}
		// Token is valid
		return 1;
	}

	// Token doesn't exist in the token list
	return 0;
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


int MailAPIProcessHTTPMessage(struct HTTPConnectionInfo * Session, char * response, char * Method, char * URL, char * request, BOOL LOCAL, char  *Params)
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

		sprintf(response, "{\"access_token\":\"%s\", \"expires_in\":%d, \"scope\":\"create\"}\r\n",
			Token->token, Token->expiration_time);

		return strlen(response);
	}

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

//	Unauthorised users can only get bulls.
//	Autothorised may read only users message or all messages depending on sysop status

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
			
			// List if it is the right type and in the page range we want

	
			if (Count++ < Session->WebMailSkip)
				continue;

			ptr += sprintf(ptr, "{\r\n");


			strcpy(Via, Msg->via);
			strlop(Via, '.');

			// make sure title is HTML safe (no < > etc) and UTF 8 encoded

			EncodedTitle = doXMLTransparency(Msg->title);

			memset(UTF8Title, 0, 4096);		// In case convert fails part way through
			ConvertTitletoUTF8(Session->WebMail, EncodedTitle, UTF8Title, 4095);

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
			
	//		ptr += sprintf(ptr, "<a href=/WebMail/WM?%s&%d>%6d</a> %s %c%c %5d %-8s%-8s%-8s%s\r\n",
	//			Session->Key, Msg->number, Msg->number,
	//			FormatDateAndTime((time_t)Msg->datecreated, TRUE), Msg->type,
	//			Msg->status, Msg->length, Msg->to, Via,
	//			Msg->from, UTF8Title);

			ptr += sprintf(ptr, "},\r\n"); 

			n--;

			if (n == 0)
				break;
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

	sprintf(timebuf, "%02d:%02d:%02d", hh, mm, ss);

	return timebuf;
}


int sendFwdConfig(struct HTTPConnectionInfo * Session, char * response, char * Rest, int Auth)
{
	struct UserInfo * USER;
	char * ptr = response;
	int n = 0;
	int i = 0;
	int Len = 0;


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


		ptr += sprintf(ptr, "{\r\n");
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
		ptr += sprintf(ptr, " }\r\n},\r\n");
	}

	if (response[n])		// No entries
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n");
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