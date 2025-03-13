
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "time.h"
#include "cheaders.h"
//#include "tncinfo.h"
//#include "adif.h"
//#include "telnetserver.h"

VOID __cdecl Debugprintf(const char * format, ...);

#define TRACKPOINTS 50

typedef struct TARGETRECORD
{  
	UINT ID;
    char name[21];
    char Callsign[8];
    char Dest[21];
    UINT IMO;
    double ROT;
    int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int Draft;
    double lat;
    double Long;
    double course;
    double speed;
    double Heading;
    int NavStatus;
    UINT TCA;
    UINT BCA;							// Bearing at closest
    UINT TimeAdded;
    UINT TimeLastUpdated;
	char Type;
	char Class;

	// Fields for Intellegent TrackLog

	double LatTrack[TRACKPOINTS];	// Cyclic Tracklog
	double LonTrack[TRACKPOINTS];
	time_t TrackTime[TRACKPOINTS];
	int Trackptr;					// Next record in Tracklog

	// info about last track point written

	double Lastlat;
    double LastLong;
	double LastCourse;				
    double LastSpeed;

 	time_t LastTime;

	int Alt;							// For SAR Aircraft

     
} TargetRecord;

typedef struct NAVAIDRECORD
{
	UINT ID;
    char name[21];
	int NavAidType;
	double lat;
    double Long;
	int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int FixType;
    time_t TimeAdded;
    time_t TimeLastUpdated;

} NavaidRecord;

typedef struct  BASESTATIONRECORD
{
	UINT ID;
    int FixType;
	double lat;
    double Long;
	int NumberofStations;
    time_t TimeAdded;
    time_t TimeLastUpdated;

} BaseStationRecord;

typedef struct SHIPDATABASERECORD
{  
	UINT ID;
    char name[21];
    char Callsign[8];
    int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int Draft;
    time_t TimeAdded;
    time_t TimeLastUpdated;
 	char Type;
	char Class;

} ShipDatabaseRecord;

void LookupVessel(UINT UserID);
void LoadVesselDataBase();
void SaveVesselDataBase();
void LoadNavAidDataBase();
void SaveNavAidDataBase();


typedef struct SARRECORD
{  
	UINT ID;
    char name[21];
    char Callsign[8];
    char Dest[21];
    UINT IMO;
    double ROT;
    int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int Draft;
    double lat;
    double Long;
    double course;
    double speed;
    double Heading;
    int NavStatus;
    UINT TCA;
    UINT BCA;							// Bearing at closest
    time_t TimeAdded;
    time_t TimeLastUpdated;
	char Type;
	char Class;

	// Fields for Intellegent TrackLog

	double LatTrack[TRACKPOINTS];	// Cyclic Tracklog
	double LonTrack[TRACKPOINTS];
	time_t TrackTime[TRACKPOINTS];
	int Trackptr;					// Next record in Tracklog

	// info about last track point written

	double Lastlat;
    double LastLong;
	double LastCourse;				
    double LastSpeed;

 	time_t LastTime;

	int Alt;							// For SAR Aircraft

       
} SARRecord;


typedef struct ADSBRECORD
{
	char hex[8];		//the 24-bit ICAO identifier of the aircraft, as 6 hex digits. The identifier may
						//start with '~', this means that the address is a non-ICAO address (e.g. from TIS-B).
	char squawk[5];		// the 4-digit squawk (octal representation)
	char flight[32];	// the flight name / callsign
	double lat;
	double lon;			// the aircraft position in decimal degrees
	// nucp: the NUCp (navigational uncertainty category) reported for the position
	int seen_pos;		// how long ago (in seconds before "now") the position was last updated
	int altitude;		// the aircraft altitude in feet, or "ground" if it is reporting it is on the ground
	int vert_rate;		// vertical rate in feet/minute
	int track;			// true track over ground in degrees (0-359)
	int speed;			// reported speed in kt. This is usually speed over ground, but might be IAS - you can't tell the difference here, sorry!
	int messages;		// total number of Mode S messages received from this aircraft
	int seen;			// how long ago (in seconds before "now") a message was last received from this aircraft
	int rssi;			// recent average RSSI (signal power), in dbFS; this will always be negative.

	time_t TimeAdded;
	time_t TimeLastUpdated;

	// Fields for Intellegent TrackLog

	double LatTrack[TRACKPOINTS];	// Cyclic Tracklog
	double LonTrack[TRACKPOINTS];
	time_t TrackTime[TRACKPOINTS];
	int Trackptr;					// Next record in Tracklog

	// info about last track point written

	double Lastlat;
    double LastLong;
	int LastCourse;				
    int LastSpeed;
 	time_t LastTime;

} ADSBRecord;

extern BOOL TraceAIS;
extern BOOL NoVDO;
extern BOOL NoAlarms;


UINT PaintColour;
//char Msg[1000];
//UINT Colour;
char CloseString[100];
//extern int ListItem;


SOCKET udpsock;

SOCKADDR_IN sinx; 

SOCKADDR_IN rxaddr, txaddr, pitxaddr;

double pi = 3.14159265389754;

double OurSog, OurCog, OurLatitude, OurLongtitude;
double OurLatIncr, OurLongIncr;

unsigned int UserID;
int NavStatus;
double V_Lat, V_Lon, V_COG, V_SOG, V_ROT;
char V_Name[21], V_Callsign[8], V_Dest[21], V_VendorID[8];  
int V_IMO, V_Heading, V_Type, B_PartNo;
int Msg_Type, Accuracy, FixType;
int UTCyear, UTCMonth, UTCDay, UTCHour, UTCMinute, UTCSecond;
int NavAidType, V_Draught, ETA;
int Alt;

int V_LenA, V_LenB, V_WidthC, V_WidthD;
int SyncState, SlotTO, SubMsg;

int GpsOK;
int DecayTimer;
static int MaxAge = 7200;		// 2 Hours

int VessselDBChanged = 0;
int NavAidDBChanged = 0;

BOOL SoundActive;

extern BOOL KeepTracks;

HANDLE TrackHandle;
HANDLE hTrack;
UINT LastTrackFileTime;

UINT NOW;

char Stack[500];

BOOL Stacked=FALSE;


int NavAidCount=0;

struct NAVAIDRECORD ** NavRecords;

int SARCount=0;
struct SARRECORD ** SARRecords;

int TargetCount=0;

struct TARGETRECORD ** TargetRecords;

int BaseStationCount=0;

struct BASESTATIONRECORD ** BaseStationRecords;

int ADSBCount = 0;

struct ADSBRECORD ** ADSBRecords;

UCHAR conv[256]={99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,	// 00
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
				  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,	// 30

				 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,	// 40 @
				 32, 33, 34, 35, 36, 37, 38, 39, 40, 33, 34, 35, 36, 37, 38, 39,	// 50 P
				 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,	// 60
				 56, 57, 58, 59, 60, 61, 62, 63,  0,  1,  2,  3,  4,  5,  6,  7,	// 77 = 

				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,

				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
				 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

char *NavAidString[] = {
	"Default, Type of A to N not specified",
	"Reference Point",
	"RACON",
	"Structure Off shore Structure",
	"Spare",
	"Light, without sectors",
	"Light, with sectors",
	"Leading Light Front",
	"Leading Light Rear",
	"Beacon, Cardinal N",
	"Beacon, Cardinal E",
	"Beacon, Cardinal S",
	"Beacon, Cardinal W",
	"Beacon, Port hand",
	"Beacon, Starboard hand",
	"Beacon, Preferred Channel port hand",
	"Beacon, Preferred Channel starboard hand",
	"Beacon, Isolated danger",
	"Beacon, Safe water",
	"Beacon, Special mark",
	"Cardinal Mark N",
	"Cardinal Mark E",
	"Cardinal Mark S",
	"Cardinal Mark W",
	"Port hand Mark",
	"Starboard hand Mark",
	"Preferred Channel Port hand",
	"Preferred Channel Starboard hand",
	"Isolated danger",
	"Safe Water",
	"Special Mark",
	"Light Vessel / LANBY / Rigs"};

char * FixTypeString[] = {
	"Undefined",
	"GPS",
	"GLONASS",
	"Combined GPS/GLONASS",
	"Loran - C",
	"Chayka",
	"Integrated Navigation System",
	"surveyed"};


char * NavStatusString[] = {
	"Under way using engine",
	"At anchor",
	"Not under command",
	"Restricted manoeuvrability",
	"Constrained by draught",
	"Moored",
	"Aground",
	"Engaged in Fishing",
	"Under way sailing",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"Not defined"};

char * VesselType[] = {	// First Digit
	"Undefined",
	"Reserved for future use",
	"WIG",						// 2 
	"",							// 3
	"HSC",
	"",
	"Passenger ship",
	"Cargo ship",
	"Tanker",
	"Other types of ship"};

char * VesselType3[] = {
	"Fishing",
	"Towing",
	"Towing Len > 200 m or breadth > 25 m",
	"Dredging or underwater operations",
	"Diving",
	"Military",
	"Sailing",
	"Pleasure Craft",
	"Reserved",
	"Reserved"};

char * VesselType5[] = {
	"Pilot vessel",
	"Search and rescue vessel",
	"Tug",
	"Port tender",
	"Vessel with anti-pollution facilities or equipment",
	"Law enforcement vessel",
	"Spare",
	"Spare",
	"Medical transport",
	"Ships according to Resolution No 18 (Mob-83)"};
		
extern struct SHIPDATABASERECORD * ShipDataRec;

int ACount = 0, BCount = 0;

char ADSBHost[64] = ""; //"10.8.0.50";
int ADSBPort = 30003;
int ADSBConnected = 0;

extern double Lat;
extern double Lon;


// Forward declarations of functions included in this code module:


double radians(double Degrees);
double degrees(double radians);
BOOL Check0183CheckSum(char * msg,int len);
int ProcessNMEAMessage(char * msg, int len);
int DecodeVDM(char * Data);
int ConvertAsciito6Bit(char * msg, char * buffer, int Len);
void ProcessAISVesselMessage();
void ProcessSARMessage();
void ProcessBaseStationMessage();
void ProcessAISVesselDetails(char Type);
void ProcessAISNavAidMessage();
void MM_Reinit();
void CalculateClosestApproach(struct TARGETRECORD * ptr);
static double Distance(double lah, double loh, double laa, double loa);
static double Bearing(double lat1, double lon1, double lat2, double lon2);
double CalcClosest(struct TARGETRECORD * ptr);
void RefreshTargetList();
void RefreshTargetSub(struct TARGETRECORD * ptr, int j);
void CheckAgeofTargets(UINT MaxAge);
void ActivateTarget();
void Write_Target_Track(struct TARGETRECORD * ptr);
void Write_Our_Track();
void MaintainVesselDatabase(UINT UserID, char Type);
VOID AutoPilotUpdate();
VOID ProcessNMEAMessageCE(char * msg, int len);
void ProcessADSBJSON(char * FN);
void CheckAgeofPlanes(UINT MaxAge);
static VOID ADSBConnect(void * unused);

char ADSBFN[256] = "/dev/shm/adsb/aircraft.json";

char DataBasefn[MAX_PATH];

int ShipDatabaseCount=0;

struct SHIPDATABASERECORD ** ShipDatabaseRecords;

struct SHIPDATABASERECORD * ShipDataRec;

void SaveTrackPoint(struct TARGETRECORD * ptr)
{
	// If course and speed have changed, and distance travelled or time is significant, add a track point to log

	if ((NOW - ptr->LastTime) < 15)	// Not More that once per 15 secs
		return;

	if (fabs(ptr->LastCourse - ptr->course) < 1.0  &&  fabs(ptr->LastSpeed - ptr->speed) < 0.2 && ptr->speed > 0.2) // if very slow, COG may be random
	{
		// Steady Course - update once per 5 minutes if moving
		
		if ((NOW - ptr->LastTime) > 600 && ptr->speed != 0) 
			goto writeRec;
	}

	if (fabs(ptr->Lastlat - ptr->lat) < 0.01  &&  fabs(ptr->LastLong - ptr->Long) < 0.01) // Not moved much
		return;

writeRec:

	ptr->LastCourse = ptr->course;
	ptr->LastSpeed = ptr->speed;
	ptr->LastTime = NOW;
	ptr->Lastlat = ptr->lat;
	ptr->LastLong = ptr->Long;

	ptr->LatTrack[ptr->Trackptr] = ptr->lat;
	ptr->LonTrack[ptr->Trackptr] = ptr->Long;
	ptr->TrackTime[ptr->Trackptr] = NOW;

	ptr->Trackptr++;
	
	if (ptr->Trackptr == TRACKPOINTS)
		ptr->Trackptr = 0;

}


void SaveSARTrackPoint(struct SARRECORD * ptr)
{
	// If course and speed have changed, and distance travelled or time is significant, add a track point to log

	if ((NOW - ptr->LastTime) < 15)	// Not More that once per 15 secs
		return;

	if (fabs(ptr->LastCourse - ptr->course) < 1.0  &&  fabs(ptr->LastSpeed - ptr->speed) < 0.2 && ptr->speed > 0.2) // if very slow, COG may be random
	{
		// Steady Course - update once per 5 minutes if moving
		
		if ((NOW - ptr->LastTime) > 600 && ptr->speed != 0) 
			goto writeRec;
	}

	if (fabs(ptr->Lastlat - ptr->lat) < 0.01  &&  fabs(ptr->LastLong - ptr->Long) < 0.01) // Not moved much
		return;

writeRec:

	ptr->LastCourse = ptr->course;
	ptr->LastSpeed = ptr->speed;
	ptr->LastTime = NOW;
	ptr->Lastlat = ptr->lat;
	ptr->LastLong = ptr->Long;

	ptr->LatTrack[ptr->Trackptr] = ptr->lat;
	ptr->LonTrack[ptr->Trackptr] = ptr->Long;
	ptr->TrackTime[ptr->Trackptr] = NOW;

	ptr->Trackptr++;
	
	if (ptr->Trackptr == TRACKPOINTS)
		ptr->Trackptr = 0;

}



void MaintainVesselDatabase(UINT UserID, char Type)
{
	LookupVessel(UserID);								// Find Vessel - if not present, add it

	if (ShipDataRec == NULL) return;					// Malloc failed!!

	if (Type == 'A') ShipDataRec->Class='A'; else ShipDataRec->Class='B';
	
	if (Type == 'A')
	{
		memcpy(ShipDataRec->name, V_Name, 21);
		memcpy(ShipDataRec->Callsign,V_Callsign,8);
		ShipDataRec->Type = V_Type;
		ShipDataRec->LengthA = V_LenA;
		ShipDataRec->LengthB = V_LenB;
		ShipDataRec->WidthC = V_WidthC;
		ShipDataRec->WidthD = V_WidthD;
	}

	if (Type == 'B')					// Type 19 Class B Details
	{
		memcpy(ShipDataRec->name, V_Name, 21);
		ShipDataRec->Type = V_Type;
		ShipDataRec->LengthA = V_LenA;
		ShipDataRec->LengthB = V_LenB;
		ShipDataRec->WidthC = V_WidthC;
		ShipDataRec->WidthD = V_WidthD;
	}

	if (Type == '0')				// Class B record 24A
	{
		memcpy(ShipDataRec->name, V_Name, 21);
	}

	if (Type == '1')				// Class B record 24B
	{
		memcpy(ShipDataRec->Callsign,V_Callsign,8);
		ShipDataRec->Type = V_Type;
		ShipDataRec->LengthA = V_LenA;
		ShipDataRec->LengthB = V_LenB;
		ShipDataRec->WidthC = V_WidthC;
		ShipDataRec->WidthD = V_WidthD;
	}
		
	ShipDataRec->TimeLastUpdated = NOW;

	VessselDBChanged = 1;					// Save on next timer tick
	return;

} 

void LookupVessel(UINT UserID)
{
	int i;

	for (i = 0; i < ShipDatabaseCount; i++)
	{
		ShipDataRec=ShipDatabaseRecords[i];
 
	    if (ShipDataRec->ID == UserID) return;
	}
 
//   Not found

	if (ShipDatabaseCount == 0)
		ShipDatabaseRecords=(struct SHIPDATABASERECORD **)malloc(sizeof(void *));
	else
		ShipDatabaseRecords=(struct SHIPDATABASERECORD **)realloc(ShipDatabaseRecords,(ShipDatabaseCount+1)*sizeof(void *));

	ShipDataRec=(struct SHIPDATABASERECORD *)malloc(sizeof(struct SHIPDATABASERECORD));
	memset(ShipDataRec, 0, sizeof(struct SHIPDATABASERECORD));

	if (ShipDataRec == NULL) return;
	
	ShipDatabaseRecords[ShipDatabaseCount]=ShipDataRec;
        
	ShipDataRec->ID = UserID;

	ShipDataRec->TimeAdded = NOW;

	ShipDatabaseCount++;

	sprintf(ShipDataRec->name,"%d",UserID);

}

void LoadVesselDataBase()
{
	int i;

	FILE *file;
	char buf[256];
	char *token;
	char FN[256];

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN, "AIS_Vessels.txt");
	}
	else
	{
		strcpy(FN, BPQDirectory);
		strcat(FN, "/");
		strcat(FN, "AIS_Vessels.txt");
	}

	if ((file = fopen(FN,"r")) == NULL) return;

	fgets(buf, 255, file);

	sscanf(buf,"%d", &ShipDatabaseCount);

	if (ShipDatabaseCount == 0)
	{
		fclose(file);
		return;
	}

	ShipDatabaseRecords = (struct SHIPDATABASERECORD **)malloc(ShipDatabaseCount * sizeof(void *));

	for (i = 0; i < ShipDatabaseCount; i++)
	{
		ShipDataRec = (struct SHIPDATABASERECORD *)malloc(sizeof(struct SHIPDATABASERECORD));
		ShipDatabaseRecords[i] = ShipDataRec;
		memset(ShipDataRec, 0, sizeof(struct SHIPDATABASERECORD));

		fgets(buf, 255, file);

		token = strtok(buf, "|\n" );
   
 		ShipDataRec->ID = atoi(token);

		token = strtok( NULL,  "|\n" );
		strcpy(&ShipDataRec->name[0],token);

		token = strtok( NULL,  "|\n" );
		strcpy(&ShipDataRec->Callsign[0],token);
	    
		token = strtok( NULL,  "|\n" );
		ShipDataRec->LengthA = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->LengthB = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->WidthC = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->WidthD = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->Draft = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->TimeAdded = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->TimeLastUpdated = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->Type = atoi(token);

		token = strtok( NULL,  "|\n" );
		ShipDataRec->Class = atoi(token);

	}

	fclose(file);

}

void SaveVesselDataBase()
{
	int i, n = 0;

	FILE *file;
	char buf[256];
	char FN[256];

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN, "AIS_Vessels.txt");
	}
	else
	{
		strcpy(FN, BPQDirectory);
		strcat(FN, "/");
		strcat(FN, "AIS_Vessels.txt");
	}

	if ((file = fopen(FN,"w")) == NULL) return;

	for (i = 0; i < ShipDatabaseCount; i++)
	{
		if (ShipDatabaseRecords[i]->Callsign[0] > 32)	// Count those with details
			n++;
	}

	sprintf(buf,"%d\n", n);
	fputs(buf, file);

	for (i = 0; i < ShipDatabaseCount; i++)
	{
		ShipDataRec = ShipDatabaseRecords[i];

		if (ShipDataRec->Callsign[0] > 32)			// No point in saving if no vessel details
		{
			sprintf(buf,"%d|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
			ShipDataRec->ID,
			ShipDataRec->name,
			ShipDataRec->Callsign,
			ShipDataRec->LengthA,
			ShipDataRec->LengthB,
			ShipDataRec->WidthC,
			ShipDataRec->WidthD,
			ShipDataRec->Draft,
			ShipDataRec->TimeAdded,
			ShipDataRec->TimeLastUpdated,
			ShipDataRec->Type,
			ShipDataRec->Class);

			fputs(buf, file);
		}
	}

	fclose(file);

}

void LoadNavAidDataBase()
{
	int i, n, count;

	FILE *file;
	char buf[256];
	char *token;
	char FN[256];
	struct NAVAIDRECORD * navptr;

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN, "AIS_NavAids.txt");
	}
	else
	{
		strcpy(FN, BPQDirectory);
		strcat(FN, "/");
		strcat(FN, "AIS_NavAids.txt");
	}

	if ((file = fopen(FN,"r")) == NULL) return;

	fgets(buf, 255, file);

	sscanf(buf,"%d", &NavAidCount);

	if (NavAidCount == 0)
	{
		fclose(file);
		return;
	}

	NavRecords = (struct NAVAIDRECORD **)malloc(NavAidCount * sizeof(void *));

	count = 0;

	for (i = 0; i < NavAidCount; i++)
	{
		navptr = (struct NAVAIDRECORD *)malloc(sizeof(struct NAVAIDRECORD));
		NavRecords[count] = navptr;
		memset(navptr, 0, sizeof(struct NAVAIDRECORD));

		fgets(buf, 255, file);

		token = strtok(buf, "|\n" );
		navptr->ID = atoi(token);

		token = strtok(NULL,  "|\n" );
		strcpy(&navptr->name[0],token);

		for (n = 0; n < 20; n++)
		{
			char c = navptr->name[n];

			if (!isalpha(c) && !isdigit(c) && c != ' ' && c != '_')
			{
				count--;
				break;
			}
		}

		count++;

		token = strtok(NULL, "|\n" );
		navptr->lat = atof(token);

		token = strtok(NULL, "|\n" );
		navptr->Long = atof(token);

		token = strtok(NULL, "|\n" );
		navptr->TimeAdded = atoi(token);

		token = strtok(NULL, "|\n" );
		navptr->TimeLastUpdated = atoi(token);
	}

	NavAidCount = count;
	fclose(file);
}


void SaveNavAidDataBase()
{
	int i;

	FILE *file;
	char FN[256];
	struct NAVAIDRECORD * navptr;


	if (BPQDirectory[0] == 0)
	{
		strcpy(FN, "AIS_NavAids.txt");
	}
	else
	{
		strcpy(FN, BPQDirectory);
		strcat(FN, "/");
		strcat(FN, "AIS_NavAids.txt");
	}

	if ((file = fopen(FN,"w")) == NULL) return;

	fprintf(file, "%d\n", NavAidCount);

	for (i = 0; i < NavAidCount; i++)
	{
		navptr=NavRecords[i];
		fprintf(file, "%d|%s|%f|%f|%d|%d\n", navptr->ID, navptr->name, navptr->lat, navptr->Long, navptr->TimeAdded, navptr->TimeLastUpdated);
	}

	fclose(file);
}

void initAIS()
{
	LoadVesselDataBase();
	LoadNavAidDataBase();
}

void SaveAIS()
{
	SaveVesselDataBase();
	SaveNavAidDataBase();
}

void initADSB()
{
	_beginthread(ADSBConnect, 0, 0);
}


int ProcessAISMessage(char * msg, int len)
{
	char * ptr1;
	char * ptr2;
	char X1[10], X2[10], X3[10], X4[10];
	char Data[256];
	float alt=0.0;

	NOW = time(NULL);

	ptr1 = &msg[7];

	//'   Looks like first is no of fragments in message, 2nd fragment seq no, 3rd msg seq, 4th port
	//'!AIVDM,2,1,7,2,502He5h000030000000AD8hTr10u9B1IA<0000000000040000<000000000,0*54
	//'!AIVDM,2,2,7,2,00000000008,2*58

	if (memcmp(&msg[1],"AIVDM",5) == 0)
	{
		if (memchr(msg,'*',len) == 0) return 0; // No Checksum

		len-=7;

		ptr2=(char *)memchr(ptr1,',',8);

		if (ptr2 == 0) return 0;	// Duff

		*(ptr2++)=0;
		strcpy(X1,ptr1);
		ptr1=ptr2;

		ptr2=(char *)memchr(ptr1,',',8);

		if (ptr2 == 0) return 0;	// Duff

		*(ptr2++)=0;
		strcpy(X2,ptr1);
		ptr1=ptr2;

		ptr2=(char *)memchr(ptr1,',',8);

		if (ptr2 == 0) return 0;	// Duff

		*(ptr2++)=0;
		strcpy(X3,ptr1);
		ptr1=ptr2;

		if (*ptr1 == 'A')
			ACount++;
		else
			BCount++;

		ptr2=(char *)memchr(ptr1,',',8);

		if (ptr2 == 0) return 0;	// Duff

		*(ptr2++)=0;
		strcpy(X4,ptr1);
		ptr1=ptr2;

		ptr2=(char *)memchr(ptr1,',',78);

		if (ptr2 == 0) return 0;	// Duff

		*(ptr2++)=0;
		strcpy(Data,ptr1);

		if (X1[0] == '1')
		{
			//  Single fragment

			DecodeVDM (Data);
			return 0;
		}

		// Multipart. 

		if (X2[0] == '1')
		{
			//   First of Multipart

			strcpy(Stack,Data);			
			return 0;				// ' this needs generalising
		}

		//   Subsequent part of Multipart

		strcat(Stack, Data);

		if (X1[0] == X2[0])
		{
			// Last Part

			DecodeVDM (Stack);
			return 0;
		}
	}

	return 0;

}
int mm1=0, nasa1=0, mm4=0, nasa4=0;



int DecodeVDM(char *msg)
{ 
	int Conv, i, bits;

	UCHAR val[500];

	// Start value at offet 1 for compatiblilty woth Basic Code

	ConvertAsciito6Bit(msg, (char *)&val[1], strlen(msg));       // Convert ascii chars to 6-bit values

	Msg_Type = val[1];

	UserID = ((val[2] & 15) << 26) + (val[3] << 20)+ (val[4] << 14)+ (val[5] << 8)+ (val[6] << 2) + (val[7] >>4); ;

	//	if (UserID == 0 ) UserID = 99998888;

	bits=strlen(msg)*6;

	if (Msg_Type == 1 || Msg_Type == 2 || Msg_Type == 3)
	{
		if (bits == 168) nasa1++; else mm1++;

		NavStatus = val[7] & 15;

		//'Rate of turn           8   50  Char 8 Char 9 bits 0-1±127 (–128 (80 hex) indicates not available, which should be the

		V_ROT = (val[8] << 2)  + (val[9] >> 4);

		/*
		'           0...+ 126 = turning right at up to 708 degrees per minute or higher;
		'           0...- 126 = turning left at up to 708 degrees per minute or higher
		'           Values between 0 and 708 degrees/min coded by
		'           ROTAIS=4.733 SQRT(ROTsensor) degrees/min
		'           where ROTsensor is the Rate of Turn as input by an external Rate of Turn
		'           Indicator. ROTAIS is rounded to the nearest integer value.
		'           + 127 = turning right at more than 5 deg/30s (No TI available)
		'           -127 = turning Left at more than 5 deg/30s (No TI available)
		'           –128 (80 hex) indicates no turn information available (default).
		*/

		if (V_ROT > 128)  V_ROT = -(256 - V_ROT);

		if (V_ROT == 128) 

			V_ROT = 0;

		else if (V_ROT == 127)

			V_ROT = 3;        // 5 deg / 30 secs or more

		else if (V_ROT == -127) 

			V_ROT = -3;

		else
		{    
			V_ROT = V_ROT / 4.733;
			V_ROT = V_ROT * fabs(V_ROT);

		} 

		//'SOG                 10     60  Char 9 bits 2-5 char 10 Speed over ground in 1/10 knot steps (0-102.2 knots)

		V_SOG = (((val[9] & 15) << 6) + val[10]); 

		if (V_SOG == 1023.0)			// Unknown
			V_SOG = 0.0;

		V_SOG = V_SOG/10;


		//'Accuracy            1      61  Char 11 bit 0

		Accuracy = val[11] >> 5;

		//'Long                28     89  char 11 bits 1-5 char 12 13 14 char 15 bita 0-4


		Conv = ((val[11] & 31) << 23) + (val[12] << 17) + (val[13] << 11) + (val[14] << 5) + (val[15] >>1); 

		if ((val[11] & 16) == 16) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;

		if (V_Lon > 18 || V_Lon < -18)
			return 0;


		//Lat                27      116 Char 15 bit 5 16 17 18 19 20 bits 0-1

		Conv = ((val[15] & 1) << 26) + (val[16]<< 20) + (val[17]<< 14) + (val[18]<< 8) + (val[19]<< 2) + (val[20] >>4);

		if ((val[15] & 1) == 1)
			Conv = -(0x8000000 - Conv);


		V_Lat = Conv;
		V_Lat/= 600000;


		//COG                12      128 char 20 bits 2-5 21 22 bit 0-1

		V_COG = ((val[20] & 15) << 8) + (val[21]<< 2) + (val[22] >>4);

		V_COG /= 10;

		//True Heading       9       137 char 22 bits 2-5 char 23 bits 0-4

		V_Heading = ((val[22] & 15) << 5) + (val[23] >>1);

		/*

		'Message ID             6       Char 1           Identifier for this message 1, 2 or 3
		'Repeat Indicator       2   8   Char 2 bits 0-1 Used by the repeater to indicate how many times a message has been
		'                                             repeated. Refer to § 4.6.1; 0 - 3; default = 0; 3 = do not repeat any more.
		'User ID                30  38  Char 2 bits 2-5, Chars 3 4 5 6  Char 7 bits 0-1 MMSI number
		'Navigational status    4   42  Char 7 bits 2-5 0 = under way using engine, 1 = at anchor, 2 = not under command,

		'Rate of turn           8   50  Char 8 Char 9 bits 0-1±127 (–128 (80 hex) indicates not available, which should be the
		'SOG                 10     60  Char 9 bits 2-5 char 10 Speed over ground in 1/10 knot steps (0-102.2 knots)

		'Accuracy            1      61  Char 11 bit 0
		'Long                28     89  char 11 bits 1-5 char 12 13 14 char 15 bita 0-4
		'Lat                27      116 Char 15 bit 5 16 17 18 19 20 bits 0-1
		'COG                12      128 char 20 bits 2-5 21 22 bit 0-1
		'True Heading       9       137 char 22 bits 2-5 char 23 bits 0-4
		'Time Stamp         6       143 char 23 bits 5 24 bits 0-4
		'Reserved            4      147 char 24 bits 5 char 25 bits 0-2
		'Spare              1       148 char 25 bit 3
		'RAIM               1       149 char 25 bit 4
		'Comms State         19     168 char 25 bit 5 26 27 28  See below.
		'Total number of bits 168   28 chars

		*/

		ProcessAISVesselMessage();

		return 0;
	}


	//	if (Msg_Type == 4  || Msg_Type == 11)		// 11 are probably ships responding to Tamestamp requests
	if (Msg_Type == 4)
	{
		if (bits == 168) nasa4++; else mm4++;


		//       Base Station

		//'Message ID             6       Char 1           Identifier for this message 1, 2 or 3
		//'Repeat Indicator       2   8   Char 2 bits 0-1 Used by the repeater to indicate how many times a message has been
		//'                                             repeated. Refer to § 4.6.1; 0 - 3; default = 0; 3 = do not repeat any more.
		//'User ID                30  38  Char 2 bits 2-5, Chars 3 4 5 6  Char 7 bits 0-1 MMSI number

		//'UTC year               14  52    Char 7 bits 2-5, char 8, char 9 bits 0-3  1 - 9999; 0 = UTC year not available = default.

		UTCyear = ((val[7] & 15) << 10) + (val[8] << 4) + (val[9] >> 2);

		//'UTC month              4   56    Char 9 bits 4-5, char 10 bits 0-1  1 - 12; 0 = UTC month not available = default; 13 - 15 not used

		UTCMonth = ((val[9] & 3) << 2) + (val[10] >> 4);

		//'UTC day                5   61  Char 10 bits 2-5, chr 11 bit 0  1 - 31; 0 = UTC day not available = default.

		UTCDay = ((val[10] & 15) << 1) + (val[11] >> 5);

		//'UTC hour               5   66  Char 11 bits 2-5  0 - 23; 24 = UTC hour not available = default;

		UTCHour = val[11] & 31;


		//'UTC minute             6   72    Char 12      0 - 59; 60 = UTC minute not available = default;

		UTCMinute = val[12];

		//'UTC second             6   78    Char 13     0 - 59; 60 = UTC second not available = default;

		UTCSecond = val[13];


		//'Position accuracy      1   79     Char 14 bit 0        1 = high (< 10 m; Differential Mode of e.g. DGNSS receiver) 0 = low
		//'(> 10 m; Autonomous Mode of e.g. GNSS receiver, or of other Electronic
		//'Position Fixing Device), default = 0

		Accuracy = val[14] >> 5;

		//'Long                28     107  char 14 bits 1-5 char 15 16 17 char 18 bitS 0-4
		//'Longitude 28 Longitude in 1/10 000 minute (±180 degrees, East = positive (as per 2´s
		//'complement), West = negative(as per 2´s complement);
		//'181 degrees (6791AC0 hex) = not available = default)


		Conv = ((val[14] & 31) << 23) + (val[15] << 17) + (val[16] << 11) + (val[17] << 5) + (val[18] >>1); 

		if ((val[14] & 16) == 16) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;

		//'Latitude 27 Latitude in 1/10 000 minute (±90 degrees, North = positive (as per 2´s
		//'complement), South = negative (as per 2´s complement);
		//'91 degrees (3412140 hex) = not available = default)


		//'Lat                27      134 Char 18 bit 5 19 20 21 22 23 bits 0-1

		Conv = ((val[18] & 1) << 26) + (val[19]<< 20) + (val[20]<< 14) + (val[21]<< 8) + (val[22]<< 2) + (val[23] >>4);

		if ((val[18] & 1) == 1)
			Conv = -(0x8000000 - Conv);

		V_Lat = Conv;
		V_Lat/= 600000;

		/*
		'Type of Electronic
		'Position Fixing
		'Device
		'               4          138     Char 23 bits 2-5 Use of differential corrections is defined by field "position accuracy"
		'above;
		'0 = Undefined (default),
		'1 = GPS,
		'2 = GLONASS,
		'3 = Combined GPS/GLONASS,

		'4 = Loran-C,
		'5 = Chayka,
		'6 = Integrated Navigation System,

		'7 = surveyed,
		'8 - 15 = not used.
		*/

		FixType = val[23] & 15; 

		/*
		'Spare          10       148       char 24, char 25 bits 0-3  Not used. Should be set to zero.
		'RAIM-Flag      1        149       char 25 bit 4               RAIM (Receiver Autonomous Integrity Monitoring) flag of Electronic
		'Position Fixing Device; 0 = RAIM not in use = default; 1 = RAIM in use)
		'Communication State 19  168       char 25 bit 5, 26 27 28      SOTDMA Communication State as described in § 3.3.7.2.2.


		'Sync state         2       0 UTC Direct (refer to § 3.1.1.1).
		'                           1 UTC Indirect (refer to § 3.1.1.2).
		'                           2 Station is synchronized to a Base station(Base direct) (refer to § 3.1.1.3).
		'                           3 Station is synchronized to another station based on the highest
		'                               number of received stations or to another mobile station, which is
		'                               directly synchronized to a base station (refer to § 3.1.1.3 and § 3.1.1.4).
		'Slot Time-Out      3           Specifies frames remaining until a new slot is selected.
		'0 means that this was the last transmission in this slot.
		'1-7 means that 1 to 7 frames respectively are left until slot change.
		'Sub message        14       The sub message depends on the current value in slot time-out as

		*/
		SyncState = ((val[25] & 1) << 1) + (val[26] >> 5); 

		//'    Debug.Print "Sync State "; SyncState;

		SlotTO = (val[26] >>2) & 7;

		//'    Debug.Print "Slot TO "; SlotTO;

		SubMsg = ((val[26] & 3) << 12) + (val[27] << 6) + val[28];

		//'Total number of bits 168

		ProcessBaseStationMessage();
		return 0;
	}

	if (Msg_Type == 5)
	{

		//'IMO No                 30  70  char 7 4-5, 8 9 10 11, char 12 0-3

		V_IMO = ((val[7] & 3) << 28) + (val[8]<< 22) + (val[9]<< 16) + (val[10]<< 10) + (val[11]<< 4) + (val[12] >>2);

		//'Callsign               42  112 char 12 4-5, 13 14 15 16 17 18 19 bits 0-3


		for (i = 0; i <= 6; i++)
		{
			Conv = ((val[12 + i] & 3) << 4) + (val[13 + i] >> 2);

			if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

			V_Callsign[i] = Conv;
		}

		//Name                   120 232 char 19 bits 4-5 20-38 39 bits 0-3

		for (i = 0; i <= 19; i++)
		{
			Conv = ((val[19 + i] & 3) << 4) + (val[20 + i] >> 2);

			if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

			V_Name[i] = Conv;
		}

		//'Type of Ship           8   240 char 39 bits 4-5 40

		V_Type=((val[39] & 3) << 6) + val[40];  


		//'Dimensions             30  270 char 41 42 43 44 45

		//'A Bit 21 – Bit 29 0 – 511 9
		//'B Bit 12 – Bit 20 0 – 511 9
		//'C Bit 6 – Bit 11 0 - 63 ; 6
		//'D Bit 0 – Bit 5 0 - 63 ;  6


		V_LenA = (val[41] << 3) + (val[42] >> 3);
		V_LenB = ((val[42] & 7) << 6) + val[43];
		V_WidthC = val[44];
		V_WidthD = val[45];

		//		sprintf(TraceMsg,"%s  %d %d %d %d %d  LenA %d LenB %d\n", V_Name, val[41], val[42], val[43], val[44], val[45], V_LenA, V_LenB);
		//		OutputDebugString(TraceMsg);

		//'    Type of Fix            4   274 char 46 bits 0-3

		FixType = val[46] >>2;

		//'ETA                    20  294     char 46 bits 4-5, 47,48,49

		ETA =((val[46] & 3) << 18) + (val[47]<< 12) + (val[48]<< 6) + val[49];

		//'Draught                8   302     char 50, char 51 bits 0-1

		V_Draught = (val[50] << 2) + (val[51] >>4);

		//'Destination            120 422     char 51 bits 2-5,  52-70 71 bits 0-1

		for (i = 0; i <= 19; i++)
		{
			Conv = ((val[51 + i] & 15) << 2) + (val[52 + i] >> 4);

			if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

			V_Dest[i] = Conv;
		}

		//'DTE                    1   423     char 71 bit 2
		//'Spare                  1   424     char 71 bit 3

		ProcessAISVesselDetails('A');

		return 0;
	}

	/*

	'   Type 5

	'Message ID             6       Char 1           Identifier for this message 1, 2 or 3
	'Repeat Indicator       2   8   Char 2 bits 0-1 Used by the repeater to indicate how many times a message has been
	'                                             repeated. Refer to § 4.6.1; 0 - 3; default = 0; 3 = do not repeat any more.
	'User ID                30  38  Char 2 bits 2-5, Chars 3 4 5 6  Char 7 bits 0-1 MMSI number
	'AIS Version            2   40  char 7 2-3
	'IMO No                 30  70  char 7 4-5, 8 9 10 11, char 12 0-3
	'Callsign               42  112 char 12 4-5, 13 14 15 16 17 18 19 bits 0-3
	'Name                   120 232 char 19 bits 4-5 20-38 39 bits 0-3
	'Type of Ship           8   240 char 39 bits 4-5 40

	'Dimensions             30  270 char 41 42 43 44 45

	'A Bit 21 – Bit 29 0 – 511 9
	'B Bit 12 – Bit 20 0 – 511 9
	'C Bit 6 – Bit 11 0 - 63 ; 6
	'D Bit 0 – Bit 5 0 - 63 ;  6




	'Type of Fix            4   274 char 46 bits 0-3

	'ETA                    20  294
	'Draught                8   302
	'Destination            120 422
	'DTE                    1   423
	'Spare                  1   424

	'                   Total 424 bits = 53 bytes = 70 chars  + 4 bits


	'A Bit 21 – Bit 29 0 – 511
	'B Bit 12 – Bit 20 0 – 511
	'C Bit 6 – Bit 11 0 - 63 ;
	'D Bit 0 – Bit 5 0 - 63 ;

	*/


	if (Msg_Type == 18)				// Class B Position
	{


		/*
		Message ID										6	Identifier for Message 18; always 18
		char 1

		Repeat indicator								2	Used by the repeater to indicate how many times a message has been repeated. 0-3; shall be 0 for Class B “CS” transmissions
		char 2 bits 0-1

		User ID											30	MMSI number
		Char 2 2-5, 3, 4, 5, 6, 7 bits 0-1

		Reserved for regional or local applications		8	Reserved for definition by a competent regional or local authority. Shall be set to zero, if not used for any regional or local application. Regional applications should not use zero
		Char 7 bits 2-5, Char 8 bits 1-3

		SOG												10	Speed over ground in 1/10 knot steps (0-102.2 knots) 1 023 = not available, 1 022 = 102.2 knots or higher
		Char 8 bits 4,5, Char 9, Char 10 bits 0-1
		*/

		V_SOG = (((val[8] & 3) <<8) + (val[9] << 2) + (val[10] >>4)); 
		V_SOG = V_SOG/10;


		//	Position accuracy								1	1 = high (<10 m) 0 = low (>10 m)
		//		Char 10 bit 2

		Accuracy = ((val[10] & 8) >> 3);

		//Longitude										28	Longitude in 1/10 000 min (±180°, East = positive (as per 2's complement), West = negative (as per 2's complement), 181° (6791AC0 hex) = not available = default)
		//		Char 10 bits 3-5, 11, 12, 13, 14, 15 bit 0 		

		Conv = ((val[10] & 7) << 25) + (val[11] << 19) + (val[12] << 13) + (val[13] << 7) + (val[14] << 1) + (val[15] >>5); 

		if ((val[10] & 4) == 4) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;


		//Latitude										27	Latitude in 1/10 000 min (±90°, North = positive (as per 2's complement), South = negative (as per 2's complement), 91° (3412140 hex) = not available = default)
		//		 Char 15 bits 1-5 16 17 18 19 bits 0-3

		Conv = ((val[15] & 31) << 22) + (val[16]<< 16) + (val[17]<< 10) + (val[18]<< 4) + (val[19] >>2);

		if ((val[15] & 16) == 16)
			Conv = -(0x8000000 - Conv);


		V_Lat = Conv;
		V_Lat/= 600000;


		//COG												12	Course over ground in 1/10° (0-3 599). 3 600 (E10h) = not available = default; 3 601-4 095 shall not be used
		//		 Char 19 bits 4-5 20 21 bit 0-3

		V_COG = ((val[19] & 3) << 10) + (val[20]<< 4) + (val[21] >>2);

		V_COG /= 10;

		//True heading									9	Degrees (0-359) (511 indicates not available = default)
		//		char 21 bits 4-5 char 22 char 23 bit 0

		V_Heading = ((val[21] & 3) << 7) + (val[22]<< 1) + (val[23] >>5);

		//Time stamp										6	UTC second when the report was generated by the EPFS (0-59); 60 if time stamp is not available, which shall also be the default value 61, 62 and 63 are not used by the Class B “CS” AIS
		//		Char 23 bits 1-5, 24 bit 0


		//Reserved for regional applications				2	Reserved for definition by a competent regional authority. Shall be set to zero, if not used for any regional application. Regional applications should not use zero
		//		24 bit 1-2


		//Class B unit flag								1	0 = Class B SOTDMA unit 1 = Class B “CS” unit
		//		24 bit 3

		//Class B display flag							1	0 = No display available; not capable of displaying Message 12 and 14 1 = Equipped with integrated display displaying Message 12 and 14
		//		24 bit 4

		//Class B DSC flag								1	0 = Not equipped with DSC function 1 = Equipped with DSC function (dedicated or time-shared)
		//		24 bit 5

		//Class B band flag								1	0 = Capable of operating over the upper 525 kHz band of the marine band 1 = Capable of operating over the whole marine band (irrelevant if “Class B Msg22 flag” is 0)
		//		25 bit 0

		//Class B Message 22 flag							1	0 = No frequency management via Message 22 , operating on AIS1, AIS2 only 1 = Frequency management via Message 22
		//		25 bit 1

		//Mode flag										1	0 = Station operating in autonomous mode = default 1 = Station operating in assigned mode
		//		25 bit 2

		//RAIM-flag										1	RAIM flag of electronic position fixing device, optional; 0 = RAIM not in use = default; 1 = RAIM in use (valid data for expected position error)
		//		25 bit 3

		//Communication state selector flag				1	1 = ITDMA communication state follows
		//		25 bit 4

		//Communication state								19	ITDMA communication state; refer to § 4.3.3.5
		//		25 bit  5, 26, 27. 28

		//Total number of bits 168 Occupies one-time period (28 chars)

		ProcessAISVesselMessage();

		return 0;


	}



	if (Msg_Type == 19)				// Class B Extended Position
	{


		/*
		Message ID										6	Identifier for Message 18; always 18
		char 1

		Repeat indicator								2	Used by the repeater to indicate how many times a message has been repeated. 0-3; shall be 0 for Class B “CS” transmissions
		char 2 bits 0-1

		User ID											30	MMSI number
		Char 2 2-5, 3, 4, 5, 6, 7 bits 0-1

		Reserved for regional or local applications		8	Reserved for definition by a competent regional or local authority. Shall be set to zero, if not used for any regional or local application. Regional applications should not use zero
		Char 7 bits 2-5, Char 8 bits 1-3

		SOG												10	Speed over ground in 1/10 knot steps (0-102.2 knots) 1 023 = not available, 1 022 = 102.2 knots or higher
		Char 8 bits 4,5, Char 9, Char 10 bits 0-1
		*/

		V_SOG = (((val[8] & 3) <<8) + (val[9] << 2) + (val[10] >>4)); 
		V_SOG = V_SOG/10;


		//	Position accuracy								1	1 = high (<10 m) 0 = low (>10 m)
		//		Char 10 bit 2

		Accuracy = ((val[10] & 8) >> 3);

		//Longitude										28	Longitude in 1/10 000 min (±180°, East = positive (as per 2's complement), West = negative (as per 2's complement), 181° (6791AC0 hex) = not available = default)
		//		Char 10 bits 3-5, 11, 12, 13, 14, 15 bit 0 		

		Conv = ((val[10] & 7) << 25) + (val[11] << 19) + (val[12] << 13) + (val[13] << 7) + (val[14] << 1) + (val[15] >>5); 

		if ((val[10] & 4) == 4) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;


		//Latitude										27	Latitude in 1/10 000 min (±90°, North = positive (as per 2's complement), South = negative (as per 2's complement), 91° (3412140 hex) = not available = default)
		//		 Char 15 bits 1-5 16 17 18 19 bits 0-3

		Conv = ((val[15] & 31) << 22) + (val[16]<< 16) + (val[17]<< 10) + (val[18]<< 4) + (val[19] >>2);

		if ((val[15] & 16) == 16)
			Conv = -(0x8000000 - Conv);


		V_Lat = Conv;
		V_Lat/= 600000;


		//COG												12	Course over ground in 1/10° (0-3 599). 3 600 (E10h) = not available = default; 3 601-4 095 shall not be used
		//		 Char 19 bits 4-5 20 21 bit 0-3

		V_COG = ((val[19] & 3) << 10) + (val[20]<< 4) + (val[21] >>2);

		V_COG /= 10;

		//True heading									9	Degrees (0-359) (511 indicates not available = default)
		//		char 21 bits 4-5 char 22 char 23 bit 0

		V_Heading = ((val[21] & 3) << 7) + (val[22]<< 1) + (val[23] >>5);

		//Time stamp										6	UTC second when the report was generated by the EPFS (0-59); 60 if time stamp is not available, which shall also be the default value 61, 62 and 63 are not used by the Class B “CS” AIS
		//		Char 23 bits 1-5, 24 bit 0


		//Reserved for regional applications				4	Reserved for definition by a competent regional authority. Shall be set to zero, if not used for any regional application. Regional applications should not use zero
		//		24 bit 1-4



		//Name								120	Maximum 20 characters 6-bit ASCII, @@@@@@@@@@@@@@@@@@@@ = not available = default
		//		24 bit 5 - 44 bits 0-4


		for (i = 0; i <= 19; i++)
		{
			Conv = (((val[24 + i]) & 1) << 5) + (val[25 + i] >> 1);

			if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

			V_Name[i] = Conv;
		}



		//Type of ship and cargo type			8	0 = not available or no ship = default
		//										1-99 = as defined in § 3.3.8.2.3.2
		//										100-199 = preserved, for regional use
		//										200-255 = preserved, for future use
		//		44 bit 5 45 46 bit 0

		V_Type=((val[44] & 1) << 5) + (val[45] << 1) + (val[46] >> 5);  

		//Dimension of ship/reference for position	30	Dimensions of ship in metres and reference point for reported position (see Fig. 17 and § 3.3.8.2.3.3)
		//		46 bit 1-5 47 48 49 50 51 bit 0	
		//		46 bit 1-5, 47 bits 0-3 - 47 bits 4,5 48, 49 bit bit 0 - 49 bits 1-5, 50 bit 0 - 50 1-5, 51 bit 0

		V_LenA = ((val[46] & 31) << 5) + (val[47] >> 2);
		V_LenB = ((val[47] & 3) << 7) + (val[48] << 1) + (val[49] >> 5);
		V_WidthC = ((val[49] & 31) << 1) + (val[50] >> 5);
		V_WidthD = ((val[50] & 31) << 1) + (val[51] >> 5);


		//Type of electronic position fixing device	4	0 = Undefined (default); 1 = GPS, 2 = GLONASS, 3 = combined GPS/GLONASS, 4 = Loran-C, 5 = Chayka, 6 = integrated navigation system, 7 = surveyed; 8-15 = not used
		//		51 bits 1-4

		//RAIM-flag									1	RAIM flag of electronic position fixing device; 0 = RAIM not in use = default; 1 = RAIM in use
		//		51 bit 5

		//DTE											1	Data terminal ready (0 = available 1 = not available = default) (see § 3.3.8.2.3.1)
		//		52 bit 0
		//Spare										5	Not used. Should be set to zero 
		//		52 bits 1-5
		// Total number of bits 312 (52 chars) Occupies two slots

		ProcessAISVesselMessage();
		ProcessAISVesselDetails('B');

		return 0;


	}


	// Navaid

	if (Msg_Type == 21)
	{

		//'Message ID             6       Char 1           Identifier for this message 21
		//'Repeat Indicator       2   8   Char 2 bits 0-1 Used by the repeater to indicate how many times a message has been
		//'                                             repeated. Refer to § 4.6.1; 0 - 3; default = 0; 3 = do not repeat any more.
		//'User ID                30  38  Char 2 bits 2-5, Chars 3 4 5 6  Char 7 bits 0-1 MMSI number


		//'Type of Aids-to- Navigation   5 43      Char 7 bits 2-5   Char 8 bit 0  0 = not available = default; 1 – 15 = Fixed Aid-to-Navigation; 16 - 31 =
		//'                                               Floating Aid-to-Navigation; refer to appropriate definition set up by IALA;
		//'                                                refer to Table 34bis.

		NavAidType = ((val[7] & 15) << 1) + (val[8] >>5);

		//'Name of Aid s-to-Navigation
		//'                                           120 Maximum 20 characters 6 bit ASCII,
		//'                                           "@@@@@@@@@@@@@@@@@@@@" = not available = default.
		//'                                           Navigation "@@@@@@@@@@@@@@@@@@@@" = not available = default.
		//'The name of the Aid-to-Navigation may be extended by the parameter
		//'"Name of Aid-to-Navigation Extension" below.

		//'Name                   120 163          char 8 bits 1-5 9-27 28 bits 0

		for (i = 0; i <= 19; i++)
		{
			Conv = ((val[8 + i] & 31) << 1) + (val[9 + i] >> 5);

			if (Conv < 32) Conv = Conv + 64;

			V_Name[i] = Conv;
		}

		V_Name[20]=0;

		//'Position accuracy                  1   164     28 bit 1   1 = high (< 10 m; Differential Mode of e.g. DGNSS receiver) 0 = low

		//'(> 10 m; Autonomous Mode of e.g. GNSS receiver or of other Electronic
		//'Position Fixing Device) ; Default = 0

		Accuracy = ((val[28] & 16) >> 4);


		//'Long                               28  192     char 28 bits 2-5 char 29 30 31 32
		//'Longitude 28 Longitude in 1/10 000 minute (±180 degrees, East = positive (as per 2´s
		//'complement), West = negative(as per 2´s complement);
		//'181 degrees (6791AC0 hex) = not available = default)

		Conv = ((val[28] & 15) << 24) + (val[29] << 18) + (val[30] << 12) + (val[31] << 6) + val[32]; 

		if ((val[28] & 8) == 8) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;

		//'Latitude 27 Latitude in 1/10 000 minute (±90 degrees, North = positive (as per 2´s
		//'complement), South = negative (as per 2´s complement);
		//'91 degrees (3412140 hex) = not available = default)

		//'Lat                27      219  Char 33 34 35 36 37 bits 0-2

		Conv = (val[33] << 21) + (val[34]<< 15) + (val[35]<< 9) + (val[36]<< 3) + (val[37] >> 3);

		if ((val[33] & 32) == 32)
			Conv = -(0x8000000 - Conv);

		V_Lat = Conv;
		V_Lat/= 600000;



		//'Dimension/Reference for Position    30 249   37 bits 3-5 38 39 40 41 42 bits  0-2   Reference point for reported position; also indicates the dimension of an
		//'Aid-to-Navigation in metres (see Fig. 18 and § 3.3.8.2.3.3), if relevant. (1)
		//'Type of Electronic

		//'A Bit 21 – Bit 29 0 – 511 9
		//'B Bit 12 – Bit 20 0 – 511 9
		//'C Bit 6 – Bit 11 0 - 63 ; 6
		//'D Bit 0 – Bit 5 0 - 63 ;  6


		V_LenA = ((val[37] & 7) << 3) + val[38];

		V_LenB = (val[39] << 6) + (val[40] >> 3);

		V_WidthC = ((val[40] & 7) << 3) + (val[41] >> 3);

		V_WidthD = ((val[41] & 7) << 3) + (val[42] >> 3);

		/*

		'Position Fixing Device                4  253      42 bits 3-5 43 bit 0
		'0 = Undefined (default);
		'1 = GPS,
		'2 = GLONASS,
		'3 = Combined GPS/GLONASS,
		'4 = Loran-C,
		'5 = Chayka,
		'6 = Integrated Navigation System,
		'7 = surveyed.For fixed AtoNs and virtual AtoNs, the surveyed position
		'should be used. The accurate position enhances its function as a radar
		'reference target.
		'8 – 15 = not used.

		*/   
		FixType = ((val[42] & 7) << 1) + (val[43] >> 5);

		/*
		'Time Stamp                 6  259  43 bit 1-5 44 bit 0    UTC second when the report was generated by the EPFS (0 –59,

		'or 60 if time stamp is not available, which should also be the default value,
		'or 61 if positioning system is in manual input mode,
		'or 62 if Ele ctronic Position Fixing System operates in estimated (dead reckoning) mode,
		'or 63 if the positioning system is inoperative)


		'Off-Position Indicator    1    260   44 bit 1  For floating Aids-to-Navigation, only: 0 = on position; 1 = off position;
		'NOTE – This flag should only be considered valid by receiving station, if
		'the Aid -to-Navigation is a floating aid, and if Time Stamp is equal to or
		'below 59. For floating AtoN the guard zone parameters should be set on installation.

		'Reserved for regional or local application

		'                           8  268  44 bits 2-5 45 bit 0-3

		'Reserved for definition by a competent regional or local authority. Should
		'be set to zero, if not used for any regional or local application. Regional
		'applications should not use zero.


		'RAIM-Flag                 1  269   45 bit 4    RAIM (Receiver Autonomous Integrity Monitoring) flag of Electronic
		'Position Fixing Device; 0 = RAIM not in use = default; 1 = RAIM in use)

		'Virtual AtoN Flag         1  270   45 bit 5    0 = default = real A to N at indicated position; 1 = virtual AtoN, does not
		'physically exist, may only be transmitted from an AIS station nearby under
		'the direction of a competent authority. (2)


		'Assigned Mode Flag         1 271   46 bit 0    0 = Station operating in autonomous and continuous mode =default

		'1 = Station operating in assigned mode

		'Spare                     1 272    46 bit 1   Spare. Not used. Should be set to zero.

		'Name of Aid-to-Navigation Extension
		'0, 6, 12,
		'18, 24,
		'30, 36,
		'This parameter of up to 14 additional 6-bit-ASCII characters for a 2-slot
		'message may be combined with the parameter "Name of Aid-to-
		'Navigation " at the end of that parameter, when more than 20 characters are"
		'30, 36,
		'... 84
		'Navigation " at the end of that parameter, when more than 20 characters are"
		'needed for the Name of the Aid-to-Navigation. This parameter should be
		'omitted when no more than 20 characters for the name of the A-to-N are
		'needed in total. Only the required number of characters should be
		'transmitted, i. e. no @-character should be used.
		'Spare 0, 2, 4,
		'or 6
		'Spare. Used only when parameter "Name of Aid-to-Navigation Extension"
		'is used. Should be set to zero. The number of spare bits should be adjusted
		'in order to observe byte boundaries.
		'Number of bits 272 – 360
		'Occupies two slots.
		*/


		ProcessAISNavAidMessage();

		return 0;
	}


	if (Msg_Type == 24)					// Class B Details
	{
		//Message ID		6 Identifier for Message 24; always 24
		//Repeat indicator	2 Used by the repeater to indicate how many times a message has been repeated. 0 = default; 3 = do not repeat any more
		//User ID				30 MMSI number
		//	char 2 bits 2-5, 3 4, 5, 6, 7 bits 0,1

		//Part number			2 Identifier for the message part number; always 0 for Part A
		//	char 7 bits 2,3

		B_PartNo=(val[7] & 15) >> 2;


		if (B_PartNo == 0)
		{

			//Name				120 Name of the MMSI-registered vessel. Maximum 20 characters 6-bit ASCII, @@@@@@@@@@@@@@@@@@@@ = not available = default
			//	char 7 bits 4,5 - char 26 bits 0-3

			for (i = 0; i <= 19; i++)
			{
				Conv = (((val[7 + i]) & 3) << 4) + (val[8 + i] >> 2);

				if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

				V_Name[i] = Conv;
			}


			//Total number of bits 160 = 26 chars 4 bits Occupies one-time period

			ProcessAISVesselDetails('0');

			return 0;
		}

		if (B_PartNo == 1)
		{
			//Type of ship and cargo type	8 0 = not available or no ship = default
			//									1-99 = as defined in § 3.3.8.2.3.2 of Annex 2
			//									100-199 = preserved, for regional use
			//									200-255 = preserved, for future use
			//	char 7 bits 4-5, char 8

			V_Type=((val[7] & 3) << 2) + val[8];  

			//Vendor ID						42	Unique identification of the Unit by a number as defined by the manufacturer (option; “@@@@@@@“ = not available = default)//	char 9-15

			for (i = 0; i <= 6; i++)
			{
				Conv = val[i+9];

				if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

				V_VendorID[i] = Conv;
			}


			//Call sign						42 Call sign of the MMSI-registered vessel. 7 X 6-bit ASCII characters, “@@@@@@@“ = not available = default
			//	char 16-22

			for (i = 0; i <= 6; i++)
			{
				Conv = val[i+16];

				if (Conv == 0) Conv = 32; else if (Conv < 32) Conv = Conv + 64;

				V_Callsign[i] = Conv;
			}

			//Dimension of ship/reference for position. Or, for unregistered daughter vessels, use the MMSI of the mother ship
			//								30 Dimensions of ship in meters and reference point for reported position (see Annex 2 Fig. 17 and § 3.3.8.2.3.3). Or, for an unregistered daughter vessel, use the MMSI of the associated mother ship in this data field
			//	char 23-27

			V_LenA = (val[23] << 3) + (val[24] >> 3);
			V_LenB = ((val[24] & 7) << 3) + val[25];
			V_WidthC = val[26];
			V_WidthD = val[27];


			//	Spare 6
			//	Total number of bits 168 (28 chars) Occupies one-time period


			ProcessAISVesselDetails('1');

			return 0;
		}
	}

	if (Msg_Type == 9)
	{
		//	Msg_Type = val[1];
		//Message ID 6 Identifier for message 9; always 9
		//Repeat Indicator 2 Used by the repeater to indicate how many times a message has beenrepeated. Refer to § 4.6.1; 0 - 3; default = 0; 3 = do not repeat any more.

		//	char 2 bits 2-5, 3 4, 5, 6, 7 bits 0,1
		//	UserID = ((val[2] & 15) << 26) + (val[3] << 20)+ (val[4] << 14)+ (val[5] << 8)+ (val[6] << 2) + (val[7] >>4); ;

		//Part number			2 Identifier for the message part number; always 0 for Part A
		//	char 7 bits 2,3

		//Altitude (GNSS) 12 Altitude 7 bits 2-5 8 9 bits 0, 1

		Alt = ((val[7] & 15) << 8) + (val[8] <<  2) + (val[9] >> 4);

		//'SOG   10     60  Char 9 bits 2-5 char 10 Speed over ground in knot steps (0-1022 knots)

		V_SOG = (((val[9] & 15) <<6) + val[10]); 


		//Position accuracy 1 Char 11 bit 0 1 = high (< 10 m; Differential Mode of e.g. DGNSS receiver) 0 = low(> 10 m; Autonomous Mode of e.g. GNSS receiver or of other ElectronicPosition Fixing Device) ; default = 0


		//Longitude										28	Longitude in 1/10 000 min (±180°, East = positive (as per 2's complement), West = negative (as per 2's complement), 181° (6791AC0 hex) = not available = default)
		//		Char 11 bits 1-5, 12, 13, 14, 15 0-4 		

		Conv = ((val[11] & 31) << 23) + (val[12] << 17) + (val[13] << 11) + (val[14] << 5) + (val[15] >> 1);

		if ((val[11] & 16) == 16) 
			Conv = -(0x10000000 - Conv);

		V_Lon = Conv;
		V_Lon/= 600000;


		//Latitude										27	Latitude in 1/10 000 min (±90°, North = positive (as per 2's complement), South = negative (as per 2's complement), 91° (3412140 hex) = not available = default)
		//		 Char 15 bit 5 16 17 18 19 20 bits 0-1

		Conv = ((val[15] & 1) << 26) + (val[16]<< 20) + (val[17]<< 14) + (val[18]<< 8) + (val[19]<< 2) + (val[20] >> 4);

		if ((val[15] & 1) == 1)
			Conv = -(0x8000000 - Conv);


		V_Lat = Conv;
		V_Lat/= 600000;

		//COG                12      128 char 20 bits 2-5 21 22 bit 0-1

		V_COG = ((val[20] & 15) << 8) + (val[21]<< 2) + (val[22] >>4);

		V_COG /= 10;

		//Time stamp 6 UTC second when the report was generated by the EPFS (0-59,or 60 if time stamp is not available, which should also be the defaultvalue,or 62 if Electronic Position Fixing System operates in estimated (deadreckoning) mode,or 61 if positioning system is in manual input modeor 63 if the positioning system is inoperative)
		//Reserved forregionalapplications 8 Reserved for definition by a competent regional authority. Should be setto zero, if not used for any regional application. Regional applicationsshould not use zero.
		//DTE 1 Data terminal ready (0 = available 1 = not available = default) (refer to§ 3.3.8.2.3.1).
		//Spare 5 3 Not used. Should be set to zero
		//Assigned ModeFlag1 0 = Station operating in autonomous and continous mode =default1 = Station operating in assigned mode
		//RAIM-Flag 1 RAIM (Receiver Autonomous Integrity Monitoring) flag of ElectronicPosition Fixing Device; 0 = RAIM not in use = default; 1 = RAIM in use)Communication
		//State SelectorFlag1 0 = SOTDMA Communication State follows;1 = ITDMA Communication State follows.
		//Communication State 19 SOTDMA (refer to § 3.3.7.2.2).
		//Total number of bits168

		ProcessSARMessage();

	}

	return Msg_Type;


	/*
	'@   0   0x00    00 0000 64  0x40    0100 0000   !   33  0x21    10 0001 33  0x21    0010 0001
	'A   1   0x01    00 0001 65  0x41    0100 0001   "   34  0x22    10 0010 34  0x22    0010 0010
	'B   2   0x02    00 0010 66  0x42    0100 0010   #   35  0x23    10 0011 35  0x23    0010 0011
	'C   3   0x03    00 0011 67  0x43    0100 0011   $   36  0x24    10 0100 36  0x24    0010 0100
	'D   4   0x04    00 0100 68  0x44    0100 0100   %   37  0x25    10 0101 37  0x25    0010 0101
	'E   5   0x05    00 0101 69  0x45    0100 0101   &   38  0x26    10 0110 38  0x26    0010 0110
	'F   6   0x06    00 0110 70  0x46    0100 0110   `   39  0x27    10 0111 39  0x27    0010 0111
	'G   7   0x07    00 0111 71  0x47    0100 0111   (   40  0x28    10 1000 40  0x28    0010 1000
	'H   8   0x08    00 1000 72  0x48    0100 1000   )   41  0x29    10 1001 41  0x29    0010 1001
	'I   9   0x09    00 1001 73  0x49    0100 1001   *   42  0x2A    10 1010 42  0x2A    0010 1010
	'J   10  0x0A    00 1010 74  0x4A    0100 1010   +   43  0x2B    10 1011 43  0x2B    0010 1011
	'K   11  0x0B    00 1011 75  0x4B    0100 1011   ´   44  0x2C    10 1100 44  0x2C    0010 1100
	'L   12  0x0C    00 1100 76  0x4C    0100 1100   -   45  0x2D    10 1101 45  0x2D    0010 1101
	'M   13  0x0D    00 1101 77  0x4D    0100 1101   ,   46  0x2E    10 1110 46  0x2E    0010 1110
	'N   14  0x0E    00 1110 78  0x4E    0100 1110   /   47  0x2F    10 1111 47  0x2F    0010 1111
	'O   15  0x0F    00 1111 79  0x4F    0100 1111   0   48  0x30    11 0000 48  0x30    0011 0000
	'P   16  0x10    01 0000 80  0x50    0101 0000   1   49  0x31    11 0001 49  0x31    0011 0001
	'Q   17  0x11    01 0001 81  0x51    0101 0001   2   50  0x32    11 0010 50  0x32    0011 0010
	'R   18  0x12    01 0010 82  0x52    0101 0010   3   51  0x33    11 0011 51  0x33    0011 0011
	'S   19  0x13    01 0011 83  0x53    0101 0011   4   52  0x34    11 0100 52  0x34    0011 0100
	'T   20  0x14    01 0100 84  0x54    0101 0100   5   53  0x35    11 0101 53  0x35    0011 0101
	'U   21  0x15    01 0101 85  0x55    0101 0101   6   54  0x36    11 0110 54  0x36    0011 0110
	'V   22  0x16    01 0110 86  0x56    0101 0110   7   55  0x37    11 0111 55  0x37    0011 0111
	'W   23  0x17    01 0111 87  0x57    0101 0111   8   56  0x38    11 1000 56  0x38    0011 1000
	'X   24  0x18    01 1000 88  0x58    0101 1000   9   57  0x39    11 1001 57  0x39    0011 1001
	'Y   25  0x19    01 1001 89  0x59    0101 1001   :   58  0x3A    11 1010 58  0x3A    0011 1010
	'Z   26  0x1A    01 1010 90  0x5A    0101 1010   ;   59  0x3B    11 1011 59  0x3B    0011 1011
	'[   27  0x1B    01 1011 91  0x5B    0101 1011   <   60  0x3C    11 1100 60  0x3C    0011 1100
	'\   28  0x1C    01 1100 92  0x5C    0101 1100   =   61  0x3D    11 1101 61  0x3D    0011 1101
	']   29  0x1D    01 1101 93  0x5D    0101 1101   >   62  0x3E    11 1110 62  0x3E    0011 1110
	'^   30  0x1E    01 1110 94  0x5E    0101 1110   ?   63  0x3F    11 1111 63  0x3F    0011 1111
	'_   31  0x1F    01 1111 95  0x5F    0101 1111
	'[SP]    32  0x20    10 0000 32  0x20    0010 0000

	*/

	return Msg_Type;

}



int ConvertAsciito6Bit(char * msg, char * buffer, int Len)
{
	int i;
	
	for (i = 0; i< Len; i++)
	{
		buffer[i]=conv[msg[i]];
	}

    return 0;
}

void ProcessSARMessage()
{
	int i;
	struct SARRECORD * ptr;

	for (i = 0; i < SARCount; i++)
	{
		ptr=SARRecords[i];

		if (ptr->ID == UserID)
		{
			if (V_Lat < 50 || V_Lat > 70)
				return;

			ptr->lat = V_Lat;
			ptr->Long = V_Lon;
			ptr->course = V_COG;
			ptr->speed = V_SOG;
			ptr->Alt = Alt;

			ptr->TimeLastUpdated = NOW;

			SaveSARTrackPoint(ptr);
			return;
		}
	}

	if (SARCount == 0)
		SARRecords = (struct SARRECORD **)malloc(sizeof(void *));
	else
		SARRecords = (struct SARRECORD **)realloc(SARRecords,(SARCount+1)*sizeof(void *));

	ptr = (struct SARRECORD *)malloc(sizeof(struct SARRECORD));

	if (ptr == NULL) return;

	memset(ptr, 0, sizeof(struct SARRECORD));

	SARRecords[SARCount] = ptr;

	ptr->ID = UserID;
	ptr->TimeAdded = NOW;

	SARCount++;

	LookupVessel(UserID);	

	if (ShipDataRec != NULL)
	{
		memcpy(ptr->name, ShipDataRec->name, 21);
	}

	ProcessSARMessage();

	return;
}



void ProcessAISVesselMessage()
{
	int i;
	struct TARGETRECORD * ptr;
	int Length;
	int Dimensions[10];

	if (V_Lat > 90.0)
		return;

	for (i = 0; i < TargetCount; i++)
	{
		ptr=TargetRecords[i];
 
	    if (ptr->ID == UserID)
		{       
            ptr->lat = V_Lat;
            ptr->Long = V_Lon;
            ptr->course = V_COG;
            if (V_COG == 360)  ptr->course = V_Heading; //' 360 means not available
            ptr->speed = V_SOG;
            ptr->NavStatus = NavStatus;
            ptr->ROT = V_ROT;
            ptr->Heading = V_Heading;
    
            ptr->TimeLastUpdated = NOW;

			SaveTrackPoint(ptr);
    			
			return;

		}
	}

//   Not found - add on end

	if (TargetCount == 0)

		TargetRecords=(struct TARGETRECORD **)malloc(sizeof(void *));
	else
		TargetRecords=(struct TARGETRECORD **)realloc(TargetRecords,(TargetCount+1)*sizeof(void *));

	ptr=(struct TARGETRECORD *)malloc(sizeof(struct TARGETRECORD));
	memset(ptr, 0, sizeof(struct TARGETRECORD));

	if (ptr == NULL) return;
	
	TargetRecords[TargetCount]=ptr;
        
	ptr->ID = UserID;
	ptr->TimeAdded = NOW;

	TargetCount++;

	// See if vessel is in database - if not, add

	LookupVessel(UserID);								// Find Vessel - if not present, add it

	if (ShipDataRec != NULL)					// Malloc failed!!
	{
		memcpy(ptr->name,ShipDataRec->name, 21);
		memcpy(ptr->Callsign, ShipDataRec->Callsign, 8);
		ptr->Type = ShipDataRec->Type;
		ptr->LengthA = ShipDataRec->LengthA;
		ptr->LengthB = ShipDataRec->LengthB;
		ptr->WidthC = ShipDataRec->WidthC;
		ptr->WidthD = ShipDataRec->WidthD;

		V_LenA = ShipDataRec->LengthA;
		V_LenB = ShipDataRec->LengthB;
		V_WidthC = ShipDataRec->WidthC;
		V_WidthD = ShipDataRec->WidthD;
		
		Length = V_LenA + V_LenB;

		if(Length > 0)
		{
			Dimensions[0] = -V_WidthC;
		    Dimensions[1] = -V_LenB;
		    Dimensions[2] = V_WidthD;
			Dimensions[3] = -V_LenB;
			Dimensions[4] = V_WidthD;
			Dimensions[5] = V_LenA - Length / 5;
			Dimensions[6] = ((V_WidthC + V_WidthD) / 2) - V_WidthC;
			Dimensions[7] = V_LenA;
			Dimensions[8] = -V_WidthC;
			Dimensions[9] = V_LenA - Length / 5;
		}
	}

	ProcessAISVesselMessage();

	return;
}

void ProcessBaseStationMessage()
{
	int i;
	struct BASESTATIONRECORD * ptr;

	for (i = 0; i < BaseStationCount; i++)
	{
		ptr=BaseStationRecords[i];
 
	    if (ptr->ID == UserID)
		{
			ptr->lat = V_Lat;
			ptr->Long = V_Lon;
			ptr->FixType = FixType;

			if (SlotTO == 3 || SlotTO == 5 || SlotTO == 7)
				ptr->NumberofStations = SubMsg;

			ptr->TimeLastUpdated = NOW;

			return;

		}

	}
 
	if (BaseStationCount == 0)

		BaseStationRecords=(struct BASESTATIONRECORD **)malloc(sizeof(void *));
	else
		BaseStationRecords=(struct BASESTATIONRECORD **)realloc(BaseStationRecords,(BaseStationCount+1)*sizeof(void *));

	ptr=(struct BASESTATIONRECORD *)malloc(sizeof(struct BASESTATIONRECORD));

	if (ptr == NULL) return;
	memset(ptr, 0, sizeof(struct BASESTATIONRECORD));
	
	BaseStationRecords[BaseStationCount]=ptr;
        
	ptr->ID = UserID;
	ptr->TimeAdded = NOW;

	BaseStationCount++;

	ProcessBaseStationMessage();

	return;
}


void ProcessAISVesselDetails(char Type)
{
	int i;
	struct TARGETRECORD * ptr;
	int Dimensions[10];
	int Length;

	for (i = 0; i < TargetCount; i++)
	{
		ptr=TargetRecords[i];
 
	    if (ptr->ID == UserID)
		{
			if (Type == 'A')
			{
				memcpy(ptr->name, V_Name, 21);
				ptr->IMO = V_IMO;
				memcpy(ptr->Callsign,V_Callsign,8);
				memcpy(ptr->Dest, V_Dest, 21);
				ptr->Type = V_Type;
				ptr->LengthA = V_LenA;
				ptr->LengthB = V_LenB;
				ptr->WidthC = V_WidthC;
				ptr->WidthD = V_WidthD;
				ptr->Class='A';
			}

			if (Type == 'B')					// Type 19 Class B Details
			{
				memcpy(ptr->name, V_Name, 21);
				ptr->Type = V_Type;
				ptr->LengthA = V_LenA;
				ptr->LengthB = V_LenB;
				ptr->WidthC = V_WidthC;
				ptr->WidthD = V_WidthD;
			}

			if (Type == '0')				// Class B record 24A
			{
				memcpy(ptr->name, V_Name, 21);
			}

			if (Type == '1')				// Class B record 24B
			{
				memcpy(ptr->Callsign,V_Callsign,8);
				ptr->Type = V_Type;
				ptr->LengthA = V_LenA;
				ptr->LengthB = V_LenB;
				ptr->WidthC = V_WidthC;
				ptr->WidthD = V_WidthD;
			}

			ptr->TimeLastUpdated = NOW;

			Length = V_LenA + V_LenB;

			if(Length > 0)
			{
				Dimensions[0] = -V_WidthC;
		        Dimensions[1] = -V_LenB;
			    Dimensions[2] = V_WidthD;
				Dimensions[3] = -V_LenB;
	            Dimensions[4] = V_WidthD;
		        Dimensions[5] = V_LenA - Length / 5;
			    Dimensions[6] = ((V_WidthC + V_WidthD) / 2) - V_WidthC;
			    Dimensions[7] = V_LenA;
				Dimensions[8] = -V_WidthC;
				Dimensions[9] = V_LenA - Length / 5;
			}
       
			MaintainVesselDatabase(UserID, Type);
 
		return;
		}
	}
 
//   Not found

	if (TargetCount == 0)
		TargetRecords=(struct TARGETRECORD **)malloc(sizeof(void *));
	else
		TargetRecords=(struct TARGETRECORD **)realloc(TargetRecords,(TargetCount+1)*sizeof(void *));

	ptr=(struct TARGETRECORD *)malloc(sizeof(struct TARGETRECORD));
	memset(ptr, 0, sizeof(struct TARGETRECORD));

	if (ptr == NULL) return;
	
	TargetRecords[TargetCount]=ptr;
        
	ptr->ID = UserID;

	if (Type == 'A') ptr->Class='A'; else ptr->Class='B';

	ptr->TimeAdded = NOW;

	TargetCount++;

	sprintf(ptr->name,"%d",UserID);

	memcpy(ptr->name, V_Name,21);

	ProcessAISVesselDetails(Type);

	return;
}


void ProcessAISNavAidMessage()
{
	int i;
	struct NAVAIDRECORD * navptr;

	for (i = 0; i < NavAidCount; i++)
	{
		navptr=NavRecords[i];
 
	    if (navptr->ID == UserID)
		{
		    memcpy(navptr->name,V_Name,21);
			navptr->lat = V_Lat;
			navptr->Long = V_Lon;
			navptr->LengthA = V_LenA;
			navptr->LengthB = V_LenB;
			navptr->WidthC = V_WidthC;
			navptr->WidthD = V_WidthD;

			navptr->NavAidType = NavAidType;
			navptr->FixType = FixType;
             
			navptr->TimeLastUpdated = NOW;
			NavAidDBChanged = 1;
	
		    return;
		}
	}

	// Not found, so add

	if (NavAidCount == 0)
		NavRecords=(struct NAVAIDRECORD **)malloc(sizeof(void *));
	else
		NavRecords=(struct NAVAIDRECORD **)realloc(NavRecords,(NavAidCount+1)*sizeof(void *));

	navptr=(struct NAVAIDRECORD *)malloc(sizeof(struct NAVAIDRECORD));

	if (navptr == NULL) return;
	
	NavRecords[NavAidCount]=navptr;

	navptr->ID = UserID;
	navptr->TimeAdded = NOW;

	NavAidCount++;

	ProcessAISNavAidMessage();
	NavAidDBChanged = 1;
	
	
	return;
	
}

static double Distance(double lah, double loh, double laa, double loa)
{

	//'   Our Lat, Long other Lat,Long

/*

'Great Circle Calculations.

'dif = longitute home - longitute away


'      (this should be within -180 to +180 degrees)
'      (Hint: This number should be non-zero, programs should check for
'             this and make dif=0.0001 as a minimum)
'lah = latitude of home
'laa = latitude of away

'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
'distance = dis / 180 * pi * ERAD
'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians
*/

loh = radians(loh); lah = radians(lah);
loa = radians(loa); laa = radians(laa);

return 60*degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa-loh)));

}

static double Bearing(double lat1, double lon1, double lat2, double lon2)
{

//' Our Lat, Long other Lat,Long
 
	double dlat, dlon, TC1;

	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;
	
	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);
		
	if (fabs(TC1) > 89.5) if (dlon > 0) return 90; else return 270;

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}

	return 0;

}

void AISTimer()
{
	// Entered every minute

	NOW = time(NULL);

	CheckAgeofTargets(MaxAge);

	if (VessselDBChanged)
	{
		VessselDBChanged = 0;
		SaveVesselDataBase();
	}

	if (NavAidDBChanged)
	{
		NavAidDBChanged = 0;
		SaveNavAidDataBase();
	}
}

void ADSBTimer()
{
	// Entered every minute

	NOW = time(NULL);
	CheckAgeofPlanes(300);
	ProcessADSBJSON(ADSBFN);

//	WriteMiniDump();

	if (ADSBHost[0])
	{
		if (ADSBConnected == 0)		// Try to connect again
			_beginthread(ADSBConnect, 0, 0);
	}
}

void CheckAgeofTargets(UINT MaxAge)
{
	int i, j=0;
	struct TARGETRECORD * ptr;

	for (i = 0; i < TargetCount; i++)
	{
		ptr=TargetRecords[i];
		
		if (NOW - ptr->TimeLastUpdated > MaxAge)
		{       
			free(ptr);						// Release Memory Block
    
			//   Delete Current, and move all others down
                  
			TargetCount--;

			while (i < TargetCount)
			{
	              TargetRecords[i] = TargetRecords[i+1];
				  i++;
			}

			return;				// Test rest on next pass
		}
	}
}



void CheckAgeofPlanes(UINT MaxAge)
{
	int i, j=0;
	struct ADSBRECORD * ptr;
	
	for (i = 0; i < ADSBCount; i++)
	{
		ptr = ADSBRecords[i];

		if (ptr->hex[0] && NOW - ptr->seen_pos > MaxAge)
			memset(ptr, 0, sizeof(struct ADSBRECORD));		// Zap the record						// Release Memory Block
    }
}

int GetAISPageInfo(char * Buffer, int ais, int adsb)
{
	int i;
	int Len = 0;
	struct NAVAIDRECORD * navptr;
	struct TARGETRECORD * ptr;
	char Line[280];
	int Age;
	char * pType;
	char * Name;
	char nameString[32];
	struct tm * TM;
	char TimeAdded[50];
	char TimeHeard[50];
	char * p;
	char DestBuffer[128] = "";

	NOW = time(NULL);

	if (adsb)
		ProcessADSBJSON(ADSBFN);

	if (Lat != 0.0 && Lon != 0)
		Len += sprintf(&Buffer[Len],"H,%f,%f,\r\n|", Lat, Lon);

	if (ais)
	{
		for (i = 0; i < NavAidCount; i++)
		{
			navptr=NavRecords[i];

			if (strchr(navptr->name, '>') || strchr(navptr->name, '<'))
				continue;

			TM = gmtime(&navptr->TimeAdded);

			sprintf(TimeAdded, "<br>Created %02d:%02d %02d/%02d/%04d",
				TM->tm_hour, TM->tm_min, TM->tm_mday, TM->tm_mon + 1, TM->tm_year + 1900);

			TM = gmtime(&navptr->TimeLastUpdated);

			sprintf(TimeHeard, "<br>Last Heard %02d:%02d %02d/%02d/%04d",
				TM->tm_hour, TM->tm_min, TM->tm_mday, TM->tm_mon + 1, TM->tm_year + 1900);

			Len += sprintf(&Buffer[Len],"N,%f,%f,%s%s%s,\r\n|",
				navptr->lat, navptr->Long, navptr->name, TimeAdded, TimeHeard);
		}

		// Vessels

		for (i = 0; i < TargetCount; i++)
		{
			ptr = TargetRecords[i];

			Age = NOW - ptr->TimeLastUpdated;

			if (Age > 3600)
				continue;

			if (ptr->Type >= 30 && ptr->Type < 40)
				pType=VesselType3[ptr->Type-30];
			else
				if (ptr->Type >= 50 && ptr->Type < 60)
					pType=VesselType5[ptr->Type-50];
				else
					pType=VesselType[ptr->Type/10];

			if (ptr->name[0])
				Name = ptr->name;
			else
			{
				sprintf(nameString, "%d", ptr->ID);
				Name = nameString;
			}

			// Dest may contain comma

			strcpy(DestBuffer, ptr->Dest);

			while (p = strchr(DestBuffer, ','))
			{
				memmove(p + 4, p, strlen(p));
				memcpy(p, "&#44;", 5);
			}

			sprintf(Line,"%s<br>Dest %s<br>MMSI %d<br>Callsign %s<br>Type %d<br>Heading %2.0f&deg; ROT %2.1f<br>Dimensions %d %d %d %d<br>%2.1f kts %2.0f&deg;<br>Age %d",
				Name, DestBuffer, ptr->ID, ptr->Callsign, ptr->Type , ptr->Heading, ptr->ROT,
				ptr->LengthA, ptr->LengthB, ptr->WidthC, ptr->WidthD,
				ptr->speed, ptr->course, Age);

			Len += sprintf(&Buffer[Len],"V,%.4f,%.4f,%s,%.0f,%.1f,%d\r\n|",
				ptr->lat, ptr->Long, Line, ptr->course, ptr->speed, Age);

			if (ptr->TrackTime[0])	// Have trackpoints
			{
				int n = ptr->Trackptr;
				int i;

				// We read from next track point (oldest) for TRACKPOINT records, ignoring zeros

				Len += sprintf(&Buffer[Len],"T,");

				for (i = 0; i < TRACKPOINTS; i++)
				{
					if (ptr->TrackTime[n])
					{
						Len += sprintf(&Buffer[Len],"%.4f,%.4f,", ptr->LatTrack[n], ptr->LonTrack[n]);
					}

					n++;
					if (n == TRACKPOINTS)
						n = 0;
				}
				Len += sprintf(&Buffer[Len],"%.4f,%.4f,\r\n|", ptr->lat, ptr->Long);		//Add current position to end of track
			}
		}

		// SAR Vessels

		for (i = 0; i < SARCount; i++)
		{
			struct SARRECORD * ptr = SARRecords[i];

			Age = NOW - ptr->TimeLastUpdated;

			if (Age > 3600)
				continue;

			if (ptr->name[0])
				Name = ptr->name;
			else
			{
				sprintf(nameString, "%d", ptr->ID);
				Name = nameString;
			}

			sprintf(Line,"%s<br>Dest %s<br>MMSI %d<br>%2.1f kts %2.0f&deg;<br>Age %d",
				Name, ptr->Dest, ptr->ID, ptr->speed, ptr->course, Age);

			Len += sprintf(&Buffer[Len],"S,%.4f,%.4f,%s,%.0f,%.0f,%d\r\n|",
				ptr->lat, ptr->Long, Line, ptr->course, ptr->speed, Age);

			if (ptr->TrackTime[0])	// Have trackpoints
			{
				int n = ptr->Trackptr;
				int i;

				// We read from next track point (oldest) for TRACKPOINT records, ignoring zeros

				Len += sprintf(&Buffer[Len],"T,");

				for (i = 0; i < TRACKPOINTS; i++)
				{
					if (ptr->TrackTime[n])
					{
						Len += sprintf(&Buffer[Len],"%.4f,%.4f,", ptr->LatTrack[n], ptr->LonTrack[n]);
					}

					n++;
					if (n == TRACKPOINTS)
						n = 0;
				}
				Len += sprintf(&Buffer[Len],"%.4f,%.4f,\r\n|", ptr->lat, ptr->Long);		//Add current position to end of track
			}
		}

	}

	if (adsb)
	{
		// Aircraft


		for (i = 0; i < ADSBCount; i++)
		{
			struct ADSBRECORD * ptr = ADSBRecords[i];

			Age = NOW - ptr->seen_pos;

			if (Age > 600)
				continue;

			if (ptr->lat == 0.0 && ptr->lon == 0.0)
				continue;

			sprintf(Line,"%s<br>Flight %s<br>Heading %d&deg;<br>Speed %d kts<br>Altitude %d feet<br>rssi %d<br>Age %d",
				ptr->hex, ptr->flight, ptr->track, ptr->speed, ptr->altitude, ptr->rssi, Age);

			Len += sprintf(&Buffer[Len],"P,%.4f,%.4f,%s,%s,%d,%d,%d\r\n|",
				ptr->lat, ptr->lon, Line, ptr->flight, ptr->track, ptr->speed, Age);

			if (ptr->TrackTime[0])	// Have trackpoints
			{
				int n = ptr->Trackptr;
				int i;

				// We read from next track point (oldest) for TRACKPOINT records, ignoring zeros

				Len += sprintf(&Buffer[Len],"T,");

				for (i = 0; i < TRACKPOINTS; i++)
				{
					if (ptr->TrackTime[n])
					{
						Len += sprintf(&Buffer[Len],"%.4f,%.4f,", ptr->LatTrack[n], ptr->LonTrack[n]);
					}

					n++;
					if (n == TRACKPOINTS)
						n = 0;
				}
				Len += sprintf(&Buffer[Len],"%.4f,%.4f,\r\n|", ptr->lat, ptr->lon);		//Add current position to end of track
			}
		}
	}
	return Len;
}

/*
{ "now" : 1634055090.3,
  "messages" : 28814,
  "aircraft" : [
    {"hex":"4b8467","squawk":"1146","flight":"RUN410  ","lat":58.554749,"lon":-7.728350,"nucp":7,"seen_pos":0.0,"altitude":37000,"vert_rate":0,"track":111,"speed":537,"messages":197,"seen":0.0,"rssi":-32.7},
    {"hex":"abed10","squawk":"1170","flight":"FDX4    ","lat":57.969498,"lon":-7.888449,"nucp":7,"seen_pos":7.2,"altitude":37000,"vert_rate":0,"track":137,"speed":553,"messages":588,"seen":0.1,"rssi":-34.6},
    {"hex":"4007dd","squawk":"2000","altitude":1825,"messages":180,"seen":37.8,"rssi":-33.4}
  ]
}
*/

void ProcessADSBMessage(struct ADSBRECORD * plane);
void ProcessADSBLine(char * Msg, int Len);

void ProcessADSBJSON(char * FN)
{
	FILE *file;
	char buf[2048];
	char *token;
	struct ADSBRECORD plane;
	time_t ReportTime;

	// Testing, read dump1090 file

	if (ADSBHost[0])
		return;						// Using TCP 
	
	if ((file = fopen(FN,"r")) == NULL) return;
	
	fgets(buf, 2048, file);		// now
	ReportTime = atoi(&buf[9]);
	fgets(buf, 2048, file);		// messages
	fgets(buf, 2048, file);		// aircraft

	while (1)
	{
		if (fgets(buf, 2048, file) == 0)
		{
			fclose(file);
			return;
		}

		if (strchr(buf, ']'))
		{
			fclose(file);
			return;
		}

		memset(&plane, 0, sizeof(struct ADSBRECORD));

		if (token = strstr(buf, "hex"))
		{
			memcpy(plane.hex, &token[6], 7);
			strlop(plane.hex, '"');
		}

		if (token = strstr(buf, "squawk"))
		{
			memcpy(plane.squawk, &token[9], 4);
		}

		if (token = strstr(buf, "flight"))
		{
			memcpy(plane.flight, &token[9], 8);
		}

		if (token = strstr(buf, "lat"))
		{
			plane.lat = atof(&token[5]);
		}

		if (token = strstr(buf, "lon"))
		{
			plane.lon = atof(&token[5]);
		}

		if (token = strstr(buf, "seen_pos"))
		{
			plane.seen_pos = ReportTime - atoi(&token[10]);
		}

		if (token = strstr(buf, "altitude"))
		{
			plane.altitude = atoi(&token[10]);
		}

		if (token = strstr(buf, "vert_rate"))
		{
			plane.vert_rate = atoi(&token[11]);
		}

		if (token = strstr(buf, "track"))
		{
			plane.track = atoi(&token[7]);
		}

		if (token = strstr(buf, "speed"))
		{
			plane.speed = atoi(&token[7]);
		}

		if (token = strstr(buf, "messages"))
		{
			plane.messages = atoi(&token[10]);
		}

		if (token = strstr(buf, "seen"))
		{
			plane.seen = ReportTime - atoi(&token[6]);
		}


		if (token = strstr(buf, "rssi"))
		{
			plane.rssi = atoi(&token[6]);
		}
		ProcessADSBMessage(&plane);	
	}
}

void ProcessADSBMessage(struct ADSBRECORD * plane)
{
	int i;
	struct ADSBRECORD * ptr;
	struct ADSBRECORD * spare = 0;		// Pointer to a cleared record

	for (i = 0; i < ADSBCount; i++)
	{
		ptr = ADSBRecords[i];
 
	    if (strcmp(ptr->hex, plane->hex) == 0)
		{
			if (plane->squawk[0])
				strcpy(ptr->squawk,plane->squawk);

			if (plane->flight[0])
				strcpy(ptr->flight, plane->flight);

			if (plane->lat)
				ptr->lat = plane->lat;

			if (plane->lon)
				ptr->lon = plane->lon;
			
			if (plane->altitude)
				ptr->altitude = plane->altitude;

			if (plane->seen_pos)
				ptr->seen_pos = plane->seen_pos;
		
			if (plane->vert_rate)
				ptr->vert_rate = plane->vert_rate;

			if (plane->track)
				ptr->track = plane->track;

			if (plane->speed)
				ptr->speed = plane->speed;

			if (plane->messages)
				ptr->messages = plane->messages;

			if (plane->seen)
				ptr->seen = plane->seen;

			if (plane->rssi)
				ptr->rssi = plane->rssi;

			ptr->TimeLastUpdated = ptr->seen;

			if (ptr->lat == 0.0)
				return;

			// Save Track Point

			if ((NOW - ptr->LastTime) < 15)	// Not More that once per 15 secs
				return;

			ptr->LastCourse = ptr->track;
			ptr->LastSpeed = ptr->speed;
			ptr->LastTime = NOW;
			ptr->Lastlat = ptr->lat;
			ptr->LastLong = ptr->lon;

			ptr->LatTrack[ptr->Trackptr] = ptr->lat;
			ptr->LonTrack[ptr->Trackptr] = ptr->lon;
			ptr->TrackTime[ptr->Trackptr] = NOW;

			ptr->Trackptr++;

			if (ptr->Trackptr == TRACKPOINTS)
				ptr->Trackptr = 0;

			return;
		}

		if (ptr->hex[0] == 0 && spare == 0)			//Zapped record
			spare = ptr;
	}

	// Not found, if got a freed entry use it, elsee add new one

	if (spare)
		ptr = spare;
	else
	{
		if (ADSBCount == 0)
			ADSBRecords = (struct ADSBRECORD **)malloc(sizeof(void *));
		else
			ADSBRecords = (struct ADSBRECORD **)realloc(ADSBRecords,(ADSBCount+1)*sizeof(void *));

		ptr = (struct ADSBRECORD *)zalloc(sizeof(struct ADSBRECORD));

		if (ptr == NULL) return;
	
		ADSBRecords[ADSBCount] = ptr;
		ADSBCount++;
	}

	strcpy(ptr->hex, plane->hex);
	ptr->TimeAdded = NOW;

	ProcessADSBMessage(plane);			// Reenter to save details
	return;
}


// Thread to process ADS-B messages from dump1090


void ProcessADSBLine(char * Msg, int Len)
{
	int i;
	struct ADSBRECORD * rec;
	struct ADSBRECORD * spare = 0;		// Pointer to a cleared record

	char * p[22] = {0};
	char * ptr;
	char Type;
	char hex[10];

//MSG,3,111,11111,394A05,111111,2021/10/13,07:56:46.075,2021/10/13,07:56:46.128,,37000,,,59.11278,-6.53052,,,,,,0

	if (memcmp(Msg, "MSG,", 4) != 0)
		return;

	// Simplest way to process comma separated with null fields is strlop;

	ptr = &Msg[4];

	for (i = 0; i < 21; i++)
	{
		p[i] = ptr;
		ptr = strlop(ptr, ',');
	}
	
	if (p[20] == 0)
		return;		// Should have 21 params

	strcpy(hex, p[3]);				// identifier

	for (i = 0; i < ADSBCount; i++)
	{
		rec = ADSBRecords[i];
 
	    if (strcmp(rec->hex, hex) == 0)
			goto Found;

		if (rec->hex[0] == 0 && spare == 0)			//Zapped record
			spare = rec;
	}
	
	if (spare)
		rec = spare;
	else
	{
		// Not found, so add

		if (ADSBCount == 0)
			ADSBRecords = (struct ADSBRECORD **)malloc(sizeof(void *));
		else
			ADSBRecords = (struct ADSBRECORD **)realloc(ADSBRecords,(ADSBCount+1) * sizeof(void *));

		rec = (struct ADSBRECORD *)zalloc(sizeof(struct ADSBRECORD));

		if (rec == NULL) return;

		ADSBRecords[ADSBCount] = rec;
		ADSBCount++;
	}

	strcpy(rec->hex, hex);
	rec->TimeAdded = NOW;

Found:

	rec->seen = NOW;
	rec->messages++;

	Type = atoi(p[0]);

	switch (Type)
	{
	case 1:

		strcpy(rec->flight, p[9]);
		return;

	case 2:

		rec->altitude = atoi(p[10]);
		rec->speed = atoi(p[11]);
		rec->track = atoi(p[12]);
		rec->lat = atof(p[13]);
		rec->lon = atof(p[14]);
		rec->seen_pos = NOW;
		break;

	case 3:

		// Position

		rec->altitude = atoi(p[10]);
		rec->lat = atof(p[13]);
		rec->lon = atof(p[14]);
		rec->seen_pos = NOW;
		break;

	case 4:

		rec->speed = atoi(p[11]);
		rec->track = atoi(p[12]);
		rec->vert_rate = atoi(p[15]);
		return;

	case 5:
		return;

	case 6:

		strcpy(rec->squawk, p[16]);
		return;

	case 7:
	case 8:
		return;
	}

	// Recs with position get here

	if (rec->lat == 0.0)
		return;

	// Save Track Point

//	if ((NOW - rec->LastTime) < 15)	// Not More that once per 15 secs
//		return;

	rec->LastCourse = rec->track;
	rec->LastSpeed = rec->speed;
	rec->LastTime = NOW;
	rec->Lastlat = rec->lat;
	rec->LastLong = rec->lon;

	rec->LatTrack[rec->Trackptr] = rec->lat;
	rec->LonTrack[rec->Trackptr] = rec->lon;
	rec->TrackTime[rec->Trackptr] = NOW;

	rec->Trackptr++;

	if (rec->Trackptr == TRACKPOINTS)
		rec->Trackptr = 0;
}

static VOID ProcessADSBData(SOCKET TCPSock)
{
	char Buffer[66000];
	char Msg[1024];

	int len;

	char * ptr;
	char * Lastptr;

	// Although it is possible that a packet will be split by a partial read.
	// it is so unlikely with a 65536 buffer that I'll just accept the loss

	len = recv(TCPSock, Buffer, 65536, 0);

	if (len <= 0)
	{
		closesocket(TCPSock);
		ADSBConnected = FALSE;
		return;
	}

	if (len > 5000)
		Debugprintf("ADSB Len %d", len);

	ptr = Lastptr = Buffer;

	Buffer[len] = 0;

	while (len > 0)
	{
		ptr = strchr(Lastptr, 10);

		if (ptr)
		{
			size_t Len = ptr - Lastptr - 1;

			if (Len > 1020)
				return;
		
			memcpy(Msg, Lastptr, Len);
			Msg[Len++] = 13;
			Msg[Len++] = 10;
			Msg[Len] = 0;

			ProcessADSBLine(Msg, Len);

			Lastptr = ptr + 1;
			len -= (int)Len;
		}
		else
			return;
	}
}

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

static VOID ADSBConnect(void * unused)
{
	int err, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	struct sockaddr_in destaddr;
	SOCKET TCPSock;
	int addrlen=sizeof(sinx);

	if (ADSBHost[0] == 0)
		return;

	destaddr.sin_addr.s_addr = inet_addr(ADSBHost);
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(ADSBPort);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		struct hostent * HostEnt = gethostbyname(ADSBHost);
		 
		if (!HostEnt)
			return;			// Resolve failed

		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}


	TCPSock = socket(AF_INET,SOCK_STREAM,0);

	if (TCPSock == INVALID_SOCKET)
	{
  	 	return; 
	}
 
	setsockopt (TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	ADSBConnected = TRUE;			// So we don't try to reconnect while waiting		

	if (connect(TCPSock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		ioctl(TCPSock, FIONBIO, &param);
	}
	else
	{
		err=WSAGetLastError();
#ifdef LINBPQ
   		printf("Connect Failed for ADSB socket - error code = %d\n", err);
#else
   		Debugprintf("Connect Failed for ADSB socket - error code = %d", err);
#endif		
		closesocket(TCPSock);
		ADSBConnected = FALSE;

		return;
	}

	ADSBConnected = TRUE;

	while (TRUE)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TCPSock,&readfs);
		FD_SET(TCPSock,&errorfs);

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select((int)TCPSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TCPSock, &readfs))
			{
				ProcessADSBData(TCPSock);			
			}
								
			if (FD_ISSET(TCPSock, &errorfs))
			{
Lost:				
#ifdef LINBPQ
				printf("ADSB Connection lost\n");
#endif			
				closesocket(TCPSock);
				ADSBConnected = FALSE;
				return;
			}
		}
		else
		{
			// 60 secs without data. Shouldn't happen

			shutdown(TCPSock, SD_BOTH);
			Sleep(100);

			closesocket(TCPSock);
			ADSBConnected = FALSE;
			return;
		}
	}
}

//http://fr24.com/+selected.flight
//http://www.flightstats.com/go/FlightStatus/flightStatusByFlight.do?flightNumber='+selected.flight