
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <stdio.h>


struct RIGPORTINFO
{
	char IOBASE[80];
	char PTTIOBASE[80];			// Port for Hardware PTT - may be same as control port.
	int SPEED;

	HANDLE hDevice;					// COM device Handle
	int ReopenDelay;
	SOCKET remoteSock;				// Socket for use with WINMORCONROL
	struct sockaddr remoteDest;		// Dest for above
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	int CONNECTED;					// for HAMLIB
	int CONNECTING;
	int Alerted;
};

struct RIGPORTINFO PORTS[4];

char PTTCATPort[4][64] = {"", "", "", ""};
HANDLE PTTCATHandles[4] = {0, 0, 0, 0};
int RealMux[4] = {0, 0, 0, 0};		// BPQ Virtual or Real


VOID PTTCATThread();
VOID ConnecttoHAMLIB(struct RIGPORTINFO * PORT);

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;

	if (buf == NULL) return NULL;		// Protect

	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}


void SendHamLib(struct RIGPORTINFO * PORT, int PTTState)
{
	char cmd[32];
	int Len = sprintf(cmd, "T %d\n", PTTState);
	
	Len = send(PORT->remoteSock, cmd, Len, 0);	
	Len = GetLastError();

}

int DecodeHAMLIBAddr(struct RIGPORTINFO * PORT, char * ptr)
{
	// Param is IPADDR:PORT. Only Allow numeric addresses 
	
	struct sockaddr_in * destaddr = (SOCKADDR_IN *)&PORT->remoteDest;
	char * port;

	strcpy(PORT->IOBASE, ptr);

	port = strlop(ptr, ':');

	if (port == NULL)
		return 0;

	destaddr->sin_family = AF_INET;
	destaddr->sin_addr.s_addr = inet_addr(ptr);
	destaddr->sin_port = htons(atoi(port));

	return 1;
}


void HAMLIBProcessMessage(struct RIGPORTINFO * PORT)
{
	// Called from Background thread. I think we just need to read and discard

	char RXBuffer[512];

	int InputLen = recv(PORT->remoteSock, RXBuffer, 500, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;

		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;
		return;					
	}
}


int main(int argc, char ** argv)
{
	struct RIGPORTINFO * PORT;
	int n = 0;
	WSADATA WsaData;            // receives data from WSAStartup

	if (argc < 3)
	{
		printf ("Missing parameters - you need COM port and IP Address and rigctl port of BPQ, eg \r\n"
			"  WinRPRHelper com10 192.168.1.64:4532\r\n\r\n"
			"Press any key to exit\r\n");

		getchar();

		exit (0);
	}

	WSAStartup(MAKEWORD(2, 0), &WsaData);

	while (argc > 2)
	{
		PORT = &PORTS[n];
		strcpy(PTTCATPort[n], argv[n + n +1]);
		DecodeHAMLIBAddr(PORT, argv[n + n + 2]);
		n++;
		argc -= 2;
	}

	_beginthread(PTTCATThread, 0, 0);

	for (n = 0; n < 4; n++)
	{
		if (PTTCATPort[n][0])			// Serial port RTS to HAMLIB PTT 
		{
			PORT = &PORTS[n];
			ConnecttoHAMLIB(PORT);
		}
	}

	Sleep(2000);

	printf("WinRPRHelper running - Press ctrl/c to exit\r\n");
	fflush(stdout);

	while(1)
	{
		Sleep(1000);

		for (n = 0; n < 4; n++)
		{
			if (PTTCATPort[n][0])			// Serial port RTS to HAMLIB PTT 
			{
				PORT = &PORTS[n];

				if (PORT->hDevice == 0)
				{
					// Try to reopen every 15 secs 

					PORT->ReopenDelay++;

					if (PORT->ReopenDelay > 15)
					{
						PORT->ReopenDelay = 0;

						ConnecttoHAMLIB(PORT);
					}
				}
			}
		}
	}
}
VOID PTTCATThread()
{
	DWORD dwLength = 0;
	int Length, ret, i;
	UCHAR * ptr1;
	UCHAR * ptr2;
	UCHAR c;
	UCHAR Block[4][80];
	UCHAR CurrentState[4] = {0};
#define RTS 2
#define DTR 4
	HANDLE Event;
	HANDLE Handle[4];
	DWORD EvtMask[4];
	OVERLAPPED Overlapped[4];
	char Port[32];
	int PIndex = 0;
	int HIndex = 0;
	int rc;

	while (PIndex < 4 && PTTCATPort[PIndex][0])
	{
		RealMux[HIndex] = 0;

		sprintf(Port, "\\\\.\\pipe\\BPQ%s", PTTCATPort[PIndex]);

		Handle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
                  0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

		if (Handle[HIndex] == (HANDLE) -1)
		{
			int Err = GetLastError();
			printf("PTTMUX port BPQ%s Open failed code %d - trying real com port\r\n", PTTCATPort[PIndex], Err);

			// See if real com port

			sprintf(Port, "\\\\.\\\\%s", PTTCATPort[PIndex]);

			Handle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

			RealMux[HIndex] = 1;

			if (Handle[HIndex] == (HANDLE) -1)
			{
				int Err = GetLastError();
				printf("PTTMUX port %s Open failed code %d\r\n", PTTCATPort[PIndex], Err);
			}
			else
			{
				rc = SetCommMask(Handle[HIndex], EV_CTS | EV_DSR);		// Request notifications
				HIndex++;
				printf("PTTMUX port %s Opened\r\n", PTTCATPort[PIndex]);
			}
		}
		else
			HIndex++;

		PIndex++;

	}

	if (PIndex == 0)
		return;				// No ports

	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (i = 0; i < HIndex; i ++)
	{
		memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
		Overlapped[i].hEvent = Event;

		if (RealMux[i])
		{
			// Request Interface change notifications

			rc = WaitCommEvent(Handle[i], &EvtMask[i], &Overlapped[i]);
			rc = GetLastError();
 
		}
		else
		{

			// Prime a read on each handle

			ReadFile(Handle[i], Block[i], 80, &Length, &Overlapped[i]);
		}
	}
		
	while (1)
	{

WaitAgain:

		ret = WaitForSingleObject(Event, 1000);

		if (ret == WAIT_TIMEOUT)
			goto WaitAgain;

		ResetEvent(Event);

		// See which request(s) have completed

		for (i = 0; i < HIndex; i ++)
		{
			ret =  GetOverlappedResult(Handle[i], &Overlapped[i], &Length, FALSE);

			if (ret)
			{
				if (RealMux[i])
				{
					// Request Interface change notifications

					DWORD Mask;

					GetCommModemStatus(Handle[i], &Mask);

					if (Mask & MS_CTS_ON)
						SendHamLib(&PORTS[i], TRUE);
					else
						SendHamLib(&PORTS[i], FALSE);

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;
					WaitCommEvent(Handle[i], &EvtMask[i], &Overlapped[i]);

				}
				else
				{

					ptr1 = Block[i];
					ptr2 = Block[i];

					while (Length > 0)
					{
						c = *(ptr1++);

						Length--;

						if (c == 0xff)
						{
							c = *(ptr1++);
							Length--;

							if (c == 0xff)			// ff ff means ff
							{
								Length--;
							}
							else
							{
								// This is connection / RTS/DTR statua from other end
								// Convert to CAT Command

								if (c == CurrentState[i])
									continue;

								if (c & RTS)
									SendHamLib(&PORTS[i], TRUE);
								else
									SendHamLib(&PORTS[i], FALSE);

								CurrentState[i] = c;
								continue;
							}
						}
					}

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;

					ReadFile(Handle[i], Block[i], 80, &Length, &Overlapped[i]);
				}
			}
		}
	}

}



VOID HAMLIBThread(struct RIGPORTINFO * PORT);

VOID ConnecttoHAMLIB(struct RIGPORTINFO * PORT)
{
	_beginthread(HAMLIBThread, 0, (void *)PORT);
	return ;
}

VOID HAMLIBThread(struct RIGPORTINFO * PORT)
{
	// Opens sockets and looks for data
	
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;

	if (PORT->remoteSock)
	{
		closesocket(PORT->remoteSock);
	}

	PORT->remoteSock = 0;
	PORT->remoteSock = socket(AF_INET,SOCK_STREAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for HAMLIB socket - error code = %d\r\n", WSAGetLastError());
		printf(Msg);

	 	PORT->CONNECTING = FALSE;
  	 	return; 
	}

	setsockopt(PORT->remoteSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(PORT->remoteSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	if (connect(PORT->remoteSock,(LPSOCKADDR) &PORT->remoteDest,sizeof(PORT->remoteDest)) == 0)
	{
		//
		//	Connected successful
		//

		ioctlsocket(PORT->remoteSock, FIONBIO, &param);
   		printf("Connected to HAMLIB socket Addr %s\r\n", PORT->IOBASE);
	}
	else
	{
		if (PORT->Alerted == FALSE)
		{
			struct sockaddr_in * destaddr = (SOCKADDR_IN * )&PORT->remoteDest;

			err = WSAGetLastError();

   			sprintf(Msg, "Connect Failed for HAMLIB socket - error code = %d Addr %s\r\n", err, PORT->IOBASE);

			printf(Msg);
				PORT->Alerted = TRUE;
		}
		
		closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
	 	PORT->CONNECTING = FALSE;
		return;
	}

	PORT->CONNECTED = TRUE;
	PORT->hDevice = (HANDLE)1;				// simplifies check code

	PORT->Alerted = TRUE;

	while (PORT->CONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(PORT->remoteSock,&readfs);
		FD_SET(PORT->remoteSock,&errorfs);
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		ret = select((int)PORT->remoteSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (ret == SOCKET_ERROR)
		{
			printf("HAMLIB Select failed %d r\n", WSAGetLastError());
			goto Lost;
		}

		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(PORT->remoteSock, &readfs))
			{
				HAMLIBProcessMessage(PORT);
			}

			if (FD_ISSET(PORT->remoteSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "HAMLIB Connection lost for Addr %s\r\n", PORT->IOBASE);
				printf(Msg);

				PORT->CONNECTED = FALSE;
				PORT->Alerted = FALSE;
				PORT->hDevice = 0;				// simplifies check code

				closesocket(PORT->remoteSock);
				PORT->remoteSock = 0;
				return;
			}


			continue;
		}
		else
		{
		}
	}
	sprintf(Msg, "HAMLIB Thread Terminated Addr %s\r\n", PORT->IOBASE);
	printf(Msg);
}


