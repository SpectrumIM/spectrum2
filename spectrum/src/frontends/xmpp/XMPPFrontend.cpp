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

#include "XMPPFrontend.h"
#include "XMPPRosterManager.h"
#include "XMPPUser.h"
#include "XMPPUserManager.h"
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "transport/StorageBackend.h"
#include "transport/Factory.h"
#include "transport/UserRegistry.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/Transport.h"
#include "storageparser.h"
#ifdef _WIN32
#include <Swiften/TLS/CAPICertificate.h>
#include "Swiften/TLS/Schannel/SchannelServerContext.h"
#include "Swiften/TLS/Schannel/SchannelServerContextFactory.h"
#elif defined(__APPLE__) && HAVE_SWIFTEN_3
#include <Swiften/TLS/SecureTransport/SecureTransportCertificate.h>
#include <Swiften/TLS/SecureTransport/SecureTransportServerContext.h>
#else
#include "Swiften/TLS/PKCS12Certificate.h"
#include "Swiften/TLS/CertificateWithKey.h"
#include "Swiften/TLS/OpenSSL/OpenSSLServerContext.h"
#include "Swiften/TLS/OpenSSL/OpenSSLServerContextFactory.h"
#endif
#include "Swiften/Parser/PayloadParsers/AttentionParser.h"
#include "Swiften/Serializer/PayloadSerializers/AttentionSerializer.h"
#include "Swiften/Parser/PayloadParsers/XHTMLIMParser.h"
#include "Swiften/Serializer/PayloadSerializers/XHTMLIMSerializer.h"
#include "Swiften/Parser/PayloadParsers/StatsParser.h"
#include "Swiften/Serializer/PayloadSerializers/StatsSerializer.h"
#include "Swiften/Parser/PayloadParsers/GatewayPayloadParser.h"
#include "Swiften/Serializer/PayloadSerializers/GatewayPayloadSerializer.h"
#include "Swiften/Serializer/PayloadSerializers/SpectrumErrorSerializer.h"
#include "Swiften/Parser/PayloadParsers/MUCPayloadParser.h"
#include "BlockParser.h"
#include "BlockSerializer.h"
#include "Swiften/Parser/PayloadParsers/InvisibleParser.h"
#include "Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h"
#include "Swiften/Parser/PayloadParsers/HintPayloadParser.h"
#include "Swiften/Serializer/PayloadSerializers/HintPayloadSerializer.h"
#include "Swiften/Parser/PayloadParsers/PrivilegeParser.h"
#include "Swiften/Serializer/PayloadSerializers/PrivilegeSerializer.h"
#include "Swiften/Parser/GenericPayloadParserFactory.h"
#include "Swiften/Parser/GenericPayloadParserFactory2.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/RosterPayload.h"
#include "discoitemsresponder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"

using namespace Swift;

namespace Transport {
	
DEFINE_LOGGER(logger, "XMPPFrontend");

XMPPFrontend::XMPPFrontend() {
}

class SwiftServerExposed: public Swift::Server
{
public:
	PayloadParserFactoryCollection* getPayloadParserFactories() { return Swift::Server::getPayloadParserFactories(); }
	PayloadSerializerCollection* getPayloadSerializers() { return Swift::Server::getPayloadSerializers(); }
};

class SwiftComponentExposed: public Swift::Component
{
public:
	PayloadParserFactoryCollection* getPayloadParserFactories() { return Swift::Component::getPayloadParserFactories(); }
	PayloadSerializerCollection* getPayloadSerializers() { return Swift::Component::getPayloadSerializers(); }
};

void XMPPFrontend::init(Component *transport, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Transport::UserRegistry *userRegistry) {
	m_transport = transport;
	m_component = NULL;
	m_server = NULL;
	m_rawXML = false;
	m_config = transport->getConfig();
	m_userManager = NULL;
	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));

	m_config->onBackendConfigUpdated.connect(boost::bind(&XMPPFrontend::handleBackendConfigChanged, this));

	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<StorageParser>("private", "jabber:iq:private"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::AttentionParser>("attention", "urn:xmpp:attention:0"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::XHTMLIMParser>("html", "http://jabber.org/protocol/xhtml-im"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Transport::BlockParser>("block", "urn:xmpp:block:0"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::InvisibleParser>("invisible", "urn:xmpp:invisible:0"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::StatsParser>("query", "http://jabber.org/protocol/stats"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::GatewayPayloadParser>("query", "jabber:iq:gateway"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::MUCPayloadParser>("x", "http://jabber.org/protocol/muc"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::HintPayloadParser>("no-permanent-store", "urn:xmpp:hints"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::HintPayloadParser>("no-store", "urn:xmpp:hints"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::HintPayloadParser>("no-copy", "urn:xmpp:hints"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::HintPayloadParser>("store", "urn:xmpp:hints"));

	m_payloadSerializers.push_back(new Swift::AttentionSerializer());
	m_payloadSerializers.push_back(new Swift::XHTMLIMSerializer());
	m_payloadSerializers.push_back(new Transport::BlockSerializer());
	m_payloadSerializers.push_back(new Swift::InvisibleSerializer());
	m_payloadSerializers.push_back(new Swift::StatsSerializer());
	m_payloadSerializers.push_back(new Swift::SpectrumErrorSerializer());
	m_payloadSerializers.push_back(new Swift::GatewayPayloadSerializer());
	m_payloadSerializers.push_back(new Swift::HintPayloadSerializer());

	if (CONFIG_BOOL(m_config, "service.server_mode")) {
		LOG4CXX_INFO(logger, "Creating component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server = new Swift::Server(loop, factories, userRegistry, m_jid, CONFIG_STRING(m_config, "service.server"), CONFIG_INT(m_config, "service.port"));
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
#ifndef _WIN32
#ifndef __APPLE__
//TODO: fix
			LOG4CXX_INFO(logger, "Using PKCS#12 certificate " << CONFIG_STRING(m_config, "service.cert"));
			LOG4CXX_INFO(logger, "SSLv23_server_method used.");
			TLSServerContextFactory *f = new OpenSSLServerContextFactory();
			CertificateWithKey::ref certificate = SWIFTEN_SHRPTR_NAMESPACE::make_shared<PKCS12Certificate>(CONFIG_STRING(m_config, "service.cert"), createSafeByteArray(CONFIG_STRING(m_config, "service.cert_password")));
			m_server->addTLSEncryption(f, certificate);
#endif
#endif
			
		}
		else {
			LOG4CXX_WARN(logger, "No PKCS#12 certificate used. TLS is disabled.");
		}
// 		m_server->start();
		m_stanzaChannel = m_server->getStanzaChannel();
		m_iqRouter = m_server->getIQRouter();

		SwiftServerExposed* entity(reinterpret_cast<SwiftServerExposed*>(m_server));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:1", entity->getPayloadParserFactories()));
		m_payloadSerializers.push_back(new Swift::PrivilegeSerializer(entity->getPayloadSerializers()));

		BOOST_FOREACH(Swift::PayloadParserFactory *factory, m_parserFactories) {
			m_server->addPayloadParserFactory(factory);
		}

		BOOST_FOREACH(Swift::PayloadSerializer *serializer, m_payloadSerializers) {
			m_server->addPayloadSerializer(serializer);
		}

		m_server->onDataRead.connect(boost::bind(&XMPPFrontend::handleDataRead, this, _1));
		m_server->onDataWritten.connect(boost::bind(&XMPPFrontend::handleDataWritten, this, _1));
	}
	else {
		LOG4CXX_INFO(logger, "Creating component in gateway mode");
#if HAVE_SWIFTEN_3
		m_component = new Swift::Component(m_jid, CONFIG_STRING(m_config, "service.password"), factories);
#else
		m_component = new Swift::Component(loop, factories, m_jid, CONFIG_STRING(m_config, "service.password"));
#endif
		m_component->setSoftwareVersion("Spectrum", SPECTRUM_VERSION);
		m_component->onConnected.connect(boost::bind(&XMPPFrontend::handleConnected, this));
		m_component->onError.connect(boost::bind(&XMPPFrontend::handleConnectionError, this, _1));
		m_component->onDataRead.connect(boost::bind(&XMPPFrontend::handleDataRead, this, _1));
		m_component->onDataWritten.connect(boost::bind(&XMPPFrontend::handleDataWritten, this, _1));

		SwiftComponentExposed* entity(reinterpret_cast<SwiftComponentExposed*>(m_component));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:1", entity->getPayloadParserFactories()));
		m_payloadSerializers.push_back(new Swift::PrivilegeSerializer(entity->getPayloadSerializers()));

		BOOST_FOREACH(Swift::PayloadParserFactory *factory, m_parserFactories) {
			m_component->addPayloadParserFactory(factory);
		}

		BOOST_FOREACH(Swift::PayloadSerializer *serializer, m_payloadSerializers) {
			m_component->addPayloadSerializer(serializer);
		}

		m_stanzaChannel = m_component->getStanzaChannel();
		m_iqRouter = m_component->getIQRouter();
	}


	m_capsMemoryStorage = new CapsMemoryStorage();
#if HAVE_SWIFTEN_3
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_stanzaChannel, m_iqRouter, factories->getCryptoProvider());
#else
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_stanzaChannel, m_iqRouter);
#endif
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_stanzaChannel);
	m_entityCapsManager->onCapsChanged.connect(boost::bind(&XMPPFrontend::handleCapsChanged, this, _1));

	m_stanzaChannel->onPresenceReceived.connect(bind(&XMPPFrontend::handleGeneralPresence, this, _1));
	m_stanzaChannel->onMessageReceived.connect(bind(&XMPPFrontend::handleMessage, this, _1));
}

XMPPFrontend::~XMPPFrontend() {
	delete m_entityCapsManager;
	delete m_capsManager;
	delete m_capsMemoryStorage;
	if (m_component)
		delete m_component;
	if (m_server) {
		m_server->stop();
		delete m_server;
	}

	BOOST_FOREACH(Swift::PayloadParserFactory *factory, m_parserFactories) {
		delete factory;
	}
	m_parserFactories.clear();

	BOOST_FOREACH(Swift::PayloadSerializer *serializer, m_payloadSerializers) {
		delete serializer;
	}
	m_payloadSerializers.clear();
}

void XMPPFrontend::handleGeneralPresence(Swift::Presence::ref presence) {
	onPresenceReceived(presence);
}

void XMPPFrontend::handleMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message) {
	onMessageReceived(message);
}


void XMPPFrontend::clearRoomList() {
	static_cast<XMPPUserManager *>(m_userManager)->getDiscoItemsResponder()->clearRooms();
}

void XMPPFrontend::addRoomToRoomList(const std::string &handle, const std::string &name) {
	static_cast<XMPPUserManager *>(m_userManager)->getDiscoItemsResponder()->addRoom(handle, name);
}

void XMPPFrontend::sendPresence(Swift::Presence::ref presence) {
	if (!presence->getFrom().getNode().empty()) {
		presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::CapsInfo(static_cast<XMPPUserManager *>(m_userManager)->getDiscoItemsResponder()->getBuddyCapsInfo())));
	}

	m_stanzaChannel->sendPresence(presence);
}

void XMPPFrontend::sendVCard(Swift::VCard::ref vcard, Swift::JID to) {
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GenericRequest<Swift::VCard> > request(new Swift::GenericRequest<Swift::VCard>(Swift::IQ::Result, to, vcard, m_iqRouter));
	request->send();
}

void XMPPFrontend::sendRosterRequest(Swift::RosterPayload::ref payload, Swift::JID to) {
	Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, to, m_iqRouter);
	request->send();
}

void XMPPFrontend::sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message) {
	m_stanzaChannel->sendMessage(message);
}

void XMPPFrontend::sendIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq) {
	m_iqRouter->sendIQ(iq);
}

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> XMPPFrontend::sendCapabilitiesRequest(Swift::JID to) {
	Swift::DiscoInfo::ref caps = m_entityCapsManager->getCaps(to);
	if (caps != Swift::DiscoInfo::ref()) {
		onCapabilitiesReceived(to, caps);
		return caps;
	}
#ifdef SUPPORT_LEGACY_CAPS
	else {
		GetDiscoInfoRequest::ref discoInfoRequest = GetDiscoInfoRequest::create(to, m_iqRouter);
		discoInfoRequest->onResponse.connect(boost::bind(&XMPPFrontend::handleDiscoInfoResponse, this, _1, _2, to));
		discoInfoRequest->send();
	}
#endif

	return Swift::DiscoInfo::ref();
}

void XMPPFrontend::reconnectUser(const std::string &user) {
	if (inServerMode()) {
		return;
	}

	LOG4CXX_INFO(logger, "Sending probe presence to " << user);
	Swift::Presence::ref response = Swift::Presence::create();
	try {
		response->setTo(user);
	}
	catch (...) { return; }
	
	response->setFrom(m_component->getJID());
	response->setType(Swift::Presence::Probe);

	m_stanzaChannel->sendPresence(response);
}

RosterManager *XMPPFrontend::createRosterManager(User *user, Component *component) {
	return new XMPPRosterManager(user, component);
}

User *XMPPFrontend::createUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) {
	return new XMPPUser(jid, userInfo, component, userManager);
}

UserManager *XMPPFrontend::createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	if (m_userManager) {
		delete m_userManager;
	}
	m_userManager = new XMPPUserManager(component, userRegistry, storageBackend);
	return m_userManager;
}

bool XMPPFrontend::handleIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq) {
	if (!m_rawXML) {
		return false;
	}

	if (iq->getPayload<Swift::RosterPayload>() != NULL) { return false; }
	if (iq->getPayload<Swift::InBandRegistrationPayload>() != NULL) { return false; }
	if (iq->getPayload<Swift::StatsPayload>() != NULL) { return false; }

	if (iq->getTo().getNode().empty()) {
		return false;
	}

	m_transport->onRawIQReceived(iq);
	return true;
}

void XMPPFrontend::handleBackendConfigChanged() {
	if (!m_rawXML && CONFIG_BOOL_DEFAULTED(m_config, "features.rawxml", false)) {
		LOG4CXX_INFO(logger, "Enabled Raw XML mode");
		m_rawXML = true;
		m_iqRouter->addHandler(this);
	}
}

Swift::StanzaChannel *XMPPFrontend::getStanzaChannel() {
	return m_stanzaChannel;
}

void XMPPFrontend::connectToServer() {
	if (m_component && !m_component->isAvailable()) {
		LOG4CXX_INFO(logger, "Connecting XMPP server " << CONFIG_STRING(m_config, "service.server") << " port " << CONFIG_INT(m_config, "service.port"));
		if (CONFIG_INT(m_config, "service.port") == 5222) {
			LOG4CXX_WARN(logger, "Port 5222 is usually used for client connections, not for component connections! Are you sure you are using right port?");
		}
		m_component->connect(CONFIG_STRING(m_config, "service.server"), CONFIG_INT(m_config, "service.port"));
	}
	else if (m_server) {
		LOG4CXX_INFO(logger, "Starting XMPPFrontend in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server->start();

		//Type casting to BoostConnectionServer since onStopped signal is not defined in ConnectionServer
		//Ideally, onStopped must be defined in ConnectionServer
		if (SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::BoostConnectionServer>(m_server->getConnectionServer())) {
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::BoostConnectionServer>(m_server->getConnectionServer())->onStopped.connect(boost::bind(&XMPPFrontend::handleServerStopped, this, _1));
		}
		
		// We're connected right here, because we're in server mode...
		handleConnected();
	}
}

void XMPPFrontend::disconnectFromServer() {
	if (m_component) {
		// TODO: Call this once swiften will fix assert(!session_);
// 		m_component->disconnect();
	}
	else if (m_server) {
		LOG4CXX_INFO(logger, "Stopping component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server->stop();
	}
}

void XMPPFrontend::handleConnected() {
	m_transport->handleConnected();
}

void XMPPFrontend::handleServerStopped(boost::optional<Swift::BoostConnectionServer::Error> e) {
	if(e) {
		if(*e == Swift::BoostConnectionServer::Conflict) {
			LOG4CXX_INFO(logger, "Port "<< CONFIG_INT(m_config, "service.port") << " already in use! Stopping server..");
			if (CONFIG_INT(m_config, "service.port") == 5347) {
				LOG4CXX_INFO(logger, "Port 5347 is usually used for components. You are using server_mode=1. Are you sure you don't want to use server_mode=0 and run spectrum as component?");
			}
		}
		if(*e == Swift::BoostConnectionServer::UnknownError)
			LOG4CXX_INFO(logger, "Unknown error occured! Stopping server..");
		exit(1);
	}
}


void XMPPFrontend::handleConnectionError(const ComponentError &error) {
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

	m_transport->handleConnectionError(str);
}

void XMPPFrontend::handleDataRead(const Swift::SafeByteArray &data) {
	std::string d = safeByteArrayToString(data);
	m_transport->handleDataRead(d);
}

void XMPPFrontend::handleDataWritten(const Swift::SafeByteArray &data) {
	std::string d = safeByteArrayToString(data);
	m_transport->handleDataWritten(d);
}

void XMPPFrontend::handleDiscoInfoResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid) {
#ifdef SUPPORT_LEGACY_CAPS
	onCapabilitiesReceived(jid, info);
#endif
}

void XMPPFrontend::handleCapsChanged(const Swift::JID& jid) {
	onCapabilitiesReceived(jid, m_entityCapsManager->getCaps(jid));
}

std::string XMPPFrontend::getRegistrationFields() {
	std::string fields = "Jabber ID";
// 	if (CONFIG_BOOL(m_config, "registration.needRegistration")) {
		fields += "\n" + CONFIG_STRING(m_config, "registration.username_label") + "\n";
		fields += CONFIG_STRING(m_config, "registration.password_label");
// 	}
	return fields;
}

}
