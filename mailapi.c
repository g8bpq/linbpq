// basic JASON API to BPQ Node

// Authentication is via Telnet USER records.


#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include "CHeaders.h"
#include <stdlib.h>
#include "bpqmail.h"


// Constants
#define TOKEN_SIZE 32 // Length of the authentication token
#define TOKEN_EXPIRATION 7200 // Token expiration time in seconds (2 hours)

// Token data structure
typedef struct MailToken {
	char token[TOKEN_SIZE + 1];
	time_t expiration_time;
	struct UserInfo * User;
	char Call[10]; 
	struct MailToken* next;
} MailToken;

static MailToken * token_list = NULL;

static int verify_token(const char* token);
static void remove_expired_tokens();
static int  request_token(char * response);
static void add_token_to_list(MailToken* token);
static MailToken * find_token(const char* token); 

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


int MailAPIProcessHTTPMessage(char * response, char * Method, char * URL, char * request, BOOL LOCAL, char  *Params)
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
					Token->token, TOKEN_EXPIRATION);

			return strlen(response);

		}
	}

	return 0;
}
