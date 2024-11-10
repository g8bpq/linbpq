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
//
//	409l	Oct 2001 Fix l3timeout for KISS
//
//	409m	Oct 2001 Fix Crossband Digi
//
//	409n	May 2002 Change error handling on load ext DLL

//	409p	March 2005 Allow Multidigit COM Ports (kiss.c)

//	409r	August 2005 Treat NULL string in Registry as use current directory
//						Allow shutdown to close BPQ Applications

//	409s	October 2005 Add DLL:Export entries to API for BPQTNC2

//	409t	January 2006
//
//			Add API for Perl "GetPerlMsg"
//			Add	API for BPQ1632	"GETBPQAPI"	- returns address of Assembler API routine
//			Add Registry Entry "BPQ Directory". If present, overrides "Config File Location"
//			Add New API "GetBPQDirectory" - Returns location of config file
//			Add New API "ChangeSessionCallsign" - equivalent to "*** linked to" command
//			Rename BPQNODES to BPQNODES.dat
//			New API "GetAttachedProcesses" - returns number of processes connected.
//			Warn if user trys to close Console Window.
//			Add Debug entries to record Process Attach/Detach
//			Fix recovery following closure of first process

//	409t Beta 2	February 2006
//
//			Add API Entry "GetPortNumber"
//
//	409u	February 2006
//
//			Fix crash if allocate/deallocate called with stream=0
//			Add API to ch
//			Display config file path
//			Fix saving of Locked Node flag
//			Added SAVENODES SYSOP command
//
//	409u 2	March 2006
//
//			Fix SetupBPQDirectory
//			Add CopyBPQDirectory (for Basic Programs)
//
//	409u 3	March 2006
//
//			Release streams on DLL unload

//	409v	October 2006
//
//			Support Minimize to Tray for all BPQ progams
//			Implement L4 application callsigns

//	410		November 2006
//
//			Modified to compile with C++ 2005 Express Edition
//			Make MCOM MTX MMASK local variables
//
// 410a		January 2007
//
//			Add program name to Attach-Detach messages
//			Attempt to detect processes which have died 
//			Fix bug in NETROM and IFrame decode which would cause crash if frame was corrupt
//			Add BCALL - origin call for Beacons
//			Fix KISS ACKMODE ACK processing
//

//	410b	November 2007
//
//			Allow CTEXT of up to 510, and enforce PACLEN, fragmenting if necessary

// 410c		December 2007

//			Fix problem with NT introduced in V410a
//			Display location of DLL on Console

// 410d		January 2008

//				Fix crash in DLL Init caused by long path to program
//				Invoke Appl2 alias on C command (if enabled)
//				Allow C command to be disabled
//				Remove debug trap in GETRAWFRAME
//				Validate Alias of directly connected node, mainly for KPC3 DISABL Problem
//				Move Port statup code out of DLLInit (mainly for perl)
//				Changes to allow Load/Unload of bpq32.dll by appl
//				CloseBPQ32 API added
//				Ext Driver Close routes called
//				Changes to release Mutex

// 410e		May 2008

//				Fix missing SSID on last call of UNPROTO string (CONVTOAX25 in main.asm)
//				Fix VCOM Driver (RX Len was 1 byte too long)
//				Fix possible crash on L4CODE if L4DACK received out of sequence
//				Add basic IP decoding

// 410f		October 2008

//				Add IP Gateway
//				Add Multiport DIGI capability
//				Add GetPortDescription API
//				Fix potential hangs if RNR lost
//				Fix problem if External driver failes to load
//				Put pushad/popad round _INITIALISEPORTS (main.asm)
//				Add APIs GetApplCallVB and GetPortDescription (mainly for RMS)
//				Ensure Route Qual is updated if Port Qual changed
//				Add Reload Option, plus menu items for DUMP and SAVENODES

// 410g		December 2008

//				Restore API Exports BPQHOSTAPIPTR and MONDECODEPTR (accidentally deleted)
//				Fix changed init of BPQDirectory (accidentally changed)
//				Fix Checks for lost processes (accidentally deleted)
//				Support HDLC Cards on W2K and above
//				Delete Tray List entries for crashed processes
//				Add Option to NODES command to sort by Callsign
//				Add options to save or clear BPQNODES before Reconfig.
//				Fix Reconfig in Win98
//				Monitor buffering tweaks
//				Fix Init for large (>64k) tables
//				Fix Nodes count in Stats

// 410h		January 2009

//				Add Start Minimized Option
//				Changes to KISS for WIn98 Virtual COM
//					Open \\.\com instead of //./COM
//					Extra Dignostics

// 410i		Febuary 2009

//				Revert KISS Changes
//				Save Window positions

// 410j		June 2009

//				Fix tidying of window List when program crashed
//				Add Max Nodes to Stats
//				Don't update APPLnALIAS with received NODES info
//				Fix MH display in other timezones
//				Fix Possible crash when processing NETROM type Zero frames (eg NRR)
//				Basic INP3 Stuff
//				Add extra diagnostics to Lost Process detection
//				Process Netrom Record Route frames.

// 410k		June 2009

//				Fix calculation of %retries in extended ROUTES display
//				Fix corruption of ROUTES table

// 410l		October 2009

//				Add GetVersionString API call.
//				Add GetPortTableEntry API call
//				Keep links to neighbouring nodes open

// Build 2

//				Fix PE in NOROUTETODEST (missing POP EBX)

// 410m		November 2009

//				Changes for PACTOR and WINMOR to support the ATTACH command
//				Enable INP3 if configured on a route.
//				Fix count of nodes in Stats Display
//				Overwrite the worst quality unused route if a call is received from a node not in your
//				table when the table is full

//	Build 5

//	Rig Control Interface
//	Limit KAM VHF attach and RADIO commands to authorised programs (MailChat and BPQTerminal)

// Build 6

// Fix reading INP3 Flag from BPQNODES

// Build 7

//	Add MAXHOPS and MAXRTT config options

// Build 8

// Fix INP3 deletion of Application Nodes.
// Fix GETCALLSIGN for Pactor Sessions
// Add N Call* to display all SSID's of a call
// Fix flow control on Pactor sessions.

// Build 9

// HDLC Support for XP
// Add AUTH routines

// Build 10

// Fix handling commands split over more that one packet.

// Build 11

// Attach cmd changes for winmor disconnecting state
// Option Interlock Winmor/Pactor ports

// Build 12

// Add APPLS export for winmor
// Handle commands ending CR LF

// Build 13

// Incorporate Rig Control in Kernel

// Build 14

// Fix config reload for Rig COntrol 

// 410n		March 2010

// Implement C P via PACTOR/WINMOR (for Airmail)

// Build 2

// Don't flip SSID bits on Downlink Connect if uplink is Pactor/WINMOR
// Fix resetting IDLE Timer on Pactor/WINMOR sessions
// Send L4 KEEPLI messages based on IDLETIME

// 410o		July 2010

// Read bpqcfg.txt instead of .bin
// Support 32 bit MMASK (Allowing 32 Ports)
// Support 32 bit _APPLMASK (Allowing 32 Applications)
// Allow more commands
// Allow longer command aliases
// Fix logic error in RIGControl Port Initialisation (wasn't always raising RTS and DTR
// Clear RIGControl RTS and DTR on close

// 410o		Build 2 August 2010

// Fix couple of errors in config (needed APPLICATIONS and BBSCALL/ALIAS/QUAL)
// Fix Kenwood Rig Control when more than one message received at once.
// Save minimzed state of Rigcontrol Window

// 410o		Build 3 August 2010

// Fix reporting of set errors in scan to a random session

// 410o		Build 4 August 2010

// Change All xxx Ports are in use to no xxxx Ports are available if there are no sessions with _APPLMASK
// Fix validation of TRANSDELAY

// 410o		Build 5 August 2010

//	Add Repeater Shift and Set Data Mode options to Rigcontrol (for ICOM only)
//	Add WINMOR and SCS Pactor mode control option to RigControl
//  Extend INFOMSG to 2000 bytes
//  Improve Scan freq change lock (check both SCS and WINMOR Ports)

// 410o		Build 6 September 2010

// Incorporate IPGateway in main code.
// Fix GetSessionInfo for Pactor/Winmor Ports
// Add Antenna Selection to RigControl
// Allow Bandwidth options on RADIO command line (as well as in Scan definitions)

// 410o		Build 7 September 2010

// Move rigconrtol display to driver windows
// Move rigcontrol config to driver config.
// Allow driver and IPGateway config info in bpq32.cfg
// Move IPGateway, AXIP, VKISS, AGW and WINMOR drivers into bpq32.dll
// Add option to reread IP Gateway config.
// Fix Reinit after process with timer closes (error in TellSessions).

// 410p		Build 2 October 2010

// Move KAM and SCS drivers to bpq32.dll

// 410p		Build 3 October 2010

// Support more than one axip port.

// 410p		Build 4 October 2010

// Dynamically load psapi.dll (for 98/ME)

// 410p		Build 5 October 2010

// Incorporate TelnetServer
// Fix AXIP ReRead Config
// Report AXIP accept() fails to syslog, not a popup.

// 410p		Build 6 October 2010

// Includes HAL support
// Changes to Pactor Drivers disconnect code
// AXIP now sends with source port = dest port, unless overridden by SOURCEPORT param
// Config now checks for duplicate port definitions
// Add Node Map reporting
// Fix WINMOR deferred disconnect.
// Report Pactor PORTCALL to WL2K instead of RMS Applcall

// 410p		Build 7 October 2010

// Add In/Out flag to Map reporting, and report centre, not dial
// Write Telnet log to BPQ Directory
// Add Port to AXIP resolver display
// Send Reports to update.g8bpq.net:81
// Add support for FT100 to Rigcontrol
// Add timeout to Rigcontrol PTT
// Add Save Registry Command

// 410p		Build 8 November 2010

// Add NOKEEPALIVES Port Param
// Renumbered for release

// 410p		Build 9 November 2010

// Get Bandwith for map report from WL2K Report Command
// Fix freq display for FT100 (was KHz, not MHz)
// Don't try to change SCS mode whilst initialising
// Allow reporting of Lat/Lon as well as Locator
// Fix Telnet Log Name
// Fix starting with Minimized windows when Minimizetotray isn't set
// Extra Program Error trapping in SessionControl
// Fix reporting same freq with different bandwidths at different times.
// Code changes to support SCS Robust Packet Mode.
// Add FT2000 to Rigcontrol
// Only Send CTEXT to connects to Node (not to connects to an Application Call)

// Released as Build 10

// 410p		Build 11 January 2011

// Fix MH Update for SCS Outgoing Calls
// Add Direct CMS Access to TelnetServer
// Restructure DISCONNECT processing to run in Timer owning process

// 410p		Build 12 January 2011

// Add option for Hardware PTT to use a different com port from the scan port 
// Add CAT PTT for Yaesu 897 (and maybe others)
// Fix RMS Packet ports busy after restart
// Fix CMS Telnet with MAXSESSIONS > 10

// 410p		Build 13 January 2011

// Fix loss of buffers in TelnetServer
// Add CMS logging.
// Add non - Promiscuous mode option for BPQETHER 

// 410p		Build 14 January 2011

// Add support for BPQTermTCP
// Allow more that one FBBPORT
// Allow Telnet FBB mode sessions to send CRLF as well as CR on user and pass msgs
// Add session length to CMS Telnet logging.
// Return Secure Session Flag from GetConnectionInfo
// Show Uptime as dd/hh/mm

// 4.10.16.17	March 2011

// Add "Close all programs" command
// Add BPQ Program Directory registry key
// Use HKEY_CURRENT_USER on Vista and above (and move registry if necessary)
// Time out IP Gateway ARP entries, and only reload ax.25 ARP entries
// Add support for SCS Tracker HF Modes
// Fix WL2K Reporting
// Report Version to WL2K
// Add Driver to support Tracker with multiple sessions (but no scanning, wl2k report, etc)


// Above released as 5.0.0.1

// 5.2.0.1

// Add caching of CMS Server IP addresses
// Initialise TNC State on Pactor Dialogs
// Add Shortened (6 digit) AUTH mode.
// Update MH with all frames (not just I/UI)
// Add IPV6 Support for TelnetServer and AXIP
// Fix TNC OK Test for Tracker
// Fix crash in CMS mode if terminal disconnects while tcp commect in progress
// Add WL2K reporting for Robust Packet
// Add option to suppress WL2K reporting for specific frequencies
// Fix Timeband processing for Rig Control
// New Driver for SCS Tracker allowing multiple connects, so Tracker can be used for user access 
// New Driver for V4 TNC

// 5.2.1.3 October 2011

// Combine busy detector on Interlocked Ports (SCS PTC, WINMOR or KAM)
// Improved program error logging
// WL2K reporting changed to new format agreed with Lee Inman

// 5.2.3.1 January 2012

// Connects from the console to an APPLCALL or APPLALIAS now invoke any Command Alias that has been defined.
// Fix reporting of Tracker freqs to WL2K.
// Fix Tracker monitoring setup (sending M UISC)
// Fix possible call/application routing error on RP
// Changes for P4Dragon
// Include APRS Digi/IGate
// Tracker monitoring now includes DIGIS
// Support sending UI frames using SCSTRACKER, SCTRKMULTI and UZ7HO drivers
// Include driver for UZ7HO soundcard modem.
// Accept DRIVER as well as DLLNAME, and COMPORT as well as IOADDR in bpq32.cfg. COMPORT is decimal
// No longer supports separate config files, or BPQTELNETSERVER.exe
// Improved flow control for Telnet CMS Sessions
// Fix handling Config file without a newline after last line
// Add non - Promiscuous mode option for BPQETHER 
// Change Console Window to a Dialog Box.
// Fix possible corruption and loss of buffers in Tracker drivers
// Add Beacon After Session option to Tracker and UZ7HO Drivers
// Rewrite RigControl and add "Reread Config Command"
// Support User Mode VCOM Driver for VKISS ports

// 5.2.4.1 January 2012

// Remove CR from Telnet User and Password Prompts
// Add Rigcontrol to UZ7HO driver
// Fix corruption of Free Buffer Count by Rigcontol
// Fix WINMOR and V4 PTT
// Add MultiPSK Driver
// Add SendBeacon export for BPQAPRS
// Add SendChatReport function
// Fix check on length of Port Config ID String with trailing spaces
// Fix interlock when Port Number <> Port Slot
// Add NETROMCALL for L3 Activity
// Add support for APRS Application
// Fix Telnet with FBBPORT and no TCPPORT
// Add Reread APRS Config
// Fix switching to Pactor after scanning in normal packet mode (PTC)

// 5.2.5.1 February 2012

// Stop reading Password file.
// Add extra MPSK commands 
// Fix MPSK Transparency
// Make LOCATOR command compulsory
// Add MobileBeaconInterval APRS param
// Send Course and Speed when APRS is using GPS
// Fix Robust Packet reporting in PTC driver
// Fix corruption of some MIC-E APRS packets

// 5.2.6.1 February 2012

// Convert to MDI presentation of BPQ32.dll windows
// Send APRS Status packets
// Send QUIT not EXIT in PTC Init
// Implement new WL2K reporting format and include traffic reporting info in CMS signon
// New WL2KREPORT format
// Prevent loops when APPL alias refers to itself
// Add RigControl for Flex radios and ICOM IC-M710 Marine radio

// 5.2.7.1

//	Fix opening more thn one console window on Win98
//  Change method of configuring multiple timelots on WL2K reporting
//  Add option to update WK2K Sysop Database
//	Add Web server
//	Add UIONLY port option

// 5.2.7.2

//	Fix handling TelnetServer packets over 500 bytes in normal mode

// 5.2.7.3

//	Fix Igate handling packets from UIView

// 5.2.7.4

//	Prototype Baycom driver.

// 5.2.7.5

//  Set WK2K group ref to MARS (3) if using a MARS service code

// 5.2.7.7

//	Check for programs calling CloseBPQ32 when holding semaphore
//  Try/Except round Status Timer Processing

// 5.2.7.8

//  More Try/Except round Timer Processing

// 5.2.7.9

//	Enable RX in Baycom, and remove test loopback in tx

// 5.2.7.10

//	Try/Except round ProcessHTTPMessage

// 5.2.7.11

//	BAYCOM tweaks

// 5.2.7.13

//	Release semaphore after program error in Timer Processing
//  Check fro valid dest in REFRESHROUTE


//	Add TNC-X KISSOPTION (includes the ACKMODE bytes in the checksum(

// Version 5.2.9.1 Sept 2012

// Fix using KISS ports with COMn > 16
// Add "KISS over UDP" driver for PI as a TNC concentrator

// Version 6.0.1.1

// Convert to C for linux portability
// Try to speed up kiss polling

// Version 6.0.2.1

// Fix operation on Win98
// Fix callsign error with AGWtoBPQ
// Fix PTT problem with WINMOR
// Fix Reread telnet config
// Add Secure CMS signon
// Fix error in cashing addresses of CMS servers
// Fix Port Number when using Send Raw.
// Fix PE in KISS driver if invalid subchannel received
// Fix Orignal address of beacons
// Speed up Telnet port monitoring.
// Add TNC Emulators
// Add CountFramesQueuedOnStream API
// Limit number of frames that can be queued on a session.
// Add XDIGI feature
// Add Winmor Robust Mode switching for compatibility with new Winmor TNC
// Move most APRS code from BPQAPRS to here
// Stop corruption caused by overlong KISS frames

// Version 6.0.3.1

// Add starting/killing WINMOR TNC on remote host
// Fix Program Error when APRS Item or Object name is same as call of reporting station
// Dont digi a frame that we have already digi'ed
// Add ChangeSessionIdleTime API
// Add WK2KSYSOP Command
// Add IDLETIME Command
// Fix Errors in RELAYAPPL processing
// Fix PE cauaed by invalid Rigcontrol Line

// Version 6.0.4.1

// Add frequency dependent autoconnect appls for SCS Pactor
// Fix DED Monitoring of I and UI with no data
// Include AGWPE Emulator (from AGWtoBPQ)
// accept DEL (Hex 7F) as backspace in Telnet
// Fix re-running resolver on re-read AXIP config
// Speed up processing, mainly for Telnet Sessions
// Fix APRS init on restart of bpq32.exe
// Change to 2 stop bits
// Fix scrolling of WINMOR trace window
// Fix Crash when ueing DED TNC Emulator
// Fix Disconnect when using BPQDED2 Driver with Telnet Sessions
// Allow HOST applications even when CMS option is disabled
// Fix processing of APRS DIGIMAP command with no targets (didn't suppress default settings)

// Version 6.0.5.1 January 2014

//	Add UTF8 conversion mode to Telnet (converts non-UTF-8 chars to UTF-8)
//	Add "Clear" option to MH command
//	Add "Connect to RMS Relay" Option
//  Revert to one stop bit on serial ports, explictly set two on FT2000 rig control
//  Fix routing of first call in Robust Packet
//	Add Options to switch input source on rigs with build in soundcards (sor far only IC7100 and Kenwood 590)
//  Add RTS>CAT PTT option for Sound Card rigs
//	Add Clear Nodes Option (NODE DEL ALL)
//  SCS Pactor can set differeant APPLCALLS when scanning.
//	Fix possible Scan hangup after a manual requency change with SCS Pactor
//	Accept Scan entry of W0 to disable WINMOR on that frequency
//  Fix corruption of NETROMCALL by SIMPLE config command
//	Enforce Pactor Levels
//	Add Telnet outward connect
//	Add Relay/Trimode Emulation
//	Fix V4 Driver
//	Add PTT Mux
//  Add Locked ARP Entries (via bpq32.cfg)
//	Fix IDLETIME node command
//	Fix STAY param on connect
//	Add STAY option to Attach and Application Commands
//	Fix crash on copying a large AXIP MH Window
//	Fix possible crash when bpq32.exe dies
//	Fix DIGIPORT for UI frames

//  Version 6.0.6.1 April 2014

//  FLDigi Interface
//  Fix "All CMS Servers are inaccessible" message so Mail Forwarding ELSE works.
//	Validate INP3 messages to try to prevent crash
//  Fix possible crash if an overlarge KISS frame is received
//	Fix error in AXR command
//	Add LF to Telnet Outward Connect signin if NEEDLF added to connect line
//	Add CBELL to TNC21 emulator
//	Add sent objects and third party messages to APRS Dup List
//  Incorporate UIUtil
//	Use Memory Mapped file to pass APRS info to BPQAPRS, and process APRS HTTP in BPQ32
//	Improvements to FLDIGI interlocking
//	Fix TNC State Display for Tracker
//	Cache CMS Addresses on LinBPQ
//	Fix count error on DED Driver when handling 256 byte packets
//	Add basic SNMP interface for MRTG
//	Fix memory loss from getaddrinfo
//	Process "BUSY" response from Tracker
//	Handle serial port writes that don't accept all the data
//	Trap Error 10038 and try to reopen socket
//	Fix crash if overlong command line received

//  Version 6.0.7.1 Aptil 2014
//	Fix RigContol with no frequencies for Kenwood and Yaesu
//	Add busy check to FLDIGI connects

//  Version 6.0.8.1 August 2014

//	Use HKEY_CURRENT_USER on all OS versions
//	Fix crash when APRS symbol is a space.
//	Fixes for FT847 CAT
//	Fix display of 3rd byte of FRMR
//	Add "DEFAULT ROBUST" and "FORCE ROBUST" commands to SCSPactor Driver
//	Fix possible memory corruption in WINMOR driver
//	Fix FT2000 Modes
//	Use new WL2K reporting system (Web API Based)
//	APRS Server now cycles through hosts if DNS returns more than one
//	BPQ32 can now start and stop FLDIGI
//	Fix loss of AXIP Resolver when running more than one AXIP port

//  Version 6.0.9.1 November 2014

//	Fix setting NOKEEPALIVE flag on route created from incoming L3 message
//	Ignore NODES from locked route with quality 0
//	Fix seting source port in AXIP
//	Fix Dual Stack (IPV4/V6) on Linux.
//	Fix RELAYSOCK if IPv6 is enabled.
//	Add support for FT1000
//	Fix hang when APRS Messaging packet received on RF
//	Attempt to normalize Node qualies when stations use widely differing Route qualities
//	Add NODES VIA command to display nodes reachable via a specified neighbour
//	Fix applying "DisconnectOnClose" setting on HOST API connects (Telnet Server)
//	Fix buffering large messages in Telnet Host API
//	Fix occasional crash in terminal part line processing
//	Add "NoFallback" command to Telnet server to disable "fallback to Relay"
//	Improved support for APPLCALL scanning with Pactor
//	MAXBUFFS config statement is no longer needed.
//	Fix USEAPPLCALLS with Tracker when connect to APPLCALL fails
//	Implement LISTEN and CQ commands
//	FLDIGI driver can now start FLDIGI on a remote system.
//	Add IGNOREUNLOCKEDROUTES parameter
//	Fix error if too many Telnet server connections

//  Version 6.0.10.1 Feb 2015

//	Fix crash if corrupt HTML request received.
//	Allow SSID's of 'R' and 'T' on non-ax.25 ports for WL2K Radio Only network.
//	Make HTTP server HTTP Version 1.1 complient - use persistent conections and close after 2.5 mins
//	Add	INP3ONLY flag.
//	Fix program error if enter UNPROTO without a destination path
//	Show client IP address on HTTP sessions in Telnet Server
//	Reduce frequency and number of attempts to connect to routes when Keepalives or INP3 is set
//	Add FT990 RigControl support, fix FT1000MP support.
//	Support ARMV5 processors
//  Changes to support LinBPQ APRS Client
//	Add IC7410 to supported Soundcard rigs
//	Add CAT PTT to NMEA type (for ICOM Marine Radios_
//	Fix ACKMODE
//	Add KISS over TCP
//	Support ACKMode on VKISS
//	Improved reporting of configuration file format errors
//	Experimental driver to support ARQ sessions using UI frames

//  Version 6.0.11.1 September 2015

//	Fixes for IPGateway configuration and Virtual Circuit Mode
//	Separate Portmapper from IPGateway
//	Add PING Command
//	Add ARDOP Driver
//	Add basic APPLCALL support for PTC-PRO/Dragon 7800 Packet (using MYALIAS)
//	Add "VeryOldMode" for KAM Version 5.02
//	Add KISS over TCP Slave Mode.
//	Support Pactor and Packet on P4Dragon on one port
//	Add "Remote Staton Quality" to Web ROUTES display
//	Add Virtual Host option for IPGateway NET44 Encap
//	Add NAT for local hosts to IPGateway
//	Fix setting filter from RADIO command for IC7410
//	Add Memory Channel Scanning for ICOM Radios
//	Try to reopen Rig Control port if it fails (could be unplugged USB)
//	Fix restoring position of Monitor Window
//	Stop Codec on Winmor and ARDOP when an interlocked port is attached (instead of listen false)
//	Support APRS beacons in RP mode on Dragon//
//	Change Virtual MAC address on IPGateway to include last octet of IP Address
//	Fix "NOS Fragmentation" in IP over ax.25 Virtual Circuit Mode
//	Fix sending I frames before L2 session is up
//	Fix Flow control on Telnet outbound sessions.
//	Fix reporting of unterminatred comments in config
//	Add option for RigControl to not change mode on FT100/FT990/FT1000
//	Add "Attach and Connect" for Telnet ports

//  Version 6.0.12.1	November 2015

//	Fix logging of IP addresses for connects to FBBPORT
//	Allow lower case user and passwords in Telnet "Attach and Connect"
//	Fix possible hang in KISS over TCP Slave mode
//	Fix duplicating LinBPQ process if running ARDOP fails
//	Allow lower case command aliases and increase alias length to 48
//	Fix saving long IP frames pending ARP resolution
//	Fix dropping last entry from a RIP44 message.
//	Fix displaying Digis in MH list
//	Add port name to Monitor config screen port list
//	Fix APRS command display filter and add port filter
//	Support port names in BPQTermTCP Monitor config
//	Add FINDBUFFS command to dump lost buffers to Debugview/Syslog
//	Buffer Web Mgmt Edit Config output
//	Add WebMail Support
//	Fix not closing APRS Send WX file.
//	Add RUN option to APRS Config to start APRS Client
//	LinBPQ run FindLostBuffers and exit if QCOUNT < 5
//	Close and reopen ARDOP connection if nothing received for 90 secs
//	Add facility to bridge traffic between ports (similar to APRS Bridge but for all frame types)
//	Add KISSOPTION TRACKER to set SCS Tracker into KISS Mode

// 6.0.13.1

//	Allow /ex to exit UNPROTO mode
//	Support ARQBW commands.
//	Support IC735
//	Fix sending ARDOP beacons after a busy holdoff
//	Enable BPQDED driver to beacon via non-ax.25 ports.
//	Fix channel number in UZ7HO monitoring
//	Add SATGate mode to APRSIS Code.
//	Fix crash caused by overlong user name in telnet logon
//	Add option to log L4 connects
//	Add AUTOADDQuiet mode to AXIP.
//	Add EXCLUDE processing
//	Support WinmorControl in UZ7HO driver and fix starting TNC on Linux
//	Convert calls in MAP entries to upper case.
//	Support Linux COM Port names for APRS GPS
//	Fix using NETROM serial protocol on ASYNC Port
//	Fix setting MYLEVEL by scanner after manual level change.
//	Add DEBUGLOG config param to SCS Pactor Driver to log serial port traffic
//	Uue #myl to set SCS Pactor MYLEVEL, and add checklevel command
//	Add Multicast RX interface to FLDIGI Driver
//	Fix processing application aliases to a connect command.
//	Fix Buffer loss if radio connected to PTC rig port but BPQ not configured to use it
//	Save backups of bpq32.cfg when editing with Web interface and report old and new length
//	Add DD command to SCS Pactor, and use it for forced disconnect.
//	Add ARDOP mode select to scan config
//	ARDOP changes for ARDOP V 0.5+
//	Flip SSID bits on UZ7HO downlink connects


// Version 6.0.14.1

//	Fix Socket leak in ARDOP and FLDIGI drivers.
//	Add option to change CMS Server hostname
//	ARDOP Changes for 0.8.0+
//	Discard Terminal Keepalive message (two nulls) in ARDOP command hander
//	Allow parameters to be passed to ARDOP TNC when starting it
//	Fix Web update of Beacon params
//	Retry connects to KISS ports after failure
//	Add support for ARDOP Serial Interface Native mode.
//	Fix gating APRS-IS Messages to RF
//	Fix Beacons when PORTNUM used
//	Make sure old monitor flag is cleared for TermTCP sessions
//	Add CI-V antenna control for IC746
//	Don't allow ARDOP beacons when connected
//	Add support for ARDOP Serial over I2C
//	Fix possble crash when using manual RADIO messages
//	Save out of sequence L2 frames for possible reuse after retry
//	Add KISS command to send KISS control frame to TNC
//	Stop removing unused digis from packets sent to APRS-IS

//	Processing of ARDOP PING and PINGACK responses
//	Handle changed encoding of WL2K update responses.
//	Allow anonymous logon to telnet
//	Don't use APPL= for RP Calls in Dragon Single mode.
//	Add basic messaging page to APRS Web Server
//	Add debug log option to SCSTracker and TrkMulti Driver
//	Support REBOOT command on LinBPQ
//  Allow LISTEN command on all ports that support ax.25 monitoring

//	Version 6.0.15.1 Feb 2018

//	partial support for ax.25 V2.2
//	Add MHU and MHL commands and MH filter option
//	Fix scan interlock with ARDOP
//	Add Input source seiect for IC7300
//	Remove % transparency from web terminal signon message
//	Fix L4 Connects In count on stats
//	Fix crash caused by corrupt CMSInfo.txt
//	Add Input peaks display to ARDOP status window
//  Add options to show time in local and distances in KM on APRS Web pages
//	Add VARA support
//	Fix WINMOR Busy left set when port Suspended
//	Add ARDOP-Packet Support
//	Add Antenna Switching for TS 480
//	Fix possible crash in Web Terminal
//	Support different Code Pages on Console sessions
//	Use new Winlink API interface (api.winlink.org)
//	Support USB/ACC switching on TS590SG
//	Fix scanning when ARDOP or WINMOR is used without an Interlocked Pactor port.
//	Set NODECALL to first Application Callsign if NODE=0 and BBSCALL not set.
//	Add RIGCONTROL TUNE and POWER commands for some ICOM and Kenwwod rigs
//	Fix timing out ARDOP PENDING Lock
//	Support mixed case WINLINK Passwords
//	Add TUNE and POWER Rigcontol Commands for some radios
//  ADD LOCALTIME and DISPKM options to APRS Digi/Igate

// 6.0.16.1 March 2018

//	Fix Setting data mode and filter for IC7300 radios
//	Add VARA to WL2KREPORT
//	Add trace to SCS Tracker status window
//	Fix possible hang in IPGATEWAY
//	Add BeacontoIS parameter to APRSDIGI. Allows you to stop sending beacons to APRS-IS.
//	Fix sending CTEXT on WINMOR sessions 

// 6.0.17.1 November 2018

//	Change WINMOR Restart after connection to Restart after Failure and add same option to ARDOP and VARA
//	Add Abort Connection to WINMOR and VARA Interfaces
//	Reinstate accidentally removed CMS Access logging
//	Fix MH CLEAR 
//  Fix corruption of NODE table if NODES received from station with null alias
//	Fix loss of buffer if session closed with something in PARTCMDBUFFER
//	Fix Spurious GUARD ZONE CORRUPT message in IP Code.
//	Remove "reread bpq32.cfg and reconfigure" menu options
//	Add support for PTT using CM108 based soundcard interfaces
//	Datestamp Telnet log files and delete old Telnet and CMSAcces logs

// 6.0.18.1 January 2019

//	Fix validation of NODES broadcasts
//  Fix HIDENODES
//  Check for failure to reread config on axip reconfigure
//	Fix crash if STOPPORT or STARTPORT used on KISS over TCP port
//	Send Beacons from BCALL or PORTCALL if configured
//  Fix possible corruption of last entry in MH display
//	Ensure RTS/DTR is down when opening PTT Port
//	Remove RECONFIG command
//	Preparations for 64 bit version

// 6.0.19 Sept 2019
//	Fix UZ7HO interlock
//	Add commands to set Centre Frequency and Modem with UZ7HO Soundmodem (on Windows only)
//	Add option to save and restore MH lists and SAVEMH command
//	Add Frequency (if known) to UZ7HO MH lists
//	Add Gateway option to Telnet for PAT
//	Try to fix SCS Tracker recovery
//	Ensure RTS/DTR is down on CAT port if using that line for PTT
//	Experimental APRS Messaging in Kernel
//	Add Rigcontrol on remote PC's using WinmorControl
//	ADD VARAFM and VARAFM96 WL2KREPORT modes
//	Fix WL2K sysop update for new Winlink API
//	Fix APRS when using PORTNUM higher than the number of ports
//	Add Serial Port Type
//	Add option to linbpq to log APRS-IS messages.
//	Send WL2K Session Reports
//	Drop Tunneled Packets from 44.192 - 44.255 
//	Log incoming Telnet Connects
//	Add IPV4: and IPV6: overrides on AXIP Resolver.
//	Add SessionTimeLimit to HF sessions (ARDOP, SCSPactor, WINMOR, VARA)
//	Add RADIO FREQ command to display current frequency

// 6.0.20 April 2020

//	Trap and reject YAPP file transfer request.
//	Fix possible overrun of TCP to Node Buffer
//	Fix possible crash if APRS WX file doesn't have a terminating newline
//	Change communication with BPQAPRS.exe to restore old message popup behaviour
//	Preparation for 64 bit version
//	Improve flow control on SCS Dragon
//	Fragment messages from network links to L2 links with smaller paclen
//	Change WL2K report rate to once every two hours
//	Add PASS, CTEXT and CMSG commands and Stream Switch support to TNC2 Emulator
//	Add SessionTimeLimit command to  HF drivers (ARDOP, SCSPactor, WINMOR, VARA)
//	Add links to Ports Web Manangement Page to open individual Driver windows
//	Add STOPPORT/STARTPORT support to ARDOP, KAM and SCSPactor drivers
//	Add CLOSE and OPEN RADIO command so Rigcontrol port can be freed fpr other use.
//	Don't try to send WL2K Traffic report if Internet is down
//	Move WL2K Traffic reporting to a separate thread so it doesn't block if it can't connect to server
//	ADD AGWAPPL config command to set application number. AGWMASK is still supported
//	Register Node Alias with UZ7HO Driver
//	Register calls when UZ7HO TNC Restarts and at intervals afterwards
//	Fix crash when no IOADDR or COMPORT in async port definition
//  Fix Crash with Paclink-Unix when parsing ; VE7SPR-10 DE N7NIX QTC 1
//	Only apply BBSFLAG=NOBBS to APPPLICATION 1
//	Add RIGREONFIG command
//	fix APRS RECONFIG on LinBPQ
//	Fix Web Terminal scroll to end problem on some browsers
//	Add PTT_SETS_INPUT option for IC7600
//	Add TELRECONFIG command to reread users or whole config
//	Enforce PACLEN on UZ7HO ports
//	Fix PACLEN on Command Output.
//	Retry axip resolver if it fails at startup
//	Fix AGWAPI connect via digis
//  Fix Select() for Linux in MultiPSK, UZ7HO and V4 drivers
//	Limit APRS OBJECT length to 80 chars
//	UZ7HO disconnect incoming call if no free streams
//	Improve response to REJ (no F) followed by RR (F). 
//	Try to prevent more than MAXFRAME frames outstanding when transmitting
//	Allow more than one instance of APRS on Linux
//	Stop APRS digi by originating station
//	Send driver window trace to main monitor system
//	Improve handling of IPOLL messages
//	Fix setting end of address bit on dest call on connects to listening sessions
//	Set default BBS and CHAT application number and number of streams on LinBPQ
//	Support #include in bpq32.cfg processing

// Version 6.0.21 14 December 2020

//	Fix occasional missing newlines in some node command reponses
//	More 64 bit fixes
//	Add option to stop setting PDUPLEX param in SCSPACTOR
//	Try to fix buffer loss
//	Remove extra space from APRS position reports
//	Suppress VARA IAMALIVE messages
//	Add display and control of QtSoundModem modems
//	Only send "No CMS connection available" message if fallbacktorelay is set.
//	Add HAMLIB backend and emulator support to RIGCONTROL
//	Ensure all beacons are sent even with very short beacon intervals
//	Add VARA500 WL2K Reporting Mode
//  Fix problem with prpcessing frame collector
//	Temporarily disable L2 and L4 collectors till I can find problem
//	Fix possible problem with interactive RADIO commands not giving a response,
//	Incease maximum length of NODE command responses to handle maximum length INFO message,
//	Allow WL2KREPORT in CONFIG section of UZ7HO port config.
//	Fix program error in processing hamlib frame
//	Save RestartAfterFailure option for VARA
//	Check callsign has a winlink account before sending WL2KREPORT messages
//	Add Bandwidth control to VARA scanning
//	Renable L2 collector
//	Fix TNCPORT reconnect on Linux
//	Add SecureTelnet option to limit telnet outward connect to sysop mode sessions or Application Aliases
//	Add option to suppress sending call to application in Telnet HOST API
//	Add FT991A support to RigControl
//	Use background.jpg for Edit Config page
//	Send OK response to SCS Pactor commands starting with #
//	Resend ICOM PTT OFF command after 30 seconds
//	Add WXCall to APRS config
//	Fixes for AEAPactor
//	Allow PTTMUX to use real or com0com com ports
//	Fix monitoring with AGW Emulator
//	Derive approx position from packets on APRS ports with a valid 6 char location
//	Fix corruption of APRS message lists if the station table fills up.
//	Don't accept empty username or password on Relay sessions.
//	Fix occasional empty Nodes broadcasts
//	Add Digis to UZ7HO Port MH list
//	Add PERMITTEDAPPLS port param
//	Fix WK2K Session Record Reporting for Airmail and some Pactor Modes.
//	Fix handling AX/IP (proto 93) frames
//	Fix possible corruption sending APRS messages
//	Allow Telnet connections to be made using Connect command as well as Attach then Connect
//	Fix Cancel Sysop Signin
//	Save axip resolver info and restore on restart
//	Add Transparent mode to Telnet Server HOST API 
//	Fix Tracker driver if WL2KREPRRT is in main config section
//	SNMP InOctets count corrected to include all frames and encoding of zero values fixed.
//	Change IP Gateway to exclude handling bits of 44 Net sold to Amazon
//	Fix crash in Web terminal when processing very long lines

//  Version 6.0.22.1 August 2021

//	Fix bug in KAM TNCEMULATOR
//	Add WinRPR Driver (DED over TCP)
//	Fix handling of VARA config commands FM1200 and FM9600
//	Improve Web Termanal Line folding
//	Add StartTNC to WinRPR driver
//	Add support for VARA2750 Mode
//	Add support for VARA connects via a VARA Digipeater
//	Add digis to SCSTracker and WinRPR MHeard
//	Separate RIGCONTROL config from PORT config and add RigControl window
//	Fix crash when a Windows HID device doesn't have a product_string
//	Changes to VARA TNC connection and restart process
//	Trigger FALLBACKTORELAY if attempt to connect to all CMS servers fail.
//	Fix saving part lines in adif log and Winlink Session reporting
//	Add port specific CTEXT
//	Add FRMR monitoring to UZ7HO driver
//	Add audio input switching for IC7610
//	Include Rigcontrol Support for IC-F8101E
//	Process any response to KISS command 
//	Fix NODE ADD command
//	Add noUpdate flag to AXIP MAP
//	Fix clearing NOFALLBACK flag in Telnet Server
//	Allow connects to RMS Relay running on another host 
//	Allow use of Power setting in Rigcontol scan lines for Kenwood radios
//	Prevent problems caused by using "CMS" as a Node Alias
//	Include standard APRS Station pages in code
//	Fix VALIDCALLS processing in HF drivers
//	Send Netrom Link reports to Node Map
//	Add REALTELNET mode to Telnet Outward Connect
//	Fix using S (Stay) parameter on Telnet connects when using CMDPORT and C HOST
//	Add Default frequency to rigcontrol to set a freq/mode to return to after a connection
//	Fix long (> 60 seconds) scan intervals
//	Improved debugging of stuck semaphores
//	Fix potential securiby bug in BPQ Web server
//	Send Chat Updates to chatupdate.g8bpq.net port 81
//	Add ReportRelayTraffic to Telnet config to send WL2K traffic reports for connections to RELAY
//	Add experimental Mode reporting
//	Add SendTandRtoRelay param to SCS Pactor, ARDOP and VARA drivers to divert calls to CMS for -T and -R to RELAY
//	Add UPNP Support

//  Version 6.0.23.1 June 2022 

//	Add option to control which applcalls are enabled in VARA
//	Add support for rtl_udp to Rig Control
//	Fix Telnet Auto Conneect to Application when using TermTCP or Web Terminal
//	Allow setting css styles for Web Terminal
//	And Kill TNC and Kill and Restart TNC commands to Web Driver Windows
//	More flexible RigControl for split frequency operation, eg for QO100
//	Increase stack size for ProcessHTMLMessage	(.11)	
//	Fix HTML Content-Type on images (.12)
//	Add AIS and ADSB Support (.13)
//	Compress web pages (.14)
//	Change minidump routine and close after program error (.15)
//  Add RMS Relay SYNC Mode (.17)
//  Changes for compatibility with Winlink Hybrid
//	Add Rigcontrol CMD feature to Yaesu code (21)
//  More diagnostic code
//	Trap potential buffer overrun in ax/tcp code
//	Fix possible hang in UZ7HO driver if connect takes a long time to succeed or fail
//	Add FLRIG as backend for RigControl (.24)
//	Fix bug in compressing some management web pages
//	Fix bugs in AGW Emulator (.25)
//	Add more PTT_Sets_Freq options for split frequency working (.26)
//	Allow RIGCONTROL using Radio Number (Rnn) as well as Port (.26)
//	Fix Telnet negotiation and backspace processing (.29)
//	Fix VARA Mode change when scanning (.30)
//	Add Web Mgmt Log Display (.33)
//	Fix crash when connecting to RELAY when CMS=0 (.36)
//	Send OK to user for manual freq changes with hamlib or flrig
//	Fix Rigcontrol leaving port disabled when using an empty timeband
//	Fix processing of backspace in Telnet character processing (.40)
//	Increase max size of connect script
//	Fix HAMLIB Slave Thread control
//	Add processing of VARA mode responses and display of VARA Mode (41)
//	Fix crash when VARA session aborted on LinBPQ (43)
//	Fix handling port selector (2:call or p2 call) on SCS PTC packet ports (44)
//	Include APRS Map web page
//	Add Enable/Disable to KAMPACTOR scan control (use P0 or P1) (45)
//	Add Basic DRATS interface (46)
//	Fix MYCALLS on VARA (49)
//	Add FreeData driver (51)
//	Add additonal Rigcontrol options for QO100 (51)
//	Set Content-Type: application/pdf for pdf files downloaded via web interface (51)
//	Fix sending large compressed web messages (52)
//	Fix freq display when using flrig or hamlib backends to rigcontrol
//	Change VARA Driver to send ABORT when Session Time limit expires
//	Add Chat Log to Web Logs display
//	Fix possible buffer loss in RigControl
//	Allow hosts on local lan to be treated as secure
//	Improve validation of data sent to Winlink SessionAdd API call
//	Add support for FreeDATA modem.
//	Add GetLOC API Call
//	Change Leaflet link in aprs map.
//	Add Connect Log (64)
//	Fix crash when Resolve CMS Servers returns ipv6 addresses
//	Fix Reporting P4 sessions to Winlink (68)
//	Add support for FreeBSD (68)
//	Fix Rigcontrol PTCPORT (69)
//	Set TNC Emulator sessions as secure (72)
//	Fix not always detecting loss of FLRIG (73)
//	Add ? and * wildcards to NODES command (74)
//  Add Port RADIO config parameter (74)

//  Version 6.0.24.1 August 2024

//	Apply NODES command wildcard to alias as well a call (2)
//	Add STOPPORT/STARTPORT to VARA Driver (2)
//	Add bandwidth setting to FLRIG interface. (2)
//	Fix N VIA (3)
//	Fix NODE ADD and NODE DEL (4)
//	Improvements to FLRIG Rigcontrol backend (6, 7)
//	Fix UZ7HO Window Title Update
//	Reject L2 calls with a blank from call (8)
//	Update WinRPR Window header with BPQ Port Description (8)
//	Fix error in blank call code (9)
//	Change web buttons to white on black when pressed (10)
//	Fix Port CTEXT paclen on Tracker and WinRPR drivers (11)
//	Add RADIO PTT command for testing PTT (11)
//	Fix using APPLCALLs on SCSTracker RP call (12)
//	Add Rigcntol Web Page (13)
//	Fix scan bandwidth change with ARDOPOFDM (13)
//	Fix setting Min Pactor Level in SCSPactor (13)
//	Fix length of commands sent via CMD_TO_APPL flag (14)
//	Add filter by quality option to N display (15)
//	Fix VARA Mode reporting to WL2K (16)
//	Add FLRIG POWER and TUNE commands (18)
//	Fix crash when processing "C " without a call in UZ7HO, FLDIGI or MULTIPSK drivers (19)
//	FLDIGI improvements (19)
//	Fix hang at start if Telnet port Number > Number of Telnet Streams (20)
//	Fix processing C command if first port driver is SCSPACTROR (20)
//	Fix crash in UZ7HO driver if bad raw frame received (21)
//	Fix using FLARQ chat mode with FLDIGI ddriover (22)
//	Fix to KISSHF driver (23)
//	Fix for application buffer loss (24)
//	Add Web Sockets auto-refresh option for Webmail index page (25)
//	Fix FREEDATA driver for compatibility with FreeData TNC version 0.6.4-alpha.3 (25)
//	Add SmartID for bridged frames - Send ID only if packets sent recently (26)
//	Add option to save and restore received APRS messages (27)
//	Add mechanism to run a user program on certain events (27)
//	If BeacontoIS is zero don't Gate any of our messages received locally to APRS-IS (28)
//	Add Node Help command (28)
//	Add APRS Igate RXOnly option (29)
//	Fix RMC message handling with prefixes other than GP (29)
//	Add GPSD support for APRS (30)
//	Attempt to fix Tracker/WinRPR reconnect code (30)
//	Changes to FreeDATA - Don't use deamon and add txlevel and send text commands (31)
//	Fix interactive commands in tracker driver (33)
//  Fix SESSIONTIMELIMIT processing
//	Add STOPPORT/STARTPORT for UZ7HO driver
//	Fix processing of extended QtSM 'g' frame (36)
//	Allow setting just freq on Yaseu rigs (37)
//	Enable KISSHF driver on Linux (40)
//	Allow AISHOST and ADSBHOST to be a name as well as an address (41)
//	Fix Interlock of incoming UZ7HO connections (41)
//	Disable VARA Actions menu if not sysop (41)
//	Fix Port CTEXT on UZ7HO B C or D channels (42)
//	Fix repeated trigger of SessionTimeLimit (43)
//  Fix posible memory corruption in UpateMH (44)
//	Add PHG to APRS beacons (45)
//	Dont send DM to stations in exclude list(45)
//	Improvements to RMS Relay SYNC Mode (46)
//	Check L4 connects against EXCLUDE list (47)
//	Add vaidation of LOC in WL2K Session Reports (49)
//	Change gpsd support for compatibility with Share Gps (50)
//	Switch APRS Map to my Tiles (52)
//	Fix using ; in UNPROTO Mode messages (52)
//	Use sha1 code from https://www.packetizer.com/security/sha1/ instead of openssl (53)
//	Fix TNC Emulator Monitoring (53)
//	Fix attach and connect on Telnet port bug introduced in .55 (56)
//	Fix stopping WinRPR TNC and Start/Stop UZ7HO TNCX on Linux (57)
//	Fix stack size in beginthread for MAC (58)
//	Add NETROM over VARA (60)
//	Add Disconnect Script (64)
//	Add node commands to set UZ7HO modem mode and freq (64)
//	Trap empty NODECALL or NETROMCALL(65)
//	Trap NODES messages with empty From Call (65)
//	Add RigControl for SDRConsole (66)
//  Fix FLRig crash (66)
//	Fix VARA disconnect handling (67)
//	Support 64 ports (69)
//	Fix Node commands for setting UZ7HO Modem (70)
//	Fix processing SABM on an existing session (71)
//	Extend KISS Node command to send more than one parameter byte (72)
//	Add G7TAJ's code to record activity of HF ports for stats display (72)
//	Add option to send KISS command to TNC on startup (73)
//	Fix Bug in DED Emulator Monitor code (74)
//	Add Filters to DED Monitor code (75)
//	Detect loss of DED application (76)
//	Fix connects to Application Alias with UZ7HO Driver (76)
//	Fix Interlock of ports on same UZ7HO modem. (76)
//	Add extended Ports command (77)
//	Fix crash in Linbpq when stdout is redirected to /dev/tty? and stdin ia redirected (78)
//	Fix Web Terminal (80)
//	Trap ENCRYPTION message from VARA (81)
//	Fix processing of the Winlink API /account/exists response (82)
//	Fix sending CTEXT to L4 connects to Node when FULL_CTEXT is not set

//  Version 6.0.25.? 

//	Fix 64 bit compatibility problems in SCSTracker and UZ7HO drivers
//	Add Chat PACLEN config (5)
//	Fix NC to Application Call (6)
//	Fix INP3 L3RTT messages on Linux and correct RTT calculation (9)
//	Get Beacon config from config file on Windows (9)
//	fix processing DED TNC Emulator M command with space between M and params (10)
//	Fix sending UI frames on SCSPACTOR (11)
//	Dont allow ports that can't set digi'ed bit in callsigns to digipeat. (11)
//	Add SDRAngel rig control (11)
//	Add option to specify config and data directories on linbpq (12)
//	Allow zero resptime (send RR immediately) (13)
//	Make sure CMD bit is set on UI frames
//	Add setting Modem Flags in QtSM AGW mode
//	If FT847 om PTC Port send a "Cat On" command (17)
//	Fix some 63 port bugs in RigCOntrol (17)
//	Fix 63 port bug in Bridging (18)
//	Add FTDX10 Rigcontrol (19)
//	Fix 64 bit bug in displaying INP3 Messages (20)
//	Improve restart of WinRPR TNC on remote host (21)
//	Fix some Rigcontrol issues with empty timebands (22)
//	Fix 64 bit bug in processing INP3 Messages (22)
//	First pass at api (24)
//	Send OK in response to Rigcontrol CMD (24)
//	Disable CTS check in WriteComBlock (26) 
//	Improvments to reporting to M0LTE Map (26)
//	IPGateway fix from github user isavitsky (27)
//  Fix possible crash in SCSPactor PTCPORT code (29)
//	Add NodeAPI call sendLinks and remove get from other calls (32)
//	Improve validation of Web Beacon Config (33)
//	Support SNMP via host ip stack as well as IPGateway (34)
//	Switch APRS Map to OSM tile servers (36)
//	Fix potential buffer overflow in Telnet login (36)
//	Allow longer serial device names (37)
//	Fix ICF8101 Mode setting (37)
//	Kill link if we are getting repeated RR(F) after timeout 
//		(Indicating other station is seeing our RR(P) but not the resent I frame) (40)
//	Change default of SECURETELNET to 1 (41)
//	Add optional ATTACH time limit for ARDOP (42)
//	Fix buffer overflow risk in HTTP Terminal(42)
//	Fix KISSHF Interlock (43)
//	Support other than channel A on HFKISS (43)
//	Support additional port info reporting for M0LTE Map (44)
//	Allow interlocking of KISS and Session mode ports (eg ARDOP and VARA) (45)
//	Add ARDOP UI Packets to MH (45)
//	Add support for Qtsm Mgmt Interface (45)
//	NodeAPI improvements (46)
//	Add MQTT Interface (46)
//	Fix buffer leak in ARDOP code(46)
//	Fix possible crash if MQTT not in use (47)
//	Add optional ATTACH time limit for VARA (48)
//	API format fixes (48)
//	AGWAPI Add protection against accidental connects from a non-agw application (50)

#define CKernel

#include "Versions.h"

#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#include "time.h"
#include "stdio.h"
#include <fcntl.h>		

#include "compatbits.h"
#include "AsmStrucs.h"

#include "SHELLAPI.H"
#include "kernelresource.h"

#include <tlhelp32.h>
#include <Iphlpapi.h>
#include "BPQTermMDI.h"

#include "GetVersion.h"

#define DllImport	__declspec( dllimport )

#define CheckGuardZone() _CheckGuardZone(__FILE__, __LINE__)
void _CheckGuardZone(char * File, int Line);

#define	CHECKLOADED		  0
#define	SETAPPLFLAGS	  1
#define	SENDBPQFRAME	  2
#define	GETBPQFRAME	      3
#define	GETSTREAMSTATUS	  4
#define	CLEARSTREAMSTATUS 5
#define	BPQCONDIS		  6
#define	GETBUFFERSTATUS	  7
#define	GETCONNECTIONINFO 8
#define	BPQRETURN		  9  // GETCALLS
//#define	RAWTX			  10  //IE KISS TYPE DATA
#define	GETRAWFRAME		  11
#define	UPDATESWITCH	  12
#define	BPQALLOC		  13
//#define	SENDNETFRAME	  14
#define	GETTIME			  15

extern short NUMBEROFPORTS;
extern long PORTENTRYLEN;
extern long LINKTABLELEN;
extern struct PORTCONTROL * PORTTABLE;
extern void * FREE_Q;
extern UINT APPL_Q;				// Queue of frames for APRS Appl

extern TRANSPORTENTRY * L4TABLE;
extern UCHAR NEXTID;
extern DWORD MAXCIRCUITS;
extern DWORD L4DEFAULTWINDOW;
extern DWORD L4T1;
extern APPLCALLS APPLCALLTABLE[];
extern char * APPLS;

extern struct WL2KInfo * WL2KReports;

extern int NUMBEROFTNCPORTS;


void * VCOMExtInit(struct PORTCONTROL *  PortEntry);
void * AXIPExtInit(struct PORTCONTROL *  PortEntry);
void * SCSExtInit(struct PORTCONTROL *  PortEntry);
void * AEAExtInit(struct PORTCONTROL *  PortEntry);
void * KAMExtInit(struct PORTCONTROL *  PortEntry);
void * HALExtInit(struct PORTCONTROL *  PortEntry);
void * ETHERExtInit(struct PORTCONTROL *  PortEntry);
void * AGWExtInit(struct PORTCONTROL *  PortEntry);
void * WinmorExtInit(EXTPORTDATA * PortEntry);
void * TelnetExtInit(EXTPORTDATA * PortEntry);
//void * SoundModemExtInit(EXTPORTDATA * PortEntry);
void * TrackerExtInit(EXTPORTDATA * PortEntry);
void * TrackerMExtInit(EXTPORTDATA * PortEntry);
void * V4ExtInit(EXTPORTDATA * PortEntry);
void * UZ7HOExtInit(EXTPORTDATA * PortEntry);
void * MPSKExtInit(EXTPORTDATA * PortEntry);
void * FLDigiExtInit(EXTPORTDATA * PortEntry);
void * UIARQExtInit(EXTPORTDATA * PortEntry);
void * SerialExtInit(EXTPORTDATA * PortEntry);
void * ARDOPExtInit(EXTPORTDATA * PortEntry);
void * VARAExtInit(EXTPORTDATA * PortEntry);
void * KISSHFExtInit(EXTPORTDATA * PortEntry);
void * WinRPRExtInit(EXTPORTDATA * PortEntry);
void * HSMODEMExtInit(EXTPORTDATA * PortEntry);
void * FreeDataExtInit(EXTPORTDATA * PortEntry);
void * SIXPACKExtInit(EXTPORTDATA * PortEntry);

extern char * ConfigBuffer;	// Config Area
VOID REMOVENODE(dest_list * DEST);
DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall);
DllExport int ConvToAX25(unsigned char * incall,unsigned char * outcall);
VOID GetUIConfig();
VOID ADIFWriteFreqList();
void SaveAIS();
void initAIS();
void initADSB();

extern BOOL ADIFLogEnabled;

int CloseOnError = 0;

char UIClassName[]="UIMAINWINDOW";					// the main window class name

HWND UIhWnd;

extern char AUTOSAVE;
extern char AUTOSAVEMH;

extern char MYNODECALL;	// 10 chars,not null terminated

extern QCOUNT; 
extern BPQVECSTRUC BPQHOSTVECTOR[];
#define BPQHOSTSTREAMS 64
#define IPHOSTVECTOR BPQHOSTVECTOR[BPQHOSTSTREAMS + 3]

extern char * CONFIGFILENAME;

DllExport BPQVECSTRUC * BPQHOSTVECPTR;

extern int DATABASESTART;

extern struct ROUTE * NEIGHBOURS;
extern int  ROUTE_LEN;
extern int  MAXNEIGHBOURS;

extern struct DEST_LIST * DESTS;				// NODE LIST
extern int  DEST_LIST_LEN;
extern int  MAXDESTS;			// MAX NODES IN SYSTEM

extern struct _LINKTABLE * LINKS;
extern int	LINK_TABLE_LEN; 
extern int	MAXLINKS;


extern int BPQHOSTAPI();
extern int INITIALISEPORTS();
extern int TIMERINTERRUPT();
extern int MONDECODE();
extern int BPQMONOPTIONS();
extern char PWTEXT[];
extern char PWLen;

extern int FINDFREEDESTINATION();
extern int RAWTX();
extern int RELBUFF();
extern int SENDNETFRAME();
extern char MYCALL[];			// 7 chars, ax.25 format

extern HWND hIPResWnd;
extern BOOL IPMinimized;

extern int	NODESINPROGRESS;
extern VOID * CURRENTNODE;


BOOL Start();

VOID SaveWindowPos(int port);
VOID SaveAXIPWindowPos(int port);
VOID SetupRTFHddr();
DllExport VOID APIENTRY CreateNewTrayIcon();
int DoReceivedData(int Stream);
int	DoStateChange(int Stream);
int DoMonData(int Stream);
struct ConsoleInfo * CreateChildWindow(int Stream, BOOL DuringInit);
CloseHostSessions();
SaveHostSessions();
VOID SaveBPQ32Windows();
VOID CloseDriverWindow(int port);
VOID CheckWL2KReportTimer();
VOID SetApplPorts();
VOID WriteMiniDump();
VOID FindLostBuffers();
BOOL InitializeTNCEmulator();
VOID TNCTimer();
char * strlop(char * buf, char delim);

DllExport int APIENTRY Get_APPLMASK(int Stream);
DllExport int APIENTRY GetStreamPID(int Stream);
DllExport int APIENTRY GetApplFlags(int Stream);
DllExport int APIENTRY GetApplNum(int Stream);
DllExport BOOL APIENTRY GetAllocationState(int Stream);
DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count );
DllExport int APIENTRY RXCount(int Stream);
DllExport int APIENTRY TXCount(int Stream);
DllExport int APIENTRY MONCount(int Stream);
DllExport int APIENTRY GetCallsign(int stream, char * callsign);
DllExport VOID APIENTRY RelBuff(VOID * Msg);
void SaveMH();
void DRATSPoll();

#define C_Q_ADD(s, b) _C_Q_ADD(s, b, __FILE__, __LINE__);
int _C_Q_ADD(VOID *PQ, VOID *PBUFF, char * File, int Line);

VOID SetWindowTextSupport();
int WritetoConsoleSupport(char * buff);
VOID PMClose();
VOID MySetWindowText(HWND hWnd, char * Msg);
BOOL CreateMonitorWindow(char * MonSize);
VOID FormatTime3(char * Time, time_t cTime);

char EXCEPTMSG[80] = "";

char SIGNONMSG[128] = "";
char SESSIONHDDR[80] = "";
int SESSHDDRLEN = 0;

BOOL IncludesMail = FALSE;
BOOL IncludesChat = FALSE;		// Set if pgram is running - used for Web Page Index


char WL2KCall[10];
char WL2KLoc[7];

extern char LOCATOR[];			// Locator for Reporting - may be Maidenhead or LAT:LON
extern char MAPCOMMENT[];		// Locator for Reporting - may be Maidenhead or LAT:LON
extern char LOC[7];				// Maidenhead Locator for Reporting
extern char ReportDest[7];

extern UCHAR ConfigDirectory[260];

extern uint64_t timeLoadedMS;

VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Consoleprintf(const char * format, ...);

DllExport int APIENTRY CloseBPQ32();
DllExport char * APIENTRY GetLOC();
DllExport int APIENTRY SessionControl(int stream, int command, int param);

int DoRefreshWebMailIndex();

BOOL APIENTRY Init_IP();
BOOL APIENTRY Poll_IP();

BOOL APIENTRY Init_PM();
BOOL APIENTRY Poll_PM();

BOOL APIENTRY Init_APRS();
BOOL APIENTRY Poll_APRS();
VOID HTTPTimer();

BOOL APIENTRY Rig_Init();
BOOL APIENTRY Rig_Close();
BOOL Rig_Poll();

VOID IPClose();
VOID APRSClose();
VOID CloseTNCEmulator();

VOID Poll_AGW();
BOOL AGWAPIInit();
int AGWAPITerminate();

int * Flag = (int *)&Flag;			//	 for Dump Analysis
int MAJORVERSION=4;
int MINORVERSION=9;

struct SEM Semaphore = {0, 0, 0, 0};
struct SEM APISemaphore = {0, 0, 0, 0};
int SemHeldByAPI = 0;
int LastSemGets = 0;
UINT Sem_eax = 0;
UINT Sem_ebx = 0;
UINT Sem_ecx = 0;
UINT Sem_edx = 0;
UINT Sem_esi = 0;
UINT Sem_edi = 0;

void GetSemaphore(struct SEM * Semaphore, int ID);
void FreeSemaphore(struct SEM * Semaphore);

DllExport void * BPQHOSTAPIPTR = &BPQHOSTAPI;
//DllExport long  MONDECODEPTR = (long)&MONDECODE;

extern UCHAR BPQDirectory[];
extern UCHAR LogDirectory[];
extern UCHAR BPQProgramDirectory[];

static char BPQWinMsg[] = "BPQWindowMessage";

static char ClassName[] = "BPQMAINWINDOW";

HKEY REGTREE = HKEY_CURRENT_USER;
char REGTREETEXT[100] = "HKEY_CURRENT_USER";

UINT BPQMsg=0;

#define MAXLINELEN 120
#define MAXSCREENLEN 50

#define BGCOLOUR RGB(236,233,216)

HBRUSH bgBrush = NULL;

//int LINELEN=120;
//int SCREENLEN=50;

//char Screen[MAXLINELEN*MAXSCREENLEN]={0};

//int lineno=0;
//int col=0;

#define REPORTINTERVAL 15 * 549;	// Magic Ticks Per Minute for PC's nominal 100 ms timer 
int ReportTimer = 0;

HANDLE OpenConfigFile(char * file);

VOID SetupBPQDirectory();
VOID SendLocation();

//uintptr_t _beginthread(void(*start_address)(), unsigned stack_size, int arglist);

#define TRAY_ICON_ID	      1		    //		ID number for the Notify Icon
#define MY_TRAY_ICON_MESSAGE  WM_APP	//		the message ID sent to our window

NOTIFYICONDATA niData; 

int SetupConsoleWindow();

BOOL StartMinimized=FALSE;
BOOL MinimizetoTray=TRUE;

BOOL StatusMinimized = FALSE;
BOOL ConsoleMinimized = FALSE;

HMENU trayMenu=0;

HWND hConsWnd = NULL, hWndCons = NULL, hWndBG = NULL, ClientWnd = NULL,  FrameWnd = NULL, StatusWnd = NULL;

BOOL FrameMaximized = FALSE;

BOOL IGateEnabled = TRUE;
extern int ISDelayTimer;			// Time before trying to reopen APRS-IS link
extern int ISPort;

UINT * WINMORTraceQ = NULL;
UINT * SetWindowTextQ = NULL;

static RECT Rect = {100,100,400,400};	// Console Window Position
RECT FRect = {100,100,800,600};	// Frame 
static RECT StatusRect = {100,100,850,500};	// Status Window

DllExport int APIENTRY DumpSystem();
DllExport int APIENTRY SaveNodes ();
DllExport int APIENTRY ClearNodes ();
DllExport int APIENTRY SetupTrayIcon();

#define Q_REM(s) _Q_REM(s, __FILE__, __LINE__)

VOID * _Q_REM(VOID *Q, char * File, int Line);

UINT ReleaseBuffer(UINT *BUFF);


VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

DllExport int APIENTRY DeallocateStream(int stream);

int VECTORLENGTH = sizeof (struct _BPQVECSTRUC);

int FirstEntry = 1;
BOOL CloseLast = TRUE;			// If the user started BPQ32.exe, don't close it when other programs close
BOOL Closing = FALSE;			// Set if Close All called - prevents respawning bpq32.exe

BOOL BPQ32_EXE;					// Set if Process is running BPQ32.exe. Not initialised.
								// Used to Kill surplus BPQ32.exe processes

DWORD Our_PID;					// Our Process ID - local variable

void * InitDone = 0;
int FirstInitDone = 0;
int PerlReinit = 0;
UINT_PTR TimerHandle = 0;
UINT_PTR SessHandle = 0;

BOOL EventsEnabled = 0;

unsigned int TimerInst = 0xffffffff;

HANDLE hInstance = 0;

int AttachedProcesses = 0;
int AttachingProcess = 0;
HINSTANCE hIPModule = 0;
HINSTANCE hRigModule = 0;

BOOL ReconfigFlag = FALSE;
BOOL RigReconfigFlag = FALSE;
BOOL APRSReconfigFlag = FALSE;
BOOL CloseAllNeeded = FALSE;
BOOL NeedWebMailRefresh = FALSE;

int AttachedPIDList[100] = {0};

HWND hWndArray[100] = {0};
int PIDArray[100] = {0};
char PopupText[30][100] = {""};

// Next 3 should be uninitialised so they are local to each process

UCHAR	MCOM;
UCHAR	MTX;						// Top bit indicates use local time
uint64_t MMASK;
UCHAR	MUIONLY;

UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

char pgm[256];						// Uninitialised so per process

HANDLE Mutex;

BOOL PartLine = FALSE;
int pindex = 0;
DWORD * WritetoConsoleQ;


LARGE_INTEGER lpFrequency = {0};
LARGE_INTEGER lastRunTime;
LARGE_INTEGER currentTime; 

int ticksPerMillisec;
int interval;


VOID CALLBACK SetupTermSessions(HWND hwnd, UINT  uMsg, UINT  idEvent,  DWORD  dwTime);


TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
TIMERPROC lpSetupTermSessions = (TIMERPROC) SetupTermSessions;


BOOL ProcessConfig();
VOID FreeConfig();

DllExport int APIENTRY WritetoConsole(char * buff);

BOOLEAN CheckifBPQ32isLoaded();
BOOLEAN StartBPQ32();
DllExport VOID APIENTRY  Send_AX(VOID * Block, DWORD len, UCHAR Port);
BOOL LoadIPDriver();
BOOL Send_IP(VOID * Block, DWORD len);
VOID CheckforLostProcesses();
BOOL LoadRigDriver();
VOID SaveConfig();
VOID CreateRegBackup();
VOID ResolveUpdateThread();
VOID OpenReportingSockets();
DllExport VOID APIENTRY CloseAllPrograms();
DllExport BOOL APIENTRY SaveReg(char * KeyIn, HANDLE hFile);
int upnpClose();

BOOL IPActive = FALSE;
extern BOOL IPRequired;
BOOL PMActive = FALSE;
extern BOOL PMRequired;
BOOL RigRequired = TRUE;
BOOL RigActive = FALSE;
BOOL APRSActive = FALSE;
BOOL AGWActive = FALSE;
BOOL needAIS = FALSE;
int needADSB = 0;

extern int AGWPort;

Tell_Sessions();


typedef  int (WINAPI FAR *FARPROCX)();

FARPROCX CreateToolHelp32SnapShotPtr;
FARPROCX Process32Firstptr;
FARPROCX Process32Nextptr;

void LoadToolHelperRoutines()
{
	HINSTANCE ExtDriver=0;
	int err;
	char msg[100];

	ExtDriver=LoadLibrary("kernel32.dll");

	if (ExtDriver == NULL)
	{
		err=GetLastError();
		sprintf(msg,"BPQ32 Error loading kernel32.dll - Error code %d\n", err);
		OutputDebugString(msg);
		return;
	}

	CreateToolHelp32SnapShotPtr = (FARPROCX)GetProcAddress(ExtDriver,"CreateToolhelp32Snapshot");
	Process32Firstptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32First");
	Process32Nextptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32Next");
	
	if (CreateToolHelp32SnapShotPtr == 0)
	{
		err=GetLastError();
		sprintf(msg,"BPQ32 Error getting CreateToolhelp32Snapshot entry point - Error code %d\n", err);
		OutputDebugString(msg);
		return;
	}
}

BOOL GetProcess(int ProcessID, char * Program)
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  int p;

   if (CreateToolHelp32SnapShotPtr==0)
   {
	   return (TRUE);	// Routine not available
   }
  // Take a snapshot of all processes in the system.
  hProcessSnap = (HANDLE)CreateToolHelp32SnapShotPtr(TH32CS_SNAPPROCESS, 0);
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    OutputDebugString( "CreateToolhelp32Snapshot (of processes) Failed\n" );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32Firstptr( hProcessSnap, &pe32 ) )
  {
    OutputDebugString( "Process32First Failed\n" );  // Show cause of failure
    CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
    return( FALSE );
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  do
  {
	if (ProcessID==pe32.th32ProcessID)
	{
		  // if running on 98, program contains the full path - remove it

		for (p = (int)strlen(pe32.szExeFile); p >= 0; p--)
		{
			if (pe32.szExeFile[p]=='\\') 
			{
				break;
			}
		}
		p++;		
		  
		sprintf(Program,"%s", &pe32.szExeFile[p]);
		CloseHandle( hProcessSnap );
		return( TRUE );
	}

  } while( Process32Nextptr( hProcessSnap, &pe32 ) );


  sprintf(Program,"PID %d Not Found", ProcessID);
  CloseHandle( hProcessSnap );
  return(FALSE);
}

BOOL IsProcess(int ProcessID)
{
	// Check that Process exists

  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;

  if (CreateToolHelp32SnapShotPtr==0) return (TRUE);  // Routine not available

  hProcessSnap = (HANDLE)CreateToolHelp32SnapShotPtr(TH32CS_SNAPPROCESS, 0);

  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    OutputDebugString( "CreateToolhelp32Snapshot (of processes) Failed\n" );
    return(TRUE);		// Don't know, so assume ok
  }

  pe32.dwSize = sizeof( PROCESSENTRY32 );

   if( !Process32Firstptr( hProcessSnap, &pe32 ) )
  {
    OutputDebugString( "Process32First Failed\n" );  // Show cause of failure
    CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
    return(TRUE);		// Don't know, so assume ok
   }

   do
   {
	  if (ProcessID==pe32.th32ProcessID)
	  {
		  CloseHandle( hProcessSnap );
		  return( TRUE );
	  }

  } while( Process32Nextptr( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return(FALSE);
}

#include "DbgHelp.h"

VOID MonitorThread(int x)
{
	// Thread to detect killed processes. Runs in process owning timer.

	// Obviously can't detect loss of timer owning thread!

	do 
	{
		if (Semaphore.Gets == LastSemGets && Semaphore.Flag)
		{
			// It is stuck - try to release

				Debugprintf ("Semaphore locked - Process ID = %d, Held By %d",
					Semaphore.SemProcessID, SemHeldByAPI);
			
			// Write a minidump

			WriteMiniDump();
			
			Semaphore.Flag = 0;
		}

		LastSemGets = Semaphore.Gets;

		Sleep(30000);	
		CheckforLostProcesses();

	} while (TRUE);
}

VOID CheckforLostProcesses()
{
	UCHAR buff[100];
	char Log[80];
	int i, n, ProcessID;

	for (n=0; n < AttachedProcesses; n++)
	{
		ProcessID=AttachedPIDList[n];
			
		if (!IsProcess(ProcessID))
		{
			// Process has died - Treat as a detach

			sprintf(Log,"BPQ32 Process %d Died\n", ProcessID);
			OutputDebugString(Log);

			// Remove Tray Icon Entry

			for( i = 0; i < 100; ++i )
			{
				if (PIDArray[i] == ProcessID)
				{
					hWndArray[i] = 0;
					sprintf(Log,"BPQ32 Removing Tray Item %s\n", PopupText[i]);
					OutputDebugString(Log);
					DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
				}
			}

			// If process had the semaphore, release it

			if (Semaphore.Flag == 1 && ProcessID == Semaphore.SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 

				Semaphore.Flag = 0;
				SemHeldByAPI = 0;
			}

			for (i=1;i<65;i++)
			{
				if (BPQHOSTVECTOR[i-1].STREAMOWNER == AttachedPIDList[n])
				{
					DeallocateStream(i);
				}
			}
				
			if (TimerInst == ProcessID)
			{
				KillTimer(NULL,TimerHandle);
				TimerHandle=0;
				TimerInst=0xffffffff;
//				Tell_Sessions();
				OutputDebugString("BPQ32 Process was running timer \n");
			
				if (MinimizetoTray)
					Shell_NotifyIcon(NIM_DELETE,&niData);

				
			}
				
			//	Remove this entry from PID List

			for (i=n; i< AttachedProcesses; i++)
			{
				AttachedPIDList[i]=AttachedPIDList[i+1];
			}
			AttachedProcesses--;

			sprintf(buff,"BPQ32 Lost Process - %d Process(es) Attached\n", AttachedProcesses);
			OutputDebugString(buff);
		}
	}
}
VOID MonitorTimerThread(int x)
{	
	// Thread to detect killed timer process. Runs in all other BPQ32 processes.

	do {

		Sleep(60000);

		if (TimerInst != 0xffffffff && !IsProcess(TimerInst))
		{
			// Timer owning Process has died - Force a new timer to be created
			//	New timer thread will detect lost process and tidy up
		
			Debugprintf("BPQ32 Process %d with Timer died", TimerInst);

			// If process was holding the semaphore, release it

			if (Semaphore.Flag == 1 && TimerInst == Semaphore.SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 
				Semaphore.Flag = 0;
				SemHeldByAPI = 0;
			}

//			KillTimer(NULL,TimerHandle);
//			TimerHandle=0;
//			TimerInst=0xffffffff;
//			Tell_Sessions();

			CheckforLostProcesses();		// Normally only done in timer thread, which is now dead

			// Timer can only run in BPQ32.exe

			TimerInst=0xffffffff;			// So we dont keep doing it
			TimerHandle = 0;				// So new process attaches

			if (Closing == FALSE && AttachingProcess == FALSE)
			{
				OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
				StartBPQ32();
			}

//			if (MinimizetoTray)
//				Shell_NotifyIcon(NIM_DELETE,&niData);
		}
	
	} while (TRUE);
}

VOID WritetoTraceSupport(struct TNCINFO * TNC, char * Msg, int Len);

VOID TimerProcX();

VOID CALLBACK TimerProc(
	HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime)	// current system time	
{
 	KillTimer(NULL,TimerHandle);
	TimerProcX();
	TimerHandle = SetTimer(NULL,0,100,lpTimerFunc);
}
VOID TimerProcX()
{
	struct _EXCEPTION_POINTERS exinfo;

	//
	//	Get semaphore before proceeeding
	//

	GetSemaphore(&Semaphore, 2);

	// Get time since last run

	QueryPerformanceCounter(&currentTime);

	interval = (int)(currentTime.QuadPart - lastRunTime.QuadPart) / ticksPerMillisec;
	lastRunTime.QuadPart = currentTime.QuadPart;

	//Debugprintf("%d", interval);

	// Process WINMORTraceQ

	while (WINMORTraceQ)
	{
		UINT * Buffer = Q_REM(&WINMORTraceQ);
		struct TNCINFO * TNC = (struct TNCINFO * )Buffer[1];
		int Len = Buffer[2];
		char * Msg = (char *)&Buffer[3];

		WritetoTraceSupport(TNC, Msg, Len);
		RelBuff(Buffer);
	}

	if (SetWindowTextQ)
		SetWindowTextSupport();

	while (WritetoConsoleQ)
	{
		UINT * Buffer = Q_REM(&WritetoConsoleQ);
		WritetoConsoleSupport((char *)&Buffer[2]);
		RelBuff(Buffer);
	}

	strcpy(EXCEPTMSG, "Timer ReconfigProcessing");
	
	__try 
	{

	if (trayMenu == NULL)
		SetupTrayIcon();

	// See if reconfigure requested

	if (CloseAllNeeded)
	{
		CloseAllNeeded = FALSE;
		CloseAllPrograms();
	}

	if (ReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			int i;
			BPQVECSTRUC * HOSTVEC;
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
			WSADATA       WsaData;            // receives data from WSAStartup
			RECT cRect;

			ReconfigFlag = FALSE;

			SetupBPQDirectory();

			WritetoConsole("Reconfiguring ...\n\n");
			OutputDebugString("BPQ32 Reconfiguring ...\n");	

			GetWindowRect(FrameWnd, &FRect);

			SaveWindowPos(70);		// Rigcontrol

			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR)
					{
						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						CloseDriverWindow(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}
				}
				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);
				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}

			IPClose();
			PMClose();
			APRSClose();
			Rig_Close();
			CloseTNCEmulator();
			if (AGWActive)	
				AGWAPITerminate();

			WSACleanup();

			WL2KReports = NULL;

			Sleep(2000);

			WSAStartup(MAKEWORD(2, 0), &WsaData);

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);
	
			Start();

			INITIALISEPORTS();			// Restart Ports

			SetApplPorts();

			FreeConfig();

			for (i=1; i<68; i++)			// Include Telnet, APRS and IP Vec
			{
				HOSTVEC=&BPQHOSTVECTOR[i-1];

				HOSTVEC->HOSTTRACEQ=0;			// Clear header (pool has been reinitialized

				if (HOSTVEC->HOSTSESSION !=0)
				{
					// Had a connection
					
					HOSTVEC->HOSTSESSION=0;
					HOSTVEC->HOSTFLAGS |=3;	// Disconnected
					
					PostMessage(HOSTVEC->HOSTHANDLE, BPQMsg, i, 4);
				}
			}

			// Free the APRS Appl Q

			APPL_Q = 0;

			OpenReportingSockets();
		
			WritetoConsole("\n\nReconfiguration Complete\n");

			if (IPRequired)	IPActive = Init_IP();
			if (PMRequired)	PMActive = Init_PM();

			APRSActive = Init_APRS();

			if (ISPort == 0)
				IGateEnabled = 0;

			CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);

			GetClientRect(hConsWnd, &cRect); 
			MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);
			if (APRSActive)
				MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
			else
			{
				ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
				MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
			}
			InvalidateRect(hConsWnd, NULL, TRUE);

			RigActive = Rig_Init();
			
			if (NUMBEROFTNCPORTS)
			{
				FreeSemaphore(&Semaphore);
				InitializeTNCEmulator();
				GetSemaphore(&Semaphore, 0);
			}

			FreeSemaphore(&Semaphore);
			AGWActive = AGWAPIInit();
			GetSemaphore(&Semaphore, 0);
		
			OutputDebugString("BPQ32 Reconfiguration Complete\n");	
		}
	}


	if (RigReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			RigReconfigFlag = FALSE;
			CloseDriverWindow(70);
			Rig_Close();
			Sleep(6000);		// Allow any CATPTT, HAMLIB and FLRIG threads to close
			RigActive = Rig_Init();
			
			WritetoConsole("Rigcontrol Reconfiguration Complete\n");	
		}
	}

	if (APRSReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			APRSReconfigFlag = FALSE;
			APRSClose();				
			APRSActive = Init_APRS();
			
			WritetoConsole("APRS Reconfiguration Complete\n");	
		}
	}

	}
	#include "StdExcept.c"

	if (Semaphore.Flag && Semaphore.SemProcessID == GetCurrentProcessId())
		FreeSemaphore(&Semaphore);

	}

	strcpy(EXCEPTMSG, "Timer Processing");

	__try 
	{
		if (IPActive) Poll_IP();
		if (PMActive) Poll_PM();
		if (RigActive) Rig_Poll();

		if (NeedWebMailRefresh)
			DoRefreshWebMailIndex();

		CheckGuardZone();

		if (APRSActive)
		{
			Poll_APRS();
			CheckGuardZone();
		}

	 	CheckWL2KReportTimer();

		CheckGuardZone();
		
		TIMERINTERRUPT();

		CheckGuardZone();

		FreeSemaphore(&Semaphore);			// SendLocation needs to get the semaphore

		if (NUMBEROFTNCPORTS)
			TNCTimer();

		if (AGWActive)
			Poll_AGW();

		DRATSPoll();

		CheckGuardZone();

		strcpy(EXCEPTMSG, "HTTP Timer Processing");

		HTTPTimer();

		CheckGuardZone();

		strcpy(EXCEPTMSG, "WL2K Report Timer Processing");

		if (ReportTimer)
		{		
			ReportTimer--;
	
			if (ReportTimer == 0)
			{
				ReportTimer = REPORTINTERVAL;
				SendLocation();
			}
		}
	}
	
	#include "StdExcept.c"

	if (Semaphore.Flag && Semaphore.SemProcessID == GetCurrentProcessId())
		FreeSemaphore(&Semaphore);

	}

	CheckGuardZone();

	return;
}

HANDLE NPHandle;

int (WINAPI FAR *GetModuleFileNameExPtr)() = NULL;
int (WINAPI FAR *EnumProcessesPtr)() = NULL;

FirstInit()
{
    WSADATA       WsaData;            // receives data from WSAStartup
	HINSTANCE ExtDriver=0;
	RECT cRect;


	// First Time Ports and Timer init

	// Moved from DLLINIT to sort out perl problem, and meet MS Guidelines on minimising DLLMain 

	// Call wsastartup - most systems need winsock, and duplicate statups could be a problem

    WSAStartup(MAKEWORD(2, 0), &WsaData);

	// Load Psapi.dll if possible

	ExtDriver=LoadLibrary("Psapi.dll");

	SetupTrayIcon();

	if (ExtDriver)
	{
		GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");
		EnumProcessesPtr = (FARPROCX)GetProcAddress(ExtDriver,"EnumProcesses");
	}

	timeLoadedMS = GetTickCount();
	
	INITIALISEPORTS();

	OpenReportingSockets();

 	WritetoConsole("\n");
 	WritetoConsole("Port Initialisation Complete\n");

	if (IPRequired)	IPActive = Init_IP();
	if (PMRequired)	PMActive = Init_PM();

	APRSActive = Init_APRS();

	if (APRSActive)
	{
		hWndBG = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0,0,40,546, hConsWnd, NULL, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "Enable IGate", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
			8,0,90,24, hConsWnd, (HMENU)-1, hInstance, NULL);
		
		CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_TABSTOP,
			95,1,18,24, hConsWnd, (HMENU)IDC_ENIGATE, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "IGate State - Disconnected",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 125, 0, 195, 24, hConsWnd, (HMENU)IGATESTATE, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "IGATE Stats - Msgs 0   Local Stns 0",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 320, 0, 240, 24, hConsWnd, (HMENU)IGATESTATS, hInstance, NULL);

		CreateWindowEx(0,  "STATIC", "GPS Off",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 560, 0, 80, 24, hConsWnd, (HMENU)IDC_GPS, hInstance, NULL);
	}

	if (ISPort == 0)
		IGateEnabled = 0;

	CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);
	
	GetClientRect(hConsWnd, &cRect); 
	MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);
	if (APRSActive)
		MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
	else
	{
		ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
		MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
	}
	InvalidateRect(hConsWnd, NULL, TRUE);

	RigActive = Rig_Init();

	_beginthread(MonitorThread,0,0);
	
	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
	TimerInst=GetCurrentProcessId();
	SessHandle = SetTimer(NULL, 0, 5000, lpSetupTermSessions);

	// If ARIF reporting is enabled write a Trimode Like ini for RMS Analyser

	if (ADIFLogEnabled)
		ADIFWriteFreqList();

	OutputDebugString("BPQ32 Port Initialisation Complete\n");

	if (needAIS)
		initAIS();

	if (needADSB)
		initADSB();

	return 0;
}

Check_Timer()
{
	if (Closing)
		return 0;

	if (Semaphore.Flag)
		return 0;

	if (InitDone == (void *)-1)
	{
		GetSemaphore(&Semaphore, 3);
		Sleep(15000);
		FreeSemaphore(&Semaphore);
		exit (0);
	}

	if (FirstInitDone == 0)
	{
		GetSemaphore(&Semaphore, 3);

		if (_stricmp(pgm, "bpq32.exe") == 0)
		{
			FirstInit();
			FreeSemaphore(&Semaphore);
			if (NUMBEROFTNCPORTS)
				InitializeTNCEmulator();

			AGWActive = AGWAPIInit();
			FirstInitDone=1;					// Only init in BPQ32.exe
			return 0;
		}
		else
		{
			FreeSemaphore(&Semaphore);
			return 0;
		}
	}

	if (TimerHandle == 0 && FirstInitDone == 1)
	{
	    WSADATA       WsaData;            // receives data from WSAStartup
		HINSTANCE ExtDriver=0;
		RECT cRect;

		// Only attach timer to bpq32.exe

		if (_stricmp(pgm, "bpq32.exe") != 0)
		{
			return 0;
		}

		GetSemaphore(&Semaphore, 3);
		OutputDebugString("BPQ32 Reinitialising External Ports and Attaching Timer\n");

		if (!ProcessConfig())
		{
			ShowWindow(hConsWnd, SW_RESTORE);
			SendMessage(hConsWnd, WM_PAINT, 0, 0);
			SetForegroundWindow(hConsWnd);

			InitDone = (void *)-1;
			FreeSemaphore(&Semaphore);

			MessageBox(NULL,"Configuration File Error","BPQ32",MB_ICONSTOP);

			exit (0);
		}

		GetVersionInfo("bpq32.dll");

		SetupConsoleWindow();

		Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
		Consoleprintf(VerCopyright);
		Consoleprintf("Reinitialising...");
 
		SetupBPQDirectory();

		Sleep(1000);			// Allow time for sockets to close	

		WSAStartup(MAKEWORD(2, 0), &WsaData);

		// Load Psapi.dll if possible

		ExtDriver = LoadLibrary("Psapi.dll");

		SetupTrayIcon();

		if (ExtDriver)
		{
			GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");
			EnumProcessesPtr = (FARPROCX)GetProcAddress(ExtDriver,"EnumProcesses");
		}
			
		Start();
	
		INITIALISEPORTS();

		OpenReportingSockets();

		NODESINPROGRESS = 0;
		CURRENTNODE = 0;

		SetApplPorts();

		WritetoConsole("\n\nPort Reinitialisation Complete\n");

		BPQMsg = RegisterWindowMessage(BPQWinMsg);

		CreateMutex(NULL,TRUE,"BPQLOCKMUTEX");

//		NPHandle=CreateNamedPipe("\\\\.\\pipe\\BPQ32pipe",
//					PIPE_ACCESS_DUPLEX,0,64,4096,4096,1000,NULL);

		if (IPRequired)	IPActive = Init_IP();
		if (PMRequired)	PMActive = Init_PM();

		RigActive = Rig_Init();	
		APRSActive = Init_APRS();

		if (ISPort == 0)
			IGateEnabled = 0;

		CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);
	
		GetClientRect(hConsWnd, &cRect); 
		MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);

		if (APRSActive)
			MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
		else
		{
			ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
			MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
		}
		InvalidateRect(hConsWnd, NULL, TRUE);

		FreeConfig();

		_beginthread(MonitorThread,0,0);

		ReportTimer = 0;

		OpenReportingSockets();

		FreeSemaphore(&Semaphore);

		if (NUMBEROFTNCPORTS)
			InitializeTNCEmulator();

		AGWActive = AGWAPIInit();

		if (StartMinimized)
			if (MinimizetoTray)
				ShowWindow(FrameWnd, SW_HIDE);
			else
				ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
		else
			ShowWindow(FrameWnd, SW_RESTORE);

		TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
		TimerInst=GetCurrentProcessId();
		SessHandle = SetTimer(NULL, 0, 5000, lpSetupTermSessions);

		return (1);
	}

	return (0);
}

DllExport INT APIENTRY CheckTimer()
{
	return Check_Timer();
}

Tell_Sessions()
{
	//
	//	Post a message to all listening sessions, so they call the 
	//	API, and cause a new timer to be allocated
	//
	HWND hWnd;
	int i;

	for (i=1;i<65;i++)
	{
		if (BPQHOSTVECTOR[i-1].HOSTFLAGS & 0x80)
		{
			hWnd = BPQHOSTVECTOR[i-1].HOSTHANDLE;
			PostMessage(hWnd, BPQMsg,i, 1);
			PostMessage(hWnd, BPQMsg,i, 2);
		}
	}
	return (0);
}

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	DWORD n;
	char buf[350];

	int i;
	unsigned int ProcessID;

	OSVERSIONINFO osvi;

	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);


	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
			  
		if (sizeof(HDLCDATA) > PORTENTRYLEN + 200)	// 200 bytes of Hardwaredata
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"BPQ32 Too much HDLC data - Recompile","BPQ32", MB_OK);
			return 0;
		}
			  
		if (sizeof(struct KISSINFO) > PORTENTRYLEN + 200)	// 200 bytes of Hardwaredata
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"BPQ32 Too much KISS data - Recompile","BPQ32", MB_OK);
			return 0;
		}

		if (sizeof(struct _EXTPORTDATA) > PORTENTRYLEN + 200)	// 200 bytes of Hardwaredata
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"BPQ32 Too much _EXTPORTDATA data - Recompile","BPQ32", MB_OK);
			return 0;
		}
		  
		if (sizeof(LINKTABLE) != LINK_TABLE_LEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"L2 LINK Table .c and .asm mismatch - fix and rebuild","BPQ32", MB_OK);
			return 0;
		}
		if (sizeof(struct ROUTE) != ROUTE_LEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"ROUTE Table .c and .asm mismatch - fix and rebuild", "BPQ32", MB_OK);
			return 0;
		}
	
		if (sizeof(struct DEST_LIST) != DEST_LIST_LEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"NODES Table .c and .asm mismatch - fix and rebuild", "BPQ32", MB_OK);
			return 0;
		}
	
		GetSemaphore(&Semaphore, 4);

		BPQHOSTVECPTR = &BPQHOSTVECTOR[0];
		
		LoadToolHelperRoutines();

		Our_PID = GetCurrentProcessId();

		QueryPerformanceFrequency(&lpFrequency);

		ticksPerMillisec = (int)lpFrequency.QuadPart / 1000;

		lastRunTime.QuadPart = lpFrequency.QuadPart;

		GetProcess(Our_PID, pgm);

		if (_stricmp(pgm, "regsvr32.exe") == 0 || _stricmp(pgm, "bpqcontrol.exe") == 0)
		{
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore(&Semaphore);
			return 1;
		}

		if (_stricmp(pgm,"BPQ32.exe") == 0)
			BPQ32_EXE = TRUE;

		if (_stricmp(pgm,"BPQMailChat.exe") == 0)
			IncludesMail = TRUE;

		if (_stricmp(pgm,"BPQMail.exe") == 0)
			IncludesMail = TRUE;

		if (_stricmp(pgm,"BPQChat.exe") == 0)
			IncludesChat = TRUE;

		if (FirstEntry)				// If loaded by BPQ32.exe, dont close it at end
		{
			FirstEntry = 0;
			if (BPQ32_EXE)
				CloseLast = FALSE;
		}
		else
		{
			if (BPQ32_EXE && AttachingProcess == 0)
			{
				AttachedProcesses++;			// We will get a detach
				FreeSemaphore(&Semaphore);
				MessageBox(NULL,"BPQ32.exe is already running\r\n\r\nIt should only be run once", "BPQ32", MB_OK);
				return 0;
			}
		}

		if (_stricmp(pgm,"BPQTelnetServer.exe") == 0)
		{
			MessageBox(NULL,"BPQTelnetServer is no longer supported\r\n\r\nUse the TelnetServer in BPQ32.dll", "BPQ32", MB_OK);
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore(&Semaphore);
			return 0;
		}

		if (_stricmp(pgm,"BPQUIUtil.exe") == 0)
		{
			MessageBox(NULL,"BPQUIUtil is now part of BPQ32.dll\r\nBPQUIUtil.exe cannot be run\r\n", "BPQ32", MB_OK);
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore(&Semaphore);
			return 0;
		}

		if (_stricmp(pgm,"BPQMailChat.exe") == 0)
		{
			MessageBox(NULL,"BPQMailChat is obsolete. Run BPQMail.exe and/or BPQChat.exe instead", "BPQ32", MB_OK);
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore(&Semaphore);
			return 0;
		}
		AuthorisedProgram = TRUE;

		if (InitDone == 0)
		{
//			#pragma warning(push)
//			#pragma warning(disable : 4996)

//			if (_winver < 0x0600)
//			#pragma warning(pop)
//			{
//				// Below Vista
//
//				REGTREE = HKEY_LOCAL_MACHINE;
//				strcpy(REGTREETEXT, "HKEY_LOCAL_MACHINE");
//			}

			hInstance=hInst;

			Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

			if (Mutex != NULL)
			{
				OutputDebugString("Another BPQ32.dll is loaded\n");
				i=MessageBox(NULL,"BPQ32 DLL already loaded from another directory\nIf you REALLY want this, hit OK, else hit Cancel","BPQ32",MB_OKCANCEL);
				FreeSemaphore(&Semaphore);

				if (i != IDOK) return (0);

				CloseHandle(Mutex);
			}

			if (!BPQ32_EXE)
			{
				if (CheckifBPQ32isLoaded() == FALSE)		// Start BPQ32.exe if needed
				{
					// Wasn't Loaded, so we have started it, and should let it init system

					goto SkipInit;		
				}
			}

			GetVersionInfo("bpq32.dll");

			sprintf (SIGNONMSG, "G8BPQ AX25 Packet Switch System Version %s %s\r\n%s\r\n",
				TextVerstring, Datestring, VerCopyright);
				 
			SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for Win32 (", TextVerstring);

			SetupConsoleWindow();
			SetupBPQDirectory();
		
			if (!ProcessConfig())
			{
				StartMinimized = FALSE;
				MinimizetoTray = FALSE;
				ShowWindow(FrameWnd, SW_MAXIMIZE);
				ShowWindow(hConsWnd, SW_MAXIMIZE);
				ShowWindow(StatusWnd, SW_HIDE);

				SendMessage(hConsWnd, WM_PAINT, 0, 0);
				SetForegroundWindow(hConsWnd);

				InitDone = (void *)-1;
				FreeSemaphore(&Semaphore);

				MessageBox(NULL,"Configuration File Error\r\nProgram will close in 15 seconds","BPQ32",MB_ICONSTOP);

				return (0);
			}

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);

			if (Start() !=0)
			{
				Sleep(3000);
				FreeSemaphore(&Semaphore);
				return (0);
			}
			else
			{
				SetApplPorts();

				GetUIConfig();

				InitDone = &InitDone;
				BPQMsg = RegisterWindowMessage(BPQWinMsg);
//				TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
//				TimerInst=GetCurrentProcessId();

/*				Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

				if (Mutex != NULL)
				{
					OutputDebugString("Another BPQ32.dll is loaded\n");
					MessageBox(NULL,"BPQ32 DLL already loaded from another directory","BPQ32",MB_ICONSTOP);
					FreeSemaphore(&Semaphore);
					return (0);
				}

*/
				Mutex=CreateMutex(NULL,TRUE,"BPQLOCKMUTEX");

//				CreatePipe(&H1,&H2,NULL,1000);
  
//				GetLastError();

//				NPHandle=CreateNamedPipe("\\\\.\\pipe\\BPQ32pipe",
//					PIPE_ACCESS_DUPLEX,0,64,4096,4096,1000,NULL);
				
//				GetLastError();

/*
				//
				//	Read SYSOP password
				//

				if (PWTEXT[0] == 0)
				{
					handle = OpenConfigFile("PASSWORD.BPQ");

					if (handle == INVALID_HANDLE_VALUE)
					{
						WritetoConsole("Can't open PASSWORD.BPQ\n");
						PWLen=0;
						PWTEXT[0]=0;
					}
					else
					{
						ReadFile(handle,PWTEXT,78,&n,NULL); 
						CloseHandle(handle);
					}
				}
*/			
				for (i=0;PWTEXT[i] > 0x20;i++); //Scan for cr or null 
				PWLen=i;
				
			}
		}
		else
		{
			if (InitDone !=  &InitDone)
			{
				MessageBox(NULL,"BPQ32 DLL already loaded at another address","BPQ32",MB_ICONSTOP);
				FreeSemaphore(&Semaphore);
				return (0);
			}
		}
			
		// Run timer monitor thread in all processes - it is possible for the TImer thread not to be the first thread
SkipInit:

		_beginthread(MonitorTimerThread,0,0);

		FreeSemaphore(&Semaphore);

		AttachedPIDList[AttachedProcesses++] = GetCurrentProcessId();

		if (_stricmp(pgm,"bpq32.exe") == 0 &&  AttachingProcess == 1) AttachingProcess = 0;

		GetProcess(GetCurrentProcessId(),pgm);
		n=sprintf(buf,"BPQ32 DLL Attach complete - Program %s - %d Process(es) Attached\n",pgm,AttachedProcesses);
		OutputDebugString(buf);

		// Set up local variables
		
		MCOM=1;
		MTX=1;
		MMASK=0xffffffffffffffff;

//		if (StartMinimized)
//			if (MinimizetoTray)
//				ShowWindow(FrameWnd, SW_HIDE);
//			else
//				ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
//		else
//			ShowWindow(FrameWnd, SW_RESTORE);

		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
		
		return 1;
    
	case DLL_PROCESS_DETACH:
		
		if (_stricmp(pgm,"BPQMailChat.exe") == 0)
			IncludesMail = FALSE;

		if (_stricmp(pgm,"BPQChat.exe") == 0)
			IncludesChat = FALSE;

		ProcessID=GetCurrentProcessId();

		Debugprintf("BPQ32 Process %d Detaching", ProcessID); 
		
		// Release any streams that the app has failed to release

		for (i=1;i<65;i++)
		{
			if (BPQHOSTVECTOR[i-1].STREAMOWNER == ProcessID)
			{
				// If connected, disconnect

				SessionControl(i, 2, 0);
				DeallocateStream(i);
			}
		}

		// Remove any Tray Icon Entries

		for( i = 0; i < 100; ++i )
		{
			if (PIDArray[i] == ProcessID)
			{
				char Log[80];
				hWndArray[i] = 0;
				sprintf(Log,"BPQ32 Removing Tray Item %s\n", PopupText[i]);
				OutputDebugString(Log);
				DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
			}
		}

		if (Mutex) CloseHandle(Mutex);

		//	Remove our entry from PID List

		for (i=0;  i< AttachedProcesses; i++)
			if (AttachedPIDList[i] == ProcessID)
				break;

		for (; i< AttachedProcesses; i++)
		{
			AttachedPIDList[i]=AttachedPIDList[i+1];
		}

		AttachedProcesses--;

		if (TimerInst == ProcessID)
		{
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	
			OutputDebugString("BPQ32 Process with Timer closing\n");

			// Call Port Close Routines
			
			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR && PORTVEC->DLLhandle == NULL) // Don't call if real .dll - it's not there!
					{
						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}	
				}

				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);

				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}


			IPClose();
			PMClose();
			APRSClose();
			Rig_Close();
			CloseTNCEmulator();
			if (AGWActive)
				AGWAPITerminate();

			upnpClose();

			WSACleanup();
			WSAGetLastError();

			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			if (hConsWnd) DestroyWindow(hConsWnd);

			KillTimer(NULL,TimerHandle);
			TimerHandle=0;
			TimerInst=0xffffffff;

			if (AttachedProcesses && Closing == FALSE && AttachingProcess == 0)		// Other processes 
			{
				OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
				StartBPQ32();
			}
		}
		else
		{
			// Not Timer Process

			if (AttachedProcesses == 1 && CloseLast)		// Only bpq32.exe left
			{
				Debugprintf("Only BPQ32.exe running - close it");
				CloseAllNeeded = TRUE;
			}
		}

		if (AttachedProcesses < 2)
		{
			if (AUTOSAVE == 1)
				SaveNodes();
			if (AUTOSAVEMH)
				SaveMH();

			if (needAIS)
				SaveAIS();
		}
		if (AttachedProcesses == 0)
		{
			Closing  = TRUE;
			KillTimer(NULL,TimerHandle);
						
			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			// Unload External Drivers

			{
				PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
				
				for (i=0;i<NUMBEROFPORTS;i++)
				{
					if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10 && PORTVEC->DLLhandle)
						FreeLibrary(PORTVEC->DLLhandle);

					PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;
				}
			}
		}

		GetProcess(GetCurrentProcessId(),pgm);
		n=sprintf(buf,"BPQ32 DLL Detach complete - Program %s - %d Process(es) Attached\n",pgm,AttachedProcesses);
		OutputDebugString(buf);

		return 1;
	}
	return 1;
}

DllExport int APIENTRY CloseBPQ32()	
{
	// Unload External Drivers

	PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	int i;
	int ProcessID = GetCurrentProcessId();

	if (Semaphore.Flag == 1 && ProcessID == Semaphore.SemProcessID)
	{
		OutputDebugString("BPQ32 Process holding Semaphore called CloseBPQ32 - attempting recovery\r\n");
		Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
			Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 

		Semaphore.Flag = 0;
		SemHeldByAPI = 0;
	}

	if (TimerInst == ProcessID)
	{	
		OutputDebugString("BPQ32 Process with Timer called CloseBPQ32\n");

		if (MinimizetoTray)
			Shell_NotifyIcon(NIM_DELETE,&niData);

		for (i=0;i<NUMBEROFPORTS;i++)
		{
			if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
			{
				if (PORTVEC->PORT_EXT_ADDR)
				{
					PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);
				}
			}
			PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);

			PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
		}

		KillTimer(NULL,TimerHandle);
		TimerHandle=0;
		TimerInst=0xffffffff;

		IPClose();
		PMClose();
		APRSClose();
		Rig_Close();
		if (AGWActive)	
			AGWAPITerminate();

		upnpClose();

		CloseTNCEmulator();
		WSACleanup();

		if (hConsWnd) DestroyWindow(hConsWnd);

		Debugprintf("AttachedProcesses %d ", AttachedProcesses);

		if (AttachedProcesses > 1 && Closing == FALSE && AttachingProcess == 0)		// Other processes 
		{
			OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
			StartBPQ32();
		}
	}

	return 0;
}

BOOL CopyReg(HKEY hKeyIn, HKEY hKeyOut);

VOID SetupBPQDirectory()
{
	HKEY hKey = 0;
	HKEY hKeyIn = 0;
	HKEY hKeyOut = 0;
	int disp;
	int retCode,Type,Vallen=MAX_PATH,i;
	char msg[512];
	char ValfromReg[MAX_PATH] = "";
	char DLLName[256]="Not Known";
	char LogDir[256];
	char Time[64];

/*
•NT4 was/is '4' 
•Win 95 is 4.00.950 
•Win 98 is 4.10.1998 
•Win 98 SE is 4.10.2222 
•Win ME is 4.90.3000 
•2000 is NT 5.0.2195 
•XP is actually 5.1 
•Vista is 6.0 
•Win7 is 6.1 

	i = _osver;		/ Build
	i = _winmajor;
	i = _winminor;
*/
/*
#pragma warning(push)
#pragma warning(disable : 4996)

if (_winver < 0x0600)
#pragma warning(pop)
	{
		// Below Vista

		REGTREE = HKEY_LOCAL_MACHINE;
		strcpy(REGTREETEXT, "HKEY_LOCAL_MACHINE");
		ValfromReg[0] = 0;
	}
	else
*/
	{
		if (_stricmp(pgm, "regsvr32.exe") == 0)
		{
			Debugprintf("BPQ32 loaded by regsvr32.exe - Registry not copied");
		}
		else
		{
			// If necessary, move reg from HKEY_LOCAL_MACHINE to HKEY_CURRENT_USER

			retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
				                  "SOFTWARE\\G8BPQ\\BPQ32",
					              0,
						          KEY_READ,
							      &hKeyIn);

			retCode = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKeyOut, &disp);

			// See if Version Key exists in HKEY_CURRENT_USER - if it does, we have already done the copy

			Vallen = MAX_PATH;
			retCode = RegQueryValueEx(hKeyOut, "Version" ,0 , &Type,(UCHAR *)&msg, &Vallen);

			if (retCode != ERROR_SUCCESS)
				if (hKeyIn)
					CopyReg(hKeyIn, hKeyOut);

			RegCloseKey(hKeyIn);
			RegCloseKey(hKeyOut);
		}
	}

	GetModuleFileName(hInstance,DLLName,256);

	BPQDirectory[0]=0;

	retCode = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

    if (retCode == ERROR_SUCCESS)
	{
		// Try "BPQ Directory"
		
		Vallen = MAX_PATH;
		retCode = RegQueryValueEx(hKey,"BPQ Directory",0,			
			&Type,(UCHAR *)&ValfromReg,&Vallen);

		if (retCode == ERROR_SUCCESS)
		{
			if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
				ValfromReg[0]=0;
		}

		if (ValfromReg[0] == 0)
		{
			// BPQ Directory absent or = "" - try "Config File Location"

			Vallen = MAX_PATH;
			
			retCode = RegQueryValueEx(hKey,"Config File Location",0,			
				&Type,(UCHAR *)&ValfromReg,&Vallen);

			if (retCode == ERROR_SUCCESS)
			{
				if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
					ValfromReg[0]=0;
			}
		}

 		if (ValfromReg[0] == 0) GetCurrentDirectory(MAX_PATH, ValfromReg);

		// Get StartMinimized and MinimizetoTray flags

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Start Minimized", 0, &Type, (UCHAR *)&StartMinimized, &Vallen);

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Minimize to Tray", 0, &Type, (UCHAR *)&MinimizetoTray, &Vallen);

		ExpandEnvironmentStrings(ValfromReg, BPQDirectory, MAX_PATH);

		// Also get "BPQ Program Directory"

		ValfromReg[0] = 0;
		Vallen = MAX_PATH;

		retCode = RegQueryValueEx(hKey, "BPQ Program Directory",0 , &Type, (UCHAR *)&ValfromReg, &Vallen);

		if (retCode == ERROR_SUCCESS)
			ExpandEnvironmentStrings(ValfromReg, BPQProgramDirectory, MAX_PATH);

		// And Log Directory

		ValfromReg[0] = 0;
		Vallen = MAX_PATH;

		retCode = RegQueryValueEx(hKey, "Log Directory",0 , &Type, (UCHAR *)&ValfromReg, &Vallen);

		if (retCode == ERROR_SUCCESS)
			ExpandEnvironmentStrings(ValfromReg, LogDirectory, MAX_PATH);

		RegCloseKey(hKey);
	}

	strcpy(ConfigDirectory, BPQDirectory);

	if (LogDirectory[0] == 0)
		strcpy(LogDirectory, BPQDirectory);

	if (BPQProgramDirectory[0] == 0)
		strcpy(BPQProgramDirectory, BPQDirectory);

	sprintf(msg,"BPQ32 Ver %s Loaded from: %s by %s\n", VersionString, DLLName, pgm);
	WritetoConsole(msg);
	OutputDebugString(msg);
	FormatTime3(Time, time(NULL));
	sprintf(msg,"Loaded %s\n", Time);
	WritetoConsole(msg);
	OutputDebugString(msg);

#pragma warning(push)
#pragma warning(disable : 4996)

#if _MSC_VER >= 1400

#define _winmajor 6
#define _winminor 0

#endif

	i=sprintf(msg,"Windows Ver %d.%d, Using Registry Key %s\n" ,_winmajor,  _winminor, REGTREETEXT);

#pragma warning(pop)

 	WritetoConsole(msg);
	OutputDebugString(msg);
	
	i=sprintf(msg,"BPQ32 Using config from: %s\n\n",BPQDirectory);
 	WritetoConsole(&msg[6]);
	msg[i-1]=0;
	OutputDebugString(msg);

	// Don't write the Version Key if loaded by regsvr32.exe (Installer is running with Admin rights,
	//	so will write the wrong tree on )

	if (_stricmp(pgm, "regsvr32.exe") == 0)
	{
		Debugprintf("BPQ32 loaded by regsvr32.exe - Version String not written");
	}
	else
	{
		retCode = RegCreateKeyEx(REGTREE, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		sprintf(msg,"%d,%d,%d,%d", Ver[0], Ver[1], Ver[2], Ver[3]);
		retCode = RegSetValueEx(hKey, "Version",0, REG_SZ,(BYTE *)msg, strlen(msg) + 1);

		RegCloseKey(hKey);
	}

	// Make sure Logs Directory exists
					
	sprintf(LogDir, "%s/Logs", LogDirectory);
	
	CreateDirectory(LogDir, NULL);

	return;	
}

HANDLE OpenConfigFile(char *fn)
{
	HANDLE handle;
	UCHAR Value[MAX_PATH];
	FILETIME LastWriteTime;
	SYSTEMTIME Time;
	char Msg[256];


	// If no directory, use current
	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
		
	handle = CreateFile(Value,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	GetFileTime(handle, NULL, NULL, &LastWriteTime);
	FileTimeToSystemTime(&LastWriteTime, &Time);

	sprintf(Msg,"BPQ32 Config File %s Created %.2d:%.2d %d/%.2d/%.2d\n", Value,
				Time.wHour, Time.wMinute, Time.wYear, Time.wMonth, Time.wDay);

	OutputDebugString(Msg);

	return(handle);
}

#ifdef _WIN64
int BPQHOSTAPI()
{
	return 0;
}
#endif


DllExport int APIENTRY GETBPQAPI()
{
 	return (int)BPQHOSTAPI;
}
    
//DllExport UINT APIENTRY GETMONDECODE()
//{
// 	return (UINT)MONDECODE;
//}
    

DllExport INT APIENTRY BPQAPI(int Fn, char * params)
{

/*
;
;	BPQ HOST MODE SUPPORT CODE 
;
;	22/11/95
;
;	MOVED FROM TNCODE.ASM COS CONITIONALS WERE GETTING TOO COMPLICATED
;	(OS2 VERSION HAD UPSET KANT VERISON
;
;
*/


/*

  BPQHOSTPORT:
;
;	SPECIAL INTERFACE, MAINLY FOR EXTERNAL HOST MODE SUPPORT PROGS
;
;	COMMANDS SUPPORTED ARE
;
;	AH = 0	Get node/switch version number and description.  On return
;		AH='B',AL='P',BH='Q',BL=' '
;		DH = major version number and DL = minor version number.
;
;
;	AH = 1	Set application mask to value in DL (or even DX if 16
;		applications are ever to be supported).
;
;		Set application flag(s) to value in CL (or CX).
;		whether user gets connected/disconnected messages issued
;		by the node etc.
;
;
;	AH = 2	Send frame in ES:SI (length CX)
;
;
;	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
;		in CX.  BX returns the number of outstanding frames still to
;		be received (ie. after this one) or zero if no more frames
;		(ie. this is last one).
;
;
;
;	AH = 4	Get stream status.  Returns:
;
;		CX = 0 if stream disconnected or CX = 1 if stream connected
;		DX = 0 if no change of state since last read, or DX = 1 if
;		       the connected/disconnected state has changed since
;		       last read (ie. delta-stream status).
;
;
;
;	AH = 6	Session control.
;
;		CX = 0 Conneect - _APPLMASK in DL
;		CX = 1 connect
;		CX = 2 disconnect
;		CX = 3 return user to node
;
;
;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		BX = L2 paclen for the radio port
;		CX = L2 maxframe for the radio port
;		DX = L4 window size (if L4 circuit, or zero)
;		ES:DI = CALLSIGN

;AH = 9		Fetch node/application callsign & alias.  AL = application
;		number:
;
;		0 = node
;		1 = BBS
;		2 = HOST
;		3 = SYSOP etc. etc.
;
;		Returns string with alias & callsign or application name in
;		user's buffer pointed to by ES:SI length CX.  For example:
;
;		"WORCS:G8TIC"  or "TICPMS:G8TIC-10".
;
;
;	AH = 10	Unproto transmit frame.  Data pointed to by ES:SI, of
;		length CX, is transmitted as a HDLC frame on the radio
;		port (not stream) in AL.
;
;
;	AH = 11 Get Trace (RAW Data) Frame into ES:DI,
;			 Length to CX, Timestamp to AX
;
;
;	AH = 12 Update Switch. At the moment only Beacon Text may be updated
;		DX = Function
;		     1=update BT. ES:SI, Len CX = Text
;		     2=kick off nodes broadcast
;
;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;
;
;	AH = 14 Internal Interface for IP Router
;
;		Send frame - to NETROM L3 if DL=0
;			     to L2 Session if DL<>0
;
;
; 	AH = 15 Get interval timer


*/


    switch(Fn)
	{

	case CHECKLOADED:

		params[0]=MAJORVERSION;
		params[1]=MINORVERSION;
		params[2]=QCOUNT;
		
		return (1);
	}
	return 0;
}

DllExport int APIENTRY InitSwitch()
{
	return (0);
}

/*DllExport int APIENTRY SwitchTimer()
{
	GetSemaphore((&Semaphore);

	TIMERINTERRUPT();
	
	FreeSemaphore(&Semaphore);

	return (0);
}
*/
DllExport int APIENTRY GetFreeBuffs()
{
//	Returns number of free buffers
//	(BPQHOST function 7 (part)).
	return (QCOUNT);
}

DllExport UCHAR * APIENTRY GetNodeCall()
{
	return (&MYNODECALL);
}


DllExport UCHAR * APIENTRY GetNodeAlias()
{
	return (&MYALIASTEXT[0]);
}

DllExport UCHAR * APIENTRY GetBBSCall()
{
	return (UCHAR *)(&APPLCALLTABLE[0].APPLCALL_TEXT);
}


DllExport UCHAR * APIENTRY GetBBSAlias()
{
	return (UCHAR *)(&APPLCALLTABLE[0].APPLALIAS_TEXT);
}

DllExport VOID APIENTRY GetApplCallVB(int Appl, char * ApplCall)
{
	if (Appl < 1 || Appl > NumberofAppls ) return;

	strncpy(ApplCall,(char *)&APPLCALLTABLE[Appl-1].APPLCALL_TEXT, 10);
}

BOOL UpdateNodesForApp(int Appl);

DllExport BOOL APIENTRY SetApplCall(int Appl, char * NewCall)
{
	char Call[10]="          ";
	int i;

	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	i=strlen(NewCall);

	if (i > 10) i=10;

	strncpy(Call,NewCall,i);

	strncpy((char *)&APPLCALLTABLE[Appl-1].APPLCALL_TEXT,Call,10);

	if (!ConvToAX25(Call,APPLCALLTABLE[Appl-1].APPLCALL)) return FALSE;

	UpdateNodesForApp(Appl);

	return TRUE;

}

DllExport BOOL APIENTRY SetApplAlias(int Appl, char * NewCall)
{
	char Call[10]="          ";
	int i;

	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	i=strlen(NewCall);

	if (i > 10) i=10;

	strncpy(Call,NewCall,i);

	strncpy((char *)&APPLCALLTABLE[Appl-1].APPLALIAS_TEXT,Call,10);

	if (!ConvToAX25(Call,APPLCALLTABLE[Appl-1].APPLALIAS)) return FALSE;

	UpdateNodesForApp(Appl);

	return TRUE;

}



DllExport BOOL APIENTRY SetApplQual(int Appl, int NewQual)
{
	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	APPLCALLTABLE[Appl-1].APPLQUAL=NewQual;

	UpdateNodesForApp(Appl);

	return TRUE;

}


BOOL UpdateNodesForApp(int Appl)
{
	int App=Appl-1;
	int DestLen = sizeof (struct DEST_LIST);
	int n = MAXDESTS;

	struct DEST_LIST * DEST = APPLCALLTABLE[App].NODEPOINTER;
	APPLCALLS * APPL=&APPLCALLTABLE[App];

	if (DEST == NULL)
	{
		// No dest at the moment. If we have valid call and Qual, create an entry
		
		if (APPLCALLTABLE[App].APPLQUAL == 0) return FALSE;

		if (APPLCALLTABLE[App].APPLCALL[0] < 41) return FALSE;


		GetSemaphore(&Semaphore, 5);
	
		DEST = DESTS;
	
		while (n--)
		{
			if (DEST->DEST_CALL[0] == 0)		// Spare
				break;
		}		

		if (n == 0)
		{
			// no dests

			FreeSemaphore(&Semaphore);
			return FALSE;
		}

		NUMBEROFNODES++;
		APPL->NODEPOINTER = DEST;
		
		memmove (DEST->DEST_CALL,APPL->APPLCALL,13);

		DEST->DEST_STATE=0x80;	// SPECIAL ENTRY
	
		DEST->NRROUTE[0].ROUT_QUALITY = (BYTE)APPL->APPLQUAL;
		DEST->NRROUTE[0].ROUT_OBSCOUNT = 255;

		FreeSemaphore(&Semaphore);

		return TRUE;
	}

	//	We have a destination. If Quality is zero, remove it, else update it

	if (APPLCALLTABLE[App].APPLQUAL == 0)
	{
		GetSemaphore(&Semaphore, 6);

		REMOVENODE(DEST);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry
		
		APPL->NODEPOINTER=NULL;

		FreeSemaphore(&Semaphore);
		return FALSE;

	}

	if (APPLCALLTABLE[App].APPLCALL[0] < 41)	return FALSE;

	GetSemaphore(&Semaphore, 7);

	memmove (DEST->DEST_CALL,APPL->APPLCALL,13);

	DEST->DEST_STATE=0x80;	// SPECIAL ENTRY
	
	DEST->NRROUTE[0].ROUT_QUALITY = (BYTE)APPL->APPLQUAL;
	DEST->NRROUTE[0].ROUT_OBSCOUNT = 255;

	FreeSemaphore(&Semaphore);
	return TRUE;

}


DllExport UCHAR * APIENTRY GetSignOnMsg()
{
	return (&SIGNONMSG[0]);
}


DllExport HKEY APIENTRY GetRegistryKey()
{
	return REGTREE;
}

DllExport char * APIENTRY GetRegistryKeyText()
{
	return REGTREETEXT;;
}

DllExport UCHAR * APIENTRY GetBPQDirectory()
{
	while (BPQDirectory[0] == 0)
	{
		Debugprintf("BPQ Directory not set up - waiting");
		Sleep(1000);
	}
	return (&BPQDirectory[0]);
}

DllExport UCHAR * APIENTRY GetProgramDirectory()
{
	return (&BPQProgramDirectory[0]);
}

DllExport UCHAR * APIENTRY GetLogDirectory()
{
	return (&LogDirectory[0]);
}

// Version for Visual Basic

DllExport char * APIENTRY CopyBPQDirectory(char * dir)
{
	return (strcpy(dir,BPQDirectory));
}

DllExport int APIENTRY GetMsgPerl(int stream, char * msg)
{
	int len,count;

	GetMsg(stream, msg, &len, &count );

	return len;
}

int Rig_Command(int Session, char * Command);

BOOL Rig_CommandInt(int Session, char * Command)
{
	return Rig_Command(Session, Command);
}

DllExport int APIENTRY BPQSetHandle(int Stream, HWND hWnd)
{ 
	BPQHOSTVECTOR[Stream-1].HOSTHANDLE=hWnd;
	return (0);
}

#define L4USER 0

BPQVECSTRUC * PORTVEC ;

VOID * InitializeExtDriver(PEXTPORTDATA PORTVEC)
{
	HINSTANCE ExtDriver=0;
	char msg[128];
	int err=0;
	HKEY hKey=0;
	UCHAR Value[MAX_PATH];
	
	// If no directory, use current

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,PORTVEC->PORT_DLL_NAME);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,PORTVEC->PORT_DLL_NAME);
	}

	// Several Drivers are now built into bpq32.dll

	_strupr(Value);

	if (strstr(Value, "BPQVKISS"))
		return VCOMExtInit;

	if (strstr(Value, "BPQAXIP"))
		return AXIPExtInit;

	if (strstr(Value, "BPQETHER"))
		return ETHERExtInit;

	if (strstr(Value, "BPQTOAGW"))
		return AGWExtInit;

	if (strstr(Value, "AEAPACTOR"))
		return AEAExtInit;

	if (strstr(Value, "HALDRIVER"))
		return HALExtInit;

	if (strstr(Value, "KAMPACTOR"))
		return KAMExtInit;

	if (strstr(Value, "SCSPACTOR"))
		return SCSExtInit;

	if (strstr(Value, "WINMOR"))
		return WinmorExtInit;
	
	if (strstr(Value, "V4"))
		return V4ExtInit;
	
	if (strstr(Value, "TELNET"))
		return TelnetExtInit;

//	if (strstr(Value, "SOUNDMODEM"))
//		return SoundModemExtInit;

	if (strstr(Value, "SCSTRACKER"))
		return TrackerExtInit;

	if (strstr(Value, "TRKMULTI"))
		return TrackerMExtInit;

	if (strstr(Value, "UZ7HO"))
		return UZ7HOExtInit;

	if (strstr(Value, "MULTIPSK"))
		return MPSKExtInit;

	if (strstr(Value, "FLDIGI"))
		return FLDigiExtInit;

	if (strstr(Value, "UIARQ"))
		return UIARQExtInit;

//	if (strstr(Value, "BAYCOM"))
//		return (UINT) BaycomExtInit;

	if (strstr(Value, "VARA"))
		return VARAExtInit;

	if (strstr(Value, "ARDOP"))
		return ARDOPExtInit;
	
	if (strstr(Value, "SERIAL"))
		return SerialExtInit;

	if (strstr(Value, "KISSHF"))
		return KISSHFExtInit;

	if (strstr(Value, "WINRPR"))
		return WinRPRExtInit;

	if (strstr(Value, "HSMODEM"))
		return HSMODEMExtInit;

	if (strstr(Value, "FREEDATA"))
		return FreeDataExtInit;

	if (strstr(Value, "6PACK"))
		return SIXPACKExtInit;

	ExtDriver = LoadLibrary(Value);

	if (ExtDriver == NULL)
	{
		err=GetLastError();

		sprintf(msg,"Error loading Driver %s - Error code %d",
				PORTVEC->PORT_DLL_NAME,err);
		
		MessageBox(NULL,msg,"BPQ32",MB_ICONSTOP);

		return(0);
	}

	PORTVEC->DLLhandle=ExtDriver;

	return (GetProcAddress(ExtDriver,"_ExtInit@4"));

}

/*
_DATABASE	LABEL	BYTE

FILLER		DB	14 DUP (0)	; PROTECTION AGENST BUFFER PROBLEMS!
			DB	MAJORVERSION,MINORVERSION
_NEIGHBOURS	DD	0
			DW	TYPE ROUTE
_MAXNEIGHBOURS	DW	20		; MAX ADJACENT NODES

_DESTS		DD	0		; NODE LIST
			DW	TYPE DEST_LIST
MAXDESTS	DW	100		; MAX NODES IN SYSTEM
*/


DllExport int APIENTRY GetAttachedProcesses()
{
	return (AttachedProcesses);
}

DllExport int * APIENTRY GetAttachedProcessList()
{
	return (&AttachedPIDList[0]);
}

DllExport int * APIENTRY SaveNodesSupport()
{
	return (&DATABASESTART);
}

//
//	Internal BPQNODES support
//

#define UCHAR unsigned char

/*
ROUTE ADD G1HTL-1 2 200  0 0 0
ROUTE ADD G4IRX-3 2 200  0 0 0
NODE ADD MAPPLY:G1HTL-1 G1HTL-1 2 200 G4IRX-3 2 98 
NODE ADD NOT:GB7NOT G1HTL-1 2 199 G4IRX-3 2 98 

*/

struct DEST_LIST * Dests;
struct ROUTE * Routes;

int MaxNodes;
int MaxRoutes;
int NodeLen;
int RouteLen;

int count;
int cursor;

int len,i;
	
ULONG cnt;
char Normcall[10];
char Portcall[10];
char Alias[7];

char line[100];

HANDLE handle;

int APIENTRY Restart()
{
	int i, Count = AttachedProcesses;
	HANDLE hProc;
	DWORD PID;

	for (i = 0; i < Count; i++)
	{
		PID = AttachedPIDList[i];
		
		// Kill Timer Owner last

		if (TimerInst != PID)
		{
			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);

			if (hProc)
			{
				TerminateProcess(hProc, 0);
				CloseHandle(hProc);
			}
		}
	}

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TimerInst);

		if (hProc)
		{
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
		}

	
	return 0;
}

int APIENTRY Reboot()
{
	// Run shutdown -r -f

	STARTUPINFO  SInfo;
    PROCESS_INFORMATION PInfo;
	char Cmd[] = "shutdown -r -f";

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	return CreateProcess(NULL, Cmd, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);
}
/*
int APIENTRY Reconfig()
{
	if (!ProcessConfig())
	{
		return (0);
	}
	SaveNodes();
	WritetoConsole("Nodes Saved\n");
	ReconfigFlag=TRUE;	
	WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
	return 1;
}
*/
// Code to support minimizing all BPQ Apps to a single Tray ICON

// As we can't minimize the console window to the tray, I'll use an ordinary
// window instead. This also gives me somewhere to post the messages to


char AppName[] = "BPQ32";
char Title[80] = "BPQ32.dll Console";

int NewLine();

char FrameClassName[]	= TEXT("MdiFrame");

HWND ClientWnd; //This stores the MDI client area window handle

LOGFONT LFTTYFONT ;

HFONT hFont ;

HMENU hPopMenu, hWndMenu;
HMENU hMainFrameMenu = NULL;
HMENU hBaseMenu = NULL;
HMENU hConsMenu = NULL;
HMENU hTermMenu = NULL;
HMENU hMonMenu = NULL;
HMENU hTermActMenu, hTermCfgMenu, hTermEdtMenu, hTermHlpMenu;
HMENU hMonActMenu, hMonCfgMenu, hMonEdtMenu, hMonHlpMenu;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DllExport int APIENTRY DeleteTrayMenuItem(HWND hWnd);

#define BPQMonitorAvail 1
#define BPQDataAvail 2
#define BPQStateChange 4

VOID GetJSONValue(char * _REPLYBUFFER, char * Name, char * Value);
SOCKET OpenWL2KHTTPSock();
SendHTTPRequest(SOCKET sock, char * Request, char * Params, int Len, char * Return);

BOOL GetWL2KSYSOPInfo(char * Call, char * _REPLYBUFFER);
BOOL UpdateWL2KSYSOPInfo(char * Call, char * SQL);


static INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		char _REPLYBUFFER[1000] = "";
		char Value[1000];

		if (GetWL2KSYSOPInfo(WL2KCall, _REPLYBUFFER))
		{
//			if (strstr(_REPLYBUFFER, "\"ErrorMessage\":") == 0)

			GetJSONValue(_REPLYBUFFER, "\"SysopName\":", Value);	
			SetDlgItemText(hDlg, NAME, Value);

			GetJSONValue(_REPLYBUFFER, "\"GridSquare\":", Value);	
			SetDlgItemText(hDlg, IDC_Locator, Value);

			GetJSONValue(_REPLYBUFFER, "\"StreetAddress1\":", Value);	
			SetDlgItemText(hDlg, ADDR1, Value);

			GetJSONValue(_REPLYBUFFER, "\"StreetAddress2\":", Value);	
			SetDlgItemText(hDlg, ADDR2, Value);

			GetJSONValue(_REPLYBUFFER, "\"City\":", Value);	
			SetDlgItemText(hDlg, CITY, Value);

			GetJSONValue(_REPLYBUFFER, "\"State\":", Value);	
			SetDlgItemText(hDlg, STATE, Value);
			
			GetJSONValue(_REPLYBUFFER, "\"Country\":", Value);	
			SetDlgItemText(hDlg, COUNTRY, Value);

			GetJSONValue(_REPLYBUFFER, "\"PostalCode\":", Value);	
			SetDlgItemText(hDlg, POSTCODE, Value);

			GetJSONValue(_REPLYBUFFER, "\"Email\":", Value);	
			SetDlgItemText(hDlg, EMAIL, Value);

			GetJSONValue(_REPLYBUFFER, "\"Website\":", Value);	
			SetDlgItemText(hDlg, WEBSITE, Value);

			GetJSONValue(_REPLYBUFFER, "\"Phones\":", Value);	
			SetDlgItemText(hDlg, PHONE, Value);

			GetJSONValue(_REPLYBUFFER, "\"Comments\":", Value);	
			SetDlgItemText(hDlg, ADDITIONALDATA, Value);

		}
	
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:

		switch(LOWORD(wParam))
		{

		case ID_SAVE:
		{
			char Name[100];
			char PasswordText[100];
			char LocatorText[100];
			char Addr1[100];
			char Addr2[100];
			char City[100];
			char State[100];
			char Country[100];
			char PostCode[100];
			char Email[100];
			char Website[100];
			char Phone[100];
			char Data[100];

			SOCKET sock;
			
			int Len;
			char Message[2048];
			char Reply[2048] = "";


			GetDlgItemText(hDlg, NAME, Name, 99);
			GetDlgItemText(hDlg, IDC_Password, PasswordText, 99);
			GetDlgItemText(hDlg, IDC_Locator, LocatorText, 99);
			GetDlgItemText(hDlg, ADDR1, Addr1, 99);
			GetDlgItemText(hDlg, ADDR2, Addr2, 99);
			GetDlgItemText(hDlg, CITY, City, 99);
			GetDlgItemText(hDlg, STATE, State, 99);
			GetDlgItemText(hDlg, COUNTRY, Country, 99);
			GetDlgItemText(hDlg, POSTCODE, PostCode, 99);
			GetDlgItemText(hDlg, EMAIL, Email, 99);
			GetDlgItemText(hDlg, WEBSITE, Website, 99);
			GetDlgItemText(hDlg, PHONE, Phone, 99);
			GetDlgItemText(hDlg, ADDITIONALDATA, Data, 99);


//{"Callsign":"String","GridSquare":"String","SysopName":"String",
//"StreetAddress1":"String","StreetAddress2":"String","City":"String",
//"State":"String","Country":"String","PostalCode":"String","Email":"String",
//"Phones":"String","Website":"String","Comments":"String"}

			Len = sprintf(Message,
				"\"Callsign\":\"%s\","
				"\"Password\":\"%s\","
				"\"GridSquare\":\"%s\","
				"\"SysopName\":\"%s\","
				"\"StreetAddress1\":\"%s\","
				"\"StreetAddress2\":\"%s\","
				"\"City\":\"%s\","
				"\"State\":\"%s\","
				"\"Country\":\"%s\","
				"\"PostalCode\":\"%s\","
				"\"Email\":\"%s\","
				"\"Phones\":\"%s\","
				"\"Website\":\"%s\","
				"\"Comments\":\"%s\"",

				WL2KCall, PasswordText, LocatorText, Name, Addr1, Addr2, City, State, Country, PostCode, Email, Phone, Website, Data);
		
				Debugprintf("Sending %s", Message);

				sock = OpenWL2KHTTPSock();

				if (sock)
				{
					char * ptr;
					
					SendHTTPRequest(sock, 
						"/sysop/add", Message, Len, Reply);

					ptr = strstr(Reply, "\"ErrorCode\":");

					if (ptr)
					{
						ptr = strstr(ptr, "Message");
						if (ptr)
						{
							ptr += 10;
							strlop(ptr, '"');
							MessageBox(NULL ,ptr, "Error", MB_OK);
						}
					}
					else
						MessageBox(NULL, "Sysop Record Updated", "BPQ32", MB_OK);

				}
				closesocket(sock);
		}

		case ID_CANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
		}
	}
	return (INT_PTR)FALSE;
}



LRESULT CALLBACK UIWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
VOID WINAPI OnTabbedDialogInit(HWND hDlg);

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT pos;
	BOOL ret;

	CLIENTCREATESTRUCT MDIClientCreateStruct; // Structure to be used for MDI client area
	//HWND m_hwndSystemInformation = 0;

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(wParam);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(wParam);
				
		if (lParam & BPQStateChange)
			DoStateChange(wParam);

		return (0);
	}

	switch (message)
	{ 
		case MY_TRAY_ICON_MESSAGE:
			
			switch(lParam)
			{
			case WM_RBUTTONUP:	
			case WM_LBUTTONUP:

				GetCursorPos(&pos);

	//			SetForegroundWindow(FrameWnd);

				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, FrameWnd, 0);
				return 0;
			}

			break;

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;
			
		case WM_SIZING:
		case WM_SIZE:

			SendMessage(ClientWnd, WM_MDIICONARRANGE, 0 ,0);
			break;

		case WM_NCCREATE:

			ret = DefFrameProc(hWnd, ClientWnd, message, wParam, lParam);
			return TRUE;

		case WM_CREATE:

		// On creation of main frame, create the MDI client area

		MDIClientCreateStruct.hWindowMenu	= NULL;
		MDIClientCreateStruct.idFirstChild	= IDM_FIRSTCHILD;
		
		ClientWnd = CreateWindow(TEXT("MDICLIENT"), // predefined value for MDI client area
									NULL, // no caption required
									WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
									0, // No need to give any x/y or height/width since this client
									   // will just be used to get client windows created, effectively
									   // in the main window we will be seeing the mainframe window client area itself.
									0, 
									0,
									0,
									hWnd,
									NULL,
									hInstance,
									(void *) &MDIClientCreateStruct);


		return 0;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			if (wmId >= TRAYBASEID && wmId < (TRAYBASEID + 100))
			{ 
				handle=hWndArray[wmId-TRAYBASEID];

				if (handle == FrameWnd)
					ShowWindow(handle, SW_NORMAL);

				if (handle == FrameWnd && FrameMaximized == TRUE)
					PostMessage(handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				else
					PostMessage(handle, WM_SYSCOMMAND, SC_RESTORE, 0);
				
				SetForegroundWindow(handle);
				return 0;
			}

			switch(wmId)
			{
			struct ConsoleInfo * Cinfo = NULL;

			case ID_NEWWINDOW:
				Cinfo = CreateChildWindow(0, FALSE);
				if (Cinfo)
					SendMessage(ClientWnd, WM_MDIACTIVATE, (WPARAM)Cinfo->hConsole, 0);
				break;

			case ID_WINDOWS_CASCADE:
				SendMessage(ClientWnd, WM_MDICASCADE, 0, 0);
				return 0;
					
			case ID_WINDOWS_TILE:
				SendMessage(ClientWnd, WM_MDITILE , MDITILE_HORIZONTAL, 0);
				return 0;

			case BPQCLOSEALL:
				CloseAllPrograms();
	//			SendMessage(ClientWnd, WM_MDIICONARRANGE, 0 ,0);

				return 0;

			case BPQUICONFIG:
			{
				int err, i=0;
				char Title[80];
				WNDCLASS  wc;
	
				wc.style = CS_HREDRAW | CS_VREDRAW;
				wc.lpfnWndProc = UIWndProc;       
				wc.cbClsExtra = 0;                
				wc.cbWndExtra = DLGWINDOWEXTRA;
				wc.hInstance = hInstance;
				wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
				wc.hCursor = LoadCursor(NULL, IDC_ARROW);
				wc.hbrBackground = bgBrush; 

				wc.lpszMenuName = NULL;	
				wc.lpszClassName = UIClassName; 

				RegisterClass(&wc);

				UIhWnd = CreateDialog(hInstance, UIClassName, 0, NULL);

				if (!UIhWnd)
				{	
					err=GetLastError();
					return FALSE;
				}

				wsprintf(Title,"BPQ32 Beacon Configuration");
				MySetWindowText(UIhWnd, Title);
				ShowWindow(UIhWnd, SW_NORMAL);
	
				OnTabbedDialogInit(UIhWnd);			// Set up pages

	//			UpdateWindow(UIhWnd);
				return 0;
			}


			case IDD_WL2KSYSOP:

				if (WL2KCall[0] == 0)
				{
					MessageBox(NULL,"WL2K Reporting is not configured","BPQ32", MB_OK);
					break;
				}
					
				DialogBox(hInstance, MAKEINTRESOURCE(IDD_WL2KSYSOP), hWnd, ConfigWndProc);
				break;

		
			 // Handle MDI Window commands
            
			default:
			{
				if(wmId >= IDM_FIRSTCHILD)
				{
					DefFrameProc(hWnd, ClientWnd, message, wParam, lParam);
				}
				else 
				{
					HWND hChild = (HWND)SendMessage(ClientWnd, WM_MDIGETACTIVE,0,0);

					if(hChild)
						SendMessage(hChild, WM_COMMAND, wParam, lParam);
				}
			}
			}
  
			break;

		case WM_INITMENUPOPUP:
		{
			HWND hChild = (HWND)SendMessage(ClientWnd, WM_MDIGETACTIVE,0,0);

			if(hChild)
				SendMessage(hChild, WM_INITMENUPOPUP, wParam, lParam);
		}

		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case SC_MAXIMIZE: 

			FrameMaximized = TRUE;
			break;

		case SC_RESTORE: 

			FrameMaximized = FALSE;
			break;

		case SC_MINIMIZE: 

			if (MinimizetoTray)
			{
				ShowWindow(hWnd, SW_HIDE);
				return TRUE;
			}
		}

		return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));

		case WM_CLOSE:
	
			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));

	}	
	return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));
}

int OffsetH, OffsetW;

int SetupConsoleWindow()
{
    WNDCLASS  wc;
	int i;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];
	WNDCLASSEX wndclassMainFrame;
	RECT CRect;

	retCode = RegOpenKeyEx (REGTREE,
                "SOFTWARE\\G8BPQ\\BPQ32",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"FrameWindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&FRect.left,&FRect.right,&FRect.top,&FRect.bottom);

		if (FRect.top < - 500 || FRect.left < - 500)
		{
			FRect.left = 0;
			FRect.top = 0;
			FRect.right = 600;
			FRect.bottom = 400;
		}


		Vallen=80;
		retCode = RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &ConsoleMinimized);

		if (Rect.top < - 500 || Rect.left < - 500)
		{
			Rect.left = 0;
			Rect.top = 0;
			Rect.right = 600;
			Rect.bottom = 400;
		}

		Vallen=80;

		retCode = RegQueryValueEx(hKey,"StatusWindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size, "%d,%d,%d,%d,%d", &StatusRect.left, &StatusRect.right,
				&StatusRect.top, &StatusRect.bottom, &StatusMinimized);

		if (StatusRect.top < - 500 || StatusRect.left < - 500)
		{
			StatusRect.left = 0;
			StatusRect.top = 0;
			StatusRect.right = 850;
			StatusRect.bottom = 500;
		}


		// Get StartMinimized and MinimizetoTray flags

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Start Minimized", 0, &Type, (UCHAR *)&StartMinimized, &Vallen);

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Minimize to Tray", 0, &Type, (UCHAR *)&MinimizetoTray, &Vallen);
	}

	wndclassMainFrame.cbSize		= sizeof(WNDCLASSEX);
	wndclassMainFrame.style			= CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wndclassMainFrame.lpfnWndProc	= FrameWndProc;
	wndclassMainFrame.cbClsExtra	= 0;
	wndclassMainFrame.cbWndExtra	= 0;
	wndclassMainFrame.hInstance		= hInstance;
    wndclassMainFrame.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
	wndclassMainFrame.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclassMainFrame.hbrBackground	= (HBRUSH) GetStockObject(GRAY_BRUSH);
	wndclassMainFrame.lpszMenuName	= NULL;
	wndclassMainFrame.lpszClassName	= FrameClassName;
	wndclassMainFrame.hIconSm		= NULL;
	
	if(!RegisterClassEx(&wndclassMainFrame))
	{
		return 0;
	}

	pindex = 0;
	PartLine = FALSE;

	bgBrush = CreateSolidBrush(BGCOLOUR);

//	hMainFrameMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME_MENU));

	hBaseMenu = LoadMenu(hInstance, MAKEINTRESOURCE(CONS_MENU));
	hConsMenu = GetSubMenu(hBaseMenu, 1);
	hWndMenu = GetSubMenu(hBaseMenu, 0);

	hTermMenu = LoadMenu(hInstance, MAKEINTRESOURCE(TERM_MENU));
	hTermActMenu = GetSubMenu(hTermMenu, 1);
	hTermCfgMenu = GetSubMenu(hTermMenu, 2);
	hTermEdtMenu = GetSubMenu(hTermMenu, 3);
	hTermHlpMenu = GetSubMenu(hTermMenu, 4);

	hMonMenu = LoadMenu(hInstance, MAKEINTRESOURCE(MON_MENU));
	hMonCfgMenu = GetSubMenu(hMonMenu, 1);
	hMonEdtMenu = GetSubMenu(hMonMenu, 2);
	hMonHlpMenu = GetSubMenu(hMonMenu, 3);

	hMainFrameMenu = CreateMenu();
	AppendMenu(hMainFrameMenu, MF_STRING + MF_POPUP, (UINT)hWndMenu, "Window");

	//Create the main MDI frame window

	ClientWnd = NULL;

	FrameWnd = CreateWindow(FrameClassName, 
								"BPQ32 Console", 
								WS_OVERLAPPEDWINDOW |WS_CLIPCHILDREN,
								FRect.left,	
								FRect.top,
								FRect.right - FRect.left,
								FRect.bottom - FRect.top,
								NULL,			// handle to parent window
								hMainFrameMenu, // handle to menu
								hInstance,	// handle to the instance of module
								NULL);		// Long pointer to a value to be passed to the window through the 
											// CREATESTRUCT structure passed in the lParam parameter the WM_CREATE message


	// Get Client Params

	if (FrameWnd == 0)
	{
		Debugprintf("SetupConsoleWindow Create Frame failed %d", GetLastError());
		return 0;
	}

	ShowWindow(FrameWnd, SW_RESTORE);


	GetWindowRect(FrameWnd, &FRect);
	OffsetH = FRect.bottom - FRect.top;
	OffsetW = FRect.right - FRect.left;
	GetClientRect(FrameWnd, &CRect);
	OffsetH -= CRect.bottom;
	OffsetW -= CRect.right;
	OffsetH -= 4;

	// Create Console Window
        
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = ClassName; 

	i=RegisterClass(&wc);
	
	sprintf (Title, "BPQ32.dll Console Version %s", VersionString);

	hConsWnd =  CreateMDIWindow(ClassName, "Console", 0,
		  0,0,0,0, ClientWnd, hInstance, 1234);

	i = GetLastError();

	if (!hConsWnd) {
		return (FALSE);
	}

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)StatusWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = "Status"; 

	i=RegisterClass(&wc);

	if (StatusRect.top < OffsetH)			// Make sure not off top of MDI frame
	{
		int Error = OffsetH - StatusRect.top;
		StatusRect.top += Error;
		StatusRect.bottom += Error;
	}

	StatusWnd =  CreateMDIWindow("Status", "Stream Status", 0,
		  StatusRect.left,	StatusRect.top, StatusRect.right - StatusRect.left,
		  StatusRect.bottom - StatusRect.top, ClientWnd, hInstance, 1234);

	SetTimer(StatusWnd, 1, 1000, NULL);

	hPopMenu = GetSubMenu(hBaseMenu, 1) ;

	if (MinimizetoTray)
		CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_CHECKED);
	else
		CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_UNCHECKED);
				
	if (StartMinimized)
		CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_CHECKED);
	else
		CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_UNCHECKED);
	
	DrawMenuBar(hConsWnd);	

	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        0;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH;
   lstrcpy(LFTTYFONT.lfFaceName, "FIXEDSYS" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	
	SetWindowText(hConsWnd,Title);

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hConsWnd, &Rect);
	}

	if (Rect.top < OffsetH)			// Make sure not off top of MDI frame
	{
		int Error = OffsetH - Rect.top;
		Rect.top += Error;
		Rect.bottom += Error;
	}


	MoveWindow(hConsWnd, Rect.left - (OffsetW /2), Rect.top - OffsetH, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindow(StatusWnd, StatusRect.left - (OffsetW /2), StatusRect.top - OffsetH,
		StatusRect.right-StatusRect.left, StatusRect.bottom-StatusRect.top, TRUE);

	hWndCons = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",
		WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
                    LBS_DISABLENOSCROLL | LBS_NOSEL | WS_VSCROLL | WS_HSCROLL,
		Rect.left,	Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top,
		hConsWnd, NULL, hInstance, NULL);

//	SendMessage(hWndCons, WM_SETFONT, hFont, 0);

	SendMessage(hWndCons, LB_SETHORIZONTALEXTENT , 1000, 0);

	if (ConsoleMinimized)
		ShowWindow(hConsWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hConsWnd, SW_RESTORE);

	if (StatusMinimized)
		ShowWindow(StatusWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(StatusWnd, SW_RESTORE);

	ShowWindow(FrameWnd, SW_RESTORE);


	LoadLibrary("riched20.dll");

	if (StartMinimized)
		if (MinimizetoTray)
			ShowWindow(FrameWnd, SW_HIDE);
		else
			ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
	else
		ShowWindow(FrameWnd, SW_RESTORE);
	
	CreateMonitorWindow(Size);

	return 0;
}

DllExport int APIENTRY SetupTrayIcon()
{
	if (MinimizetoTray == 0) 
		return 0;

	trayMenu = CreatePopupMenu();

	for( i = 0; i < 100; ++i )
	{
		if (strcmp(PopupText[i],"BPQ32 Console") == 0)
		{
			hWndArray[i] = FrameWnd;
			goto doneit;
		}
	}

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == 0)
		{
			hWndArray[i] = FrameWnd;
			strcpy(PopupText[i],"BPQ32 Console");
			break;
		}
	}
doneit:

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] != 0)
			AppendMenu(trayMenu,MF_STRING,TRAYBASEID+i,PopupText[i]);
	}

	//	Set up Tray ICON

	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);

	// the ID number can be any UINT you choose and will
	// be used to identify your icon in later calls to
	// Shell_NotifyIcon

	niData.uID = TRAY_ICON_ID;

	// state which structure members are valid
	// here you can also choose the style of tooltip
	// window if any - specifying a balloon window:
	// NIF_INFO is a little more complicated 

	strcpy(niData.szTip,"BPQ32 Windows"); 

	niData.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;

	// load the icon note: you should destroy the icon
	// after the call to Shell_NotifyIcon

	niData.hIcon = 
		
		//LoadIcon(NULL, IDI_APPLICATION);

		(HICON)LoadImage( hInstance,
			MAKEINTRESOURCE(BPQICON),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR);


	// set the window you want to receive event messages

	niData.hWnd = FrameWnd;

	// set the message to send
	// note: the message value should be in the
	// range of WM_APP through 0xBFFF

	niData.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

	//	Call Shell_NotifyIcon. NIM_ADD adds a new tray icon

	if (Shell_NotifyIcon(NIM_ADD,&niData))
		Debugprintf("BPQ32 Create Tray Icon Ok");
//	else
//		Debugprintf("BPQ32 Create Tray Icon failed %d", GetLastError());

	return 0;
}

VOID SaveConfig()
{
	HKEY hKey=0;
	int retCode, disp;

	retCode = RegCreateKeyEx(REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey, "Start Minimized", 0, REG_DWORD, (UCHAR *)&StartMinimized, 4);
		retCode = RegSetValueEx(hKey, "Minimize to Tray", 0, REG_DWORD, (UCHAR *)&MinimizetoTray, 4);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
    POINT pos;
	HWND handle;
	RECT cRect;

	switch (message)
	{
	case WM_MDIACTIVATE:
				 
	// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			// GetSubMenu function should retrieve a handle to the drop-down menu or submenu.

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hConsMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);
		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		}
	 
		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	case MY_TRAY_ICON_MESSAGE:
			
			switch(lParam)
			{
			case WM_RBUTTONUP:	
			case WM_LBUTTONUP:

				GetCursorPos(&pos);

				SetForegroundWindow(hWnd);

				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return 0;
			}

			break;

		case WM_CTLCOLORDLG:
		   return (LONG)bgBrush;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			if (wmId == IDC_ENIGATE)
			{
				int retCode, disp;
				HKEY hKey=0;

				IGateEnabled = IsDlgButtonChecked(hWnd, IDC_ENIGATE); 

				if (IGateEnabled)
					ISDelayTimer = 60;

				retCode = RegCreateKeyEx(REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

				if (retCode == ERROR_SUCCESS)
				{
					retCode = RegSetValueEx(hKey,"IGateEnabled", 0 , REG_DWORD,(BYTE *)&IGateEnabled, 4);
					RegCloseKey(hKey);
				}

				return 0;
			}		

			if (wmId == BPQSAVENODES)
			{
				SaveNodes();
				WritetoConsole("Nodes Saved\n");
				return 0;
			}		
			if (wmId == BPQCLEARRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}
		
				ClearNodes();
				WritetoConsole("Nodes file Cleared\n");
				ReconfigFlag=TRUE;	
				WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}
			if (wmId == BPQRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}
				SaveNodes();
				WritetoConsole("Nodes Saved\n");
				ReconfigFlag=TRUE;	
				WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}

			if (wmId == SCANRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}

				RigReconfigFlag = TRUE;	
				WritetoConsole("Rigcontrol Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}

			if (wmId == APRSRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}

				APRSReconfigFlag=TRUE;	
				WritetoConsole("APRS Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}
			if (wmId == BPQDUMP)
			{
				DumpSystem();
				return 0;
			}

			if (wmId == BPQCLOSEALL)
			{
				CloseAllPrograms();
				return 0;
			}

			if (wmId == BPQUICONFIG)
			{
				int err, i=0;
				char Title[80];
				WNDCLASS  wc;
	
				wc.style = CS_HREDRAW | CS_VREDRAW;
				wc.lpfnWndProc = UIWndProc;       
				wc.cbClsExtra = 0;                
				wc.cbWndExtra = DLGWINDOWEXTRA;
				wc.hInstance = hInstance;
				wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
				wc.hCursor = LoadCursor(NULL, IDC_ARROW);
				wc.hbrBackground = bgBrush; 

				wc.lpszMenuName = NULL;	
				wc.lpszClassName = UIClassName; 

				RegisterClass(&wc);

				UIhWnd = CreateDialog(hInstance, UIClassName,0,NULL);

				if (!UIhWnd)
				{	
					err=GetLastError();
					return FALSE;
				}

				wsprintf(Title,"BPQ32 Beacon Utility Version");
				MySetWindowText(UIhWnd, Title);
				return 0;
			}

			if (wmId == BPQSAVEREG)
			{
				CreateRegBackup();
				return 0;
			}

			if (wmId == BPQMINTOTRAY)
			{
				MinimizetoTray = !MinimizetoTray;
				
				if (MinimizetoTray)
					CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_CHECKED);
				else
					CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_UNCHECKED);

				SaveConfig();
				return 0;
			}

			if (wmId == BPQSTARTMIN)
			{
				StartMinimized = !StartMinimized;
				
				if (StartMinimized)
					CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_CHECKED);
				else
					CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_UNCHECKED);

				SaveConfig();
				return 0;
			}

			if (wmId >= TRAYBASEID && wmId < (TRAYBASEID + 100))
			{ 
				handle=hWndArray[wmId-TRAYBASEID];

				if (handle == FrameWnd && FrameMaximized == TRUE)
					PostMessage(handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				else
					PostMessage(handle, WM_SYSCOMMAND, SC_RESTORE, 0);

				SetForegroundWindow(handle);
				return 0;
			}		
	
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!
	
			switch (wmId)
			{ 
			case  SC_MINIMIZE: 

				ConsoleMinimized = TRUE;
				break;

			case  SC_RESTORE: 

				ConsoleMinimized = FALSE;
				SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);

				break;
			}

			return DefMDIChildProc(hWnd, message, wParam, lParam);
	

		case WM_SIZE:

		GetClientRect(hWnd, &cRect); 

		MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);

		if (APRSActive)
			MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
		else
			MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);

//		InvalidateRect(hWnd, NULL, TRUE);
		break;

/*
		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
			for (i=0; i<SCREENLEN; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*LINELEN],LINELEN);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        
*/

		case WM_DESTROY:
		
//			SessionControl(Stream, 2, 0);
//			DeallocateStream(Stream);
//			PostQuitMessage(0);
			
			break;

		case WM_CHAR:
		
			if (wParam == 03)
			{
				DumpSystem();	
				return 0;
			}

 		case WM_CLOSE:
		
			break;

		default:
	
			return DefMDIChildProc(hWnd, message, wParam, lParam);


	}
			
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}

DllExport BOOL APIENTRY GetMinimizetoTrayFlag()
{
	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == (VOID *)-1)			// Init failed
		exit(0);
	
	return MinimizetoTray;
}

DllExport BOOL APIENTRY GetStartMinimizedFlag()
{
	return StartMinimized;
}

DllExport int APIENTRY AddTrayMenuItem(HWND hWnd, char * Label)
{
	int i;
	
	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == 0)
		{
			hWndArray[i]=hWnd;
			PIDArray[i] = GetCurrentProcessId();
			strcpy(PopupText[i],Label);
			AppendMenu(trayMenu,MF_STRING,TRAYBASEID+i,Label);
			CreateNewTrayIcon();
			return 0;
		}
	}
	return -1;
}
 
DllExport int APIENTRY DeleteTrayMenuItem(HWND hWnd)
{
	int i;
	
	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == hWnd)
		{
			hWndArray[i] = 0;
			PIDArray[i] = 0;
			DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
			CreateNewTrayIcon();
			return 0;
		}
	}
	return -1;
}

int WritetoConsoleLocal(char * buff);

DllExport int APIENTRY WritetoConsole(char * buff)
{
	return WritetoConsoleLocal(buff);
}

DllExport VOID * APIENTRY GetBuff();

int WritetoConsoleLocal(char * buff)
{
	int len=strlen(buff);
	UINT * buffptr;

	if (Semaphore.Flag == 0)
		return WritetoConsoleSupport(buff);

	buffptr = GetBuff();
	if (buffptr == 0)	// No buffers, so send direct
		return WritetoConsoleSupport(buff);

	if (len > 300)
		len = 300;

	memcpy(&buffptr[2], buff, len + 1);
	
	C_Q_ADD(&WritetoConsoleQ, buffptr);

	return 0;
}

int WritetoConsoleSupport(char * buff)
{

	int len=strlen(buff);
	char Temp[2000]= "";
	char * ptr;

	if (PartLine)
	{
		SendMessage(hWndCons, LB_GETTEXT, pindex, (LPARAM)(LPCTSTR) Temp);
		SendMessage(hWndCons, LB_DELETESTRING, pindex, 0);
		PartLine = FALSE;
	}

	if ((strlen(Temp) + strlen(buff)) > 1990)
		Temp[0] = 0;			// Should never have anything this long
	
	strcat(Temp, buff);

	ptr = strchr(Temp, '\n');

	if (ptr)
		*ptr = 0;
	else
		PartLine = TRUE;

	pindex=SendMessage(hWndCons, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Temp);
	return 0;
 }

DllExport VOID APIENTRY  BPQOutputDebugString(char * String)
{
	OutputDebugString(String);
	return;
 }

HANDLE handle;
char fn[]="BPQDUMP";
ULONG cnt;
char * stack;
//char screen[1920];
//COORD ReadCoord;

#define DATABYTES 400000

extern UCHAR DATAAREA[];

DllExport int APIENTRY  DumpSystem()
{
	char fn[200];
	char Msg[250];

	sprintf(fn,"%s\\BPQDUMP",BPQDirectory);

	handle = CreateFile(fn,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

#ifndef _WIN64

	_asm {

	mov	stack,esp
	}

	WriteFile(handle,stack,128,&cnt,NULL);
#endif

//	WriteFile(handle,Screen,MAXLINELEN*MAXSCREENLEN,&cnt,NULL);

	WriteFile(handle,DATAAREA, DATABYTES,&cnt,NULL);

 	CloseHandle(handle);

	sprintf(Msg, "Dump to %s Completed\n", fn);
	WritetoConsole(Msg);

	FindLostBuffers();

	return (0);
}

BOOLEAN CheckifBPQ32isLoaded()
{
	HANDLE Mutex;
	
	// See if BPQ32 is running - if we create it in the NTVDM address space by
	// loading bpq32.dll it will not work.

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)
	{	
		if (AttachingProcess == 0)			// Already starting BPQ32
		{
			OutputDebugString("BPQ32 No other bpq32 programs running - Loading BPQ32.exe\n");
			StartBPQ32();
		}
		return FALSE;
	}

	CloseHandle(Mutex);

	return TRUE;
}

BOOLEAN StartBPQ32()
{
	UCHAR Value[100];

	char bpq[]="BPQ32.exe";
	char *fn=(char *)&bpq;
	HKEY hKey=0;
	int ret,Type,Vallen=99;

	char Errbuff[100];
	char buff[20];		

	STARTUPINFO  StartupInfo;					// pointer to STARTUPINFO 
    PROCESS_INFORMATION  ProcessInformation; 	// pointer to PROCESS_INFORMATION 

	AttachingProcess = 1;

// Get address of BPQ Directory

	Value[0]=0;

	ret = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (ret == ERROR_SUCCESS)
	{
		ret = RegQueryValueEx(hKey, "BPQ Program Directory", 0, &Type,(UCHAR *)&Value, &Vallen);
		
		if (ret == ERROR_SUCCESS)
		{
			if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
				Value[0]=0;
		}


		if (Value[0] == 0)
		{
		
			// BPQ Directory absent or = "" - "try Config File Location"
			
			ret = RegQueryValueEx(hKey,"BPQ Directory",0,			
							&Type,(UCHAR *)&Value,&Vallen);

			if (ret == ERROR_SUCCESS)
			{
				if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
					Value[0]=0;
			}

		}
		RegCloseKey(hKey);
	}
				
	if (Value[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcat(Value,"\\");
		strcat(Value,fn);				
	}

	StartupInfo.cb=sizeof(StartupInfo);
	StartupInfo.lpReserved=NULL; 
	StartupInfo.lpDesktop=NULL; 
	StartupInfo.lpTitle=NULL; 
	StartupInfo.dwFlags=0; 
	StartupInfo.cbReserved2=0; 
  	StartupInfo.lpReserved2=NULL; 

	if (!CreateProcess(Value,NULL,NULL,NULL,FALSE,
							CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
							NULL,NULL,&StartupInfo,&ProcessInformation))
	{				
		ret=GetLastError();

		_itoa(ret,buff,10);

		strcpy(Errbuff,	"BPQ32 Load ");
		strcat(Errbuff,Value);
		strcat(Errbuff," failed ");
		strcat(Errbuff,buff);
		OutputDebugString(Errbuff);
		AttachingProcess = 0;
		return FALSE;		
	}

	return TRUE;
}


DllExport BPQVECSTRUC * APIENTRY GetIPVectorAddr()
{
	return &IPHOSTVECTOR;
}

DllExport UINT APIENTRY GETSENDNETFRAMEADDR()
{
	return (UINT)&SENDNETFRAME;
}

DllExport VOID APIENTRY RelBuff(VOID * Msg)
{
	UINT * pointer, * BUFF = Msg;

	if (Semaphore.Flag == 0)
		Debugprintf("ReleaseBuffer called without semaphore");
	
	pointer = FREE_Q;

	*BUFF =(UINT)pointer;

	FREE_Q = BUFF;

	QCOUNT++;

	return;
}

extern int MINBUFFCOUNT;

DllExport VOID * APIENTRY GetBuff()
{
	UINT * Temp = Q_REM(&FREE_Q);

	if (Semaphore.Flag == 0)
		Debugprintf("GetBuff called without semaphore");

	if (Temp)
	{
		QCOUNT--;

		if (QCOUNT < MINBUFFCOUNT)
			MINBUFFCOUNT = QCOUNT;
	}
	
	return Temp;
}


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

unsigned short int compute_crc(unsigned char *buf, int txlen);

extern SOCKADDR_IN reportdest;

extern SOCKET ReportSocket;

extern SOCKADDR_IN Chatreportdest;

DllExport VOID APIENTRY SendChatReport(SOCKET ChatReportSocket, char * buff, int txlen)
{
 	unsigned short int crc = compute_crc(buff, txlen);

	crc ^= 0xffff;

	buff[txlen++] = (crc&0xff);
	buff[txlen++] = (crc>>8);

	sendto(ChatReportSocket, buff, txlen, 0, (LPSOCKADDR)&Chatreportdest, sizeof(Chatreportdest));
}

VOID CreateRegBackup()
{
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];
	char RegFileName[MAX_PATH];
	char Msg[80];
	HANDLE handle;
	int len, written;
	char RegLine[300];

//	SHELLEXECUTEINFO   sei;
//	STARTUPINFO SInfo;
//	PROCESS_INFORMATION PInfo;

	sprintf(RegFileName, "%s\\BPQ32.reg", BPQDirectory);

	// Keep 4 Generations

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, RegFileName);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, RegFileName);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		//Move .bak to .bak.1

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak");

	CopyFile(RegFileName, Backup2, FALSE);	// Copy to .bak

	handle = CreateFile(RegFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		sprintf(Msg, "Failed to open Registry Save File\n");
		WritetoConsole(Msg);
		return;
	}

	len = sprintf(RegLine, "Windows Registry Editor Version 5.00\r\n\r\n");
	WriteFile(handle, RegLine, len, &written, NULL);

	if (SaveReg("Software\\G8BPQ\\BPQ32", handle))
		WritetoConsole("Registry Save complete\n");
	else
		WritetoConsole("Registry Save failed\n");

	CloseHandle(handle);			
	return ;
/*

	if (REGTREE == HKEY_LOCAL_MACHINE)		// < Vista
	{
		sprintf(cmd,
			"regedit /E \"%s\\BPQ32.reg\" %s\\Software\\G8BPQ\\BPQ32", BPQDirectory, REGTREETEXT);

		ZeroMemory(&SInfo, sizeof(SInfo));

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0 ,NULL, NULL, &SInfo, &PInfo) == 0)
		{
			sprintf(Msg, "Error: CreateProcess for regedit failed 0%d\n", GetLastError() );
		   	WritetoConsole(Msg);
			return;
		}
	}
	else
	{

		sprintf(cmd,
			"/E \"%s\\BPQ32.reg\" %s\\Software\\G8BPQ\\BPQ32", BPQDirectory, REGTREETEXT);	

	    ZeroMemory(&sei, sizeof(sei));

		sei.cbSize          = sizeof(SHELLEXECUTEINFOW);
		sei.hwnd            = hWnd;
		sei.fMask           = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
		sei.lpVerb          = "runas";
		sei.lpFile          = "regedit.exe";
		sei.lpParameters    = cmd;
		sei.nShow           = SW_SHOWNORMAL;

		if (!ShellExecuteEx(&sei))
		 {
		    sprintf(Msg, "Error: ShellExecuteEx for regedit failed %d\n", GetLastError() );
		   	WritetoConsole(Msg);
			return;
		}
	}

	sprintf(Msg, "Registry Save Initiated\n", fn);
	WritetoConsole(Msg);
			
	return ;
*/
}

BOOL CALLBACK EnumForCloseProc(HWND hwnd, LPARAM  lParam)
{
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowThreadProcessId(hwnd, &ProcessId);

	for (i=0; i< AttachedProcesses; i++)
	{
		if (AttachedPIDList[i] == ProcessId)
		{
			Debugprintf("BPQ32 Close All Closing PID %d", ProcessId);
			PostMessage(hwnd, WM_CLOSE, 1, 1);
	//		AttachedPIDList[i] = 0;				// So we don't do it again
			break;
		}
	}
	
	return (TRUE);
}
DllExport BOOL APIENTRY RestoreFrameWindow()
{
	return 	ShowWindow(FrameWnd, SW_RESTORE);
}

DllExport VOID APIENTRY CreateNewTrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE,&niData);
	trayMenu = NULL;
}

DllExport VOID APIENTRY CloseAllPrograms()
{
//	HANDLE hProc;

	// Close all attached BPQ32 programs

	Closing  = TRUE;

	ShowWindow(FrameWnd, SW_RESTORE);

	GetWindowRect(FrameWnd, &FRect);

	SaveBPQ32Windows();
	CloseHostSessions();

	if (AttachedProcesses == 1)
		CloseBPQ32();
		
	Debugprintf("BPQ32 Close All Processes %d PIDS %d %d %d %d", AttachedProcesses, AttachedPIDList[0],
		AttachedPIDList[1], AttachedPIDList[2], AttachedPIDList[3]);

	if (MinimizetoTray)
		Shell_NotifyIcon(NIM_DELETE,&niData);
	
	EnumWindows(EnumForCloseProc, (LPARAM)NULL);
}

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define MAX_VALUE_DATA 65536

BOOL CopyReg(HKEY hKeyIn, HKEY hKeyOut)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string 
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
 
    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
 
    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKeyIn,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 
 
    // Enumerate the subkeys, until RegEnumKeyEx fails.
    
    if (cSubKeys)
    {
        Debugprintf( "\nNumber of subkeys: %d\n", cSubKeys);

        for (i=0; i<cSubKeys; i++) 
        { 
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKeyIn, i,
                     achKey, 
                     &cbName, 
                     NULL, 
                     NULL, 
                     NULL, 
                     &ftLastWriteTime); 

            if (retCode == ERROR_SUCCESS) 
            {
                HKEY NextKeyIn;
                HKEY NextKeyOut;
				int disp;

				Debugprintf(TEXT("(%d) %s\n"), i+1, achKey);
				retCode = RegOpenKeyEx(hKeyIn, achKey, 0, KEY_READ, &NextKeyIn);
				retCode += RegCreateKeyEx(hKeyOut, achKey, 0, 0, 0, KEY_ALL_ACCESS, NULL, &NextKeyOut, &disp);

				if (retCode == 0)
					CopyReg(NextKeyIn, NextKeyOut);
            }
        }
    } 
 
    // Enumerate the key values. 

    if (cValues) 
    {
        Debugprintf( "\nNumber of values: %d\n", cValues);

        for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
        { 
			int Type;
			int ValLen = MAX_VALUE_DATA;
			UCHAR Value[MAX_VALUE_DATA] = "";

            cchValue = MAX_VALUE_NAME; 
            achValue[0] = '\0'; 
            retCode = RegEnumValue(hKeyIn, i, 
                achValue, 
                &cchValue, 
                NULL, 
                &Type,
                &Value[0],
                &ValLen);
 
            if (retCode == ERROR_SUCCESS ) 
            { 
				if (ValLen == 4)
					Debugprintf(TEXT("(%d) %s = %x len %d\n"), i+1, achValue, *Value, ValLen); 
				else
					Debugprintf(TEXT("(%d) %s = %s len %d\n"), i+1, achValue, Value, ValLen); 

				retCode = RegSetValueEx(hKeyOut, achValue, 0 , Type, (BYTE *)&Value[0], ValLen);
            } 
        }
    }
	return TRUE;
}


DllExport BOOL APIENTRY SaveReg(char * KeyIn, HANDLE hFile)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string 
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
	HKEY hKeyIn;
    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
	int len, written;
	char RegLine[300];
	char * ptr1, * ptr2;
	char c;

	retCode = RegOpenKeyEx (REGTREE, KeyIn, 0, KEY_READ, &hKeyIn);
	
	if (retCode != ERROR_SUCCESS)
	{
		Debugprintf("Open Reg Key %s failed", KeyIn);
		return FALSE;
	}

	len = sprintf(RegLine, "[%s\\%s]\r\n", REGTREETEXT, KeyIn);

	WriteFile(hFile, RegLine, len, &written, NULL);
 
    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKeyIn,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 

	    // Enumerate the key values. 

    if (cValues) 
	{
        for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
		{
			int Type, k;
			int ValLen = MAX_VALUE_DATA;
			UCHAR Value[MAX_VALUE_DATA] = "";
			UCHAR ValCopy[MAX_VALUE_DATA];

			UINT Intval;

            cchValue = MAX_VALUE_NAME; 
            achValue[0] = '\0'; 
            retCode = RegEnumValue(hKeyIn, i, 
                achValue, 
                &cchValue, 
                NULL, 
                &Type,
                &Value[0],
                &ValLen);
 
            if (retCode == ERROR_SUCCESS ) 
            {
				// Encode the param depending on Type

				    switch(Type)
					{
					case REG_NONE:						//( 0 )   // No value type
						break;
					case REG_SZ:						//( 1 )   // Unicode nul terminated string
						
						// Need to escape any \ or " in Value
						
						ptr1 = Value;
						ptr2 = ValCopy;

						c = *ptr1++;

						while (c)
						{	
							switch (c)
							{
								case '\\':
								case '"':
									*ptr2++ = '\\';
							}
							*ptr2++ = c;
							c = *ptr1++;
						}
						*ptr2 = 0;
					
						len = sprintf(RegLine, "\"%s\"=\"%s\"\r\n", achValue, ValCopy);
						break;

					case REG_EXPAND_SZ:					//( 2 )   // Unicode nul terminated string
														// (with environment variable references)
						break;

					case REG_BINARY:					//( 3 )   // Free form binary - hex:86,50 etc

						len = sprintf(RegLine, "\"%s\"=hex:%02x,", achValue, Value[0]);
						for (k = 1; k < ValLen; k++)
						{
							if (len > 76)
							{
								len = sprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
								len = 2;
							}

							len = sprintf(RegLine, "%s%02x,", RegLine, Value[k]);
						}
						RegLine[--len] = 0x0d;
						RegLine[++len] = 0x0a;	
						len++;

						break;

					case REG_DWORD:						//( 4 )   // 32-bit number
//					case REG_DWORD_LITTLE_ENDIAN:		//( 4 )   // 32-bit number (same as REG_DWORD)
					
						memcpy(&Intval, Value, 4);
						len = sprintf(RegLine, "\"%s\"=dword:%08x\r\n", achValue, Intval);
					break;
				
					case REG_DWORD_BIG_ENDIAN:			//( 5 )   // 32-bit number
						break;
					case REG_LINK:						//( 6 )   // Symbolic Link (unicode)
						break;
					case REG_MULTI_SZ:					//( 7 )   // Multiple Unicode strings

						len = sprintf(RegLine, "\"%s\"=hex(7):%02x,00,", achValue, Value[0]);
						for (k = 1; k < ValLen; k++)
						{
							if (len > 76)
							{
								len = sprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
								len = 2;
							}
							len = sprintf(RegLine, "%s%02x,", RegLine, Value[k]);
							if (len > 76)
							{
								len = sprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
							}
							len = sprintf(RegLine, "%s00,", RegLine);
						}

						RegLine[--len] = 0x0d;
						RegLine[++len] = 0x0a;	
						len++;
						break;

					case REG_RESOURCE_LIST:				//( 8 )   // Resource list in the resource map
						break;
					case REG_FULL_RESOURCE_DESCRIPTOR:	//( 9 )  // Resource list in the hardware description
						break;
					case REG_RESOURCE_REQUIREMENTS_LIST://( 10 )
						break;
					case REG_QWORD:						//( 11 )  // 64-bit number
//					case REG_QWORD_LITTLE_ENDIAN:		//( 11 )  // 64-bit number (same as REG_QWORD)
						break;

					}
				
				WriteFile(hFile, RegLine, len, &written, NULL);
            } 
        }
	}	
	
	WriteFile(hFile, "\r\n", 2, &written, NULL);
	
    // Enumerate the subkeys, until RegEnumKeyEx fails.
    
    if (cSubKeys)
    {
        for (i=0; i<cSubKeys; i++) 
        { 
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKeyIn, i,
                     achKey, 
                     &cbName, 
                     NULL, 
                     NULL, 
                     NULL, 
                     &ftLastWriteTime); 

            if (retCode == ERROR_SUCCESS) 
            {
                char NextKeyIn[MAX_KEY_LENGTH];

				sprintf(NextKeyIn, "%s\\%s", KeyIn, achKey);
				SaveReg(NextKeyIn, hFile);
            }
        }
    } 
	return TRUE;
}

char Screen[4000];
char NewScreen[4000];

int DoStatus()
{
	int i;
	char callsign[12] = "";
	char flag[3];
	UINT Mask, MaskCopy;
	int Flags;
	int AppNumber;
	int OneBits;
	struct _EXCEPTION_POINTERS exinfo;

	memset(NewScreen, ' ', 33 * 108); 

	strcpy(NewScreen,"    RX  TX MON App Flg Callsign  Program                  RX  TX MON App Flg Callsign  Program");

	strcpy(EXCEPTMSG, "Status Timer Processing");
	__try
	{
	for (i=1;i<65; i++)
	{		
		callsign[0]=0;
		
		if (GetAllocationState(i))

			strcpy(flag,"*");
		else
			strcpy(flag," ");

		GetCallsign(i,callsign);

		Mask = MaskCopy = Get_APPLMASK(i);

		// if only one bit set, convert to number

		AppNumber = 0;
		OneBits = 0;

		while (MaskCopy)
		{
			if (MaskCopy & 1)
				OneBits++;
			
			AppNumber++;
			MaskCopy = MaskCopy >> 1;
		}

		Flags=GetApplFlags(i);

		if (OneBits > 1)
			sprintf(&NewScreen[(i+1)*54],"%2d%s%3d %3d %3d %03x %3x %10s%-20s",
				i, flag, RXCount(i), TXCount(i), MONCount(i), Mask, Flags, callsign,
				BPQHOSTVECTOR[i-1].PgmName);
		else
			sprintf(&NewScreen[(i+1)*54],"%2d%s%3d %3d %3d %3d %3x %10s%-20s",
				i, flag, RXCount(i), TXCount(i), MONCount(i), AppNumber, Flags, callsign,
				BPQHOSTVECTOR[i-1].PgmName);

	}
	}

	#include "StdExcept.c"

	if (Semaphore.Flag && Semaphore.SemProcessID == GetCurrentProcessId())
		FreeSemaphore(&Semaphore);

	}

	if (memcmp(Screen, NewScreen, 33 * 108) == 0)	// No Change
		return 0;

	memcpy(Screen, NewScreen, 33 * 108);
	InvalidateRect(StatusWnd,NULL,FALSE);

	return(0);
}

LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;
	HGLOBAL	hMem;
	MINMAXINFO * mmi;
	int i;

	switch (message)
	{
	case WM_TIMER:

		if  (Semaphore.Flag == 0)
			DoStatus();
		break;

	case WM_MDIACTIVATE:
				 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hConsMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);
		}
		else
		{
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		}
	 
		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_GETMINMAXINFO:
			
		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 850;
		mmi->ptMaxSize.y = 500;
		mmi->ptMaxTrackSize.x = 850;
		mmi->ptMaxTrackSize.y = 500;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		switch (wmId)
		{

/*
			case BPQSTREAMS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_UNCHECKED);

				StreamDisplay = TRUE;

				break;

			case BPQIPSTATUS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_UNCHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_CHECKED);

				StreamDisplay = FALSE;
				memset(Screen, ' ', 4000); 


				break;

*/
	
			case BPQCOPY:
		
			//
			//	Copy buffer to clipboard
			//
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 33*110);
		
			if (hMem != 0)
			{
				if (OpenClipboard(hWnd))
				{
//					CopyScreentoBuffer(GlobalLock(hMem));
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
				else
				{
					GlobalFree(hMem);
				}

			}

			break;

		}
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case  SC_MAXIMIZE: 

			break;

		case  SC_MINIMIZE: 

			StatusMinimized = TRUE;
			break;

		case  SC_RESTORE: 

			StatusMinimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			break;
		}

		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
			for (i=0; i<33; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*108],108);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        

		case WM_DESTROY:
		
//			PostQuitMessage(0);
			
			break;


		default:
		
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	}
	return (0);
}

VOID SaveMDIWindowPos(HWND hWnd, char * RegKey, char * Value, BOOL Minimized)
{
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;

	if (IsWindow(hWnd) == FALSE)
		return;

	ShowWindow(hWnd, SW_RESTORE);

	if (GetWindowRect(hWnd, &Rect) == FALSE)
		return;

	// Make relative to Frame

	Rect.top -= FRect.top ;
	Rect.left -= FRect.left;
	Rect.bottom -= FRect.top;
	Rect.right -= FRect.left;

	sprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\%s", RegKey);
	
	retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0,
            KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		sprintf(Size,"%d,%d,%d,%d,%d", Rect.left, Rect.right, Rect.top ,Rect.bottom, Minimized);
		retCode = RegSetValueEx(hKey, Value, 0, REG_SZ,(BYTE *)&Size, strlen(Size));
		RegCloseKey(hKey);
	}
}

extern int GPSPort;
extern char LAT[];			// in standard APRS Format      
extern char LON[];			// in standard APRS Format

VOID SaveBPQ32Windows()
{
	HKEY hKey=0;
	char Size[80];
	int retCode, disp;
	PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	int i;

	retCode = RegCreateKeyEx(REGTREE, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		sprintf(Size,"%d,%d,%d,%d", FRect.left, FRect.right, FRect.top, FRect.bottom);
		retCode = RegSetValueEx(hKey, "FrameWindowSize", 0, REG_SZ, (BYTE *)&Size, strlen(Size));

		// Save GPS Position

		if (GPSPort)
		{
			sprintf(Size, "%s, %s", LAT, LON);
			retCode = RegSetValueEx(hKey, "GPS", 0, REG_SZ,(BYTE *)&Size, strlen(Size));
		}

		RegCloseKey(hKey);
	}
	
	SaveMDIWindowPos(StatusWnd, "", "StatusWindowSize", StatusMinimized);
	SaveMDIWindowPos(hConsWnd, "", "WindowSize", ConsoleMinimized);

	for (i=0; i<NUMBEROFPORTS; i++)
	{
		if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
		{
			if (PORTVEC->PORT_EXT_ADDR)
			{
				SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
				SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
			}
		}
		PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
	}

	SaveWindowPos(70);		// Rigcontrol


	if (hIPResWnd)
		SaveMDIWindowPos(hIPResWnd, "", "IPResSize", IPMinimized);

	SaveHostSessions();
}

DllExport BOOL APIENTRY CheckIfOwner()
{
	//
	//	Returns TRUE if current process is root process
	//	that loaded the DLL
	//
	
	if (TimerInst == GetCurrentProcessId())

		return (TRUE);
	else
		return (FALSE);	
}

VOID GetParam(char * input, char * key, char * value)
{
	char * ptr = strstr(input, key);
	char Param[2048];
	char * ptr1, * ptr2;
	char c;


	if (ptr)
	{
		ptr2 = strchr(ptr, '&');
		if (ptr2) *ptr2 = 0;
		strcpy(Param, ptr + strlen(key));
		if (ptr2) *ptr2 = '&';					// Restore string

		// Undo any % transparency

		ptr1 = Param;
		ptr2 = Param;

		c = *(ptr1++);

		while (c)
		{
			if (c == '%')
			{
				int n;
				int m = *(ptr1++) - '0';
				if (m > 9) m = m - 7;
				n = *(ptr1++) - '0';
				if (n > 9) n = n - 7;

				*(ptr2++) = m * 16 + n;
			}
			else if (c == '+')
				*(ptr2++) = ' ';
			else
				*(ptr2++) = c;

			c = *(ptr1++);
		}

		*(ptr2++) = 0;

		strcpy(value, Param);
	}
}

int GetListeningPortsPID(int Port)
{
	MIB_TCPTABLE_OWNER_PID * TcpTable = NULL;
	PMIB_TCPROW_OWNER_PID Row;
	int dwSize = 0;
	DWORD n;

	// Get PID of process for this TCP Port

	// Get Length of table
	
	GetExtendedTcpTable(TcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);

	TcpTable = malloc(dwSize);

	if (TcpTable == NULL)
		return 0;

	GetExtendedTcpTable(TcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);

	for (n = 0; n < TcpTable->dwNumEntries; n++)
	{
		Row = &TcpTable->table[n];
		
		if (Row->dwLocalPort == Port && Row->dwState == MIB_TCP_STATE_LISTEN)
		{
			return Row->dwOwningPid;
			break;
		}
	}
	return 0;			// Not found
}

DllExport char *  APIENTRY GetLOC()
{
	return LOC;
}

// UZ7HO Dll PTT interface

// 1 ext_PTT_info
// 2 ext_PTT_settings
// 3 ext_PTT_OFF
// 4 ext_PTT_ON
// 5 ext_PTT_close
// 6 ext_PTT_open

extern struct RIGINFO * DLLRIG;			// Rig record for dll PTT interface (currently only for UZ7HO);

VOID Rig_PTT(struct TNCINFO * TNC, BOOL PTTState);
VOID Rig_PTTEx(struct RIGINFO * RIG, BOOL PTTState, struct TNCINFO * TNC);

int WINAPI ext_PTT_info()
{
	return 0;
}

int WINAPI ext_PTT_settings()
{
	return 0;
}

int WINAPI ext_PTT_OFF(int Port)
{
	if (DLLRIG)
		Rig_PTTEx(DLLRIG, 0, 0);

	return 0;
}

int WINAPI ext_PTT_ON(int Port)
{
	if (DLLRIG)
		Rig_PTTEx(DLLRIG, 1, 0);

	return 0;
}
int WINAPI ext_PTT_close()
{
	if (DLLRIG)
		Rig_PTTEx(DLLRIG, 0, 0);

	return 0;
}

DllExport INT WINAPI ext_PTT_open()
{
	return 1;
}

char * stristr (char *ch1, char *ch2)
{
	char	*chN1, *chN2;
	char	*chNdx;
	char	*chRet = NULL;

	chN1 = _strdup(ch1);
	chN2 = _strdup(ch2);

	if (chN1 && chN2)
	{
		chNdx = chN1;
		while (*chNdx)
		{
			*chNdx = (char) tolower(*chNdx);
			chNdx ++;
		}
		chNdx = chN2;

		while (*chNdx)
		{
			*chNdx = (char) tolower(*chNdx);
			chNdx ++;
		}

		chNdx = strstr(chN1, chN2);

		if (chNdx)
			chRet = ch1 + (chNdx - chN1);
	}

	free (chN1);
	free (chN2);
	return chRet;
}

