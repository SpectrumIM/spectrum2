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

#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/ConversationManager.h"
#include "transport/RosterManager.h"
#include "transport/UserRegistry.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/PresenceOracle.h"
#include "transport/Config.h"

#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/ChatState.h"
#ifndef __FreeBSD__
#ifndef __MACH__
#include "malloc.h"
#endif
#endif
// #include "valgrind/memcheck.h"

namespace Transport {

DEFINE_LOGGER(userManagerLogger, "UserManager");

UserManager::UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	m_cachedUser = NULL;
	m_onlineBuddies = 0;
	m_sentToXMPP = 0;
	m_sentToBackend = 0;
	m_component = component;
	m_storageBackend = storageBackend;
	m_userRegistry = userRegistry;

	component->onUserPresenceReceived.connect(bind(&UserManager::handlePresence, this, _1));
	component->getFrontend()->onCapabilitiesReceived.connect(bind(&UserManager::handleDiscoInfo, this, _1, _2));
	m_component->getFrontend()->onMessageReceived.connect(bind(&UserManager::handleMessageReceived, this, _1));
	m_component->getFrontend()->onPresenceReceived.connect(bind(&UserManager::handleGeneralPresenceReceived, this, _1));

	m_userRegistry->onConnectUser.connect(bind(&UserManager::connectUser, this, _1));
	m_userRegistry->onDisconnectUser.connect(bind(&UserManager::disconnectUser, this, _1));

	m_removeTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1);
}

UserManager::~UserManager() {

}

void UserManager::addUser(User *user) {
	m_users[user->getJID().toBare().toString()] = user;
	if (m_storageBackend) {
		m_storageBackend->setUserOnline(user->getUserInfo().id, true);
	}
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

// Swift::DiscoInfo::ref UserManager::getCaps(const Swift::JID &jid) const {
// 	std::map<std::string, User *>::const_iterator it = m_users.find(jid.toBare().toString());
// 	if (it == m_users.end()) {
// 		return Swift::DiscoInfo::ref();
// 	}
//
// 	User *user = it->second;
// 	return user->getCaps(jid);
// }

void UserManager::removeUser(User *user, bool onUserBehalf) {
	m_users.erase(user->getJID().toBare().toString());
	if (m_cachedUser == user)
		m_cachedUser = NULL;

	if (m_component->inServerMode()) {
		disconnectUser(user->getJID());
	}
	else {
		// User could be disconnected by User::handleDisconnect() method, but
		// Transport::PresenceOracle could still contain his last presence.
		// We have to clear all received presences for this user in PresenceOracle.
		m_component->getPresenceOracle()->clearPresences(user->getJID().toBare());
	}

	if (m_storageBackend && onUserBehalf) {
		m_storageBackend->setUserOnline(user->getUserInfo().id, false);
	}

	LOG4CXX_INFO(userManagerLogger, user->getJID().toBare().toString() << ": Disconnecting user");
	onUserDestroyed(user);
	delete user;
#ifndef WIN32
#ifndef __FreeBSD__
#ifndef __MACH__
#if defined (__GLIBC__)
	malloc_trim(0);
#endif
#endif
#endif
#endif
// 	VALGRIND_DO_LEAK_CHECK;
}

void UserManager::removeAllUsers(bool onUserBehalf) {
	while (m_users.begin() != m_users.end()) {
		removeUser((*m_users.begin()).second, onUserBehalf);
	}
}

int UserManager::getUserCount() {
	return m_users.size();
}

void UserManager::handleDiscoInfo(const Swift::JID& jid, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> info) {
	User *user = getUser(jid.toBare().toString());
	if (!user) {
		return;
	}

	user->handleDiscoInfo(jid, info);
}

void UserManager::handlePresence(Swift::Presence::ref presence) {
	std::string barejid = presence->getTo().toBare().toString();
	std::string userkey = presence->getFrom().toBare().toString();

	User *user = getUser(userkey);
	// Create user class if it's not there
	if (!user) {
		// Admin user is not legacy network user, so do not create User class instance for him
		if (m_component->inServerMode()) {
			std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"service.admin_jid");
			if (std::find(x.begin(), x.end(), presence->getFrom().toBare().toString()) != x.end()) {
				// Send admin contact to the user.
				Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
				Swift::RosterItemPayload item;
				item.setJID(m_component->getJID());
				item.setName("Admin");
				item.setSubscription(Swift::RosterItemPayload::Both);
				payload->addItem(item);

				m_component->getFrontend()->sendRosterRequest(payload, presence->getFrom());

				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(m_component->getJID());
				m_component->getFrontend()->sendPresence(response);
				return;
		    }
		}

		UserInfo res;
		bool registered = m_storageBackend ? m_storageBackend->getUser(userkey, res) : false;

		// No user and unavailable presence -> answer with unavailable
		if (presence->getType() == Swift::Presence::Unavailable || presence->getType() == Swift::Presence::Probe) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(presence->getFrom());
			response->setFrom(presence->getTo());
			response->setType(Swift::Presence::Unavailable);
			m_component->getFrontend()->sendPresence(response);

			// bother him with probe presence, just to be
			// sure he is subscribed to us.
			if (/*registered && */presence->getType() == Swift::Presence::Probe) {
				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(presence->getTo());
				response->setType(Swift::Presence::Probe);
				m_component->getFrontend()->sendPresence(response);
			}

			// Set user offline in database
			if (m_storageBackend) {
				UserInfo res;
				bool registered = m_storageBackend->getUser(userkey, res);
				if (registered) {
					m_storageBackend->setUserOnline(res.id, false);
				}
			}
			return;
		}

		// In server mode, we don't need registration normally, but for networks like IRC
		// or Twitter where there's no real authorization using password, we have to force
		// registration otherwise some data (like bookmarked rooms) could leak.
		if (m_component->inServerMode()) {
			if (!registered) {
				// If we need registration, stop login process because user is not registered
				if (CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", false)
					&& CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needPassword", true)) {
					m_userRegistry->onPasswordInvalid(presence->getFrom());
					LOG4CXX_INFO(userManagerLogger, userkey << ": Tried to login, but is not registered.");
					return;
				}
				res.password = "";
				res.uin = presence->getFrom().getNode();
				res.jid = userkey;
				while (res.uin.find_last_of("%") != std::string::npos) { // OK
					res.uin.replace(res.uin.find_last_of("%"), 1, "@"); // OK
				}
				if (m_storageBackend) {
					// store user and getUser again to get user ID.
					m_storageBackend->setUser(res);
					registered = m_storageBackend->getUser(userkey, res);
				}
				else {
					registered = true;
				}
			}
			res.password = m_userRegistry->getUserPassword(userkey);
		}

		// We allow auto_register feature in gateway-mode. This allows IRC user to register
		// the transport just by joining the room.
		if (!m_component->inServerMode()) {
			if (!registered && (CONFIG_BOOL(m_component->getConfig(), "registration.auto_register")
				 || !CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true))) {
				res.password = "";
				res.jid = userkey;

				bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
				if (isMUC) {
					res.uin = presence->getTo().getResource();
				}
				else {
					res.uin = presence->getFrom().toString();
				}
				LOG4CXX_INFO(userManagerLogger, "Auto-registering user " << userkey << " with uin=" << res.uin);

				if (m_storageBackend) {
					// store user and getUser again to get user ID.
					m_storageBackend->setUser(res);
					registered = m_storageBackend->getUser(userkey, res);
				}
				else {
					registered = true;
				}
			}
		}

		// Unregistered users are not able to login
		if (!registered) {
			LOG4CXX_WARN(userManagerLogger, "Unregistered user " << userkey << " tried to login");
			return;
		}

		if (CONFIG_BOOL(m_component->getConfig(), "service.vip_only") && res.vip == false) {
			if (!CONFIG_STRING(m_component->getConfig(), "service.vip_message").empty()) {
				SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
				msg->setBody(CONFIG_STRING(m_component->getConfig(), "service.vip_message"));
				msg->setTo(presence->getFrom());
				msg->setFrom(m_component->getJID());
				m_component->getFrontend()->sendMessage(msg);
			}

			LOG4CXX_WARN(userManagerLogger, "Non VIP user " << userkey << " tried to login");
			if (m_component->inServerMode()) {
				m_userRegistry->onPasswordInvalid(presence->getFrom());
			}
			return;
		}

		bool transport_enabled = true;
		if (m_storageBackend) {
			std::string value = "1";
			int type = (int) TYPE_BOOLEAN;
			m_storageBackend->getUserSetting(res.id, "enable_transport", type, value);
			transport_enabled = value == "1";
		}
		// User can disabled the transport using adhoc commands
		if (!transport_enabled) {
			LOG4CXX_INFO(userManagerLogger, "User " << userkey << " has disabled transport, not logging");
			return;
		}

		// Create new user class and set storagebackend
		user = m_component->getFrontend()->createUser(presence->getFrom(), res, m_component, this);
		user->getRosterManager()->setStorageBackend(m_storageBackend);
		addUser(user);
	}

	// User can be handleDisconnected in addUser callback, so refresh the pointer
	user = getUser(userkey);
	if (!user) {
		m_userRegistry->onPasswordInvalid(presence->getFrom());
		return;
	}

	// Handle this presence
	user->handlePresence(presence);

	// Unavailable MUC presence should not trigger whole account disconnection, so block it here.
	bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	if (isMUC)
		return;

	// Unavailable presence could remove this user, because he could be unavailable
	if (presence->getType() == Swift::Presence::Unavailable) {
		if (user) {
			if (user->getUserSetting("stay_connected") == "1") {
				return;
			}

			Swift::Presence::ref highest = m_component->getPresenceOracle()->getHighestPriorityPresence(presence->getFrom().toBare());
			// There's no presence for this user, so disconnect
			if (!highest || (highest && highest->getType() == Swift::Presence::Unavailable)) {
				m_removeTimer->onTick.connect(boost::bind(&UserManager::handleRemoveTimeout, this, user->getJID().toBare().toString(), user, false));
				m_removeTimer->start();
			}
		}
	}
}

void UserManager::handleRemoveTimeout(const std::string jid, User *u, bool reconnect) {
	m_removeTimer->onTick.disconnect(boost::bind(&UserManager::handleRemoveTimeout, this, jid, u, reconnect));

	// Maybe this User instance has been deleted in mean time and we would remove new one,
	// so better check for it and ignore deletion if "u" does not exist anymore.
	User *user = getUser(jid);
	if (user != u) {
		return;
	}

	// Remove user
	if (user) {
		removeUser(user);
	}

	// Connect the user again when we're reconnecting.
	if (reconnect) {
		connectUser(jid);
	}
}

void UserManager::handleMessageReceived(Swift::Message::ref message) {
	if (message->getType() == Swift::Message::Error) {
		return;
	}

	// Do not count chatstate notification...
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ChatState> statePayload = message->getPayload<Swift::ChatState>();
	if (!statePayload) {
		messageToBackendSent();
	}

#if HAVE_SWIFTEN_3
	std::string body = message->getBody().get_value_or("");
#else
	std::string body = message->getBody();
#endif
	if (body.empty() && !statePayload && message->getSubject().empty()) {
		return;
	}

	User *user = getUser(message->getFrom().toBare().toString());
	if (!user){
		return;
	}

	user->getConversationManager()->handleMessageReceived(message);
}

void UserManager::handleGeneralPresenceReceived(Swift::Presence::ref presence) {
	LOG4CXX_INFO(userManagerLogger, "PRESENCE2 " << presence->getTo().toString());
	switch(presence->getType()) {
		case Swift::Presence::Subscribe:
		case Swift::Presence::Subscribed:
		case Swift::Presence::Unsubscribe:
		case Swift::Presence::Unsubscribed:
			handleSubscription(presence);
			break;
		case Swift::Presence::Available:
		case Swift::Presence::Unavailable:
			handleMUCPresence(presence);
			break;
		case Swift::Presence::Probe:
			handleProbePresence(presence);
			break;
		case Swift::Presence::Error:
			handleErrorPresence(presence);
			break;
		default:
			break;
	};
}

void UserManager::handleMUCPresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
		return;
	}

	if (presence->getType() == Swift::Presence::Available) {
		handlePresence(presence);
	}
	else if (presence->getType() == Swift::Presence::Unavailable) {
		std::string userkey = presence->getFrom().toBare().toString();
		User *user = getUser(userkey);
		if (user) {
			user->handlePresence(presence);
		}
	}
}

void UserManager::handleProbePresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
		return;
	}

	User *user = getUser(presence->getFrom().toBare().toString());

 	if (user) {
 		user->getRosterManager()->sendCurrentPresence(presence->getTo(), presence->getFrom());
 	}
 	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Unavailable);
		m_component->getFrontend()->sendPresence(response);
	}
}

void UserManager::handleErrorPresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (!presence->getTo().getNode().empty()) {
		return;
	}

	if (!presence->getPayload<Swift::ErrorPayload>()) {
		return;
	}

	if (presence->getPayload<Swift::ErrorPayload>()->getCondition() != Swift::ErrorPayload::SubscriptionRequired) {
		return;
	}

	std::string userkey = presence->getFrom().toBare().toString();
	UserInfo res;
	bool registered = m_storageBackend ? m_storageBackend->getUser(userkey, res) : false;
	if (registered) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo().toBare());
		response->setTo(presence->getFrom().toBare());
		response->setType(Swift::Presence::Subscribe);
		m_component->getFrontend()->sendPresence(response);
	}
}

void UserManager::handleSubscription(Swift::Presence::ref presence) {

	// answer to subscibe for transport itself
	if (presence->getType() == Swift::Presence::Subscribe && presence->getTo().getNode().empty()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo().toBare());
		response->setTo(presence->getFrom().toBare());
		response->setType(Swift::Presence::Subscribed);
		m_component->getFrontend()->sendPresence(response);

// 		response = Swift::Presence::create();
// 		response->setFrom(presence->getTo());
// 		response->setTo(presence->getFrom());
// 		response->setType(Swift::Presence::Subscribe);
// 		m_component->getFrontend()->sendPresence(response);
		return;
	}
	else if (presence->getType() == Swift::Presence::Unsubscribed && presence->getTo().getNode().empty()) {
		std::string userkey = presence->getFrom().toBare().toString();
		UserInfo res;
		bool registered = m_storageBackend ? m_storageBackend->getUser(userkey, res) : false;
		if (registered) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setFrom(presence->getTo().toBare());
			response->setTo(presence->getFrom().toBare());
			response->setType(Swift::Presence::Subscribe);
			m_component->getFrontend()->sendPresence(response);
		}
		return;
	}

	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
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
		m_component->getFrontend()->sendPresence(response);
	}
// 	else {
// // 		Log(presence->getFrom().toString().getUTF8String(), "Subscribe presence received, but this user is not logged in");
// 	}
}

void UserManager::connectUser(const Swift::JID &user) {
	// Called by UserRegistry in server mode when user connects the server and wants
	// to connect legacy network
	if (m_users.find(user.toBare().toString()) != m_users.end()) {
		if (!m_component->inServerMode()) {
			return;
		}

		User *u = m_users[user.toBare().toString()];
		if (u->isConnected()) {
			// User is already logged in, so his password is OK, but this new user has different password => bad password.
			// We can't call m_userRegistry->onPasswordInvalid() here, because this fuction is called from Swift::Parser
			// and onPasswordInvalid destroys whole session together with parser itself, which leads to crash.
			if (m_userRegistry->getUserPassword(user.toBare().toString()) != u->getUserInfo().password) {
				m_userRegistry->removeLater(user);
				return;
			}
			if (CONFIG_BOOL(m_component->getConfig(), "service.more_resources")) {
				m_userRegistry->onPasswordValid(user);
			}
			else {
				// Send message to currently logged in session
				SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
				msg->setBody("You have signed on from another location.");
				msg->setTo(user);
				msg->setFrom(m_component->getJID());
				m_component->getFrontend()->sendMessage(msg);

				// Switch the session = accept new one, disconnect old one.
				// Unavailable presence from old session has to be ignored, otherwise it would disconnect the user from legacy network.
				m_userRegistry->onPasswordValid(user);
				m_component->onUserPresenceReceived.disconnect(bind(&UserManager::handlePresence, this, _1));
// #if HAVE_SWIFTEN_3
// 				dynamic_cast<Swift::ServerStanzaChannel *>(m_component->getFrontend())->finishSession(user, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ToplevelElement>(new Swift::StreamError()), true);
// #else
// 				dynamic_cast<Swift::ServerStanzaChannel *>(m_component->getFrontend())->finishSession(user, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Element>(new Swift::StreamError()), true);
// #endif
				m_component->onUserPresenceReceived.connect(bind(&UserManager::handlePresence, this, _1));
			}
		}
		else {
			// User is created, but not connected => he's loggin in or he just logged out, but hasn't been deleted yet.
			// Stop deletion process if there's any
			m_removeTimer->onTick.disconnect(boost::bind(&UserManager::handleRemoveTimeout, this, user.toBare().toString(), m_users[user.toBare().toString()], false));

			// Delete old User instance but create new one immediatelly
			m_removeTimer->onTick.connect(boost::bind(&UserManager::handleRemoveTimeout, this, user.toBare().toString(), m_users[user.toBare().toString()], true));
			m_removeTimer->start();
		}
	}
	else {
		// simulate initial available presence to start connecting this user.
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(m_component->getJID());
		response->setFrom(user);
		response->setType(Swift::Presence::Available);
		m_component->getFrontend()->onPresenceReceived(response);
	}
}


void UserManager::disconnectUser(const Swift::JID &user) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_component->getJID());
	response->setFrom(user);
	response->setType(Swift::Presence::Unavailable);
	m_component->getFrontend()->onPresenceReceived(response);
}

}
