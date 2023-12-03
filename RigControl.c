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
//	Rig Control Module
//

// Dec 29 2009

//	Add Scan Control for SCS 

// August 2010

// Fix logic error in Port Initialisation (wasn't always raising RTS and DTR
// Clear RTS and DTR on close

// Fix Kenwood processing of multiple messages in one packet.

// Fix reporting of set errors in scan to the wrong session


// Yaesu List

// FT990 define as FT100


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE


#include <stdio.h>
#include <stdlib.h>
#include "time.h"

#include "CHeaders.h"
#include "tncinfo.h"
#ifdef WIN32
#include <commctrl.h>
#else
char *fcvt(double number, int ndigits, int *decpt, int *sign);  
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include "bpq32.h"

#include "hidapi.h"

int Row = -20;

extern struct PORTCONTROL * PORTTABLE;

VOID __cdecl Debugprintf(const char * format, ...);

struct RIGINFO * RigConfig(struct TNCINFO * TNC, char * buf, int Port);
struct RIGPORTINFO * CreateTTYInfo(int port, int speed);
BOOL RigCloseConnection(struct RIGPORTINFO * PORT);
BOOL RigWriteCommBlock(struct RIGPORTINFO * PORT);
BOOL DestroyTTYInfo(int port);
void CheckRX(struct RIGPORTINFO * PORT);
static int OpenRigCOMMPort(struct RIGPORTINFO * PORT, VOID * Port, int Speed);
VOID ICOMPoll(struct RIGPORTINFO * PORT);
VOID ProcessFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuff, int len);
VOID ProcessICOMFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuffer, int Len);
int SendResponse(int Stream, char * Msg);
VOID ProcessYaesuFrame(struct RIGPORTINFO * PORT);
VOID YaesuPoll(struct RIGPORTINFO * PORT);
VOID ProcessYaesuCmdAck(struct RIGPORTINFO * PORT);
VOID ProcessKenwoodFrame(struct RIGPORTINFO * PORT, int Length);
VOID KenwoodPoll(struct RIGPORTINFO * PORT);
VOID DummyPoll(struct RIGPORTINFO * PORT);
VOID SwitchAntenna(struct RIGINFO * RIG, char Antenna);
VOID DoBandwidthandAntenna(struct RIGINFO *RIG, struct ScanEntry * ptr);
VOID SetupScanInterLockGroups(struct RIGINFO *RIG);
VOID ProcessFT100Frame(struct RIGPORTINFO * PORT);
VOID ProcessFT990Frame(struct RIGPORTINFO * PORT);
VOID ProcessFT1000Frame(struct RIGPORTINFO * PORT);
VOID AddNMEAChecksum(char * msg);
VOID ProcessNMEA(struct RIGPORTINFO * PORT, char * NMEAMsg, int len);
VOID COMSetDTR(HANDLE fd);
VOID COMClearDTR(HANDLE fd);
VOID COMSetRTS(HANDLE fd);
VOID COMClearRTS(HANDLE fd);
void CM108_set_ptt(struct RIGINFO *RIG, int PTTState);
BOOL OpenHIDPort(struct RIGPORTINFO * PORT, VOID * Port, int Speed);
int HID_Read_Block(struct RIGPORTINFO * PORT);
int HID_Write_Block(struct RIGPORTINFO * PORT);
HANDLE rawhid_open(char * Device);
int rawhid_recv(int num, void *buf, int len, int timeout);
int rawhid_send(int num, void *buf, int len, int timeout);
void rawhid_close(int num);
VOID ConnecttoHAMLIB(struct RIGPORTINFO * PORT);
VOID ConnecttoFLRIG(struct RIGPORTINFO * PORT);
int DecodeHAMLIBAddr(struct RIGPORTINFO * PORT, char * ptr);
void ProcessHAMLIBFrame(struct RIGPORTINFO * PORT, int Length);
VOID HAMLIBPoll(struct RIGPORTINFO * PORT);
void HAMLIBSlaveThread(struct RIGINFO * RIG);
void CheckAndProcessRTLUDP(struct RIGPORTINFO * PORT);
VOID RTLUDPPoll(struct RIGPORTINFO * PORT);
VOID ConnecttoRTLUDP(struct RIGPORTINFO * PORT);
VOID FLRIGPoll(struct RIGPORTINFO * PORT);
void ProcessFLRIGFrame(struct RIGPORTINFO * PORT);
VOID FLRIGSendCommand(struct RIGPORTINFO * PORT, char * Command, char * Value);

VOID ProcessSDRRadioFrame(struct RIGPORTINFO * PORT, int Length);
VOID SDRRadioPoll(struct RIGPORTINFO * PORT);

VOID SetupPortRIGPointers();
VOID PTTCATThread(struct RIGINFO *RIG);
VOID ConnecttoHAMLIB(struct RIGPORTINFO * PORT);

// ----- G7TAJ ----
VOID ConnecttoSDRANGEL(struct RIGPORTINFO * PORT);
VOID SDRANGELPoll(struct RIGPORTINFO * PORT);
void ProcessSDRANGELFrame(struct RIGPORTINFO * PORT);
VOID SDRANGELSendCommand(struct RIGPORTINFO * PORT, char * Command, char * Value);
void SDRANGELProcessMessage(struct RIGPORTINFO * PORT);

// ----- G7TAJ ----


int SendPTCRadioCommand(struct TNCINFO * TNC, char * Block, int Length);
int GetPTCRadioCommand(struct TNCINFO * TNC, char * Block);
int BuildRigCtlPage(char * _REPLYBUFFER);
void SendRigWebPage();

extern  TRANSPORTENTRY * L4TABLE;
HANDLE hInstance;

VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset); 
BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase);

char * GetApplCallFromName(char * App);

char Modes[25][6] = {"LSB",  "USB", "AM", "CW", "RTTY", "FM", "WFM", "CW-R", "RTTY-R",
					"????","????","????","????","????","????","????","????","DV", "LSBD1",
					"USBD1", "LSBD2","USBD2", "LSBD3","USBD3", "????"};

/*
DV = 17
F8101
(0000=LSB, 0001=USB, 0002=AM,
0003=CW, 0004=RTTY,
0018=LSB D1, 0019=USB D1,
0020=LSB D2, 0021=USB D2,
0022=LSB D3, 0023=USB D3
*/
//							0		1	  2		3	   4	5	6	7	8	9		0A  0B    0C    88

char YaesuModes[16][6] = {"LSB",  "USB", "CW", "CWR", "AM", "", "", "", "FM", "", "DIG", "", "PKT", "FMN", "????"};

char FT100Modes[9][6] = {"LSB",  "USB", "CW", "CWR", "AM", "DIG", "FM", "WFM", "????"};

char FT990Modes[13][6] = {"LSB",  "USB", "CW2k4", "CW500", "AM6k", "AM2k4", "FM", "FM", "RTTYL", "RTTYU", "PKTL", "PKTFM", "????"};

char FT1000Modes[13][6] = {"LSB",  "USB", "CW", "CWR", "AM", "AMS", "FM", "WFM", "RTTYL", "RTTYU", "PKTL", "PKTF", "????"};

char FTRXModes[8][6] = {"LSB", "USB", "CW", "AM", "FM", "RTTY", "PKT", ""};

char KenwoodModes[16][6] = {"????", "LSB",  "USB", "CW", "FM", "AM", "FSK", "????"};

char FT2000Modes[16][6] = {"????", "LSB",  "USB", "CW", "FM", "AM", "FSK", "CW-R", "PKT-L", "FSK-R", "PKT-FM", "FM-N", "PKT-U", "????"};

char FTDX10Modes[16][9] = {"????", "LSB",  "USB", "CW-U", "FM", "AM", "RTTY-L", "CW-L", "DATA-L", "RTTY-U", "DATA-FM", "FM-N", "DATA-U", "AM-N", "PSK", "DATA-FM-N"};

char FT991AModes[16][9] = {"????", "LSB",  "USB", "CW-U", "FM", "AM", "RTTY-LSB", "CW-L", "DATA-LSB", "RTTY-USB", "DATA-FM", "FM-N", "DATA-USB", "AM-N", "C4FM", "????"};

char FLEXModes[16][6] = {"LSB", "USB", "DSB", "CWL", "CWU", "FM", "AM", "DIGU", "SPEC", "DIGL", "SAM", "DRM"};

char AuthPassword[100] = "";

char LastPassword[17];

int NumberofPorts = 0;

BOOL EndPTTCATThread = FALSE;

int HAMLIBMasterRunning = 0;
int HAMLIBSlaveRunning = 0;
int FLRIGRunning = 0;

// ---- G7TAJ ----
int SDRANGELRunning = 0;
// ---- G7TAJ ----

char * RigWebPage = 0;
int RigWebPageLen = 0;


struct RIGPORTINFO * PORTInfo[MAXBPQPORTS + 2] = {NULL};		// Records are Malloc'd

struct RIGINFO * DLLRIG = NULL;			// Rig record for dll PTT interface (currently only for UZ7HO);


struct TimeScan * AllocateTimeRec(struct RIGINFO * RIG)
{
	struct TimeScan * Band = zalloc(sizeof (struct TimeScan));
	
	RIG->TimeBands = realloc(RIG->TimeBands, (++RIG->NumberofBands+2) * sizeof(void *));
	RIG->TimeBands[RIG->NumberofBands] = Band;
	RIG->TimeBands[RIG->NumberofBands+1] = NULL;

	return Band;
}

struct ScanEntry ** CheckTimeBands(struct RIGINFO * RIG)
{
	int i = 0;
	time_t NOW = time(NULL) % 86400;
				
	// Find TimeBand

	while (i < RIG->NumberofBands)
	{
		if (RIG->TimeBands[i + 1]->Start > NOW)
		{
			break;
		}
		i++;
	}

	RIG->FreqPtr = RIG->TimeBands[i]->Scanlist;

	return RIG->FreqPtr;
}

VOID Rig_PTTEx(struct RIGINFO * RIG, BOOL PTTState, struct TNCINFO * TNC);

VOID Rig_PTT(struct TNCINFO * TNC, BOOL PTTState)
{
	if (TNC == NULL) return;

	if (TNC->TXRIG)
		Rig_PTTEx(TNC->TXRIG, PTTState, TNC);
	else
		Rig_PTTEx(TNC->RIG, PTTState, TNC);
}

VOID Rig_PTTEx(struct RIGINFO * RIG, BOOL PTTState, struct TNCINFO * TNC)
{
	struct RIGPORTINFO * PORT;
	int i, Len;
	char cmd[32];
	char onString[128];			// Actual CAT strings to send. May be modified for QSY on PTT
	char offString[128];
	int onLen = 0, offLen = 0;

	if (RIG == NULL) return;

	PORT = RIG->PORT;

	if (PORT == NULL)
		return;

	// CAT string defaults to that set up by RIGConfig in RIG->PTTOn and RIG->PTTOff,
	// but can be overidden by Port specify strings from TNC->PTTOn and TNC->PTTOff.
	// If PTTSetsFreq is set on a Rig, that overrides the RIG->PTTOn but not TNC->PTTOn


	if (PTTState)
	{
		MySetWindowText(RIG->hPTT, "T");
		RIG->WEB_PTT = 'T';
		RIG->PTTTimer = PTTLimit;
		RIG->repeatPTTOFFTimer = 0;				// Cancel repeated off command

		if (TNC && TNC->PTTOn[0])
		{
			memcpy(onString, TNC->PTTOn, TNC->PTTOnLen);
			onLen = TNC->PTTOnLen;
		}
		else
		{
			memcpy(onString, RIG->PTTOn, RIG->PTTOnLen);
			onLen = RIG->PTTOnLen;

			// If PTT_SETS_FREQ set calculate TX Freq and see if changed

			// Freq can be set on the TNC, the RIG, or calculated from current rx freq + pttOffset

			if (TNC && RIG->PTTSetsFreq)
			{
				long long txfreq = 0;

				if (TNC->TXFreq)
					txfreq = TNC->TXFreq + TNC->TXOffset + RIG->txError;
				else if (TNC->RIG && TNC->RIG->txFreq)
					txfreq = RIG->txFreq;		// Used if not associated with a TNC port - eg HAMLIB + WSJT
				else if (TNC->RIG && TNC->RIG->RigFreq != 0.0)
				{
					// Use RigFreq + pttOffset, so TX Tracks RX
					
					long long rxfreq = (long long)(TNC->RIG->RigFreq * 1000000.0) - TNC->RIG->rxOffset;
					txfreq = rxfreq + RIG->pttOffset + RIG->txError;
					txfreq += RIG->rxError;
				}

				if (txfreq)
				{
					if (RIG->lastSetFreq != txfreq)
					{
						char FreqString[80];
						char * CmdPtr = onString;
						UCHAR * Poll = PORT->TXBuffer;


						RIG->lastSetFreq = txfreq;

						// Convert to CAT string

						sprintf(FreqString, "%012d", txfreq);

						switch (PORT->PortType)
						{
						case ICOM:

							// CI-V must send all commands as one string, or Radio will start to ack them and
							// collide with rest of command

							// Set Freq is sent before set PTT, so set up QSY string then copy PTT string to buffer
							// Need to convert two chars to bcd digit

							*(CmdPtr++) = 0xFE;
							*(CmdPtr++) = 0xFE;
							*(CmdPtr++) = RIG->RigAddr;
							*(CmdPtr++) = 0xE0;
							*(CmdPtr++) = 0x5;		// Set frequency command

							*(CmdPtr++) = (FreqString[11] - 48) | ((FreqString[10] - 48) << 4);
							*(CmdPtr++) = (FreqString[9] - 48) | ((FreqString[8] - 48) << 4);
							*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
							*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		
							if (RIG->IC735)
							{
								*(CmdPtr++) = 0xFD;
								onLen = 10;
							}
							else
							{
								*(CmdPtr++) = (FreqString[3] - 48);
								*(CmdPtr++) = 0xFD;
								onLen = 11;
							}

							// Now add PTT String

							memcpy(&onString[onLen], RIG->PTTOn, RIG->PTTOnLen);
							onLen += RIG->PTTOnLen;

							break;

						case YAESU:

							*(Poll++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
							*(Poll++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
							*(Poll++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
							*(Poll++) = (FreqString[10] - 48) | ((FreqString[9] - 48) << 4);
							*(Poll++) = 1;		// Set Freq

							PORT->TXLen = 5;
							RigWriteCommBlock(PORT);


							if (RIG->PTTMode & PTTCI_V)
							{
								Sleep(150);
								Poll = PORT->TXBuffer;
								*(Poll++) = 0;
								*(Poll++) = 0;
								*(Poll++) = 0;

								*(Poll++) = 0;
								*(Poll++) = PTTState ? 0x08 : 0x88;		// CMD = 08 : PTT ON CMD = 88 : PTT OFF

								PORT->TXLen = 5;
								RigWriteCommBlock(PORT);
							}

							PORT->Retries = 1;
							PORT->Timeout = 0;

							return;



						case HAMLIB:

							// Dont need to save, as we can send strings separately

							Len = sprintf(cmd, "F %lld\n", txfreq);
							i = send(PORT->remoteSock, cmd, Len, 0);
							RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF

							break;

						}
					}
				}
			}
		}
	}
	else
	{
		// Drop PTT

		MySetWindowText(RIG->hPTT, " ");
		RIG->WEB_PTT = ' ';
		RIG->PTTTimer = 0;
		if (PORT->PortType == ICOM)
			RIG->repeatPTTOFFTimer = 300;			// set 30 second repeated off command

		if (TNC && TNC->PTTOff[0])
		{
			memcpy(offString, TNC->PTTOff, TNC->PTTOffLen);
			offLen = TNC->PTTOffLen;
			RIG->lastSetFreq = 0;
		}
		else
		{
			memcpy(offString, RIG->PTTOff, RIG->PTTOffLen);
			offLen = RIG->PTTOffLen;

			// If PTT_SETS_FREQ set calculate TX Freq and see if changed

			if (PTTState == 0 && RIG->PTTSetsFreq && RIG->defaultFreq)
			{
				// Dropped PTT. See if need to set freq back to default

				long long txfreq = RIG->defaultFreq + RIG->txError;

				if (RIG->lastSetFreq != txfreq)
				{
					char FreqString[80];
					char * CmdPtr = offString;
					UCHAR * Poll = PORT->TXBuffer;

					RIG->lastSetFreq = txfreq;

					// Convert to CAT string

					sprintf(FreqString, "%012d", txfreq);

					switch (PORT->PortType)
					{
					case ICOM:

						// CI-V must send all commands as one string, or Radio will start to ack them and
						// collide with rest of command

						// Set Freq is sent after drop PTT, so copy PTT string to buffer then set up QSY string  
						// Need to convert two chars to bcd digit

						// We copied off string earlier, so just append QSY string

						CmdPtr += offLen;

						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x5;		// Set frequency command

						*(CmdPtr++) = (FreqString[11] - 48) | ((FreqString[10] - 48) << 4);
						*(CmdPtr++) = (FreqString[9] - 48) | ((FreqString[8] - 48) << 4);
						*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
						*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
						if (RIG->IC735)
						{
							*(CmdPtr++) = 0xFD;
							offLen += 10;
						}
						else
						{
							*(CmdPtr++) = (FreqString[3] - 48);
							*(CmdPtr++) = 0xFD;
							offLen += 11;
						}

					case FLRIG:
		
						sprintf(cmd, "<double>%lld</double>", txfreq);
						FLRIGSendCommand(PORT, "rig.set_vfo", cmd);
						RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF

					case YAESU:

						// Easier to add PTT string, send and return;

						Len = 0;

						if (RIG->PTTMode & PTTCI_V)
						{
							*(Poll++) = 0;
							*(Poll++) = 0;
							*(Poll++) = 0;
							*(Poll++) = 0;
							*(Poll++) = PTTState ? 0x08 : 0x88;		// CMD = 08 : PTT ON CMD = 88 : PTT OFF

							PORT->TXLen = 5;
							RigWriteCommBlock(PORT);
							Poll = PORT->TXBuffer;
							Sleep(100);
						}

						*(Poll++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
						*(Poll++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
						*(Poll++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
						*(Poll++) = (FreqString[10] - 48) | ((FreqString[9] - 48) << 4);
						*(Poll++) = 1;		// Set Freq

						PORT->TXLen = 5;
						RigWriteCommBlock(PORT);

						PORT->Retries = 1;
						PORT->Timeout = 0;

						return;

					case HAMLIB:

						// Dont need to save, as we can send strings separately

						Len = sprintf(cmd, "F %lld\n", txfreq);
						send(PORT->remoteSock, cmd, Len, 0);
						RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF
					}
				}
			}
		}
	}

	// Now send the command

	if (RIG->PTTMode & PTTCI_V)
	{
		UCHAR * Poll = PORT->TXBuffer;

		// Don't read for 10 secs to avoid clash with PTT OFF
		// Should do this for all rigs on port

		for (i = 0; i< PORT->ConfiguredRigs; i++)
			PORT->Rigs[i].PollCounter = 100;

		PORT->AutoPoll = TRUE;

		switch (PORT->PortType)
		{
		case ICOM:
		case KENWOOD:
		case FT2000:
		case FTDX10:
		case FT991A:
		case FLEX:
		case NMEA:

			if (PTTState)
			{
				memcpy(Poll, onString, onLen);
				PORT->TXLen = onLen;
			}
			else
			{
				memcpy(Poll, offString, offLen);
				PORT->TXLen = offLen;
			}

			RigWriteCommBlock(PORT);

			if (PORT->PortType == ICOM && !PTTState)
				RigWriteCommBlock(PORT); // Send ICOM PTT OFF Twice

			PORT->Retries = 1;
			
			if (PORT->PortType != ICOM)
				PORT->Timeout = 0;
			
			return;

		case FT100:
		case FT990:
		case FT1000:

			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = PTTState;	// OFF/ON
			*(Poll++) = 15;
	
			PORT->TXLen = 5;
			RigWriteCommBlock(PORT);

			PORT->Retries = 1;
			PORT->Timeout = 0;

			return;

		case YAESU:  // 897 - maybe others

			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = PTTState ? 0x08 : 0x88;		// CMD = 08 : PTT ON CMD = 88 : PTT OFF
	
			PORT->TXLen = 5;
			RigWriteCommBlock(PORT);

			PORT->Retries = 1;
			PORT->Timeout = 0;

			return;

		case FLRIG:
		
			sprintf(cmd, "<i4>%d</i4>", PTTState);
			FLRIGSendCommand(PORT, "rig.set_ptt", cmd);
			RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF

			return;

		case HAMLIB:
	
			Len = sprintf(cmd, "T %d\n", PTTState);
			send(PORT->remoteSock, cmd, Len, 0);
			RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF
		
			return;
		}
	}

	if (RIG->PTTMode & PTTRTS)
		if (PTTState)
			COMSetRTS(PORT->hPTTDevice);
		else
			COMClearRTS(PORT->hPTTDevice);

	if (RIG->PTTMode & PTTDTR)
		if (PTTState)
			COMSetDTR(PORT->hPTTDevice);
		else
			COMClearDTR(PORT->hPTTDevice);

	if (RIG->PTTMode & PTTCM108)
		CM108_set_ptt(RIG, PTTState);

	if (RIG->PTTMode & PTTHAMLIB)
	{
		char Msg[16];
		int Len = sprintf(Msg, "T %d\n", PTTState);
	
		Len = send(PORT->remoteSock, Msg, Len, 0);
		RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF
	}
	if (RIG->PTTMode & PTTFLRIG)
	{
		char cmd[32];

		sprintf(cmd, "<i4>%d</i4>", PTTState);
		FLRIGSendCommand(PORT, "rig.set_ptt", cmd);
		RIG->PollCounter = 100;		// Don't read for 10 secs to avoid clash with PTT OFF
	}
}

void saveNewFreq(struct RIGINFO * RIG, double Freq, char * Mode)
{
	if (Freq > 0.0)
	{
		_gcvt((Freq + RIG->rxOffset) / 1000000.0, 9, RIG->Valchar);
		strcpy(RIG->WEB_FREQ, RIG->Valchar);
		MySetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
	}

	if (Mode[0])
	{
		strcpy(RIG->ModeString, Mode);
		MySetWindowText(RIG->hMODE, Mode);
	}

}

// Need version that doesn't need Port Number

int Rig_CommandEx(struct RIGPORTINFO * PORT, struct RIGINFO * RIG, TRANSPORTENTRY * Session, char * Command);

int Rig_Command(TRANSPORTENTRY * Session, char * Command)
{
	char * ptr;
	int i, n, p, Port;
	TRANSPORTENTRY * L4 = L4TABLE;
	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;

	//	Only Allow RADIO from Secure Applications

	_strupr(Command);

	ptr = strchr(Command, 13);
	if (ptr) *(ptr) = 0;						// Null Terminate

	if (memcmp(Command, "AUTH ", 5) == 0)
	{
		if (AuthPassword[0] && (memcmp(LastPassword, &Command[5], 16) != 0))
		{
			if (CheckOneTimePassword(&Command[5], AuthPassword))
			{
				Session->Secure_Session = 1;

				sprintf(Command, "Ok\r");

				memcpy(LastPassword, &Command[5], 16);	// Save

				return FALSE;
			}
		}
		
		sprintf(Command, "Sorry AUTH failed\r");
		return FALSE;
	}

	if (Session != (TRANSPORTENTRY *) -1)				// Used for internal Stop/Start
	{		
		if (Session->Secure_Session == 0)
		{
			sprintf(Command, "Sorry - you are not allowed to use this command\r");
			return FALSE;
		}
	}
	if (NumberofPorts == 0)
	{
		sprintf(Command, "Sorry - Rig Control not configured\r");
		return FALSE;
	}

	// if Port starts with 'R' then select Radio (was Interlock) number, not BPQ Port

	if (Command[0] == 'R')
	{
		n = sscanf(&Command[1],"%d ", &Port);

		for (p = 0; p < NumberofPorts; p++)
		{
			PORT = PORTInfo[p];
	
			for (i=0; i< PORT->ConfiguredRigs; i++)
			{
				RIG = &PORT->Rigs[i];

				if (RIG->Interlock == Port)
					goto portok;
			}
		}

		sprintf(Command, "Sorry - Port not found\r");
		return FALSE;
	}

	n = sscanf(Command,"%d ", &Port);

	// Look for the port 


	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->BPQPort & ((uint64_t)1 << Port))
				goto portok;
		}
	}

	sprintf(Command, "Sorry - Port not found\r");
	return FALSE;

portok:

	return Rig_CommandEx(PORT, RIG, Session, Command);
}



static char MsgHddr[] = "POST /RPC2 HTTP/1.1\r\n"
					"User-Agent: XMLRPC++ 0.8\r\n"
					"Host: 127.0.0.1:7362\r\n"
					"Content-Type: text/xml\r\n"
					"Content-length: %d\r\n"
					"\r\n%s";

static char Req[] = "<?xml version=\"1.0\"?>\r\n"
					"<methodCall><methodName>%s</methodName>\r\n"
					"%s"
					"</methodCall>\r\n";


// ---- G7TAJ ----
static char SDRANGEL_MsgHddr[] = "PATCH HTTP/1.1\r\n"
					"User-Agent: BPQ32\r\n"
					"Host: %s\r\n"
					"accept: application/json"
				 	"Content-Type: application/json"
					"Content-length: %d\r\n"
					"\r\n%s";

static char SDRANGEL_FREQ_DATA[] = "{"
    "\"deviceHwType\": \"%s\", "
    "\"direction\": 0,"
    "\"rtlSdrSettings\": {"
    "  \"centerFrequency\": \"%s\""
    "}}";

//freq =  10489630000

// ---- G7TAJ ----



int Rig_CommandEx(struct RIGPORTINFO * PORT, struct RIGINFO * RIG, TRANSPORTENTRY * Session, char * Command)
{
	int n, ModeNo, Filter, Port = 0;
	double Freq = 0.0;
	char FreqString[80]="", FilterString[80]="", Mode[80]="", Data[80] = "", Dummy[80] = "";
	struct MSGWITHOUTLEN * buffptr;
	UCHAR * Poll;

	int i;
	TRANSPORTENTRY * L4 = L4TABLE;
	char * ptr;
	int Split, DataFlag, Bandwidth, Antenna;
	struct ScanEntry * FreqPtr;
	char * CmdPtr;
	int Len;
	char MemoryBank = 0;	// For Memory Scanning
	int MemoryNumber = 0;

	//	Only Allow RADIO from Secure Applications

	_strupr(Command);

	ptr = strchr(Command, 13);
	if (ptr) *(ptr) = 0;						// Null Terminate

	if (memcmp(Command, "AUTH ", 5) == 0)
	{
		if (AuthPassword[0] && (memcmp(LastPassword, &Command[5], 16) != 0))
		{
			if (CheckOneTimePassword(&Command[5], AuthPassword))
			{
				Session->Secure_Session = 1;

				sprintf(Command, "Ok\r");

				memcpy(LastPassword, &Command[5], 16);	// Save

				return FALSE;
			}
		}
		
		sprintf(Command, "Sorry AUTH failed\r");
		return FALSE;
	}

	if (Session != (TRANSPORTENTRY *) -1)				// Used for internal Stop/Start
	{		
		if (Session->Secure_Session == 0)
		{
			sprintf(Command, "Sorry - you are not allowed to use this command\r");
			return FALSE;
		}
	}
	if (NumberofPorts == 0)
	{
		sprintf(Command, "Sorry - Rig Control not configured\r");
		return FALSE;
	}

	// if Port starts with 'R' then select Radio (was Interlock) number, not BPQ Port

	if (Command[0] == 'R')
		n = sscanf(Command,"%s %s %s %s %s", &Dummy, &FreqString[0], &Mode[0], &FilterString[0], &Data[0]);
	else
		n = sscanf(Command,"%d %s %s %s %s", &Port, &FreqString[0], &Mode[0], &FilterString[0], &Data[0]);

	if (_stricmp(FreqString, "CLOSE") == 0)
	{
		PORT->Closed = 1;
		RigCloseConnection(PORT);

		MySetWindowText(RIG->hSCAN, "C");
		RIG->WEB_SCAN = 'C';

		sprintf(Command, "Ok\r");
		return FALSE;
	}

	if (_stricmp(FreqString, "OPEN") == 0)
	{
		PORT->ReopenDelay = 300;
		PORT->Closed = 0;
		
		MySetWindowText(RIG->hSCAN, "");
		RIG->WEB_SCAN = ' ';

		sprintf(Command, "Ok\r");
		return FALSE;
	}

	if (n > 1)
	{
		if (_stricmp(FreqString, "SCANSTART") == 0)
		{
			if (RIG->NumberofBands)
			{
				RIG->ScanStopped &= (0xffffffffffffffff ^ ((uint64_t)1 << Port));

				if (Session != (TRANSPORTENTRY *) -1)				// Used for internal Stop/Start
					RIG->ScanStopped &= 0xfffffffffffffffe;			// Clear Manual Stopped Bit

				if (n > 2)
					RIG->ScanCounter = atoi(Mode) * 10;  //Start Delay
				else
					RIG->ScanCounter = 10;

				RIG->WaitingForPermission = FALSE;		// In case stuck	

				if (RIG->ScanStopped == 0)
				{
					SetWindowText(RIG->hSCAN, "S");
					RIG->WEB_SCAN = 'S';
				}
				sprintf(Command, "Ok\r");

				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 SCANSTART Port %d", Port);
			}
			else
				sprintf(Command, "Sorry no Scan List defined for this port\r");

			return FALSE;
		}

		if (_stricmp(FreqString, "SCANSTOP") == 0)
		{
			RIG->ScanStopped |= ((uint64_t)1 << Port);

			if (Session != (TRANSPORTENTRY *) -1)				// Used for internal Stop/Start
				RIG->ScanStopped |= 1;		// Set Manual Stopped Bit

			MySetWindowText(RIG->hSCAN, "");
			RIG->WEB_SCAN = ' ';

			sprintf(Command, "Ok\r");

			if (RIG->RIG_DEBUG)
				Debugprintf("BPQ32 SCANSTOP Port %d", Port);

			RIG->PollCounter = 50 / RIG->PORT->ConfiguredRigs;	// Dont read freq for 5 secs

			return FALSE;
		}
	}



	if (RIG->RIGOK == 0)
	{
		if (Session != (TRANSPORTENTRY *) -1)
		{
			if (PORT->Closed)
				sprintf(Command, "Sorry - Radio port closed\r");
			else
				sprintf(Command, "Sorry - Radio not responding\r");
		}
		return FALSE;
	}

	if (n == 2 && _stricmp(FreqString, "FREQ") == 0)
	{
		if (RIG->Valchar[0])
			sprintf(Command, "Frequency is %s MHz\r", RIG->Valchar);
		else
			sprintf(Command, "Frequency not known\r");

		return FALSE;
	}

	if (n == 2 && _stricmp(FreqString, "PTT") == 0)
	{
		Rig_PTTEx(RIG, TRUE, NULL);
		RIG->PTTTimer = 10;				// 1 sec
		sprintf(Command, "Ok\r");
		return FALSE;
	}

	if (Session != (void *)-1)
	{
		if (Session->CIRCUITINDEX == 255)
			RIG->Session = -1;
		else
			RIG->Session = Session->CIRCUITINDEX;		// BPQ Stream
	
		RIG->PollCounter = 50;		// Dont read freq for 5 secs in case clash with Poll
	}

	if (_stricmp(FreqString, "TUNE") == 0)
	{
		char ReqBuf[256];
		char SendBuff[256];
		char FLPoll[80];

		switch (PORT->PortType)
		{ 
		case ICOM:

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				sprintf(Command, "Sorry - No Buffers available\r");
				return FALSE;
			}

			
			// Build a ScanEntry in the buffer

			FreqPtr = (struct ScanEntry *)buffptr->Data;
			memset(FreqPtr, 0, sizeof(struct ScanEntry));

			CmdPtr = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;
			FreqPtr->Cmd2 = NULL;
			FreqPtr->Cmd3 = NULL;

			// IC7100 Tune  Fe fe 88 e0 1c 01 02 fd


			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;
			*(CmdPtr++) = 0x1C;
			*(CmdPtr++) = 0x01;
			*(CmdPtr++) = 0x02;
			*(CmdPtr++) = 0xFD;
			FreqPtr[0].Cmd1Len = 8;
			
			C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

			return TRUE;

		case KENWOOD:

			buffptr = GetBuff();
	
			if (buffptr == 0)
			{
				sprintf(Command, "Sorry - No Buffers available\r");
				return FALSE;
			}

			// Build a ScanEntry in the buffer

			FreqPtr = (struct ScanEntry *)buffptr->Data;
			memset(FreqPtr, 0, sizeof(struct ScanEntry));
	
			Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

			FreqPtr->Cmd1Len = sprintf(Poll, "AC111;AC;");

			C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);
			return TRUE;

		case FLRIG:

			strcpy(FLPoll, "rig.tune");

			Len = sprintf(ReqBuf, Req, FLPoll, "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

			if (PORT->CONNECTED)
			{
				if (send(PORT->remoteSock, SendBuff, Len, 0) != Len)
				{
					if (PORT->remoteSock)
						closesocket(PORT->remoteSock);

					PORT->remoteSock = 0;
					PORT->CONNECTED = FALSE;
					PORT->hDevice = 0;	
				}
			}
			sprintf(Command, "Ok\r");
			return FALSE;
		}

		sprintf(Command, "Sorry - TUNE not supported on your radio\r");
		return FALSE;
	}

	if (_stricmp(FreqString, "POWER") == 0)
	{
		char PowerString[8] = "";
		int Power = atoi(Mode);
		int len;
		char cmd[80];

		switch (PORT->PortType)
		{ 
		case ICOM:

			if (n != 3 || Power > 255)
			{
				strcpy(Command, "Sorry - Invalid Format - should be POWER Level (0 - 255)\r");
				return FALSE;
			}

			sprintf(PowerString, "%04d", Power);

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				sprintf(Command, "Sorry - No Buffers available\r");
				return FALSE;
			}

			
			// Build a ScanEntry in the buffer

			FreqPtr = (struct ScanEntry *)buffptr->Data;
			memset(FreqPtr, 0, sizeof(struct ScanEntry));

			CmdPtr = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;
			FreqPtr->Cmd2 = NULL;
			FreqPtr->Cmd3 = NULL;

			// IC7100 Set Power Fe fe 88 e0 14 0a xx xx fd


			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;
			*(CmdPtr++) = 0x14;
			*(CmdPtr++) = 0x0A;

			// Need to convert param to decimal digits

			*(CmdPtr++) = (PowerString[1] - 48) | ((PowerString[0] - 48) << 4);
			*(CmdPtr++) = (PowerString[3] - 48) | ((PowerString[2] - 48) << 4);

			*(CmdPtr++) = 0xFD;
			FreqPtr[0].Cmd1Len = 9;
			
			C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

			return TRUE;

		case KENWOOD:

			if (n != 3 || Power > 200 || Power < 5)
			{
				strcpy(Command, "Sorry - Invalid Format - should be POWER Level (5 - 200)\r");
				return FALSE;
			}

			buffptr = GetBuff();

			if (buffptr == 0)
			{
				sprintf(Command, "Sorry - No Buffers available\r");
				return FALSE;
			}

			// Build a ScanEntry in the buffer

			FreqPtr = (struct ScanEntry *)buffptr->Data;
			memset(FreqPtr, 0, sizeof(struct ScanEntry));
	
			Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

			FreqPtr->Cmd1Len = sprintf(Poll, "PC%03d;PC;", Power);

			C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);
			return TRUE;

		case FLRIG:

			len = sprintf(cmd, "<i4>%d</i4>", Power);

			FLRIGSendCommand(PORT, "rig.set_power", cmd);

			sprintf(Command, "Ok\r");
			return FALSE;
		}

		sprintf(Command, "Sorry - POWER not supported on your Radio\r");
		return FALSE;
	}

	if (_stricmp(FreqString, "CMD") == 0)
	{
		// Send arbitrary command to radio

		char c;
		int val;
 		char * ptr1;
		int Len;

		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be HEX Hexstring\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == NULL)
			return FALSE;

		ptr1 = strstr(Command, "CMD");

		if (ptr1 == NULL)
			return FALSE;

		ptr1 += 4;


		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		CmdPtr = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;
		FreqPtr->Cmd2 = NULL;
		FreqPtr->Cmd3 = NULL;

	
		switch (PORT->PortType)
		{ 
		case ICOM:

			// String is in Hex

			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;

			while (c = *(ptr1++))
			{
				if (c == ' ') continue;		// Allow space between pairs
			
				val = c - 0x30;
				if (val > 15) val -= 7;
				val <<= 4;
				c = *(ptr1++) - 0x30;
				if (c > 15) c -= 7;
				val |= c;
				*(CmdPtr++) = val;
			}

			*(CmdPtr++) = 0xFD;

 		
			*(CmdPtr) = 0; 

			Len = (int)(CmdPtr - (char *)&buffptr[30]);
			break;

		case KENWOOD:
		case FT991A:
		case FT2000:
		case FTDX10:
		case FLEX:
		case NMEA:

			// use text command

			Len = sprintf(CmdPtr, ptr1);
			break;

		case YAESU:

			// String is in Hex (5 values)

			while (c = *(ptr1++))
			{
				if (c == ' ') continue;		// Allow space between pairs

				val = c - 0x30;
				if (val > 15) val -= 7;
				val <<= 4;
				c = *(ptr1++) - 0x30;
				if (c > 15) c -= 7;
				val |= c;
				*(CmdPtr++) = val;
			}

			*(CmdPtr) = 0; 

			Len = 5;
			break;

		default:
			sprintf(Command, "Sorry - CMD not supported on your Radio\r");
			return FALSE;
		}

		FreqPtr[0].Cmd1Len = Len;		// for ICOM
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);
		return TRUE;
	}

	if (_memicmp(FreqString, "Chan", 4) == 0)
	{
		if (strchr(FreqString, '/')	)	// Bank/Chan
		{
			MemoryBank = FreqString[4];
			MemoryNumber = atoi(&FreqString[6]);
		}
		else
			MemoryNumber = atoi(&FreqString[4]);	// Just Chan

		Freq = 0.0;
	}
	else
	{
		Freq = atof(FreqString);

		if (Freq < 0.1 && PORT->PortType != FLRIG)
		{
			strcpy(Command, "Sorry - Invalid Frequency\r");
			return FALSE;
		}
	}

	Freq = Freq * 1000000.0;

	sprintf(FreqString, "%09.0f", Freq);

	if (PORT->PortType != ICOM)
		strcpy(Data, FilterString);			// Others don't have a filter.

	Split = DataFlag = Bandwidth = Antenna = 0;

	_strupr(Data);

	if (strchr(Data, '+'))
		Split = '+';
	else if (strchr(Data, '-'))				
		Split = '-';
	else if (strchr(Data, 'S'))
		Split = 'S';	
	else if (strchr(Data, 'D'))	
		DataFlag = 1;
								
	if (strchr(Data, 'W'))
		Bandwidth = 'W';	
	else if (strchr(Data, 'N'))
		Bandwidth = 'N';

	if (strstr(Data, "A1"))
		Antenna = '1';
	else if (strstr(Data, "A2"))
		Antenna = '2';
	else if (strstr(Data, "A3"))
		Antenna = '3';
	else if (strstr(Data, "A4"))
		Antenna = '4';
	else if (strstr(Data, "A5"))
		Antenna = '5';
	else if (strstr(Data, "A6"))
		Antenna = '6';

	switch (PORT->PortType)
	{ 
	case ICOM:

		if (n == 2)
			// Set Freq Only

			ModeNo = -1;
		else
		{
			if (n < 4 && RIG->ICF8101 == 0)
			{
				strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode Filter Width\r");
				return FALSE;
			}

			Filter = atoi(FilterString);

			for (ModeNo = 0; ModeNo < 24; ModeNo++)
			{
				if (_stricmp(Modes[ModeNo], Mode) == 0)
					break;
			}

			if (ModeNo == 24)
			{
				sprintf(Command, "Sorry - Invalid Mode\r");
				return FALSE;
			}
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;
		FreqPtr->Dwell = 51;

		CmdPtr = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;
		FreqPtr->Cmd2 = NULL;
		FreqPtr->Cmd3 = NULL;

		*(CmdPtr++) = 0xFE;
		*(CmdPtr++) = 0xFE;
		*(CmdPtr++) = RIG->RigAddr;
		*(CmdPtr++) = 0xE0;


		if (MemoryNumber)
		{
			// Set Memory Channel instead of Freq, Mode, etc

			char ChanString[5];

			// Send Set Memory, then Channel
								
			*(CmdPtr++) = 0x08;
			*(CmdPtr++) = 0xFD;

			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;

			sprintf(ChanString, "%04d", MemoryNumber); 
	
			*(CmdPtr++) = 0x08;
			*(CmdPtr++) = (ChanString[1] - 48) | ((ChanString[0] - 48) << 4);
			*(CmdPtr++) = (ChanString[3] - 48) | ((ChanString[2] - 48) << 4);
			*(CmdPtr++) = 0xFD;
				
			FreqPtr[0].Cmd1Len = 14;

			if (MemoryBank)
			{						
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = RIG->RigAddr;
				*(CmdPtr++) = 0xE0;
				*(CmdPtr++) = 0x08;
				*(CmdPtr++) = 0xA0;
				*(CmdPtr++) = MemoryBank - 0x40;
				*(CmdPtr++) = 0xFD;

				FreqPtr[0].Cmd1Len += 8;
			}	
		}
		else
		{
			if (RIG->ICF8101)
			{
				// Set Freq is 1A 35 and set Mode 1A 36

				*(CmdPtr++) = 0x1A;
				*(CmdPtr++) = 0x35;		// Set frequency command

				// Need to convert two chars to bcd digit

				*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
				*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
				*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
				*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
				*(CmdPtr++) = (FreqString[0] - 48);
				*(CmdPtr++) = 0xFD;
				FreqPtr[0].Cmd1Len = 12;

				if (ModeNo != -1)			// Dont Set
				{		
					CmdPtr = FreqPtr->Cmd2 = FreqPtr->Cmd2Msg;
					FreqPtr[0].Cmd2Len = 9;		

					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = RIG->RigAddr;
					*(CmdPtr++) = 0xE0;
					*(CmdPtr++) = 0x1A;
					*(CmdPtr++) = 0x36;		// Set mode command
					*(CmdPtr++) = 0;
					if (ModeNo > 10)
						*(CmdPtr++) = ModeNo + 6;
					else
						*(CmdPtr++) = ModeNo;
					*(CmdPtr++) = 0xFD;
				}
			}
			else
			{

				*(CmdPtr++) = 0x5;		// Set frequency command

				// Need to convert two chars to bcd digit

				*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
				*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
				*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
				*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
				if (RIG->IC735)
				{
					*(CmdPtr++) = 0xFD;
					FreqPtr[0].Cmd1Len = 10;
				}
				else
				{
					*(CmdPtr++) = (FreqString[0] - 48);
					*(CmdPtr++) = 0xFD;
					FreqPtr[0].Cmd1Len = 11;
				}

				// Send Set VFO in case last chan was memory

				//		*(CmdPtr++) = 0xFE;
				//		*(CmdPtr++) = 0xFE;
				//		*(CmdPtr++) = RIG->RigAddr;
				//		*(CmdPtr++) = 0xE0;

				//		*(CmdPtr++) = 0x07;
				//		*(CmdPtr++) = 0xFD;

				//		FreqPtr[0].Cmd1Len = 17;

				if (ModeNo != -1)			// Dont Set
				{		
					CmdPtr = FreqPtr->Cmd2 = FreqPtr->Cmd2Msg;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = RIG->RigAddr;
					*(CmdPtr++) = 0xE0;
					*(CmdPtr++) = 0x6;		// Set Mode
					*(CmdPtr++) = ModeNo;
					*(CmdPtr++) = Filter;
					*(CmdPtr++) = 0xFD;

					FreqPtr->Cmd2Len = 8;

					if (Split)
					{
						CmdPtr = FreqPtr->Cmd3 = FreqPtr->Cmd3Msg;
						FreqPtr->Cmd3Len = 7;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0xF;		// Set Mode
						if (Split == 'S')
							*(CmdPtr++) = 0x10;
						else
							if (Split == '+')
								*(CmdPtr++) = 0x12;
							else
								if (Split == '-')
									*(CmdPtr++) = 0x11;

						*(CmdPtr++) = 0xFD;
					}
					else if (DataFlag)
					{
						CmdPtr = FreqPtr->Cmd3 = FreqPtr->Cmd3Msg;

						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x1a;	

						if ((strcmp(RIG->RigName, "IC7100") == 0) ||
							(strcmp(RIG->RigName, "IC7410") == 0) ||
							(strcmp(RIG->RigName, "IC7600") == 0) ||
							(strcmp(RIG->RigName, "IC7610") == 0) ||
							(strcmp(RIG->RigName, "IC7300") == 0))
						{
							FreqPtr[0].Cmd3Len = 9;
							*(CmdPtr++) = 0x6;		// Send/read DATA mode with filter set
							*(CmdPtr++) = 0x1;		// Data On
							*(CmdPtr++) = Filter;	//Filter
						}
						else if (strcmp(RIG->RigName, "IC7200") == 0)
						{
							FreqPtr[0].Cmd3Len = 9;
							*(CmdPtr++) = 0x4;		// Send/read DATA mode with filter set
							*(CmdPtr++) = 0x1;		// Data On
							*(CmdPtr++) = Filter;	// Filter
						}
						else
						{
							FreqPtr[0].Cmd3Len = 8;
							*(CmdPtr++) = 0x6;		// Set Data
							*(CmdPtr++) = 0x1;		//On		
						}

						*(CmdPtr++) = 0xFD;
					}
				}

				if (Antenna == '5' || Antenna == '6')
				{
					// Antenna select for 746 and maybe others

					// Could be going in cmd2 3 or 4

					if (FreqPtr[0].Cmd2 == NULL)
					{
						CmdPtr = FreqPtr->Cmd2 = FreqPtr->Cmd2Msg;
						FreqPtr[0].Cmd2Len = 7;
					}
					else if (FreqPtr[0].Cmd3 == NULL)
					{
						CmdPtr = FreqPtr->Cmd3 = FreqPtr->Cmd3Msg;
						FreqPtr[0].Cmd3Len = 7;
					}
					else 
					{
						CmdPtr = FreqPtr->Cmd4 = FreqPtr->Cmd4Msg;
						FreqPtr[0].Cmd4Len = 7;
					}

					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = RIG->RigAddr;
					*(CmdPtr++) = 0xE0;
					*(CmdPtr++) = 0x12;		// Set Antenna
					*(CmdPtr++) = Antenna - '5';	// 0 for A5 1 for A6
					*(CmdPtr++) = 0xFD;
				}
			}
		}
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case YAESU:
			
		if (n == 2)			// Just set freq
		{
			ModeNo = -1;
		}
		else
		{
			for (ModeNo = 0; ModeNo < 15; ModeNo++)
			{
				if (_stricmp(YaesuModes[ModeNo], Mode) == 0)
					break;
			}

			if (ModeNo == 15)
			{
				sprintf(Command, "Sorry -Invalid Mode\r");
				return FALSE;
			}
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

		// Send Mode then Freq - setting Mode seems to change frequency

		FreqPtr->Cmd1Len = 0;

		if (ModeNo != -1)		// Set freq only
		{
			*(Poll++) = ModeNo;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 7;		// Set Mode
			FreqPtr->Cmd1Len = 5;
		}

		*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
		*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
		*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
		*(Poll++) = 1;		// Set Freq
					
		FreqPtr->Cmd1Len += 5;

		if (strcmp(PORT->Rigs[0].RigName, "FT847") == 0)
		{
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 3;		// Status Poll
	
			FreqPtr->Cmd1Len = 15;
		}
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		saveNewFreq(RIG, Freq, Mode);

		return TRUE;


	case FT100:
	case FT990:
	case FT1000:

		if (n == 2)			// Set Freq Only
			ModeNo = -1;
		else
		{
			if (n < 3)
			{
				strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
				return FALSE;
			}
		
			if (PORT->PortType == FT100)
			{
				for (ModeNo = 0; ModeNo < 8; ModeNo++)	
				{
					if (_stricmp(FT100Modes[ModeNo], Mode) == 0)
						break;
				}

				if (ModeNo == 8)
				{
					sprintf(Command, "Sorry -Invalid Mode\r");
					return FALSE;
				}
			}
			else if (PORT->PortType == FT990)
			{
				for (ModeNo = 0; ModeNo < 12; ModeNo++)	
				{
					if (_stricmp(FT990Modes[ModeNo], Mode) == 0)
						break;
				}

				if (ModeNo == 12)
				{
					sprintf(Command, "Sorry -Invalid Mode\r");
					return FALSE;
				}
			}
			else
			{
				for (ModeNo = 0; ModeNo < 12; ModeNo++)	
				{
					if (_stricmp(FT1000Modes[ModeNo], Mode) == 0)
						break;
				}

				if (ModeNo == 12)
				{
					sprintf(Command, "Sorry -Invalid Mode\r");
					return FALSE;
				}
			}
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

		// Send Mode then Freq - setting Mode seems to change frequency

		if (ModeNo == -1)		// Don't set Mode
		{
			// Changing the length messes up a lot of code,
			// so set freq twice instead of omitting entry

			*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
			*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
			*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
			*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
			*(Poll++) = 10;		// Set Freq
		}
		else
		{
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = ModeNo;
			*(Poll++) = 12;		// Set Mode
		}

		*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
		*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
		*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
		*(Poll++) = 10;		// Set Freq

		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		if (PORT->PortType == FT990 || PORT->YaesuVariant == FT1000D)
			*(Poll++) = 3;
		else
			*(Poll++) = 2;		// 100 or 1000MP

		*(Poll++) = 16;		// Status Poll
	
		FreqPtr->Cmd1Len = 15;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case KENWOOD:
	case FT2000:
	case FTDX10:
	case FT991A:
	case FLEX:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 16; ModeNo++)
		{
			if (PORT->PortType == FT2000)
				if (_stricmp(FT2000Modes[ModeNo], Mode) == 0)
				break;

			if (PORT->PortType == FTDX10)
				if (_stricmp(FTDX10Modes[ModeNo], Mode) == 0)
				break;

			if (PORT->PortType == FT991A)
				if (_stricmp(FT991AModes[ModeNo], Mode) == 0)
				break;

			if (PORT->PortType == KENWOOD)
				if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
				break;
			if (PORT->PortType == FLEX)
				if (_stricmp(FLEXModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo > 15)
		{
			sprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

		if (PORT->PortType == FT2000)
			FreqPtr->Cmd1Len = sprintf(Poll, "FA%s;MD0%X;FA;MD;", &FreqString[1], ModeNo);
		else
		if (PORT->PortType == FT991A || PORT->PortType == FTDX10)
			FreqPtr->Cmd1Len = sprintf(Poll, "FA%s;MD0%X;FA;MD;", FreqString, ModeNo);
		else
		if (PORT->PortType == FLEX)
			FreqPtr->Cmd1Len = sprintf(Poll, "ZZFA00%s;ZZMD%02d;ZZFA;ZZMD;", &FreqString[1], ModeNo);
		else
		{
			if (Antenna == '5' || Antenna == '6')
				FreqPtr->Cmd1Len = sprintf(Poll, "FA00%s;MD%d;AN%c;FA;MD;", FreqString, ModeNo, Antenna - 4);
			else
				FreqPtr->Cmd1Len = sprintf(Poll, "FA00%s;MD%d;FA;MD;", FreqString, ModeNo);
		}
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;


	case SDRRADIO:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 16; ModeNo++)
		{
			if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo > 15)
		{
			sprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

		FreqPtr->Cmd1Len = sprintf(Poll, "F%c00%s;MD%d;F%c;MD;", RIG->RigAddr, FreqString, ModeNo, RIG->RigAddr);

		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;


	case NMEA:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}
		buffptr = GetBuff();

		if (buffptr == 0)
		{
			sprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)buffptr->Data;
		memset(FreqPtr, 0, sizeof(struct ScanEntry));

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = FreqPtr->Cmd1 = FreqPtr->Cmd1Msg;

		i = sprintf(Poll, "$PICOA,90,%02x,RXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
		AddNMEAChecksum(Poll);
		Len = i;
		i = sprintf(Poll + Len, "$PICOA,90,%02x,TXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
		AddNMEAChecksum(Poll + Len);
		Len += i;
		i = sprintf(Poll + Len, "$PICOA,90,%02x,MODE,%s*xx\r\n", RIG->RigAddr, Mode);
		AddNMEAChecksum(Poll + Len);
			
		FreqPtr->Cmd1Len = i + Len;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case HAMLIB:
	{
		char cmd[80];

		int len = sprintf(cmd, "F %s\n+f\nM %s %d\n+m\n",
			FreqString, Mode, atoi(Data));
	
		send(PORT->remoteSock, cmd, len, 0);
		sprintf(Command, "Ok\r");
		
		saveNewFreq(RIG, Freq, Mode);

		return FALSE;
	}

	case FLRIG:
	{
		char cmd[80];

		int len = sprintf(cmd, "<double>%.0f</double>", Freq);

		strcpy(PORT->ScanEntry.Cmd2Msg, Mode);
		strcpy(PORT->ScanEntry.Cmd3Msg, FilterString);

		if (Freq > 0.0)
			FLRIGSendCommand(PORT, "rig.set_vfo", cmd);

		else if (PORT->ScanEntry.Cmd2Msg[0] && Mode[0] != '*')
		{
			sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd2Msg);
			FLRIGSendCommand(PORT, "rig.set_mode", cmd);
		}

		else if (PORT->ScanEntry.Cmd3Msg[0] && strcmp(PORT->ScanEntry.Cmd3Msg, "0") != 0)
		{
			sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd3Msg);
			FLRIGSendCommand(PORT, "rig.set_bandwidth", cmd);
		}
		else
		{
			sprintf(Command, "Sorry - Nothing to do\r");
			return FALSE;
		}
				
		PORT->AutoPoll = 0;

		saveNewFreq(RIG, Freq, Mode);
		return TRUE;
	}


	case RTLUDP:
	{
		char cmd[80];
		int len = 5;
		int FreqInt = (int)Freq;
		int FM = 0;
		int AM = 1;
		int USB = 2;
		int LSB = 3;
		
		cmd[0] = 0;	
		cmd[1] = FreqInt & 0xff;
		cmd[2] = (FreqInt >> 8) & 0xff;
		cmd[3] = (FreqInt >> 16) & 0xff;
		cmd[4] = (FreqInt >> 24) & 0xff;

		len = sendto(PORT->remoteSock, cmd, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));

		if (Mode[0])
		{
			if (strcmp(Mode, "FM") == 0)
				memcpy(&cmd[1], &FM, 4); 
			else if (strcmp(Mode, "AM") == 0)
				memcpy(&cmd[1], &AM, 4); 
			else if (strcmp(Mode, "USB") == 0)
				memcpy(&cmd[1], &USB, 4); 
			else if (strcmp(Mode, "LSB") == 0)
				memcpy(&cmd[1], &LSB, 4); 

			cmd[0] = 1;
			len = sendto(PORT->remoteSock, cmd, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));
		}

		saveNewFreq(RIG, Freq, Mode);

		sprintf(Command, "Ok\r");
		return FALSE;
	}
// --- G7TAJ ----
	case SDRANGEL:
	{
		char cmd[80];
		int len = sprintf(cmd, "%.0f", Freq);

		strcpy(PORT->ScanEntry.Cmd2Msg, Mode);
		strcpy(PORT->ScanEntry.Cmd3Msg, FilterString);
		
		if (Freq > 0.0)
		{
			SDRANGELSendCommand(PORT, "FREQSET", cmd);
			sprintf(Command, "Ok\r");
			return FALSE;
		}
//TODO
/*                else if (PORT->ScanEntry.Cmd2Msg[0] && Mode[0] != '*')
                {
                        sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd2Msg);
                        FLRIGSendCommand(PORT, "rig.set_mode", cmd);
                }

                else if (PORT->ScanEntry.Cmd3Msg[0] && strcmp(PORT->ScanEntry.Cmd3Msg, "0") != 0)
                {
                        sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd3Msg);
                        FLRIGSendCommand(PORT, "rig.set_bandwidth", cmd);
                }
*/
                else
                {
                        sprintf(Command, "Sorry - Nothing to do\r");
                        return FALSE;
                }

                PORT->AutoPoll = 0;
	}
// --- G7TAJ ----




	}
	return TRUE;
}

int BittoInt(UINT BitMask)
{
	// Returns bit position of first 1 bit in BitMask
	
	int i = 0;
	while ((BitMask & 1) == 0)
	{	
		BitMask >>= 1;
		i ++;
	}
	return i;
}

extern char * RadioConfigMsg[36];

struct TNCINFO RIGTNC;			// Dummy TNC Record for Rigcontrol without a corresponding TNC 

DllExport BOOL APIENTRY Rig_Init()
{
	struct RIGPORTINFO * PORT;
	int i, p, port;
	struct RIGINFO * RIG;
	struct TNCINFO * TNC = &RIGTNC;
	HWND hDlg;
#ifndef LINBPQ
	int RigRow = 0;
#endif
	int NeedRig = 0;
	int Interlock = 0;

	memset(&RIGTNC, 0, sizeof(struct TNCINFO));

	TNCInfo[40] = TNC;

	// Get config info

	NumberofPorts = 0;

	for (port = 0; port < MAXBPQPORTS; port++)
		PORTInfo[port] = NULL;

	// See if any rigcontrol defined (either RADIO or RIGCONTROL lines)

	for (port = 0; port < MAXBPQPORTS; port++)
	{
		if (RadioConfigMsg[port])
			NeedRig++;
	}

	if (NeedRig == 0)
	{
		SetupPortRIGPointers();			// Sets up Dummy Rig Pointer
		return FALSE;
	}


#ifndef LINBPQ

	TNC->Port = 40;
	CreatePactorWindow(TNC, "RIGCONTROL", "RigControl", 10, PacWndProc, 550, NeedRig * 20 + 60, NULL);
	hDlg = TNC->hDlg;

#endif

	TNC->ClientHeight = NeedRig * 20 + 60;
	TNC->ClientWidth = 550;

	for (port = 0; port < MAXBPQPORTS; port++)
	{
		if (RadioConfigMsg[port])
		{
			char msg[1000];

			char * SaveRigConfig = _strdup(RadioConfigMsg[port]);
			char * RigConfigMsg1 = _strdup(RadioConfigMsg[port]);

			Interlock = atoi(&RigConfigMsg1[5]);

			RIG = RigConfig(TNC, RigConfigMsg1, port);

			if (RIG == NULL)
			{
				// Report Error

				sprintf(msg,"Port %d Invalid Rig Config %s", port, SaveRigConfig);
				WritetoConsole(msg);
				free(SaveRigConfig);
				free(RigConfigMsg1);
				continue;
			}

			RIG->Interlock = Interlock;

			if (RIG->PORT->IOBASE[0] == 0)
			{
				// We have to find the SCS TNC in this scan group as TNC is now a dummy

				struct TNCINFO * PTCTNC;
				int n;

				for (n = 1; n < MAXBPQPORTS; n++)
				{
					PTCTNC = TNCInfo[n];

					if (PTCTNC == NULL || (PTCTNC->Hardware != H_SCS && PTCTNC->Hardware != H_ARDOP))
						continue;

					if (PTCTNC->RXRadio != Interlock)
						continue;

					RIG->PORT->PTC = PTCTNC;
				}
			}

#ifndef LINBPQ

			// Create line for this rig

			RIG->hLabel = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
				10, RigRow, 80,20, hDlg, NULL, hInstance, NULL);

			//			RIG->hCAT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
			//                 90, RigRow, 40,20, hDlg, NULL, hInstance, NULL);

			RIG->hFREQ = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
				95, RigRow, 100,20, hDlg, NULL, hInstance, NULL);

			RIG->hMODE = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
				200, RigRow, 100,20, hDlg, NULL, hInstance, NULL);

			RIG->hSCAN = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
				300, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

			RIG->hPTT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
				320, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

			RIG->hPORTS = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
				340, RigRow, 210, 20, hDlg, NULL, hInstance, NULL);

			RigRow += 20;

			//if (PORT->PortType == ICOM)
			//{
			//	sprintf(msg,"%02X", PORT->Rigs[i].RigAddr);
			//	SetWindowText(RIG->hCAT, msg);
			//}
			SetWindowText(RIG->hLabel, RIG->RigName);
#endif

			RIG->WEB_Label = _strdup(RIG->RigName);
			//		RIG->WEB_CAT;
			RIG->WEB_FREQ = zalloc(80);
			RIG->WEB_MODE = zalloc(80);
			RIG->WEB_PORTS = zalloc(80);
			strcpy(RIG->WEB_FREQ, "-----------");
			strcpy(RIG->WEB_MODE, "------");

			RIG->WEB_PTT = ' ';
			RIG->WEB_SCAN = ' ';
		}
	}

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		if (PORT->PortType == HAMLIB)
		{
			HAMLIBMasterRunning = 1;
			ConnecttoHAMLIB(PORT);
		}
		else if (PORT->PortType == FLRIG)
		{
			FLRIGRunning = 1;
			ConnecttoFLRIG(PORT);
		}
		else if (PORT->PortType == RTLUDP)
			ConnecttoRTLUDP(PORT);
//---- G7TAJ ----
		else if (PORT->PortType == SDRANGEL)
		{
			SDRANGELRunning = 1;
			ConnecttoSDRANGEL(PORT);
		}
//---- G7TAJ ----
		else if (PORT->HIDDevice)		// This is RAWHID, Not CM108
			OpenHIDPort(PORT, PORT->IOBASE, PORT->SPEED);
		else if (PORT->PTC == 0 && _stricmp(PORT->IOBASE, "CM108") != 0)
			OpenRigCOMMPort(PORT, PORT->IOBASE, PORT->SPEED);

		if (PORT->PTTIOBASE[0])		// Using separate port for PTT?
		{
			if (PORT->PTTIOBASE[3] == '=')
				PORT->hPTTDevice = OpenCOMPort(&PORT->PTTIOBASE[4], PORT->SPEED, FALSE, FALSE, FALSE, 0);
			else
				PORT->hPTTDevice = OpenCOMPort(&PORT->PTTIOBASE[3], PORT->SPEED, FALSE, FALSE, FALSE, 0);
		}
		else
			PORT->hPTTDevice = PORT->hDevice;	// Use same port for PTT
	}

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i < PORT->ConfiguredRigs; i++)
		{
			int j;
			int k = 0;
			uint64_t BitMask;
			struct _EXTPORTDATA * PortEntry;

			RIG = &PORT->Rigs[i];

			SetupScanInterLockGroups(RIG);

			// Get record for each port in Port Bitmap

			// The Scan "Request Permission to Change" code needs the Port Records in order - 
			// Those with active connect lock (eg SCS) first, then those with just a connect pending lock (eg WINMOR)
			// then those with neither

			BitMask = RIG->BPQPort;
			for (j = 0; j < MAXBPQPORTS; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == CONLOCK)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			BitMask = RIG->BPQPort;
			for (j = 0; j < MAXBPQPORTS; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == SIMPLE)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			BitMask = RIG->BPQPort;
			for (j = 0; j < MAXBPQPORTS; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == NONE)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			RIG->PORT = PORT;		// For PTT

			if (RIG->NumberofBands)
				CheckTimeBands(RIG);		// Set initial timeband

#ifdef WIN32
			if (RIG->PTTCATPort[0])			// Serial port RTS to CAT 
				_beginthread(PTTCATThread,0,RIG);
#endif
			if (RIG->HAMLIBPORT)
			{
				// Open listening socket

				struct sockaddr_in local_sin;  /* Local socket - internet style */
				struct sockaddr_in * psin;
				SOCKET sock = 0;
				u_long param=1;

				char szBuff[80];

				psin=&local_sin;
				psin->sin_family = AF_INET;
				psin->sin_addr.s_addr = INADDR_ANY;

				sock = socket(AF_INET, SOCK_STREAM, 0);

				setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&param,4);

				psin->sin_port = htons(RIG->HAMLIBPORT);        // Convert to network ordering 

				if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
				{
					sprintf(szBuff, "bind(sock) failed port %d Error %d\n", port, WSAGetLastError());
					WritetoConsoleLocal(szBuff);
					closesocket(sock);
				}
				else
				{
					if (listen(sock, 5) < 0)
					{
						sprintf(szBuff, "listen(sock) failed port %d Error %d\n", port, WSAGetLastError());
						WritetoConsoleLocal(szBuff);
					}
					else
					{
						ioctl(sock, FIONBIO, &param);
						RIG->ListenSocket = sock;
						_beginthread(HAMLIBSlaveThread, 0, RIG);
					}
				}
			}
			Rig_PTTEx(RIG, 0, NULL);				// Send initial PTT Off (Mainly to set input mux on soundcard rigs)
		}
	}

	//	MoveWindow(hDlg, Rect.left, Rect.top, Rect.right - Rect.left, Row + 100, TRUE);

	SetupPortRIGPointers();

	RigWebPage = zalloc(10);

	WritetoConsole("\nRig Control Enabled\n");

	return TRUE;
}

DllExport BOOL APIENTRY Rig_Close()
{
	struct RIGPORTINFO * PORT;
	struct TNCINFO * TNC;
	int n, p;

	HAMLIBMasterRunning = 0;			// Close HAMLIB thread(s)
	HAMLIBSlaveRunning = 0;				// Close HAMLIB thread(s)
	FLRIGRunning = 0;				// Close FLRIG thread(s)
// ---- G7TAJ ----
	SDRANGELRunning = 0;				// Close SDRANGEL thread(s)
// ---- G7TAJ ----

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		if (PORT->PortType == NMEA)
		{
			// Send Remote OFF

			int i;
			char REMOFF[80];

			i = sprintf(REMOFF, "$PICOA,90,%02x,REMOTE,OFF*xx\r\n", PORT->Rigs[0].RigAddr);
			AddNMEAChecksum(REMOFF);

			WriteCOMBlock(PORT->hDevice, REMOFF, i);
			Sleep(200);
		}

		if (PORT->PortType != HAMLIB && PORT->PortType != RTLUDP && PORT->PortType != FLRIG && strcmp(PORT->IOBASE, "RAWHID") != 0)
		{
			if (PORT->hPTTDevice != PORT->hDevice)
				CloseCOMPort(PORT->hPTTDevice);

			CloseCOMPort(PORT->hDevice);
		}

		PORT->hDevice = 0;
		PORT->hPTTDevice = 0;

		// Free the RIG and Port Records

		for (n = 0; n < PORT->ConfiguredRigs; n++)
		{
			struct RIGINFO * RIG = &PORT->Rigs[n];
			struct HAMLIBSOCK * Entry = RIG->Sockets;
			struct HAMLIBSOCK * Save;

			// if RIG has any HAMLIB slave connections, close them

			Entry = RIG->Sockets;

			while (Entry)
			{
				closesocket(Entry->Sock);
				Save = Entry;
				Entry = Entry->Next;
				free(Save);
			}

			RIG->Sockets = 0;
		
			if (RIG->TimeBands)
				free (RIG->TimeBands[1]->Scanlist);

			if (RIG->PTTCATPort[0])
			{
				Rig_PTTEx(RIG, FALSE, NULL);				// Make sure PTT is down
				EndPTTCATThread = TRUE;
			}
		}

		free (PORT);
		PORTInfo[p] = NULL;
	}

	NumberofPorts = 0;		// For possible restart

	// And free the TNC config info

	for (p = 1; p < MAXBPQPORTS; p++)
	{
		TNC = TNCInfo[p];

		if (TNC == NULL)
			continue;

		TNC->RIG = NULL;

		//		memset(TNC->WL2KInfoList, 0, sizeof(TNC->WL2KInfoList));

	}

	return TRUE;
}



BOOL Rig_Poll()
{
	int p, i, Len;
	char RigWeb[16384];

	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		if (PORT->PortType == DUMMY)
		{
			DummyPoll(PORT);
			return TRUE;
		}

		if (PORT->hDevice == 0)
		{
			// Try to reopen every 15 secs 

			PORT->ReopenDelay++;

			if (PORT->ReopenDelay > 150)
			{
				PORT->ReopenDelay = 0;

				if (PORT->PortType == HAMLIB)
					ConnecttoHAMLIB(PORT);
				else if (PORT->PortType == FLRIG)
					ConnecttoFLRIG(PORT);
				else if (PORT->PortType == RTLUDP)
					ConnecttoRTLUDP(PORT);
// ---- G7TAJ ----
				else if (PORT->PortType == SDRANGEL)
					ConnecttoSDRANGEL(PORT);
// ---- G7TAJ ----
				else if (PORT->HIDDevice)
					OpenHIDPort(PORT, PORT->IOBASE, PORT->SPEED);
				else if (PORT->PTC == 0
					&& _stricmp(PORT->IOBASE, "REMOTE") != 0
					&& _stricmp(PORT->IOBASE, "CM108") != 0)
				{
					if (PORT->Closed == 0)
					{
						OpenRigCOMMPort(PORT, PORT->IOBASE, PORT->SPEED);
						if (PORT->IOBASE && PORT->PTTIOBASE[0] == 0)		// Using separate port for PTT?
							PORT->hPTTDevice = PORT->hDevice;
					}
				}
			}
		}

		if (PORT == NULL || (PORT->hDevice == 0 && PORT->PTC == 0 && PORT->remoteSock == 0))
			continue;

		// Check PTT Timers

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			// Active PTT command

			if (RIG->PTTTimer)
			{
				RIG->PTTTimer--;
				if (RIG->PTTTimer == 0)
					Rig_PTTEx(RIG, FALSE, NULL);
			}

			// repeated off timer

			if (RIG->repeatPTTOFFTimer)
			{
				RIG->repeatPTTOFFTimer--;
				if (RIG->repeatPTTOFFTimer == 0)
				{
					Rig_PTTEx(RIG, FALSE, NULL);
					RIG->repeatPTTOFFTimer = 0;			// Don't repeat repeat!
				}
			}
		}

		CheckRX(PORT);

		switch (PORT->PortType)
		{ 
		case ICOM:

			ICOMPoll(PORT);
			break;

		case YAESU:
		case FT100:
		case FT990:
		case FT1000:

			YaesuPoll(PORT);
			break;

		case KENWOOD:
		case FT2000:
		case FTDX10:
		case FT991A:
		case FLEX:
		case NMEA:

			KenwoodPoll(PORT);
			break;


		case SDRRADIO:
			SDRRadioPoll(PORT);
			break;

		case HAMLIB:
			HAMLIBPoll(PORT);
			break;

		case RTLUDP:
			RTLUDPPoll(PORT);
			break;

		case FLRIG:
			FLRIGPoll(PORT);
			break;
// ---- G7TAJ ----
		case SDRANGEL:
			SDRANGELPoll(PORT);
			break;		}
// ---- G7TAJ ----
	}

	// Build page for Web Display

	Len = BuildRigCtlPage(RigWeb);

	if (strcmp(RigWebPage, RigWeb) == 0)
		return TRUE;				// No change

	free(RigWebPage);
	
	RigWebPage = _strdup(RigWeb);

	SendRigWebPage();

	return TRUE;
}


BOOL RigCloseConnection(struct RIGPORTINFO * PORT)
{
	// disable event notification and wait for thread
	// to halt

	CloseCOMPort(PORT->hDevice); 
	PORT->hDevice = 0;
	return TRUE;

} // end of CloseConnection()

#ifndef WIN32
#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2
#endif

int OpenRigCOMMPort(struct RIGPORTINFO * PORT, VOID * Port, int Speed)
{
	if (PORT->remoteSock)		// Using WINMORCONTROL
		return TRUE;

	if (PORT->PortType == FT2000 || PORT->PortType == FT991A || PORT->PortType == FTDX10 || strcmp(PORT->Rigs[0].RigName, "FT847") == 0)		// FT2000 and similar seem to need two stop bits
		PORT->hDevice = OpenCOMPort((VOID *)Port, Speed, FALSE, FALSE, PORT->Alerted, TWOSTOPBITS);
	else if (PORT->PortType == NMEA)
		PORT->hDevice = OpenCOMPort((VOID *)Port, Speed, FALSE, FALSE, PORT->Alerted, ONESTOPBIT);
	else
		PORT->hDevice = OpenCOMPort((VOID *)Port, Speed, FALSE, FALSE, PORT->Alerted, TWOSTOPBITS);

	if (PORT->hDevice == 0)
	{
		PORT->Alerted = TRUE;
		return FALSE;
	}

	PORT->Alerted = FALSE;

	if (PORT->PortType == PTT || (PORT->Rigs[0].PTTMode & PTTRTS))
		COMClearRTS(PORT->hDevice);
	else
		COMSetRTS(PORT->hDevice);

	if (PORT->PortType == PTT || (PORT->Rigs[0].PTTMode & PTTDTR))
		COMClearDTR(PORT->hDevice);
	else
		COMSetDTR(PORT->hDevice);
	if (strcmp(PORT->Rigs[0].RigName, "FT847") == 0)
	{
		// Looks like FT847 Needa a "Cat On" Command

		UCHAR CATON[6] = {0,0,0,0,0};

		WriteCOMBlock(PORT->hDevice, CATON, 5);
	}


	if (PORT->PortType == NMEA)
	{
		// Looks like NMEA Needs Remote ON

		int i;
		char REMON[80];

		i = sprintf(REMON, "$PICOA,90,%02x,REMOTE,ON*xx\r\n", PORT->Rigs[0].RigAddr);
		AddNMEAChecksum(REMON);

		WriteCOMBlock(PORT->hDevice, REMON, i);
	}


	return TRUE;
}

void CheckRX(struct RIGPORTINFO * PORT)
{
	int Length;
	char NMEAMsg[100];
	unsigned char * ptr;
	int len;

	if (PORT->PortType == FLRIG)
	{
		// Data received in thread
		
		return;
	}

	if (PORT->PortType == HAMLIB)
	{
		// Data received in thread (do we need thread??
		
		Length = PORT->RXLen;
		PORT->RXLen = 0;
	}

	else if (PORT->PortType == RTLUDP)
	{
		CheckAndProcessRTLUDP(PORT);
		Length = 0;
		PORT->RXLen = 0;
		return;
	}

	else if (PORT->HIDDevice)
		Length = HID_Read_Block(PORT);
	else if (PORT->PTC)
		Length = GetPTCRadioCommand(PORT->PTC, &PORT->RXBuffer[PORT->RXLen]);
	else if (PORT->remoteSock)
	{
		struct sockaddr_in rxaddr;
		int addrlen = sizeof(struct sockaddr_in);

		Length = recvfrom(PORT->remoteSock, PORT->RXBuffer, 500, 0, (struct sockaddr *)&rxaddr, &addrlen);
		if (Length == -1)
			Length = 0;
	}
	else
	{
		if (PORT->hDevice == 0) 
			return;

		// only try to read number of bytes in queue 

		if (PORT->RXLen == 500)
			PORT->RXLen = 0;

		Length = 500 - (DWORD)PORT->RXLen;

		Length = ReadCOMBlock(PORT->hDevice, &PORT->RXBuffer[PORT->RXLen], Length);
	}

	if (Length == 0)
		return;					// Nothing doing
	
	PORT->RXLen += Length;

	Length = PORT->RXLen;

	switch (PORT->PortType)
	{ 
	case ICOM:
	
		if (Length < 6)				// Minimum Frame Sise
			return;

		if (PORT->RXBuffer[Length-1] != 0xfd)
			return;					// Echo

		ProcessICOMFrame(PORT, PORT->RXBuffer, Length);	// Could have multiple packets in buffer

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case YAESU:

		// Possible responses are a single byte ACK/NAK or a 5 byte info frame

		if (Length == 1 && PORT->CmdSent > 0)
		{
			ProcessYaesuCmdAck(PORT);
			return;
		}
	
		if (Length < 5)			// Frame Sise
			return;

		if (Length > 5)			// Frame Sise
		{
			PORT->RXLen = 0;	// Corruption - reset and wait for retry	
			return;
		}

		ProcessYaesuFrame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case FT100:

		// Only response should be a 16 byte info frame

		if (Length < 32)		// Frame Sise  why???????
			return;

		if (Length > 32)			// Frame Sise
		{
			PORT->RXLen = 0;	// Corruption - reset and wait for retry	
			return;
		}

		ProcessFT100Frame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case FT990:

		// Only response should be a 32 byte info frame
	
		if (Length < 32)			// Frame Sise
			return;

		if (Length > 32)			// Frame Sise
		{
			PORT->RXLen = 0;		// Corruption - reset and wait for retry	
			return;
		}

		ProcessFT990Frame(PORT);
		PORT->RXLen = 0;		// Ready for next frame	
		return;


	case FT1000:

		// Only response should be a 16 byte info frame
	
		ptr = PORT->RXBuffer;

		if (Length < 16)			// Frame Sise
			return;

		if (Length > 16)			// Frame Sise
		{
			PORT->RXLen = 0;		// Corruption - reset and wait for retry	
			return;
		}

		ProcessFT1000Frame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case KENWOOD:
	case FT2000:
	case FTDX10:
	case FT991A:
	case FLEX:

		if (Length < 2)				// Minimum Frame Sise
			return;

		if (Length > 50)			// Garbage
		{
			PORT->RXLen = 0;		// Ready for next frame	
			return;
		}

		if (PORT->RXBuffer[Length-1] != ';')
			return;	

		ProcessKenwoodFrame(PORT, Length);	

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case SDRRADIO:

		if (Length < 2)				// Minimum Frame Sise
			return;

		if (Length > 50)			// Garbage
		{
			PORT->RXLen = 0;		// Ready for next frame	
			return;
		}

		if (PORT->RXBuffer[Length-1] != ';')
			return;	

		ProcessSDRRadioFrame(PORT, Length);	

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case NMEA:

		ptr = memchr(PORT->RXBuffer, 0x0a, Length);

		while (ptr != NULL)
		{
			ptr++;									// include lf
			len = (int)(ptr - &PORT->RXBuffer[0]);	
			
			memcpy(NMEAMsg, PORT->RXBuffer, len);	

			NMEAMsg[len] = 0;

//			if (Check0183CheckSum(NMEAMsg, len))
				ProcessNMEA(PORT, NMEAMsg, len);

			Length -= len;							// bytes left

			if (Length > 0)
			{
				memmove(PORT->RXBuffer, ptr, Length);
				ptr = memchr(PORT->RXBuffer, 0x0a, Length);
			}
			else
				ptr=0;
		}

		PORT->RXLen = Length;
		return;
	
	case HAMLIB:

		ProcessHAMLIBFrame(PORT, Length);
		PORT->RXLen = 0;
		return;
	}
}

VOID ProcessICOMFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;

	//	Split into Packets. By far the most likely is a single frame
	//	so treat as special case
	
	FendPtr = memchr(rxbuffer, 0xfd, Len);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessFrame(PORT, rxbuffer, Len);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  (int)(FendPtr - rxbuffer + 1);

	ProcessFrame(PORT, rxbuffer, NewLen);
	
	// Loop Back

	ProcessICOMFrame(PORT, FendPtr+1, Len - NewLen);
	return;
}


BOOL RigWriteCommBlock(struct RIGPORTINFO * PORT)
{
	// if using a PTC radio interface send to the SCSPactor Driver, else send to COM port

	int ret;

	if (PORT->HIDDevice)
		HID_Write_Block(PORT);
	else
	if (PORT->PTC)
		SendPTCRadioCommand(PORT->PTC, PORT->TXBuffer, PORT->TXLen);
	else if (PORT->remoteSock)
		ret = sendto(PORT->remoteSock, PORT->TXBuffer, PORT->TXLen,  0, &PORT->remoteDest, sizeof(struct sockaddr));
	else
	{
		BOOL        fWriteStat;
		DWORD       BytesWritten;

#ifndef WIN32
		BytesWritten = write(PORT->hDevice, PORT->TXBuffer, PORT->TXLen);
#else
		DWORD Mask = 0;
		int Err;

		Err = GetCommModemStatus(PORT->hDevice, &Mask);

		if (Mask == 0)		// trap com0com other end not open
			return TRUE;

		fWriteStat = WriteFile(PORT->hDevice, PORT->TXBuffer, PORT->TXLen, &BytesWritten, NULL );
#endif
		if (PORT->TXLen != BytesWritten)
		{
			struct RIGINFO * RIG = &PORT->Rigs[PORT->CurrentRig];		// Only one on Yaseu

			if (RIG->RIGOK)
			{
				SetWindowText(RIG->hFREQ, "Port Closed");
				SetWindowText(RIG->hMODE, "----------");
				strcpy(RIG->WEB_FREQ, "-----------");;
				strcpy(RIG->WEB_MODE, "------");
			}

			RIG->RIGOK = FALSE;

			if (PORT->hDevice)
				 CloseCOMPort(PORT->hDevice);
		
			OpenRigCOMMPort(PORT, PORT->IOBASE, PORT->SPEED);
			
			if (PORT->IOBASE && PORT->PTTIOBASE[0] == 0)		// Using separate port for PTT?
				PORT->hPTTDevice = PORT->hDevice;
			
			if (PORT->hDevice)
			{
				// Try Again

#ifndef WIN32
				BytesWritten = write(PORT->hDevice, PORT->TXBuffer, PORT->TXLen);
#else
				fWriteStat = WriteFile(PORT->hDevice, PORT->TXBuffer, PORT->TXLen, &BytesWritten, NULL );
#endif
			}
		}
	}

	ret = GetLastError();

	PORT->Timeout = 100;		// 2 secs
	return TRUE;  
}

VOID ReleasePermission(struct RIGINFO *RIG)
{
	int i = 0;
	struct _EXTPORTDATA * PortRecord;

	while (RIG->PortRecord[i])
	{
		PortRecord = RIG->PortRecord[i];
		PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 3);	// Release Perrmission
		i++;
	}
}

int GetPermissionToChange(struct RIGPORTINFO * PORT, struct RIGINFO *RIG)
{
	struct ScanEntry ** ptr;
	struct _EXTPORTDATA * PortRecord;
	int i = 0;

	// Get Permission to change

	if (RIG->RIG_DEBUG)
		Debugprintf("GetPermissionToChange - WaitingForPermission = %d",  RIG->WaitingForPermission);

	// See if any two stage (CONLOCK) ports

	if (RIG->PortRecord[0] && RIG->PortRecord[0]->SCANCAPABILITIES != CONLOCK)
		goto CheckOtherPorts;

	
	if (RIG->WaitingForPermission)
	{
		// This code should only be called if port needs two stage
		// eg Request then confirm. only SCS Pactor at the moment.
		
		// TNC has been asked for permission, and we are waiting respoonse
		// Only SCS pactor returns WaitingForPrmission, so check shouldn't be called on others
		
		RIG->OKtoChange = (int)(intptr_t)RIG->PortRecord[0]->PORT_EXT_ADDR(6, RIG->PortRecord[0]->PORTCONTROL.PORTNUMBER, 2);	// Get Ok Flag
	
		if (RIG->OKtoChange == 1)
		{
			if (RIG->RIG_DEBUG)
				Debugprintf("Check Permission returned OK to change");

			i = 1;
			goto CheckOtherPorts;
		}

		if (RIG->OKtoChange == -1)
		{
			// Permission Refused. Wait Scan Interval and try again

			if (RIG->RIG_DEBUG)
				Debugprintf("Scan Debug %s Refused permission - waiting ScanInterval %d",
						RIG->PortRecord[0]->PORT_DLL_NAME, PORT->FreqPtr->Dwell ); 

			RIG->WaitingForPermission = FALSE;
			SetWindowText(RIG->hSCAN, "-");
			RIG->WEB_SCAN = '=';

			RIG->ScanCounter = PORT->FreqPtr->Dwell; 
			
			if (RIG->ScanCounter == 0)		// ? After manual change
				RIG->ScanCounter = 50;

			return FALSE;
		}

		if (RIG->RIG_DEBUG)
			Debugprintf("Scan Debug %s Still Waiting", RIG->PortRecord[0]->PORT_DLL_NAME); 

		
		return FALSE;			// Haven't got reply yet. Will re-enter next tick
	}
	else
	{
		// not waiting for permission, so must be first call of a cycle

		if (RIG->PortRecord[0] && RIG->PortRecord[0]->PORT_EXT_ADDR)
			RIG->WaitingForPermission = (int)(intptr_t)RIG->PortRecord[0]->PORT_EXT_ADDR(6, RIG->PortRecord[0]->PORTCONTROL.PORTNUMBER, 1);	// Request Perrmission
				
		// If it returns zero there is no need to wait.
		// Normally SCS Returns True for first call, but returns 0 if Link not running

		if (RIG->WaitingForPermission)
			return FALSE;		// Wait till driver has status

		if (RIG->RIG_DEBUG)
			Debugprintf("First call to %s returned OK to change", RIG->PortRecord[0]->PORT_DLL_NAME);

		i = 1;
	}

CheckOtherPorts:

	// Either first TNC gave permission or there are no SCS like ports.
	// Ask any others (these are assumed to give immediate yes/no

	while (RIG->PortRecord[i])
	{
		PortRecord = RIG->PortRecord[i];

		if (PortRecord->PORT_EXT_ADDR && PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 1))
		{
			// 1 means can't change - release all

			if (RIG->RIG_DEBUG && PORT->FreqPtr)
				Debugprintf("Scan Debug %s Refused permission - waiting ScanInterval %d",
						PortRecord->PORT_DLL_NAME, PORT->FreqPtr->Dwell); 

			RIG->WaitingForPermission = FALSE;
			MySetWindowText(RIG->hSCAN, "-");
			RIG->WEB_SCAN = '-';

			if (PORT->FreqPtr)
				RIG->ScanCounter = PORT->FreqPtr->Dwell;

			if (RIG->ScanCounter == 0)		// ? After manual change
				RIG->ScanCounter = 50; 

			ReleasePermission(RIG);
			return FALSE;
		}
		else
			if (RIG->RIG_DEBUG)
				Debugprintf("Scan Debug %s gave permission", PortRecord->PORT_DLL_NAME); 

		i++;
	}


	RIG->WaitingForPermission = FALSE;

	// Update pointer to next frequency

	RIG->FreqPtr++;

	ptr = RIG->FreqPtr;

	if (ptr == NULL)
	{
		Debugprintf("Scan Debug - No freqs - quitting"); 
		return FALSE;					 // No Freqs
	}

	if (ptr[0] == (struct ScanEntry *)0) // End of list - reset to start
	{
 		ptr = CheckTimeBands(RIG);
	
		if (ptr[0] == (struct ScanEntry *)0)
		{
			// Empty Timeband - delay 60 secs (we need to check for timeband change sometimes)

			RIG->ScanCounter = 600;
			ReleasePermission(RIG);
			return FALSE;
		}
	}

	PORT->FreqPtr = ptr[0];				// Save Scan Command Block

	RIG->ScanCounter = PORT->FreqPtr->Dwell; 
		
	if (RIG->ScanCounter == 0)		// ? After manual change
		RIG->ScanCounter = 150;
	
	MySetWindowText(RIG->hSCAN, "S");
	RIG->WEB_SCAN = 'S';

	// Do Bandwidth and antenna switches (if needed)

	DoBandwidthandAntenna(RIG, ptr[0]);

	return TRUE;
}

VOID DoBandwidthandAntenna(struct RIGINFO *RIG, struct ScanEntry * ptr)
{
	// If Bandwidth Change needed, do it

	int i;
	struct _EXTPORTDATA * PortRecord;

	if (ptr->Bandwidth || ptr->RPacketMode || ptr->HFPacketMode 
		|| ptr->PMaxLevel || ptr->ARDOPMode[0] || ptr->VARAMode)
	{
		i = 0;

		while (RIG->PortRecord[i])
		{
			PortRecord = RIG->PortRecord[i];

			RIG->CurrentBandWidth = ptr->Bandwidth;

			PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, ptr);

/*			if (ptr->Bandwidth == 'R')			// Robust Packet
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 6);	// Set Robust Packet
			else 
				
			if (ptr->Bandwidth == 'W')
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 4);	// Set Wide Mode
			else
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 5);	// Set Narrow Mode
*/
			i++;
		}
	}

	// If Antenna Change needed, do it
		
	// 5 and 6 are used for CI-V switching so ignore here

	if (ptr->Antenna && ptr->Antenna < '5')
	{
		SwitchAntenna(RIG, ptr->Antenna);
	}

	return;	
}

VOID ICOMPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	int i;

	struct RIGINFO * RIG;

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];

		if (RIG->ScanStopped == 0)
			if (RIG->ScanCounter)
				RIG->ScanCounter--;
	}

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		RIG = &PORT->Rigs[PORT->CurrentRig];


		SetWindowText(RIG->hFREQ, "-----------");
		strcpy(RIG->WEB_FREQ, "-----------");
		SetWindowText(RIG->hMODE, "------");
		strcpy(RIG->WEB_MODE, "------");

//		SetWindowText(RIG->hFREQ, "145.810000");
//		SetWindowText(RIG->hMODE, "RTTY/1");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	PORT->CurrentRig++;

	if (PORT->CurrentRig >= PORT->ConfiguredRigs)
		PORT->CurrentRig = 0;

	RIG = &PORT->Rigs[PORT->CurrentRig];

/*
	RIG->DebugDelay ++;

	if (RIG->DebugDelay > 600)
	{
		RIG->DebugDelay = 0;
		Debugprintf("Scan Debug %d %d %d %d %d %d", PORT->CurrentRig, 
			RIG->NumberofBands, RIG->RIGOK, RIG->ScanStopped, RIG->ScanCounter,
			RIG->WaitingForPermission);
	}
*/
	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if	(GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq/1000000.0);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
	
				if (PORT->FreqPtr->Freq > 0.0)
				{
					_gcvt((PORT->FreqPtr->Freq + RIG->rxOffset) / 1000000.0, 9, RIG->Valchar); // For MH
					strcpy(RIG->WEB_FREQ, RIG->Valchar);
					SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
				}

		
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->AutoPoll = TRUE;

				return;
			}
		}
	}

	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		struct MSGWITHOUTLEN * buffptr;
			
		buffptr = Q_REM(&RIG->BPQtoRADIO_Q);

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr->Data, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.

		if (RIG->RIG_DEBUG)
			Debugprintf("BPQ32 Manual Change Freq to %9.4f", PORT->FreqPtr->Freq/1000000.0);


		memcpy(Poll, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		if (PORT->FreqPtr->Freq > 0.0)
		{
			_gcvt((PORT->FreqPtr->Freq + RIG->rxOffset) / 1000000.0, 9, RIG->Valchar); // For MH
			strcpy(RIG->WEB_FREQ, RIG->Valchar);
			SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		}

		PORT->TXLen = PORT->FreqPtr->Cmd1Len;					// First send the set Freq
		RigWriteCommBlock(PORT);
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);

		PORT->AutoPoll = FALSE;

		return;
	}

		
	if (RIG->RIGOK && RIG->ScanStopped == 0 && RIG->NumberofBands &&
		RIG->ScanCounter && RIG->ScanCounter < 30) // no point in reading freq if we are about to change
		return;

	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter)
			return;
	}

	// Need to make sure we don't poll multiple rigs on port at the same time

	if (RIG->RIGOK)
	{
		PORT->Retries = 2;
		RIG->PollCounter = 10 / PORT->ConfiguredRigs;			// Once Per Sec
	}
	else
	{
		PORT->Retries = 1;
		RIG->PollCounter = 100 / PORT->ConfiguredRigs;			// Slow Poll if down
	}

	RIG->PollCounter += PORT->CurrentRig * 3;

	PORT->AutoPoll = TRUE;

	// Read Frequency 

	Poll[0] = 0xFE;
	Poll[1] = 0xFE;
	Poll[2] = RIG->RigAddr;
	Poll[3] = 0xE0;
	Poll[4] = 0x3;		// Get frequency command
	Poll[5] = 0xFD;

	PORT->TXLen = 6;

	RigWriteCommBlock(PORT);

	PORT->Retries = 1;
	PORT->Timeout = 10;

	return;
}


VOID ProcessFrame(struct RIGPORTINFO * PORT, UCHAR * Msg, int framelen)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG;
	int i;
	int cmdSent = PORT->TXBuffer[4];

	if (Msg[0] != 0xfe || Msg[1] != 0xfe)

		// Duff Packet - return

		return;	

	if (Msg[2] != 0xe0 && Msg[2] != 0)
	{
		// Echo - Proves a CI-V interface is attached

		if (PORT->PORTOK == FALSE)
		{
			// Just come up		
			PORT->PORTOK = TRUE;
			Debugprintf("Cat Port OK");
		}
		return;
	}

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];
		if (Msg[3] == RIG->RigAddr)
			goto ok;
	}

	return;

ok:

	if (Msg[4] == 0xFB)
	{
		// Accept

		if (cmdSent == 0x1C && PORT->TXBuffer[5] == 1)		// Tune
		{
			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Tune OK");
		
			PORT->Timeout = 0;
			return;
		}

		if (cmdSent == 0x14 && PORT->TXBuffer[5] == 0x0A)		// Power
		{
			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Set Power OK");
		
			PORT->Timeout = 0;
			return;
		}

		// if it was the set freq, send the set mode

		if (RIG->ICF8101)
		{
			if (cmdSent == 0x1A && PORT->TXBuffer[5] == 0x35)
			{
				if (PORT->FreqPtr->Cmd2)
				{
					memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd2, PORT->FreqPtr->Cmd2Len);
					PORT->TXLen = PORT->FreqPtr->Cmd2Len;
					RigWriteCommBlock(PORT);
					PORT->Retries = 2;
				}
				else
				{
					if (!PORT->AutoPoll)
						SendResponse(RIG->Session, "Frequency Set OK");

					PORT->Timeout = 0;
				}
				return;
			}

			if (cmdSent == 0x1a && PORT->TXBuffer[5] == 0x36)
			{
				// Set Mode Response - if scanning read freq, else return OK to user

				if (RIG->ScanStopped == 0)
				{
					ReleasePermission(RIG);	// Release Perrmission

					Poll[0] = 0xFE;
					Poll[1] = 0xFE;
					Poll[2] = RIG->RigAddr;
					Poll[3] = 0xE0;
					Poll[4] = 0x3;		// Get frequency command
					Poll[5] = 0xFD;

					PORT->TXLen = 6;
					RigWriteCommBlock(PORT);
					PORT->Retries = 2;
					return;
				}
				else
				{
					if (!PORT->AutoPoll)
						SendResponse(RIG->Session, "Frequency and Mode Set OK");

					RIG->PollCounter = 30 / RIG->PORT->ConfiguredRigs;	// Dont read freq for 3 secs
				}
			}

			PORT->Timeout = 0;
			return;
		}	

		if (cmdSent == 5 || cmdSent == 7) // Freq or VFO
		{
			if (PORT->FreqPtr && PORT->FreqPtr->Cmd2)
			{
				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd2, PORT->FreqPtr->Cmd2Len);
				PORT->TXLen = PORT->FreqPtr->Cmd2Len;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
			}
			else
			{
				if (!PORT->AutoPoll)
 					SendResponse(RIG->Session, "Frequency Set OK");
		
				PORT->Timeout = 0;
			}
			return;
		}

		if (cmdSent == 6)		// Set Mode
		{
			if (PORT->FreqPtr && PORT->FreqPtr->Cmd3)
			{
				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd3, PORT->FreqPtr->Cmd3Len);
				PORT->TXLen = PORT->FreqPtr->Cmd3Len;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

			goto SetFinished;
		}

		if (cmdSent == 0x08)
		{
			// Memory Chan
			
			cmdSent = 0;			// So we only do it once

			goto SetFinished;
		}

		if (cmdSent == 0x12)
			goto SetFinished;		// Antenna always sent last


		if (cmdSent == 0x0f || cmdSent == 0x01a)	// Set DUP or Data
		{
			// These are send from cmd3. There may be a cmd4
			// for antenna switching

			if (PORT->FreqPtr && PORT->FreqPtr->Cmd4)
			{
				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd4, PORT->FreqPtr->Cmd4Len);
				PORT->TXLen = PORT->FreqPtr->Cmd4Len;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

SetFinished:

			// Set Mode Response - if scanning read freq, else return OK to user

			if (RIG->ScanStopped == 0 && PORT->AutoPoll)
			{
				ReleasePermission(RIG);	// Release Perrmission

				Poll[0] = 0xFE;
				Poll[1] = 0xFE;
				Poll[2] = RIG->RigAddr;
				Poll[3] = 0xE0;
				Poll[4] = 0x3;		// Get frequency command
				Poll[5] = 0xFD;

				PORT->TXLen = 6;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

			else
			{
				if (!PORT->AutoPoll)
					SendResponse(RIG->Session, "Frequency and Mode Set OK");

				RIG->PollCounter = 30 / RIG->PORT->ConfiguredRigs;	// Dont read freq for 3 secs
			}
		}

		PORT->Timeout = 0;
		return;
	}

	if (Msg[4] == 0xFA)
	{
		// Reject

		PORT->Timeout = 0;

		if (!PORT->AutoPoll)
		{
			if (cmdSent == 5)
				SendResponse(RIG->Session, "Sorry - Set Frequency Command Rejected");
			else
			if (cmdSent == 6)
				SendResponse(RIG->Session, "Sorry - Set Mode Command Rejected");
			else
			if (cmdSent == 0x0f)
				SendResponse(RIG->Session, "Sorry - Set Shift Command Rejected");
			else
			if (cmdSent == 0x12)
				SendResponse(RIG->Session, "Sorry - Set Antenna Command Rejected");
			else
			if (cmdSent == 0x1C && PORT->TXBuffer[5] == 1)		// Tune
				SendResponse(RIG->Session, "Sorry - Tune Command Rejected");
			else
			if (cmdSent == 0x14 && PORT->TXBuffer[5] == 0x0a)		// Power
				SendResponse(RIG->Session, "Sorry - Power Command Rejected");
			else
				SendResponse(RIG->Session, "Sorry - Command Rejected");

		}
		return;
	}

	if (Msg[4] == PORT->TXBuffer[4])
	{
		// Response to our command

		// Any valid frame is an ACK

		RIG->RIGOK = TRUE;
		PORT->Timeout = 0;

		if (!PORT->AutoPoll)
		{
			// Manual response probably to CMD Query. Return response

			char reply[256] = "CMD Response ";
			char * p1 = &reply[13];
			UCHAR * p2 = &Msg[4];
			int n = framelen - 4;

			while (n > 0)
			{
				sprintf(p1, "%02X ", *(p2));
				p1 += 3;
				p2++;
				n--;
			}

			SendResponse(RIG->Session, reply);

		}

	}
	else 
		return;		// What does this mean??


	if (PORT->PORTOK == FALSE)
	{
		// Just come up
//		char Status[80];
		
		PORT->PORTOK = TRUE;
//		sprintf(Status,"COM%d PORT link OK", PORT->IOBASE);
//		SetWindowText(PORT->hStatus, Status);
	}

	if (Msg[4] == 3)
	{
		// Rig Frequency
		int n, j, decdigit;
		long long Freq = 0;
		int start = 9;
		long long MHz, Hz;
		char CharMHz[16] = "";
		char CharHz[16] = "";

		int i;

	
		if (RIG->IC735)
			start = 8;		// shorted msg

		for (j = start; j > 4; j--)
		{
			n = Msg[j];
			decdigit = (n >> 4);
			decdigit *= 10;
			decdigit += n & 0xf;
			Freq = (Freq *100 ) + decdigit;
		}

		Freq += RIG->rxOffset;

		RIG->RigFreq = Freq / 1000000.0;

		// If we convert to float to display we get rounding errors, so convert to MHz and Hz to display

		MHz = Freq / 1000000;

		Hz = Freq - MHz * 1000000;

		sprintf(CharMHz, "%lld", MHz);
		sprintf(CharHz, "%06lld", Hz);

		for (i = 5; i > 2; i--)
		{
			if (CharHz[i] == '0')
				CharHz[i] = 0;
			else
				break;
		}

		sprintf(RIG->WEB_FREQ,"%lld.%s", MHz, CharHz);
		SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		strcpy(RIG->Valchar, RIG->WEB_FREQ);
 

		// Now get Mode

		Poll[0] = 0xFE;
		Poll[1] = 0xFE;
		Poll[2] = RIG->RigAddr;
		Poll[3] = 0xE0;

		if (RIG->ICF8101)
		{
			Poll[4] = 0x1A;		// Get Mode
			Poll[5] = 0x34;
			Poll[6] = 0xFD;
			PORT->TXLen = 7;
		}
		else
		{
			Poll[4] = 0x4;		// Get Mode
			Poll[5] = 0xFD;

			PORT->TXLen = 6;
		}

		RigWriteCommBlock(PORT);
		PORT->Retries = 2;
		return;
	}

	if (RIG->ICF8101)
	{
		if (cmdSent == 0x1A && PORT->TXBuffer[5] == 0x34)
		{
			// Get Mode

			unsigned int Mode;

			Mode = (Msg[7] >> 4);
			Mode *= 10;
			Mode += Msg[7] & 0xf;

			if (Mode > 24) Mode = 24;

			sprintf(RIG->WEB_MODE,"%s", Modes[Mode]);

			strcpy(RIG->ModeString, Modes[Mode]);
			SetWindowText(RIG->hMODE, RIG->WEB_MODE);

			return;
		}
	}


	if (Msg[4] == 4)
	{
		// Mode

		unsigned int Mode;

		Mode = (Msg[5] >> 4);
		Mode *= 10;
		Mode += Msg[5] & 0xf;

		if (Mode > 24) Mode = 24;

		if (RIG->IC735)
			sprintf(RIG->WEB_MODE,"%s", Modes[Mode]);
		else
			sprintf(RIG->WEB_MODE,"%s/%d", Modes[Mode], Msg[6]);

		strcpy(RIG->ModeString, Modes[Mode]);
		SetWindowText(RIG->hMODE, RIG->WEB_MODE);
	}
}

int SendResponse(int Session, char * Msg)
{
	PDATAMESSAGE Buffer;
	BPQVECSTRUC * VEC;
	TRANSPORTENTRY * L4 = L4TABLE;

	if (Session == -1)
		return 0;

	Buffer = GetBuff();

	L4 += Session;

	Buffer->PID = 0xf0;
	Buffer->LENGTH = MSGHDDRLEN + 1; // Includes PID
	Buffer->LENGTH += sprintf(Buffer->L2DATA, "%s\r", Msg);

	VEC = L4->L4TARGET.HOST;

	C_Q_ADD(&L4->L4TX_Q, (UINT *)Buffer);

#ifndef LINBPQ

	if (VEC)
		PostMessage(VEC->HOSTHANDLE, BPQMsg, VEC->HOSTSTREAM, 2);  
#endif
	return 0;
}

VOID ProcessFT100Frame(struct RIGPORTINFO * PORT)
{
	// Only one we should see is a Status Message

	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int Freq;
	double FreqF;
	unsigned int Mode;
	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	// Bytes 0 is Band
	// 1 - 4 is Freq in binary in units of 1.25 HZ (!)
	// Byte 5 is Mode

	Freq =  (Msg[1] << 24) + (Msg[2] << 16) + (Msg[3] << 8) + Msg[4];
	
	FreqF = (Freq * 1.25) / 1000000;

	if (PORT->YaesuVariant == FT1000MP)
		FreqF = FreqF / 2;				// No idea why!

	RIG->RigFreq = FreqF;

	_gcvt(FreqF, 9, RIG->Valchar);

	sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

	if (PORT->PortType == FT100)
	{
		Mode = Msg[5] & 15;
		if (Mode > 8) Mode = 8;
		sprintf(RIG->WEB_MODE,"%s", FT100Modes[Mode]);
	}
	else	// FT1000
	{
		Mode = Msg[7] & 7;
		sprintf(RIG->WEB_MODE,"%s", FTRXModes[Mode]);
	}

	strcpy(RIG->ModeString, RIG->WEB_MODE);
	SetWindowText(RIG->hMODE, RIG->WEB_MODE);

	if (!PORT->AutoPoll)
		SendResponse(RIG->Session, "Mode and Frequency Set OK");
	else
		if (PORT->TXLen > 5)		// Poll is 5 Change is more
			ReleasePermission(RIG);		// Release Perrmission to change
}



VOID ProcessFT990Frame(struct RIGPORTINFO * PORT)
{
	// Only one we should see is a Status Message

	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int Freq;
	double FreqF;
	unsigned int Mode;
	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	// Bytes 0 is Band
	// 1 - 4 is Freq in units of 10Hz (I think!)
	// Byte 5 is Mode

	Freq =  (Msg[1] << 16) + (Msg[2] << 8) + Msg[3];
	
	FreqF = (Freq * 10.0) / 1000000;

	RIG->RigFreq = FreqF;

	_gcvt(FreqF, 9, RIG->Valchar);

	sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

	Mode = Msg[7] & 7;
	sprintf(RIG->WEB_MODE,"%s", FTRXModes[Mode]);

	strcpy(RIG->ModeString, RIG->WEB_MODE);
	SetWindowText(RIG->hMODE, RIG->WEB_MODE);

	if (!PORT->AutoPoll)
		SendResponse(RIG->Session, "Mode and Frequency Set OK");
	else
		if (PORT->TXLen > 5)		// Poll is 5 change is more
			ReleasePermission(RIG);		// Release Perrmission to change
}

VOID ProcessFT1000Frame(struct RIGPORTINFO * PORT)
{
	// Only one we should see is a Status Message

	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int Freq;
	double FreqF;
	unsigned int Mode;
	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	// I think the FT1000/1000D is same as 990
	//	FT1000MP is similar to FT100, but steps on .625 Hz (despite manual)
	// Bytes 0 is Band
	// 1 - 4 is Freq in binary in units of 1.25 HZ (!)
	// Byte 5 is Mode

	if (PORT->YaesuVariant == FT1000MP)
	{
		Freq =  (Msg[1] << 24) + (Msg[2] << 16) + (Msg[3] << 8) + Msg[4];	
		FreqF = (Freq * 1.25) / 1000000;
		FreqF = FreqF / 2;				// No idea why!
	}
	else
	{
		Freq =  (Msg[1] << 16) + (Msg[2] << 8) + Msg[3];
		FreqF = (Freq * 10.0) / 1000000;
	}

	RIG->RigFreq = FreqF;

	_gcvt(FreqF, 9, RIG->Valchar);

	sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

	if (PORT->PortType == FT100)
	{
		Mode = Msg[5] & 15;
		if (Mode > 8) Mode = 8;
		sprintf(RIG->WEB_MODE,"%s", FT100Modes[Mode]);
	}
	else	// FT1000
	{
		Mode = Msg[7] & 7;
		sprintf(RIG->WEB_MODE,"%s", FTRXModes[Mode]);
	}

	strcpy(RIG->ModeString, RIG->WEB_MODE);
	SetWindowText(RIG->hMODE, RIG->WEB_MODE);

	if (!PORT->AutoPoll)
		SendResponse(RIG->Session, "Mode and Frequency Set OK");
	else
		if (PORT->TXLen > 5)		// Poll is 5 change is more
			ReleasePermission(RIG);		// Release Perrmission to change
}





VOID ProcessYaesuCmdAck(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	PORT->Timeout = 0;
	PORT->RXLen = 0;					// Ready for next frame	

	if (PORT->CmdSent == 1)				// Set Freq
	{
		ReleasePermission(RIG);			// Release Perrmission

		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Frequency Rejected");

			return;
		}
		else
		{
			if (RIG->ScanStopped == 0)
			{
				// Send a Get Freq - We Don't Poll when scanning

				Poll[0] = Poll[1] = Poll[2] = Poll[3] = 0;
				Poll[4] = 0x3;		// Get frequency amd mode command

				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->CmdSent = 0;
			}
			else

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Mode and Frequency Set OK");

			return;
		}
	}

	if (PORT->CmdSent == 7)						// Set Mode
	{
		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Mode Rejected");

			return;
		}
		else
		{
			// Send the Frequency
			
			memcpy(Poll, &Poll[5], 5);
			RigWriteCommBlock(PORT);
			PORT->CmdSent = Poll[4];
			PORT->Retries = 2;

			return;
		}
	}

}
VOID ProcessYaesuFrame(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int n, j, Freq = 0, decdigit;
	double FreqF;
	unsigned int Mode;

	// I'm not sure we get anything but a Command Response,
	// and the only command we send is Get Rig Frequency and Mode

	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	for (j = 0; j < 4; j++)
	{
		n = Msg[j];
		decdigit = (n >> 4);
		decdigit *= 10;
		decdigit += n & 0xf;
		Freq = (Freq *100 ) + decdigit;
	}

	FreqF = Freq / 100000.0;

	RIG->RigFreq = FreqF;

//		Valchar = _fcvt(FreqF, 6, &dec, &sign);
	_gcvt(FreqF, 9, RIG->Valchar);

	sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

	Mode = Msg[4];

	if (Mode > 15) Mode = 15;

	sprintf(RIG->WEB_MODE,"%s", YaesuModes[Mode]);
	strcpy(RIG->ModeString, RIG->WEB_MODE);
	SetWindowText(RIG->hMODE, RIG->WEB_MODE);

	//	FT847 Manual Freq Change response ends up here
	
	if (strcmp(RIG->RigName, "FT847") == 0)
	{
		if (!PORT->AutoPoll)
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
			
		if (PORT->CmdSent == -1)
			ReleasePermission(RIG);			// Release Perrmission to change
	}
}

VOID YaesuPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");


		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if	(GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, 24);

				if (PORT->PortType == YAESU)
				{
					if (strcmp(PORT->Rigs[0].RigName, "FT847") == 0)
					{
						PORT->TXLen = 15; // No Cmd ACK, so send Mode, Freq and Poll
						PORT->CmdSent = -1;
					}
					else
					{
						PORT->TXLen = 5;
						PORT->CmdSent = Poll[4];
					}
					RigWriteCommBlock(PORT);
					PORT->Retries = 2;
					PORT->AutoPoll = TRUE;
					return;
				}

				// FT100

				PORT->TXLen = 15;			// Set Mode, Set Freq, Poll
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->AutoPoll = TRUE;
			}
		}
	}
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		struct MSGWITHOUTLEN * buffptr;
			
		buffptr = Q_REM(&RIG->BPQtoRADIO_Q);

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr->Data, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.
		
		if (RIG->RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

		_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		memcpy(Poll, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);

		if (PORT->PortType == YAESU)
		{
			if (strcmp(PORT->Rigs[0].RigName, "FT847") == 0)
			{
				PORT->TXLen = 15;					// Send all
				PORT->CmdSent = -1;
			}
			else
			{
				PORT->TXLen = 5;					// First send the set Freq
				PORT->CmdSent = Poll[4];
			}
		}
		else
			PORT->TXLen = 15;					// Send all

		RigWriteCommBlock(PORT);
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}

	if (RIG->RIGOK && RIG->ScanStopped == 0 && RIG->NumberofBands &&
		RIG->ScanCounter && RIG->ScanCounter < 30) // no point in reading freq if we are about to change
		return;
	
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter)
			return;
	}

	RIG->PollCounter = 10;			// Once Per Sec

	// Read Frequency 

	Poll[0] = 0;
	Poll[1] = 0;
	Poll[2] = 0;

	if (PORT->PortType == FT990 || PORT->PortType == YAESU || PORT->YaesuVariant == FT1000D)
		Poll[3] = 3;
	else
		Poll[3] = 2;
	
	if (PORT->PortType == YAESU)
		Poll[4] = 0x3;		// Get frequency amd mode command
	else
		Poll[4] = 16;		// FT100/990/1000 Get frequency amd mode command

	PORT->TXLen = 5;
	RigWriteCommBlock(PORT);
	PORT->Retries = 2;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID ProcessNMEA(struct RIGPORTINFO * PORT, char * Msg, int Length)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	Msg[Length] = 0;
	
	if (PORT->PORTOK == FALSE)
	{
		// Just come up		
		PORT->PORTOK = TRUE;
	}

	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	if (!PORT->AutoPoll)
	{
		// Response to a RADIO Command

		if (Msg[0] == '?')
			SendResponse(RIG->Session, "Sorry - Command Rejected");
		else
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
	
		PORT->AutoPoll = TRUE;
	}

	if (memcmp(&Msg[13], "RXF", 3) == 0)
	{
		if (Length < 24)
			return;

		RIG->RigFreq = atof(&Msg[17]);
	
		sprintf(RIG->WEB_FREQ,"%f", RIG->RigFreq);
		SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		strcpy(RIG->Valchar, RIG->WEB_FREQ);

		return;
	}

	if (memcmp(&Msg[13], "MODE", 3) == 0)
	{
		char * ptr;

		if (Length < 24)
			return;

		ptr = strchr(&Msg[18], '*');
		if (ptr) *(ptr) = 0;

		SetWindowText(RIG->hMODE, &Msg[18]);
		strcpy(RIG->WEB_MODE, &Msg[18]);
		strcpy(RIG->ModeString, RIG->WEB_MODE);
		return;
	}


}


//FA00014103000;MD2;


VOID ProcessSDRRadioFrame(struct RIGPORTINFO * PORT, int Length)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];
	UCHAR * ptr;
	int CmdLen;
	int i;

	Msg[Length] = 0;

	if (PORT->PORTOK == FALSE)
	{
		// Just come up		
		PORT->PORTOK = TRUE;
	}

	if (!PORT->AutoPoll)
	{
		// Response to a RADIO Command

		if (Msg[0] == '?')
			SendResponse(RIG->Session, "Sorry - Command Rejected");
		else if (Msg[0] == 'A' && Msg[1] == 'C')
			SendResponse(RIG->Session, "TUNE OK");
		else if (Msg[0] == 'P' && Msg[1] == 'C') 
			SendResponse(RIG->Session, "Power Set OK");
		else
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
	
		PORT->AutoPoll = TRUE;
		return;
	}


LoopR:

	ptr = strchr(Msg, ';');
	CmdLen = (int)(ptr - Msg + 1);

	// Find Device. FA to FF for frequency

	// Note. SDRConsole reports the same mode for all receivers, so don't rely on reported mode

	if (Msg[0] == 'F')
	{
		for (i = 0; i < PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];
			if (Msg[1] == RIG->RigAddr)
				goto ok;
		}
		return;
ok:

		if (CmdLen > 9)
		{
			char CharMHz[16] = "";
			char CharHz[16] = "";

			int i;
			long long Freq;
			long long MHz, Hz;

			RIG->RIGOK = TRUE;

			Freq = strtoll(&Msg[2], NULL, 10) + RIG->rxOffset;

			RIG->RigFreq = Freq / 1000000.0;

			// If we convert to float to display we get rounding errors, so convert to MHz and Hz to display

			MHz = Freq / 1000000;

			Hz = Freq - MHz * 1000000;

			sprintf(CharMHz, "%lld", MHz);
			sprintf(CharHz, "%06lld", Hz);

			for (i = 5; i > 2; i--)
			{
				if (CharHz[i] == '0')
					CharHz[i] = 0;
				else
					break;
			}


			sprintf(RIG->WEB_FREQ,"%lld.%s", MHz, CharHz);
			SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
			strcpy(RIG->Valchar, RIG->WEB_FREQ);

			PORT->Timeout = 0;
		}
	}
	else if (Msg[0] == 'M' && Msg[1] == 'D')
	{
		int Mode;

		Mode = Msg[2] - 48;
		if (Mode > 7) Mode = 7;
		SetWindowText(RIG->hMODE, KenwoodModes[Mode]);
		strcpy(RIG->WEB_MODE, KenwoodModes[Mode]);
		strcpy(RIG->ModeString, RIG->WEB_MODE);
	
	}

	if (CmdLen < Length)
	{
		// Another Message in Buffer

		ptr++;
		Length -= (int)(ptr - Msg);

		if (Length <= 0)
			return;

		memmove(Msg, ptr, Length +1);

		goto LoopR;
	}
}



VOID ProcessKenwoodFrame(struct RIGPORTINFO * PORT, int Length)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	UCHAR * ptr;
	int CmdLen;

	Msg[Length] = 0;
	
	if (PORT->PORTOK == FALSE)
	{
		// Just come up		
		PORT->PORTOK = TRUE;
	}

	RIG->RIGOK = TRUE;

	if (!PORT->AutoPoll)
	{
		// Response to a RADIO Command

		if (Msg[0] == '?')
			SendResponse(RIG->Session, "Sorry - Command Rejected");
		else if (Msg[0] == 'A' && Msg[1] == 'C')
			SendResponse(RIG->Session, "TUNE OK");
		else if (Msg[0] == 'P' && Msg[1] == 'C') 
			SendResponse(RIG->Session, "Power Set OK");
		else
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
	
		PORT->AutoPoll = TRUE;
		return;
	}

Loop:

	if (PORT->PortType == FLEX)
	{
		Msg += 2;						// Skip ZZ
		Length -= 2;
	}

	ptr = strchr(Msg, ';');
	CmdLen = (int)(ptr - Msg + 1);

	if (Msg[0] == 'F' && Msg[1] == 'A' && CmdLen > 9)
	{
		char CharMHz[16] = "";
		char CharHz[16] = "";

		int i;
		long long Freq;
		long long MHz, Hz;

		Freq = strtoll(&Msg[2], NULL, 10) + RIG->rxOffset;

		RIG->RigFreq = Freq / 1000000.0;

		// If we convert to float to display we get rounding errors, so convert to MHz and Hz to display

		MHz = Freq / 1000000;

		Hz = Freq - MHz * 1000000;

		sprintf(CharMHz, "%lld", MHz);
		sprintf(CharHz, "%06lld", Hz);

/*
		if (PORT->PortType == FT2000)
		{
			memcpy(FreqDecimal,&Msg[4], 6);
			Msg[4] = 0;
		}
		else if (PORT->PortType == FT991A)
		{
			memcpy(FreqDecimal,&Msg[5], 6);
			Msg[5] = 0;
		}

		else
		{
			memcpy(FreqDecimal,&Msg[7], 6);
			Msg[7] = 0;
		}
		FreqDecimal[6] = 0;
*/

		for (i = 5; i > 2; i--)
		{
			if (CharHz[i] == '0')
				CharHz[i] = 0;
			else
				break;
		}


		sprintf(RIG->WEB_FREQ,"%lld.%s", MHz, CharHz);
		SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		strcpy(RIG->Valchar, RIG->WEB_FREQ);

		PORT->Timeout = 0;
	}
	else if (Msg[0] == 'M' && Msg[1] == 'D')
	{
		int Mode;
		
		if (PORT->PortType == FT2000)
		{
			Mode = Msg[3] - 48;
			if (Mode > 13) Mode = 13;
			SetWindowText(RIG->hMODE, FT2000Modes[Mode]);
			strcpy(RIG->WEB_MODE, FT2000Modes[Mode]);
			strcpy(RIG->ModeString, RIG->WEB_MODE);
		}
		else if (PORT->PortType == FT991A)
		{
			Mode = Msg[3] - 48;
			if (Mode > 16)
				Mode -= 7;
			
			if (Mode > 15) Mode = 13;
			SetWindowText(RIG->hMODE, FT991AModes[Mode]);
			strcpy(RIG->WEB_MODE, FT991AModes[Mode]);
			strcpy(RIG->ModeString, RIG->WEB_MODE);
		}
		else if (PORT->PortType == FTDX10)
		{
			Mode = Msg[3] - 48;
			if (Mode > 16)
				Mode -= 7;
			
			if (Mode > 15) Mode = 15;
			SetWindowText(RIG->hMODE, FTDX10Modes[Mode]);
			strcpy(RIG->WEB_MODE, FTDX10Modes[Mode]);
			strcpy(RIG->ModeString, RIG->WEB_MODE);
		}
		else if (PORT->PortType == FLEX)
		{
			Mode = atoi(&Msg[3]);
			if (Mode > 12) Mode = 12;
			SetWindowText(RIG->hMODE, FLEXModes[Mode]);
			strcpy(RIG->WEB_MODE, FLEXModes[Mode]);
			strcpy(RIG->ModeString, RIG->WEB_MODE);
		}
		else
		{
			Mode = Msg[2] - 48;
			if (Mode > 7) Mode = 7;
			SetWindowText(RIG->hMODE, KenwoodModes[Mode]);
			strcpy(RIG->WEB_MODE, KenwoodModes[Mode]);
			strcpy(RIG->ModeString, RIG->WEB_MODE);
		}
	}

	if (CmdLen < Length)
	{
		// Another Message in Buffer

		ptr++;
		Length -= (int)(ptr - Msg);

		if (Length <= 0)
			return;

		memmove(Msg, ptr, Length +1);

		goto Loop;
	}
}


VOID KenwoodPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Kenwood

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");

		RIG->RIGOK = FALSE;

		return;
	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				RigWriteCommBlock(PORT);
				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission

			return;
			}
		}
	}
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		struct MSGWITHOUTLEN * buffptr;
			
		buffptr = Q_REM(&RIG->BPQtoRADIO_Q);

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr->Data, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.
		
		if (RIG->RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

		memcpy(Poll, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);

		PORT->TXLen = PORT->FreqPtr->Cmd1Len;
		RigWriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Timeout = 0;
		RIG->PollCounter = 10;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}
		
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && RIG->ScanStopped == 0 && RIG->NumberofBands &&
		RIG->ScanCounter && RIG->ScanCounter < 30)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 10;			// Once Per Sec
		
	// Read Frequency 

	PORT->TXLen = RIG->PollLen;
	strcpy(Poll, RIG->Poll);
	
	RigWriteCommBlock(PORT);
	PORT->Retries = 1;
	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID SDRRadioPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG;
	int i;

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];

		if (RIG->ScanStopped == 0)
			if (RIG->ScanCounter)
				RIG->ScanCounter--;
	}

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");

		RIG->RIGOK = FALSE;

		return;
	}

	// Send Data if avail, else send poll


	PORT->CurrentRig++;

	if (PORT->CurrentRig >= PORT->ConfiguredRigs)
		PORT->CurrentRig = 0;

	RIG = &PORT->Rigs[PORT->CurrentRig];


	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				RigWriteCommBlock(PORT);
				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission

			return;
			}
		}
	}
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		struct MSGWITHOUTLEN * buffptr;
			
		buffptr = Q_REM(&RIG->BPQtoRADIO_Q);

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr->Data, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.
		
		if (RIG->RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

		memcpy(Poll, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);

		PORT->TXLen = PORT->FreqPtr->Cmd1Len;
		RigWriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Timeout = 0;
		RIG->PollCounter = 10;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}
		
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && RIG->ScanStopped == 0 && RIG->NumberofBands &&
		RIG->ScanCounter && RIG->ScanCounter < 30)
		return;						// no point in reading freq if we are about to change it

	// Need to make sure we don't poll multiple rigs on port at the same time

	if (RIG->RIGOK)
	{
		PORT->Retries = 2;
		RIG->PollCounter = 10 / PORT->ConfiguredRigs;			// Once Per Sec
	}
	else
	{
		PORT->Retries = 1;
		RIG->PollCounter = 100 / PORT->ConfiguredRigs;			// Slow Poll if down
	}

	RIG->PollCounter += PORT->CurrentRig * 3;

	// Read Frequency 

	PORT->TXLen = RIG->PollLen;
	strcpy(Poll, RIG->Poll);
	
	RigWriteCommBlock(PORT);
	PORT->Retries = 1;
	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID DummyPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;
	
	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;
/*
				RigWriteCommBlock(PORT);
				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;
*/
				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission

			return;
			}
		}
	}
	
	return;
}

VOID SwitchAntenna(struct RIGINFO * RIG, char Antenna)
{
	struct RIGPORTINFO * PORT;
	char Ant[3]="  ";

	if (RIG == NULL) return;

	PORT = RIG->PORT;

	Ant[1] = Antenna;

	SetWindowText(RIG->hPTT, Ant);

	switch (Antenna)
	{
	case '1':
		COMClearDTR(PORT->hDevice);
		COMClearRTS(PORT->hDevice);
		break;
	case '2':
		COMSetDTR(PORT->hDevice);
		COMClearRTS(PORT->hDevice);
		break;
	case '3':
		COMClearDTR(PORT->hDevice);
		COMSetRTS(PORT->hDevice);
		break;
	case '4':
		COMSetDTR(PORT->hDevice);
		COMSetRTS(PORT->hDevice);
		break;
	}	
}

BOOL DecodeModePtr(char * Param, double * Dwell, double * Freq, char * Mode,
				   char * PMinLevel, char * PMaxLevel, char * PacketMode,
				   char * RPacketMode, char * Split, char * Data, char * WinmorMode,
				   char * Antenna, BOOL * Supress, char * Filter, char * Appl,
				   char * MemoryBank, int * MemoryNumber, char * ARDOPMode, char * VARAMode, int * BandWidth, int * Power)
{
	char * Context;
	char * ptr;

	*Filter = '1';			// Defaults
	*PMinLevel = 1;
	*MemoryBank = 0;
	*MemoryNumber = 0;
	*Mode = 0;
	*ARDOPMode = 0;
	*VARAMode = 0;
	*Power = 0;

	
	ptr = strtok_s(Param, ",", &Context);

	if (ptr == NULL)
		return FALSE;

	// "New" format - Dwell, Freq, Params.
	
	//	Each param is a 2 char pair, separated by commas

	// An - Antenna
	// Pn - Pactor
	// Wn - Winmor
	// Pn - Packet
	// Fn - Filter
	// Sx - Split

	// 7.0770/LSB,F1,A3,WN,P1,R1 

	*Dwell = atof(ptr);
	
	ptr = strtok_s(NULL, ",", &Context);

	if (ptr == NULL)
		return FALSE;


	// May be a frequency or a Memory Bank/Channel 

	if (_memicmp(ptr, "Chan", 4) == 0)
	{
		if (strchr(ptr, '/'))		// Bank/Chan
		{
			memcpy(MemoryBank, &ptr[4], 1);
			*MemoryNumber = atoi(&ptr[6]);
		}
		else
			*MemoryNumber = atoi(&ptr[4]);	// Just Chan

		*Freq = 0.0;
	}
	else
		*Freq = atof(ptr);

	ptr = strtok_s(NULL, ",", &Context);

	if (ptr == NULL || strlen(ptr) >  8)
		return FALSE;

	// If channel, dont need mode

	if (*MemoryNumber == 0)
	{
		strcpy(Mode, ptr); 
		ptr = strtok_s(NULL, ",", &Context);
	}

	while (ptr)
	{
		if (isdigit(ptr[0]))
			*BandWidth = atoi(ptr);

		else if (memcmp(ptr, "APPL=", 5) == 0)
			strcpy(Appl, ptr + 5);

		else if (memcmp(ptr, "POWER=", 6) == 0)
			*Power = atoi(ptr + 6);

		else if (ptr[0] == 'A' && (ptr[1] == 'S' || ptr[1] == '0') && strlen(ptr) < 7)
			strcpy(ARDOPMode, "S");

		else if (ptr[0] == 'A' && strlen(ptr) > 2 && strlen(ptr) < 7)
			strcpy(ARDOPMode, &ptr[1]);

		else if (ptr[0] == 'A' && strlen(ptr) == 2)
			*Antenna = ptr[1];
		
		else if (ptr[0] == 'F')
			*Filter = ptr[1];

		else if (ptr[0] == 'R')
			*RPacketMode = ptr[1];
		
		else if (ptr[0] == 'H')
			*PacketMode = ptr[1];

		else if (ptr[0] == 'N')
			*PacketMode = ptr[1];

		else if (ptr[0] == 'P')
		{
			*PMinLevel = ptr[1];
			*PMaxLevel = ptr[strlen(ptr) - 1];
		}
		else if (ptr[0] == 'W')
		{
			*WinmorMode = ptr[1];
			if (*WinmorMode == '0')
				*WinmorMode = 'X';
			else if (*WinmorMode == '1')
				*WinmorMode = 'N';
			else if (*WinmorMode == '2')
				*WinmorMode = 'W';
		}

		else if (ptr[0] == 'V')
		{
			*VARAMode = ptr[1];
			// W N S T (skip) or 0 (also Skip)
			if (ptr[1] == '0')
				*VARAMode = 'S';
		}
		else if (ptr[0] == '+')
			*Split = '+';
		else if (ptr[0] == '-')
			*Split = '-';
		else if (ptr[0] == 'S')
			*Split = 'S';
		else if (ptr[0] == 'D')
			*Data = 1;
		else if (ptr[0] == 'X')
			*Supress = TRUE;

		ptr = strtok_s(NULL, ",", &Context);
	}
	return TRUE;
}

VOID AddNMEAChecksum(char * msg)
{
	UCHAR CRC = 0;

	msg++;					// Skip $

	while (*(msg) != '*')
	{
		CRC ^= *(msg++);
	}

	sprintf(++msg, "%02X\r\n", CRC);
}

void DecodeRemote(struct RIGPORTINFO * PORT, char * ptr)
{
	// Param is IPHOST:PORT for use with WINMORCONTROL
	
	struct sockaddr_in * destaddr = (SOCKADDR_IN * )&PORT->remoteDest;
	UCHAR param = 1;
	u_long ioparam = 1;

	char * port = strlop(ptr, ':');

	PORT->remoteSock = socket(AF_INET,SOCK_DGRAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
		return;

	setsockopt (PORT->remoteSock, SOL_SOCKET, SO_BROADCAST, &param, 1);

	if (port == NULL)
		return;

	ioctl (PORT->remoteSock, FIONBIO, &ioparam);

	destaddr->sin_family = AF_INET;
	destaddr->sin_addr.s_addr = inet_addr(ptr);
	destaddr->sin_port = htons(atoi(port));

	if (destaddr->sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		struct hostent * HostEnt = gethostbyname(ptr);
		 
		if (!HostEnt)
			return;			// Resolve failed

		memcpy(&destaddr->sin_addr.s_addr,HostEnt->h_addr,4);
	}

	return;
}


char * CM108Device = NULL;

void DecodeCM108(int Port, char * ptr)
{
	// Called if Device Name or PTT = Param is CM108

#ifdef WIN32

	// Next Param is VID and PID - 0xd8c:0x8 or Full device name
	// On Windows device name is very long and difficult to find, so 
	//	easier to use VID/PID, but allow device in case more than one needed

	char * next;
	int32_t  VID = 0, PID = 0;
	char product[256];
	char sernum[256] = "NULL";

	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;

	if (strlen(ptr) > 16)
		CM108Device = _strdup(ptr);
	else
	{
		VID = strtol(ptr, &next, 0);
		if (next)
			PID = strtol(++next, &next, 0);

		// Look for Device
	
		devs = hid_enumerate(0,0); // so we list devices(USHORT)VID, (USHORT)PID);
		cur_dev = devs;
		while (cur_dev)
		{
			wcstombs(product, cur_dev->product_string, 255);
			if (cur_dev->serial_number)
				wcstombs(sernum, cur_dev->serial_number, 255);

			if (product)
				Debugprintf("HID Device %s VID %X PID %X Ser %s %s", product, cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);
			else
				Debugprintf("HID Device %s VID %X PID %X Ser %s %s", "Missing Product", cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);
				
			if (cur_dev->vendor_id == VID && cur_dev->product_id == PID)
				path_to_open = cur_dev->path;

			cur_dev = cur_dev->next;
		}
	
		if (path_to_open)
		{
			handle = hid_open_path(path_to_open);
	
			if (handle)
			{
				hid_close(handle);
				CM108Device = _strdup(path_to_open);
			}
			else
			{
				char msg[128];
				sprintf(msg,"Port %d Unable to open CM108 device %x %x", Port, VID, PID);
				WritetoConsole(msg);
			}
		}
		hid_free_enumeration(devs);
	}
#else
		
	// Linux - Next Param HID Device, eg /dev/hidraw0

	CM108Device = _strdup(ptr);
#endif
}



// Called by Port Driver .dll to add/update rig info 

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1


struct RIGINFO * RigConfig(struct TNCINFO * TNC, char * buf, int Port)
{
	int i;
	char * ptr;
	char * COMPort = NULL;
	char * RigName;
	int RigAddr;
	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;
	struct ScanEntry ** FreqPtr;
	char * CmdPtr;
	char * Context;
	struct TimeScan * SaveBand;
	char PTTRigName[] = "PTT";
	double ScanFreq;
	double Dwell;
	char MemoryBank;	// For Memory Scanning
	int MemoryNumber;
	BOOL RIG_DEBUG = FALSE;

	BOOL PTTControlsInputMUX = FALSE;
	BOOL DataPTT = FALSE;
	int DataPTTOffMode = 1;				// ACC
	int ICF8101Mode = 8;				// USB (as in upper sideband)

	char onString[256] = "";
	char offString[256] = "";
	int onLen = 0;
	int offLen = 0;

	CM108Device = NULL;
	Debugprintf("Processing RIG line %s", buf);

	// Starts RADIO Interlockgroup

	ptr = strtok_s(&buf[5], " \t\n\r", &Context);	// Skip Interlock
	if (ptr == NULL) return FALSE;

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr == NULL) return FALSE;

	if (_memicmp(ptr, "DEBUG", 5) == 0)
	{
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		RIG_DEBUG = TRUE;
	}

	if (_memicmp(ptr, "AUTH", 4) == 0)
	{
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if (ptr == NULL) return FALSE;
		if (strlen(ptr) > 100) return FALSE;

		strcpy(AuthPassword, ptr);
		ptr = strtok_s(NULL, " \t\n\r", &Context);
	}

	if (ptr == NULL || ptr[0] == 0)
		return FALSE;

	if (_memicmp(ptr, "DUMMY", 5) == 0)
	{
		// Dummy to allow PTC application scanning

		PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));
		PORT->PortType = DUMMY;
		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
		RIG->RIGOK = TRUE;

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		goto CheckScan;
	}

	if (_memicmp(ptr, "FLRIG", 5) == 0)
	{
		// Use FLRIG

		// Need parameter - Host:Port

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		if (ptr == NULL || strlen(ptr) > 79) return FALSE;

		// Have a parameter to define port. Will decode it later

		PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));
		PORT->PortType = FLRIG;
		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
		RIG->RIGOK = TRUE;
		RIG->PORT = PORT;

		strcpy(PORT->IOBASE, ptr);
		strcpy(RIG->RigName, "FLRIG");

		// Decode host

		DecodeHAMLIBAddr(PORT, ptr);

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		// look for other config statementes and scan params

		goto CheckOtherParams;
	}

	if (_memicmp(ptr, "HAMLIB", 5) == 0)
	{
		// Use rigctld

		// Need parameter - Host:Port

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		if (ptr == NULL || strlen(ptr) > 79) return FALSE;

		// Have a parameter to define port. Will decode it later

		PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));
		PORT->PortType = HAMLIB;
		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
		RIG->RIGOK = TRUE;
		RIG->PORT = PORT;

		strcpy(PORT->IOBASE, ptr);
		strcpy(RIG->RigName, "HAMLIB");

		// Decode host

		DecodeHAMLIBAddr(PORT, ptr);

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		// look for scan params

		goto CheckOtherParams;
	}

	if (_memicmp(ptr, "rtludp", 5) == 0)
	{
		// rtl_fm with udp freq control 

		// Need parameter - Host:Port

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		if (ptr == NULL || strlen(ptr) > 79) return FALSE;

		// Have a parameter to define port. Will decode it later

		PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));
		PORT->PortType = RTLUDP;
		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
		RIG->RIGOK = TRUE;
		RIG->PORT = PORT;

		strcpy(PORT->IOBASE, ptr);
		strcpy(RIG->RigName, "RTLUDP");

		// Decode host

		DecodeHAMLIBAddr(PORT, ptr);

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		// look for scan params

		goto CheckOtherParams;
	}

// ---- G7TAJ ----

	if (_memicmp(ptr, "sdrangel", 5) == 0)
	{
		// each instance (ip addr/port) of sdrangle can have one or more sampling devices (eg rltsdr) each with one ot
		// more channels (eg ssb demod, ssb mod). each set of sampling device = channel(s) is a device set.

		// We poll all devices/channels at once. we one PORT record plus a RIG record for each channel

		// Need parameters - Host:Port device channel. Device and Channel will default to zero

		int device = 0, channel = 0;
		char * Name;
		char * nptr1;
		char * nptr2;


		ptr = strtok_s(NULL, " \t\n\r", &Context);

		if (ptr == NULL || strlen(ptr) > 79) return FALSE;

		Name = strtok_s(NULL, " \t\n\r", &Context);
		nptr1 = strtok_s(NULL, " \t\n\r", &Context);
		nptr2 = strtok_s(NULL, " \t\n\r", &Context);

		if (nptr1 == 0 || nptr2 == 0 || Name == NULL || strlen(Name) > 9)
			return FALSE;

		device = atoi(nptr1);	
		channel = atoi(nptr2);

		// Have a parameter to define port. Will decode it later

		// See if already defined. PORT->IOBASE has Host:Port

		for (i = 0; i < NumberofPorts; i++)
		{
			PORT = PORTInfo[i];

			if (strcmp(PORT->IOBASE, ptr) == 0) 
				goto AngelRigFound;
		}

		// New Port

		PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));
		PORT->PortType = SDRANGEL;
		PORT->ConfiguredRigs = 0;
		strcpy(PORT->IOBASE, ptr);

		// Decode host
		
		DecodeHAMLIBAddr(PORT, ptr);


AngelRigFound:

		RIG = &PORT->Rigs[PORT->ConfiguredRigs++];
		RIG->RIGOK = TRUE;
		RIG->PORT = PORT;
		RIG->RigAddr = device;
		RIG->Channel = channel;

		strcpy(RIG->RigName, Name);

		ptr = strtok_s(NULL, " \t\n\r", &Context);

		// look for scan params

		goto CheckOtherParams;
	}
// ---- G7TAJ ----

	if ((_memicmp(ptr, "VCOM", 4) == 0) && TNC->Hardware == H_SCS)		// Using Radio Port on PTC
		COMPort = 0;
	else if (_memicmp(ptr, "PTCPORT", 7) == 0)
		COMPort = 0;
	else
		COMPort = ptr;

	// See if port is already defined. We may be adding another radio (ICOM or SDRRADIO only) or updating an existing one

	// Unless CM108 - they must be on separate Ports

	if (COMPort && _stricmp("CM108", COMPort) != 0)
	{
		for (i = 0; i < NumberofPorts; i++)
		{
			PORT = PORTInfo[i];

			if (COMPort)
				if (strcmp(PORT->IOBASE, COMPort) == 0)
					goto PortFound;

			if (COMPort == 0)
				if (PORT->IOBASE == COMPort)
					goto PortFound;
		}
	}

	// Allocate a new one

	PORT = PORTInfo[NumberofPorts++] = zalloc(sizeof(struct RIGPORTINFO));

	if (COMPort)
		strcpy(PORT->IOBASE, COMPort);

PortFound:

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr == NULL) return (FALSE);

	if (_stricmp(PORT->IOBASE, "RAWHID") == 0) // HID Addr instead of Speed
	{
		DecodeCM108(Port, ptr);
		if (CM108Device)	
			PORT->HIDDevice = CM108Device;
		else
			PORT->HIDDevice = _strdup ("MissingHID");

		CM108Device = 0;
	}

	if ( _stricmp(PORT->IOBASE, "CM108") == 0) // HID Addr instead of Speed
	{
		DecodeCM108(Port, ptr);
	}

	if (_stricmp(PORT->IOBASE, "REMOTE") == 0) // IP Addr/Port
		DecodeRemote(PORT, ptr);
	else	
		PORT->SPEED = atoi(ptr);

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr == NULL) return (FALSE);

	if (_memicmp(ptr, "PTTCOM", 6) == 0 || _memicmp(ptr, "PTT=", 4) == 0)
	{
		if (_stricmp(ptr, "PTT=CM108") == 0)
		{
			// PTT on CM108 GPIO

			ptr = strtok_s(NULL, " \t\n\r", &Context);
			if (ptr == NULL) return (FALSE);

			DecodeCM108(Port, ptr);
		}
		else
		{
			strcpy(PORT->PTTIOBASE, ptr);
		}
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if (ptr == NULL) return (FALSE);

	}

	//		if (strcmp(ptr, "ICOM") == 0 || strcmp(ptr, "YAESU") == 0 
	//			|| strcmp(ptr, "KENWOOD") == 0 || strcmp(ptr, "PTTONLY") == 0 || strcmp(ptr, "ANTENNA") == 0)

	// RADIO IC706 4E 5 14.103/U1 14.112/u1 18.1/U1 10.12/l1
	// Read RADIO Lines

	_strupr(ptr);


	if (strcmp(ptr, "ICOM") == 0)
		PORT->PortType = ICOM;
	else if (strcmp(ptr, "YAESU") == 0)
		PORT->PortType = YAESU;
	else if (strcmp(ptr, "KENWOOD") == 0)
		PORT->PortType = KENWOOD;
	else if (strcmp(ptr, "SDRRADIO") == 0)
		PORT->PortType = SDRRADIO;				// Varient of KENWOOD that supports multiple devices on one serial port
	else if (strcmp(ptr, "FLEX") == 0)
		PORT->PortType = FLEX;
	else if (strcmp(ptr, "NMEA") == 0)
		PORT->PortType = NMEA;
	else if (strcmp(ptr, "PTTONLY") == 0)
		PORT->PortType = PTT;
	else if (strcmp(ptr, "ANTENNA") == 0)
		PORT->PortType = ANT;
	else
		return FALSE;

	Debugprintf("Port type = %d", PORT->PortType);

	_strupr(Context);

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr && memcmp(ptr, "HAMLIB=", 7) == 0)
	{
		// HAMLIB Emulator - param is port to listen on

		if (PORT->PortType == PTT)
		{
			RIG = &PORT->Rigs[PORT->ConfiguredRigs++];
			strcpy(RIG->RigName, PTTRigName);

			RIG->HAMLIBPORT = atoi(&ptr[7]);
			ptr = strtok_s(NULL, " \t\n\r", &Context);

		}
	}

	if (ptr && strcmp(ptr, "PTTMUX") == 0)
	{
		if (PORT->PortType == PTT)
		{
			RIG = &PORT->Rigs[PORT->ConfiguredRigs++];
			strcpy(RIG->RigName, PTTRigName);
			goto domux;	// PTTONLY with PTTMUX
		}
	}


	if (ptr == NULL)
	{
		if (PORT->PortType == PTT)
			ptr = PTTRigName;
		else
			return FALSE;
	}

	if (strlen(ptr) > 9) return FALSE;

	RigName =  ptr;

	Debugprintf("Rigname = *%s*", RigName);

	// FT100 seems to be different to most other YAESU types

	if (strcmp(RigName, "FT100") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT100;
	}

	// FT990 seems to be different to most other YAESU types

	if (strcmp(RigName, "FT990") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT990;
	}

	// FT1000 seems to be different to most other YAESU types

	if (strstr(RigName, "FT1000") && PORT->PortType == YAESU)
	{
		PORT->PortType = FT1000;

		// Subtypes need different code. D and no suffix are same

		if (strstr(RigName, "FT1000MP"))
			PORT->YaesuVariant = FT1000MP;
		else
			PORT->YaesuVariant = FT1000D;
	}

	// FT2000 seems to be different to most other YAESU types

	if (strcmp(RigName, "FT2000") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT2000;
	}

	// FTDX10 seems to be different to most other YAESU types

	if (strcmp(RigName, "FTDX10") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FTDX10;
	}

	// FT991A seems to be different to most other YAESU types

	if (strcmp(RigName, "FT991A") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT991A;
	}


	// If ICOM or SDRRADIO, we may be adding a new Rig

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (PORT->PortType == ICOM || PORT->PortType == NMEA || PORT->PortType == SDRRADIO)
	{
		if (ptr == NULL) return (FALSE);

		if (PORT->PortType == SDRRADIO)
			RigAddr = ptr[0];
		else
			sscanf(ptr, "%x", &RigAddr);

		// See if already defined

		for (i = 0; i < PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->RigAddr == RigAddr)
				goto RigFound;
		}

		// Allocate a new one

		RIG = &PORT->Rigs[PORT->ConfiguredRigs++];
		RIG->RigAddr = RigAddr;

RigFound:

		ptr = strtok_s(NULL, " \t\n\r", &Context);
		//		if (ptr == NULL) return (FALSE);
	}
	else
	{
		// Only allows one RIG

		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
	}

	RIG->RIG_DEBUG = RIG_DEBUG;
	RIG->PORT = PORT;

	strcpy(RIG->RigName, RigName);

	RIG->TSMenu = 63;		// Default to TS590S

	// IC735 uses shorter freq message

	if (strcmp(RigName, "IC735") == 0 && PORT->PortType == ICOM)
		RIG->IC735 = TRUE;

	// IC-F8101 uses a different set of commands

	if (strstr(RigName, "F8101") && PORT->PortType == ICOM)
		RIG->ICF8101 = TRUE;

	if (PORT->PortType == KENWOOD)
	{
		RIG->TSMenu = 63;		// Default to TS590S

		if (strcmp(RigName, "TS590SG") == 0)
			RIG->TSMenu = 69;
	}

	if (PORT->PortType == FT991A)
		RIG->TSMenu = 72;			//Menu for Data/USB siwtching

domux:	

	RIG->CM108Device = CM108Device;

CheckOtherParams:

	while (ptr)
	{
		if (strcmp(ptr, "PTTMUX") == 0)
		{
			// Ports whose RTS/DTR will be converted to CAT commands (for IC7100/IC7200 etc)

			int PIndex = 0;

			ptr = strtok_s(NULL, " \t\n\r", &Context);

			while (memcmp(ptr, "COM", 3) == 0)
			{
				char * tncport = strlop(ptr, '/');

				strcpy(RIG->PTTCATPort[PIndex], &ptr[3]);

				if (tncport)
					RIG->PTTCATTNC[PIndex] = TNCInfo[atoi(tncport)];
				
				if (PIndex < 3)
					PIndex++;

				ptr = strtok_s(NULL, " \t\n\r", &Context);

				if (ptr == NULL)
					break;
			}
			if (ptr == NULL)
				break;
			else 
				continue;
		}
		else if (strcmp(ptr, "PTT_SETS_INPUT") == 0)
		{
			// Send Select Soundcard as mod source with PTT commands

			PTTControlsInputMUX = TRUE;

			// See if following param is an PTT Off Mode

			ptr = strtok_s(NULL, " \t\n\r", &Context);

			if (ptr == NULL)
				break;

			if (strcmp(ptr, "MIC") == 0)
				DataPTTOffMode = 0;
			else if (strcmp(ptr, "AUX") == 0)
				DataPTTOffMode = 1;
			else if (strcmp(ptr, "MICAUX") == 0)
				DataPTTOffMode = 2;
			else if (RIG->ICF8101 && strcmp(ptr, "LSB") == 0)
				ICF8101Mode = 0x7;
			else if (RIG->ICF8101 && strcmp(ptr, "USB") == 0)
				ICF8101Mode = 0x8;
			else if (RIG->ICF8101 && strcmp(ptr, "LSBD1") == 0)
				ICF8101Mode = 0x18;
			else if (RIG->ICF8101 && strcmp(ptr, "USBD1") == 0)
				ICF8101Mode = 0x19;
			else if (RIG->ICF8101 && strcmp(ptr, "LSBD2") == 0)
				ICF8101Mode = 0x20;
			else if (RIG->ICF8101 && strcmp(ptr, "USBD2") == 0)
				ICF8101Mode = 0x21;
			else if (RIG->ICF8101 && strcmp(ptr, "LSBD3") == 0)
				ICF8101Mode = 0x22;
			else if (RIG->ICF8101 && strcmp(ptr, "USBD3") == 0)
				ICF8101Mode = 0x23;
			else
				continue;

			ptr = strtok_s(NULL, " \t\n\r", &Context);

			continue;

		}
		else if (strcmp(ptr, "PTT_SETS_FREQ") == 0)
		{
			// Send Select Soundcard as mod source with PTT commands

			RIG->PTTSetsFreq = TRUE;
			ptr = strtok_s(NULL, " \t\n\r", &Context);

			continue;

		}

		else if (strcmp(ptr, "DATAPTT") == 0)
		{
			// Send Select Soundcard as mod source with PTT commands

			DataPTT = TRUE;
		}

		else if (memcmp(ptr, "PTTONHEX=", 9) == 0)
		{
			// Hex String to use for PTT on

			char * ptr1 = ptr;
			char * ptr2 = onString ;
			int i, j, len;

			ptr1 += 9;
			onLen = len = strlen(ptr1) / 2;

			if (len < 240)
			{
				while ((len--) > 0)
				{
					i = *(ptr1++);
					i -= '0';
					if (i > 9)
						i -= 7;

					j = i << 4;

					i = *(ptr1++);
					i -= '0';
					if (i > 9)
						i -= 7;

					*(ptr2++) = j | i;
				}
			}

			ptr = strtok_s(NULL, " \t\n\r", &Context);

			if (ptr == NULL)
				break;
		}

		else if (memcmp(ptr, "PTTOFFHEX=", 10) == 0)
		{
			// Hex String to use for PTT off

			char * ptr2 = offString ;
			int i, j, len;

			ptr += 10;
			offLen = len = strlen(ptr) / 2;

			if (len < 240)
			{
				while ((len--) > 0)
				{
					i = *(ptr++);
					i -= '0';
					if (i > 9)
						i -= 7;

					j = i << 4;

					i = *(ptr++);
					i -= '0';
					if (i > 9)
						i -= 7;

					*(ptr2++) = j | i;
				}
			}
			ptr = strtok_s(NULL, " \t\n\r", &Context);

			if (ptr == NULL)
				break;

		}

		else if (memcmp(ptr, "HAMLIB=", 7) == 0)
		{
			// HAMLIB Emulator - param is port to listen on

			RIG->HAMLIBPORT = atoi(&ptr[7]);
		}

		else if (memcmp(ptr, "TXOFFSET", 8) == 0)
		{
			RIG->txOffset =  strtoll(&ptr[9], NULL, 10);
		}

		else if (memcmp(ptr, "RXOFFSET", 8) == 0)
		{
			RIG->rxOffset =  strtoll(&ptr[9], NULL, 10);
		}

		else if (memcmp(ptr, "PTTOFFSET", 9) == 0)
		{
			RIG->pttOffset =  strtoll(&ptr[10], NULL, 10);
		}

		else if (memcmp(ptr, "RXERROR", 7) == 0)
		{
			RIG->rxError = atoi(&ptr[8]);
		}

		else if (memcmp(ptr, "TXERROR", 7) == 0)
		{
			RIG->txError = atoi(&ptr[8]);
		}

		else if (memcmp(ptr, "REPORTFREQS", 11) == 0)
		{
			RIG->reportFreqs = _strdup(&ptr[12]);
		}

		else if (memcmp(ptr, "DEFAULTFREQ", 11) == 0)
		{
			RIG->defaultFreq = atoi(&ptr[12]);
		}
		
		else if (atoi(ptr) || ptr[2] == ':')
			break;					// Not scan freq oe timeband, so see if another param

		ptr = strtok_s(NULL, " \t\n\r", &Context);
	}

	if (PORT->PortType == PTT || PORT->PortType == ANT)
		return RIG;

	// Set up PTT and Poll Strings

	if (PORT->PortType == ICOM)
	{
		char * Poll;
		Poll = &RIG->PTTOn[0];

		if (onLen && offLen)
		{
			memcpy(RIG->PTTOn, onString, onLen);
			RIG->PTTOnLen = onLen;
			memcpy(RIG->PTTOff, offString, offLen);
			RIG->PTTOffLen = offLen;
		}

		else if (RIG->ICF8101)
		{
			// Need to send ACC PTT command (1A 37 0002), not normal ICOM IC 00

			if (PTTControlsInputMUX)
			{
				*(Poll++) = 0xFE;
				*(Poll++) = 0xFE;
				*(Poll++) = RIG->RigAddr;
				*(Poll++) = 0xE0;
				*(Poll++) = 0x1a;
				*(Poll++) = 0x05;
				*(Poll++) = ICF8101Mode;
				*(Poll++) = 0x03;			// USB Data Mode Source
				*(Poll++) = 0x00;
				*(Poll++) = 0x01;			// Soundcard
				*(Poll++) = 0xFD;
			}
			*(Poll++) = 0xFE;
			*(Poll++) = 0xFE;
			*(Poll++) = RIG->RigAddr;
			*(Poll++) = 0xE0;
			*(Poll++) = 0x1A; 
			*(Poll++) = 0x37;		// Send/read the TX status
			*(Poll++) = 0x00;
			*(Poll++) = 0x02;		// ACC PTT
			*(Poll++) = 0xFD;

			RIG->PTTOnLen = (int)(Poll - &RIG->PTTOn[0]);

			Poll = &RIG->PTTOff[0];

			*(Poll++) = 0xFE;
			*(Poll++) = 0xFE;
			*(Poll++) = RIG->RigAddr;
			*(Poll++) = 0xE0;
			*(Poll++) = 0x1A; 
			*(Poll++) = 0x37;		// Send/read the TX status
			*(Poll++) = 0x00;
			*(Poll++) = 0x00;		// RX
			*(Poll++) = 0xFD;

			if (PTTControlsInputMUX)
			{
				*(Poll++) = 0xFE;
				*(Poll++) = 0xFE;
				*(Poll++) = RIG->RigAddr;
				*(Poll++) = 0xE0;
				*(Poll++) = 0x1a;

				*(Poll++) = 0x05;
				*(Poll++) = ICF8101Mode;
				*(Poll++) = 0x03;			// USB Data Mode Source
				*(Poll++) = 0x00;
				*(Poll++) = 0x02;			// ACC
				*(Poll++) = 0xFD;
			}
			RIG->PTTOffLen = (int)(Poll - &RIG->PTTOff[0]);
		}
		else
		{
			if (PTTControlsInputMUX)
			{
				*(Poll++) = 0xFE;
				*(Poll++) = 0xFE;
				*(Poll++) = RIG->RigAddr;
				*(Poll++) = 0xE0;
				*(Poll++) = 0x1a;

				if (strcmp(RIG->RigName, "IC7100") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x91;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7200") == 0)
				{
					*(Poll++) = 0x03;
					*(Poll++) = 0x24;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7300") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x67;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7600") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x31;			// Data1 Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7610") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x92;			// Data1 Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7410") == 0)
				{
					*(Poll++) = 0x03;
					*(Poll++) = 0x39;			// Data Mode Source
				}

				*(Poll++) = 0x03;			// USB Soundcard
				*(Poll++) = 0xFD;
			}

			*(Poll++) = 0xFE;
			*(Poll++) = 0xFE;
			*(Poll++) = RIG->RigAddr;
			*(Poll++) = 0xE0;
			*(Poll++) = 0x1C;		// RIG STATE
			*(Poll++) = 0x00;		// PTT
			*(Poll++) = 1;			// ON
			*(Poll++) = 0xFD;

			RIG->PTTOnLen = (int)(Poll - &RIG->PTTOn[0]);

			Poll = &RIG->PTTOff[0];

			*(Poll++) = 0xFE;
			*(Poll++) = 0xFE;
			*(Poll++) = RIG->RigAddr;
			*(Poll++) = 0xE0;
			*(Poll++) = 0x1C;		// RIG STATE
			*(Poll++) = 0x00;		// PTT
			*(Poll++) = 0;			// OFF
			*(Poll++) = 0xFD;

			if (PTTControlsInputMUX)
			{
				*(Poll++) = 0xFE;
				*(Poll++) = 0xFE;
				*(Poll++) = RIG->RigAddr;
				*(Poll++) = 0xE0;
				*(Poll++) = 0x1a;

				if (strcmp(RIG->RigName, "IC7100") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x91;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7200") == 0)
				{
					*(Poll++) = 0x03;
					*(Poll++) = 0x24;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7300") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x67;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7410") == 0)
				{
					*(Poll++) = 0x03;
					*(Poll++) = 0x39;			// Data Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7600") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x31;			// Data1 Mode Source
				}
				else if (strcmp(RIG->RigName, "IC7610") == 0)
				{
					*(Poll++) = 0x05;
					*(Poll++) = 0x00;
					*(Poll++) = 0x92;			// Data1 Mode Source
				}

				*(Poll++) = DataPTTOffMode;
				*(Poll++) = 0xFD;
			}
			RIG->PTTOffLen = (int)(Poll - &RIG->PTTOff[0]);
		}
	}
	else if	(PORT->PortType == KENWOOD)
	{	
		RIG->PollLen = 6;
		strcpy(RIG->Poll, "FA;MD;");

		if (PTTControlsInputMUX)
		{
			sprintf(RIG->PTTOn, "EX%03d00001;TX1;", RIG->TSMenu); // Select USB before PTT
			sprintf(RIG->PTTOff, "RX;EX%03d00000;", RIG->TSMenu); // Select ACC after dropping PTT
		}
		else
		{
			strcpy(RIG->PTTOff, "RX;");

			if (DataPTT)
				strcpy(RIG->PTTOn, "TX1;");
			else
				strcpy(RIG->PTTOn, "TX;");
		}

		RIG->PTTOnLen = (int)strlen(RIG->PTTOn);
		RIG->PTTOffLen = (int)strlen(RIG->PTTOff);

	}
	else if	(PORT->PortType == SDRRADIO)
	{	
		RIG->PollLen = sprintf(RIG->Poll, "F%c;MD;", RIG->RigAddr);

/*		if (PTTControlsInputMUX)
		{
			sprintf(RIG->PTTOn, "EX%03d00001;TX1;", RIG->TSMenu); // Select USB before PTT
			sprintf(RIG->PTTOff, "RX;EX%03d00000;", RIG->TSMenu); // Select ACC after dropping PTT
		}
		else
		{
			strcpy(RIG->PTTOff, "RX;");

			if (DataPTT)
				strcpy(RIG->PTTOn, "TX1;");
			else
				strcpy(RIG->PTTOn, "TX;");
		}
*/
		RIG->PTTOnLen = (int)strlen(RIG->PTTOn);
		RIG->PTTOffLen = (int)strlen(RIG->PTTOff);

	}	else if	(PORT->PortType == FLEX)
	{	
		RIG->PollLen = 10;
		strcpy(RIG->Poll, "ZZFA;ZZMD;");

		strcpy(RIG->PTTOn, "ZZTX1;");
		RIG->PTTOnLen = 6;
		strcpy(RIG->PTTOff, "ZZTX0;");
		RIG->PTTOffLen = 6;
	}
	else if	(PORT->PortType == FT2000)
	{	
		RIG->PollLen = 6;
		strcpy(RIG->Poll, "FA;MD;");

		strcpy(RIG->PTTOn, "TX1;");
		RIG->PTTOnLen = 4;
		strcpy(RIG->PTTOff, "TX0;");
		RIG->PTTOffLen = 4;
	}
	else if	(PORT->PortType == FT991A || PORT->PortType == FTDX10)
	{	
		RIG->PollLen = 7;
		strcpy(RIG->Poll, "FA;MD0;");

		if (PTTControlsInputMUX)
		{
			RIG->PTTOnLen = sprintf(RIG->PTTOn, "EX0721;TX1;");    // Select USB before PTT
			RIG->PTTOffLen = sprintf(RIG->PTTOff, "TX0;EX0720;");  // Select DATA after dropping PTT
		}
		else
		{
			strcpy(RIG->PTTOn, "TX1;");
			RIG->PTTOnLen = 4;
			strcpy(RIG->PTTOff, "TX0;");
			RIG->PTTOffLen = 4;
		}
	}
	else if	(PORT->PortType == NMEA)
	{	
		int Len;

		i = sprintf(RIG->Poll, "$PICOA,90,%02x,RXF*xx\r\n", RIG->RigAddr);
		AddNMEAChecksum(RIG->Poll);
		Len = i;
		i = sprintf(RIG->Poll + Len, "$PICOA,90,%02x,MODE*xx\r\n", RIG->RigAddr);
		AddNMEAChecksum(RIG->Poll + Len);
		RIG->PollLen = Len + i;

		i = sprintf(RIG->PTTOn, "$PICOA,90,%02x,TRX,TX*xx\r\n", RIG->RigAddr);
		AddNMEAChecksum(RIG->PTTOn);
		RIG->PTTOnLen = i;

		i = sprintf(RIG->PTTOff, "$PICOA,90,%02x,TRX,RX*xx\r\n", RIG->RigAddr);
		AddNMEAChecksum(RIG->PTTOff);
		RIG->PTTOffLen = i;
	}

	if (ptr == NULL) return RIG;			// No Scanning, just Interactive control

	if (strchr(ptr, ',') == 0 && strchr(ptr, ':') == 0) // Old Format
	{
		ScanFreq = atof(ptr);

#pragma warning(push)
#pragma warning(disable : 4244)

		RIG->ScanFreq = ScanFreq * 10;

#pragma warning(push)

		ptr = strtok_s(NULL, " \t\n\r", &Context);
	}

	// Frequency List

CheckScan:

	if (ptr)
		if (ptr[0] == ';' || ptr[0] == '#')
			ptr = NULL;

	if (ptr != NULL)
	{
		// Create Default Timeband

		struct TimeScan * Band;
		
		RIG->TimeBands = zalloc(sizeof(void *));

		Band = AllocateTimeRec(RIG);
		SaveBand = Band;

		Band->Start = 0;
		Band->End = 84540;	//23:59
		FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);
		memset(FreqPtr, 0, 1000);
	}

	while(ptr)
	{
		int ModeNo;
		BOOL Supress;
		double Freq = 0.0;
		int FreqInt = 0;
		char FreqString[80]="";
		char * Modeptr = NULL;
		char Split, Data, PacketMode, RPacketMode, PMinLevel, PMaxLevel, Filter;
		char Mode[10] = "";
		char WinmorMode, Antenna;
		char ARDOPMode[6] = "";
		char VARAMode[6] = "";
		char Appl[13];
		char * ApplCall;
		int BandWidth;
		int Power;

		if (ptr[0] == ';' || ptr[0] == '#')
			break;

		Filter = PMinLevel = PMaxLevel = PacketMode = RPacketMode = Split =
			Data = WinmorMode = Antenna = ModeNo = Supress = 
			MemoryBank = MemoryNumber = BandWidth = 0;

		Appl[0] = 0;
		ARDOPMode[0] = 0;
		VARAMode[0] = 0;
		Dwell = 0.0;

		while (ptr && strchr(ptr, ':'))
		{
			// New TimeBand

			struct TimeScan * Band;

			Band = AllocateTimeRec(RIG);

			*FreqPtr = (struct ScanEntry *)0;		// Terminate Last Band

			Band->Start = (atoi(ptr) * 3600) + (atoi(&ptr[3]) * 60);
			Band->End = 84540;	//23:59
			SaveBand->End = Band->Start - 60;

			SaveBand = Band;

			FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);
			memset(FreqPtr, 0, 1000);

			ptr = strtok_s(NULL, " \t\n\r", &Context);										
		}

		if (ptr == 0)
			break;

		if (strchr(ptr, ','))			// New Format
		{
			DecodeModePtr(ptr, &Dwell, &Freq, Mode, &PMinLevel, &PMaxLevel, &PacketMode,
				&RPacketMode, &Split, &Data, &WinmorMode, &Antenna, &Supress, &Filter, &Appl[0],
				&MemoryBank, &MemoryNumber, ARDOPMode, VARAMode, &BandWidth, &Power);
		}
		else
		{
			Modeptr = strchr(ptr, '/');

			if (Modeptr)
				*Modeptr++ = 0;

			Freq = atof(ptr);

			if (Modeptr)
			{
				// Mode can include 1/2/3 for Icom Filers. W/N for Winmor/Pactor Bandwidth, and +/-/S for Repeater Shift (S = Simplex) 
				// First is always Mode
				// First char is Mode (USB, LSB etc)

				Mode[0] = Modeptr[0];
				Filter = Modeptr[1];

				if (strchr(&Modeptr[1], '+'))
					Split = '+';
				else if (strchr(&Modeptr[1], '-'))
					Split = '-';
				else if (strchr(&Modeptr[1], 'S'))
					Split = 'S';
				else if (strchr(&Modeptr[1], 'D'))
					Data = 1;

				if (strchr(&Modeptr[1], 'W'))
				{
					WinmorMode = 'W';
					PMaxLevel = '3';
					PMinLevel = '1';
				}
				else if (strchr(&Modeptr[1], 'N'))
				{
					WinmorMode = 'N';
					PMaxLevel = '2';
					PMinLevel = '1';
				}

				if (strchr(&Modeptr[1], 'R'))		// Robust Packet
					RPacketMode = '2';				// R600
				else if (strchr(&Modeptr[1], 'H'))	// HF Packet on Tracker
					PacketMode = '1';				// 300

				if (strchr(&Modeptr[1], 'X'))		// Dont Report to WL2K
					Supress = 1;

				if (strstr(&Modeptr[1], "A1"))
					Antenna = '1';
				else if (strstr(&Modeptr[1], "A2"))
					Antenna = '2';
				else if (strstr(&Modeptr[1], "A3"))
					Antenna = '3';
				else if (strstr(&Modeptr[1], "A4"))
					Antenna = '4';
				else if (strstr(&Modeptr[1], "A5"))
					Antenna = '5';
				else if (strstr(&Modeptr[1], "A6"))
					Antenna = '6';
			}
		}

		switch(PORT->PortType)
		{
		case ICOM:						

			for (ModeNo = 0; ModeNo < 24; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case YAESU:						

			for (ModeNo = 0; ModeNo < 16; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (YaesuModes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(YaesuModes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case KENWOOD:

			for (ModeNo = 0; ModeNo < 8; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (KenwoodModes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case FLEX:

			for (ModeNo = 0; ModeNo < 12; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FLEXModes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FLEXModes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case FT2000:

			if (Modeptr)
			{
				if (strstr(Modeptr, "PL"))
				{
					ModeNo = 8;
					break;
				}
				if (strstr(Modeptr, "PU"))
				{
					ModeNo = 12;
					break;
				}
			}
			for (ModeNo = 0; ModeNo < 14; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FT2000Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FT2000Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;
			
		case FT991A:

/*			if (Modeptr)
			{
				if (strstr(Modeptr, "PL"))
				{
					ModeNo = 8;
					break;
				}
				if (strstr(Modeptr, "PU"))
				{
					ModeNo = 12;
					break;
				}
			}
*/	
			for (ModeNo = 0; ModeNo < 15; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FT991AModes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FT991AModes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case FTDX10:

			for (ModeNo = 0; ModeNo < 16; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FTDX10Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FTDX10Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;



		case FT100:						

			for (ModeNo = 0; ModeNo < 8; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FT100Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FT100Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case FT990:	

			for (ModeNo = 0; ModeNo < 12; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FT990Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FT990Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;

		case FT1000:						

			for (ModeNo = 0; ModeNo < 12; ModeNo++)
			{
				if (strlen(Mode) == 1)
				{
					if (FT1000Modes[ModeNo][0] == Mode[0])
						break;
				}
				else
				{
					if (_stricmp(FT1000Modes[ModeNo], Mode) == 0)
						break;
				}
			}
			break;
		}

		Freq = Freq * 1000000.0;

		sprintf(FreqString, "%09.0f", Freq);

		FreqInt = Freq;

		FreqPtr[0] = malloc(sizeof(struct ScanEntry));
		memset(FreqPtr[0], 0, sizeof(struct ScanEntry));

#pragma warning(push)
#pragma warning(disable : 4244)

		if (Dwell == 0.0)
			FreqPtr[0]->Dwell = ScanFreq * 10;
		else
			FreqPtr[0]->Dwell = Dwell * 10;

#pragma warning(pop) 

		FreqPtr[0]->Freq = Freq;
		FreqPtr[0]->Bandwidth = WinmorMode;
		FreqPtr[0]->RPacketMode = RPacketMode;
		FreqPtr[0]->HFPacketMode = PacketMode;
		FreqPtr[0]->PMaxLevel = PMaxLevel;
		FreqPtr[0]->PMinLevel = PMinLevel;
		FreqPtr[0]->Antenna = Antenna;
		strcpy(FreqPtr[0]->ARDOPMode, ARDOPMode);
		FreqPtr[0]->VARAMode = VARAMode[0];

		strcpy(FreqPtr[0]->APPL, Appl);

		ApplCall = GetApplCallFromName(Appl);

		if (strcmp(Appl, "NODE") == 0)
		{
			memcpy(FreqPtr[0]->APPLCALL, TNC->NodeCall, 9);
			strlop(FreqPtr[0]->APPLCALL, ' ');
		}
		else
		{
			if (ApplCall && ApplCall[0] > 32)
			{
				memcpy(FreqPtr[0]->APPLCALL, ApplCall, 9);
				strlop(FreqPtr[0]->APPLCALL, ' ');
			}
		}

		CmdPtr = FreqPtr[0]->Cmd1 = malloc(100);

		if (PORT->PortType == ICOM)
		{
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;

			if (RIG->ICF8101)
			{
				// Set Freq is 1A 35 and set Mode 1A 36


				*(CmdPtr++) = 0x1A;
				*(CmdPtr++) = 0x35;		// Set frequency command

				// Need to convert two chars to bcd digit

				*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
				*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
				*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
				*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
				*(CmdPtr++) = (FreqString[0] - 48);
				*(CmdPtr++) = 0xFD;
				FreqPtr[0]->Cmd1Len = 12;

				CmdPtr = FreqPtr[0]->Cmd2 = malloc(10);
				FreqPtr[0]->Cmd2Len = 9;		

				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = RIG->RigAddr;
				*(CmdPtr++) = 0xE0;
				*(CmdPtr++) = 0x1A;
				*(CmdPtr++) = 0x36;		// Set mode command
				*(CmdPtr++) = 0;
				if (ModeNo > 10)
					*(CmdPtr++) = ModeNo + 6;
				else
					*(CmdPtr++) = ModeNo;

				*(CmdPtr++) = 0xFD;
			}
			else
			{
				if (MemoryNumber)
				{
					// Set Memory Channel instead of Freq, Mode, etc

					char ChanString[5];

					// Send Set Memory, then Channel

					*(CmdPtr++) = 0x08;
					*(CmdPtr++) = 0xFD;

					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = RIG->RigAddr;
					*(CmdPtr++) = 0xE0;

					sprintf(ChanString, "%04d", MemoryNumber); 

					*(CmdPtr++) = 0x08;
					*(CmdPtr++) = (ChanString[1] - 48) | ((ChanString[0] - 48) << 4);
					*(CmdPtr++) = (ChanString[3] - 48) | ((ChanString[2] - 48) << 4);
					*(CmdPtr++) = 0xFD;

					FreqPtr[0]->Cmd1Len = 14;

					if (MemoryBank)
					{						
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x08;
						*(CmdPtr++) = 0xA0;
						*(CmdPtr++) = MemoryBank - 0x40;
						*(CmdPtr++) = 0xFD;

						FreqPtr[0]->Cmd1Len += 8;
					}
				}	
				else
				{
					*(CmdPtr++) = 0x5;		// Set frequency command

					// Need to convert two chars to bcd digit

					*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
					*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
					*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
					*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
					if (RIG->IC735)
					{
						*(CmdPtr++) = 0xFD;
						FreqPtr[0]->Cmd1Len = 10;
					}
					else
					{
						*(CmdPtr++) = (FreqString[0] - 48);
						*(CmdPtr++) = 0xFD;
						FreqPtr[0]->Cmd1Len = 11;
					}

					// Send Set VFO in case last chan was memory

					//				*(CmdPtr++) = 0xFE;
					//				*(CmdPtr++) = 0xFE;
					//				*(CmdPtr++) = RIG->RigAddr;
					//				*(CmdPtr++) = 0xE0;

					//				*(CmdPtr++) = 0x07;
					//				*(CmdPtr++) = 0xFD;

					//				FreqPtr[0]->Cmd1Len = 17;

					if (Filter)
					{						
						CmdPtr = FreqPtr[0]->Cmd2 = malloc(10);
						FreqPtr[0]->Cmd2Len = 8;		
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x6;		// Set Mode
						*(CmdPtr++) = ModeNo;
						*(CmdPtr++) = Filter - '0'; //Filter
						*(CmdPtr++) = 0xFD;

						if (Split)
						{
							CmdPtr = FreqPtr[0]->Cmd3 = malloc(10);
							FreqPtr[0]->Cmd3Len = 7;
							*(CmdPtr++) = 0xFE;
							*(CmdPtr++) = 0xFE;
							*(CmdPtr++) = RIG->RigAddr;
							*(CmdPtr++) = 0xE0;
							*(CmdPtr++) = 0xF;		// Set Mode
							if (Split == 'S')
								*(CmdPtr++) = 0x10;
							else
								if (Split == '+')
									*(CmdPtr++) = 0x12;
								else
									if (Split == '-')
										*(CmdPtr++) = 0x11;

							*(CmdPtr++) = 0xFD;
						}
						else
						{
							if (Data)
							{
								CmdPtr = FreqPtr[0]->Cmd3 = malloc(10);

								*(CmdPtr++) = 0xFE;
								*(CmdPtr++) = 0xFE;
								*(CmdPtr++) = RIG->RigAddr;
								*(CmdPtr++) = 0xE0;
								*(CmdPtr++) = 0x1a;	


								if ((strcmp(RIG->RigName, "IC7100") == 0) ||
									(strcmp(RIG->RigName, "IC7410") == 0) ||
									(strcmp(RIG->RigName, "IC7600") == 0) ||
									(strcmp(RIG->RigName, "IC7610") == 0) ||
									(strcmp(RIG->RigName, "IC7300") == 0))
								{
									FreqPtr[0]->Cmd3Len = 9;
									*(CmdPtr++) = 0x6;		// Send/read DATA mode with filter set
									*(CmdPtr++) = 0x1;		// Data On
									*(CmdPtr++) = Filter - '0'; //Filter
								}
								else if (strcmp(RIG->RigName, "IC7200") == 0)
								{
									FreqPtr[0]->Cmd3Len = 9;
									*(CmdPtr++) = 0x4;		// Send/read DATA mode with filter set
									*(CmdPtr++) = 0x1;		// Data On
									*(CmdPtr++) = Filter - '0'; //Filter
								}
								else
								{
									FreqPtr[0]->Cmd3Len = 8;
									*(CmdPtr++) = 0x6;		// Set Data
									*(CmdPtr++) = 0x1;		//On		
								}

								*(CmdPtr++) = 0xFD;
							}
						}
					}

					if (Antenna == '5' || Antenna == '6')
					{
						// Antenna select for 746 and maybe others

						// Could be going in cmd2 3 or 4

						if (FreqPtr[0]->Cmd2 == NULL)
						{
							CmdPtr = FreqPtr[0]->Cmd2 = malloc(10);
							FreqPtr[0]->Cmd2Len = 7;
						}
						else if (FreqPtr[0]->Cmd3 == NULL)
						{
							CmdPtr = FreqPtr[0]->Cmd3 = malloc(10);
							FreqPtr[0]->Cmd3Len = 7;
						}
						else 
						{
							CmdPtr = FreqPtr[0]->Cmd4 = malloc(10);
							FreqPtr[0]->Cmd4Len = 7;
						}

						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x12;		// Set Antenna
						*(CmdPtr++) = Antenna - '5';	// 0 for A5 1 for A6
						*(CmdPtr++) = 0xFD;
					}
				}
			}
		}
		else if	(PORT->PortType == YAESU)
		{	
			//Send Mode first - changing mode can change freq

			*(CmdPtr++) = ModeNo;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 7;

			*(CmdPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
			*(CmdPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
			*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
			*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
			*(CmdPtr++) = 1;

			// FT847 Needs a Poll Here. Set up anyway, but only send if 847

			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 3;


		}
		else if	(PORT->PortType == KENWOOD)
		{	
			if (Power == 0)
			{
				if (Antenna == '5' || Antenna == '6')
					FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA00%s;MD%d;AN%c;FA;MD;", FreqString, ModeNo, Antenna - 4);
				else
					FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA00%s;MD%d;FA;MD;", FreqString, ModeNo);
			}
			else
			{
				if (Antenna == '5' || Antenna == '6')
					FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA00%s;MD%d;AN%c;PC%03d;FA;MD;PC;", FreqString, ModeNo, Antenna - 4, Power);
				else
					FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA00%s;MD%d;PC%03d;FA;MD;PC;", FreqString, ModeNo, Power);
			}
		}
		else if	(PORT->PortType == FLEX)
		{	
			FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "ZZFA00%s;ZZMD%02d;ZZFA;ZZMD;", FreqString, ModeNo);
		}
		else if	(PORT->PortType == FT2000)
		{	
			FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA%s;MD0%X;FA;MD;", &FreqString[1], ModeNo);
		}
		else if	(PORT->PortType == FT991A || PORT->PortType == FTDX10)
		{	
			FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "FA%s;MD0%X;FA;MD0;", &FreqString, ModeNo);
		}
		else if	(PORT->PortType == FT100 || PORT->PortType == FT990
			|| PORT->PortType == FT1000)
		{
			// Allow Mode = "LEAVE" to suppress mode change

			//Send Mode first - changing mode can change freq

			if (strcmp(Mode, "LEAVE") == 0)
			{
				// we can't easily change the string length,
				// so just set freq twice

				*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
				*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
				*(CmdPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
				*(CmdPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
				*(CmdPtr++) = 10;
			}
			else
			{
				*(CmdPtr++) = 0;
				*(CmdPtr++) = 0;
				*(CmdPtr++) = 0;
				*(CmdPtr++) = ModeNo;
				*(CmdPtr++) = 12;
			}

			*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
			*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
			*(CmdPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
			*(CmdPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
			*(CmdPtr++) = 10;

			// Send Get Status, as these types doesn't ack commands

			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;

			if (PORT->PortType == FT990 || PORT->YaesuVariant == FT1000D)
				*(CmdPtr++) = 3;
			else
				*(CmdPtr++) = 2;		// F100 or FT1000MP

			*(CmdPtr++) = 16;			// Get Status
		}
		else if	(PORT->PortType == NMEA)
		{	
			int Len;

			i = sprintf(CmdPtr, "$PICOA,90,%02x,RXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
			AddNMEAChecksum(CmdPtr);
			Len = i;
			i = sprintf(CmdPtr + Len, "$PICOA,90,%02x,TXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
			AddNMEAChecksum(CmdPtr + Len);
			Len += i;
			i = sprintf(CmdPtr + Len, "$PICOA,90,%02x,MODE,%s*xx\r\n", RIG->RigAddr, Mode);
			AddNMEAChecksum(CmdPtr + Len);
			FreqPtr[0]->Cmd1Len = Len + i;

			i = sprintf(RIG->Poll, "$PICOA,90,%02x,RXF*xx\r\n", RIG->RigAddr);
			AddNMEAChecksum(RIG->Poll);
			Len = i;
			i = sprintf(RIG->Poll + Len, "$PICOA,90,%02x,MODE*xx\r\n", RIG->RigAddr);
			AddNMEAChecksum(RIG->Poll + Len);
			RIG->PollLen = Len + i;
		}
		else if	(PORT->PortType == HAMLIB)
		{
			FreqPtr[0]->Cmd1Len = sprintf(CmdPtr, "F %s\n+f\nM %s %d\n+m\n", FreqString, Mode, BandWidth);
		}

		else if	(PORT->PortType == FLRIG)
		{
			sprintf(FreqPtr[0]->Cmd1Msg, "%.0f", Freq);
			sprintf(FreqPtr[0]->Cmd2Msg, "%s", Mode);
			sprintf(FreqPtr[0]->Cmd3Msg, "%d", BandWidth);
		}

		else if	(PORT->PortType == RTLUDP)
		{
			int FM = 0;
			int AM = 1;
			int USB = 2;
			int LSB = 3;
		
			CmdPtr[0] = 0;
			memcpy(&CmdPtr[1], &FreqInt, 4); 

			CmdPtr[1] = FreqInt & 0xff;
			CmdPtr[2] = (FreqInt >> 8) & 0xff;
			CmdPtr[3] = (FreqInt >> 16) & 0xff;
			CmdPtr[4] = (FreqInt >> 24) & 0xff;

			FreqPtr[0]->Cmd1Len = 5;
	
			if (Mode[0])
			{
				CmdPtr[5] = 1;
				FreqPtr[0]->Cmd1Len = 10;

				if (strcmp(Mode, "FM") == 0)
					memcpy(&CmdPtr[6], &FM, 4); 
				else if (strcmp(Mode, "AM") == 0)
					memcpy(&CmdPtr[6], &AM, 4); 
				else if (strcmp(Mode, "USB") == 0)
					memcpy(&CmdPtr[6], &USB, 4); 
				else if (strcmp(Mode, "LSB") == 0)
					memcpy(&CmdPtr[6], &LSB, 4); 
			}
		
		}

		FreqPtr++;

		RIG->ScanCounter = 20;

		ptr = strtok_s(NULL, " \t\n\r", &Context);		// Next Freq
	}



	if (RIG->NumberofBands)
	{
		CheckTimeBands(RIG);		// Set initial FreqPtr;
		PORT->FreqPtr = RIG->FreqPtr[0];	
	}

	return RIG;
}

VOID SetupScanInterLockGroups(struct RIGINFO *RIG)
{
	struct PORTCONTROL * PortRecord;
	struct TNCINFO * TNC;
	int port;
	int Interlock = RIG->Interlock;
	char PortString[128] = "";
	char TxPortString[128] = "";

	// Find TNC ports in this Rig's scan group

	for (port = 1; port < MAXBPQPORTS; port++)
	{
		TNC = TNCInfo[port];

		if (TNC == NULL)
			continue;

		PortRecord = &TNC->PortRecord->PORTCONTROL;

		if (TNC->RXRadio == Interlock)
		{
			int p = PortRecord->PORTNUMBER;
			RIG->BPQPort |= ((uint64_t)1 << p);
			sprintf(PortString, "%s,%d", PortString, p);
			TNC->RIG = RIG;

			if (RIG->PTTMode == 0 && TNC->PTTMode)
				RIG->PTTMode = TNC->PTTMode;
		}
		if (TNC->TXRadio == Interlock && TNC->TXRadio != TNC->RXRadio)
		{
			int p = PortRecord->PORTNUMBER;
			RIG->BPQPort |= ((uint64_t)1 << p);
			sprintf(TxPortString, "%s,%d", TxPortString, p);
			TNC->TXRIG = RIG;

			if (RIG->PTTMode == 0 && TNC->PTTMode)
				RIG->PTTMode = TNC->PTTMode;
		}
	}

	if (RIG->PTTMode == 0 && (RIG->PTTCATPort[0] || RIG->HAMLIBPORT)) // PTT Mux Implies CAT
		RIG->PTTMode = PTTCI_V;

	if (PortString[0] && TxPortString[0])			// Have both
		sprintf(RIG->WEB_PORTS, "Rx: %s Tx: %s", &PortString[1], &TxPortString[1]);
	else if (PortString[0])
		strcpy(RIG->WEB_PORTS, &PortString[1]);
	else  if (TxPortString[0])
		sprintf(RIG->WEB_PORTS, "Tx: %s", &TxPortString[1]);

	SetWindowText(RIG->hPORTS, RIG->WEB_PORTS);
}

VOID SetupPortRIGPointers()
{
	struct TNCINFO * TNC;
	int port;

	for (port = 1; port < MAXBPQPORTS; port++)
	{
		TNC = TNCInfo[port];

		if (TNC == NULL)
			continue;

		if (TNC->RIG == NULL)
			TNC->RIG = &TNC->DummyRig;		// Not using Rig control, so use Dummy
	}
}

#ifdef WIN32

VOID PTTCATThread(struct RIGINFO *RIG)
{
	DWORD dwLength = 0;
	int Length, ret, i;
	UCHAR * ptr1;
	UCHAR * ptr2;
	UCHAR c;
	UCHAR Block[4][80];
	UCHAR CurrentState[4] = {0};
#define RTS 2
#define DTR 4
	HANDLE Event;
	HANDLE Handle[4];
	DWORD EvtMask[4];
	OVERLAPPED Overlapped[4];
	char Port[32];
	int PIndex = 0;
	int HIndex = 0;
	int rc;

	EndPTTCATThread = FALSE;

	while (PIndex < 4 && RIG->PTTCATPort[PIndex][0])
	{
		RIG->RealMux[HIndex] = 0;

		sprintf(Port, "\\\\.\\pipe\\BPQCOM%s", RIG->PTTCATPort[PIndex]);

		Handle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
                  0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

		if (Handle[HIndex] == (HANDLE) -1)
		{
			int Err = GetLastError();
			Consoleprintf("PTTMUX port BPQCOM%s Open failed code %d - trying real com port", RIG->PTTCATPort[PIndex], Err);

			// See if real com port

			sprintf(Port, "\\\\.\\\\COM%s", RIG->PTTCATPort[PIndex]);

			Handle[HIndex] = CreateFile(Port, GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

			RIG->RealMux[HIndex] = 1;

			if (Handle[HIndex] == (HANDLE) -1)
			{
				int Err = GetLastError();
				Consoleprintf("PTTMUX port COM%s Open failed code %d", RIG->PTTCATPort[PIndex], Err);
			}
			else
			{
				rc = SetCommMask(Handle[HIndex], EV_CTS | EV_DSR);		// Request notifications
				HIndex++;
			}
		}
		else
			HIndex++;

		PIndex++;

	}

	if (PIndex == 0)
		return;				// No ports

	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (i = 0; i < HIndex; i ++)
	{
		memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
		Overlapped[i].hEvent = Event;

		if (RIG->RealMux[i])
		{
			// Request Interface change notifications

			rc = WaitCommEvent(Handle[i], &EvtMask[i], &Overlapped[i]);
			rc = GetLastError();
 
		}
		else
		{

			// Prime a read on each handle

			ReadFile(Handle[i], Block[i], 80, &Length, &Overlapped[i]);
		}
	}
		
	while (EndPTTCATThread == FALSE)
	{

WaitAgain:

		ret = WaitForSingleObject(Event, 1000);

		if (ret == WAIT_TIMEOUT)
		{
			if (EndPTTCATThread)
			{
				for (i = 0; i < HIndex; i ++)
				{
					CancelIo(Handle[i]);
					CloseHandle(Handle[i]);
					Handle[i] = INVALID_HANDLE_VALUE;
				}
				CloseHandle(Event);
				return;
			}
			goto WaitAgain;
		}

		ResetEvent(Event);

		// See which request(s) have completed

		for (i = 0; i < HIndex; i ++)
		{
			ret =  GetOverlappedResult(Handle[i], &Overlapped[i], &Length, FALSE);

			if (ret)
			{
				if (RIG->RealMux[i])
				{
					// Request Interface change notifications

					DWORD Mask;

					GetCommModemStatus(Handle[i], &Mask);

					if (Mask & MS_CTS_ON)
						Rig_PTTEx(RIG, TRUE, RIG->PTTCATTNC[i]);
					else
						Rig_PTTEx(RIG, FALSE, RIG->PTTCATTNC[i]);

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;
					WaitCommEvent(Handle[i], &EvtMask[i], &Overlapped[i]);

				}
				else
				{

					ptr1 = Block[i];
					ptr2 = Block[i];

					while (Length > 0)
					{
						c = *(ptr1++);

						Length--;

						if (c == 0xff)
						{
							c = *(ptr1++);
							Length--;

							if (c == 0xff)			// ff ff means ff
							{
								Length--;
							}
							else
							{
								// This is connection / RTS/DTR statua from other end
								// Convert to CAT Command

								if (c == CurrentState[i])
									continue;

								if (c & RTS)
									Rig_PTTEx(RIG, TRUE, RIG->PTTCATTNC[i]);
								else
									Rig_PTTEx(RIG, FALSE, RIG->PTTCATTNC[i]);

								CurrentState[i] = c;
								continue;
							}
						}
					}

					memset(&Overlapped[i], 0, sizeof(OVERLAPPED));
					Overlapped[i].hEvent = Event;

					ReadFile(Handle[i], Block[i], 80, &Length, &Overlapped[i]);
				}
			}
		}
	}
	EndPTTCATThread = FALSE;
}

/*
		memset(&Overlapped, 0, sizeof(Overlapped));
		Overlapped.hEvent = Event;
		ResetEvent(Event);

		ret = ReadFile(Handle, Block, 80, &Length, &Overlapped);
		
		if (ret == 0)
		{
			ret = GetLastError();

			if (ret != ERROR_IO_PENDING)
			{
				if (ret == ERROR_BROKEN_PIPE || ret == ERROR_INVALID_HANDLE)
				{
					CloseHandle(Handle);
					RIG->PTTCATHandles[0] = INVALID_HANDLE_VALUE;
					return;
				}
			}
		}	
*/
#endif

// HAMLIB Support Code

VOID HAMLIBPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on HAMLIB
	char cmd[80];
	int len;

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");

		RIG->RIGOK = FALSE;
		return;
	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				send(PORT->remoteSock, PORT->TXBuffer, PORT->TXLen, 0);
				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission
				return;
			}
		}
	}
			
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 10;			// Once Per Sec
		
	// Read Frequency 

	len = sprintf(cmd, "+f\n+m\n");

	send(PORT->remoteSock, cmd, len, 0);

	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}


void HAMLIBProcessMessage(struct RIGPORTINFO * PORT)
{
	// Called from Background thread

	int InputLen = recv(PORT->remoteSock, &PORT->RXBuffer[PORT->RXLen], 500 - PORT->RXLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;

		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;
		return;					
	}

	PORT->RXLen += InputLen;
}

void FLRIGProcessMessage(struct RIGPORTINFO * PORT)
{
	// Called from Background thread

	int InputLen = recv(PORT->remoteSock, &PORT->RXBuffer[PORT->RXLen], 500 - PORT->RXLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;

		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;
		return;					
	}

	PORT->RXLen += InputLen;
	ProcessFLRIGFrame(PORT);
}

void ProcessHAMLIBFrame(struct RIGPORTINFO * PORT, int Length)
{
	char * msg = PORT->RXBuffer;
	char * rest;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	msg[Length] = 0;

	PORT->Timeout = 0;
	RIG->RIGOK = 1;

	// extract lines from input

	while (msg && msg[0])
	{
		rest = strlop(msg, 10);

		if (memcmp(msg, "Frequency:", 10) == 0)
		{
			RIG->RigFreq = atof(&msg[11]) / 1000000.0;

			_gcvt(RIG->RigFreq, 9, RIG->Valchar);
 
			sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
			SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		}

		else if (memcmp(msg, "Mode:", 5) == 0)
		{
			if (strlen(&msg[6]) < 15)
				strcpy(RIG->ModeString, &msg[6]);
		}

		else if (memcmp(msg, "Passband:", 9) == 0)
		{
			RIG->Passband = atoi(&msg[10]);
			sprintf(RIG->WEB_MODE, "%s/%d", RIG->ModeString, RIG->Passband);
			SetWindowText(RIG->hMODE, RIG->WEB_MODE);
		}
		
		msg = rest;
	}
}


void ProcessFLRIGFrame(struct RIGPORTINFO * PORT)
{
	char * msg;
	int Length;

	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one 
	char * ptr1, * ptr2, * val;
	int Len, TotalLen;
	char cmd[80];
	char ReqBuf[256];
	char SendBuff[256];

	while (PORT->RXLen > 0)
	{
		int b1 = 0, b2 = 0;

		msg = PORT->RXBuffer;
		Length = PORT->RXLen;

		msg[Length] = 0;

		ptr1 = strstr(msg, "Content-length:");

		if (ptr1 == NULL)
			return;

		ptr2 = strstr(ptr1, "\r\n\r\n");

		if (ptr2 == NULL)
			return;

		Len = atoi(&ptr1[15]);

		TotalLen = ptr2 + 4 + Len - msg;

		if (TotalLen > Length)		// Don't have it all
			return;

		val = strstr(ptr2, "<value>");

		if (val)
		{
			val += 7;

			RIG->RIGOK = 1;
			PORT->RXLen -= TotalLen;

			// It is quite difficult to corrolate responses with commands, but we only poll for freq, mode and bandwidth
			// and the responses can be easily identified

			if (strstr(val, "<i4>") || memcmp(val, "</value>", 8) == 0)
			{
				// Reply to set command - may need to send next in set or use to send OK if interactive

				if (PORT->ScanEntry.Cmd2Msg[0])
				{
					sprintf(cmd, "<string>%s</string>", PORT->ScanEntry.Cmd2Msg);
					FLRIGSendCommand(PORT, "rig.set_mode", cmd);
					PORT->ScanEntry.Cmd2Msg[0] = 0;
				}

				else if (PORT->ScanEntry.Cmd3Msg[0] && strcmp(PORT->ScanEntry.Cmd3Msg, "0") != 0)
				{
					sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd3Msg);
					FLRIGSendCommand(PORT, "rig.set_bandwidth", cmd);
					PORT->ScanEntry.Cmd3Msg[0] = 0;
				}

				else if (!PORT->AutoPoll)
				{
					GetSemaphore(&Semaphore, 61);
					SendResponse(RIG->Session, "Set OK");
					FreeSemaphore(&Semaphore);
					PORT->AutoPoll = 1;		// So we dond send another
				}
			}
			else if(strstr(val, "<array>"))
			{
				// Reply to get BW

				char * p1, * p2;

				p1 = strstr(val, "<value>");

				if (p1)
				{
					p1 += 7;
					b1 = atoi(p1);

					p2 = strstr(p1, "<value>");

					if (p2)
					{
						p2 +=7;
						b2 = atoi(p2);
					}
				}

				if (b1)
				{
					if (b2)
						sprintf(RIG->WEB_MODE, "%s/%d:%d", RIG->ModeString, b1, b2);
					else
						sprintf(RIG->WEB_MODE, "%s/%d", RIG->ModeString, b1);

					MySetWindowText(RIG->hMODE, RIG->WEB_MODE);
				}
			}
			else
			{
				// Either freq or mode. See if numeric

				double freq;

				strlop(val, '<');

				freq = atof(val) / 1000000.0;

				if (freq > 0.0)
				{
					RIG->RigFreq = freq; 
					_gcvt(RIG->RigFreq, 9, RIG->Valchar);

					sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
					MySetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

					// Read Mode

					Len = sprintf(ReqBuf, Req, "rig.get_mode", "");
					Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
					send(PORT->remoteSock, SendBuff, Len, 0);

				}
				else
				{
					if (strlen(val)> 1)
					{
						strcpy(RIG->ModeString, val);

						if (b1)
						{
							if (b2)
								sprintf(RIG->WEB_MODE, "%s/%d:%d", RIG->ModeString, b1, b2);
							else
								sprintf(RIG->WEB_MODE, "%s/%d", RIG->ModeString, b1);
						}
						else
							SetWindowText(RIG->hMODE, RIG->WEB_MODE);

						MySetWindowText(RIG->hMODE, RIG->WEB_MODE);

						Len = sprintf(ReqBuf, Req, "rig.get_bw", "");
						Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

						send(PORT->remoteSock, SendBuff, Len, 0);
					}
				}
			}



			// We now send all setting commands at once

			/*


			if (memcmp(PORT->TXBuffer, "rig.set_vfo", 11) == 0)
			{
			//	Set Freq - Send Set Mode if needed

			if (PORT->ScanEntry.Cmd2Msg[0])
			{
			sprintf(cmd, "<string>%s</string>", PORT->ScanEntry.Cmd2Msg);
			FLRIGSendCommand(PORT, "rig.set_mode", cmd);
			if (strcmp(PORT->ScanEntry.Cmd3Msg, "0") != 0)
			{
			sprintf(cmd, "<i4>%s</i4>", PORT->ScanEntry.Cmd3Msg);
			FLRIGSendCommand(PORT, "rig.set_bandwidth", cmd);
			}
			strcpy(RIG->ModeString, PORT->ScanEntry.Cmd2Msg);
			}
			else
			{
			if (!PORT->AutoPoll)
			SendResponse(RIG->Session, "Set Freq OK");

			strcpy(PORT->TXBuffer, "rig.get_vfo"); 
			Len = sprintf(ReqBuf, Req, "rig.get_vfo", "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
			strcpy(PORT->TXBuffer, "rig.get_bw"); 
			Len = sprintf(ReqBuf, Req, "rig.get_bw", "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

			send(PORT->remoteSock, SendBuff, Len, 0);
			}
			continue;
			}

			if (memcmp(PORT->TXBuffer, "rig.set_mode", 11) == 0)
			{
			strcpy(PORT->TXBuffer, "rig.get_vfo"); 		
			Len = sprintf(ReqBuf, Req, "rig.get_vfo", "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

			send(PORT->remoteSock, SendBuff, Len, 0);

			if (!PORT->AutoPoll)
			SendResponse(RIG->Session, "Set Freq and Mode OK");

			continue;
			}

			if (memcmp(PORT->TXBuffer, "rig.get_vfo", 11) == 0)
			{
			RIG->RigFreq = atof(val) / 1000000.0;

			_gcvt(RIG->RigFreq, 9, RIG->Valchar);

			sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
			MySetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

			strcpy(PORT->TXBuffer, "rig.get_mode"); 
			Len = sprintf(ReqBuf, Req, "rig.get_mode", "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
			send(PORT->remoteSock, SendBuff, Len, 0);

			Len = sprintf(ReqBuf, Req, "rig.get_bw", "");
			Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
			send(PORT->remoteSock, SendBuff, Len, 0);
			continue;
			}

			if (memcmp(PORT->TXBuffer, "rig.get_mode", 11) == 0)
			{
			strlop(val, '<');
			sprintf(RIG->WEB_MODE, "%s", val);
			MySetWindowText(RIG->hMODE, RIG->WEB_MODE);
			}
			}

			*/
		}

		if (PORT->RXLen > 0)
			memmove(PORT->RXBuffer, &PORT->RXBuffer[TotalLen], PORT->RXLen);
		else
			PORT->RXLen = 0;

		PORT->Timeout = 0;
	}

	/*
	POST /RPC2 HTTP/1.1
	User-Agent: XMLRPC++ 0.8
	Host: 127.0.0.1:12345
	Content-type: text/xml
	Content-length: 89

	<?xml version="1.0"?>
	<methodCall><methodName>rig.get_vfoA</methodName>
	</methodCall>
	HTTP/1.1 200 OK
	Server: XMLRPC++ 0.8
	Content-Type: text/xml
	Content-length: 118

	<?xml version="1.0"?>
	<methodResponse><params><param>
	<value>14070000</value>
	</param></params></methodResponse>
	*/


}





void HLSetMode(SOCKET Sock, struct RIGINFO * RIG, unsigned char * Msg, char sep)
{
	char Resp[80];
	int Len;
	char mode[80] = "";
	int filter = 0;
	int n = sscanf(&Msg[1], "%s %d", mode, &filter); 

	// Send to RIGCommand. Can't set Mode without Freq so need to use current

	// ?? Should be try to convert bandwidth to filter ??

	RIG->Passband = filter;

	if (RIG->PORT->PortType == ICOM)		// Needs a Filter
		sprintf(Resp, "%d %s %s 1\n", 0, RIG->Valchar, mode);
	else
		sprintf(Resp, "%d %s %s\n", 0, RIG->Valchar, mode);

	GetSemaphore(&Semaphore, 60);
	Rig_CommandEx(RIG->PORT, RIG, (TRANSPORTENTRY *) -1, Resp);
	FreeSemaphore(&Semaphore);

	if (sep)
		Len = sprintf(Resp, "set_mode: %s %d%cRPRT 0\n", mode, filter, sep);
	else
		Len = sprintf(Resp, "RPRT 0\n");

	send(Sock, Resp, Len, 0);
}


void HLSetFreq(SOCKET Sock, struct RIGINFO * RIG, unsigned char * Msg, char sep)
{
	char Resp[80];
	int Len;
	int freq = atoi(&Msg[1]);

	// Send to RIGCommand

	sprintf(Resp, "%d %f\n", 0, freq/1000000.0);
	GetSemaphore(&Semaphore, 60);
	Rig_CommandEx(RIG->PORT, RIG, (TRANSPORTENTRY *) -1, Resp);
	FreeSemaphore(&Semaphore);

	if (sep)
		Len = sprintf(Resp, "set_freq: %d%cRPRT 0\n", freq, sep);
	else
		Len = sprintf(Resp, "RPRT 0\n");

	send(Sock, Resp, Len, 0);
}


void HLGetPTT(SOCKET Sock, struct RIGINFO * RIG, char sep)
{
	char Resp[80];
	int Len;
	int ptt = 0;
	
	if (RIG->PTTTimer)
		ptt = 1;

	if (sep)
		Len = sprintf(Resp, "get_ptt:%cPTT: %d%cRPRT 0\n", sep, ptt, sep);
	else
		Len = sprintf(Resp, "%d\n", ptt);

	send(Sock, Resp, Len, 0);
}

void HLSetPTT(SOCKET Sock, struct RIGINFO * RIG, unsigned char * Msg, char sep)
{
	char Resp[80];
	int Len;
	int ptt = atoi(&Msg[1]);

	if (ptt)
		Rig_PTTEx(RIG, 1, NULL);
	else
		Rig_PTTEx(RIG, 0, NULL);

	if (sep)
		Len = sprintf(Resp, "set_ptt: %d%cRPRT 0\n", ptt, sep);
	else
		Len = sprintf(Resp, "RPRT 0\n");

	send(Sock, Resp, Len, 0);
}

void HLGetMode(SOCKET Sock, struct RIGINFO * RIG, char sep)
{
	char Resp[80];
	int Len;

	if (sep)
		Len = sprintf(Resp, "get_mode:%cMode: %s%cPassband: %d%cRPRT 0\n", sep, RIG->ModeString, sep, RIG->Passband, sep);
	else
		Len = sprintf(Resp, "%s\n%d\n", RIG->ModeString, RIG->Passband);

	send(Sock, Resp, Len, 0);

}

void HLGetFreq(SOCKET Sock, struct RIGINFO * RIG, char sep)
{
	char Resp[80];
	int Len;
	char freqval[64];
	double freq = atof(RIG->Valchar) * 1000000.0;

	sprintf(freqval, "%f", freq);
	strlop(freqval, '.');

	if (sep)
		Len = sprintf(Resp, "get_freq:%cFrequency: %s%cRPRT 0\n", sep, freqval, sep);
	else
		Len = sprintf(Resp, "%s\n", freqval);

	send(Sock, Resp, Len, 0);
}

void HLGetVFO(SOCKET Sock, struct RIGINFO * RIG, char sep)
{
	char Resp[80];
	int Len;

	if (sep)
		Len = sprintf(Resp, "get_vfo:%s%cRPRT 0\n", "VFOA", sep);
	else
		Len = sprintf(Resp, "%s\n", "VFOA");

	send(Sock, Resp, Len, 0);
}

void HLGetSplit(SOCKET Sock, struct RIGINFO * RIG, char sep)
{
	char Resp[80];
	int Len;

	if (sep)
		Len = sprintf(Resp, "get_vfo:%s%cRPRT 0\n", "VFOA", sep);
	else
		Len = sprintf(Resp, "0\n%s\n", "VFOA");

	send(Sock, Resp, Len, 0);

}



int ProcessHAMLIBSlaveMessage(SOCKET Sock, struct RIGINFO * RIG, unsigned char * Msg, int MsgLen)
{
	// We only process a pretty small subset of rigctl messages

	// commands are generally a single character, upper case for set
	// and lower case for get. If preceeded by + ; | or , response will
	// be in extended form. + adds an LF between field, other values the
	// supplied char is used as seperator.

	// At the moments we support freq (F) mode (m) and PTT (T)

	char sep = 0;

	if (Msg[0] == '#')
		return 0;				// Comment

	strlop(Msg, 13);
	strlop(Msg, 10);

	// \ on front is used for long mode. Hopefully not used much

	// WSJT seems ro need \chk_vfo and \dump_state

	if (Msg[0] == '\\')
	{
		if (strcmp(&Msg[1], "chk_vfo") == 0)
		{
			char Reply[80];
			int Len = sprintf(Reply, "CHKVFO 0\n");
			send(Sock, Reply, Len, 0);
			return 0;
		}

		if (strcmp(&Msg[1], "dump_state") == 0)
		{
			char Reply[4096];
			int Len = sprintf(Reply,
				"0\n"
				"1\n"
				"2\n"
				"150000.000000 1500000000.000000 0x1ff -1 -1 0x10000003 0x3\n"
				"0 0 0 0 0 0 0\n"
				"0 0 0 0 0 0 0\n"
				"0x1ff 1\n"
				"0x1ff 0\n"
				"0 0\n"
				"0x1e 2400\n"
				"0x2 500\n"
				"0x1 8000\n"
				"0x1 2400\n"
				"0x20 15000\n"
				"0x20 8000\n"
				"0x40 230000\n"
				"0 0\n"
				"9990\n"
				"9990\n"
				"10000\n"
				"0\n"
				"10 \n"
				"10 20 30 \n"
				"0xffffffff\n"
				"0xffffffff\n"
				"0xf7ffffff\n"
				"0x83ffffff\n"
				"0xffffffff\n"
				"0xffffffbf\n");

			send(Sock, Reply, Len, 0);
			return 0;
		}
	}

	if (Msg[0] == 'q' || Msg[0] == 'Q')
	{
		// close connection

		return 1;
	}

	if (Msg[0] == '+')
	{
		sep = 10;
		Msg++;
		MsgLen --;
	}
	else if (Msg[0] == '_' || Msg[0] == '?')
	{
	}
	else if (ispunct(Msg[0]))
	{
		sep = Msg[0];	
		Msg++;
		MsgLen --;
	}

	switch (Msg[0])
	{
	case 'f':			// Get Freqency

		HLGetFreq(Sock, RIG, sep);
		return 0;

	case 'm':			// Get Mode

		HLGetMode(Sock, RIG, sep);
		return 0;

	case 't':			// Get PTT

		HLGetPTT(Sock, RIG, sep);
		return 0;

	case 'v':			// Get VFO

		HLGetVFO(Sock, RIG, sep);
		return 0;

	case 's':			// Get Split

		HLGetSplit(Sock, RIG, sep);
		return 0;

	case 'F':

		HLSetFreq(Sock, RIG, Msg, sep);
		return 0;

	case 'M':

		HLSetMode(Sock, RIG, Msg, sep);
		return 0;

	case 'T':

		HLSetPTT(Sock, RIG, Msg, sep);
		return 0;
	}
	return 0;
}

int DecodeHAMLIBAddr(struct RIGPORTINFO * PORT, char * ptr)
{
	// Param is IPADDR:PORT. Only Allow numeric addresses 
	
	struct sockaddr_in * destaddr = (SOCKADDR_IN *)&PORT->remoteDest;

	char * port = strlop(ptr, ':');

	if (port == NULL)
		return 0;

	destaddr->sin_family = AF_INET;
	destaddr->sin_addr.s_addr = inet_addr(ptr);
	destaddr->sin_port = htons(atoi(port));

	return 1;
}

VOID HAMLIBThread(struct RIGPORTINFO * PORT);

VOID ConnecttoHAMLIB(struct RIGPORTINFO * PORT)
{
	if (HAMLIBMasterRunning)
		_beginthread(HAMLIBThread, 0, (void *)PORT);

	return ;
}

VOID HAMLIBThread(struct RIGPORTINFO * PORT)
{
	// Opens sockets and looks for data
	
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;

	if (PORT->remoteSock)
	{
		closesocket(PORT->remoteSock);
	}

	PORT->remoteSock = 0;
	PORT->remoteSock = socket(AF_INET,SOCK_STREAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for HAMLIB socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	PORT->CONNECTING = FALSE;
  	 	return; 
	}

	setsockopt(PORT->remoteSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(PORT->remoteSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	if (connect(PORT->remoteSock,(LPSOCKADDR) &PORT->remoteDest,sizeof(PORT->remoteDest)) == 0)
	{
		//
		//	Connected successful
		//

		ioctl(PORT->remoteSock, FIONBIO, &param);
	}
	else
	{
		if (PORT->Alerted == FALSE)
		{
			struct sockaddr_in * destaddr = (SOCKADDR_IN * )&PORT->remoteDest;

			err = WSAGetLastError();

   			sprintf(Msg, "Connect Failed for HAMLIB socket - error code = %d Addr %s\r\n", err, PORT->IOBASE);

			WritetoConsole(Msg);
				PORT->Alerted = TRUE;
		}
		
		closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
	 	PORT->CONNECTING = FALSE;
		return;
	}

	PORT->CONNECTED = TRUE;
	PORT->hDevice = (HANDLE)1;				// simplifies check code

	PORT->Alerted = TRUE;

	while (PORT->CONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(PORT->remoteSock,&readfs);
		FD_SET(PORT->remoteSock,&errorfs);
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		ret = select((int)PORT->remoteSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (HAMLIBMasterRunning == 0)
			return;

		if (ret == SOCKET_ERROR)
		{
			Debugprintf("HAMLIB Select failed %d ", WSAGetLastError());
			goto Lost;
		}

		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(PORT->remoteSock, &readfs))
			{
				HAMLIBProcessMessage(PORT);
			}

			if (FD_ISSET(PORT->remoteSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "HAMLIB Connection lost for Addr %s\r\n", PORT->IOBASE);
				WritetoConsole(Msg);

				PORT->CONNECTED = FALSE;
				PORT->Alerted = FALSE;
				PORT->hDevice = 0;				// simplifies check code

				closesocket(PORT->remoteSock);
				PORT->remoteSock = 0;
				return;
			}


			continue;
		}
		else
		{
		}
	}
	sprintf(Msg, "HAMLIB Thread Terminated Addr %s\r\n", PORT->IOBASE);
	WritetoConsole(Msg);
}



void HAMLIBSlaveThread(struct RIGINFO * RIG)
{
	// Wait for connections and messages from HAMLIB Clients

	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	int ret;
	unsigned int maxsock;

	HAMLIBSlaveRunning = 1;

	Consoleprintf("HAMLIB Slave Thread %d Running", RIG->HAMLIBPORT);

	while (HAMLIBSlaveRunning)
	{
		struct HAMLIBSOCK * Entry = RIG->Sockets;
		struct HAMLIBSOCK * Prev;
	
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(RIG->ListenSocket, &readfs);
		FD_SET(RIG->ListenSocket, &errorfs);
	
		maxsock = RIG->ListenSocket;

		while (Entry && HAMLIBSlaveRunning)
		{
			FD_SET(Entry->Sock, &readfs);
			FD_SET(Entry->Sock, &errorfs);

			if (Entry->Sock > maxsock)
				maxsock = Entry->Sock;

			Entry = Entry->Next;
		}

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		ret = select(maxsock + 1, &readfs, NULL, &errorfs, &timeout);

		if (HAMLIBSlaveRunning == 0)
			return;

		if (ret == -1)
		{
			perror("listen select");
			continue;
		}

		if (ret)
		{
			if (FD_ISSET(RIG->ListenSocket, &readfs))
			{
				// Connection. Accept it and create a socket enty

				int addrlen = sizeof(struct sockaddr_in6);
				struct sockaddr_in6 sin6; 
				struct HAMLIBSOCK * Entry = zalloc(sizeof(struct HAMLIBSOCK));

				Entry->Sock = accept(RIG->ListenSocket, (struct sockaddr *)&sin6, &addrlen);
				
				if (RIG->Sockets)
					Entry->Next = RIG->Sockets;

				RIG->Sockets = Entry;
			}

			// See if any Data Sockets

			Entry = RIG->Sockets;
			Prev = 0;

			while (Entry)
			{
				unsigned char MsgBuf[256];
				unsigned char * Msg = MsgBuf;
				int MsgLen;

				if (FD_ISSET(Entry->Sock, &readfs))
				{
					MsgLen = recv(Entry->Sock, Msg, 256, 0);

					if (MsgLen <= 0)
					{
						// Closed - close socket and remove from chain

						closesocket(Entry->Sock);

						if (Prev == 0)
							RIG->Sockets = Entry->Next;
						else
							Prev->Next = Entry->Next;

						free (Entry);
						break;
					}
					else
					{
						// Could have multiple messages in packet
						// Terminator can be CR LF or CRLF

						char * ptr;
						int Len;

						Msg[MsgLen] = 0;
Loop:
						ptr = strlop(Msg, 10);
						if (ptr == NULL)
							strlop(Msg, 13);

						Len = strlen(Msg);

						if (ProcessHAMLIBSlaveMessage(Entry->Sock, RIG, Msg, Len) == 1)
						{
							// close request

							closesocket(Entry->Sock);

							if (Prev == 0)
								RIG->Sockets = Entry->Next;
							else
								Prev->Next = Entry->Next;

							free (Entry);
							break;
						}
						Msg = ptr;

						if (Msg)
						{
							while (Msg[0] == 10 || Msg[0] == 13)
								Msg++;

							if (Msg[0])
								goto Loop;
						}
					}
				}

				if (FD_ISSET(Entry->Sock, &errorfs))
				{
					// Closed - close socket and remove from chai

					closesocket(Entry->Sock);

					if (Prev == 0)
						RIG->Sockets = Entry->Next;
					else
						Prev->Next = Entry->Next;

					free (Entry);
					break;
				}

				// Check Next Client

				Prev = Entry;
				Entry = Entry->Next;
			}
		}
	}
	Consoleprintf("HAMLIB Slave Thread %d Exited", RIG->HAMLIBPORT);
}


VOID FLRIGPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];
	int Len;
	char ReqBuf[256];
	char SendBuff[256];

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");

		RIG->RIGOK = FALSE;
		return;
	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				char cmd[80];
				double freq;

				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				// Send the Set Freq here, send set mode when we get a response
				
				memcpy(&PORT->ScanEntry, PORT->FreqPtr, sizeof(struct ScanEntry));

				sprintf(cmd, "<double>%s</double>", PORT->FreqPtr->Cmd1Msg);
				FLRIGSendCommand(PORT, "rig.set_vfo", cmd);

				// Update display as we don't get a response

				freq = atof(PORT->FreqPtr->Cmd1Msg) / 1000000.0;

				if (freq > 0.0)
				{
					RIG->RigFreq = freq; 
					_gcvt(RIG->RigFreq, 9, RIG->Valchar);

					sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
					MySetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
				}


				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 10;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission
				return;
			}
		}
	}
			
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 10;			// Once Per Sec
		
	// Read Frequency 

	strcpy(Poll, "rig.get_vfo");

	Len = sprintf(ReqBuf, Req, Poll, "");
	Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

	if (PORT->CONNECTED)
	{
		if (send(PORT->remoteSock, SendBuff, Len, 0) != Len)
		{
			if (PORT->remoteSock)
				closesocket(PORT->remoteSock);

			PORT->remoteSock = 0;
			PORT->CONNECTED = FALSE;
			PORT->hDevice = 0;	
			return;
		}
	}

	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID FLRIGSendCommand(struct RIGPORTINFO * PORT, char * Command, char * Value)
{
	int Len;
	char ReqBuf[512];
	char SendBuff[512];
	char ValueString[256] ="";

	if (!PORT->CONNECTED)
		return;
	
	sprintf(ValueString, "<params><param><value>%s</value></param></params\r\n>", Value);
	
	strcpy(PORT->TXBuffer, Command);
	Len = sprintf(ReqBuf, Req, PORT->TXBuffer, ValueString);
	Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
	if (send(PORT->remoteSock, SendBuff, Len, 0) != Len)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;				
	}

	return;
}



VOID FLRIGThread(struct RIGPORTINFO * PORT);

VOID ConnecttoFLRIG(struct RIGPORTINFO * PORT)
{
	if (FLRIGRunning)
		_beginthread(FLRIGThread, 0, (void *)PORT);
	return ;
}

VOID FLRIGThread(struct RIGPORTINFO * PORT)
{
	// Opens sockets and looks for data
	
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;

	if (PORT->remoteSock)
	{
		closesocket(PORT->remoteSock);
	}

	PORT->remoteSock = 0;
	PORT->remoteSock = socket(AF_INET,SOCK_STREAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for FLRIG socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	PORT->CONNECTING = FALSE;
  	 	return; 
	}

	setsockopt(PORT->remoteSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(PORT->remoteSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4); 

	if (connect(PORT->remoteSock,(LPSOCKADDR) &PORT->remoteDest,sizeof(PORT->remoteDest)) == 0)
	{
		//
		//	Connected successful
		//

		ioctl(PORT->remoteSock, FIONBIO, &param);
	}
	else
	{
		if (PORT->Alerted == FALSE)
		{
			struct sockaddr_in * destaddr = (SOCKADDR_IN * )&PORT->remoteDest;

			err = WSAGetLastError();

   			sprintf(Msg, "Connect Failed for FLRIG socket - error code = %d Port %d\r\n",
				err, htons(destaddr->sin_port));

			WritetoConsole(Msg);
				PORT->Alerted = TRUE;
		}
		
		closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
	 	PORT->CONNECTING = FALSE;
		return;
	}

	PORT->CONNECTED = TRUE;
	PORT->hDevice = (HANDLE)1;				// simplifies check code

	PORT->Alerted = TRUE;

	while (PORT->CONNECTED && FLRIGRunning)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(PORT->remoteSock,&readfs);
		FD_SET(PORT->remoteSock,&errorfs);
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		ret = select((int)PORT->remoteSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (FLRIGRunning == 0)
			return;

		if (ret == SOCKET_ERROR)
		{
			Debugprintf("FLRIG Select failed %d ", WSAGetLastError());
			goto Lost;
		}

		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(PORT->remoteSock, &readfs))
			{
				FLRIGProcessMessage(PORT);
			}

			if (FD_ISSET(PORT->remoteSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "FLRIG Connection lost for Port %s\r\n", PORT->IOBASE);
				WritetoConsole(Msg);

				PORT->CONNECTED = FALSE;
				PORT->Alerted = FALSE;
				PORT->hDevice = 0;				// simplifies check code

				closesocket(PORT->remoteSock);
				PORT->remoteSock = 0;
				return;
			}
			continue;
		}
		else
		{
		}
	}
	sprintf(Msg, "FLRIG Thread Terminated Port %s\r\n", PORT->IOBASE);
	WritetoConsole(Msg);
}




// HID Support Code

int HID_Read_Block(struct RIGPORTINFO * PORT)
{
	int Len;
	unsigned char Msg[65] = "";

	if (PORT->RXLen > 400)
		PORT->RXLen = 0;

	// Don't try to read more than 64

#ifdef WIN32
	Len = rawhid_recv(0, Msg, 64, 100);
#else
	Len = read(PORT->hDevice, Msg, 64);
#endif

	if (Len <= 0)
		return 0;

	// First byte is actual length

	Len = Msg[0];

	if (Len > 0)
	{
		if (Len < 64)		// Max in HID Packet
		{
			memcpy(&PORT->RXBuffer[PORT->RXLen], Msg + 1, Len);
			return Len;
		}
	}
	return 0;
}

void rawhid_close(int num);

BOOL HID_Write_Block(struct RIGPORTINFO * PORT)
{
	int n = PORT->TXLen;
	UCHAR * ptr = PORT->TXBuffer;
	UCHAR Msg[64] = "";
	int ret, i;

	while (n)
	{
		i = n;
		if (i > 63)
			i = 63;

		Msg[0] = i;		// Length on front
		memcpy(&Msg[1], ptr, i);
		ptr += i;
		n -= i;
		//	n = hid_write(PORT->hDevice, PORT->TXBuffer, PORT->TXLen);
#ifdef WIN32
		ret = rawhid_send(0, Msg, 64, 100);		// Always send 64

		if (ret < 0)
		{
			Debugprintf("Rigcontrol HID Write Failed %d", errno);
			rawhid_close(0);
			PORT->hDevice = NULL;
			return FALSE;
		}
#else
		ret = write(PORT->hDevice, Msg, 64);

		if (ret != 64)
		{
			printf ("Write to %s failed, n=%d, errno=%d\n", PORT->HIDDevice, ret, errno);
			close (PORT->hDevice);
			PORT->hDevice = 0;
			return FALSE;
		}

//		printf("HID Write %d\n", i);
#endif
	}
	return TRUE;
}


BOOL OpenHIDPort(struct RIGPORTINFO * PORT, VOID * Port, int Speed)
{
#ifdef WIN32

	if (PORT->HIDDevice== NULL)
		return FALSE;

	PORT->hDevice = rawhid_open(PORT->HIDDevice);

	if (PORT->hDevice)
		Debugprintf("Rigcontrol HID Device %s opened", PORT->HIDDevice);

	/*
	handle = hid_open_path(PORT->HIDDevice);

	if (handle)
	hid_set_nonblocking(handle, 1);

	PORT->hDevice = handle;
	*/
#else
	int fd;
	unsigned int param = 1;

	if (PORT->HIDDevice== NULL)
		return FALSE;

	fd = open (PORT->HIDDevice, O_RDWR);
		
	if (fd == -1)
	{
          printf ("Could not open %s, errno=%d\n", PORT->HIDDevice, errno);
          return FALSE;
	}

	ioctl(fd, FIONBIO, &param);
	printf("Rigcontrol HID Device %s opened", PORT->HIDDevice);
	
	PORT->hDevice = fd;
#endif
	if (PORT->hDevice == 0)
		return (FALSE);

	return TRUE;
}


void CM108_set_ptt(struct RIGINFO *RIG, int PTTState)
{
	char io[5];
	hid_device *handle;
	int n;

	io[0] = 0;
	io[1] = 0;
	io[2] = 1 << (3 - 1);
	io[3] = PTTState << (3 - 1);
	io[4] = 0;

	if (RIG->CM108Device == NULL)
		return;

#ifdef WIN32
	handle = hid_open_path(RIG->CM108Device);

	if (!handle) {
		printf("unable to open device\n");
 		return;
	}

	n = hid_write(handle, io, 5);
	if (n < 0)
	{
		Debugprintf("Unable to write()\n");
		Debugprintf("Error: %ls\n", hid_error(RIG->PORT->hDevice));
	}
	
	hid_close(handle);

#else

	int fd;

	fd = open (RIG->CM108Device, O_WRONLY);
	
	if (fd == -1)
	{
          printf ("Could not open %s for write, errno=%d\n", RIG->CM108Device, errno);
          return;
	}
	
	io[0] = 0;
	io[1] = 0;
	io[2] = 1 << (3 - 1);
	io[3] = PTTState << (3 - 1);
	io[4] = 0;

	n = write (fd, io, 5);
	if (n != 5)
	{
		printf ("Write to %s failed, n=%d, errno=%d\n", RIG->CM108Device, n, errno);
	}
	
	close (fd);
#endif
	return;

}

/*
int CRow;

HANDLE hComPort, hSpeed, hRigType, hButton, hAddr, hLabel, hTimes, hFreqs, hBPQPort;

VOID CreateRigConfigLine(HWND hDlg, struct RIGPORTINFO * PORT, struct RIGINFO * RIG)
{
	char Port[10];

	hButton =  CreateWindow(WC_BUTTON , "", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
					10, CRow+5, 10,10, hDlg, NULL, hInstance, NULL);

	if (PORT->PortType == ICOM)
	{
		char Addr[10];

		sprintf(Addr, "%X", RIG->RigAddr);

		hAddr =  CreateWindow(WC_EDIT , Addr, WS_CHILD | WS_VISIBLE  | WS_TABSTOP | WS_BORDER,
                 305, CRow, 30,20, hDlg, NULL, hInstance, NULL);

	}
	hLabel =  CreateWindow(WC_EDIT , RIG->RigName, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
                 340, CRow, 60,20, hDlg, NULL, hInstance, NULL);

	sprintf(Port, "%d", RIG->PortRecord->PORTCONTROL.PORTNUMBER);
	hBPQPort =  CreateWindow(WC_EDIT , Port, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 405, CRow, 20, 20, hDlg, NULL, hInstance, NULL);

	hTimes =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 430, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	hFreqs =  CreateWindow(WC_EDIT , RIG->FreqText, WS_CHILD | WS_VISIBLE| WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 535, CRow, 300, 20, hDlg, NULL, hInstance, NULL);

	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "0000:1159");
	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200:2359");
	SendMessage(hTimes, CB_SETCURSEL, 0, 0);

	CRow += 30;	

}

VOID CreatePortConfigLine(HWND hDlg, struct RIGPORTINFO * PORT)
{	
	char Port[20]; 
	int i;

	hComPort =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 30, CRow, 90, 160, hDlg, NULL, hInstance, NULL);

	for (i = 1; i < 256; i++)
	{
		sprintf(Port, "COM%d", i);
		SendMessage(hComPort, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Port);
	}

	sprintf(Port, "COM%d", PORT->IOBASE);

	i = SendMessage(hComPort, CB_FINDSTRINGEXACT, 0,(LPARAM) Port);

	SendMessage(hComPort, CB_SETCURSEL, i, 0);
	
	
	hSpeed =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 120, CRow, 75, 80, hDlg, NULL, hInstance, NULL);

	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "2400");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "4800");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "9600");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "19200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "38400");

	sprintf(Port, "%d", PORT->SPEED);

	i = SendMessage(hSpeed, CB_FINDSTRINGEXACT, 0, (LPARAM)Port);

	SendMessage(hSpeed, CB_SETCURSEL, i, 0);

	hRigType =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP,
                 200, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "ICOM");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "YAESU");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "KENWOOD");

	SendMessage(hRigType, CB_SETCURSEL, PORT->PortType -1, 0);

}

INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct RIGPORTINFO * PORT;
		struct RIGINFO * RIG;
		int i, p;

		CRow = 40;

		for (p = 0; p < NumberofPorts; p++)
		{
			PORT = PORTInfo[p];

			CreatePortConfigLine(hDlg, PORT);
		
			for (i=0; i < PORT->ConfiguredRigs; i++)
			{
				RIG = &PORT->Rigs[i];
				CreateRigConfigLine(hDlg, PORT, RIG);
			}
		}



//	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
//                 90, Row, 40,20, hDlg, NULL, hInstance, NULL);
	
//	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
//                 135, Row, 100,20, hDlg, NULL, hInstance, NULL);

return TRUE; 
	}

	case WM_SIZING:
	{
		return TRUE;
	}

	case WM_ACTIVATE:

//		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:

		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

*/

#ifdef WIN32

/* Simple Raw HID functions for Windows - for use with Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 *
 *  rawhid_open - open 1 or more devices
 *  rawhid_recv - receive a packet
 *  rawhid_send - send a packet
 *  rawhid_close - close a device
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Version 1.0: Initial Release
 */

#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
//#include <windows.h>
#include <setupapi.h>
//#include <ddk/hidsdi.h>
//#include <ddk/hidclass.h>

typedef USHORT USAGE;


typedef struct _HIDD_CONFIGURATION {
	PVOID cookie;
 	ULONG size;
	ULONG RingBufferSize;
} HIDD_CONFIGURATION, *PHIDD_CONFIGURATION;

typedef struct _HIDD_ATTRIBUTES {
	ULONG Size;
	USHORT VendorID;
	USHORT ProductID;
	USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;


typedef struct _HIDP_CAPS {
  USAGE  Usage;
  USAGE  UsagePage;
  USHORT  InputReportByteLength;
  USHORT  OutputReportByteLength;
  USHORT  FeatureReportByteLength;
  USHORT  Reserved[17];
  USHORT  NumberLinkCollectionNodes;
  USHORT  NumberInputButtonCaps;
  USHORT  NumberInputValueCaps;
  USHORT  NumberInputDataIndices;
  USHORT  NumberOutputButtonCaps;
  USHORT  NumberOutputValueCaps;
  USHORT  NumberOutputDataIndices;
  USHORT  NumberFeatureButtonCaps;
  USHORT  NumberFeatureValueCaps;
  USHORT  NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;


typedef struct _HIDP_PREPARSED_DATA * PHIDP_PREPARSED_DATA;



// a list of all opened HID devices, so the caller can
// simply refer to them by number
typedef struct hid_struct hid_t;
static hid_t *first_hid = NULL;
static hid_t *last_hid = NULL;
struct hid_struct {
	HANDLE handle;
	int open;
	struct hid_struct *prev;
	struct hid_struct *next;
};
static HANDLE rx_event=NULL;
static HANDLE tx_event=NULL;
static CRITICAL_SECTION rx_mutex;
static CRITICAL_SECTION tx_mutex;


// private functions, not intended to be used from outside this file
static void add_hid(hid_t *h);
static hid_t * get_hid(int num);
static void free_all_hid(void);
void print_win32_err(void);




//  rawhid_recv - receive a packet
//    Inputs:
//	num = device to receive from (zero based)
//	buf = buffer to receive packet
//	len = buffer's size
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes received, or -1 on error
//
int rawhid_recv(int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD r;
	int n;

	if (sizeof(tmpbuf) < len + 1) return -1;
	hid = get_hid(num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&rx_mutex);
	ResetEvent(&rx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = rx_event;
	if (!ReadFile(hid->handle, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) goto return_error;
		r = WaitForSingleObject(rx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&rx_mutex);
	if (n <= 0) return -1;
	n--;
	if (n > len) n = len;
	memcpy(buf, tmpbuf + 1, n);
	return n;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&rx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&rx_mutex);
	return -1;
}

//  rawhid_send - send a packet
//    Inputs:
//	num = device to transmit to (zero based)
//	buf = buffer containing packet to send
//	len = number of bytes to transmit
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes sent, or -1 on error
//
int rawhid_send(int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD n, r;

	if (sizeof(tmpbuf) < len + 1) return -1;
	hid = get_hid(num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&tx_mutex);
	ResetEvent(&tx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = tx_event;
	tmpbuf[0] = 0;
	memcpy(tmpbuf + 1, buf, len);
	if (!WriteFile(hid->handle, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) goto return_error;
		r = WaitForSingleObject(tx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&tx_mutex);
	if (n <= 0) return -1;
	return n - 1;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&tx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&tx_mutex);
	return -1;
}

HANDLE rawhid_open(char * Device)
{
	DWORD index=0;
	HANDLE h;
	hid_t *hid;
	int count=0;

	if (first_hid) free_all_hid();

	if (!rx_event)
	{
		rx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		tx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		InitializeCriticalSection(&rx_mutex);
		InitializeCriticalSection(&tx_mutex);
	}
	h = CreateFile(Device, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (h == INVALID_HANDLE_VALUE) 
		return 0;

	hid = (struct hid_struct *)malloc(sizeof(struct hid_struct));
	if (!hid)
	{
		CloseHandle(h);
		return 0;
	}
	hid->handle = h;
	hid->open = 1;
	add_hid(hid);

	return h;
}


//  rawhid_close - close a device
//
//    Inputs:
//	num = device to close (zero based)
//    Output
//	(nothing)
//
void rawhid_close(int num)
{
	hid_t *hid;

	hid = get_hid(num);
	if (!hid || !hid->open) return;

	CloseHandle(hid->handle);
	hid->handle = NULL;
	hid->open = FALSE;
}




static void add_hid(hid_t *h)
{
	if (!first_hid || !last_hid) {
		first_hid = last_hid = h;
		h->next = h->prev = NULL;
		return;
	}
	last_hid->next = h;
	h->prev = last_hid;
	h->next = NULL;
	last_hid = h;
}


static hid_t * get_hid(int num)
{
	hid_t *p;
	for (p = first_hid; p && num > 0; p = p->next, num--) ;
	return p;
}


static void free_all_hid(void)
{
	hid_t *p, *q;

	for (p = first_hid; p; p = p->next)
	{
		CloseHandle(p->handle);
		p->handle = NULL;
		p->open = FALSE;
	}
	p = first_hid;
	while (p) {
		q = p;
		p = p->next;
		free(q);
	}
	first_hid = last_hid = NULL;
}



void print_win32_err(void)
{
	char buf[256];
	DWORD err;

	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		0, buf, sizeof(buf), NULL);
	Debugprintf("err %ld: %s\n", err, buf);
}

#endif

// RTL_SDR support code

char RTLModes[5][6] = {"FM",  "AM", "USB", "LSB", "????"};


void CheckAndProcessRTLUDP(struct RIGPORTINFO * PORT)
{
	int Length;
	struct sockaddr_in rxaddr;
	int addrlen = sizeof(struct sockaddr_in);
	int Freq;
	unsigned char RXBuffer[16];

	struct RIGINFO * RIG = &PORT->Rigs[0];

	Length = recvfrom(PORT->remoteSock, RXBuffer, 16, 0, (struct sockaddr *)&rxaddr, &addrlen);

	if (Length == 6)
	{
		long long MHz, Hz;
		char CharMHz[16] = "";
		char CharHz[16] = "";
		int i;
		int Mode;

		PORT->Timeout = 0;
		RIG->RIGOK = TRUE;

		Freq = (RXBuffer[4] << 24) + (RXBuffer[3] << 16) + (RXBuffer[2] << 8) + RXBuffer[1];
		Mode = RXBuffer[5]; 

		Freq += RIG->rxOffset;

		RIG->RigFreq = Freq / 1000000.0;

		// If we convert to float to display we get rounding errors, so convert to MHz and Hz to display

		MHz = Freq / 1000000;

		Hz = Freq - MHz * 1000000;

		sprintf(CharMHz, "%lld", MHz);
		sprintf(CharHz, "%06lld", Hz);

		for (i = 5; i > 2; i--)
		{
			if (CharHz[i] == '0')
				CharHz[i] = 0;
			else
				break;
		}

		sprintf(RIG->WEB_FREQ,"%lld.%s", MHz, CharHz);
		SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
		strcpy(RIG->Valchar, RIG->WEB_FREQ);

		sprintf(RIG->WEB_MODE,"%s", RTLModes[Mode]);

		strcpy(RIG->ModeString, Modes[Mode]);
			SetWindowText(RIG->hMODE, RIG->WEB_MODE);

	}
}

VOID RTLUDPPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on HAMLIB
	char cmd[80];
	int len;

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
		strcpy(RIG->WEB_FREQ, "-----------");;
		strcpy(RIG->WEB_MODE, "------");

		RIG->RIGOK = FALSE;
		return;
	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				int n;
				
				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				n = sendto(PORT->remoteSock, PORT->TXBuffer, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));
				
				if (PORT->TXLen == 10)
					n = sendto(PORT->remoteSock, PORT->TXBuffer + 5, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));

				PORT->TXBuffer[0] = 'P';		// Send Poll
				
				n = sendto(PORT->remoteSock, PORT->TXBuffer, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));

				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission
				return;
			}
		}
	}
			
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 10;			// Once Per Sec
		
	// Read Frequency 

	cmd[0] = 'P';
	
	len = sendto(PORT->remoteSock, cmd, 5,  0, &PORT->remoteDest, sizeof(struct sockaddr));

	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID ConnecttoRTLUDP(struct RIGPORTINFO * PORT)
{
	char Msg[255];
	int i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct sockaddr_in sinx;

	if (PORT->remoteSock)
	{
		closesocket(PORT->remoteSock);
	}

	PORT->remoteSock = 0;
	PORT->remoteSock = socket(AF_INET,SOCK_DGRAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for RTLUDP socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	PORT->CONNECTING = FALSE;
  	 	return; 
	}

	ioctl(PORT->remoteSock, FIONBIO, &param);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	PORT->CONNECTED = TRUE;
	PORT->hDevice = (HANDLE)1;				// simplifies check code
	PORT->Alerted = TRUE;
}

char * getObjectFromArray(char * Msg);	// This gets the next object from an array ({} = object, [] = array



char * getArrayFromMsg(char * Msg)
{
	// This gets the next object from an array ({} = object, [] = array
	// We look for the end of the object same number of { and }, teminate after } and return pointer to next object
	// So we have terminated Msg, and returned next object in array

	// Only call if Msg is the next array in Msg


	char * ptr = Msg;
	char c;

	int Open = 0;
	int Close = 0;
		
	while (c = *(ptr++))
	{
		if (c == '[') Open ++; else if (c == ']') Close ++;

		if (Open == Close)
		{
			*(ptr++) = 0;
			return ptr;
		}
	}
	return 0;
}


//----- G7TAJ -----

void ProcessSDRANGELFrame(struct RIGPORTINFO * PORT)
{

	int Length;

	char * msg;
	char * rest;

	struct RIGINFO * RIG;
	char * ptr, * ptr1, * ptr2, * ptr3, * pos;
	int Len, TotalLen;
	char cmd[80];
	char ReqBuf[256];
	char SendBuff[256];
	int chunklength;
	int headerlen;
	int i, n = 0;
	char * Sets;
	char * Rest;
	char * Set;
	int channelcount;
	char * channels;
	char * channel;
	char * samplingDevice;
	char * save;
  
	//Debugprintf("Process SDRANGEL Frame %d\n", PORT->RXLen);

	msg = PORT->RXBuffer;
	Length = PORT->RXLen;

	msg[Length] = 0;

	ptr1 = strstr(msg, "Transfer-Encoding: chunked" );

	if (ptr1 == NULL)
		return;

	ptr2 = strstr(ptr1, "\r\n\r\n");

	if (ptr2 == NULL)
		return;

	// ptr2 +4 points to the length of the first chunk (in hex), terminated by crlf

	chunklength = (int)strtol(ptr2 + 4, &ptr3, 16); 
	ptr3 += 2;		// pointer to first chunk data
	headerlen = ptr3 - msg;

	// make sure we have first chunk

	if (chunklength + headerlen > Length)
		return;

	PORT->RXLen = 0; //we have all the frame now
	PORT->Timeout = 0;

	if (strstr(ptr3, "deviceSets") == 0)
	{
		return;
	}

	// Message has info for all rigs

	// As we mess with the message, save a copy and restore for each Rig

	save = _strdup(ptr3);

	for (i = 0; i < PORT->ConfiguredRigs; i++)
	{
		strcpy(ptr3, save);
		n = 0;

		RIG = &PORT->Rigs[i];	
		RIG->RIGOK = 1;

		// we can have one or more sampling devices (eg rltsdr) each with one or
		// more channels (eg ssb demod, ssb mod). each set of sampling device = channel(s) is a device set.

		// Find Device Set for this device (in RIG->

		// Message Structure is

		//{
		//	"deviceSets": [...].
		//	"devicesetcount": 2,
		//	"devicesetfocus": 0
		//}

		// Get the device sets (JSON [..] is an array

		Sets = strchr(ptr3, '[');

		if (Sets == 0)
			continue;

		Rest = getArrayFromMsg(Sets);

		// Do we need to check devicesetcount ??. Maybe use to loop through sets, or just stop at end

		// get the set for our device

		while (RIG->RigAddr >= n)
		{
			Set = strchr(Sets, '{');		// Position to start of first Object

			if (Set == 0)
				break;

			Sets = getObjectFromArray(Set);
			n++;
		}

		if (Set == 0)
			continue;


		// Now get the channel. looking for key "index": 

		// we could have a number of sampling devices and channels but for now get sampling device freq
		// and first channel freq. Channels are in an Array

		if ((ptr = strstr(Set, "channelcount")) == 0)
			continue;
	
		channelcount = atoi(&ptr[15]);
		
		if ((channels = strchr(Set, '[')) == 0)
			continue;

		samplingDevice = getArrayFromMsg(channels);

		while(channelcount--)
		{
			channel = strchr(channels, '{');
			channels = getObjectFromArray(channel);

			if ((ptr = strstr(channel, "index")))
			{
				n = atoi(&ptr[7]);
				if (n == RIG->Channel)
					break;
			}
		}



		if (pos = strstr(samplingDevice, "centerFrequency")) 	//"centerFrequency": 10489630000,
		{
			pos += 18;
			strncpy(cmd, pos, 20);
			RIG->RigFreq = atof(cmd) / 1000000.0;
		}

		if (pos = strstr(channel, "deltaFrequency")) 
		{
			pos += 17;
			strncpy(cmd, pos, 20);
			RIG->RigFreq += (atof(cmd) + RIG->rxOffset) / 1000000.0;;
		}


		_gcvt(RIG->RigFreq, 9, RIG->Valchar);

		sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
		SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);

		// we could get mode from Title line:
		//"title": "SSB Demodulator",

		if (pos = strstr(channel, "title")) 	
		{
			pos += 9;
			strncpy(cmd, pos, 20);
			strlop(pos, ' ');
			strncpy(RIG->ModeString, pos, 15);
			sprintf(RIG->WEB_MODE, "%s", RIG->ModeString);
			SetWindowText(RIG->hMODE, RIG->WEB_MODE);
		}

	}


	/*
	while (msg && msg[0])
	{
	rest = strlop(msg, ',');

	if ( pos = strstr(msg, "centerFrequency")) 	//"centerFrequency": 10489630000,
	{
	pos += 18;
	strncpy(cmd, pos,20);

	RIG->RigFreq = atof(cmd) / 1000000.0;

	//					printf("FREQ=%f\t%s\n", RIG->RigFreq, cmd);

	_gcvt(RIG->RigFreq, 9, RIG->Valchar);

	sprintf(RIG->WEB_FREQ,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, RIG->WEB_FREQ);
	}

	else if (memcmp(msg, "Mode:", 5) == 0)
	{
	if (strlen(&msg[6]) < 15)
	strcpy(RIG->ModeString, &msg[6]);
	}

	else if (memcmp(msg, "Passband:", 9) == 0)
	{
	RIG->Passband = atoi(&msg[10]);
	sprintf(RIG->WEB_MODE, "%s/%d", RIG->ModeString, RIG->Passband);
	SetWindowText(RIG->hMODE, RIG->WEB_MODE);
	}

	msg = rest;
	}
	*/
	free (save);
}



VOID SDRANGELThread(struct RIGPORTINFO * PORT);

VOID ConnecttoSDRANGEL(struct RIGPORTINFO * PORT)
{
	if (SDRANGELRunning)
		_beginthread(SDRANGELThread, 0, (void *)PORT);
	return ;
}

VOID SDRANGELThread(struct RIGPORTINFO * PORT)
{
	// Opens sockets and looks for data
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;

	if (PORT->CONNECTING)
		return;

	PORT->RXLen = 0;

	PORT->CONNECTING = 1;

	if (PORT->remoteSock)
	{
		closesocket(PORT->remoteSock);
	}

	PORT->remoteSock = 0;
	PORT->remoteSock = socket(AF_INET,SOCK_STREAM,0);

	if (PORT->remoteSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for SDRAngel socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	PORT->CONNECTING = FALSE;
  	 	return;
	}

	setsockopt(PORT->remoteSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
	setsockopt(PORT->remoteSock, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&bcopt, 4);

	if (connect(PORT->remoteSock,(LPSOCKADDR) &PORT->remoteDest,sizeof(PORT->remoteDest)) == 0)
	{
		//
		//	Connected successful
		//

		ioctl(PORT->remoteSock, FIONBIO, &param);
	}
	else
	{
		if (PORT->Alerted == FALSE)
		{
			struct sockaddr_in * destaddr = (SOCKADDR_IN * )&PORT->remoteDest;

			err = WSAGetLastError();

   			sprintf(Msg, "Connect Failed for SDRAngel socket - error code = %d Port %d\r\n",
				err, htons(destaddr->sin_port));

			WritetoConsole(Msg);
				PORT->Alerted = TRUE;
		}

		closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
	 	PORT->CONNECTING = FALSE;
		return;
	}

	PORT->CONNECTED = TRUE;
	PORT->CONNECTING = 0;

	PORT->hDevice = (HANDLE)1;				// simplifies check code

	PORT->Alerted = TRUE;

	while (PORT->CONNECTED && SDRANGELRunning)
	{
		FD_ZERO(&readfs);
		FD_ZERO(&errorfs);

		FD_SET(PORT->remoteSock,&readfs);
		FD_SET(PORT->remoteSock,&errorfs);

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		ret = select((int)PORT->remoteSock + 1, &readfs, NULL, &errorfs, &timeout);

		if (SDRANGELRunning == 0)
			return;

		if (ret == SOCKET_ERROR)
		{
			Debugprintf("SDRAngel Select failed %d ", WSAGetLastError());
			goto Lost;
		}

		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(PORT->remoteSock, &readfs))
			{
				SDRANGELProcessMessage(PORT);
			}

			if (FD_ISSET(PORT->remoteSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "SDRAngel Connection lost for Port %s\r\n", PORT->IOBASE);
				WritetoConsole(Msg);

				PORT->CONNECTED = FALSE;
				PORT->Alerted = FALSE;
				PORT->hDevice = 0;				// simplifies check code

				closesocket(PORT->remoteSock);
				PORT->remoteSock = 0;
				return;
			}
			continue;
		}
		else
		{
		}
	}
	sprintf(Msg, "SDRAngel Thread Terminated Port %s\r\n", PORT->IOBASE);
	WritetoConsole(Msg);
}

/*
# 10489630000

CURL_DATA='{
    "deviceHwType": "RTLSDR",
    "direction": 0,
    "rtlSdrSettings": {
       "centerFrequency": "'$1'"
     }

}';


curl -X PATCH "http://127.0.0.1:8091/sdrangel/deviceset/0/device/settings" \
     -H  "accept: application/json" \
     -H  "Content-Type: application/json" \
     -d "$CURL_DATA"









*/


VOID SDRANGELPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;

	// SDRAngel can have muliple rigs but we only need to poll once to get info for all rigs so just use first entry

	struct RIGINFO * RIG = &PORT->Rigs[0];
	int Len, i;
	char ReqBuf[256];
	char SendBuff[256];
	//char * SDRANGEL_GETheader = "GET /sdrangel/deviceset/%d/device/settings "
	//			   "HTTP/1.1\nHost: %s\nConnection: keep-alive\n\r\n";

	char * SDRANGEL_GETheader = "GET /sdrangel/devicesets "
				   "HTTP/1.1\nHost: %s\nConnection: keep-alive\n\r\n";


	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;

		if (PORT->Timeout)			// Still waiting
			return;

		// Loop through all Rigs

		for (i = 0; i <	PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			SetWindowText(RIG->hFREQ, "------------------");
			SetWindowText(RIG->hMODE, "----------");
			strcpy(RIG->WEB_FREQ, "-----------");;
			strcpy(RIG->WEB_MODE, "------");
	
			RIG->RIGOK = FALSE;
		}
		return;
	}

	// Send Data if avail, else send poll

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				char cmd[80];
				double freq;

				if (RIG->RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				_gcvt(PORT->FreqPtr->Freq / 1000000.0, 9, RIG->Valchar); // For MH

				// Send the Set Freq here, send set mode when we get a response

				memcpy(&PORT->ScanEntry, PORT->FreqPtr, sizeof(struct ScanEntry));

//TODO
				sprintf(cmd, "%.0f", PORT->FreqPtr->Freq);
				SDRANGELSendCommand(PORT, "SETFREQ", cmd);


				PORT->CmdSent = 1;
				PORT->Retries = 0;
				PORT->Timeout = 10;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
				ReleasePermission(RIG);			// Release Perrmission
				return;
			}
		}
	}

	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 40;	

	// Read Frequency
//TODO


//	Len = sprintf(SendBuff, SDRANGEL_GETheader, 0, &PORT->remoteDest );  // devicenum, host:port
	Len = sprintf(SendBuff, SDRANGEL_GETheader, &PORT->remoteDest );  // devicenum, host:port

	if (PORT->CONNECTED)
	{
		if (send(PORT->remoteSock, SendBuff, Len, 0) != Len)
		{
			if (PORT->remoteSock)
				closesocket(PORT->remoteSock);

			PORT->remoteSock = 0;
			PORT->CONNECTED = FALSE;
			PORT->hDevice = 0;
			return;
		}
	}

	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID SDRANGELSendCommand(struct RIGPORTINFO * PORT, char * Command, char * Value)
{
	int Len, ret;
	char ReqBuf[512];
	char SendBuff[512];
	char ValueString[256] ="";
	char * SDRANGEL_PATCHheader = "PATCH /sdrangel/deviceset/%d/device/settings "
 				     "HTTP/1.1\nHost: %s\n"
				     "accept: application/json\n"
				     "Content-Type: application/json\n"
				     "Connection: keep-alive\n"
				     "Content-length: %d\r\n"
				     "\r\n%s";

	if (!PORT->CONNECTED)
		return;

	sprintf(ValueString, SDRANGEL_FREQ_DATA, "RTLSDR", Value);

	Len = sprintf(SendBuff, SDRANGEL_PATCHheader, 0, &PORT->remoteDest, strlen(ValueString), ValueString);

	ret = send(PORT->remoteSock, SendBuff, Len, 0);
	
	if (ret != Len)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;
		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;
	}

	return;
}


void SDRANGELProcessMessage(struct RIGPORTINFO * PORT)
{
	// Called from Background thread

	int InputLen = recv(PORT->remoteSock, &PORT->RXBuffer[PORT->RXLen], 8192 - PORT->RXLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		if (PORT->remoteSock)
			closesocket(PORT->remoteSock);

		PORT->remoteSock = 0;

		PORT->CONNECTED = FALSE;
		PORT->hDevice = 0;
		return;
	}

	PORT->RXLen += InputLen;
	ProcessSDRANGELFrame(PORT);
}



// ---- G7TAJ ----
