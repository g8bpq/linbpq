/*
Copyright 2001-2015 John Wiseman G8BPQ

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

// Only used on Windows. Linux uses linether.c

//
//	DLL to provide BPQEther support for G8BPQ switch in a 
//	32bit environment,
//
//	Uses BPQ EXTERNAL interface
//
//	Uses WINPACAP library

//	Version 1.0 December 2005

//  Version 1.1	January 2006
//
//		Get config file location from Node (will check bpq directory)

//  Version 1.2	October 2006

//		Write diagnostics to BPQ console window instead of STDOUT

//	Version 1.3 February 2008

//		Changes for dynamic unload of bpq32.dll

// Version 1.3.1 Spetmber 2010

//		Add option to get config from bpq32.dll


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>

//#include <time.h>

#include "CHeaders.h"
#include <process.h>
#include "pcap.h"

#include "../CommonSource/bpq32.h"

//#include "packet32.h"
//#include "ntddndis.h"
 
extern char * PortConfig[33];

typedef struct PCAPStruct
{
   pcap_t	*adhandle;
   UCHAR	EthSource[6];
   UCHAR	EthDest[6];
   short	EtherType;
   BOOL		RLITX;
   BOOL		RLIRX;
   BOOL		Promiscuous;
   int		pcap_reopen_delay;
   char		Adapter[256];

} PCAPINFO, *PPCAPINFO ;

PCAPINFO * PCAPInfo[32];

//PPCAPINFO PCAPInfo[16]={0};

UCHAR EthDest[7]={01,'B','P','Q',0,0};

char EtherType[10]="0x08FF";

//pcap_t *adhandle;

struct tagMSG Msg;

short udpport=0;

extern UCHAR BPQDirectory[];

unsigned int OurInst = 0;

HWND hWnd;
BOOL GotMsg;

DWORD n;

static HINSTANCE PcapDriver=0;

typedef int (FAR *FARPROCX)();

int (FAR * pcap_sendpacketx)();

FARPROCX pcap_compilex;
FARPROCX pcap_setfilterx;
FARPROCX pcap_datalinkx;
FARPROCX pcap_next_exx;
FARPROCX pcap_geterrx;
pcap_t * (FAR * pcap_open_livex)(const char *, int, int, int, char *);

static char Dllname[6]="wpcap";

FARPROCX GetAddress(char * Proc);

int InitPCAP(void);
static FARPROCX GetAddress(char * Proc);

static BOOL ReadConfigFile(int Port);
static int ProcessLine(char * buf,int Port, BOOL CheckPort);
static int OpenPCAP(PPCAPINFO PCAP);

static size_t ExtProc(int fn, int port, PMESSAGE buff)
{
	int len,txlen=0,res;
	char txbuff[500];
	struct pcap_pkthdr *header;
	u_char *pkt_data;
	PPCAPINFO PCAP = PCAPInfo[port];

	//	char dcall[10],scall[10];

	if (PCAP ==NULL)
		return 0;

	if (PCAP->adhandle == 0)
	{
		// No handle. 
		
		if (PCAP->Adapter[0])	
		{
			// Try reopening periodically
			
			PCAP->pcap_reopen_delay --;
			
			if (PCAP->pcap_reopen_delay < 0)
				if (OpenPCAP(PCAP) == FALSE)
					PCAP->pcap_reopen_delay = 300;	// Retry every 30 seconds
		}
		return 0;
	}
	switch (fn)
	{
	case 1:				// poll

		res = pcap_next_exx(PCAP->adhandle, &header, &pkt_data);

		if (res == 0)
			/* Timeout elapsed */
			return 0;

		if (res == -1)
		{
			// Failed - try to reopen
			
			if (OpenPCAP(PCAP) == FALSE)
				PCAP->pcap_reopen_delay = 300;
			return 0;
		}

		// The message has the 14 Byte Etherent II header, followed
		// by a length field and the ax.25 packet.

		// The length seems to include the length field and an extra
		// 3 bytes. I can;t remember if this is a bug or I had a good reason at the time


		if (PCAP->RLIRX)
		{
		//	RLI MODE - An extra 3 bytes before len, seem to be 00 00 41
			len=pkt_data[18]*256 + pkt_data[17];

			if ((len < 16) || (len > 320)) return 0; // Probably BPQ Mode Frame

			len-=3;
		
			memcpy(&buff->DEST, &pkt_data[19], len);
			len += (1 + sizeof(void *));

		}
		else
		{
			len=pkt_data[15]*256 + pkt_data[14];

			if ((len < 16) || (len > 320)) return 0; // Probably RLI Mode Frame

			memcpy(&buff->DEST, &pkt_data[16], len);
		
			// The DATAMESSAGE struct has a chain word, port and length 
			// field - 7 bytes on 32 bit and 11 on 64. MSGHDDRLIN 
			// has this value

			// We take off 5 - 2 for length and 3 for who knows why

			len += (MSGHDDRLEN - 5);
		}
		
		PutLengthinBuffer((PDATAMESSAGE)buff, len);

		return 1;

		
	case 2:				// send
		
 		if (PCAP->RLITX)
		
		//	RLI MODE - An extra 3 bytes before len, seem to be 00 00 41
		{

			txlen = GetLengthfromBuffer((PDATAMESSAGE)buff); // 2 for CRC
			txlen -= 2;
			txbuff[16]=0x41;
			txbuff[17]=(txlen & 0xff);
			txbuff[18]=(txlen >> 8);

			if (txlen < 1 || txlen > 400)
				return 0;
			
			memcpy(&txbuff[19], &buff->L2DATA[0], txlen);

		}
		else
		{
			txlen = GetLengthfromBuffer((PDATAMESSAGE)buff); 

			txlen -= (sizeof(void *) - 2);

			txbuff[14]=(txlen & 0xff);
			txbuff[15]=(txlen >> 8);

			if (txlen < 1 || txlen > 400)
				return 0;

			memcpy(&txbuff[16], &buff->DEST[0], txlen);
		}

		memcpy(&txbuff[0],&PCAP->EthDest[0],6);
		memcpy(&txbuff[6],&PCAP->EthSource[0],6);
		memcpy(&txbuff[12],&PCAP->EtherType,2);

		txlen += 14;
		
		if (txlen < 60) txlen = 60;

		// Send down the packet 

		if (pcap_sendpacketx(PCAP->adhandle,	// Adapter
			txbuff,				// buffer with the packet
			txlen				// size
			) != 0)
		{

	//		n=sprintf(buf,"\nError sending the packet: \n", pcap_geterrx(PCAPInfo[port].adhandle));		
	//		WritetoConsole(buf);
			
			return 3;
		}


		return (0);


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK	

	case 4:				// reinit

		return 0;

	case 5:				// reinit

		return 0;
	}

	return (0);
}


void * ETHERExtInit(struct PORTCONTROL *  PortEntry)
{
	//	Can have multiple ports, each mapping to a different Ethernet Adapter
	
	//	The Adapter number is in IOADDR

	int Port = PortEntry->PORTNUMBER;
	PPCAPINFO PCAP;
	char buf[80];

	if (InitPCAP())		// Make sure pcap is installed
	{
		WritetoConsole("BPQEther ");

		//
		//	Read config first, to get UDP info if needed
		//

		PCAP = PCAPInfo[Port] = zalloc(sizeof(struct PCAPStruct));

		if (!ReadConfigFile(Port))
		return (FALSE);

		if (OpenPCAP(PCAP))
			sprintf(buf,"Using %s\n", PCAP->Adapter);
		else
			sprintf(buf,"Failed to open %s\n", PCAP->Adapter);
		
		WritetoConsole(buf);
	}
	return ExtProc;
}


InitPCAP()
{
	char Msg[255];
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;

//	Use LoadLibrary/GetProcADDR to get round link problem

	if (PcapDriver)
		return TRUE;						// Already loaded

	PcapDriver=LoadLibrary(Dllname);

	if (PcapDriver == NULL)
	{
		err=GetLastError();
		sprintf(Msg,"Error loading Driver %s - Error code %d", Dllname,err);
		WritetoConsole(Msg);
		return(FALSE);
	}

	if ((pcap_sendpacketx=GetAddress("pcap_sendpacket")) == 0 ) return FALSE;

	if ((pcap_datalinkx=GetAddress("pcap_datalink")) == 0 ) return FALSE;

	if ((pcap_compilex=GetAddress("pcap_compile")) == 0 ) return FALSE;

	if ((pcap_setfilterx=GetAddress("pcap_setfilter")) == 0 ) return FALSE;
	
	if ((pcap_open_livex = (pcap_t * (__cdecl *)())
		GetProcAddress(PcapDriver,"pcap_open_live")) == 0 ) return FALSE;

	if ((pcap_geterrx=GetAddress("pcap_geterr")) == 0 ) return FALSE;

	if ((pcap_next_exx=GetAddress("pcap_next_ex")) == 0 ) return FALSE;
	
	return (TRUE);
}

FARPROCX GetAddress(char * Proc)
{
	FARPROCX ProcAddr;
	int err=0;
	char buf[256];
	int n;


	ProcAddr=(FARPROCX) GetProcAddress(PcapDriver,Proc);

	if (ProcAddr == 0)
	{
		err=GetLastError();

		n=sprintf(buf,"Error finding %s - %d\n", Proc,err);
		WritetoConsole(buf);
	
		return(0);
	}

	return ProcAddr;
}

#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

int OpenPCAP(PPCAPINFO PCAP)
{
	int i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	char packet_filter[30];
	struct bpf_program fcode;
	char buf[256];
	int n;

	/* Open the adapter */
	if ((PCAP->adhandle= pcap_open_livex(PCAP->Adapter,	// name of the device
							 65536,			// portion of the packet to capture. 
											// 65536 grants that the whole packet will be captured on all the MACs.
							 PCAP->Promiscuous,	// promiscuous mode (nonzero means promiscuous)
							 1,				// read timeout
							 errbuf			// error buffer
							 )) == NULL)
	{
		return FALSE;
	}
	
	/* Check the link layer. We support only Ethernet for simplicity. */
	if(pcap_datalinkx(PCAP->adhandle) != DLT_EN10MB)
	{
		n=sprintf(buf,"This program works only on Ethernet networks.\n");
		WritetoConsole(buf);
		
		return FALSE;
	}

	netmask=0xffffff; 

	sprintf(packet_filter,"ether[12:2]=0x%x",
		ntohs(PCAP->EtherType));

	//compile the filter
	if (pcap_compilex(PCAP->adhandle, &fcode, packet_filter, 1, netmask) <0 )
	{	
		n=sprintf(buf,"Unable to compile the packet filter. Check the syntax.\n");
		WritetoConsole(buf);

		/* Free the device list */
		return FALSE;
	}
	
	//set the filter

	if (pcap_setfilterx(PCAP->adhandle, &fcode)<0)
	{
		n=sprintf(buf,"Error setting the filter.\n");
		WritetoConsole(buf);

		/* Free the device list */
		return FALSE;
	}
	
	return TRUE;

}

static BOOL ReadConfigFile(int Port)
{
//TYPE	1	08FF                            # Ethernet Type
//ETH	1	FF:FF:FF:FF:FF:FF				#	Target Ethernet AddrMAP G8BPQ-7 10.2.77.1                  # IP 93 for compatibility
//ADAPTER	1	\Device\NPF_{21B601E8-8088-4F7D-96 29-EDE2A9243CF4}	# Adapter Name

	char buf[256],errbuf[256];
	char * Config;
	PPCAPINFO PCAP = PCAPInfo[Port];

	Config = PortConfig[Port];

	PCAP->Promiscuous = 1;				// Defaults
	PCAP->EtherType=htons(0x08FF);
	memset(PCAP->EthDest, 0xff, 6); 

	if (Config)
	{
		// Using config from bpq32.cfg

		char * ptr1 = Config, * ptr2;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf, Port, FALSE))
			{
				WritetoConsole("BPQEther - Bad config record ");
				WritetoConsole(errbuf);
				WritetoConsole("\n");
			}
		}
		return (TRUE);
	}
		
	n=sprintf(buf,"No config info found in bpq32.cfg\n");
	WritetoConsole(buf);

	return (FALSE);
}

static int ProcessLine(char * buf, int Port, BOOL CheckPort)
{
	char * ptr;
	char * p_port;
	char * p_mac;
	char * p_Adapter;
	char * p_type;

	int	port;
	int a,b,c,d,e,f,num;

	PPCAPINFO PCAP = PCAPInfo[Port];

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if (CheckPort)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
	}

	if(_stricmp(ptr,"ADAPTER") == 0)
	{
		IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information for up to 16 NICs
		DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer
		DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
 
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

		p_Adapter = strtok(NULL, " \t\n\r");
		
		strcpy(PCAP->Adapter, p_Adapter);

		// Look for MAC Address
		
		do
		{
			if (strcmp(pAdapterInfo->AdapterName, &PCAP->Adapter[12]) == 0)
			{
				memcpy(PCAP->EthSource, pAdapterInfo->Address, 6);
				break;
			}

			pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list

		} while(pAdapterInfo);                    // Terminate if last adapter

		return (TRUE);
	}

	if(_stricmp(ptr,"TYPE") == 0)
	{
		p_type = strtok(NULL, " \t\n\r");
		
		if (p_type == NULL) return (FALSE);

		num=sscanf(p_type,"%x",&a);

		if (num != 1) return FALSE;

		PCAP->EtherType=htons(a);
		return (TRUE);

	}

	if(_stricmp(ptr,"promiscuous") == 0)
	{
		ptr = strtok(NULL, " \t\n\r");
		
		if (ptr == NULL) return (FALSE);

		PCAP->Promiscuous = atoi(ptr);

		return (TRUE);

	}

	if(_stricmp(ptr,"RXMODE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		if(_stricmp(p_port,"RLI") == 0)
		{
			PCAP->RLIRX=TRUE;
			return (TRUE);
		}

		if(_stricmp(p_port,"BPQ") == 0)
		{
			PCAP->RLIRX=FALSE;
			return (TRUE);
		}

		return FALSE;
	
	}

	if(_stricmp(ptr,"TXMODE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		if(_stricmp(p_port,"RLI") == 0)
		{
			PCAP->RLITX=TRUE;
			return (TRUE);
		}

		if(_stricmp(p_port,"BPQ") == 0)
		{
			PCAP->RLITX=FALSE;
			return (TRUE);
		}

		return FALSE;

	}

	if(_stricmp(ptr,"DEST") == 0)
	{
		p_mac = strtok(NULL, " \t\n\r");
		
		if (p_mac == NULL) return (FALSE);

		num=sscanf(p_mac,"%x-%x-%x-%x-%x-%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		PCAP->EthDest[0]=a;
		PCAP->EthDest[1]=b;
		PCAP->EthDest[2]=c;
		PCAP->EthDest[3]=d;
		PCAP->EthDest[4]=e;
		PCAP->EthDest[5]=f;


	//	strcpy(Adapter,p_Adapter);

		return (TRUE);
	}

	if(_stricmp(ptr,"SOURCE") == 0)
	{	
		p_mac = strtok(NULL, " \t\n\r");
		
		if (p_mac == NULL) return (FALSE);

		num=sscanf(p_mac,"%x-%x-%x-%x-%x-%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		PCAP->EthSource[0]=a;
		PCAP->EthSource[1]=b;
		PCAP->EthSource[2]=c;
		PCAP->EthSource[3]=d;
		PCAP->EthSource[4]=e;
		PCAP->EthSource[5]=f;

	//	strcpy(Adapter,p_Adapter);

		return (TRUE);

	}
	//
	//	Bad line
	//
	return (FALSE);
	
}
	


