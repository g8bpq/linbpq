// Mail and Chat Server for BPQ32 Packet Switch
//
//

// Version 1.0.0.17re

//	Split Messasge, User and BBS Editing from Main Config.
//	Add word wrap to Console input and output
//  Flash Console on chat user connect
//	Fix processing Name response in chat mode
//	Fix processing of *RTL from station not defined as a Chat Node
//	Fix overlength lines ln List responses
//  Housekeeping expires BIDs 
//  Killing a message removes it from the forwarding counts

// Version 1.0.0.18

// Save User Database when name is entered or updated so it is not lost on a crash
// Fix Protocol Error in Compressed Forwarding when switching direction
// Add Housekeeping results dialog.

// Version 1.0.0.19

// Allow PACLEN in forward scripts.
// Store and forward messages with CRLF as line ends
// Send Disconnect after FQ ( for LinFBB)
// "Last Listed" is saved if MailChat is closed without closing Console
// Maximum acceptable message length can be specified (in Forwarding Config)

// Version 1.0.0.20

// Fix error in saving forwarding config (introduced in .19)
// Limit size of FBB forwarding block.
// Clear old connection (instead of new) if duplicate connect on Chat Node-Node link
// Send FA for Compressed Mail (was sending FB for both Compressed and Uncompressed)

// Version 1.0.0.21

// Fix Connect Script Processing (wasn't waiting for CONNECTED from last step)
// Implement Defer
// Fix MBL-style forwarding
// Fix Add User (Params were not saved)
// Add SC (Send Copy) Command
// Accept call@bbs as well as call @ bbs

// Version 1.0.0.22

// Implement RB RP LN LR LF LN L$ Commands.
// Implement QTH and ZIP Commands.
// Entering an empty Title cancels the message.
// Uses HomeBBS field to set @ field for local users.
// Creates basic WP Database.
// Uses WP to lookup @ field for non-local calls.
// Console "Actions" Menu renamed "Options".
// Excluded flag is actioned.
// Asks user to set HomeBBS if not already set.
// Fix "Shrinking Message" problem, where message got shorter each time it was read Initroduced in .19).
// Flash Server window when anyone connects to chat (If Console Option "Flash on Chat User Connect" set).

// Version 1.0.0.23

// Fix R: line scan bug

// Version 1.0.0.24

// Fix closing console window on 'B'.
// Fix Message Creation time.
// Enable Delete function in WP edit dialog

// Version 1.0.0.25

// Implement K< and K> commands
// Experimental support for B1 and B2 forwarding
// Experimental UI System
// Fix extracting QTH from WP updates

// Version 1.0.0.26

// Add YN etc responses for FBB B1/B2

// Version 1.0.0.27

// Fix crash if NULL received as start of a packet.
// Add Save WP command
// Make B2 flag BBS-specific.
// Implement B2 Send

// Version 1.0.0.28

// Fix parsing of smtp to addresses - eg smtp:john.wiseman@cantab.net
// Flag messages as Held if smtp server rejects from or to addresses
// Fix Kill to (K> Call)
// Edit Message dialog shows latest first
// Add chat debug window to try to track down occasional chat connection problems

// Version 1.0.0.29

// Add loads of try/excspt

// Version 1.0.0.30

// Writes Debug output to LOG_DEBUG_X and Monitor Window

// Version 1.0.0.32

// Allow use of GoogleMail for ISP functions
// Accept SYSOP as alias for SYSOPCall - ie user can do SP SYSOP, and it will appear in sysop's LM, RM, etc
// Email Housekeeping Results to SYSOP

// Version 1.0.0.33

// Housekeeping now runs at Maintenance Time. Maintenance Interval removed. 
// Allow multiple numbers on R and K commands
// Fix L command with single number
// Log if Forward count is out of step with messages to forward.
// UI Processing improved and F< command implemented

// Version 1.0.0.34

// Semaphore Chat Messages
// Display Semaphore Clashes
// More Program Error Traps
// Kill Messages more than BIDLifetime old

// Version 1.0.0.35

// Test for Mike - Remove B1 check from Parse_SID

// Version 1.0.0.36

// Fix calculation of Housekeeping Time.
// Set dialog box background explicitly.
// Remove tray entry for chat debug window.
// Add date to log file name.
// Add Actions Menu option to disable logging.
// Fix size of main window when it changes between versions.

// Version 1.0.0.37

// Implement Paging.
// Fix L< command (was giving no messages).
// Implement LR LR mmm-nnn LR nnn- (and L nnn-)
// KM should no longer kill SYSOP bulls.
// ISP interfaces allows SMTP Auth to be configured
// SMTP Client would fail to send any more messages if a connection failed

// Version 1.0.0.38

// Don't include killed messages in L commands (except LK!)
// Implement l@
// Add forwarding timebands
// Allow resizing of main window.
// Add Ver command.

// Version 1.0.1.1

// First Public Beta

// Fix part line handling in Console
// Maintenance deletes old log files.
// Add option to delete files to the recycle bin.

// Version 1.0.2.1

// Allow all Node SYSOP commands in connect scripts.
// Implement FBB B1 Protocol with Resume
// Make FBB Max Block size settable for each BBS.
// Add extra logging when Chat Sessions refused.
// Fix Crash on invalid housekeeping override.
// Add Hold Messages option.
// Trap CRT Errors
// Sort Actions/Start Forwarding List

// Version 1.0.2.2

// Fill in gaps in BBS Number sequence
// Fix PE if ctext contains }
// Run Houskeeping at startup if previous Housekeeping was missed

// Version 1.0.2.3

// Add configured nodes to /p listing

// Version 1.0.2.4

// Fix RMS (it wanted B2 not B12)
// Send messages if available after rejecting all proposals
// Dont try to send msg back to originator.

// Version 1.0.2.5

// Fix timeband processing when none specified.
// Improved Chat Help display.
// Add helpful responses to /n /q and /t

// Version 1.0.2.6

// Kill Personal WP messages after processing
// Make sure a node doesnt try to "join" or "leave" a node as a user.
// More tracing to try to track down lost topic links.
// Add command recall to Console
// Show users in new topic when changing topic
// Add Send From Clipboard" Action

// Version 1.0.2.7

// Hold messages from the future, or with invalid dates.
// Add KH (kill held) command.
// Send Message to SYSOP when a new user connects.

// Version 1.0.2.8

// Don't reject personal message on Dup BID unless we already have an unforwarded copy.
// Hold Looping messages.
// Warn SYSOP of held messages.

// Version 1.0.2.9

// Close connecton on receipt of *** DONE (MBL style forwarding).
// Improved validation in link_drop (Chat Node)
// Change to welcome prompt and Msg Header for Outpost.
// Fix Connect Script processing for KA Nodes

// Version 1.0.3.1

// Fix incorrect sending of NO - BID.
// Fix problems caused by a user being connected to more than one chat node.
// Show idle time on Chat /u display.
// Rewrite forwarding by HA.
// Add "Bad Words" Test.
// Add reason for holding to SYSOP "Message Held" Message.
// Make topics case-insensitive.
// Allow SR for smtp mail.
// Try to fix some user's "Add User" problem.


// Version 1.0.3.2

// Fix program error when prcessing - response in FBB forwarding.
// Fix code to flag messages as sent.


// Version 1.0.3.3

// Attempt to fix message loop on topic_change
// Fix loop if compressed size is greater than 32K when receiving with B1 protocol.
// Fix selection of B1

// Version 1.0.3.4

// Add "KISS ONLY" Flag to R: Lines (Needs Node Version 4.10.12 (4.10l) or above)
// Add Basic NNTP Interface
// Fix possible loop in lzhuf encode

// Version 1.0.3.5

// Fix forwarding of Held Messages
// More attempts to fix Chat crashes.
// Limit join/leave problem with mismatched nodes.
// Add Chat Node Monitoring System.
// Change order of elements in nntp addresses (now to.at, was at.to)

// Version 1.0.3.6

// Restart and Exit if too many errors
// Fix forwarding of killed messages.
// Fix Forwarding to PaKet.
// Fix problem if BBS signon contains words from the "Fail" list

// Version 1.0.3.7

// re-fix loop if compressed size is greater than 32K - reintroduced in 1.0.3.4
// Add last message to edit users
// Change Console and Monitor Buffer sizes
// Don't flag msg as 'Y' on read if it was Held or Killed

// Version 1.0.3.8

// Don't connect if all messages for a BBS are held.
// Hold message if From or To are missing.
// Fix parsing of /n and /q commands
// fix possible loop on changing name or qth

// Version 1.0.3.9

// More Chat fixes and monitoring
// Added additional console for chat

// Version 1.0.3.10

// Fix for corruption of CIrcuit-Node chain.

// Version 1.0.3.11

// Fix flow control for SMTP and NNTP 

// Version 1.0.3.12

// Fix crash in SendChatStatus if no Chat Links Defined.
// Disable Chat Mode if there is no ApplCall for ChatApplNum,
// Add Edit Message to Manage Messages Dialog
// NNTP needs authentication


// Version 1.0.3.13

// Fix Chat ApplCall warning when ChatAppl = 0
// Add NNTP NEWGROUPS Command
// Fix MBL Forwarding (remove extra > prompt after SP)

// Version 1.0.3.14

// Fix topic switch code.
// Send SYSOP messages on POP3 interface if User SYSOP flag is set.
// NNTP only needs Authentication for posting, not reading.

// Version 1.0.3.15

// Fix reset of First to Forward after househeeping

// Version 1.0.3.16

// Fix check of HA for terminating WW
// MBL Mode remove extra > prompts
// Fix program error if WP record has unexpected format
// Connect Script changes for WINMOR
// Fix typo in unconfigured node has connected message

// Version 1.0.3.17

// Fix forwarding of Personals 

// Version 1.0.3.18

// Fix detection of misconfigured nodes to work with new nodes.
// Limit connection attempt rate when a chat node is unavailable.
// Fix Program Error on long input lines (> ~250 chars).

// Version 1.0.3.19

// Fix Restart of B2 mode transfers.
// Fix error if other end offers B1 and you are configured for B2 only.


// Version 1.0.3.20

// Fix Paging in Chat Mode.
// Report Node Versions.

// Version 1.0.3.21

// Check node is not already known when processing OK
// Add option to suppress emailing of housekeeping results

// Version 1.0.3.22

// Correct Version processing when user connects via the network
// Add time controlled forwarding scripts

// Version 1.0.3.23

// Changes to RMS forwarding

// Version 1.0.3.24

// Fix RMS: from SMTP interface
// Accept RMS/ instead of RMS: for Thunderbird

// Version 1.0.3.25

// Accept smtp: addresses from smtp client, and route to ISP gateway.
// Set FROM address of messages from RMS that are delivered to smtp client so a reply will go back via RMS.

// Version 1.0.3.26

// Improve display of rms and smtp messages in message lists and message display.

// Version 1.0.3.27

// Correct code that prevents mail being retured to originating BBS.
// Tidy stuck Nodes and Topics when all links close
// Fix B2 handling of @ to TO Address.

// Version 1.0.3.28

// Ensure user Record for the BBS Call has BBS bit set.
// Don't send messages addressed @winlink.org if addressee is a local user with Poll RMS set.
// Add user configurable welcome messages.

// Version 1.0.3.29

// Add AUTH feature to Rig Control

// Version 1.0.3.30

// Process Paclink Header (;FW:)

// Version 1.0.3.31

// Process Messages with attachments.
// Add inactivity timeout to Chat Console sessions.

// Version 1.0.3.32

// Fix for Paclink > BBS Addresses 

// Version 1.0.3.33

// Fix multiple transfers per session for B2.
// Kill messages eent to paclink.
// Add option to forward messages on arrival.

// Version 1.0.3.34

// Fix bbs addresses to winlink.
// Fix adding @winlink.org to imcoming paclink msgs

// Version 1.0.3.35

// Fix bbs addresses to winlink. (Again)

// Version 1.0.3.36

// Restart changes for RMS/paclink

// Version 1.0.3.37

// Fix for RMS Express forwarding

// Version 1.0.3.38

// Fixes for smtp and lower case packet addresses from Airmail
// Fix missing > afer NO - Bid in MBL mode

// Version 1.0.3.39

// Use ;FW: for RMS polling.

// Version 1.0.3.40

// Add ELSE Option to connect scripts.

// Version 1.0.3.41

// Improved handling of Multiple Addresses
// Add user colours to chat.

// Version 1.0.3.42

// Poll multiple SSID's for RMS
// Colour support for BPQTEerminal
// New /C chat command to toggle colour on or off.

// Version 1.0.3.43

// Add SKIPPROMPT command to forward scripts

// Version 1.0.4.1

// Non - Beta Release
// Fix possible crash/corruption with long B2 messages

// Version 1.0.4.2

// Add @winlink.org to the B2 From addresss if it is just a callsign
// Route Flood Bulls on TO as well as @

// Version 1.0.4.3

// Handle Packet Addresses from RMS Express
// Fix for Housekeeping B$ messages

// Version 1.0.4.4

// Remove B2 header and all but the Body part from messages forwared using MBL
// Fix handling of ;FW: from RMS Express

// Version 1.0.4.5

// Disable Paging on forwarding sessions.
// Kill Msgs sent to RMS Exxpress
// Add Name to Chat *** Joined msg

// Version 1.0.4.6

// Pass smtp:winlink.org messages from Airmail to local user check
// Only apply local user check to RMS: messages @winlink.org
// Check locally input smtp: messages for local winlink.org users
// Provide facility to allow only one connect on a port

// Version 1.0.4.8

//	Only reset last listed on L or LR commands.

// Version 1.0.4.9

// Fix error in handling smtp: messages to winlink.org addresses from Airmail

// Version 1.0.4.10

// Fix Badwords processing
// Add Connect Script PAUSE command

// Version 1.0.4.11

// Suppress display and listing of held messages
// Add option to exclude SYSOP messages from LM, KM, etc
// Fix crash whan receiving messages with long lines via plain text forwarding

// Version 1.0.4.12 Jul 2010

// Route P messages on AT
// Allow Applications above 8

// Version 1.0.4.13 Aug 2010

// Fix TidyString for addresses of form John Wiseman <john.wiseman@ntlworld.com>
// Add Try/Except around socket routines

// Version 1.0.4.14 Aug 2010

// Trap "Error - TNC Not Ready" in forward script response
// Fix restart after program error
// Add INFO command
// Add SYSOP-configurable HELP Text.

// Version 1.0.4.15 Aug 2010

// Semaphore Connect/Disconnect
// Semaphore RemoveTempBIDS

// Version 1.0.4.16 Aug 2010

// Remove prompt after receiving unrecognised line in MBL mode. (for MSYS)

// Version 1.0.4.17 Aug 2010

// Fix receiving multiple messages in FBB Uncompressed Mode
// Try to trap phantom chat node connections
// Add delay to close


// Version 1.0.4.18 Aug 2010

// Add "Send SYSTEM messages to SYSOP Call" Option
// set fwd bit on local winlink.org msgs if user is a BBS
// add winlink.org to from address of messages from WL2K that don't already have an @ 

// Version 1.0.4.19 Sept 2010

// Build a B2 From: address if possible, so RMS Express can reply to packet messages.
// Fix handling of addresses from WL2K with SSID's
// L@ now only matches up to length of input string.
// Remove "Type H for help" from login prompt.

// Version 1.0.4.20 Sept 2010

// Process FBB 'E' response
// Handle FROM addresses with an @BBS
// Fix FROM addresses with @ on end.
// Extend delay before close after sending FQ on winmor/pactor sessions.

// Version 1.0.4.21 Sept 2010

// Fix handling B2 From: with an HA
// Add "Expert User" welcome message.

// Version 1.0.4.22 Sept 2010

// Version 1.0.4.23 Oct 2010

// Add Dup message supression
// Dont change B2 from if going to RMS

// Version 1.0.4.24 Oct 2010

// Add "Save Registry Config" command
// Add forwarding on wildcarded TO for NTS
// Add option to force text mode forwarding
// Define new users as a temporaty BBS if SID received in reply to Name prompt
// Reduce delay before sending close after sending FQ on pactor sessions
// Fix processing of MIME boundary from GMail

// Send /ex instead of ctrl/z for text mode forwarding
// Send [WL2K-BPQ... SID if user flagged as RMS Express
// Fix Chat Map reporting when more than one AXIP port
// Add Message State D for NTS Messages
// Forward messages in priority order - T, P, B
// Add Reject and Hold Filters
// Fix holding messages to local RMS users when received as part of a multiple addressee message

// Version 1.0.4.25 Nov 2010

// Renumbered for release
// Add option to save Registry Config during Housekeeping

// Version 1.0.4.26 Nov 2010

// Fix F> loop when doing MBL forwarding between BPQ BBSes
// Allow multiple To: addresses, separated by ;
// Allow Houskeeping Lifetime Overrides to apply to Unsent Messages.
// Set Unforwarded Bulls to status '$'
// Accept MARS and USA as continent codes for MARS Packet Addresses
// Add option to send Non-delivery notifications.

// Version 1.0.4.27 Dec 2010

// Add MSGTYPES fwd file option

// Version 1.0.4.28 Dec 2010

// Renumbered to for release

// Version 1.0.4.30 Dec 2010

// Fix rescan requeuing where bull was rejected by a BBS
// Fiz flagging bulls received by NNTP with $ if they need to be forwarded.
// Add Chat Keepalive option.
// Fix bug in non-delivery notification.

// Version 1.0.4.32 Jan 2011

// Allow "Send from Clipboard" to send to rms: or smtp:
// Allow messages received via SMTP to be bulls (TO preceeded by bull/) or NTS (to nnnnn@NTSXX or nnnnn@NTSXX.NTS)
// Fix corruption of messages converted to B2 if body contains binary data
// Fix occasional program error when forwarding B2 messages
// Limit FBB protocol data blocks to 250 to try to fix restart problem.
// Add F2 to F5 to open windows.

// Version 1.0.4.33 Jan 2011

// Fix holding old bulls with forwarding info.

// Version 1.0.4.33 Jan 2011

// Prevent transfer restarting after a program error.
// Allow Housekeeping to kill held messages.

// Version 1.0.4.35 Jan 2011

// Add Size limits for P and T messages to MSGTYPES command
// Fix Error in MBL processing when blank lines received (introduced in .33)
// Trap possible PE in Send_MON_Datagram
// Don't use paging on chat sessions

// Version 1.0.4.36 Jan 2011

// Fix error after handling first FBB block.
// Add $X and $x welcome message options.

// Version 1.0.4.37 Jan 2011

// Change L command not to list the last message if no new ones are available
// Add LC I I@ IH IZ commands
// Add option to send warning to sysop if forwarded P or T message has nowhere to go
// Fixes for Winpack Compressed Download
// Fix Houskeeping when "Apply Overrides to Unsent Bulls" is set.
// Add console copy/paste.
// Add "No Bulls" Option.
// Add "Mail For" Beacon.
// Tidied up Tab order in config dialogs to help text-to-speech programs.
// Limit MaxMsgno to 99000.

// Version 1.0.4.38 Feb 2011

// Renumbered for release

// Version 1.0.4.40 April 2011

// Add POLLRMS command

// Changes for Vista/Win7 (registry key change)
// Workaround for changes to RMS Express
// Fix AUTH bug in SMTP server
// Add filter to Edit Messages dialog

// Version 1.0.4.41 April 2011

// Extend B2 proposals to other BPQMail systems so Reject Filter will work.
// Add Edit User Command
// Use internal Registry Save routine instead of Regedit
// Fix Start Forward/All
// Allow Winpack Compressed Upload/Download if PMS flag set (as well as BBS flag) 
// Add FWD SYSOP command
// Fix security on POLLRMS command
// Add AUTH command
// Leave selection in same place after Delete User
// Combine SMTP server messages to multiple WL2K addresses into one message to WL2k
// Add option to show name as well as call on Chat messages
// Fix program error if you try to define more than 80 BBS's

// Version 1.0.4.45 October 2011

// Changes to program error reporting.
// BBS "Returh to Node" command added
// Move config to "Standard" location (BPQ Directory/BPQMailChat) .
// Fix crash if "Edit Message" clicked with no message selected.

// Version 1.0.4.46 October 2011

//	Fix BaseDir test when BaseDir ends with \ or /
//  Fix long BaseDir values (>50 chars)

// Version 1.4.47.1 January 2012

//  Call CloseBPQ32 on exit
//  Add option to flash window instead of sounding bell on Chat Connects
//  Add ShowRMS SYSOP command
//	Update WP with I records from R: lines
//	Send WP Updates
//  Fix Paclen on Pactor-like sessions
//	Fix SID and Prompt when RMS Express User is set
//  Try to stop loop in Program Error/Restarting code
//  Trap "UNABLE TO CONNECT" response in connect script
//  Add facility to print messages or save them to a text file

// Version 1.4.48.1 January 2012

//	Add Send Message (as well as Send from Clipboard)
//	Fix Email From: Address when forwaring using B2
//	Send WP from BBSCALL not SYSOPCALL
//  Send Chat Map reports via BPQ32.dll


// Version 1.4.49.1 February 2012


//	Fix Setting Paclink mode on SNOS connects
//	Remove creation of debugging file for each message
//	Add Message Export and Import functions
//	All printing of more than one message at a time
//	Add command to toggle "Expert" status

// Version 1.4.50.1 February 2012

//  Fix forwarding to RMS Express users
//  Route messages received via B2 to an Internet email address to RMS
//  Add Reverse Poll interval
//  Add full FROM address to POP3 messages
//	Include HOMEBBS command in Help


// Version 1.4.51.1 June 2012

//  Allow bulls to be sent from RMS Express.
//  Handle BASE64 and Quoted-printable encoding of single part messages
//	Work round for RMS Express "All proposals rejected" Bug.

// Version 1.4.52.1 August 2012

//  Fix size limit on B2 To List when sending to multiple dests
//  Fix initialisation of DIRMES.SYS control record 
//	Allow use of Tracker and UZ7HO ports for UI messages

// Version 1.4.53.1 September 2012

//	Fix crash if R: line with out a CR found.

// Version 1.4.54.1 ?? 2012

//	Add configurable prompts
//	Fix KISS-Only Test
//	Send EHLO instead of HELO when Authentication is needed on SMTP session
//	Add option to use local tome for bbs forwarding config
//	Allow comment lines (; or @) or single space in fwd scripts
//  Fix loss of forwarding info if SAVE is clicked before selecting a call

// Version 1.4.55.1 June 2013

// Add option to remove users that have not connected for a long time.
// Add l@ smtp:
// Fix From: sent to POP3 Client when meaages is from RMS
// Display Email From on Manage Messages

// Version 1.4.56.1 July 2013

// Add timeout
// Verify prompts
// Add IDLETIME command



// Version 1.4.57.1

//	Change default IDLETIME
//	Fix display of BBS's in Web "Manage Messages"
//	Add separate househeeping lifetines for T messages
//  Don't change flag on forwarded or delivered messages if they sre subsequently read
//  Speed up processing, mainly to stop RMS Express timing out when connecting via Telnet
//  Don't append winlink.org to RMS Express or Paclink addresses if RMS is not configured
//	Fix receiving NTS messages via B2
//	Add option to send "Mail For", but not FBB Headers
//	Fix corruption caused with Subject longer than 60 bytes reveived from Winlink systems
//	Fix Endian bug in FBB Compression code


// Version 1.4.58.1

//  Change control of appending winlink.org to RMS Express or Paclink addresses to a user flag
//	Lookup HomeBBS and WP for calls without a via received from RMS Express or Paclink 
//	Treat call@bpq as request to look up address in Home BBS/WP for messages received from RMS Express or Paclink 
//	Collect stats by message type
//	Fix Non-Delivery notifications to SMTP messages
//	Add Message Type Stats to BBS Trafic Report
//	Add "Batch forward to email"
//	Add EXPORT command
//	Allow more BBS records
//	Allow lower case connect scripts
//  Fix POP3 LIST command
//	Fix MIME Multipart Alternate with first part Base64 or Quoted Printable encoding
//	Fix duplicates of SP SYSOP@WW Messages
//	Add command line option (tidymail) to delete redundant Mail files
//	Add command line option (nohomebbs) to suppress HomeBBS prompt

// 59 April 2014

//	Add FLARQ Mail Mode
//	Fix possible crash saving restart data
//	Add script command ADDLF for connect scripts over Telnet
//	Add recogniton of URONODE connected message
//	Add option to stop Name prompt
//	Add new RMS Express users with "RMS Express User" flag set
//	Validate HTML Pages
//	Add NTS swap file
//	Add basic File list and read functions
//	Fix Traffic report

// 60

//	Fix security hole in readfile

// 61 August 2014
//	Set Messages to NTS:nnnnn@NTSXX to type 'T' and remove NTS
//	Dont treat "Attempting downlink" as a failure
//	Add option to read messages during a list
//	Fix crash during message renumber on MAC
//	Timeout response to SID to try to avoid hang on an incomplete connection.
//	Save config in file instead of registry
//	Fix Manage Messages "EXPORT" option and check filename on EXPORT command
//	Fix reverse forward prompt in MBL mode.
//	Fix From address in POP3 messages where path is @winlink.org
//	Fix possible program error in T message procesing
//	Add MaxAge param (for incoming Bulls)


//62 November 2014
//	Add ZIP and Permit Bulls flag to Manage Users 
//	Allow users to kill their own B and anyone to kill T messages
//	Improve saving of "Last Listed"
//	Fix LL when paging
//	Send Date received in R: Line (should fix B2 message restarts)
//	Fix occasional crash in terminal part line processing
//	Add "SKIPCON" forwarding command to handle nodes that include "Connected" in their CTEXT
//	Fix possible retry loop when message is deferred (FBB '=' response);
//	Don't remove Attachments from received bulls.

//63 Feb 2015

//	Fix creating Bulls from RMS Express messages.
//	Fix PE if message with no To: received.
//	Fix setting "RMS Express User" flag on new connects from RMS Express 
//	Fix deleting 'T' messages downloaded by RMS Express
//	Include MPS messages in count of messages to forward.
//	Add new Welcome Message variable $F for messages to forward
//	Fix setting Type in B2 header when usong NTS: or BULL: 
//	Remove trailing spaces from BID when Creating Message from Clipboard.
//	Improved handling of FBB B1/B2 Restarts.

//64 September 2015

//	Fix Message Type in msgs from RMS Express to Internet
//	Reopen Monitor window if open when program list closed
//	Only apply NTS alias file to NTS Messages
//	Fix failure to store some encrypted ISP passwords
//	Allow EDITUSER to change "RMS Express User" flag
//	Fix reporting of Config File errors
//	Fix Finding MPS Messages (First to Forward was being used incorrectly)
//	Add "Save Attachment" to Web Mgmt Interface
//	Support Secure Signon on Forwarding sessions to CMS
//	Save Forwarding config when BBS flag on user is cleared
//	Pass internally generated SYSOP messages through routing process
//	Add POP3 TOP command.
//	Don't set 'T' messages to 'Y' when read.
//	Add optional temporary connect script on "FWD NOW" command
//	Add automatic import facility
//	Accept RMS mail to BBS Call even if "Poll RMS" not set.

// 65 November 2015

//	Fix loading Housekeeping value for forwarded bulls.
//	Fix re-using Fwd script override in timer driven forwarding.
//	Add ampr.org handling
//	Add "Dont forward" match on TO address for NTS
//	Allow listing a combinatiom of state and type, such as LNT or LPF
//	Fix handling ISP messages from gmail without a '+'
//	Add basic WebMail support

// 66

//	Autoimport messages as Dummy Call, not SYSOP Call
//	Add "My Messages" display option to WebMail
//	Create .csv extract of User List during hourekeeping.
//	Fix processing of NTS Alising of @ Addresses
//	Don't reroute Delivered NTS Messages
//	Add option to stop users killing T messages
//	Add multicast Receive
//	Fix initialising new message database format field
//	Fix "Forward Messages to BBS Call" option.
//	Add Filter WP Bulls option and allow multiple WP "TO" addresses
//	Fix deleting P WP messages for other stations
//	Fix saving blank lines in forwarding config
//	Fix paging on L@ and l<
//	Fix removing DELETE from IMPORT XXX DELETE and allow multiple IMPORT lines in script
//	Run DeleteRedundantMessages before renumbering messages
//	Connect script now tries ELSE lines if prompt not received from remote BBS
//	Send connecting call instead of BBS Name when connecting to CMS server.
//	Add BID filter to Manage Messages
//	Fix handling of over long suject lines in IMPORT
//	Allow comments before ELSE in connect script
//	Add Copy and Clear to Multicast Window
//	Fix possible duplicate messages with MBL forwarding
//	Set "Permit EMail" on IMPORT dummy User.
//	Fix repeated running of housekeeping if clock is stepped forward.
//	Fix corruption of CMS Pass field by Web interface
//	Kill B2 WP bulls if FilterWPBulls set
//	Include Message Type in BPQ B2 proposal extensions

//  6.0.14.1 July 2017

//	Fix corruption of BBSNumber if RMS Ex User and BBS both checked
//	Tread B messages without an AT as Flood.
//	Make sure Message headers are always saved to disk when a message status changes
//	Reject message instead of failing session if TO address too long in FBB forwarding
//	Fix error when FBB restart data exactly fills a packet.
//	Fix possible generation of msg number zero in send nondlivery notification 
//	Fix problem with Web "Manage Messages" when stray message number zero appears
//	Fix Crash in AMPR forward when host missing from VIA
//	Fix possible addition of an spurious password entry to the ;FW: line when connecting to CMS
//	Fix test for Status "D" in forward check.
//	Don't cancel AUTH on SMTP RSET
//	Fix "nowhere to go" message on some messages sent to smtp addresses
//	Add @ from Home BBS or WP is not spcified in "Send from Clipboard"

//	6.0.15.1 Feb 2018

//	Fix PE if Filename missing from FILE connect script command
//	Suppress reporting errors after receiving FQ
//	Fix problem caused by trailing spaces on callsign in WP database
//	Support mixed case WINLINK Passwords

// 6.0.16.1 March 2018

//	Make sure messages sent to WL2K don;'t have @ on from: address
//  If message to saildocs add R: line as an X header instead of to body
//	Close session if more than 4 Invalid Commmad responses sent
//	Report TOP in POP3 CAPA list. Allows POP3 to work with Windows Mail client

// 6.0.17.1 November 2018

//	Add source routing using ! eg sp g8bpq@winlink.org!gm8bpq to send via RMS on gm8bpq
//	Accept an internet email address without rms: or smtp: 
//	Fix "Forward messages for BBS Call" when TO isn't BBS Call
//	Accept NNTP commands in either case
//  Add NNTP BODY command
//	Timeout POP or SMTP TCP connections that are open too long
//  Add YAPP support
//  Fix connect script when Node CTEXT contains "} BBS "
//	Fix handling null H Route
//	Detect and correct duplicate BBS Numbers
//	Fix problem if BBS requests FBB blocked forwarding without compression (ie SID of F without B)
//	Fix crash if YAPP entered without filenmame and send BBS prompt after YAPP error messages
//	Add support for Winlink HTML Forms to WebMail interface
//	Update B2 header when using NTS alias file with B2 messages

// 6.0.18.1 January 2019

//	Ensure callsigns in WP database are upper case.
//	Various fixes for Webmail
//	Fix sending direct to ampr.org addresses
//	Use SYSOP Call as default for Webmail if set
//	Preparations for 64 bit version


// 6.0.19.1 September 2019

//	Trap missing HTML reply Template or HTML files
//	Fix case problems in HTML Templates
//	Fix setting To call on reply to HTML messages
//  More preparations for 64 bit including saving WP info as a text file.
//	Set "RMS Express User" when a new user connects using PAT
//	Increace maximum length on Forwarding Alias string in Web interface
//	Expand multiaddress messages from Winlink Express if "Don't add @Winlink.org" set or no RMS BBS
//	Fix program error if READ used without a filename
//	Trap reject messages from Winlink CMS
//	Fix "delete to recycle bin" on Linux
//	Handle Radio Only Messages (-T or -R suffix on calling station)
//	Fix program error on saving empty Alias list on Web Forwarding page
//	Add REQDIR and REQFIL
//	Experimental Blocked Uncompressed forwarding
//	Security fix for YAPP
//	Fix WebMail Cancel Send Message
//	Fix processing Hold Message response from Winlink Express

// 6.0.20.1 April 2020

//	Improvments to YAPP
//	Add Copy forwarding config
//	Add Next and Previous buttons to Webmail message read screen
//	Move HTML templates from HTMLPages to inline code.
//	Fix Paclen on YAPP send
//	Fix bug in handling "RMS Express User"
//	Fix WINPACK compressed forwarding
//	Add option to send P messages to more than one BBS
//	Add "Default to Don't Add WINLINK.ORG" Config option
//	Re-read Badwords.sys during Housekeeping
//	Add BID Hold and Reject Filters
//	On SMTP Send try HELO if EHLO rejected
//	Allow SID response timeout to be configured per BBS
//	Fix sending bulls with PAT
//  Set "Forward Messages to BBS Call" when routing Bulls on TO
//	Add option to send Mail For Message to APRS
//	Fix WP update
//	Fix Holding messages from Webmail Interface
//	Add RMR command
//	Add REROUTEMSGS BBS SYSOP command
//	Disable null passwords and check Exclude flag in Webmail Signin
//	Add basic Webmail logging 

// 6.0.21.1    December 2020

//	Remove nulls from displayed messages.
//	Fix Holding messages from SMTP and POP3 Interfaces
//	Various fixes for handling messages to/from Internet email addresses
//	Fix saving Email From field in Manage Messages
//	Fix sending WL2K traffic reports via TriMode.
//	Fix removing successive CR from Webmail Message display
//	Fix Wildcarded @ forwarding
//	Fix message type when receiving NTS Msgs form Airmail
//	Fix address on SERVICE messages from Winlink
//	Add multiple TO processing to Webmail non-template messages
//	Don't backup config file if reading it fails
//	Include Port and Freq on Connected log record
//	Make sure welcome mesages don't end in >
//  Allow flagging unread T messages as Delivered
//  Replace \ with # in forward script so commands starting with # can be sent
//  Fix forwarding NTS on TO field
//	Fix possible crash in text mode forwarding
//	Allow decimals of days in P message lifetimes and allow Houskeeping interval to be configured
//	Add DOHOUSEKEEPING sysop command
//  Add MARS continent code
//	Try to trap 'zombie' BBS Sessions
//	On Linux if "Delete to Recycle Bin" is set move deleted messages and logs to directory Deleted under current directory.
//	Fix corruption of message length when reading R2 message via Read command
//	Fix paging on List command and add new combinations of List options
//	Fix NNTP list and LC command when bulls are killed

//  6.0.22.1       August 2021

//	Fix flagging messages with attachments as read.
//	Fix possible corruption of WP database and subsequent crash on reloading.
//	Fix format of Web Manage Messages display
//	Include SETNEXTMESSAGENUMBER in SYSOP Help Message
//	Fix occasional "Incoming Connect from SWITCH"
//	Fix L> with numeric dests
//	Improved diagnostic for MailTCP select() error.
//	Clear "RMS Express User"  if user is changed to a BBS
//	Fix saving Window positions on exit
//	Fix parsing ReplyTemplate name in Webmail
//	Handle multiple addressees for WebMail Forms messages to packet stations
//	Add option to allow only known users to connect
//	Add basic callsign validation to From address
//	Add option to forward a user's messages to Winlink
//	Move User config to main config file.
//	Update message status whne reading a Forms Webmail message
//	Speed up killing multiple messages
//	Allow SendWL2KFW as well as the (incorrect)SendWL2KPM command

//  6.0.23.1  June 2022

//	Fix crash when ; added to call in send commands
//	Allow smtp/ override for messages from RMS Express to send via ISP gateway
//	Send Internet email from RMS Express to ISP Gateway if enabled and RMS BBS not configured 
//	Recompiled for Web Interface changes in Node
//  Add RMS Relay SYNC Mode (.17)
//	Add Protocol changes for Relay RO forwarding
//	Add SendWL2KPM command to connect script to allow users other than RMS to send ;FW: string to RMS Relay
//	Fix B2 Header Date in Webmail message with sttachments.
//	Fix bug when using YAPP with VARA (.27)
//	Allow SendWL2KFW as well as the (incorrect)SendWL2KPM command
//	Add mechsnism to send bbs log records to qttermtcp. (32)
//	Add MFJ forwarding Mode (No @BBS on send)
//	Fix handling CR/LF split over packet boundaries
//	Add Header and Footers for Webmail Send (42)
//	Fix Maintenance Interval in LinBPQ (53)
//	Add RMS: to valid from addresses (.56)
//	Fix Web management on Android deviced (.58)
//	Disconnect immediately if "Invalid Command" "*** Protocol Error" or "Already Connected" received (.70)
//	Check Badword and Reject filters before processing WP Messages

//  6.0.24.1  August 2023

//	Fix ' in Webmail subject (8)
//	Change web buttons to white on black when pressed (10)
//	Add auto-refresh option to Webmail index page (25)
//	Fix displaying help and info files with crlf line endings on Linux (28)
//	Improve validation of extended FC message (32)
//	Improve WP check for SYSTEM as a callsign (33)
//	Improvements to RMS Relay SYNC mode (47)
//	Fix BID Hold and Reject filters
//	Fix Webmail auto-refresh when page exceeds 64K bytes (54)
//	Fix Webmail send when using both headers/footers and attachmonts (55)
//	Fix R: line corruption on some 64 bit builds
//	Dont drop empty lines in TEXTFORWARDING (61)
//	Dont wait for body prompt for TEXTFORWARDING for SID [PMS-3.2-C$] (62)
//	Add forwarding mode SETCALLTOSENDER for PMS Systems that don't accept < in SP (63)
//	QtTerm Monitoring fixed for 63 port version of BPQ (69)
//	Fix to UI system to support up to 63 ports (79)
//	Fix recently introduced crash when "Don't allow new users" is set (81)
//	Skip comments before TIMES at start of Connect Script (83)

//  6.0.25.1  ?? 

//	Add FBB reject.sys style filters (3)
//	Improve Webmail on 64 bit builds
//	Fix setting status '$' on Bulls sent via WebMail (22)
//	Implement New Message and Message Read Events (23)
//	Start adding json api (25)
//	Fix reading nested directories when loading Standard Templates and other template bugs (25)
//	Add TO and AT to "Message has nowhere to go" message (28)
//	Add My Sent and My Received filter options to Webmail (30)
//	Add Send P to multiple BBS's when routing on HR (30)

#include "bpqmail.h"
#include "winstdint.h"
#define MAIL
#include "Versions.h"

#include "GetVersion.h"

#define MAX_LOADSTRING 100

typedef int (WINAPI FAR *FARPROCX)();
typedef int (WINAPI FAR *FARPROCZ)();

FARPROCX pDllBPQTRACE;
FARPROCZ pGetLOC;
FARPROCX pRefreshWebMailIndex;
FARPROCX pRunEventProgram;

BOOL WINE = FALSE;

INT_PTR CALLBACK UserEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MsgEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FwdEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WPEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID SetupNTSAliases(char * FN);

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default
char * REGTREETEXT = "HKEY_LOCAL_MACHINE";

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

extern int LastVer[4];							// In case we need to do somthing the first time a version is run

UINT BPQMsg;

HWND MainWnd;
HWND hWndSess;
RECT MainRect;
HMENU hActionMenu;
static HMENU hMenu;
HMENU hDisMenu;									// Disconnect Menu Handle
HMENU hFWDMenu;									// Forward Menu Handle

int SessX, SessY, SessWidth;					// Params for Session Window

char szBuff[80];

#define MaxSockets 64

int _MYTIMEZONE = 0;

ConnectionInfo Connections[MaxSockets+1];

//struct SEM AllocSemaphore = {0, 0};
//struct SEM ConSemaphore = {0, 0};
//struct SEM OutputSEM = {0, 0};

//struct UserInfo ** UserRecPtr=NULL;
//int NumberofUsers=0;

//struct UserInfo * BBSChain = NULL;					// Chain of users that are BBSes

//struct MsgInfo ** MsgHddrPtr=NULL;
//int NumberofMessages=0;

//int FirstMessageIndextoForward=0;					// Lowest Message wirh a forward bit set - limits search

//BIDRec ** BIDRecPtr=NULL;
//int NumberofBIDs=0;

extern BIDRec ** TempBIDRecPtr;
//int NumberofTempBIDs=0;

//WPRec ** WPRecPtr=NULL;
//int NumberofWPrecs=0;

extern char ** BadWords;
//int NumberofBadWords=0;
extern char * BadFile;

//int LatestMsg = 0;
//struct SEM MsgNoSemaphore = {0, 0};					// For locking updates to LatestMsg
//int HighestBBSNumber = 0;

//int MaxMsgno = 60000;
//int BidLifetime = 60;
//int MaintInterval = 24;
//int MaintTime = 0;
//int UserLifetime = 0;


BOOL cfgMinToTray;

BOOL DisconnectOnClose;

extern char PasswordMsg[100];

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[];
char disMsg[];

char LoginMsg[];

char BlankCall[];


ULONG BBSApplMask;
ULONG ChatApplMask;

int BBSApplNum;

//int	StartStream=0;
int	NumberofStreams;
int MaxStreams;

extern char BBSSID[];
extern char ChatSID[];

extern char NewUserPrompt[100];

extern char * WelcomeMsg;
extern char * NewWelcomeMsg;
extern char * ExpertWelcomeMsg;

extern char * Prompt;
extern char * NewPrompt;
extern char * ExpertPrompt;

extern BOOL DontNeedHomeBBS;

char BBSName[100];
char MailForText[100];

char SignoffMsg[100];

char AbortedMsg[100];

extern char UserDatabaseName[MAX_PATH];
extern char UserDatabasePath[MAX_PATH];

extern char MsgDatabasePath[MAX_PATH];
extern char MsgDatabaseName[MAX_PATH];

extern char BIDDatabasePath[MAX_PATH];
extern char BIDDatabaseName[MAX_PATH];

extern char WPDatabasePath[MAX_PATH];
extern char WPDatabaseName[MAX_PATH];

extern char BadWordsPath[MAX_PATH];
extern char BadWordsName[MAX_PATH];

char NTSAliasesPath[MAX_PATH];
extern char NTSAliasesName[MAX_PATH];

char BaseDir[MAX_PATH];
char BaseDirRaw[MAX_PATH];			// As set in registry - may contain %NAME%

char MailDir[MAX_PATH];

char RlineVer[50];

extern BOOL KISSOnly;

extern BOOL OpenMon;

extern struct ALIAS ** NTSAliases;

extern int EnableUI;
extern int RefuseBulls;
extern int SendSYStoSYSOPCall;
extern int SendBBStoSYSOPCall;
extern int DontHoldNewUsers;
extern int ForwardToMe;

extern int MailForInterval;

char zeros[NBMASK];						// For forward bitmask tests

time_t MaintClock;						// Time to run housekeeping

struct MsgInfo * MsgnotoMsg[100000];	// Message Number to Message Slot List.

// Filter Params

char ** RejFrom;					// Reject on FROM Call
char ** RejTo;						// Reject on TO Call
char ** RejAt;						// Reject on AT Call
char ** RejBID;						// Reject on BID 

char ** HoldFrom;					// Hold on FROM Call
char ** HoldTo;						// Hold on TO Call
char ** HoldAt;						// Hold on AT Call
char ** HoldBID;					// Hold on BID


// Send WP Params

BOOL SendWP;
char SendWPVIA[81];
char SendWPTO[11];
int SendWPType;


int ProgramErrors = 0;

UCHAR BPQDirectory[260] = "";


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				RegisterMainWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ClpMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    SendMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

unsigned long _beginthread( void( *start_address )(VOID * DParam),
				unsigned stack_size, VOID * DParam);

VOID SendMailForThread(VOID * Param);
BOOL CreatePipeThread();
int DeleteRedundantMessages();
VOID BBSSlowTimer();
VOID CopyConfigFile(char * ConfigName);
BOOL CreateMulticastConsole();
char * CheckToAddress(CIRCUIT * conn, char * Addr);
BOOL CheckifPacket(char * Via);
int GetHTMLForms();
VOID GetPGConfig();

struct _EXCEPTION_POINTERS exinfox;
	
CONTEXT ContextRecord;
EXCEPTION_RECORD ExceptionRecord;

DWORD Stack[16];

BOOL Restarting = FALSE;

Dump_Process_State(struct _EXCEPTION_POINTERS * exinfo, char * Msg)
{
	unsigned int SPPtr;
	unsigned int SPVal;

	memcpy(&ContextRecord, exinfo->ContextRecord, sizeof(ContextRecord));
	memcpy(&ExceptionRecord, exinfo->ExceptionRecord, sizeof(ExceptionRecord));
		
	SPPtr = ContextRecord.Esp;

	Debugprintf("BPQMail *** Program Error %x at %x in %s",
	ExceptionRecord.ExceptionCode, ExceptionRecord.ExceptionAddress, Msg);	


	__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

	}

	Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
		ContextRecord.Eax, ContextRecord.Ebx, ContextRecord.Ecx,
		ContextRecord.Edx, ContextRecord.Esi, ContextRecord.Edi, SPVal);
		
	Debugprintf("Stack:");

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

}



void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	Logprintf(LOG_DEBUG_X, NULL, '!', "*** Error **** C Run Time Invalid Parameter Handler Called");

	if (expression && function && file)
	{
		Logprintf(LOG_DEBUG_X, NULL, '!', "Expression = %S", expression);
		Logprintf(LOG_DEBUG_X, NULL, '!', "Function %S", function);
		Logprintf(LOG_DEBUG_X, NULL, '!', "File %S Line %d", file, line);
	}
}

// If program gets too many program errors, it will restart itself  and shut down

VOID CheckProgramErrors()
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	char ProgName[256];

	if (Restarting)
		exit(0);				// Make sure can't loop in restarting

	ProgramErrors++;

	if (ProgramErrors > 25)
	{
		Restarting = TRUE;

		Logprintf(LOG_DEBUG_X, NULL, '!', "Too Many Program Errors - Closing");

		if (cfgMinToTray)
		{
			DeleteTrayMenuItem(MainWnd);
			if (ConsHeader[0]->hConsole)
				DeleteTrayMenuItem(ConsHeader[0]->hConsole);
			if (ConsHeader[1]->hConsole)
				DeleteTrayMenuItem(ConsHeader[1]->hConsole);
			if (hMonitor)
				DeleteTrayMenuItem(hMonitor);
		}

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		GetModuleFileName(NULL, ProgName, 256);

		Debugprintf("Attempting to Restart %s", ProgName);

		CreateProcess(ProgName, "MailChat.exe WAIT", NULL, NULL, FALSE, 0, NULL, NULL, &SInfo, &PInfo);
					
		exit(0);
	}
}


VOID WriteMiniDump()
{
#ifdef WIN32

	HANDLE hFile;
	BOOL ret;
	char FN[256];

	sprintf(FN, "%s/Logs/MiniDump%x.dmp", GetBPQDirectory(), time(NULL));

	hFile = CreateFile(FN, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		// Create the minidump

		ret = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, MiniDumpNormal, 0, 0, 0 );

		if(!ret)
			Debugprintf("MiniDumpWriteDump failed. Error: %u", GetLastError());
		else
			Debugprintf("Minidump %s created.", FN);
			CloseHandle(hFile);
	}
#endif
}


void GetSemaphore(struct SEM * Semaphore, int ID)
{
	//
	//	Wait for it to be free
	//
#ifdef WIN32
	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
	}
loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	mov ebx, Semaphore
	xchg [ebx],eax		// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, weve got the semaphore
;
	}
#else

	while (Semaphore->Flag)
		usleep(10000);

	Semaphore->Flag = 1;

#endif
	return;
}

void FreeSemaphore(struct SEM * Semaphore)
{
	Semaphore->Flag = 0;

	return; 
}

char * CmdLine;

extern int configSaved;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	struct UserInfo * user;
	struct _EXCEPTION_POINTERS exinfo;
	_invalid_parameter_handler oldHandler, newHandler;
	char Msg[100];
	int i = 60;
	struct NNTPRec * NNTPREC;
	struct NNTPRec * SaveNNTPREC;

	CmdLine = _strdup(lpCmdLine);
	_strlwr(CmdLine);

	if (_stricmp(lpCmdLine, "Wait") == 0)				// If AutoRestart then Delay 60 Secs
	{	
		hWnd = CreateWindow("STATIC", "Mail Restarting after Failure - Please Wait", 0,
		CW_USEDEFAULT, 100, 550, 70,
		NULL, NULL, hInstance, NULL);

		ShowWindow(hWnd, nCmdShow);

		while (i-- > 0)
		{
			sprintf(Msg, "Mail Restarting after Failure - Please Wait %d secs.", i);
			SetWindowText(hWnd, Msg);
			
			Sleep(1000);
		}

		DestroyWindow(hWnd);
	}

	__try {

	// Trap CRT Errors
	
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BPQMailChat, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BPQMailChat));

	// Main message loop:

	Logprintf(LOG_DEBUG_X, NULL, '!', "Program Starting");
	Logprintf(LOG_BBS, NULL, '!', "BPQMail Starting");
	Debugprintf("BPQMail Starting");

	if (pDllBPQTRACE == 0)
		Logprintf(LOG_BBS, NULL, '!', "Remote Monitor Log not available - update BPQ32.dll to enable");


	} My__except_Routine("Init");

	while (GetMessage(&msg, NULL, 0, 0))
	{
		__try
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		#define EXCEPTMSG "GetMessageLoop"
		#include "StdExcept.c"

		CheckProgramErrors();
		}
	}

	__try
	{
	for (n = 0; n < NumberofStreams; n++)
	{
		BPQStream=Connections[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}


	hWnd = CreateWindow("STATIC", "Mail Closing - Please Wait", 0,
				150, 200, 350, 40, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	Sleep(1000);				// A bit of time for links to close

	DestroyWindow(hWnd);

	if (ConsHeader[0]->hConsole)
		DestroyWindow(ConsHeader[0]->hConsole);
	if (ConsHeader[1]->hConsole)
		DestroyWindow(ConsHeader[1]->hConsole);
	if (hMonitor)
	{
		DestroyWindow(hMonitor);
		hMonitor = (HWND)1;					// For status Save
	}


//	SaveUserDatabase();
	SaveMessageDatabase();
	SaveBIDDatabase();

	configSaved = 1;
	SaveConfig(ConfigName);

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		if (ConsHeader[0]->hConsole)
			DeleteTrayMenuItem(ConsHeader[0]->hConsole);
		if (ConsHeader[1]->hConsole)
			DeleteTrayMenuItem(ConsHeader[1]->hConsole);
		if (hMonitor)
			DeleteTrayMenuItem(hMonitor);
	}

	// Free all allocated memory

	for (n = 0; n <= NumberofUsers; n++)
	{
		user = UserRecPtr[n];

		if (user->ForwardingInfo)
		{
			FreeForwardingStruct(user);
			free(user->ForwardingInfo); 
		}
		/* ---------- TAJ --PG Server------*/

			if (user->Temp && user->Temp->RUNPGPARAMS ) {

				printf("Also freeing RUNPGARGS\n");
				free(user->Temp->RUNPGPARAMS);
			}
		/* --------------------------------*/

		free(user->Temp);

		free(user);
	}
	
	free(UserRecPtr);

	for (n = 0; n <= NumberofMessages; n++)
		free(MsgHddrPtr[n]);

	free(MsgHddrPtr);

	for (n = 0; n <= NumberofWPrecs; n++)
		free(WPRecPtr[n]);

	free(WPRecPtr);

	for (n = 0; n <= NumberofBIDs; n++)
		free(BIDRecPtr[n]);

	free(BIDRecPtr);

	if (TempBIDRecPtr)
		free(TempBIDRecPtr);

	NNTPREC = FirstNNTPRec;

	while (NNTPREC)
	{
		SaveNNTPREC = NNTPREC->Next;
		free(NNTPREC);
		NNTPREC = SaveNNTPREC;
	}

	if (BadWords) free(BadWords);
	if (BadFile) free(BadFile);

	n = 0;

	if (Aliases)
	{
		while(Aliases[n])
		{
			free(Aliases[n]->Dest);
			free(Aliases[n]);
			n++;
		}

		free(Aliases);
		FreeList(AliasText);
	}

	n = 0;
	
	if (NTSAliases)
	{
		while(NTSAliases[n])
		{
			free(NTSAliases[n]->Dest);
			free(NTSAliases[n]);
			n++;
		}

		free(NTSAliases);
	}

	FreeOverrides();

	FreeList(RejFrom);
	FreeList(RejTo);
	FreeList(RejAt);
	FreeList(RejBID);
	FreeList(HoldFrom);
	FreeList(HoldTo);
	FreeList(HoldAt);
	FreeList(HoldBID);
	FreeList(SendWPAddrs);

	Free_UI();

	for (n=1; n<20; n++)
	{
		if (MyElements[n]) free(MyElements[n]);
	}

	free(WelcomeMsg);
	free(NewWelcomeMsg);
	free(ExpertWelcomeMsg);

	free(Prompt);
	free(NewPrompt);
	free(ExpertPrompt);

	FreeWebMailMallocs();

	free(CmdLine);

	_CrtDumpMemoryLeaks();

	}
	My__except_Routine("Close Processing");

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
//
#define BGCOLOUR RGB(236,233,216)
//#define BGCOLOUR RGB(245,245,245)

HBRUSH bgBrush;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(BPQICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(BPQICON));

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;

int AXIPPort = 0;

char LOC[7] = "";

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	char Title[80];
	WSADATA WsaData;
	HMENU hTopMenu;		// handle of menu 
	HKEY hKey=0;
	int retCode;
	RECT InitRect;
	RECT SessRect;
	struct _EXCEPTION_POINTERS exinfo;

	HMODULE ExtDriver = LoadLibrary("bpq32.dll");

	if (ExtDriver)
	{
		pDllBPQTRACE = GetProcAddress(ExtDriver,"_DllBPQTRACE@8");
		pGetLOC = GetProcAddress(ExtDriver,"_GetLOC@0");
		pRefreshWebMailIndex = GetProcAddress(ExtDriver,"_RefreshWebMailIndex@0");
		pRunEventProgram = GetProcAddress(ExtDriver,"_RunEventProgram@8");

		if (pGetLOC)
		{
			char * pLOC = (char *)pGetLOC();
			memcpy(LOC, pLOC, 6);
		}
	}

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE =TRUE;
		Debugprintf("Running under WINE");
	}


	REGTREE = GetRegistryKey();
	REGTREETEXT = GetRegistryKeyText();

	Sleep(1000);

	{
		int n;
		struct _EXTPORTDATA * PORTVEC;

		KISSOnly = TRUE;
		
		for (n=1; n <= GetNumberofPorts(); n++)
		{
			PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntryFromSlot(n);

			if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			{
				if (_memicmp(PORTVEC->PORT_DLL_NAME, "TELNET", 6) == 0)
					KISSOnly = FALSE;

				if (PORTVEC->PORTCONTROL.PROTOCOL != 10)	// Pactor/WINMOR
					KISSOnly = FALSE;

				if (AXIPPort == 0)
				{
					if (_memicmp(PORTVEC->PORT_DLL_NAME, "BPQAXIP", 7) == 0)
					{
						AXIPPort = PORTVEC->PORTCONTROL.PORTNUMBER;
						KISSOnly = FALSE;
					}
				}
			}
		}
	}

	hInst = hInstance;

	hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	MainWnd = hWnd;

	GetVersionInfo(NULL);

	sprintf(Title,"G8BPQ Mail Server Version %s", VersionString);

	sprintf(RlineVer, "BPQ%s%d.%d.%d", (KISSOnly) ? "K" : "", Ver[0], Ver[1], Ver[2]);

	SetWindowText(hWnd,Title);

	hWndSess = GetDlgItem(hWnd, 100); 

	GetWindowRect(hWnd,	&InitRect);
	GetWindowRect(hWndSess, &SessRect);

	SessX = SessRect.left - InitRect.left ;
	SessY = SessRect.top -InitRect.top;
	SessWidth = SessRect.right - SessRect.left;

   	// Get handles for updating menu items

	hTopMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hTopMenu,0);

	hFWDMenu=GetSubMenu(hActionMenu,0);
	hMenu=GetSubMenu(hActionMenu,1);
	hDisMenu=GetSubMenu(hActionMenu,2);

   CheckTimer();

 	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
		{
			ShowWindow(hWnd, SW_HIDE);
		}
		else
		{
			ShowWindow(hWnd, nCmdShow);
		}
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

   WSAStartup(MAKEWORD(2, 0), &WsaData);

   __try {
    
   return Initialise();

   }My__except_Routine("Initialise");

   return FALSE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int state,change;
	ConnectionInfo * conn;
	struct _EXCEPTION_POINTERS exinfo;


	if (message == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
		{
			__try
			{
				DoBBSMonitorData(wParam);
			}
			My__except_Routine("DoMonitorData");

			return 0;
			
		}
		if (lParam & BPQDataAvail)
		{
			//	Dont trap error at this level - let Node error handler pick it up
//			__try
//			{
				DoReceivedData(wParam);
//			}
//			My__except_Routine("DoReceivedData")
			return 0;
		}
		if (lParam & BPQStateChange)
		{
			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			__try
			{
				SessionState(wParam, &state, &change);
		
				if (change == 1)
				{
					if (state == 1) // Connected	
					{
						GetSemaphore(&ConSemaphore, 0);
						__try {Connected(wParam);}
						My__except_Routine("Connected");
						FreeSemaphore(&ConSemaphore);
					}
					else
					{
						GetSemaphore(&ConSemaphore, 0);
						__try{Disconnected(wParam);}
						My__except_Routine("Disconnected");
						FreeSemaphore(&ConSemaphore);
					}
				}
			}
			My__except_Routine("DoStateChange");

		}

		return 0;
	}


	switch (message)
	{

	case WM_KEYUP:	

		switch (wParam)
		{	
		case VK_F2:
			CreateConsole(-1);
			return 0;

		case VK_F3:
			CreateMulticastConsole();
			return 0;

		case VK_F4:
			CreateMonitor();
			return 0;

		case VK_TAB:
			return TRUE;

		break;



		}
		return 0;
 			
	case WM_TIMER:

		if (wParam == 1)		// Slow = 10 secs
		{
			__try
			{
				time_t NOW = time(NULL);
				struct tm * tm;
				RefreshMainWindow();
				CheckTimer();
				TCPTimer();
				BBSSlowTimer();
				FWDTimerProc();
				if (MaintClock < NOW)
				{
					while (MaintClock < NOW)		// in case large time step
						MaintClock += MaintInterval * 3600;

					Debugprintf("|Enter HouseKeeping");
					DoHouseKeeping(FALSE);
				}
				tm = gmtime(&NOW);	

				if (tm->tm_wday == 0)		// Sunday
				{
					if (GenerateTrafficReport && (LastTrafficTime + 86400) < NOW)
					{
						CreateBBSTrafficReport();
						LastTrafficTime = NOW;	
					}
				}
			}
			My__except_Routine("Slow Timer");
		}
		else
			__try
			{
				TrytoSend();
				TCPFastTimer();
			}
			My__except_Routine("TrytoSend");
		
		return (0);

	
	case WM_CTLCOLORDLG:
        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)hActionMenu)
		{
			if (IsClipboardFormatAvailable(CF_TEXT))
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_GRAYED );
			
			return TRUE;
		}

		if (wParam == (WPARAM)hFWDMenu)
		{
			// Set up Forward Menu

			struct UserInfo * user;
			char MenuLine[30];

			for (user = BBSChain; user; user = user->BBSNext)
			{
				sprintf(MenuLine, "%s %d Msgs", user->Call, CountMessagestoForward(user));

				if (ModifyMenu(hFWDMenu, IDM_FORWARD_ALL + user->BBSNumber, 
					MF_BYCOMMAND | MF_STRING, IDM_FORWARD_ALL + user->BBSNumber, MenuLine) == 0)
	
				AppendMenu(hFWDMenu, MF_STRING,IDM_FORWARD_ALL + user->BBSNumber, MenuLine);
			}
			return TRUE;
		}

		if (wParam == (WPARAM)hDisMenu)
		{
			// Set up Disconnect Menu

			CIRCUIT * conn;
			char MenuLine[30];
			int n;

			for (n = 0; n <= NumberofStreams-1; n++)
			{
				conn=&Connections[n];

				RemoveMenu(hDisMenu, IDM_DISCONNECT + n, MF_BYCOMMAND);

				if (conn->Active)
				{
					sprintf_s(MenuLine, 30, "%d %s", conn->BPQStream, conn->Callsign);
					AppendMenu(hDisMenu, MF_STRING, IDM_DISCONNECT + n, MenuLine);
				}
			}
			return TRUE;
		}
		break;


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmEvent == LBN_DBLCLK)

				break;

		if (wmId >= IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&Connections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				Disconnect(conn->BPQStream);
			}
		}

		if (wmId >= IDM_FORWARD_ALL && wmId < IDM_FORWARD_ALL + 100)
		{
			StartForwarding(wmId - IDM_FORWARD_ALL, NULL);
			return 0;
		}

		switch (wmId)
		{
		case IDM_LOGBBS:

			ToggleParam(hMenu, hWnd, &LogBBS, IDM_LOGBBS);
			break;

		case IDM_LOGCHAT:

			ToggleParam(hMenu, hWnd, &LogCHAT, IDM_LOGCHAT);
			break;

		case IDM_LOGTCP:

			ToggleParam(hMenu, hWnd, &LogTCP, IDM_LOGTCP);
			break;

		case IDM_HOUSEKEEPING:

			DoHouseKeeping(TRUE);

			break;

		case IDM_CONSOLE:

			CreateConsole(-1);
			break;

		case IDM_MCMONITOR:

			CreateMulticastConsole();
			break;

		case IDM_MONITOR:

			CreateMonitor();
			break;

		case RESCANMSGS:

			ReRouteMessages();
			break;

		case IDM_IMPORT:

			ImportMessages(NULL, "", FALSE);
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case ID_HELP_ONLINEHELP:

			ShellExecute(hWnd,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/MailServer.html",
				"", NULL, SW_SHOWNORMAL); 
		
			break;

		case IDM_CONFIG:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;

		case IDM_USERS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_USEREDIT), hWnd, UserEditDialogProc);
			break;

		case IDM_FWD:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_FORWARDING), hWnd, FwdEditDialogProc);
			break;

		case IDM_MESSAGES:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGEDIT), hWnd, MsgEditDialogProc);
			break;

		case IDM_WP:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_EDITWP), hWnd, WPEditDialogProc);
			break;

		case ID_ACTIONS_SENDMSGFROMCLIPBOARD:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGFROMCLIPBOARD), hWnd, ClpMsgDialogProc);
			break;

		case ID_ACTIONS_SENDMESSAGE:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGFROMCLIPBOARD), hWnd, SendMsgDialogProc);
			break;

		case ID_MULTICAST:

			MulticastRX = !MulticastRX;
			CheckMenuItem(hActionMenu, ID_MULTICAST, (MulticastRX) ? MF_CHECKED : MF_UNCHECKED);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;


  
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	
	case WM_SIZING:
	{
		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndSess, 0, 30, SessWidth, Height - 100, TRUE);
			
		return TRUE;
	}


	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:

		GetWindowRect(MainWnd,	&MainRect);	// For save soutine
		if (ConsHeader[0]->hConsole)
			GetWindowRect(ConsHeader[0]->hConsole, &ConsHeader[0]->ConsoleRect);	// For save soutine
		if (ConsHeader[1]->hConsole)
			GetWindowRect(ConsHeader[1]->hConsole, &ConsHeader[1]->ConsoleRect);	// For save soutine
		if (hMonitor)
			GetWindowRect(hMonitor,	&MonitorRect);	// For save soutine

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK SendMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "B");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "P");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "T");

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, 0, 0);

		return TRUE; 

	case WM_SIZING:
	{
		HWND hWndEdit = GetDlgItem(hDlg, IDC_EDIT1); 

		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndEdit, 5, 90, Width-20, Height - 140, TRUE);
			
		return TRUE;
	}

	case WM_COMMAND:

		if (LOWORD(wParam) == IDSEND)
		{
			char status [3];
			struct MsgInfo * Msg;
			char * via = NULL;
			char BID[13];
			char FileList[32768];
			BIDRec * BIDRec;
			int MsgLen;
			char * MailBuffer;
			char MsgFile[MAX_PATH];
			HANDLE hFile = INVALID_HANDLE_VALUE;
			int WriteLen=0;
			char HDest[61];
			char Destcopy[61];
			char * Vptr;
			char * FileName[100];
			int FileLen[100];
			char * FileBody[100];
			int n, Files = 0;
			int TotalFileSize = 0;
			char * NewMsg;

			GetDlgItemText(hDlg, IDC_MSGTO, HDest, 60);
			strcpy(Destcopy, HDest);

			GetDlgItemText(hDlg, IDC_MSGBID, BID, 13);
			strlop(BID, ' ');
	
			GetDlgItemText(hDlg, IDC_ATTACHMENTS, FileList, 32767);

			// if there are attachments, check that they can be opened ane read

			n = 0;
		
			if (FileList[0])
			{
				FILE * Handle;
				struct stat STAT;
				char * ptr1 = FileList, * ptr2;

				while(ptr1 && ptr1[0])
				{
					ptr2 = strchr(ptr1, ';');

					if (ptr2)
						*(ptr2++) = 0;

					FileName[n++] = ptr1;
 
					ptr1 = ptr2;
				}

				FileName[n] = 0;
				
				// read the files

				Files = n;
				n = 0;
				
				while (FileName[n])
				{
					if (stat(FileName[n], &STAT) == -1)
					{
						char ErrorMessage[512];
						sprintf(ErrorMessage,"Can't find file %s", FileName[n]);
						MessageBox(NULL, ErrorMessage, "BPQMail", MB_ICONERROR);
						return TRUE;
					}

					FileLen[n] = STAT.st_size;

					Handle = fopen(FileName[n], "rb");

					if (Handle == NULL)
					{
						char ErrorMessage[512];
						sprintf(ErrorMessage,"Can't open file %s", FileName[n]);
						MessageBox(NULL, ErrorMessage, "BPQMail", MB_ICONERROR);
						return TRUE;
					}

					FileBody[n] = malloc(FileLen[n]+1);

					fread(FileBody[n], 1, FileLen[n], Handle); 

					fclose(Handle);

					TotalFileSize += FileLen[n];
					n++;
				}
			}

			if (strlen(HDest) == 0)
			{		
				MessageBox(NULL, "To: Call Missing!", "BPQMail", MB_ICONERROR);
				return TRUE;
			}

			if (strlen(BID))
			{		
				if (LookupBID(BID))
				{
					// Duplicate bid

					MessageBox(NULL, "Duplicate BID", "BPQMail", MB_ICONERROR);
					return TRUE;
				}
			}

			Msg = AllocateMsgRecord();
		
			// Set number here so they remain in sequence
		
			Msg->number = ++LatestMsg;
			MsgnotoMsg[Msg->number] = Msg;

			strcpy(Msg->from, SYSOPCall);

			Vptr = strlop(Destcopy, '@');

			if (Vptr  == 0 && strchr(Destcopy, '!')) // Bang route without @
			{
				Vptr = strchr(Destcopy, '!');
				strcpy(Msg->via, Vptr);
				strlop(Destcopy, '!');
				
				if (strlen(Destcopy) > 6)
					memcpy(Msg->to, Destcopy, 6);
				else
					strcpy(Msg->to, Destcopy);
				goto gotAddr;
			}

			if (strlen(Destcopy) > 6)
				memcpy(Msg->to, Destcopy, 6);
			else
				strcpy(Msg->to, Destcopy);
			
			_strupr(Msg->to);
			
			if (_memicmp(HDest, "rms:", 4) == 0 || _memicmp(HDest, "rms/", 4) == 0)
			{
				Vptr = HDest;
				memmove(HDest, &HDest[4], strlen(HDest));
				strcpy(Msg->to, "RMS");

			}
			else if (_memicmp(HDest, "smtp:", 5) == 0)
			{
				if (ISP_Gateway_Enabled)
				{
					Vptr = HDest;
					memmove(HDest, &HDest[5], strlen(HDest));
					Msg->to[0] = 0;
				}
			}
			else if (Vptr)
			{
				// If looks like a valid email address, treat as such

				int tolen = (Vptr - Destcopy) - 1;

				if (tolen > 6 || !CheckifPacket(Vptr))
				{
					// Assume Email address

					Vptr = HDest;

					if (FindRMS() || strchr(Vptr, '!')) // have RMS or source route
						strcpy(Msg->to, "RMS");
					else if (ISP_Gateway_Enabled)
						Msg->to[0] = 0;
					else
					{		
						MessageBox(NULL, "Sending to Internet Email not available", "BPQMail", MB_ICONERROR);
						return TRUE;
					}
				}
			}
			if (Vptr)
			{
				if (strlen(Vptr) > 40)
					Vptr[40] = 0;

				strcpy(Msg->via, Vptr);
			}
gotAddr:
			GetDlgItemText(hDlg, IDC_MSGTITLE, Msg->title, 61);
			GetDlgItemText(hDlg, IDC_MSGTYPE, status, 2);
			Msg->type = status[0];
			Msg->status = 'N';

			if (strlen(BID) == 0)
				sprintf_s(BID, sizeof(BID), "%d_%s", LatestMsg, BBSName);

			strcpy(Msg->bid, BID);

			Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

			BIDRec = AllocateBIDRecord();

			strcpy(BIDRec->BID, Msg->bid);
			BIDRec->mode = Msg->type;
			BIDRec->u.msgno = LOWORD(Msg->number);
			BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

			MsgLen = SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, 0 ,0);

			MailBuffer = malloc(MsgLen + TotalFileSize + 2000);		// Allow for a B2 Header if attachments

			if (Files)
			{
				char DateString[80];
				struct tm * tm;
						
				char Type[16] = "Private";
					
				// Get Type
	
				if (Msg->type == 'B')
					strcpy(Type, "Bulletin");
				else if (Msg->type == 'T')
					strcpy(Type, "Traffic");

				// Create a B2 Message

				// B2 Header

				NewMsg = MailBuffer + 1000;

				tm = gmtime((time_t *)&Msg->datecreated);	
	
				sprintf(DateString, "%04d/%02d/%02d %02d:%02d",
					tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

				// Remove last Source Route

				if (strchr(HDest, '!'))
				{
					char * bang = HDest + strlen(HDest);
	
					while (*(--bang) != '!');		// Find last !

					*(bang) = 0;					// remove it;
				}
		
				NewMsg += sprintf(NewMsg,
					"MID: %s\r\nDate: %s\r\nType: %s\r\nFrom: %s\r\nTo: %s\r\nSubject: %s\r\nMbo: %s\r\n",
						Msg->bid, DateString, Type, Msg->from, HDest, Msg->title, BBSName);
				

				NewMsg += sprintf(NewMsg, "Body: %d\r\n", MsgLen);

				for (n = 0; n < Files; n++)
				{
					char * p = FileName[n], * q;

					// Remove any path

					q = strchr(p, '\\');
					
					while (q)
					{
						if (q)
							*q++ = 0;
						p = q;
						q = strchr(p, '\\');
					}

					NewMsg += sprintf(NewMsg, "File: %d %s\r\n", FileLen[n], p);
				}

				NewMsg += sprintf(NewMsg, "\r\n");
				GetDlgItemText(hDlg, IDC_EDIT1, NewMsg, MsgLen+1); 
				NewMsg += MsgLen;
				NewMsg += sprintf(NewMsg, "\r\n");

				for (n = 0; n < Files; n++)
				{
					memcpy(NewMsg, FileBody[n], FileLen[n]);
					NewMsg += FileLen[n];
					free(FileBody[n]);
					NewMsg += sprintf(NewMsg, "\r\n");
				}

				Msg->length = NewMsg - (MailBuffer + 1000);
				NewMsg = MailBuffer + 1000;
				Msg->B2Flags = B2Msg | Attachments;
			}

			else
			{
				GetDlgItemText(hDlg, IDC_EDIT1, MailBuffer, MsgLen+1);
				Msg->length = MsgLen;
				NewMsg = MailBuffer;
			}

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
			hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, NewMsg, Msg->length, &WriteLen, NULL);
				CloseHandle(hFile);
			}

			free(MailBuffer);

			MatchMessagetoBBSList(Msg, 0);

			BuildNNTPList(Msg);				// Build NNTP Groups list

			SaveMessageDatabase();
			SaveBIDDatabase();

			EndDialog(hDlg, LOWORD(wParam));

			return TRUE;
		}


		if (LOWORD(wParam) == IDSelectFiles)
		{
			char FileNames[2048];
			char FullFileNames[32768];
			OPENFILENAME Ofn; 		
			int err;

			FileNames[0] = 0;

			memset(&Ofn, 0, sizeof(Ofn));
 
			Ofn.lStructSize = sizeof(OPENFILENAME); 
			Ofn.hInstance = hInst;
			Ofn.hwndOwner = hDlg; 
			Ofn.lpstrFilter = NULL; 
			Ofn.lpstrFile= FileNames; 
			Ofn.nMaxFile = 2048; 
			Ofn.lpstrFileTitle = NULL; 
			Ofn.nMaxFileTitle = 0; 
			Ofn.lpstrInitialDir = (LPSTR)NULL; 
			Ofn.Flags = OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT |  OFN_EXPLORER; 
			Ofn.lpstrTitle = NULL;//; 

			if (GetOpenFileName(&Ofn))
			{
				// if one is selected, a single string is returned, if more than one, a single
				// path, followed by all the strings, duuble null terminated.

				char * Names[101];			// Allow up to 100 names
				int n = 0;
				char * ptr = FileNames;

				while (*ptr)
				{
					Names[n++] = ptr;
					ptr += strlen(ptr);
					ptr++;
				}

				GetDlgItemText(hDlg, IDC_ATTACHMENTS, FullFileNames, 32768);

				if (strlen(FullFileNames))
						strcat(FullFileNames, ";");

				if (n == 1)
				{
					// Single Select

					strcat(FullFileNames, FileNames);
				}
				else
				{
					int i = 1;

					while(i < n)
					{
						strcat(FullFileNames, Names[0]);
						strcat(FullFileNames, "\\");
						strcat(FullFileNames, Names[i]);
						i++;
						if (i < n)
							strcat(FullFileNames, ";");
					}
				}
				SetDlgItemText(hDlg, IDC_ATTACHMENTS, FullFileNames);
			}
			else
				err = GetLastError();
			return (INT_PTR)TRUE;
		}


		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ClpMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HGLOBAL   hglb; 
    LPTSTR    lptstr; 

	switch (message)
	{
	case WM_INITDIALOG:

       		SetWindowText(hDlg, "Send Message from Clipboard");

			if (!IsClipboardFormatAvailable(CF_TEXT)) 
            break; 

        if (!OpenClipboard(hDlg)) 
            break; 
 
        hglb = GetClipboardData(CF_TEXT); 

        if (hglb != NULL) 
        { 
            lptstr = GlobalLock(hglb);

            if (lptstr != NULL) 
            { 
				SetDlgItemText(hDlg, IDC_EDIT1, lptstr);
				GlobalUnlock(hglb); 
            } 
        } 
        CloseClipboard(); 
	}

	return SendMsgDialogProc(hDlg, message, wParam, lParam);

}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

SMTPMsgs = 0;

int RefreshMainWindow()
{
	char msg[80];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0; 
	time_t now;
	struct tm * tm;
	char tim[20];

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	SMTPMsgs = 0;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in");
				else
				{
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
						conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
						"BBS", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	SetDlgItemInt(hWnd, IDC_MSGS, NumberofMessages, FALSE);

	n = 0;

	for (i=1; i <= NumberofMessages; i++)
	{
		if (MsgHddrPtr[i]->status == 'N')
		{
			if (_stricmp(MsgHddrPtr[i]->to, SYSOPCall) == 0  || _stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)
				SYSOPMsgs++;
			else
			if (MsgHddrPtr[i]->to[0] == 0)
				SMTPMsgs++;
		}
		else
		{
			if (MsgHddrPtr[i]->status == 'H')
				HeldMsgs++;
		}
	}

	SetDlgItemInt(hWnd, IDC_SYSOPMSGS, SYSOPMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_HELD, HeldMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_SMTP, SMTPMsgs, FALSE);

	SetDlgItemInt(hWnd, IDC_MSGSEM, MsgNoSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_ALLOCSEM, AllocSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_CONSEM, ConSemaphore.Clashes, FALSE);

	now = time(NULL);

	tm = gmtime(&now);	
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_UTC, tim);

	tm = localtime(&now);
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_LOCAL, tim);


	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

static SOCKADDR_IN local_sin;  /* Local socket - internet style */

static PSOCKADDR_IN psin;

SOCKET sock;



BOOL Initialise()
{
	int i, len;
	ConnectionInfo * conn;
	struct UserInfo * user = NULL;
	HKEY hKey=0;
	char * ptr1;
	int Attrs, ret;
	char msg[500];
	TIME_ZONE_INFORMATION TimeZoneInformation;
	struct stat STAT;

	GetTimeZoneInformation(&TimeZoneInformation);

	_tzset();
	_MYTIMEZONE = timezone;
	_MYTIMEZONE = TimeZoneInformation.Bias * 60;

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	// See if we need to warn of possible problem with BaseDir moved by installer

	strcpy(BPQDirectory, GetBPQDirectory());

	sprintf(BaseDir, "%s/BPQMailChat", BPQDirectory);
	
	len = strlen(BaseDir);
	ptr1 = BaseDir;

	while (*ptr1)
	{
		if (*(ptr1) == '/') *(ptr1) = '\\';
		ptr1++;
	}

	// Make Sure BASEDIR Exists

	Attrs = GetFileAttributes(BaseDir);

	if (Attrs == -1)
	{
		sprintf_s(msg, sizeof(msg), "Base Directory %s not found - should it be created?", BaseDir);
		ret = MessageBox(NULL, msg, "BPQMail", MB_YESNO);

		if (ret == IDYES)
		{
			ret = CreateDirectory(BaseDir, NULL);
			if (ret == 0)
			{
				MessageBox(NULL, "Failed to created Base Directory - exiting", "BPQMail", MB_ICONSTOP);
				return FALSE;
			}
		}
		else
		{
			MessageBox(NULL, "Can't Continue without a Base Directory - exiting", "BPQMailChat", MB_ICONSTOP);
			return FALSE;
		}
	}
	else
	{
		if (!(Attrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			sprintf_s(msg, sizeof(msg), "Base Directory %s is a file not a directory - exiting", BaseDir);
			ret = MessageBox(NULL, msg, "BPQMail", MB_ICONSTOP);

			return FALSE;
		}
	}

	initUTF8();

	// Set up file and directory names
		
	strcpy(UserDatabasePath, BaseDir);
	strcat(UserDatabasePath, "\\");
	strcat(UserDatabasePath, UserDatabaseName);

	strcpy(MsgDatabasePath, BaseDir);
	strcat(MsgDatabasePath, "\\");
	strcat(MsgDatabasePath, MsgDatabaseName);

	strcpy(BIDDatabasePath, BaseDir);
	strcat(BIDDatabasePath, "\\");
	strcat(BIDDatabasePath, BIDDatabaseName);

	strcpy(WPDatabasePath, BaseDir);
	strcat(WPDatabasePath, "\\");
	strcat(WPDatabasePath, WPDatabaseName);

	strcpy(BadWordsPath, BaseDir);
	strcat(BadWordsPath, "\\");
	strcat(BadWordsPath, BadWordsName);

	strcpy(NTSAliasesPath, BaseDir);
	strcat(NTSAliasesPath, "/");
	strcat(NTSAliasesPath, NTSAliasesName);

	strcpy(MailDir, BaseDir);
	strcat(MailDir, "\\");
	strcat(MailDir, "Mail");

	CreateDirectory(MailDir, NULL);		// Just in case

	strcpy(ConfigName, BaseDir);
	strcat(ConfigName, "\\");
	strcat(ConfigName, "BPQMail.cfg");

	UsingingRegConfig = FALSE;

	//	if config file exists use it else try to get from Registry

	if (stat(ConfigName, &STAT) == -1)
	{
		UsingingRegConfig = TRUE;
	
		if (GetConfigFromRegistry())
		{
			SaveConfig(ConfigName);
		}
		else
		{
			int retCode;

			strcpy(BBSName, GetNodeCall());
			strlop(BBSName, '-');
			strlop(BBSName, ' ');

			sprintf(msg, "No configuration found - Dummy Config created");

			retCode = MessageBox(NULL, msg, "BPQMailChat", MB_OKCANCEL);

			if (retCode == IDCANCEL)
				return FALSE;

			SaveConfig(ConfigName);
		}
	}

	if (GetConfig(ConfigName) == EXIT_FAILURE)
	{
		ret = MessageBox(NULL,
			"BBS Config File seems corrupt - check before continuing", "BPQMail", MB_ICONSTOP);
		return FALSE;
	}

	// Got a Config File
	
	if (MainRect.right < 100 || MainRect.bottom < 100)
	{
		GetWindowRect(MainWnd,	&MainRect);
	}

	MoveWindow(MainWnd, MainRect.left, MainRect.top, MainRect.right-MainRect.left, MainRect.bottom-MainRect.top, TRUE);

	if (OpenMon)
		CreateMonitor();

	BBSApplMask = 1<<(BBSApplNum-1);

	ShowWindow(GetDlgItem(MainWnd, 901), SW_HIDE);
	ShowWindow(GetDlgItem(MainWnd, 902), SW_HIDE);
	ShowWindow(GetDlgItem(MainWnd, 903), SW_HIDE);

	// Make backup copies of Databases
	
	CopyBIDDatabase();
	CopyMessageDatabase();
	CopyUserDatabase();
	CopyWPDatabase();

	SetupMyHA();
	SetupFwdAliases();
	SetupNTSAliases(NTSAliasesPath);

	GetWPDatabase();
	GetMessageDatabase();
	GetUserDatabase();
	GetBIDDatabase();
	GetBadWordFile();
	GetHTMLForms();

	UsingingRegConfig = FALSE;

	// Make sure SYSOPCALL is set

	if (SYSOPCall[0] == 0)
		strcpy(SYSOPCall, BBSName);

	// Make sure there is a user record for the BBS, with BBS bit set.

	user = LookupCall(BBSName);
		
	if (user == NULL)
	{
		user = AllocateUserRecord(BBSName);
		user->Temp = zalloc(sizeof (struct TempUserInfo));
	}

	if ((user->flags & F_BBS) == 0)
	{
		// Not Defined as a BBS

		if (SetupNewBBS(user))
			user->flags |= F_BBS;
	}

	// if forwarding AMPR mail make sure User/BBS AMPR exists

	if (SendAMPRDirect)
	{
		BOOL NeedSave = FALSE;
		
		user = LookupCall("AMPR");
		
		if (user == NULL)
		{
			user = AllocateUserRecord("AMPR");
			user->Temp = zalloc(sizeof (struct TempUserInfo));
			NeedSave = TRUE;
		}

		if ((user->flags & F_BBS) == 0)
		{
			// Not Defined as a BBS

			if (SetupNewBBS(user))
				user->flags |= F_BBS;
			NeedSave = TRUE;
		}

		if (NeedSave)
			SaveUserDatabase();
	}

	// Allocate Streams

	for (i=0; i < MaxStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofStreams++;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, (i == 0 && EnableUI) ? 0x82 : 2, BBSApplMask | ChatApplMask);
		Disconnect(conn->BPQStream);
	}

	InitialiseTCP();

	InitialiseNNTP();

	SetupListenSet();		// Master set of listening sockets

	if (BBSApplNum)
	{
		SetupUIInterface();
		if (MailForInterval)
			_beginthread(SendMailForThread, 0, 0);
	}

	if (cfgMinToTray)
	{
		AddTrayMenuItem(MainWnd, "Mail Server");
	}
	
	SetTimer(hWnd,1,10000,NULL);	// Slow Timer (10 Secs)
	SetTimer(hWnd,2,100,NULL);		// Send to Node and TCP Poll (100 ms)

	// Calulate time to run Housekeeping
	{
		struct tm *tm;
		time_t now;

		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

		MaintClock = _mkgmtime(tm);

		while (MaintClock < now)
			MaintClock += MaintInterval * 3600;

		Debugprintf("Maint Clock %lld NOW %lld Time to HouseKeeping %lld", (long long)MaintClock, (long long)now, (long long)(MaintClock - now));

		if (LastHouseKeepingTime)
		{
			if ((now - LastHouseKeepingTime) > MaintInterval * 3600)
			{
				DoHouseKeeping(FALSE);
			}
		}
	}

	if (strstr(CmdLine, "tidymail"))
		DeleteRedundantMessages();

	if (strstr(CmdLine, "nohomebbs"))
		DontNeedHomeBBS = TRUE;

	if (strstr(CmdLine, "DontCheckFromCall"))
		DontCheckFromCall = TRUE;

	CheckMenuItem(hMenu,IDM_LOGBBS, (LogBBS) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGTCP, (LogTCP) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGCHAT, (LogCHAT) ? MF_CHECKED : MF_UNCHECKED);

	RefreshMainWindow();

//	CreateWPReport();

	CreatePipeThread();
	GetPGConfig();

	return TRUE;
}

int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}

/*
VOID FindNextRMSUser(struct BBSForwardingInfo * FWDInfo)
{
	struct UserInfo * user;

	int i = FWDInfo->UserIndex;

	if (i == -1)
	{
		FWDInfo->UserIndex = FWDInfo->UserCall[0] = 0;	// Not scanning users
	}

	for (i++; i <= NumberofUsers; i++)
	{
		user = UserRecPtr[i];

		if (user->flags & F_POLLRMS)
		{
			FWDInfo->UserIndex = i;
			strcpy(FWDInfo->UserCall, user->Call);
			FWDInfo->FwdTimer = FWDInfo->FwdInterval - 20;
			return ;
		}
	}

	// Finished Scan

	FWDInfo->UserIndex = FWDInfo->FwdTimer = FWDInfo->UserCall[0] = 0;	
}
*/

#ifndef NEWROUTING

VOID SetupHAddreses(struct BBSForwardingInfo * ForwardingInfo)
{
}
VOID SetupMyHA()
{
}
VOID SetupFwdAliases()
{
}

int MatchMessagetoBBSList(struct MsgInfo * Msg, CIRCUIT * conn)
{
	struct UserInfo * bbs;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char * HRoute;
	int Count =0;

	strcpy(ATBBS, Msg->via);
	HRoute = strlop(ATBBS, '.');

	if (Msg->type == 'P')
	{
		// P messages are only sent to one BBS, but check the TO and AT of all BBSs before routing on HA

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSToList(Msg, bbs, ForwardingInfo))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSAtList(Msg, ForwardingInfo, ATBBS))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSHList(Msg, bbs, ForwardingInfo, ATBBS, HRoute))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		return FALSE;
	}

	// Bulls go to all matching BBSs, so the order of checking doesn't matter

	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		ForwardingInfo = bbs->ForwardingInfo;

		if (CheckABBS(Msg, bbs, ForwardingInfo, ATBBS, HRoute))		
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
			}
			Count++;
		}
	}

	return Count;
}
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** Calls;
	char ** HRoutes;
	int i, j;

	if (strcmp(ATBBS, bbs->Call) == 0)					// @BBS = BBS
		return TRUE;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}

	// Check AT distributions

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}


	return FALSE;

}

BOOL CheckBBSToList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo)
{
	char ** Calls;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSAtList(struct MsgInfo * Msg, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS)
{
	char ** Calls;

	// Check AT distributions

	if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS
		return TRUE;

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSHList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** HRoutes;
	int i, j;

	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}
	return FALSE;
}

#endif

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;

	if (buf == NULL) return NULL;		// Protect

	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}
