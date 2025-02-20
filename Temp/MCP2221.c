// MCP2221.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include "winstdint.h"

#include "hidapi.h"
void DecodeCM108(int Port, char * ptr);

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
#include <windows.h>
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
	printf("err %ld: %s\n", err, buf);
}

#endif

HANDLE hDevice;

char * HIDDevice;

int main()
{
	int Len;
	unsigned char Msg[65] = "";

	DecodeCM108(0, "0x4D8:0xDD"); 

	hDevice = rawhid_open(HIDDevice);

	if (hDevice)
		printf("Rigcontrol HID Device %s opened\n", HIDDevice);

	Msg[0] = 0x51;
	Msg[0] = 0xB0;
	Msg[1] = 0x1;

	Len = rawhid_send(0, Msg, 64, 100);

	Msg[0] = 0;

#ifdef WIN32
	Len = rawhid_recv(0, Msg, 64, 100);
#else
	Len = read(PORT->hDevice, Msg, 64);
#endif


	return 0;
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

			devs = hid_enumerate(0, 0); // so we list devices(USHORT)VID, (USHORT)PID);
			cur_dev = devs;
			while (cur_dev)
			{
				wcstombs(product, cur_dev->product_string, 255);
				if (cur_dev->serial_number)
					wcstombs(sernum, cur_dev->serial_number, 255);

				if (product)
					printf("HID Device %s VID %X PID %X Ser %s %s\n", product, cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);
				else
					printf("HID Device %s VID %X PID %X Ser %s %s", "Missing Product\n", cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);

				if (cur_dev->vendor_id == VID && cur_dev->product_id == PID)
					path_to_open = cur_dev->path;

				cur_dev = cur_dev->next;
			}

			if (path_to_open)
			{
				HIDDevice = _strdup(path_to_open);
			}
			hid_free_enumeration(devs);
		}
#else

	// Linux - Next Param HID Device, eg /dev/hidraw0

		CM108Device = _strdup(ptr);
#endif
	}


