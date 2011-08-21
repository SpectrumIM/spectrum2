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
	
class UserRegistry : public Swift::UserRegistry {
	public:
		UserRegistry(Config *cfg);
		virtual ~UserRegistry();

		void isValidUserPassword(const Swift::JID& user, Swift::ServerFromClientSession *session, const Swift::SafeByteArray& password);

		void stopLogin(const Swift::JID& user, Swift::ServerFromClientSession *session);

		void onPasswordValid(const Swift::JID &user);

		void onPasswordInvalid(const Swift::JID &user);

		const std::string &getUserPassword(const std::string &barejid);

		boost::signal<void (const Swift::JID &user)> onConnectUser;
		boost::signal<void (const Swift::JID &user)> onDisconnectUser;


	private:
		typedef struct {
			std::string password;
			Swift::ServerFromClientSession *session;
		} Sess;

		mutable std::map<std::string, Sess> users;
		mutable Config *config;
};

}
