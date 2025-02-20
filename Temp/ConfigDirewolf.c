
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

int CaptureIndex = -1;		// Card number
int PlayBackIndex = -1;

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

char CaptureNames[16][MAXPNAMELEN + 2] = { "" };
char PlaybackNames[16][MAXPNAMELEN + 2] = { "" };

void main(int argc, char * argv[])
{
	int i;
	FILE *file;


	if (argc < 3)
		return;

	_strupr(argv[1]);
	_strupr(argv[2]);

	CaptureCount = waveInGetNumDevs();

	CaptureDevices = malloc((MAXPNAMELEN + 2) * CaptureCount);
	CaptureDevices[0] = 0;

	printf("Capture Devices\r\n");

	for (i = 0; i < CaptureCount; i++)
	{
		waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));

		if (CaptureDevices)
			strcat(CaptureDevices, ",");
		strcat(CaptureDevices, pwic.szPname);
		printf("%d %s\r\n", i, pwic.szPname);
		memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
		_strupr(&CaptureNames[i][0]);
	}

	printf("\r\n");

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


	for (i = 0; i < CaptureCount; i++)
	{
		if (strstr(&CaptureNames[i][0], argv[1]))
		{
			CaptureIndex = i;
			break;
		}
	}

	for (i = 0; i < PlaybackCount; i++)
	{
		if (strstr(&PlaybackNames[i][0], argv[2]))
		{
			PlayBackIndex = i;
			break;
		}
	}

	CopyFile("direwolf.in", "direwolf.conf", 0);


	if ((file = fopen("direwolf.conf", "ab")) == NULL)
		return;


	printf("ADEVICE %d %d\r\n", CaptureIndex, PlayBackIndex);
	fprintf(file, "ADEVICE %d %d\r\n", CaptureIndex, PlayBackIndex);

	fclose(file);

	printf("File updated");



}





