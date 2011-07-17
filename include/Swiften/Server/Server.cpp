/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/Server/Server.h"

#include <string>
#include <boost/bind.hpp>

#include "Swiften/Base/String.h"
#include "Swiften/Base/foreach.h"
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
		int port) :
			userRegistry_(userRegistry),
			port_(port),
			eventLoop(eventLoop),
			networkFactories_(networkFactories),
			stopping(false),
			selfJID(jid),
			stanzaChannel_(){
	stanzaChannel_ = new ServerStanzaChannel();
	iqRouter_ = new IQRouter(stanzaChannel_);
	tlsFactory = NULL;
}

Server::~Server() {
	stop();
	delete iqRouter_;
	delete stanzaChannel_;
}

void Server::start() {
	if (serverFromClientConnectionServer) {
		return;
	}
	serverFromClientConnectionServer = networkFactories_->getConnectionServerFactory()->createConnectionServer(port_);
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

	foreach(boost::shared_ptr<ServerFromClientSession> session, serverFromClientSessions) {
		session->finishSession();
	}
	serverFromClientSessions.clear();

	if (serverFromClientConnectionServer) {
		serverFromClientConnectionServer->stop();
		foreach(boost::bsignals::connection& connection, serverFromClientConnectionServerSignalConnections) {
			connection.disconnect();
		}
		serverFromClientConnectionServerSignalConnections.clear();
		serverFromClientConnectionServer.reset();
	}

	stopping = false;
// 	onStopped(e);
}

void Server::handleNewClientConnection(boost::shared_ptr<Connection> connection) {

	boost::shared_ptr<ServerFromClientSession> serverFromClientSession = boost::shared_ptr<ServerFromClientSession>(
			new ServerFromClientSession(idGenerator.generateID(), connection, 
					getPayloadParserFactories(), getPayloadSerializers(), userRegistry_));
	//serverFromClientSession->setAllowSASLEXTERNAL();

	serverFromClientSession->onSessionStarted.connect(
			boost::bind(&Server::handleSessionStarted, this, serverFromClientSession));
	serverFromClientSession->onSessionFinished.connect(
			boost::bind(&Server::handleSessionFinished, this, 
			serverFromClientSession));
	serverFromClientSession->onPasswordInvalid.connect(
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

void Server::handleSessionStarted(boost::shared_ptr<ServerFromClientSession> session) {
	dynamic_cast<ServerStanzaChannel *>(stanzaChannel_)->addSession(session);
}

void Server::handleSessionFinished(boost::shared_ptr<ServerFromClientSession> session) {
	serverFromClientSessions.erase(std::remove(serverFromClientSessions.begin(), serverFromClientSessions.end(), session), serverFromClientSessions.end());
}

void Server::addTLSEncryption(TLSServerContextFactory* tlsContextFactory, const PKCS12Certificate& cert) {
	tlsFactory = tlsContextFactory;
	this->cert = cert;
}

}
