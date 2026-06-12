#ifndef BADKING_COMPAT_H
#define BADKING_COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>

/* Ghidra auto-generated types */
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined4;
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long ulong;
typedef int bool;
#define true 1
#define false 0

/* Remove 16-bit calling conventions */
#define __cdecl16far
#define __cdecl16near
#define __cdecl16
#define __stdcall16far
#define __stdcall16near
#define __stdcall16
#define __pascal16far
#define __pascal16near
#define __pascal16

/* halt_baddata stub for Ghidra bad instruction markers */
static inline void halt_baddata(void) { return; }

/* Windows types */
typedef void* HWND;
typedef void* HDC;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HMENU;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HBITMAP;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long LONG;
typedef int BOOL;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef void* LPVOID;
typedef void* FARPROC;

/* MSG structure stub */
typedef struct {
    HWND hwnd;
    UINT message;
    int wParam;
    long lParam;
    DWORD time;
} MSG;

/* PAINTSTRUCT stub */
typedef struct {
    HDC hdc;
    int fErase;
} PAINTSTRUCT;

/* RECT stub */
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} RECT;

/* TEXTMETRIC stub */
typedef struct {
    int tmHeight;
    int tmAscent;
    int tmDescent;
    int tmInternalLeading;
    int tmExternalLeading;
    int tmAveCharWidth;
    int tmMaxCharWidth;
    int tmWeight;
    int tmOverhang;
    int tmDigitizedAspectX;
    int tmDigitizedAspectY;
    char tmFirstChar;
    char tmLastChar;
    char tmDefaultChar;
    char tmBreakChar;
    char tmItalic;
    char tmUnderlined;
    char tmStruckOut;
    char tmPitchAndFamily;
    char tmCharSet;
} TEXTMETRIC;

/* WNDCLASS stub */
typedef struct {
    UINT style;
    void* lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASS;

/* Point stub */
typedef struct {
    int x;
    int y;
} POINT;

/* GetProcAddress stub */
#define GetProcAddress(x,y) NULL

/* MAKEPOINT stub */
#define MAKEPOINT(x) ((POINT){0,0})

/* LOWORD/HIWORD stubs */
#define LOWORD(x) ((unsigned short)(x))
#define HIWORD(x) ((unsigned short)(((unsigned int)(x)) >> 16))

/* VK_ key constants */
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20

/* Message constants */
#define WM_CREATE 0x0001
#define WM_PAINT 0x000F
#define WM_DESTROY 0x0002
#define WM_KEYDOWN 0x0100
#define WM_CHAR 0x0102
#define WM_LBUTTONDOWN 0x0201
#define WM_COMMAND 0x0111
#define WM_CLOSE 0x0010

/* Window style constants */
#define WS_OVERLAPPEDWINDOW 0x00CF0000
#define CW_USEDEFAULT 0x80000000

/* swi stub - just return 0 */
#define swi(x) 0

/* code type stub */
typedef int (*code)();

/* Ghidra helper macros */
#define CONCAT11(a,b) ((unsigned short)(((unsigned char)(a)) << 8 | ((unsigned char)(b))))
#define CONCAT22(a,b) ((unsigned int)(((unsigned short)(a)) << 16 | ((unsigned short)(b))))
#define SCARRY1(a,b) ((((signed char)(a)) < 0) == (((signed char)(b)) < 0) && (((signed char)(a)) + ((signed char)(b)) < 0) != (((signed char)(a)) < 0))
#define SCARRY2(a,b) ((((signed short)(a)) < 0) == (((signed short)(b)) < 0) && (((signed short)(a)) + ((signed short)(b)) < 0) != (((signed short)(a)) < 0))
#define SBORROW2(a,b) ((((signed short)(a)) < 0) != (((signed short)(b)) < 0) && (((signed short)(a)) - ((signed short)(b)) < 0) != (((signed short)(a)) < 0))
#define CARRY1(a,b) ((unsigned int)(a) + (unsigned int)(b) > 0xFF)
#define CARRY2(a,b) ((unsigned int)(a) + (unsigned int)(b) > 0xFFFF)

/* Additional Ghidra helper macros */
#define CONCAT12(a,b) ((unsigned int)(((unsigned char)(a)) << 8 | ((unsigned char)(b))))
#define LOCK()
#define UNLOCK()
#define SBORROW1(a,b) ((((signed char)(a)) < 0) != (((signed char)(b)) < 0) && (((signed char)(a)) - ((signed char)(b)) < 0) != (((signed char)(a)) < 0))

/* Console I/O stubs */
#define _getch getchar
#define _getche getchar
#define _kbhit() 1
#define _putch putchar

/* Window/GUI stubs */
#define __InitEasyWin()
#define __DoneEasyWin()
#define __ExitEasyWin()
#define __CreateEasyWin() NULL
#define __errorBox(x,y)
#define __errorExitBox(x,y)

/* Stack variable stubs - used as addresses in Ghidra output */
extern int stack0x0002_val;
extern int stack0x0004_val;
extern int stack0x0006_val;
#define stack0x0002 stack0x0002_val
#define stack0x0004 stack0x0004_val
#define stack0x0006 stack0x0006_val

/* Thunk function stubs - removed to avoid conflicts with actual definitions */

/* Additional type stubs */
typedef int undefined;
typedef unsigned char unkbyte10[10];
typedef long double longdouble;

/* _WSPRINTF - map to sprintf (not defined as macro to avoid conflict with function stub) */

#endif /* BADKING_COMPAT_H */
