 /**
 * Spectrum 2 Slack Frontend
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
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

#include "SlackFrontend.h"
#include "SlackConversation.h"
#include "SlackRosterManager.h"
#include "SlackUser.h"
#include "SlackUserManager.h"

#include "transport/StorageBackend.h"
#include "transport/Factory.h"
#include "transport/UserRegistry.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/Transport.h"
#include "transport/ThreadPool.h"
#include "transport/HTTPRequest.h"

#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>

#if defined(_WIN32) && !defined(__cplusplus_winrt)
// Extra includes for Windows desktop.
#include <windows.h>
#include <Shellapi.h>
#endif

namespace Transport {


DEFINE_LOGGER(logger, "SlackFrontend");

SlackFrontend::SlackFrontend() {
	m_userManager = NULL;
	HTTPRequest::globalInit();
}

void SlackFrontend::init(Component *transport, Config *config, Transport::UserRegistry *userRegistry) {
	m_transport = transport;
	m_config = transport->getConfig();
	m_tp = new ThreadPool(transport->getEventLoop(), 10);
}

SlackFrontend::~SlackFrontend() {
	HTTPRequest::globalCleanup();
}

void SlackFrontend::clearRoomList() {

}

void SlackFrontend::addRoomToRoomList(const std::string &room, const std::string &name) {

}

void SlackFrontend::reconnectUser(const std::string &user) {
	return m_userManager->reconnectUser(user);
}

RosterManager *SlackFrontend::createRosterManager(User *user, Component *component) {
	return new SlackRosterManager(user, component);
}

User *SlackFrontend::createUser(const std::string &userName, UserInfo &userInfo, Component *component, UserManager *userManager) {
	SlackUser *user = new SlackUser(userName, userInfo, component, userManager);
	user->setReconnectLimit(-1);
	return user;
}

UserManager *SlackFrontend::createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	m_userManager = new SlackUserManager(component, userRegistry, storageBackend);
	return m_userManager;
}

Conversation *SlackFrontend::createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) {
	return new SlackConversation(conversationManager, legacyName, isMUC);
}

void SlackFrontend::connectToServer() {
	LOG4CXX_INFO(logger, "Started.");
	m_transport->handleConnected();
}

std::string SlackFrontend::setOAuth2Code(const std::string &code, const std::string &state) {
	return m_userManager->handleOAuth2Code(code, state);
}

std::string SlackFrontend::getOAuth2URL(const std::vector<std::string> &args) {
	return m_userManager->getOAuth2URL(args);
}

std::string SlackFrontend::getRegistrationFields() {
	std::string fields = "Main Slack channel";
	if (CONFIG_BOOL_DEFAULTED(m_config, "registration.needRegistration", true)) {
		fields += "\n" + CONFIG_STRING(m_config, "registration.username_label") + "\n";
		fields += CONFIG_STRING(m_config, "registration.password_label");
	}
	return fields;
}

void SlackFrontend::disconnectFromServer() {

}

}
