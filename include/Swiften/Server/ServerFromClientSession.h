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
					UserRegistry* userRegistry);
			~ServerFromClientSession();

			boost::signal<void ()> onSessionStarted;
			void setAllowSASLEXTERNAL();
			const std::string &getUser() {
				return user_;
			}

			void addTLSEncryption(TLSServerContextFactory* tlsContextFactory, const PKCS12Certificate& cert);

		private:
			void handleElement(boost::shared_ptr<Element>);
			void handleStreamStart(const ProtocolHeader& header);
			void handlePasswordValid(const std::string &user);
			void handlePasswordInvalid(const std::string &user);

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
