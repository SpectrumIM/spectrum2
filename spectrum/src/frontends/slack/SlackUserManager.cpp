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

#include "SlackUserManager.h"
#include "SlackUserRegistration.h"
#include "SlackFrontend.h"

#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/Logging.h"

namespace Transport {

DEFINE_LOGGER(logger, "SlackUserManager");

SlackUserManager::SlackUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) : UserManager(component, userRegistry, storageBackend) {
	m_component = component;
    m_userRegistration = new SlackUserRegistration(component, this, storageBackend);
}

SlackUserManager::~SlackUserManager() {
    delete m_userRegistration;
}

void SlackUserManager::sendVCard(unsigned int id, Swift::VCard::ref vcard) {

}


UserRegistration *SlackUserManager::getUserRegistration() {
	return m_userRegistration;
}

}
