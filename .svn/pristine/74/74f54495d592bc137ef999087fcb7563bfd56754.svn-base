
#ifndef CONFIGSTRUCT
#define CONFIGSTRUCT


// MAKE SURE SHORTS ARE CORRECTLY ALLIGNED FOR ARMV5

struct PORTCONFIG
{
	short PORTNUM;
	char ID[30];			//2
	short TYPE;			    // 32,
	short PROTOCOL;			// 34,
	short IOADDR;			// 36,
	short INTLEVEL;			// 38,
	unsigned short SPEED;	// 40,
	unsigned char CHANNEL;	// 42,
	unsigned char pad;
	short BBSFLAG;			// 44, 
	short QUALITY;			// 46, 
	short MAXFRAME;			// 48,
	short TXDELAY;			// 50,
	short SLOTTIME;			// 52, 
	short PERSIST;			// 54,

	short FULLDUP;			// 56,
	short SOFTDCD;			// 58, 
	short FRACK;			// 60, 
	short RESPTIME;			// 62,
	short RETRIES;			// 64, 

	short PACLEN;			// 66,
	short QUALADJUST;		// 68,
	UCHAR DIGIFLAG;			// 70,
	UCHAR DIGIPORT;			// 71 
	short DIGIMASK;			// 72
	short USERS;			// 74,
	short TXTAIL;			// 76
	unsigned char  ALIAS_IS_BBS;		// 78
	unsigned char pad2;
	char CWID[10];			// 80,
	char PORTCALL[10];		//  90,
	char PORTALIAS[10];		// 100,
	char L3ONLY;			// 110,
	char IGNOREUNLOCKED;	// 111
	short KISSOPTIONS;		// 112,
	short INTERLOCK;		// 114,
	short NODESPACLEN;		// 116,
	short TXPORT;			// 118,
	UCHAR MHEARD;			// 120,
	UCHAR CWIDTYPE;			// 121,
	char MINQUAL;			// 122, 
	char MAXDIGIS;			//  123,
	char DefaultNoKeepAlives; // 124
	char UIONLY;			// 125,
	unsigned short ListenPort;	// 126
	char UNPROTO[72];		//  128, 
	char PORTALIAS2[10];	//  200,
	char DLLNAME[16];		//  210,
	char BCALL[10];			// 226,
	unsigned long IPADDR;	// 236
	char I2CMode;			// 240
	char I2CAddr;			// 241
	char INP3ONLY;			// 242
	char NoNormalize;		// 243 Normalise Nodes Qualities
	unsigned short TCPPORT;	// 244
	char Pad2[10];			// 246
	char VALIDCALLS[256];	//   256 - 512
	struct WL2KInfo * WL2K;			// 512
	char * SerialPortName; // 516
	struct XDIGI * XDIGIS;	//  596 Cross port digi setup
	int RIGPORT;			// Linked port with RigControl 
	unsigned int PERMITTEDAPPLS;	// Appls allowed on this port
	int HavePermittedAppls;			// Indicated PERMITTEDAPPLS uses
	int Hide;				// Don't show on Ports display or AGW Connect Menu
//	long long txOffset;		// Transverter tx offset 
//	long long rxOffset;		// Transverter rx offset ppa
	int SmartID;
	unsigned char * KissParams;
	int SendtoM0LTEMap;
	uint64_t PortFreq;
	char * M0LTEMapInfo;
	int QtSMPort;
};

struct ROUTECONFIG
{
	char call[80];		// May have VIA
	int quality;
	int port;
	int pwind;
	int pfrack;
	int ppacl;
	int farQual;
};

struct CONFIGTABLE
{
//	CONFIGURATION DATA STRUCTURE

//	DEFINES LAYOUT OF CONFIG RECORD PRODUCED BY CONFIGURATION PROG

	char C_NODECALL[10];		// OFFSET = 0 
	char C_NODEALIAS[10];		// OFFSET = 10
	char C_BBSCALL[10];			// OFFSET = 20
	char C_BBSALIAS[10];		// OFFSET = 30

	short C_OBSINIT;			// OFFSET = 40
	short C_OBSMIN;				// OFFSET = 42
	short C_NODESINTERVAL;		// OFFSET = 44
	short C_L3TIMETOLIVE;		// OFFSET = 46
	short C_L4RETRIES;			// OFFSET = 48
	short C_L4TIMEOUT;			// OFFSET = 50
	short C_BUFFERS;			// OFFSET = 52
	short C_PACLEN;				// OFFSET = 54
	short C_TRANSDELAY;			// OFFSET = 56
	short C_T3;					// OFFSET = 58
	short Spare1;				// OFFSET = 60
	short Spare2;				// OFFSET = 62
	short C_IDLETIME;			// OFFSET = 64
	UCHAR C_EMSFLAG;			// OFFSET = 66
	UCHAR C_LINKEDFLAG;			// OFFSET = 67
	UCHAR C_BBS;				// OFFSET = 68
	UCHAR C_NODE;				// OFFSET = 69
	UCHAR C_HOSTINTERRUPT;		// OFFSET = 70
	UCHAR C_DESQVIEW;			// OFFSET = 71
	short C_MAXLINKS;			// OFFSET = 72
	short C_MAXDESTS;
	short C_MAXNEIGHBOURS;
	short C_MAXCIRCUITS;		// 78
	UCHAR C_TNCPORTLIST[16];	// OFFSET = 80
	short C_IDINTERVAL;			// 96
	short C_FULLCTEXT;			// 98    ; SPARE (WAS DIGIFLAG)
	short C_MINQUAL;			// 100
	UCHAR C_HIDENODES;			// 102
	UCHAR C_AUTOSAVE;			// 103
	short C_L4DELAY;			// 104
	short C_L4WINDOW;			// 106
	short C_BTINTERVAL;			// 108
	UCHAR C_L4APPL;				// 110
	UCHAR C_C;					//  111 "C" = HOST Command Enabled
	UCHAR C_IP;					//  112 IP Enabled
	UCHAR C_MAXRTT;				// 113
	UCHAR C_MAXHOPS;			// 114
	UCHAR C_PM;					// 115 Poermapper Enabled
	UCHAR C_LogL4Connects;		// 116
	UCHAR C_SaveMH;				// 117
	short C_BBSQUAL;			// 118
	UCHAR C_WASUNPROTO;
	UCHAR C_BTEXT[120];			// 121
	char C_VERSTRING[10];		// 241 Version String from Config File
	UCHAR C_ADIF;
	UCHAR C_EVENTS;
	UCHAR C_LogAllConnects;	
	UCHAR C_SaveAPRSMsgs;
	UCHAR C_M0LTEMap;
	UCHAR C_VERSION;			// CONFIG PROG VERSION
//	Reuse C_APPLICATIONS - no longer used
	char C_NETROMCALL[10];
	UCHAR C_EXCLUDE[71];
	char C_IDMSG[512];
	char C_CTEXT[512];
	char C_INFOMSG[2048];
	UCHAR CfgBridgeMap[MaxBPQPortNo + 1][MaxBPQPortNo + 1];
	struct ROUTECONFIG C_ROUTE[MaxLockedRoutes];
	struct APPLCONFIG C_APPL[NumberofAppls];
	struct PORTCONFIG C_PORT[MaxBPQPortNo + 4];
	int C_MQTT;
	char C_MQTT_HOST[80];
	int C_MQTT_PORT;
	char C_MQTT_USER[80];
	char C_MQTT_PASS[80];
	int C_L4Compress;
	int C_L4CompMaxframe;
	int C_L4CompPaclen;
	int C_L2Compress;
	int C_L2CompMaxframe;
	int C_L2CompPaclen;


//#define ApplOffset 80000			// Applications offset in config buffer
//#define InfoOffset 85000			// Infomsg offset in  buffer
//#define InfoMax	2000				// Max Info

//#define C_IDMSG	512 
//#define C_ROUTES 90000				// Allow 2500
//#define C_CTEXT	2048
//#define C_PORTS	2560
//#define C_INFOMSG 85000

};

struct UPNP 
{
	struct UPNP * Next;
	char * Protocol;
	char * LANport;
	char * WANPort;
};

#endif



