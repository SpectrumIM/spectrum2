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


#include "transport/Transport.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/PresenceOracle.h"
#include "transport/Config.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace Swift;

namespace Transport {
	
DEFINE_LOGGER(logger, "Component");
DEFINE_LOGGER(logger_xml, "Component.RAW");

Component::Component(Frontend *frontend, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Factory *factory, Transport::UserRegistry *userRegistry) {
	m_frontend = frontend;
	m_userRegistry = NULL;
	m_reconnectCount = 0;
	m_config = config;
	m_factory = factory;
	m_loop = loop;
	m_userRegistry = userRegistry;
	m_rawXML = false;
	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));

	m_factories = factories;

	m_reconnectTimer = m_factories->getTimerFactory()->createTimer(3000);
	m_reconnectTimer->onTick.connect(boost::bind(&Component::start, this)); 

	m_presenceOracle = new Transport::PresenceOracle(frontend);
	m_presenceOracle->onPresenceChange.connect(boost::bind(&Component::handlePresence, this, _1));

	m_frontend->init(this, loop, factories, config, userRegistry);
}

Component::~Component() {
	delete m_presenceOracle;
}

Transport::PresenceOracle *Component::getPresenceOracle() {
	return m_presenceOracle;
}

bool Component::inServerMode() {
	 return CONFIG_BOOL_DEFAULTED(m_config, "service.server_mode", true);
}

void Component::start() {
	m_frontend->connectToServer();
	m_reconnectCount++;
	m_reconnectTimer->stop();
}

void Component::stop() {
	m_reconnectCount = 0;
	m_frontend->disconnectFromServer();
	m_reconnectTimer->stop();
}

void Component::handleConnected() {
	onConnected();
	m_reconnectCount = 0;
	m_reconnectTimer->stop();
	LOG4CXX_INFO(logger, "Connected to Frontend server.");
}


void Component::handleConnectionError(const std::string &error) {
	onConnectionError(error);
	LOG4CXX_INFO(logger, "Disconnected from Frontend server. Error: " << error);
	m_reconnectTimer->start();
}

void Component::handleDataRead(const std::string &data) {
	if (!boost::starts_with(data, "<auth")) {
		LOG4CXX_INFO(logger_xml, "RAW DATA IN " << data);
	}
}

void Component::handleDataWritten(const std::string &data) {
	LOG4CXX_INFO(logger_xml, "RAW DATA OUT " << data);
}

void Component::handlePresence(Swift::Presence::ref presence) {
	// filter out login/logout presence spam
	if (!presence->getTo().getNode().empty())
		return;

	// filter out bad presences
	if (!presence->getFrom().isValid()) {
		return;
	}

	switch (presence->getType()) {
		case Presence::Error:
		case Presence::Subscribe:
		case Presence::Subscribed:
		case Presence::Unsubscribe:
		case Presence::Unsubscribed:
			return;
		default:
			break;
	};

	// check if we have this client's capabilities and ask for them
	if (presence->getType() != Swift::Presence::Unavailable) {
		m_frontend->sendCapabilitiesRequest(presence->getFrom());
	}

	onUserPresenceReceived(presence);
}

}
