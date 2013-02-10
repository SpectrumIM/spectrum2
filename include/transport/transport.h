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
#include "Swiften/Server/Server.h"
#include "Swiften/Disco/GetDiscoInfoRequest.h"
#include "Swiften/Disco/EntityCapsManager.h"
#include "Swiften/Disco/CapsManager.h"
#include "Swiften/Disco/CapsMemoryStorage.h"
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include "Swiften/Server/UserRegistry.h"
#include "Swiften/Base/SafeByteArray.h"
#include "Swiften/Queries/IQHandler.h"
#include "Swiften/Jingle/JingleSessionManager.h"
#include "Swiften/Component/ComponentError.h"
#include "Swiften/Component/Component.h"
#include "Swiften/Queries/IQHandler.h"

#include <boost/bind.hpp>
#include "transport/config.h"
#include "transport/factory.h"
#include "transport/presenceoracle.h"
#include <Swiften/Network/BoostConnectionServer.h>

namespace Transport {
	class StorageBackend;
	class Factory;
	class UserRegistry;

	/// Represents one transport instance.

	/// It's used to connect the Jabber server and provides transaction layer
	/// between Jabber server and other classes.
	///
	/// In server mode it represents Jabber server to which users can connect and use
	/// it as transport.
	class Component : Swift::IQHandler {
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
			Component(Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, Factory *factory, Transport::UserRegistry *userRegistry = NULL);

			/// Component destructor.
			~Component();

			/// Returns Swift::StanzaChannel associated with this Transport::Component.

			/// It can be used to send presences and other stanzas.
			/// \return Swift::StanzaChannel associated with this Transport::Component.
			Swift::StanzaChannel *getStanzaChannel();

			/// Returns Swift::IQRouter associated with this Component.

			/// \return Swift::IQRouter associated with this Component.
			Swift::IQRouter *getIQRouter() { return m_iqRouter; }

			/// Returns Swift::PresenceOracle associated with this Transport::Component.

			/// You can use it to check current resource connected for particular user.
			/// \return Swift::PresenceOracle associated with this Transport::Component.
			PresenceOracle *getPresenceOracle();

			/// Returns True if the component is in server mode.

			/// \return True if the component is in server mode.
			bool inServerMode() { return m_server != NULL; }

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
			boost::signal<void (const Swift::ComponentError &error)> onConnectionError;

			/// This signal is emitted when transport successfully connects the server.
			boost::signal<void ()> onConnected;

			/// This signal is emitted when XML stanza is sent to server.

			/// \param xml xml stanza
			boost::signal<void (const std::string &xml)> onXMLOut;

			/// This signal is emitted when XML stanza is received from server.

			/// \param xml xml stanza
			boost::signal<void (const std::string &xml)> onXMLIn;

			Config *getConfig() { return m_config; }

			/// This signal is emitted when presence from XMPP user is received.

			/// It's emitted only for presences addressed to transport itself
			/// (for example to="j2j.domain.tld") and for presences comming to
			/// MUC (for example to="#chat%irc.freenode.org@irc.domain.tld")
			/// \param presence Presence.
			boost::signal<void (Swift::Presence::ref presence)> onUserPresenceReceived;

			/// Component class asks the XMPP clients automatically for their capabilities.
			/// This signal is emitted when capabilities have been received or changed.
			/// \param jid JID of the client for which we received capabilities
			/// \param info disco#info with response.
			boost::signal<void (const Swift::JID& jid, boost::shared_ptr<Swift::DiscoInfo> info)> onUserDiscoInfoReceived;

			boost::signal<void (boost::shared_ptr<Swift::IQ>)> onRawIQReceived;

		private:
			void handleConnected();
			void handleConnectionError(const Swift::ComponentError &error);
			void handleServerStopped(boost::optional<Swift::BoostConnectionServer::Error> e);
			void handlePresence(Swift::Presence::ref presence);
			void handleDataRead(const Swift::SafeByteArray &data);
			void handleDataWritten(const Swift::SafeByteArray &data);

			void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
			void handleCapsChanged(const Swift::JID& jid);

			void handleBackendConfigChanged();
			bool handleIQ(boost::shared_ptr<Swift::IQ>);

			Swift::NetworkFactories *m_factories;
			Swift::Component *m_component;
			Swift::Server *m_server;
			Swift::Timer::ref m_reconnectTimer;
			Swift::EntityCapsManager *m_entityCapsManager;
			Swift::CapsManager *m_capsManager;
			Swift::CapsMemoryStorage *m_capsMemoryStorage;
			PresenceOracle *m_presenceOracle;
			Swift::StanzaChannel *m_stanzaChannel;
			Swift::IQRouter *m_iqRouter;
			
			Transport::UserRegistry *m_userRegistry;
			StorageBackend *m_storageBackend;
			int m_reconnectCount;
			Config* m_config;
			std::string m_protocol;
			Swift::JID m_jid;
			Factory *m_factory;
			Swift::EventLoop *m_loop;
			bool m_rawXML;

		friend class User;
		friend class UserRegistration;
		friend class NetworkPluginServer;
	};
}
