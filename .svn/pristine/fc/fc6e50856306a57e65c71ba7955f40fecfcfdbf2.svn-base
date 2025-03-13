// UDPtoTCP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#ifdef WIN32
#include "winsock2.h"
#include "WS2tcpip.h"
#else
#define VOID void
#define SOCKET int 
#define BOOL int
#define TRUE 1
#define FALSE 0
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define closesocket close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#endif

#define MaxSockets 64


struct SocketConnectionInfo
{
	int Number;					// Number of record - for AGWConnections display
    SOCKET socket;
	struct sockaddr_in sin;  
	BOOL SocketActive;
 };


static struct SocketConnectionInfo Sockets[MaxSockets+1];

int CurrentConnections;

static int CurrentSockets=0;

int TCPPort = 10110;
int UDPPort = 10110;
	
SOCKET tcpsock;
SOCKET udpsock;

void Poll();
int Init();
int Socket_Accept(SOCKET SocketId);
int DataSocket_Disconnect(struct SocketConnectionInfo * sockptr);
int DataSocket_Read(struct SocketConnectionInfo * sockptr, SOCKET sock);


#ifndef WIN32


#include <pthread.h>

pthread_t _beginthread(void(*start_address)(), unsigned stack_size, VOID * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*) arglist) != 0)
		perror("New Thread");
	else
		pthread_detach(thread);

	return thread;
}

int Sleep(int ms)
{
	usleep(ms * 1000);
	return 0;
}

#endif


int main(int argc, char * argv[])
{
	if (Init() == 0)
		return 0;

	while (1)
	{
		Sleep(1000);
		Poll();
	}
	return 0;
}


VOID Poll()
{
	struct SocketConnectionInfo * sockptr;

	// Look for incoming connects

	fd_set readfd, writefd, exceptfd;
	struct timeval timeout;
	int retval;
	int n;
	int Active = 0;
	SOCKET maxsock;

	// Look for connects

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;				// poll
		
	FD_ZERO(&readfd);

	FD_SET(tcpsock, &readfd);

	retval = select((int)tcpsock + 1, &readfd, NULL, NULL, &timeout);

	if (retval == -1)
	{
		perror("listen select");
	}

	if (retval)
		if (FD_ISSET((int)tcpsock, &readfd))
			Socket_Accept(tcpsock);

	// Look for UDP Data

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;				// poll
		
	FD_ZERO(&readfd);

	FD_SET(udpsock, &readfd);

	retval = select((int)udpsock + 1, &readfd, NULL, NULL, &timeout);

	if (retval == -1)
	{
		perror("udp poll select");
	}

	if (retval)
	{
		if (FD_ISSET((int)udpsock, &readfd))
		{
			char Message[512] = "";
			struct sockaddr_in rxaddr;
			int addrlen = sizeof(struct sockaddr_in);
			int len, n;
			struct SocketConnectionInfo * sockptr;

			len = recvfrom(udpsock, Message, 512, 0, (struct sockaddr *)&rxaddr, &addrlen);

			if (len > 0)
				printf("%s", Message);

			// Send to all connected clients

			for (n = 1; n <= CurrentSockets; n++)
			{
				sockptr = &Sockets[n];
		
				if (sockptr->SocketActive)
				{
					send(sockptr->socket, Message, len, 0);
				}
			}
		}
	}


	// look for data on any active sockets

	maxsock = 0;

	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	FD_ZERO(&exceptfd);

	// Check for data on active streams
	
	for (n = 1; n <= MaxSockets; n++)
	{
		sockptr=&Sockets[n];
		
		if (sockptr->SocketActive)
		{
			SOCKET sock = sockptr->socket;

			FD_SET(sock, &readfd);
			FD_SET(sock, &exceptfd);
				
			Active++;
			if (sock > maxsock)
				maxsock = sock;		
		}
	}

	if (Active)
	{
		retval = select((int)maxsock + 1, &readfd, &writefd, &exceptfd, &timeout);

		if (retval == -1)
		{				
			perror("data select");
		}
		else
		{
			if (retval)
			{
				// see who has data

				for (n = 1; n <= MaxSockets; n++)
				{
					sockptr=&Sockets[n];
		
					if (sockptr->SocketActive)
					{
						SOCKET sock = sockptr->socket;

						if (FD_ISSET(sock, &exceptfd))
							DataSocket_Disconnect(sockptr);

						if (FD_ISSET(sock, &readfd))
							DataSocket_Read(sockptr, sock);

					}
				}
			}		
		}
	}
}


SOCKADDR_IN local_sin; 
SOCKADDR_IN sinx;

SOCKADDR_IN * psin;

int Init()
{	
	char opt = TRUE;
#ifdef WIN32	
	WSADATA WsaData;            // receives data from WSAStartup
    WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif

	if (TCPPort == 0)
		return 0;

//	Create listening socket

	tcpsock = socket( AF_INET, SOCK_STREAM, 0);

    if (tcpsock == INVALID_SOCKET)
	{
        printf("socket() failed error %d\r\n", errno);
		return 0;        
	}

	setsockopt (tcpsock, SOL_SOCKET, SO_REUSEADDR, &opt, 1);

	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;
    psin->sin_port = htons(TCPPort);        /* Convert to network ordering */

    if (bind(tcpsock, (struct sockaddr *)&local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
        printf("bind(sock) failed Port %d Error %d\r\n", TCPPort, errno);
        closesocket(tcpsock);

		return 0;
	}

	udpsock = socket(AF_INET,SOCK_DGRAM,0);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;		
	sinx.sin_port = htons(UDPPort);

	if (bind(udpsock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		//	Bind Failed

		int err = errno;
		printf("Bind Failed for UDP port %d - error code = %d\r\n", UDPPort, err);
		return 0;
	}

    if (listen(tcpsock, 5) < 0)
	{
		printf("listen(tcpsock) failed Error %d\r\n", errno);
		return 0;
	} 

	_beginthread(Poll,0,0);

	return 1;
}







int Socket_Accept(SOCKET SocketId)
{
	int n,addrlen;
	struct SocketConnectionInfo * sockptr;
	SOCKET sock;
	unsigned char work[4];

//   Find a free Socket

	for (n = 1; n <= MaxSockets; n++)
	{
		sockptr=&Sockets[n];
		
		if (sockptr->SocketActive == FALSE)
		{
			addrlen=sizeof(struct sockaddr);
		
			memset(sockptr, 0, sizeof(struct SocketConnectionInfo));

			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				printf("accept() failed Error %d\r", errno);
				return FALSE;
			}

			sockptr->socket = sock;
			sockptr->SocketActive = TRUE;
			sockptr->Number = n;

			if (CurrentSockets < n) CurrentSockets = n;  //Record max used to save searching all entries

			memcpy(work, &sockptr->sin.sin_addr.s_addr, 4);

			printf("Connected Session %d from %d.%d.%d.%d\r\n", n, work[0], work[1], work[2], work[3]);
			return 0;
		}
	}

	// Should accept, then immediately close

	return 0;
}

int DataSocket_Read(struct SocketConnectionInfo * sockptr, SOCKET sock)
{
	int i;
	char Message[512] = "";

    i = recv(sock, Message, 512, 0);

	if (i == SOCKET_ERROR || i == 0)
	{
		// Failed or closed - clear connection

		DataSocket_Disconnect(sockptr);
		return 0;
	}
    
	printf(Message);

	return 0;
}

int DataSocket_Disconnect(struct SocketConnectionInfo * sockptr)
{
	closesocket(sockptr->socket);

	sockptr->SocketActive = FALSE;

	printf("Session %d Disconnected\r\n", sockptr->Number);

	return 0;
}



