//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// History:
//    1.0      Initial version                  Zoltan Csizmadia
//
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.h
//

#ifndef EXTENDEDTRACE_H_INCLUDED
#define EXTENDEDTRACE_H_INCLUDED

#if defined(_DEBUG) && defined(WIN32)

#include <windows.h>
#include <tchar.h>

#pragma comment( lib, "imagehlp.lib" )

#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )	InitSymInfo( IniSymbolPath )
#define EXTENDEDTRACEUNINITIALIZE()			         UninitSymInfo()
#define STACKTRACE(file)							            StackTrace( GetCurrentThread(), _T(""), file)
#define STACKTRACE2(file, eip, esp, ebp) StackTrace(GetCurrentThread(), _T(""), file, eip, esp, ebp)
class File;

BOOL InitSymInfo( PCSTR );
BOOL UninitSymInfo();
void StackTrace( HANDLE, LPCTSTR, File& file);
void StackTrace( HANDLE, LPCTSTR, File& file, DWORD eip, DWORD esp, DWORD ebp);

#else

#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )   ((void)0)
#define EXTENDEDTRACEUNINITIALIZE()			         ((void)0)
#define STACKTRACE(file)						         	   ((void)0)
#define STACKTRACE2(file, eip, esp, ebp) ((void)0)
#endif

#endif