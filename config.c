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


//	July 2010

//	BPQ32 now reads bpqcfg.txt. This module converts it to the original binary format.

//	Based on the standalonw bpqcfg.c

/************************************************************************/
/*	CONFIG.C  Jonathan Naylor G4KLX, 19th November 1988		*/
/*									*/
/*	Program to produce configuration file for G8BPQ Network Switch	*/
/*	based on the original written in BASIC by G8BPQ.		*/
/*									*/
/*	Subsequently extended by G8BPQ					*/
/*									*/
/************************************************************************/
//
//	22/11/95 - Add second port alias for digipeating (for APRS)
//		 - Add PORTMAXDIGIS param

//	1999 - Win32 Version (but should also compile in 16 bit

//	5/12/99 - Add DLLNAME Param for ext driver

//	26/11/02 - Added AUTOSAVE

// Jan 2006

//		Add params for input and output names
//		Wait before exiting if error detected

// March 2006

//		Accept # as comment delimiter
//		Display input and output filenames
//		Wait before exit, even if ok

// March 2006 

//		Add L4APPL param

// Jan 2007

//		Remove UNPROTO
//		Add BTEXT
//		Add BCALL

// Nov 2007

//		Convert calls and APPLICATIONS string to upper case

// Jan 2008

//		Remove trailing space from UNPROTO
//		Don't warn BBSCALL etc missing if APPL1CALL etc present

// August 2008

//		Add IPGATEWAY Parameter
//		Add Port DIGIMASK Parameter

// December 2008

//		Add C_IS_CHAT Parameter

// March 2009

//		Add C style COmments (/* */ at start of line)

// August 2009

//		Add INP3 flag to locked routes

// November 2009

//		Add PROTOCOL=PACTOR or WINMOR option

//	December 2009

//		Add INP3 MAXRTT and MAXHOPS Commands

// March 2010

//		Add SIMPLE mode

// March 2010

//		Change SIMPLE mode default of Full_CTEXT to 1

// April 2010

//		Add NoKeepAlive ROUTE option

// Converted to intenal bpq32 process

// Spetember 2010

// Add option of embedded port configuration 



#define _CRT_SECURE_NO_DEPRECATE

#include "cheaders.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "configstructs.h"

// KISS Options Equates

#define CHECKSUM 1
#define POLLINGKISS	2			// KISSFLAGS BITS
#define ACKMODE	4				// CAN USE ACK REQURED FRAMES
#define POLLEDKISS	8			// OTHER END IS POLLING US
#define D700 16					// D700 Mode (Escape "C" chars
#define TNCX 32					// TNC-X Mode (Checksum of ACKMODE frames includes ACK bytes
#define PITNC 64				// PITNC Mode - can reset TNC with FEND 15 2
#define NOPARAMS 128			// Don't send SETPARAMS frame
#define FLDIGI 256				// Support FLDIGI COmmand Frames
#define TRACKER 512				// SCS Tracker. Need to set KISS Mode 
#define FASTI2C 1024			// Use BLocked I2C Reads (like ARDOP)
#define DRATS 2048



struct WL2KInfo * DecodeWL2KReportLine(char *  buf);

// Dummy file routines - write to buffer instead

char * PortConfig[70];
char * RadioConfigMsg[70];
char * WL2KReportLine[70];

int nextRadioPort = 0;
int nextDummyInterlock = 233; 

BOOL PortDefined[70];

extern BOOL IncludesMail;
extern BOOL IncludesChat;
extern int needAIS;
extern int needADSB;

extern int AGWPort;
extern int AGWSessions;
extern int AGWMask;

extern BOOL LoopMonFlag;
extern BOOL Loopflag;

extern char NodeMapServer[80];
extern char ChatMapServer[80];

double LatFromLOC;
double LonFromLOC;



VOID * zalloc(int len);

int WritetoConsoleLocal(char * buff);
char * stristr (char *ch1, char *ch2);
int FromLOC(char * Locator, double * pLat, double * pLon);

VOID Consoleprintf(const char * format, ...)
{
	char Mess[512];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\n");
	WritetoConsoleLocal(Mess);

	return;
}


#pragma pack()

int tnctypes(int i, char value[],char rec[]);
int do_kiss (char value[],char rec[]);

struct TNCDATA * TNCCONFIGTABLE = NULL;		// malloc'ed
int NUMBEROFTNCPORTS = 0;

struct UPNP * UPNPConfig = NULL;

struct TNCDATA * TNC2ENTRY;

extern char PWTEXT[];
extern char HFCTEXT[];
extern int HFCTEXTLEN;
extern char LOCATOR[];
extern char MAPCOMMENT[];
extern char LOC[];
extern int RFOnly;

int decode_rec(char *rec);
int applcallsign(int i,char *value,char *rec);
int appl_qual(int i,char *value,char *rec);
int callsign(char * val, char *value, char *rec);
int int_value(short * val, char *value, char *rec);
int hex_value(int * val, char *value, char *rec);
int bin_switch(char * val, char *value, char *rec);
int dec_switch(char * val, char *value, char *rec);
int applstrings(int i,char *value, char *rec);
int dotext(char * val, char *key_word, int max);
int dolinked(int i, char * value, char * rec);
int routes(int i);
int ports(int i);
int tncports(int i);
int dedports(int i);
int xindex(char *s,char *t);
int verify(char *s,char c);
int GetNextLine(char * rec);
int call_check(char *callsign, char * val);
int call_check_internal(char * callsign);
int callstring(int i,char *value,char *rec);
int decode_port_rec(char *rec);
int doid(int i,char *value,char *rec);
int dodll(int i,char *value,char *rec);
int doDriver(int i,char *value,char *rec);
int hwtypes(int i,char *value,char *rec);
int protocols(int i,char *value,char *rec);
int bbsflag(int i,char *value,char *rec);
int channel(int i,char *value,char *rec);
int validcalls(int i,char *value,char *rec);
int kissoptions(int i,char *value,char *rec);
int decode_tnc_rec(char *rec);
int tnctypes(int i,char *value,char *rec);
int do_kiss(char *value,char *rec);
int decode_ded_rec(char *rec);
int simple(int i);
int64_t int64_value(int64_t * val, char value[], char rec[]);


int C_Q_ADD_NP(VOID *PQ, VOID *PBUFF);
int doSerialPortName(int i, char * value, char * rec);
int doPermittedAppls(int i, char * value, char * rec);
int doKissCommand(int i, char * value, char * rec);

BOOL ProcessAPPLDef(char * rec);
BOOL ToLOC(double Lat, double Lon , char * Locator);

//int i;
//char value[];
//char rec[];

int FIRSTAPPL=1;
BOOL Comment = FALSE;
int CommentLine = 0;

#define MAXLINE 512
#define FILEVERSION 22

extern UCHAR BPQDirectory[];

struct CONFIGTABLE xxcfg;
struct PORTCONFIG xxp;

char inputname[250]="bpqcfg.txt";
char option[250];

/************************************************************************/
/*      STATIC VARIABLES                                                */
/************************************************************************/

static char *keywords[] = 
{
"OBSINIT", "OBSMIN", "NODESINTERVAL", "L3TIMETOLIVE", "L4RETRIES", "L4TIMEOUT",
"BUFFERS", "PACLEN", "TRANSDELAY", "T3", "IDLETIME", "BBS",
"NODE", "NODEALIAS", "BBSALIAS", "NODECALL", "BBSCALL",
"TNCPORT", "IDMSG:", "INFOMSG:", "ROUTES:", "PORT",  "MAXLINKS",
"MAXNODES", "MAXROUTES", "MAXCIRCUITS", "IDINTERVAL", "MINQUAL",
"HIDENODES", "L4DELAY", "L4WINDOW", "BTINTERVAL", "UNPROTO", "BBSQUAL",
"APPLICATIONS", "EMS", "CTEXT:", "DESQVIEW", "HOSTINTERRUPT", "ENABLE_LINKED",
"XXDEDHOST", "FULL_CTEXT", "SIMPLE", "AUTOSAVE" , "L4APPL",
"APPL1CALL", "APPL2CALL", "APPL3CALL", "APPL4CALL",
"APPL5CALL", "APPL6CALL", "APPL7CALL", "APPL8CALL",
"APPL1ALIAS", "APPL2ALIAS", "APPL3ALIAS", "APPL4ALIAS",
"APPL5ALIAS", "APPL6ALIAS", "APPL7ALIAS", "APPL8ALIAS",
"APPL1QUAL", "APPL2QUAL", "APPL3QUAL", "APPL4QUAL",
"APPL5QUAL", "APPL6QUAL", "APPL7QUAL", "APPL8QUAL",
"BTEXT:", "NETROMCALL", "C_IS_CHAT", "MAXRTT", "MAXHOPS",		// IPGATEWAY= no longer allowed
"LogL4Connects", "LogAllConnects", "SAVEMH", "ENABLEADIFLOG", "ENABLEEVENTS", "SAVEAPRSMSGS", 
"EnableM0LTEMap", "MQTT", "MQTT_HOST", "MQTT_PORT", "MQTT_USER", "MQTT_PASS",
"L4Compress", "L4CompMaxframe", "L4CompPaclen", "L2Compress", "L2CompMaxframe",
"L2CompPaclen", "PREFERINP3ROUTES", "OnlyVer2point0", "DEBUGINP3", "ENABLEOARCAPI"
};           /* parameter keywords */

static void * offset[] =
{
&xxcfg.C_OBSINIT, &xxcfg.C_OBSMIN, &xxcfg.C_NODESINTERVAL, &xxcfg.C_L3TIMETOLIVE, &xxcfg.C_L4RETRIES, &xxcfg.C_L4TIMEOUT,
&xxcfg.C_BUFFERS, &xxcfg.C_PACLEN, &xxcfg.C_TRANSDELAY, &xxcfg.C_T3, &xxcfg.C_IDLETIME, &xxcfg.C_BBS,
&xxcfg.C_NODE, &xxcfg.C_NODEALIAS, &xxcfg.C_BBSALIAS, &xxcfg.C_NODECALL, &xxcfg.C_BBSCALL,
0, &xxcfg.C_IDMSG, &xxcfg.C_INFOMSG, &xxcfg.C_ROUTE, &xxcfg.C_PORT, &xxcfg.C_MAXLINKS,
&xxcfg.C_MAXDESTS, &xxcfg.C_MAXNEIGHBOURS, &xxcfg.C_MAXCIRCUITS, &xxcfg.C_IDINTERVAL, &xxcfg.C_MINQUAL,
&xxcfg.C_HIDENODES, &xxcfg.C_L4DELAY, &xxcfg.C_L4WINDOW, &xxcfg.C_BTINTERVAL, &xxcfg.C_WASUNPROTO, &xxcfg.C_BBSQUAL,
&xxcfg.C_APPL, &xxcfg.C_EMSFLAG, &xxcfg.C_CTEXT , &xxcfg.C_DESQVIEW, &xxcfg.C_HOSTINTERRUPT, &xxcfg.C_LINKEDFLAG,
0, &xxcfg.C_FULLCTEXT, 0, &xxcfg.C_AUTOSAVE, &xxcfg.C_L4APPL,
&xxcfg.C_APPL[0].ApplCall, &xxcfg.C_APPL[1].ApplCall, &xxcfg.C_APPL[2].ApplCall, &xxcfg.C_APPL[3].ApplCall,
&xxcfg.C_APPL[4].ApplCall, &xxcfg.C_APPL[5].ApplCall, &xxcfg.C_APPL[6].ApplCall, &xxcfg.C_APPL[7].ApplCall,
&xxcfg.C_APPL[0].ApplAlias, &xxcfg.C_APPL[1].ApplAlias, &xxcfg.C_APPL[2].ApplAlias, &xxcfg.C_APPL[3].ApplAlias,
&xxcfg.C_APPL[4].ApplAlias, &xxcfg.C_APPL[5].ApplAlias, &xxcfg.C_APPL[6].ApplAlias, &xxcfg.C_APPL[7].ApplAlias,
&xxcfg.C_APPL[0].ApplQual, &xxcfg.C_APPL[1].ApplQual, &xxcfg.C_APPL[2].ApplQual, &xxcfg.C_APPL[3].ApplQual,
&xxcfg.C_APPL[4].ApplQual, &xxcfg.C_APPL[5].ApplQual, &xxcfg.C_APPL[6].ApplQual, &xxcfg.C_APPL[7].ApplQual,
&xxcfg.C_BTEXT, &xxcfg.C_NETROMCALL, &xxcfg.C_C, &xxcfg.C_MAXRTT, &xxcfg.C_MAXHOPS,		// IPGATEWAY= no longer allowed
&xxcfg.C_LogL4Connects, &xxcfg.C_LogAllConnects, &xxcfg.C_SaveMH, &xxcfg.C_ADIF, &xxcfg.C_EVENTS, &xxcfg.C_SaveAPRSMsgs,
&xxcfg.C_M0LTEMap, &xxcfg.C_MQTT, &xxcfg.C_MQTT_HOST, &xxcfg.C_MQTT_PORT, &xxcfg.C_MQTT_USER, &xxcfg.C_MQTT_PASS,
&xxcfg.C_L4Compress, &xxcfg.C_L4CompMaxframe, &xxcfg.C_L4CompPaclen, &xxcfg.C_L2Compress, &xxcfg.C_L2CompMaxframe, 
&xxcfg.C_L2CompPaclen, &xxcfg.C_PREFERINP3ROUTES, &xxcfg.C_OnlyVer2point0,  &xxcfg.C_DEBUGINP3, &xxcfg.C_OARCAPI};		/* offset for corresponding data in config file */

static int routine[] = 
{
1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 2,
2, 0, 0, 0, 0,
3, 4, 20, 5, 6, 1,
1, 1, 1, 1, 1,
2, 1, 1, 1, 7, 1,
8, 2, 4, 2, 9, 10,
11, 1, 12, 2 , 1,
13, 13, 13, 13,
13, 13 ,13, 13,
13, 13, 13, 13,
13, 13 ,13, 13,
14, 14, 14, 14,
14, 14 ,14, 14,
15, 0, 2, 9, 9,
2, 2, 1, 2, 2, 2,
2, 2, 0, 1, 20, 20,
1, 1, 1, 1, 1, 
1, 1, 1, 1, 1};			// Routine to process param

int PARAMLIM = sizeof(routine)/sizeof(int);
//int NUMBEROFKEYWORDS = sizeof(routine)/sizeof(int);

//#define PARAMLIM 74


static char eof_message[] = "Unexpected end of file on input\n";

static char *pkeywords[] = 
{
"ID", "TYPE", "PROTOCOL", "IOADDR", "INTLEVEL", "SPEED", "CHANNEL",
"BBSFLAG", "QUALITY", "MAXFRAME", "TXDELAY", "SLOTTIME", "PERSIST",
"FULLDUP", "SOFTDCD", "FRACK", "RESPTIME", "RETRIES",
"PACLEN", "CWID", "PORTCALL", "PORTALIAS", "ENDPORT", "VALIDCALLS",
"QUALADJUST", "DIGIFLAG", "DIGIPORT", "USERS" ,"UNPROTO", "PORTNUM",
"TXTAIL", "ALIAS_IS_BBS", "L3ONLY", "KISSOPTIONS", "INTERLOCK", "NODESPACLEN",
"TXPORT", "MHEARD", "CWIDTYPE", "MINQUAL", "MAXDIGIS", "PORTALIAS2", "DLLNAME",
"BCALL", "DIGIMASK", "NOKEEPALIVES", "COMPORT", "DRIVER", "WL2KREPORT", "UIONLY",
"UDPPORT", "IPADDR", "I2CBUS", "I2CDEVICE", "UDPTXPORT", "UDPRXPORT", "NONORMALIZE",
"IGNOREUNLOCKEDROUTES", "INP3ONLY", "TCPPORT", "RIGPORT", "PERMITTEDAPPLS", "HIDE",
"SMARTID", "KISSCOMMAND", "SendtoM0LTEMap", "PortFreq", "M0LTEMapInfo", "QTSMPort", "ALLOWINP3", "ENABLEINP3"};         /* parameter keywords */

static void * poffset[] =
{
&xxp.ID, &xxp.TYPE, &xxp.PROTOCOL, &xxp.IOADDR, &xxp.INTLEVEL, &xxp.SPEED, &xxp.CHANNEL,
&xxp.BBSFLAG, &xxp.QUALITY, &xxp.MAXFRAME, &xxp.TXDELAY, &xxp.SLOTTIME, &xxp.PERSIST,
&xxp.FULLDUP, &xxp.SOFTDCD, &xxp.FRACK, &xxp.RESPTIME, &xxp.RETRIES,
&xxp.PACLEN, &xxp.CWID, &xxp.PORTCALL, &xxp.PORTALIAS, 0, &xxp.VALIDCALLS,
&xxp.QUALADJUST, &xxp.DIGIFLAG, &xxp.DIGIPORT, &xxp.USERS, &xxp.UNPROTO, &xxp.PORTNUM,
&xxp.TXTAIL, &xxp.ALIAS_IS_BBS, &xxp.L3ONLY, &xxp.KISSOPTIONS, &xxp.INTERLOCK, &xxp.NODESPACLEN,
&xxp.TXPORT, &xxp.MHEARD, &xxp.CWIDTYPE, &xxp.MINQUAL, &xxp.MAXDIGIS, &xxp.PORTALIAS2, &xxp.DLLNAME,
&xxp.BCALL, &xxp.DIGIMASK, &xxp.DefaultNoKeepAlives, &xxp.IOADDR, &xxp.DLLNAME, &xxp.WL2K, &xxp.UIONLY,
&xxp.IOADDR, &xxp.IPADDR, &xxp.INTLEVEL, &xxp.IOADDR, &xxp.IOADDR, &xxp.ListenPort, &xxp.NoNormalize,
&xxp.IGNOREUNLOCKED, &xxp.INP3ONLY, &xxp.TCPPORT, &xxp.RIGPORT, &xxp.PERMITTEDAPPLS, &xxp.Hide,
&xxp.SmartID, &xxp.KissParams, &xxp.SendtoM0LTEMap, &xxp.PortFreq, &xxp.M0LTEMapInfo, &xxp.QtSMPort, &xxp.AllowINP3, &xxp.EnableINP3};	/* offset for corresponding data in config file */

static int proutine[] = 
{
4, 5, 8, 3, 1, 1, 7,
6, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1,
1, 0, 0, 0, 9, 10,
1, 13, 13, 1, 11, 1,
1, 2, 2, 12, 1, 1,
1, 7, 7, 13, 13, 0, 14,
0, 1, 2, 18, 15, 16, 2,
1, 17, 1, 1, 1, 1, 2,
2, 2, 1, 1, 19, 2,
1, 20, 1, 21, 22, 1, 1, 1};							/* routine to process parameter */

int PPARAMLIM = sizeof(proutine)/sizeof(int);

static int endport = 0;
static int portnum = 1;
static int portindex = 0;
static int porterror = 0;
static int tncporterror = 0;
static int dedporterror = 0;
static int kissflags = 0;
static int NextAppl = 0;
static int routeindex = 0;


/************************************************************************/
/* Global variables							*/
/************************************************************************/

int paramok[100] = {0};		/* PARAMETER OK FLAG  */

FILE *fp1;			/* TEXT INPUT FILE    */

static char s1[80];
static char s2[80];
static char s3[80];
static char s4[80];
static char s5[80];
static char s6[80];
static char s7[80];
static char s8[80];

char commas[]=",,,,,,,,,,,,,,,,";

char bbscall[11];
char bbsalias[11];
int bbsqual;


extern UCHAR ConfigDirectory[260];

BOOL LocSpecified = FALSE;

/************************************************************************/
/*   MAIN PROGRAM							*/
/************************************************************************/
 
VOID WarnThread();

int LineNo = 0;

int heading = 0;


BOOL ProcessConfig()
{
	int i;
	char rec[MAXLINE];
	int Cfglen = sizeof(xxcfg);
	struct APPLCONFIG * App;

	memset(&xxcfg, 0, sizeof(xxcfg));
	memset(&xxp, 0, sizeof(xxp));

	heading = 0;
	portnum = 1;
	NextAppl = 0;
	LOCATOR[0] = 0;
	MAPCOMMENT[0] = 0;
	routeindex = 0;
	portindex = 0;

	for (i = 0; i < 70; i++)
	{
		if (PortConfig[i])
		{
			free(PortConfig[i]);
			PortConfig[i] = NULL;
		}
		PortDefined[i] = FALSE;

		if (RadioConfigMsg[i])
		{
			free(RadioConfigMsg[i]);
			RadioConfigMsg[i] = NULL;
		}
	}

	nextRadioPort = 0;

	TNCCONFIGTABLE = NULL;
	NUMBEROFTNCPORTS = 0;

	AGWMask = 0;

	UPNPConfig = NULL;

	Consoleprintf("Configuration file Preprocessor.");

	if (ConfigDirectory[0] == 0)
	{
		strcpy(inputname, "bpq32.cfg");
	}
		else
	{
		strcpy(inputname,ConfigDirectory);
		strcat(inputname,"/");
		strcat(inputname, "bpq32.cfg");
	}

	if ((fp1 = fopen(inputname,"r")) == NULL)
	{
		Consoleprintf("Could not open file %s Error code %d", inputname, errno);
		return FALSE;
	}

	Consoleprintf("Using Configuration file %s",inputname);

	memset(&xxcfg, 0, sizeof(xxcfg));

	App = (struct APPLCONFIG *)&xxcfg.C_APPL[0];
	
	for (i=0; i < NumberofAppls; i++)
	{
		memset(App->Command, ' ', 12);
		memset(App->CommandAlias, ' ', 48);
		memset(App->ApplCall, ' ', 10);
		memset(App->ApplAlias, ' ', 10);

		App++;
	}

//	xxcfg.SaveMH = TRUE;		// Default to save

	xxcfg.C_PREFERINP3ROUTES = 0;	// Default to false

	GetNextLine(rec);

	while (rec[0])
	{
	   decode_rec(rec);
	   GetNextLine(rec);
	}

	if (xxcfg.C_NODECALL[0] == ' ')		
	{
	   Consoleprintf("Missing NODECALL");
	   heading = 1;
	}


	paramok[6]=1;          /* dont need BUFFERS */
	paramok[8]=1;          /* dont need TRANSDELAY */
	paramok[13]=1;			// NodeAlias 
	paramok[17]=1;          /* dont need TNCPORTS */
	paramok[20]=1;         // Or ROUTES

	paramok[32]=1;          /* dont need UNPROTO */

	paramok[35]=1;          /* dont need EMS */
	paramok[37]=1;          /* dont need DESQVIEW */
	paramok[38]=1;          /* dont need HOSTINTERRUPT */

	paramok[40]=1;			/* or DEDHOST */

	paramok[42]=1;			/* or SIMPLE */

	paramok[43]=1;			/* or AUTOSAVE */

	paramok[44]=1;			/* or L4APPL */


	paramok[16]=1;	//  BBSCALL
	paramok[14]=1;	//  BBSALIAS
	paramok[33]=1;	//  BBSQUAL
	paramok[34]=1;	//  APPLICATIONS

	if (paramok[45]==1)
	{
		paramok[16]=1;	//  APPL1CALL overrides BBSCALL
		memcpy(xxcfg.C_BBSCALL, xxcfg.C_APPL[0].ApplCall, 10);	
	}
	
	if (paramok[53]==1)
	{
		paramok[14]=1;	//  APPL1ALIAS overrides BBSALIAS
		memcpy(xxcfg.C_BBSALIAS, xxcfg.C_APPL[0].ApplAlias, 10);
	}

	if (paramok[61]==1) 
	{
		paramok[33]=1;	//  APPL1QUAL overrides BBSQUAL
		xxcfg.C_BBSQUAL = xxcfg.C_APPL[0].ApplQual;
	}
			
	
	for (i=0;i<24;i++)
		paramok[45+i]=1;	/* or APPLCALLS, APPLALIASS APPLQUAL */

	paramok[69]=1;			// BText optional
	paramok[70]=1;			// IPGateway optional
	paramok[71]=1;			// C_IS_CHAT optional

	paramok[72]=1;			// MAXRTT optional
	paramok[73]=1;			// MAXHOPS optional
	paramok[74]=1;			// LogL4Connects optional
	paramok[75]=1;			// LogAllConnects optional
	paramok[76]=1;			// SAVEMH optional
	paramok[77]=1;			// ENABLEADIFLOG optional
	paramok[78]=1;			// EnableEvents optional
	paramok[79]=1;			// SaveAPRSMsgs optional
	paramok[79]=1;			// SaveAPRSMsgs optional
	paramok[80]=1;			// EnableM0LTEMap optional
	paramok[81]=1;			// MQTT Params
	paramok[82]=1;			// MQTT Params
	paramok[83]=1;			// MQTT Params
	paramok[84]=1;			// MQTT Params
	paramok[85]=1;			// MQTT Params
	paramok[86]=1;			// L4Compress
	paramok[87]=1;			// L4Compress Maxframe
	paramok[88]=1;			// L4Compress Paclen
	paramok[89]=1;			// L2Compress
	paramok[90]=1;			// L2Compress Maxframe
	paramok[91]=1;			// L2Compress Paclen
	paramok[92]=1;			// PREFERINP3ROUTES
	paramok[93]=1;			// ONLYVer2point0
	paramok[94]=1;			// DEBUGINP3
	paramok[95]=1;			// EnableOARCAPI

	for (i=0; i < PARAMLIM; i++)
	{
		if (paramok[i] == 0)
		{
			if (heading == 0)
			{
				Consoleprintf(" ");
				Consoleprintf("The following parameters were not correctly specified");
				heading = 1;
			}
			Consoleprintf(keywords[i]);
		}
	}

	if (portnum == 1)
	{
	   Consoleprintf("No valid radio ports defined");
	   heading = 1;
	}

	if (Comment)
	{
		Consoleprintf("Unterminated Comment (Missing */) at line %d", CommentLine);
		heading = 1;
	}

	fclose(fp1);

	if (LOCATOR[0] == 0 && LocSpecified == 0 && RFOnly == 0)
	{
		Consoleprintf("");
		Consoleprintf("Please enter a LOCATOR statement in your BPQ32.cfg");
		Consoleprintf("If you really don't want to be on the Node Map you can enter LOCATOR=NONE");
		Consoleprintf("");

//		_beginthread(WarnThread, 0, 0);
	}

	if (heading == 0)
	{
	   Consoleprintf("Conversion (probably) successful");
	   Consoleprintf("");
	}
	else
	{
   	   Consoleprintf("Conversion failed");
   	   return FALSE;
	}

/*
	// Dump to file for debugging
	
	sprintf_s(inputname, sizeof(inputname), "CFG%d", time(NULL));
	
	fp1 = fopen(inputname, "wb");

	if (fp1)
	{
		fwrite(ConfigBuffer, 1, 100000, fp1);
		fclose(fp1);
	}
*/
	return TRUE;
}

/************************************************************************/
/*	Decode PARAM=							*/
/************************************************************************/

int decode_rec(char * rec)
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */

	char key_word[300] = "";
	char value[300] = "";

	if (_memicmp(rec, "NODEMAPSERVER=", 14) == 0)
	{
		// Map reporting override

		strcpy(NodeMapServer, &rec[14]);
		strlop(NodeMapServer, ' ');

		return 1;
	}

	if (_memicmp(rec, "CloseOnError=", 13) == 0)
	{
		// Close BPQ on trapped program error

		CloseOnError = atoi(&rec[13]);
		return 1;
	}

	if (_memicmp(rec, "CHATMAPSERVER=", 14) == 0)
	{
		// Map reporting override

		strcpy(ChatMapServer, &rec[14]);
		strlop(ChatMapServer, ' ');

		return 1;
	}

	if (_memicmp(rec, "IPGATEWAY", 9) == 0 && rec[9] != '=')	// IPGATEWAY, not IPGATEWAY=
	{
		// Create Embedded IPGateway Config

		// Copy all subsequent lines up to **** to a memory buffer

		char * ptr;
		
		PortConfig[IPConfigSlot] = ptr = malloc(50000);

		*ptr = 0;

		GetNextLine(rec);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "****", 3) == 0)
			{
				PortConfig[IPConfigSlot] = realloc(PortConfig[IPConfigSlot], (strlen(ptr) + 1));
				xxcfg.C_IP = 1;
				return 0;
			}

			strcat(ptr, rec);
			strcat(ptr, "\r\n");
			GetNextLine(rec);
		}

		Consoleprintf("Missing **** for IPGateway Config");
		heading = 1;

		return 0;
	}

	if (_memicmp(rec, "PORTMAPPER", 10) == 0)	
	{
		// Create Embedded portmapper Config

		// Copy all subsequent lines up to **** to a memory buffer

		char * ptr;
		
		PortConfig[PortMapConfigSlot] = ptr = malloc(50000);

		*ptr = 0;

		GetNextLine(rec);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "****", 3) == 0)
			{
				PortConfig[PortMapConfigSlot] = realloc(PortConfig[PortMapConfigSlot], (strlen(ptr) + 1));
				xxcfg.C_PM = 1;
				return 0;
			}

			strcat(ptr, rec);
			strcat(ptr, "\r\n");
			GetNextLine(rec);
		}

		Consoleprintf("Missing **** for Portmapper Config");
		heading = 1;

		return 0;
	}

	if (_memicmp(rec, "APRSDIGI", 8) == 0)
	{		
		// Create Embedded APRS Config

		// Copy all subsequent lines up to **** to a memory buffer

		char * ptr;
		
		PortConfig[APRSConfigSlot] = ptr = malloc(50000);

		*ptr = 0;

		// Don't use GetNextLine - we need to keep ; in messages
		
		fgets(rec,MAXLINE,fp1);
		LineNo++;

		while (!feof(fp1))
		{
			if (_memicmp(rec, "****", 3) == 0)
			{
				PortConfig[APRSConfigSlot] = realloc(PortConfig[APRSConfigSlot], (strlen(ptr) + 1));
				return 0;
			}
			if (strlen(rec) > 1)
			{
				if (memcmp(rec, "/*", 2) == 0)
				{
					Comment = TRUE;
					CommentLine = LineNo;
					goto NextAPRS;
				}
				else if (memcmp(rec, "*/", 2) == 0)
				{
					Comment = FALSE;
					goto NextAPRS;
				}
			}
			
			if (Comment)
				goto NextAPRS;

			strcat(ptr, rec);
			strcat(ptr, "\r\n");
NextAPRS:
			fgets(rec,MAXLINE,fp1);
			LineNo++;
		}

		if (_memicmp(rec, "****", 3) == 0)
			return 0;						// No Newline after ***

		Consoleprintf("Missing **** for APRS Config");
		heading = 1;

		return 0;
	}

	if (_memicmp(rec, "PASSWORD", 8) == 0)
	{
		// SYSOP Password

		if (strlen(rec) > 88) rec[88] = 0;

		_strupr(rec);
		
		strcpy(PWTEXT, &rec[9]);
		return 0;
	}

#ifdef LINBPQ

	if (_memicmp(rec, "LINMAIL", 7) == 0)
	{
		// Enable Mail on Linux Verdion

		IncludesMail = TRUE;

		return 0;
	}

	if (_memicmp(rec, "LINCHAT", 7) == 0)
	{
		// Enable Chat on Linux Verdion

		IncludesChat = TRUE;

		return 0;
	}
#endif

	if (_memicmp(rec, "ENABLEAIS", 9) == 0)
	{
		needAIS = TRUE;
		return 0;
	}

	if (_memicmp(rec, "ENABLEADSB", 9) == 0)
	{
		needADSB = TRUE;
		return 0;
	}
	
	if (_memicmp(rec, "HFCTEXT", 7) == 0)
	{
		// HF only CTEXT (normlly short to reduce traffic)

		if (strlen(rec) > 87) rec[87] = 0;
		strcpy(HFCTEXT, &rec[8]);
		HFCTEXTLEN = (int)(strlen(HFCTEXT));
		HFCTEXT[HFCTEXTLEN - 1] = '\r';
		return 0;
	}

	if (_memicmp(rec, "LOCATOR", 7) == 0)
	{
		// Station Maidenhead Locator or Lat/Long

		char * Context;		
		char * ptr1 = strtok_s(&rec[7], " ,=\t\n\r:", &Context);
		char * ptr2 = strtok_s(NULL, " ,=\t\n\r:", &Context);

		LocSpecified = TRUE;

		if (_memicmp(&rec[8], "NONE", 4) == 0)
			return 0;

		if (_memicmp(&rec[8], "XXnnXX", 6) == 0)
			return 0;

		if (ptr1)
		{
			strcpy(LOCATOR, ptr1);
			if (ptr2)
			{
				strcat(LOCATOR, ":");
				strcat(LOCATOR, ptr2);
				ToLOC(atof(ptr1), atof(ptr2), LOC);
				LatFromLOC = atof(ptr1);
				LonFromLOC = atof(ptr2);

			}
			else
			{
				if (strlen(ptr1) == 6)
				{
					strcpy(LOC, ptr1);
					FromLOC(LOC, &LatFromLOC, &LonFromLOC);
					// Randomise in square
					LatFromLOC += ((rand() / 24.0) / RAND_MAX);
					LonFromLOC += ((rand() / 12.0) / RAND_MAX);

				}
			}
		}
		return 0;
	}

	if (_memicmp(rec, "MAPCOMMENT", 10) == 0)
	{
		// Additional Info for Node Map

		char * ptr1 = &rec[11];
		char * ptr2 = MAPCOMMENT;

		while (*ptr1)
		{
			if (*ptr1 == ',')
			{
				*ptr2++ = '&';
				*ptr2++ = '#';
				*ptr2++ = '4';
				*ptr2++ = '4';
				*ptr2++ = ';';
			}
			else
				*(ptr2++) = *ptr1;

			ptr1++;

			if ((ptr2 - MAPCOMMENT) > 248)
				break;
		}

		*ptr2 = 0;

		return 0;
	}

	//	Process BRIDGE statement

	if (_memicmp(rec, "BRIDGE", 6) == 0)
	{
		int DigiTo;
		int Port;
		char * Context;
		char * p_value;
		char * ptr;

		p_value = strtok_s(&rec[7], ",=\t\n\r", &Context);

		Port = atoi(p_value);

		if (Port > MaxBPQPortNo)
			return FALSE;

		if (Context == NULL)
			return FALSE;
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			DigiTo = atoi(ptr);
	
			if (DigiTo > MaxBPQPortNo)
				return 0;

			if (Port != DigiTo)				// Not to our port!
				xxcfg.CfgBridgeMap[Port][DigiTo] = TRUE;	

			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		return 0;
	}


	// AGW Emulator Params

	if (_memicmp(rec, "AGWPORT", 7) == 0)
	{
		AGWPort = atoi(&rec[8]);
		return 0;
	}

	if (_memicmp(rec, "AGWSESSIONS", 11) == 0)
	{
		AGWSessions = atoi(&rec[12]);
		return 0;
	}

	if (_memicmp(rec, "AGWMASK", 7) == 0)
	{
		AGWMask = strtol(&rec[8], 0, 0);
		return 0;
	}

	if (_memicmp(rec, "AGWAPPL", 7) == 0)
	{
		AGWMask |= 1 << (strtol(&rec[8], 0, 0) - 1);
		return 0;
	}

	if (_memicmp(rec, "AGWLOOPMON", 10) == 0)
	{
		LoopMonFlag = strtol(&rec[11], 0, 0);
		return 0;
	}
	if (_memicmp(rec, "AGWLOOPTX", 9) == 0)
	{
		Loopflag = strtol(&rec[10], 0, 0);
		return 0;
	}

	if (_memicmp(rec, "APPLICATION ", 12) == 0 || _memicmp(rec, "APPLICATION=", 12) == 0)
	{
		// New Style APPLICATION Definition

		char save[300];

		strcpy(save, rec);			// Save in case error
		
		if (!ProcessAPPLDef(&rec[12]))
		{
			Consoleprintf("Invalid Record %s", save);
			heading = 1;
		}
		else
			paramok[34]=1;		// Got APPLICATIONS

		return 0;
	}

	if (_memicmp(rec, "EXCLUDE=", 8) == 0)
	{
		char * ptr2 = &rec[8];
		UCHAR * ptr3 = xxcfg.C_EXCLUDE;

		_strupr(ptr2);
		while (*(ptr2) > 32)
		{
			ConvToAX25(ptr2, ptr3);
			ptr3 += 7;

			if (strchr(ptr2, ','))
			{
				ptr2 = strchr(ptr2, ',');
				ptr2++;
			}
			else
				break;

			if (ptr3 > &xxcfg.C_EXCLUDE[63])
			{
				Consoleprintf("Too Many Excluded Calls");
				heading = 1;
				break;
			}
		}

		return 0;
	}
	if (_memicmp(rec, "RADIO", 5) == 0)
	{
		if (strlen(rec) > 11)
		{
			RadioConfigMsg[nextRadioPort++] = _strdup(rec);	
			return 0;
		}
		else
		{
			// Multiline config, ending in ****

			char * rptr;

			RadioConfigMsg[nextRadioPort] = rptr = zalloc(50000);

			strcpy(rptr, rec);

			GetNextLine(rec);

			while(!feof(fp1))
			{
				if (memcmp(rec, "***", 3) == 0)
				{
					RadioConfigMsg[nextRadioPort] = realloc(RadioConfigMsg[nextRadioPort], (strlen(rptr) + 1));		
					nextRadioPort++;
					return 0;
				}
				strcat(rptr, rec);
				GetNextLine(rec);
			}
		}
	}

	if (_memicmp(rec, "UPNP ", 5) == 0)
	{
		struct UPNP * Entry = (struct UPNP *)zalloc(sizeof(struct UPNP));
		char * ptr, * context;
		char copy[256];

		strcpy(copy, rec);

		ptr = strtok_s(&rec[5], ", ", &context);
	 
		if (ptr)
			Entry->Protocol = _strdup(ptr);

		ptr = strtok_s(NULL, ", ", &context);

		if (ptr)
			Entry->LANport = Entry->WANPort = _strdup(ptr);;

		ptr = strtok_s(NULL, ", ", &context);

		if (ptr)
			Entry->WANPort = _strdup(ptr);;

		if (Entry->LANport)
		{
			Entry->Next = UPNPConfig;
			UPNPConfig = Entry;
			return 1;
		}

		Consoleprintf("Bad UPNP Line %s", copy);
		heading = 1;

		return 0;
	}


	if (_memicmp("MQTT_HOST=", rec, 10) == 0)
	{
		strcpy(xxcfg.C_MQTT_HOST, &rec[10]);
		xxcfg.C_MQTT_HOST[strlen(xxcfg.C_MQTT_HOST)-1] = '\0';
		return 0;
	}
	if (_memicmp("MQTT_USER=", rec, 10) == 0)
	{
		strcpy(xxcfg.C_MQTT_USER, &rec[10]);
		xxcfg.C_MQTT_USER[strlen(xxcfg.C_MQTT_USER)-1] = '\0';
		return 0;
	}
	if (_memicmp("MQTT_PASS=", rec, 10) == 0)
	{
		strcpy(xxcfg.C_MQTT_PASS, &rec[10]);
		xxcfg.C_MQTT_PASS[strlen(xxcfg.C_MQTT_PASS)-1] = '\0';
		return 0;
}


	if (xindex(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0;  i < PARAMLIM && _stricmp(keywords[i],key_word) != 0 ; i++)
	   ;

	if (i == PARAMLIM)
	   Consoleprintf("bpq32.cfg line no %d not recognised - Ignored: %s" ,LineNo, rec);
	else
	{

		switch (routine[i])
		{
		case 0:
			cn = callsign((char *)offset[i], value, rec);        /* CALLSIGNS */
			break;

        case 1:
			cn = int_value((short *)offset[i], value, rec);	     /* INTEGER VALUES */
			break;

		case 2:
			cn = bin_switch((char *)offset[i], value, rec);        /* 0/1 SWITCHES */
			break;

		case 3:
			cn = tncports(i);	             /* VIRTUAL COMBIOS PORTS */
			break;

		case 4:
			cn = dotext((char *)offset[i], key_word, 510);             /* TEXT PARMS */
			break;

		case 20:
			cn = dotext((char *)offset[i], key_word, 2000);         /* INFO TEXT PARM */
			break;

		case 5:
			cn = routes(i);                      /* ROUTES TO LOCK IN */
			break;

		case 6:
			cn = ports(i);                       /* PORTS DEFINITION */
			break;

		case 7:
			 Consoleprintf("Obsolete Record %s ignored",rec);
			 Consoleprintf("UNPROTO address should now be specified in PORT definition");

			 break;
		
		case 8:
			cn = applstrings(i,value,rec);        /* APPLICATIONS LIST */
			break;

		case 9:
			cn = dec_switch((char *)offset[i],value,rec);        /* 0/9 SWITCHES */
			break;

		case 10:
			cn = dolinked(i,value,rec);	     /* SINGLE CHAR  */	
			break;

		case 11:
			Consoleprintf("Obsolete Record %s ignored", rec);
			break;

		case 12:
			cn = simple(i);			   /* Set up basic L2 system*/
			break;

		case 13:
			cn = applcallsign(i,value,rec);          /* CALLSIGNS */
			break;

		case 14:
			cn = appl_qual(i,value,rec);	     /* INTEGER VALUES */
			break;
		
		case 15:   
			cn = dotext((char *)offset[i], key_word, 120);             /* BTEXT */
			break;
	     }

		paramok[i] = cn;
	}

	return 0;
}

/************************************************************************/
/*   CALLSIGNS								*/
/************************************************************************/
int applcallsign(int i, char * value, char * rec)
{
	char * val = (char *)offset[i];

	if (call_check_internal(value))
	{
		// Invalid

		return 0;
	}

	memcpy(val, value, 10);

	if (i==45)
		strcpy(bbscall,value);
	if (i==53)
		strcpy(bbsalias,value);

	return 1;
}

int appl_qual(int i, char * value, char * rec)
{
	int j, k;
	int * val = (int *)offset[i];

	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   Consoleprintf("Invalid numerical value ");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
	
	if (i==61) bbsqual=j;

	*val = j;
	return(1);
}


int callsign(char * ptr, char * value, char * rec)
{
	if (call_check(value, ptr) == 1)
	{
	   Consoleprintf("%s",rec);
	   return(0);
	}

	return(1);
}


/************************************************************************/
/*   VALIDATE INT VALUES						*/
/************************************************************************/

int int_value(short * val, char value[], char rec[])
{
	int j,k;
	
	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   Consoleprintf("Invalid numerical value ");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}

	val[0] = j;
	return(1);
}

int64_t int64_value(int64_t * val, char value[], char rec[])
{
	*val = strtoll(value, NULL, 10);
	return(1);
}

/************************************************************************/
/*   VALIDATE HEX INT VALUES						*/
/************************************************************************/

int hex_value(int * val, char value[], char rec[])
{
	int j = -1, k = 0;

	k = sscanf(value, " %xH", &j);

	if (j < 0)
	{
		Consoleprintf("Bad Hex Value");
		Consoleprintf("%s\r\n", rec);
		return(0);
	}

	val[0] = j;
	return(1);
}
;

/************************************************************************/
/*   VALIDATE BINARY SWITCH DATA AND WRITE TO FILE			*/
/************************************************************************/

int bin_switch(char * val, char * value, char * rec)
{
	int value_int;

	value_int = atoi(value);

	if (value_int == 0 || value_int == 1)
	{
		val[0] = value_int;
		return 1;
	}
	else
	{
	   Consoleprintf("Invalid switch value, must be either 0 or 1");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
}
/*
;	single byte decimal
*/
int dec_switch (char * val, char * value, char * rec)
{
	int value_int;

	value_int = atoi(value);

	if (value_int < 256)
	{
		val[0] = value_int;
		return 1;
	}
	else
	{
	   Consoleprintf("Invalid value, must be between 0 and 255");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
}


int applstrings(int i, char * value, char * rec)
{
	char appl[250];	// In case trailing spaces
	char * ptr1;
	char * ptr2;
	struct APPLCONFIG * App;
	int j;

 //  strcat(rec,commas);		// Ensure 16 commas

   ptr1 = &rec[13];		// skip APPLICATIONS=
	  
   App = &xxcfg.C_APPL[0];

   while (NextAppl++ < NumberofAppls)
   {
       memset(appl, ' ', 249);
	   appl[249] = 0;

	   ptr2=appl;
 		
       j = *ptr1++;

       while (j != ',' && j)
       {
		   *(ptr2++) = toupper(j);
		   j = *ptr1++;
	   }

	   ptr2 = strchr(appl, '/');

	   if (ptr2)
	   {
		   // Command has an Alias

		   *ptr2++ = 0;
		   memcpy(App->CommandAlias, ptr2, 48);
		   strcat(appl, "            ");
	   }
	   
	   memcpy(App->Command, appl, 12);
	   xxcfg.C_BBS = 1;
 
	   if (*(ptr1 - 1) == 0)
		   return 1;

	   App++;
   }
	return(1);
}


/************************************************************************/
/*    USE FOR FREE FORM TEXT IN MESSAGES				*/
/************************************************************************/

int dotext(char * val, char * key_word, int max)
{
	int len = 0;
	char * ptr;

	char rec[MAXLINE];

	GetNextLine(rec);

	if (xindex(rec,"***") == 0)
		*val = '\r';

	while (xindex(rec,"***") != 0 && !feof(fp1))
	{
		ptr = strchr(rec, 10);
		if (ptr) *ptr = 0;
		ptr = strchr(rec, 13);
		if (ptr) *ptr = 0;

		strcat(rec, "\r");

		len += (int)strlen(rec);

		if (len <= max)
		{
			strcpy(val, rec);
			val += (int)strlen(rec);
		}

		fgets(rec,MAXLINE,fp1);
		LineNo++;
	}

	if (len > max)
	{
		Consoleprintf("Text too long: %s (max %d\r\n",key_word, max);
		return(0);
	}

	if (feof(fp1))
	   return(0);
	else
	   return(1);
}


/************************************************************************/
/*     CONVERT PRE-SET ROUTES PARAMETERS TO BINARY			*/
/************************************************************************/

int routes(int i)
{
	struct ROUTECONFIG * Route;
	
	int err_flag = 0;
	int main_err = 0;

	char rec[MAXLINE];


	GetNextLine(rec);

	while (xindex(rec,"***") != 0 && !feof(fp1))
	{
		char Param[8][256];
		char * ptr1, * ptr2;
		int n = 0, inp3 = 0;

		Route = &xxcfg.C_ROUTE[routeindex++];

		// strtok and sscanf can't handle successive commas, so split up usig strchr

		memset(Param, 0, 2048);
		strlop(rec, 13);
		strlop(rec, ';');

		ptr1 = rec;

		while (ptr1 && *ptr1 && n < 8)
		{
			ptr2 = strchr(ptr1, ',');
			if (ptr2) *ptr2++ = 0;

			strcpy(&Param[n++][0], ptr1);
			ptr1 = ptr2;
			while(ptr1 && *ptr1 && *ptr1 == ' ')
				ptr1++;
		}

		strcpy(Route->call, &Param[0][0]);

		Route->quality = atoi(Param[1]);
		Route->port = atoi(Param[2]);
		Route->pwind = atoi(Param[3]);
		Route->pfrack = atoi(Param[4]);
		Route->ppacl = atoi(Param[5]);
		inp3 = atoi(Param[6]);
		Route->farQual = atoi(Param[7]);

	   if (Route->farQual < 0 || Route->farQual > 255)
	   {
	      Consoleprintf("Remote Quality must be between 0 and 255");
		  Consoleprintf("%s\r\n",rec);

	      err_flag = 1;
	   }

	   if (Route->quality < 0 || Route->quality > 255)
	   {
	      Consoleprintf("Quality must be between 0 and 255");
		  Consoleprintf("%s\r\n",rec);

	      err_flag = 1;
	   }

	   if (Route->port < 1 || Route->port > MaxBPQPortNo)
	   {
			Consoleprintf("Port number must be between 1 and 64");
			Consoleprintf("%s\r\n",rec);
			err_flag = 1;
	   }

	   // Use top bit of window as INP3 Flag, next as NoKeepAlive

	   if (inp3 & 1)
		   Route->pwind |= 0x80;

	   if (inp3 & 2)
		   Route->pwind |= 0x40;

	   if (err_flag == 1)
	   {
	      Consoleprintf("%s\r\n",rec);
	      main_err = 1;
	      err_flag = 0;
	   }
		GetNextLine(rec);
	}

	if (routeindex > MaxLockedRoutes)
	{
		routeindex--;
		Consoleprintf("Route information too long ");
		main_err = 1;
	}

	if (feof(fp1))
	{
	   Consoleprintf(eof_message);
	   return(0);
	}	

	if (main_err == 1)
	   return(0);
	else
	   return(1);
}


/************************************************************************/
/*     CONVERT PORT DEFINITIONS TO BINARY				*/
/************************************************************************/
int hw;		// Hardware type
int LogicalPortNum;				// As set by PORTNUM

int ports(int i)
{
	char rec[MAXLINE];
	endport=0;
	porterror=0;
	kissflags=0;

	xxp.PORTNUM = portnum;

	LogicalPortNum = portnum;

	if (LogicalPortNum > MaxBPQPortNo)
	{
		Consoleprintf("Port Number must be between 1 and %d", MaxBPQPortNo);
		heading = 1;
	}

	xxp.SendtoM0LTEMap = 1;			// Default to enabled

	while (endport == 0 && !feof(fp1))
	{
	   GetNextLine(rec);
	   decode_port_rec(rec);
	}
	if (porterror != 0)
	{
	   Consoleprintf("Error in port definition");
	   return(0);
	}

	if (PortDefined[LogicalPortNum]) // Already defined?
	{
		Consoleprintf("Port %d already defined", LogicalPortNum);
		heading = 1;
	}

	PortDefined[LogicalPortNum] = TRUE;

    xxp.KISSOPTIONS = kissflags;

	// copy Port Config to main config

	memcpy(&xxcfg.C_PORT[portindex++], &xxp, sizeof(xxp));
	memset(&xxp, 0, sizeof(xxp));

	portnum++;

	return(1); 

}


int tncports(int i)
{
	char rec[MAXLINE];
	endport=0;
	tncporterror=0;

	TNC2ENTRY = zalloc(sizeof(struct TNCDATA));

	TNC2ENTRY->APPLFLAGS = 6;
	TNC2ENTRY->PollDelay = 1;

	while (endport == 0 && !feof(fp1))
	{
	   GetNextLine(rec);
	   decode_tnc_rec(rec);
	}
	if (tncporterror != 0)
	{
	   Consoleprintf("Error in TNC PORT definition");
	   free (TNC2ENTRY);
		 return(0);
	}

	C_Q_ADD_NP(&TNCCONFIGTABLE, TNC2ENTRY);		// Add to chain
	
	NUMBEROFTNCPORTS++;

	return(1); 


}


/************************************************************************/
/*   MISC FUNCTIONS							*/
/************************************************************************/

/************************************************************************/
/*   FIND OCCURENCE OF ONE STRING WITHIN ANOTHER			*/
/************************************************************************/

int xindex(char s[], char t[])
{
	int i, j ,k;

	for (i=0; s[i] != '\0'; i++)
	{
	   for (j=i, k=0; t[k] != '\0' && s[i] == t[k]; j++, k++)
	      ;
	   if (t[k] == '\0')
		return(i);
	}
	return(-1);
}


/************************************************************************/
/*   FIND FIRST OCCURENCE OF A CHARACTER THAT IS NOT c			*/
/************************************************************************/

int verify(char s[], char c)
{
	int i;

	for (i = 0; s[i] != '\0'; i++)
	   if (s[i] != c)
	      return(i);

	return(-1);
}

/************************************************************************/
/*   GET NEXT LINE THAT ISN'T BLANK OR IS A COMMENT LINE		*/
/************************************************************************/

// Returns an empty string to indicate end of config

// Modified Aril 2020 to allow #include of file fragments

FILE * savefp = NULL;
int saveLineNo;
char includefilename[250];

int GetNextLine(char *rec)
{
	int i, j;
	char * ret;
	char * ptr, *context;

	while (TRUE)
	{
		ret = fgets(rec,MAXLINE,fp1);
		LineNo++;

		if (ret == NULL)
		{
			if (savefp)
			{
				// we have reached eof on an include file - switch back

				fclose(fp1);
				fp1 = savefp;
				savefp = NULL;
				LineNo = saveLineNo;
				continue;
			}

			rec[0] = 0;
			return 0;			// return end of config
		}

		for (i=0; rec[i] != '\0'; i++)
			if (rec[i] == '\t' || rec[i] == '\n' || rec[i] == '\r')
				rec[i] = ' ';



		j = verify(rec,' ');

		if (j > 0)
		{
			// Remove Leading Spaces
				
			for (i=0; rec[j] != '\0'; i++, j++)
				rec[i] = rec[j];

			rec[i] = '\0';
		}

		if (stristr(rec,"WebTermCSS") == 0 && stristr(rec,"HybridCoLocatedRMS") == 0 && stristr(rec,"HybridFrequencies") == 0)	// Needs ; in string
			strlop(rec, ';');
		else
			j = j;

		if (strlen(rec) > 1)
			if (memcmp(rec, "/*",2) == 0)
			{
				Comment = TRUE;
				CommentLine = LineNo;
			}
			else
				if (memcmp(rec, "*/",2) == 0)
				{
					rec[0] = 32;
					rec[1] = 0;
					Comment = FALSE;
				}

		if (Comment)
		{
			rec[0] = 32;
			rec[1] = 0;
			continue;
		}

		// remove trailing spaces

		while(strlen(rec) && rec[strlen(rec) - 1] == ' ')
			rec[strlen(rec) - 1] = 0;

		strcat(rec, " ");

		ptr = strtok_s(rec, " ", &context);

		// Put one back

		if (ptr)
		{
			if (context)
			{
				ptr[strlen(ptr)] = ' ';
			}
			rec = ptr;

			// look for #include

			if (_memicmp(rec, "#include ", 9) == 0)
			{
				savefp = fp1;
				
				if (BPQDirectory[0] == 0)
				{
					strcpy(includefilename, &rec[9]);
				}
				else
				{
					strcpy(includefilename,BPQDirectory);
					strcat(includefilename,"/");
					strcat(includefilename, &rec[9]);
				}

				if ((fp1 = fopen(includefilename,"r")) == NULL)
				{
					Consoleprintf("Could not open #include file %s Error code %d", includefilename, errno);
					fp1 = savefp;
					savefp = NULL;
				}
				else
				{
					saveLineNo = LineNo;
					LineNo = 0;
				}
				continue;			// get next line
			}
			return 0;
		}
	} 

	// Should never reach this

	return 0;
}


/************************************************************************/
/*   TEST VALIDITY OF CALLSIGN						*/
/************************************************************************/

int call_check_internal(char * callsign)
{
	char call[20];
	int ssid;
	int err_flag = 0;
	int i;

	if (xindex(callsign,"-") > 0)	/* There is an SSID field */
	{
	   sscanf(callsign,"%[^-]-%d",call,&ssid);
	   if (strlen(call) > 6)
	   {
		  Consoleprintf("Callsign too long, 6 characters before SSID");
	  	  Consoleprintf("%s\r\n",callsign);
	      err_flag = 1;
	   }
	   if (ssid < 0 || ssid > 15)
	   {
	      Consoleprintf("SSID out of range, must be between 0 and 15");
	      Consoleprintf("%s\r\n",callsign);
		  err_flag = 1;
	   }
	}
	else				/* No SSID field */
	{
	   if (strlen(callsign) > 6)
	   {
		  Consoleprintf("Callsign too long, 6 characters maximum");
		  Consoleprintf("%s\r\n",callsign);
	      err_flag = 1;
	   }
	}  

	strcat(callsign,"          ");
	callsign[10] = '\0';
	for (i=0; i< 10; i++)
		callsign[i]=toupper(callsign[i]);

	return(err_flag);
}

int call_check(char * callsign, char * loc)
{
	int err = call_check_internal(callsign);
	memcpy(loc, callsign, 10);
	return err;
}


/* Process  UNPROTO string allowing VIA */

char workstring[80];

int callstring(int i, char * value, char * rec)
{
	char * val = (char *)poffset[i];
	size_t j = (int)strlen(value);

	memcpy(val, value, j);
	return 1;
}

/*
		RADIO PORT PROCESSING 
*/


int decode_port_rec(char * rec)
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */
	uint32_t IPADDR;
#ifdef WIN32
	WSADATA	WsaData;	// receives data from WSAStartupproblem
#endif
	char key_word[30]="";
	char value[300]="";

	if (_memicmp(rec, "CONFIG", 6) == 0)
	{
		// Create Embedded PORT Config

		// Copy all subseuent lines up to ENDPORT to a memory buffer

		char * ptr;
		int i;

		if (LogicalPortNum > 64)
		{
			Consoleprintf("Portnum %d is invalid", LogicalPortNum);
			LogicalPortNum = 0;
		}

		PortConfig[LogicalPortNum] = ptr = malloc(50000);
		*ptr = 0;
	
		GetNextLine(rec);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "ENDPORT", 7) == 0)
			{
				PortConfig[LogicalPortNum] = realloc(PortConfig[LogicalPortNum], (strlen(ptr) + 1));		
				endport = 1;
				return 0;
			}

			i = (int)strlen(rec);
			i--;

			while(i > 1)
			{
				if (rec[i] == ' ')
					rec[i] = 0;				// Remove trailing spaces
				else
					break;

				i--;
			}

			// Pick out RIGCONFIG Records

			if (_memicmp(rec, "RIGCONTROL", 10) == 0)
			{
				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				// Convert to new format (RADIO Interlockno);

				int Interlock = xxp.INTERLOCK;
				char radio[16];

				if (Interlock == 0)			// Replace with dummy
				{
					Interlock = xxp.INTERLOCK = nextDummyInterlock;
					nextDummyInterlock++;
				}

				sprintf(radio, "RADIO %d    ", Interlock);
				memcpy(rec, radio, 10);

				if (strlen(rec) > 15)
				{
					RadioConfigMsg[nextRadioPort++] = _strdup(rec);
				}
				else
				{
					// Multiline config, ending in ****

					char * rptr;
					
					RadioConfigMsg[nextRadioPort] = rptr = zalloc(50000);

					strcpy(rptr, radio);

					GetNextLine(rec);

					while(!feof(fp1))
					{
						if (memcmp(rec, "***", 3) == 0)
						{
							RadioConfigMsg[nextRadioPort] = realloc(RadioConfigMsg[nextRadioPort], (strlen(rptr) + 1));		
							nextRadioPort++;
							break;
						}
						strcat(rptr, rec);
						GetNextLine(rec);
					}
				}
			}
			else
			{
				strcat(ptr, rec);
				strcat(ptr, "\r\n");
			}
			GetNextLine(rec);
		}

		Consoleprintf("Missing ENDPORT for Port %d", portnum);
		heading = 1;

		return 0;
	}

	if (xindex(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

	if (_stricmp(key_word, "portnum") == 0)
	{
		// Save as LogicalPortNum

		LogicalPortNum = atoi(value);
	}

	if (_stricmp(key_word, "XDIGI") == 0)
	{
		// Cross Port Digi definition

		// XDIGI=CALL,PORT,UI

		struct XDIGI * Digi = zalloc(sizeof(struct XDIGI));	//  Chain
		char * call, * pport, * Context;
		
		call = strtok_s(value, ",", &Context);
		pport = strtok_s(NULL, ",", &Context);

		if (call && pport && ConvToAX25(call, Digi->Call))
		{
			Digi->Port = atoi(pport);
			if (Digi->Port)
			{
				if (Context)
				{
					_strupr(Context);
					if (strstr(Context, "UI"))
						Digi->UIOnly = TRUE;
				}

				// Add to chain

				if (xxp.XDIGIS)
					Digi->Next = xxp.XDIGIS;
		
				xxp.XDIGIS = Digi;
				return 0;
			}
		}
		Consoleprintf("Invalid XDIGI Statement %s", rec);
		porterror = 1;
		return 0;
	}


/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0; i < PPARAMLIM && _stricmp(pkeywords[i],key_word) != 0  ; i++)
	   ;

	if (i == PPARAMLIM)
	   Consoleprintf("Source record not recognised - Ignored:%s\r\n",rec);
	else
	{

	   switch (proutine[i])
	   {
	   
	   case 0:
		   cn = callsign((char *)poffset[i], value, rec);        /* CALLSIGNS */
		   break;

	   case 1:
		   cn = int_value((short *)poffset[i], value, rec);	     /* INTEGER VALUES */
			break;

	   case 2:
		   cn = bin_switch((char *)poffset[i], value, rec);        /* 0/1 SWITCHES */
		   break;

	   case 3:
			cn = hex_value((int *)poffset[i], value, rec);         /* HEX NUMBERS */
			break;

	   case 4:
		   cn = doid(i,value,rec);               /* ID PARMS */
		   break;

	   case 5:
		   cn = hwtypes(i,value,rec);              /* HARDWARE TYPES */
		   break;

	   case 6:
		   cn = bbsflag(i,value,rec);
		   break;

	   case 7:
		   cn = channel(i,value,rec);
		   break;

	    case 8:
		cn = protocols(i,value,rec);
		break;

	    case 10:
			cn = validcalls(i,value,rec);
			break;

	    case 11:
			cn = callstring(i,value,rec);
			break;

	    case 12:
			cn = kissoptions(i,value,rec);
			break;

        case 13:
	        cn = dec_switch((char *)poffset[i], value, rec);        /* 0/9 SWITCHES */
			break;

        case 14:
            cn = dodll(i,value,rec);               /* DLL PARMS */
			break;

        case 15:
            cn = doDriver(i,value,rec);               /* DLL PARMS */
			break;

        case 16:

			xxp.WL2K = DecodeWL2KReportLine(rec);
			break;
		
		case 17:

			// IP Address for KISS over UDP
			
#ifdef WIN32
			WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif
			IPADDR = inet_addr(&rec[7]);
			memcpy(&xxp.IPADDR, &IPADDR, 4);
#ifdef WIN32
			WSACleanup();
#endif
			break;

		case 18:
            cn = doSerialPortName(i,value,rec);              // COMPORT
			break;

		case 19:
            cn = doPermittedAppls(i,value,rec);              // Permitted Apps
			break;

		case 20:
            cn = doKissCommand(i, value, rec);              // Permitted Apps
			break;

		case 21:
			cn = int64_value(poffset[i], value, rec);	     /* INTEGER VALUES */
			break;

		case 22:
			xxp.M0LTEMapInfo = _strdup(value);
			cn = 1;
			break;

		case 9:
			
			cn = 1;
			endport=1;

		break;
	   }
	}
	if (cn == 0) porterror=1;

	return 0;
}


int doid(int i, char value[], char rec[])
{
	unsigned int j;
	for (j = 3;( j < (unsigned int)strlen(rec)+1); j++)
	    
	workstring[j-3] = rec[j];

	// Remove trailing spaces before checking length

	i = (int)strlen(workstring);
	i--;

	while(i > 1)
	{
		if (workstring[i] == ' ')
			workstring[i] = 0;				// Remove trailing spaces
		else
			break;

		i--;
	}

	if (i > 29)
	{
	   Consoleprintf("Port description too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	}
	strcat(workstring,"                             ");
	workstring[30] = '\0';

	memcpy(xxp.ID, workstring, 30);
	return(1);
}

int dodll(int i, char value[], char rec[])
{
	unsigned int j;

	strlop(rec, ' ');
	for (j = 8;( j < (unsigned int)strlen(rec)+1); j++)
	    workstring[j-8] = rec[j];

	if (j > 24)
	{
	   Consoleprintf("DLL name too long - Truncated");
	   Consoleprintf("%s\r\n",rec);

	}
		
	_strupr(workstring);
	strcat(workstring,"                ");

	memcpy(xxp.DLLNAME, workstring, 16);
	xxp.TYPE = 16;		// External

	if (strstr(xxp.DLLNAME, "TELNET") || strstr(xxp.DLLNAME, "AXIP"))
		RFOnly = FALSE;

	return(1);
}

int doDriver(int i, char * value, char * rec)
{
	unsigned int j;
	for (j = 7;( j < (unsigned int)strlen(rec)+1); j++)
	    workstring[j-7] = rec[j];

	if (j > 23)
	{
	   Consoleprintf("Driver name too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	}

	_strupr(workstring);
	strcat(workstring,"                ");

	memcpy(xxp.DLLNAME, workstring, 16);
	xxp.TYPE = 16;				// External

	// Set some defaults in case HFKISS

	xxp.CHANNEL = 'A';
	xxp.FRACK = 7000;
	xxp.RESPTIME = 1000;
	xxp.MAXFRAME = 4;
	xxp.RETRIES = 6;
	
	if (strstr(xxp.DLLNAME, "TELNET") || strstr(xxp.DLLNAME, "AXIP"))
		RFOnly = FALSE;

	return 1;
}
int IsNumeric(char *str)
{
  while(*str)
  {
    if(!isdigit(*str))
      return 0;
    str++;
  }

  return 1;
}


int doSerialPortName(int i, char * value, char * rec)
{
	rec += 8;

	if (strlen(rec) > 250)
	{
	   Consoleprintf("Serial Port Name too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	   rec[250] = 0;
	}

	strlop(rec, ' ');

	if (IsNumeric(rec))
		xxp.IOADDR = atoi(rec);
	else
		xxp.SerialPortName = _strdup(rec);

	return 1;
}

int doPermittedAppls(int i, char * value, char * rec)
{
	unsigned int Mask = 0;
	char * Context;		
	char * ptr1 = strtok_s(value, " ,=\t\n\r", &Context);

	// Param is a comma separated list of Appl Numbers allowed to connect on this port

	while (ptr1 && ptr1[0])
	{
		Mask |= 1 << (atoi(ptr1) - 1);
		ptr1 = strtok_s(NULL, " ,=\t\n\r", &Context);
	}

	xxp.HavePermittedAppls = 1;		// indicate used
	xxp.PERMITTEDAPPLS = Mask;

	return 1;
}
int doKissCommand(int i, char * value, char * rec)
{
	// Param is kiss command and any operands as decimal bytes

	xxp.KissParams = _strdup(strlop(rec, '='));
	return 1;
}


int hwtypes(int i, char value[], char rec[])
{
	hw = 255;
	if (_stricmp(value,"ASYNC") == 0)
	{
		// Set some defaults

		xxp.CHANNEL = 'A';
		xxp.FRACK = 7000;
		xxp.RESPTIME = 1000;
		xxp.MAXFRAME = 4;
		xxp.RETRIES = 6;
		hw = 0;
	}

	if (_stricmp(value,"PC120") == 0)
	   hw = 2;
	if (_stricmp(value,"DRSI") == 0)
	   hw = 4;
	if (_stricmp(value,"DE56") == 0)
	   hw = 4;
	if (_stricmp(value,"TOSH") == 0)
	   hw = 6;
	if (_stricmp(value,"QUAD") == 0)
	   hw = 8;
	if (_stricmp(value,"RLC100") == 0)
	   hw = 10;
	if (_stricmp(value,"RLC400") == 0)
	   hw = 12;
	if (_stricmp(value,"INTERNAL") == 0 || _stricmp(value,"LOOPBACK") == 0)
	{
		// Set Sensible defaults

		memset(xxp.ID, ' ', 30);
		memcpy(xxp.ID, "Loopback", 8);
		xxp.CHANNEL = 'A';
		xxp.FRACK = 5000;
		xxp.RESPTIME = 1000;
		xxp.MAXFRAME = 4;
		xxp.RETRIES = 5;
		xxp.DIGIFLAG = 1;
		hw = 14;
	}
	if (_stricmp(value,"EXTERNAL") == 0)
	{
	   hw = 16;

		// Set some defaults in case KISSHF

		xxp.CHANNEL = 'A';
		xxp.FRACK = 7000;
		xxp.RESPTIME = 1000;
		xxp.MAXFRAME = 4;
		xxp.RETRIES = 6;
	}

	if (_stricmp(value,"BAYCOM") == 0)
	   hw = 18;
	if (_stricmp(value,"PA0HZP") == 0)
	   hw = 20;
	if (_stricmp(value,"I2C") == 0)
	{
		// Set some defaults

		xxp.CHANNEL = 'A';
		xxp.FRACK = 7000;
		xxp.RESPTIME = 1000;
		xxp.MAXFRAME = 4;
		xxp.RETRIES = 6;
		hw = 22;
	}

	if (hw == 255)
	{
		Consoleprintf("Invalid Hardware Type (not DRSI PC120 INTERNAL EXTERNAL BAYCOM PA0HZP ASYNC QUAD)");
		Consoleprintf("%s\r\n", rec);
		return (0);
	}
	else
		xxp.TYPE = hw;

	return(1);
}
int protocols(int i, char value[], char rec[])
{
	int hw;

	hw = 255;
	if (_stricmp(value,"KISS") == 0)
	   hw = 0;
	if (_stricmp(value,"NETROM") == 0)
	   hw = 2;
	if (_stricmp(value,"BPQKISS") == 0)
	   hw = 4;
	if (_stricmp(value,"HDLC") == 0)
	   hw = 6;
	if (_stricmp(value,"L2") == 0)
	   hw = 8;
	if (_stricmp(value,"PACTOR") == 0)
	   hw = 10;
	if (_stricmp(value,"WINMOR") == 0)
	   hw = 10;
	if (_stricmp(value,"ARQ") == 0)
	   hw = 12;

	if (hw == 255)
	{
		Consoleprintf("Invalid Protocol (not KISS NETROM PACTOR WINMOR ARQ HDLC )");
		Consoleprintf("%s\r\n", rec);
		return (0);
	}
	else
		xxp.PROTOCOL = hw;
	return(1);
}


int bbsflag(int i, char value[],char rec[])
{
	int hw=255;

	if (_stricmp(value,"NOBBS") == 0)
	   hw = 1;
	if (_stricmp(value,"BBSOK") == 0)
	   hw = 0;
	if (_stricmp(value,"") == 0)
	   hw = 0;

	if (hw==255)
	{
	   Consoleprintf("BBS Flag must be NOBBS, BBSOK, or null");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}

	xxp.BBSFLAG = hw;
	
	return(1);
}

int channel(int i, char * value, char * rec)
{
	char * val = (char *)poffset[i];
	val[0]  = value[0];
	return 1;
}

int dolinked(int i, char * value, char * rec)
{
	char * val = (char *)offset[i];
	val[0]  = value[0];
	return 1;
}

int validcalls(int i, char * value, char * rec)
{
	if ((strlen(value) + (int)strlen(xxp.VALIDCALLS)) > 255)
	{
		Consoleprintf("Too Many VALIDCALLS");
		Consoleprintf("%s\r\n", rec);
		return(0);
	}

	strcat(xxp.VALIDCALLS, value);
    return(1);
}


int kissoptions(int i, char value[], char rec[])
{
	int err=255;

	char opt1[12] = "";
	char opt2[12] = "";
	char opt3[12] = "";
	char opt4[12] = "";
	char opt5[12] = "";
	char opt6[12] = "";
	char opt7[12] = "";
	char opt8[12] = "";



	sscanf(value,"%[^,+],%[^,+],%[^,+],%[^,+],%[^,+],%[^,+],%[^,+],%[^,+]",
		opt1,opt2,opt3,opt4,opt5,opt6,opt6,opt8);

	if (opt1[0] != '\0') {do_kiss(opt1,rec);}
	if (opt2[0] != '\0') {do_kiss(opt2,rec);}
	if (opt3[0] != '\0') {do_kiss(opt3,rec);}
	if (opt4[0] != '\0') {do_kiss(opt4,rec);}
	if (opt5[0] != '\0') {do_kiss(opt5,rec);}
	if (opt6[0] != '\0') {do_kiss(opt6,rec);}
	if (opt7[0] != '\0') {do_kiss(opt7,rec);}
	if (opt8[0] != '\0') {do_kiss(opt8,rec);}

	return(1);
}



/*
		TNC PORT PROCESSING 
*/
static char *tkeywords[] = 
{
"COM", "TYPE", "APPLMASK", "KISSMASK", "APPLFLAGS", "ENDPORT"
};           /* parameter keywords */

static int toffset[] =
{
0, 1, 2, 3, 5, 8
};		/* offset for corresponding data in config file */

static int troutine[] = 
{
1, 5, 1, 3, 1, 9
};		/* routine to process parameter */

#define TPARAMLIM 6


typedef struct _TCMDX
{
	char String[12];			// COMMAND STRING
	UCHAR CMDLEN;				// SIGNIFICANT LENGTH
	VOID (* CMDPROC)(struct TNCDATA * TNC, char * Tail, struct _TCMDX * CMD);// COMMAND PROCESSOR
	size_t CMDFLAG;				// FLAG/VALUE Offset

} TCMDX;



extern TCMDX TNCCOMMANDLIST[];
extern int NUMBEROFTNCCOMMANDS;

int decode_tnc_rec(char * rec)
{
	char key_word[20];
	char value[300];

	if (xindex(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

	if (_stricmp(key_word, "ENDPORT") == 0)
	{
		endport=1;
		return 0;
	}
	else if (_stricmp(key_word, "TYPE") == 0)
	{
		if (_stricmp(value, "TNC2") == 0)
		{
			TNC2ENTRY->Mode = TNC2;

			// Set Defaults

			TNC2ENTRY->SENDPAC = 13;
			TNC2ENTRY->CRFLAG = 1;
			TNC2ENTRY->MTX = 1;
			TNC2ENTRY->MCOM = 1;
			TNC2ENTRY->MMASK = -1;			//  MONITOR MASK FOR PORTS

			TNC2ENTRY->COMCHAR = 3;
			TNC2ENTRY->CMDTIME = 10;		// SYSTEM TIMER = 100MS
			TNC2ENTRY->PASSCHAR = 0x16;		// CTRL-V
			TNC2ENTRY->StreamSW = 0x7C;		// |
			TNC2ENTRY->LCStream = 1;
		}
		else if (_stricmp(value, "DED") == 0)
			TNC2ENTRY->Mode = DED;
		else if (_stricmp(value, "KANT") == 0)
			TNC2ENTRY->Mode = KANTRONICS;
		else if (_stricmp(value, "SCS") == 0)
			TNC2ENTRY->Mode = SCS;
		else
		{
			Consoleprintf("Invalid TNC Type");
			Consoleprintf("%s\r\n",rec);
		}
	}
	else if (_stricmp(key_word, "COMPORT") == 0)
		strcpy(TNC2ENTRY->PORTNAME, value);
	else if (_stricmp(key_word, "APPLMASK") == 0)
		TNC2ENTRY->APPLICATION =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "APPLNUM") == 0)
		TNC2ENTRY->APPLICATION =  1 << (strtol(value, 0, 0) - 1);
	else if (_stricmp(key_word, "APPLFLAGS") == 0)
		TNC2ENTRY->APPLFLAGS =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "CHANNELS") == 0)
		TNC2ENTRY->HOSTSTREAMS =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "STREAMS") == 0)
		TNC2ENTRY->HOSTSTREAMS =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "POLLDELAY") == 0)
		TNC2ENTRY->PollDelay =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "CONOK") == 0)
		TNC2ENTRY->CONOK =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "AUTOLF") == 0)
		TNC2ENTRY->AUTOLF =  strtol(value, 0, 0);
	else if (_stricmp(key_word, "ECHO") == 0)
		TNC2ENTRY->ECHOFLAG =  (char)strtol(value, 0, 0);
	else
	{
		if (TNC2ENTRY->Mode == TNC2)
		{
			// Try process as TNC2 Command

			int n = 0;
			TCMDX * CMD = &TNCCOMMANDLIST[0];
			char * ptr1 = key_word;
			UCHAR * valueptr;

			strcat(key_word, "  ");

			_strupr(key_word);
	
			for (n = 0; n < NUMBEROFTNCCOMMANDS; n++)
			{
				int CL = CMD->CMDLEN;

				// ptr1 is input command
				
				ptr1 = key_word;

				if (memcmp(CMD->String, ptr1, CL) == 0)
				{
					// Found match so far - check rest
		
					char * ptr2 = &CMD->String[CL];
			
					ptr1 += CL;

					if (*(ptr1) != ' ')
					{
						while(*(ptr1) == *ptr2 && *(ptr1) != ' ')
						{
							ptr1++;
							ptr2++;
						}
					}

					if (*(ptr1) == ' ')
					{
						valueptr = (UCHAR *)TNC2ENTRY + CMD->CMDFLAG;
						*valueptr = (UCHAR)strtol(value, 0, 0);
						return 0;
					}
				}
				CMD++;
			}
		}
		
		Consoleprintf("Source record not recognised - Ignored:%s\r\n",rec);
	}
	return 0;
}


int do_kiss (char * value,char * rec)
{
	int err=255;

	if (_stricmp(value,"POLLED") == 0)
	{
		err=0;
		kissflags=kissflags | POLLINGKISS;
	}
	else if (_stricmp(value,"CHECKSUM") == 0)
	{
		err=0;
		kissflags=kissflags | CHECKSUM;
	}
	else if (_stricmp(value,"D700") == 0)
	{
		err=0;
		kissflags=kissflags | D700;
	}
	else if (_stricmp(value,"TNCX") == 0)
	{
		err=0;
		kissflags=kissflags | TNCX;
	}
	else if (_stricmp(value,"PITNC") == 0)
	{
		err=0;
		kissflags=kissflags | PITNC;
	}
	else if (_stricmp(value,"TRACKER") == 0)
	{
		err=0;
		kissflags |= TRACKER;
	}
	else if (_stricmp(value,"NOPARAMS") == 0)
	{
		err=0;
		kissflags=kissflags | NOPARAMS;
	}
	else if (_stricmp(value,"ACKMODE") == 0)
	{
		err=0;
		kissflags=kissflags | ACKMODE;
	}
	else if (_stricmp(value,"SLAVE") == 0)
	{
		err=0;
		kissflags=kissflags | POLLEDKISS;
	}
	else if (_stricmp(value,"FLDIGI") == 0)
	{
		err=0;
		kissflags |= FLDIGI;
	}
	else if (_stricmp(value,"FASTI2C") == 0)
	{
		err=0;
		kissflags |= FASTI2C;
	}

	else if (_stricmp(value,"DRATS") == 0)
	{
		err=0;
		kissflags |= DRATS;
	}

	if (err == 255)
	{
	   Consoleprintf("Invalid KISS Options (not POLLED ACKMODE CHECKSUM D700 SLAVE TNCX PITNC NOPARAMS FASTI2C DRATS)");
	   Consoleprintf("%s\r\n",rec);
	}
	return (err);
}


int simple(int i)
{
	// Set up the basic config header

	xxcfg.C_AUTOSAVE = 1;
	xxcfg.C_SaveMH = 1;
	xxcfg.C_BBS = 1;
	xxcfg.C_BTINTERVAL = 60;
	xxcfg.C_BUFFERS = 999;
	xxcfg.C_C = 1;
	xxcfg.C_DESQVIEW = 0;
	xxcfg.C_EMSFLAG = 0;
	xxcfg.C_FULLCTEXT = 1;
	xxcfg.C_HIDENODES = 0;
	xxcfg.C_HOSTINTERRUPT = 127;
	xxcfg.C_IDINTERVAL = 10;
	xxcfg.C_IDLETIME = 900;
	xxcfg.C_IP = 0;
	xxcfg.C_PM = 0;
	xxcfg.C_L3TIMETOLIVE = 25;
	xxcfg.C_L4DELAY = 10;
	xxcfg.C_L4RETRIES = 3;
	xxcfg.C_L4TIMEOUT = 60;
	xxcfg.C_L4WINDOW = 4;
	xxcfg.C_LINKEDFLAG = 'A';
	xxcfg.C_MAXCIRCUITS = 128;
	xxcfg.C_MAXDESTS = 250;
	xxcfg.C_MAXHOPS = 4;
	xxcfg.C_MAXLINKS = 64;
	xxcfg.C_MAXNEIGHBOURS = 64;
	xxcfg.C_MAXRTT = 90;
	xxcfg.C_MINQUAL = 150;
	xxcfg.C_NODE = 1;
	xxcfg.C_NODESINTERVAL = 30;
	xxcfg.C_OBSINIT = 6;
	xxcfg.C_OBSMIN = 5;
	xxcfg.C_PACLEN = 236;
	xxcfg.C_T3 = 180;
	xxcfg.C_TRANSDELAY = 1;

	/* Set PARAMOK flags on all values that are defaulted */

	for (i=0; i < PARAMLIM; i++)
	   paramok[i]=1;

	paramok[15] = 0;		// Must have callsign 
	paramok[45] = 0;		// Dont Have Appl1Call
	paramok[53] = 0;		// or APPL1ALIAS

	return(1);
}

VOID FreeConfig()
{
}

BOOL ProcessAPPLDef(char * buf)
{
	// New Style APPL definition

	// APPL n,COMMAND,CMDALIAS,APPLCALL,APPLALIAS,APPLQUAL,L2ALIAS

	char * ptr1, * ptr2;
	int Appl, n = 0;
	char Param[8][256];
	struct APPLCONFIG * App;

	memset(Param, 0, 2048);

	ptr1 = buf;

	while (ptr1 && *ptr1 && n < 8)
	{
		ptr2 = strchr(ptr1, ',');
		if (ptr2) *ptr2++ = 0;

		strcpy(&Param[n++][0], ptr1);
		ptr1 = ptr2;
	}

	if (_stricmp(Param[1], Param[2]) == 0)
	{
		// Alias = Application - will loop.

		return FALSE;
	}

	_strupr(Param[0]);
	_strupr(Param[1]);

	//	Leave Alias in original case

	_strupr(Param[3]);
	_strupr(Param[4]);
	_strupr(Param[5]);
	_strupr(Param[6]);
	_strupr(Param[7]);


	Appl = atoi(Param[0]);

	if (Appl < 1 || Appl > NumberofAppls) return FALSE;

	App = &xxcfg.C_APPL[Appl - 1];		// Recs from zero

	if (Param[1][0] == 0)				// No Application
		return FALSE;

	if (strlen(Param[1]) > 12) return FALSE;

	memcpy(App->Command, Param[1], (int)strlen(Param[1]));

	xxcfg.C_BBS = 1;
		
	if (strlen(Param[2]) > 48) return FALSE;

	memcpy(App->CommandAlias, Param[2], (int)strlen(Param[2]));

	if (strlen(Param[3]) > 10) return FALSE;

	memcpy(App->ApplCall, Param[3], (int)strlen(Param[3]));

	if (strlen(Param[4]) > 10) return FALSE;

	memcpy(App->ApplAlias, Param[4], (int)strlen(Param[4]));

	App->ApplQual = atoi(Param[5]);

	if (strlen(Param[6]) > 10) return FALSE;

	memcpy(App->L2Alias, Param[6], (int)strlen(Param[6]));

	return TRUE;
}

double xfmod(double p1, double p2)
{
	int temp;

	temp = (int)(p1/p2);
	p1 = p1 -(p2 * temp);
	return p1;
}

 BOOL ToLOC(double Lat, double Lon , char * Locator)
 {
	int i;
	double S1, S2;

	Lon = Lon + 180;
	Lat = Lat + 90;

	S1 = xfmod(Lon, 20);

	#pragma warning(push)
	#pragma warning(disable : 4244)
	
	i = Lon / 20;
	Locator[0] = 65 + i;

	S2 = xfmod(S1, 2);

	i = S1 / 2;
	Locator[2] = 48 + i;

	i = S2 * 12;
	Locator[4] = 65 + i;

	S1 = xfmod(Lat,10);

	i = Lat / 10;
	Locator[1] = 65 + i;

	S2 = xfmod(S1,1);

	i = S1;
	Locator[3] = 48 + i;

	i = S2 * 24;
	Locator[5] = 65 + i;

	#pragma warning(pop)

	return TRUE;
}

int FromLOC(char * Locator, double * pLat, double * pLon)
{
	double i;
	double Lat, Lon;

	_strupr(Locator);

	*pLon = 0;
	*pLat = 0;			// in case invalid


	// Basic validation for APRS positions

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
