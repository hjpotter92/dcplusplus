/* 
 * Copyright (C) 2001 Jacek Sieka, jacek@creatio.se
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

#if !defined(AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_)
#define AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ClientListener.h"
#include "BufferedSocket.h"
#include "User.h"
#include "Util.h"

class Client : public Speaker<ClientListener>, public BufferedSocketListener
{
public:
	enum {
		SEARCH_PLAIN,
		SEARCH_ATLEAST,
		SEARCH_ATMOST
	};
	typedef Client* Ptr;
	typedef list<Ptr> List;
	typedef List::iterator Iter;

	Client() : socket('|') {
		clientList.push_back(this);
		listeners.insert(listeners.end(), staticListeners.begin(), staticListeners.end());
	};

	virtual ~Client() {
		for(Iter i = clientList.begin(); i != clientList.end(); ++i) {
			if(*i == this) {
				clientList.erase(i);
				break;
			}				
		}
	};
	
	static void addStaticListener(ClientListener::Ptr aListener) {
		staticListenersCS.enter();
		staticListeners.push_back(aListener);
		staticListenersCS.leave();

		for(Iter i = clientList.begin(); i != clientList.end(); ++i) {
			(*i)->addListener(aListener);
		}
	}
	
	static void removeStaticListener(ClientListener::Ptr aListener) {
		for(Iter i = clientList.begin(); i != clientList.end(); ++i) {
			(*i)->removeListener(aListener);
		}
		staticListenersCS.enter();
		for(ClientListener::Iter j = staticListeners.begin(); j != staticListeners.end(); ++j) {
			if(*j == aListener) {
				staticListeners.erase(j);
				break;
			}
		}
		staticListenersCS.leave();
	}
	
	static void removeStaticListeners() {
		for(Iter i = clientList.begin(); i != clientList.end(); ++i) {
			for(ClientListener::Iter j = staticListeners.begin(); j != staticListeners.end(); ++j) {
				(*i)->removeListener(*j);
			}
		}
		staticListenersCS.enter();
		staticListeners.clear();
		staticListenersCS.leave();
	}

	void disconnect() {	
		socket.removeListener(this);
		socket.disconnect();
	}

	void validateNick(const string& aNick) {
		dcdebug("validateNick %s\n", aNick.c_str());
		send("$ValidateNick " + aNick + "|");
	}
	
	void key(const string& aKey) {
		dcdebug("key xxx\n");
		send("$Key " + aKey + "|");
	}
	
	void version(const string& aVersion) {
		dcdebug("version %s\n", aVersion.c_str());
		send("$Version " + aVersion + "|");
	}
	
	void getNickList() {
		dcdebug("getNickList\n");
		send("$GetNickList|");
	}
	
	void sendMessage(const string& aMessage) {
		dcdebug("sendMessage ...\n");
		int i;
		string tmp = aMessage;
		while( (i = tmp.find_first_of("|$")) != string::npos) {
			tmp.erase(i, 1);
		}
		send("<" + Settings::getNick() + "> " + tmp + "|");
	}
	void getInfo(User* aUser) {
		dcdebug("GetInfo %s\n", aUser->getNick().c_str());
		send("$GetINFO " + aUser->getNick() + " " + Settings::getNick() + "|");
	}
	void getInfo(const string& aNick) {
		if(users.find(aNick) != users.end())
			send("$GetINFO " + aNick + " " + Settings::getNick() + "|");
	}
	
	void myInfo(const string& aNick, const string& aDescription, const string& aSpeed, const string& aEmail, const string& aBytesShared) {
		dcdebug("MyInfo %s...\n", aNick.c_str());
		send("$MyINFO $ALL " + aNick + " " + aDescription+ " $ $" + aSpeed + "$" + aEmail + "$" + aBytesShared + "$|");
	}

	void connectToMe(User* aUser) {
		send("$ConnectToMe " + aUser->getNick() + " " + Settings::getServer() + ":" + Settings::getPort() + "|");
	}
	void revConnectToMe(User* aUser) {
		send("$RevConnectToMe " + Settings::getNick() + " " + aUser->getNick()  + "|");
	}
	void connect(const string& aServer, short aPort = 411);

	bool userConnected(const string& aNick) {
		return !(users.find(aNick) == users.end());
	}

	const string& getName() { return name; };
	const string& getServer() { return server; };

	User* getUser(const string& aNick) {
		User::NickIter i = users.find(aNick);
		if(i == users.end()) {
			return NULL;
		} else {
			return i->second;
		}
	}
	static List& getList() { return clientList; }
protected:
	
	/** A list of listeners that receive all client messages (from all clients) */
	static ClientListener::List staticListeners;
	static CriticalSection staticListenersCS;

	string server;
	short port;
	BufferedSocket socket;
	string name;

	User::NickMap users;

	static List clientList;

	virtual void onLine(const string& aLine);
	
	virtual void onError(const string& aReason) {
		fireError(aReason);
	}

	virtual void onConnected() {
		fireConnected();
	}

	void send(const string& a) {
		socket.write(a);
	}
	
	void fireConnecting() {
		dcdebug("fireConnecting\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientConnecting(this);
		}
		listenerCS.leave();
	}
	void fireConnected() {
		dcdebug("fireConnected\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientConnected(this);
		}
		listenerCS.leave();
	}
	void fireConnectToMe(const string& aServer, const string& aPort) {
		dcdebug("fireConnectToMe %s:%s\n", aServer.c_str(), aPort.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientConnectToMe(this, aServer, aPort);
		}
		listenerCS.leave();
	}
	void fireSearch(const string& aSeeker, int aSearchType, const string& aSize, int aFileType, const string& aString) {
		dcdebug("fireSearch\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientSearch(this, aSeeker, aSearchType, aSize, aFileType, aString);
		}
		listenerCS.leave();
	}
	void fireHubName() {
		dcdebug("fireHubName\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientHubName(this);
		}
		listenerCS.leave();
	}
	void fireLock(const string& aLock, const string& aPk) {
		dcdebug("fireLock %s\n", aLock.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientLock(this, aLock, aPk);
		}
		listenerCS.leave();
	}
	void firePrivateMessage(const string& aFrom, const string& aMessage) {
		dcdebug("firePM %s ...\n", aFrom.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientPrivateMessage(this, aFrom, aMessage);
		}
		listenerCS.leave();
	}
	void fireHello(User* aUser) {
		//dcdebug("fireHello\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientHello(this, aUser);
		}
		listenerCS.leave();
	}
	void fireForceMove(const string& aServer) {
		dcdebug("fireForceMove %s\n", aServer.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientForceMove(this, aServer);
		}
		listenerCS.leave();
	}
	void fireHubFull() {
		dcdebug("fireHubFull\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientHubFull(this);
		}
		listenerCS.leave();
	}
	void fireMyInfo(User* aUser) {
		//dcdebug("fireMyInfo\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientMyInfo(this, aUser);
		}
		listenerCS.leave();
	}
	void fireValidateDenied() {
		dcdebug("fireValidateDenied\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientValidateDenied(this);
		}
		listenerCS.leave();
	}
	void fireQuit(User* aUser) {
		//dcdebug("fireQuit\n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientQuit(this, aUser);
		}
		listenerCS.leave();
	}
	void fireRevConnectToMe(const string& aNick) {
		dcdebug("fireRevConnectToMe %s\n", aNick.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientRevConnectToMe(this, aNick);
		}
		listenerCS.leave();
	}
	void fireNickList(StringList& aList) {
		dcdebug("fireNickList ... \n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientNickList(this, aList);
		}
		listenerCS.leave();
	}
	void fireOpList(StringList& aList) {
		dcdebug("fireOpList ... \n");
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientOpList(this, aList);
		}
		listenerCS.leave();
	}
	void fireUnknown(const string& aString) {
		dcdebug("fireUnknown %s\n", aString.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientUnknown(this, aString);
		}
		listenerCS.leave();
	}
	void fireMessage(const string& aMessage) {
		// dcdebug("fireMessage %s\n", aMessage.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientMessage(this, aMessage);
		}
		listenerCS.leave();
	}
	void fireError(const string& aMessage) {
		dcdebug("fireError %s\n", aMessage.c_str());
		listenerCS.enter();
		ClientListener::List tmp = listeners;
		for(ClientListener::Iter i=tmp.begin(); i != tmp.end(); ++i) {
			(*i)->onClientError(this, aMessage);
		}
		listenerCS.leave();
	}
};


#endif // !defined(AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_)

/**
 * @file Client.h
 * $Id: Client.h,v 1.3 2001/12/02 11:16:46 arnetheduck Exp $
 * @if LOG
 * $Log: Client.h,v $
 * Revision 1.3  2001/12/02 11:16:46  arnetheduck
 * Optimised hub listing, removed a few bugs and leaks, and added a few small
 * things...downloads are now working, time to start writing the sharing
 * code...
 *
 * Revision 1.2  2001/11/29 19:10:54  arnetheduck
 * Refactored down/uploading and some other things completely.
 * Also added download indicators and download resuming, along
 * with some other stuff.
 *
 * Revision 1.1  2001/11/27 22:10:08  arnetheduck
 * Renamed DCClient* to Client*
 *
 * Revision 1.5  2001/11/26 23:40:36  arnetheduck
 * Downloads!! Now downloads are possible, although the implementation is
 * likely to change in the future...more UI work (splitters...) and some bug
 * fixes. Only user file listings are downloadable, but at least it's something...
 *
 * Revision 1.4  2001/11/25 22:06:25  arnetheduck
 * Finally downloading is working! There are now a few quirks and bugs to be fixed
 * but what the heck....!
 *
 * Revision 1.3  2001/11/24 10:34:02  arnetheduck
 * Updated to use BufferedSocket instead of handling threads by itself.
 *
 * Revision 1.2  2001/11/22 19:47:42  arnetheduck
 * A simple XML parser. Doesn't have all the features, but works good enough for
 * the configuration file.
 *
 * Revision 1.1.1.1  2001/11/21 17:33:20  arnetheduck
 * Inital release
 *
 * @endif
 */

