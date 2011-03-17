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

#include "Swiften/Swiften.h"
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Queries/SetResponder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"

namespace Transport {

struct UserInfo;
class Component;
class StorageBackend;
class UserManager;
class Config;

/// Allows users to register the transport using service discovery.
class UserRegistration : Swift::GetResponder<Swift::InBandRegistrationPayload>, Swift::SetResponder<Swift::InBandRegistrationPayload> {
	public:
		/// Creates new UserRegistration handler.
		/// \param component Component associated with this class
		/// \param userManager UserManager associated with this class
		/// \param storageBackend StorageBackend where the registered users will be stored
		UserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend);

		/// Destroys UserRegistration.
		~UserRegistration();

		/// Registers new user. This function stores user into database and subscribe user to transport.
		/// \param userInfo UserInfo struct with informations about registered user
		/// \return false if user is already registered
		bool registerUser(const UserInfo &userInfo);

		/// Unregisters user. This function removes all data about user from databa, unsubscribe all buddies
		/// managed by this transport and disconnects user if he's connected.
		/// \param barejid bare JID of user to unregister
		/// \return false if there is no such user registered
		bool unregisterUser(const std::string &barejid);

		/// Called when new user has been registered.
		/// \param userInfo UserInfo struct with informations about user
		boost::signal<void (const UserInfo &userInfo)> onUserRegistered;

		/// Called when user has been unregistered.
		/// \param userInfo UserInfo struct with informations about user
		boost::signal<void (const UserInfo &userInfo)> onUserUnregistered;

		/// Called when user's registration has been updated.
		/// \param userInfo UserInfo struct with informations about user
		boost::signal<void (const UserInfo &userInfo)> onUserUpdated;

	private:
		bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserManager *m_userManager;
		Config *m_config;

};

}
