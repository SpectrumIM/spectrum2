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

#include "transport/Event.h"

#include <string>
#include <map>
#include <functional>

#include "transport/protocol.pb.h"

namespace Transport {

class User;
class Component;
class StorageBackend;
class UserRegistration;
class UserRegistry;
struct UserInfo;

/// Manages online Users.

/// The frontend's subclass logs users in (loginUser) when they connect and
/// removes them (removeUser) when they go away.

/// Basic user creation process:
/**
	\msc
	Frontend,UserManager,StorageBackend,User,Slot;
	---  [ label = "User connects"];
	Frontend->StorageBackend [label="getUser(...)", URL="\ref StorageBackend::getUser()"];
	Frontend->UserManager [label="loginUser(...)", URL="\ref UserManager::loginUser()"];
	UserManager->User [label="User::User(...)", URL="\ref User"];
	UserManager->Slot [label="onUserCreated(...)", URL="\ref UserManager::onUserCreated()"];
	\endmsc
*/
class UserManager {
	public:
		/// Why loginUser refused the login.
		typedef enum {
			LoginOK,
			LoginNotVIP,
			LoginTransportDisabled,
		} LoginError;

		/// Creates new UserManager.
		/// \param component Component
		/// \param storageBackend Storage backend used to fetch UserInfos
		UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);

		/// Destroys UserManager.
		virtual ~UserManager();

		/// Answers the vcard request with this id (see Frontend::onVCardRequired).
		virtual void sendVCard(unsigned int id, const pbnetwork::VCard &vcard) = 0;

		virtual UserRegistration *getUserRegistration() = 0;

		/// Returns user according to his name (bare JID for XMPP).
		/// \param user name of user
		/// \return User class associated with this user
		User *getUser(const std::string &user);

		/// Returns map with all connected users.
		/// \return All connected users.
		const std::map<std::string, User *> &getUsers() {
			return m_users;
		}

		/// Returns number of online users.
		/// \return number of online users
		int getUserCount();

		/// Stores res as a new registration of the user (auto-registration).
		/// \param res UserInfo to store; reloaded from the storage to get the user ID.
		/// \return true if the user is registered now.
		bool autoRegisterUser(UserInfo &res);

		/// Logs in a registered user: checks service.vip_only and the user's
		/// enable_transport setting, creates the User and emits onUserCreated.
		/// \param user name of user
		/// \param res UserInfo of the registered user
		/// \param error why the login was refused
		/// \return the User, or NULL if the login was refused or the user was disconnected
		/// in onUserCreated.
		User *loginUser(const std::string &user, UserInfo &res, LoginError &error);

		/// Removes user. This function disconnects user and safely removes
		/// User class. This does *not* remove user from StorageBackend.
		/// \param user User class to remove
		void removeUser(User *user, bool onUserBehalf = true);

		void removeAllUsers(bool onUserBehalf = true);

		/// Called when new User class is created.
		/// \param user newly created User class
		Event<void (User *user)> onUserCreated;

		/// Called when User class is going to be removed
		/// \param user removed User class
		Event<void (User *user)> onUserDestroyed;

		/// Returns true if user is connected.
		/// \return True if user is connected.
		bool isUserConnected(const std::string &user) const {
			return m_users.find(user) != m_users.end();
		}

		/// Returns pointer to UserRegistry.
		/// \return Pointer to UserRegistry.
		UserRegistry *getUserRegistry() {
			return m_userRegistry;
		}

		Component *getComponent() {
			return m_component;
		}

		StorageBackend *getStorageBackend() {
			return m_storageBackend;
		}

		void messageToXMPPSent() { m_sentToXMPP++; }
		void messageToBackendSent() { m_sentToBackend++; }

		unsigned long getMessagesToXMPP() { return m_sentToXMPP; }
		unsigned long getMessagesToBackend() { return m_sentToBackend; }

	protected:
		/// Called by removeUser after the user is taken out of the user list, before
		/// onUserDestroyed and its deletion.
		virtual void doRemoveUser(User *user) {}

	private:
		void addUser(User *user);

		User *m_cachedUser;
		std::map<std::string, User *> m_users;
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserRegistry *m_userRegistry;
		unsigned long m_sentToXMPP;
		unsigned long m_sentToBackend;
};

}
