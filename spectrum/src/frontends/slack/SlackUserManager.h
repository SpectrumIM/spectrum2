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

#include "transport/UserManager.h"

#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/DiscoInfo.h"

#include <string>
#include <map>

namespace Transport {

class Component;
class StorageBackend;
class StorageResponder;
class VCardResponder;
class XMPPUserRegistration;
class GatewayResponder;
class AdHocManager;
class SettingsAdHocCommandFactory;
class SlackInstallation;

class SlackUserManager : public UserManager {
	public:
		SlackUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);

		virtual ~SlackUserManager();

		void reconnectUser(const std::string &user);

		virtual void sendVCard(unsigned int id, Swift::VCard::ref vcard);

		UserRegistration *getUserRegistration();

		std::string handleOAuth2Code(const std::string &code, const std::string &state);

		std::string getOAuth2URL(const std::vector<std::string> &args);

	private:
		Component *m_component;
		UserRegistration *m_userRegistration;
		StorageBackend *m_storageBackend;
		std::map<std::string, SlackInstallation *> m_installations;
};

}
