
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include "winsock.h"
#define ioctl ioctlsocket
#else

#define SOCKET int
#define BOOL int
#define TRUE 1
#define FALSE 0

#define strtok_s strtok_r

#define _memicmp memicmp
#define _stricmp stricmp
#define _strdup strdup
#define _strupr strupr
#define _strlwr strlwr
#define WSAGetLastError() errno
#define GetLastError() errno 

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <string.h>

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>
#include <stdint.h>

char * strupr(char* s)
{
	char* p = s;

	if (s == 0)
		return 0;

	while (*p = toupper( *p )) p++;
	return s;
}

char * strlwr(char* s)
{
	char* p = s;
	while (*p = tolower( *p )) p++;
	return s;
}

#endif

struct ChatNodeData
{
	char Call[10];
	char NAlias[10];
	double Lat;
	double Lon;
	double derivedLat;
	double derivedLon;
	int PopupMode;
	char Comment[512];
	int hasLinks;
	int derivedPosn;
	time_t LastHeard;
};

struct ChatLink
{
	struct ChatNodeData * Call1;
	struct ChatNodeData * Call2;
	int Call1State;					// reported state from each end
	int Call2State;
	time_t LastHeard1;
	time_t LastHeard2;
};

struct ModeItem
{
	int Mode;
	int Interlock;
	double Freq;
	time_t LastHeard;
};

struct ModeEntries
{
	struct ModeItem * Mode[32];		// One per port
};

struct FreqItem
{
	int Interlock;
	char * Freqs;					// Store as text as that is what the web page will want
	time_t LastHeard;
};

struct FreqEntries
{
	struct FreqItem * Freq[32];		// One per interlock group
};

struct ChatNodeData ** ChatNodes = NULL;

int NumberOfChatNodes = 0;

struct ChatLink ** ChatLinks = NULL;

int NumberOfChatLinks = 0;

struct NodeData
{
	char Call[10];
	double Lat;
	double Lon;
	int PopupMode;
	char Comment[512];
	time_t LastHeard;
	int onlyHeard;			// Station heard but not reporting
	struct HeardItem * HeardBy;
	struct HeardItem * Heard;
	struct ModeEntries * Modes;
	struct FreqEntries * Freqs;
};

struct NodeData ** Nodes = NULL;

int NumberOfNodes = 0;


struct NodeLink
{
	struct NodeData * Call1;
	struct NodeData * Call2;
	int Type;
	time_t LastHeard;
};

struct NodeLink ** NodeLinks = NULL;

int NumberOfNodeLinks = 0;


struct HeardItem
{
	struct HeardItem * Next;			// Chein of heard calls
	struct HeardItem * NextBy;			// Chain of heardby calls
	time_t Time;
	struct NodeData * ReportingCall;
	struct NodeData * HeardCall;
	char Freq[16];
	char Flags[16];
};

/*
struct HeardData
{
struct NodeData * CallSign;
struct HeardItem * HeardItems;
};

struct HeardByData
{
struct NodeData * CallSign;
struct HeardItem * HeardItems;
};
*/
SOCKET sock, chatsock;

time_t LastUpdate = 0;


int ConvFromAX25(unsigned char * incall, char * outcall);
void GenerateOutputFiles(time_t Now);
void UpdateHeardData(struct NodeData * Node, struct NodeData * Call, char * Freq, char * LOC, char * Flags);
char * strlop(char * buf, char delim);
void ProcessChatUpdate(char * From, char * Msg);
void ProcessNodeUpdate(char * From, char * Msg);

struct NodeData * FindNode(char * Call)
{
	struct NodeData * Node;
	int i;

	// Find, and if not found add

	for (i = 0; i < NumberOfNodes; i++)
	{
		if (strcmp(Nodes[i]->Call, Call) == 0)
			return Nodes[i];
	}

	Node = malloc(sizeof(struct NodeData));
	memset (Node, 0, sizeof(struct NodeData))
		;
	Nodes = realloc(Nodes, (NumberOfNodes + 1) * sizeof(void *));
	Nodes[NumberOfNodes++] = Node;

	strcpy(Node->Call, Call);
	strcpy(Node->Comment, Call);

	return Node;
}

struct NodeData * FindBaseCall(char * Call)
{
	// See if we have any other ssid of the requested call

	char FindCall[12]= "";
	char compareCall[12] = "";

	int i;

	if (strlen(Call) > 9)
		return NULL;

	if (strcmp(Call, "PE1NNZ-8") == 0)
		i = 0;

	memcpy(FindCall, Call, 10);
	strlop(FindCall, '-');

	for (i = 0; i < NumberOfNodes; i++)
	{
		memcpy(compareCall, Nodes[i]->Call, 10);
		strlop(compareCall, '-');
		if (strcmp(compareCall, FindCall) == 0 && Nodes[i]->Lat != 0.0)
			return Nodes[i];
	}

	return NULL;
}

struct NodeLink * FindLink(char * Call1, char * Call2, int Type)
{
	struct NodeLink * Link;
	struct NodeData * Node;

	int i;

	// Find, and if not found add

	for (i = 0; i < NumberOfNodeLinks; i++)
	{
		// We only want one copy, whichever end it is reported from

		Link = NodeLinks[i];

		if (strcmp(Link->Call1->Call, Call1) == 0 && Link->Type == Type && strcmp(Link->Call2->Call, Call2) == 0)
			return Link;

		if (strcmp(Link->Call1->Call, Call2) == 0 && Link->Type == Type && strcmp(Link->Call2->Call, Call1) == 0)
			return Link;
	}

	Link = malloc(sizeof(struct NodeLink));
	memset (Link, 0, sizeof(struct NodeLink))
		;
	NodeLinks = realloc(NodeLinks, (NumberOfNodeLinks + 1) * sizeof(void *));
	NodeLinks[NumberOfNodeLinks++] = Link;


	Node = FindNode(Call1);
	Node->onlyHeard = 0;			// Reported
	Link->Call1 = Node;
	Node = FindNode(Call2);
	Node->onlyHeard = 0;			// Reported
	Link->Call2 = Node;
	Link->Type = Type;

	return Link;
}

struct ChatNodeData * FindChatNode(char * Call)
{
	struct ChatNodeData * Node;
	int i;

	if (strcmp("KD8MJL-11", Call) == 0)
	{
		int xx = 1;
	}


	// Find, and if not found add

	for (i = 0; i < NumberOfChatNodes; i++)
	{
		if (strcmp(ChatNodes[i]->Call, Call) == 0)
			return ChatNodes[i];
	}

	Node = malloc(sizeof(struct ChatNodeData));
	memset (Node, 0, sizeof(struct ChatNodeData))
		;
	ChatNodes = realloc(ChatNodes, (NumberOfChatNodes + 1) * sizeof(void *));
	ChatNodes[NumberOfChatNodes++] = Node;

	strcpy(Node->Call, Call);
	strcpy(Node->Comment, Call);
	return Node;
}

struct ChatLink * FindChatLink(char * Call1, char * Call2, int State, time_t Time)
{
	struct ChatLink * Link;
	struct ChatNodeData * Node;

	int i;

	// Find, and if not found add

	for (i = 0; i < NumberOfChatLinks; i++)
	{
		// We only want one copy, whichever end it is reported from

		Link = ChatLinks[i];

		if (strcmp(Link->Call1->Call, Call1) == 0 && strcmp(Link->Call2->Call, Call2) == 0)
		{
			Link->Call1State = State;
			Link->LastHeard1 = Time;
			return Link;
		}
		if (strcmp(Link->Call1->Call, Call2) == 0 && strcmp(Link->Call2->Call, Call1) == 0)
		{
			Link->Call2State = State;
			Link->LastHeard2 = Time;
			return Link;
		}
	}

	Link = malloc(sizeof(struct ChatLink));
	memset (Link, 0, sizeof(struct ChatLink));

	ChatLinks = realloc(ChatLinks, (NumberOfChatLinks + 1) * sizeof(void *));
	ChatLinks[NumberOfChatLinks++] = Link;

	Node = FindChatNode(Call1);
	Link->Call1 = Node;
	Node->hasLinks = 1;
	Node = FindChatNode(Call2);
	Link->Call2 = Node;
	Link->Call1State = State;
	Link->Call2State = -1;
	Node->hasLinks = 1;

	return Link;
}

int CompareChatCalls(const struct ChatNodeData ** a, const struct ChatNodeData ** b)
{
	return memcmp(a[0]->Call, b[0]->Call, 7);
}


int FromLOC(char * Locator, double * pLat, double * pLon)
{
	double i;
	double Lat, Lon;

	_strupr(Locator);

	*pLon = 0;
	*pLat = 0;			// in case invalid


	// The first pair (a field) encodes with base 18 and the letters "A" to "R".
	// The second pair (square) encodes with base 10 and the digits "0" to "9".
	// The third pair (subsquare) encodes with base 24 and the letters "a" to "x".

	i = Locator[0];

	if (i < 'A' || i > 'R')
		return 0;

	Lon = (i - 65) * 20;

	i = Locator[2];
	if (i < '0' || i > '9')
		return 0;

	Lon = Lon + (i - 48) * 2;

	i = Locator[4];
	if (i < 'A' || i > 'X')
		return 0;

	Lon = Lon + (i - 65) / 12;

	i = Locator[1];
	if (i < 'A' || i > 'R')
		return 0;

	Lat = (i - 65) * 10;

	i = Locator[3];
	if (i < '0' || i > '9')
		return 0;

	Lat = Lat + (i - 48);

	i = Locator[5];
	if (i < 'A' || i > 'X')
		return 0;

	Lat = Lat + (i - 65) / 24;

	if (Lon < 0 || Lon > 360)
		Lon = 180;
	if (Lat < 0 || Lat > 180)
		Lat = 90;

	*pLon = Lon - 180;
	*pLat = Lat - 90;

	return 1;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;

	if (buf == NULL) return NULL;		// Protect

	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr = malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

FILE * logFile;

time_t Now;

int main(int argc, char * argv[])
{
	char RXBUFFER[512];
	struct NodeData * Node;
	struct ChatNodeData * ChatNode;
	struct NodeData * HeardNode;
	struct NodeData * HeardByNode;
	struct NodeLink * Link;
	struct ChatLink * ChatLink;
	char * Msg = &RXBUFFER[16];

	struct sockaddr_in rxaddr, txaddr, txaddr2;
	int addrlen = sizeof(struct sockaddr_in);
	u_long param = 1;
	BOOL bcopt=TRUE;
	struct sockaddr_in sinx;
	int nLength;

	struct hostent * HostEnt1, * HostEnt2;
	char * Context;
	FILE * pFile;
	char Line[65536];

	char * pCall;
	char * pCall2;
	char * pComment;
	char * pLat, * pLon;
	char * pPopupMode;
	char * pLastHeard;
	char * pType;
	char * pHeardOnly;
	char * pHeard;
	char * pHeardby;
	char * pModes;
	char * pFreqs;
	char * Next;

	char * pHeardCall;
	char * pFreq;
	char * pFlags;
	char * pHeardTime = "0";
	char * pInterLock;
	char * pTime;
	char * pMode;
	char * pPort;
	char * pStatus1;
	char * pStatus2;

	double Lat, Lon;

	struct HeardItem * Item;
	struct HeardItem * lastItem = NULL;
	SOCKET maxsock;
	fd_set readfd, writefd, exceptfd;
	struct timeval timeout;
	int retval;
	char * ptr;
	time_t restartTime = 0;

#ifdef WIN32
	WSADATA WsaData;            // receives data from WSAStartup
	WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif

	timeout.tv_sec = 60;
	timeout.tv_usec = 0;


	logFile = fopen("nodelog.txt","ab");

	// Read Saved Nodes

	pFile = fopen("savenodes.txt","r");

	while (pFile)
	{
		if (fgets(Line, 65535, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			int j;

			pCall = strtok_s(Line, ",", &Context); 

			if (strcmp(pCall, "EI5HBB-10") == 0)
			{
				int i = 1;
			}
			pLat = strtok_s(NULL, ",", &Context); 
			pLon = strtok_s(NULL, ",", &Context); 
			pPopupMode = strtok_s(NULL, ",", &Context); 
			pComment = strtok_s(NULL, "|", &Context); 
			pLastHeard = strtok_s(NULL, "|", &Context);

			// Can't use strtok from here as fields may be missing

			pHeardOnly = Context;
			pHeard = strlop(pHeardOnly, '|');

			if (pLastHeard == NULL)
				continue;

			Node = FindNode(pCall);
			Lat = atof(pLat);
			Lon = atof(pLon);

			if (Lat >  90.0 || Lon > 180.0 || Lat < -90.0 || Lon < -180.0)
			{
				Lat = 0.0;
				Lon = 0.0;
			}

			Node->Lat = Lat;
			Node->Lon = Lon;

			Node->PopupMode = atoi(pPopupMode);
			Node->LastHeard = atoi(pLastHeard);
			Node->onlyHeard = atoi(pHeardOnly);
			if (strlen(pComment) > 510)
				pComment[510] = 0;

			strcpy(Node->Comment, pComment);

			pHeardby = strlop(pHeard, '|');

			// Look for heard/heard by entries

			// Each heard entry is five comma delimited fields, ending with /

			while (pHeard && strlen(pHeard) > 2)
			{			
				Next = strlop(pHeard, '/');

				pHeardCall = strlop(pHeard, ',');
				pFreq = strlop(pHeardCall, ',');
				pFlags = strlop(pFreq, ',');
				pHeardTime = strlop(pFlags, ',');

				if (pFlags && strlen(pFlags) > 14)
					break;

				HeardNode = FindNode(pHeard);

				if (HeardNode->LastHeard == 0)
					HeardNode->onlyHeard = 1;			// So far not reported

				// See if we already have it

				Item = Node->Heard;

				while (Item)
				{
					if (Item->HeardCall->Call == HeardNode->Call && strcmp(Item->Freq, pFreq) == 0 && strcmp(Item->Flags, pFlags) == 0)
						break;

					Item = Item->Next;
				}

				if (Item == NULL)
				{
					Item = malloc(sizeof(struct HeardItem));

					Item->ReportingCall = Node;
					Item->HeardCall = HeardNode;
					strcpy(Item->Freq, pFreq);
					strcpy(Item->Flags, pFlags);

					if (pHeardTime)
						Item->Time = atoi(pHeardTime);

					Item->Next = 0;
					Item->NextBy = 0;

					if (Node->Heard == NULL)		// First
						Node->Heard = Item;
					else
						lastItem->Next = Item;

					lastItem = Item;
				}

				pHeard = Next;
			}

			// Now do heard by

			pHeard = pHeardby;
			pModes = strlop(pHeard, '|');

			while (pHeard && strlen(pHeard) > 2)
			{				
				Next = strlop(pHeard, '/');

				pHeardCall = strlop(pHeard, ',');
				pFreq = strlop(pHeardCall, ',');
				pFlags = strlop(pFreq, ',');
				pHeardTime = strlop(pFlags, ',');

				if (pFlags && strlen(pFlags) > 14)
					break;

				if (pHeardCall == NULL)
					break;

				HeardByNode = FindNode(pHeardCall);

				if (HeardByNode->LastHeard == 0)
					HeardByNode->onlyHeard = 1;			// So far not reported

				HeardNode = FindNode(pHeard);

				// See if we already have it

				Item = Node->HeardBy;

				while (Item)
				{
					if (Item->HeardCall->Call == HeardNode->Call && strcmp(Item->Freq, pFreq) == 0 && strcmp(Item->Flags, pFlags) == 0)
						break;

					Item = Item->NextBy;
				}

				if (Item == NULL)
				{

					Item = malloc(sizeof(struct HeardItem));

					Item->ReportingCall = HeardByNode;
					Item->HeardCall = HeardNode;
					strcpy(Item->Freq, pFreq);
					strcpy(Item->Flags, pFlags);
					if (pHeardTime)
						Item->Time = atoi(pHeardTime);

					Item->Next = 0;
					Item->NextBy = 0;

					if (Node->HeardBy == NULL)		// First
						Node->HeardBy = Item;
					else
						lastItem->Next = Item;

					lastItem = Item;
				}
				pHeard = Next;
			}

			pFreqs = strlop(pModes, '|');

			while (pModes && strlen(pModes) > 2)
			{	
				struct ModeEntries * Modes;
				struct ModeItem * Mode;

				Next = strlop(pModes, '/');

				pPort = pModes;
				pMode = strlop(pPort, ',');
				pInterLock = strlop(pMode, ',');
				pHeardTime = strlop(pInterLock, ',');
				pFreq = strlop(pHeardTime, ',');

				if (pHeardTime)
				{
					int Port = atoi(pPort);

					if (Port < 32)
					{
						Modes = Node->Modes;

						if (Modes == NULL)
							Modes = Node->Modes = (struct ModeEntries *)zalloc(sizeof(struct ModeEntries));

						Mode = Modes->Mode[Port];

						if (Mode == NULL)
							Mode = Modes->Mode[Port] = (struct ModeItem *)zalloc(sizeof(struct ModeItem));

						Mode->LastHeard = atoi(pHeardTime);
						Mode->Mode = atoi(pMode);
						Mode->Interlock = atoi(pInterLock);

						if (pFreq)
							Mode->Freq = atof(pFreq);
					}
				}

				pModes = Next;
			}

			strlop(pFreqs, '|');

			j = 0;

			while (pFreqs && strlen(pFreqs) > 2)
			{	
				struct FreqItem * FreqItem;

				Next = strlop(pFreqs, '&');

				pInterLock = pFreqs;
				pTime = strlop(pInterLock, ',');
				pHeardTime = strlop(pTime, ',');

				if (pHeardTime)
				{
					if (Node->Freqs == NULL)
						Node->Freqs = (struct FreqEntries *)zalloc(sizeof(struct FreqEntries));

					FreqItem = Node->Freqs->Freq[j] = (struct FreqItem *)zalloc(sizeof(struct FreqItem));

					FreqItem->Interlock = atoi(pInterLock);
					FreqItem->LastHeard = atoi(pHeardTime);
					FreqItem->Freqs = _strdup(pTime);
					j++;
				}
				pFreqs = Next;
			}

			if (Node->Heard == 0 && Node->HeardBy && Node->LastHeard == 0)
				Node->onlyHeard = 1;
		}
	}


	pFile = fopen("savelinks.txt","r");

	while (pFile)
	{
		if (fgets(Line, 499, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			pCall = strtok_s(Line, ",", &Context); 
			pCall2 = strtok_s(NULL, ",", &Context); 
			pType = strtok_s(NULL, ",", &Context); 
			pLastHeard = strtok_s(NULL, ",", &Context); 

			if (pLastHeard == NULL)
				continue;

			Link = FindLink(pCall, pCall2, atoi(pType));
			Link->LastHeard = atoi(pLastHeard);
		}
	}

	// Read Chat Files

	pFile = fopen("savechat.txt","r");

	while (pFile)
	{
		if (fgets(Line, 16383, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			pCall = strtok_s(Line, ",", &Context); 
			pLat = strtok_s(NULL, ",", &Context); 
			pLon = strtok_s(NULL, ",", &Context); 
			pPopupMode = strtok_s(NULL, ",", &Context); 

			// Can't use strtok from here as fields may be missing

			pComment = Context;
			pLastHeard = strlop(pComment, '|');

			if (pLastHeard == NULL)
				continue;

			ChatNode = FindChatNode(pCall);

			Lat = atof(pLat);
			Lon = atof(pLon);

			if (Lat >  90.0 || Lon > 180.0 || Lat < -90.0 || Lon < -180.0)
			{
				Lat = 0.0;
				Lon = 0.0;
			}

			ChatNode->Lat = Lat;
			ChatNode->Lon = Lon;
			ChatNode->derivedLat = 	Lat;
			ChatNode->derivedLon = 	Lon;

			ChatNode->PopupMode = atoi(pPopupMode);
			ChatNode->LastHeard = atoi(pLastHeard);
			if (strlen(pComment) > 510)
				pComment[510] = 0;

			strcpy(ChatNode->Comment, pComment);
		}
	}

	pFile = fopen("savechatlinks.txt","r");

	while (pFile)
	{
		if (fgets(Line, 16383, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			pCall = strtok_s(Line, ",", &Context); 
			pCall2 = strtok_s(NULL, ",", &Context); 
			pStatus1 = strtok_s(NULL, ",", &Context); 
			pStatus2 = strtok_s(NULL, ",", &Context); 
			pHeardTime = strtok_s(NULL, ",", &Context); 

			if (pHeardTime == NULL)
				continue;

			ChatLink = FindChatLink(pCall, pCall2, atoi(pStatus1), 0);
			ChatLink->LastHeard1 = atoi(pHeardTime);

			pHeardTime = strtok_s(NULL, ",", &Context); 

			if (pHeardTime)
				ChatLink->LastHeard2 = atoi(pHeardTime);
		}
	}

	LastUpdate = Now = time(NULL);

	sock = socket(AF_INET,SOCK_DGRAM,0);
	chatsock = socket(AF_INET,SOCK_DGRAM,0);

	maxsock = sock;

	if (chatsock > sock)
		maxsock = chatsock;

	ioctl(sock, FIONBIO, &param);
	ioctl(chatsock, FIONBIO, &param);

	//	ioctl(sock, FIONBIO, &param);

	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;		
	sinx.sin_port = htons(81);

	txaddr.sin_family = AF_INET;
	txaddr.sin_addr.s_addr = INADDR_ANY;		
	txaddr.sin_port = htons(81);

	txaddr2.sin_family = AF_INET;
	txaddr2.sin_addr.s_addr = INADDR_ANY;		
	txaddr2.sin_port = htons(81);

	//	Resolve name to address

	printf("Resolving %s\n", "guardian.no-ip.org.");
	HostEnt1 = gethostbyname ("guardian.no-ip.org.");

	if (HostEnt1)
		memcpy(&txaddr.sin_addr.s_addr,HostEnt1->h_addr,4);
	else
	{
		perror("Resolve");
		printf("Resolve %s Failed\r\n", "guardian.no-ip.org.");
		restartTime = time(NULL) + 120;		// Restart in 2 mins
	}

	HostEnt2 = gethostbyname ("10.8.0.38");

	if (HostEnt2)
		memcpy(&txaddr2.sin_addr.s_addr,HostEnt2->h_addr,4);

	if (bind(sock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		//	Bind Failed

		int err = GetLastError();
		printf("Bind Failed for UDP port %d - error code = %d", 81, err);
	}

	sinx.sin_port = htons(10090);

	if (bind(chatsock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		//	Bind Failed

		int err = GetLastError();
		printf("Bind Failed for UDP port %d - error code = %d", 10090, err);
	}

	printf("Map Update Started\n");
	fflush(stdout);

	while (1)
	{
		int ret = 0;

		if (time(NULL) > restartTime)
			return 0;

		FD_ZERO(&readfd);
		FD_ZERO(&writefd);
		FD_ZERO(&exceptfd);

		FD_SET(sock, &readfd);
		FD_SET(chatsock, &readfd);

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		retval = select((int)maxsock + 1, &readfd, &writefd, &exceptfd, &timeout);

		if (retval == -1)
		{				
			perror("select");
		}
		else
		{
			if (retval)
			{
				// see who has data

				if (FD_ISSET(sock, &readfd))
					nLength = recvfrom(sock, RXBUFFER, 512, 0, (struct sockaddr *)&rxaddr, &addrlen);

				else if (FD_ISSET(chatsock, &readfd))
					nLength = recvfrom(chatsock, RXBUFFER, 512, 0, (struct sockaddr *)&rxaddr, &addrlen);
			}
			else
				nLength = 0;

		}

		if (nLength < 0)
		{
			int err = WSAGetLastError();
			//			if (err != 11)
			//				printf("KISS Error %d %d\n", nLength, err);
			nLength = 0;
		}

		if (nLength == 0)
			continue;

		//	ret = sendto(sock, RXBUFFER, nLength, 0, (struct sockaddr *)&txaddr2, sizeof(txaddr));

		if (ret == -1)
			perror("sendto 1");

		RXBUFFER[nLength - 2] = 0;
		
		if (memcmp(&RXBUFFER[16], "LINK ", 5) != 0 && memcmp(&RXBUFFER[16], "INFO ", 5) != 0 && memcmp(&RXBUFFER[16], "FREQ ", 5) != 0)
		{
			ptr = &RXBUFFER[16];
		
			while (ptr = strchr(Msg, '|'))
				*ptr = '/';
		}

		if (memcmp(&RXBUFFER[16], "LINK ", 5) != 0)
		{
			if (HostEnt1)
			{
				ret = sendto(sock, RXBUFFER, nLength, 0, (struct sockaddr *)&txaddr, sizeof(txaddr));
				if (ret == -1)
					perror("sendto 1");
			}
		}

#ifdef WIN32
		Sleep(10);
#else
		usleep(10000);
#endif

		if (strstr(RXBUFFER, "ctl"))
		{
			//			return;
		}

		Now = time(NULL);

		if ((Now - LastUpdate) > 60)
			GenerateOutputFiles(Now);

		RXBUFFER[nLength - 2] = 0;

		if (RXBUFFER[14] == 3)			// UI
		{
			char From[10], To[10];

			From[ConvFromAX25((unsigned char *)&RXBUFFER[7], From)] = 0;
			To[ConvFromAX25((unsigned char *)&RXBUFFER[0], To)] = 0;

			if (strcmp(To, "DUMMY") == 0)
			{
				ProcessChatUpdate(From, Msg);
				continue;
			}

			if (strcmp(To, "DUMMY-1") == 0)
			{
				ProcessNodeUpdate(From, Msg);
				continue;
			}
		}
	}
}

#define H_WINMOR 1
#define H_SCS 2
#define H_KAM 3
#define H_AEA 4
#define H_HAL 5
#define H_TELNET 6
#define H_TRK 7
#define H_TRKM 7
#define H_V4 8
#define H_UZ7HO 9
#define H_MPSK 10
#define H_FLDIGI 11
#define H_UIARQ 12
#define H_ARDOP 13
#define H_VARA 14
#define H_SERIAL 15
#define H_KISSHF 16
#define H_WINRPR 17
#define H_HSMODEM 18

char ModeNames[20][16] = {
	"", "WINMOR", "PACTOR",  "PACTOR", "PACTOR", "PACTOR", "TELNET", "ROBUST", "V4",
	"PACKET", "MPSK", "FLDIGI", "UIARQ", "ARDOP", "VARA", "SERIAL", "PACKET", "ROBUST", "HSMODEM"};


void ProcessNodeUpdate(char * From, char * Msg)
{
	struct NodeData * Node;

	char * ptr, *Context;

	char * pHeardTime = "0";
	char * Comment, * Version;

	double Lat, Lon;

	struct HeardItem * lastItem = NULL;

	if (strcmp("KO0OOO-8", From) == 0)
	{
		int i = 1;
	}

	Node = FindNode(From);
	Node->LastHeard = Now;
	Node->onlyHeard = 0;			// Reported

	// printf("%s %s\n", From, Msg);
	//			fprintf(logFile, "%s %s %s\n", From, To, Msg);


	if (memcmp(Msg, "MH ", 3) == 0)
	{
		// There are 4 fields - Call, Freq, Loc, Flags

		char * Call, * Freq = 0, * LOC = 0, * Flags = 0;
		struct NodeData * heardCall;

		// I think strlop will work - cant use strtok with null fields

		Call = &Msg[3];
		Freq = strlop(Call, ',');
		LOC = strlop(Freq, ',');
		Flags = strlop(LOC, ',');

		if (strstr(Call, " to ") || strlen(Call) > 10)			// Node sending duff report
			strlop(Call, ' ');

		if (Flags == NULL)
			return;				// Corrupt

		heardCall = FindNode(Call);

		if (heardCall->LastHeard == 0)	// First time, so set onlyHeard
		{
			// Maybe also add position if LOC supplied

			Node->onlyHeard = 1;
		}

		if (LOC[0] && heardCall->Lat == 0.0 && heardCall->Lon == 0.0)
		{
			double Lat, Lon;

			if (FromLOC(LOC, &Lat, &Lon))
			{
				heardCall->Lat = Lat;
				heardCall->Lon = Lon;
			}
		}

		Node->LastHeard = Now;

		UpdateHeardData(Node, heardCall, Freq, LOC, Flags);
		return;
	}

	if (memcmp(Msg, "LINK ", 3) == 0)
	{
		// GM8BPQ-2 DUMMY-1 LINK LU1HVK-4,16,YU4ZED-4,16,AE5E-14,16,

		char * Call;
		int Type;
		struct NodeLink * Link;

		ptr = strtok_s(Msg, " ", &Context);

		while (Context && Context[0])
		{
			Call = strtok_s(NULL, ",", &Context);
			ptr = strtok_s(NULL, ",", &Context);

			if (ptr == 0)
				break;

			Type = atoi(ptr);

			Link = FindLink(From, Call, Type);

			Link->LastHeard = Now;
		}

		return;
	}

	if (memcmp(Msg, "MODE ", 5) == 0)
	{
		char * pPort, * pType, * pInterlock, * pFreq;
		int Port;
		int Type;
		int Interlock;
		struct ModeEntries * Modes;
		struct ModeItem * Mode;
		char * nextBit = strlop(Msg, '/');

		Msg += 5;

		while (strlen(Msg) > 3)
		{
			pPort = Msg;
			pType = strlop(pPort, ',');
			pInterlock = strlop(pType, ',');
			pFreq = strlop(pInterlock, ',');

			if (pInterlock == NULL)
				return;

			Port = atoi(pPort);
			Type = atoi(pType);
			Interlock = atoi(pInterlock);

			if (Port == 0)
				return;

			if (pFreq)
				printf("%s %d %s %d %s\n", From, Port, ModeNames[Type], Interlock, pFreq);
			else
				printf("%s %d %s %d\n", From, Port, ModeNames[Type], Interlock);

			Port--;				// Index from zero

			Modes = Node->Modes;

			if (Modes == NULL)
				Modes = Node->Modes = (struct ModeEntries *)zalloc(sizeof(struct ModeEntries));

			Mode = Modes->Mode[Port];

			if (Mode == NULL)
				Mode = Modes->Mode[Port] = (struct ModeItem *)zalloc(sizeof(struct ModeItem));

			Mode->LastHeard = Now;
			Mode->Mode = Type;
			Mode->Interlock = Interlock;
			if (pFreq)
				Mode->Freq = atof(pFreq);

			Msg = nextBit;
			nextBit = strlop(Msg, '/');
		}
		return;
	}

	if (memcmp(Msg, "FREQ ", 5) == 0)
	{
		int Interlock = 0;
		double Freq;
		char TimeBand[16];
		char * nextBit;
		char * pInterlock, * pTime, *pFreq;
		struct FreqEntries * Freqs;
		struct FreqItem * FreqItem;
		int n;
		char Line[16384];
		int lineLen = 0;
		char freqString[256];

		printf("%s %s\r\n", From, Msg);

		if (strcmp("GM8BPQ-2", From) == 0)
			n = 0;

		nextBit = strlop(Msg, '|');

		Msg += 5;

		while (Msg && Msg[0])
		{
			char * nextTime;

			lineLen = 0;

			pInterlock = Msg;
			pTime = strlop(pInterlock, '/');

			Interlock = atoi(pInterlock);	

			if (Interlock < 0)
			{
				// Try to convert to new format report

				int Port = -Interlock;
				struct ModeItem * Mode;
				char * FreqPtr = strlop(pTime, '/');
				
				if (Node->Modes)
				{
					Mode = Node->Modes->Mode[Port - 1];

					if (Mode && FreqPtr)
						Mode->Freq = atof(FreqPtr) / 1000000.0;
				}

				Msg = nextBit;
				nextBit = strlop(Msg, '|');
				continue;
			}

			if (Node->Freqs == NULL)
				Node->Freqs = (struct FreqEntries *)zalloc(sizeof(struct FreqEntries));

			Freqs = Node->Freqs;

			// Do we have a record for this group?

			for (n = 0; n < 32; n++)
			{
				FreqItem = Node->Freqs->Freq[n];

				if (FreqItem == NULL || FreqItem->Interlock == Interlock)
					break;
			}

			// We have either found it or first free slot

			if (FreqItem == NULL)			// Not found
			{
				FreqItem = Node->Freqs->Freq[n] = (struct FreqItem *)zalloc(sizeof(struct FreqItem));
				FreqItem->Interlock = Interlock;
			}

			FreqItem->LastHeard = Now;

			nextTime = strlop(pTime, '\\');

			while (pTime && pTime[0])
			{
				pFreq = strlop(pTime, '/');

				if (pTime)
				{
					strcpy(TimeBand, pTime);

					lineLen += sprintf(&Line[lineLen], "%s ", TimeBand);

					while (pFreq && pFreq[0])
					{
						Freq = atof(pFreq);
						pFreq = strlop(pFreq, ',');
						sprintf(freqString, "%.6f", Freq/1000000.0);

						while (freqString[strlen(freqString) - 1] == '0')
							freqString[strlen(freqString) - 1] = 0;

						lineLen += sprintf(&Line[lineLen], "%s ", freqString);
					}

					lineLen += sprintf(&Line[lineLen], "/ ");
				}

				pTime = nextTime;
				nextTime = strlop(pTime, '\\');
			}

			if (FreqItem->Freqs)
				free (FreqItem->Freqs);

			Line[strlen(Line) -2] = 0;

			FreqItem->Freqs = _strdup(Line);

			Msg = nextBit;
			nextBit = strlop(Msg, '|');
		}

		return;
	}

	// Node Info Message

	// Location Space Version<br>Comment. Location and version may contain spaces!

	// There is always <br> between Version and Comment

	Comment = strstr(Msg, "<br>");

	if (Comment == 0)
		return;			// Corrupt

	*(Comment)= 0;
	Comment += 4;

	// We now have Location and Version, both of which may contain spaces

	// Actually, looks like node always reformats lat and lon with colon but no spaces

	ptr = strtok_s(Msg, " ,:", &Context);

	if ((strlen(ptr) == 6 || strlen(ptr) == 10) && FromLOC(ptr, &Lat, &Lon))	// Valid Locator
	{
		// Rest must be version

		Version = Context;
	}
	else
	{
		// See if Space Comma or Colon Separated Lat Lon

		char * ptr2 = strtok_s(NULL, " ,:", &Context);

		if (ptr2 == NULL)
			return;			// Invalid

		Lat = atof(ptr);
		Lon = atof(ptr2);

		if (Lat == 0.0 || Lon == 0.0)
			return;					// Invalid

		if (Lat >  90.0 || Lon > 180.0 || Lat <  -90.0 || Lon < -180.0)
			return;					// Invalid

		// Rest should be version

		Version = Context;
	}

	Node->Lat = Lat;
	Node->Lon = Lon;

	// Check Comment for Embedded | which will mess up javascript

	while (ptr = strchr(Comment, '|'))
		*ptr = '/';

	sprintf(Node->Comment, "%s %s<br>%s", From, Version, Comment);
	return;
}


int ConvFromAX25(unsigned char * incall, char * outcall)
{
	int in,out=0;
	unsigned char chr;

	memset(outcall,0x20,10);

	for (in=0;in<6;in++)
	{
		chr=incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr = incall[6];                          // ssid

	if (chr == 0x42)
	{
		outcall[out++]='-';
		outcall[out++]='T';
		return out;
	}

	if (chr == 0x44)
	{
		outcall[out++]='-';
		outcall[out++]='R';
		return out;
	}

	chr >>= 1;
	chr     &= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}
	return (out);

}
void UpdateHeardData(struct NodeData * HeardBy, struct NodeData * Heard, char * Freq, char * LOC, char * Flags)
{
	// I think the logic should be to keep only the latest record for any
	// Call, Reporting Call Pair, Freq, Flags combination

	// I think I'll just store that and postprocess to link to nodes when NodeStatus.txt is generated

	// No, on reflection better to chain heard and heard by records off the Node record
	// Is a linked list best? Maybe, as we'll have to follow the list anyway to see if
	// we aleady have it. Will also remove old entries from front


	struct HeardItem * Item = HeardBy->Heard;
	struct HeardItem * lastItem = Item;
	struct HeardItem * NewItem;

	while (Item)
	{
		if (Item->HeardCall == Heard && strcmp(Item->Freq, Freq) == 0 && strcmp(Item->Flags, Flags) == 0)
		{
			// Already have it - just update time

			Item->Time = Now;
			return;
		}

		lastItem = Item;
		Item = Item->Next;
	}

	// Not found - add

	NewItem = malloc(sizeof(struct HeardItem));
	memset(NewItem, 0, sizeof(struct HeardItem));

	NewItem->Time = Now;
	NewItem->ReportingCall = HeardBy;
	NewItem->HeardCall = Heard;
	strcpy(NewItem->Freq, Freq);
	strcpy(NewItem->Flags, Flags);
	NewItem->Next = 0;
	NewItem->NextBy = 0;

	if (HeardBy->Heard == NULL)		// First
		HeardBy->Heard = NewItem;
	else
		lastItem->Next = NewItem;

	// Now add to heardby

	// Why do we have two copies? 
	// Shouldn't there just be one record chained to the hearing node as heard and the heard node as heardby
	// Which I think means two chains

	// If we only have one copy, then if we find and update on heard, it will also exist on heardby


	Item = Heard->HeardBy;

	if (Item == NULL)
	{
		Heard->HeardBy = NewItem;
		return;
	}

	// Find end of chain

	while (Item->NextBy)
		Item = Item->NextBy;
	
	Item->NextBy = NewItem;
	return;


	/*
	// Not found - add

	// Why do we have two copies? 
	// Shouldn't there just be one record chained to the hearing node as heard and the heard node as heardby
	// Which I think means two chains


	Item = malloc(sizeof(struct HeardItem));

	Item->Time = Now;
	Item->ReportingCall = HeardBy;
	Item->HeardCall = Heard;
	strcpy(Item->Freq, Freq);
	strcpy(Item->Flags, Flags);
	Item->Next = 0;

	if (Heard->HeardBy == NULL)		// First
		Heard->HeardBy = Item;
	else
		lastItem->Next = Item;
*/
}


/*
5/9/2021 2:36:18 PM - 618 Active Nodes
|N3HYM-5,-77.5036,39.42477,greenmarker.png,0,N3HYM-5 Ver 6.0.21.24 <br>BBS N3HYM&#44;CHAT N3HYM-1&#44;NODE 
|RSNOD,27.57229,61.46665,redmarker.png,0,RSNOD KP31SL Ver 6.0.12.1 <br>APRS on 27.235 ,0,
|Link,GM8BPQ-2,PE1RRR-7,58.476,-6.211,51.62416,4.950115,green
|Link,GM8BPQ-2,G8BPQ-2,58.476,-6.211,52.983812,-1.11646,red,
|
*/

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char reportHeader[] = 
		"<html><body><h1>Chat Configuration Report</h1>"
		"This lists the status of all known Chat nodes. This includes all nodes sending"
		" reports and all nodes which a reporting node is configured to link to. "
		"The latter may not exist if there is a configuration error. Stations that have no configured"
		" links are listed, though this isn't necessarily an error.<br><br>";


void GenerateOutputFiles(time_t Now)
{
	struct tm * TM;
	struct NodeData * Node;
	struct NodeLink * Link;
	int i, j;
	FILE * pFile;
	int activeNodes = 0;
	char HeardString[16384];
	char HeardCalls[16384];
	char ModesString[16384];
	char FreqsString[16384];
	int ModesLen;
	int FreqsLen;
	double Lat, Lon;
	char Colour, State;
	struct ChatNodeData * ChatNode;
	struct ChatLink * ChatLink;
	FILE * errFile;
	int noPosition;
	int Age;

	int hl = 0;
	struct HeardItem * Heard;
	char * startofHeardBy;

	LastUpdate = Now;

	fclose(logFile);
	logFile = fopen("nodelog.txt","ab");

	pFile = fopen("nodestatus.txt","wb");

	if (pFile == NULL)
		return;

	TM = gmtime(&Now);

	// Count active nodes

	for (i = 0; i < NumberOfNodes; i++)
	{
		if ((Now - Nodes[i]->LastHeard) < 3600)			//  Only count green stations
			activeNodes++;
	}

	fprintf(pFile, "%04d/%02d/%.2d %02d:%02d:%02d - %d Active Nodes\r\n|",
		TM->tm_year + 1900, TM->tm_mon + 1, TM->tm_mday, TM->tm_hour,
		TM->tm_min, TM->tm_sec, activeNodes);

	for (i = 0; i < NumberOfNodes; i++)
	{
		Node = Nodes[i];

		Age = (int)(Now - Node->LastHeard);

		if (strcmp("KF4YMC-3", Node->Call) == 0)
			i = i;


		//		if (Node->onlyHeard)
		//			continue;					// Only heard, not reported

		// We need unheard nodes in the file for the MH list. Display on main map in blue

		if (Age < 3600) 
			Colour = 'G';
		else if (Age < 86400)
			Colour ='R';
		else
		{
			if (Node->Lat != 0.0 && Node->Lon != 0.0 && Node->LastHeard == 0) 
				Colour = 'B';
			else
				Colour ='0';
		}

		HeardString[0] = 0;
		hl = 0;
		HeardCalls[0] = 0;

		hl = sprintf(&HeardString[0], "%s<br>", Node->Call);

		if (Node->Heard || Node->HeardBy)
		{
			Heard = Node->Heard;

			hl += sprintf(&HeardString[hl], "Heard<br>");

			while (Heard)
			{
				if ((Now - Heard->Time) < 4 * 86400)  // Less than 4 days old
				{
					TM = gmtime(&Heard->Time);

					strcat(HeardCalls, Heard->HeardCall->Call);
					strcat(HeardCalls, "%");
					hl += sprintf(&HeardString[hl], "%04d/%02d/%.2d %02d:%02d:%02d %s %s %s<br>",
						TM->tm_year + 1900, TM->tm_mon + 1, TM->tm_mday, TM->tm_hour,
						TM->tm_min, TM->tm_sec, Heard->HeardCall, Heard->Freq, Heard->Flags);
				}
				Heard = Heard->Next;
			}

			startofHeardBy = &HeardString[hl];

			hl += sprintf(&HeardString[hl], "Heard By<br>");

			Heard = Node->HeardBy;

			while (Heard)
			{
				if ((Now - Heard->Time) < 4 * 86400)  // Less than 4 days old
				{
					if (strstr(startofHeardBy, Heard->ReportingCall->Call))
					{
						Heard = Heard->NextBy;
						continue;		// only a call add once
					}
					strcat(HeardCalls, Heard->ReportingCall->Call);
					strcat(HeardCalls, "%");
					hl += sprintf(&HeardString[hl], "%s<br>", Heard->ReportingCall);
				}
				Heard = Heard->NextBy;
			}
		}

		// Build Mode List

		ModesString[0] = 0;
		ModesLen = 0;

		if (Node->Modes)
		{
			struct ModeEntries * Modes = Node->Modes;

			for (j = 0; j < 32; j++)
			{
				if (Modes->Mode[j])
				{
					struct ModeItem * Mode = Modes->Mode[j];

					if (Now - Mode->LastHeard < 2400)
					{
						if (Mode->Freq)
						{
							char freqString[256];
							int freqLen = sprintf(freqString, "%.6f", Mode->Freq);

							while (freqString[--freqLen] == '0')
							{
								freqString[freqLen] = 0;
							}

							ModesLen += sprintf(&ModesString[ModesLen], "%02d:%d:%s/", Mode->Mode, Mode->Interlock, freqString);
						}
						else
							ModesLen += sprintf(&ModesString[ModesLen], "%02d:%d/", Mode->Mode, Mode->Interlock);
					}
				}
			}
		}

		// Build Freq List

		FreqsString[0] = 0;
		FreqsLen = 0;

		for (j = 0; j < 32; j++)
		{
			if (Node->Freqs)
			{
				struct FreqEntries * Freqs = Node->Freqs;

				if (Freqs->Freq[j])
				{
					struct FreqItem * Freq = Freqs->Freq[j];

					FreqsLen += sprintf(&FreqsString[FreqsLen], "%02d!%s\\", Freq->Interlock, &Freq->Freqs[6]);
				}
				else
					break;
			}
		}

		Lat = Node->Lat;
		Lon = Node->Lon;

		if (Lat == 0.0 && Lon == 0.0)
		{
			struct NodeData * x = FindBaseCall(Node->Call);

			if (x)
			{
				Lat = x->Lat;
				Lon = x->Lon;
			}
		}
		fprintf(pFile, "%s,%f,%f,%c,%d,%s,%d,%s,%s,%s,%s\r\n|",
			Node->Call, Lon, Lat, Colour, Node->PopupMode, Node->Comment, (HeardCalls[0] != 0), HeardString, HeardCalls, ModesString, FreqsString); 
		
//		printf("%s,%f,%f,%c,%d,%s,%d,%s,%s,%s,%s\r\n|",
//			Node->Call, Lon, Lat, Colour, Node->PopupMode, Node->Comment, (HeardCalls[0] != 0), HeardString, HeardCalls, ModesString, FreqsString); 	
	}

	for (i = 0; i < NumberOfNodeLinks; i++)
	{		
		Link = NodeLinks[i];

		if ((Now - Link->LastHeard) > 86400)			//  > One day old - don't show
			continue;

		if ((Link->Call1->Lat == 0.0 && Link->Call1->Lon == 0.0) || (Link->Call2->Lat == 0.0 && Link->Call2->Lon == 0.0))
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,red,\r\n|",
			Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
		else if (Link->Type == 16)
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,green,\r\n|",
			Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
		else 
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,blue,\r\n|",
			Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
	}
	fclose(pFile);

	// Generate Chat File

	//	Sort Calls for error report

	qsort((void *)ChatNodes, NumberOfChatNodes, sizeof(void *), CompareChatCalls);

	pFile = fopen("status.txt","wb");

	if (pFile == NULL)
		return;

	errFile = fopen("ChatErrors.html","wb");

	if (errFile == NULL)
		return;

	fprintf(errFile, reportHeader);

	activeNodes = 0;

	// Count active nodes

	for (i = 0; i < NumberOfChatNodes; i++)
	{
		if ((Now - ChatNodes[i]->LastHeard) < 3600)			//  Only current
			activeNodes++;
	}

	TM = gmtime(&Now);

	fprintf(pFile, "%04d/%02d/%.2d %02d:%02d:%02d - %d Active Nodes\r\n|",
		TM->tm_year + 1900, TM->tm_mon + 1, TM->tm_mday, TM->tm_hour,
		TM->tm_min, TM->tm_sec, activeNodes);

	for (i = 0; i < NumberOfChatNodes; i++)
	{
		ChatNode = ChatNodes[i];
		noPosition = 0;
		Age = Now - ChatNode->LastHeard;

		if (strcmp(ChatNode->Call, "KG4FZR-1") == 0)
			i = i;

		// |GM8BPQ-4,-6.21863670150439,58.4901567374667,GM8BPQ.ok.png,0,BPQ Chat Node, Skigersta, |

		// derivedLat/Lon will be overwritten if a real report is received, so if that
		// is set use it

		if (Age > 86400 && ChatNode->LastHeard != 0)
			continue;					

		Lat = ChatNode->Lat;
		Lon = ChatNode->Lon;

		// If we don't have a real lat/lon, try to find one from other ssid's of call.
		// If still not found generate randomised position around 0, 0

		if (Lat == 0.0 && Lon == 0.0)
		{
			struct NodeData * x = FindBaseCall(ChatNode->Call);

			noPosition = 1;

			if (x)
			{
				Lat = x->Lat;
				Lon = x->Lon;
				ChatNode->Lat = Lat;
				ChatNode->Lon = Lon;
				ChatNode->derivedLat = 	Lat;
				ChatNode->derivedLon = 	Lon;
				ChatNode->derivedPosn = 1;
				noPosition = 0;
			}

			if (Lat == 0.0 && Lon == 0.0)
			{
				// Radmomise unknown posn around 0, 0

				Lat += (rand() * 0.1) / RAND_MAX;
				Lon += (rand() * 0.1) / RAND_MAX;
				ChatNode->derivedLat = 	Lat;
				ChatNode->derivedLon = 	Lon;
			}
		}

		// Try with error file as a status file, so include all calls and
		// list after any misconfigured links

		fprintf(errFile, "<b>%s</b>", ChatNode->Call);

		if (noPosition)
			fprintf(errFile, " Unknown position");

	
		if (ChatNode->LastHeard == 0)
 			fprintf(errFile, " Not Reporting");
					
		if (ChatNode->hasLinks == 0)
 			fprintf(errFile, " No Links");
		else
		{
			// Check through links for any mismatches

			struct ChatLink * Link;
			char * Call = ChatNode->Call;
			char * OtherCall;
			int reported = 0;
			
			int i;

			for (i = 0; i < NumberOfChatLinks; i++)
			{
				Link = ChatLinks[i];

				if (strcmp(Link->Call1->Call, Call) == 0)
					OtherCall = Link->Call2->Call;
				else if (strcmp(Link->Call2->Call, Call) == 0)
					OtherCall = Link->Call1->Call;
				else
					continue;

				if ((Now - Link->LastHeard1) < 3600 && Link->Call1State != Link->Call2State )
				{
					if (reported == 0)
					{
						fprintf(errFile, " Mismatched Links");
						reported = 1;
					}
					fprintf(errFile, " %s", OtherCall);
				}		
			}
		}		
	
		fprintf(errFile, "<br>");

		if (ChatNode->LastHeard == 0)		// Don't display if not reporting
			continue;					

		if (ChatNode->hasLinks == 0)



		if (noPosition && ChatNode->hasLinks == 0)
		{
			continue;				// Don't display if no posn and no links
		}

		if ((Now - ChatNode->LastHeard) < 86400)
		{
			if ((Now - ChatNode->LastHeard) < 3600)
				fprintf(pFile, "%s,%f,%f,xx.ok.,%d,%s,\r\n|",
				ChatNode->Call, Lon, Lat, ChatNode->PopupMode, ChatNode->Comment);
			else
				fprintf(pFile, "%s,%f,%f,xx.down.,%d,%s,\r\n|",
				ChatNode->Call, Lon, Lat, ChatNode->PopupMode, ChatNode->Comment);
		}
	}

	for (i = 0; i < NumberOfChatLinks; i++)
	{
		ChatLink = ChatLinks[i];

		// |Line,-122.1225,47.6891666666667, -93.294332,36.620264,#000000,2

		if ((Now - ChatLink->LastHeard1) < 3600)
		{
			if (ChatLink->Call1State == 0 && ChatLink->Call2State == -1) 
				State = 3;      // Down Mismatch
			else if (ChatLink->Call1State == -1 && ChatLink->Call2State == 0)
				State = 3;
			else if (ChatLink->Call1State == 2 && ChatLink->Call2State == 2)
				State = 2;		// Up
			else if (ChatLink->Call1State == 2 || (ChatLink->Call2State == 2 && (Now - ChatLink->LastHeard2) < 3600))
				State = 1;	
			else
				State = 0;

			if (State == 1 || State == 2 || State == 0)
			{
				fprintf(pFile, "Line,%f,%f,%f,%f,%d,%s,%s\r\n|",
					ChatLink->Call1->derivedLon, ChatLink->Call1->derivedLat,
					ChatLink->Call2->derivedLon, ChatLink->Call2->derivedLat, 
					State, ChatLink->Call1->Call, ChatLink->Call2->Call);
			}
//			if (State == 1 || State == 3)
//				fprintf(errFile, "Config Mismatch %s > %s<br>", ChatLink->Call1->Call, ChatLink->Call2->Call); 

		}
	}

	fprintf(errFile, "</body></html>\r\n");

	fclose(pFile);
	fclose(errFile);


	// Save status for restart

	pFile = fopen("savenodes.txt","wb");

	if (pFile == NULL)
		return;

	for (i = 0; i < NumberOfNodes; i++)
	{
		Node = Nodes[i];

		if ((Now - Node->LastHeard) < 30 * 86400)		// Drop very old records
		{
			struct HeardItem * Heard = Node->Heard;

			fprintf(pFile, "%s,%f,%f,%d,%s|%d|%d|", Node->Call, Node->Lat, Node->Lon, Node->PopupMode, Node->Comment, Node->LastHeard, Node->onlyHeard); 

			while (Heard)
			{
				if ((Now - Heard->Time) < 4 * 86400)  // Less than 4 days old
					fprintf(pFile, "%s,%s,%s,%s,%d/", Heard->HeardCall, Heard->ReportingCall, Heard->Freq, Heard->Flags, Heard->Time);
				Heard = Heard->Next;
			}
			fprintf(pFile, "|");

			Heard = Node->HeardBy;

			while (Heard)
			{
				if ((Now - Heard->Time) < 4 * 86400)  // Less than 4 days old
					fprintf(pFile, "%s,%s,%s,%s,%d/", Heard->HeardCall, Heard->ReportingCall, Heard->Freq, Heard->Flags, Heard->Time);
				Heard = Heard->NextBy;
			}

			fprintf(pFile, "|");

			// Do Modes
	
			for (j = 0; j < 32; j++)
			{
				if (Node->Modes)
				{
					struct ModeEntries * Modes = Node->Modes;

					if (Modes->Mode[j])
					{
						struct ModeItem * Mode = Modes->Mode[j];

						if (Now - Mode->LastHeard < 2400)
						{
							if (Mode->Freq)
								fprintf(pFile, "%d,%d,%d,%lld,%f/", j, Mode->Mode, Mode->Interlock, Mode->LastHeard, Mode->Freq);
							else
								fprintf(pFile, "%d,%d,%d,%d/", j, Mode->Mode, Mode->Interlock, Mode->LastHeard);
						}
					}
				}
			}

			fprintf(pFile, "|");

			// Do Frequencies
	
			if (Node->Freqs)
			{
				struct FreqEntries * Freqs = Node->Freqs;
		
				for (j = 0; j < 32; j++)
				{
					struct FreqItem * Freq = Freqs->Freq[j];

					if (Freq)
					{
						if (Now - Freq->LastHeard < 86400)
							fprintf(pFile, "%d,%s,%d&", Freq->Interlock, Freq->Freqs, Freq->LastHeard);
						else
							break;
					}
				}
			}

			fprintf(pFile, "|\r\n");
		}
	}

	fclose(pFile);

	pFile = fopen("savelinks.txt","wb");

	if (pFile == NULL)
		return;

	for (i = 0; i < NumberOfNodeLinks; i++)
	{
		Link = NodeLinks[i];
		if ((Now - Link->LastHeard) < 86400)  // Less than a days old
			fprintf(pFile, "%s,%s,%d,%d\r\n", Link->Call1->Call, Link->Call2->Call, Link->Type, Link->LastHeard);
	}
	fclose(pFile);

	// Save Chat Info

	pFile = fopen("savechat.txt","wb");

	if (pFile == NULL)
		return;

	for (i = 0; i < NumberOfChatNodes; i++)
	{
		ChatNode = ChatNodes[i];

		if ((Now - ChatNode->LastHeard) < 30 * 86400)		// Drop very old records
			fprintf(pFile, "%s,%f,%f,%d,%s|%d|\r\n",
				ChatNode->Call, ChatNode->Lat, ChatNode->Lon, ChatNode->PopupMode, ChatNode->Comment, ChatNode->LastHeard);

	}
	fclose(pFile);

	pFile = fopen("savechatlinks.txt","wb");

	if (pFile == NULL)
		return;

	for (i = 0; i < NumberOfChatLinks; i++)
	{
		ChatLink = ChatLinks[i];

		if ((Now - ChatLink->LastHeard1) < 30 * 86400 || (Now - ChatLink->LastHeard2) < 30 * 86400)		// Drop very old records
			fprintf(pFile, "%s,%s,%d,%d,%lld,%lld\r\n",
				ChatLink->Call1->Call, ChatLink->Call2->Call, ChatLink->Call1State, ChatLink->Call2State, ChatLink->LastHeard1, ChatLink->LastHeard2);
	}
	fclose(pFile);
	fflush(NULL);
}

void ProcessChatUpdate(char * From, char * Msg)
{
	struct ChatNodeData * Node = FindChatNode(From);
	struct ChatNodeData * OtherNode;
	struct ChatLink * Link;

	char * p1, * p2, * p3, * p4, * context;
	int NewState;

	Node->LastHeard = Now;

	if (memcmp(Msg, "INFO", 4) == 0)
	{
		int i = 0;

		char * latstring;
		char * Popup;
		char * PopupMode;
		double Lat, Lon;


		//		printf("%s %s\r\n", From, Msg);

		if (memcmp(&Msg[5], "MapPosition=", 12) == 0)
			Msg += 12;

		latstring = &Msg[5];

		Popup = strlop(latstring, '|');
		PopupMode = strlop(Popup, '|');

		if (strlen(latstring) == 6)
		{
			// Most Likely a Lccator

			_strupr(latstring);

			if (FromLOC(latstring, &Lat, &Lon))
			{
				//				printf("**%s %s %f %f\r\n", From, latstring, Lat, Lon);
				goto gotPos;
			}
		}

		// Split latstring into up to 4 components

		p1 = strtok_s(latstring, " ,", &context);
		p2 = strtok_s(NULL, " ,", &context);
		p3 = strtok_s(NULL, " ,", &context);
		p4 = strtok_s(NULL, " ,", &context);

		if (p1 == 0 || p2 == 0)
		{
			return;
		}

		if (p3 == 0 || p3[0] == 0)
		{
			// Only two - Lat and Lon. May have NS or EW on end, and may be aprs format

			char * dot = strchr(p1, '.');

			if (dot && (dot - p1) == 4)
			{
				// APRS Format (probably)

				char NS, EW;
				char LatDeg[3], LonDeg[4];

				NS = p1[7];
				EW = p2[8];

				// Standard format ddmm.mmN/dddmm.mmE?

				memcpy(LatDeg, p1,2);
				LatDeg[2]=0;
				Lat = atof(LatDeg) + (atof(p1+2) / 60);

				if (NS == 'S')
					Lat = -Lat;

				memcpy(LonDeg,p2, 3);
				LonDeg[3]=0;

				Lon = atof(LonDeg) + (atof(p2+3) / 60);

				if (EW == 'W')
					Lon = -Lon;

				//				printf("APRS %s %f %f\r\n", From, Lat, Lon);
				goto gotPos;
			}

			Lat = atof(p1);
			Lon = atof(p2);

			if (Lat >  90.0 || Lon > 180.0 || Lat < -90.0 || Lon < -180.0)
			{
				printf("**%s Corrupt %s %s \r\n", From, p1, p2);
				return;
			}

			//	See if NS/EW

			if (strchr(p1, 'S'))
				Lat -= Lat;
			if (strchr(p2, 'W'))
				Lon -= Lon;

			//			printf("**%s %f %f\r\n", From, Lat, Lon);
			goto gotPos;
		}

		if (p3 && p4 && strchr(p2, 'N') || strchr(p2, 'S'))
		{
			char * min;

			// Spaces between lat and n/s, lon e/s 

			// Could be decimal degrees or deg min or seg min s

			if (strchr(p1, '\xb0'))
			{
				min = strlop(p1, '\xb0');

				Lat = atof(p1) + atof(min) /60;

				if (strchr(p2, 'S'))
					Lat -= Lat;
			}

			if (strchr(p3, '\xb0'))
			{
				min = strlop(p3, '\xb0');

				Lon = atof(p3) + atof(min) /60;

				if (strchr(p4, 'W'))
					Lon -= Lon;
			}
		}

gotPos:

		if (Lat >  90.0 || Lat < -90.0)
			return;

		Node->derivedLat = Node->Lat = Lat;
		Node->derivedLon = Node->Lon = Lon;
		Node->derivedPosn = 0;

		strcpy(Node->Comment, Popup);
		Node->PopupMode = atoi(PopupMode);

		return;
	}

	// Link Info

	// Call / State Pairs

//	printf("%s\r\n", Msg);

	p1 = strtok_s(Msg, " \r", &context);
	p2 = strtok_s(NULL, " \r", &context);

	while (p1 && p2)
	{
		//	printf("%s %s\r\n", p1, p2);

		if (strcmp(From, "KG4FZR-1") == 0 || strcmp(p1, "KG4FZR-1") == 0)
			printf("%s\r\n", Msg);
		
		NewState = atoi(p2);

		// Ignore down reports if target doesn't exist

		if (NewState != 4)					// 4 is setting up
		{
			OtherNode = FindChatNode(p1);
			Link = FindChatLink(From, p1, NewState, Now);
		}
		p1 = strtok_s(NULL, " \r", &context);
		p2 = strtok_s(NULL, " \r", &context);

	}
	return;
}
