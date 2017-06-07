/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/Server/Server.h"

#include <string>
#include <boost/bind.hpp>
#include <boost/signal.hpp>

#include "Swiften/Base/String.h"
#include "Swiften/Network/Connection.h"
#include "Swiften/Network/ConnectionServer.h"
#include "Swiften/Network/ConnectionServerFactory.h"
#include "Swiften/Elements/Element.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Network/NetworkFactories.h"
#include "Swiften/Session/SessionTracer.h"
#include "Swiften/Elements/IQ.h"
#include "Swiften/Elements/VCard.h"
#include "Swiften/Server/UserRegistry.h"
#include <string>
#include "Swiften/Network/ConnectionServer.h"
#include "Swiften/Network/ConnectionFactory.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Queries/IQRouter.h"
#include <iostream>


namespace Swift {

Server::Server(
		EventLoop* eventLoop,
		NetworkFactories* networkFactories,
		UserRegistry *userRegistry,
		const JID& jid,
		const std::string &address,
		int port) :
			userRegistry_(userRegistry),
			port_(port),
			eventLoop(eventLoop),
			networkFactories_(networkFactories),
			stopping(false),
			selfJID(jid),
			stanzaChannel_(),
			address_(address){
	stanzaChannel_ = new ServerStanzaChannel(selfJID);
	iqRouter_ = new IQRouter(stanzaChannel_);
	tlsFactory = NULL;
	parserFactory_ = new PlatformXMLParserFactory();
}

Server::~Server() {
	stop();
	delete iqRouter_;
	delete stanzaChannel_;
	delete parserFactory_;
}

void Server::start() {
	if (serverFromClientConnectionServer) {
		return;
	}
	if (address_ == "0.0.0.0") {
		serverFromClientConnectionServer = networkFactories_->getConnectionServerFactory()->createConnectionServer(port_);
	}
	else {
		auto hostAddress = Swift::HostAddress::fromString(address_);
		if (!hostAddress) {
			hostAddress = Swift::HostAddress::fromString("0.0.0.0");
		}
		serverFromClientConnectionServer = networkFactories_->getConnectionServerFactory()->createConnectionServer(*hostAddress, port_);
	}
	serverFromClientConnectionServerSignalConnections.push_back(
		serverFromClientConnectionServer->onNewConnection.connect(
				boost::bind(&Server::handleNewClientConnection, this, _1)));
// 	serverFromClientConnectionServerSignalConnections.push_back(
// 		serverFromClientConnectionServer->onStopped.connect(
// 				boost::bind(&Server::handleClientConnectionServerStopped, this, _1)));

	serverFromClientConnectionServer->start();
}

void Server::stop() {
	if (stopping) {
		return;
	}

	stopping = true;

// 	for(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session: serverFromClientSessions) {
// 		session->finishSession();
// 	}
	serverFromClientSessions.clear();

	if (serverFromClientConnectionServer) {
		serverFromClientConnectionServer->stop();
		for(SWIFTEN_SIGNAL_NAMESPACE::connection& connection: serverFromClientConnectionServerSignalConnections) {
			connection.disconnect();
		}
		serverFromClientConnectionServerSignalConnections.clear();
		serverFromClientConnectionServer.reset();
	}

	stopping = false;
// 	onStopped(e);
}

void Server::handleNewClientConnection(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Connection> connection) {

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> serverFromClientSession = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession>(
			new ServerFromClientSession(idGenerator.generateID(), connection, 
					getPayloadParserFactories(), getPayloadSerializers(), userRegistry_, parserFactory_));
	//serverFromClientSession->setAllowSASLEXTERNAL();

	serverFromClientSession->onSessionStarted.connect(
			boost::bind(&Server::handleSessionStarted, this, serverFromClientSession));
	serverFromClientSession->onSessionFinished.connect(
			boost::bind(&Server::handleSessionFinished, this, 
			serverFromClientSession));
	serverFromClientSession->onDataRead.connect(boost::bind(&Server::handleDataRead, this, _1));
	serverFromClientSession->onDataWritten.connect(boost::bind(&Server::handleDataWritten, this, _1));

	if (tlsFactory) {
		serverFromClientSession->addTLSEncryption(tlsFactory, cert);
	}

	serverFromClientSession->startSession();

	serverFromClientSessions.push_back(serverFromClientSession);
}

void Server::handleDataRead(const SafeByteArray& data) {
	onDataRead(data);
}

void Server::handleDataWritten(const SafeByteArray& data) {
	onDataWritten(data);
}

void Server::handleSessionStarted(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session) {
	dynamic_cast<ServerStanzaChannel *>(stanzaChannel_)->addSession(session);
}

void Server::handleSessionFinished(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session) {
// 	if (!session->getRemoteJID().isValid()) {
// 		Swift::Presence::ref presence = Swift::Presence::create();
// 		presence->setFrom(session->getBareJID());
// 		presence->setType(Swift::Presence::Unavailable);
// 		dynamic_cast<ServerStanzaChannel *>(stanzaChannel_)->onPresenceReceived(presence);
// 	}
	serverFromClientSessions.erase(std::remove(serverFromClientSessions.begin(), serverFromClientSessions.end(), session), serverFromClientSessions.end());
	session->onSessionStarted.disconnect(
			boost::bind(&Server::handleSessionStarted, this, session));
	session->onSessionFinished.disconnect(
			boost::bind(&Server::handleSessionFinished, this, session));
}

void Server::addTLSEncryption(TLSServerContextFactory* tlsContextFactory, CertificateWithKey::ref cert) {
	tlsFactory = tlsContextFactory;
	this->cert = cert;
}

}
