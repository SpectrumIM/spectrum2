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
#include <boost/algorithm/string/predicate.hpp>
#include "transport/storagebackend.h"
#include "transport/factory.h"
#include "transport/userregistry.h"
#include "transport/logging.h"
#include "discoinforesponder.h"
#include "discoitemsresponder.h"
#include "storageparser.h"
#include "Swiften/TLS/OpenSSL/OpenSSLServerContext.h"
#include "Swiften/TLS/PKCS12Certificate.h"
#include "Swiften/TLS/OpenSSL/OpenSSLServerContextFactory.h"
#include "Swiften/Parser/PayloadParsers/AttentionParser.h"
#include "Swiften/Serializer/PayloadSerializers/AttentionSerializer.h"
#include "Swiften/Parser/PayloadParsers/XHTMLIMParser.h"
#include "Swiften/Serializer/PayloadSerializers/XHTMLIMSerializer.h"
#include "Swiften/Parser/PayloadParsers/StatsParser.h"
#include "Swiften/Serializer/PayloadSerializers/StatsSerializer.h"
#include "Swiften/Parser/PayloadParsers/GatewayPayloadParser.h"
#include "Swiften/Serializer/PayloadSerializers/GatewayPayloadSerializer.h"
#include "Swiften/Serializer/PayloadSerializers/SpectrumErrorSerializer.h"
#include "transport/BlockParser.h"
#include "transport/BlockSerializer.h"
#include "Swiften/Parser/PayloadParsers/InvisibleParser.h"
#include "Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h"
#include "Swiften/Swiften.h"

using namespace Swift;
using namespace boost;

namespace Transport {
	
DEFINE_LOGGER(logger, "Component");
DEFINE_LOGGER(logger_xml, "Component.XML");

Component::Component(Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Factory *factory, Transport::UserRegistry *userRegistry) {
	m_component = NULL;
	m_userRegistry = NULL;
	m_server = NULL;
	m_reconnectCount = 0;
	m_config = config;
	m_factory = factory;
	m_loop = loop;
	m_userRegistry = userRegistry;

	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));

	m_factories = factories;

	m_reconnectTimer = m_factories->getTimerFactory()->createTimer(3000);
	m_reconnectTimer->onTick.connect(bind(&Component::start, this)); 

	if (CONFIG_BOOL(m_config, "service.server_mode")) {
		LOG4CXX_INFO(logger, "Creating component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server = new Swift::Server(loop, m_factories, m_userRegistry, m_jid, CONFIG_INT(m_config, "service.port"));
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
			LOG4CXX_INFO(logger, "Using PKCS#12 certificate " << CONFIG_STRING(m_config, "service.cert"));
			LOG4CXX_INFO(logger, "SSLv23_server_method used.");
			TLSServerContextFactory *f = new OpenSSLServerContextFactory();
			m_server->addTLSEncryption(f, PKCS12Certificate(CONFIG_STRING(m_config, "service.cert"), createSafeByteArray(CONFIG_STRING(m_config, "service.cert_password"))));
		}
		else {
			LOG4CXX_WARN(logger, "No PKCS#12 certificate used. TLS is disabled.");
		}
// 		m_server->start();
		m_stanzaChannel = m_server->getStanzaChannel();
		m_iqRouter = m_server->getIQRouter();

		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<StorageParser>("private", "jabber:iq:private"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::AttentionParser>("attention", "urn:xmpp:attention:0"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::XHTMLIMParser>("html", "http://jabber.org/protocol/xhtml-im"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Transport::BlockParser>("block", "urn:xmpp:block:0"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::InvisibleParser>("invisible", "urn:xmpp:invisible:0"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::StatsParser>("query", "http://jabber.org/protocol/stats"));
		m_server->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::GatewayPayloadParser>("query", "jabber:iq:gateway"));

		m_server->addPayloadSerializer(new Swift::AttentionSerializer());
		m_server->addPayloadSerializer(new Swift::XHTMLIMSerializer());
		m_server->addPayloadSerializer(new Transport::BlockSerializer());
		m_server->addPayloadSerializer(new Swift::InvisibleSerializer());
		m_server->addPayloadSerializer(new Swift::StatsSerializer());
		m_server->addPayloadSerializer(new Swift::SpectrumErrorSerializer());
		m_server->addPayloadSerializer(new Swift::GatewayPayloadSerializer());

		m_server->onDataRead.connect(boost::bind(&Component::handleDataRead, this, _1));
		m_server->onDataWritten.connect(boost::bind(&Component::handleDataWritten, this, _1));
	}
	else {
		LOG4CXX_INFO(logger, "Creating component in gateway mode");
		m_component = new Swift::Component(loop, m_factories, m_jid, CONFIG_STRING(m_config, "service.password"));
		m_component->setSoftwareVersion("", "");
		m_component->onConnected.connect(bind(&Component::handleConnected, this));
		m_component->onError.connect(boost::bind(&Component::handleConnectionError, this, _1));
		m_component->onDataRead.connect(boost::bind(&Component::handleDataRead, this, _1));
		m_component->onDataWritten.connect(boost::bind(&Component::handleDataWritten, this, _1));

		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<StorageParser>("private", "jabber:iq:private"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::AttentionParser>("attention", "urn:xmpp:attention:0"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::XHTMLIMParser>("html", "http://jabber.org/protocol/xhtml-im"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Transport::BlockParser>("block", "urn:xmpp:block:0"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::InvisibleParser>("invisible", "urn:xmpp:invisible:0"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::StatsParser>("query", "http://jabber.org/protocol/stats"));
		m_component->addPayloadParserFactory(new GenericPayloadParserFactory<Swift::GatewayPayloadParser>("query", "jabber:iq:gateway"));

		m_component->addPayloadSerializer(new Swift::AttentionSerializer());
		m_component->addPayloadSerializer(new Swift::XHTMLIMSerializer());
		m_component->addPayloadSerializer(new Transport::BlockSerializer());
		m_component->addPayloadSerializer(new Swift::InvisibleSerializer());
		m_component->addPayloadSerializer(new Swift::StatsSerializer());
		m_component->addPayloadSerializer(new Swift::SpectrumErrorSerializer());
		m_component->addPayloadSerializer(new Swift::GatewayPayloadSerializer());

		m_stanzaChannel = m_component->getStanzaChannel();
		m_iqRouter = m_component->getIQRouter();
	}

	m_capsMemoryStorage = new CapsMemoryStorage();
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_stanzaChannel, m_iqRouter);
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_stanzaChannel);
 	m_entityCapsManager->onCapsChanged.connect(boost::bind(&Component::handleCapsChanged, this, _1));
	
	m_presenceOracle = new Transport::PresenceOracle(m_stanzaChannel);
	m_presenceOracle->onPresenceChange.connect(bind(&Component::handlePresence, this, _1));

	m_discoInfoResponder = new DiscoInfoResponder(m_iqRouter, m_config);
	m_discoInfoResponder->start();

	m_discoItemsResponder = new DiscoItemsResponder(m_iqRouter);
	m_discoItemsResponder->start();

// 
// 	m_registerHandler = new SpectrumRegisterHandler(m_component);
// 	m_registerHandler->start();
}

Component::~Component() {
	delete m_presenceOracle;
	delete m_entityCapsManager;
	delete m_capsManager;
	delete m_capsMemoryStorage;
	delete m_discoInfoResponder;
	delete m_discoItemsResponder;
	if (m_component)
		delete m_component;
	if (m_server) {
		m_server->stop();
		delete m_server;
	}
}

Swift::StanzaChannel *Component::getStanzaChannel() {
	return m_stanzaChannel;
}

Transport::PresenceOracle *Component::getPresenceOracle() {
	return m_presenceOracle;
}

void Component::setTransportFeatures(std::list<std::string> &features) {
	m_discoInfoResponder->setTransportFeatures(features);
}

Swift::CapsInfo &Component::getBuddyCapsInfo() {
		return m_discoInfoResponder->getBuddyCapsInfo();
}

void Component::setBuddyFeatures(std::list<std::string> &features) {
	// TODO: handle caps change
	m_discoInfoResponder->setBuddyFeatures(features);
}

void Component::start() {
	if (m_component && !m_component->isAvailable()) {
		LOG4CXX_INFO(logger, "Connecting XMPP server " << CONFIG_STRING(m_config, "service.server") << " port " << CONFIG_INT(m_config, "service.port"));
		m_reconnectCount++;
		m_component->connect(CONFIG_STRING(m_config, "service.server"), CONFIG_INT(m_config, "service.port"));
		m_reconnectTimer->stop();
	}
	else if (m_server) {
		LOG4CXX_INFO(logger, "Starting component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server->start();
		// We're connected right here, because we're in server mode...
		handleConnected();
	}
}

void Component::stop() {
	if (m_component) {
		m_reconnectCount = 0;
		// TODO: Call this once swiften will fix assert(!session_);
// 		m_component->disconnect();
		m_reconnectTimer->stop();
	}
	else if (m_server) {
		LOG4CXX_INFO(logger, "Stopping component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server->stop();
	}
}

void Component::handleConnected() {
	onConnected();
	m_reconnectCount = 0;
}

void Component::handleConnectionError(const ComponentError &error) {
	onConnectionError(error);
// 	if (m_reconnectCount == 2)
// 		Component::instance()->userManager()->removeAllUsers();
	std::string str = "Unknown error";
	switch (error.getType()) {
		case ComponentError::UnknownError: str = "Unknown error"; break;
		case ComponentError::ConnectionError: str = "Connection error"; break;
		case ComponentError::ConnectionReadError: str = "Connection read error"; break;
		case ComponentError::ConnectionWriteError: str = "Connection write error"; break;
		case ComponentError::XMLError: str = "XML Error"; break;
		case ComponentError::AuthenticationFailedError: str = "Authentication failed error"; break;
		case ComponentError::UnexpectedElementError: str = "Unexpected element error"; break;
	}
	LOG4CXX_INFO(logger, "Disconnected from XMPP server. Error: " << str);

	m_reconnectTimer->start();
}

void Component::handleDataRead(const Swift::SafeByteArray &data) {
	std::string d = safeByteArrayToString(data);
	if (!boost::starts_with(d, "<auth")) {
		LOG4CXX_INFO(logger_xml, "XML IN " << d);
	}
}

void Component::handleDataWritten(const Swift::SafeByteArray &data) {
	LOG4CXX_INFO(logger_xml, "XML OUT " << safeByteArrayToString(data));
}

void Component::handlePresence(Swift::Presence::ref presence) {
	bool isMUC = presence->getPayload<MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	// filter out login/logout presence spam
	if (!presence->getTo().getNode().empty() && isMUC == false)
		return;

	// filter out bad presences
	if (!presence->getFrom().isValid()) {
		return;
	}

	// check if we have this client's capabilities and ask for them
	if (presence->getType() != Swift::Presence::Unavailable) {
		boost::shared_ptr<CapsInfo> capsInfo = presence->getPayload<CapsInfo>();
		if (capsInfo && capsInfo->getHash() == "sha-1") {
			/*haveFeatures = */m_entityCapsManager->getCaps(presence->getFrom()) != DiscoInfo::ref();
		}
#ifdef SUPPORT_LEGACY_CAPS
		else {
			GetDiscoInfoRequest::ref discoInfoRequest = GetDiscoInfoRequest::create(presence->getFrom(), m_iqRouter);
			discoInfoRequest->onResponse.connect(boost::bind(&Component::handleDiscoInfoResponse, this, _1, _2, presence->getFrom()));
			discoInfoRequest->send();
		}
#endif
	}

	onUserPresenceReceived(presence);
}

void Component::handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid) {
#ifdef SUPPORT_LEGACY_CAPS
	onUserDiscoInfoReceived(jid, info);
#endif
}

void Component::handleCapsChanged(const Swift::JID& jid) {
	onUserDiscoInfoReceived(jid, m_entityCapsManager->getCaps(jid));
}

}
