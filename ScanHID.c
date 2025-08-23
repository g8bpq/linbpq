// ScanHID.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include "hidapi.h"

int main(int argc, char* argv[])
{
	char product[256];
	char sernum[256] = "NULL";

	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;

	// Look for Device

	devs = hid_enumerate(0,0); // so we list devices(USHORT)VID, (USHORT)PID);
	cur_dev = devs;
	while (cur_dev)
	{
		wcstombs(product, cur_dev->product_string, 255);
		if (cur_dev->serial_number)
			wcstombs(sernum, cur_dev->serial_number, 255);

		if (product)
			printf("HID Device %s VID %X PID %X Ser %s\r\n Path %s\r\n\r\n", product, cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);
		else
			printf("HID Device %s VID %X PID %X Ser %s\r\n Path %s\r\n\r\n", "Missing Product", cur_dev->vendor_id, cur_dev->product_id, sernum, cur_dev->path);


		cur_dev = cur_dev->next;
	}


	hid_free_enumeration(devs);
	printf("Press any key to Exit");
	_getch();


}




