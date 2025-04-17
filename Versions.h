
#ifdef Kernel

#define Vers 5,2,9,2
#define Verstring "5.2.9.2\0"
#define Datestring "September 2012"
#define VerComments "G8BPQ Packet Switch V5.2.9.2\0"
#define VerCopyright "Copyright � 2001-2012 John Wiseman G8BPQ\0"
#define VerDesc "BPQ32 Switch\0"

#endif

#define KVers 6,0,24,70
#define KVerstring "6.0.24.70\0"


#ifdef CKernel

#define Vers KVers
#define Verstring KVerstring
#define Datestring "April 2025"
#define VerComments "G8BPQ Packet Switch (C Version)" KVerstring
#define VerCopyright "Copyright � 2001-2025 John Wiseman G8BPQ\0"
#define VerDesc "BPQ32 Switch\0"
#define VerProduct "BPQ32"

#endif

#ifdef TermTCP

#define Vers 1,0,16,2
#define Verstring "1.0.16.2\0"
#define VerComments "Internet Terminal for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2011-2025 John Wiseman G8BPQ\0"
#define VerDesc "Simple TCP Terminal Program for G8BPQ Switch\0"
#define VerProduct "BPQTermTCP"

#endif

#ifdef BPQTerm

#define Vers 2,2,5,2
#define Verstring "2.2.5.2\0"
#define VerComments "Simple Terminal for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 1999-2025 John Wiseman G8BPQ\0"
#define VerDesc "Simple Terminal Program for G8BPQ Switch\0"
#define VerProduct "BPQTerminal"

#endif

#ifdef BPQTermMDI

#define Vers 2,2,0,3
#define Verstring "2.2.0.3\0"
#define VerComments "MDI Terminal for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 1999-2025 John Wiseman G8BPQ\0"
#define VerDesc "MDI Terminal Program for G8BPQ Switch\0"

#endif

#ifdef MAIL

#define Vers KVers
#define Verstring KVerstring
#define VerComments "Mail server for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2009-2025 John Wiseman G8BPQ\0"
#define VerDesc "Mail server for G8BPQ's 32 Bit Switch\0"
#define VerProduct "BPQMail"

#endif

#ifdef HOSTMODES

#define Vers 1,1,8,1
#define Verstring "1.1.8.1\0"
//#define SPECIALVERSION "Test 3"
#define VerComments "Host Modes Emulator for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2009-2019 John Wiseman G8BPQ\0"
#define VerDesc "Host Modes Emulator for G8BPQ's 32 Bit Switch\0"
#define VerProduct "BPQHostModes"

#endif


#ifdef UIUTIL

#define Vers 0,1,3,1
#define Verstring "0.1.3.1\0"
#define VerComments "Beacon Utility for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2011-2019 John Wiseman G8BPQ\0"
#define VerDesc "Beacon Utility for G8BPQ Switch\0"
#define VerProduct "BPQUIUtil"

#endif

#ifdef AUTH

#define Vers 0,1,0,0
#define Verstring "0.1.0.0\0"
#define VerComments "Password Generation Utility for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2011-2025 John Wiseman G8BPQ\0"
#define VerDesc "Password Generation Utility for G8BPQ Switch\0"

#endif

#ifdef APRS

#define Vers KVers
#define Verstring KVerstring
#define VerComments  "APRS Client for G8BPQ Switch\0"
#define VerCopyright "Copyright � 2012-2025 John Wiseman G8BPQ\0"
#define VerDesc "APRS Client for G8BPQ Switch\0"
#define VerProduct "BPQAPRS"

#endif

#ifdef CHAT
 
#define Vers KVers
#define Verstring KVerstring
#define VerComments "Chat server for G8BPQ Packet Switch\0"
#define VerCopyright "Copyright � 2009-2025 John Wiseman G8BPQ\0"
#define VerDesc "Chat server for G8BPQ's 32 Bit Switch\0"
#define VerProduct "BPQChat"

#endif
