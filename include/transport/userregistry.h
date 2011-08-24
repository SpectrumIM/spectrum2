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
#include "Swiften/Server/UserRegistry.h"
#include "transport/config.h"

namespace Transport {

/// Validates passwords in Server mode.

/// Normal login workflow could work like following:
/**
	\msc
	UserManager,UserRegistry,ServerFromClientSession;
	ServerFromClientSession->UserRegistry [label="isValidUserPassword(...)", URL="\ref UserRegistry::isValidUserPassword()"];
	UserManager<-UserRegistry [label="onConnectUser(...)", URL="\ref UserRegistry::onConnectUser()"];
	---  [ label = "UserManager logins user and validates password"];
	UserManager->UserRegistry [label="onPasswordValid(...)", URL="\ref UserRegistry::onPasswordValid()"];
	ServerFromClientSession<-UserRegistry [label="handlePasswordValid(...)", URL="\ref ServerFromClientSession::handlePasswordValid()"];
	\endmsc
*/
/// User can of course disconnect during login process. In this case, stopLogin() method is called which informs upper layer
/// that user disconnected:
/**
	\msc
	UserManager,UserRegistry,ServerFromClientSession;
	ServerFromClientSession->UserRegistry [label="isValidUserPassword(...)", URL="\ref UserRegistry::isValidUserPassword()"];
	UserManager<-UserRegistry [label="onConnectUser(...)", URL="\ref UserRegistry::onConnectUser()"];
	---  [ label = "UserManager is logging in the user, but he disconnected"];
	ServerFromClientSession->UserRegistry [label="stopLogin(...)", URL="\ref UserRegistry::stopLogin()"];
	UserManager<-UserRegistry [label="onDisconnectUser(...)", URL="\ref UserRegistry::onDisconnectUser()"];
	---  [ label = "UserManager disconnects the user"];
	\endmsc
*/
class UserRegistry : public Swift::UserRegistry {
	public:
		/// Creates new UserRegistry.
		/// \param cfg Config file
		/// 	- service.admin_username - username for admin account
		/// 	- service.admin_password - password for admin account
		UserRegistry(Config *cfg, Swift::NetworkFactories *factories);

		/// Destructor.
		virtual ~UserRegistry();

		/// Called to detect wheter the password is valid.
		/// \param user JID of user who is logging in.
		/// \param session Session associated with this user.
		/// \param password Password used for this session.
		void isValidUserPassword(const Swift::JID& user, Swift::ServerFromClientSession *session, const Swift::SafeByteArray& password);

		/// Called when user disconnects during login process. Disconnects user from legacy network.
		/// \param user JID.
		/// \param session Session.
		void stopLogin(const Swift::JID& user, Swift::ServerFromClientSession *session);

		/// Informs user that the password is valid and finishes login process.
		/// \param user JID.
		void onPasswordValid(const Swift::JID &user);

		/// Informs user that the password is invalid and disconnects him.
		/// \param user JID.
		void onPasswordInvalid(const Swift::JID &user);

		/// Removes session later.
		/// \param user JID.
		void removeLater(const Swift::JID &user);

		/// Returns current password for particular user
		/// \param barejid JID.
		const std::string getUserPassword(const std::string &barejid);

		/// Emitted when user wants to connect legacy network to validate the password.
		boost::signal<void (const Swift::JID &user)> onConnectUser;

		/// Emitted when user disconnected XMPP server and therefore should disconnect legacy network.
		boost::signal<void (const Swift::JID &user)> onDisconnectUser;

	private:
		typedef struct {
			std::string password;
			Swift::ServerFromClientSession *session;
		} Sess;

		void handleRemoveTimeout(const Swift::JID &user);

		mutable std::map<std::string, Sess> users;
		mutable Config *config;
		Swift::Timer::ref m_removeTimer;
		bool m_inRemoveLater;
};

}
