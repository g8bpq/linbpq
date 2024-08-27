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

/*

Using code from 6pack Linux Kernel driver with the following licence and credits

 *	6pack driver version 0.4.2, 1999/08/22
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * 		This module implements the AX.25 protocol for kernel-based
 *		devices like TTYs. It interfaces between a raw TTY, and the
 *		kernel's AX.25 protocol layers, just like slip.c.
 *		AX.25 needs to be separated from slip.c while slip.c is no
 *		longer a static kernel device since it is a module.
 *
 *	Author: Andreas Könsgen <ajk@ccac.rwth-aachen.de>
 *
 *	Lots of stuff has been taken from mkiss.c, written by
 *	Hans Alblas <hans@esrac.ele.tue.nl>
 *
 *	with the fixes from
 *	
 *	Jonathan (G4KLX)	Fixed to match Linux networking changes - 2.1.15.
 *	Matthias (DG2FEF)       Fixed bug in ax25_close(): dev_lock_wait() was
 *                              called twice, causing a deadlock.
 */


//	6pack needs fast response to received characters, and I want to be able to operate over TCP links as well as serial.
//	So I think the character level stuff may need to run in a separate thread, probably using select.
//
//	I also need to support multiple 6pack ports.

// ?? Do we add this as a backend to KISS driver or a separate Driver. KISS Driver is already quite messy. Not decided yet.

// ?? If using serial/real TNC we need to be able to interleave control and data bytes, but I think with TCP/QtSM it won't be necessary
// ?? Also a don't see any point in running multiple copies of QtSM on one port, but maybe should treat the QtSM channels as
//	multidropped ports for scheduling (?? only if on same radio ??)

//	?? I think it needs to look like a KISS (L2) driver but will need a transmit scheduler level to do DCD/CSMA/PTT processing,
//	ideally with an interlock to other drivers on same port. This needs some thought with QtSM KISS with multiple modems on one channel





#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#include "compatbits.h"
#include <string.h>
#include "CHeaders.h"

#include "bpq32.h"

#include "tncinfo.h"

#ifndef WIN32

#define APIENTRY
#define DllExport
#define VOID void

#else
#include <windows.h>
#endif

/****************************************************************************
 *	Defines for the 6pack driver.
 ****************************************************************************/

/*
	provisoric define
*/
#define SIXP_MAJOR	MKISS_MAJOR
/* end of provisoric defines */


#define TRUE	1
#define FALSE	0

#define AX25_MAXDEV	16		/* MAX number of AX25 channels;
					   This can be overridden with
					   insmod -oax25_maxdev=nnn	*/
#define AX_MTU		236	

/* 6pack protocol bytes/masks. */
#define SIXP_INIT_CMD		0xE8
#define SIXP_TNC_FOUND		0xE9
#define SIXP_CMD_MASK		0xC0
#define SIXP_PRIO_CMD_MASK	0x80
#define SIXP_PRIO_DATA_MASK	0x38
#define SIXP_STD_CMD_MASK	0x40
#define SIXP_DCD_MASK		0x08
#define SIXP_RX_DCD_MASK	0x18
#define SIXP_CHN_MASK		0x07
#define SIXP_TX_MASK		0x20
#define SIXP_CON_LED_ON		0x68
#define SIXP_STA_LED_ON		0x70
#define SIXP_LED_OFF		0x60

/* checksum for a valid 6pack encapsulated packet */
#define SIXP_CHKSUM		0xFF

/* priority commands */
#define SIXP_SEOF		0x40	/* TX underrun */
#define SIXP_TX_URUN		0x48	/* TX underrun */
#define SIXP_RX_ORUN		0x50	/* RX overrun */
#define SIXP_RX_BUF_OVL		0x58	/* RX overrun */

struct sixPackTNCInfo
{
	// info for each TNC in chain

	int                magic;

	char * name;

	/* These are pointers to the malloc()ed frame buffers. */
	unsigned char      *rbuff;		/* receiver buffer		*/
	int                rcount;		/* received chars counter       */
	unsigned char      *xbuff;		/* transmitter buffer		*/
	unsigned char      *xhead;		/* pointer to next byte to XMIT */
	int                xleft;		/* bytes left in XMIT queue     */

	/* SLIP interface statistics. */
	unsigned long      rx_packets;		/* inbound frames counter	*/
	unsigned long      tx_packets;		/* outbound frames counter      */
	unsigned long      rx_errors;		/* Parity, etc. errors          */
	unsigned long      tx_errors;		/* Planned stuff                */
	unsigned long      rx_dropped;		/* No memory for skb            */
	unsigned long      tx_dropped;		/* When MTU change              */
	unsigned long      rx_over_errors;	/* Frame bigger then SLIP buf.  */

	/* Detailed SLIP statistics. */
	int                 mtu;		/* Our mtu (to spot changes!)   */
	int                 buffsize;	/* Max buffers sizes            */


	unsigned char       flags;		/* Flag values/ mode etc	*/
#define AXF_INUSE	0				/* Channel in use               */
#define AXF_ESCAPE	1               /* ESC received                 */
#define AXF_ERROR	2               /* Parity, etc. error           */
#define AXF_KEEPTEST	3			/* Keepalive test flag		*/
#define AXF_OUTWAIT	4				/* is outpacket was flag	*/

	int                 mode;


	/* variables for the state machine */

	unsigned char	tnc_ok;
	unsigned char	status;
	unsigned char	status1;
	unsigned char	status2;

	unsigned char	duplex;
	unsigned char	led_state;
	unsigned char	tx_enable;
	
	unsigned char	raw_buf[4];		/* receive buffer */
	unsigned char	cooked_buf[400];	/* receive buffer after 6pack decoding */
	
	unsigned int	rx_count;		/* counter for receive buffer */
	unsigned int	rx_count_cooked;	/* counter for receive buffer after 6pack decoding */

	unsigned char	tx_delay;
	unsigned char	persistance;
	unsigned char	slottime;

};


struct sixPackPortInfo
{
	// Per port (chain of TNC's)

	unsigned int linkOK;			// Set if response is received
	unsigned int reinitTimer;

	struct sixPackTNCInfo * TNCS[8];	// Max TNCs in chain Not sure if real 6pack uses o or 1 for first
};


#define AX25_MAGIC		0x5316
#define SIXP_DRIVER_MAGIC	0x5304

#define SIXP_INIT_RESYNC_TIMEOUT	150	/* in 10 ms */
#define SIXP_RESYNC_TIMEOUT		500	/* in 10 ms */

/* default radio channel access parameters */
#define SIXP_TXDELAY			25	/* in 10 ms */
#define SIXP_PERSIST			50
#define SIXP_SLOTTIME			10	/* in 10 ms */

static int sixpack_encaps(unsigned char *tx_buf, unsigned char *tx_buf_raw, int length, unsigned char tx_delay);
static void sixpack_decaps(struct sixPackPortInfo *, unsigned char);

static void decode_prio_command(unsigned char, struct sixPackTNCInfo *);
static void decode_std_command(unsigned char, struct sixPackTNCInfo *);
static void decode_data(unsigned char, struct sixPackTNCInfo *);
static void resync_tnc(unsigned long);
static void xmit_on_air(struct sixPackTNCInfo *ax);
static void start_tx_timer(struct sixPackTNCInfo *ax);
int Connectto6Pack(int port);

VOID __cdecl Debugprintf(const char * format, ...);

/* Set the "sending" flag.  This must be atomic, hence the ASM. */

static void ax_lock(struct sixPackTNCInfo *ax)
{
//	if (test_and_set_bit(0, (void *)&ax->dev->tbusy))
//		printk(KERN_ERR "6pack: %s: trying to lock already locked device!\n", ax->dev->name);
}


/* Clear the "sending" flag.  This must be atomic, hence the ASM. */
static void ax_unlock(struct sixPackTNCInfo *ax)
{
//	if (!test_and_clear_bit(0, (void *)&ax->dev->tbusy))
//		printk(KERN_ERR "6pack: %s: trying to unlock already unlocked device!\n", ax->dev->name);
}
/* Send one completely decapsulated AX.25 packet to the AX.25 layer. */
static void ax_bump(struct sixPackTNCInfo *ax)
{
}

VOID SixPackProcessReceivedPacket(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	unsigned char * ptr;
	char Buffer[4096];

	if (TNC->InputLen > 8000)	// Shouldnt have packets longer than this
		TNC->InputLen=0;

	InputLen = recv(TNC->TCPSock, &TNC->ARDOPBuffer[TNC->InputLen], 8192 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?

		int err = GetLastError();

		closesocket(TNC->TCPSock);

		TNC->TCPSock = 0;

		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		return;					
	}

	TNC->InputLen += InputLen;

	// Process Bytes

}



static int sixpack_encaps(unsigned char *tx_buf, unsigned char *tx_buf_raw, int length, unsigned char tx_delay)
{
	int count = 0;
	unsigned char checksum = 0, buf[400];
	int raw_count = 0;

	tx_buf_raw[raw_count++] = SIXP_PRIO_CMD_MASK | SIXP_TX_MASK;
	tx_buf_raw[raw_count++] = SIXP_SEOF;

	buf[0] = tx_delay;
	for(count = 1; count < length; count++)
		buf[count] = tx_buf[count];

	for(count = 0; count < length; count++)
		checksum += buf[count];
	buf[length] = (unsigned char)0xff - checksum;
	
	for(count = 0; count <= length; count++) {
		if((count % 3) == 0) {
			tx_buf_raw[raw_count++] = (buf[count] & 0x3f);
			tx_buf_raw[raw_count] = ((buf[count] >> 2) & 0x30);
		}
		else if((count % 3) == 1) {
			tx_buf_raw[raw_count++] |= (buf[count] & 0x0f);
			tx_buf_raw[raw_count] =
				((buf[count] >> 2) & 0x3c);
		} else {
			tx_buf_raw[raw_count++] |= (buf[count] & 0x03);
			tx_buf_raw[raw_count++] =
				(buf[count] >> 2);
		} /* else */
	} /* for */
	if ((length % 3) != 2)
		raw_count++;
	tx_buf_raw[raw_count++] = SIXP_SEOF;
	return(raw_count);
}


static void sixpack_decaps(struct sixPackPortInfo *ax, unsigned char inbyte)
{
	// if (inbyte == SIXP_TNC_FOUND) {
	// This is wrong. It will only work with one TNC in the chain. Should look for E8 in top bits. Bottome 3 bits are number in chain

	if ((inbyte & SIXP_INIT_CMD) == 0)
	{
		int Count = inbyte & SIXP_CHN_MASK;
		struct sixPackTNCInfo * sixTNC;

		int i;

		Debugprintf("6pack: %d TNCs found. ", Count);

		// clear existing TNC strucs and allocate new ones if needed

		for (i = 0; i < Count; i++)
		{
			sixTNC = ax->TNCS[i];

			if (sixTNC)
				memset(sixTNC, 0, sizeof(struct sixPackTNCInfo));
			else
				sixTNC = ax->TNCS[i] = zalloc(sizeof(struct sixPackTNCInfo));

			sixTNC->tnc_ok = 1;
		}

		ax->linkOK = TRUE;

	}
/*
if((inbyte & SIXP_PRIO_CMD_MASK) != 0)
		decode_prio_command(inbyte, ax);
	else if((inbyte & SIXP_STD_CMD_MASK) != 0)
		decode_std_command(inbyte, ax);
	else {
		if ((ax->status & SIXP_RX_DCD_MASK) == SIXP_RX_DCD_MASK)
			decode_data(inbyte, ax);
	} 
*/
}

/* identify and execute a 6pack priority command byte */

void decode_prio_command(unsigned char cmd, struct sixPackTNCInfo *ax)
{
	unsigned char channel;

	channel = cmd & SIXP_CHN_MASK;
	if ((cmd & SIXP_PRIO_DATA_MASK) != 0) {     /* idle ? */

	/* RX and DCD flags can only be set in the same prio command,
	   if the DCD flag has been set without the RX flag in the previous
	   prio command. If DCD has not been set before, something in the
	   transmission has gone wrong. In this case, RX and DCD are
	   cleared in order to prevent the decode_data routine from
	   reading further data that might be corrupt. */

		if (((ax->status & SIXP_DCD_MASK) == 0) &&
			((cmd & SIXP_RX_DCD_MASK) == SIXP_RX_DCD_MASK)) {
				if (ax->status != 1)
					Debugprintf("6pack: protocol violation\n");
				else
					ax->status = 0;
				cmd &= !SIXP_RX_DCD_MASK;
		}
		ax->status = cmd & SIXP_PRIO_DATA_MASK;
	} /* if */


        /* if the state byte has been received, the TNC is present,
           so the resync timer can be reset. */

	if (ax->tnc_ok == 1) {
//		del_timer(&(ax->resync_t));
//		ax->resync_t.data = (unsigned long) ax;
//		ax->resync_t.function = resync_tnc;
//		ax->resync_t.expires = jiffies + SIXP_INIT_RESYNC_TIMEOUT;
//		add_timer(&(ax->resync_t));
	}

	ax->status1 = cmd & SIXP_PRIO_DATA_MASK;
}

/* try to resync the TNC. Called by the resync timer defined in
  decode_prio_command */

static void resync_tnc(unsigned long channel)
{
	static char resync_cmd = SIXP_INIT_CMD;
	struct sixPackTNCInfo *ax = (struct sixPackTNCInfo *) channel;

	Debugprintf("6pack: resyncing TNC\n");

	/* clear any data that might have been received */
	
	ax->rx_count = 0;
	ax->rx_count_cooked = 0;

	/* reset state machine */

	ax->status = 1;
	ax->status1 = 1;
	ax->status2 = 0;
	ax->tnc_ok = 0;
	
	/* resync the TNC */

	ax->led_state = SIXP_LED_OFF;
//	ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
//	ax->tty->driver.write(ax->tty, 0, &resync_cmd, 1);


	/* Start resync timer again -- the TNC might be still absent */

//	del_timer(&(ax->resync_t));
//	ax->resync_t.data = (unsigned long) ax;
//	ax->resync_t.function = resync_tnc;
//	ax->resync_t.expires = jiffies + SIXP_RESYNC_TIMEOUT;
//	add_timer(&(ax->resync_t));
}



/* identify and execute a standard 6pack command byte */

void decode_std_command(unsigned char cmd, struct sixPackTNCInfo *ax)
{
	unsigned char checksum = 0, channel;
	unsigned int i;

	channel = cmd & SIXP_CHN_MASK;
	switch(cmd & SIXP_CMD_MASK) {     /* normal command */
		case SIXP_SEOF:
			if ((ax->rx_count == 0) && (ax->rx_count_cooked == 0)) {
				if ((ax->status & SIXP_RX_DCD_MASK) ==
					SIXP_RX_DCD_MASK) {
					ax->led_state = SIXP_CON_LED_ON;
//					ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
				} /* if */
			} else {
				ax->led_state = SIXP_LED_OFF;
//				ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
				/* fill trailing bytes with zeroes */
				if (ax->rx_count == 2) {
					decode_data(0, ax);
					decode_data(0, ax);
					ax->rx_count_cooked -= 2;
				}
				else if (ax->rx_count == 3) {
					decode_data(0, ax);
					ax->rx_count_cooked -= 1;
				}
				for (i=0; i< ax->rx_count_cooked; i++)
					checksum += ax->cooked_buf[i];
				if (checksum != SIXP_CHKSUM) {
					Debugprintf("6pack: bad checksum %2.2x\n", checksum);
				} else {
					ax->rcount = ax->rx_count_cooked-1;
					ax_bump(ax);
				} /* else */
				ax->rx_count_cooked = 0;
			} /* else */
			break;
		case SIXP_TX_URUN:
			Debugprintf("6pack: TX underrun\n");
			break;
		case SIXP_RX_ORUN:
			Debugprintf("6pack: RX overrun\n");
			break;
		case SIXP_RX_BUF_OVL:
			Debugprintf("6pack: RX buffer overflow\n");
	} /* switch */
} /* function */

/* decode 4 sixpack-encoded bytes into 3 data bytes */

void decode_data(unsigned char inbyte, struct sixPackTNCInfo *ax)
{

	unsigned char *buf;

	if (ax->rx_count != 3)
		ax->raw_buf[ax->rx_count++] = inbyte;
	else {
		buf = ax->raw_buf;
		ax->cooked_buf[ax->rx_count_cooked++] =
			buf[0] | ((buf[1] << 2) & 0xc0);
		ax->cooked_buf[ax->rx_count_cooked++] =
			(buf[1] & 0x0f) | ((buf[2] << 2) & 0xf0);
		ax->cooked_buf[ax->rx_count_cooked++] =
			(buf[2] & 0x03) | (inbyte << 2);
		ax->rx_count = 0;
	}
}

// This takes a packet from the KISS driver and converts to 6pack format

static void ax_encaps(struct sixPackTNCInfo *ax, unsigned char *icp, int len)
{
	unsigned char *p;
	int count;
	struct sixPackTNCInfo *tmp_ax;

	if (len > ax->mtu) {		/* Sigh, shouldn't occur BUT ... */
		len = ax->mtu;
		Debugprintf("6pack: %s: truncating oversized transmit packet!\n", ax->name);
		ax->tx_dropped++;
		ax_unlock(ax);
		return;
	}

	p = icp;

	if (p[0] > 5)
	{
		Debugprintf("%s: invalid KISS command -- dropped\n", ax->name);
		ax_unlock(ax);
		return;
	}

	if ((p[0] != 0) && (len > 2))
	{
		Debugprintf("%s: KISS command packet too long -- dropped\n", ax->name);
		ax_unlock(ax);
		return;
	}

	if ((p[0] == 0) && (len < 15))
	{
		Debugprintf("%s: bad AX.25 packet to transmit -- dropped\n", ax->name);
		ax->tx_dropped++;
		ax_unlock(ax);
		return;
	}

	switch(p[0])
	{
		case 1:	tmp_ax->tx_delay = p[1];	break;
		case 2:	tmp_ax->persistance = p[1];	break;
		case 3: tmp_ax->slottime = p[1];	break;
		case 4: Debugprintf("6pack: got KISS command 4 -- ignored\n");
			break;
		case 5: tmp_ax->duplex = p[1];		break;
	}

	// How do we handle ACKMODE ??

	if (p[0] != 0) {
		ax_unlock(ax);
		return;
	}
	
	count = sixpack_encaps(p, (unsigned char *) tmp_ax->xbuff, len, tmp_ax->tx_delay);
	tmp_ax->xleft = count;
	tmp_ax->status2 = count;
	tmp_ax->xhead = tmp_ax->xbuff;
	
	/* in case of DAMA or fullduplex operation, we don't take care
	   about the state of the DCD or of any timers, as the determination
	   of the correct time to send is the job of the AX.25 layer. We send
	   immediately after data has arrived. */

	if (tmp_ax->duplex == 1) { /* DAMA / fullduplex */
		xmit_on_air(tmp_ax);
	} else { /* CSMA */
		start_tx_timer(tmp_ax);
	} /* if ax->duplex */
} /* ax_encaps */

// This starts sending a packet to a TNC. TNC will immediately send (after TXD)

/* the next two functions perform the persistence/slottime algorithm for CSMA access.
   If the persistence check was successful, write the data to the serial driver.
   Note that in case of DAMA operation, the data is not sent here. */

/* set timer to time out slottime */
static void start_tx_timer(struct sixPackTNCInfo *ax)
{
//	del_timer(&(ax->tx_t));
//	ax->tx_t.data = (unsigned long) ax;
//	ax->tx_t.function = tx_schedule;
//	ax->tx_t.expires = jiffies + (((ax->slottime)+1)*HZ)/100;
//	add_timer(&(ax->tx_t));
}

/* compute random number and check if it is ok to send */
static void tx_schedule(unsigned long channel)
{
	struct sixPackTNCInfo *ax = (struct sixPackTNCInfo *) channel;
	static unsigned char random;
	
	random = random * 17 + 41;

	if (((ax->status1 & SIXP_DCD_MASK) == 0) && (random < ax->persistance)) 
		xmit_on_air(ax);
	else
		start_tx_timer(ax);
}


static void xmit_on_air(struct sixPackTNCInfo *ax)
{
	int actual;

//	ax->tty->flags |= (1 << TTY_DO_WRITE_WAKEUP);
	ax->led_state = SIXP_STA_LED_ON;
//	ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
	ax->tx_enable = 1;
//	actual = ax->tty->driver.write(ax->tty, 0, ax->xbuff, ax->status2);
	ax->tx_packets++;
//	ax->dev->trans_start = jiffies;
	ax->xleft -= actual;
	ax->xhead = ax->xbuff + actual;
	ax->led_state = SIXP_LED_OFF;
//	ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
} /* xmit_on_air */


// BPQ Driver Module Interface


extern int (WINAPI FAR *GetModuleFileNameExPtr)();
extern int (WINAPI FAR *EnumProcessesPtr)();

static int Socket_Data(int sock, int error, int eventcode);

VOID MoveWindows(struct TNCINFO * TNC);
static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen);
int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);
BOOL SerialWriteCommBlock(struct TNCINFO * TNC);
void SerialCheckRX(struct TNCINFO * TNC);
int SerialSendData(struct TNCINFO * TNC, UCHAR * data, int txlen);
int SerialSendCommand(struct TNCINFO * TNC, UCHAR * data);
int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
VOID SendInitScript(struct TNCINFO * TNC);
int SIXPACKGetLine(char * buf);
int ProcessEscape(UCHAR * TXMsg);
VOID SIXPACKProcessReceivedPacket(struct TNCINFO * TNC);
static int KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int ConnecttoKISS(int port);
TRANSPORTENTRY * SetupNewSession(TRANSPORTENTRY * Session, char * Bufferptr);
BOOL DecodeCallString(char * Calls, BOOL * Stay, BOOL * Spy, UCHAR * AXCalls);
BOOL FindLink(UCHAR * LinkCall, UCHAR * OurCall, int Port, struct _LINKTABLE ** REQLINK);
VOID RESET2(struct _LINKTABLE * LINK);
VOID L2SENDXID(struct _LINKTABLE * LINK);
VOID SENDSABM(struct _LINKTABLE * LINK);

static char ClassName[]="KISSSTATUS";
static char WindowTitle[] = "SIXPACK";
static int RigControlRow = 165;

#ifndef LINBPQ
#include <commctrl.h>
#endif

extern int SemHeldByAPI;

static RECT Rect;

static int ProcessLine(char * buf, int Port);

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

static int ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if (_stricmp(buf, "ADDR"))
		return FALSE;						// Must start with ADDR

	ptr = strtok(NULL, " \t\n\r");

	BPQport = Port;
	p_ipad = ptr;

	TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->DefaultMode = TNC->WL2KMode = 0;	// Packet 1200

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
	if (p_ipad == NULL)
		p_ipad = strtok(NULL, " \t\n\r");

	if (p_ipad == NULL) return (FALSE);
	
	p_port = strtok(NULL, " \t\n\r");
			
	if (p_port == NULL) return (FALSE);

	WINMORport = atoi(p_port);

	TNC->destaddr.sin_family = AF_INET;
	TNC->destaddr.sin_port = htons(WINMORport);

	TNC->HostName = malloc(strlen(p_ipad)+1);

	if (TNC->HostName == NULL) return TRUE;

	strcpy(TNC->HostName,p_ipad);

	ptr = strtok(NULL, " \t\n\r");

	if (ptr)
	{
		if (_stricmp(ptr, "PTT") == 0)
		{
			ptr = strtok(NULL, " \t\n\r");

			if (ptr)
			{
				DecodePTTString(TNC, ptr);
				ptr = strtok(NULL, " \t\n\r");
			}
		}
	}
		
	if (ptr)
	{
		if (_memicmp(ptr, "PATH", 4) == 0)
		{
			p_cmd = strtok(NULL, "\n\r");
			if (p_cmd) TNC->ProgramPath = _strdup(p_cmd);
		}
	}

	// Read Initialisation lines

	while (TRUE)
	{
		if (GetLine(buf) == 0)
			return TRUE;

		strcpy(errbuf, buf);

		if (memcmp(buf, "****", 4) == 0)
			return TRUE;

		ptr = strchr(buf, ';');
		if (ptr)
		{
			*ptr++ = 13;
			*ptr = 0;
		}

		if (_memicmp(buf, "UPDATEMAP", 9) == 0)
			TNC->PktUpdateMap = TRUE;
		else if (standardParams(TNC, buf) == FALSE)
			strcat(TNC->InitScript, buf);	
	}
	return (TRUE);	
}

char * Config;
static char * ptr1, * ptr2;

int SIXPACKGetLine(char * buf)
{
loop:

	if (ptr2 == NULL)
		return 0;

	memcpy(buf, ptr1, ptr2 - ptr1 + 2);
	buf[ptr2 - ptr1 + 2] = 0;
	ptr1 = ptr2 + 2;
	ptr2 = strchr(ptr1, 13);

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}

VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

static time_t ltime;

static VOID SendToTNC(struct TNCINFO * TNC, int Stream, UCHAR * Encoded, int EncLen)
{
	if (TNC->hDevice)
	{
		// Serial mode. Queue to Hostmode driver
		
		PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

		if (buffptr == 0) return;			// No buffers, so ignore

		buffptr->Len = EncLen;
		memcpy(&buffptr->Data[0], Encoded, EncLen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
		TNC->Streams[Stream].FramesQueued++;

		return;
	}
}


static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);


static size_t ExtProc(int fn, int port, PDATAMESSAGE buff)
{
	int datalen;
	PMSGWITHLEN buffptr;
	char txbuff[500];
	unsigned int bytes,txlen = 0;
	UCHAR * TXMsg;

	size_t Param;
	int Stream = 0;
	HKEY hKey=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct ScanEntry * Scan;

	if (TNC == NULL)
		return 0;							// Port not defined

	if (TNC->CONNECTED == 0 && TNC->CONNECTING == 0)
	{
		// Try to reopen every 30 secs

		if (fn > 3  && fn < 7)
			goto ok;

		TNC->ReopenTimer++;

		if (TNC->ReopenTimer < 150)
			return 0;

		TNC->ReopenTimer = 0;

		Connectto6Pack(TNC->Port);
		
		return 0;

		SendInitScript(TNC);

	}
ok:

	switch (fn)
	{
		case 7:			

		// 100 mS Timer. May now be needed, as Poll can be called more frequently in some circumstances

		SerialCheckRX(TNC);
		return 0;

		case 1:				// poll

			STREAM = &TNC->Streams[0];

			if (STREAM->NeedDisc)
			{
				STREAM->NeedDisc--;

				if (STREAM->NeedDisc == 0)
				{
					// Send the DISCONNECT

					SerialSendCommand(TNC, "DISCONNECT\r");
				}
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				STREAM->Attached = TRUE;

				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;


				// Stop other ports in same group

				SuspendOtherPorts(TNC);

				sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
				MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

				// Stop Scanning

				sprintf(Msg, "%d SCANSTOP", TNC->Port);

				Rig_Command( (TRANSPORTENTRY *) -1, Msg);
			}

			if (STREAM->Attached)
				CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

			// See if any frames for this port

			STREAM = &TNC->Streams[0];

			if (STREAM->BPQtoPACTOR_Q)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)Q_REM(&STREAM->BPQtoPACTOR_Q);
				UCHAR * data = &buffptr->Data[0];
				STREAM->FramesQueued--;
				txlen = (int)buffptr->Len;
				STREAM->BytesTXed += txlen;

				bytes=SerialSendData(TNC, data, txlen);
				WritetoTrace(TNC, data, txlen);
			}

			if (STREAM->PACTORtoBPQ_Q != 0)
			{
				buffptr = (PMSGWITHLEN)Q_REM(&STREAM->PACTORtoBPQ_Q);

				datalen = (int)buffptr->Len;

				buff->PORT = Stream;						// Compatibility with Kam Driver
				buff->PID = 0xf0;
				memcpy(&buff->L2DATA, &buffptr->Data[0], datalen);		// Data goes to + 7, but we have an extra byte
				datalen += sizeof(void *) + 4;

				PutLengthinBuffer(buff, datalen);

				ReleaseBuffer(buffptr);

				return (1);
			}

			if (STREAM->ReportDISC)		// May need a delay so treat as a counter
			{
				STREAM->ReportDISC--;

				if (STREAM->ReportDISC == 0)
				{
					buff->PORT = Stream;
					return -1;
				}
			}

			return (0);

	case 2:				// send

		Stream = 0;

		if (!TNC->CONNECTED)
			return 0;		// Don't try if not connected

		STREAM = &TNC->Streams[0];
		
		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}

		// We may get KISS packets (UI or session related) or text commands such as RADIO, CONNECT

		txlen = GetLengthfromBuffer(buff) - (MSGHDDRLEN);

		TXMsg = &buff->L2DATA[0];

		if (buff->PID != 240)			// ax.25 address
		{
			txlen = KissEncode(&buff->PID, txbuff, txlen);
			txlen = send(TNC->TCPSock, txbuff, txlen, 0);
			return 1;
		}

		TXMsg[txlen - 1] = 0;
		strcpy(txbuff, TXMsg);

		if (STREAM->Attached == 0)
			return 0;

		if (STREAM->Connected)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			if (buffptr == 0) return 1;			// No buffers, so ignore

			buffptr->Len  = txlen;
			memcpy((UCHAR *)&buffptr->Data[0], &buff->L2DATA[0], txlen);
			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

			// connected data

			return 1;
		}

		// Process as Text Command

		if (_memicmp(txbuff, "D\r", 2) == 0 || _memicmp(txbuff, "BYE\r", 4) == 0)
		{
			STREAM->ReportDISC = TRUE;		// Tell Node
			return 0;
		}
	
		if (_memicmp(txbuff, "RADIO ", 6) == 0)
		{
			sprintf(&buff->L2DATA[0], "%d %s", TNC->Port, &txbuff[6]);

			if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK, &buff->L2DATA[0]))
			{
			}
			else
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				if (buffptr == 0) return 1;			// No buffers, so ignore

				buffptr->Len  = sprintf((UCHAR *)&buffptr->Data[0], "%s", &buff->L2DATA[0]);
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}
			return 1;
		}


		if (_memicmp(txbuff, "OVERRIDEBUSY", 12) == 0)
		{
			PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

			TNC->OverrideBusy = TRUE;

			if (buffptr)
			{
				buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "SIXPACK} OK\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			}

			return 0;

		}

		if (_memicmp(&buff->L2DATA[0], "SessionTimeLimit", 16) == 0)
		{
			if (buff->L2DATA[16] != 13)
			{					
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();

				TNC->SessionTimeLimit = atoi(&buff->L2DATA[16]) * 60;

				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "SIXPACK} OK\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}
				return 0;
			}
		}

		if (toupper(buff->L2DATA[0]) == 'C' && buff->L2DATA[1] == ' ' && txlen > 2)	// Connect
		{
			// Connect Command. Pass to L2 code to start session

			char * ptr = strchr(&buff->L2DATA[2], 13);
			TRANSPORTENTRY * NewSess = L4TABLE;
			struct _LINKTABLE * LINK;
			TRANSPORTENTRY * Session = TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK;
			struct PORTCONTROL * PORT = &TNC->PortRecord->PORTCONTROL;

			UCHAR axcalls[64];
			UCHAR ourcall[7];					// Call we are using (may have SSID bits inverted
			int Stay = 0, Spy = 0, CQFLAG = 0, n;

			if (ptr)
				*ptr = 0;

			_strupr(&buff->L2DATA[2]);

			if (DecodeCallString(&buff->L2DATA[2], &Stay, &Spy, &axcalls[0]) == 0)
			{
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "SIXPACK} Invalid Call\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}
				return 0;
			}

			// Code copied from cmdc00

			// Get Session Entry for Downlink

			NewSess = SetupNewSession(Session, NULL);
	
			if (NewSess == NULL)
				return 0;

			NewSess->L4CIRCUITTYPE = L2LINK + DOWNLINK;

			//	FORMAT LINK TABLE ENTRY FOR THIS CONNECTION

			memcpy(Session->L4USER, NewSess->L4USER, 7);
			memcpy(ourcall, NewSess->L4USER, 7);

			// SSID SWAP TEST - LEAVE ALONE FOR HOST or Pactor like (unless UZ7HO)

			if ((Session->L4CIRCUITTYPE & BPQHOST))// host
				goto noFlip3;

			if ((Session->L4CIRCUITTYPE & PACTOR))
			{
				// incoming is Pactorlike - see if UZ7HO

				if (memcmp(Session->L4TARGET.EXTPORT->PORT_DLL_NAME, "UZ7HO", 5) != 0)
					goto noFlip3;

				if (Session->L4TARGET.EXTPORT->MAXHOSTMODESESSIONS < 2)	// Not multisession	
					goto noFlip3;

				ourcall[6] ^= 0x1e;		// UZ7HO Uplink - flip
			}
			else

				// Must be L2 uplink - flip

				ourcall[6] ^= 0x1e;									// Flip SSID

noFlip3:

			//	SET UP NEW SESSION (OR RESET EXISTING ONE)

			FindLink(axcalls, ourcall, TNC->Port, &LINK);

			if (LINK == NULL)
			{			
				PMSGWITHLEN buffptr = (PMSGWITHLEN)GetBuff();
				if (buffptr)
				{
					buffptr->Len = sprintf((UCHAR *)&buffptr->Data[0], "SIXPACK} Sorry - System Tables Full\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}
				return 0;
			}

			memcpy(LINK->LINKCALL, axcalls, 7);
			memcpy(LINK->OURCALL, ourcall, 7);

			LINK->LINKPORT = PORT;

			LINK->L2TIME = PORT->PORTT1;

			// Copy Digis

			n = 7;
			ptr = &LINK->DIGIS[0];

			while (axcalls[n])
			{
				memcpy(ptr, &axcalls[n], 7);
				n += 7;
				ptr += 7;

				LINK->L2TIME += 2 * PORT->PORTT1;	// ADJUST TIMER VALUE FOR 1 DIGI
			}

			LINK->LINKTYPE = 2;						// DOWNLINK
			LINK->LINKWINDOW = PORT->PORTWINDOW;

			RESET2(LINK);						// RESET ALL FLAGS

//			if (CMD->String[0] == 'N' && SUPPORT2point2)
//				LINK->L2STATE = 1;					// New (2.2) send XID
//			else
				LINK->L2STATE = 2;					// Send SABM

			LINK->CIRCUITPOINTER = NewSess;

			NewSess->L4TARGET.LINK = LINK;

			if (PORT->PORTPACLEN)
				NewSess->SESSPACLEN = Session->SESSPACLEN = PORT->PORTPACLEN;

			STREAM->Connecting = TRUE;

			if (CQFLAG == 0)			// if a CQ CALL  DONT SEND SABM
			{
				if (LINK->L2STATE == 1)
					L2SENDXID(LINK);
				else	
					SENDSABM(LINK);
			}
			return 0;
		}	

		return 0;

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		return ((TNC->CONNECTED) << 8 | TNC->Streams[0].Disconnecting << 15);		// OK
		
	case 4:				// reinit7

		return 0;

	case 5:				// Close

		return 0;

	case 6:				// Scan Stop Interface

		Param = (size_t)buff;
	
		if (Param == 2)		// Check  Permission (Shouldn't happen)
		{
			Debugprintf("Scan Check Permission called on SIXPACK");
			return 1;		// OK to change
		}

		if (Param == 1)		// Request Permission
		{
			if (!TNC->CONNECTED)
				return 0;					// No connection so no interlock

			if (TNC->ConnectPending == 0 && TNC->PTTState == 0)
			{
				SerialSendCommand(TNC, "CONOK OFF");
				TNC->GavePermission = TRUE;
				return 0;	// OK to Change
			}

			if (TNC->ConnectPending)
				TNC->ConnectPending--;		// Time out if set too long

			return TRUE;
		}

		if (Param == 3)		// Release  Permission
		{
			if (TNC->GavePermission)
			{
				TNC->GavePermission = FALSE;
				if (TNC->ARDOPCurrentMode[0] != 'S')	// Skip
					SerialSendCommand(TNC, "CONOK ON");
			}
			return 0;
		}

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;
		return 0;
	}
	return 0;
}

VOID SIXPACKReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[64];

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);
	Rig_Command( (TRANSPORTENTRY *) -1, TXMsg);

	strcpy(TNC->WEB_TNCSTATE, "Free");
	MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	ReleaseOtherPorts(TNC);
}

VOID SIXPACKSuspendPort(struct TNCINFO * TNC, struct TNCINFO * ThisTNC)
{
}

VOID SIXPACKReleasePort(struct TNCINFO * TNC)
{
}

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}</script>"
		"</head><title>VARA Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2><form method=post target=\"POPUPW\" onsubmit=\"POPUPW = window.open('about:blank','POPUPW',"
		"'width=440,height=150');\" action=ARDOPAbort?%d>SIXPACK Status"
		"<input name=Save value=\"Abort Session\" type=submit style=\"position: absolute; right: 20;\"></form></h2>",
		TNC->Port);


	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>TNC State</td><td>%s</td></tr>", TNC->WEB_TNCSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Mode</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "<tr><td>Channel State</td><td>%s</td></tr>", TNC->WEB_CHANSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Proto State</td><td>%s</td></tr>", TNC->WEB_PROTOSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Traffic</td><td>%s</td></tr>", TNC->WEB_TRAFFIC);
//	Len += sprintf(&Buff[Len], "<tr><td>TNC Restarts</td><td></td></tr>", TNC->WEB_RESTARTS);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}



VOID * SIXPACKExtInit(EXTPORTDATA * PortEntry)
{
	int port;
	char Msg[255];
	char * ptr;
	struct TNCINFO * TNC;
	char * TempScript;
	struct PORTCONTROL * PORT = &PortEntry->PORTCONTROL;
	
	port = PORT->PORTNUMBER;

	if (TNCInfo[port])					// If restarting, free old config
		free(TNCInfo[port]);

	TNC = TNCInfo[port] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;

	if (PortConfig[port])			// May not have config
		ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		sprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return ExtProc;
	}

	TNC->sixPack = zalloc(sizeof(struct sixPackTNCInfo));

	TNC->Port = port;
	TNC->Hardware = H_SIXPACK;
	TNC->ARDOPBuffer = malloc(8192);

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	if (PortEntry->PORTCONTROL.PORTINTERLOCK && TNC->RXRadio == 0 && TNC->TXRadio == 0)
		TNC->RXRadio = TNC->TXRadio = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->PORTCONTROL.USERS = 1;			// Max 1 Session

	TNC->PacketChannels = 0;

	PortEntry->MAXHOSTMODESESSIONS = 1;

	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only
	PortEntry->PERMITGATEWAY = TRUE;				// Can change ax.25 call on each stream

	PortEntry->PORTCONTROL.UICAPABLE = TRUE;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->SuspendPortProc = SIXPACKSuspendPort;
	TNC->ReleasePortProc = SIXPACKReleasePort;

//	PortEntry->PORTCONTROL.PORTSTARTCODE = KISSStartPort;
//	PortEntry->PORTCONTROL.PORTSTOPCODE = KISSStopPort;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = zalloc(1000);

	// cant think of any yet

	if (TNC->InitScript)
	{
		strcat(TempScript, TNC->InitScript);
		free(TNC->InitScript);
	}

	TNC->InitScript = TempScript;

	if (TNC->WL2K == NULL)
		if (PortEntry->PORTCONTROL.WL2KInfo.RMSCall[0])			// Alrerady decoded
			TNC->WL2K = &PortEntry->PORTCONTROL.WL2KInfo;

	PortEntry->PORTCONTROL.TNC = TNC;

	TNC->WebWindowProc = WebProc;
	TNC->WebWinX = 520;
	TNC->WebWinY = 500;
	TNC->WebBuffer = zalloc(5000);

	TNC->WEB_COMMSSTATE = zalloc(100);
	TNC->WEB_TNCSTATE = zalloc(100);
	TNC->WEB_CHANSTATE = zalloc(100);
	TNC->WEB_BUFFERS = zalloc(100);
	TNC->WEB_PROTOSTATE = zalloc(100);
	TNC->WEB_RESTARTTIME = zalloc(100);
	TNC->WEB_RESTARTS = zalloc(100);

	TNC->WEB_MODE = zalloc(20);
	TNC->WEB_TRAFFIC = zalloc(100);


#ifndef LINBPQ

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 700, 450, ForcedClose);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,6,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,6,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,28,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,28,520,20, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,50,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,50,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,72,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,72,144,20, TNC->hDlg, NULL, hInstance, NULL);
 
 	CreateWindowEx(0, "STATIC", "Proto State", WS_CHILD | WS_VISIBLE,10,94,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,116,94,374,20 , TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,116,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,116,116,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTS = CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,138,40,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_RESTARTTIME = CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,138,200,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after failed Connection");	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);
	AppendMenu(TNC->hMenu, MF_STRING, ARDOP_ABORT, "Abort Current Session");

	MoveWindows(TNC);
#endif
	Consoleprintf("SIXPACK Host %s %d", TNC->HostName, htons(TNC->destaddr.sin_port));

	Connectto6Pack(port);

	time(&TNC->lasttime);			// Get initial time value

	return ExtProc;
}


VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[Stream].BytesOutstanding == 0)
		SerialSendCommand(TNC, "DISCONNECT\r");
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	SerialSendCommand(TNC, "DISCONNECT\r");
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	if (Stream == 0)
	{
		SIXPACKReleaseTNC(TNC);
	}
}


static int	KissDecode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];

		if (c == FESC)
		{
			c=inbuff[++i];
			{
				if (c == TFESC)
					c=FESC;
				else
				if (c == TFEND)
					c=FEND;
			}
		}

		outbuff[txptr++]=c;
	}

	return txptr;

}


static int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	outbuff[0]=FEND;
	outbuff[1]=0;
	txptr=2;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];
		
		switch (c)
		{
		case FEND:
			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFEND;
			break;

		case FESC:

			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFESC;
			break;

		default:

			outbuff[txptr++]=c;
		}
	}

	outbuff[txptr++]=FEND;

	return txptr;

}


VOID SIXPACKProcessReceivedPacket(struct TNCINFO * TNC)
{
	int InputLen, MsgLen;
	unsigned char * ptr;
	char Buffer[4096];

	if (TNC->InputLen > 8000)	// Shouldnt have packets longer than this
		TNC->InputLen=0;

	InputLen = recv(TNC->TCPSock, &TNC->ARDOPBuffer[TNC->InputLen], 8192 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?

		int err = GetLastError();

		closesocket(TNC->TCPSock);

		TNC->TCPSock = 0;

		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
		MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

		return;					
	}

	TNC->InputLen += InputLen;

	// Extract and decode KISS frames

	ptr = memchr(TNC->ARDOPBuffer + 1, FEND, TNC->InputLen - 1);		// Ignore leading FEND

	while (ptr)	//  FEND in buffer
	{
		ptr++;

		MsgLen = ptr - TNC->ARDOPBuffer;

		if (MsgLen > 360)
		{
			TNC->InputLen = 0;
			return;
		}

		TNC->InputLen -= MsgLen;

		if (MsgLen > 1)
		{
			PMESSAGE Buff = GetBuff();

			MsgLen = KissDecode(TNC->ARDOPBuffer, Buffer, MsgLen);

			// we dont need the FENDS or control byte

			MsgLen -= 3;
				
			if (Buff)
			{
				memcpy(&Buff->DEST, &Buffer[2], MsgLen);
				MsgLen += (3 + sizeof(void *));

				PutLengthinBuffer((PDATAMESSAGE)Buff, MsgLen);		// Needed for arm5 portability

				C_Q_ADD(&TNC->PortRecord->PORTCONTROL.PORTRX_Q, (UINT *)Buff);
			}
		}

		if (TNC->InputLen == 0)
			return;

		memmove(TNC->ARDOPBuffer, ptr, TNC->InputLen);
		ptr = memchr(TNC->ARDOPBuffer + 1, FEND, TNC->InputLen - 1);		// Ignore leading FEND
	}
		
}



/*

// we dont need the control byte

len --;

if (Buffer)
{
memcpy(&Buffer->DEST, &Port->RXMSG[1], len);
len += (3 + sizeof(void *));

PutLengthinBuffer((PDATAMESSAGE)Buffer, len);		// Needed for arm5 portability

C_Q_ADD(TNC->PortRecord->PORTCONTROL.PORTRX_Q, (UINT *)Buffer);
}

*/

//	TNC->InputLen -= MsgLen;
//	goto loop;




void AttachSIXPACK(struct PORTCONTROL * PORT, MESSAGE * Buffer)
{
	// SABM on HFKISS port. L2 code will accepr call and connect to appl if necessary, but
	// need to attach the port

	char Call[16] = "";
	char OrigCall[16] = "";
	struct TNCINFO * TNC = PORT->TNC;
	struct WL2KInfo * WL2K = TNC->WL2K;

	if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
	{
		TRANSPORTENTRY * SESS;
		struct TNCINFO * TNC = PORT->TNC;

		// Incoming Connect

		Call[ConvFromAX25(Buffer->DEST, Call)] = 0;
		OrigCall[ConvFromAX25(Buffer->ORIGIN, OrigCall)] = 0;

		// Stop other ports in same group

		SuspendOtherPorts(TNC);

		TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

		ProcessIncommingConnectEx(TNC, Call, 0, FALSE, FALSE);

		SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

		SESS->Mode = TNC->WL2KMode;

		TNC->ConnectPending = FALSE;

		if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", OrigCall, Call, TNC->RIG->Valchar);
			SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
			if (SESS->Frequency == 1500)
			{
				// try to get from WL2K record

				if (WL2K)
				{
					SESS->Frequency = WL2K->Freq;
				}
			}
			SESS->Mode = TNC->WL2KMode;
		}
		else
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", OrigCall, Call);
			if (WL2K)
			{
				SESS->Frequency = WL2K->Freq;
				SESS->Mode = WL2K->mode;
			}
		}

		if (WL2K)
			strcpy(SESS->RMSCall, WL2K->RMSCall);

		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
	}
}

void DetachSIXPACK(struct PORTCONTROL * PORT)
{
	// L2 Link Closed. Detach.

	struct TNCINFO * TNC = PORT->TNC;
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	if (STREAM->Attached)
		STREAM->ReportDISC = TRUE;		// Tell Node

	STREAM->Connecting = FALSE;
	STREAM->Connected = FALSE;

}

void SIXPACKConnected(struct PORTCONTROL * PORT, struct _LINKTABLE * LINK)
{
	// UA received when connecting

	struct TNCINFO * TNC = PORT->TNC;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	struct WL2KInfo * WL2K = TNC->WL2K;

	TRANSPORTENTRY * SESS;
	char Call[16] = "";
	char OrigCall[16] = "";


	if (STREAM->Connecting)
	{
		STREAM->Connecting = FALSE;
		STREAM->Connected = TRUE;

		Call[ConvFromAX25(LINK->LINKCALL, Call)] = 0;
		OrigCall[ConvFromAX25(LINK->OURCALL, OrigCall)] = 0;

		TNC->SessionTimeLimit = TNC->DefaultSessionTimeLimit;		// Reset Limit

		SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];
		SESS->Mode = TNC->WL2KMode;

		if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s", OrigCall, Call, TNC->RIG->Valchar);
			SESS->Frequency = (int)(atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
			
			if (SESS->Frequency == 1500)
			{
				// try to get from WL2K record

				if (WL2K)
				{
					SESS->Frequency = WL2K->Freq;
				}
			}
			SESS->Mode = TNC->WL2KMode;
		}
		else
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", OrigCall, Call);

			if (WL2K)
			{
				SESS->Frequency = WL2K->Freq;
				SESS->Mode = WL2K->mode;
			}
		}

		if (WL2K)
			strcpy(SESS->RMSCall, WL2K->RMSCall);

		MySetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);
	}
}



VOID x6PackThead(void * portptr);

int Connectto6Pack(int port)
{
	_beginthread(x6PackThead, 0, (void *)(size_t)port);

	return 0;
}

VOID x6PackThead(void * portptr)
{
	// 6pack over TNC thread

	// Opens socket and looks for data 
	
	int port = (int)(size_t)portptr;
	char Msg[255];
	int err, i, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	char * ptr1;
	char * ptr2;
	UINT * buffptr;
	int reinitTimer = 0;
	int linkactive = 0;

	struct sixPackPortInfo * sixPack = TNC->sixPack;

	if (sixPack == NULL || TNC->HostName == NULL)
		return;

	TNC->BusyFlags = 0;

	TNC->CONNECTING = TRUE;

	Sleep(3000);		// Allow init to complete 

#ifdef WIN32
	if (strcmp(TNC->HostName, "127.0.0.1") == 0)
	{
		// can only check if running on local host
		
		TNC->PID = GetListeningPortsPID(TNC->destaddr.sin_port);
		if (TNC->PID == 0)
		{
			TNC->CONNECTING = FALSE;
			return;						// Not listening so no point trying to connect
		}

		// Get the File Name in case we want to restart it.

		if (TNC->ProgramPath == NULL)
		{
			if (GetModuleFileNameExPtr)
			{
				HANDLE hProc;
				char ExeName[256] = "";

				hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->PID);
	
				if (hProc)
				{
					GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);
					CloseHandle(hProc);

					TNC->ProgramPath = _strdup(ExeName);
				}
			}
		}
	}
#endif

//	// If we started the TNC make sure it is still running.

//	if (!IsProcess(TNC->PID))
//	{
//		RestartTNC(TNC);
//		Sleep(3000);
//	}


	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->HostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		HostEnt = gethostbyname (TNC->HostName);
		 
		 if (!HostEnt)
		 {
		 	TNC->CONNECTING = FALSE;
			return;			// Resolve failed
		 }
		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//	closesocket(TNC->TCPSock);

	TNC->TCPSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->TCPSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for 6Pack socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}

	setsockopt(TNC->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (connect(TNC->TCPSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=sprintf(Msg, "Connect Failed for 6Pack socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

			TNC->Alerted = TRUE;
		}
		
		closesocket(TNC->TCPSock);
		TNC->TCPSock = 0;
	 	TNC->CONNECTING = FALSE;
		return;
	}

	Sleep(1000);

	TNC->LastFreq = 0;			//	so V4 display will be updated

 	TNC->CONNECTING = FALSE;
	TNC->CONNECTED = TRUE;
	TNC->BusyFlags = 0;
	TNC->InputLen = 0;

	TNC->Alerted = TRUE;

	sprintf(TNC->WEB_COMMSSTATE, "Connected to TNC");		
	MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

	FreeSemaphore(&Semaphore);

	sprintf(Msg, "Connected to TNC Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);

	while (TNC->CONNECTED)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TNC->TCPSock,&readfs);
		FD_SET(TNC->TCPSock,&errorfs);

		timeout.tv_sec = 1;	
		timeout.tv_usec = 0;				// We use this to run timeouts

		ret = select((int)TNC->TCPSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			Debugprintf("6Pack Select failed %d ", WSAGetLastError());
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TNC->TCPSock, &readfs))
			{
				GetSemaphore(&Semaphore, 52);
				SixPackProcessReceivedPacket(TNC);
				FreeSemaphore(&Semaphore);
			}
								
			if (FD_ISSET(TNC->TCPSock, &errorfs))
			{
Lost:	
				sprintf(Msg, "6Pack Connection lost for Port %d\r\n", TNC->Port);
				WritetoConsole(Msg);

				sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC lost");
				MySetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

				TNC->CONNECTED = FALSE;
				TNC->Alerted = FALSE;

				if (TNC->PTTMode)
					Rig_PTT(TNC, FALSE);			// Make sure PTT is down

				if (TNC->Streams[0].Attached)
					TNC->Streams[0].ReportDISC = TRUE;

				TNC->TCPSock = 0;
				return;
			}
		}
		else
		{
			// Timeout. See if we need to reinit tnc chain

			if (sixPack->linkOK == 0)
			{
				sixPack->reinitTimer++;
				if (sixPack->reinitTimer > 30)
				{
					// send SIXP_INIT_CMD which should rescan the tnc chain

					unsigned char Msg = SIXP_INIT_CMD;
					int txlen = send(TNC->TCPSock, &Msg, 1, 0);
					sixPack->reinitTimer = 0;
				}
			}
		}

	}
	sprintf(Msg, "6Pack Thread Terminated Port %d\r\n", TNC->Port);
	WritetoConsole(Msg);
}






