/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "transport/UsersReconnecter.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/Config.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Network/NetworkFactories.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(userReconnecterLogger, "UserReconnecter");

UsersReconnecter::UsersReconnecter(Component *component, StorageBackend *storageBackend) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_started = false;

	m_nextUserTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1000);
	m_nextUserTimer->onTick.connect(boost::bind(&UsersReconnecter::reconnectNextUser, this));

	m_component->onConnected.connect(boost::bind(&UsersReconnecter::handleConnected, this));
}

UsersReconnecter::~UsersReconnecter() {
	m_component->onConnected.disconnect(boost::bind(&UsersReconnecter::handleConnected, this));
	m_nextUserTimer->stop();
	m_nextUserTimer->onTick.disconnect(boost::bind(&UsersReconnecter::reconnectNextUser, this));
}

void UsersReconnecter::reconnectNextUser() {
	if (m_users.empty()) {
		LOG4CXX_INFO(userReconnecterLogger, "All users reconnected, stopping UserReconnecter.");
		return;
	}

	std::string user = m_users.back();
	m_users.pop_back();

	m_component->getFrontend()->reconnectUser(user);
	m_nextUserTimer->start();
}

void UsersReconnecter::handleConnected() {
	if (m_started)
		return;

	LOG4CXX_INFO(userReconnecterLogger, "Starting UserReconnecter.");
	m_started = true;

	if (CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "service.reconnect_all_users", false)) {
		m_storageBackend->getUsers(m_users);
	}
	else {
		m_storageBackend->getOnlineUsers(m_users);
	}

	reconnectNextUser();
}


}
