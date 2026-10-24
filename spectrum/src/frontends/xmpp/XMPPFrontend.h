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

#include <map>
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
#include "Swiften/Component/ComponentError.h"
#include "Swiften/Component/Component.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/IQ.h"
#include "Swiften/Elements/VCard.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "Swiften/Serializer/PayloadSerializers/FullPayloadSerializerCollection.h"
#include "Swiften/Parser/XMPPParser.h"
#include "Swiften/Parser/XMPPParserClient.h"
#include "Swiften/Serializer/XMPPSerializer.h"

#include <boost/bind.hpp>
#include <Swiften/Network/BoostConnectionServer.h>

namespace Transport {
	class UserRegistry;
	class Frontend;
	class Config;
	class VCardResponder;
	class CarbonResponder;
	class PresenceOracle;
	class XMPPUserManager;

	class XMPPFrontend : public Frontend, Swift::IQHandler, Swift::XMPPParserClient {
		public:
			XMPPFrontend();

			virtual ~XMPPFrontend();

			virtual void init(Component *component, Config *config, UserRegistry *userRegistry);

			virtual void connectToServer();

			virtual void disconnectFromServer();

			virtual void reconnectUser(const std::string &user);

			virtual RosterManager *createRosterManager(User *user, Component *component);
			virtual User *createUser(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager);
			virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);
			virtual Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC = false);
			virtual void handleLoginResult(const std::string &user, bool connected, const std::string &error);
			virtual void sendRawXML(const std::string &xml);
			virtual void clearRoomList();
			virtual void addRoomToRoomList(const std::string &room, const std::string &name);

			void sendPresence(Swift::Presence::ref presence);

			void sendVCard(Swift::VCard::ref vcard, Swift::JID to);

			void sendRosterRequest(Swift::RosterPayload::ref, Swift::JID to);

			void sendMessage(std::shared_ptr<Swift::Message> message);

			void sendIQ(std::shared_ptr<Swift::IQ>);

			std::shared_ptr<Swift::DiscoInfo> sendCapabilitiesRequest(Swift::JID to);

			/// Passes the user's stanza to his backend as raw XML (features.rawxml).
			void sendRawStanza(User *user, Swift::Presence::ref presence);
			void sendRawStanza(User *user, std::shared_ptr<Swift::Message> message);
			void sendRawStanza(User *user, std::shared_ptr<Swift::IQ> iq);

			/// Handles every presence the transport receives.
			void handleGeneralPresence(Swift::Presence::ref presence);

			void handleMessage(std::shared_ptr<Swift::Message> message);

			static pbnetwork::VCard toVCard(User *user, std::shared_ptr<Swift::VCard> vcard);

			Swift::StanzaChannel *getStanzaChannel();

			Swift::IQRouter *getIQRouter() { return m_iqRouter; }

			PresenceOracle *getPresenceOracle() { return m_presenceOracle; }

			const Swift::JID &getJID() { return m_jid; }

			bool inServerMode() { return m_server != NULL; }

			bool isRawXMLEnabled() {
				return m_rawXML;
			}

			std::string getRegistrationFields();

		private:
			void handleConnected();
			void handleConnectionError(const Swift::ComponentError &error);
			void handleServerStopped(boost::optional<Swift::BoostConnectionServer::Error> e);
			void handlePresence(Swift::Presence::ref presence);
			void handleAdminMessage(std::shared_ptr<Swift::Message> message);
			void handleDataRead(const Swift::SafeByteArray &data);
			void handleDataWritten(const Swift::SafeByteArray &data);

			void handleDiscoInfoResponse(std::shared_ptr<Swift::DiscoInfo> info, Swift::ErrorPayload::ref error, const Swift::JID& jid);
			void handleCapsChanged(const Swift::JID& jid);

			void handleBackendConfigChanged();
			bool handleIQ(std::shared_ptr<Swift::IQ>);
			void doSendRawStanza(User *user, std::shared_ptr<Swift::Stanza> stanza);

			void handleStreamStart(const Swift::ProtocolHeader&) {}
			void handleElement(std::shared_ptr<Swift::ToplevelElement> element);
			void handleStreamEnd() {}

			Swift::NetworkFactories *m_factories;
			Swift::Component *m_component;
			Swift::Server *m_server;
			Swift::EntityCapsManager *m_entityCapsManager;
			Swift::CapsManager *m_capsManager;
			Swift::CapsMemoryStorage *m_capsMemoryStorage;
			Swift::StanzaChannel *m_stanzaChannel;
			Swift::IQRouter *m_iqRouter;
			VCardResponder *m_vcardResponder;
			CarbonResponder *m_carbonResponder;
			PresenceOracle *m_presenceOracle;

			Config* m_config;
			Swift::JID m_jid;
			bool m_rawXML;
			Component *m_transport;
			UserRegistry *m_userRegistry;
			XMPPUserManager *m_userManager;
			std::vector<Swift::PayloadParserFactory *> m_parserFactories;
			std::vector<Swift::PayloadSerializer *> m_payloadSerializers;
			std::string m_privilegeNamespace;

			// Raw XML (features.rawxml)
			Swift::XMPPParser *m_xmppParser;
			Swift::FullPayloadParserFactoryCollection m_collection;
			Swift::XMPPSerializer *m_serializer;
			Swift::FullPayloadSerializerCollection m_collection2;
			std::map <std::string, std::string> m_id2resource;

		friend class XMPPUser;
		friend class UserRegistration;
	};
}
