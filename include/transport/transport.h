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

#pragma once

#include <vector>
#include "Swiften/Swiften.h"
#include "Swiften/Disco/GetDiscoInfoRequest.h"
#include "Swiften/Disco/EntityCapsManager.h"
#include "Swiften/Disco/CapsManager.h"
#include "Swiften/Disco/CapsMemoryStorage.h"
#include "Swiften/Presence/PresenceOracle.h"
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include <boost/bind.hpp>
#include "transport/config.h"

#define tr(lang,STRING)    (STRING)
#define _(STRING)    (STRING)

namespace Transport {
	// typedef enum { 	CLIENT_FEATURE_ROSTERX = 2,
	// 				CLIENT_FEATURE_XHTML_IM = 4,
	// 				CLIENT_FEATURE_FILETRANSFER = 8,
	// 				CLIENT_FEATURE_CHATSTATES = 16
	// 				} SpectrumImportantFeatures;
	// 
	class StorageBackend;
	class DiscoInfoResponder;
	class DiscoItemsResponder;

	/// Represents one transport instance.

	/// It's used to connect
	/// the Jabber server and provides transaction layer between Jabber server
	/// and other classes.
	class Component {
		public:
			/// Creates new Component instance.
			/// \param loop main event loop 
			/// \param config cofiguration, this class uses following Config values:
			/// 	- service.jid
			/// 	- service.password
			/// 	- service.server
			/// 	- service.port
			Component(Swift::EventLoop *loop, Config *config);

			/// Component destructor.
			~Component();

			/// Returns Swift::Component associated with this Transport::Component.
			/// You can use it to send presences and other stanzas.
			/// \return Swift::Component associated with this Transport::Component
			Swift::Component *getComponent();

			/// Returns Swift::PresenceOracle associated with this Transport::Component.
			/// You can use it to check current resource connected for particular user.
			/// \return Swift::PresenceOracle associated with this Transport::Component
			Swift::PresenceOracle *getPresenceOracle();

			/// Connects the Jabber server.
			/// \see Component()
			void connect();

			/// Sets disco#info features which are sent as answer to
			/// disco#info IQ-get. This sets features of transport contact (For example "j2j.domain.tld").
			/// \param features list of features as sent in disco#info response
			void setTransportFeatures(std::list<std::string> &features);

			/// Sets disco#info features which are sent as answer to
			/// disco#info IQ-get. This sets features of legacy network buddies (For example "me\40gmail.com@j2j.domain.tld").
			/// \param features list of features as sent in disco#info response
			void setBuddyFeatures(std::list<std::string> &features);

			/// Returns Jabber ID of this transport.
			/// \return Jabber ID of this transport
			Swift::JID &getJID() { return m_jid; }

			Swift::BoostNetworkFactories *getFactories() { return m_factories; }

			/// This signal is emitted when server disconnects the transport because of some error.
			/// \param error disconnection error
			boost::signal<void (const Swift::ComponentError &error)> onConnectionError;

			/// This signal is emitted when transport successfully connects the server.
			boost::signal<void ()> onConnected;

			/// This signal is emitted when XML stanza is sent to server.
			/// \param xml xml stanza
			boost::signal<void (const std::string &xml)> onXMLOut;

			/// This signal is emitted when XML stanza is received from server.
			/// \param xml xml stanza
			boost::signal<void (const std::string &xml)> onXMLIn;

			/// This signal is emitted when presence from XMPP user (for example "user@domain.tld")
			/// is received. It's emitted only for presences addressed to transport itself
			/// (for example to="j2j.domain.tld").
			/// \param presence presence data
			boost::signal<void (Swift::Presence::ref presence)> onUserPresenceReceived;

// 			boost::signal<void (boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid)> onDiscoInfoResponse;

		private:
			void handleConnected();
			void handleConnectionError(const Swift::ComponentError &error);
			void handlePresenceReceived(Swift::Presence::ref presence);
// 			void handleMessageReceived(Swift::Message::ref message);
			void handlePresence(Swift::Presence::ref presence);
			void handleSubscription(Swift::Presence::ref presence);
			void handleProbePresence(Swift::Presence::ref presence);
			void handleDataRead(const std::string &data);
			void handleDataWritten(const std::string &data);

// 			void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
			void handleCapsChanged(const Swift::JID& jid);

			Swift::BoostNetworkFactories *m_factories;
			Swift::Component *m_component;
			Swift::Timer::ref m_reconnectTimer;
			Swift::BoostIOServiceThread m_boostIOServiceThread;
			Swift::EntityCapsManager *m_entityCapsManager;
			Swift::CapsManager *m_capsManager;
			Swift::CapsMemoryStorage *m_capsMemoryStorage;
			Swift::PresenceOracle *m_presenceOracle;
			StorageBackend *m_storageBackend;
 			DiscoInfoResponder *m_discoInfoResponder;
			DiscoItemsResponder *m_discoItemsResponder;
// 			SpectrumRegisterHandler *m_registerHandler;
			int m_reconnectCount;
			Config* m_config;
			std::string m_protocol;
			Swift::JID m_jid;

		friend class User;
		friend class UserRegistration;
	};
}
