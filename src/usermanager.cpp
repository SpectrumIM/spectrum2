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

namespace Transport {

UserManager::UserManager(Component *component, StorageBackend *storageBackend) {
	m_cachedUser = NULL;
	m_onlineBuddies = 0;
	m_component = component;
	m_storageBackend = storageBackend;

	component->onUserPresenceReceived.connect(bind(&UserManager::handlePresence, this, _1));
// 	component->onDiscoInfoResponse.connect(bind(&UserManager::handleDiscoInfoResponse, this, _1, _2, _3));
}

UserManager::~UserManager(){
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
	if (!user ) {
		// No user and unavailable presence -> answer with unavailable
		if (presence->getType() == Swift::Presence::Unavailable) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(presence->getFrom());
			response->setFrom(presence->getTo());
			response->setType(Swift::Presence::Unavailable);
			m_component->getStanzaChannel()->sendPresence(response);

			UserInfo res;
			bool registered = m_storageBackend->getUser(userkey, res);
			if (registered) {
				m_storageBackend->setUserOnline(res.id, false);
			}
			return;
		}

		UserInfo res;
		bool registered = m_storageBackend->getUser(userkey, res);

		if (!registered && m_component->inServerMode()) {
			res.password = m_component->getUserRegistryPassword(userkey);
			res.uin = presence->getFrom().getNode();
			if (res.uin.find_last_of("%") != std::string::npos) {
				res.uin.replace(res.uin.find_last_of("%"), 1, "@");
			}
			registered = true;
			m_storageBackend->setUser(res);
		}

		if (!registered) {
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
				user = new User(presence->getFrom(), res, m_component);
				// TODO: handle features somehow
// // 			user->setFeatures(isVip ? CONFIG().VIPFeatures : CONFIG().transportFeatures);
// // // 				if (c != NULL)
// // // 					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
// // // 						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));
// // // 
			addUser(user);
	}
	user->handlePresence(presence);

	if (presence->getType() == Swift::Presence::Unavailable) {
		if (user) {
			Swift::Presence::ref highest = m_component->getPresenceOracle()->getHighestPriorityPresence(presence->getFrom().toBare());
			// There's no presence for this user, so disconnect
			if (!highest || (highest && highest->getType() == Swift::Presence::Unavailable)) {
				removeUser(user);
			}
		}
		// TODO: HANDLE MUC SOMEHOW
// 		else if (user && Transport::instance()->protocol()->tempAccountsAllowed() && !((User *) user)->hasOpenedMUC()) {
// 			Transport::instance()->userManager()->removeUser(user);
// 		}
	}
}

}
