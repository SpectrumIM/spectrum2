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
		UserRegistry(Config *cfg) {config = cfg;}
		~UserRegistry() {}
		void isValidUserPassword(const Swift::JID& user, Swift::ServerFromClientSession *session, const Swift::SafeByteArray& password) {
			if (!CONFIG_STRING(config, "service.admin_username").empty() && user.getNode() == CONFIG_STRING(config, "service.admin_username")) {
				if (Swift::safeByteArrayToString(password) == CONFIG_STRING(config, "service.admin_password")) {
					session->handlePasswordValid();
				}
				else {
					session->handlePasswordInvalid();
				}
				return;
			}

			std::string key = user.toBare().toString();

			// Users try to connect twice
			if (users.find(key) != users.end()) {
				// Kill the first session if the second password is same
				if (Swift::safeByteArrayToString(password) == users[key].password) {
					Swift::ServerFromClientSession *tmp = users[key].session;
					users[key].session = session;
					tmp->handlePasswordInvalid();
				}
				else {
					session->handlePasswordInvalid();
					std::cout << "invalid " << session << "\n";
					return;
				}
			}
			std::cout << "adding " << session << "\n";
			users[key].password = Swift::safeByteArrayToString(password);
			users[key].session = session;
			onConnectUser(user);

			return;
		}

		void stopLogin(const Swift::JID& user, Swift::ServerFromClientSession *session) {
			std::cout << "stopping " << session << "\n";
			std::string key = user.toBare().toString();
			if (users.find(key) != users.end()) {
				if (users[key].session == session) {
					std::cout << "DISCONNECT USER\n";
					onDisconnectUser(user);
					users.erase(key);
				}
			}
		}

		void onPasswordValid(const Swift::JID &user) {
			std::string key = user.toBare().toString();
			if (users.find(key) != users.end()) {
				users[key].session->handlePasswordValid();
				users.erase(key);
			}
		}

		void onPasswordInvalid(const Swift::JID &user) {
			std::string key = user.toBare().toString();
			if (users.find(key) != users.end()) {
				users[key].session->handlePasswordInvalid();
				users.erase(key);
			}
		}

		const std::string &getUserPassword(const std::string &barejid) {
			return users[barejid].password;
		}

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
