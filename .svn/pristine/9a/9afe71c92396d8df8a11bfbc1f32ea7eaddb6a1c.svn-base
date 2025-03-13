/*

Stuff to make compiling on WINDOWS and LINUX easier

*/

#ifndef _COMPATBITS_
#define _COMPATBITS_

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stddef.h>

#ifdef WIN32

#define _CRT_SECURE_NO_DEPRECATE 
#include <crtdbg.h>

#define strtoll _strtoi64

#ifdef _WIN64
#include "stdint.h"
#else
#include "winstdint.h"
#endif

#include "winsock2.h"
#include "WS2tcpip.h"

#include <windowsx.h>

#include <Richedit.h>
#include "commctrl.h"
#include "Commdlg.h"
#include <shellapi.h>

#define Dll	__declspec(dllexport)
#define DllExport __declspec(dllexport)

#define ioctl ioctlsocket

#define pthread_t uintptr_t

int pthread_equal(pthread_t T1, pthread_t T2);

uintptr_t _beginthread(void(__cdecl *start_address)(void *), unsigned stack_size, void *arglist);

#else

int Sleep(int ms);

#define ioctlsocket ioctl

#define Dll
#define DllExport 

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>
#include <stdint.h>

#define BOOL int
#define VOID void
#define UCHAR unsigned char
#define USHORT unsigned short
#define ULONG unsigned long
#define UINT unsigned int
#define SHORT short
#define DWORD unsigned int
#define BYTE unsigned char
#define APIENTRY
#define WINAPI
#define WINUSERAPI
#define TCHAR char
#define TRUE 1
#define FALSE 0
#define FAR
#define byte UCHAR
//#define Byte UCHAR
#define Word WORD

typedef DWORD   COLORREF;
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((USHORT)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)      rgb & 0xFF
#define GetGValue(rgb)      (rgb >> 8) & 0xFF
#define GetBValue(rgb)      (rgb >> 16) & 0xFF


#define HWND unsigned int
#define HINSTANCE unsigned int
#define HKEY unsigned int
#define UINT_PTR unsigned int *

#define HANDLE UINT
#define SOCKET int

#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

#define HMENU UINT
#define WNDPROC UINT
#define __cdecl

#define strtok_s strtok_r

#define _memicmp memicmp
#define _stricmp stricmp
#define _strdup strdup
#define _strupr strupr
#define _strlwr strlwr

#define _snprintf snprintf

#define _gcvt gcvt
#define _fcvt fcvt
#define _atoi64 atoll 

#define DeleteFile unlink
#define MoveFile rename
#define CreateDirectory mkdir

uint64_t GetTickCount();


int sprintf_s(char * string, int plen, const char * format, ...);


int memicmp(unsigned char *a, unsigned char *b, int n);
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2);
char * strupr(char* s);
char * strlwr(char* s);

pthread_t _beginthread(void(*start_address)(), unsigned stack_size, VOID * arglist);



#define WSAGetLastError() errno
#define GetLastError() errno 
void closesocket(int sock);
#define GetCurrentProcessId getpid
#define GetCurrentThreadId pthread_self

char * inet_ntoa(struct in_addr in);

#define LOBYTE(w)           ((BYTE)((ULONG *)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((ULONG *)(w) >> 8))

#define LOWORD(l)           ((SHORT) ((l) & 0xffff))

#define WSAEWOULDBLOCK 11

#define MAX_PATH 250

typedef int (*PROC)();

typedef struct tagRECT
{
    unsigned int	left;
    unsigned int    top;
    unsigned int    right;
    unsigned int    bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

#define HBRUSH int

#define _timezone timezone

#endif



#ifdef LINBPQ


#ifndef WIN32
void SetWindowText(HWND hWnd, char * lpString);
BOOL MySetDlgItemText(HWND hWnd, int item, char * lpString);
BOOL CopyFile(char * In, char * Out, BOOL Failifexists);
int GetNumberofPorts();
int SendMsg(int stream, char * msg, int len);
int SendRaw(int port, char * msg, int len);
VOID md5(char *arg, unsigned char * checksum);

//	Send Raw (KISS mode) frame to port (BPQHOST function 10)

int APIENTRY SendRaw(int port, char * msg, int len);


VOID OutputDebugString(char * string);

#endif

#ifdef APIENTRY
#undef APIENTRY
#endif
#define APIENTRY

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in *PSOCKADDR_IN;
typedef struct sockaddr_in *LPSOCKADDR_IN;

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;
typedef struct sockaddr *LPSOCKADDR;

#define __int16 short
#define __int32 long


#endif

#endif

