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

#include "transport/transport.h"
#include <boost/bind.hpp>

using namespace Swift;
using namespace boost;

namespace Transport {

Transport::Transport(Swift::EventLoop *loop, Config::Variables &config) {
	m_reconnectCount = 0;
	m_config = config;

	for (Config::Variables::iterator i = config.begin() ; i != config.end() ; ++i )
	{
		std::cout << (*i).first << "\n";
	} 

	m_jid = Swift::JID(m_config["service.jid"].as<std::string>());

	m_factories = new BoostNetworkFactories(loop);

	m_reconnectTimer = m_factories->getTimerFactory()->createTimer(1000);
	m_reconnectTimer->onTick.connect(bind(&Transport::connect, this)); 

	m_component = new Component(loop, m_factories, m_jid, m_config["service.password"].as<std::string>());
	m_component->setSoftwareVersion("", "");
	m_component->onConnected.connect(bind(&Transport::handleConnected, this));
	m_component->onError.connect(bind(&Transport::handleConnectionError, this, _1));
// 	m_component->onDataRead.connect(bind(&Transport::handleDataRead, this, _1));
// 	m_component->onDataWritten.connect(bind(&Transport::handleDataWritten, this, _1));
// 	m_component->onPresenceReceived.connect(bind(&Transport::handlePresenceReceived, this, _1));
// 	m_component->onMessageReceived.connect(bind(&Transport::handleMessageReceived, this, _1));

	m_capsMemoryStorage = new CapsMemoryStorage();
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_component->getStanzaChannel(), m_component->getIQRouter());
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_component->getStanzaChannel());
// 	m_entityCapsManager->onCapsChanged.connect(boost::bind(&Transport::handleCapsChanged, this, _1));
	
	m_presenceOracle = new PresenceOracle(m_component->getStanzaChannel());
// 	m_presenceOracle->onPresenceChange.connect(bind(&Transport::handlePresence, this, _1));

// 	m_discoInfoResponder = new SpectrumDiscoInfoResponder(m_component->getIQRouter());
// 	m_discoInfoResponder->start();
// 
// 	m_registerHandler = new SpectrumRegisterHandler(m_component);
// 	m_registerHandler->start();
}

Transport::~Transport() {
	delete m_presenceOracle;
	delete m_entityCapsManager;
	delete m_capsManager;
	delete m_capsMemoryStorage;
// 	delete m_discoInfoResponder;
// 	delete m_registerHandler;
	delete m_component;
	delete m_factories;
}

void Transport::connect() {
	m_reconnectCount++;
	m_component->connect(m_config["service.server"].as<std::string>(), m_config["service.port"].as<int>());
	m_reconnectTimer->stop();
}

void Transport::handleConnected() {
	std::cout <<"Transport" << " CONNECTED!\n";
	m_reconnectCount = 0;
}

void Transport::handleConnectionError(const ComponentError &error) {
	std::cout << "Transport" << " Disconnected from Jabber server!\n";

// 	if (m_reconnectCount == 2)
// 		Transport::instance()->userManager()->removeAllUsers();

	m_reconnectTimer->start();
}

}
