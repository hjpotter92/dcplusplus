/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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


#if !defined(AFX_STDINC_H__65559042_5D04_44EF_9ECF_E0A7FA6E1348__INCLUDED_)
#define AFX_STDINC_H__65559042_5D04_44EF_9ECF_E0A7FA6E1348__INCLUDED_

#include "config.h"

#ifdef WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WTL_NO_CSTRING
#define _ATL_NO_OPENGL
#define _ATL_NO_MSIMG
#define _ATL_NO_COM
#define _ATL_NO_HOSTING
#define _ATL_NO_OLD_NAMES

#include <Winsock2.h>

#include <windows.h>
#include <crtdbg.h>

#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/types.h>

#include <time.h>

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <set>
#ifdef HAS_HASH
#include <hash_map>
#endif

#ifdef _STLPORT_VERSION
using namespace _STL;
#else //_STLPORT_VERSION
using namespace std;

template<>
class std::hash_compare<string, less<string> > {
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;

	size_t operator()(const string& s) const {
		size_t x = 0;
		const char* y = s.data();
		string::size_type j = s.size();
		for(string::size_type i = 0; i < j; ++i) {
			x = x*31 + (size_t)y[i];
		}
		return x;
	}
	bool operator()(const string& a, const string& b) const {
		return strcmp(a.c_str(), b.c_str()) == -1;
	}
};
#endif

#endif // !defined(AFX_STDINC_H__65559042_5D04_44EF_9ECF_E0A7FA6E1348__INCLUDED_)

/**
 * @file
 * $Id: stdinc.h,v 1.5 2003/09/22 13:17:23 arnetheduck Exp $
 */
