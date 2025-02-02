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

// Module to implement APRS "New Paradigm" Digipeater and APRS-IS Gateway

// First Version, November 2011

#pragma data_seg("_BPQDATA")
#define _CRT_SECURE_NO_DEPRECATE 

#include <stdio.h>
#include "cheaders.h"
#include "bpq32.h"
#include <time.h>
#include "kernelresource.h"

#include "tncinfo.h"

#include "bpqaprs.h"

#ifndef WIN32

#include <unistd.h>
#include <sys/mman.h>
#include <sys/un.h>

int sfd;
struct sockaddr_un my_addr, peer_addr;
socklen_t peer_addr_size;


#endif


#define MAXAGE 3600 * 12	  // 12 Hours
#define MAXCALLS 20			  // Max Flood, Trace and Digi
#define GATETIMELIMIT 40 * 60 // Don't gate to RF if station not heard for this time (40 mins)

static BOOL APIENTRY  GETSENDNETFRAMEADDR();
static VOID DoSecTimer();
static VOID DoMinTimer();
static int APRSProcessLine(char * buf);
static BOOL APRSReadConfigFile();
VOID APRSISThread(void * Report);
VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Consoleprintf(const char * format, ...);
BOOL APIENTRY  Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port);
VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);
int APRSDecodeFrame(char * msg, char * buffer, time_t Stamp, uint64_t Mask);		// Unsemaphored DecodeFrame
APRSHEARDRECORD * UpdateHeard(UCHAR * Call, int Port);
BOOL CheckforDups(char * Call, char * Msg, int Len);
VOID ProcessQuery(char * Query);
VOID ProcessSpecificQuery(char * Query, int Port, char * Origin, char * DestPlusDigis);
VOID CheckandDigi(DIGIMESSAGE * Msg, int Port, int FirstUnused, int Digis, int Len);		
VOID SendBeacon(int toPort, char * Msg, BOOL SendISStatus, BOOL SendSOGCOG);
Dll BOOL APIENTRY PutAPRSMessage(char * Frame, int Len);
VOID ProcessAPRSISMsg(char * APRSMsg);
static VOID SendtoDigiPorts(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);
APRSHEARDRECORD * FindStationInMH(char * call);
BOOL OpenGPSPort();
void PollGPSIn();
int CountLocalStations();
BOOL SendAPPLAPRSMessage(char * Frame);
VOID SendAPRSMessage(char * Message, int toPort);
static VOID TCPConnect(void * unuxed);
struct STATIONRECORD * DecodeAPRSISMsg(char * msg);
struct STATIONRECORD * ProcessRFFrame(char * buffer, int len, int * ourMessage);
VOID APRSSecTimer();
double myDistance(double laa, double loa, BOOL KM);
struct STATIONRECORD * FindStation(char * Call, BOOL AddIfNotFound);
int DecodeAPRSPayload(char * Payload, struct STATIONRECORD * Station);
BOOL KillOldTNC(char * Path);

BOOL ToLOC(double Lat, double Lon , char * Locator);
BOOL InternalSendAPRSMessage(char * Text, char * Call);
void UndoTransparency(char * input);
char * __cdecl Cmdprintf(TRANSPORTENTRY * Session, char * Bufferptr, const char * format, ...);
char * GetStandardPage(char * FN, int * Len);
VOID WriteMiniDump();
BOOL ProcessConfig();
int ProcessAISMessage(char * msg, int len);
int read_png(unsigned char *bytes);
VOID sendandcheck(SOCKET sock, const char * Buffer, int Len);
void SaveAPRSMessage(struct APRSMESSAGE * ptr);
void ClearSavedMessages();
void GetSavedAPRSMessages();
static VOID GPSDConnect(void * unused);
int CanPortDigi(int Port);
int FromLOC(char * Locator, double * pLat, double * pLon);

extern int SemHeldByAPI;
extern int APRSMONDECODE();
extern struct ConsoleInfo MonWindow;
extern char VersionString[];

BOOL SaveAPRSMsgs = 0;

BOOL LogAPRSIS = FALSE;

// All data should be initialised to force into shared segment

static char ConfigClassName[]="CONFIG";

extern BPQVECSTRUC * APRSMONVECPTR;

extern int MONDECODE();
extern VOID * zalloc(int len);
extern BOOL StartMinimized;

extern char TextVerstring[];

extern HWND hConsWnd;
extern HKEY REGTREE;

extern char LOCATOR[80];
extern char LOC[7];

static int SecTimer = 10;
static int MinTimer = 60;

BOOL APRSApplConnected = FALSE;  
BOOL APRSWeb = FALSE;  

void * APPL_Q = 0;				// Queue of frames for APRS Appl
void * APPLTX_Q = 0;			// Queue of frames from APRS Appl
uint64_t APRSPortMask = 0;

char APRSCall[10] = "";
char APRSDest[10] = "APBPQ1";

char WXCall[10];

UCHAR AXCall[7] = "";

char CallPadded[10] = "         ";

char GPSPort[80] = "";
int GPSSpeed = 0;
char GPSRelay[80] = "";

BOOL GateLocal = FALSE;
double GateLocalDistance = 0.0;

int MaxDigisforIS = 7;			// Dont send to IS if more digis uued to reach us

char WXFileName[MAX_PATH];
char WXComment[80];
BOOL SendWX = FALSE;
int WXInterval = 30;
int WXCounter = 29 * 60;

char APRSCall[10];
char LoppedAPRSCall[10];

BOOL WXPort[MaxBPQPortNo + 1];				// Ports to send WX to

BOOL GPSOK = 0;

char LAT[] = "0000.00N";	// in standard APRS Format      
char LON[] = "00000.00W";	//in standard APRS Format

char HostName[80];			// for BlueNMEA
int HostPort = 4352;

char GPSDHost[80];
int GPSDPort = 2947;


extern int ADSBPort;
extern char ADSBHost[];

BOOL BlueNMEAOK = FALSE;
int BlueNMEATimer = 0;

BOOL GPSDOK = FALSE;
int GPSDTimer = 0;


BOOL GPSSetsLocator = 0;	// Update Map Location from GPS

double SOG, COG;		// From GPS

double Lat = 0.0;
double Lon = 0.0;

BOOL PosnSet = FALSE;
/*
The null position should be include the \. symbol (unknown/indeterminate
position). For example, a Position Report for a station with unknown position
will contain the coordinates …0000.00N\00000.00W.…
*/
char * FloodCalls = 0;			// Calls to relay using N-n without tracing
char * TraceCalls = 0;			// Calls to relay using N-n with tracing
char * DigiCalls = 0;			// Calls for normal relaying

UCHAR FloodAX[MAXCALLS][7] = {0};
UCHAR TraceAX[MAXCALLS][7] = {0};
UCHAR DigiAX[MAXCALLS][7] = {0};

int FloodLen[MAXCALLS];
int TraceLen[MAXCALLS];
int DigiLen[MAXCALLS];

int ISPort = 0;
char ISHost[256] = "";
int ISPasscode = 0;
char NodeFilter[1000] = "m/50";		// Filter when the isn't an application
char ISFilter[1000] = "m/50";		// Current Filter
char APPLFilter[1000] = "";			// Filter when an Applcation is running

extern BOOL IGateEnabled;

char StatusMsg[256] = "";			// Must be in shared segment
int StatusMsgLen = 0;

char * BeaconPath[65] = {0};

char CrossPortMap[65][65] = {0};
char APRSBridgeMap[65][65] = {0};

UCHAR BeaconHeader[65][10][7] = {""};	//	Dest, Source and up to 8 digis 
int BeaconHddrLen[65] = {0};			// Actual Length used

UCHAR GatedHeader[65][10][7] = {""};	//	Dest, Source and up to 8 digis for messages gated from IS
int GatedHddrLen[65] = {0};			    // Actual Length used


char CFGSYMBOL = 'a';
char CFGSYMSET = 'B';

char SYMBOL = '=';						// Unknown Locaton
char SYMSET = '/';

char * PHG = 0;							// Optional PHG (Power-Height-Gain) string for beacon

BOOL TraceDigi = FALSE;					// Add Trace to packets relayed on Digi Calls
BOOL SATGate = FALSE;					// Delay Gating to IS directly heard packets
BOOL RXOnly = FALSE;					// Run as RX only IGATE, ie don't gate anything to RF

BOOL DefaultLocalTime = FALSE;
BOOL DefaultDistKM = FALSE;

int multiple = 0;						// Allows multiple copies of LinBPQ/APRS on one machine

extern BOOL needAIS;

extern unsigned long long IconData[];  // Symbols as a png image.

typedef struct _ISDELAY
{
	struct _ISDELAY * Next;
	char * ISMSG;
	time_t SendTIme;
} ISDELAY;

ISDELAY * SatISQueue = NULL;

int MaxTraceHops = 2;
int MaxFloodHops = 2;

int BeaconInterval = 0;
int MobileBeaconInterval = 0;
time_t LastMobileBeacon = 0;
int BeaconCounter = 0;
int IStatusCounter = 3600;				// Used to send ?ISTATUS? Responses
//int StatusCounter = 0;					// Used to send Status Messages

char RunProgram[128] = "";				// Program to start

BOOL APRSISOpen = FALSE;
BOOL BeacontoIS = TRUE;

int ISDelayTimer = 0;					// Time before trying to reopen APRS-IS link

char APRSDESTS[][7] = {"AIR*", "ALL*", "AP*", "BEACON", "CQ*", "GPS*", "DF*", "DGPS*", "DRILL*",
				"DX*", "ID*", "JAVA*", "MAIL*", "MICE*", "QST*", "QTH*", "RTCM*", "SKY*",
				"SPACE*", "SPC*", "SYM*", "TEL*", "TEST*", "TLM*", "WX*", "ZIP"};

UCHAR AXDESTS[30][7] = {""};
int AXDESTLEN[30] = {0};

UCHAR axTCPIP[7];
UCHAR axRFONLY[7];
UCHAR axNOGATE[7];

int MessageCount = 0;

struct PortInfo
{ 
	int Index;
	int ComPort;
	char PortType[2];
	BOOL NewVCOM;				// Using User Mode Virtual COM Driver
	int ReopenTimer;			// Retry if open failed delay
	int RTS;
	int CTS;
	int DCD;
	int DTR;
	int DSR;
	char Params[20];				// Init Params (eg 9600,n,8)
	char PortLabel[20];
	HANDLE hDevice;
	BOOL Created;
	BOOL PortEnabled;
	int FLOWCTRL;
	int gpsinptr;
#ifdef WIN32
	OVERLAPPED Overlapped;
	OVERLAPPED OverlappedRead;
#endif
	char GPSinMsg[160];
	int GPSTypeFlag;					// GPS Source flags
	BOOL RMCOnly;						// Only send RMC msgs to this port
};



struct PortInfo InPorts[1] = {0};

// Heard Station info

#define MAXHEARD 1000

int HEARDENTRIES = 0;
int MAXHEARDENTRIES = 0;
int MHLEN = sizeof(APRSHEARDRECORD);

// Area is allocated as needed

APRSHEARDRECORD MHTABLE[MAXHEARD] = {0};

APRSHEARDRECORD * MHDATA = &MHTABLE[0];

static SOCKET sock = 0;

//Duplicate suppression Code

#define MAXDUPS 100			// Number to keep
#define DUPSECONDS 28		// Time to Keep

struct DUPINFO
{
	time_t DupTime;
	int DupLen;
	char  DupUser[8];		// Call in ax.35 format
	char  DupText[100];
};

struct DUPINFO DupInfo[MAXDUPS];

struct OBJECT
{
	struct OBJECT * Next;
	UCHAR Path[10][7];		//	Dest, Source and up to 8 digis 
	int PathLen;			// Actual Length used
	char Message[81];
	char PortMap[MaxBPQPortNo + 1];
	int	Interval;
	int Timer;
};

struct OBJECT * ObjectList;		// List of objects to send;

int ObjectCount = 0;

#include <math.h>

#define M_PI       3.14159265358979323846

int RetryCount = 4;
int RetryTimer = 45;
int ExpireTime = 120;
int TrackExpireTime = 1440;
BOOL SuppressNullPosn = FALSE;
BOOL DefaultNoTracks = FALSE;

int MaxStations = 1000;

int SharedMemorySize = 0;


RECT Rect, MsgRect, StnRect;

char Key[80];

// function prototypes

VOID RefreshMessages();

// a few global variables

char APRSDir[MAX_PATH] = "BPQAPRS";
char DF[MAX_PATH];

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

int StationCount = 0;

UCHAR NextSeq = 1;

//	Stationrecords are stored in a shared memory segment. based at APRSStationMemory (normally 0x43000000)

//	A pointer to the first is placed at the start of this

struct STATIONRECORD ** StationRecords = NULL;
struct STATIONRECORD * StationRecordPool = NULL;
struct APRSMESSAGE * MessageRecordPool = NULL;

struct SharedMem * SMEM;

UCHAR * Shared;
UCHAR * StnRecordBase;

VOID SendObject(struct OBJECT * Object);
VOID MonitorAPRSIS(char * Msg, int MsgLen, BOOL TX);

#ifndef WIN32
#define WSAEWOULDBLOCK 11
#endif

HANDLE hMapFile;

// Logging

static int LogAge = 14;

#ifdef WIN32

int DeleteAPRSLogFiles()
{
   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   char File[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   LARGE_INTEGER ft;
   time_t now = time(NULL);
   int Age;

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, GetLogDirectory());
   strcat(szDir, "/logs/APRS*.log");

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
      return dwError;

   // Walk directory

   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         OutputDebugString(ffd.cFileName);
      }
      else
      {
         ft.HighPart = ffd.ftCreationTime.dwHighDateTime;
         ft.LowPart = ffd.ftCreationTime.dwLowDateTime;

		 ft.QuadPart -=  116444736000000000;
		 ft.QuadPart /= 10000000;

		 Age = (int)((now - ft.LowPart) / 86400); 

		 if (Age > LogAge)
		 {
			 sprintf(File, "%s/logs/%s%c", GetLogDirectory(), ffd.cFileName, 0);
			 Debugprintf("Deleting %s", File);
			 DeleteFile(File);
		 }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   FindClose(hFind);
   return dwError;
}

#else

#include <dirent.h>

int APRSFilter(const struct dirent * dir)
{
	return (memcmp(dir->d_name, "APRS", 4) == 0  && strstr(dir->d_name, ".log"));
}

int DeleteAPRSLogFiles()
{
	struct dirent **namelist;
    int n;
	struct stat STAT;
	time_t now = time(NULL);
	int Age = 0, res;
	char FN[256];
     	
    n = scandir("logs", &namelist, APRSFilter, alphasort);

	if (n < 0) 
		perror("scandir");
	else  
	{ 
		while(n--)
		{
			sprintf(FN, "logs/%s", namelist[n]->d_name);
			if (stat(FN, &STAT) == 0)
			{
				Age = (now - STAT.st_mtime) / 86400;
				
				if (Age > LogAge)
				{
					Debugprintf("Deleting  %s\n", FN);
					unlink(FN);
				}
			}
			free(namelist[n]);
		}
		free(namelist);
    }
	return 0;
}
#endif

int APRSWriteLog(char * msg)
{
	FILE *file;
	UCHAR Value[MAX_PATH];
	time_t T;
	struct tm * tm;

	if (LogAPRSIS == 0)
		return 0;

	if (strchr(msg, '\n') == 0)
		strcat(msg, "\r\n");

	T = time(NULL);
	tm = gmtime(&T);

	if (GetLogDirectory()[0] == 0)
	{
		strcpy(Value, "logs/APRS_");
	}
	else
	{
		strcpy(Value, GetLogDirectory());
		strcat(Value, "/");
		strcat(Value, "logs/APRS_");
	}

	sprintf(Value, "%s%02d%02d%02d.log", Value,
				tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);

	if ((file = fopen(Value, "ab")) == NULL)
		return FALSE;

	fputs(msg, file);
	fclose(file);
	return 0;
}


int ISSend(SOCKET sock, char * Msg, int Len, int flags)
{
	int Loops = 0;
	int Sent;

	MonitorAPRSIS(Msg, Len, TRUE);

	Sent = send(sock, Msg, Len, flags);

	while (Sent != Len && Loops++ < 300)					// 10 secs max
	{					
		if ((Sent == SOCKET_ERROR) && (WSAGetLastError() != WSAEWOULDBLOCK))
			return SOCKET_ERROR;
		
		if (Sent > 0)					// something sent
		{
			Len -= Sent;
			memmove(Msg, &Msg[Sent], Len);
		}		
		
		Sleep(30);
		Sent = send(sock, Msg, Len, flags);
	}

	return Sent;
}

void * endofStations;

Dll BOOL APIENTRY Init_APRS()
{
	int i;
	char * DCall;

#ifdef WIN32
	HKEY hKey=0;
	int retCode, Vallen, Type; 
#else
	int fd;
	char RX_SOCK_PATH[] = "BPQAPRSrxsock";
	char TX_SOCK_PATH[] = "BPQAPRStxsock";
	char SharedName[256];
	char * ptr1;
#endif
	struct STATIONRECORD * Stn1, * Stn2;
	struct APRSMESSAGE * Msg1, * Msg2;

	// Clear tables in case a restart

	StationRecords = NULL;

	StationCount = 0;
	HEARDENTRIES = 0;
	MAXHEARDENTRIES = 0;
	MobileBeaconInterval = 0;
	BeaconInterval = 0;

	DeleteAPRSLogFiles();

	memset(MHTABLE, 0, sizeof(MHTABLE));

	ConvToAX25(MYNODECALL, MYCALL);

	ConvToAX25("TCPIP", axTCPIP);
	ConvToAX25("RFONLY", axRFONLY);
	ConvToAX25("NOGATE", axNOGATE);

	memset(&FloodAX[0][0], 0, sizeof(FloodAX));
	memset(&TraceAX[0][0], 0, sizeof(TraceAX));
	memset(&DigiAX[0][0], 0, sizeof(DigiAX));

	APRSPortMask = 0;

	memset(BeaconPath, sizeof(BeaconPath), 0);

	memset(&CrossPortMap[0][0], 0, sizeof(CrossPortMap));
	memset(&APRSBridgeMap[0][0], 0, sizeof(APRSBridgeMap));

	for (i = 1; i <= MaxBPQPortNo; i++)
	{
		if (CanPortDigi(i))
			CrossPortMap[i][i] = TRUE;		// Set Defaults - Same Port
		CrossPortMap[i][0] = TRUE;			// and APRS-IS
	}

	PosnSet = 0;
	ObjectList = NULL;
	ObjectCount = 0;

	ISPort = ISHost[0] = ISPasscode = 0;

	if (APRSReadConfigFile() == 0)
		return FALSE;

	if (APRSCall[0] == 0)
	{
		strcpy(APRSCall, MYNODECALL);
		strlop(APRSCall, ' ');
		strcpy(LoppedAPRSCall, APRSCall);
		memcpy(CallPadded, APRSCall, (int)strlen(APRSCall));	// Call Padded to 9 chars for APRS Messaging
		ConvToAX25(APRSCall, AXCall);
	}

	if (WXCall[0] == 0)
		strcpy(WXCall, APRSCall);

	// Caluclate size of Shared Segment

	SharedMemorySize = sizeof(struct STATIONRECORD) * (MaxStations + 4) +
				sizeof(struct APRSMESSAGE) * (MAXMESSAGES + 4) + 32;	// 32 for header


#ifndef WIN32

	// Create a Shared Memory Object

	Shared = NULL;

	// Append last bit of current directory to shared name

	ptr1 = BPQDirectory;

	while (strchr(ptr1, '/'))
	{
		ptr1 = strchr(ptr1, '/');
		ptr1++;
	}

	if (multiple)
		sprintf(SharedName, "/BPQAPRSSharedMem%s", ptr1);
	else
		strcpy(SharedName, "/BPQAPRSSharedMem");

	printf("Using Shared Memory %s\n", SharedName);

#ifndef WIN32

	fd = shm_open(SharedName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		perror("Create Shared Memory");
		printf("Create APRS Shared Memory Failed\n");
	}
	else
	{
		if (ftruncate(fd, SharedMemorySize))
		{
			perror("Extend Shared Memory");
			printf("Extend APRS Shared Memory Failed\n");
		}
		else
		{
			// Map shared memory object

			Shared = mmap((void *)APRSSHAREDMEMORYBASE,
				SharedMemorySize,
			    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

			if (Shared == MAP_FAILED)
			{
				perror("Map Shared Memory");
				printf("Map APRS Shared Memory Failed\n");
				Shared = NULL;
			}

			if (Shared != (void *)APRSSHAREDMEMORYBASE)
			{
				printf("Map APRS Shared Memory Allocated at %x\n", Shared);
				Shared = NULL;
			}

		}
	}

#endif

	printf("Map APRS Shared Memory Allocated at %p\n", Shared);

	if (Shared == NULL)
	{
		printf("APRS not using shared memory\n");
		Shared = malloc(SharedMemorySize);
		printf("APRS Non-Shared Memory Allocated at %x\n", Shared);
	}

#else

#ifndef LINBPQ

	retCode = RegOpenKeyEx (REGTREE,
                "SOFTWARE\\G8BPQ\\BPQ32",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "IGateEnabled", 0, &Type, (UCHAR *)&IGateEnabled, &Vallen);
	}

#endif

	// Create Memory Mapping for Station List

	hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD)
                 SharedMemorySize,		 // maximum object size (low-order DWORD)
                 "BPQAPRSStationsMappingObject");// name of mapping object

   if (hMapFile == NULL)
   {
      Consoleprintf("Could not create file mapping object (%d).\n", GetLastError());
      return 0;
   }

   UnmapViewOfFile((void *)APRSSHAREDMEMORYBASE);


   Shared = (LPTSTR) MapViewOfFileEx(hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,
                        0,
                        SharedMemorySize,
						(void *)APRSSHAREDMEMORYBASE);

   if (Shared == NULL)
   {
	   Consoleprintf("Could not map view of file (%d).\n", GetLastError());
	   CloseHandle(hMapFile);
	   return 0;
   }

#endif

	// First record has pointer to table

	memset(Shared, 0, SharedMemorySize);

	StnRecordBase = Shared + 32;
	SMEM = (struct SharedMem *)Shared;
   
	SMEM->Version = 1;
	SMEM->SharedMemLen = SharedMemorySize;
   	SMEM->NeedRefresh = TRUE;
	SMEM->Arch = sizeof(void *);
	SMEM->SubVersion = 1;

	Stn1  = (struct STATIONRECORD *)StnRecordBase;

	StationRecords = (struct STATIONRECORD **)Stn1;
   
	Stn1++;
   
	StationRecordPool = Stn1;

	for (i = 1; i < MaxStations; i++)		// Already have first
	{	   
		Stn2 = Stn1;
		Stn2++;
		Stn1->Next = Stn2;

		Stn1 = Stn2;
	}

	Debugprintf("End of Stations %p", Stn1);
	endofStations = Stn1;

	Stn1 += 2;				// Try to fix corruption of messages.

	// Build Message Record Pool

	Msg1  = (struct APRSMESSAGE *)Stn1;

	MessageRecordPool = Msg1;

	for (i = 1; i < MAXMESSAGES; i++)		// Already have first
	{	   
		Msg2 = Msg1;
		Msg2++;
		Msg1->Next = Msg2;

		Msg1 = Msg2;
	}

	if (PosnSet == 0)
	{
		SYMBOL = '.';
		SYMSET = '\\';				// Undefined Posn Symbol
	}
	else
	{
		// Convert posn to floating degrees

		char LatDeg[3], LonDeg[4];
		memcpy(LatDeg, LAT, 2);
		LatDeg[2]=0;
		Lat=atof(LatDeg) + (atof(LAT+2)/60);
	
		if (LAT[7] == 'S') Lat=-Lat;
		
		memcpy(LonDeg, LON, 3);
		LonDeg[3]=0;
		Lon=atof(LonDeg) + (atof(LON+3)/60);
       
		if (LON[8]== 'W') Lon=-Lon;

		SYMBOL = CFGSYMBOL;
		SYMSET = CFGSYMSET;
	}

	//	First record has control info for APRS Mapping App

	Stn1  = (struct STATIONRECORD *)StnRecordBase;
	memcpy(Stn1->Callsign, APRSCall, 10);
	Stn1->Lat = Lat;
	Stn1->Lon = Lon;
	Stn1->LastPort = MaxStations;

#ifndef WIN32

	// Open unix socket for messaging app

	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sfd == -1)
	{
		perror("Socket");
	}
	else
	{
		u_long param=1;
		ioctl(sfd, FIONBIO, &param);			// Set non-blocking

		memset(&my_addr, 0, sizeof(struct sockaddr_un));
		my_addr.sun_family = AF_UNIX;
		strncpy(my_addr.sun_path, TX_SOCK_PATH, sizeof(my_addr.sun_path) - 1);
	
		memset(&peer_addr, 0, sizeof(struct sockaddr_un));
		peer_addr.sun_family = AF_UNIX;
		strncpy(peer_addr.sun_path, RX_SOCK_PATH, sizeof(peer_addr.sun_path) - 1);

		unlink(TX_SOCK_PATH);

		if (bind(sfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) == -1)
            perror("bind");
	}
#endif

	// Convert Dest ADDRS to AX.25

	for (i = 0; i < 26; i++)
	{
		DCall = &APRSDESTS[i][0];
		if (strchr(DCall, '*'))
			AXDESTLEN[i] = (int)strlen(DCall) - 1;
		else
			AXDESTLEN[i] = 6;

		ConvToAX25(DCall, &AXDESTS[i][0]);
	}

	// Process any Object Definitions

	// Setup Heard Data Area

	HEARDENTRIES = 0;
	MAXHEARDENTRIES = MAXHEARD;

	APRSMONVECPTR->HOSTAPPLFLAGS = 0x80;		// Request Monitoring

	if (ISPort && IGateEnabled)
	{
		_beginthread(APRSISThread, 0, (VOID *) TRUE);
	}

	if (GPSPort[0])
		OpenGPSPort();

	WritetoConsole("APRS Digi/Gateway Enabled\n");

	APRSWeb = TRUE;

	read_png((unsigned char *)IconData);

	// Reload saved messages

	if (SaveAPRSMsgs)
		GetSavedAPRSMessages();

	// If a Run parameter was supplied, run the program

	if (RunProgram[0] == 0)
		return TRUE;

	#ifndef WIN32
	{
		char * arg_list[] = {NULL, NULL};
		pid_t child_pid;	

		signal(SIGCHLD, SIG_IGN); // Silently (and portably) reap children. 

		//	Fork and Exec program

		printf("Trying to start %s\n", RunProgram);

		arg_list[0] = RunProgram;
	 
    	// Duplicate this process.

		child_pid = fork (); 

		if (child_pid == -1) 
 		{    				
			printf ("APRS fork() Failed\n"); 
			return 0;
		}

		if (child_pid == 0) 
 		{    				
			execvp (arg_list[0], arg_list); 
        
			// The execvp  function returns only if an error occurs.  

			printf ("Failed to run %s\n", RunProgram); 
			exit(0);			// Kill the new process
		}
	}								 
#else
	{
		int n = 0;
		
		STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
	    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
	  	SInfo.lpReserved2=NULL; 

		while (KillOldTNC(RunProgram) && n++ < 100)
		{
			Sleep(100);
		}

		if (!CreateProcess(RunProgram, NULL, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo))
			Debugprintf("Failed to Start %s Error %d ", RunProgram, GetLastError());
	}
#endif

	return TRUE;
}

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

BOOL APRSActive;

VOID APRSClose()
{
	APRSActive = FALSE;

	if (sock)
	{		
		shutdown(sock, SD_BOTH);
		Sleep(50);

		closesocket(sock);
	}
#ifdef WIN32
	if (InPorts[0].hDevice)
		CloseHandle(InPorts[0].hDevice);
#endif
}

time_t lastSecTimer = 0;


Dll VOID APIENTRY Poll_APRS()
{
	time_t Now = time(NULL);

	if (lastSecTimer != Now)
	{
		lastSecTimer = Now;

		DoSecTimer();

		MinTimer--;

		if (MinTimer == 0)
		{
			MinTimer = 60;
			DoMinTimer();
		}
	}

	if (SMEM->ClearRX)
	{
		// Clear Received Messages request from GUI

		struct APRSMESSAGE * ptr = SMEM->Messages;

		// Move Message Queue to Free Queue

		if (ptr)
		{
			while (ptr->Next)		// Find end of chain
			{
				ptr = ptr->Next;
			}

			// ptr is end of chain - chain free pool to it

			ptr->Next = MessageRecordPool;

			MessageRecordPool = SMEM->Messages;
			MessageCount = 0;
		}

		SMEM->Messages = NULL;
		SMEM->ClearRX = 0;
		SMEM->NeedRefresh = TRUE;

		ClearSavedMessages();
	}

	if (SMEM->ClearTX)
	{
		// Clear Sent Messages )request from GUI

		struct APRSMESSAGE * ptr = SMEM->OutstandingMsgs;

		// Move Message Queue to Free Queue

		if (ptr)
		{
			while (ptr->Next)		// Find end of chain
			{
				ptr = ptr->Next;
			}

			// ptr is end of chain - chain free pool to it

			ptr->Next = MessageRecordPool;

			MessageRecordPool = SMEM->OutstandingMsgs;
			MessageCount = 0;
		}

		SMEM->OutstandingMsgs = NULL;
		SMEM->ClearTX = 0;
		SMEM->NeedRefresh = TRUE;
	}
#ifdef LINBPQ
#ifndef WIN32
	{
		char Msg[256];
		int numBytes;

		// Look for messages from App

		numBytes = recvfrom(sfd, Msg, 256, 0, NULL, NULL);

		if (numBytes > 0)
		{
			InternalSendAPRSMessage(&Msg[10], &Msg[0]);
		}
	}
#endif
#endif

	if (GPSPort[0])
		PollGPSIn();

	if (APPLTX_Q)
	{
		PMSGWITHLEN buffptr = Q_REM(&APPLTX_Q);

		InternalSendAPRSMessage(&buffptr->Data[10], &buffptr->Data[0]);
		ReleaseBuffer(buffptr);
	}

	while (APRSMONVECPTR->HOSTTRACEQ)
	{
		time_t stamp;
		int len;
		BOOL MonitorNODES = FALSE;
		PMESSAGE monbuff;
		UCHAR * monchars;
		MESSAGE * Orig;
		int Digis = 0;
		MESSAGE * AdjBuff;		// Adjusted for digis
		BOOL FirstUnused = FALSE;
		int DigisUsed = 0;		// Digis used to reach us
		DIGIMESSAGE Msg = {0};
		int Port;
		unsigned char buffer[1024];
		char ISMsg[500];
		char * ptr1;
		char * Payload;
		char * ptr3;
		char * ptr4;
		BOOL ThirdParty = FALSE;
		BOOL NoGate = FALSE;
		APRSHEARDRECORD * MH;
		char MsgCopy[500];
		int toPort;
		struct STATIONRECORD * Station;
		int ourMessage = 0;
	
#ifdef WIN32
		struct _EXCEPTION_POINTERS exinfo;
		char EXCEPTMSG[80] = "";
#endif		
		monbuff = Q_REM((VOID **)&APRSMONVECPTR->HOSTTRACEQ);

		monchars = (UCHAR *)monbuff;
		AdjBuff = Orig = (MESSAGE *)monchars;	// Adjusted for digis

		Port = Orig->PORT;
		
		if (Port & 0x80)		// TX
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		if ((APRSPortMask & ((uint64_t)1 << (Port - 1))) == 0)// Port in use for APRS?
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		stamp = monbuff->Timestamp;

		if ((UCHAR)monchars[4] & 0x80)		// TX
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		// See if digipeaters present. 

		while ((AdjBuff->ORIGIN[6] & 1) == 0 && Digis < 9)
		{
			UCHAR * temp = (UCHAR *)AdjBuff;
			temp += 7;
			AdjBuff = (MESSAGE *)temp;

			// If we have already digi'ed it or if we sent it,
			// ignore (Dup Check my fail on slow links)
	
			if (AdjBuff->ORIGIN[6] & 0x80)
			{
				// Used Digi

				if (memcmp(AdjBuff->ORIGIN, AXCall, 7) == 0)
				{
					ReleaseBuffer(monbuff);
					return;
				}
				DigisUsed++;
			}
	
			if (memcmp(AdjBuff->ORIGIN, axTCPIP, 6) == 0)
				ThirdParty = TRUE;

			Digis ++;

			if (FirstUnused == FALSE && (AdjBuff->ORIGIN[6] & 0x80) == 0)
			{
				// Unused Digi - see if we should digi it

				FirstUnused = Digis;
		//		CheckDigi(buff, AdjBuff->ORIGIN);
			}
		}

		if (Digis > 8)
		{
			ReleaseBuffer(monbuff);
			continue;					// Corrupt
		}

		if (Digis)
		{
			if (memcmp(AdjBuff->ORIGIN, axNOGATE, 6) == 0 
				|| memcmp(AdjBuff->ORIGIN, axRFONLY, 6) == 0
				|| DigisUsed > MaxDigisforIS)

				// Too many digis or Last digis is NOGATE or RFONLY - dont send to IS

				NoGate = TRUE;
		}
		if (AdjBuff->CTL != 3 || AdjBuff->PID != 0xf0)				// Only UI
		{
			ReleaseBuffer(monbuff);
			continue;			
		}

		// Bridge if requested

		for (toPort = 1; toPort <= MaxBPQPortNo; toPort++)
		{
			if (APRSBridgeMap[Port][toPort])
			{
				MESSAGE * Buffer = GetBuff();
				struct PORTCONTROL * PORT;

				if (Buffer)
				{
					memcpy(Buffer, Orig, Orig->LENGTH);
					Buffer->PORT = toPort;
					PORT = GetPortTableEntryFromPortNum(toPort);

					if (PORT)
					{
						if (PORT->SmartIDInterval && PORT->SmartIDNeeded == 0)
						{
							// Using Smart ID, but none scheduled

							PORT->SmartIDNeeded = time(NULL) + PORT->SmartIDInterval;
						}
						PUT_ON_PORT_Q(PORT, Buffer);
					}
					else
						ReleaseBuffer(Buffer);
				}	
			}
		}

		// Used to check for dups here but according to "Notes to iGate developers" IS should be sent dups, and dup 
		// check only applied to digi'ing

//		if (SATGate == 0)
//		{
//			if (CheckforDups(Orig->ORIGIN, AdjBuff->L2DATA, Orig->LENGTH - Digis * 7 - (19 + sizeof(void *)))
//			{	
//				ReleaseBuffer(monbuff);
//				continue;			
//			}
//		}
		// Decode Frame to TNC2 Monitor Format

		len = APRSDecodeFrame((char *)monchars,  buffer, stamp, APRSPortMask);

		if (len == 0)
		{
			// Couldn't Decode

			ReleaseBuffer(monbuff);
			Debugprintf("APRS discarded frame - decode failed\n");
			continue;			
		}

		buffer[len] = 0;

		memcpy(MsgCopy, buffer, len);
		MsgCopy[len] = 0;

		// Do internal Decode

#ifdef WIN32

		strcpy(EXCEPTMSG, "ProcessRFFrame");

		__try 
		{

		Station = ProcessRFFrame(MsgCopy, len, &ourMessage);
		}
		#include "StdExcept.c"

		}
#else
		Station = ProcessRFFrame(MsgCopy, len, &ourMessage);
#endif

		if (Station == NULL)
		{	
			ReleaseBuffer(monbuff);
			continue;			
		}

		memcpy(MsgCopy, buffer, len);			// Process RF Frame may have changed it
		MsgCopy[len] = 0;

		buffer[len++] = 10;
		buffer[len] = 0;
		ptr1 = &buffer[10];				// Skip Timestamp
		Payload = strchr(ptr1, ':') + 2; // Start of Payload
		ptr3 = strchr(ptr1, ' ');		// End of addresses
		*ptr3 = 0;

		// We should send path to IS unchanged, so create IS
		// message before chopping path. We won't decide if 
		// we will actually send it to IS till later

		len = sprintf(ISMsg, "%s,qAR,%s:%s", ptr1, APRSCall, Payload);


		// if digis, remove any unactioned ones

		if (Digis)
		{
			ptr4 = strchr(ptr1, '*');		// Last Used Digi

			if (ptr4)
			{
				// We need header up to ptr4

				*(ptr4) = 0;
			}
			else
			{
				// No digis actioned - remove them all

				ptr4 = strchr(ptr1, ',');		// End of Dest
				if (ptr4)
					*ptr4 = 0;
			}
		}

		ptr4 = strchr(ptr1, '>');		// End of Source
		*ptr4++ = 0;

		MH = UpdateHeard(ptr1, Port);

		MH->Station = Station;

		if (ThirdParty)
		{
//			Debugprintf("Setting Igate Flag - %s", MsgCopy);
			MH->IGate = TRUE;			// if we've seen msgs to TCPIP, it must be an Igate
		}

		if (NoGate || RXOnly)
			goto NoIS;

		// I think all PID F0 UI frames go to APRS-IS,
		// Except General Queries, Frames Gated from IS to RF, and Messages Addressed to us

		// or should we process Query frames locally ??

		if (Payload[0] == '}')
			goto NoIS;

		if (Payload[0] == '?')
		{
			// General Query

			ProcessQuery(&Payload[1]);

			// ?? Should we pass addressed Queries to IS ??
	
			goto NoIS;
		}

		if (Payload[0] == ':' && memcmp(&Payload[1], CallPadded, 9) == 0)
		{
			// Message for us

			if (Payload[11] == '?')			// Only queries - the node doesnt do messaging
				ProcessSpecificQuery(&Payload[12], Port, ptr1, ptr4);

			goto NoIS;
		}

		if (APRSISOpen && CrossPortMap[Port][0])	// No point if not open
		{
			// was done above     len = sprintf(ISMsg, "%s>%s,qAR,%s:%s", ptr1, ptr4, APRSCall, Payload);

			if (BeacontoIS == 0)
			{
				// Don't send anything we've received as an echo

				char SaveCall[7];
				memcpy(SaveCall, &monbuff->ORIGIN, 7);
				SaveCall[6] &= 0x7e;	// Mask End of address bit

				if (memcmp(SaveCall, AXCall, 7) == 0)		// We sent it
				{
					// Should we check for being received via digi? - not for now 

					goto NoIS;
				}
			}

			if (SATGate && (DigisUsed == 0))
			{
				// If in Satgate mode delay directly heard to IGate

				ISDELAY * SatISEntry = malloc(sizeof(ISDELAY));
				SatISEntry->Next =	NULL;
				SatISEntry->ISMSG = _strdup(ISMsg);
				SatISEntry->SendTIme = time(NULL) + 10;	// Delay 10 seconds

				if (SatISQueue)
					SatISEntry->Next = SatISQueue;		// Chain

				SatISQueue = SatISEntry;
				goto NoIS;
			}

			ISSend(sock, ISMsg, len, 0);

			ptr1 = strchr(ISMsg, 13);
			if (ptr1) *ptr1 = 0;
//			Debugprintf(">%s", ISMsg);
		}	
	
	NoIS:

		//	We skipped DUP check for SATGate Mode, so apply it here

		// Now we don't dup check earlier so always check here

//		if (SATGate)
//		{
		if (CheckforDups(Orig->ORIGIN, AdjBuff->L2DATA, Orig->LENGTH - Digis * 7 - (19 + sizeof(void *))))
		{	
			ReleaseBuffer(monbuff);
			continue;			
		}
//		}

		// See if it is an APRS frame

		// If MIC-E, we need to process, whatever the destination

		// Now process any dest

/*
		DEST = &Orig->DEST[0];

		for (i = 0; i < 26; i++)
		{
			if (memcmp(DEST, &AXDESTS[i][0], AXDESTLEN[i]) == 0)
				goto OK;
		}

		switch(AdjBuff->L2DATA[0])
		{
			case '`':
			case 0x27:					// '
			case 0x1c:
			case 0x1d:					// MIC-E

				break;
		//	default:

				// Not to an APRS Destination
			
//				ReleaseBuffer(monbuff);
//				continue;
		}

OK:
*/
		
		// If there are unused digis, we may need to digi it.

		if (ourMessage)
		{
			// A message addressed to us, so no point in digi'ing it

			ReleaseBuffer(monbuff);
			continue;
		}

		if (Digis == 0 || FirstUnused == 0)
		{
			// No Digis, so finished

			ReleaseBuffer(monbuff);
			continue;
		}

		if (memcmp(monbuff->ORIGIN, AXCall, 7) == 0)		// We sent it
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		// Copy frame to a DIGIMessage Struct

		memcpy(&Msg, monbuff, MSGHDDRLEN + 14 + (7 * Digis));		// Header, Dest, Source, Addresses and Digis

		len = Msg.LENGTH - (MSGHDDRLEN + 14) - (7 * Digis);			// Payload Length (including CTL and PID

		memcpy(&Msg.CTL, &AdjBuff->CTL, len);

		// Pass to Digi Code

		CheckandDigi(&Msg, Port, FirstUnused, Digis, len);		// Digi if necessary		

		ReleaseBuffer(monbuff);
	}

	return;
}

VOID CheckandDigi(DIGIMESSAGE * Msg, int Port, int FirstUnused, int Digis, int Len)
{
	UCHAR * Digi = &Msg->DIGIS[--FirstUnused][0];
	UCHAR * Call;
	int Index = 0;
	int SSID;

	// Check ordinary digi first

	Call = &DigiAX[0][0];
	SSID = Digi[6] & 0x1e;

	while (*Call)
	{
		if ((memcmp(Digi, Call, 6) == 0) && ((Call[6] & 0x1e) == SSID))
		{
			// Trace Call if enabled

			if (TraceDigi)
				memcpy(Digi, AXCall, 7);
	
			// mark as used;
		
			Digi[6] |= 0x80;	// Used bit

			SendtoDigiPorts(Msg, Len, Port);
			return;
		}
		Call += 7;
		Index++;
	}

	Call = &TraceAX[0][0];
	Index = 0;

	while (*Call)
	{
		if (memcmp(Digi, Call, TraceLen[Index]) == 0)
		{
			// if possible move calls along
			// insert our call, set used
			// decrement ssid, and if zero, mark as used;

			SSID = (Digi[6] & 0x1E) >> 1;

			if (SSID == 0)	
				return;					// Shouldn't have SSID 0 for Rrace/Flood

			if (SSID > MaxTraceHops)
				SSID = MaxTraceHops;	// Enforce our limit

			SSID--;

			if (SSID ==0)				// Finihed with it ?
				Digi[6] = (SSID << 1) | 0xe0;	// Used and Fixed bits
			else
				Digi[6] = (SSID << 1) | 0x60;	// Fixed bits

			if (Digis < 8)
			{
				memmove(Digi + 7, Digi, (Digis - FirstUnused) * 7);
			}
				
			memcpy(Digi, AXCall, 7);
			Digi[6] |= 0x80;

			SendtoDigiPorts(Msg, Len, Port);

			return;
		}
		Call += 7;
		Index++;
	}

	Index = 0;
	Call = &FloodAX[0][0];

	while (*Call)
	{
		if (memcmp(Digi, Call, FloodLen[Index]) == 0)
		{
			// decrement ssid, and if zero, mark as used;

			SSID = (Digi[6] & 0x1E) >> 1;

			if (SSID == 0)	
				return;					// Shouldn't have SSID 0 for Trace/Flood

			if (SSID > MaxFloodHops)
				SSID = MaxFloodHops;	// Enforce our limit

			SSID--;

			if (SSID ==0)						// Finihed with it ?
				Digi[6] = (SSID << 1) | 0xe0;	// Used and Fixed bits
			else
				Digi[6] = (SSID << 1) | 0x60;	// Fixed bits

			SendtoDigiPorts(Msg, Len, Port);

			return;
		}
		Call += 7;
		Index++;
	}
}


  
static VOID SendtoDigiPorts(PDIGIMESSAGE Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have
	//  Len is the Payload Length (from CTL onwards)
	// The message can contain DIGIS - The payload must be copied forwards if there are less than 8

	// We send to all ports enabled in CrossPortMap

	UCHAR * EndofDigis = &Block->CTL;
	int i = 0;
	int toPort;

	while (Block->DIGIS[i][0] && i < 8)
	{
		i++;
	}

	EndofDigis = &Block->DIGIS[i][0];
	*(EndofDigis -1) |= 1;					// Set End of Address Bit

	if (i != 8)
		memmove(EndofDigis, &Block->CTL, Len);

	Len = Len + (i * 7) + 14;					// Include Source, Dest and Digis

//	Block->DEST[6] &= 0x7e;						// Clear End of Call
//	Block->ORIGIN[6] |= 1;						// Set End of Call

//	Block->CTL = 3;		//UI

	for (toPort = 1; toPort <= MaxBPQPortNo; toPort++)
	{
		if (CrossPortMap[Port][toPort])
			Send_AX((PMESSAGE)Block, Len, toPort);
	}
	return;

}

VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	//  Len is the Payload Length (CTL, PID, Data)

	// The message can contain DIGIS - The payload must be copied forwards if there are less than 8

	UCHAR * EndofDigis = &Block->CTL;

	int i = 0;

	while (Block->DIGIS[i][0] && i < 8)
	{
		i++;
	}

	EndofDigis = &Block->DIGIS[i][0];
	*(EndofDigis -1) |= 1;					// Set End of Address Bit

	if (i != 8)
		memmove(EndofDigis, &Block->CTL, Len); // Include PID

	Len = Len + (i * 7) + 14;					// Include Source, Dest and Digis

	Send_AX((PMESSAGE)Block, Len, Port);

	return;

}

static BOOL APRSReadConfigFile()
{
	char * Config;
	char * ptr1, * ptr2;

	char buf[256],errbuf[256];

	Config = PortConfig[APRSConfigSlot];		// Config from bpq32.cfg

	sprintf(StatusMsg, "BPQ32 Igate V %s", VersionString);		// Set Default Status Message

	if (Config)
	{
		// Using config from bpq32.cfg

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!APRSProcessLine(buf))
			{
				WritetoConsole("APRS Bad config record ");
				strcat(errbuf, "\r\n");
				WritetoConsole(errbuf);
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL ConvertCalls(char * DigiCalls, UCHAR * AX, int * Lens)
{
	int Index = 0;
	char * ptr;
	char * Context;
	UCHAR Work[MAXCALLS][7] = {0};
	int Len[MAXCALLS] = {0};
		
	ptr = strtok_s(DigiCalls, ", ", &Context);

	while(ptr)
	{
		if (Index == MAXCALLS) return FALSE;

		ConvToAX25(ptr, &Work[Index][0]);
		Len[Index++] = (int)strlen(ptr);
		ptr = strtok_s(NULL, ", ", &Context);
	}

	memcpy(AX, Work, sizeof(Work));
	memcpy(Lens, Len, sizeof(Len));
	return TRUE;
}



static int APRSProcessLine(char * buf)
{
	char * ptr, * p_value;

	ptr = strtok(buf, "= \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment


//	 OBJECT PATH=APRS,WIDE1-1 PORT=1,IS INTERVAL=30 TEXT=;444.80TRF*111111z4807.60N/09610.63Wr%156 R15m

	if (_stricmp(ptr, "OBJECT") == 0)
	{
		char * p_Path, * p_Port, * p_Text;
		int Interval;
		struct OBJECT * Object;
		int Digi = 2;
		char * Context;
		int SendTo;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (_stricmp(p_value, "PATH"))
			return FALSE;

		p_Path = strtok(NULL, "\t\n\r ");
		if (p_Path == NULL) return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (_stricmp(p_value, "PORT"))
			return FALSE;

		p_Port = strtok(NULL, "\t\n\r ");
		if (p_Port == NULL) return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (_stricmp(p_value, "INTERVAL"))
			return FALSE;

		p_value = strtok(NULL, " \t");
		if (p_value == NULL) return FALSE;

		Interval = atoi(p_value);

		if (Interval == 0)
			return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (_stricmp(p_value, "TEXT"))
			return FALSE;

		p_Text = strtok(NULL, "\n\r");
		if (p_Text == NULL) return FALSE;

		Object = zalloc(sizeof(struct OBJECT));

		if (Object == NULL)
			return FALSE;

		Object->Next = ObjectList;
		ObjectList = Object;

		if (Interval < 10)
			Interval = 10;

		Object->Interval = Interval;
		Object->Timer = (ObjectCount++) * 10 + 30;	// Spread them out;

		// Convert Path to AX.25 

		ConvToAX25(APRSCall, &Object->Path[1][0]);

		ptr = strtok_s(p_Path, ",\t\n\r", &Context);

		if (_stricmp(ptr, "APRS") == 0)			// First is Dest
			ConvToAX25(APRSDest, &Object->Path[0][0]);
		else if (_stricmp(ptr, "APRS-0") == 0)
			ConvToAX25("APRS", &Object->Path[0][0]);
		else
			ConvToAX25(ptr, &Object->Path[0][0]);
		
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			ConvToAX25(ptr, &Object->Path[Digi++][0]);
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		Object->PathLen = Digi * 7;

		// Process Port List

		ptr = strtok_s(p_Port, ",", &Context);

		while (ptr)
		{
			SendTo = atoi(ptr);				// this gives zero for IS
	
			if (SendTo > MaxBPQPortNo)
				return FALSE;

			Object->PortMap[SendTo] = TRUE;	
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		if (strlen(p_Text) > 80)
			p_Text[80] = 0;

		strcpy(Object->Message, p_Text);
		return TRUE;
	}

	if (_stricmp(ptr, "STATUSMSG") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		memcpy(StatusMsg, p_value, 128);	// Just in case too long
		StatusMsgLen = (int)strlen(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "WXFileName") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		strcpy(WXFileName, p_value);
		SendWX = TRUE;
		return TRUE;
	}
	if (_stricmp(ptr, "WXComment") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");

		if (p_value == NULL)
			return TRUE;
		
		if (strlen(p_value) > 79)
			p_value[80] = 0;

		strcpy(WXComment, p_value);
		return TRUE;
	}


	if (_stricmp(ptr, "ISFILTER") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		strcpy(ISFilter, p_value);
		strcpy(NodeFilter, ISFilter);
		return TRUE;
	}

	if (_stricmp(ptr, "ReplaceDigiCalls") == 0)
	{
		TraceDigi = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "Multiple") == 0)
	{
		multiple = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "SATGate") == 0)
	{
		SATGate = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "RXOnly") == 0)
	{
		RXOnly = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "DISTKM") == 0)
	{
		DefaultDistKM = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "LOCALTIME") == 0)
	{
		DefaultLocalTime = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "LOGAPRSIS") == 0)
	{
		LogAPRSIS = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "SaveAPRSMsgs") == 0)
	{
		SaveAPRSMsgs = TRUE;
		return TRUE;
	}

	p_value = strtok(NULL, " \t\n\r");

	if (p_value == NULL)
		return FALSE;

	if (_stricmp(ptr, "APRSCALL") == 0)
	{
		strcpy(APRSCall, p_value);
		strcpy(LoppedAPRSCall, p_value);
		memcpy(CallPadded, APRSCall, (int)strlen(APRSCall));	// Call Padded to 9 chars for APRS Messaging

		// Convert to ax.25

		return ConvToAX25(APRSCall, AXCall);
	}

	if (_stricmp(ptr, "WXCALL") == 0)
	{
		strcpy(WXCall, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "APRSPATH") == 0)
	{
		int Digi = 2;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

		APRSPortMask |= (uint64_t)1 << (Port - 1);

		if (Context == NULL || Context[0] == 0)
			return TRUE;					// No dest - a receive-only port

		BeaconPath[Port] = _strdup(_strupr(Context));
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		if (ptr == NULL)
			return FALSE;

		ConvToAX25(APRSCall, &BeaconHeader[Port][1][0]);

		if (_stricmp(ptr, "APRS") == 0)			// First is Dest
			ConvToAX25(APRSDest, &BeaconHeader[Port][0][0]);
		else if (_stricmp(ptr, "APRS-0") == 0)
			ConvToAX25("APRS", &BeaconHeader[Port][0][0]);
		else
			ConvToAX25(ptr, &BeaconHeader[Port][0][0]);
		
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			ConvToAX25(ptr, &BeaconHeader[Port][Digi++][0]);
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		BeaconHddrLen[Port] = Digi * 7;

		return TRUE;
	}

	if (_stricmp(ptr, "GATEDPATH") == 0)
	{
		int Digi = 2;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

//		APRSPortMask |= 1 << (Port - 1);

		if (Context == NULL || Context[0] == 0)
			return TRUE;					// No dest - a receive-only port

		BeaconPath[Port] = _strdup(_strupr(Context));
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		if (ptr == NULL)
			return FALSE;

		ConvToAX25(APRSCall, &GatedHeader[Port][1][0]);

		if (_stricmp(ptr, "APRS") == 0)			// First is Dest
			ConvToAX25(APRSDest, &GatedHeader[Port][0][0]);
		else if (_stricmp(ptr, "APRS-0") == 0)
			ConvToAX25("APRS", &GatedHeader[Port][0][0]);
		else
			ConvToAX25(ptr, &GatedHeader[Port][0][0]);
		
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			ConvToAX25(ptr, &GatedHeader[Port][Digi++][0]);
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		GatedHddrLen[Port] = Digi * 7;

		return TRUE;
	}


	if (_stricmp(ptr, "DIGIMAP") == 0)
	{
		int DigiTo;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

		// Check that port can digi (SCS Pactor can't set digi'd bit in calls)

		if (CanPortDigi(Port) == 0)
			return FALSE;


		CrossPortMap[Port][Port] = FALSE;	// Cancel Default mapping
		CrossPortMap[Port][0] = FALSE;		// Cancel Default APRSIS

		if (Context == NULL || Context[0] == 0)
			return TRUE;

		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			DigiTo = atoi(ptr);				// this gives zero for IS
	
			if (DigiTo && GetPortTableEntryFromPortNum(DigiTo) == NULL)
				return FALSE;

			CrossPortMap[Port][DigiTo] = TRUE;	
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		return TRUE;
	}
	if (_stricmp(ptr, "BRIDGE") == 0)
	{
		int DigiTo;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

		if (Context == NULL)
			return FALSE;
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			DigiTo = atoi(ptr);				// this gives zero for IS
	
			if (DigiTo > MaxBPQPortNo)
				return FALSE;

			APRSBridgeMap[Port][DigiTo] = TRUE;	
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		return TRUE;
	}


	if (_stricmp(ptr, "BeaconInterval") == 0)
	{
		BeaconInterval = atoi(p_value);

		if (BeaconInterval < 5)
			BeaconInterval = 5;

		if (BeaconInterval)
			BeaconCounter = 30;				// Send first after 30 secs

		return TRUE;
	}

	if (_stricmp(ptr, "MobileBeaconInterval") == 0)
	{
		MobileBeaconInterval = atoi(p_value) * 60;
		return TRUE;
	}
	if (_stricmp(ptr, "MobileBeaconIntervalSecs") == 0)
	{
		MobileBeaconInterval = atoi(p_value);
		if (MobileBeaconInterval < 10)
			MobileBeaconInterval = 10;

		return TRUE;
	}

	if (_stricmp(ptr, "BeacontoIS") == 0)
	{
		BeacontoIS = atoi(p_value);
		return TRUE;
	}


	if (_stricmp(ptr, "TRACECALLS") == 0)
	{
		TraceCalls = _strdup(_strupr(p_value));
		ConvertCalls(TraceCalls, &TraceAX[0][0], &TraceLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "FLOODCALLS") == 0)
	{
		FloodCalls = _strdup(_strupr(p_value));
		ConvertCalls(FloodCalls, &FloodAX[0][0], &FloodLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "DIGICALLS") == 0)
	{
		char AllCalls[1024];
		
		DigiCalls = _strdup(_strupr(p_value));
		strcpy(AllCalls, APRSCall);
		strcat(AllCalls, ",");
		strcat(AllCalls, DigiCalls);
		ConvertCalls(AllCalls, &DigiAX[0][0], &DigiLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "MaxStations") == 0)
	{
		MaxStations = atoi(p_value);

		if (MaxStations > 10000)
			MaxStations = 10000;

		return TRUE;
	}

	if (_stricmp(ptr, "MaxAge") == 0)
	{
		ExpireTime = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSPort") == 0)
	{
		if (strcmp(p_value, "0") != 0)
			strcpy(GPSPort, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSSpeed") == 0)
	{
		GPSSpeed = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSRelay") == 0)
	{
		if (strlen(p_value) > 79)
			return FALSE;

		strcpy(GPSRelay, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "BlueNMEA") == 0 || _stricmp(ptr, "TCPHost") == 0 || _stricmp(ptr, "AISHost") == 0)
	{
		if (strlen(p_value) > 70)
			return FALSE;

		strcpy(HostName, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "TCPPort") == 0  || _stricmp(ptr, "AISPort") == 0)
	{
		HostPort = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSDHost") == 0)
	{
		if (strlen(p_value) > 70)
			return FALSE;

		strcpy(GPSDHost, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSDPort") == 0)
	{
		GPSDPort = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ADSBHost") == 0)
	{
		if (strlen(p_value) > 70)
			return FALSE;

		strcpy(ADSBHost, p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ADSBPort") == 0)
	{
		ADSBPort = atoi(p_value);
		return TRUE;
	}

	

	if (_stricmp(ptr, "GPSSetsLocator") == 0)
	{
		GPSSetsLocator = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "LAT") == 0)
	{
		if (strlen(p_value) != 8)
			return FALSE;

		memcpy(LAT, _strupr(p_value), 8);
		PosnSet = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "LON") == 0)
	{
		if (strlen(p_value) != 9)
			return FALSE;

		memcpy(LON, _strupr(p_value), 9);
		PosnSet = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "SYMBOL") == 0)
	{
		if (p_value[0] > ' ' && p_value[0] < 0x7f)
			CFGSYMBOL = p_value[0];

		return TRUE;
	}

	if (_stricmp(ptr, "SYMSET") == 0)
	{
		CFGSYMSET = p_value[0];
		return TRUE;
	}

	if (_stricmp(ptr, "PHG") == 0)
	{
		PHG = _strdup(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "MaxTraceHops") == 0)
	{
		MaxTraceHops = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "MaxFloodHops") == 0)
	{
		MaxFloodHops = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ISHOST") == 0)
	{
		strncpy(ISHost, p_value, 250);
		return TRUE;
	}

	if (_stricmp(ptr, "ISPORT") == 0)
	{
		ISPort = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ISPASSCODE") == 0)
	{
		ISPasscode = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "MaxDigisforIS") == 0)
	{
		MaxDigisforIS = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GateLocalDistance") == 0)
	{
		GateLocalDistance = atoi(p_value);
		if (GateLocalDistance > 0.0)
			GateLocal = TRUE;

		return TRUE;
	}

	if (_stricmp(ptr, "WXInterval") == 0)
	{
		WXInterval = atoi(p_value);
		WXCounter = (WXInterval - 1) * 60;
		return TRUE;
	}

	if (_stricmp(ptr, "WXPortList") == 0)
	{
		char ParamCopy[80];
		char * Context;
		int Port;
		char * ptr;
		int index = 0;

		for (index = 0; index < MaxBPQPortNo; index++)
			WXPort[index] = FALSE;
	
		if (strlen(p_value) > 79)
			p_value[80] = 0;

		strcpy(ParamCopy, p_value);
		
		ptr = strtok_s(ParamCopy, " ,\t\n\r", &Context);

		while (ptr)
		{
			Port = atoi(ptr);				// this gives zero for IS
	
			WXPort[Port] = TRUE;	
			
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}
		return TRUE;
	}

	if (_stricmp(ptr, "Run") == 0)
	{
		strcpy(RunProgram, p_value);
		return TRUE;
	}

	//
	//	Bad line
	//
	return (FALSE);	
}

VOID SendAPRSMessageEx(char * Message, int toPort, char * FromCall, int Gated);


VOID SendAPRSMessage(char * Message, int toPort)
{
	SendAPRSMessageEx(Message, toPort, APRSCall, 0);
}

// Ex allows setting source call (For WX Messages)


VOID SendAPRSMessageEx(char * Message, int toPort, char * FromCall, int Gated)
{
	int Port;
	DIGIMESSAGE Msg;
	
	int Len;

	// toPort = -1 means all tadio ports. 0 = IS

	if (toPort == -1)
	{
		for (Port = 1; Port <= MaxBPQPortNo; Port++)
		{
			if (Gated && GatedHddrLen[Port])
				memcpy(Msg.DEST, &GatedHeader[Port][0][0],  10 * 7);
			else if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0],  10 * 7);
			else
				continue;

			Msg.DEST[6] |= 0x80;			// set Command Bit

			ConvToAX25(FromCall, Msg.ORIGIN);
			Msg.PID = 0xf0;
			Msg.CTL = 3;
			Len = sprintf(Msg.L2DATA, "%s", Message);
			Send_AX_Datagram(&Msg, Len + 2, Port);
		}

		return;
	}

	if (toPort == 0 && APRSISOpen)
	{
		char ISMsg[300];

		Len = sprintf(ISMsg, "%s>%s,TCPIP*:%s\r\n", FromCall, APRSDest, Message);
		ISSend(sock, ISMsg, Len, 0);
	}

	if (toPort == 0)
		return;
	
	if (Gated && GatedHddrLen[toPort])
		memcpy(Msg.DEST, &GatedHeader[toPort][0][0],  10 * 7);
	else if (BeaconHddrLen[toPort])		// Only send to ports with a DEST defined
		memcpy(Msg.DEST, &BeaconHeader[toPort][0][0],  10 * 7);
	else
		return;

	Msg.DEST[6] |= 0x80;			// set Command Bit

	ConvToAX25(FromCall, Msg.ORIGIN);
	Msg.PID = 0xf0;
	Msg.CTL = 3;
	Len = sprintf(Msg.L2DATA, "%s", Message);
	Send_AX_Datagram(&Msg, Len + 2, toPort);

	return;
}


VOID ProcessSpecificQuery(char * Query, int Port, char * Origin, char * DestPlusDigis)
{
	if (_memicmp(Query, "APRSS", 5) == 0)
	{
		char Message[255];
	
		sprintf(Message, ":%-9s:%s", Origin, StatusMsg);
		SendAPRSMessage(Message, Port);

		return;
	}

	if (_memicmp(Query, "APRST", 5) == 0 || _memicmp(Query, "PING?", 5) == 0)
	{
		// Trace Route
		//:KH2ZV   :?APRST :N8UR     :KH2Z>APRS,DIGI1,WIDE*:
		//:G8BPQ-14 :Path - G8BPQ-14>APU25N

		char Message[255];
	
		sprintf(Message, ":%-9s:Path - %s>%s", Origin, Origin, DestPlusDigis);
		SendAPRSMessage(Message, Port);

		return;
	}
}

VOID ProcessQuery(char * Query)
{
	if (memcmp(Query, "IGATE?", 6) == 0)
	{
		IStatusCounter = (rand() & 31) + 5;			// 5 - 36 secs delay
		return;
	}

	if (memcmp(Query, "APRS?", 5) == 0)
	{
		BeaconCounter = (rand() & 31) + 5;			// 5 - 36 secs delay
		return;
	}
}
Dll VOID APIENTRY APISendBeacon()
{
	BeaconCounter = 2;
}

typedef struct _BeaconParams
{
	int toPort;
	char * BeaconText;
	BOOL SendStatus;
	BOOL SendSOGCOG;
} Params;


Params BeaconParams;

void SendBeaconThread(void * Params);

VOID SendBeacon(int toPort, char * BeaconText, BOOL SendStatus, BOOL SendSOGCOG)
{
	// Send to IS if needed then start a thread to send to radio ports

	if (PosnSet == FALSE)
		return;

	if (APRSISOpen && toPort == 0 && BeacontoIS)
	{
		char SOGCOG[10] = "";
		char ISMsg[300];
		int Len;

		Debugprintf("Sending APRS Beacon to APRS-IS");

		if (SendSOGCOG | (COG != 0.0))
			sprintf(SOGCOG, "%03.0f/%03.0f", COG, SOG);
	
		if (PHG)			// Send PHG instead of SOG COG	
			Len = sprintf(ISMsg, "%s>%s,TCPIP*:%c%s%c%s%c%s%s\r\n", APRSCall, APRSDest,
				(APRSApplConnected) ? '=' : '!', LAT, SYMSET, LON, SYMBOL, PHG, BeaconText);
		else
			Len = sprintf(ISMsg, "%s>%s,TCPIP*:%c%s%c%s%c%s%s\r\n", APRSCall, APRSDest,
				(APRSApplConnected) ? '=' : '!', LAT, SYMSET, LON, SYMBOL, SOGCOG, BeaconText);

		ISSend(sock, ISMsg, Len, 0);
		Debugprintf(">%s", ISMsg);
	}

	BeaconParams.toPort = toPort;
	BeaconParams.BeaconText = BeaconText;
	BeaconParams.SendStatus = SendStatus;
	BeaconParams.SendSOGCOG = SendSOGCOG;

	_beginthread(SendBeaconThread, 0, (VOID *) &BeaconParams);
}

void SendBeaconThread(void * Param)
{
	// runs as a thread so we can sleep() between calls

	// Params are passed via a param block

	Params * BeaconParams = (Params *)Param;

	int toPort = BeaconParams->toPort;
	char * BeaconText = BeaconParams->BeaconText;
	BOOL SendStatus = BeaconParams->SendStatus;
	BOOL SendSOGCOG = BeaconParams->SendSOGCOG;

	int Port;
	DIGIMESSAGE Msg;
	int Len;
	char SOGCOG[256] = "";
	struct STATIONRECORD * Station;
	struct PORTCONTROL * PORT;
	
	if (PosnSet == FALSE)
		return;

	if (PHG)			// Send PHG instead of SOG COG	
		strcpy(SOGCOG, PHG);
	else
		if (SendSOGCOG | (COG != 0.0))
			sprintf(SOGCOG, "%03.0f/%03.0f", COG, SOG);

	BeaconCounter = BeaconInterval * 60;
	
	if (ISPort && IGateEnabled)
		Len = sprintf(Msg.L2DATA, "%c%s%c%s%c%s%s", (APRSApplConnected) ? '=' : '!',
			LAT, SYMSET, LON, SYMBOL, SOGCOG, BeaconText);
	else
		Len = sprintf(Msg.L2DATA, "%c%s%c%s%c%s%s", (APRSApplConnected) ? '=' : '!',
			LAT, SYMSET, LON, SYMBOL, SOGCOG, BeaconText);
	
	Msg.PID = 0xf0;
	Msg.CTL = 3;

	// Add to dup check list, so we won't digi it if we hear it back 
	// Should we drop it if we've sent it recently ??

	if (CheckforDups(APRSCall, Msg.L2DATA, Len - (19 + sizeof(void *))))
		return;

	// Add to our station list

	Station = FindStation(APRSCall, TRUE);

	if (Station == NULL)
		return;

		
	strcpy(Station->Path, "APBPQ1");
	strcpy(Station->LastPacket, Msg.L2DATA);
//	Station->LastPort = Port;

	DecodeAPRSPayload(Msg.L2DATA, Station);
	Station->TimeLastUpdated = time(NULL);

	if (toPort)
	{
		if (BeaconHddrLen[toPort] == 0)
			return;

		Debugprintf("Sending APRS Beacon to port %d", toPort);

		memcpy(Msg.DEST, &BeaconHeader[toPort][0][0], 10 * 7);		// Clear unused digis
		Msg.DEST[6] |= 0x80;			// set Command Bit

		GetSemaphore(&Semaphore, 12);
		Send_AX_Datagram(&Msg, Len + 2, toPort);
		FreeSemaphore(&Semaphore);

		return;
	}

	for (Port = 1; Port <= MaxBPQPortNo; Port++)	// Check all ports
	{
		if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
		{
			Debugprintf("Sending APRS Beacon to port %d", Port);

			if (ISPort && IGateEnabled)
		Len = sprintf(Msg.L2DATA, "%c%s%c%s%c%s%s", (APRSApplConnected) ? '=' : '!',
			LAT, SYMSET, LON, SYMBOL, SOGCOG, BeaconText);
			else
		Len = sprintf(Msg.L2DATA, "%c%s%c%s%c%s%s", (APRSApplConnected) ? '=' : '!',
			LAT, SYMSET, LON, SYMBOL, SOGCOG, BeaconText);
			Msg.PID = 0xf0;
			Msg.CTL = 3;

			memcpy(Msg.DEST, &BeaconHeader[Port][0][0], 10 * 7);
			Msg.DEST[6] |= 0x80;			// set Command Bit

			GetSemaphore(&Semaphore, 12);
			Send_AX_Datagram(&Msg, Len + 2, Port);
			FreeSemaphore(&Semaphore);

			// if Port has interlock set pause before next

			PORT = GetPortTableEntryFromPortNum(Port);
	
			// Just pause for all ports

//			if (PORT && PORT->PORTINTERLOCK)
				Sleep(20000);
		}
	}
	return ;
}

VOID SendObject(struct OBJECT * Object)
{
	int Port;
	DIGIMESSAGE Msg;
	int Len;
	
	//	Add to dup list in case we get it back

	CheckforDups(APRSCall, Object->Message, (int)strlen(Object->Message));

	for (Port = 1; Port <= MaxBPQPortNo; Port++)
	{
		if (Object->PortMap[Port])
		{
			Msg.PID = 0xf0;
			Msg.CTL = 3;
			Len = sprintf(Msg.L2DATA, "%s", Object->Message);
			memcpy(Msg.DEST, &Object->Path[0][0],  Object->PathLen + 1);
			Msg.DEST[6] |= 0x80;			// set Command Bit

			Send_AX_Datagram(&Msg, Len + 2, Port);
		}
	}

	// Also send to APRS-IS if connected

	if (APRSISOpen && Object->PortMap[0])
	{
		char ISMsg[300];
		Len = sprintf(ISMsg, "%s>%s,TCPIP*:%s\r\n", APRSCall, APRSDest, Object->Message);
		ISSend(sock, ISMsg, Len, 0);
	}
}


/*
VOID SendStatus(char * StatusText)
{
	int Port;
	DIGIMESSAGE Msg;
	int Len;

	if (APRSISOpen)
	{
		Msg.PID = 0xf0;
		Msg.CTL = 3;

		Len = sprintf(Msg.L2DATA, ">%s", StatusText);

		for (Port = 1; Port <= NUMBEROFPORTS; Port++)
		{
			if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
			{
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0], 10 * 7);
				Send_AX_Datagram(&Msg, Len + 2, Port);
			}
		}

		Len = sprintf(Msg.L2DATA, "%s>%s,TCPIP*:>%s\r\n", APRSCall, APRSDest, StatusText);
		ISSend(sock, Msg.L2DATA, Len, 0);
//		Debugprintf(">%s", Msg.L2DATA);
	}
}


*/
VOID SendIStatus()
{
	int Port;
	DIGIMESSAGE Msg;
	int Len;

	IStatusCounter = 3600;		// One per hour

	if (APRSISOpen && BeacontoIS && RXOnly == 0)
	{
		Msg.PID = 0xf0;
		Msg.CTL = 3;

		Len = sprintf(Msg.L2DATA, "<IGATE,MSG_CNT=%d,LOC_CNT=%d", MessageCount , CountLocalStations());

		for (Port = 1; Port <= MaxBPQPortNo; Port++)
		{
			if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
			{
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0], 10 * 7);
				Msg.DEST[6] |= 0x80;			// set Command Bit

				Send_AX_Datagram(&Msg, Len + 2, Port);
			}
		}

		Len = sprintf(Msg.L2DATA, "%s>%s,TCPIP*:<IGATE,MSG_CNT=%d,LOC_CNT=%d\r\n", APRSCall, APRSDest, MessageCount, CountLocalStations());
		ISSend(sock, Msg.L2DATA, Len, 0);
//		Debugprintf(">%s", Msg.L2DATA);
	}

}


VOID DoSecTimer()
{
	struct OBJECT * Object = ObjectList;

	while (Object)
	{
		Object->Timer--;

		if (Object->Timer == 0)
		{
			Object->Timer = 60 * Object->Interval;
			SendObject(Object);
		}
		Object = Object->Next;
	}

	//	Check SatGate Mode delay Q

	if (SatISQueue)
	{
		time_t NOW = time(NULL);
		ISDELAY * SatISEntry = SatISQueue;
		ISDELAY * Prev = NULL;

		while (SatISEntry)
		{
			if (SatISEntry->SendTIme < NOW)
			{
				// Send it

				ISSend(sock, SatISEntry->ISMSG, (int)strlen(SatISEntry->ISMSG), 0);
				free(SatISEntry->ISMSG);

				if (Prev)
					Prev->Next = SatISEntry->Next;
				else
					SatISQueue = SatISEntry->Next;

				free(SatISEntry);
				return;				// unlinkely to get 2 in sam esecond and doesn;t matter if we delay a bit more
			}

			Prev = SatISEntry;
			SatISEntry = SatISEntry->Next;
		}
	}

	if (ISPort && APRSISOpen == 0 && IGateEnabled)
	{
		ISDelayTimer++;

		if (ISDelayTimer > 60)
		{
			ISDelayTimer = 0;
			_beginthread(APRSISThread, 0, (VOID *) TRUE);
		}
	}

	if (HostName[0])
	{
		if (BlueNMEAOK == 0)
		{
			BlueNMEATimer++;
			if (BlueNMEATimer > 15)
			{
				BlueNMEATimer = 0;
				_beginthread(TCPConnect, 0, 0);
			}
		}
	}

	if (GPSDHost[0])
	{
		if (GPSDOK == 0)
		{
			GPSDTimer++;
			if (GPSDTimer > 15)
			{
				GPSDTimer = 0;
				_beginthread(GPSDConnect, 0, 0);
			}
		}
	}

	if (BeaconCounter)
	{
		BeaconCounter--;

		if (BeaconCounter == 0)
		{
			BeaconCounter = BeaconInterval * 60;
			SendBeacon(0, StatusMsg, TRUE, FALSE);
		}
	}

	if (IStatusCounter)
	{
		IStatusCounter--;

		if (IStatusCounter == 0)
		{
			SendIStatus();
		}
	}

	if (GPSOK)
	{
		GPSOK--;

		if (GPSOK == 0)
#ifdef LINBPQ
			Debugprintf("GPS Lost");
#else
			SetDlgItemText(hConsWnd, IDC_GPS, "No GPS");
#endif
	}

	APRSSecTimer();				// Code from APRS APPL
}

int CountPool()
{
	struct STATIONRECORD * ptr = StationRecordPool;
	int n = 0;

	while (ptr)
	{
		n++;
		ptr = ptr->Next;
	}
	return n;
}

static VOID DoMinTimer()
{
	struct STATIONRECORD * ptr = *StationRecords;
	struct STATIONRECORD * last = NULL;
	time_t AgeLimit = time(NULL ) - (ExpireTime * 60);
	int i = 0;

	// Remove old records

	while (ptr)
	{
		if (ptr->TimeLastUpdated < AgeLimit)
		{
			StationCount--;

			if (last)
			{
				last->Next = ptr->Next;
			
				// Put on front of free chain

				ptr->Next = StationRecordPool;
				StationRecordPool = ptr;

				ptr = last->Next;
			}
			else
			{
				// First in list
				
				*StationRecords = ptr->Next;
			
				// Put on front of free chain

				ptr->Next = StationRecordPool;
				StationRecordPool = ptr;

				if (*StationRecords)
				{
					ptr = *StationRecords;
				}
				else
				{
					ptr = NULL;
				}
			}
		}
		else
		{
			last = ptr;
			ptr = ptr->Next;
		}
	}
}

char APRSMsg[300];

int ISHostIndex = 0;
char RealISHost[256];

VOID APRSISThread(void * Report)
{
	// Receive from core server

	char Signon[500];
	unsigned char work[4];

	struct sockaddr_in sinx; 
	int addrlen=sizeof(sinx);
	struct addrinfo hints, *res = 0, *saveres;
	size_t len;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[1000];
	int InputLen = 1;		// Non-zero
	char errmsg[300];
	char * ptr;
	size_t inptr = 0;
	char APRSinMsg[1000];
	char PortString[20];
	char serv[256];

	Debugprintf("BPQ32 APRS IS Thread");
#ifndef LINBPQ
	SetDlgItemText(hConsWnd, IGATESTATE, "IGate State: Connecting");
#endif

	if (ISFilter[0])
		sprintf(Signon, "user %s pass %d vers BPQ32 %s filter %s\r\n",
			APRSCall, ISPasscode, TextVerstring, ISFilter);
	else
		sprintf(Signon, "user %s pass %d vers BPQ32 %s\r\n",
			APRSCall, ISPasscode, TextVerstring);


	sprintf(PortString, "%d", ISPort);

	// get host info, make socket, and connect it

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(ISHost, PortString, &hints, &res);

	InputLen = sprintf(errmsg, "Connecting to APRS Host %s\r\n", ISHost);
	MonitorAPRSIS(errmsg, InputLen, FALSE);

	if (!res)
	{
		err = WSAGetLastError();
		InputLen = sprintf(errmsg, "APRS IS Resolve %s Failed Error %d\r\n", ISHost, err);
		MonitorAPRSIS(errmsg, InputLen, FALSE);

		return;					// Resolve failed
	
	}

	// Step thorough the list of hosts

	saveres = res;				// Save for free

	if (res->ai_next)			// More than one
	{
		int n = ISHostIndex;

		while (n && res->ai_next)
		{
			res = res->ai_next;
			n--;
		}

		if (n)
		{
			// We have run off the end of the list

			ISHostIndex = 0;	// Back to start
			res = saveres;
		}
		else
			ISHostIndex++;

	}

	getnameinfo(res->ai_addr, (int)res->ai_addrlen, RealISHost, 256, serv, 256, 0);

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock == INVALID_SOCKET)
  	 	return; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	memcpy(work, res->ai_addr->sa_data, 4);

	Debugprintf("Trying  APRSIS Host %d.%d.%d.%d (%d) %s", work[0], work[1], work[2], work[3], ISHostIndex, RealISHost);
	
	if (connect(sock, res->ai_addr, (int)res->ai_addrlen))
	{
		err=WSAGetLastError();

		//
		//	Connect failed
		//

#ifndef LINBPQ
		MySetWindowText(GetDlgItem(hConsWnd, IGATESTATE), "IGate State: Connect Failed");
#else
		printf("APRS Igate connect failed\n");
#endif
		err=WSAGetLastError();
		InputLen = sprintf(errmsg, "Connect Failed %s af %d Error %d \r\n", RealISHost, res->ai_family, err);
		MonitorAPRSIS(errmsg, InputLen, FALSE);

		freeaddrinfo(res);
		return;
	}

	freeaddrinfo(saveres);

	APRSISOpen = TRUE;

#ifndef LINBPQ
	MySetWindowText(GetDlgItem(hConsWnd, IGATESTATE), "IGate State: Connected");
#endif

	InputLen=recv(sock, Buffer, 500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
		MonitorAPRSIS(Buffer, InputLen, FALSE);
	}

	ISSend(sock, Signon, (int)strlen(Signon), 0);
/*
	InputLen=recv(sock, Buffer, 500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
		MonitorAPRSIS(Buffer, InputLen, FALSE);
	}
 
	InputLen=recv(sock, Buffer, 500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
		MonitorAPRSIS(Buffer, InputLen, FALSE);
	}
*/
	while (InputLen > 0 && IGateEnabled)
	{
		InputLen = recv(sock, &APRSinMsg[inptr], (int)(500 - inptr), 0);

		if (InputLen > 0)
		{
			inptr += InputLen;

			ptr = memchr(APRSinMsg, 0x0a, inptr);

			while (ptr != NULL)
			{
				ptr++;									// include lf
				len = ptr-(char *)APRSinMsg;	

				inptr -= len;						// bytes left
			
				// UIView server has a null before crlf

				if (*(ptr - 3) == 0)
				{
					*(ptr - 3) = 13;
					*(ptr - 2) = 10;
					*(ptr - 1) = 0;

					len --;
				}

				if (len > 10 && len < 300)							// Ignore if way too long or too short
				{
					memcpy(&APRSMsg, APRSinMsg, len);	
					MonitorAPRSIS(APRSMsg, (int)len, FALSE);
					if (APRSMsg[len - 2] == 13)
						APRSMsg[len - 2] = 0;
					else 
						APRSMsg[len - 1] = 0;

//					Debugprintf("%s", APRSMsg);

					ProcessAPRSISMsg(APRSMsg);
				}

				if (inptr > 0)
				{
					memmove(APRSinMsg, ptr, inptr);
					ptr = memchr(APRSinMsg, 0x0a, inptr);
				}
				else
					ptr = 0;

				if (inptr < 0)
					break;
			}
		}
	}

	closesocket(sock);

	APRSISOpen = FALSE;

	Debugprintf("BPQ32 APRS IS Thread Exited");

#ifndef LINBPQ
	if (IGateEnabled)
		SetDlgItemText(hConsWnd, IGATESTATE, "IGate State: Disconnected");
	else
		SetDlgItemText(hConsWnd, IGATESTATE, "IGate State: Disabled");
#endif
	ISDelayTimer = 30;		// Retry pretty quickly
	return;
}

VOID ProcessAPRSISMsg(char * APRSMsg)
{
	char * Payload;
	char * Source;
	char * Dest;
	char IGateCall[10] = "         ";
	char * ptr;
	char Message[255];
	PAPRSHEARDRECORD MH;
	time_t NOW = time(NULL);
	char ISCopy[1024];
	struct STATIONRECORD * Station = NULL;
#ifdef WIN32
	struct _EXCEPTION_POINTERS exinfo;
	char EXCEPTMSG[80] = "";
#endif		

	if (APRSMsg[0] == '#')		// Comment
		return;

	// if APRS Appl is atttached, queue message to it

	strcpy(ISCopy, APRSMsg);

	GetSemaphore(&Semaphore, 12);

#ifdef WIN32

	strcpy(EXCEPTMSG, "ProcessAPRSISMsg");

	__try 
	{

	Station = DecodeAPRSISMsg(ISCopy);

	}
	#include "StdExcept.c"
	Debugprintf(APRSMsg);
	}
#else
	Station = DecodeAPRSISMsg(ISCopy);
#endif

	FreeSemaphore(&Semaphore);

//}WB4APR-14>APRS,RELAY,TCPIP,G9RXG*::G3NRWVVVV:Hi Ian{001
//KE7XO-2>hg,TCPIP*,qAC,T2USASW::G8BPQ-14 :Path - G8BPQ-14>APU25N
//IGATECALL>APRS,GATEPATH}FROMCALL>TOCALL,TCPIP,IGATECALL*:original packet data
	
	Payload = strchr(APRSMsg, ':');

	// Get call of originating Igate

	ptr = Payload;

	if (Payload == NULL)
		return;

	*(Payload++) = 0;

	while (ptr[0] != ',')
		ptr--;

	ptr++;

	if (strlen(ptr) > 9)
		return;

	memcpy(IGateCall, ptr, (int)strlen(ptr));

	if (strstr(APRSMsg, ",qAS,") == 0)		// Findu generates invalid q construct
	{
		MH = FindStationInMH(IGateCall);
		if (MH)
		{
//			Debugprintf("Setting Igate Flag - %s:%s", APRSMsg, Payload);
			MH->IGate = TRUE;						// If we have seen this station on RF, set it as an Igate
		}
	}
	Source = APRSMsg;
	Dest = strchr(APRSMsg, '>');

	if (Dest == NULL)
		return;

	*(Dest++) = 0;				// Termainate Source
	ptr = strchr(Dest, ',');

	if (ptr)
		*ptr = 0;

	MH = UpdateHeard(Source, 0);

	MH->Station = Station;

	// See if we should gate to RF. 

	// Have we heard dest recently? (use the message dest (not ax.25 dest) - does this mean we only gate Messages?
	// Not if it is an Igate (it will get a copy direct)
	// Have we recently sent a message from this call - if so, we gate the next Position

/*

	From http://www.aprs-is.net/IGateDetails.aspx

	Gate message packets and associated posits to RF if all of the following are true:

	the receiving station has been heard within range within a predefined time period (range defined 
	as digi hops, distance, or both).

	the sending station has not been heard via RF within a predefined time period (packets gated
	from the Internet by other stations are excluded from this test).

	the sending station does not have TCPXX, NOGATE, or RFONLY in the header.

	the receiving station has not been heard via the Internet within a predefined time period.

	A station is said to be heard via the Internet if packets from the station contain TCPIP* or
	TCPXX* in the header or if gated (3rd-party) packets are seen on RF gated by the station
	and containing TCPIP or TCPXX in the 3rd-party header (in other words, the station is seen on RF
	as being an IGate).

*/

	if (Payload[0] == ':')		// Message
	{
		char MsgDest[10];
		APRSHEARDRECORD * STN;

		if (strlen(Payload) > 100)	// I don't think any valid APRS msgs are more than this
			return;

		memcpy(MsgDest, &Payload[1], 9);
		MsgDest[9] = 0;

		if (strcmp(MsgDest, CallPadded) == 0) // to us
			return;

		// Check that the sending station has not been heard via RF recently

		if (MH->rfPort && (NOW - MH->MHTIME) < GATETIMELIMIT)
			return;

		STN = FindStationInMH(MsgDest);

		// Shouldn't we check DUP list, in case we have digi'ed this message directly?

		if (CheckforDups(Source, Payload, (int)strlen(Payload)))
			return;

		// has the receiving station has been heard on RF and is not an IGate

		if (STN && STN->rfPort && !STN->IGate && (NOW - STN->MHTIME) < GATETIMELIMIT) 
		{
			sprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);

			GetSemaphore(&Semaphore, 12);
			SendAPRSMessageEx(Message, STN->rfPort, APRSCall, 1);		// Set gated to IS flag
			FreeSemaphore(&Semaphore);

			MessageCount++;
			MH->LASTMSG = NOW;

			return;
		}
	}

	// Not a message. If it is a position report gate if have sent a message recently

	if (Payload[0] == '!' || Payload[0] == '/' || Payload[0] == '=' || Payload[0] == '@')	// Posn Reports
	{
		if ((NOW - MH->LASTMSG) < 900 && MH->rfPort)
		{
			sprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);
	
			GetSemaphore(&Semaphore, 12);
			SendAPRSMessageEx(Message, MH->rfPort, APRSCall, 1);		// Set gated to IS flag
			FreeSemaphore(&Semaphore);

			return;
		}
	}

	// If Gate Local to RF is defined, and station is in range, Gate it

	if (GateLocal && Station)
	{
		if (Station->Object)
			Station = Station->Object;		// If Object Report, base distance on Object, not station
		
		if (Station->Lat != 0.0 && Station->Lon != 0.0 && myDistance(Station->Lat, Station->Lon, 0) < GateLocalDistance)
		{
			sprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);
			GetSemaphore(&Semaphore, 12);
			SendAPRSMessage(Message, -1);		// Send to all APRS Ports
			FreeSemaphore(&Semaphore);

			return;
		}
	}
}

APRSHEARDRECORD * FindStationInMH(char * Call)
{
	APRSHEARDRECORD * MH = MHDATA;
	int i;

	// We keep call in ascii format, as that is what we get from APRS-IS, and we have it in that form

	for (i = 0; i < HEARDENTRIES; i++)
	{
		if (memcmp(Call, MH->MHCALL, 9) == 0)
			return MH;

		MH++;
	}

	return NULL;
}

APRSHEARDRECORD * UpdateHeard(UCHAR * Call, int Port)
{
	APRSHEARDRECORD * MH = MHDATA;
	APRSHEARDRECORD * MHBASE = MH;
	int i;
	time_t NOW = time(NULL);
	time_t OLDEST = NOW - MAXAGE;
	char CallPadded[10] = "         ";
	BOOL SaveIGate = FALSE;
	time_t SaveLastMsg = 0;
	int SaveheardViaIS = 0;

	// We keep call in ascii format, space padded, as that is what we get from APRS-IS, and we have it in that form

	// Make Sure Space Padded

	memcpy(CallPadded, Call, (int)strlen(Call));

	for (i = 0; i < MAXHEARDENTRIES; i++)
	{
		if (memcmp(CallPadded, MH->MHCALL, 10) == 0)
		{
			// if from APRS-IS, only update if record hasn't been heard via RF

			if (Port == 0)
				MH->heardViaIS = 1;			// Flag heard via IS
			
			if (Port == 0 && MH->rfPort) 
				return MH;					// Don't update RF with IS

			if (Port == MH->rfPort)
			{
				SaveIGate = MH->IGate;
				SaveLastMsg = MH->LASTMSG;
				SaveheardViaIS = MH->heardViaIS;
				goto DoMove;
			}
		}

		if (MH->MHCALL[0] == 0 || MH->MHTIME < OLDEST)		// Spare entry
			goto DoMove;

		MH++;
	}

	//	TABLE FULL AND ENTRY NOT FOUND - MOVE DOWN ONE, AND ADD TO TOP

	i = MAXHEARDENTRIES - 1;
		
	// Move others down and add at front
DoMove:
	if (i != 0)				// First
		memmove(MHBASE + 1, MHBASE, i * sizeof(APRSHEARDRECORD));

	if (i >= HEARDENTRIES) 
	{
		char Status[80];
	
		HEARDENTRIES = i + 1;

		sprintf(Status, "IGATE Stats: Msgs %d  Local Stns %d", MessageCount , CountLocalStations());
#ifndef LINBPQ
		SetDlgItemText(hConsWnd, IGATESTATS, Status);
#endif
	}

	memcpy (MHBASE->MHCALL, CallPadded, 10);
	MHBASE->rfPort = Port;
	MHBASE->MHTIME = NOW;
	MHBASE->IGate = SaveIGate;
	MHBASE->LASTMSG = SaveLastMsg;
	MHBASE->heardViaIS = SaveheardViaIS;

	return MHBASE;
}

int CountLocalStations()
{
	APRSHEARDRECORD * MH = MHDATA;
	int i, n = 0;

	for (i = 0; i < HEARDENTRIES; i++)
	{
		if (MH->rfPort)			// DOn't count IS Stations
			n++;

		MH++;
	}
	return n;
}


BOOL CheckforDups(char * Call, char * Msg, int Len)
{
	// Primitive duplicate suppression - see if same call and text reeived in last few seconds
	
	time_t Now = time(NULL);
	time_t DupCheck = Now - DUPSECONDS;
	int i, saveindex = -1;
	char * ptr1;

	if (Len < 1)
		return TRUE;

	for (i = 0; i < MAXDUPS; i++)
	{
		if (DupInfo[i].DupTime < DupCheck)
		{
			// too old - use first if we need to save it 

			if (saveindex == -1)
			{
				saveindex = i;
			}

			if (DupInfo[i].DupTime == 0)		// Off end of used area
				break;

			continue;	
		}

		if ((Len == DupInfo[i].DupLen || (DupInfo[i].DupLen == 99 && Len > 99)) && memcmp(Call, DupInfo[i].DupUser, 7) == 0 && (memcmp(Msg, DupInfo[i].DupText, DupInfo[i].DupLen) == 0))
		{
			// Duplicate, so discard

			Msg[Len] = 0;
			ptr1 = strchr(Msg, 13);
			if (ptr1)
				*ptr1 = 0;

//			Debugprintf("Duplicate Message suppressed %s", Msg);
			return TRUE;					// Duplicate
		}
	}

	// Not in list

	if (saveindex == -1)  // List is full
		saveindex = MAXDUPS - 1;	// Stick on end	

	DupInfo[saveindex].DupTime = Now;
	memcpy(DupInfo[saveindex].DupUser, Call, 7);

	if (Len > 99) Len = 99;

	DupInfo[saveindex].DupLen = Len;
	memcpy(DupInfo[saveindex].DupText, Msg, Len);

	return FALSE;
}

char * FormatAPRSMH(APRSHEARDRECORD * MH)
 {
	 // Called from CMD.ASM

	struct tm * TM;
	static char MHLine[50];
	time_t szClock = MH->MHTIME;

	szClock = (time(NULL) - szClock);
	TM = gmtime(&szClock);

	sprintf(MHLine, "%-10s %d %.2d:%.2d:%.2d:%.2d %s\r",
		MH->MHCALL, MH->rfPort, TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, (MH->IGate) ? "IGATE" : "");

	return MHLine;
 }

// GPS Handling Code

void SelectSource(BOOL Recovering);
void DecodeRMC(char * msg, size_t len);

void PollGPSIn();


UINT GPSType = 0xffff;		// Source of Postion info - 1 = Phillips 2 = AIT1000. ffff = not posn message

int RecoveryTimer;			// Serial Port recovery

double PI = 3.1415926535;
double P2 = 3.1415926535 / 180;

double Latitude, Longtitude, SOG, COG, LatIncrement, LongIncrement;
double LastSOG = -1.0;

BOOL Check0183CheckSum(char * msg, size_t len)
{
	BOOL retcode=TRUE;
	char * ptr;
	UCHAR sum,xsum1,xsum2;

	sum=0;
	ptr=++msg;	//	Skip $

loop:

	if (*(ptr)=='*') goto eom;
	
	sum ^=*(ptr++);

	len--;

	if (len > 0) goto loop;

	return TRUE;		// No Checksum

eom:
	_strupr(ptr);

	xsum1=*(++ptr);
	xsum1-=0x30;
	if (xsum1 > 9) xsum1-=7;

	xsum2=*(++ptr);
	xsum2-=0x30;
	if (xsum2 > 9) xsum2-=7;

	xsum1=xsum1<<4;
	xsum1+=xsum2;

	return (xsum1==sum);
}

BOOL OpenGPSPort()
{
	struct PortInfo * portptr = &InPorts[0];

	// open COMM device

	if (strlen(GPSPort) < 4)
	{
		int port = atoi(GPSPort);
#ifdef WIN32
		sprintf(GPSPort, "COM%d", port);
#else
		sprintf(GPSPort, "com%d", port);
#endif	
	}

	portptr->hDevice = OpenCOMPort(GPSPort, GPSSpeed, TRUE, TRUE, FALSE, 0);
				  
	if (portptr->hDevice == 0)
	{
		return FALSE;
	}

 	return TRUE;
}

void PollGPSIn()
{
	size_t len;
	char GPSMsg[2000] = "$GPRMC,061213.000,A,5151.5021,N,00056.8388,E,0.15,324.11,190414,,,A*6F";
	char * ptr;
	struct PortInfo * portptr;

	portptr = &InPorts[0];
	
	if (!portptr->hDevice)
		return;

	getgpsin:

// Comm Error - probably lost USB Port. Try closing and reopening after a delay

//			if (RecoveryTimer == 0)
//			{
//				RecoveryTimer = 100;			// 10 Secs
//				return;
//			}
//		}

		if (portptr->gpsinptr == 160)
			portptr->gpsinptr = 0;

		len = ReadCOMBlock(portptr->hDevice, &portptr->GPSinMsg[portptr->gpsinptr],
				160 - portptr->gpsinptr);

		if (len > 0)
		{
			portptr->gpsinptr += (int)len;

			ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);

			while (ptr != NULL)
			{
				ptr++;									// include lf
				len=ptr-(char *)&portptr->GPSinMsg;					
				memcpy(&GPSMsg,portptr->GPSinMsg,len);	

				GPSMsg[len] = 0;

				if (Check0183CheckSum(GPSMsg, len))
					if (memcmp(&GPSMsg[3], "RMC", 3) == 0)
						DecodeRMC(GPSMsg, len);	

				portptr->gpsinptr -= (int)len;			// bytes left

				if (portptr->gpsinptr > 0 && *ptr == 0)
				{
					*ptr++;
					portptr->gpsinptr--;
				}

				if (portptr->gpsinptr > 0)
				{
					memmove(portptr->GPSinMsg,ptr, portptr->gpsinptr);
					ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);
				}
				else
					ptr=0;
			}
			
			goto getgpsin;
	}
	return;
}


void ClosePorts()
{
	if (InPorts[0].hDevice)
	{
		CloseCOMPort(InPorts[0].hDevice);
		InPorts[0].hDevice=0;
	}

	return;
}

void DecodeRMC(char * msg, size_t len)
{
	char * ptr1;
	char * ptr2;
	char TimHH[3], TimMM[3], TimSS[3];
	char OurSog[5], OurCog[4];
	char LatDeg[3], LonDeg[4];
	char NewLat[10] = "", NewLon[10] = "";
	struct STATIONRECORD * Stn1;

	char Day[3];

	ptr1 = &msg[7];
        
	len-=7;
	
	ptr2=(char *)memchr(ptr1,',',15);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(TimHH,ptr1,2);
	memcpy(TimMM,ptr1+2,2);
	memcpy(TimSS,ptr1+4,2);
	TimHH[2]=0;
	TimMM[2]=0;
	TimSS[2]=0;

	ptr1=ptr2;
	
	if (*(ptr1) != 'A') // ' Data Not Valid
	{
#ifndef LINBPQ
		SetDlgItemText(hConsWnd, IDC_GPS, "No GPS Fix");	
#endif
		return;
	}
        
	ptr1+=2;

	ptr2=(char *)memchr(ptr1,',',15);
		
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;
 
	memcpy(NewLat, ptr1, 7);
	memcpy(LatDeg, ptr1, 2);
	LatDeg[2]=0;
	Lat=atof(LatDeg) + (atof(ptr1+2)/60);
	
	if (*(ptr1+7) > '4') if (NewLat[6] < '9') NewLat[6]++;

	ptr1=ptr2;

	NewLat[7] = (*ptr1);
	if ((*ptr1) == 'S') Lat=-Lat;
	
	ptr1+=2;

	ptr2=(char *)memchr(ptr1,',',15);
		
	if (ptr2 == 0) return;	// Duff
	*(ptr2++)=0;

	memcpy(NewLon, ptr1, 8);
	
	memcpy(LonDeg,ptr1,3);
	LonDeg[3]=0;
	Lon=atof(LonDeg) + (atof(ptr1+3)/60);
       
	if (*(ptr1+8) > '4') if (NewLon[7] < '9') NewLon[7]++;

	ptr1=ptr2;

	NewLon[8] = (*ptr1);
	if ((*ptr1) == 'W') Lon=-Lon;

	// Now have a valid posn, so stop sending Undefined LOC Sysbol
	
	SYMBOL = CFGSYMBOL;
	SYMSET = CFGSYMSET;

	PosnSet = TRUE;

	Stn1  = (struct STATIONRECORD *)StnRecordBase;		// Pass to App
	Stn1->Lat = Lat;
	Stn1->Lon = Lon;

	if (GPSOK == 0)
	{
#ifdef LINBPQ
		Debugprintf("GPS OK");
		printf("GPS OK\n");
#else
		SetDlgItemText(hConsWnd, IDC_GPS, "GPS OK");
#endif
	}

	GPSOK = 30;	

	ptr1+=2;

	ptr2 = (char *)memchr(ptr1,',',30);
	
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(OurSog, ptr1, 4);
	OurSog[4] = 0;

	ptr1=ptr2;

	ptr2 = (char *)memchr(ptr1,',',15);
	
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(OurCog, ptr1, 3);
	OurCog[3] = 0;

	memcpy(Day,ptr2,2);
	Day[2]=0;

	SOG = atof(OurSog);
	COG = atof(OurCog);

	if (strcmp(NewLat, LAT) || strcmp(NewLon, LON))
	{
		if (MobileBeaconInterval)
		{
			time_t NOW = time(NULL);

			if ((NOW - LastMobileBeacon) > MobileBeaconInterval)
			{
				LastMobileBeacon = NOW;
				SendBeacon(0, StatusMsg, FALSE, TRUE);
			}
		}
		if (GPSSetsLocator)
		{
			ToLOC(Lat, Lon, LOC);
			sprintf(LOCATOR, "%f:%f", Lat, Lon);
		}
	}

	strcpy(LAT, NewLat);
	strcpy(LON, NewLon);
}

Dll VOID APIENTRY APRSConnect(char * Call, char * Filter)
{
	// Request APRS Data from Switch (called by APRS Applications)

	APRSApplConnected = TRUE;
	APRSWeb = TRUE;

	strcpy(APPLFilter, Filter);

	if (APPLFilter[0])
	{
		// This is called in APPL context so must queue the message

		char Msg[2000];
		PMSGWITHLEN buffptr;

		sprintf(Msg, "filter %s", Filter);

		if (strlen(Msg) > 240)
			Msg[240] = 0;


		GetSemaphore(&Semaphore, 11);

		buffptr = GetBuff();

		if (buffptr)
		{
			buffptr->Len = 0;
			strcpy(&buffptr->Data[0], "SERVER");
			strcpy(&buffptr->Data[10], Msg);
			C_Q_ADD(&APPLTX_Q, buffptr);
		}

		buffptr = GetBuff();

		if (buffptr)
		{
			buffptr->Len = 0;
			strcpy(&buffptr->Data[0], "SERVER");
			strcpy(&buffptr->Data[10], "filter?");
			C_Q_ADD(&APPLTX_Q, buffptr);
		}
		FreeSemaphore(&Semaphore);
	}
	strcpy(Call, CallPadded);
}

Dll VOID APIENTRY APRSDisconnect()
{
	// Stop requesting APRS Data from Switch (called by APRS Applications)

	char Msg[2000];
	PMSGWITHLEN buffptr;

	strcpy(ISFilter, NodeFilter);
	sprintf(Msg, "filter %s", NodeFilter);

	APRSApplConnected = FALSE;
	APRSWeb = FALSE;

	GetSemaphore(&Semaphore, 11);

	buffptr = GetBuff();

	if (buffptr)
	{
		buffptr->Len = 0;
		strcpy(&buffptr->Data[0], "SERVER");
		strcpy(&buffptr->Data[10], Msg);
		C_Q_ADD(&APPLTX_Q, buffptr);
	}

	buffptr = GetBuff();

	if (buffptr)
	{
		buffptr->Len = 0;
		strcpy(&buffptr->Data[0], "SERVER");
		strcpy(&buffptr->Data[10], "filter?");
		C_Q_ADD(&APPLTX_Q, buffptr);
	}

	while (APPL_Q)
	{
		buffptr = Q_REM(&APPL_Q);
		ReleaseBuffer(buffptr);
	}

	FreeSemaphore(&Semaphore);
}


Dll char * APIENTRY APRSGetStatusMsgPtr()
{
	return StatusMsg;
}



Dll BOOL APIENTRY GetAPRSFrame(char * Frame, char * Call)
{
	// Request APRS Data from Switch (called by APRS Applications)

	void ** buffptr;
#ifdef bpq32
	struct _EXCEPTION_POINTERS exinfo;
#endif

	GetSemaphore(&Semaphore, 10);
	{
		if (APPL_Q)
		{
			buffptr = Q_REM(&APPL_Q);

			memcpy(Call, (char *)&buffptr[2], 12);
			strcpy(Frame, (char *)&buffptr[5]);

			ReleaseBuffer(buffptr);
			FreeSemaphore(&Semaphore);
			return TRUE;
		}
	}

	FreeSemaphore(&Semaphore);

	return FALSE;
}

Dll BOOL APIENTRY PutAPRSFrame(char * Frame, int Len, int Port)
{
	// Called from BPQAPRS App
	// Message has to be queued so it can be sent by Timer Process (IS sock is not valid in this context)

	PMSGWITHLEN buffptr;

	GetSemaphore(&Semaphore, 11);

	buffptr = GetBuff();

	if (buffptr)
	{
		buffptr->Len = ++Len;			// Len doesn't include Null
		memcpy(&buffptr->Data[0], Frame, Len);
		C_Q_ADD(&APPLTX_Q, buffptr);
	}

//	buffptr-> = Port;				// Pass to SendAPRSMessage();

	FreeSemaphore(&Semaphore);

	return TRUE;
}

Dll BOOL APIENTRY APISendAPRSMessage(char * Text, char * ToCall)
{
	// Called from BPQAPRS App or BPQMail
	// Message has to be queued so it can be sent by Timer Process (IS sock is not valid in this context)

	PMSGWITHLEN buffptr;

	if (APRSActive == 0)
		return FALSE;

	GetSemaphore(&Semaphore, 11);

	buffptr = GetBuff();

	if (buffptr)
	{
		buffptr->Len = 0;
		memcpy(&buffptr->Data[0], ToCall, 9);
		buffptr->Data[9] = 0;
		strcpy(&buffptr->Data[10], Text);
		C_Q_ADD(&APPLTX_Q, buffptr);
	}

	FreeSemaphore(&Semaphore);

	return TRUE;
}

Dll BOOL APIENTRY GetAPRSLatLon(double * PLat,  double * PLon)
{
	*PLat = Lat;
	*PLon = Lon;

	return GPSOK;
}

Dll BOOL APIENTRY GetAPRSLatLonString(char * PLat,  char * PLon)
{
	strcpy(PLat, LAT);
	strcpy(PLon, LON);

	return GPSOK;
}

// Code to support getting GPS from Andriod Device running BlueNMEA


#define SD_BOTH         0x02

static VOID ProcessReceivedData(SOCKET TCPSock)
{
	char UDPMsg[8192];
	char Buffer[65536];

	int len = recv(TCPSock, Buffer, 65500, 0);

	char * ptr;
	char * Lastptr;

	if (len <= 0)
	{
		closesocket(TCPSock);
		BlueNMEAOK = FALSE;
		return;
	}

	ptr = Lastptr = Buffer;
	Buffer[len] = 0;

	while (len > 0)
	{
		ptr = strchr(Lastptr, 10);

		if (ptr)
		{
			size_t Len = ptr - Lastptr -1;

			if (Len > 8100)
				return;
		
			memcpy(UDPMsg, Lastptr, Len);
			UDPMsg[Len++] = 13;
			UDPMsg[Len++] = 10;
			UDPMsg[Len] = 0;

			if (!Check0183CheckSum(UDPMsg, Len))
			{
				Debugprintf("Checksum Error %s", UDPMsg);
			}
			else
			{			
				if (memcmp(&UDPMsg[3], "RMC", 3) == 0)
					DecodeRMC(UDPMsg, Len);

				else if (memcmp(UDPMsg, "!AIVDM", 6) == 0)
					ProcessAISMessage(UDPMsg, Len);

			}
			Lastptr = ptr + 1;
			len -= (int)Len;
		}
		else
			return;
	}
}

static VOID TCPConnect(void * unused)
{
	int err, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	struct sockaddr_in destaddr;
	SOCKET TCPSock;

	if (HostName[0] == 0)
		return;

	destaddr.sin_addr.s_addr = inet_addr(HostName);
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(HostPort);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		struct hostent * HostEnt = gethostbyname(HostName);
		 
		if (!HostEnt)
			return;			// Resolve failed

		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

	TCPSock = socket(AF_INET,SOCK_STREAM,0);

	if (TCPSock == INVALID_SOCKET)
	{
  	 	return; 
	}
 
	setsockopt (TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	BlueNMEAOK = TRUE;			// So we don't try to reconnect while waiting		

	if (connect(TCPSock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		ioctl(TCPSock, FIONBIO, &param);
	}
	else
	{
		err=WSAGetLastError();
#ifdef LINBPQ
   		printf("Connect Failed for AIS socket - error code = %d\n", err);
#else
   		Debugprintf("Connect Failed for AIS socket - error code = %d", err);
#endif		
		closesocket(TCPSock);
		BlueNMEAOK = FALSE;

		return;
	}

	BlueNMEAOK = TRUE;

	while (TRUE)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TCPSock,&readfs);
		FD_SET(TCPSock,&errorfs);

		timeout.tv_sec = 900;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select((int)TCPSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TCPSock, &readfs))
			{
				ProcessReceivedData(TCPSock);			
			}
								
			if (FD_ISSET(TCPSock, &errorfs))
			{
Lost:				
#ifdef LINBPQ
				printf("AIS Connection lost\n");
#endif			
				closesocket(TCPSock);
				BlueNMEAOK = FALSE;;
				return;
			}
		}
		else
		{
			// 15 mins without data. Shouldn't happen

			shutdown(TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TCPSock);
			BlueNMEAOK = FALSE;
			return;
		}
	}
}

int GPSDAlerted = 0;

static VOID GPSDConnect(void * unused)
{
	int err, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	struct sockaddr_in destaddr;
	SOCKET TCPSock;

	if (GPSDHost[0] == 0)
		return;

	destaddr.sin_addr.s_addr = inet_addr(GPSDHost);
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(GPSDPort);

	TCPSock = socket(AF_INET,SOCK_STREAM,0);

	if (TCPSock == INVALID_SOCKET)
	{
  	 	return; 
	}
 
	setsockopt (TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	GPSDOK = TRUE;			// So we don't try to reconnect while waiting		

	if (connect(TCPSock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

#ifdef LINBPQ
   		printf("GPSD Connected\n");
#else
   		Debugprintf("GPSD Connected");
#endif	
		GPSDAlerted = 0;
		ioctl(TCPSock, FIONBIO, &param);

		// Request data 

		send(TCPSock, "?WATCH={\"enable\":true,\"nmea\":true}\r\n", 36, 0);
	}
	else
	{
		err=WSAGetLastError();
   		if (GPSDAlerted == 0)
#ifdef LINBPQ
   			printf("GPSD Connect Failed - error code = %d\n", err);
#else
			Debugprintf("GPSD Connect Failed - error code = %d", err);
#endif
		GPSDAlerted = 1;
		closesocket(TCPSock);
		GPSDOK = FALSE;

		return;
	}

	while (TRUE)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TCPSock,&readfs);
		FD_SET(TCPSock,&errorfs);

		timeout.tv_sec = 900;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select((int)TCPSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (ret == SOCKET_ERROR)
		{
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TCPSock, &readfs))
			{
				char Buffer[65536];
				int len = recv(TCPSock, Buffer, 65500, 0);
				char TCPMsg[8192];

				char * ptr;
				char * Lastptr;

				if (len == 0)
				{
					closesocket(TCPSock);
					GPSDOK = FALSE;;
					return;
				}

				if (len < 9000)
				{
					Buffer[len] = 0;

					ptr = Lastptr = Buffer;
					Buffer[len] = 0;

					while (len > 0)
					{
						ptr = strchr(Lastptr, 10);

						if (ptr)
						{
							size_t Len = ptr - Lastptr -1;

							if (Len > 8100)
								return;

							memcpy(TCPMsg, Lastptr, Len);
							TCPMsg[Len++] = 13;
							TCPMsg[Len++] = 10;
							TCPMsg[Len] = 0;

							if (!Check0183CheckSum(TCPMsg, Len))
							{
								Debugprintf("Checksum Error %s", TCPMsg);
							}
							else
							{			
								if (memcmp(&TCPMsg[3], "RMC", 3) == 0)
									DecodeRMC(TCPMsg, Len);
							}
							Lastptr = ptr + 1;
							len -= (int)Len;
						}
						else
							return;
					}
				}
			}

			if (FD_ISSET(TCPSock, &errorfs))
			{
Lost:				
#ifdef LINBPQ
				printf("GPSD Connection lost\n");
#endif			
				closesocket(TCPSock);
				GPSDOK = FALSE;;
				return;
			}
		}
		else
		{
			// 15 mins without data. Shouldn't happen

			shutdown(TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TCPSock);
			GPSDOK = FALSE;
			return;
		}
	}
}





// Code Moved from APRS Application

//
// APRS Mapping and Messaging App for BPQ32 Switch.
//


VOID APIENTRY APRSConnect(char * Call, char * Filter);
VOID APIENTRY APRSDisconnect();
BOOL APIENTRY GetAPRSFrame(char * Frame, char * Call);
BOOL APIENTRY PutAPRSFrame(char * Frame, int Len, int Port);
BOOL APIENTRY PutAPRSMessage(char * Frame, int Len);
BOOL APIENTRY GetAPRSLatLon(double * PLat,  double * PLon);
BOOL APIENTRY GetAPRSLatLonString(char * PLat,  char * PLon);
VOID APIENTRY APISendBeacon();


int NewLine(HWND hWnd);
VOID	ProcessBuff(HWND hWnd, MESSAGE * buff,int len,int stamp);
int TogglePort(HWND hWnd, int Item, int mask);
VOID SendFrame(UCHAR * buff, int txlen);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
//void UpdateStation(char * Call, char * Path, char * Comment, double V_Lat, double V_Lon, double V_SOG, double V_COG, int iconRow, int iconCol);
VOID FindStationsByPixel(int MouseX, int MouseY);
void RefreshStation(struct STATIONRECORD * ptr);
void RefreshStationList();
void RefreshStationMap();
BOOL DecodeLocationString(UCHAR * Payload, struct STATIONRECORD * Station);
VOID Decode_MIC_E_Packet(char * Payload, struct STATIONRECORD * Station);
BOOL GetLocPixels(double Lat, double Lon, int * X, int * Y);
VOID APRSPoll();
VOID OSMThread();
VOID ResolveThread();
VOID RefreshTile(char * FN, int Zoom, int x, int y);
int ProcessMessage(char * Payload, struct STATIONRECORD * Station);
VOID APRSSecTimer();
double myBearing(double laa, double loa);

BOOL CreatePipeThread();

VOID SendWeatherBeacon();
VOID DecodeWXPortList();

	
VOID DecodeWXReport(struct APRSConnectionInfo * sockptr, char * WX)
{
	UCHAR * ptr = strchr(WX, '_');
	char Type;
	int Val;

	if (ptr == 0)
		return;

	sockptr->WindDirn = atoi(++ptr);
	ptr += 4;
	sockptr->WindSpeed = atoi(ptr);
	ptr += 3;
WXLoop:

	Type = *(ptr++);

	if (*ptr =='.')	// Missing Value
	{
		while (*ptr == '.')
			ptr++;

		goto WXLoop;
	}

	Val = atoi(ptr);

	switch (Type)
	{
	case 'c': // = wind direction (in degrees).	
		
		sockptr->WindDirn = Val;
		break;
	
	case 's': // = sustained one-minute wind speed (in mph).
	
		sockptr->WindSpeed = Val;
		break;
	
	case 'g': // = gust (peak wind speed in mph in the last 5 minutes).
	
		sockptr->WindGust = Val;
		break;

	case 't': // = temperature (in degrees Fahrenheit). Temperatures below zero are expressed as -01 to -99.
	
		sockptr->Temp = Val;
		break;

	case 'r': // = rainfall (in hundredths of an inch) in the last hour.
		
		sockptr->RainLastHour = Val;
		break;

	case 'p': // = rainfall (in hundredths of an inch) in the last 24 hours.

		sockptr->RainLastDay = Val;
		break;

	case 'P': // = rainfall (in hundredths of an inch) since midnight.

		sockptr->RainToday = Val;
		break;

	case 'h': // = humidity (in %. 00 = 100%).
	
		sockptr->Humidity = Val;
		break;

	case 'b': // = barometric pressure (in tenths of millibars/tenths of hPascal).

		sockptr->Pressure = Val;
		break;

	default:

		return;
	}
	while(isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr != ' ')
		goto WXLoop;
}

static char HeaderTemplate[] = "Accept: */*\r\nHost: %s\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";
//char Header[] = "Accept: */*\r\nHost: tile.openstreetmap.org\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";

char APRSMsg[300];

Dll struct STATIONRECORD *  APIENTRY APPLFindStation(char * Call, BOOL AddIfNotFount)
{
	//	Called from APRS Appl

	struct STATIONRECORD * Stn;

	GetSemaphore(&Semaphore, 12);
	Stn = FindStation(Call, AddIfNotFount)	;		
	FreeSemaphore(&Semaphore);

	return Stn;
}

Dll struct APRSMESSAGE * APIENTRY APRSGetMessageBuffer()
{
	struct APRSMESSAGE * ptr = MessageRecordPool;

	if (ptr == NULL)
	{
		// try getting oldest

		ptr = SMEM->Messages;

		if (ptr)
		{
			SMEM->Messages = ptr->Next;
			memset(ptr, 0, sizeof(struct APRSMESSAGE));
		}
		return ptr;
	}
		
	if (ptr)
	{
		MessageRecordPool = ptr->Next;	// Unchain
		MessageCount++;
	
		ptr->Next = NULL;

		memset(ptr, 0, sizeof(struct APRSMESSAGE));
	}

	return ptr; 
}


struct STATIONRECORD * FindStation(char * Call, BOOL AddIfNotFount)
{
	int i = 0;
	struct STATIONRECORD * find;
	struct STATIONRECORD * ptr;
	struct STATIONRECORD * last = NULL;
	int sum = 0;

	if (APRSActive == 0 || StationRecords == 0)
		return FALSE;

	if (strlen(Call) > 9)
	{
		Debugprintf("APRS Call too long %s", Call);
		Call[9] = 0;
	}

	find = *StationRecords;
	while(find)
	{
		if (strlen(find->Callsign) > 9)
		{
			Debugprintf("APRS Call in Station List too long %s", find->Callsign);
			find->Callsign[9] = 0;
		}

	    if (strcmp(find->Callsign, Call) == 0)
			return find;

		last = find;
		find = find->Next;
		i++;
	}
 
	//   Not found - add on end

	if (AddIfNotFount)
	{
		// Get first from station record pool
		
		ptr = StationRecordPool;
		
		if (ptr)
		{
			StationRecordPool = ptr->Next;	// Unchain
			StationCount++;
		}
		else
		{
			//	Get First from Stations

			ptr = *StationRecords;

			if (ptr)
				*StationRecords = ptr->Next;
		}

		if (ptr == NULL)
			return NULL;

		memset(ptr, 0, sizeof(struct STATIONRECORD));
	
//		EnterCriticalSection(&Crit);

		if (*StationRecords == NULL)
			*StationRecords = ptr;
		else
			last->Next = ptr;

//		LeaveCriticalSection(&Crit);

		//	Debugprintf("APRS Add Stn %s Station Count = %d", Call, StationCount);
       
		strcpy(ptr->Callsign, Call);
		ptr->TimeLastUpdated = ptr->TimeAdded = time(NULL);
		ptr->Index = i;
		ptr->NoTracks = DefaultNoTracks;

		for (i = 0; i < 9; i++)
			sum += Call[i];

		sum %= 20;

		ptr->TrackColour = sum;
		ptr->Moved = TRUE;

		return ptr;
	}
	else
		return NULL;
}

struct STATIONRECORD * ProcessRFFrame(char * Msg, int len, int * ourMessage)
{
	char * Payload;
	char * Path = NULL;
	char * Comment = NULL;
	char * Callsign;
	char * ptr;
	int Port = 0;

	struct STATIONRECORD * Station = NULL;

	Msg[len - 1] = 0;

//	Debugprintf("RF Frame %s", Msg);

	Msg += 10;				// Skip Timestamp
	
	Payload = strchr(Msg, ':');			// End of Address String

	if (Payload == NULL)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return Station;
	}

	ptr = strstr(Msg, "Port=");

	if (ptr)
		Port = atoi(&ptr[5]);

	Payload++;

	if (*Payload != 0x0d)
		return Station;

	*Payload++ = 0;

	Callsign = Msg;

	Path = strchr(Msg, '>');

	if (Path == NULL)
	{
		Debugprintf("Invalid Header %s", Msg);
		return Station;
	}

	*Path++ = 0;

	ptr = strchr(Path, ' ');

	if (ptr)
		*ptr = 0;

	// Look up station - create a new one if not found

	if (strcmp(Callsign, "AIS") == 0)
	{
		if (needAIS)
		{
			Payload += 3;
			ProcessAISMessage(Payload, strlen(Payload));
		}
		else
			Debugprintf(Payload);

		return 0;
	}

	Station = FindStation(Callsign, TRUE);
	
	strcpy(Station->Path, Path);
	strcpy(Station->LastPacket, Payload);
	Station->LastPort = Port;

	*ourMessage = DecodeAPRSPayload(Payload, Station);
	Station->TimeLastUpdated = time(NULL);

	return Station;
}


/*
2E0AYY>APU25N,TCPIP*,qAC,AHUBSWE2:=5105.18N/00108.19E-Paul in Folkestone Kent {UIV32N}
G0AVP-12>APT310,MB7UC*,WIDE3-2,qAR,G3PWJ:!5047.19N\00108.45Wk074/000/Paul mobile
G0CJM-12>CQ,TCPIP*,qAC,AHUBSWE2:=/3&R<NDEp/  B>io94sg
M0HFC>APRS,WIDE2-1,qAR,G0MNI:!5342.83N/00013.79W# Humber Fortress ARC Look us up on QRZ
G8WVW-3>APTT4,WIDE1-1,WIDE2-1,qAS,G8WVW:T#063,123,036,000,000,000,00000000
*/


struct STATIONRECORD * DecodeAPRSISMsg(char * Msg)
{
	char * Payload;
	char * Path = NULL;
	char * Comment = NULL;
	char * Callsign;
	struct STATIONRECORD * Station = NULL;

//	Debugprintf(Msg);
		
	Payload = strchr(Msg, ':');			// End of Address String

	if (Payload == NULL)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return Station;
	}

	*Payload++ = 0;

	Callsign = Msg;

	Path = strchr(Msg, '>');

	if (Path == NULL)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return Station;
	}

	*Path++ = 0;

	// Look up station - create a new one if not found

	if (strlen(Callsign) > 11)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return Station;
	}

	Station = FindStation(Callsign, TRUE);
	
	strcpy(Station->Path, Path);
	strcpy(Station->LastPacket, Payload);
	Station->LastPort = 0;

	DecodeAPRSPayload(Payload, Station);
	Station->TimeLastUpdated = time(NULL);

	return Station;
}

double Cube91 = 91.0 * 91.0 * 91.0;
double Square91 = 91.0 * 91.0;

BOOL DecodeLocationString(UCHAR * Payload, struct STATIONRECORD * Station)
{
	UCHAR SymChar;
	char SymSet;
	char NS;
	char EW;
	double NewLat, NewLon;
	char LatDeg[3], LonDeg[4];
	char save;

	// Compressed has first character not a digit (it is symbol table)

	// /YYYYXXXX$csT

	if (Payload[0] == '!')
		return FALSE;					// Ultimeter 2000 Weather Station

	if (!isdigit(*Payload))
	{
		int C, S;
		
		SymSet = *Payload;
		SymChar = Payload[9];

		NewLat = 90.0 - ((Payload[1] - 33) * Cube91 + (Payload[2] - 33) * Square91 +
			(Payload[3] - 33) * 91.0 + (Payload[4] - 33)) / 380926.0;

		Payload += 4;
				
		NewLon = -180.0 + ((Payload[1] - 33) * Cube91 + (Payload[2] - 33) * Square91 +
			(Payload[3] - 33) * 91.0 + (Payload[4] - 33)) / 190463.0;

		C = Payload[6] - 33;

		if (C >= 0 && C < 90 )
		{
			S = Payload[7] - 33;

			Station->Course = C * 4;
			Station->Speed = (pow(1.08, S) - 1) * 1.15077945;	// MPH; 
		}



	}
	else
	{
		// Standard format ddmm.mmN/dddmm.mmE?

		NS = Payload[7] & 0xdf;		// Mask Lower Case Bit
		EW = Payload[17] & 0xdf;

		SymSet = Payload[8];
		SymChar = Payload[18];

		if (SymChar == '_')		// WX
		{
			if (strlen(Payload) > 30)
				strcpy(Station->LastWXPacket, Payload);
		}

		memcpy(LatDeg, Payload,2);
		LatDeg[2]=0;
		NewLat = atof(LatDeg) + (atof(Payload+2) / 60);
       
		if (NS == 'S')
			NewLat = -NewLat;
		else
			if (NS != 'N')
				return FALSE;

		memcpy(LonDeg,Payload + 9, 3);

		if (SymChar != '_' && Payload[22] == '/')		// not if WX
		{
			Station->Course = atoi(Payload + 19);
			Station->Speed = atoi(Payload + 23);
		}

		LonDeg[3]=0;

		save = Payload[17];
		Payload[17] = 0;
		NewLon = atof(LonDeg) + (atof(Payload+12) / 60);
		Payload[17] = save;
		
		if (EW == 'W')
			NewLon = -NewLon;
		else
			if (EW != 'E')
				return FALSE;
	}

	Station->Symbol = SymChar;

	if (SymChar > ' ' && SymChar < 0x7f)
		SymChar -= '!';
	else
		SymChar = 0;

	Station->IconOverlay = 0;

	if ((SymSet >= '0' && SymSet <= '9') || (SymSet >= 'A' && SymSet <= 'Z'))
	{
		SymChar += 96;
		Station->IconOverlay = SymSet;
	}
	else
		if (SymSet == '\\')
			SymChar += 96;

	Station->iconRow = SymChar >> 4;
	Station->iconCol = SymChar & 15;

	if (NewLat > 90 || NewLat < -90 || NewLon > 180 || NewLon < -180)
		return TRUE;

	if (Station->Lat != NewLat || Station->Lon != NewLon)
	{
		time_t NOW = time(NULL);
		time_t Age = NOW - Station->TimeLastTracked;

		if (Age > 15)				// Don't update too often
		{
			// Add to track

			Station->TimeLastTracked = NOW;

//			if (memcmp(Station->Callsign, "ISS ", 4) == 0)
//				Debugprintf("%s %s %s ",Station->Callsign, Station->Path, Station->LastPacket);

			Station->LatTrack[Station->Trackptr] = NewLat;
			Station->LonTrack[Station->Trackptr] = NewLon;
			Station->TrackTime[Station->Trackptr] = NOW;

			Station->Trackptr++;
			Station->Moved = TRUE;

			if (Station->Trackptr == TRACKPOINTS)
				Station->Trackptr = 0;
		}

		Station->Lat = NewLat;
		Station->Lon = NewLon;
		Station->Approx = 0;
	}


	return TRUE;
}

int DecodeAPRSPayload(char * Payload, struct STATIONRECORD * Station)
{
	char * TimeStamp;
	char * ObjName;
	char ObjState;
	struct STATIONRECORD * Object;
	BOOL Item = FALSE;
	char * ptr;
	char * Callsign;
	char * Path;
	char * Msg;
	char * context;
	struct STATIONRECORD * TPStation;

	Station->Object = NULL;

	if (strcmp(Station->Callsign, "LA1ZDA-2") == 0)
	{
		int i = 1;	
	}
	switch(*Payload)
	{
	case '`':
	case 0x27:					// '
	case 0x1c:
	case 0x1d:					// MIC-E

		Decode_MIC_E_Packet(Payload, Station);
		return 0;

	case '$':					// NMEA
		Debugprintf(Payload);
		break;

	case ')':					// Item	

//		Debugprintf("%s %s %s", Station->Callsign, Station->Path, Payload);

		Item = TRUE;
		ObjName = ptr = Payload + 1;

		while (TRUE)
		{
			ObjState = *ptr;
			if (ObjState == 0)
				return 0;					// Corrupt

			if (ObjState == '!' || ObjState == '_')	// Item Terminator
				break;

			ptr++;
		}

		*ptr = 0;						// Terminate Name

		Object = FindStation(ObjName, TRUE);
		Object->ObjState = *ptr++ = ObjState;

		strcpy(Object->Path, Station->Callsign);
		strcat(Object->Path, ">");
		if (Object == Station)
		{
			char Temp[256];
			strcpy(Temp, Station->Path);
			strcat(Object->Path, Temp);
			Debugprintf("item is station %s", Payload);
		}
		else
			strcat(Object->Path, Station->Path);

		strcpy(Object->LastPacket, Payload);

		if (ObjState != '_')		// Deleted Objects may have odd positions
			DecodeLocationString(ptr, Object);

		Object->TimeLastUpdated = time(NULL);
		Station->Object = Object;
		return 0;


	case ';':					// Object

		ObjName = Payload + 1;
		ObjState = Payload[10];	// * Live, _Killed

		Payload[10] = 0;
		Object = FindStation(ObjName, TRUE);
		Object->ObjState = Payload[10] = ObjState;

		strcpy(Object->Path, Station->Callsign);
		strcat(Object->Path, ">");
		if (Object == Station)
		{
			char Temp[256];
			strcpy(Temp, Station->Path);
			strcat(Object->Path, Temp);
			Debugprintf("Object is station %s", Payload);
		}
		else
			strcat(Object->Path, Station->Path);


		strcpy(Object->LastPacket, Payload);

		TimeStamp = Payload + 11;

		if (ObjState != '_')		// Deleted Objects may have odd positions
			DecodeLocationString(Payload + 18, Object);
		
		Object->TimeLastUpdated = time(NULL);
		Object->LastPort = Station->LastPort;
		Station->Object = Object;
		return 0;

	case '@':
	case '/':					// Timestamp, No Messaging

		TimeStamp = ++Payload;
		Payload += 6;

	case '=':
	case '!':

		Payload++;
	
		DecodeLocationString(Payload, Station);

		return 0;	

	case '>':				// Status

		strcpy(Station->Status, &Payload[1]);

	case '<':				// Capabilities
	case '_':				// Weather
	case 'T':				// Telemetry

		break;

	case ':':				// Message

		return ProcessMessage(Payload, Station);

	case '}':			// Third Party Header
			
		// Process Payload as a new message

		// }GM7HHB-9>APDR12,TCPIP,MM1AVR*:=5556.62N/00303.55W>204/000/A=000213 http://www.dstartv.com

		Callsign = Msg = &Payload[1];
		Path = strchr(Msg, '>');

		if (Path == NULL)
			return 0;

		*Path++ = 0;

		Payload = strchr(Path, ':');

		if (Payload == NULL)
			return 0;

		*(Payload++) = 0;

		// Check Dup Filter

		if (CheckforDups(Callsign, Payload, (int)strlen(Payload)))
			return 0;

		// Look up station - create a new one if not found

		TPStation = FindStation(Callsign, TRUE);
	
		strcpy(TPStation->Path, Path);
		strcpy(TPStation->LastPacket, Payload);
		TPStation->LastPort = 0;					// Heard on RF, but info is from IS

		DecodeAPRSPayload(Payload, TPStation);
		TPStation->TimeLastUpdated = time(NULL);

		return 0;

	default:

		// Non - APRS Message. If Payload contains a valid 6 char locator derive a position from it

		if (Station->Lat != 0.0 || Station->Lon != 0.0)
			return 0;				// already have position

		ptr = strtok_s(Payload, ",[](){} \n", &context);

		while (ptr && ptr[0])
		{
			if (strlen(ptr) == 6)		// could be locator
			{
				double Lat = 0.0, Lon = 0.0;

				if (FromLOC(ptr, &Lat, &Lon))
				{
					if (Lat != 0.0  && Lon != 0.0)
					{
						// Randomise in locator square.

						Lat = Lat + ((rand() / 24.0) / RAND_MAX);
						Lon = Lon + ((rand() / 12.0) / RAND_MAX);
						Station->Lat = Lat;
						Station->Lon = Lon;
						Station->Approx = 1;
						Debugprintf("%s %s %s", Station->Callsign, Station->Path, Payload);
					}
				}
			}

			ptr = strtok_s(NULL, ",[](){} \n", &context);
		}

		return 0;
	}
	return 0;
}

// Convert MIC-E Char to Lat Digit (offset by 0x30)
//				  0123456789      @ABCDEFGHIJKLMNOPQRSTUVWXYZ				
char MicELat[] = "0123456789???????0123456789  ???0123456789 " ;

char MicECode[]= "0000000000???????111111111110???22222222222" ;


VOID Decode_MIC_E_Packet(char * Payload, struct STATIONRECORD * Station)
{
	// Info is encoded in the Dest Addr (in Station->Path) as well as Payload. 
	// See APRS Spec for full details

	char Lat[10];		// DDMMHH
	char LatDeg[3];
	char * ptr;
	char c;
	int i, n;
	int LonDeg, LonMin;
	BOOL LonOffset = FALSE;
	char NS = 'S';
	char EW = 'E';
	UCHAR SymChar, SymSet;
	double NewLat, NewLon;
	int SP, DC, SE;				// Course/Speed Encoded
	int Course, Speed;

	// Make sure packet is long enough to have an valid address

 	if (strlen(Payload) < 9)
		return;

	ptr = &Station->Path[0];

	for (i = 0; i < 6; i++)
	{
		n = (*(ptr++)) - 0x30;
		c = MicELat[n];

		if (c == '?')			// Illegal
			return;

		if (c == ' ')
			c = '0';			// Limited Precision
		
		Lat[i] = c;

	}

	Lat[6] = 0;

	if (Station->Path[3] > 'O')
		NS = 'N';

	if (Station->Path[5] > 'O')
		EW = 'W';

	if (Station->Path[4] > 'O')
		LonOffset = TRUE;

	n = Payload[1] - 28;			// Lon Degrees S9PU0T,WIDE1-1,WIDE2-2,qAR,WB9TLH-15:`rB0oII>/]"6W}44

	if (LonOffset)
		n += 100;

	if (n > 179 && n < 190)
		n -= 80;
	else
	if (n > 189 && n < 200)
		n -= 190;

	LonDeg = n;

/*
	To decode the longitude degrees value:
1. subtract 28 from the d+28 value to obtain d.
2. if the longitude offset is +100 degrees, add 100 to d.
3. subtract 80 if 180 ˜ d ˜ 189
(i.e. the longitude is in the range 100–109 degrees).
4. or, subtract 190 if 190 ˜ d ˜ 199.
(i.e. the longitude is in the range 0–9 degrees).
*/

	n = Payload[2] - 28;			// Lon Mins

	if (n > 59)
		n -= 60;

	LonMin = n;

	n = Payload[3] - 28;			// Lon Mins/100;

//1. subtract 28 from the m+28 value to obtain m.
//2. subtract 60 if m ™ 60.
//(i.e. the longitude minutes is in the range 0–9).


	memcpy(LatDeg, Lat, 2);
	LatDeg[2]=0;
	
	NewLat = atof(LatDeg) + (atof(Lat+2) / 6000.0);
       
	if (NS == 'S')
		NewLat = -NewLat;

	NewLon = LonDeg + LonMin / 60.0 + n / 6000.0;
       
	if (EW == 'W')				// West
		NewLon = -NewLon;


	SP = Payload[4] - 28;
	DC = Payload[5] - 28;
	SE = Payload[6] - 28;		// Course 100 and 10 degs

	Speed = DC / 10;		// Quotient = Speed Units
	Course = DC - (Speed * 10);	// Remainder = Course Deg/100

	Course = SE + (Course * 100);

	Speed += SP * 10;

	if (Speed >= 800)
		Speed -= 800;

	if (Course >= 400)
		Course -= 400;

	Station->Course = Course;
	Station->Speed = Speed * 1.15077945;	// MPH

//	Debugprintf("MIC-E Course/Speed %s %d %d", Station->Callsign, Course, Speed);

	SymChar = Payload[7];			// Symbol
	SymSet = Payload[8];			// Symbol

	SymChar -= '!';

	Station->IconOverlay = 0;

	if ((SymSet >= '0' && SymSet <= '9') || (SymSet >= 'A' && SymSet <= 'Z'))
	{
		SymChar += 96;
		Station->IconOverlay = SymSet;
	}
	else
		if (SymSet == '\\')
			SymChar += 96;

	Station->iconRow = SymChar >> 4;
	Station->iconCol = SymChar & 15;

	if (NewLat > 90 || NewLat < -90 || NewLon > 180 || NewLon < -180)
		return;

	if (Station->Lat != NewLat || Station->Lon != NewLon)
	{
		time_t NOW = time(NULL);
		time_t Age = NOW - Station->TimeLastUpdated;

		if (Age > 15)				// Don't update too often
		{
			// Add to track

//			if (memcmp(Station->Callsign, "ISS ", 4) == 0)
//				Debugprintf("%s %s %s ",Station->Callsign, Station->Path, Station->LastPacket);

			Station->LatTrack[Station->Trackptr] = NewLat;
			Station->LonTrack[Station->Trackptr] = NewLon;
			Station->TrackTime[Station->Trackptr] = NOW;

			Station->Trackptr++;
			Station->Moved = TRUE;

			if (Station->Trackptr == TRACKPOINTS)
				Station->Trackptr = 0;
		}

		Station->Lat = NewLat;
		Station->Lon = NewLon;
		Station->Approx = 0;
	}

	return;

}

/*

INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

//	int retCode, disp;
//	char Key[80];
//	HKEY hKey;
//	BOOL OK;
//	OPENFILENAME ofn;
//	char Digis[100];

	int Port = PortNum[CurrentPage];

	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{
/*			case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = &FN[Port][0];
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0]);

			break;


		case IDOK:

			GetDlgItemText(hDlg, IDC_UIDEST, &UIDEST[Port][0], 10);

			if (UIDigi[Port])
			{
				free(UIDigi[Port]);
				UIDigi[Port] = NULL;
			}

			if (UIDigiAX[Port])
			{
				free(UIDigiAX[Port]);
				UIDigiAX[Port] = NULL;
			}

			GetDlgItemText(hDlg, IDC_UIDIGIS, Digis, 99); 
		
			UIDigi[Port] = _strdup(Digis);
		
			GetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0], 255); 
			GetDlgItemText(hDlg, IDC_MESSAGE, &Message[Port][0], 1000); 
	
			Interval[Port] = GetDlgItemInt(hDlg, IDC_INTERVAL, &OK, FALSE); 

			MinCounter[Port] = Interval[Port];

			SendFromFile[Port] = IsDlgButtonChecked(hDlg, IDC_FROMFILE);

			sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", PortNum[CurrentPage]);

			retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIDEST[Port][0], strlen(&UIDEST[Port][0]));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
				retCode = RegSetValueEx(hKey, "Enabled", 0, REG_DWORD,(BYTE *)&UIEnabled[Port], 4);
				retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, Digis, strlen(Digis));

				RegCloseKey(hKey);
			}

			SetupUI(Port);

			return (INT_PTR)TRUE;


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case ID_TEST:

			SendBeacon(Port);
			return TRUE;




		}
		break;

	}	
	return (INT_PTR)FALSE;
}




VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos, tab = 0;
	INITCOMMONCONTROLSEX init;

	char PortNo[60];
	struct _EXTPORTDATA * PORTVEC;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC = ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		// Only allow UI on ax.25 ports

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntry(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				continue;

		sprintf(PortNo, "Port %2d", GetPortNumber(i));
		PortNum[tab] = GetPortNumber(i);

		tie.pszText = PortNo;
		TabCtrl_InsertItem(pHdr->hwndTab, tab, &tie);
	
		pHdr->apRes[tab++] = DoLockDlgRes("PORTPAGE");

	}

	PageCount = tab;

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < PageCount; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char PortDesc[40];
	int Port;

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	Port = PortNum[CurrentPage];
	// Fill in the controls

	GetPortDescription(PortNum[CurrentPage], PortDesc);

	SetDlgItemText(hwndDisplay, IDC_PORTNAME, PortDesc);

//	CheckDlgButton(hwndDisplay, IDC_FROMFILE, SendFromFile[Port]);

//	SetDlgItemInt(hwndDisplay, IDC_INTERVAL, Interval[Port], FALSE);

	SetDlgItemText(hwndDisplay, IDC_UIDEST, &UIDEST[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_UIDIGIS, UIDigi[Port]);



//	SetDlgItemText(hwndDisplay, IDC_FILENAME, &FN[Port][0]);
//	SetDlgItemText(hwndDisplay, IDC_MESSAGE, &Message[Port][0]);

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}


*/


/*
VOID ProcessMessage(char * Payload, struct STATIONRECORD * Station)
{
	char MsgDest[10];
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr = Messages;
	char * TextPtr = &Payload[11];
	char * SeqPtr;
	int n = 0;
	char FromCall[10] = "         ";
	struct tm * TM;
	time_t NOW;

	memcpy(FromCall, Station->Callsign, (int)strlen(Station->Callsign));
	memcpy(MsgDest, &Payload[1], 9);
	MsgDest[9] = 0;

	SeqPtr = strchr(TextPtr, '{');

	if (SeqPtr)
	{
		*(SeqPtr++) = 0;
		if(strlen(SeqPtr) > 6)
			SeqPtr[7] = 0;		
	}

	if (_memicmp(TextPtr, "ack", 3) == 0)
	{
		// Message Ack. See if for one of our messages

		ptr = OutstandingMsgs;

		if (ptr == 0)
			return;

		do
		{
			if (strcmp(ptr->FromCall, MsgDest) == 0
				&& strcmp(ptr->ToCall, FromCall) == 0
				&& strcmp(ptr->Seq, &TextPtr[3]) == 0)
			{
				// Message is acked

				ptr->Retries = 0;
				ptr->Acked = TRUE;
//				if (hMsgsOut)
//					UpdateTXMessageLine(hMsgsOut, n, ptr);

				return;
			}
			ptr = ptr->Next;
			n++;

		} while (ptr);
	
		return;
	}

	Message = malloc(sizeof(struct APRSMESSAGE));
	memset(Message, 0, sizeof(struct APRSMESSAGE));
	strcpy(Message->FromCall, Station->Callsign);
	strcpy(Message->ToCall, MsgDest);

	if (SeqPtr)
	{
		strcpy(Message->Seq, SeqPtr);

		// If a REPLY-ACK Seg, copy to LastRXSeq, and see if it acks a message

		if (SeqPtr[2] == '}')
		{
			struct APRSMESSAGE * ptr1;
			int nn = 0;

			strcpy(Station->LastRXSeq, SeqPtr);

			ptr1 = OutstandingMsgs;

			while (ptr1)
			{
				if (strcmp(ptr1->FromCall, MsgDest) == 0
					&& strcmp(ptr1->ToCall, FromCall) == 0
					&& memcmp(&ptr1->Seq, &SeqPtr[3], 2) == 0)
				{
					// Message is acked

					ptr1->Acked = TRUE;
					ptr1->Retries = 0;
//					if (hMsgsOut)
//						UpdateTXMessageLine(hMsgsOut, nn, ptr);
					
					break;
				}
				ptr1 = ptr1->Next;
				nn++;
			}
		}
		else
		{
			// Station is not using reply-ack - set to send simple numeric sequence (workround for bug in APRS Messanger
		
			Station->SimpleNumericSeq = TRUE;
		}
	}

	if (strlen(TextPtr) > 100)
		TextPtr[100] = 0;

	strcpy(Message->Text, TextPtr);
		
	NOW = time(NULL);

	if (DefaultLocalTime)
		TM = localtime(&NOW);
	else
		TM = gmtime(&NOW);
					
	sprintf(Message->Time, "%.2d:%.2d", TM->tm_hour, TM->tm_min);

	if (_stricmp(MsgDest, APRSCall) == 0 && SeqPtr)	// ack it if it has a sequence
	{
		// For us - send an Ack

		char ack[30];

		int n = sprintf(ack, ":%-9s:ack%s", Message->FromCall, Message->Seq);
		PutAPRSMessage(ack, n);
	}

	if (ptr == NULL)
	{
		Messages = Message;
	}
	else
	{
		n++;
		while(ptr->Next)
		{
			ptr = ptr->Next;
			n++;
		}
		ptr->Next = Message;
	}

	if (strcmp(MsgDest, APRSCall) == 0)			// to me?
	{
	}
}

*/

VOID APRSSecTimer()
{

	// Check Message Retries

	struct APRSMESSAGE * ptr = SMEM->OutstandingMsgs;
	int n = 0;

	if (SendWX)
		SendWeatherBeacon();


	while (ptr)
	{				
		if (ptr->Acked == FALSE)
		{
			if (ptr->Retries)
			{
				ptr->RetryTimer--;
				
				if (ptr->RetryTimer == 0)
				{
					ptr->Retries--;

					if (ptr->Retries)
					{
						// Send Again
						
						char Msg[255];
						APRSHEARDRECORD * STN;

						sprintf(Msg, ":%-9s:%s{%s", ptr->ToCall, ptr->Text, ptr->Seq);

						STN = FindStationInMH(ptr->ToCall);

						if (STN)
							SendAPRSMessage(Msg, STN->rfPort);
						else
						{
							if (memcmp(ptr->ToCall, "SERVER   ", 9))
								SendAPRSMessage(Msg, -1);		// All RF ports unless to SERVER
							SendAPRSMessage(Msg, 0);			// IS
						}
						ptr->RetryTimer = RetryTimer;
					}
				}
			}
		}

		ptr = ptr->Next;
		n++;
	} 
}

double radians(double Degrees)
{
    return M_PI * Degrees / 180;
}
double degrees(double Radians)
{
	return Radians * 180 / M_PI;
}

double Distance(double laa, double loa, double lah, double loh, BOOL KM)
{
	
/*

'Great Circle Calculations.

'dif = longitute home - longitute away


'      (this should be within -180 to +180 degrees)
'      (Hint: This number should be non-zero, programs should check for
'             this and make dif=0.0001 as a minimum)
'lah = latitude of home
'laa = latitude of away

'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
'distance = dis / 180 * pi * ERAD
'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians
*/

	loh = radians(loh); lah = radians(lah);
	loa = radians(loa); laa = radians(laa);

	loh = 60*degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa-loh))) * 1.15077945;
	
	if (KM)
		loh *= 1.60934;
	
	return loh;
}


double myDistance(double laa, double loa, BOOL KM)
{
	double lah, loh;

	GetAPRSLatLon(&lah, &loh);

	return Distance(laa, loa, lah, loh, KM);
}

/*

'Great Circle Calculations.

'dif = longitute home - longitute away


'      (this should be within -180 to +180 degrees)
'      (Hint: This number should be non-zero, programs should check for
'             this and make dif=0.0001 as a minimum)
'lah = latitude of home
'laa = latitude of away

'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
'distance = dis / 180 * pi * ERAD
'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians


	loh = radians(loh); lah = radians(lah);
	loa = radians(loa); laa = radians(laa);

	loh = 60*degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa-loh))) * 1.15077945;
	
	if (KM)
		loh *= 1.60934;
	
	return loh;
}
*/

double Bearing(double lat2, double lon2, double lat1, double lon1)
{
	double dlat, dlon, TC1;

	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;
	
	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);
		
	if (fabs(TC1) > 89.5) if (dlon > 0) return 90; else return 270;

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}

	return 0;
}


double myBearing(double lat2, double lon2)
{
	double lat1, lon1;

	GetAPRSLatLon(&lat1, &lon1);

	return Bearing(lat2, lon2, lat1, lon1);
}
/*



 
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;
	
	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);
		
	if (fabs(TC1) > 89.5) if (dlon > 0) return 90; else return 270;

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}

	return 0;
}
*/

// Weather Data 
	
static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

VOID SendWeatherBeacon()
{
	char Msg[256];
	char DD[3]="";
	char HH[3]="";
	char MM[3]="";
	char Lat[10], Lon[10];
	size_t Len;
	int index;
	char WXMessage[1024];
	char * WXptr;
	char * WXend;
	time_t WXTime;
	time_t now = time(NULL);
	FILE * hFile;
	struct tm * TM;
	struct stat STAT;
 
	WXCounter++;

	if (WXCounter < WXInterval * 60)
		return;

	WXCounter = 0;

//	Debugprintf("BPQAPRS - Trying to open WX file %s", WXFileName);

	if (stat(WXFileName, &STAT))
	{
		Debugprintf("APRS WX File %s stat() failed %d", WXFileName, GetLastError());
		return;
	}

	WXTime = (now - STAT.st_mtime) /60;			// Minutes

	if (WXTime > (3 * WXInterval))
	{
		Debugprintf("APRS Send WX File %s too old - %d minutes", WXFileName, WXTime);
		return;
	}
	
	hFile = fopen(WXFileName, "rb");
	
	if (hFile)
		Len = fread(WXMessage, 1, 1024, hFile); 
	else
	{
		Debugprintf("APRS WX File %s open() failed %d", WXFileName, GetLastError());
		return;
	}

	
	if (Len < 30)
	{
		Debugprintf("BPQAPRS - WX file %s is too short - %d Chars", WXFileName, Len);
		fclose(hFile);
		return;
	}

	WXMessage[Len] = 0;

	// see if wview format

//04-09-13, 2245
//TempIn 23
//TempEx 18
//WindHi 0
//WindAv 0
//WindDr 200
//BarmPs 30167
//HumdIn 56
//HumdEx 100
//RnFall 0.00
//DailyRnFall 0.00

	if (strstr(WXMessage, "TempIn"))
	{
		int Wind  =  0;
		int Gust = 0;
		int Temp = 0;
		int Winddir = 0;
		int Humidity = 0;
		int Raintoday = 0;
		int Rain24hrs = 0;
		int Pressure = 0;

		char * ptr;

		ptr = strstr(WXMessage, "TempEx");
		if (ptr)
			Temp = (int)(atof(ptr + 7) * 1.8) + 32;

		ptr = strstr(WXMessage, "WindHi");
		if (ptr)
			Gust = atoi(ptr + 7);

		ptr = strstr(WXMessage, "WindAv");
		if (ptr)
			Wind = atoi(ptr + 7);

		ptr = strstr(WXMessage, "WindDr");
		if (ptr)
			Winddir = atoi(ptr + 7);

		ptr = strstr(WXMessage, "BarmPs");
		if (ptr)
			Pressure = (int)(atof(ptr + 7) * 0.338638866667);			// Inches to 1/10 millbars

		ptr = strstr(WXMessage, "HumdEx");
		if (ptr)
			Humidity = atoi(ptr + 7);

		ptr = strstr(WXMessage, "RnFall");
		if (ptr)
			Rain24hrs = (int)(atof(ptr + 7) * 100.0);

		ptr = strstr(WXMessage, "DailyRnFall");
		if (ptr)
			Raintoday = (int)(atof(ptr + 12) * 100.0);

		if (Humidity > 99)
			Humidity = 99;
		
		sprintf(WXMessage, "%03d/%03dg%03dt%03dr%03dP%03dp%03dh%02db%05d",
			Winddir, Wind, Gust, Temp, 0, Raintoday, Rain24hrs, Humidity, Pressure);

	}

	WXptr = strchr(WXMessage, 10);

	if (WXptr)
	{
		WXend = strchr(++WXptr, 13);
		if (WXend == 0)
			WXend = strchr(WXptr, 10);
		if (WXend)
			*WXend = 0;
	}
	else
		WXptr = &WXMessage[0];

	// Get DDHHMM from Filetime

	TM = gmtime(&STAT.st_mtime);

	sprintf(DD, "%02d", TM->tm_mday);
	sprintf(HH, "%02d", TM->tm_hour);
	sprintf(MM, "%02d", TM->tm_min);

	GetAPRSLatLonString(Lat, Lon);

	Len = sprintf(Msg, "@%s%s%sz%s/%s_%s%s", DD, HH, MM, Lat, Lon, WXptr, WXComment);

	Debugprintf(Msg);

	for (index = 0; index < MaxBPQPortNo; index++)
	{
		if (WXPort[index])
			SendAPRSMessageEx(Msg, index, WXCall, FALSE);
	}

	fclose(hFile);
}


/*
Jan 22 2012 14:10
123/005g011t031r000P000p000h00b10161

/MITWXN Mitchell IN weather Station N9LYA-3 {UIV32} 
< previous

@221452z3844.42N/08628.33W_203/006g007t032r000P000p000h00b10171
Complete Weather Report Format — with Lat/Long position, no Timestamp
! or = Lat   Sym Table ID   Long   Symbol Code _  Wind Directn/ Speed Weather Data APRS Software   WX Unit uuuu
 1      8          1         9          1                 7                 n            1              2-4
Examples
!4903.50N/07201.75W_220/004g005t077r000p000P000h50b09900wRSW
!4903.50N/07201.75W_220/004g005t077r000p000P000h50b.....wRSW

*/

//	Web Server Code

//	The actual HTTP socket code is in bpq32.dll. Any requests for APRS data are passed in 
//	using a Named Pipe. The request looks exactly like one from a local socket, and the respone is
//	a fully pormatted HTTP packet


#define InputBufferLen 1000


#define MaxSessions 100


HANDLE PipeHandle;

int HTTPPort = 80;
BOOL IPV6 = TRUE;

#define MAX_PENDING_CONNECTS 5

BOOL OpenSockets6();

char HTDocs[MAX_PATH] = "HTML";
char SpecialDocs[MAX_PATH] = "Special Pages";

char SymbolText[192][20] = {

"Police Stn", "No Symbol", "Digi", "Phone", "DX Cluster", "HF Gateway", "Plane sm", "Mob Sat Stn",
"WheelChair", "Snowmobile", "Red Cross", "Boy Scout", "Home", "X", "Red Dot", "Circle (0)", 
"Circle (1)", "Circle (2)", "Circle (3)", "Circle (4)", "Circle (5)", "Circle (6)", "Circle (7)", "Circle (8)", 
"Circle (9)", "Fire", "Campground", "Motorcycle", "Rail Eng.", "Car", "File svr", "HC Future", 

"Aid Stn", "BBS", "Canoe", "No Symbol", "Eyeball", "Tractor", "Grid Squ.", "Hotel", 
"Tcp/ip", "No Symbol", "School", "Usr Log-ON", "MacAPRS", "NTS Stn", "Balloon", "Police", 
"TBD", "Rec Veh'le", "Shuttle", "SSTV", "Bus", "ATV", "WX Service", "Helo", 
"Yacht", "WinAPRS", "Jogger", "Triangle", "PBBS", "Plane lrge", "WX Station", "Dish Ant.", 

"Ambulance", "Bike", "ICP", "Fire Station", "Horse", "Fire Truck", "Glider", "Hospital", 
"IOTA", "Jeep", "Truck", "Laptop", "Mic-E Rptr", "Node", "EOC", "Rover", 
"Grid squ.", "Antenna", "Power Boat", "Truck Stop", "Truck 18wh", "Van", "Water Stn", "XAPRS", 
"Yagi", "Shelter", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "", "",

"Emergency", "No Symbol", "No. Digi", "Bank", "No Symbol", "No. Diam'd", "Crash site", "Cloudy", 
"MEO", "Snow", "Church", "Girl Scout", "Home (HF)", "UnknownPos", "Destination", "No. Circle", 
"No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", 
"Petrol Stn", "Hail", "Park", "Gale Fl", "No Symbol", "No. Car", "Info Kiosk", "Hurricane", 

"No. Box", "Snow blwng", "Coast G'rd", "Drizzle", "Smoke", "Fr'ze Rain", "Snow Shwr", "Haze", 
"Rain Shwr", "Lightning", "Kenwood", "Lighthouse", "No Symbol", "Nav Buoy", "Rocket", "Parking  ", 
"Quake", "Restaurant", "Sat/Pacsat", "T'storm", "Sunny", "VORTAC", "No. WXS", "Pharmacy", 
"No Symbol", "No Symbol", "Wall Cloud", "No Symbol", "No Symbol", "No. Plane", "No. WX Stn", "Rain",

"No. Diamond", "Dust blwng", "No. CivDef", "DX Spot", "Sleet", "Funnel Cld", "Gale", "HAM store",
"No. Blk Box", "WorkZone", "SUV", "Area Locns", "Milepost", "No. Triang", "Circle sm", "Part Cloud",
"No Symbol", "Restrooms", "No. Boat", "Tornado", "No. Truck", "No. Van", "Flooding", "No Symbol",
"Sky Warn", "No Symbol", "Fog", "No Symbol", "No Symbol", "No Symbol", "", ""};

// All Calls (8 per line)

//<td><a href="find.cgi?call=EI7IG-1">EI7IG-1</a></td>
//<td><a href="find.cgi?call=G7TKK-1">G7TKK-1</a></td>
//<td><a href="find.cgi?call=GB7GL-B">GB7GL-B</a></td>
//<td><a href="find.cgi?call=GM1TCN">GM1TCN</a></td>
//<td><a href="find.cgi?call=GM8BPQ">GM8BPQ</a></td>
//<td><a href="find.cgi?call=GM8BPQ-14">GM8BPQ-14</a></td>
//<td><a href="find.cgi?call=LA2VPA-9">LA2VPA-9</a></td>
//<td><a href="find.cgi?call=LA3FIA-10">LA3FIA-10</a></td></tr><tr>
//<td><a href="find.cgi?call=LA6JF-2">LA6JF-2</a></td><td><a href="find.cgi?call=LD4ST">LD4ST</a></td><td><a href="find.cgi?call=M0CHK-7">M0CHK-7</a></td><td><a href="find.cgi?call=M0OZH-7">M0OZH-7</a></td><td><a href="find.cgi?call=MB7UFO-1">MB7UFO-1</a></td><td><a href="find.cgi?call=MB7UN">MB7UN</a></td><td><a href="find.cgi?call=MM0DXE-15">MM0DXE-15</a></td><td><a href="find.cgi?call=PA2AYX-9">PA2AYX-9</a></td></tr><tr>
//<td><a href="find.cgi?call=PA3AQW-5">PA3AQW-5</a></td><td><a href="find.cgi?call=PD1C">PD1C</a></td><td><a href="find.cgi?call=PD5LWD-2">PD5LWD-2</a></td><td><a href="find.cgi?call=PI1ECO">PI1ECO</a></td></tr>


char * DoSummaryLine(struct STATIONRECORD * ptr, int n, int Width)
{
	static char Line2[80];
	int x;
	char XCall[256];
	char * ptr1 = ptr->Callsign;
	char * ptr2 = XCall;

	// Object Names can contain spaces

	while(*ptr1)
	{
		if (*ptr1 == ' ')
		{		
			memcpy(ptr2, "%20", 3);
			ptr2 += 3;
		}
		else
			*(ptr2++) = *ptr1;

		ptr1++;
	}

	*ptr2 = 0;


	// Object Names can contain spaces
	

	sprintf(Line2, "<td><a href=""find.cgi?call=%s"">%s</a></td>",
		XCall, ptr->Callsign);

	x = ++n/Width;
	x = x * Width;

	if (x == n)
		strcat(Line2, "</tr><tr>");

	return Line2;
}

char * DoDetailLine(struct STATIONRECORD * ptr, BOOL LocalTime, BOOL KM)
{
	static char Line[512];
	double Lat = ptr->Lat;
	double Lon = ptr->Lon;
	char NS='N', EW='E';

	char LatString[20], LongString[20], DistString[20], BearingString[20];
	int Degrees;
	double Minutes;
	char Time[80];
	struct tm * TM;
	char XCall[256];

	char * ptr1 = ptr->Callsign;
	char * ptr2 = XCall;

	// Object Names can contain spaces

	while(*ptr1)
	{
		if (*ptr1 == ' ')
		{		
			memcpy(ptr2, "%20", 3);
			ptr2 += 3;
		}
		else
			*(ptr2++) = *ptr1;

		ptr1++;
	}

	*ptr2 = 0;

	
//	if (ptr->ObjState == '_')	// Killed Object
//		return;

	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

	sprintf(Time, "%.2d:%.2d:%.2d", TM->tm_hour, TM->tm_min, TM->tm_sec);

	if (ptr->Lat < 0)
	{
		NS = 'S';
		Lat=-Lat;
	}
	if (Lon < 0)
	{
		EW = 'W';
		Lon=-Lon;
	}

#pragma warning(push)
#pragma warning(disable:4244)

	Degrees = Lat;
	Minutes = Lat * 60.0 - (60 * Degrees);

	sprintf(LatString,"%2d°%05.2f'%c", Degrees, Minutes, NS);
		
	Degrees = Lon;

#pragma warning(pop)

	Minutes = Lon * 60 - 60 * Degrees;

	sprintf(LongString, "%3d°%05.2f'%c",Degrees, Minutes, EW);

	sprintf(DistString, "%6.1f", myDistance(ptr->Lat, ptr->Lon, KM));
	sprintf(BearingString, "%3.0f", myBearing(ptr->Lat, ptr->Lon));
	
	sprintf(Line, "<tr><td align=""left""><a href=""find.cgi?call=%s"">&nbsp;%s%s</a></td><td align=""left"">%s</td><td align=""center"">%s  %s</td><td align=""right"">%s</td><td align=""right"">%s</td><td align=""left"">%s</td></tr>\r\n",
			XCall, ptr->Callsign, 
			(strchr(ptr->Path, '*'))?  "*": "", &SymbolText[ptr->iconRow << 4 | ptr->iconCol][0], LatString, LongString, DistString, BearingString, Time);

	return Line;
}

 
int CompareFN(const void *a, const void *b) 
{
	const struct STATIONRECORD * x = a;
	const struct STATIONRECORD * y = b;

	x = x->Next;
	y = y->Next;

	return strcmp(x->Callsign, y->Callsign);

	/* strcmp functions works exactly as expected from
	comparison function */ 
} 



char * CreateStationList(BOOL RFOnly, BOOL WX, BOOL Mobile, char Objects, int * Count, char * Param, BOOL KM)
{
	char * Line = malloc(100000);
	struct STATIONRECORD * ptr = *StationRecords;
	int n = 0, i;
	struct STATIONRECORD * List[1000];
	int TableWidth = 8;
	BOOL LocalTime = DefaultLocalTime;

	if (strstr(Param, "time=local"))
		LocalTime = TRUE;
	else if (strstr(Param, "time=utc"))
		LocalTime = FALSE;

	Line[0] = 0;
	
	if (Param && Param[0])
	{
		char * Key, *Context;

		Key = strtok_s(Param, "=", &Context);

		if (_stricmp(Key, "width") == 0)
			TableWidth = atoi(Context);

		if (TableWidth == 0)
			TableWidth = 8;
	}

	// Build list of calls

	while (ptr)
	{
		if (ptr->ObjState == Objects && ptr->Lat != 0.0 && ptr->Lon != 0.0)
		{
			if ((WX && (ptr->LastWXPacket[0] == 0)) || (RFOnly && (ptr->LastPort == 0)) ||
				(Mobile && ((ptr->Speed < 0.1) || ptr->LastWXPacket[0] != 0)))
			{
				ptr = ptr->Next;
				continue;
			}

			List[n++] = ptr;

			if (n > 999)
				break;

		}
		ptr = ptr->Next;		
	}

	if (n >  1)
		qsort(List, n, sizeof(void *), CompareFN);

	for (i = 0; i < n; i++)
	{
		if (RFOnly)
			strcat(Line, DoDetailLine(List[i], LocalTime, KM));
		else
			strcat(Line, DoSummaryLine(List[i], i, TableWidth));
	}	
		
	*Count = n;

	return Line;

}

char * APRSLookupKey(struct APRSConnectionInfo * sockptr, char * Key, BOOL KM)
{
	struct STATIONRECORD * stn = sockptr->SelCall;

	if (strcmp(Key, "##MY_CALLSIGN##") == 0)
		return _strdup(LoppedAPRSCall);

	if (strcmp(Key, "##CALLSIGN##") == 0)
		return _strdup(sockptr->Callsign);

	if (strcmp(Key, "##CALLSIGN_NOSSID##") == 0)
	{
		char * Call = _strdup(sockptr->Callsign);
		char * ptr = strchr(Call, '-');
		if (ptr)
			*ptr = 0;
		return Call;
	}

	if (strcmp(Key, "##MY_WX_CALLSIGN##") == 0)
		return _strdup(LoppedAPRSCall);

	if (strcmp(Key, "##MY_BEACON_COMMENT##") == 0)
		return _strdup(StatusMsg);

	if (strcmp(Key, "##MY_WX_BEACON_COMMENT##") == 0)
		return _strdup(WXComment);

	if (strcmp(Key, "##MILES_KM##") == 0)
		if (KM)
			return _strdup("KM");
		else
			return _strdup("Miles");

	if (strcmp(Key, "##EXPIRE_TIME##") == 0)
	{
		char val[80];
		sprintf(val, "%d", ExpireTime);
		return _strdup(val);
	}

	if (strcmp(Key, "##LOCATION##") == 0)
	{
		char val[80];
		double Lat = sockptr->SelCall->Lat;
		double Lon = sockptr->SelCall->Lon;
		char NS='N', EW='E';
		char LatString[20];
		int Degrees;
		double Minutes;
	
		if (Lat < 0)
		{
			NS = 'S';
			Lat=-Lat;
		}
		if (Lon < 0)
		{
			EW = 'W';
			Lon=-Lon;
		}

#pragma warning(push)
#pragma warning(disable:4244)

		Degrees = Lat;
		Minutes = Lat * 60.0 - (60 * Degrees);

		sprintf(LatString,"%2d°%05.2f'%c",Degrees, Minutes, NS);
		
		Degrees = Lon;

#pragma warning(pop)

		Minutes = Lon * 60 - 60 * Degrees;

		sprintf(val,"%s %3d°%05.2f'%c", LatString, Degrees, Minutes, EW);

		return _strdup(val);
	}

	if (strcmp(Key, "##LOCDDMMSS##") == 0)
	{
		char val[80];
		double Lat = sockptr->SelCall->Lat;
		double Lon = sockptr->SelCall->Lon;
		char NS='N', EW='E';
		char LatString[20];
		int Degrees;
		double Minutes;

		// 48.45.18N, 002.18.37E
			
		if (Lat < 0)
		{
			NS = 'S';
			Lat=-Lat;
		}
		if (Lon < 0)
		{
			EW = 'W';
			Lon=-Lon;
		}

#pragma warning(push)
#pragma warning(disable:4244)

		Degrees = Lat;
		Minutes = Lat * 60.0 - (60 * Degrees);
//		IntMins = Minutes;
//		Seconds = Minutes * 60.0 - (60 * IntMins);

		sprintf(LatString,"%2d.%05.2f%c",Degrees, Minutes, NS);
		
		Degrees = Lon;
		Minutes = Lon * 60.0 - 60 * Degrees;
//		IntMins = Minutes;
//		Seconds = Minutes * 60.0 - (60 * IntMins);

#pragma warning(pop)

		sprintf(val,"%s, %03d.%05.2f%c", LatString, Degrees, Minutes, EW);

		return _strdup(val);
	}

	if (strcmp(Key, "##LATLON##") == 0)
	{
		char val[80];
		double Lat = sockptr->SelCall->Lat;
		double Lon = sockptr->SelCall->Lon;
		char NS='N', EW='E';

		// 58.5, -6.2

		sprintf(val,"%f, %f", Lat, Lon);
		return _strdup(val);
	}

	if (strcmp(Key, "##STATUS_TEXT##") == 0)
		return _strdup(stn->Status);
	
	if (strcmp(Key, "##LASTPACKET##") == 0)
		return _strdup(stn->LastPacket);


	if (strcmp(Key, "##LAST_HEARD##") == 0)
	{
		char Time[80];
		struct tm * TM;
		time_t Age = time(NULL) - stn->TimeLastUpdated;

		TM = gmtime(&Age);

		sprintf(Time, "%.2d:%.2d:%.2d", TM->tm_hour, TM->tm_min, TM->tm_sec);

		return _strdup(Time);
	}

	if (strcmp(Key, "##FRAME_HEADER##") == 0)
		return _strdup(stn->Path);

	if (strcmp(Key, "##FRAME_INFO##") == 0)
		return _strdup(stn->LastWXPacket);
	
	if (strcmp(Key, "##BEARING##") == 0)
	{
		char val[80];

		sprintf(val, "%03.0f", myBearing(sockptr->SelCall->Lat, sockptr->SelCall->Lon));
		return _strdup(val);
	}

	if (strcmp(Key, "##COURSE##") == 0)
	{
		char val[80];

		sprintf(val, "%03.0f", stn->Course);
		return _strdup(val);
	}

	if (strcmp(Key, "##SPEED_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%5.1f", stn->Speed);
		return _strdup(val);
	}

	if (strcmp(Key, "##DISTANCE##") == 0)
	{
		char val[80];

		sprintf(val, "%5.1f", myDistance(sockptr->SelCall->Lat, sockptr->SelCall->Lon, KM));
		return _strdup(val);
	}



	if (strcmp(Key, "##WIND_DIRECTION##") == 0)
	{
		char val[80];

		sprintf(val, "%03d", sockptr->WindDirn);
		return _strdup(val);
	}

	if (strcmp(Key, "##WIND_SPEED_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->WindSpeed);
		return _strdup(val);
	}

	if (strcmp(Key, "##WIND_GUST_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->WindGust);
		return _strdup(val);
	}

	if (strcmp(Key, "##TEMPERATURE_F##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->Temp);
		return _strdup(val);
	}

	if (strcmp(Key, "##HUMIDITY##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->Humidity);
		return _strdup(val);
	}

	if (strcmp(Key, "##PRESSURE_HPA##") == 0)
	{
		char val[80];

		sprintf(val, "%05.1f", sockptr->Pressure /10.0);
		return _strdup(val);
	}

	if (strcmp(Key, "##RAIN_TODAY_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainToday /100.0);
		return _strdup(val);
	}


	if (strcmp(Key, "##RAIN_24_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainLastDay /100.0);
		return _strdup(val);
	}


	if (strcmp(Key, "##RAIN_HOUR_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainLastHour /100.0);
		return _strdup(val);
	}

	if (strcmp(Key, "##MAP_LAT_LON##") == 0)
	{
		char val[256];

		sprintf(val, "%f,%f", stn->Lat, stn->Lon);
		return _strdup(val);
	}

	if (strcmp(Key, "##SYMBOL_DESCRIPTION##") == 0)
		return _strdup(&SymbolText[stn->iconRow << 4 | stn->iconCol][0]);


/*
##WIND_SPEED_MS## - wind speed metres/sec
##WIND_SPEED_KMH## - wind speed km/hour
##WIND_GUST_MPH## - wind gust miles/hour
##WIND_GUST_MS## - wind gust metres/sec
##WIND_GUST_KMH## - wind gust km/hour
##WIND_CHILL_F## - wind chill F
##WIND_CHILL_C## - wind chill C
##TEMPERATURE_C## - temperature C
##DEWPOINT_F## - dew point temperature F
##DEWPOINT_C## - dew point temperature C
##PRESSURE_IN## - pressure inches of mercury
##PRESSURE_HPA## - pressure hPa (mb)
##RAIN_HOUR_MM## - rain in last hour mm
##RAIN_TODAY_MM## - rain today mm
##RAIN_24_MM## - rain in last 24 hours mm
##FRAME_HEADER## - frame header of the last posit heard from the station
##FRAME_INFO## - information field of the last posit heard from the station
##MAP_LARGE_SCALE##" - URL of a suitable large scale map on www.vicinity.com
##MEDIUM_LARGE_SCALE##" - URL of a suitable medium scale map on www.vicinity.com
##MAP_SMALL_SCALE##" - URL of a suitable small scale map on www.vicinity.com
##MY_LOCATION## - 'Latitude', 'Longitude' in 'Station Setup'
##MY_STATUS_TEXT## - status text configured in 'Status Text'
##MY_SYMBOL_DESCRIPTION## - 'Symbol' that would be shown for our station in 'Station List'
##HIT_COUNTER## - The number of times the page has been accessed
##DOCUMENT_LAST_CHANGED## - The date/time the page was last edited

##FRAME_HEADER## - frame header of the last posit heard from the station
##FRAME_INFO## - information field of the last posit heard from the station

*/
	return NULL;
}

VOID APRSProcessSpecialPage(struct APRSConnectionInfo * sockptr, char * Buffer, int FileSize, char * StationTable, int Count, BOOL WX, BOOL KM)
{
	// replaces ##xxx### constructs with the requested data

	char * NewMessage = malloc(250000);
	char * ptr1 = Buffer, * ptr2, * ptr3, * ptr4, * NewPtr = NewMessage;
	size_t PrevLen;
	size_t BytesLeft = FileSize;
	size_t NewFileSize = FileSize;
	char * StripPtr = ptr1;
	int HeaderLen;
	char Header[256];

	if (WX && sockptr->SelCall && sockptr->SelCall->LastWXPacket)
	{
		DecodeWXReport(sockptr, sockptr->SelCall->LastWXPacket);
	}

	// strip comments blocks

	while (ptr4 = strstr(ptr1, "<!--"))
	{
		ptr2 = strstr(ptr4, "-->");
		if (ptr2)
		{
			PrevLen = (ptr4 - ptr1);
			memcpy(StripPtr, ptr1, PrevLen);
			StripPtr += PrevLen;
			ptr1 = ptr2 + 3;
			BytesLeft = FileSize - (ptr1 - Buffer);
		}
	}


	memcpy(StripPtr, ptr1, BytesLeft);
	StripPtr += BytesLeft;

	BytesLeft = StripPtr - Buffer;

	FileSize = (int)BytesLeft;
	NewFileSize = FileSize;
	ptr1 = Buffer;
	ptr1[FileSize] = 0;

loop:
	ptr2 = strstr(ptr1, "##");

	if (ptr2)
	{
		PrevLen = (ptr2 - ptr1);			// Bytes before special text
		
		ptr3 = strstr(ptr2+2, "##");

		if (ptr3)
		{
			char Key[80] = "";
			size_t KeyLen;
			char * NewText;
			size_t NewTextLen;

			ptr3 += 2;
			KeyLen = ptr3 - ptr2;

			if (KeyLen < 80)
				memcpy(Key, ptr2, KeyLen);

			if (strcmp(Key, "##STATION_TABLE##") == 0)
			{
				NewText = _strdup(StationTable);
			}
			else
			{
				if (strcmp(Key, "##TABLE_COUNT##") == 0)
				{
					char val[80];
					sprintf(val, "%d", Count);
					NewText = _strdup(val);
				}
				else
					NewText = APRSLookupKey(sockptr, Key, KM);
			}
			
			if (NewText)
			{
				NewTextLen = strlen(NewText);
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
			BytesLeft = Buffer + FileSize - ptr3;
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
	
	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", (int)NewFileSize);
	send(sockptr->sock, Header, HeaderLen, 0); 
	send(sockptr->sock, NewMessage, (int)NewFileSize, 0); 

	free (NewMessage);
	free(StationTable);
	
	return;
}

VOID APRSSendMessageFile(struct APRSConnectionInfo * sockptr, char * FN)
{
	int FileSize = 0;
	char * MsgBytes = 0;
	char * SaveMsgBytes = 0;

	char MsgFile[MAX_PATH];
	FILE * hFile;
	BOOL Special = FALSE;
	int HeaderLen;
	char Header[256];
	char * Param = 0;
	struct stat STAT;
	int Sent;
	char * ptr;

	if (strchr(FN, '?'))
		strtok_s(FN, "?", &Param);

	UndoTransparency(FN);

	if (strstr(FN, ".."))
	{
		FN[0] = '/';
		FN[1] = 0;
	}

	if (strcmp(FN, "/") == 0)
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s/%s/index.html", BPQDirectory, APRSDir, SpecialDocs);
	else
		sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s/%s%s", BPQDirectory, APRSDir, SpecialDocs, &FN[5]);
	
	hFile = fopen(MsgFile, "rb");

	if (hFile == NULL)
	{
		// Try normal pages


		if (strcmp(FN, "/") == 0)
			sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s/%s/index.html", BPQDirectory, APRSDir, HTDocs);
		else
			sprintf_s(MsgFile, sizeof(MsgFile), "%s/%s/%s%s", BPQDirectory,APRSDir, HTDocs, &FN[5]);
	
		// My standard page set is now hard coded



		MsgBytes = SaveMsgBytes = GetStandardPage(&FN[6], &FileSize);

		if (MsgBytes)
		{
			if (FileSize == 0)
				FileSize = strlen(MsgBytes);
		}
		else
		{
			hFile = fopen(MsgFile, "rb");

			if (hFile == NULL)
			{
				HeaderLen = sprintf(Header, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
				send(sockptr->sock, Header, HeaderLen, 0); 
				return;
			}
		}
	}
	else
		Special = TRUE;

	if (FileSize == 0)				// Not from internal template
	{
		if (stat(MsgFile, &STAT) == 0)
			FileSize = STAT.st_size;

		MsgBytes = SaveMsgBytes = malloc(FileSize+1);

		fread(MsgBytes, 1, FileSize, hFile); 

		fclose(hFile);
	}

	// if HTML file, look for ##...## substitutions

	if ((strstr(FN, "htm" ) || strstr(FN, "HTM")) &&  strstr(MsgBytes, "##" ))
	{
		// Build Station list, depending on URL
	
		int Count = 0;
		BOOL RFOnly = !(strstr(_strlwr(FN), "rf") == NULL);		// Leaves FN in lower case
		BOOL WX =!(strstr(FN, "wx") == NULL);
		BOOL Mobile = !(strstr(FN, "mobile") == NULL);
		char Objects = (strstr(FN, "obj"))? '*' :0;
		char * StationList;
		BOOL KM = DefaultDistKM;

		if (Param == 0)
			Param ="";
		else
			_strlwr(Param);
		
		if (strstr(Param, "dist=km"))
			KM = TRUE;
		else if (strstr(Param, "dist=miles"))
			KM = FALSE;
	

		StationList = CreateStationList(RFOnly, WX, Mobile, Objects, &Count, Param, KM);

		APRSProcessSpecialPage(sockptr, MsgBytes, FileSize, StationList, Count, WX, KM); 
		free (MsgBytes);
		return;			// ProcessSpecial has sent the reply
	}

	ptr = FN;

	while (strchr(ptr, '.'))
	{
		ptr = strchr(ptr, '.');
		++ptr;
	}

	if (_stricmp(ptr, "jpg") == 0 || _stricmp(ptr, "jpeg") == 0 || _stricmp(ptr, "png") == 0 || _stricmp(ptr, "gif") == 0 || _stricmp(ptr, "ico") == 0)
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: image\r\n\r\n", FileSize);
	else
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", FileSize);
	
	send(sockptr->sock, Header, HeaderLen, 0); 
	
	Sent = send(sockptr->sock, MsgBytes, FileSize, 0);
//	printf("Send %d %d\n", FileSize, Sent); 

	while (Sent < FileSize)
	{
		FileSize -= Sent;
		MsgBytes += Sent;
		Sent = send(sockptr->sock, MsgBytes, FileSize, 0);
//		printf("Send %d %d\n", FileSize, Sent); 
		if (Sent == -1)
		{
			Sleep(10);
			Sent = 0;
		}
	}

	free (SaveMsgBytes);
}

char WebHeader[] = "<HTML><HEAD><meta http-equiv=\"expires\" content=\"-1\">"
	"<meta http-equiv=\"pragma\" content=\"no-cache\">"
	"<TITLE>APRS Messaging</TITLE></HEAD>"
	"<BODY alink=\"#008000\" bgcolor=\"#F5F5DC\" link=\"#0000FF\" vlink=\"#000080\"  background=/background.jpg>"
	"<table  align=center border=2 cellpadding=2 cellspacing=2 bgcolor=white><tr>"
	"<td align=center><a href=/aprs.html>APRS Map</a></td>"
	"<td align=center><a href=/aprs/msgs>Received Messages</a></td>"
	"<td align=center><a href=/aprs/txmsgs>Sent Messages</a></td>"
	"<td align=center><a href=/aprs/msgs/entermsg>Send Message</a></td>"
	"<td align=center><a href=/aprs/all.html>Station Pages</a></td>"
	"<td align=center><a href=/Node/NodeMenu.html>Return to Node Pages</a></td>"
	"</tr></table>"
	"<center><h2>%s's Messages</h2><TABLE BORDER=\"3\" CELLSPACING=\"2\" CELLPADDING=\"1\">"
	"<tr><td>From</td><td>To</td><td>Seq</td><td>Time</td><td>&nbsp;</td><td>Message</td></tr>";

char WebTXHeader[] = "<HTML><HEAD><meta http-equiv=\"expires\" content=\"-1\">"
	"<meta http-equiv=\"pragma\" content=\"no-cache\">"
	"<TITLE>APRS Messaging</TITLE></HEAD>"
	"<BODY alink=\"#008000\" bgcolor=\"#F5F5DC\" link=\"#0000FF\" vlink=\"#000080\"  background=/background.jpg>"
	"<table align=center border=2 cellpadding=2 cellspacing=2 bgcolor=white><tr>"
	"<td align=center><a href=/aprs.html>APRS Map</a></td>"
	"<td align=center><a href=/aprs/msgs>Received Messages</a></td>"
	"<td align=center><a href=/aprs/txmsgs>Sent Messages</a></td>"
	"<td align=center><a href=/aprs/msgs/entermsg>Send Message</a></td>"
	"<td align=center><a href=/aprs/all.html>Station Pages</a></td>"
	"<td align=center><a href=/Node/NodeMenu.html>Return to Node Pages</a></td>"
	"</tr></table>"
	"<center><h2>Message Sent by %s</h2><TABLE BORDER=\"3\" CELLSPACING=\"2\" CELLPADDING=\"1\">"
	"<tr><td>To</td><td>Seq</td><td>Time</td><td>State</td><td>message</td></tr>";

char WebLine[] = "<tr bgcolor=\"#ffcccc\"><td>%s </td><td> %s </td><td> %s </td><td> %s</td><td>"
	"<a href=\"entermsg?tocall=%s&fromcall=%s\">Reply</a></td><td> %s</td></tr>";

char WebTXLine[] = "<tr bgcolor=\"#ffcccc\">"
	"<td>%s </td><td> %s </td><td> %s </td><td> %s </td><td> %s</td></tr>";


char WebTrailer[] = "</table></BODY></HTML>";

char SendMsgPage[] = "<html><head><title>BPQ32 APRS Messaging</title></head><body background=\"/background.jpg\">"
	"<center><h2>APRS Message Input</h1>"
	"<form method=post action=/APRS/Msgs/SendMsg>"
	"<table align=center  bgcolor=white>"
	"<tr><td>To</td><td><input type=text name=call tabindex=1 size=10 maxlength=12 value=\"%s\"/></td></tr>" 
	"<tr><td>Message</td><td><input type=text name=message tabindex=2 size=80 maxlength=100 /></td></tr></table>"  
	"<p align=center><input type=submit value=Submit /><input type=submit value=Cancel name=Cancel /></form>";

char APRSIndexPage[] = "<html><head><title>BPQ32 Web Server APRS Pages</title></head>"
	"<body background=/background.jpg><P align=center>"
	"<h2 align=center>BPQ32 APRS Server</h2><P align=center>"
	"<table border=2 cellpadding=2 cellspacing=2 bgcolor=white><tr>"
	"<td align=center><a href=/aprs.html>APRS Map</a></td>"
	"<td align=center><a href=/aprs/msgs>Received Messages</a></td>"
	"<td align=center><a href=/aprs/txmsgs>Sent Messages</a></td>"
	"<td align=center><a href=/aprs/msgs/entermsg>Send Message</a></td>"
	"<td align=center><a href=/aprs/all.html>Station Pages</a></td>"
	"<td align=center><a href=/Node/NodeMenu.html>Return to Node Pages</a></td>"
	"</tr></table>%s</body></html>";

extern char Tail[];

VOID APRSProcessHTTPMessage(SOCKET sock, char * MsgPtr,	BOOL LOCAL, BOOL COOKIE)
{
	int InputLen = 0;
	int OutputLen = 0;
   	char * URL;
	char * ptr;
	struct APRSConnectionInfo CI;
	struct APRSConnectionInfo * sockptr = &CI;
	char Key[12] = "";
	char OutBuffer[100000];
	char Header[1024];
	int HeaderLen = 0;

	memset(&CI, 0, sizeof(CI));

	sockptr->sock = sock;

	if (memcmp(MsgPtr, "POST" , 3) == 0)
	{
		char * To;
		char * Msg = "";

		URL = &MsgPtr[5];

		ptr = strstr(URL, "\r\n\r\n");

		if (ptr)
		{
			char * param, * context;

			UndoTransparency(ptr);

			param = strtok_s(ptr + 4, "&", &context);
			
			while (param)
			{
				char * val = strlop(param, '=');

				if (val)
				{
					if (_stricmp(param, "call") == 0)
						To = _strupr(val);
					else if (_stricmp(param, "message") == 0)
						Msg = val;
					else if (_stricmp(param, "Cancel") == 0)
					{
						
						// Return APRS Index Page

						OutputLen = sprintf(OutBuffer, APRSIndexPage, "<br><br><br><br><h2 align=center>Message Cancelled</h2>");
						HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
						send(sockptr->sock, Header, HeaderLen, 0); 
						send(sockptr->sock, OutBuffer, OutputLen, 0); 

						return;
					}
				}
					
				param = strtok_s(NULL,"&", &context);
			}

			strlop(To, ' ');

			if (strlen(To) < 2)
			{
				OutputLen = sprintf(OutBuffer, SendMsgPage, To);
				OutputLen += sprintf(&OutBuffer[OutputLen], "<br><br><h2 align=center>Invalid Callsign</h2>");
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
				send(sockptr->sock, Header, HeaderLen, 0); 
				send(sockptr->sock, OutBuffer, OutputLen, 0); 

				return;
			}

			if (Msg[0] == 0)
			{
				OutputLen = sprintf(OutBuffer, SendMsgPage, To);
				OutputLen += sprintf(&OutBuffer[OutputLen], "<br><br><h2 align=center>No Message</h2>");
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
				send(sockptr->sock, Header, HeaderLen, 0); 
				send(sockptr->sock, OutBuffer, OutputLen, 0); 

				return;
			}

			// Send APRS Messsage

			if (strlen(Msg) > 100)
					Msg[100] = 0;

			InternalSendAPRSMessage(Msg, To);

			OutputLen = sprintf(OutBuffer, APRSIndexPage, "<br><br><br><br><h2 align=center>Message Sent</h2>");
			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
			send(sockptr->sock, Header, HeaderLen, 0); 
			send(sockptr->sock, OutBuffer, OutputLen, 0); 

			return;
		}
	}

	URL = &MsgPtr[4];

	ptr = strstr(URL, " HTTP");

	if (ptr)
		*ptr = 0;

	if (_stricmp(URL, "/APRS") == 0)
	{
		// Return APRS Index Page

		OutputLen = sprintf(OutBuffer, APRSIndexPage, "");
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
		send(sockptr->sock, Header, HeaderLen, 0); 
		send(sockptr->sock, OutBuffer, OutputLen, 0); 

		return;
	}


	if (_memicmp(URL, "/aprs/msgs/entermsg", 19) == 0 || _memicmp(URL, "/aprs/entermsg", 14) == 0)
	{
		char * To = strchr(URL, '=');

		if (LOCAL == FALSE && COOKIE == FALSE)
		{
			//	Send Not Authorized

			OutputLen = sprintf(OutBuffer, APRSIndexPage, "<br><B>Not authorized - please return to Node Menu and sign in</B>");
			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", (int)(OutputLen + strlen(Tail)));
			send(sock, Header, HeaderLen, 0);
			send(sock, OutBuffer, OutputLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);
			return;
		}


		if (To)
		{
			To++;
			UndoTransparency(To);
			strlop(To, '&');
		}
		else
			To = "";

		OutputLen = sprintf(OutBuffer, SendMsgPage, To);
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
		send(sockptr->sock, Header, HeaderLen, 0); 
		send(sockptr->sock, OutBuffer, OutputLen, 0); 

		return;
		
	}
	
	else if (_memicmp(URL, "/aprs/msgs", 10) == 0)
	{	
		// Return Messages Received Page

			struct APRSMESSAGE * ptr = SMEM->Messages;
			int n = 0;
			char BaseCall[10];
			char BaseFrom[10];
			char * MsgCall = LoppedAPRSCall;
			BOOL OnlyMine = TRUE;
			BOOL AllSSID = TRUE;
			BOOL OnlySeq = FALSE;
			BOOL ShowBulls = TRUE;

			// Parse parameters

			// ?call=g8bpq&bulls=true&seqonly=true&onlymine=true

			char * params = strchr(URL, '?');

			if (params)
			{
				char * param, * context;

				param = strtok_s(++params, "&", &context);

				while (param)
				{
					char * val = strlop(param, '=');

					if (val)
					{
						strlop(val, ' ');
						if (_stricmp(param, "call") == 0)
							MsgCall = _strupr(val);
						else if (_stricmp(param, "bulls") == 0)
							ShowBulls = !_stricmp(val, "true");
						else if (_stricmp(param, "onlyseq") == 0)
							OnlySeq = !_stricmp(val, "true");
						else if (_stricmp(param, "onlymine") == 0)
							OnlyMine = !_stricmp(val, "true");
						else if (_stricmp(param, "AllSSID") == 0)
							AllSSID = !_stricmp(val, "true");
					}
					param = strtok_s(NULL,"&", &context);
				}
			}
			if (AllSSID)
			{
				memcpy(BaseCall, MsgCall, 10);
				strlop(BaseCall, '-');
			}

			OutputLen = sprintf(OutBuffer, WebHeader, MsgCall, MsgCall);

			while (ptr)
			{
					char ToLopped[11] = "";
					memcpy(ToLopped, ptr->ToCall, 10);
					strlop(ToLopped, ' ');

					if (memcmp(ToLopped, "BLN", 3) == 0)
						if (ShowBulls == TRUE)
							goto wantit;

					if (strcmp(ToLopped, MsgCall) == 0)			//  to me?
						goto wantit;

					if (strcmp(ptr->FromCall, MsgCall) == 0)			//  to me?
						goto wantit;

					if (AllSSID)
					{
						memcpy(BaseFrom, ToLopped, 10);
						strlop(BaseFrom, '-');

						if (strcmp(BaseFrom, BaseCall) == 0)
							goto wantit;

						memcpy(BaseFrom, ptr->FromCall, 10);
						strlop(BaseFrom, '-');

						if (strcmp(BaseFrom, BaseCall) == 0)
							goto wantit;

					}

					if (OnlyMine == FALSE)		// Want All
						if (OnlySeq == FALSE || ptr->Seq[0] != 0)
							goto wantit;
			
					// ignore

					ptr = ptr->Next;
					continue;
	wantit:
					OutputLen += sprintf(&OutBuffer[OutputLen], WebLine,
						ptr->FromCall, ptr->ToCall, ptr->Seq, ptr->Time,
						ptr->FromCall, ptr->ToCall, ptr->Text);

					ptr = ptr->Next;

					if (OutputLen > 99000)
						break;

			}

			OutputLen += sprintf(&OutBuffer[OutputLen], "%s", WebTrailer);

			HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, OutBuffer, OutputLen);

			return;
		
	}

	else if (_memicmp(URL, "/aprs/txmsgs", 12) == 0)
	{	
		// Return Messages Received Page

		struct APRSMESSAGE * ptr = SMEM->OutstandingMsgs;
		char * MsgCall = LoppedAPRSCall;

		char Retries[10];


		OutputLen = sprintf(OutBuffer, WebTXHeader, MsgCall, MsgCall);

		while (ptr)
		{
			char ToLopped[11] = "";
		
			if (ptr->Acked)
				strcpy(Retries, "A");
			else if (ptr->Retries == 0)
				strcpy(Retries, "F");
			else
				sprintf(Retries, "%d", ptr->Retries);

				memcpy(ToLopped, ptr->ToCall, 10);
				strlop(ToLopped, ' ');

				OutputLen += sprintf(&OutBuffer[OutputLen], WebTXLine,
					ptr->ToCall, ptr->Seq, ptr->Time, Retries, ptr->Text);
				ptr = ptr->Next;

				if (OutputLen > 99000)
					break;

			}

			OutputLen += sprintf(&OutBuffer[OutputLen], "%s", WebTrailer);

			HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", OutputLen);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, OutBuffer, OutputLen);

			return;
		
	}


	if (_memicmp(URL, "/aprs/find.cgi?call=", 20) == 0)
	{
		// return Station details

		char * Call = &URL[20];
		BOOL RFOnly, WX, Mobile, Object = FALSE;
		struct STATIONRECORD * stn;
		char * Referrer = strstr(ptr + 1, "Referer:");

		// Undo any % transparency in call

		char * ptr1 = Call;
		char * ptr2 = Key;
		char c;

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

		if (Referrer)
		{
			ptr = strchr(Referrer, 13);
			if (ptr)
			{
				*ptr = 0;
				RFOnly = !(strstr(Referrer, "rf") == NULL);
				WX = !(strstr(Referrer, "wx") == NULL);
				Mobile = !(strstr(Referrer, "mobile") == NULL);
				Object = !(strstr(Referrer, "obj") == NULL);

				if (WX)
					strcpy(URL, "/aprs/infowx_call.html");
				else if (Mobile)
					strcpy(URL, "/aprs/infomobile_call.html");
				else if (Object)
					strcpy(URL, "/aprs/infoobj_call.html");
				else
					strcpy(URL, "/aprs/info_call.html");
			}
		}

		if (Object)
		{
			// Name is space padded, and could have embedded spaces
				
			int Keylen = (int)strlen(Key);
				
			if (Keylen < 9)
				memset(&Key[Keylen], 32, 9 - Keylen);
		}			
			
		stn = FindStation(Key, FALSE);

		if (stn == NULL)
			strcpy(URL, "/aprs/noinfo.html");
		else
			sockptr->SelCall = stn;
	}

	
	strcpy(sockptr->Callsign, Key);

	APRSSendMessageFile(sockptr, URL);

	return;
}

// Code for handling APRS messages within BPQ32/LinBPQ instead of GUI


int ProcessMessage(char * Payload, struct STATIONRECORD * Station)
{
	char MsgDest[10];
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr = SMEM->Messages;
	char * TextPtr = &Payload[11];
	char * SeqPtr;
	int n = 0;
	char FromCall[10] = "         ";
	struct tm * TM;
	time_t NOW;
	char noSeq[] = "";
	int ourMessage = 0;

	memcpy(FromCall, Station->Callsign, strlen(Station->Callsign));
	memcpy(MsgDest, &Payload[1], 9);
	MsgDest[9] = 0;

	if (strcmp(MsgDest, CallPadded) == 0)		// to me?
	{
		SMEM->NeedRefresh = 255;				// Flag to control Msg popup
		ourMessage = 1;
	}
	else
		SMEM->NeedRefresh = 1;

	SeqPtr = strchr(TextPtr, '{');

	if (SeqPtr)
	{
		*(SeqPtr++) = 0;
		if(strlen(SeqPtr) > 6)
			SeqPtr[7] = 0;	
	}
	else
		SeqPtr = noSeq;

	if (_memicmp(TextPtr, "ack", 3) == 0)
	{
		// Message Ack. See if for one of our messages

		ptr = SMEM->OutstandingMsgs;

		if (ptr == 0)
			return ourMessage;

		do
		{
			if (strcmp(ptr->FromCall, MsgDest) == 0
				&& strcmp(ptr->ToCall, FromCall) == 0
				&& strcmp(ptr->Seq, &TextPtr[3]) == 0)
			{
				// Message is acked

				ptr->Retries = 0;
				ptr->Acked = TRUE;

				return ourMessage;
			}
			ptr = ptr->Next;
			n++;

		} while (ptr);
	
		return ourMessage;
	}

	// See if we already have this message

	ptr = SMEM->Messages;

	while(ptr)
	{
		if (strcmp(ptr->ToCall, MsgDest) == 0
			&& strcmp(ptr->FromCall, FromCall) == 0
			&& strcmp(ptr->Seq, SeqPtr) == 0
			&& strcmp(ptr->Text, TextPtr) == 0)
		
			// Duplicate

			return ourMessage;

		ptr = ptr->Next;
	}

	Message = APRSGetMessageBuffer();

	if (Message == NULL)
		return ourMessage;

	memset(Message, 0, sizeof(struct APRSMESSAGE));
	memset(Message->FromCall, ' ', 9);
	memcpy(Message->FromCall, Station->Callsign, strlen(Station->Callsign));
	strcpy(Message->ToCall, MsgDest);

	if (SeqPtr)
	{
		strcpy(Message->Seq, SeqPtr);

		// If a REPLY-ACK Seg, copy to LastRXSeq, and see if it acks a message

		if (SeqPtr[2] == '}')
		{
			struct APRSMESSAGE * ptr1;
			int nn = 0;

			strcpy(Station->LastRXSeq, SeqPtr);

			ptr1 = SMEM->OutstandingMsgs;

			while (ptr1)
			{
				if (strcmp(ptr1->FromCall, MsgDest) == 0
					&& strcmp(ptr1->ToCall, FromCall) == 0
					&& memcmp(&ptr1->Seq, &SeqPtr[3], 2) == 0)
				{
					// Message is acked

					ptr1->Acked = TRUE;
					ptr1->Retries = 0;
					
					break;
				}
				ptr1 = ptr1->Next;
				nn++;
			}
		}
		else
		{
			// Station is not using reply-ack - set to send simple numeric sequence (workround for bug in APRS Messanges
		
			Station->SimpleNumericSeq = TRUE;
		}
	}

	if (strlen(TextPtr) > 100)
		TextPtr[100] = 0;

	strcpy(Message->Text, TextPtr);
		
	NOW = time(NULL);

	if (DefaultLocalTime)
		TM = localtime(&NOW);
	else
		TM = gmtime(&NOW);
					
	sprintf(Message->Time, "%.2d:%.2d", TM->tm_hour, TM->tm_min);

	if (_stricmp(MsgDest, CallPadded) == 0 && SeqPtr)	// ack it if it has a sequence
	{
		// For us - send an Ack

		char ack[30];
		APRSHEARDRECORD * STN;
		
		sprintf(ack, ":%-9s:ack%s", Message->FromCall, Message->Seq);

		if (memcmp(Message->FromCall, "SERVER   ", 9) == 0)
		{
			SendAPRSMessage(ack, 0);			// IS
		}
		else
		{
			STN = FindStationInMH(Message->ToCall);

			if (STN)
				SendAPRSMessage(ack, STN->rfPort);
			else
			{
				SendAPRSMessage(ack, -1);			// All RF ports
				SendAPRSMessage(ack, 0);			// IS
			}
		}
	}

	if (SaveAPRSMsgs)
		SaveAPRSMessage(Message);

	ptr = SMEM->Messages;

	if (ptr == NULL)
	{
		SMEM->Messages = Message;
	}
	else
	{
		n++;
		while(ptr->Next)
		{
			ptr = ptr->Next;
			n++;
		}
		ptr->Next = Message;
	}

	return ourMessage;
}

BOOL InternalSendAPRSMessage(char * Text, char * Call)
{
	char Msg[255];
	size_t len = strlen(Call);
	APRSHEARDRECORD * STN;
	struct tm * TM;
	time_t NOW;
	
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr = SMEM->OutstandingMsgs;

	Message = APRSGetMessageBuffer();

	if (Message == NULL)
		return FALSE;

	memset(Message, 0, sizeof(struct APRSMESSAGE));
	strcpy(Message->FromCall, CallPadded);

	memset(Message->ToCall, ' ', 9);
	memcpy(Message->ToCall, Call, len);

	Message->ToStation = FindStation(Call, TRUE);

	if (Message->ToStation == NULL)
		return FALSE;

	SMEM->NeedRefresh = TRUE;

	if (Message->ToStation->LastRXSeq[0])		// Have we received a Reply-Ack message from him?
		sprintf(Message->Seq, "%02X}%c%c", ++Message->ToStation->NextSeq, Message->ToStation->LastRXSeq[0], Message->ToStation->LastRXSeq[1]);
	else
	{
		if (Message->ToStation->SimpleNumericSeq)
			sprintf(Message->Seq, "%d", ++Message->ToStation->NextSeq);
		else
			sprintf(Message->Seq, "%02X}", ++Message->ToStation->NextSeq);	// Don't know, so assume message-ack capable
	}

	if (strlen(Text) > 100)
		Text[100] = 0;

	strcpy(Message->Text, Text);
	Message->Retries = RetryCount;
	Message->RetryTimer = RetryTimer;

	NOW = time(NULL);

	if (DefaultLocalTime)
		TM = localtime(&NOW);
	else
		TM = gmtime(&NOW);

	sprintf(Message->Time, "%.2d:%.2d", TM->tm_hour, TM->tm_min);


	// Chain to Outstanding Queue

	if (ptr == NULL)
	{
		SMEM->OutstandingMsgs = Message;
	}
	else
	{
		while(ptr->Next)
		{
			ptr = ptr->Next;
		}
		ptr->Next = Message;
	}

	sprintf(Msg, ":%-9s:%s{%s", Call, Text, Message->Seq);

	if (strcmp(Call, "SERVER") == 0)
	{
		SendAPRSMessage(Msg, 0);			// IS
		return TRUE;
	}

	STN = FindStationInMH(Message->ToCall);

	if (STN && STN->MHTIME > (time(NULL) - 900))	// Heard in last 15 mins
		SendAPRSMessage(Msg, STN->rfPort);
	else
	{
		SendAPRSMessage(Msg, -1);			// All RF ports
		SendAPRSMessage(Msg, 0);			// IS
	}
	return TRUE;
}

	



extern BOOL APRSReconfigFlag;
extern struct DATAMESSAGE * REPLYBUFFER;
extern char COMMANDBUFFER[81];
extern char OrigCmdBuffer[81];

BOOL isSYSOP(TRANSPORTENTRY * Session, char * Bufferptr);

VOID APRSCMD(TRANSPORTENTRY * Session, char * Bufferptr, char * CmdTail, struct CMDX * CMD)
{
	// APRS Subcommands. Default for compatibility is APRSMH

	// Others are STATUS ENABLEIGATE DISABLEIGATE RECONFIG

	APRSHEARDRECORD * MH = MHDATA;
	int n = MAXHEARDENTRIES;
	char * ptr;
	char * Pattern, * context;
	int Port = -1;
	char dummypattern[] ="";

	if (memcmp(CmdTail, "? ", 2) == 0)
	{
		Bufferptr = Cmdprintf(Session, Bufferptr, "APRS Subcommmands:\r");
		Bufferptr = Cmdprintf(Session, Bufferptr, "STATUS SEND MSGS SENT ENABLEIGATE DISABLEIGATE BEACON RECONFIG\r");
		Bufferptr = Cmdprintf(Session, Bufferptr, "Default is Station list - Params [Port] [Pattern]\r");
	
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "RECONFIG ", 5) == 0)
	{
		if (isSYSOP(Session, Bufferptr) == FALSE)
			return;

		if (!ProcessConfig())
		{
			Bufferptr = Cmdprintf(Session, Bufferptr, "Configuration File check failed - will continue with old config");
		}
		else
		{
			APRSReconfigFlag=TRUE;	
			Bufferptr = Cmdprintf(Session, Bufferptr, "Reconfiguration requested\r");
			SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
			return;
		}
	}

	if (memcmp(CmdTail, "ENABLEIGATE ", 6) == 0)
	{
		if (isSYSOP(Session, Bufferptr) == FALSE)
			return;

		IGateEnabled = TRUE;
		Bufferptr = Cmdprintf(Session, Bufferptr, "IGate Enabled\r");
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}
		
	if (memcmp(CmdTail, "DISABLEIGATE ", 6) == 0)
	{
		if (isSYSOP(Session, Bufferptr) == FALSE)
			return;

		IGateEnabled = FALSE;
		Bufferptr = Cmdprintf(Session, Bufferptr, "IGate Disabled\r");
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "STATUS ", 7) == 0)
	{
		if (IGateEnabled == FALSE)
			Bufferptr = Cmdprintf(Session, Bufferptr, "IGate Disabled\r");
		else
		{
			Bufferptr = Cmdprintf(Session, Bufferptr, "IGate Enabled ");
			if (APRSISOpen)
				Bufferptr = Cmdprintf(Session, Bufferptr, "and connected to %s\r", RealISHost);
			else
				Bufferptr = Cmdprintf(Session, Bufferptr, "but not connected\r");
		}

		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "BEACON ", 7) == 0)
	{
		if (isSYSOP(Session, Bufferptr) == FALSE)
			return;

		BeaconCounter = 2;
		Bufferptr = Cmdprintf(Session, Bufferptr, "Beacons requested\r");
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "MSGS ", 5) == 0)
	{
		struct APRSMESSAGE * ptr = SMEM->Messages;
		char Addrs[32];

		Bufferptr = Cmdprintf(Session, Bufferptr,
			"\rTime  Calls               Seq   Text\r");

		while (ptr)
		{
			char ToLopped[11] = "";

			memcpy(ToLopped, ptr->ToCall, 10);
			strlop(ToLopped, ' ');

			sprintf(Addrs, "%s>%s", ptr->FromCall, ToLopped);

			Bufferptr = Cmdprintf(Session, Bufferptr, "%s %-20s%-5s %s\r",
				ptr->Time, Addrs, ptr->Seq, ptr->Text);
	
			ptr = ptr->Next;
		}
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "SENT ", 5) == 0)
	{
		struct APRSMESSAGE * ptr = SMEM->OutstandingMsgs;
		char Addrs[32];

		Bufferptr = Cmdprintf(Session, Bufferptr,
			"\rTime  Calls               Seq State Text\r");

		while (ptr)
		{
			char ToLopped[11] = "";
			char Retries[10];

			if (ptr->Acked)
				strcpy(Retries, "A");
			else if (ptr->Retries == 0)
				strcpy(Retries, "F");
			else
				sprintf(Retries, "%d", ptr->Retries);

	
			memcpy(ToLopped, ptr->ToCall, 10);
			strlop(ToLopped, ' ');

			sprintf(Addrs, "%s>%s", ptr->FromCall, ToLopped);

			Bufferptr = Cmdprintf(Session, Bufferptr, "%s %-20s%-5s %-2s %s\r",
				ptr->Time, Addrs, ptr->Seq, Retries, ptr->Text);
	
			ptr = ptr->Next;
		}
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	if (memcmp(CmdTail, "SEND ", 5) == 0)
	{
		// Send Message. Params are Call and Message

		char * Call = strtok_s(&CmdTail[5], " \r", &context);
		char * Text = strtok_s(NULL, " \r", &context);
		int len = 0;
		
		if (isSYSOP(Session, Bufferptr) == FALSE)
			return;

		if (Call)
			len = (int)strlen(Call);

		if (len < 3 || len > 9)
		{
			Bufferptr = Cmdprintf(Session, Bufferptr, "Invalid Callsign\r");
			SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
			return;
		}

		if (Text == NULL)
		{
			Bufferptr = Cmdprintf(Session, Bufferptr, "No Message Text\r");
			SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
			return;
		}
		// Replace command tail with original (before conversion to upper case

		Text = Text + (OrigCmdBuffer - COMMANDBUFFER);

		InternalSendAPRSMessage(Text, Call);

		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
		return;
	}

	//	DISPLAY APRS HEARD LIST

	// APRS [Port] [Pattern] 

	Pattern = strtok_s(CmdTail, " \r", &context);

	if (Pattern && (int)strlen(Pattern) < 3)
	{
		// could be port number

		if (isdigit(Pattern[0]) && (Pattern[1] == 0 || isdigit(Pattern[1])))
		{
			Port = atoi(Pattern);
			Pattern = strtok_s(NULL, " \r", &context);
		}
	}

	// Param is a pattern to match

	if (Pattern == NULL)
		Pattern = dummypattern;

	if (Pattern[0] == ' ')
	{
		// Prepare Pattern

		char * ptr1 = Pattern + 1;
		char * ptr2 = Pattern;
		char c;
		
		do
		{
			c = *ptr1++;
			*(ptr2++) = c;
		}
		while (c != ' ');

		*(--ptr2) = 0;
	}

	strlop(Pattern, ' ');
	_strupr(Pattern);

	*(Bufferptr++) = 13;

	while (n--)
	{
		if (MH->MHCALL[0] == 0)
			break;

		if ((Port > -1) && Port != MH->rfPort)
		{
			MH++;
			continue;
		}
	
		ptr = FormatAPRSMH(MH);

		MH++;
		
		if (ptr)
		{
			if (Pattern[0] && strstr(ptr, Pattern) == 0)
				continue;

			Bufferptr = Cmdprintf(Session, Bufferptr, "%s", ptr);
		}
	}
	SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
}

int GetPosnFromAPRS(char * Call, double * Lat, double * Lon)
{
	struct STATIONRECORD * Station;

	Station = FindStation(Call, FALSE);

	if (Station)
	{
		*Lat = Station->Lat;
		*Lon = Station->Lon;

		return 1;
	}

	return 0;
}

// Station Name Font

const unsigned char ASCII[][5] = {
//const u08 ASCII[][5]  = {
  {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
  ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
  ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
  ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
  ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
  ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
  ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
  ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
  ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
  ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
  ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
  ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
  ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
  ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
  ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
  ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
  ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
  ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
  ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
  ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
  ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
  ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
  ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
  ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
  ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
  ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
  ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
  ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
  ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
  ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
  ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
  ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
  ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
  ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
  ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
  ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
  ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
  ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
  ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
  ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
  ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
  ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
  ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
  ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
  ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
  ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
  ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
  ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
  ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
  ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
  ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
  ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
  ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
  ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
  ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
  ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
  ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
  ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
  ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
  ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
  ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c 
  ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
  ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
  ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
  ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
  ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
  ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
  ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
  ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
  ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
  ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
  ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
  ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
  ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
  ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
  ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
  ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
  ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
  ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
  ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
  ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
  ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
  ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
  ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
  ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
  ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
  ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
  ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
  ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
  ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
  ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
  ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
  ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
  ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
  ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
  ,{0x78, 0x46, 0x41, 0x46, 0x78} // 7f DEL
};


// APRS Web Map Code

// Not sure yet what is best way to do station icons but for now build and cache any needed icons

extern int IconDataLen;
extern unsigned long long IconData[];  // Symbols as a png image.&

// IconData is a png image, so needs to be uncompressed to an RGB array


// Will key cached icons by IconRow, IconCol, Overlay Char - xxxxA

int cachedIconCount = 0;

// We need key, icon data, icon len for each. Maybe a simple linked list - we never remove any

struct iconCacheEntry
{
	struct iconCacheEntry * Next;
	char key[8];
	int pngimagelen;
	int pngmalloclen;
	unsigned char * pngimage;
};

struct iconCacheEntry * iconCache = NULL;


// Each icon has to be created as an RGB array, then converted to a png image, as
// Leaflet icons need a png file

#include "mypng.h"


struct png_info_struct * info_ptr = NULL;

unsigned char * PngEncode (png_byte *pDiData, int iWidth, int iHeight, struct iconCacheEntry * Icon);
	
void createIcon(char * Key, int iconRow, int iconCol, char Overlay)
{
	int i, j, index, mask;
	int row;
	int col;			// First row
	unsigned char * rgbData = malloc(68 * 22); // 1323
	unsigned char * ptr = rgbData;
	png_color colour = {0, 0, 0};
	int Pointer;
	char c;
	int bit;
	struct iconCacheEntry * Icon = zalloc(sizeof(struct iconCacheEntry));

	strcpy(Icon->key, Key);

	// icon data is in info_ptr->row_pointers (we have 255 of them)

	row = iconRow * 21;
	col = iconCol * 21 * 3;

	for (j = 0; j < 22; j++)
	{
		memcpy(ptr, info_ptr->row_pointers[row + j] + col, 22 * 3);		// One scan line
		ptr += 68;		// Rounded up to mod 4
	}


	// This code seems to assume an icon is 16 pixels, but image is 22 x 22 ???

//	j = ptr->iconRow * 21 * 337 * 3 + ptr->iconCol * 21 * 3 + 9 + 337 * 9;
//	for (i = 0; i < 16; i++)
//	{
//		memcpy(nptr, &iconImage[j], 16 * 3);
//		nptr += 6144;
//		j += 337 * 3;
//	}


	// If an overlay is specified, add it

	if (Overlay)
	{
		Pointer = 68 * 7 + 7 * 3;		// 7th row, 7th col
		mask = 1;

		for (index = 0 ; index < 7 ; index++)
		{
			rgbData[Pointer++] = 255;				// Blank line above chars 
			rgbData[Pointer++] = 255;
			rgbData[Pointer++] = 255;
		}
		
		Pointer = 68 * 8 + 7 * 3;		// 8th row, 7th col

		for (i = 0; i < 7; i++)
		{
			rgbData[Pointer++] = 255;				// Blank col 
			rgbData[Pointer++] = 255;
			rgbData[Pointer++] = 255;

			for (index = 0 ; index < 5 ; index++)
			{
				c = ASCII[Overlay - 0x20][index];	// Font data
				bit = c & mask;

				if (bit)
				{
					rgbData[Pointer++] = 0;
					rgbData[Pointer++] = 0;
					rgbData[Pointer++] = 0;
				}
				else
				{
					rgbData[Pointer++] = 255;
					rgbData[Pointer++] = 255;
					rgbData[Pointer++] = 255;
				}
			}
			
			rgbData[Pointer++] = 255;				// Blank col 
			rgbData[Pointer++] = 255;
			rgbData[Pointer++] = 255;

			mask <<= 1;
			Pointer += 47;
		}
		for (index = 0 ; index < 7 ; index++)
		{
			rgbData[Pointer++] = 255;				// Blank line above chars 
			rgbData[Pointer++] = 255;
			rgbData[Pointer++] = 255;
		}
	}

	// Encode

	PngEncode(rgbData, 22, 22, Icon);

	if (iconCache)
		Icon->Next = iconCache;
		
	iconCache = Icon;

}

int GetAPRSIcon(unsigned char * _REPLYBUFFER, char * NodeURL)
{
	char Key[8] = "";
	struct iconCacheEntry * Icon = iconCache;

	memcpy(Key, &NodeURL[5], 5);

	while (Icon)
	{
		if (strcmp(Icon->key, Key) == 0)		// Have it
		{
			memcpy(_REPLYBUFFER, Icon->pngimage, Icon->pngimagelen);
			return Icon->pngimagelen;
		}
		Icon = Icon->Next;
	}

	return 0;
}

char * doHTMLTransparency(char * string)
{
	// Make sure string doesn't contain forbidden XML chars (<>"'&)

	char * newstring = malloc(5 * strlen(string) + 1);		// If len is zero still need null terminator

	char * in = string;
	char * out = newstring;
	char c;

	c = *(in++);

	while (c)
	{
		switch (c)
		{
		case '<':

			strcpy(out, "&lt;");
			out += 4;
			break;

		case '>':

			strcpy(out, "&gt;");
			out += 4;
			break;

		case '"':

			strcpy(out, "&quot;");
			out += 6;
			break;

		case '\'':

			strcpy(out, "&apos;");
			out += 6;
			break;

		case '&':

			strcpy(out, "&amp;");
			out += 5;
			break;

		case ',':

			strcpy(out, "&#44;");
			out += 5;
			break;
		
		case '|':

			strcpy(out, "&#124;");
			out += 5;
			break;
		
		default:

			*(out++) = c;
		}
		c = *(in++);
	}

	*(out++) = 0;
	return newstring;
}

int GetAPRSPageInfo(char * Buffer, double N, double S, double W, double E, int aprs, int ais, int adsb)
{
	struct STATIONRECORD * ptr = *StationRecords;
	int n = 0, Len = 0;
	struct tm * TM;
	time_t NOW = time(NULL);
	char popup[65536] = "";
	char Msg[2048];
	int LocalTime = 0;
	int KM = DefaultDistKM; 
	char * ptr1;

	while (ptr && aprs && Len < 240000)
	{
		if (ptr->Lat != 0.0 && ptr->Lon != 0.0)
		{
			if (ptr->Lat > S && ptr->Lat < N && ptr->Lon > W && ptr->Lon < E)
			{
				// See if we have the Icon - if not build

				char IconKey[6];
				struct iconCacheEntry * Icon = iconCache;

				sprintf(IconKey, "%02X%02X ", ptr->iconRow, ptr->iconCol);
				
				if (ptr->IconOverlay)
					IconKey[4] = ptr->IconOverlay;
				else
					IconKey[4] = '@';

				while (Icon)
				{
					if (strcmp(Icon->key, IconKey) == 0)		// Have it
						break;

					Icon = Icon->Next;
				}

				if (Icon == NULL)
					createIcon(IconKey, ptr->iconRow, ptr->iconCol, ptr->IconOverlay);

				popup[0] = 0;

				if (ptr->Approx)
				{
					sprintf(Msg, "Approximate Position From Locator<br>");
					strcat(popup, Msg);
				}
				ptr1 = doHTMLTransparency(ptr->Path);
				sprintf(Msg, "%s<br>", ptr1);
				strcat(popup, Msg);
				free(ptr1);
				ptr1 = doHTMLTransparency(ptr->LastPacket);
				sprintf(Msg, "%s<br>", ptr1);
				strcat(popup, Msg);
				free(ptr1);
				ptr1 = doHTMLTransparency(ptr->Status);
				sprintf(Msg, "%s<br>", ptr1);
				strcat(popup, Msg);
				free(ptr1);
				if (LocalTime)
					TM = localtime(&ptr->TimeLastUpdated);
				else
					TM = gmtime(&ptr->TimeLastUpdated);

				sprintf(Msg, "Last Heard: %.2d:%.2d:%.2d on Port %d<br>",
					TM->tm_hour, TM->tm_min, TM->tm_sec, ptr->LastPort);

				strcat(popup, Msg);

				sprintf(Msg, "Distance %6.1f Bearing %3.0f Course %1.0f&deg; Speed %3.1f<br>",
					myDistance(ptr->Lat, ptr->Lon, KM),
					myBearing(ptr->Lat, ptr->Lon), ptr->Course, ptr->Speed);
				strcat(popup, Msg);

				if (ptr->LastWXPacket[0])
				{
					//display wx info

					struct APRSConnectionInfo temp;

					memset(&temp, 0, sizeof(temp));

					DecodeWXReport(&temp, ptr->LastWXPacket);

					sprintf(Msg, "Wind Speed %d MPH<br>", temp.WindSpeed);
					strcat(popup, Msg);

					sprintf(Msg, "Wind Gust %d MPH<br>", temp.WindGust);
					strcat(popup, Msg);

					sprintf(Msg, "Wind Direction %d\xC2\xB0<br>", temp.WindDirn);
					strcat(popup, Msg);

					sprintf(Msg, "Temperature %d\xC2\xB0 F<br>", temp.Temp);
					strcat(popup, Msg);

					sprintf(Msg, "Pressure %05.1f<br>", temp.Pressure / 10.0);
					strcat(popup, Msg);

					sprintf(Msg, "Humidity %d%%<br>", temp.Humidity);
					strcat(popup, Msg);

					sprintf(Msg, "Rainfall Last Hour/Last 24H/Today %5.2f, %5.2f, %5.2f (inches)",
						temp.RainLastHour / 100.0, temp.RainLastDay / 100.0, temp.RainToday / 100.0);

					ptr1 = doHTMLTransparency(Msg);
					sprintf(Msg, "%s<br>", ptr1);
					strcat(popup, Msg);
					free(ptr1);
				}

				Len += sprintf(&Buffer[Len],"A,%.4f,%.4f,%s,%s,%s,%d\r\n|",
					ptr->Lat, ptr->Lon, ptr->Callsign, popup, IconKey,
					NOW - ptr->TimeLastUpdated);

				if (ptr->TrackTime[0] && ptr->TrackTime[1])	// Have trackpoints
				{
					int n = ptr->Trackptr;
					int i;
					double lastLat = 0;

					// We read from next track point (oldest) for TRACKPOINT records, ignoring zeros

					Len += sprintf(&Buffer[Len],"T,");

					for (i = 0; i < TRACKPOINTS; i++)
					{
						if (ptr->TrackTime[n])
						{
							Len += sprintf(&Buffer[Len],"%.4f,%.4f,", ptr->LatTrack[n], ptr->LonTrack[n]);
							lastLat = ptr->LatTrack[n];
						}

						n++;
						if (n == TRACKPOINTS)
							n = 0;
					}
					if (lastLat != ptr->Lat)
						Len += sprintf(&Buffer[Len],"%.4f,%.4f,\r\n|", ptr->Lat, ptr->Lon);		//Add current position to end of track
					else
						Len += sprintf(&Buffer[Len],"\r\n|");
				}
			}
		}

		ptr = ptr->Next;		
	}
	return Len;
}




 /* The png_jmpbuf() macro, used in error handling, became available in
  * libpng version 1.0.6.  If you want to be able to run your code with older
  * versions of libpng, you must define the macro yourself (but only if it
  * is not already defined by libpng!).
  */
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->png_jmpbuf)
#endif
/* Check to see if a file is a PNG file using png_sig_cmp().  png_sig_cmp()
 * returns zero if the image is a PNG and nonzero if it isn't a PNG.
 *
 * The function check_if_png() shown here, but not used, returns nonzero (true)
 * if the file can be opened and is a PNG, 0 (false) otherwise.
 *
 * If this call is successful, and you are going to keep the file open,
 * you should call png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK); once
 * you have created the png_ptr, so that libpng knows your application
 * has read that many bytes from the start of the file.  Make sure you
 * don't call png_set_sig_bytes() with more than 8 bytes read or give it
 * an incorrect number of bytes read, or you will either have read too
 * many bytes (your fault), or you are telling libpng to read the wrong
 * number of magic bytes (also your fault).
 *
 * Many applications already read the first 2 or 4 bytes from the start
 * of the image to determine the file type, so it would be easiest just
 * to pass the bytes to png_sig_cmp() or even skip that if you know
 * you have a PNG file, and call png_set_sig_bytes().
 */

unsigned char * user_io_ptr = 0;

void __cdecl user_read_fn(png_struct * png, png_bytep Buffer, png_size_t Len)
{
	unsigned char ** ptr = png->io_ptr;
	unsigned char * ptr1;
	
	ptr1 = ptr[0];

	memcpy(Buffer, ptr1, Len);
	ptr[0]+= Len;
}


// This is based on example https://www1.udel.edu/CIS/software/dist/libpng-1.2.8/example.c

// This decodes a png encoded image from memory

int read_png(unsigned char *bytes)
{
   png_structp png_ptr;
   unsigned int sig_read = 0;
 
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
   if (png_ptr == NULL)
   {
      return (0);
   }
   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      return (0);
   }
   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */

   user_io_ptr = (unsigned char *)&IconData;

   png_set_read_fn(png_ptr, (void *)&user_io_ptr,(png_rw_ptr)user_read_fn);

   png_set_sig_bytes(png_ptr, sig_read);

   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, NULL);

   // Data is in info->row_pointers. Can we use it from there ??

//   printf("%d  %d  %d\n", info_ptr->width, info_ptr->height, info_ptr->valid);

   return TRUE;
}

void Myabort()
{}

// This is based on pngfile.c

//-------------------------------------
//  PNGFILE.C -- Image File Functions
//-------------------------------------

// Copyright 2000, Willem van Schaik.  For conditions of distribution and
// use, see the copyright/license/disclaimer notice in png.h

// Encodes pDiData to png format in memory



void my_png_write_data(struct png_struct_def * png_ptr, png_bytep data, png_size_t length)
{
	struct iconCacheEntry * Icon = png_ptr->io_ptr;

	if (Icon->pngimagelen + (int)length > Icon->pngmalloclen)
	{
		Icon->pngmalloclen += length;
		Icon->pngimage = realloc(Icon->pngimage, Icon->pngmalloclen);
	}

	memcpy(&Icon->pngimage[Icon->pngimagelen], data, length);
	Icon->pngimagelen += length;
}

  //  io_ptr = (FILE *)CVT_PTR((png_ptr->io_ptr));
  // Area png_uint_32 check;



static void png_flush(png_structp png_ptr)
{
}

unsigned char * PngEncode (png_byte *pDiData, int iWidth, int iHeight, struct iconCacheEntry * Icon)
{
    const int           ciBitDepth = 8;
    const int           ciChannels = 3;
	png_structp png_ptr;
	png_infop info_ptr = NULL;
    png_uint_32         ulRowBytes;
    static png_byte   **ppbRowPointers = NULL;
    int                 i;

	// Set up image array and pointer. First allocate a buffer as big as the original
	// in the unlikely event of it being too small write_data will realloc it

	Icon->pngmalloclen = iWidth * iHeight * 3;
	Icon->pngimage = malloc(Icon->pngmalloclen);
	Icon->pngimagelen = 0;

    // prepare the standard PNG structures

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
        return FALSE;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	{
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return FALSE;
    }

    {
        // initialize the png structure
        
        png_set_write_fn(png_ptr, Icon, my_png_write_data, png_flush);
       
        png_set_IHDR(png_ptr, info_ptr, iWidth, iHeight, ciBitDepth,
            PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);
        
        // write the file header information
        
        png_write_info(png_ptr, info_ptr);
               
        // row_bytes is the width x number of channels
        
        ulRowBytes = iWidth * ciChannels;
        
        // we can allocate memory for an array of row-pointers
        
        if ((ppbRowPointers = (png_bytepp) malloc(iHeight * sizeof(png_bytep))) == NULL)
            Debugprintf( "Visualpng: Out of memory");
        
        // set the individual row-pointers to point at the correct offsets
        
        for (i = 0; i < iHeight; i++)
            ppbRowPointers[i] = pDiData + i * (((ulRowBytes + 3) >> 2) << 2);
        
        // write out the entire image data in one call
        
        png_write_image (png_ptr, ppbRowPointers);
        
        // write the additional chunks to the PNG file (not really needed)
        
        png_write_end(png_ptr, info_ptr);
        
        // and we're done
        
        free (ppbRowPointers);
        ppbRowPointers = NULL;
        
        // clean up after the write, and free any memory allocated
        
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        
        // yepp, done
    }

    return Icon->pngimage;
}

void SaveAPRSMessage(struct APRSMESSAGE * ptr)
{
	// Save messages in case of a restart

	char FN[250];
	FILE *file;

	// Set up filename

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"APRSMsgs.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"APRSMsgs.dat");
	}

	if ((file = fopen(FN, "a")) == NULL)
		return ;

	fprintf(file, "%d %s,%s,%s,%s,%s\n", time(NULL), ptr->FromCall, ptr->ToCall, ptr->Seq, ptr->Time, ptr->Text);

	fclose(file);
}

void ClearSavedMessages()
{
	char FN[250];
	FILE *file;

	// Set up filename

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"APRSMsgs.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"APRSMsgs.dat");
	}

	if ((file = fopen(FN, "w")) == NULL)
		return ;

	fclose(file);
}

void GetSavedAPRSMessages()
{
	// Get Saved messages 

	// 1668768157 SERVER   ,GM8BPQ-2 ,D7Yx,10:42,filter m/200 active

	char FN[250];
	FILE *file;
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr;
	char Line[512];
	char * Stamp = 0;
	char * From = 0;
	char * To = 0;
	char * Seq = 0;
	char * Time = 0;
	char * Text = 0;

	// Set up filename

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"APRSMsgs.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"APRSMsgs.dat");
	}

	if ((file = fopen(FN, "r")) == NULL)
		return ;

	while (fgets(Line, sizeof(Line), file))
	{
		Stamp = Line;
		From = strlop(Stamp, ' ');
		To = strlop(From, ',');
		Seq = strlop(To, ',');
		Time = strlop(Seq, ',');
		Text = strlop(Time, ',');

		if (Stamp && From && To && Seq && Time && Text)
		{
			Message = APRSGetMessageBuffer();

			if (Message == NULL)
				break;

			memset(Message, 0, sizeof(struct APRSMESSAGE));

			strcpy(Message->FromCall, From);
			strcpy(Message->ToCall, To);
			strcpy(Message->Seq, Seq);
			strcpy(Message->Time, Time);
			strcpy(Message->Text, Text);

			ptr = SMEM->Messages;

			if (ptr == NULL)
			{
				SMEM->Messages = Message;
			}
			else
			{
				while(ptr->Next)
				{
					ptr = ptr->Next;
				}
				ptr->Next = Message;
			}

		}
	}
	fclose(file);
}
