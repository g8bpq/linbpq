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

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#include "compatbits.h"
#include <string.h>
#include "asmstrucs.h"
#include "tncinfo.h"
#include "cheaders.h"
#include "kiss.h"

VOID __cdecl Debugprintf(const char * format, ...);

#ifndef WIN32

#define APIENTRY
#define DllExport
#define VOID void

#else
#include <windows.h>
#endif

extern BOOL EventsEnabled;
void MQTTReportSession(char * Msg);
extern int MQTT;
extern time_t TimeLoaded;

uint16_t UDPSeq = 1;
int linkSeq = 1;
int cctSeq = 1;

extern SOCKET NodeAPISocket;
extern SOCKADDR_IN UDPreportdest;

extern char Modenames[19][10];

extern char NODECALLLOPPED[10];
extern char MYALIASLOPPED[10];
extern char	LOC[7];
extern char VersionString[50];
extern double LatFromLOC;
extern double LonFromLOC;
extern int NUMBEROFNODES, MAXDESTS, L4CONNECTSOUT, L4CONNECTSIN, L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES;
extern int  L2CONNECTSOUT, L2CONNECTSIN;

void hookL2SessionClosed(struct _LINKTABLE * LINK, char * Reason, char * Direction);
int ConvFromAX25(unsigned char * incall, unsigned char * outcall);
int COUNT_AT_L2(struct _LINKTABLE * LINK);
int CountFramesQueuedOnSession(TRANSPORTENTRY * Session);
int decodeNETROMUIMsg(unsigned char * Msg, int iLen, char * Buffer, int BufferLen);
int decodeNETROMIFrame(unsigned char * Msg, int iLen, char * Buffer, int BufferLen);
int decodeINP3RIF(unsigned char * Msg, int iLen, char * Buffer, int BufferLen);
int decodeRecordRoute(L3MESSAGE * L3, int iLen, char * Buffer, int BufferLen);
char * byte_base64_encode(char *str, int len);

// Runs use specified routine on certain event

#ifndef WIN32

void RunEventProgram(char * Program, char * Param)
{
	char * arg_list[] = {Program, NULL, NULL};
	pid_t child_pid;

	if (EventsEnabled == 0)
		return;

	signal(SIGCHLD, SIG_IGN); // Silently (and portably) reap children. 

	if (Param && Param[0])
		arg_list[1] = Param;

	//	Fork and Exec Specified program

	// Duplicate this process.

	child_pid = fork (); 

	if (child_pid == -1) 
	{    				
		printf ("Event fork() Failed\n"); 
		return;
	}

	if (child_pid == 0) 
	{    				
		execvp (arg_list[0], arg_list); 

		// The execvp  function returns only if an error occurs.  

		printf ("Failed to run %s\n", arg_list[0]); 
		exit(0);			// Kill the new process
	}
								 
#else

DllExport void APIENTRY RunEventProgram(char * Program, char * Param)
{
	int n = 0;
	char cmdLine[256];

	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
	PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

	if (EventsEnabled == 0)
		return;


	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
	SInfo.lpReserved2=NULL; 

	sprintf(cmdLine, "%s %s", Program, Param);

	if (!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo))
		Debugprintf("Failed to Start %s Error %d ", Program, GetLastError());

#endif

	return;
}

void hookL2SessionAccepted(int Port, char * remotecall, char * ourcall, struct _LINKTABLE * LINK)
{
	// Incoming SABM accepted

	char UDPMsg[1024];	
	int udplen;

	L2CONNECTSIN++;
	LINK->apiSeq = linkSeq++;

	LINK->lastStatusSentTime = LINK->ConnectTime = time(NULL);
	LINK->bytesTXed = LINK->bytesRXed = LINK->framesResent = LINK->framesRXed = LINK->framesTXed = 0;
	LINK->LastStatusbytesTXed = LINK->LastStatusbytesRXed = 0;
	strcpy(LINK->callingCall, remotecall);
	strcpy(LINK->receivingCall, ourcall);
	strcpy(LINK->Direction, "In");

	if (NodeAPISocket)
	{
		LINK->lastStatusSentTime = time(NULL);

		udplen = sprintf(UDPMsg, "{\"@type\":\"LinkUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"incoming\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\", \"isRF\": %s}",
			NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall, (LINK->LINKPORT->isRF)?"true":"false");

//		Debugprintf(UDPMsg);

		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}


void hookL2SessionDeleted(struct _LINKTABLE * LINK)
{
	// calculate session time and av bytes/min in and out

	if (LINK->ConnectTime)
	{
		if (LINK->bytesTXed == 0 && LINK->bytesRXed == 0)
		{
			// assume failed connect and ignore for now - maybe log later

		}
		else
		{
			char Msg[256];
			char timestamp[64];
			time_t sessionTime = time(NULL) - LINK->ConnectTime;
			double avBytesSent = LINK->bytesTXed / (sessionTime / 60.0);
			double avBytesRXed = LINK->bytesRXed / (sessionTime / 60.0);
			time_t Now = time(NULL);
			struct tm * TM = localtime(&Now);

			sprintf(timestamp, "%02d:%02d:%02d", TM->tm_hour, TM->tm_min, TM->tm_sec);

			if (sessionTime == 0)
				sessionTime = 1;				// Or will get divide by zero error 

			Debugprintf("KISS Session Stats Port %d %s %s %d secs Bytes Sent %d  BPM %4.2f Bytes Received %d %4.2f BPM ", 
				LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall, sessionTime, LINK->bytesTXed, avBytesSent, LINK->bytesRXed, avBytesRXed, timestamp);


			sprintf(Msg, "{\"mode\": \"%s\", \"direction\": \"%s\", \"port\": %d, \"callfrom\": \"%s\", \"callto\": \"%s\", \"time\": %d,  \"bytesSent\": %d," 
				"\"BPMSent\": %4.2f, \"BytesReceived\": %d,  \"BPMReceived\": %4.2f, \"timestamp\": \"%s\"}",
				"KISS", LINK->Direction, LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall, sessionTime,
				LINK->bytesTXed,  avBytesSent, LINK->bytesRXed, avBytesRXed, timestamp);

			if (MQTT)
				MQTTReportSession(Msg);

			LINK->ConnectTime = 0;
		}

		if (LINK->Sent && LINK->Received && (LINK->SentAfterCompression || LINK->ReceivedAfterExpansion))
			Debugprintf("L2 Compression Stats %s %s TX %d %d %d%% RX %d %d %d%%", LINK->callingCall, LINK->receivingCall,
			LINK->Sent, LINK->SentAfterCompression, ((LINK->Sent - LINK->SentAfterCompression) * 100) / LINK->Sent,
			LINK->Received, LINK->ReceivedAfterExpansion, ((LINK->ReceivedAfterExpansion - LINK->Received) * 100) / LINK->Received);

	}
}

void hookL2SessionAttempt(int Port, char * ourcall, char * remotecall, struct _LINKTABLE * LINK)
{
	LINK->lastStatusSentTime = LINK->ConnectTime = time(NULL);
	LINK->bytesTXed = LINK->bytesRXed = LINK->framesResent = LINK->framesRXed = LINK->framesTXed = 0;
	LINK->LastStatusbytesTXed = LINK->LastStatusbytesRXed = 0;
	strcpy(LINK->callingCall, ourcall);
	strcpy(LINK->receivingCall, remotecall);
	strcpy(LINK->Direction, "Out");
}

void hookL2SessionConnected(struct _LINKTABLE * LINK)
{
	// UA received in reponse to SABM

	char UDPMsg[1024];	
	int udplen;

	L2CONNECTSOUT++;
	LINK->apiSeq = linkSeq++;

	if (NodeAPISocket)
	{
		LINK->lastStatusSentTime = time(NULL);

		udplen = sprintf(UDPMsg, "{\"@type\":\"LinkUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"outgoing\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\", \"isRF\": %s}",
			NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall, (LINK->LINKPORT->isRF)?"true":"false");

//		Debugprintf(UDPMsg);

		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

void hookL2SessionClosed(struct _LINKTABLE * LINK, char * Reason, char * Direction)
{
	// Link closed. Could be normal, ie disc send/received or restried out etc

	char UDPMsg[1024];	
	int udplen;
	time_t Now = time(NULL);

	if (NodeAPISocket)
	{
		if (LINK->receivingCall[0] == 0 || LINK->callingCall[0] == 0)
			return;

		if (strcmp(Direction, "Out") == 0)
			udplen = sprintf(UDPMsg, "{\"@type\":\"LinkDownEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"outgoing\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\","
			"\"bytesSent\": %d, \"bytesRcvd\": %d, \"frmsSent\": %d, \"frmsRcvd\": %d, \"frmsQueued\": %d, \"frmsResent\": %d, \"reason\": \"%s\","
			" \"time\": %d, \"upForSecs\": %d, \"frmsQdPeak\": %d, \"isRF\": %s}",
			NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->receivingCall, LINK->callingCall,
			LINK->bytesTXed , LINK->bytesRXed, LINK->framesTXed, LINK->framesRXed, COUNT_AT_L2(LINK), LINK->framesResent, Reason,
			(int)Now, (int)Now - LINK->ConnectTime, LINK->maxQueued, (LINK->LINKPORT->isRF)?"true":"false");
		else
			udplen = sprintf(UDPMsg, "{\"@type\":\"LinkDownEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"incoming\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\","
			"\"bytesSent\": %d, \"bytesRcvd\": %d, \"frmsSent\": %d, \"frmsRcvd\": %d, \"frmsQueued\": %d, \"frmsResent\": %d, \"reason\": \"%s\","
			" \"time\": %d, \"upForSecs\": %d, \"frmsQdPeak\": %d, \"isRF\": %s}",
			NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall,
			LINK->bytesTXed , LINK->bytesRXed, LINK->framesTXed, LINK->framesRXed, COUNT_AT_L2(LINK), LINK->framesResent, Reason,
			(int)Now, (int)Now - LINK->ConnectTime, LINK->maxQueued, (LINK->LINKPORT->isRF)?"true":"false");

//		Debugprintf(UDPMsg);

		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

void hookL2SessionStatus(struct _LINKTABLE * LINK)
{
	// Send at regular intervals on open links

	char UDPMsg[1024];	
	int udplen;
	time_t Now = time(NULL);
	int bpsTx, bpsRx, interval;

	if (NodeAPISocket)
	{
		interval = Now - (int)LINK->lastStatusSentTime;
		bpsTx = (LINK->bytesTXed - LINK->LastStatusbytesTXed) / interval; 
		bpsRx = (LINK->bytesRXed - LINK->LastStatusbytesRXed) / interval; 
		
		LINK->lastStatusSentTime = Now;
		LINK->LastStatusbytesTXed = LINK->bytesTXed;
		LINK->LastStatusbytesRXed = LINK->bytesRXed;

		if (strcmp(LINK->Direction, "Out") == 0)
			udplen = sprintf(UDPMsg, "{\"@type\":\"LinkStatus\", \"node\": \"%s\", \"id\": %d, \"direction\": \"outgoing\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\","
				"\"bytesSent\": %d, \"bytesRcvd\": %d, \"frmsSent\": %d, \"frmsRcvd\": %d, \"frmsQueued\": %d, \"frmsResent\": %d,"
				"\"upForSecs\": %d, \"frmsQdPeak\": %d, \"bpsTxMean\": %d, \"bpsRxMean\": %d, \"frmQMax\": %d, \"l2rttMs\": %d, \"isRF\": %s}",
				NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->receivingCall, LINK->callingCall,
				LINK->bytesTXed, LINK->bytesRXed, LINK->framesTXed, LINK->framesRXed, 0, LINK->framesResent,
				(int)Now - LINK->ConnectTime, LINK->maxQueued, bpsTx, bpsRx, LINK->intervalMaxQueued, LINK->RTT, (LINK->LINKPORT->isRF)?"true":"false");
		else
			udplen = sprintf(UDPMsg, "{\"@type\":\"LinkStatus\", \"node\": \"%s\", \"id\": %d, \"direction\": \"incoming\", \"port\": \"%d\", \"remote\": \"%s\", \"local\": \"%s\","
				"\"bytesSent\": %d, \"bytesRcvd\": %d, \"frmsSent\": %d, \"frmsRcvd\": %d, \"frmsQueued\": %d, \"frmsResent\": %d,"
				"\"upForSecs\": %d, \"frmsQdPeak\": %d, \"bpsTxMean\": %d, \"bpsRxMean\": %d, \"frmQMax\": %d, \"l2rttMs\": %d, \"isRF\": %s}",
				NODECALLLOPPED, LINK->apiSeq, LINK->LINKPORT->PORTNUMBER, LINK->callingCall, LINK->receivingCall,
				LINK->bytesTXed, LINK->bytesRXed, LINK->framesTXed, LINK->framesRXed, 0, LINK->framesResent,
				(int)Now - LINK->ConnectTime, LINK->maxQueued, bpsTx, bpsRx, LINK->intervalMaxQueued, LINK->RTT, (LINK->LINKPORT->isRF)?"true":"false");

		LINK->intervalMaxQueued = 0;

//		Debugprintf(UDPMsg);
		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}




void hookL4SessionAttempt(struct STREAMINFO * STREAM, char * remotecall, char * ourcall)
{
	// Outgoing Connect

	STREAM->ConnectTime = time(NULL);
	STREAM->bytesTXed = STREAM->bytesRXed = 0;

	strcpy(STREAM->callingCall, ourcall);
	strcpy(STREAM->receivingCall, remotecall);
	strcpy(STREAM->Direction, "Out");
}

void hookL4SessionAccepted(struct STREAMINFO * STREAM, char * remotecall, char * ourcall)
{
	// Incoming Connect

	STREAM->ConnectTime = time(NULL);
	STREAM->bytesTXed = STREAM->bytesRXed = 0;

	strcpy(STREAM->callingCall, remotecall);
	strcpy(STREAM->receivingCall, ourcall);
	strcpy(STREAM->Direction, "In");
}

/*
{
  "eventSource": "circuit",
  "time": "2025-10-08T14:05:54+00:00",
  "id": 23,
  "direction": "incoming",
  "port": "0",
  "remote": "G8PZT@G8PZT:15aa",
  "local": "GE8PZT:0017",
  "event": "disconnect",
  "@type": "event"
}
*/

void hookL4SessionDeleted(struct TNCINFO * TNC, struct STREAMINFO * STREAM)
{
	char Msg[256];

	char timestamp[16];

	if (STREAM->ConnectTime)
	{
		time_t sessionTime = time(NULL) - STREAM->ConnectTime;
		double avBytesRXed = STREAM->bytesRXed / (sessionTime / 60.0);
		double avBytesSent = STREAM->bytesTXed / (sessionTime / 60.0);
		time_t Now = time(NULL);
		struct tm * TM = localtime(&Now);
		sprintf(timestamp, "%02d:%02d:%02d", TM->tm_hour, TM->tm_min, TM->tm_sec);

		if (sessionTime == 0)
			sessionTime = 1;				// Or will get divide by zero error 
 
		sprintf(Msg, "{\"mode\": \"%s\", \"direction\": \"%s\", \"port\": %d, \"callfrom\": \"%s\", \"callto\": \"%s\", \"time\": %d,  \"bytesSent\": %d," 
			"\"BPMSent\": %4.2f, \"BytesReceived\": %d,  \"BPMReceived\": %4.2f, \"timestamp\": \"%s\"}",
			Modenames[TNC->Hardware - 1], STREAM->Direction, TNC->Port, STREAM->callingCall, STREAM->receivingCall, sessionTime,
			STREAM->bytesTXed,  avBytesSent, STREAM->bytesRXed, avBytesRXed, timestamp);

		if (MQTT)
			MQTTReportSession(Msg);

		STREAM->ConnectTime = 0;
	}
}



void hookNodeStarted()
{
	char UDPMsg[1024];	
	int udplen;
#ifdef LINBPQ
	char Software[80] = "LinBPQ";

	if (sizeof(void *) == 4)
		strcat(Software, "(32 bit)");
#else
	char Software[80] = "BPQ32";
#endif

	if (NodeAPISocket)
	{
		int ret;

		udplen = sprintf(UDPMsg, "{\"@type\": \"NodeUpEvent\", \"nodeCall\": \"%s\", \"nodeAlias\": \"%s\", \"locator\": \"%s\","
			"\"latitude\": %f, \"longitude\": %f, \"software\": \"%s\", \"version\": \"%s\"}",
			NODECALLLOPPED, MYALIASLOPPED, LOC, LatFromLOC, LonFromLOC, Software, VersionString);
   
		ret = sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));

		if (ret != udplen)
			Debugprintf("%d %d %s", ret, WSAGetLastError(), UDPMsg);

	}
}


void hookNodeClosing(char * Reason)
{
	char UDPMsg[1024];	
	int udplen;

	if (NodeAPISocket)
	{
		udplen = sprintf(UDPMsg, "{\"@type\": \"NodeDownEvent\", \"nodeCall\": \"%s\", \"nodeAlias\": \"%s\", \"reason\": \"%s\", \"uptimeSecs\": %d,"
			"\"linksIn\": %d, \"linksOut\": %d, \"cctsIn\": %d, \"cctsOut\": %d, \"l3Relayed\": %d}",
			NODECALLLOPPED, MYALIASLOPPED, Reason, time(NULL) - TimeLoaded, L2CONNECTSIN, L2CONNECTSOUT, L4CONNECTSIN, L4CONNECTSOUT, L3FRAMES);
   
//		Debugprintf(UDPMsg);

		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

void hookNodeRunning()
{
	char UDPMsg[1024];	
	int udplen;
#ifdef LINBPQ
	char Software[80] = "LinBPQ";

	if (sizeof(void *) == 4)
		strcat(Software, "(32 bit)");
#else
	char Software[80] = "BPQ32";
#endif

	if (NodeAPISocket)
	{

		udplen = sprintf(UDPMsg, "{\"@type\": \"NodeStatus\", \"nodeCall\": \"%s\", \"nodeAlias\": \"%s\", \"locator\": \"%s\","
			"\"latitude\": %f, \"longitude\": %f, \"software\": \"%s\", \"version\": \"%s\", \"uptimeSecs\": %d,"
			"\"linksIn\": %d, \"linksOut\": %d, \"cctsIn\": %d, \"cctsOut\": %d, \"l3Relayed\": %d}",
			NODECALLLOPPED, MYALIASLOPPED, LOC, LatFromLOC, LonFromLOC, Software, VersionString, time(NULL) - TimeLoaded,
			L2CONNECTSIN, L2CONNECTSOUT, L4CONNECTSIN, L4CONNECTSOUT, L3FRAMES);

//		Debugprintf(UDPMsg);

		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

void IncomingL4ConnectionEvent(TRANSPORTENTRY * L4)
{
	char UDPMsg[1024];	
	int udplen;

	char remotecall[64];
	char ourcall[64];
	char circuitinfo[32];
	int Service = L4->Service;

	// CACK sent to CREQ

	L4->apiSeq = cctSeq++;
	strcpy(L4->Direction, "incoming");

	if (NodeAPISocket)
	{
		L4->lastStatusSentTime = time(NULL);

		remotecall[ConvFromAX25(L4->L4TARGET.DEST->DEST_CALL, remotecall)] = 0;
	//	remotecall[ConvFromAX25(L4->L4USER, remotecall)] = 0;
		ourcall[ConvFromAX25(L4->L4MYCALL, ourcall)] = 0;
		
		sprintf(circuitinfo, ":%02x%02x", L4->FARINDEX, L4->FARID);
		strcat(remotecall, circuitinfo);

		sprintf(circuitinfo, ":%02x%02x", L4->CIRCUITINDEX, L4->CIRCUITID);
		strcat(ourcall, circuitinfo);

		if (Service == -1)
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"incoming\", "
			"\"remote\": \"%s\", \"local\": \"%s\"}",
			NODECALLLOPPED, L4->apiSeq, remotecall, ourcall);
		else
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"incoming\", "
			"\"service\": %d, \"remote\": \"%s\", \"local\": \"%s\"}",
			NODECALLLOPPED, L4->apiSeq, Service, remotecall, ourcall);


//		Debugprintf(UDPMsg);
		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}


void OutgoingL4ConnectionEvent(TRANSPORTENTRY * L4)
{
	char UDPMsg[1024];	
	int udplen;
	char remotecall[64];
	char ourcall[64];
	char circuitinfo[32];
	int Service = L4->Service;

	// CACK received

	strcpy(L4->Direction, "outgoing");

	L4->apiSeq = cctSeq++;

	if (NodeAPISocket)
	{
		L4->lastStatusSentTime = time(NULL);

		remotecall[ConvFromAX25(L4->L4TARGET.DEST->DEST_CALL, remotecall)] = 0;
	//	remotecall[ConvFromAX25(L4->L4USER, remotecall)] = 0;
		ourcall[ConvFromAX25(L4->L4MYCALL, ourcall)] = 0;
		
		sprintf(circuitinfo, ":%02x%02x", L4->FARID, L4->FARINDEX);
		strcat(remotecall, circuitinfo);

		sprintf(circuitinfo, ":%02x%02x", L4->CIRCUITID, L4->CIRCUITINDEX);
		strcat(ourcall, circuitinfo);

		if (Service == -1)
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"outgoing\", "
			"\"remote\": \"%s\", \"local\": \"%s\"}",
			NODECALLLOPPED, L4->apiSeq, remotecall, ourcall);
		else
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitUpEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"outgoing\", "
			"\"service\": %d, \"remote\": \"%s\", \"local\": \"%s\"}",
			NODECALLLOPPED, L4->apiSeq, Service, remotecall, ourcall);

//		Debugprintf(UDPMsg);
		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

/*
      {
      "@type": "CircuitUpEvent",
      "node": "G8PZT"
      "id": 1,
      "direction": "incoming",
      "service": 0,
      "remote": "G8PZT@G8PZT:14c0",
      "local": "G8PZT-4:0001"
      }


	       "segsSent": 5,
      "segsRcvd": 27,
      "segsResent": 0,
      "segsQueued": 0,
      "reason": "rcvd DREQ"

*/

void L4DisconnectEvent(TRANSPORTENTRY * L4, char * Direction, char * Reason)
{
	char UDPMsg[1024];	
	int udplen;
	char remotecall[64];
	char ourcall[64];
	char circuitinfo[32];
	int Count;

	// CACK received

	if (NodeAPISocket)
	{
		remotecall[ConvFromAX25(L4->L4TARGET.DEST->DEST_CALL, remotecall)] = 0;
//		remotecall[ConvFromAX25(L4->L4USER, remotecall)] = 0;
		ourcall[ConvFromAX25(L4->L4MYCALL, ourcall)] = 0;
		
		sprintf(circuitinfo, ":%02x%02x", L4->FARINDEX, L4->FARID);
		strcat(remotecall, circuitinfo);

		sprintf(circuitinfo, ":%02x%02x", L4->CIRCUITINDEX, L4->CIRCUITID);
		strcat(ourcall, circuitinfo);

			
		if (L4->L4CROSSLINK)		// CONNECTED?
			Count = CountFramesQueuedOnSession(L4->L4CROSSLINK);
		else
			Count = CountFramesQueuedOnSession(L4);

		udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitDownEvent\", \"node\": \"%s\", \"id\": %d, \"direction\": \"%s\","
			"\"service\": %d, \"remote\": \"%s\", \"local\": \"%s\", \"segsSent\": %d, \"segsRcvd\": %d, \"segsResent\": %d, \"segsQueued\": %d, \"reason\": \"%s\"}",
			NODECALLLOPPED, L4->apiSeq, Direction, 0, remotecall, ourcall,L4->segsSent, L4->segsRcvd, L4->segsResent, Count, Reason);

//		Debugprintf(UDPMsg);
		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}

void L4StatusSeport(TRANSPORTENTRY * L4)
{
	char UDPMsg[1024];	
	int udplen;
	char remotecall[64];
	char ourcall[64];
	char nodecall[16];
	char circuitinfo[32];
	int Count;
	time_t Now = time(NULL);
	int Service = L4->Service;



	// Regular Status reports

	if (NodeAPISocket)
	{
		L4->lastStatusSentTime = Now;
		nodecall[ConvFromAX25(L4->L4TARGET.DEST->DEST_CALL, nodecall)] = 0;
		remotecall[ConvFromAX25(L4->L4USER, remotecall)] = 0;
		ourcall[ConvFromAX25(L4->L4MYCALL, ourcall)] = 0;
		
		sprintf(circuitinfo, ":%02x%02x", L4->FARINDEX, L4->FARID);
		strcat(remotecall, circuitinfo);

		sprintf(circuitinfo, ":%02x%02x", L4->CIRCUITINDEX, L4->CIRCUITID);
		strcat(ourcall, circuitinfo);


		if (L4->L4CROSSLINK)		// CONNECTED?
			Count = CountFramesQueuedOnSession(L4->L4CROSSLINK);
		else
			Count = CountFramesQueuedOnSession(L4);

		if (Service == -1)
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitStatus\", \"node\": \"%s\", \"id\": %d, \"direction\": \"%s\","
			"\"upForSecs\": %d,\"remote\": \"%s\", \"local\": \"%s\", \"segsSent\": %d, \"segsRcvd\": %d, \"segsResent\": %d, \"segsQueued\": %d}",
			NODECALLLOPPED, L4->apiSeq, L4->Direction, Now - L4->ConnectTime, remotecall, ourcall,L4->segsSent, L4->segsRcvd, L4->segsResent, Count);
		else
			udplen = sprintf(UDPMsg, "{\"@type\": \"CircuitStatus\", \"node\": \"%s\", \"id\": %d, \"direction\": \"%s\","
			"\"upForSecs\": %d, \"service\": %d, \"remote\": \"%s\", \"local\": \"%s\", \"segsSent\": %d, \"segsRcvd\": %d, \"segsResent\": %d, \"segsQueued\": %d}",
			NODECALLLOPPED, L4->apiSeq, L4->Direction, Now - L4->ConnectTime, Service, remotecall, ourcall,L4->segsSent, L4->segsRcvd, L4->segsResent, Count);

//		Debugprintf(UDPMsg);
		sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));
	}
}


// L2/3/4 Tracing

#define	PFBIT 0x10		// POLL/FINAL BIT IN CONTROL BYTE
#define	NETROM_PID 0xCF
#define	IP_PID 0xCC
#define	ARP_PID 0xCD

char * PIDtoText(int PID)
{
	switch (PID)
	{
		case 240:
			return "DATA";
		case NETROM_PID:
			return "NET/ROM";
		case IP_PID:
			return "IP";
		case ARP_PID:
			return "ARP";
	}
	return "?";
}

void dumpDuffPacket(char * call, char * Buffer, int Len)
{
	// log to syslog in base64

	char * Base64;

	Base64 = byte_base64_encode(Buffer, Len);

	Debugprintf("Trace Error %s %s", call, Base64);
	free(Base64);
}

int checkCall(char * call, unsigned char * Buffer, int Len)
{
	char c;
	int i;

	// Validate source and dest calls - if duff, dump packet

	for (i = 0; i < strlen(call); i++)
	{
		c = call[i];
		
		if (isalnum(c) || c == '-' || c == '@')
			continue;

		dumpDuffPacket(call, Buffer, Len);
		return 0;
	}

	return 1;
}



void APIL2Trace(struct _MESSAGE * Message, char * Dirn)
{
	char UDPMsg[2048];	
	int udplen;
	char srcecall[64];
	char destcall[16];
	char CR[3] = "";
	char PF[2] = "";
	int iLen = 0;
	int CTL = Message->CTL;
	char Type[16] =  "Unknown";
	int UIFlag = 0;
	int IFlag = 0;
	int UFlag = 0;
	int NS;
	int NR;
	struct PORTCONTROL * PORT = GetPortTableEntryFromPortNum(Message->PORT);
	time_t Now = time(NULL);


	if (PORT == 0)
		return;

	if ((Message->ORIGIN[6] & 1) == 0)	// Digis
		return;

	destcall[ConvFromAX25(Message->DEST, destcall)] = 0;
	srcecall[ConvFromAX25(Message->ORIGIN, srcecall)] = 0;

	// Validate source and dest calls - if duff, dump packet

	if (!checkCall(destcall,(char *) Message, Message->LENGTH))
		return;

	if (!checkCall(srcecall, (char *) Message, Message->LENGTH))
		return;
			
	// see if any Digis

	if ((Message->ORIGIN[6] & 1) == 0)	// Digis - ignore for now
		return;

	if ((Message->DEST[6] & 0x80) == 0 && (Message->ORIGIN[6] & 0x80) == 0)
		strcpy(CR, "V1");
	else if ((Message->DEST[6] & 0x80))
		strcpy(CR, "C");
	else if (Message->ORIGIN[6] & 0x80)
		strcpy(CR, "R");
	else
		strcpy(CR, "V1");

	if (CTL & PFBIT)
	{
		if (CR[0] == 'C')
			PF[0] = 'P';
		else if (CR[0] == 'R')
			PF[0] = 'F';
	}

	CTL &= ~PFBIT;

	if ((CTL & 1) == 0)						// I frame
	{
		NS = (CTL >> 1) & 7;			// ISOLATE RECEIVED N(S)
		NR = (CTL >> 5) & 7;

		IFlag = 1;
		iLen = Message->LENGTH - (MSGHDDRLEN + 16);			// Dest origin ctl pid

		strcpy(Type, "I");
	}
	else if (CTL == 3)
	{
		//	Un-numbered Information Frame 

		strcpy(Type, "UI");
		UIFlag = 1;
		iLen = Message->LENGTH - (MSGHDDRLEN + 16);			// Dest origin ctl pid
	}

	if (CTL & 2)
	{
		// UnNumbered

		UFlag = 1;

		switch (CTL)
		{
		case SABM:

			strcpy(Type, "C");
			break;

		case SABME:

			strcpy(Type, "SABME");
			break;

		case XID:

			strcpy(Type, "XID");
			break;

		case TEST:

			strcpy(Type, "TEST");
			break;

		case DISC:

			strcpy(Type, "D");
			break;

		case DM:

			strcpy(Type, "DM");
			break;

		case UA:

			strcpy(Type, "UA");
			break;


		case FRMR:

			strcpy(Type, "FRMR");
			break;
		}
	}
	else
	{
		// Super

		NR = (CTL >> 5) & 7;
		NS = (CTL >> 1) & 7;			// ISOLATE RECEIVED N(S)

		switch (CTL & 0x0F)
		{
		case RR:

			strcpy(Type, "RR");
			break;

		case RNR:

			strcpy(Type, "RNR");
			break;

		case REJ:

			strcpy(Type, "REJ");
			break;

		case SREJ:

			strcpy(Type, "SREJ");
			break;
		}
	}

	// Common to all frame types

	udplen = snprintf(UDPMsg, 2048, 
		"{\"@type\": \"L2Trace\", \"serial\": %d, \"time\": %d, \"dirn\": \"%s\", \"isRF\": %s, \"reportFrom\": \"%s\", \"port\": \"%d\", \"srce\": \"%s\", \"dest\": \"%s\", \"ctrl\": %d,"
		"\"l2Type\": \"%s\", \"modulo\": 8, \"cr\": \"%s\"",
		UDPSeq++, (int)Now, Dirn, (PORT->isRF)?"true":"false", NODECALLLOPPED, Message->PORT, srcecall, destcall, Message->CTL, Type, CR);

	if (UIFlag)
	{
		udplen += snprintf(&UDPMsg[udplen], 2048 - udplen,
			", \"ilen\": %d, \"pid\": %d, \"ptcl\": \"%s\"", iLen, Message->PID, PIDtoText(Message->PID));

		if (Message->PID == NETROM_PID)
		{
			udplen += decodeNETROMUIMsg(Message->L2DATA, iLen, &UDPMsg[udplen], 2048 - udplen);
		}
	}
	else if (IFlag)
	{
		if (PF[0])
			udplen += snprintf(&UDPMsg[udplen], 2048 - udplen,
				", \"ilen\": %d, \"pid\": %d, \"ptcl\": \"%s\", \"pf\": \"%s\", \"rseq\": %d, \"tseq\": %d",
				iLen, Message->PID, PIDtoText(Message->PID), PF, NR, NS);
		else
			udplen += snprintf(&UDPMsg[udplen], 2048 - udplen,
				", \"ilen\": %d, \"pid\": %d, \"ptcl\": \"%s\", \"rseq\": %d, \"tseq\": %d",
				iLen, Message->PID, PIDtoText(Message->PID), NR, NS);

		if (Message->PID == NETROM_PID)
		{
			int n = decodeNETROMIFrame(Message->L2DATA, iLen, &UDPMsg[udplen], 2048 - udplen);

			if (n == 0)
				return;				// Can't decode so don't trace anything;

			udplen += n;
		}

	}
	else if (UFlag)
	{
		if (PF[0])
			udplen += snprintf(&UDPMsg[udplen], 2048 - udplen, ", \"pf\": \"%s\"", PF);
	}
	else
	{
		// supervisory

		if (PF[0])
			udplen += snprintf(&UDPMsg[udplen], 2048 - udplen, ", \"pf\": \"%s\", \"rseq\": %d", PF, NR);
		else
			udplen += snprintf(&UDPMsg[udplen], 2048 - udplen, ", \"rseq\": %d", NR);
	}


	UDPMsg[udplen++] = '}';
	UDPMsg[udplen] = 0;
//	Debugprintf(UDPMsg);
	sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));

}


 //"@type" = @L3Trace, reportFrom, time, dirn, 

void NetromTCPTrace(struct _MESSAGE * Message, char * Dirn)
{
	char UDPMsg[2048];	
	int udplen;
	time_t Now = time(NULL);
	int iLen = Message->LENGTH - (15 + MSGHDDRLEN);
	int isRF = 0;


	udplen = snprintf(UDPMsg, 2048, 
		"{\"@type\": \"L3Trace\", \"serial\": %d, \"time\": %d, \"dirn\": \"%s\", \"isRF\": %s, \"reportFrom\": \"%s\", \"port\": %d",
		UDPSeq++, (int)Now, Dirn, (isRF)?"true":"false", NODECALLLOPPED, Message->PORT);


	udplen += snprintf(&UDPMsg[udplen], 2048 - udplen,
		", \"ilen\": %d, \"pid\": %d, \"ptcl\": \"%s\"",
		iLen, Message->PID, PIDtoText(Message->PID));

	if (Message->PID == NETROM_PID)
	{
		int n = decodeNETROMIFrame(Message->L2DATA, iLen, &UDPMsg[udplen], 2048 - udplen);

		if (n == 0)
			return;				// Can't decode so don't trace anything;

		udplen += n;
	}



	UDPMsg[udplen++] = '}';
	UDPMsg[udplen] = 0;
//	Debugprintf(UDPMsg);
	sendto(NodeAPISocket, UDPMsg, udplen, 0, (struct sockaddr *)&UDPreportdest, sizeof(UDPreportdest));

}




int decodeNETROMUIMsg(unsigned char * Msg, int iLen, char * Buffer, int BufferLen)
{
	int Len = 0;

	// UI with NETROM PID are assumed to by NODES broadcast (INP3 routes are sent in I frames)

	// But check first byte is 0xff to be sure, or 0xfe for Paula's	char Alias[7]= "";

	char Dest[10];
	char Node[10];
	char Alias[10] = "";

	memcpy(Alias, &Msg[1], 6);
	strlop(Alias, ' ');

	if (Msg[0] == 0xfe)			// Paula's Nodes Poll
	{
		Len = snprintf(Buffer, BufferLen, ", \"l3Type\": \"Routing info\", \"type\": \"Routing poll\"");
		return Len;
	}

	if (Msg[0] != 0xff)
		return 0;

	Msg += 7;			// to first field

	Len = snprintf(Buffer, BufferLen, ", \"l3Type\": \"Routing info\", \"type\": \"NODES\", \"fromAlias\": \"%s\", \"nodes\": [", Alias);

	iLen -= 7;					//Header, mnemonic and signature length

	if (iLen < 21)				// No Entries
	{
		Buffer[Len++] = ']';
		return Len;
	}

	while(iLen > 20)				// Entries are 21 bytes
	{
		Dest[ConvFromAX25(Msg, Dest)] = 0;
		Msg +=7;
		memcpy(Alias, Msg, 6);
		Msg +=6;
		strlop(Alias, ' ');
		Node[ConvFromAX25(Msg, Node)] = 0;
		Msg +=7;

		Len += snprintf(&Buffer[Len], BufferLen - Len, "{\"call\":  \"%s\", \"alias\": \"%s\", \"via\": \"%s\", \"qual\": %d},", Dest, Alias, Node, Msg[0]);
		Msg++;
		iLen -= 21;
	}
	// Have to replace trailing , with ]

	Buffer[Len - 1] = ']';
	return Len;
}

int decodeNETROMIFrame(unsigned char * Msg, int iLen, char * Buffer, int BufferLen)
{
	int Len = 0;
	L3MESSAGE * L3MSG = (L3MESSAGE *)Msg;
	char srcecall[64];
	char destcall[16];
	char srcUser[16];
	char srcNode[16];
	int Opcode;
	int netromx = 0;
	int service = 0;


	if (Msg[0] == 0xff)				// RIF?
		return decodeINP3RIF(&Msg[1], iLen - 1, Buffer, BufferLen);

	// Netrom L3 /4 frame. Do standard L3 header

	destcall[ConvFromAX25(L3MSG->L3DEST, destcall)] = 0;
	srcecall[ConvFromAX25(L3MSG->L3SRCE, srcecall)] = 0;

	if (!checkCall(destcall, Msg, iLen))
		return 0;

	if (!checkCall(srcecall, Msg, iLen))
		return 0;


	if (strcmp(destcall, "KEEPLI") == 0)
		return 0;

	Len = snprintf(Buffer, BufferLen, ", \"l3Type\": \"NetRom\", \"l3src\": \"%s\", \"l3dst\": \"%s\", \"ttl\": %d", srcecall, destcall, L3MSG->L3TTL);

	// L4 Stuff

	Opcode = L3MSG->L4FLAGS & 15;

	switch (Opcode)
	{
	case 0:

		//	OPCODE 0 is used for a variety of functions, using L4INDEX and L4ID as qualifiers
		//	0c0c is used for IP. Ignore for now

		//	 00 01 Seesm to be Netrom Record Route

		if (L3MSG->L4ID == 1 && L3MSG->L4INDEX == 0)
		{
			Len += decodeRecordRoute(L3MSG, iLen, &Buffer[Len], BufferLen - Len);
			return Len;
		}

	case L4CREQX:

		netromx = 1;
		service = (L3MSG->L4RXNO << 8) | L3MSG->L4TXNO;

	case L4CREQ:

		srcUser[ConvFromAX25(&L3MSG->L4DATA[1], srcUser)] = 0;
		srcNode[ConvFromAX25(&L3MSG->L4DATA[8], srcNode)] = 0;

		if (!checkCall(srcUser, Msg, iLen))
			return 0;


		if (!checkCall(srcNode, Msg, iLen))
			return 0;


		if (netromx)
			Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"CONN REQX\", \"fromCct\": %d, \"srcUser\": \"%s\", \"srcNode\": \"%s\", \"window\": %d, \"service\": %d",
				(L3MSG->L4INDEX << 8) | L3MSG->L4ID, srcUser, srcNode, L3MSG->L4DATA[0], service);
		else
			Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"CONN REQ\", \"fromCct\": %d, \"srcUser\": \"%s\", \"srcNode\": \"%s\", \"window\": %d",
				(L3MSG->L4INDEX << 8) | L3MSG->L4ID, srcUser, srcNode, L3MSG->L4DATA[0]);

		return Len;

	case L4CACK:

		// Can be ACK or NACK depending on Choke flag

		if (L3MSG->L4FLAGS & L4BUSY)	
			Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"CONN NAK\", \"toCct\": %d",
				(L3MSG->L4INDEX << 8) | L3MSG->L4ID);
		else
			Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"CONN ACK\", \"toCct\": %d, \"fromCct\": %d, \"accWin\": %d",
				(L3MSG->L4INDEX << 8) | L3MSG->L4ID, (L3MSG->L4TXNO << 8) | L3MSG->L4RXNO, L3MSG->L4DATA[0]);

		return Len;


	case L4INFO:

		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"INFO\", \"toCct\": %d, \"txSeq\": %d, \"rxSeq\": %d, \"paylen\": %d",
			(L3MSG->L4INDEX << 8) | L3MSG->L4ID, L3MSG->L4TXNO, L3MSG->L4RXNO, iLen - 20);

		return Len;

	case L4IACK:

		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"INFO ACK\", \"toCct\": %d,  \"rxSeq\": %d",
			(L3MSG->L4INDEX << 8) | L3MSG->L4ID, L3MSG->L4RXNO);

		return Len;

		
	case L4DREQ:

		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"DISC REQ\", \"toCct\": %d", (L3MSG->L4INDEX << 8) | L3MSG->L4ID);
		return Len;

	case L4DACK:

		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"DISC ACK\", \"toCct\": %d", (L3MSG->L4INDEX << 8) | L3MSG->L4ID);
		return Len;

	case L4RESET:

		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"RSET\", \"fromCct\": %d", (L3MSG->L4INDEX << 8) | L3MSG->L4ID);
		return Len;

		
		/*
	 "NRR Request"   Netrom Record Route Request
         "NRR Reply"     Netrom Record Route Reply
         "CONN REQ"      Connect Request
         "CONN REQX"     Extended Connect Request
         "CONN ACK"      Connection Acknowledgement
         "CONN NAK"      Connection Negative Ack (refusal)
         "DISC REQ"      Disconnect request
         "DISC ACK"      Disconnect Acknowledgement
         "INFO"          Information-bearing frame
         "INFO ACK"      Acknowledgement for an INFO frame.
         "RSET"          Circuit Reset (kill)
         "PROT EXT"      Protocol Extension (e.g. IP, NCMP etc)
         "unknown"       Unrecognised type (shouldn't happen)
		



        "l4type": "CONN ACK",
        "fromCct": 10,
        "toCct": 23809,
       "accWin": 4,
 */ 
 
	}
	return Len;
}

int decodeRecordRoute(L3MESSAGE * L3, int iLen, char * Buffer, int BufferLen)
{
	int Len = 0;
	char callList[512];
	char * ptr1 = callList;
	unsigned char * ptr = L3->L4DATA;
	char call[16];
	int Response = 0;

	iLen -= 20;

	while (iLen > 0)
	{
		call[ConvFromAX25(ptr, call)] = 0;
		
		ptr1 += sprintf(ptr1, " %s", call);
			
		if ((ptr[7] & 0x80) == 0x80)			// Check turnround bit
		{
			*ptr1++ = '*';
			Response = 1;
		}

		ptr += 8;
		iLen -= 8;
	}

	*ptr1 = 0;

	if (Response)
		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"NRR Reply\", \"nrrId\": %d, \"nrrRoute\": \"%s\"", 
			(L3->L4TXNO << 8) | L3->L4RXNO, callList);
	else
		Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"l4Type\": \"NRR Request\", \"nrrId\": %d, \"nrrRoute\": \"%s\"", 
			(L3->L4TXNO << 8) | L3->L4RXNO, callList);

//	Debugprintf(Buffer);

	return Len;
}


int decodeINP3RIF(unsigned char * Msg, int iLen, char * Buffer, int BufferLen)
{
	char call[10];
	int calllen;
	int hops;
	unsigned short rtt;
	unsigned int len;
	unsigned int opcode;
	char alias[10] = "";
	UCHAR IP[6];
	int i;
	int Len = 0;

	Len = snprintf(Buffer, BufferLen, ", \"l3Type\": \"Routing info\", \"type\": \"INP3\", \"nodes\": [");

	if (iLen < 10)				// No Entries
	{
		Buffer[Len++] = ']';
		return Len;
	}

	while (iLen > 1)
	{
		calllen = ConvFromAX25(Msg, call);
		call[calllen] = 0;

		// Validate the call

		for (i = 0; i < calllen; i++)
		{
			if (!isupper(call[i]) && !isdigit(call[i]) && call[i] != '-')
				return 0;
		}

		Msg+=7;

		hops = *Msg++;
		rtt = (*Msg++ << 8);
		rtt += *Msg++;

		IP[0] = 0;
		strcpy(alias, "      ");

		iLen -= 10;

		// Process optional fields

		while (*Msg && iLen > 0)			//  Have an option
		{
			len = *Msg;
			opcode = *(Msg+1);

			if (len < 2 || len > iLen)
				return 0;

			if (opcode == 0 && len < 9)
			{
				memcpy(alias, Msg+2, len-2);
			}
			else if (opcode == 1 && len < 8)
			{
				memcpy(IP, Msg+2, len-2);
			}

			Msg += len;
			iLen -= len;

		}

		Len += snprintf(&Buffer[Len], BufferLen - Len, "{\"call\": \"%s\", \"hops\": %d, \"tt\": %d", call, hops, rtt);

		if (alias[0] > ' ')
			Len += snprintf(&Buffer[Len], BufferLen - Len, ", \"alias\":  \"%s\"", alias);

		Buffer[Len++] = '}';
		Buffer[Len++] = ',';

		Msg++;
		iLen--;		// Over EOP

	}
	// Have to replace trailing , with ]

	Buffer[Len - 1] = ']';
	return Len;
}

