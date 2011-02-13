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

namespace Transport {

UserManager::UserManager(Component *component) {
	m_cachedUser = NULL;
	m_onlineBuddies = 0;

	component->onUserPresenceReceived.connect(bind(&UserManager::handlePresence, this, _1));
}

UserManager::~UserManager(){
}

User *UserManager::getUserByJID(const std::string &barejid){
	if (m_cachedUser && barejid == m_cachedUser->getJID().toBare().toString().getUTF8String()) {
		return m_cachedUser;
	}

	if (m_users.find(barejid) != m_users.end()) {
		User *user = m_users[barejid];
		m_cachedUser = user;
		return user;
	}
	return NULL;
}

int UserManager::userCount() {
	return m_users.size();
}

void UserManager::handlePresence(Swift::Presence::ref presence) {
// 	std::string barejid = presence->getTo().toBare().toString().getUTF8String();
// 	std::string userkey = presence->getFrom().toBare().toString().getUTF8String();
// // 	if (Transport::instance()->protocol()->tempAccountsAllowed()) {
// // 		std::string server = barejid.substr(barejid.find("%") + 1, barejid.length() - barejid.find("%"));
// // 		userkey += server;
// // 	}
// 
// 	User *user = getUserByJID(userkey);
// 	if (user ) {
// 		user->handlePresence(presence);
// 	}
// 	else {
// // 		// No user, unavailable presence... nothing to do
// // 		if (presence->getType() == Swift::Presence::Unavailable) {
// // 			Swift::Presence::ref response = Swift::Presence::create();
// // 			response->setTo(presence->getFrom());
// // 			response->setFrom(Swift::JID(Transport::instance()->jid()));
// // 			response->setType(Swift::Presence::Unavailable);
// // 			m_component->sendPresence(response);
// // 
// // // 			UserRow res = Transport::instance()->sql()->getUserByJid(userkey);
// // // 			if (res.id != -1) {
// // // 				Transport::instance()->sql()->setUserOnline(res.id, false);
// // // 			}
// // 			return;
// // 		}
// // 		UserRow res = Transport::instance()->sql()->getUserByJid(userkey);
// // 		if (res.id == -1 && !Transport::instance()->protocol()->tempAccountsAllowed()) {
// // 			// presence from unregistered user
// // 			Log(presence->getFrom().toString().getUTF8String(), "This user is not registered");
// // 			return;
// // 		}
// // 		else {
// // 			if (res.id == -1 && Transport::instance()->protocol()->tempAccountsAllowed()) {
// // 				res.jid = userkey;
// // 				res.uin = presence->getFrom().toBare().toString().getUTF8String();
// // 				res.password = "";
// // 				res.language = "en";
// // 				res.encoding = CONFIG().encoding;
// // 				res.vip = 0;
// // 				Transport::instance()->sql()->addUser(res);
// // 				res = Transport::instance()->sql()->getUserByJid(userkey);
// // 			}
// // 
// // 			bool isVip = res.vip;
// // 			std::list<std::string> const &x = CONFIG().allowedServers;
// // 			if (CONFIG().onlyForVIP && !isVip && std::find(x.begin(), x.end(), presence->getFrom().getDomain().getUTF8String()) == x.end()) {
// // 				Log(presence->getFrom().toString().getUTF8String(), "This user is not VIP, can't login...");
// // 				return;
// // 			}
// // 
// // 			Log(presence->getFrom().toString().getUTF8String(), "Creating new User instance");
// // 
// // 			if (Transport::instance()->protocol()->tempAccountsAllowed()) {
// // 				std::string server = barejid.substr(barejid.find("%") + 1, barejid.length() - barejid.find("%"));
// // 				res.uin = presence->getTo().getResource().getUTF8String() + "@" + server;
// // 			}
// // 			else {
// // 				if (purple_accounts_find(res.uin.c_str(), Transport::instance()->protocol()->protocol().c_str()) != NULL) {
// // 					PurpleAccount *act = purple_accounts_find(res.uin.c_str(), Transport::instance()->protocol()->protocol().c_str());
// // 					user = Transport::instance()->userManager()->getUserByAccount(act);
// // 					if (user) {
// // 						Log(presence->getFrom().toString().getUTF8String(), "This account is already connected by another jid " << user->jid());
// // 						return;
// // 					}
// // 				}
// // 			}
// // 			user = (AbstractUser *) new User(res, userkey, m_component, m_presenceOracle, m_entityCapsManager);
// // 			user->setFeatures(isVip ? CONFIG().VIPFeatures : CONFIG().transportFeatures);
// // // 				if (c != NULL)
// // // 					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
// // // 						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));
// // // 
// // 			Transport::instance()->userManager()->addUser(user);
// // 			user->receivedPresence(presence);
// // // 				if (protocol()->tempAccountsAllowed()) {
// // // 					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
// // // 					server = stanza.from().bare() + server;
// // // 					purple_timeout_add_seconds(15, &connectUser, g_strdup(server.c_str()));
// // // 				}
// // // 				else
// // // 					purple_timeout_add_seconds(15, &connectUser, g_strdup(stanza.from().bare().c_str()));
// // // 			}
// // 		}
// // // 		if (stanza.presence() == Presence::Unavailable && stanza.to().username() == ""){
// // // 			Log(stanza.from().full(), "User is already logged out => sending unavailable presence");
// // // 			Tag *tag = new Tag("presence");
// // // 			tag->addAttribute( "to", stanza.from().bare() );
// // // 			tag->addAttribute( "type", "unavailable" );
// // // 			tag->addAttribute( "from", jid() );
// // // 			j->send( tag );
// // // 		}
// 	}
// 
// 	if (presence->getType() == Swift::Presence::Unavailable) {
// 		if (user) {
// 			Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(presence->getFrom().toBare());
// 			if (presence->getType() == Swift::Presence::Unavailable && (!highest || (highest && highest->getType() == Swift::Presence::Unavailable))) {
// 				Transport::instance()->userManager()->removeUser(user);
// 			}
// 		}
// 		else if (user && Transport::instance()->protocol()->tempAccountsAllowed() && !((User *) user)->hasOpenedMUC()) {
// 			Transport::instance()->userManager()->removeUser(user);
// 		}
// 	}
}

}
