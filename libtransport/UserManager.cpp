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

#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/RosterManager.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/Config.h"

#ifndef __FreeBSD__
#ifndef __MACH__
#include "malloc.h"
#endif
#endif
// #include "valgrind/memcheck.h"

namespace Transport {

DEFINE_LOGGER(userManagerLogger, "UserManager");

UserManager::UserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	m_cachedUser = NULL;
	m_sentToXMPP = 0;
	m_sentToBackend = 0;
	m_component = component;
	m_storageBackend = storageBackend;
	m_userRegistry = userRegistry;
}

UserManager::~UserManager() {

}

void UserManager::addUser(User *user) {
	m_users[user->getUserName()] = user;
	if (m_storageBackend) {
		m_storageBackend->setUserOnline(user->getUserInfo().id, true);
	}
	onUserCreated(user);
}

User *UserManager::getUser(const std::string &user){
	if (m_cachedUser && user == m_cachedUser->getUserName()) {
		return m_cachedUser;
	}

	if (m_users.find(user) != m_users.end()) {
		User *u = m_users[user];
		m_cachedUser = u;
		if (m_storageBackend) {
			m_storageBackend->getAllSettings(u->getUserInfo().id, u->getUserInfo().settings);
		}
		return u;
	}
	return NULL;
}

void UserManager::removeUser(User *user, bool onUserBehalf) {
	m_users.erase(user->getUserName());
	if (m_cachedUser == user)
		m_cachedUser = NULL;

	doRemoveUser(user);

	if (m_storageBackend && onUserBehalf) {
		m_storageBackend->setUserOnline(user->getUserInfo().id, false);
	}

	LOG4CXX_INFO(userManagerLogger, user->getUserName() << ": Disconnecting user");
	onUserDestroyed(user);
	delete user;
#ifndef WIN32
#ifndef __FreeBSD__
#ifndef __MACH__
#if defined (__GLIBC__)
	malloc_trim(0);
#endif
#endif
#endif
#endif
// 	VALGRIND_DO_LEAK_CHECK;
}

void UserManager::removeAllUsers(bool onUserBehalf) {
	while (m_users.begin() != m_users.end()) {
		removeUser((*m_users.begin()).second, onUserBehalf);
	}
}

int UserManager::getUserCount() {
	return m_users.size();
}

bool UserManager::autoRegisterUser(UserInfo &res) {
	if (!m_storageBackend) {
		return true;
	}

	// store user and getUser again to get user ID.
	m_storageBackend->setUser(res);
	return m_storageBackend->getUser(res.jid, res);
}

User *UserManager::loginUser(const std::string &user, UserInfo &res, LoginError &error) {
	error = LoginOK;

	if (CONFIG_BOOL(m_component->getConfig(), "service.vip_only") && res.vip == false) {
		LOG4CXX_WARN(userManagerLogger, "Non VIP user " << user << " tried to login");
		error = LoginNotVIP;
		return NULL;
	}

	bool transport_enabled = true;
	if (m_storageBackend) {
		std::string value = "1";
		int type = (int) TYPE_BOOLEAN;
		m_storageBackend->getUserSetting(res.id, "enable_transport", type, value);
		transport_enabled = value == "1";
	}
	// User can disabled the transport using adhoc commands
	if (!transport_enabled) {
		LOG4CXX_INFO(userManagerLogger, "User " << user << " has disabled transport, not logging");
		error = LoginTransportDisabled;
		return NULL;
	}

	// Create new user class and set storagebackend
	User *u = m_component->getFrontend()->createUser(user, res, m_component, this);
	u->getRosterManager()->setStorageBackend(m_storageBackend);
	addUser(u);

	// User can be handleDisconnected in addUser callback, so refresh the pointer
	return getUser(user);
}

}
