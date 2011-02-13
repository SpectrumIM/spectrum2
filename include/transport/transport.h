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
	// class SpectrumDiscoInfoResponder;
	// class SpectrumRegisterHandler;
	class StorageBackend;
	class DiscoInfoResponder;

	class Component {
		public:
			Component(Swift::EventLoop *loop, Config *config);
			~Component();

			// Connect to server
			void connect();

			void setStorageBackend(StorageBackend *backend);

			void setTransportFeatures(std::list<std::string> &features);
			void setBuddyFeatures(std::list<std::string> &features);

			Swift::JID &getJID() { return m_jid; }

			boost::signal<void (const Swift::ComponentError&)> onConnectionError;
			boost::signal<void ()> onConnected;
			boost::signal<void (const std::string &)> onXMLOut;
			boost::signal<void (const std::string &)> onXMLIn;
			boost::signal<void (Swift::Presence::ref presence)> onUserPresenceReceived;

		private:
			void handleConnected();
			void handleConnectionError(const Swift::ComponentError &error);
			void handlePresenceReceived(Swift::Presence::ref presence);
// 			void handleMessageReceived(Swift::Message::ref message);
			void handlePresence(Swift::Presence::ref presence);
			void handleSubscription(Swift::Presence::ref presence);
			void handleProbePresence(Swift::Presence::ref presence);
			void handleDataRead(const Swift::String &data);
			void handleDataWritten(const Swift::String &data);

			void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
// 			void handleCapsChanged(const Swift::JID& jid);

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
// 			SpectrumRegisterHandler *m_registerHandler;
			int m_reconnectCount;
			Config* m_config;
			std::string m_protocol;
			Swift::JID m_jid;

		friend class User;
		friend class UserRegistration;
	};
}
