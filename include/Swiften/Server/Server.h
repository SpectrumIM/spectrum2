/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
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
#include "Swiften/SwiftenCompat.h"
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

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ConnectionServer> getConnectionServer() const {
				return serverFromClientConnectionServer;
			}

			boost::signal<void (const SafeByteArray&)> onDataRead;
			boost::signal<void (const SafeByteArray&)> onDataWritten;

			void addTLSEncryption(TLSServerContextFactory* tlsContextFactory, CertificateWithKey::ref cert);

		private:
			void handleNewClientConnection(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Connection> c);
			void handleSessionStarted(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession>);
			void handleSessionFinished(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession>);
			void handleElementReceived(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Element> element, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session);
			void handleDataRead(const SafeByteArray&);
			void handleDataWritten(const SafeByteArray&);

		private:
			IDGenerator idGenerator;
			UserRegistry *userRegistry_;
			int port_;
			EventLoop* eventLoop;
			NetworkFactories* networkFactories_;
			bool stopping;
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ConnectionServer> serverFromClientConnectionServer;
			std::vector<SWIFTEN_SIGNAL_NAMESPACE::connection> serverFromClientConnectionServerSignalConnections;
			std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> > serverFromClientSessions;
			JID selfJID;
			StanzaChannel *stanzaChannel_;
			IQRouter *iqRouter_;
			TLSServerContextFactory *tlsFactory;
			CertificateWithKey::ref cert;
			PlatformXMLParserFactory *parserFactory_;
			std::string address_;
	};
}
