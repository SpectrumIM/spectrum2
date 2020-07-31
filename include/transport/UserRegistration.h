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

#include <boost/signals2.hpp>

#include "Swiften/Queries/Responder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"
#include "Swiften/Elements/RosterPayload.h"

namespace Transport {

struct UserInfo;
class Component;
class StorageBackend;
class UserManager;
class Config;

/// Allows users to register the transport using service discovery.
class UserRegistration {
	public:
		/// Creates new UserRegistration handler.
		/// \param component Component associated with this class
		/// \param userManager UserManager associated with this class
		/// \param storageBackend StorageBackend where the registered users will be stored
		UserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend);

		/// Destroys UserRegistration.
		virtual ~UserRegistration();

		/// Registers new user. This function stores user into database and subscribe user to transport.
		/// \param userInfo UserInfo struct with informations about registered user
		/// \return false if user is already registered
		bool registerUser(const UserInfo &userInfo, bool allowPasswordChange = false);

		/// Unregisters user. This function removes all data about user from databa, unsubscribe all buddies
		/// managed by this transport and disconnects user if he's connected.
		/// \param barejid bare JID of user to unregister
		/// \return false if there is no such user registered
		bool unregisterUser(const std::string &barejid);

		/// Registers new user. This function stores user into database and subscribe user to transport.
		/// \param userInfo UserInfo struct with informations about registered user
		/// \return false if user is already registered
		virtual bool doUserRegistration(const UserInfo &userInfo) = 0;

		/// Unregisters user. This function removes all data about user from databa, unsubscribe all buddies
		/// managed by this transport and disconnects user if he's connected.
		/// \param barejid bare JID of user to unregister
		/// \return false if there is no such user registered
		virtual bool doUserUnregistration(const UserInfo &userInfo) = 0;

		/// Called when new user has been registered.
		/// \param userInfo UserInfo struct with informations about user
		boost::signals2::signal<void (const UserInfo &userInfo)> onUserRegistered;

		/// Called when user has been unregistered.
		/// \param userInfo UserInfo struct with informations about user
		boost::signals2::signal<void (const UserInfo &userInfo)> onUserUnregistered;

		/// Called when user's registration has been updated.
		/// \param userInfo UserInfo struct with informations about user
		boost::signals2::signal<void (const UserInfo &userInfo)> onUserUpdated;

	private:
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserManager *m_userManager;
		Config *m_config;

};

}
