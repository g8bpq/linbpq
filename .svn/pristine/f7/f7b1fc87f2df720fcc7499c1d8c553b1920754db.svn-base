//
//	DLL to inteface the BPQ Virtual COM emulator to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

//	Version 1.0 November 2005
//

//  Version 1.1	October 2006

//		Write diagmnostics to BPQ console window instead of STDOUT

// Version 1.2 February 2008

//		Changes for dynamic unload of bpq32.dll

// Version 1.2.1 May 2008

//		Correct RX length (was 1 byte too long)

// Version 1.3.1 Jan 2009

//		Support Win98 VirtualCOM Driver

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR         1
#define VERSION_MINOR         0

typedef unsigned char byte;

#include "CHeaders.h"
#include "bpqvkiss.h"

#include <stdlib.h>

//#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
//#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"
 

static int	ASYINIT(int comport, int speed, int bpqport, BOOL Report);
int	kissencode(UCHAR * inbuff, UCHAR * outbuff, int len);
int GetRXMessage(int port,UCHAR * buff);
void CheckReceivedData(PVCOMINFO  pVCOMInfo);
static int ReadCommBlock(PVCOMINFO  pVCOMInfo, LPSTR lpszBlock, DWORD nMaxLength );
static BOOL WriteCommBlock(int port, UCHAR * lpByte , DWORD dwBytesToWrite);

PVCOMINFO CreateInfo( int port,int speed, int bpqport )	;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

static BOOL Win98 = FALSE;

struct PORTCONTROL * PORTVEC[33];

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int len,txlen=0;
	char txbuff[1000];

	if (VCOMInfo[port]->ComDev == (HANDLE) -1)
	{
		// Try to reopen every 30 secs

		VCOMInfo[port]->ReopenTimer++;

		if (VCOMInfo[port]->ReopenTimer < 300)
			return 0;

		VCOMInfo[port]->ReopenTimer = 0;
		
		ASYINIT(PORTVEC[port]->IOBASE, 9600, port, FALSE);

		if (VCOMInfo[port]->ComDev == (HANDLE) -1)
			return 0;
	}


	switch (fn)
	{
	case 1:				// poll

		len = GetRXMessage(port,buff);
	
//		if (len > 0)
//		{
//			// Randomly drop packets

//			if ((rand() % 7) > 5)
//			{
//				Debugprintf("VKISS Test Drop packet");
//				return 0;
//			}
//		}
		
		return len;

	case 2:				// send

		txlen=(buff[6]<<8) + buff[5];

		txlen=kissencode(&buff[7],(char *)&txbuff,txlen-7);

		WriteCommBlock(port,txbuff,txlen);
		
		return (0);


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK
			
		break;

	case 4:				// reinit
		
		CloseHandle(VCOMInfo[port]->ComDev);
		VCOMInfo[port]->ComDev =(HANDLE) -1;
		VCOMInfo[port]->ReopenTimer = 250;

		return (0);

	case 5:				// Close

		CloseHandle(VCOMInfo[port]->ComDev);

		return (0);

	}

	return 0;

}

VOID * VCOMExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[80];
	
	//
	//	Will be called once for each port to be mapped to a BPQ Virtual COM Port
	//	The VCOM port number is in IOBASE
	//

	sprintf(msg,"VKISS COM%d", PortEntry->IOBASE);
	WritetoConsole(msg);

	PORTVEC[PortEntry->PORTNUMBER] = PortEntry;
	
	CreateInfo(PortEntry->IOBASE, 9600, PortEntry->PORTNUMBER);

	// Open File
	
	ASYINIT(PortEntry->IOBASE, 9600, PortEntry->PORTNUMBER, TRUE);

	WritetoConsole("\n");

	return ExtProc;
}

static int	kissencode(UCHAR * inbuff, UCHAR * outbuff, int len)
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

int	ASYINIT(int comport, int speed, int bpqport, BOOL Report)
{
   char       szPort[ 30 ];
   char buf[256];
   int n, Err;

#pragma warning( push )
#pragma warning( disable : 4996 )

#ifndef _winver

#define _winver 0x0600

#endif


   if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning( pop ) 

   if (Win98)
	{
		VCOMInfo[bpqport]->ComDev = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
	               NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
   else{
		sprintf( szPort, "\\\\.\\pipe\\BPQCOM%d", comport ) ;

		VCOMInfo[bpqport]->ComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );

			//Handle = CreateFile(Value, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


		Err = GetLastError();

		if (VCOMInfo[bpqport]->ComDev != (HANDLE) -1)
		{
			VCOMInfo[bpqport]->NewVCOM = TRUE;
			Err = GetFileType(VCOMInfo[bpqport]->ComDev);
		}
		else
		{
			// Try old style 	

			sprintf( szPort, "\\\\.\\BPQ%d", comport ) ;

			VCOMInfo[bpqport]->ComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
		}
   }  
	if (VCOMInfo[bpqport]->ComDev == (HANDLE) -1 && Report)
	{
		n=sprintf(buf,"Virtual COM Port %d could not be opened ",comport);
		WritetoConsole(buf);

		return (FALSE) ;
	}

	return (TRUE) ;
}

static int GetRXMessage(int port,UCHAR * buff)
{
	int len;
	PVCOMINFO pVCOMInfo ;

	if (NULL == (pVCOMInfo = VCOMInfo[port]))
		return 0;

	if (!pVCOMInfo->MSGREADY)
		CheckReceivedData(pVCOMInfo);		// Look for data in RXBUFFER and COM port

	if (pVCOMInfo->MSGREADY)
	{
		len=pVCOMInfo->RXMPTR-&pVCOMInfo->RXMSG[1];		// Don't need KISS Control Byte
		
  		if (pVCOMInfo->RXMSG[0] != 0 && pVCOMInfo->RXMSG[0] != 12)
		{
			pVCOMInfo->MSGREADY=FALSE;
			pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
			return 0;						// Not KISS Data
		}

		//
		//	Remove KISS control byte
		//

		if (pVCOMInfo->RXMSG[0] == 12)
		{
			//	AckMode Frame. Return the next 2 bytes, but don't pass them to Host

			UCHAR AckResp[8];

			AckResp[0] = FEND;
			memcpy(&AckResp[1], &pVCOMInfo->RXMSG[0], 3);	//Copy Opcode and Ack Bytes
			AckResp[4] = FEND;
			WriteCommBlock(port, AckResp, 5);

			len -= 2;
			memcpy(&buff[7],&pVCOMInfo->RXMSG[3],len);
//			Debugprintf("VKISS Ackmode Frame");
		}
		else
	
			memcpy(&buff[7],&pVCOMInfo->RXMSG[1],len);

		len+=7;
		buff[5]=(len & 0xff);
		buff[6]=(len >> 8);
		
		//
		//	reset pointers
		//

		pVCOMInfo->MSGREADY=FALSE;
		pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;

		return len;
	}
	else

	return 0;					// nothing doing
}

static void CheckReceivedData(PVCOMINFO pVCOMInfo)
{
 	UCHAR c;

	if (pVCOMInfo->RXBCOUNT == 0)
	{	
		//
		//	Check com buffer
		//
	
		pVCOMInfo->RXBCOUNT = ReadCommBlock(pVCOMInfo, (LPSTR) &pVCOMInfo->RXBUFFER, MAXBLOCK-1 );
		pVCOMInfo->RXBPTR=(UCHAR *)&pVCOMInfo->RXBUFFER; 
	}

	if (pVCOMInfo->RXBCOUNT == 0)
		return;

	while (pVCOMInfo->RXBCOUNT != 0)
	{
		pVCOMInfo->RXBCOUNT--;

		c = *(pVCOMInfo->RXBPTR++);

		if (pVCOMInfo->ESCFLAG)
		{
			//
			//	FESC received - next should be TFESC or TFEND

			pVCOMInfo->ESCFLAG = FALSE;

			if (c == TFESC)
				c=FESC;
	
			if (c == TFEND)
				c=FEND;

		}
		else
		{
			switch (c)
			{
			case FEND:		
	
				//
				//	Either start of message or message complete
				//
				
				if (pVCOMInfo->RXMPTR == (UCHAR *)&pVCOMInfo->RXMSG)
					continue;

				pVCOMInfo->MSGREADY=TRUE;
				return;

			case FESC:
		
				pVCOMInfo->ESCFLAG = TRUE;
				continue;

			}
		}
		
		//
		//	Ok, a normal char
		//

		*(pVCOMInfo->RXMPTR++) = c;

	}

	if (pVCOMInfo->RXMPTR - (UCHAR *)&pVCOMInfo->RXMSG > 500)
		pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
	
 	return;
}

static PVCOMINFO CreateInfo( int port,int speed, int bpqport )
{
   PVCOMINFO pVCOMInfo ;

   if (NULL == (pVCOMInfo =
                   (PVCOMINFO) LocalAlloc( LPTR, sizeof( VCOMINFO ) )))
      return ( (PVCOMINFO) -1 ) ;

 	pVCOMInfo->RXBCOUNT=0;
	pVCOMInfo->MSGREADY=FALSE;
	pVCOMInfo->RXBPTR=(UCHAR *)&pVCOMInfo->RXBUFFER; 
	pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
   
	pVCOMInfo->ComDev = 0 ;
	pVCOMInfo->Connected = FALSE ;
	pVCOMInfo->Port = port;

	VCOMInfo[bpqport]=pVCOMInfo;
	
	return (pVCOMInfo);
}

static BOOL NEAR DestroyTTYInfo( int port )
{
   PVCOMINFO pVCOMInfo ;

   if (NULL == (pVCOMInfo = VCOMInfo[port]))
      return ( FALSE ) ;

   LocalFree( pVCOMInfo ) ;

   VCOMInfo[port] = 0;

   return ( TRUE ) ;

} 

static int ReadCommBlock(PVCOMINFO  pVCOMInfo, LPSTR lpszBlock, DWORD nMaxLength)
{
	DWORD dwLength = 0;
	DWORD Available = 0;

	if (Win98)
		DeviceIoControl(pVCOMInfo->ComDev, (pVCOMInfo->Port << 16) | W98_SERIAL_GETDATA,
					NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);

	else if (pVCOMInfo->NewVCOM)
	{
		int ret = PeekNamedPipe(pVCOMInfo->ComDev, NULL, 0, NULL, &Available, NULL);

		if (ret == 0)
		{
			ret = GetLastError();

			if (ret == ERROR_BROKEN_PIPE)
			{
				CloseHandle(pVCOMInfo->ComDev);
				pVCOMInfo->ComDev = INVALID_HANDLE_VALUE;
				return 0;
			}
		}

		if (Available > nMaxLength)
			Available = nMaxLength;
		
		if (Available)
		{
			UCHAR * ptr1 = lpszBlock;
			UCHAR * ptr2 = lpszBlock;
			UCHAR c;
			int Length;
			
			ReadFile(pVCOMInfo->ComDev, lpszBlock, Available, &dwLength, NULL);

			// Have to look foro FF escape chars

			Length = dwLength;

			while (Length != 0)
			{
				c = *(ptr1++);
				Length--;

				if (c == 0xff)
				{
					c = c = *(ptr1++);
					Length--;
					
					if (c == 0xff)			// ff ff means ff
					{
						dwLength--;
					}
					else
					{
						// This is connection statua from other end

						dwLength -= 2;
						pVCOMInfo->NewVCOMConnected = c;
						continue;
					}
				}
				*(ptr2++) = c;
			}
		}
	}

	else
		DeviceIoControl(
			pVCOMInfo->ComDev,IOCTL_SERIAL_GETDATA,NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);

   return (dwLength);

}

static BOOL WriteCommBlock(int port, UCHAR * Message, DWORD MsgLen)
{
	ULONG bytesReturned;

//	if ((rand() % 100) > 80)
//		return 0;

	if (Win98)
		return DeviceIoControl(
			VCOMInfo[port]->ComDev,(VCOMInfo[port]->Port << 16) | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);

	else if (VCOMInfo[port]->NewVCOM)
	{
		// Have to escape all oxff chars, as these are used to get status info 

		UCHAR NewMessage[1000];
		UCHAR * ptr1 = Message;
		UCHAR * ptr2 = NewMessage;
		UCHAR c;

		int Length = MsgLen;

		while (Length != 0)
		{
			c = *(ptr1++);
			*(ptr2++) = c;

			if (c == 0xff)
			{
				*(ptr2++) = c;
				MsgLen++;
			}
			Length--;
		}

		return WriteFile(VCOMInfo[port]->ComDev, NewMessage, MsgLen, &bytesReturned, NULL);
	}
	else
		return DeviceIoControl(
			VCOMInfo[port]->ComDev,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);

}



