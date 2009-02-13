/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_WINDOWS_MANAGER_LISTENER_H
#define DCPLUSPLUS_DCPP_WINDOWS_MANAGER_LISTENER_H

#include "forward.h"

namespace dcpp {

class WindowsManagerListener {
public:
	virtual ~WindowsManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Window;
	typedef X<1> Hub;

	virtual void on(Window, const string&) throw() { }
	virtual void on(Hub, const string&, const string&) throw() { }
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_WINDOWS_MANAGER_LISTENER_H)
