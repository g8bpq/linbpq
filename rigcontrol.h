#ifndef RIGCONTROL
#define RIGCONTROL

#ifndef LINBPQ
#include "Rigresource.h"
#endif

#define IDI_ICON2 2


struct TimeScan
{
	time_t Start;
	time_t End;
	struct ScanEntry ** Scanlist;
};

struct ScanEntry
{
	//	Holds info for one frequency change. May Need to set Feeq, Mode, Repeater Split, Pactor/Winmor Bandwidth

	double Freq;		// In case needed to report to WL2K
	int Dwell;			// Dwell Time on this freq
	char Bandwidth;		// W/N For WINMOR
	char RPacketMode;	// Robust or Normal for Tracker
	char HFPacketMode;	// Robust or Normal for Tracker
	char PMaxLevel;		// Pactor Max Level
	char PMinLevel;		// Pactor Max Level
	char Antenna;
//	char Supress;		// Dont report this one to WL2K
	char ARDOPMode[6];
	char VARAMode;
	char * Cmd1;
	int Cmd1Len;
	char * Cmd2;
	int Cmd2Len;
	char * Cmd3;
	int Cmd3Len;
	char * Cmd4;
	int Cmd4Len;
	char * PollCmd;
	int PollCmdLen;
	char APPL[13];		// Autoconnect APPL for this Freq
	char APPLCALL[10];	// Callsign for autoconnect application
	char Cmd1Msg[100];	// Space for commands
	char Cmd2Msg[16];
	char Cmd3Msg[16];
	char Cmd4Msg[16];
};

struct HAMLIBSOCK 
{
	struct HAMLIBSOCK * Next;
	SOCKET Sock;
	union
	{
		struct sockaddr_in6 sin6;  
		struct sockaddr_in sin;
	};
};

struct TNCINFO;

struct RIGINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	void * BPQtoRADIO_Q;			// Frames from switch for radio

	uint64_t BPQPort;				// Port this radio is attached to. Bit Map, as may be more than one port controlling radio
//	int PortNum;				// Number of port that defined this rig
	int Interlock;				// Interlock group for this Radio
	int IC735;					// Old ICOM with shorter freq message
	int ICF8101;				// ICOM Land Mobile IC-F8101
	char * CM108Device;			// Device to open for CM108 GPIO PTT

	struct _EXTPORTDATA * PortRecord[64]; // BPQ32 port record(s) for this rig (null terminated list)

	UCHAR RigAddr;
	int Channel;				// For sdrangel
	uint64_t ScanStopped;		// Scanning enabled if zero. Bits used for interlocked scanning (eg winmor/pactor on same port
	int ScanCounter;
	int PollCounter;			// Don't poll too often;
	int ScanFreq;				// Scan Rate

	BOOL OKtoChange;			// Can Change
	BOOL WaitingForPermission;	//

	BOOL RIGOK;					// RIG is reponding

	int Session;				// BPQ L4 Record Number
	int	DebugDelay;	

	char RigName[10];

	struct TimeScan ** TimeBands;	// List of TimeBands/Frequencies
	int NumberofBands;

	char * FreqText;			// Frequency list in text format

	// Frequency list is a block of ScanEntry structs, each holding Set Freq/Mode commands in link format, null terminated

	struct ScanEntry ** FreqPtr;

	int PTTMode;				// PTT Control Flags.

	#define PTTRTS		1
	#define PTTDTR		2
	#define PTTCI_V		4
	#define PTTCM108	8
	#define PTTHAMLIB	16
	#define PTTFLRIG	32

	int PTTTimer;				// PTT Timer watchdog (limits PTT ON to PTT OFF time

	#define PTTLimit 200

	int repeatPTTOFFTimer;		// On ICOM radios send a second PTT OFF command after 30 secs

	struct RIGPORTINFO * PORT;		// For PTT Routines

	HWND hLabel;
	HWND hCAT;
	HWND hFREQ;
	HWND hMODE;
	HWND hSCAN;
	HWND hPTT;
	HWND hPORTS;

	char * WEB_Label;
	char * WEB_CAT;
	char * WEB_FREQ;
	char * WEB_MODE;
	char * WEB_PORTS;
	char WEB_SCAN;
	char WEB_PTT;

	double RigFreq;
	char Valchar[15];			// Freq as char string
	char CurrentBandWidth;		// bandwidth as character
	int Passband;				// Filter bandwidth
	char ModeString[16];

	char PTTOn[60];
	char PTTOff[60];
	int PTTOnLen;
	int PTTOffLen;

	char Poll[50];
	int PollLen;

	char PTTCATPort[4][10];
	HANDLE PTTCATHandles[4];
	struct TNCINFO * PTTCATTNC[4];

	int RealMux[4];		// BPQ Virtual or Real

	int TSMenu;			// Menu number for ACC?USB switching on TS590S/SG
	BOOL RIG_DEBUG;

	int HAMLIBPORT;		// Port Number for HAMLIB (rigctld) Emulator

	SOCKET ListenSocket;

	struct HAMLIBSOCK * Sockets;

	long long txOffset;		// Used primarily with transverters
	long long rxOffset;
	long long pttOffset;	// Used to set tx freq to corresponding rx freq (for split freq)
	long long txFreq;		// Used in preference to Rx Freq + pttOffset if set
	int rxError;			// RX Frequency Error
	int txError;			// TX Calibration Error
	int PTTSetsFreq;
	int defaultFreq;

	char * reportFreqs;

	long long lastSetFreq;	// Last freq we set (saves recalulating ptt string if not changed

};

// PortType Equates

#define ICOM 1
#define YAESU 2
#define KENWOOD 3
#define PTT 4
#define ANT 5
#define FT100 6
#define FT2000 7
#define FLEX 8
#define M710 9
#define NMEA 10
#define FT1000 11
#define DUMMY 12
#define FT990 13
#define HAMLIB 14
#define FT991A 15			// 991A is a varient of FT2000 but easier to have own type
#define RTLUDP 16
#define FLRIG 17
#define SDRRADIO 18
//G7TAJ
#define SDRANGEL 19
//G7TAJ
#define FTDX10 20

// Yease seem to have lots of variants of the same model

#define FT1000D	1
#define FT1000MP 2 


struct RIGPORTINFO
{
	int PortType;				// ICOM, Yaesu, Etc
	int YaesuVariant;			// Yaesu seems to have lots of incompatible subtypes
	char IOBASE[80];
	char PTTIOBASE[80];			// Port for Hardware PTT - may be same as control port.
	int SPEED;
	char * HIDDevice; 
	struct RIGINFO Rigs[10];	// Rigs off a port
	char * InitPtr;				// Next Command
	int CmdSent;				// Last Command sent
	int CmdStream;				// Stream last command was issued o
	int	ConfiguredRigs;			// Radios on this interface
	int CurrentRig;				// Radio last accessed.

	int Timeout;				// Timeout response counter
	int Retries;
	BOOL PORTOK;				// PORT is reponding

	int Closed;					// CLOSE command received

	HANDLE hDevice;					// COM device Handle
	int ReopenDelay;
	struct TNCINFO * PTC;			// Set to TNC record address if using an SCS PTC Radio Port
	SOCKET remoteSock;				// Socket for use with WINMORCONROL
	struct sockaddr remoteDest;		// Dest for above
	HANDLE hPTTDevice;				// May use a different port for PTT
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
// ---- G7TAJ ----
	UCHAR RXBuffer[8192];			// Message being received - may not arrive all at once. SDRANGLE needs a lot
// ---- G7TAJ ----
	int RXLen;						// Data in RXBUffer
	BOOL AutoPoll;					// set if last command was a Timer poll 
	// Local ScanStruct for Interactive Commands
	struct ScanEntry * FreqPtr;		// Block we are currently sending.
	struct ScanEntry ScanEntry;	
	int CONNECTED;					// for HAMLIB
	int CONNECTING;
	int Alerted;
};


#define IOCTL_SERIAL_IS_COM_OPEN CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_BPQ_ADD_DEVICE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_DELETE_DEVICE  CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define W98_SERIAL_GETDATA     0x801
#define W98_SERIAL_SETDATA     0x802

#endif
