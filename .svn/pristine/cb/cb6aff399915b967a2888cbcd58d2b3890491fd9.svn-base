// ADIF Logging Bits

typedef struct ADIF
{
	char Call[16];
	time_t StartTime;
	int Mode;
	char LOC[7];
	char Band[8];			// ?Derive from freq?
	long long Freq;

	// Extra fields for Trimode comment fields

	char CMSCall[16];
	char ServerSID[80];
	char UserSID[80];
	char ReportMode[16];
	char Termination[8];		// Last "F" message from CMS
	int Sent;
	int Received;
	int BytesSent;
	int BytesReceived;

	char Dirn;					// Direction of current transfer (In/Out)

	int FBBIndex;				// For saving proposals
	int FBBLen[5];				// Proposed messages
	BOOL GotFC;					// Flag for acking messages on first FC
	char PartMessageRX[512];	// Some modes frame size too small for complete lines
	char PartMessageTX[512];	// Some modes frame size too small for complete lines

} ADIF;

BOOL UpdateADIFRecord(ADIF * ADIF, char * Msg, char Dirn);
BOOL WriteADIFRecord(ADIF * ADIF);


