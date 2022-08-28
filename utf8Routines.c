/*
Copyright 2001-2018 John Wiseman G8BPQ

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

// Routines to convert to and from UTF8

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE


#ifndef WIN32

#define VOID void
#define BOOL int
#define TRUE 1
#define FALSE 0

#include <string.h>


#else
#include <windows.h>
#endif



unsigned int CP437toUTF8Data[128] = {
  34755, 48323, 43459, 41667,  
  42179, 41155, 42435, 42947,  
  43715, 43971, 43203, 44995,  
  44739, 44227, 33987, 34243,  
  35267, 42691, 34499, 46275,  
  46787, 45763, 48067, 47555,  
  49091, 38595, 40131, 41666,  
  41922, 42434, 10978018, 37574,  
  41411, 44483, 46019, 47811,  
  45507, 37315, 43714, 47810,  
  49090, 9473250, 44226, 48578,  
  48322, 41410, 43970, 48066,  
  9541346, 9606882, 9672418, 8557794,  
  10786018, 10589666, 10655202, 9868770,  
  9803234, 10720738, 9541090, 9934306,  
  10327522, 10261986, 10196450, 9475298,  
  9737442, 11834594, 11310306, 10261730,  
  8426722, 12358882, 10393058, 10458594,  
  10130914, 9737698, 11113954, 10917346,  
  10524130, 9475554, 11310562, 10982882,  
  11048418, 10786274, 10851810, 10065378,  
  9999842, 9606626, 9672162, 11245026,  
  11179490, 9999586, 9213154, 8951522,  
  8689378, 9213666, 9475810, 8427234,  
  45518, 40899, 37838, 32975,  
  41934, 33743, 46530, 33999,  
  42702, 39118, 43470, 46286,  
  10389730, 34511, 46542, 11110626,  
  10586594, 45506, 10848738, 10783202,  
  10521826, 10587362, 47043, 8948194,  
  45250, 10062050, 47042, 10127586,  
  12550626, 45762, 10524386, 41154,  
};
unsigned int CP437toUTF8DataLen[128] = {
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 3, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 3, 2, 2,  
  2, 2, 2, 2,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  3, 3, 3, 3,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  3, 2, 2, 3,  
  3, 2, 3, 3,  
  3, 3, 2, 3,  
  2, 3, 2, 3,  
  3, 2, 3, 2,  
};

unsigned int CP1251toUTF8Data[128] = {
  33488, 33744, 10125538, 37841,  
  10387682, 10911970, 10518754, 10584290,  
  11305698, 11567330, 35280, 12157154,  
  35536, 36048, 35792, 36816,  
  37585, 9994466, 10060002, 10256610,  
  10322146, 10649826, 9666786, 9732322,  
  39106, 10650850, 39377, 12222690,  
  39633, 40145, 39889, 40913,  
  41154, 36560, 40657, 35024,  
  42178, 37074, 42690, 42946,  
  33232, 43458, 34000, 43970,  
  44226, 44482, 44738, 34768,  
  45250, 45506, 34512, 38609,  
  37330, 46530, 46786, 47042,  
  37329, 9864418, 38097, 48066,  
  39121, 34256, 38353, 38865,  
  37072, 37328, 37584, 37840,  
  38096, 38352, 38608, 38864,  
  39120, 39376, 39632, 39888,  
  40144, 40400, 40656, 40912,  
  41168, 41424, 41680, 41936,  
  42192, 42448, 42704, 42960,  
  43216, 43472, 43728, 43984,  
  44240, 44496, 44752, 45008,  
  45264, 45520, 45776, 46032,  
  46288, 46544, 46800, 47056,  
  47312, 47568, 47824, 48080,  
  48336, 48592, 48848, 49104,  
  32977, 33233, 33489, 33745,  
  34001, 34257, 34513, 34769,  
  35025, 35281, 35537, 35793,  
  36049, 36305, 36561, 36817,  
};
unsigned int CP1251toUTF8DataLen[128] = {
  2, 2, 3, 2,  
  3, 3, 3, 3,  
  3, 3, 2, 3,  
  2, 2, 2, 2,  
  2, 3, 3, 3,  
  3, 3, 3, 3,  
  2, 3, 2, 3,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 3, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
};


unsigned int CP1252toUTF8Data[128] = {
  11305698, 33218, 10125538, 37574,  
  10387682, 10911970, 10518754, 10584290,  
  34507, 11567330, 41157, 12157154,  
  37573, 36290, 48581, 36802,  
  37058, 9994466, 10060002, 10256610,  
  10322146, 10649826, 9666786, 9732322,  
  40139, 10650850, 41413, 12222690,  
  37829, 40386, 48837, 47301,  
  41154, 41410, 41666, 41922,  
  42178, 42434, 42690, 42946,  
  43202, 43458, 43714, 43970,  
  44226, 44482, 44738, 44994,  
  45250, 45506, 45762, 46018,  
  46274, 46530, 46786, 47042,  
  47298, 47554, 47810, 48066,  
  48322, 48578, 48834, 49090,  
  32963, 33219, 33475, 33731,  
  33987, 34243, 34499, 34755,  
  35011, 35267, 35523, 35779,  
  36035, 36291, 36547, 36803,  
  37059, 37315, 37571, 37827,  
  38083, 38339, 38595, 38851,  
  39107, 39363, 39619, 39875,  
  40131, 40387, 40643, 40899,  
  41155, 41411, 41667, 41923,  
  42179, 42435, 42691, 42947,  
  43203, 43459, 43715, 43971,  
  44227, 44483, 44739, 44995,  
  45251, 45507, 45763, 46019,  
  46275, 46531, 46787, 47043,  
  47299, 47555, 47811, 48067,  
  48323, 48579, 48835, 49091,  
};
unsigned int CP1252toUTF8DataLen[128] = {
  3, 2, 3, 2,  
  3, 3, 3, 3,  
  2, 3, 2, 3,  
  2, 2, 2, 2,  
  2, 3, 3, 3,  
  3, 3, 3, 3,  
  2, 3, 2, 3,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
  2, 2, 2, 2,  
};

#ifdef __BIG_ENDIAN__
BOOL initUTF8Done = FALSE;
#else
BOOL initUTF8Done = TRUE;
#endif

VOID initUTF8()
{
	// Swap bytes of UTF-8 COde on Big-endian systems
	
	int n;
	char temp[4];
	char rev[4];

	if (initUTF8Done)
		return;
	
	for (n = 0; n <128; n++)
	{
		memcpy(temp, &CP437toUTF8Data[n], 4);
		rev[0] = temp[3];
		rev[1] = temp[2];
		rev[2] = temp[1];
		rev[3] = temp[0];

		memcpy(&CP437toUTF8Data[n], rev, 4);

		memcpy(temp, &CP1251toUTF8Data[n], 4);
		rev[0] = temp[3];
		rev[1] = temp[2];
		rev[2] = temp[1];
		rev[3] = temp[0];

		memcpy(&CP1251toUTF8Data[n], rev, 4);

		memcpy(temp, &CP1252toUTF8Data[n], 4);
		rev[0] = temp[3];
		rev[1] = temp[2];
		rev[2] = temp[1];
		rev[3] = temp[0];

		memcpy(&CP1252toUTF8Data[n], rev, 4);
	}

	initUTF8Done = TRUE;
}



int Is8Bit(unsigned char *cpt, int len)
{
	int n; 

	cpt--;
										
	for (n = 0; n < len; n++)
	{
		cpt++;
		
		if (*cpt > 127)
			return TRUE; 
	}

    return FALSE;
}


int IsUTF8(unsigned char *ptr, int len)
{
	int n; 
	unsigned char * cpt = ptr;

	// This has to be a bit loose, as UTF8 sequences may split over packets

	memcpy(&ptr[len], "\x80\x80\x80", 3);		// in case trailing bytes are in next packet

	// Don't check first 3 if could be part of sequence
	
	if ((*(cpt) & 0xC0) == 0x80)				// Valid continuation
	{
		cpt++;
		len--;
		if ((*(cpt) & 0xC0) == 0x80)			// Valid continuation
		{
			cpt++;
			len--;
			if ((*(cpt) & 0xC0) == 0x80)		// Valid continuation
			{
				cpt++;
				len--;
			}
		}
	}

	cpt--;
										
	for (n = 0; n < len; n++)
	{
		cpt++;
		
		if (*cpt < 128)
			continue;

		if ((*cpt & 0xF8) == 0xF0)
		{ // start of 4-byte sequence
			if (((*(cpt + 1) & 0xC0) == 0x80)
		     && ((*(cpt + 2) & 0xC0) == 0x80)
			 && ((*(cpt + 3) & 0xC0) == 0x80))
			{
				cpt += 3;
				n += 3;
				continue;
			}
			return FALSE;
	    }
		else if ((*cpt & 0xF0) == 0xE0)
		{ // start of 3-byte sequence
	        if (((*(cpt + 1) & 0xC0) == 0x80)
		     && ((*(cpt + 2) & 0xC0) == 0x80))
			{
				cpt += 2;
				n += 2;
				continue;
			}
			return FALSE;
		}
		else if ((*cpt & 0xE0) == 0xC0)
		{ // start of 2-byte sequence
	        if ((*(cpt + 1) & 0xC0) == 0x80)
			{
				cpt++;
				n++;
				continue;
			}
			return FALSE;
		}
		return FALSE;
	}

    return TRUE;
}

int WebIsUTF8(unsigned char *ptr, int len)
{
	int n; 
	unsigned char * cpt = ptr;

	// This is simpler than the Term version, as it only handles complete lines of text, so cant get split sequences

	cpt--;
										
	for (n = 0; n < len; n++)
	{
		cpt++;
		
		if (*cpt < 128)
			continue;

		if ((*cpt & 0xF8) == 0xF0)
		{ // start of 4-byte sequence
			if (((*(cpt + 1) & 0xC0) == 0x80)
		     && ((*(cpt + 2) & 0xC0) == 0x80)
			 && ((*(cpt + 3) & 0xC0) == 0x80))
			{
				cpt += 3;
				n += 3;
				continue;
			}
			return FALSE;
	    }
		else if ((*cpt & 0xF0) == 0xE0)
		{ // start of 3-byte sequence
	        if (((*(cpt + 1) & 0xC0) == 0x80)
		     && ((*(cpt + 2) & 0xC0) == 0x80))
			{
				cpt += 2;
				n += 2;
				continue;
			}
			return FALSE;
		}
		else if ((*cpt & 0xE0) == 0xC0)
		{ // start of 2-byte sequence
	        if ((*(cpt + 1) & 0xC0) == 0x80)
			{
				cpt++;
				n++;
				continue;
			}
			return FALSE;
		}
		return FALSE;
	}

    return TRUE;
}



int Convert437toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF)
{
	unsigned char * ptr1 = MsgPtr;
	unsigned char * ptr2 = UTF;
	int n;
	unsigned int c;

	for (n = 0; n < len; n++)
	{
		c = *(ptr1++);

		if (c < 128)
		{
			*(ptr2++) = c;
			continue;
		}

		memcpy(ptr2, &CP437toUTF8Data[c - 128], CP437toUTF8DataLen[c - 128]);
		ptr2 += CP437toUTF8DataLen[c - 128];
	}

	return (int)(ptr2 - UTF);
}

int Convert1251toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF)
{
	unsigned char * ptr1 = MsgPtr;
	unsigned char * ptr2 = UTF;
	int n;
	unsigned int c;

	for (n = 0; n < len; n++)
	{
		c = *(ptr1++);

		if (c < 128)
		{
			*(ptr2++) = c;
			continue;
		}

		memcpy(ptr2, &CP1251toUTF8Data[c - 128], CP1251toUTF8DataLen[c - 128]);
		ptr2 += CP1251toUTF8DataLen[c - 128];
	}

	return (int)(ptr2 - UTF);
}

int Convert1252toUTF8(unsigned char * MsgPtr, int len, unsigned char * UTF)
{
	unsigned char * ptr1 = MsgPtr;
	unsigned char * ptr2 = UTF;
	int n;
	unsigned int c;

	for (n = 0; n < len; n++)
	{
		c = *(ptr1++);

		if (c < 128)
		{
			*(ptr2++) = c;
			continue;
		}

		memcpy(ptr2, &CP1252toUTF8Data[c - 128], CP1252toUTF8DataLen[c - 128]);
		ptr2 += CP1252toUTF8DataLen[c - 128];
	}

	return (int)(ptr2 - UTF);
}

int TrytoGuessCode(unsigned char * Char, int Len)
{
	int Above127 = 0;
	int LineDraw = 0;
	int NumericAndSpaces = 0;
	int n;

	for (n = 0; n < Len; n++)
	{
		if (Char[n] < 65)
		{
			NumericAndSpaces++;
		}
		else
		{
			if (Char[n] > 127)
			{
				Above127++;
				if (Char[n] > 178 && Char[n] < 219)
				{
					LineDraw++;
				}
			}
		}
	}

	if (Above127 == 0)			// DOesn't really matter!
		return 1252;

	if (Above127 == LineDraw)
		return 437;			// If only Line Draw chars, assume line draw

	// If mainly below 128, it is probably Latin if mainly above, probably Cyrillic

	if ((Len - (NumericAndSpaces + Above127)) < Above127) 
		return 1251;
	else
		return 1252;
}
