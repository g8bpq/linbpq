
char VersionString[50]="";
char VersionStringWithBuild[50]="";
int Ver[4] = {Vers};
char TextVerstring[50] = "";

VOID GetVersionInfo(TCHAR * File)
{
#ifndef LINBPQ

	char isDebug[40]="";

#ifdef SPECIALVERSION
	strcat(isDebug, SPECIALVERSION);
#endif
#ifdef _DEBUG 
	strcat(isDebug, "Debug Build ");
#endif

	sprintf(VersionString,"%d.%d.%d.%d %s", Ver[0], Ver[1], Ver[2], Ver[3], isDebug);

	sprintf(TextVerstring,"V%d.%d.%d.%d", Ver[0], Ver[1], Ver[2], Ver[3]);
	
	sprintf(VersionStringWithBuild,"%d.%d.%d Build %d %s", Ver[0], Ver[1], Ver[2], Ver[3], isDebug);

	return;
#endif
}
