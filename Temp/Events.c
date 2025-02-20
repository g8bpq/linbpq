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


extern char Modenames[19][10];

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
	// Incoming SABM

	LINK->ConnectTime = time(NULL);
	LINK->bytesTXed = LINK->bytesRXed = 0;

	strcpy(LINK->callingCall, remotecall);
	strcpy(LINK->receivingCall, ourcall);
	strcpy(LINK->Direction, "In");
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
			char timestamp[16];
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
		}

		LINK->ConnectTime = 0;
	}
}

void hookL2SessionAttempt(int Port, char * ourcall, char * remotecall, struct _LINKTABLE * LINK)
{
	LINK->ConnectTime = time(NULL);
	LINK->bytesTXed = LINK->bytesRXed = 0;

	strcpy(LINK->callingCall, ourcall);
	strcpy(LINK->receivingCall, remotecall);
	strcpy(LINK->Direction, "Out");
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


