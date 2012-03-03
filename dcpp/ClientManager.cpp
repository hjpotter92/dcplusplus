/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "ClientManager.h"

#include "AdcHub.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "File.h"
#include "NmdcHub.h"
#include "SearchManager.h"
#include "SearchResult.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"

namespace dcpp {

ClientManager::ClientManager() : udp(Socket::TYPE_UDP) {
	TimerManager::getInstance()->addListener(this);
}

ClientManager::~ClientManager() {
	TimerManager::getInstance()->removeListener(this);
}

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if(Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, true);
	} else {
		c = new NmdcHub(aHubURL);
	}

	{
		Lock l(cs);
		clients.insert(c);
	}

	c->addListener(this);

	return c;
}

void ClientManager::putClient(Client* aClient) {
	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.erase(aClient);
	}
	aClient->shutdown();
	delete aClient;
}

size_t ClientManager::getUserCount() const {
	Lock l(cs);
	return onlineUsers.size();
}

StringList ClientManager::getHubUrls(const CID& cid, const string& hintUrl) {
	return getHubUrls(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getHubNames(const CID& cid, const string& hintUrl) {
	return getHubNames(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getNicks(const CID& cid, const string& hintUrl) {
	return getNicks(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getHubUrls(const CID& cid, const string& hintUrl, bool priv) {
	Lock l(cs);
	StringList lst;
	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(cid);
		for(auto i = op.first; i != op.second; ++i) {
			lst.push_back(i->second->getClient().getHubUrl());
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			lst.push_back(u->getClient().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid, const string& hintUrl, bool priv) {
	Lock l(cs);
	StringList lst;
	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(cid);
		for(auto i = op.first; i != op.second; ++i) {
			lst.push_back(i->second->getClient().getHubName());
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			lst.push_back(u->getClient().getHubName());
	}
	return lst;
}

StringList ClientManager::getNicks(const CID& cid, const string& hintUrl, bool priv) {
	Lock l(cs);
	StringSet ret;

	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(cid);
		for(auto i = op.first; i != op.second; ++i) {
			ret.insert(i->second->getIdentity().getNick());
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			ret.insert(u->getIdentity().getNick());
	}

	if(ret.empty()) {
		// offline
		auto i = nicks.find(cid);
		if(i != nicks.end()) {
			ret.insert(i->second.first);
		} else {
			ret.insert('{' + cid.toBase32() + '}');
		}
	}

	return StringList(ret.begin(), ret.end());
}

StringPairList ClientManager::getHubs(const CID& cid, const string& hintUrl, bool priv) {
	Lock l(cs);
	StringPairList lst;
	if(!priv) {
		auto op = onlineUsers.equal_range(cid);
		for(auto i = op.first; i != op.second; ++i) {
			lst.push_back(make_pair(i->second->getClient().getHubUrl(), i->second->getClient().getHubName()));
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			lst.push_back(make_pair(u->getClient().getHubUrl(), u->getClient().getHubName()));
	}
	return lst;
}

vector<Identity> ClientManager::getIdentities(const UserPtr &u) const {
	Lock l(cs);
	auto op = onlineUsers.equal_range(u->getCID());
	auto ret = vector<Identity>();
	for(auto i = op.first; i != op.second; ++i) {
		ret.push_back(i->second->getIdentity());
	}

	return ret;
}

string ClientManager::getField(const CID& cid, const string& hint, const char* field) const {
	Lock l(cs);

	OnlinePairC p;
	auto u = findOnlineUserHint(cid, hint, p);
	if(u) {
		auto value = u->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	for(auto i = p.first; i != p.second; ++i) {
		auto value = i->second->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	return Util::emptyString;
}

string ClientManager::getConnection(const CID& cid) const {
	Lock l(cs);
	auto i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return _("Offline");
}

int64_t ClientManager::getAvailable() const {
	Lock l(cs);
	int64_t bytes = 0;
	for(auto& i: onlineUsers) {
		bytes += i.second->getIdentity().getBytesShared();
	}

	return bytes;
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	for(auto i: clients) {
		if(i->getHubUrl() == aUrl) {
			return true;
		}
	}
	return false;
}

string ClientManager::findHub(const string& ipPort) const {
	Lock l(cs);

	string ip;
	string port = "411";
	string::size_type i = ipPort.rfind(':');
	if(i == string::npos) {
		ip = ipPort;
	} else {
		ip = ipPort.substr(0, i);
		port = ipPort.substr(i+1);
	}

	string url;
	for(auto c: clients) {
		if(c->getIp() == ip) {
			// If exact match is found, return it
			if(c->getPort() == port)
				return c->getHubUrl();

			// Port is not always correct, so use this as a best guess...
			url = c->getHubUrl();
		}
	}

	return url;
}

string ClientManager::findHubEncoding(const string& aUrl) const {
	Lock l(cs);

	for(auto i: clients) {
		if(i->getHubUrl() == aUrl) {
			return i->getEncoding();
		}
	}
	return Text::systemCharset;
}

UserPtr ClientManager::findLegacyUser(const string& aNick) const noexcept {
	if (aNick.empty())
		return UserPtr();

	Lock l(cs);

	for(auto& i: onlineUsers) {
		const OnlineUser* ou = i.second;
		if(ou->getUser()->isSet(User::NMDC) && Util::stricmp(ou->getIdentity().getNick(), aNick) == 0)
			return ou->getUser();
	}
	return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) noexcept {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	auto ui = users.find(cid);
	if(ui != users.end()) {
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFlag(User::NMDC);
	users.insert(make_pair(cid, p));

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) noexcept {
	Lock l(cs);
	auto ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}

	UserPtr p(new User(cid));
	users.insert(make_pair(cid, p));
	return p;
}

UserPtr ClientManager::findUser(const CID& cid) const noexcept {
	Lock l(cs);
	auto ui = users.find(cid);
	return ui == users.end() ? 0 : ui->second;
}

bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(user->getCID());
	for(auto i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const noexcept {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::putOnline(OnlineUser* ou) noexcept {
	{
		Lock l(cs);
		onlineUsers.insert(make_pair(ou->getUser()->getCID(), ou));
	}

	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou, bool disconnect) noexcept {
	OnlineIter::difference_type diff = 0;
	{
		Lock l(cs);
		auto op = onlineUsers.equal_range(ou->getUser()->getCID());
		dcassert(op.first != op.second);
		for(auto i = op.first; i != op.second; ++i) {
			auto ou2 = i->second;
			if(ou == ou2) {
				diff = distance(op.first, op.second);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(diff == 1) { //last user
		UserPtr& u = ou->getUser();
		u->unsetFlag(User::ONLINE);
		if(disconnect)
			ConnectionManager::getInstance()->disconnect(u);
		fire(ClientManagerListener::UserDisconnected(), u);
	} else if(diff > 1) {
			fire(ClientManagerListener::UserUpdated(), *ou);
	}
}

OnlineUser* ClientManager::findOnlineUserHint(const CID& cid, const string& hintUrl, OnlinePairC& p) const {
	p = onlineUsers.equal_range(cid);
	if(p.first == p.second) // no user found with the given CID.
		return 0;

	if(!hintUrl.empty()) {
		for(auto i = p.first; i != p.second; ++i) {
			OnlineUser* u = i->second;
			if(u->getClient().getHubUrl() == hintUrl) {
				return u;
			}
		}
	}

	return 0;
}

OnlineUser* ClientManager::findOnlineUser(const HintedUser& user, bool priv) {
	return findOnlineUser(user.user->getCID(), user.hint, priv);
}

OnlineUser* ClientManager::findOnlineUser(const CID& cid, const string& hintUrl, bool priv) {
	OnlinePairC p;
	OnlineUser* u = findOnlineUserHint(cid, hintUrl, p);
	if(u) // found an exact match (CID + hint).
		return u;

	if(p.first == p.second) // no user found with the given CID.
		return 0;

	// if the hint hub is private, don't allow connecting to the same user from another hub.
	if(priv)
		return 0;

	// ok, hub not private, return a random user that matches the given CID but not the hint.
	return p.first->second;
}

void ClientManager::connect(const HintedUser& user, const string& token) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	Lock l(cs);
	OnlineUser* u = findOnlineUser(user, priv);

	if(u) {
		u->getClient().connect(*u, token);
	}
}

void ClientManager::privateMessage(const HintedUser& user, const string& msg, bool thirdPerson) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	Lock l(cs);
	OnlineUser* u = findOnlineUser(user, priv);

	if(u) {
		u->getClient().privateMessage(*u, msg, thirdPerson);
	}
}

void ClientManager::userCommand(const HintedUser& user, const UserCommand& uc, ParamMap& params, bool compatibility) {
	Lock l(cs);
	/** @todo we allow wrong hints for now ("false" param of findOnlineUser) because users
	 * extracted from search results don't always have a correct hint; see
	 * SearchManager::onRES(const AdcCommand& cmd, ...). when that is done, and SearchResults are
	 * switched to storing only reliable HintedUsers (found with the token of the ADC command),
	 * change this call to findOnlineUserHint. */
	OnlineUser* ou = findOnlineUser(user.user->getCID(), user.hint.empty() ? uc.getHub() : user.hint, false);
	if(!ou)
		return;

	ou->getIdentity().getParams(params, "user", compatibility);
	ou->getClient().getHubIdentity().getParams(params, "hub", false);
	ou->getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou->getClient().sendUserCmd(uc, params);
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	auto i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		OnlineUser& u = *i->second;
		if(cmd.getType() == AdcCommand::TYPE_UDP && !u.getIdentity().isUdpActive()) {
			cmd.setType(AdcCommand::TYPE_DIRECT);
			cmd.setTo(u.getIdentity().getSID());
			u.getClient().send(cmd);
		} else {
			try {
				udp.writeTo(u.getIdentity().getIp(), u.getIdentity().getUdpPort(), cmd.toString(getMe()->getCID()));
			} catch(const SocketException&) {
				dcdebug("Socket exception sending ADC UDP command\n");
			}
		}
	}
}

void ClientManager::infoUpdated() {
	Lock l(cs);
	for(auto i: clients) {
		if(i->isConnected()) {
			i->info(false);
		}
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize,
									int aFileType, const string& aString) noexcept
{
	Speaker<ClientManagerListener>::fire(ClientManagerListener::IncomingSearch(), aString);

	bool isPassive = (aSeeker.compare(0, 4, "Hub:") == 0);

	// We don't wan't to answer passive searches if we're in passive mode...
	if(isPassive && !ClientManager::getInstance()->isActive()) {
		return;
	}

	SearchResultList l;
	ShareManager::getInstance()->search(l, aString, aSearchType, aSize, aFileType, aClient, isPassive ? 5 : 10);
//		dcdebug("Found %d items (%s)\n", l.size(), aString.c_str());
	if(!l.empty()) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(const auto& sr: l) {
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += name;
				str += '|';
			}

			if(!str.empty())
				aClient->send(str);

		} else {
			try {
				string ip, port, file, proto, query, fragment;

				Util::decodeUrl(aSeeker, proto, ip, port, file, query, fragment);
				ip = Socket::resolve(ip, AF_INET);
				if(static_cast<NmdcHub*>(aClient)->isProtectedIP(ip))
					return;
				if(port.empty())
					port = "412";
				for(const auto& sr: l) {
					udp.writeTo(ip, port, sr->toSR(*aClient));
				}
			} catch(const SocketException& /* e */) {
				dcdebug("Search caught error\n");
			}
		}
	}
}

void ClientManager::on(AdcSearch, Client*, const AdcCommand& adc, const CID& from) noexcept {
	bool isUdpActive = false;
	{
		Lock l(cs);

		auto i = onlineUsers.find(from);
		if(i != onlineUsers.end()) {
			OnlineUser& u = *i->second;
			isUdpActive = u.getIdentity().isUdpActive();
		}

	}
	SearchManager::getInstance()->respond(adc, from, isUdpActive);
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	for(auto i: clients) {
		if(i->isConnected()) {
			i->search(aSizeMode, aSize, aFileType, aString, aToken, StringList() /*ExtList*/);
		}
	}
}

void ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList) {
	Lock l(cs);

	for(auto& client: who) {
		for(auto c: clients) {
			if(c->isConnected() && c->getHubUrl() == client) {
				c->search(aSizeMode, aSize, aFileType, aString, aToken, aExtList);
			}
		}
	}
}

void ClientManager::on(TimerManagerListener::Minute, uint64_t /* aTick */) noexcept {
	Lock l(cs);

	// Collect some garbage...
	auto i = users.begin();
	while(i != users.end()) {
		if(i->second->unique()) {
			users.erase(i++);
		} else {
			++i;
		}
	}

	for(auto j: clients) {
		j->info(false);
	}
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			users.insert(make_pair(me->getCID(), me));
		}
	}
	return me;
}

bool ClientManager::isActive() const {
	return CONNSETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_PASSIVE;
}

const CID& ClientManager::getMyPID() {
	if(pid.isZero())
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(getMyPID().data(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::updateUser(const OnlineUser& user) noexcept {
	if(!user.getIdentity().getNick().empty()) {
		Lock l(cs);
		auto i = nicks.find(user.getUser()->getCID());
		if(i == nicks.end()) {
			nicks[user.getUser()->getCID()] = std::make_pair(user.getIdentity().getNick(), false);
		} else {
			i->second.first = user.getIdentity().getNick();
		}
	}

	fire(ClientManagerListener::UserUpdated(), user);
}

void ClientManager::loadUsers() {
	try {
		SimpleXML xml;
		xml.fromXML(File(getUsersFile(), File::READ, File::OPEN).read());

		if(xml.findChild("Users")) {
			xml.stepIn();

			{
				Lock l(cs);
				while(xml.findChild("User")) {
					nicks[CID(xml.getChildAttrib("CID"))] = std::make_pair(xml.getChildAttrib("Nick"), false);
				}
			}

			xml.stepOut();
		}
	} catch(const Exception&) { }
}

void ClientManager::saveUsers() const {
	try {
		SimpleXML xml;
		xml.addTag("Users");
		xml.stepIn();

		{
			Lock l(cs);
			for(auto& i: nicks) {
				if(i.second.second) {
					xml.addTag("User");
					xml.addChildAttrib("CID", i.first.toBase32());
					xml.addChildAttrib("Nick", i.second.first);
				}
			}
		}

		xml.stepOut();

		const string fName = getUsersFile();
		File out(fName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(fName);
		File::renameFile(fName + ".tmp", fName);
	} catch(const Exception&) { }
}

void ClientManager::saveUser(const CID& cid) {
	Lock l(cs);
	auto i = nicks.find(cid);
	if(i != nicks.end())
		i->second.second = true;
}

void ClientManager::on(Connected, Client* c) noexcept {
	fire(ClientManagerListener::ClientConnected(), c);
}

void ClientManager::on(UserUpdated, Client*, const OnlineUser& user) noexcept {
	updateUser(user);
}

void ClientManager::on(UsersUpdated, Client*, const OnlineUserList& l) noexcept {
	for(auto& i: l) {
		updateUser(*i);
	}
}

void ClientManager::on(HubUpdated, Client* c) noexcept {
	fire(ClientManagerListener::ClientUpdated(), c);
}

void ClientManager::on(Failed, Client* client, const string&) noexcept {
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(HubUserCommand, Client* client, int aType, int ctx, const string& name, const string& command) noexcept {
	if(BOOLSETTING(HUB_USER_COMMANDS)) {
		if(aType == UserCommand::TYPE_REMOVE) {
			int cmd = FavoriteManager::getInstance()->findUserCommand(name, client->getHubUrl());
			if(cmd != -1)
				FavoriteManager::getInstance()->removeUserCommand(cmd);
		} else if(aType == UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
 			FavoriteManager::getInstance()->addUserCommand(aType, ctx, UserCommand::FLAG_NOSAVE, name, command, "", client->getHubUrl());
 		}
	}
}

} // namespace dcpp
