
//
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE


#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <malloc.h>


#pragma comment(lib, "winmm.lib")

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 24000, 2, 16, 0 };

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;


char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

int CaptureCount = 0;
int PlaybackCount = 0;

int IndexA = -1;		// Card number
int IndexB = -1;		// Card number
int IndexC = -1;		// Card number

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

char CaptureNames[16][MAXPNAMELEN + 2] = { "" };
char PlaybackNames[16][MAXPNAMELEN + 2] = { "" };

void main(int argc, char * argv[])
{
	int i;
	FILE *infile;
	FILE *file;

	char line[1024] = "";
	char index[16];
	char * ptr;


	PlaybackCount = waveOutGetNumDevs();

	PlaybackDevices = malloc((MAXPNAMELEN + 2) * PlaybackCount);
	PlaybackDevices[0] = 0;

	printf("Playback Devices\r\n");

	for (i = 0; i < PlaybackCount; i++)
	{
		waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

		if (PlaybackDevices[0])
			strcat(PlaybackDevices, ",");
		strcat(PlaybackDevices, pwoc.szPname);
		printf("%i %s\r\n", i, pwoc.szPname);
		memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
		_strupr(&PlaybackNames[i][0]);
		waveOutClose(hWaveOut);
	}


	printf("\r\n");


	for (i = 0; i < PlaybackCount; i++)
	{
		if (strstr(&PlaybackNames[i][0], "CABLE-A"))
			IndexA = i;
		else if(strstr(&PlaybackNames[i][0], "CABLE-B"))
			IndexB = i;
		else if (strstr(&PlaybackNames[i][0], "CABLE-C"))
			IndexC = i;
	}

	if ((infile = fopen("C:\\Users\\johnw\\AppData\\Roaming\\SDRplay\\SDRuno.in", "rb")) == NULL)
		return;

	if ((file = fopen("C:\\Users\\johnw\\AppData\\Roaming\\SDRplay\\SDRuno.ini", "wb")) == NULL)
		return;

	while ((fgets(line, 1023, infile)))
	{
		if (ptr = strstr(line, "CABLE-A"))
		{
			*ptr = 0;
			sprintf(index, "%d\r\n", IndexA);
			strcat(line, index);
		}
		if (ptr = strstr(line, "CABLE-B"))
		{
			*ptr = 0;
			sprintf(index, "%d\r\n", IndexB);
			strcat(line, index);
		}
		if (ptr = strstr(line, "CABLE-C"))
		{
			*ptr = 0;
			sprintf(index, "%d\r\n", IndexC);
			strcat(line, index);
		}
		fprintf(file, line);
	}


	fclose(file);
	fclose(infile);

	printf("File updated");



}

