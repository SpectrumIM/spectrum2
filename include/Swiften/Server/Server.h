/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <vector>

#include "Swiften/Network/BoostIOServiceThread.h"
#include "Swiften/Network/ConnectionServer.h"
#include "Swiften/Server/UserRegistry.h"
#include "Swiften/Server/ServerSession.h"
#include "Swiften/Base/IDGenerator.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/JID/JID.h"
#include "Swiften/Base/ByteArray.h"
#include "Swiften/Entity/Entity.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "Swiften/Serializer/PayloadSerializers/FullPayloadSerializerCollection.h"
#include "Swiften/TLS/PKCS12Certificate.h"

namespace Swift {
	class ConnectionServer;
	class SessionTracer;
	class EventLoop;
	class NetworkFactories;
	class StanzaChannel;
	class IQRouter;
	class TLSServerContextFactory;

	class Server : public Entity {
		public:
			Server(EventLoop* eventLoop, NetworkFactories* networkFactories, UserRegistry *userRegistry, const JID& jid, int port);
			~Server();

			void start();
			void stop();

			int getPort() const {
				return port_;
			}

			StanzaChannel* getStanzaChannel() const {
				return stanzaChannel_;
			}

			IQRouter* getIQRouter() const {
				return iqRouter_;
			}

			boost::signal<void (const SafeByteArray&)> onDataRead;
			boost::signal<void (const SafeByteArray&)> onDataWritten;

			void addTLSEncryption(TLSServerContextFactory* tlsContextFactory, const PKCS12Certificate& cert);

		private:
			void handleNewClientConnection(boost::shared_ptr<Connection> c);
			void handleSessionStarted(boost::shared_ptr<ServerFromClientSession>);
			void handleSessionFinished(boost::shared_ptr<ServerFromClientSession>);
			void handleElementReceived(boost::shared_ptr<Element> element, boost::shared_ptr<ServerFromClientSession> session);
			void handleDataRead(const SafeByteArray&);
			void handleDataWritten(const SafeByteArray&);

		private:
			IDGenerator idGenerator;
			FullPayloadParserFactoryCollection payloadParserFactories;
			FullPayloadSerializerCollection payloadSerializers;
			UserRegistry *userRegistry_;
			int port_;
			EventLoop* eventLoop;
			NetworkFactories* networkFactories_;
			bool stopping;
			boost::shared_ptr<ConnectionServer> serverFromClientConnectionServer;
			std::vector<boost::bsignals::connection> serverFromClientConnectionServerSignalConnections;
			std::list<boost::shared_ptr<ServerFromClientSession> > serverFromClientSessions;
			JID selfJID;
			StanzaChannel *stanzaChannel_;
			IQRouter *iqRouter_;
			TLSServerContextFactory *tlsFactory;
			PKCS12Certificate cert;
	};
}
