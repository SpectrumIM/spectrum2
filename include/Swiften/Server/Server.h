/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
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
#include <Swiften/TLS/CertificateWithKey.h>
#include <Swiften/Parser/PlatformXMLParserFactory.h>

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
			Server(EventLoop* eventLoop, NetworkFactories* networkFactories, UserRegistry *userRegistry, const JID& jid, const std::string &address, int port);
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

			std::shared_ptr<ConnectionServer> getConnectionServer() const {
				return serverFromClientConnectionServer;
			}

			boost::signals2::signal<void (const SafeByteArray&)> onDataRead;
			boost::signals2::signal<void (const SafeByteArray&)> onDataWritten;

			void addTLSEncryption(TLSServerContextFactory* tlsContextFactory, CertificateWithKey::ref cert);

		private:
			void handleNewClientConnection(std::shared_ptr<Connection> c);
			void handleSessionStarted(std::shared_ptr<ServerFromClientSession>);
			void handleSessionFinished(std::shared_ptr<ServerFromClientSession>);
			void handleElementReceived(std::shared_ptr<Element> element, std::shared_ptr<ServerFromClientSession> session);
			void handleDataRead(const SafeByteArray&);
			void handleDataWritten(const SafeByteArray&);

		private:
			IDGenerator idGenerator;
			UserRegistry *userRegistry_;
			int port_;
			EventLoop* eventLoop;
			NetworkFactories* networkFactories_;
			bool stopping;
			std::shared_ptr<ConnectionServer> serverFromClientConnectionServer;
			std::vector<boost::signals2::connection> serverFromClientConnectionServerSignalConnections;
			std::list<std::shared_ptr<ServerFromClientSession> > serverFromClientSessions;
			JID selfJID;
			StanzaChannel *stanzaChannel_;
			IQRouter *iqRouter_;
			TLSServerContextFactory *tlsFactory;
			CertificateWithKey::ref cert;
			PlatformXMLParserFactory *parserFactory_;
			std::string address_;
	};
}
