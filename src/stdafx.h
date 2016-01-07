// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma warning(disable : 4996)


#include <iostream>
//#include <fstream>

// #define STDC_WANT_LIB_EXT1 1

// TODO: reference additional headers your program requires here
//#include <sys/mman.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
//#include <string.h>
//#include <wchar.h>
#include <stdlib.h>
// #include <fcntl.h>
// #include <unistd.h>

#include <vector>
using namespace std;


static wchar_t anncodes [51][10] =  { L"notQRS", L"N",       L"LBBB",    L"RBBB",     L"ABERR", L"PVC",
                                      L"FUSION", L"NPC",     L"APC",     L"SVPB",     L"VESC",  L"NESC",
                                      L"PACE",   L"UNKNOWN", L"NOISE",   L"q",        L"ARFCT", L"Q",
                                      L"STCH",   L"TCH",     L"SYSTOLE", L"DIASTOLE", L"NOTE",  L"MEASURE",
                                      L"P",      L"BBB",     L"PACESP",  L"T",        L"RTM",   L"U",
                                      L"LEARN",  L"FLWAV",   L"VFON",    L"VFOFF",    L"AESC",  L"SVESC",
                                      L"LINK",   L"NAPC",    L"PFUSE",   L"(",        L")",     L"RONT",

          //user defined beats//
                                      L"(p",     L"p)",      L"(t",      L"t)",       L"ECT",
                                      L"r",      L"R",       L"s",       L"S"};

typedef void *LPVOID;
typedef void *PVOID;
typedef PVOID HANDLE;

#define _MAX_PATH PATH_MAX

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
