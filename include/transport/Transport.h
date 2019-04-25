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
#include <boost/bind.hpp>
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include "Swiften/Network/NetworkFactories.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/IQ.h"
#include "Swiften/SwiftenCompat.h"

namespace Transport {
	class StorageBackend;
	class UserRegistry;
	class Frontend;
	class PresenceOracle;
	class Factory;
	class Config;
	class UserManager;
	class AdminInterface;

	class Component {
		public:
			/// Creates new Component instance.

			/// \param loop Main event loop.
			/// \param config Cofiguration; this class uses following Config values:
			/// 	- service.jid
			/// 	- service.password
			/// 	- service.server
			/// 	- service.port
			/// 	- service.server_mode
			/// \param factories Swift::NetworkFactories.
			/// \param factory Transport Abstract factory used to create basic transport structures.
			/// \param userRegistery UserRegistry class instance. It's needed only when running transport in server-mode.
			Component(Frontend *frontend, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Factory *factory, Transport::UserRegistry *userRegistry = NULL);

			/// Component destructor.
			~Component();

			/// Returns Swift::StanzaChannel associated with this Transport::Component.

			/// Returns True if the component is in server mode.

			/// \return True if the component is in server mode.
			bool inServerMode();

			/// Starts the Component.
			
			/// In server-mode, it starts listening on particular port for new client connections.
			/// In gateway-mode, it connects the XMPP server.
			void start();

			/// Stops the component.
			void stop();

			/// Returns Jabber ID of this transport.

			/// \return Jabber ID of this transport
			Swift::JID &getJID() { return m_jid; }

			/// Returns Swift::NetworkFactories which can be used to create new connections.

			/// \return Swift::NetworkFactories which can be used to create new connections.
			Swift::NetworkFactories *getNetworkFactories() { return m_factories; }

			/// Returns Transport Factory used to create basic Transport components.

			/// \return Transport Factory used to create basic Transport components.
			Factory *getFactory() { return m_factory; }

			/// This signal is emitted when server disconnects the transport because of some error.

			/// \param error disconnection error
			SWIFTEN_SIGNAL_NAMESPACE::signal<void (const std::string &error)> onConnectionError;

			/// This signal is emitted when transport successfully connects the server.
			SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onConnected;

			/// This signal is emitted when XML stanza is sent to server.

			Config *getConfig() { return m_config; }

			/// It's emitted only for presences addressed to transport itself
			/// (for example to="j2j.domain.tld") and for presences comming to
			/// MUC (for example to="#chat%irc.freenode.org@irc.domain.tld")
			/// \param presence Presence.
			SWIFTEN_SIGNAL_NAMESPACE::signal<void (Swift::Presence::ref presence)> onUserPresenceReceived;

			SWIFTEN_SIGNAL_NAMESPACE::signal<void (SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ>)> onRawIQReceived;

			SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onAdminInterfaceSet;
			
			void handlePresence(Swift::Presence::ref presence);
			void handleConnected();
			void handleConnectionError(const std::string &error);
			void handleDataRead(const std::string &data);
			void handleDataWritten(const std::string &data);

			Frontend *getFrontend() {
				return m_frontend;
			}

			PresenceOracle *getPresenceOracle();

			void setAdminInterface(AdminInterface *adminInterface) {
				m_adminInterface = adminInterface;
				onAdminInterfaceSet();
			}

			AdminInterface *getAdminInterface() {
				return m_adminInterface;
			}

		private:
			void handleDiscoInfoResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
			void handleCapsChanged(const Swift::JID& jid);

			void handleBackendConfigChanged();

			Swift::NetworkFactories *m_factories;
			Swift::Timer::ref m_reconnectTimer;
			PresenceOracle *m_presenceOracle;
			
			Transport::UserRegistry *m_userRegistry;
			StorageBackend *m_storageBackend;
			int m_reconnectCount;
			Config* m_config;
			Swift::JID m_jid;
			Factory *m_factory;
			Swift::EventLoop *m_loop;
			Frontend *m_frontend;
			AdminInterface *m_adminInterface;

		friend class User;
		friend class UserRegistration;
		friend class NetworkPluginServer;
	};
}
