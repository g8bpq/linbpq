// basic JASON API to BPQ Node

// Authentication is via Telnet USER records.


#define _CRT_SECURE_NO_DEPRECATE

#include "cheaders.h"
#include <stdlib.h>
#include "tncinfo.h"
#include "asmstrucs.h"
#include "telnetserver.h"
#include "kiss.h"

// Constants
#define TOKEN_SIZE 32 // Length of the authentication token
#define TOKEN_EXPIRATION 7200 // Token expiration time in seconds (2 hours)

// Token data structure
typedef struct Token {
	char token[TOKEN_SIZE + 1];
	time_t expiration_time;
	struct Token* next;
} Token;

typedef struct API
{
	char *URL;
	int URLLen;
	int (* APIRoutine)(char * response, char * token, char * param, int Local);
	int Auth;
} API;

// Auth defines

#define AuthNone 0
#define AuthUser 1
#define AuthBBSUser 2
#define AuthSysop 4

// Function prototypes
void handle_request(SOCKET client_socket, char * request, char * response);
int verify_token(const char* token);
void remove_expired_tokens();
char* fetch_data(const char* endpoint);
int  request_token(char * response);
int send_http_response(char * response, const char* msg);
int create_json_response(char * response, char* access_token, int expires_in, char* scope);
void add_token_to_list(Token* token);

Token* find_token(const char* token);
Token* generate_token();

int sendPortList(char * response, char * token, char * Rest, int Local);
int sendNodeList(char * response, char * token, char * Rest, int Local);
int sendUserList(char * response, char * token, char * Rest, int Local);
int sendInfo(char * response, char * token, char * Rest, int Local);
int sendLinks(char * response, char * token, char * Rest, int Local);
int sendPortMHList(char * response, char * token, char * Rest, int Local);
int sendPortQState(char * response, char * token, char * Rest, int Local);
int sendWhatsPacState(char * response, char * token, char * param, int Local);
int sendWhatsPacConfig(char * response, char * token, char * param, int Local);

void BuildPortMH(char * MHJSON, struct PORTCONTROL * PORT);
DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);

// Token list
Token* token_list = NULL;


struct API APIList[] =
{
	"/api/ports", 10, sendPortList, 0,
	"/api/nodes", 10, sendNodeList, 0,
	"/api/info", 9, sendInfo, 0,
	"/api/links", 10, sendLinks, 0,
	"/api/users", 10, sendUserList, 0,
	"/api/mheard", 11, sendPortMHList, 0,
	"/api/tcpqueues", 14, sendPortQState, 0,
	"/api/v1/config", 14, sendWhatsPacConfig, AuthSysop,
	"/api/v1/state", 13, sendWhatsPacState, AuthSysop
};

int APICount = sizeof(APIList) / sizeof(struct API);

extern int HTTPPort;


int xx()
{
	while (1) 
	{
		// Remove expired tokens
		remove_expired_tokens();

		// Handle the client request
		//      handle_request();
	}
	return 0;
}

int APIProcessHTTPMessage(char * response, char * Method, char * URL, char * request, BOOL LOCAL, BOOL COOKIE)
{
	const char * auth_header = "Authorization: Bearer ";
	char * token_begin = strstr(request, auth_header);
	char token[TOKEN_SIZE + 1]= "";
	int Flags = 0, n;

	// Node Flags isn't currently used

	char * Tok;
	char * param;

	if (token_begin)
	{
		// Using Auth Header

		// Extract the token from the request (assuming it's present in the request headers)

		if (token_begin == NULL)
		{		
			Debugprintf("Invalid request: No authentication token provided.\n");
			return send_http_response(response, "403 (Forbidden)");
		}

		token_begin += strlen(auth_header); // Move to the beginning of the token
		strncpy(token, token_begin, TOKEN_SIZE);
		token[TOKEN_SIZE] = '\0'; // Null-terminate the token

		param = strlop(URL, '?');
	}
	else
	{
		// There may be a token as first param, but if auth not needed may be misisng

		Tok = strlop(URL, '?');
		param = strlop(Tok, '&');

		if (Tok && strlen(Tok) == TOKEN_SIZE)
		{
			// assume auth token

			strcpy(token, Tok);
		}
		else param = Tok;
	}

	remove_expired_tokens();			// Tidy up

	// Check if the request is for token generation

	if (strcmp(Method, "OPTIONS") == 0)
	{
		// CORS Request

		char Resp[] =
			"HTTP/1.1 200 OK\r\n"
			"Access-Control-Allow-Origin: *\r\n"
			"Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
			"Access-Control-Allow-Headers: authorization";

		return send_http_response(response, Resp);
	}

	if (strcmp(Method, "POST") == 0)
	{
		if (_stricmp(URL, "/api/v1/config") == 0)
		{
			return send_http_response(response, "200 (OK)");
		}
	}

	if (strcmp(Method, "GET") != 0)
		return send_http_response(response, "403 (Bad Method)");

	if (_stricmp(URL, "/api/request_token") == 0)
		return request_token(response);
	
/*
if (token[0] == 0)
	{
		// Extract the token from the request (assuming it's present in the request headers)
		if (token_begin == NULL)
		{		
			Debugprintf("Invalid request: No authentication token provided.\n");
			return send_http_response(response, "403 (Forbidden)");
		}
		token_begin += strlen(auth_header); // Move to the beginning of the token
		strncpy(token, token_begin, TOKEN_SIZE);
		token[TOKEN_SIZE] = '\0'; // Null-terminate the token
	}

	// Verify the token
	if (!verify_token(token))
	{
		Debugprintf("Invalid authentication token.\n");
		return  send_http_response(response, "401 Unauthorized");
	}

	*/

	// Determine the requested API endpoint

	for (n = 0; n < APICount; n++)
	{
		struct API * APIEntry;
		char * rest;
		
		APIEntry = &APIList[n];

		if (_memicmp(URL, APIEntry->URL, APIEntry->URLLen) == 0)
		{
			rest = &request[4 + APIEntry->URLLen];	// Anything following?

			if (rest[0] == ' ' || rest[0] == '/' || rest[0] == '?')
				return APIEntry->APIRoutine(response, token, rest, LOCAL);
		}

	}

	return send_http_response(response, "401 Invalid API Call");
}

int request_token(char * response) 
{
	Token * token = generate_token();
	char scope[] = "create";

	printf("Token generated: %s\n", token->token);

	sprintf(response, "{\"access_token\":\"%s\", \"expires_at\":%ld,\"scope\":\"create\"}\r\n",
		token->token, token->expiration_time);

	return strlen(response);
}

Token * generate_token() 
{
	// Generate a random authentication token

	int i;

	Token * token = malloc(sizeof(Token));
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

void add_token_to_list(Token* token)
{
	if (token_list == NULL)
	{
		token_list = token;
		token->next = NULL;
	}
	else
	{
		Token* current = token_list;
		
		while (current->next != NULL) 
			current = current->next;

		current->next = token;
		token->next = NULL;
	}
}

int verify_token(const char* token)
{
	// Find the token in the token list
	Token * existing_token = find_token(token);

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

void remove_expired_tokens()
{
	time_t current_time = time(NULL);
	Token* current_token = token_list;
	Token* prev_token = NULL;
	Token* next_token;

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

Token * find_token(const char* token) 
{
	Token* current_token = token_list;
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

int send_http_response(char * response, const char* msg)
{
	return sprintf(response, "HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", msg);
}

/*
{
"access_token":"MTQ0NjJkZmQ5OTM2NDE1ZTZjNGZmZjI3",
"expires_in":3600,
"scope":"create"
}
*/

/*
{"ports":[
{"ID":"My Port", "Driver":"KISS", "Number":2, "State":"Active"),
{ ...},
{...}
]}
*/

extern int MasterPort[MAXBPQPORTS+1];	// Pointer to first BPQ port for a specific MPSK or UZ7HO host

int sendPortList(char * response, char * token, char * param, int Local)
{
	char * Array = 0;
	int ArrayLen = 0;
	int ArrayPtr = 0;
	
	struct _EXTPORTDATA * ExtPort;
	struct PORTCONTROL * Port;
	struct PORTCONTROL * SAVEPORT;
	int PortNo;

	int count;
	char DLL[20];
	char Status[32]="Unknown";
	char ID[33];
	char * ptr;

	ArrayPtr += sprintf(&response[ArrayPtr], "{\"ports\":[\r\n");

	for (count = 1; count <= NUMBEROFPORTS; count++)
	{
		Port = GetPortTableEntryFromSlot(count);
		ExtPort = (struct _EXTPORTDATA *)Port;
		PortNo = Port->PORTNUMBER;

		if (Port->PORTTYPE == 0x10)
		{	
			strcpy(DLL, ExtPort->PORT_DLL_NAME);
			strlop(DLL, '.');
			strlop(DLL, ' ');
		}
		else if (Port->PORTTYPE == 0)
			strcpy(DLL, "ASYNC");

		else if (Port->PORTTYPE == 22)
			strcpy(DLL, "I2C");

		else if (Port->PORTTYPE == 14)
			strcpy(DLL, "INTERNAL");

		else if (Port->PORTTYPE > 0 && Port->PORTTYPE < 14)
			strcpy(DLL, "HDLC");


		if (Port->PortStopped)
		{
			strcpy(Status, "Stopped");

		}
		else
		{
			if (Port->PORTTYPE == 0)
			{
				struct KISSINFO * KISS = (struct KISSINFO *)Port;
				NPASYINFO KPort;

				SAVEPORT = Port;

				if (KISS->FIRSTPORT && KISS->FIRSTPORT != KISS)
				{
					// Not first port on device

					Port = (struct PORTCONTROL *)KISS->FIRSTPORT;
					KPort = KISSInfo[PortNo];
				}

				KPort = KISSInfo[PortNo];

				if (KPort)
				{
					// KISS like - see if connected 

					if (Port->PORTIPADDR.s_addr || Port->KISSSLAVE)
					{
						// KISS over UDP or TCP

						if (Port->KISSTCP)
						{
							if (KPort->Connected)
								strcpy(Status, "Open  ");
							else
								if (Port->KISSSLAVE)
									strcpy(Status, "Listen");
								else
									strcpy(Status, "Closed");
						}
						else
							strcpy(Status, "UDP");
					}
					else
						if (KPort->idComDev)			// Serial port Open
							strcpy(Status, "Open  ");
						else
							strcpy(Status, "Closed");


					Port = SAVEPORT;
				}		
			}

			if (Port->PORTTYPE == 14)		// Loopback 
				strcpy(Status, "Open  ");

			else if (Port->PORTTYPE == 16)		// External
			{
				if (Port->PROTOCOL == 10)		// 'HF' Port
				{
					struct TNCINFO * TNC = TNCInfo[PortNo];

					if (TNC)
					{
						switch (TNC->Hardware)				// Hardware Type
						{
						case H_SCS:
						case H_KAM:
						case H_AEA:
						case H_HAL:
						case H_TRK:
						case H_SERIAL:

							// Serial

							if (TNC->hDevice)
								strcpy(Status, "Open  ");
							else
								strcpy(Status, "Closed");

							break;

						case H_UZ7HO:

							if (TNCInfo[MasterPort[PortNo]]->CONNECTED)
								strcpy(Status, "Open  ");
							else
								strcpy(Status, "Closed");

							break;

						case H_WINMOR:
						case H_V4:

						case H_MPSK:
						case H_FLDIGI:
						case H_UIARQ:
						case H_ARDOP:
						case H_VARA:
						case H_KISSHF:
						case H_WINRPR:
						case H_FREEDATA:

							// TCP

							if (TNC->CONNECTED)
							{
								if (TNC->Streams[0].Attached)
									strcpy(Status, "In Use");
								else
									strcpy(Status, "Open  ");
							}
							else
								strcpy(Status, "Closed");

							break;

						case H_TELNET:

							strcpy(Status, "Open  ");
						}
					}
				}
				else
				{
					// External but not HF - AXIP, BPQETHER VKISS, ??

					struct _EXTPORTDATA * EXTPORT = (struct _EXTPORTDATA *)Port;

					strcpy(Status, "Open  ");
				}
			}
		}

		strlop(Status, ' ');
		strcpy(ID, Port->PORTDESCRIPTION);
		ptr = &ID[29];
		while (*(ptr) == ' ')
		{
			*(ptr--) = 0;
		}

		ArrayPtr += sprintf(&response[ArrayPtr], " {\"ID\":\"%s\", \"Driver\":\"%s\", \"Number\":%d,\"State\":\"%s\"},\r\n",
			ID, DLL, Port->PORTNUMBER, Status);
	}

	ArrayPtr -= 3;		// remove trailing comma
	ArrayPtr += sprintf(&response[ArrayPtr], "\r\n]}\r\n");

	return ArrayPtr;
}

/*
{"Nodes":[
{"Call":"xx", "Alias":"xx", "Nbour1 ":"xx", "Quality":192),
{ ...},
{...}
]}
*/

extern int MaxNodes;
extern struct DEST_LIST * DESTS;		// NODE LIST
extern int DEST_LIST_LEN;


int sendNodeList(char * response, char * token, char * param, int Local)
{
	int ArrayPtr = 0;
	
	int count, len, i;
	char Normcall[10], Portcall[10];
	char Alias[7];
	struct DEST_LIST * Dests = DESTS ;
	//	struct ROUTE * Routes;

	Dests = DESTS;
	MaxNodes = MAXDESTS;

	ArrayPtr += sprintf(&response[ArrayPtr], "{\"nodes\":[\r\n");

	Dests-=1;

	for (count = 0; count < MaxNodes; count++)
	{
		Dests+=1;

		if (Dests->DEST_CALL[0] == 0)
			continue;

		len = ConvFromAX25(Dests->DEST_CALL, Normcall);
		Normcall[len] = 0;

		memcpy(Alias, Dests->DEST_ALIAS, 6);

		Alias[6]=0;

		for (i=0;i<6;i++)
		{
			if (Alias[i] == ' ')
				Alias[i] = 0;
		}


		ArrayPtr += sprintf(&response[ArrayPtr], " {\"Call\":\"%s\", \"Alias\":\"%s\", \"Routes\":[", Normcall, Alias);


		// Add an array  with up to 6 objects (3 NR + 3 INP3 Neighbours

		if (Dests->NRROUTE[0].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[0].ROUT_NEIGHBOUR->INP3Node == 0)
		{
			len = ConvFromAX25(Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len] = 0;

			ArrayPtr += sprintf(&response[ArrayPtr], "{\"Call\":\"%s\", \"Port\":%d, \"Quality\":%d},",
				Portcall, Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_PORT, Dests->NRROUTE[0].ROUT_QUALITY);


			//				if (Dests->NRROUTE[0].ROUT_OBSCOUNT > 127)
			//				{
			//					len=sprintf(&line[cursor],"! ");
			//					cursor+=len;
			//				}


			if (Dests->NRROUTE[1].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[1].ROUT_NEIGHBOUR->INP3Node == 0)
			{
			len=ConvFromAX25(Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;

	

			ArrayPtr += sprintf(&response[ArrayPtr], " {\"Call\":\"%s\", \"Port\":%d, \"Quality\":%d},",
				Portcall, Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_PORT, Dests->NRROUTE[1].ROUT_QUALITY);
			//if (Dests->NRROUTE[1].ROUT_OBSCOUNT > 127)
			//{
			//len=sprintf(&line[cursor],"! ");
			//cursor+=len;
			//}

			}

			if (Dests->NRROUTE[2].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[2].ROUT_NEIGHBOUR->INP3Node == 0)
			{
			len=ConvFromAX25(Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;


			ArrayPtr += sprintf(&response[ArrayPtr], " {\"Call\":\"%s\", \"Port\":%d, \"Quality\":%d},",
				Portcall, Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_PORT, Dests->NRROUTE[1].ROUT_QUALITY);

			//if (Dests->NRROUTE[2].ROUT_OBSCOUNT > 127)
			//{
			//len=sprintf(&line[cursor],"! ");
			//cursor+=len;

			}
			ArrayPtr -= 1;		// remove comma
		}

		ArrayPtr += sprintf(&response[ArrayPtr], "]},\r\n");
	}	

	ArrayPtr -= 3;		// remove comma
	ArrayPtr += sprintf(&response[ArrayPtr], "\r\n]}");
	
	return ArrayPtr;
}


int sendUserList(char * response, char * token, char * param, int Local)
{
	int ArrayPtr = 0;
	int n = MAXCIRCUITS;
	TRANSPORTENTRY * L4 = L4TABLE;
//	TRANSPORTENTRY * Partner;
	int MaxLinks = MAXLINKS;
	char State[12] = "", Type[12] = "Uplink";
	char LHS[50] = "", MID[10] = "", RHS[50] = "";
//	char Line[100];
	char Normcall[10];
	int len;
		
	ArrayPtr += sprintf(&response[ArrayPtr], "{\"users\":[\r\n");

	while (n--)
	{
		if (L4->L4USER[0])
		{
			RHS[0] = MID[0] = 0;

			len = ConvFromAX25(L4->L4USER, Normcall);
			Normcall[len] = 0;

			ArrayPtr += sprintf(&response[ArrayPtr], " {\"Call\": \"%s\"},\r\n", Normcall);
			L4++;
		}
	}

	if (ArrayPtr == 12)			//empty list
	{
		ArrayPtr -=2;
		ArrayPtr += sprintf(&response[ArrayPtr], "]}\r\n");
	}
	else
	{
		ArrayPtr -= 3;		// remove trailing comma
		ArrayPtr += sprintf(&response[ArrayPtr], "\r\n]}\r\n");
	}
	return ArrayPtr;
}

extern char MYALIASLOPPED[];
extern char TextVerstring[];
extern char LOCATOR[];

int sendInfo(char * response, char * token, char * param, int Local)
{
	char call[10];

	memcpy(call, MYNODECALL, 10);
	strlop(call, ' ');

	sprintf(response, "{\"info\":{\"NodeCall\":\"%s\", \"Alias\":\"%s\", \"Locator\":\"%s\", \"Version\":\"%s\"}}\r\n",
		call, MYALIASLOPPED, LOCATOR, TextVerstring);

	return strlen(response);
}

int sendLinks(char * response, char * token, char * param, int Local)
{
	struct _LINKTABLE * Links = LINKS;
	int MaxLinks = MAXLINKS;
	int count;
	char Normcall1[10];
	char Normcall2[10];
	char State[12] = "", Type[12] = "Uplink";
	int axState;
	int cctType;
	int ReplyLen = 0;
	ReplyLen += sprintf(&response[ReplyLen],"{\"links\":[\r\n");

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



			ReplyLen += sprintf(&response[ReplyLen], "{\"farCall\": \"%s\",\"ourCall\": \"%s\", \"port\": \"%d\", \"state\": \"%s\", \"linkType\": \"%s\", \"ax25Version\": \"%d\"},\r\n",
				Normcall1, Normcall2, Links->LINKPORT->PORTNUMBER,
				State, Type, 2 - Links->VER1FLAG );
			Links+=1;
		}
	}

	if (ReplyLen < 13)
		ReplyLen -= 2;          // no links
	else
		ReplyLen -= 3;         // remove trailing comma

	ReplyLen+= sprintf(&response[ReplyLen], "\r\n]}\r\n");

	return ReplyLen;
}

int sendPortMHList(char * response, char * token, char * param, int Local)
{
	struct PORTCONTROL * PORTVEC ;
	int n;
	int port = 0;

	if (param[0] = '?' || param[0] == '/')
		port = atoi(&param[1]);

	PORTVEC = GetPortTableEntryFromPortNum(port);
	response[0] = 0;

	if (PORTVEC == 0)
		return send_http_response(response, "401 Invalid API Call");

	n = sprintf(response,"{\"mheard\":[\r\n");

	BuildPortMH(&response[n], PORTVEC );

	if (response[n] == 0)		// No entries
	{
		response[strlen(response) - 2] = '\0';          // remove \r\n
		strcat(response, "]}\r\n");
	}
	else
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n]}\r\n");
		//      printf("MH for port %d:\r\n%s\r\n", PORTVEC->PORTNUMBER, response);
	}

	return strlen(response);
}

int sendPortQState(char * response, char * token, char * param, int Local)
{
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;
	struct ConnectionInfo * Conn;
	struct STREAMINFO * STREAM;
	int Stream;
	int tcpqueue;
	int Queued;
	int n;
	int port = 0;
	char Type[10];
	char Appl[20];
	int radioport = 0;


	if (param[0] = '?' || param[0] == '/')
		port = atoi(&param[1]);

	TNC = TNCInfo[port];

	// At the moment only supports Telnet Ports

	if (TNC == 0 || TNC->Hardware != H_TELNET)
		return send_http_response(response, "401 Invalid API Call");

	response[0] = 0;


	TCP = TNC->TCPInfo;

	if (TCP == 0)
		return send_http_response(response, "401 Invalid API Call");

	n = sprintf(response,"{\"QState\":[\r\n");

	for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
	{
		char Call[10];	
		STREAM = &TNC->Streams[Stream];

		Conn = TNC->Streams[Stream].ConnectionInfo;


		// if connected to the node

		if (Conn->SocketActive)
		{
			TRANSPORTENTRY * Sess1 = TNC->PortRecord->ATTACHEDSESSIONS[Stream];
			TRANSPORTENTRY * Sess2 = NULL;

			if (Sess1)
				Sess2 = Sess1->L4CROSSLINK;
			else
				continue;

			radioport = 0;
			
			// Can't use TXCount - it is Semaphored=

			Queued = C_Q_COUNT(&TNC->Streams[Stream].PACTORtoBPQ_Q);
			Queued += C_Q_COUNT((UINT *)&TNC->PortRecord->PORTCONTROL.PORTRX_Q);

			if (Sess2)
				Queued += CountFramesQueuedOnSession(Sess2);

			if (Sess1)
				Queued += CountFramesQueuedOnSession(Sess1);


	//		CountFramesQueuedOnSession(TRANSPORTENTRY * Session)

			tcpqueue = Conn->FromHostBuffPutptr - Conn->FromHostBuffGetptr;

			if (Sess2)
				Sess1 = Sess2;
	
			Call[ConvFromAX25(Sess1->L4USER, Call)] = 0;
			

			if (Sess1->L4CIRCUITTYPE & BPQHOST)
				strcpy(Type, "Host");
	
			else if (Sess1->L4CIRCUITTYPE & SESSION)
			{
				struct DEST_LIST * DEST = Sess1->L4TARGET.DEST;

				strcpy(Type, "NETROM");

				if (DEST)
				{
					int ActiveRoute = DEST->DEST_ROUTE;

					if (ActiveRoute)
					{
						struct ROUTE * ROUTE = DEST->NRROUTE[ActiveRoute - 1].ROUT_NEIGHBOUR;

						if (ROUTE)
						{
							struct _LINKTABLE * LINK = ROUTE->NEIGHBOUR_LINK;
				
							if (LINK && LINK->LINKPORT)
								radioport = LINK->LINKPORT->PORTNUMBER;
						}
					}
				}
			}
			else if (Sess1->L4CIRCUITTYPE & PACTOR)
			{
				//	PACTOR Type - Frames are queued on the Port Entry

				struct PORTCONTROL * PORT = Sess1->L4TARGET.PORT;
				strcpy(Type, "HFLINK");

				if (PORT)
					radioport = PORT->PORTNUMBER;

			}
			else
			{
				struct _LINKTABLE * LINK = Sess1->L4TARGET.LINK;
			
				strcpy(Type, "L2 Link");

				if (LINK && LINK->LINKPORT)
					radioport = LINK->LINKPORT->PORTNUMBER;
			}


			memcpy(Appl, Sess1->APPL, 16);
			strlop(Appl, ' ');
	
			n += sprintf(&response[n], "{\"APPL\": \"%s\", \"callSign\": \"%s\", \"type\": \"%s\", \"tcpqueue\": %d, \"packets\": %d, \"port\": %d},\r\n" , Appl, Call, Type, tcpqueue, Queued, radioport);
		
		}
	}

	if (n < 20)		// No entries
	{
		response[strlen(response) - 2] = '\0';          // remove \r\n
		strcat(response, "]}\r\n");
	}
	else
	{
		response[strlen(response)-3 ] = '\0';          // remove ,\r\n
		strcat(response, "\r\n]}\r\n");
		//      printf("MH for port %d:\r\n%s\r\n", PORTVEC->PORTNUMBER, response);
	}
	return strlen(response);
}




// WhatsPac configuration interface
// WhatsPac also uses Paula's Remote Host Protocol (RHP). This is in a separate module

extern int WhatsPacConfigured;


int sendWhatsPacState(char * response, char * token, char * param, int Local)
{
	if (Local == 0)
		return send_http_response(response, "401 Not Authorised");

	if (WhatsPacConfigured)
		sprintf(response, "{\"configured\": true}\r\n");
	else
		sprintf(response, "{\"configured\": false}\r\n");

	return strlen(response);
}


int sendWhatsPacConfig(char * response, char * token, char * param, int Local)
{
	char Template[] =
	"{\"MODE\": 0,"
	"\"RHPPORT\": \"%d\","
	"\"AGWPORT\": \"7000\","
	"\"INTERFACES\": [{\"INTERFACE\": 1,\"PROTOCOL\": \"KISS\",\"TYPE\": \"TCP\",\"IOADDR\": \"127.0.0.1\",\"INTNUM\": 8100}],"
	"\"PORTS\": [{\"PORT\": 1,\"ID\": \"RHPPORT\", \"INTERFACENUM\": 1}]}";

	if (Local == 0)
		return send_http_response(response, "401 Not Authorised");

	sprintf(response, Template, HTTPPort);

	return strlen(response);
}




