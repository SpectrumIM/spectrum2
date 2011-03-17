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

namespace Transport {

struct UserInfo;
class User;
class UserManager;
class Component;
class StorageBackend;
class UserRegistration;

/// Basic logging class which logs various data into std::out (standard output).
class Logger
{
	public:
		/// Creates new Logger class instance.
		/// \param component component instance
		Logger(Component *component);

		/// Logger destructor.
		~Logger();

		/// Starts logging data related to StorageBackend class.
		/// \param storage storage class
		void setStorageBackend(StorageBackend *storage);

		/// Starts logging data related to UserRegistration class.
		/// \param userRegistration userRegistration class
		void setUserRegistration(UserRegistration *userRegistration);

		/// Starts logging data related to UserManager class.
		/// \param userManager userManager class
		void setUserManager(UserManager *userManager);

	private:
		// Component
		void handleConnected();
		void handleConnectionError(const Swift::ComponentError &error);
		void handleXMLIn(const std::string &data);
		void handleXMLOut(const std::string &data);

		// StorageBackend
		void handleStorageError(const std::string &statement, const std::string &error);

		// UserRegistration
		void handleUserRegistered(const UserInfo &user);
		void handleUserUnregistered(const UserInfo &user);
		void handleUserUpdated(const UserInfo &user);

		// UserManager
		void handleUserCreated(User *user);
		void handleUserDestroyed(User *user);
};

}
