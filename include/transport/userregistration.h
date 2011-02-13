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

class UserRegistration : Swift::GetResponder<Swift::InBandRegistrationPayload>, Swift::SetResponder<Swift::InBandRegistrationPayload> {
	public:
		UserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend);
		~UserRegistration();

		// Registers new user, returns false if user was already registered.
		bool registerUser(const UserInfo &user);

		// Unregisters user, returns true if user was successfully unregistered.
		bool unregisterUser(const std::string &barejid);

		boost::signal<void (const UserInfo &user)> onUserRegistered;
		boost::signal<void (const UserInfo &user)> onUserUnregistered;
		boost::signal<void (const UserInfo &user)> onUserUpdated;

	private:
		bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserManager *m_userManager;

};

}
