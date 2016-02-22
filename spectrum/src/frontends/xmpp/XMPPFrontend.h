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

#include "transport/Frontend.h"

#include <vector>
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)
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
#include "Swiften/Component/ComponentError.h"
#include "Swiften/Component/Component.h"
#include "Swiften/Queries/IQHandler.h"
#include "Swiften/Elements/Presence.h"

#include <boost/bind.hpp>
#include <Swiften/Network/BoostConnectionServer.h>

namespace Transport {
	class UserRegistry;
	class Frontend;
	class Config;
	class VCardResponder;

	class XMPPFrontend : public Frontend, Swift::IQHandler {
		public:
			XMPPFrontend();

			virtual ~XMPPFrontend();

			virtual void init(Component *component, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, UserRegistry *userRegistry);

			virtual void connectToServer();

			virtual void disconnectFromServer();

			virtual void sendPresence(Swift::Presence::ref presence);

			virtual void sendVCard(Swift::VCard::ref vcard, Swift::JID to);

			virtual void sendRosterRequest(Swift::RosterPayload::ref, Swift::JID to);

			virtual void sendMessage(boost::shared_ptr<Swift::Message> message);

			virtual void sendIQ(boost::shared_ptr<Swift::IQ>);

			virtual void reconnectUser(const std::string &user);

			virtual RosterManager *createRosterManager(User *user, Component *component);
			virtual User *createUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager);
			virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);
			virtual boost::shared_ptr<Swift::DiscoInfo> sendCapabilitiesRequest(Swift::JID to);
			virtual void clearRoomList();
			virtual void addRoomToRoomList(const std::string &handle, const std::string &name);
		
			void handleMessage(boost::shared_ptr<Swift::Message> message);


			Swift::StanzaChannel *getStanzaChannel();

			Swift::IQRouter *getIQRouter() { return m_iqRouter; }


			bool inServerMode() { return m_server != NULL; }

			bool isRawXMLEnabled() {
				return m_rawXML;
			}

			std::string getRegistrationFields();

		private:
			void handleConnected();
			void handleConnectionError(const Swift::ComponentError &error);
			void handleServerStopped(boost::optional<Swift::BoostConnectionServer::Error> e);
			void handleGeneralPresence(Swift::Presence::ref presence);
			void handleDataRead(const Swift::SafeByteArray &data);
			void handleDataWritten(const Swift::SafeByteArray &data);

			void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
			void handleCapsChanged(const Swift::JID& jid);

			void handleBackendConfigChanged();
			bool handleIQ(boost::shared_ptr<Swift::IQ>);

			Swift::NetworkFactories *m_factories;
			Swift::Component *m_component;
			Swift::Server *m_server;
			Swift::EntityCapsManager *m_entityCapsManager;
			Swift::CapsManager *m_capsManager;
			Swift::CapsMemoryStorage *m_capsMemoryStorage;
			Swift::StanzaChannel *m_stanzaChannel;
			Swift::IQRouter *m_iqRouter;
			VCardResponder *m_vcardResponder;
			
			Config* m_config;
			Swift::JID m_jid;
			bool m_rawXML;
			Component *m_transport;
			UserManager *m_userManager;

		friend class XMPPUser;
		friend class UserRegistration;
		friend class NetworkPluginServer;
	};
}
