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

#include "transport/UserRegistration.h"
#include "transport/UserManager.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp> 

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(userRegistrationLogger, "UserRegistration");

UserRegistration::UserRegistration(Component *component, UserManager *userManager,
								   StorageBackend *storageBackend) {
	m_component = component;
	m_config = m_component->m_config;
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

UserRegistration::~UserRegistration(){
}

bool UserRegistration::registerUser(const UserInfo &row, bool allowPasswordChange) {
	UserInfo dummy;
	bool registered = m_storageBackend->getUser(row.jid, dummy);

	if (!registered || (registered && allowPasswordChange)) {
		m_storageBackend->setUser(row);
		doUserRegistration(row);
		onUserRegistered(row);
	}

	return !registered;
}

bool UserRegistration::unregisterUser(const std::string &barejid) {
	UserInfo userInfo;
	bool registered = m_storageBackend->getUser(barejid, userInfo);

	// This user is not registered, nothing to do
	if (!registered) {
		LOG4CXX_WARN(userRegistrationLogger, "Tried to unregister already unregistered user " << barejid);
		return false;
	}

	LOG4CXX_INFO(userRegistrationLogger, "Unregistering user " << barejid);
	// Remove user from database
	m_storageBackend->removeUser(userInfo.id);

	// Disconnect the user
	User *user = m_userManager->getUser(barejid);
	if (user) {
		LOG4CXX_INFO(userRegistrationLogger, "Disconnecting user " << barejid);
		m_userManager->removeUser(user);
	}

	doUserUnregistration(userInfo);
	onUserUnregistered(userInfo);

	return true;
}

}
