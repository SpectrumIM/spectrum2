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
#include "SlackRosterManager.h"
#include "SlackUser.h"
#include "SlackUserManager.h"

#include "transport/StorageBackend.h"
#include "transport/Factory.h"
#include "transport/UserRegistry.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/Transport.h"
#include "transport/OAuth2.h"

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
}

void SlackFrontend::init(Component *transport, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Transport::UserRegistry *userRegistry) {
	m_transport = transport;
	m_config = transport->getConfig();
	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));

	std::string redirect_url = "http://spectrum.im/slackoauth2/" + CONFIG_STRING(m_config, "service.jid");
	m_oauth2 = new OAuth2(CONFIG_STRING_DEFAULTED(m_config, "service.client_id",""),
						  CONFIG_STRING_DEFAULTED(m_config, "service.client_secret",""),
						  "https://slack.com/oauth/authorize",
						  "https://slack.com/api/oauth.access",
						  redirect_url,
						  "channels:read channels:write team:read");
}

SlackFrontend::~SlackFrontend() {
	delete m_oauth2;
}

void SlackFrontend::clearRoomList() {

}

void SlackFrontend::addRoomToRoomList(const std::string &handle, const std::string &name) {

}

void SlackFrontend::sendPresence(Swift::Presence::ref presence) {

}

void SlackFrontend::sendVCard(Swift::VCard::ref vcard, Swift::JID to) {
}

void SlackFrontend::sendRosterRequest(Swift::RosterPayload::ref payload, Swift::JID to) {
}

void SlackFrontend::sendMessage(boost::shared_ptr<Swift::Message> message) {
}

void SlackFrontend::sendIQ(boost::shared_ptr<Swift::IQ> iq) {
}

boost::shared_ptr<Swift::DiscoInfo> SlackFrontend::sendCapabilitiesRequest(Swift::JID to) {

	return Swift::DiscoInfo::ref();
}

void SlackFrontend::reconnectUser(const std::string &user) {

}

RosterManager *SlackFrontend::createRosterManager(User *user, Component *component) {
	return new SlackRosterManager(user, component);
}

User *SlackFrontend::createUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) {
	return new SlackUser(jid, userInfo, component, userManager);
}

UserManager *SlackFrontend::createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	return new SlackUserManager(component, userRegistry, storageBackend);
}


void SlackFrontend::connectToServer() {
	LOG4CXX_INFO(logger, "Connecting to Slack API server");

	std::string url = m_oauth2->generateAuthURL();
	LOG4CXX_INFO(logger, url);
}

std::string SlackFrontend::setOAuth2Code(const std::string &code, const std::string &state) {
	LOG4CXX_INFO(logger, "Using OAuth2 code " << code << " to get the authorization token");
	return m_oauth2->handleOAuth2Code(code, state);
}

void SlackFrontend::disconnectFromServer() {

}

}
