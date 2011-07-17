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
#include "Swiften/Swiften.h"
#include "transport/userregistry.h"

namespace Transport {

class User;
class Component;
class StorageBackend;
class StorageResponder;
class RosterResponder;

/// Manages online XMPP Users.

/// This class handles presences and creates User classes when new user connects.
/// It also removes the User class once the last user's resource disconnected.
class UserManager {
	public:
		/// Creates new UserManager.
		/// \param component Component which's presence will be handled
		/// \param storageBackend Storage backend used to fetch UserInfos
		UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);

		/// Destroys UserManager.
		~UserManager();

		/// Returns user according to his bare JID.
		/// \param barejid bare JID of user
		/// \return User class associated with this user
		User *getUser(const std::string &barejid);

		const std::map<std::string, User *> &getUsers() {
			return m_users;
		}

		/// Returns number of online users.
		/// \return number of online users
		int getUserCount();

		/// Removes user. This function disconnects user and safely removes
		/// User class. This does *not* remove user from database.
		/// \param user User class to remove
		void removeUser(User *user);

		/// Called when new User class is created.
		/// \param user newly created User class
		boost::signal<void (User *user)> onUserCreated;

		/// Called when User class is going to be removed
		/// \param user removed User class
		boost::signal<void (User *user)> onUserDestroyed;

		bool isUserConnected(const std::string &barejid) const {
			return m_users.find(barejid) != m_users.end();
		}

		UserRegistry *getUserRegistry() {
			return m_userRegistry;
		}

		void connectUser(const Swift::JID &user);

	private:
		void handlePresence(Swift::Presence::ref presence);
		void handleMessageReceived(Swift::Message::ref message);
		void handleGeneralPresenceReceived(Swift::Presence::ref presence);
		void handleProbePresence(Swift::Presence::ref presence);
		void handleSubscription(Swift::Presence::ref presence);
// 		void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
		void addUser(User *user);

		long m_onlineBuddies;
		User *m_cachedUser;
		std::map<std::string, User *> m_users;
		Component *m_component;
		StorageBackend *m_storageBackend;
		StorageResponder *m_storageResponder;
		UserRegistry *m_userRegistry;
		friend class RosterResponder;
};

}
