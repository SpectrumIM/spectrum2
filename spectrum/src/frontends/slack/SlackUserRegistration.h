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

#include "transport/UserRegistration.h"

namespace Transport {

struct UserInfo;
class Component;
class StorageBackend;
class UserManager;
class Config;
class OAuth2;

class SlackUserRegistration : public UserRegistration {
	public:
		SlackUserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend);

		~SlackUserRegistration();

		std::string createOAuth2URL(const std::vector<std::string> &args);

		std::string getTeamDomain(const std::string &token);

		std::string handleOAuth2Code(const std::string &code, const std::string &state);

		virtual bool doUserRegistration(const UserInfo &userInfo);

		virtual bool doUserUnregistration(const UserInfo &userInfo);

	private:
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserManager *m_userManager;
		Config *m_config;
		std::map<std::string, OAuth2 *> m_auths;
		std::map<std::string, std::vector<std::string> > m_authsData;

};

}
