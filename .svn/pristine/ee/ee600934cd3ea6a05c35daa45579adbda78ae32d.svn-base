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

// Module for writing ADIF compatible log records

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include "cheaders.h"
#include "tncinfo.h"
#include "adif.h"
#include "telnetserver.h"

/*

Typical File Header

Written by RMS Trimode ver: 1.3.21.0   <adif_ver:5>2.2.6<eoh>

Example TriMode Record

<call:6>VA2ROR
<qso_date:8>20190201
<time_on:6>140801
<time_off:6>140958
<mode:0>
<gridsquare:6>JG28DK
<band:3>20M
<freq:9>14.109000
<comment:145>0|2019-02-01 14:09:58|RMS Trimode|1.3.21.0|CMS|ZS1RS|VA2ROR|[AirMail-3.5.036-B2FHIM$]
   |Pactor 3|14109000|1957|311|FQ|1|0|4098|93|117|JG28DK|JF96HD
<eor>

	Info Used by RMS Analyser, Logged as an adif  <comment> field

0|<date time>|<program name>|<program version>|<name of cms site used>|<rms site callsign>|<user
callsign>|<user sid>|<mode>|<frequency>|<range>|<bearing>|<termination>|<messages sent>|<messages
received>|<bytes sent>|<bytes received>|<holding time>

<date time> in the format YYYY-MM-DD HH:MM:SS and always in UTC.
<program name> is the name of the program posting this record.
<program version> is the version number of the program.
<name of CMS site used> is the city name assigned to the CMS.
<rms site callsign> is the radio callsign used by the RMS server including any SSID.
<user callsign> is the radio callsign used by the user including any SSID.
<user sid> is the SID used by the user's client program.
<mode> is the air protocol used such as "Pactor 3" or "Packet 1200".
At the present time modes used or expected include Pactor 1, Pactor 2, Pactor 3, Pactor 4, Packet 1200,
Packet 9600, Winmor 500, Winmor 1600, and Robust Packet.
<frequency> is the frequency used in Hertz.

<range> if the distance to the user's site (if known).
<bearing> is the bearing to the user's site (if known).
<termination> is the last command sent or received.
Normally there will be "FQ" but could be "FF", "FC" "PR", "PQ", "FS", "F>", or ">" or one of the keyboard
commands. It should limited to one or two characters.
<messages sent> through <holding time> are integers and may be 0 but not empty.
An example:

0|2011-05-20 12:34.56|RMS Packet|1.2.3.4|San Diego|KN6KB-10|W1ABC|
[Paclink-4.2.0.0-N00B2FIHM$]| Packet 1200|145090000|45|244|FQ|
1|0|2454|120|135

*/

VOID FromLOC(char * Locator, double * pLat, double * pLon);
double Distance(double laa, double loa, double lah, double loh, BOOL KM);
double Bearing(double lat2, double lon2, double lat1, double lon1);

extern UCHAR LogDirectory[260];
extern char LOC[7];
extern char TextVerstring[50];
struct WL2KInfo * WL2KReports;

extern char WL2KModes[55][18];

BOOL ADIFLogEnabled = FALSE;

char ADIFLogName[80] = "ADIF.adi";

char * stristr (char *ch1, char *ch2);

VOID CountMessages(ADIF * ADIF)
{
	while (ADIF->FBBIndex)
	{
		// We have messages

		if (ADIF->FBBLen[ADIF->FBBIndex - 1])		// don't count rejected messages
		{
			if (ADIF->Dirn == 'S')
			{
				ADIF->Sent++;
				ADIF->BytesSent += ADIF->FBBLen[ADIF->FBBIndex - 1];
			}
			else
			{
				ADIF->Received++;
				ADIF->BytesReceived += ADIF->FBBLen[ADIF->FBBIndex - 1];
			}
		}
		ADIF->FBBIndex--;
	}
}


BOOL UpdateADIFRecord(ADIF * ADIF, char * Msg, char Dirn)
{
	// Entered each time a text message is received on CMS session

	// To get transfer stats we have to monitor the B2 protocol messages, tracking proposals, accept/reject and completion

	size_t Len;

	// Always keep info so we can sent as Winlink Session Record
	// even if ADIF logging is disabled.

//	if (ADIFLogEnabled == FALSE)
//		return TRUE;

	if (ADIF == NULL)
		return TRUE;

	if (ADIF->StartTime == 0)
	{
		ADIF->StartTime = time(NULL);
//		ADIF->Mode = 49;				// Unused value (unforunately 0 = PKT1200)
	}

	// Try to build complete lines

	if (Dirn == 'R')
	{
		Len = strlen(&ADIF->PartMessageRX[0]);

		if (Len + strlen(Msg) < 512)
			strcat(ADIF->PartMessageRX, Msg);

		if (strstr(Msg, "<cr>"))
			Msg = ADIF->PartMessageRX;
		else
			return TRUE;
	}
	else
	{
		Len = strlen(&ADIF->PartMessageTX[0]);

		if (Len + strlen(Msg) < 256)
			strcat(ADIF->PartMessageTX, Msg);

		if (strstr(Msg, "<cr>"))
			Msg = ADIF->PartMessageTX;
		else
			return TRUE;
	}

	switch (Dirn)
	{
	case 'R':

		if (Msg[0] == '[')
		{
			if (ADIF->ServerSID[0] == 0)
			{
				char * endsid = strchr(Msg, ']');

				if (endsid && (endsid - Msg) < 78)
					memcpy(ADIF->ServerSID, Msg, ++endsid - Msg);
			}
			Msg[0] = 0;
			return TRUE;
		}

		break;

	case 'S':

		if (Msg[0] == '[')
		{
			if (ADIF->UserSID[0] == 0)
			{
				char * endsid = strchr(Msg, ']');

				if (endsid && (endsid - Msg) < 78)
					memcpy(ADIF->UserSID, Msg, ++endsid - Msg);
			}

			Msg[0] = 0;		
			return TRUE;
		}

		if (ADIF->LOC[0] == 0 && Msg[0] == ';' && stristr(Msg, "DE "))
		{
			// Look for ; GM8BPQ-10 DE G8BPQ (IO92KX)
			// AirMail EA8URF de KG5VSG (GK86qo) QTC: 1 209 


			// Paclink-Unix Sends

			//  ; VE7SPR-10 DE N7NIX QTC 1

			char * StartLoc = strchr(Msg, '(');
			char * EndLoc = strchr(Msg, ')');

			if (StartLoc == NULL || EndLoc == NULL)
				return TRUE;

			if ((EndLoc - StartLoc) < 10)
				memcpy(ADIF->LOC, StartLoc + 1, (EndLoc - StartLoc) - 1);

			Msg[0] = 0;
			return TRUE;
		}
	}

	// Look for Proposals

	// FC EM 3909_GM8BPQ 3520 1266 0<cr>

	if (memcmp(Msg, "FC EM ", 6) == 0)
	{
		char * ptr, *Context;

		// We need to detect first of a block of proposals so we can count any acked messages

		if (ADIF->GotFC == 0)			// Last was not FC
			CountMessages(ADIF);		// This acks last batch

		ADIF->GotFC = 1;

		ptr = strtok_s(&Msg[6], " \r", &Context);		// BID

		if (ptr)
		{
			ptr = strtok_s(NULL, " \r", &Context);

			if (ptr)
			{
				ADIF->FBBLen[ADIF->FBBIndex] = atoi(ptr);		// Not really sure we need lengths, but no harm

				if (ADIF->FBBIndex++ == 5)
					ADIF->FBBIndex = 4;					// Proect ourselves

			}

			ADIF->Dirn = Dirn;
		}
		strcpy(ADIF->Termination, "FC");
		Msg[0] = 0;
		return TRUE;
	}

	if (memcmp(Msg, "FS ", 3) == 0)
	{
		// As we only count sent messages must check for rejections;

		// FS YYY<cr>

		int i = 0;
		char c;

		ADIF->GotFC = 0;				// Ready for next batch

		while (i < ADIF->FBBIndex)
		{
			c = Msg[i + 3];

			if ((c == '-') || (c == 'N') || (c == 'R') || (c == 'E'))				// Not wanted
			{
				ADIF->FBBLen[i] = 0;			// Clear corresponding length
			}
			i++;
		}

		strcpy(ADIF->Termination, "FS");
		Msg[0] = 0;
		return TRUE;
	}

	if (strcmp(Msg, "FF<cr>") == 0 || strcmp(Msg, "FQ<cr>") == 0)
	{
		// Need to count any complete messages

		CountMessages(ADIF);			// This acks last batch

		memcpy(ADIF->Termination, Msg, 2);
		Msg[0] = 0;
		return TRUE;
	}


	if (Msg[0] == ';' && Msg[3] == ':' )
		memcpy(ADIF->Termination, &Msg[1],2);

	Msg[0] = 0;
	return TRUE;
}

typedef struct BandLimits
{
	char Name[10];
	double lower;
	double upper;
} BandLimits;

BandLimits Bands[] = 
{
	{"2190m", .1357, .1378},
	{"630m", .472, .479},
	{"560m", .501, .504},
	{"160m", 1.8, 2.0}, 
	{"80m", 3.5, 4.0},
	{"60m", 5.06, 5.45},
	{"40m", 7.0, 7.3},
	{"30m", 10.1, 10.15},
	{"20m", 14.0, 14.35},
	{"17m", 18.068, 18.168},
	{"15m", 21.0, 21.45},
	{"12m", 24.890, 24.99},
	{"10m", 28.0, 29.7},
	{"6m", 50, 54},
	{"4m", 70, 72},
	{"2m", 144, 148},
	{"1.25m", 222, 225},
	{"70cm", 420, 450},
	{"33cm", 902, 928},
	{"23cm", 1240, 1300},
	{"13cm", 2300, 2450},
	{"9cm", 3300, 3500},
	{"6cm", 5650, 5925},
	{"3cm", 10000, 10500},
	{"6mm", 47000, 47200},
	{"4mm", 75500, 81000},
	{"2.5mm", 119980, 120020},
	{"2mm", 142000, 149000},
	{"1mm", 241000, 250000},
	{"unknown", 0, 9999999999}
};

int FreqCount = sizeof(Bands)/sizeof(struct BandLimits);

char ADIFModes [55][18] = {
	"PKT", "PKT", "PKT", "PKT", "PKT", "PKT", "PKT", "", "", "", // 0 - 9
	"", "PAC", "", "", "PAC/PAC2", "", "PAC/PAC3", "", "", "", "PAC/PAK4", // 10 - 20
	"WINMOR", "WINMOR", "", "", "", "", "", "", "",				// 21 - 29
	"Robust Packet", "", "", "", "", "", "", "", "", "",					// 30 - 39
	"ARDOP", "ARDOP", "ARDOP", "ARDOP", "ARDOP", "", "", "", "", "",	// 40 - 49
	"VARA", "VARAFM", "VARAFM96", "VARA500", "VARA2750"};


BOOL WriteADIFRecord(ADIF * ADIF)
{
	UCHAR Value[MAX_PATH];
	time_t T;
	struct tm * tm;
	struct tm * starttm;
	struct tm endtm;

	FILE * Handle;
	char Comment[256] = "";
	int CommentLen;
	char Date[32];
	int Dist = 0;
	int intBearing = 0;
	struct stat STAT;

	double Lat, Lon;
	double myLat, myLon;

	if (ADIFLogEnabled == FALSE)
		 return TRUE;

	if (ADIF == NULL)
		return TRUE;

	T = time(NULL);
	tm = gmtime(&T);

	memcpy(&endtm, tm, sizeof(endtm)); 

	if (LogDirectory[0] == 0)
	{
		strcpy(Value, "logs/BPQ_CMS_ADIF");
	}
	else
	{
		strcpy(Value, LogDirectory);
		strcat(Value, "/");
		strcat(Value, "logs/BPQ_CMS_ADIF");
	}

	sprintf(&Value[strlen(Value)], "_%04d%02d.adi", tm->tm_year +1900, tm->tm_mon+1);

	STAT.st_size = 0;
	stat(Value, &STAT);

	Handle = fopen(Value, "ab");

	if (Handle == NULL)
		return FALSE;

	if (STAT.st_size == 0)
	{
		// New File - Write Header

		char Header[256];
		int Len;

		Len = sprintf(Header, "Written by BPQ32  ver: %s <adif_ver:5>2.2.6<eoh>\r\n", TextVerstring);
		fwrite(Header, 1, Len, Handle);
	}

	// Extract Info we need

	// Distance and Bearing

	if (LOC[0] && ADIF->LOC[0])
	{
		FromLOC(LOC, &myLat, &myLon);
		FromLOC(ADIF->LOC, &Lat, &Lon);

		Dist = (int)Distance(myLat, myLon, Lat, Lon, 0);
		intBearing = (int)Bearing(Lat, Lon, myLat, myLon);
	}

	starttm = gmtime(&ADIF->StartTime);

	//<call:6>VA2ROR

	fprintf(Handle, "<call:%d>%s", (int)strlen(ADIF->Call), ADIF->Call);

	//<qso_date:8>20190201
	//<time_on:6>140801
	//<time_off:6>140958

	fprintf(Handle, "<qso_date:8>%04d%02d%02d<time_on:6>%02d%02d%02d<time_off:6>%02d%02d%02d",
		starttm->tm_year + 1900, starttm->tm_mon + 1, starttm->tm_mday,
		starttm->tm_hour, starttm->tm_min, starttm->tm_sec,
		endtm.tm_hour, endtm.tm_min, endtm.tm_sec);

//<mode:0>

	if (ADIFModes[ADIF->Mode][0])
	{
		// Send Mode, and Submode if present

		char Mode[32];
		char * SubMode;

		strcpy(Mode, ADIFModes[ADIF->Mode]);
		SubMode = strlop(Mode, '/');

		fprintf(Handle, "<mode:%d>%s", (int)strlen(Mode), Mode);
		if (SubMode)
			fprintf(Handle, "<submode:%d>%s", (int)strlen(SubMode), SubMode);

	}
	//<gridsquare:6>JG28DK. Doc wants it even if empty

	fprintf(Handle, "<gridsquare:%d>%s", (int)strlen(ADIF->LOC), ADIF->LOC);
	

	//<band:3>20M
	//<freq:9>14.109000
	
	if (ADIF->Freq > 1500)
	{
		char Freqstr[32];
		int i = 0;
		BandLimits * Band = &Bands[0];
		double Freq = ADIF->Freq / 1000000.0;

		while (i++ < FreqCount)
		{
			if (Band->lower <= Freq && Band->upper >= Freq)
				break;

			Band++;
		}

		sprintf(Freqstr, "%.6f", ADIF->Freq / 1000000.0);
		fprintf(Handle, "<band:%d>%s", (int)strlen(Band->Name), Band->Name);
		fprintf(Handle, "<freq:%d>%s", (int)strlen(Freqstr), Freqstr);
	}
	else
		fprintf(Handle, "<band:0><freq:0>");


 


	// Do Comment

	//0|2019-02-01 14:09:58|RMS Trimode|1.3.21.0|CMS|ZS1RS|VA2ROR|[AirMail-3.5.036-B2FHIM$]
   //|Pactor 3|14109000|1957|311|FQ|1|0|4098|93|117|JG28DK|JF96HD

	sprintf	(Date, "%04d-%02d-%02d %02d:%02d:%02d", 
		endtm.tm_year + 1900, endtm.tm_mon + 1, endtm.tm_mday, endtm.tm_hour, endtm.tm_min, endtm.tm_sec);

	CommentLen = sprintf(Comment, "0|%s|%s|%s|%s|%s|%s|%s|%s|%lld|%d|%d|%s|%d|%d|%d|%d|%d|%s|%s",
		Date,
		"BPQ32",
		TextVerstring,
		"CMS",
		ADIF->CMSCall, 
		ADIF->Call,
		ADIF->UserSID,
		WL2KModes[ADIF->Mode],
		ADIF->Freq,
		Dist,
		intBearing,
		ADIF->Termination,
		ADIF->Sent,
		ADIF->Received,
		ADIF->BytesSent,
		ADIF->BytesReceived,
		(int)(T - ADIF->StartTime),
		ADIF->LOC,
		LOC);
		
	fprintf(Handle, "<comment:%d>%s", CommentLen, Comment);
	fprintf(Handle, "<eor>\r\n");
	fclose(Handle);

	return TRUE;
}

VOID ADIFWriteFreqList()
{
	// Write info needed for RMS Analyser to a file in similar format to Trimode

	UCHAR Value[MAX_PATH];
	FILE * Handle;
	struct WL2KInfo * WL2KReport = WL2KReports;
	char Locator[16];
	char Call[16];
	int i, freqCount = 0;
	long long Freqs[100] = {0};

	if (WL2KReport == NULL)
		return;

	if (LogDirectory[0] == 0)
	{
		strcpy(Value, "logs/BPQAnalyser.ini");
	}
	else
	{
		strcpy(Value, LogDirectory);
		strcat(Value, "/");
		strcat(Value, "logs/BPQAnalyser.ini");
	}

	Handle = fopen(Value, "wb");

	if (Handle == NULL)
		return;

	while (WL2KReport)
	{
		strcpy(Call, WL2KReport->BaseCall);
		strcpy(Locator, WL2KReport->GridSquare);

		// if freq not in list add it

		i = 0;

		while (i < freqCount)
		{
			if (WL2KReport->Freq == Freqs[i])
				break;

			i++;

		}

		if (i == freqCount)
			Freqs[freqCount++] = WL2KReport->Freq;


		WL2KReport = WL2KReport->Next;
	}

	fprintf(Handle, "[Site Properties]\r\n");
	fprintf(Handle, "Grid Square=%s\r\n", Locator);
	fprintf(Handle, "[Registration]\r\n");
	fprintf(Handle, "Base Callsign=%s\r\n", Call);

	fprintf(Handle, "[Channels]\r\n");

	for (i = 0; i < freqCount; i++)
		fprintf(Handle, "Frequency %d=%lld\r\n" , i + 1, Freqs[i]); 

	fclose(Handle);
}
