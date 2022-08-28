// Includes code from MiniUPnPc, used subject to the following conditions:

/*

MiniUPnPc
Copyright (c) 2005-2020, Thomas BERNARD
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#define MINIUPNP_STATICLIB

#include "upnpcommands.h"
#include "miniupnpc.h"
#include "upnperrors.h"
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

int AddMap(char * controlURL, char * eport, char * iport, char * proto);
int DeleteMap(char * controlURL, char * eport, char * iport, char * proto);

void Consoleprintf(const char * format, ...);

struct UPNP 
{
	struct UPNP * Next;
	char * Protocol;
	char * LANport;
	char * WANPort;
};

extern struct UPNP * UPNPConfig;

char * controlURL = 0;
char * servicetype = 0;
char iaddr[] = "IP";
char * inClient = NULL;
#ifdef LINBPQ
char desc[] = "LinBPQ ";
#else
char desc[] = "BPQ32 ";
#endif
char * remoteHost = NULL;
char * leaseDuration = NULL;

struct UPNPDev * devlist = 0;
char lanaddr[64] = "unset";	/* my ip address on the LAN */
struct UPNPUrls urls;
struct IGDdatas data;

int i;
const char * rootdescurl = 0;
const char * multicastif = 0;
const char * minissdpdpath = 0;
int localport = UPNP_LOCAL_PORT_ANY;
int retcode = 0;
int error = 0;
int ipv6 = 0;
int ignore = 0;
unsigned char ttl = 2;


int upnpInit()
{
	struct UPNP * Config = UPNPConfig;
	int i;
#ifdef WIN32
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(nResult != NO_ERROR)
	{
		fprintf(stderr, "WSAStartup() failed.\n");
		return -1;
	}
#endif

	while (Config)
	{
		if (devlist == NULL)
		{
			devlist = upnpDiscover(2000, multicastif, minissdpdpath, localport, ipv6, ttl, &error);

			if (devlist == NULL)
			{
				Consoleprintf("Failed to find a UPNP device");
				return 0;
			}

			i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
		}
		
		AddMap(devlist->descURL, Config->LANport, Config->WANPort, Config->Protocol);		
		Config = Config->Next;
	}

	return 0;
}

int upnpClose()
{
	struct UPNP * Config = UPNPConfig;
	int i;

	while (Config)
	{
		if (devlist == NULL)
		{
			devlist = upnpDiscover(2000, multicastif, minissdpdpath, localport, ipv6, ttl, &error);

			if (devlist == NULL)
			{
				Consoleprintf("Failed to find a UPNP device");
				return 0;
			}

			i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
		}
		
		DeleteMap(devlist->descURL, Config->LANport, Config->WANPort, Config->Protocol);		
		Config = Config->Next;
	}

	return 0;
}

int AddMap(char * controlURL, char * eport, char * iport, char * proto)
{
	int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
					eport, iport, lanaddr, desc,
					proto, remoteHost, leaseDuration);
		
	if (r != UPNPCOMMAND_SUCCESS)
	{
		Consoleprintf("UPNP AddPortMapping(%s, %s, %s) failed with code %d (%s)", eport, iport, lanaddr, r, strupnperror(r));
		return -2;
	}
	Consoleprintf("UPNP AddPortMapping(%s, %s, %s) Succeeded", eport, iport, lanaddr, r);
	return 0;
}

int DeleteMap(char * controlURL, char * eport, char * iport, char * proto)
{
	int r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, eport, proto, remoteHost);

	if(r != UPNPCOMMAND_SUCCESS)
	{
		Consoleprintf("UPNP DeletePortMapping(%s, %s, %s) failed with code %d (%s)", eport, iport, lanaddr, r, strupnperror(r));
		return -2;
	}
	Consoleprintf("UPNP DeletePortMapping(%s, %s, %s) Succeeded", eport, iport, lanaddr, r);

	return 0;
}




