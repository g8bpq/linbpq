//
// Prototypes for BPQ32 Node Functions
//

#define DllImport

#define EXCLUDEBITS

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "compatbits.h"

#include "asmstrucs.h"

BOOL CheckExcludeList(UCHAR * Call);

Dll int ConvFromAX25(unsigned char * incall,unsigned char * outcall);
Dll BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call);
DllExport BOOL ConvToAX25Ex(unsigned char * callsign, unsigned char * ax25call);
int WritetoConsoleLocal(char * buff);
VOID Consoleprintf(const char * format, ...);
VOID FreeConfig();
int GetListeningPortsPID(int Port);

void * InitializeExtDriver(PEXTPORTDATA PORTVEC);

VOID PutLengthinBuffer(PDATAMESSAGE buff, USHORT datalen);			// Needed for arm5 portability
int GetLengthfromBuffer(PDATAMESSAGE buff);	
int IntDecodeFrame(MESSAGE * msg, char * buffer, time_t Stamp, uint64_t Mask, BOOL APRS, BOOL MCTL);
int IntSetTraceOptionsEx(uint64_t mask, int mtxparam, int mcomparam, int monUIOnly);
int CountBits64(uint64_t in);


#define GetSemaphore(Semaphore,ID) _GetSemaphore(Semaphore, ID, __FILE__, __LINE__)

#define GetBuff() _GetBuff(__FILE__, __LINE__)
#define ReleaseBuffer(s) _ReleaseBuffer(s, __FILE__, __LINE__)
#define CheckGuardZone() _CheckGuardZone(__FILE__, __LINE__)

#define Q_REM(s) _Q_REM(s, __FILE__, __LINE__)
#define Q_REM_NP(s) _Q_REM_NP(s, __FILE__, __LINE__)

#define C_Q_ADD(s, b) _C_Q_ADD(s, b, __FILE__, __LINE__)

void _CheckGuardZone(char * File, int Line);

VOID * _Q_REM(VOID **Q, char * File, int Line);
VOID * _Q_REM_NP(VOID *Q, char * File, int Line);

int _C_Q_ADD(VOID *Q, VOID *BUFF, char * File, int Line);

UINT _ReleaseBuffer(VOID *BUFF, char * File, int Line);

VOID * _GetBuff(char * File, int Line);
int _C_Q_ADD(VOID *PQ, VOID *PBUFF, char * File, int Line);

int C_Q_COUNT(VOID *Q);

DllExport char * APIENTRY GetApplCall(int Appl);
DllExport char * APIENTRY GetApplAlias(int Appl);
DllExport int APIENTRY FindFreeStream();
DllExport int APIENTRY DeallocateStream(int stream);
DllExport int APIENTRY SessionState(int stream, int * state, int * change);
DllExport int APIENTRY SetAppl(int stream, int flags, int mask);
DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count );
DllExport int APIENTRY GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window);

#define LIBCONFIG_STATIC
#include "libconfig.h"

int GetIntValue(config_setting_t * group, char * name);
BOOL GetStringValue(config_setting_t * group, char * name, char * value, int maxlen);
VOID SaveIntValue(config_setting_t * group, char * name, int value);
VOID SaveStringValue(config_setting_t * group, char * name, char * value);

int EncryptPass(char * Pass, char * Encrypt);
VOID DecryptPass(char * Encrypt, unsigned char * Pass, unsigned int len);
Dll VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset);
Dll BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase);

DllExport int APIENTRY TXCount(int stream);
DllExport int APIENTRY RXCount(int stream);
DllExport int APIENTRY MONCount(int stream);

VOID ReadNodes();
int BPQTRACE(MESSAGE * Msg, BOOL APRS);

VOID CommandHandler(TRANSPORTENTRY * Session, struct DATAMESSAGE * Buffer);

VOID PostStateChange(TRANSPORTENTRY * Session);

VOID InnerCommandHandler(TRANSPORTENTRY * Session, struct DATAMESSAGE * Buffer);
VOID DoTheCommand(TRANSPORTENTRY * Session);
char * MOVEANDCHECK(TRANSPORTENTRY * Session, char * Bufferptr, char * Source, int Len);
VOID DISPLAYCIRCUIT(TRANSPORTENTRY * L4, char * Buffer);
char * strlop(char * buf, char delim);
BOOL CompareCalls(UCHAR * c1, UCHAR * c2);

VOID PostDataAvailable(TRANSPORTENTRY * Session);
int WritetoConsoleLocal(char * buff);
char * CHECKBUFFER(TRANSPORTENTRY * Session, char * Bufferptr);
VOID CLOSECURRENTSESSION(TRANSPORTENTRY * Session);

VOID SendCommandReply(TRANSPORTENTRY * Session, struct DATAMESSAGE * Buffer, int Len);

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromPortNum(int portnum);

int cCOUNT_AT_L2(struct _LINKTABLE * LINK);
VOID SENDL4CONNECT(TRANSPORTENTRY * Session, int Service);

VOID CloseSessionPartner(TRANSPORTENTRY * Session);
int COUNTNODES(struct ROUTE * ROUTE);
int DecodeNodeName(char * NodeName, char * ptr);;
VOID DISPLAYCIRCUIT(TRANSPORTENTRY * L4, char * Buffer);
int cCOUNT_AT_L2(struct _LINKTABLE * LINK);
void * zalloc(int len);
BOOL FindDestination(UCHAR * Call, struct DEST_LIST ** REQDEST);

BOOL ProcessConfig();

VOID PUT_ON_PORT_Q(struct PORTCONTROL * PORT, MESSAGE * Buffer);
VOID CLEAROUTLINK(struct _LINKTABLE * LINK);
VOID TellINP3LinkGone(struct ROUTE * Route);
VOID CLEARACTIVEROUTE(struct ROUTE * ROUTE, int Reason);

// Reason Equates

#define NORMALCLOSE 0
#define RETRIEDOUT 1
#define SETUPFAILED 2
#define LINKLOST 3
#define LINKSTUCK 4

int COUNT_AT_L2(struct _LINKTABLE * LINK);
VOID SENDIDMSG();
VOID SENDBTMSG();
VOID INP3TIMER();
VOID REMOVENODE(dest_list * DEST);
BOOL ACTIVATE_DEST(struct DEST_LIST * DEST);
VOID TellINP3LinkSetupFailed(struct ROUTE * Route);
BOOL FindNeighbour(UCHAR * Call, int Port, struct ROUTE ** REQROUTE);
VOID PROCROUTES(struct DEST_LIST * DEST, struct ROUTE * ROUTE, int Qual);
BOOL L2SETUPCROSSLINK(PROUTE ROUTE);
VOID REMOVENODE(dest_list * DEST);
char * SetupNodeHeader(struct DATAMESSAGE * Buffer);
VOID L4CONNECTFAILED(TRANSPORTENTRY * L4);
int CountFramesQueuedOnSession(TRANSPORTENTRY * Session);
VOID CLEARSESSIONENTRY(TRANSPORTENTRY * Session);
VOID __cdecl Debugprintf(const char * format, ...);

int APIENTRY Restart();
int APIENTRY Reboot();
int APIENTRY Reconfig();
Dll int APIENTRY SaveNodes ();


struct SEM;

void _GetSemaphore(struct SEM * Semaphore, int ID, char * File, int Line);
void FreeSemaphore(struct SEM * Semaphore);

void MySetWindowText(HWND hWnd, char * Msg);

Dll int APIENTRY SessionControl(int stream, int command, int Mask);

HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits);
int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength);
BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite);
VOID CloseCOMPort(HANDLE fd);

VOID initUTF8();
int Is8Bit(unsigned char *cpt, int len);
int WebIsUTF8(unsigned char *ptr, int len);
int IsUTF8(unsigned char *ptr, int len);
int Convert437toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF);
int Convert1251toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF);
int Convert1252toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF);
int TrytoGuessCode(unsigned char * Char, int Len);


#define CMD_TO_APPL	1	// PASS COMMAND TO APPLICATION
#define MSG_TO_USER	2	// SEND 'CONNECTED' TO USER
#define MSG_TO_APPL	4	//	SEND 'CONECTED' TO APPL
#define CHECK_FOR_ESC 8	// Look for ^d (^D) to disconnect session)

#define	UI	3
#define	SABM 0x2F
#define	DISC 0x43
#define	DM	0x0F
#define	UA	0x63
#define	FRMR 0x87
#define	RR	1
#define	RNR	5
#define	REJ	9

// V2.2 Types

#define	SREJ 0x0D
#define SABME 0x6F
#define XID 0xAF
#define TEST 0xE3

// XID Optional Functions

#define OPMustHave 0x02A080		// Sync TEST 16 bit FCS Extended Address
#define OPSREJ 4
#define OPSREJMult 0x200000
#define OPREJ 2
#define OPMod8 0x400
#define OPMod128 0x800

#define BPQHOSTSTREAMS	64

extern TRANSPORTENTRY * L4TABLE;
extern unsigned char NEXTID;
extern int MAXCIRCUITS;
extern int L4DEFAULTWINDOW;
extern int L4T1;
extern APPLCALLS APPLCALLTABLE[];
extern char * APPLS;
extern int NEEDMH;
extern int RFOnly;

extern char SESSIONHDDR[];

extern UCHAR NEXTID;

extern struct ROUTE * NEIGHBOURS;
extern int  MAXNEIGHBOURS;

extern struct ROUTE * NEIGHBOURS;
extern int  ROUTE_LEN;
extern int  MAXNEIGHBOURS;

extern struct DEST_LIST * DESTS;				// NODE LIST
extern struct DEST_LIST * ENDDESTLIST;
extern int  DEST_LIST_LEN;
extern int  MAXDESTS;			// MAX NODES IN SYSTEM

extern struct _LINKTABLE * LINKS;
extern int	LINK_TABLE_LEN; 
extern int	MAXLINKS;



extern char	MYCALL[]; //		DB	7 DUP (0)	; NODE CALLSIGN (BIT SHIFTED)
extern char	MYALIASTEXT[]; //	{"      "	; NODE ALIAS (KEEP TOGETHER)

extern UCHAR	MYCALLWITHALIAS[13];
extern APPLCALLS APPLCALLTABLE[NumberofAppls];

extern UCHAR MYNODECALL[];				// NODE CALLSIGN (ASCII)
extern char NODECALLLOPPED[];			// NODE CALLSIGN (ASCII). Null terminated
extern UCHAR MYNETROMCALL[];			// NETROM CALLSIGN (ASCII)

extern UCHAR NETROMCALL[];				// NETORM CALL (AX25)

extern VOID * FREE_Q;

extern struct PORTCONTROL * PORTTABLE;
extern int	NUMBEROFPORTS;


extern int OBSINIT;				// INITIAL OBSOLESCENCE VALUE
extern int OBSMIN;					// MINIMUM TO BROADCAST
extern int L3INTERVAL;			// "NODES" INTERVAL IN MINS
extern int IDINTERVAL;			// "ID" BROADCAST INTERVAL
extern int BTINTERVAL;			// "BT" BROADCAST INTERVAL
extern int MINQUAL;				// MIN QUALITY FOR AUTOUPDATES
extern int HIDENODES;				// N * COMMAND SWITCH
extern int BBSQUAL;				// QUALITY OF BBS RELATIVE TO NODE

extern int NUMBEROFBUFFERS;		// PACKET BUFFERS
extern int PACLEN;				//MAX PACKET SIZE

//	L2 SYSTEM TIMER RUNS AT 3 HZ

extern int T3;				// LINK VALIDATION TIMER (3 MINS) (+ a bit to reduce RR collisions)

extern int L2KILLTIME;		// IDLE LINK TIMER (16 MINS)	
extern int L3LIVES;				// MAX L3 HOPS
extern int L4N2;					// LEVEL 4 RETRY COUNT
extern int L4LIMIT;			// IDLE SESSION LIMIT - 15 MINS
extern int L4DELAY;				// L4 DELAYED ACK TIMER
	
extern int BBS;					// INCLUDE BBS SUPPORT
extern int NODE;					// INCLUDE SWITCH SUPPORT

extern int FULL_CTEXT;				// CTEXT ON ALL CONNECTS IF NZ


// Although externally streams are numbered 1 to 64, internally offsets are 0 - 63

extern BPQVECSTRUC DUMMYVEC;					// Needed to force correct order of following

extern BPQVECSTRUC BPQHOSTVECTOR[BPQHOSTSTREAMS + 5];

extern int NODEORDER;
extern UCHAR LINKEDFLAG;

extern UCHAR UNPROTOCALL[80];


extern char * INFOMSG;

extern char * CTEXTMSG;
extern int CTEXTLEN;

extern UCHAR MYALIAS[7];				// ALIAS IN AX25 FORM
extern UCHAR BBSALIAS[7];

extern VOID * TRACE_Q;				// TRANSMITTED FRAMES TO BE TRACED

extern char HEADERCHAR;				// CHAR FOR _NODE HEADER MSGS

extern int AUTOSAVE;				// AUTO SAVE NODES ON EXIT FLAG
extern int L4APPL;					// Application for BBSCALL/ALIAS connects
extern int CFLAG;					// C =HOST Command

extern VOID * IDMSG_Q;				// ID/BEACONS WAITING TO BE SENT

extern struct DATAMESSAGE BTHDDR;
extern struct _MESSAGE IDHDDR;

extern VOID * IDMSG;

extern int	L3TIMER;					// TIMER FOR 'NODES' MESSAGE
extern int	IDTIMER;					// TIMER FOR ID MESSAGE
extern int	BTTIMER;					// TIMER FOR BT MESSAGE

extern int STATSTIME;


extern BOOL IPRequired;
extern int MaxHops;
extern int MAXRTT;
extern USHORT CWTABLE[];
extern TRANSPORTENTRY * L4TABLE;
extern UCHAR ROUTEQUAL;
extern UINT BPQMsg;
extern UCHAR ExcludeList[];


extern APPLCALLS APPLCALLTABLE[];

extern char VersionStringWithBuild[];
extern char VersionString[];

extern int MAXHEARDENTRIES;
extern int MHLEN;

extern int APPL1;
extern int PASSCMD;
extern int NUMBEROFCOMMANDS;

extern char * ConfigBuffer;

extern char * WL2KReportLine[];

extern struct CMDX COMMANDS[];

extern int QCOUNT, MAXBUFFS, MAXCIRCUITS, L4DEFAULTWINDOW, L4T1, CMDXLEN;
extern char CMDALIAS[ALIASLEN][NumberofAppls];

extern int SEMGETS;
extern int SEMRELEASES;
extern int SEMCLASHES;
extern int MINBUFFCOUNT;

extern UCHAR BPQDirectory[];
extern UCHAR BPQProgramDirectory[];

extern UCHAR WINMOR[];
extern UCHAR PACTORCALL[]; 

extern UCHAR MCOM;
extern UCHAR MUIONLY;
extern UCHAR MTX;
extern uint64_t MMASK;

extern UCHAR NODECALL[];			//  NODES in ax.25

extern int L4CONNECTSOUT;
extern int L4CONNECTSIN;
extern int L4FRAMESTX;
extern int L4FRAMESRX;
extern int L4FRAMESRETRIED;
extern int OLDFRAMES;
extern int L3FRAMES;

extern char * PortConfig[];
extern struct SEM Semaphore;
extern UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

extern int REALTIMETICKS;

extern time_t CurrentSecs;
extern time_t lastSlowSecs;
extern time_t lastSaveSecs;

// SNMP Variables

extern int InOctets[64];
extern int OutOctets[64];

extern BOOL CloseAllNeeded;
extern int CloseOnError;

extern char * PortConfig[70];
extern struct TNCINFO * TNCInfo[71];		// Records are Malloc'd

#define MaxBPQPortNo 63		// Port 64 reserved for BBS Mon
#define MAXBPQPORTS 63

// IP, APRS use port ocnfig slots above the real port range

#define IPConfigSlot MaxBPQPortNo + 1
#define PortMapConfigSlot MaxBPQPortNo + 2
#define APRSConfigSlot MaxBPQPortNo + 3


extern char * UIUIDigi[MaxBPQPortNo + 1];
extern char UIUIDEST[MaxBPQPortNo + 1][11];		// Dest for Beacons
extern UCHAR FN[MaxBPQPortNo + 1][256];			// Filename
extern int Interval[MaxBPQPortNo + 1];			// Beacon Interval (Mins)
extern char Message[MaxBPQPortNo + 1][1000];		// Beacon Text

extern int MinCounter[MaxBPQPortNo + 1];			// Interval Countdown
extern BOOL SendFromFile[MaxBPQPortNo + 1];

extern BOOL MQTT;
extern char MQTT_HOST[80];
extern int MQTT_PORT;
extern char MQTT_USER[80];
extern char MQTT_PASS[80];

extern int SUPPORT2point2;


DllExport uint64_t APIENTRY GetPortFrequency(int PortNo, char * FreqStringMhz);


void hookL2SessionAccepted(int Port, char * remotecall, char * ourcall, struct _LINKTABLE * LINK);
void hookL2SessionDeleted(struct _LINKTABLE * LINK);
void hookL2SessionAttempt(int Port, char * ourcall, char * remotecall, struct _LINKTABLE * LINK);
