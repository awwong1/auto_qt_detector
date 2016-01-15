// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
//#pragma warning(disable : 4996)


#include <iostream>
//#include <fstream>

// #define STDC_WANT_LIB_EXT1 1

// TODO: reference additional headers your program requires here
#include <sys/mman.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
//#include <string.h>
//#include <wchar.h>
#include <stdlib.h>
// #include <fcntl.h>
#include <unistd.h>

#include <vector>
using namespace std;


// static wchar_t anncodes [51][10] =  { L"notQRS", L"N",       L"LBBB",    L"RBBB",     L"ABERR", L"PVC",     //  0-5
//                                       L"FUSION", L"NPC",     L"APC",     L"SVPB",     L"VESC",  L"NESC",    //  6-11
//                                       L"PACE",   L"UNKNOWN", L"NOISE",   L"q",        L"ARFCT", L"Q",       // 12-17
//                                       L"STCH",   L"TCH",     L"SYSTOLE", L"DIASTOLE", L"NOTE",  L"MEASURE", // 18-23
//                                       L"P",      L"BBB",     L"PACESP",  L"T",        L"RTM",   L"U",       // 24-29
//                                       L"LEARN",  L"FLWAV",   L"VFON",    L"VFOFF",    L"AESC",  L"SVESC",   // 30-35
//                                       L"LINK",   L"NAPC",    L"PFUSE",   L"(",        L")",     L"RONT",    // 36-41

//           //user defined beats//
//                                       L"(p",     L"p)",      L"(t",      L"t)",       L"ECT",               // 42-46
//                                       L"r",      L"R",       L"s",       L"S"};                             // 47-50

               /*
                  [16] - ARFCT
                  [15] - q
                  [17] - Q
                  [24] - P
                  [27] - T
                  [39, 40] - '(' QRS ')'  PQ, J point
                  42 - (p Pwave onset
                  43 - p) Pwave offset
                  44 - (t Twave onset
                  45 - t) Twave offset
                  46 - ect Ectopic of any origin beat
                  47 - r
                  48 - R
                  49 - s
                  50 - S
                                               */

typedef void *LPVOID;
typedef void *PVOID;
typedef PVOID HANDLE;

#define _MAX_PATH PATH_MAX

// Stuff from Windows headers:
typedef unsigned long       DWORD;
typedef unsigned long       ULONG_PTR;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef ULONG_PTR           DWORD_PTR;
#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD_PTR)(w) >> 8))

// typedef union _LARGE_INTEGER {
//   struct {
//     DWORD LowPart;
//     LONG  HighPart;
//   };
//   struct {
//     DWORD LowPart;
//     LONG  HighPart;
//   } u;
//   LONGLONG QuadPart;
// } LARGE_INTEGER, *PLARGE_INTEGER;
