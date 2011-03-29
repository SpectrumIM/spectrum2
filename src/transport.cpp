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
#include "transport/storagebackend.h"
#include "discoinforesponder.h"
#include "discoitemsresponder.h"

using namespace Swift;
using namespace boost;

namespace Transport {


class MyUserRegistry : public Swift::UserRegistry {
	public:
		MyUserRegistry() {}
		~MyUserRegistry() {}
		bool isValidUserPassword(const JID& user, const std::string& password) const {
			users[user.toBare().toString()] = password;
			return true;
		}
		mutable std::map<std::string, std::string> users;
};

Component::Component(Swift::EventLoop *loop, Config *config) {
	m_component = NULL;
	m_userRegistry = NULL;
	m_server = NULL;
	m_reconnectCount = 0;
	m_config = config;

	m_jid = Swift::JID(CONFIG_STRING(m_config, "service.jid"));

	m_factories = new BoostNetworkFactories(loop);

	m_reconnectTimer = m_factories->getTimerFactory()->createTimer(1000);
	m_reconnectTimer->onTick.connect(bind(&Component::connect, this)); 

	if (CONFIG_BOOL(m_config, "service.server_mode")) {
		m_userRegistry = new MyUserRegistry();
		m_server = new Swift::Server(loop, m_factories, m_userRegistry, m_jid, CONFIG_INT(m_config, "service.port"));
		m_server->start();
		m_stanzaChannel = m_server->getStanzaChannel();
		m_iqRouter = m_server->getIQRouter();
	}
	else {
		m_component = new Swift::Component(loop, m_factories, m_jid, CONFIG_STRING(m_config, "service.password"));
		m_component->setSoftwareVersion("", "");
		m_component->onConnected.connect(bind(&Component::handleConnected, this));
		m_component->onError.connect(bind(&Component::handleConnectionError, this, _1));
		m_component->onDataRead.connect(bind(&Component::handleDataRead, this, _1));
		m_component->onDataWritten.connect(bind(&Component::handleDataWritten, this, _1));
		m_component->onPresenceReceived.connect(bind(&Component::handlePresenceReceived, this, _1));
// 		m_component->onMessageReceived.connect(bind(&Component::handleMessageReceived, this, _1));
		m_stanzaChannel = m_component->getStanzaChannel();
		m_iqRouter = m_component->getIQRouter();
	}

	m_capsMemoryStorage = new CapsMemoryStorage();
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_stanzaChannel, m_iqRouter);
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_stanzaChannel);
 	m_entityCapsManager->onCapsChanged.connect(boost::bind(&Component::handleCapsChanged, this, _1));
	
	m_presenceOracle = new PresenceOracle(m_stanzaChannel);
	m_presenceOracle->onPresenceChange.connect(bind(&Component::handlePresence, this, _1));

	m_discoInfoResponder = new DiscoInfoResponder(m_iqRouter);
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
// 	delete m_discoInfoResponder;
// 	delete m_registerHandler;
	if (m_component)
		delete m_component;
	if (m_server)
		delete m_server;
	if (m_userRegistry)
		delete m_userRegistry;
	delete m_factories;
}

const std::string &Component::getUserRegistryPassword(const std::string &barejid) {
	MyUserRegistry *registry = dynamic_cast<MyUserRegistry *>(m_userRegistry);
	return registry->users[barejid];
}

Swift::StanzaChannel *Component::getStanzaChannel() {
	return m_stanzaChannel;
}

Swift::PresenceOracle *Component::getPresenceOracle() {
	return m_presenceOracle;
}

void Component::setTransportFeatures(std::list<std::string> &features) {
	m_discoInfoResponder->setTransportFeatures(features);
}

void Component::setBuddyFeatures(std::list<std::string> &features) {
	// TODO: handle caps change
	m_discoInfoResponder->setBuddyFeatures(features);
}

void Component::connect() {
	if (!m_component)
		return;
	m_reconnectCount++;
	m_component->connect(CONFIG_STRING(m_config, "service.server"), CONFIG_INT(m_config, "service.port"));
	m_reconnectTimer->stop();
}

void Component::handleConnected() {
	onConnected();
	m_reconnectCount = 0;
}

void Component::handleConnectionError(const ComponentError &error) {
	onConnectionError(error);
// 	if (m_reconnectCount == 2)
// 		Component::instance()->userManager()->removeAllUsers();

	m_reconnectTimer->start();
}

void Component::handleDataRead(const std::string &data) {
	onXMLIn(data);
}

void Component::handleDataWritten(const std::string &data) {
	onXMLOut(data);
}

void Component::handlePresenceReceived(Swift::Presence::ref presence) {
	switch(presence->getType()) {
		case Swift::Presence::Subscribe:
		case Swift::Presence::Subscribed:
		case Swift::Presence::Unsubscribe:
		case Swift::Presence::Unsubscribed:
			handleSubscription(presence);
			break;
		case Swift::Presence::Available:
		case Swift::Presence::Unavailable:
			break;
		case Swift::Presence::Probe:
			handleProbePresence(presence);
			break;
		default:
			break;
	};
}

void Component::handlePresence(Swift::Presence::ref presence) {
	bool isMUC = presence->getPayload<MUCPayload>() != NULL;

	// filter out login/logout presence spam
	if (!presence->getTo().getNode().empty() && isMUC == false)
		return;

	// filter out bad presences
	if (!presence->getFrom().isValid()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(presence->getFrom());
		response->setFrom(presence->getTo());
		response->setType(Swift::Presence::Error);

		response->addPayload(boost::shared_ptr<Payload>(new ErrorPayload(ErrorPayload::JIDMalformed, ErrorPayload::Modify)));

		m_component->sendPresence(response);
		return;
	}

	// check if we have this client's capabilities and ask for them
	bool haveFeatures = false;
	if (presence->getType() != Swift::Presence::Unavailable) {
		boost::shared_ptr<CapsInfo> capsInfo = presence->getPayload<CapsInfo>();
		if (capsInfo && capsInfo->getHash() == "sha-1") {
			haveFeatures = m_entityCapsManager->getCaps(presence->getFrom()) != DiscoInfo::ref();
			std::cout << "has capsInfo " << haveFeatures << "\n";
		}
// 		else {
// 			GetDiscoInfoRequest::ref discoInfoRequest = GetDiscoInfoRequest::create(presence->getFrom(), m_iqRouter);
// 			discoInfoRequest->onResponse.connect(boost::bind(&Component::handleDiscoInfoResponse, this, _1, _2, presence->getFrom()));
// 			discoInfoRequest->send();
// 		}
	}

	onUserPresenceReceived(presence);
}

void Component::handleProbePresence(Swift::Presence::ref presence) {
	
}

void Component::handleSubscription(Swift::Presence::ref presence) {
	// answer to subscibe
	if (presence->getType() == Swift::Presence::Subscribe && presence->getTo().getNode().empty()) {
// 		Log(presence->getFrom().toString().getUTF8String(), "Subscribe presence received => sending subscribed");
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Subscribed);
		m_component->sendPresence(response);
		return;
	}

	if (m_protocol == "irc") {
		return;
	}

// 	User *user;
// 	std::string barejid = presence->getTo().toBare().toString().getUTF8String();
// 	std::string userkey = presence->getFrom().toBare().toString().getUTF8String();
// 	if (Transport::instance()->protocol()->tempAccountsAllowed()) {
// 		std::string server = barejid.substr(barejid.find("%") + 1, barejid.length() - barejid.find("%"));
// 		userkey += server;
// 	}

// 	user = (User *) Transport::instance()->userManager()->getUserByJID(userkey);
// 	if (user) {
// 		user->handleSubscription(presence);
// 	}
// 	else if (presence->getType() == Swift::Presence::Unsubscribe) {
// 		Swift::Presence::ref response = Swift::Presence::create();
// 		response->setFrom(presence->getTo());
// 		response->setTo(presence->getFrom());
// 		response->setType(Swift::Presence::Unsubscribed);
// 		m_component->sendPresence(response);
// 	}
// 	else {
// // 		Log(presence->getFrom().toString().getUTF8String(), "Subscribe presence received, but this user is not logged in");
// 	}
}

void Component::handleCapsChanged(const Swift::JID& jid) {
	bool haveFeatures = m_entityCapsManager->getCaps(jid) != DiscoInfo::ref();
	std::cout << "has capsInfo " << haveFeatures << "\n";
}

}
