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

#if !defined(AFX_SERVERSOCKET_H__789A5170_2834_4B7B_9E44_A22566439C9F__INCLUDED_)
#define AFX_SERVERSOCKET_H__789A5170_2834_4B7B_9E44_A22566439C9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Util.h"
#include "Socket.h"
#include "Thread.h"

class ServerSocketListener {
public:
	typedef ServerSocketListener* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	enum Types {
		INCOMING_CONNECTION
	};
	virtual void onAction(Types) { };
};

class ServerSocket : public Speaker<ServerSocketListener>, public Thread
{
public:
	void waitForConnections(short aPort) throw(SocketException);
	ServerSocket() : stop(false), sock(INVALID_SOCKET) { };

	virtual ~ServerSocket() {
		disconnect();
	}
	
	void disconnect() {
		if(sock != INVALID_SOCKET) {
			stop = true;
			closesocket(sock);
			sock = INVALID_SOCKET;
			join();
			stop = false;
		}
	}

	virtual int run();
	SOCKET getSocket() const { return sock; }
	
private:
	bool stop;	
	SOCKET sock;
};

#endif // !defined(AFX_SERVERSOCKET_H__789A5170_2834_4B7B_9E44_A22566439C9F__INCLUDED_)

/**
 * @file ServerSocket.h
 * $Id: ServerSocket.h,v 1.15 2002/04/22 13:58:14 arnetheduck Exp $
 */

