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

#pragma once

#include <string>
#include <map>
#include <boost/signals2.hpp>
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/JID/JID.h"
#include "Swiften/Disco/EntityCapsProvider.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/VCard.h"
#include "Swiften/Network/Timer.h"

namespace Transport {

class User;
class Component;
class StorageBackend;
class StorageResponder;
class RosterResponder;
class UserRegistration;
class UserRegistry;

/// Manages online XMPP Users.

/// This class handles presences and creates User classes when new user connects.
/// It also removes the User class once the last user's resource disconnected.

/// Basic user creation process:
/**
	\msc
	Component,UserManager,User,StorageBackend,Slot;
	---  [ label = "Available presence received"];
	Component->UserManager [label="handlePresence(...)", URL="\ref UserManager::handlePresence()"];
	UserManager->StorageBackend [label="getUser(...)", URL="\ref StorageBackend::getUser()"];
	UserManager->User [label="User::User(...)", URL="\ref User"];
	UserManager->Slot [label="onUserCreated(...)", URL="\ref UserManager::onUserCreated()"];
	UserManager->User [label="handlePresence(...)", URL="\ref User::handlePresence()"];
	\endmsc
*/
class UserManager /*: public Swift::EntityCapsProvider*/ {
	public:
		/// Creates new UserManager.
		/// \param component Component which's presence will be handled
		/// \param storageBackend Storage backend used to fetch UserInfos
		UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);

		/// Destroys UserManager.
		virtual ~UserManager();

		virtual void sendVCard(unsigned int id, Swift::VCard::ref vcard) = 0;

		virtual UserRegistration *getUserRegistration() = 0;

		/// Returns user according to his bare JID.
		/// \param barejid bare JID of user
		/// \return User class associated with this user
		User *getUser(const std::string &barejid);

		/// Returns map with all connected users.
		/// \return All connected users.
		const std::map<std::string, User *> &getUsers() {
			return m_users;
		}

		/// Returns number of online users.
		/// \return number of online users
		int getUserCount();

		/// Removes user. This function disconnects user and safely removes
		/// User class. This does *not* remove user from StorageBackend.
		/// \param user User class to remove
		void removeUser(User *user, bool onUserBehalf = true);

		void removeAllUsers(bool onUserBehalf = true);

// 		Swift::DiscoInfo::ref getCaps(const Swift::JID&) const;

		/// Called when new User class is created.
		/// \param user newly created User class
		boost::signals2::signal<void (User *user)> onUserCreated;

		/// Called when User class is going to be removed
		/// \param user removed User class
		boost::signals2::signal<void (User *user)> onUserDestroyed;

		/// Returns true if user is connected.
		/// \return True if user is connected.
		bool isUserConnected(const std::string &barejid) const {
			return m_users.find(barejid) != m_users.end();
		}

		/// Returns pointer to UserRegistry.
		/// \return Pointer to UserRegistry.
		UserRegistry *getUserRegistry() {
			return m_userRegistry;
		}

		Component *getComponent() {
			return m_component;
		}

		/// Connects user manually.
		/// \param user JID of user.
		void connectUser(const Swift::JID &user);

		/// Disconnects user manually.
		/// \param user JID of user.
		void disconnectUser(const Swift::JID &user);

		void messageToXMPPSent() { m_sentToXMPP++; }
		void messageToBackendSent() { m_sentToBackend++; }

		unsigned long getMessagesToXMPP() { return m_sentToXMPP; }
		unsigned long getMessagesToBackend() { return m_sentToBackend; }
		

	private:
		void handlePresence(Swift::Presence::ref presence);
		void handleMessageReceived(Swift::Message::ref message);
		void handleGeneralPresenceReceived(Swift::Presence::ref presence);
		void handleProbePresence(Swift::Presence::ref presence);
		void handleErrorPresence(Swift::Presence::ref presence);
		void handleSubscription(Swift::Presence::ref presence);
		void handleMUCPresence(Swift::Presence::ref presence);
		void handleRemoveTimeout(const std::string jid, User *user, bool reconnect);
		void handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info);
		void addUser(User *user);

		long m_onlineBuddies;
		User *m_cachedUser;
		std::map<std::string, User *> m_users;
		Component *m_component;
		StorageBackend *m_storageBackend;
		StorageResponder *m_storageResponder;
		UserRegistry *m_userRegistry;
		Swift::Timer::ref m_removeTimer;
		unsigned long m_sentToXMPP;
		unsigned long m_sentToBackend;
		friend class RosterResponder;
};

}
