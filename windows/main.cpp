/* 
 * Copyright (C) 2001 Jacek Sieka, j_s@telia.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "MainFrm.h"
#include "ExtendedTrace.h"

extern void startup();
extern void shutdown();

CAppModule _Module;

#ifdef _DEBUG
CriticalSection cs;
enum { DEBUG_BUFSIZE = 2048 };
char buf[DEBUG_BUFSIZE];

LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS e )
{
	Lock l(cs);
	
	File f(Util::getAppPath() + "exceptioninfo.txt", File::WRITE, File::OPEN | File::CREATE);
	
	DWORD exceptionCode = e->ExceptionRecord->ExceptionCode ;

	sprintf(buf, "Unhandled Exception\r\n  Code: %x\r\n", exceptionCode ) ;

	f.write(buf);
	STACKTRACE(f);

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	
	MainFrame wndMain;
	
	startup();
	
	SettingsManager::getInstance()->setDefault(SettingsManager::BACKGROUND_COLOR, (int)(GetSysColor(COLOR_WINDOW)));
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_COLOR, (int)(GetSysColor(COLOR_WINDOWTEXT)));
	
	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}
	
	wndMain.ShowWindow(nCmdShow);
	
	int nRet = theLoop.Run();
	
	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//	ATLASSERT(SUCCEEDED(hRes));
#ifdef _DEBUG
	EXTENDEDTRACEINITIALIZE( NULL );
	SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
#endif

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES |
		ICC_UPDOWN_CLASS);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	
	int nRet = Run(lpstrCmdLine, nCmdShow);
	
	_Module.Term();
	::CoUninitialize();
#ifdef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
#endif
	return nRet;
}

/**
 * @file main.cpp
 * $Id: main.cpp,v 1.4 2002/04/22 13:58:15 arnetheduck Exp $
 */
