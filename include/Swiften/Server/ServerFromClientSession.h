/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <Swiften/Base/boost_bsignals.h>
#include <boost/enable_shared_from_this.hpp>

#include <string>
#include <Swiften/Session/Session.h>
#include <Swiften/JID/JID.h>
#include <Swiften/Network/Connection.h>
#include <Swiften/Base/ByteArray.h>
#include <Swiften/TLS/CertificateWithKey.h>
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

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
					boost::shared_ptr<Connection> connection, 
					PayloadParserFactoryCollection* payloadParserFactories, 
					PayloadSerializerCollection* payloadSerializers,
					UserRegistry* userRegistry,
					XMLParserFactory* factory,
					Swift::JID remoteJID = Swift::JID());
			~ServerFromClientSession();

			boost::signal<void ()> onSessionStarted;
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
#if HAVE_SWIFTEN_3
			void handleElement(boost::shared_ptr<ToplevelElement>);
#else		
			void handleElement(boost::shared_ptr<Element>);
#endif
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
