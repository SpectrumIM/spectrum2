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

#include "SlackUserRegistration.h"
#include "SlackRosterManager.h"
#include "SlackFrontend.h"

#include "transport/UserManager.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Buddy.h"
#include "transport/Config.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp> 

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(logger, "SlackUserRegistration");

SlackUserRegistration::SlackUserRegistration(Component *component, UserManager *userManager,
								   StorageBackend *storageBackend)
: UserRegistration(component, userManager, storageBackend) {
	m_component = component;
	m_config = m_component->getConfig();
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

SlackUserRegistration::~SlackUserRegistration(){
}

bool SlackUserRegistration::doUserRegistration(const UserInfo &row) {
	return true;
}

bool SlackUserRegistration::doUserUnregistration(const UserInfo &row) {
	return true;
}



}
