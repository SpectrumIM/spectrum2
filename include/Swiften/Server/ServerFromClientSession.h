/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/signals2.hpp>

#include <string>
#include <Swiften/Session/Session.h>
#include <Swiften/JID/JID.h>
#include <Swiften/Network/Connection.h>
#include <Swiften/Base/ByteArray.h>
#include <Swiften/TLS/CertificateWithKey.h>

namespace Swift {
	class ProtocolHeader;
	class Element;
	class Stanza;
	class PayloadParserFactoryCollection;
	class PayloadSerializerCollection;
	class StreamStack;
	class UserRegistry;
	class XMPPLayer;
	class ConnectionLayer;
	class Connection;
	class TLSServerLayer;
	class TLSServerContextFactory;
	class PKCS12Certificate;

	class ServerFromClientSession : public Session {
		public:
			ServerFromClientSession(
					const std::string& id,
					std::shared_ptr<Connection> connection,
					PayloadParserFactoryCollection* payloadParserFactories, 
					PayloadSerializerCollection* payloadSerializers,
					UserRegistry* userRegistry,
					XMLParserFactory* factory,
					Swift::JID remoteJID = Swift::JID());
			~ServerFromClientSession();

			boost::signals2::signal<void ()> onSessionStarted;
			void setAllowSASLEXTERNAL();
			const std::string &getUser() {
				return user_;
			}

			void addTLSEncryption(TLSServerContextFactory* tlsContextFactory, CertificateWithKey::ref cert);

			Swift::JID getBareJID() {
				return Swift::JID(user_, getLocalJID().getDomain());
			}

			void handlePasswordValid();
			void handlePasswordInvalid(const std::string &error = "");

		private:
			void handleElement(std::shared_ptr<ToplevelElement>);
			void handleStreamStart(const ProtocolHeader& header);
			void handleSessionFinished(const boost::optional<SessionError>&);

			void setInitialized();
			bool isInitialized() const { 
				return initialized; 
			}

			void handleTLSError() { }
			void handleTLSConnected() { tlsConnected = true; }

		private:
			std::string id_;
			UserRegistry* userRegistry_;
			bool authenticated_;
			bool initialized;
			bool allowSASLEXTERNAL;
			std::string user_;
			TLSServerLayer* tlsLayer;
			bool tlsConnected;
	};
}
