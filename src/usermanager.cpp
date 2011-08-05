/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "transport/usermanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/conversationmanager.h"
#include "transport/rostermanager.h"
#include "transport/userregistry.h"
#include "storageresponder.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("UserManager");

UserManager::UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	m_cachedUser = NULL;
	m_onlineBuddies = 0;
	m_component = component;
	m_storageBackend = storageBackend;
	m_storageResponder = NULL;
	m_userRegistry = userRegistry;

	if (m_storageBackend) {
		m_storageResponder = new StorageResponder(component->getIQRouter(), m_storageBackend, this);
		m_storageResponder->start();
	}

	component->onUserPresenceReceived.connect(bind(&UserManager::handlePresence, this, _1));
	m_component->getStanzaChannel()->onMessageReceived.connect(bind(&UserManager::handleMessageReceived, this, _1));
	m_component->getStanzaChannel()->onPresenceReceived.connect(bind(&UserManager::handleGeneralPresenceReceived, this, _1));

	m_userRegistry->onConnectUser.connect(bind(&UserManager::connectUser, this, _1));
// 	component->onDiscoInfoResponse.connect(bind(&UserManager::handleDiscoInfoResponse, this, _1, _2, _3));

	m_removeTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1);
}

UserManager::~UserManager(){
	if (m_storageResponder) {
		m_storageResponder->stop();
		delete m_storageResponder;
	}
}

void UserManager::addUser(User *user) {
	m_users[user->getJID().toBare().toString()] = user;
	onUserCreated(user);
}

User *UserManager::getUser(const std::string &barejid){
	if (m_cachedUser && barejid == m_cachedUser->getJID().toBare().toString()) {
		return m_cachedUser;
	}

	if (m_users.find(barejid) != m_users.end()) {
		User *user = m_users[barejid];
		m_cachedUser = user;
		return user;
	}
	return NULL;
}

void UserManager::removeUser(User *user) {
	m_users.erase(user->getJID().toBare().toString());
	if (m_cachedUser == user)
		m_cachedUser = NULL;
	onUserDestroyed(user);
	delete user;
}

int UserManager::getUserCount() {
	return m_users.size();
}

void UserManager::handlePresence(Swift::Presence::ref presence) {
	std::string barejid = presence->getTo().toBare().toString();
	std::string userkey = presence->getFrom().toBare().toString();

	User *user = getUser(userkey);
	if (!user) {
		if (CONFIG_STRING(m_component->getConfig(), "service.admin_username") == presence->getFrom().getNode()) {
			return;
		}

		// No user and unavailable presence -> answer with unavailable
		if (presence->getType() == Swift::Presence::Unavailable) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(presence->getFrom());
			response->setFrom(presence->getTo());
			response->setType(Swift::Presence::Unavailable);
			m_component->getStanzaChannel()->sendPresence(response);

			UserInfo res;
			if (m_storageBackend) {
				bool registered = m_storageBackend->getUser(userkey, res);
				if (registered) {
					m_storageBackend->setUserOnline(res.id, false);
				}
			}
			return;
		}

		UserInfo res;
		bool registered = m_storageBackend ? m_storageBackend->getUser(userkey, res) : false;

		if (m_component->inServerMode()) {
			if (!registered) {
				res.password = "";
				res.uin = presence->getFrom().getNode();
				res.jid = userkey;
				if (res.uin.find_last_of("%") != std::string::npos) {
					res.uin.replace(res.uin.find_last_of("%"), 1, "@");
				}
				if (m_storageBackend) {
					m_storageBackend->setUser(res);
					registered = m_storageBackend->getUser(userkey, res);
				}
				else {
					registered = true;
				}
			}
			res.password = m_userRegistry->getUserPassword(userkey);
		}

		if (!registered) {
			LOG4CXX_WARN(logger, "Unregistered user " << userkey << " tried to login");
			// TODO: logging
			return;
		}

		// TODO: isVIP
// // 			bool isVip = res.vip;
// // 			std::list<std::string> const &x = CONFIG().allowedServers;
// // 			if (CONFIG().onlyForVIP && !isVip && std::find(x.begin(), x.end(), presence->getFrom().getDomain().getUTF8String()) == x.end()) {
// // 				Log(presence->getFrom().toString().getUTF8String(), "This user is not VIP, can't login...");
// // 				return;
// // 			}
// // 
// // 
				user = new User(presence->getFrom(), res, m_component, this);
				user->getRosterManager()->setStorageBackend(m_storageBackend);
				// TODO: handle features somehow
// // 			user->setFeatures(isVip ? CONFIG().VIPFeatures : CONFIG().transportFeatures);
// // // 				if (c != NULL)
// // // 					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
// // // 						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));
// // // 
			addUser(user);
	}

	// User can be handleDisconnected in addUser callbacks...
	user = getUser(userkey);
	if (!user) {
		m_userRegistry->onPasswordInvalid(presence->getFrom());
		return;
	}

	user->handlePresence(presence);

	bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	if (isMUC)
		return;

	if (presence->getType() == Swift::Presence::Unavailable) {
		if (user) {
			Swift::Presence::ref highest = m_component->getPresenceOracle()->getHighestPriorityPresence(presence->getFrom().toBare());
			// There's no presence for this user, so disconnect
			if (!highest || (highest && highest->getType() == Swift::Presence::Unavailable)) {
				m_removeTimer->onTick.connect(boost::bind(&UserManager::handleRemoveTimeout, this, user->getJID().toBare().toString()));
				m_removeTimer->start();
			}
		}
		// TODO: HANDLE MUC SOMEHOW
// 		else if (user && Transport::instance()->protocol()->tempAccountsAllowed() && !((User *) user)->hasOpenedMUC()) {
// 			Transport::instance()->userManager()->removeUser(user);
// 		}
	}
}

void UserManager::handleRemoveTimeout(const std::string jid) {
	m_removeTimer->onTick.disconnect(boost::bind(&UserManager::handleRemoveTimeout, this, jid));
	User *user = getUser(jid);
	if (user) {
		removeUser(user);
	}
}

void UserManager::handleMessageReceived(Swift::Message::ref message) {
	User *user = getUser(message->getFrom().toBare().toString());
	if (!user ){
		return;
	}

	user->getConversationManager()->handleMessageReceived(message);
}

void UserManager::handleGeneralPresenceReceived(Swift::Presence::ref presence) {
	switch(presence->getType()) {
		case Swift::Presence::Subscribe:
		case Swift::Presence::Subscribed:
		case Swift::Presence::Unsubscribe:
		case Swift::Presence::Unsubscribed:
			handleSubscription(presence);
			break;
		case Swift::Presence::Available:
		case Swift::Presence::Unavailable:
			break;
		case Swift::Presence::Probe:
			handleProbePresence(presence);
			break;
		default:
			break;
	};
}

void UserManager::handleProbePresence(Swift::Presence::ref presence) {
	User *user = getUser(presence->getFrom().toBare().toString());

 	if (user) {
 		user->getRosterManager()->sendCurrentPresence(presence->getTo(), presence->getFrom());
 	}
 	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Unavailable);
		m_component->getStanzaChannel()->sendPresence(response);
	}
}

void UserManager::handleSubscription(Swift::Presence::ref presence) {
	// answer to subscibe for transport itself
	if (presence->getType() == Swift::Presence::Subscribe && presence->getTo().getNode().empty()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Subscribed);
		m_component->getStanzaChannel()->sendPresence(response);

// 		response = Swift::Presence::create();
// 		response->setFrom(presence->getTo());
// 		response->setTo(presence->getFrom());
// 		response->setType(Swift::Presence::Subscribe);
// 		m_component->getStanzaChannel()->sendPresence(response);
		return;
	}

	User *user = getUser(presence->getFrom().toBare().toString());

 	if (user) {
 		user->handleSubscription(presence);
 	}
 	else if (presence->getType() == Swift::Presence::Unsubscribe) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Unsubscribed);
		m_component->getStanzaChannel()->sendPresence(response);
	}
// 	else {
// // 		Log(presence->getFrom().toString().getUTF8String(), "Subscribe presence received, but this user is not logged in");
// 	}
}

void UserManager::connectUser(const Swift::JID &user) {
	if (m_users.find(user.toBare().toString()) != m_users.end()) {
		std::cout << "FOUND\n";
		m_userRegistry->onPasswordValid(user);
	}
	else {
		std::cout << "NOT FOUND - PRESENCe\n";
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(m_component->getJID());
		response->setFrom(user);
		response->setType(Swift::Presence::Available);
		m_component->onUserPresenceReceived(response);
	}
}


}
