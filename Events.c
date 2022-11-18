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

VOID __cdecl Debugprintf(const char * format, ...);

#ifndef WIN32

#define APIENTRY
#define DllExport
#define VOID void

#else
#include <windows.h>
#endif

extern BOOL EventsEnabled;

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

DllExport int APIENTRY RunEventProgram(char * Program, char * Param)
{
	int n = 0;
	char cmdLine[256];

	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
	PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

	if (EventsEnabled == 0)
		return 0;


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

	return 0;
}
