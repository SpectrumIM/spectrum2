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

#include <string>
#include <map>
#include "Swiften/Swiften.h"
#include "Swiften/Server/UserRegistry.h"
#include "transport/userregistry.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("UserRegistry");

UserRegistry::UserRegistry(Config *cfg, Swift::NetworkFactories *factories) {
	config = cfg;
	m_removeTimer = factories->getTimerFactory()->createTimer(1);
	m_inRemoveLater = false;
}

UserRegistry::~UserRegistry() { m_removeTimer->stop(); }

void UserRegistry::isValidUserPassword(const Swift::JID& user, Swift::ServerFromClientSession *session, const Swift::SafeByteArray& password) {
	if (!CONFIG_STRING(config, "service.admin_username").empty() && user.getNode() == CONFIG_STRING(config, "service.admin_username")) {
		if (Swift::safeByteArrayToString(password) == CONFIG_STRING(config, "service.admin_password")) {
			session->handlePasswordValid();
		}
		else {
			session->handlePasswordInvalid();
		}
		return;
	}

	std::string key = user.toBare().toString();

	// Users try to connect twice
	if (users.find(key) != users.end()) {
		// Kill the first session
		LOG4CXX_INFO(logger, key << ": Removing previous session and making this one active");
		Swift::ServerFromClientSession *tmp = users[key].session;
		users[key].session = session;
		tmp->handlePasswordInvalid();
	}

	LOG4CXX_INFO(logger, key << ": Connecting this user to find if password is valid");

	users[key].password = Swift::safeByteArrayToString(password);
	users[key].session = session;
	onConnectUser(user);

	return;
}

void UserRegistry::stopLogin(const Swift::JID& user, Swift::ServerFromClientSession *session) {
	std::string key = user.toBare().toString();
	if (users.find(key) != users.end()) {
		if (users[key].session == session) {
			LOG4CXX_INFO(logger, key << ": Stopping login process (user probably disconnected while logging in)");
			users.erase(key);
		}
		else {
			LOG4CXX_WARN(logger, key << ": Stopping login process (user probably disconnected while logging in), but this is not active session");
		}
	}
	else {
		LOG4CXX_WARN(logger, key << ": Stopping login process (user probably disconnected while logging in) for invalid user");
	}

	if (!m_inRemoveLater)
		onDisconnectUser(user);
}

void UserRegistry::onPasswordValid(const Swift::JID &user) {
	std::string key = user.toBare().toString();
	if (users.find(key) != users.end()) {
		LOG4CXX_INFO(logger, key << ": Password is valid");
		users[key].session->handlePasswordValid();
		users.erase(key);
	}
	else {
		LOG4CXX_INFO(logger, key << ": onPasswordValid called for invalid user");
	}
}

void UserRegistry::onPasswordInvalid(const Swift::JID &user) {
	std::string key = user.toBare().toString();
	if (users.find(key) != users.end()) {
		LOG4CXX_INFO(logger, key << ": Password is invalid");
		users[key].session->handlePasswordInvalid();
		users.erase(key);
	}
	else {
		LOG4CXX_INFO(logger, key << ": onPasswordInvalid called for invalid user");
	}
}

void UserRegistry::handleRemoveTimeout(const Swift::JID &user) {
	m_inRemoveLater = true;
	onPasswordInvalid(user);
	m_inRemoveLater = false;
}

void UserRegistry::removeLater(const Swift::JID &user) {
	m_removeTimer->onTick.connect(boost::bind(&UserRegistry::handleRemoveTimeout, this, user));
	m_removeTimer->start();
}

const std::string UserRegistry::getUserPassword(const std::string &barejid) {
	if (users.find(barejid) != users.end())
		return users[barejid].password;
	return "";
}

}
