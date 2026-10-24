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
#include <boost/foreach.hpp>
#include "XMPPRosterManager.h"
#include "XMPPUser.h"
#include "XMPPUserManager.h"
#include "XMPPConversation.h"
#include "PresenceOracle.h"
#include "jidutils.h"
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "transport/StorageBackend.h"
#include "transport/Factory.h"
#include "transport/UserRegistry.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/Transport.h"
#include "transport/AdminInterface.h"
#include "transport/LocalBuddy.h"
#include "transport/ConversationManager.h"
#include "storageparser.h"
#ifdef _WIN32
#include <Swiften/TLS/CAPICertificate.h>
#include "Swiften/TLS/Schannel/SchannelServerContext.h"
#include "Swiften/TLS/Schannel/SchannelServerContextFactory.h"
#elif defined(__APPLE__)
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
#include <Swiften/Elements/BlockPayload.h>
#include <Swiften/Parser/PayloadParsers/BlockParser.h>
#include <Swiften/Serializer/PayloadSerializers/BlockSerializer.h>
#include "Swiften/Parser/PayloadParsers/InvisibleParser.h"
#include "Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h"
#include "Swiften/Parser/PayloadParsers/HintPayloadParser.h"
#include "Swiften/Serializer/PayloadSerializers/HintPayloadSerializer.h"
#include "Swiften/Parser/PayloadParsers/PrivilegeParser.h"
#include "Swiften/Serializer/PayloadSerializers/PrivilegeSerializer.h"
#include "Swiften/Parser/GenericPayloadParserFactory.h"
#include "Swiften/Parser/GenericPayloadParserFactory2.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Queries/GenericRequest.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/StatsPayload.h"
#include "discoitemsresponder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"
#include "carbonresponder.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(logger, "XMPPFrontend");

XMPPFrontend::XMPPFrontend() {
	m_userManager = NULL;
	m_presenceOracle = NULL;
	m_xmppParser = NULL;
	m_serializer = NULL;
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

void XMPPFrontend::init(Component *transport, Config *config, Transport::UserRegistry *userRegistry) {
	m_transport = transport;
	m_component = NULL;
	m_server = NULL;
	m_rawXML = false;
	// Until the server announces its XEP-0356 permissions, use the current namespace
	m_privilegeNamespace = "urn:xmpp:privilege:2";
	m_config = transport->getConfig();
	m_userManager = NULL;
	m_userRegistry = userRegistry;
	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));
	m_factories = transport->getNetworkFactories();
	Swift::EventLoop *loop = transport->getEventLoop();

	m_presenceOracle = new PresenceOracle();
	m_presenceOracle->onPresenceChange = [this](Swift::Presence::ref presence) { handlePresence(presence); };

	m_xmppParser = new Swift::XMPPParser(this, &m_collection, m_factories->getXMLParserFactory());
	m_xmppParser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
	m_serializer = new Swift::XMPPSerializer(&m_collection2, Swift::ClientStreamType, false);

	auto prevBackendConfig = m_config->onBackendConfigUpdated;
	m_config->onBackendConfigUpdated = [prevBackendConfig, this]() {
		prevBackendConfig();
		handleBackendConfigChanged();
	};

	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<StorageParser>("private", "jabber:iq:private"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::AttentionParser>("attention", "urn:xmpp:attention:0"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::XHTMLIMParser>("html", "http://jabber.org/protocol/xhtml-im"));
	m_parserFactories.push_back(new Swift::GenericPayloadParserFactory<Swift::BlockParser<Swift::BlockPayload>>("block", "urn:xmpp:block:0"));
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
	m_payloadSerializers.push_back(new Swift::BlockSerializer<Swift::BlockPayload>("block"));
	m_payloadSerializers.push_back(new Swift::InvisibleSerializer());
	m_payloadSerializers.push_back(new Swift::StatsSerializer());
	m_payloadSerializers.push_back(new Swift::SpectrumErrorSerializer());
	m_payloadSerializers.push_back(new Swift::GatewayPayloadSerializer());
	m_payloadSerializers.push_back(new Swift::HintPayloadSerializer());

	if (CONFIG_BOOL(m_config, "service.server_mode")) {
		LOG4CXX_INFO(logger, "Creating component in server mode on port " << CONFIG_INT(m_config, "service.port"));
		m_server = new Swift::Server(loop, m_factories, userRegistry, m_jid, CONFIG_STRING(m_config, "service.server"), CONFIG_INT(m_config, "service.port"));
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
#ifndef _WIN32
#ifndef __APPLE__
//TODO: fix
			LOG4CXX_INFO(logger, "Using PKCS#12 certificate " << CONFIG_STRING(m_config, "service.cert"));
			LOG4CXX_INFO(logger, "SSLv23_server_method used.");
			TLSServerContextFactory *f = new OpenSSLServerContextFactory();
			CertificateWithKey::ref certificate = std::make_shared<PKCS12Certificate>(CONFIG_STRING(m_config, "service.cert"), createSafeByteArray(CONFIG_STRING(m_config, "service.cert_password")));
			m_server->addTLSEncryption(f, certificate);
#endif
#endif

		}
		else {
			LOG4CXX_INFO(logger, "No PKCS#12 certificate used. TLS is disabled.");
		}
// 		m_server->start();
		m_stanzaChannel = m_server->getStanzaChannel();
		m_iqRouter = m_server->getIQRouter();

		SwiftServerExposed* entity(reinterpret_cast<SwiftServerExposed*>(m_server));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:1", entity->getPayloadParserFactories()));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:2", entity->getPayloadParserFactories()));
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
		m_component = new Swift::Component(m_jid, CONFIG_STRING(m_config, "service.password"), m_factories);
		m_component->setSoftwareVersion("Spectrum", SPECTRUM_VERSION);
		m_component->onConnected.connect(boost::bind(&XMPPFrontend::handleConnected, this));
		m_component->onError.connect(boost::bind(&XMPPFrontend::handleConnectionError, this, _1));
		m_component->onDataRead.connect(boost::bind(&XMPPFrontend::handleDataRead, this, _1));
		m_component->onDataWritten.connect(boost::bind(&XMPPFrontend::handleDataWritten, this, _1));

		SwiftComponentExposed* entity(reinterpret_cast<SwiftComponentExposed*>(m_component));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:1", entity->getPayloadParserFactories()));
		m_parserFactories.push_back(new Swift::GenericPayloadParserFactory2<Swift::PrivilegeParser>("privilege", "urn:xmpp:privilege:2", entity->getPayloadParserFactories()));
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
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_stanzaChannel, m_iqRouter, m_factories->getCryptoProvider());
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_stanzaChannel);
	m_entityCapsManager->onCapsChanged.connect(boost::bind(&XMPPFrontend::handleCapsChanged, this, _1));

	m_stanzaChannel->onPresenceReceived.connect(bind(&XMPPFrontend::handleGeneralPresence, this, _1));
	m_stanzaChannel->onMessageReceived.connect(bind(&XMPPFrontend::handleMessage, this, _1));

	m_carbonResponder = new CarbonResponder(m_iqRouter);
	m_carbonResponder->start();
}

XMPPFrontend::~XMPPFrontend() {
	delete m_carbonResponder;
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

	delete m_xmppParser;
	delete m_serializer;
	delete m_presenceOracle;
}

void XMPPFrontend::handleGeneralPresence(Swift::Presence::ref presence) {
	// The oracle sees presences before the user manager does.
	m_presenceOracle->handleIncomingPresence(presence);
	if (m_userManager) {
		m_userManager->handleGeneralPresenceReceived(presence);
	}
}

// Presences the oracle tracks: those addressed to the transport itself.
void XMPPFrontend::handlePresence(Swift::Presence::ref presence) {
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
		sendCapabilitiesRequest(presence->getFrom());
	}

	if (m_userManager) {
		m_userManager->handlePresence(presence);
	}
}

void XMPPFrontend::handleMessage(std::shared_ptr<Swift::Message> message) {
	// The server announces the XEP-0356 permissions it granted us; reply in the same namespace
	Swift::Privilege::ref privilege = message->getPayload<Swift::Privilege>();
	if (privilege && message->getFrom().getNode().empty() && message->getFrom().getResource().empty()) {
		m_privilegeNamespace = privilege->getNamespace();
	}

	// Message order: UserManager first, then the admin interface (if any).
	if (m_userManager) {
		m_userManager->handleMessageReceived(message);
	}
	handleAdminMessage(message);
}

void XMPPFrontend::handleAdminMessage(std::shared_ptr<Swift::Message> message) {
	AdminInterface *adminInterface = m_transport->getAdminInterface();
	if (!adminInterface) {
		return;
	}

	if (!message->getTo().getNode().empty())
		return;

	std::vector<std::string> const &x = CONFIG_VECTOR(m_config,"service.admin_jid");
	if (std::find(x.begin(), x.end(), message->getFrom().toBare().toString()) == x.end()) {
	    LOG4CXX_WARN(logger, "Message not from admin user, but from " << message->getFrom().toBare().toString());
	    return;

	}

	// Ignore empty messages
	if (message->getBody().get_value_or("").empty()) {
		return;
	}

	std::string reply = adminInterface->handleQuery(message->getBody().get_value_or(""));

	message->setTo(message->getFrom());
	message->setFrom(m_jid);
	message->setBody(reply);
	sendMessage(message);
}

void XMPPFrontend::clearRoomList() {
	m_userManager->getDiscoItemsResponder()->clearRooms();
}

void XMPPFrontend::addRoomToRoomList(const std::string &room, const std::string &name) {
	m_userManager->getDiscoItemsResponder()->addRoom(Swift::JID::getEscapedNode(room) + "@" + m_jid.toString(), name);
}

void XMPPFrontend::sendPresence(Swift::Presence::ref presence) {
	if (!presence->getFrom().getNode().empty()) {
		presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::CapsInfo(m_userManager->getDiscoItemsResponder()->getBuddyCapsInfo())));
	}

	m_stanzaChannel->sendPresence(presence);
}

void XMPPFrontend::sendVCard(Swift::VCard::ref vcard, Swift::JID to) {
	std::shared_ptr<Swift::GenericRequest<Swift::VCard> > request(new Swift::GenericRequest<Swift::VCard>(Swift::IQ::Result, to, vcard, m_iqRouter));
	request->send();
}

void XMPPFrontend::sendRosterRequest(Swift::RosterPayload::ref payload, Swift::JID to) {
	Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, to, m_iqRouter);
	request->send();
}

void XMPPFrontend::sendMessage(std::shared_ptr<Swift::Message> message) {
	Swift::Privilege::ref privilege = message->getPayload<Swift::Privilege>();
	if (privilege) {
		privilege->setNamespace(m_privilegeNamespace);
	}
	m_stanzaChannel->sendMessage(message);
}

void XMPPFrontend::sendIQ(std::shared_ptr<Swift::IQ> iq) {
	m_iqRouter->sendIQ(iq);
}

std::shared_ptr<Swift::DiscoInfo> XMPPFrontend::sendCapabilitiesRequest(Swift::JID to) {
	Swift::DiscoInfo::ref caps = m_entityCapsManager->getCaps(to);
	if (caps != Swift::DiscoInfo::ref()) {
		if (m_userManager) {
			m_userManager->handleDiscoInfo(to, caps);
		}
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

User *XMPPFrontend::createUser(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager) {
	return new XMPPUser(user, userInfo, component, userManager);
}

UserManager *XMPPFrontend::createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) {
	if (m_userManager) {
		delete m_userManager;
	}
	XMPPUserManager *xmppUserManager = new XMPPUserManager(component, userRegistry, storageBackend);
	m_userManager = xmppUserManager;
	m_carbonResponder->setDiscoInfoResponder(xmppUserManager->getDiscoItemsResponder()->getDiscoInfoResponder());
	return m_userManager;
}

Conversation *XMPPFrontend::createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) {
	return new XMPPConversation(conversationManager, legacyName, isMUC);
}

void XMPPFrontend::handleLoginResult(const std::string &user, bool connected, const std::string &error) {
	if (!m_userRegistry) {
		return;
	}

	if (connected) {
		m_userRegistry->onPasswordValid(user);
	}
	else {
		m_userRegistry->onPasswordInvalid(user, error);
	}
}

pbnetwork::VCard XMPPFrontend::toVCard(User *user, std::shared_ptr<Swift::VCard> v) {
	pbnetwork::VCard vcard;
	vcard.set_username(user->getUserName());
	vcard.set_buddyname("");
	vcard.set_id(0);
	vcard.set_fullname(v->getFullName());
	vcard.set_nickname(v->getNickname());
	const Swift::ByteArray &photo = v->getPhoto();
	vcard.set_photo(std::string(photo.begin(), photo.end()));
	return vcard;
}

bool XMPPFrontend::handleIQ(std::shared_ptr<Swift::IQ> iq) {
	if (!m_rawXML) {
		return false;
	}

	if (iq->getPayload<Swift::RosterPayload>() != NULL) { return false; }
	if (iq->getPayload<Swift::InBandRegistrationPayload>() != NULL) { return false; }
	if (iq->getPayload<Swift::StatsPayload>() != NULL) { return false; }

	User *user = m_userManager ? m_userManager->getUser(iq->getFrom().toBare()) : NULL;
	if (!user) {
		return true;
	}

	if (iq->getType() == Swift::IQ::Get) {
		m_id2resource[iq->getFrom().toBare().toString() + iq->getID()] = iq->getFrom().getResource();
	}

	sendRawStanza(user, iq);
	return true;
}

void XMPPFrontend::sendRawStanza(User *user, Swift::Presence::ref presence) {
	doSendRawStanza(user, Swift::Presence::create(presence));
}

void XMPPFrontend::sendRawStanza(User *user, std::shared_ptr<Swift::Message> message) {
	doSendRawStanza(user, std::make_shared<Swift::Message>(*message));
}

void XMPPFrontend::sendRawStanza(User *user, std::shared_ptr<Swift::IQ> iq) {
	doSendRawStanza(user, std::make_shared<Swift::IQ>(*iq));
}

void XMPPFrontend::doSendRawStanza(User *user, std::shared_ptr<Swift::Stanza> stanza) {
	if (!CONFIG_BOOL_DEFAULTED(m_config, "features.rawxml", false)) {
		return;
	}

	// The backend knows the buddy by its legacy JID.
	Swift::JID legacyname = Swift::JID(JIDUtils::JIDToLegacyName(stanza->getTo()));
	if (!stanza->getTo().getResource().empty()) {
		stanza->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain(), stanza->getTo().getResource()));
	}
	else {
		stanza->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain()));
	}

	std::string xml = safeByteArrayToString(m_serializer->serializeElement(stanza));
	onRawXMLReceived(user, xml);
}

void XMPPFrontend::sendRawXML(const std::string &xml) {
	m_xmppParser->parse(xml);
}

void XMPPFrontend::handleElement(std::shared_ptr<Swift::ToplevelElement> element) {
	std::shared_ptr<Swift::Stanza> stanza = std::dynamic_pointer_cast<Swift::Stanza>(element);
	if (!stanza) {
		return;
	}

	if (!m_userManager) {
		return;
	}

	User *user = m_userManager->getUser(stanza->getTo().toBare());
	if (!user)
		return;

	Swift::JID originalJID = stanza->getFrom();
	XMPPConversation *conv = static_cast<XMPPConversation *>(user->getConversationManager()->getConversation(originalJID.toBare()));

	XMPPRosterManager *rosterManager = static_cast<XMPPRosterManager *>(user->getRosterManager());
	LocalBuddy *buddy = (LocalBuddy *) rosterManager->getBuddy(stanza->getFrom().toBare());
	if (buddy) {
		const Swift::JID jid = JIDUtils::getBuddyJID(buddy);
		if (stanza->getFrom().getResource().empty()) {
			stanza->setFrom(Swift::JID(jid.getNode(), jid.getDomain()));
		}
		else {
			stanza->setFrom(Swift::JID(jid.getNode(), jid.getDomain(), stanza->getFrom().getResource()));
		}
	}
	else {
		std::string name = stanza->getFrom().toBare();
		if (conv && conv->isMUC()) {
			if (name.find_last_of("@") != std::string::npos) {
				name.replace(name.find_last_of("@"), 1, "%");
			}
		}
		else {
			if (CONFIG_BOOL_DEFAULTED(m_config, "service.jid_escaping", true)) {
				name = Swift::JID::getEscapedNode(name);
			}
			else {
				if (name.find_last_of("@") != std::string::npos) {
					name.replace(name.find_last_of("@"), 1, "%");
				}
			}
		}
		if (stanza->getFrom().getResource().empty()) {
			stanza->setFrom(Swift::JID(name, m_jid.toString()));
		}
		else {
			stanza->setFrom(Swift::JID(name, m_jid.toString(), stanza->getFrom().getResource()));
		}
	}

	std::shared_ptr<Swift::Message> message = std::dynamic_pointer_cast<Swift::Message>(stanza);
	if (message) {
		if (conv) {
			if (conv->isMUC() || message->getType() == Swift::Message::Groupchat) {
				conv->handleRawMessage(message);
				return;
			}

			// The message goes to the last-replied resource
			Swift::JID delivered = conv->getDeliveryJID();
			message->setTo(delivered);

			// In gateway mode the XMPP server copies the message to the other resources itself
			if (!m_transport->inServerMode()) {
				conv->handleRawMessage(message);
				return;
			}

			// The other resources of the user get a XEP-0280 <received> carbon
			std::shared_ptr<Swift::Message> copy(new Swift::Message(*message));
			conv->handleRawMessage(message);
			conv->forwardReceivedCopies(copy, delivered);
			return;
		}

		sendMessage(message);
		return;
	}

	std::shared_ptr<Swift::Presence> presence = std::dynamic_pointer_cast<Swift::Presence>(stanza);
	if (presence) {
		if (buddy) {
			if (!buddy->isAvailable() && presence->getType() != Swift::Presence::Unavailable) {
				buddy->setAvailable();
			}
			rosterManager->handleRawPresence(buddy, presence);
		}
		else if (conv) {
			conv->handleRawPresence(presence);
		}
		else {
			sendPresence(presence);
		}

		return;
	}

	// TODO: Move m_id2resource in User and clean it up
	std::shared_ptr<Swift::IQ> iq = std::dynamic_pointer_cast<Swift::IQ>(stanza);
	if (iq) {
		if (m_id2resource.find(stanza->getTo().toBare().toString() + stanza->getID()) != m_id2resource.end()) {
			iq->setTo(Swift::JID(iq->getTo().getNode(), iq->getTo().getDomain(), m_id2resource[stanza->getTo().toBare().toString() + stanza->getID()]));
			m_id2resource.erase(stanza->getTo().toBare().toString() + stanza->getID());
		}
		else {
			const Swift::JID &userJID = static_cast<XMPPUser *>(user)->getJID();
			Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(userJID);
			if (highest) {
			    iq->setTo(highest->getFrom());
			} else {
			    iq->setTo(userJID);
			}
		}
		// In gateway mode the XMPP server rejects a component stanza without a recipient and drops the stream.
		// In server mode an IQ without 'to' is a valid server-directed IQ, so it is left alone.
		if (!m_transport->inServerMode() && !iq->getTo().isValid()) {
			LOG4CXX_WARN(logger, "Dropping IQ " << iq->getID() << " from backend: no valid recipient");
			return;
		}
		sendIQ(iq);
		return;
	}
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
		if (std::dynamic_pointer_cast<Swift::BoostConnectionServer>(m_server->getConnectionServer())) {
			std::dynamic_pointer_cast<Swift::BoostConnectionServer>(m_server->getConnectionServer())->onStopped.connect(boost::bind(&XMPPFrontend::handleServerStopped, this, _1));
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
	if (e) {
		if (*e == Swift::BoostConnectionServer::Conflict) {
			LOG4CXX_INFO(logger, "Port "<< CONFIG_INT(m_config, "service.port") << " already in use! Stopping server..");
			if (CONFIG_INT(m_config, "service.port") == 5347) {
				LOG4CXX_INFO(logger, "Port 5347 is usually used for components. You are using server_mode=1. Are you sure you don't want to use server_mode=0 and run spectrum as component?");
			}
		}
		if (*e == Swift::BoostConnectionServer::UnknownError)
			LOG4CXX_INFO(logger, "Unknown error occurred! Stopping server..");
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

void XMPPFrontend::handleDiscoInfoResponse(std::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid) {
#ifdef SUPPORT_LEGACY_CAPS
	if (m_userManager) {
		m_userManager->handleDiscoInfo(jid, info);
	}
#endif
}

void XMPPFrontend::handleCapsChanged(const Swift::JID& jid) {
	if (m_userManager) {
		m_userManager->handleDiscoInfo(jid, m_entityCapsManager->getCaps(jid));
	}
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
