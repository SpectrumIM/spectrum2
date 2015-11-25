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
#include "SlackInstallation.h"

#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/Logging.h"

namespace Transport {

DEFINE_LOGGER(logger, "SlackUserManager");

SlackUserManager::SlackUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) : UserManager(component, userRegistry, storageBackend) {
	m_component = component;
	m_storageBackend = storageBackend;
    m_userRegistration = new SlackUserRegistration(component, this, storageBackend);
}

SlackUserManager::~SlackUserManager() {
    delete m_userRegistration;
}

void SlackUserManager::reconnectUser(const std::string &user) {
	UserInfo uinfo;
	if (!m_storageBackend->getUser(user, uinfo)) {
		LOG4CXX_ERROR(logger, "User " << user << " tried to reconnect, but he's not registered.");
		return;
	}

// 	if (!uinfo.uin.empty()) {
// 		LOG4CXX_INFO(logger, "Reconnecting user " << user);
// 		Swift::Presence::ref response = Swift::Presence::create();
// 		response->setTo(m_component->getJID());
// 		response->setFrom(user + "@" + m_component->getJID().toString());
// 		response->setType(Swift::Presence::Available);
// 	}
// 	else {
		LOG4CXX_INFO(logger, "Cannot reconnect user " << user << ","
			"because he does not have legacy network configured. "
			"Continuing in Installation mode for this user until "
			"he configures the legacy network.");
		m_installations[user] = new SlackInstallation(m_component, m_storageBackend, uinfo);
		m_installations[user]->onInstallationDone.connect(boost::bind(&SlackUserManager::reconnectUser, this, _1));
// 	}
}

void SlackUserManager::sendVCard(unsigned int id, Swift::VCard::ref vcard) {

}

void SlackUserManager::sendMessage(boost::shared_ptr<Swift::Message> message) {
	LOG4CXX_INFO(logger, message->getTo().toBare().toString());
	m_installations[message->getTo().toBare().toString()]->sendMessage(message);
}


UserRegistration *SlackUserManager::getUserRegistration() {
	return m_userRegistration;
}

std::string SlackUserManager::handleOAuth2Code(const std::string &code, const std::string &state) {
	return static_cast<SlackUserRegistration *>(m_userRegistration)->handleOAuth2Code(code, state);
}

std::string SlackUserManager::getOAuth2URL(const std::vector<std::string> &args) {
	return static_cast<SlackUserRegistration *>(m_userRegistration)->createOAuth2URL(args);
}


}
