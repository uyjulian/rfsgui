#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"
#include "ctype.h"

#ifdef _WIN32
#include "windows.h"
#define SUPPORT_WINDOWS_XP_PARTITIONS
#else
typedef unsigned char BYTE, *LPBYTE;
typedef unsigned char BOOLEAN;
typedef unsigned long DWORD, *LPDWORD;
typedef unsigned long BOOL;
typedef long long INT64;
typedef const char* LPCSTR;
typedef char CHAR, *LPSTR;
typedef unsigned short WORD, USHORT;
#define TRUE 1
#define FALSE 0
#define WINAPI
#endif

#include "gtools.h"

#ifdef _MSC_VER
#define FMT_QWORD "I64d"
#else
#define FMT_QWORD "qd"
#endif

#define PBytesInMBytes(BYTES) (((double)BYTES)/1048576)

