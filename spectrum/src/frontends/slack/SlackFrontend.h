/**
 * Spectrum 2 Slack Frontend
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
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
#include <boost/bind.hpp>

#define THREAD_POOL(X) static_cast<SlackFrontend *>(X->getFrontend())->getThreadPool()

namespace Transport {
	class UserRegistry;
	class Frontend;
	class Config;
	class DiscoItemsResponder;
	class VCardResponder;
	class ThreadPool;

	class SlackFrontend : public Frontend {
		public:
			SlackFrontend();

			virtual ~SlackFrontend();

			virtual void init(Component *component, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, UserRegistry *userRegistry);

			virtual void connectToServer();

			virtual void disconnectFromServer();

			virtual void sendPresence(Swift::Presence::ref presence);

			virtual void sendVCard(Swift::VCard::ref vcard, Swift::JID to);

			virtual void sendRosterRequest(Swift::RosterPayload::ref, Swift::JID to);

			virtual void sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message);

			virtual void sendIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ>);

			virtual void reconnectUser(const std::string &user);

			virtual RosterManager *createRosterManager(User *user, Component *component);
			virtual User *createUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager);
			virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);
			virtual SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> sendCapabilitiesRequest(Swift::JID to);
			virtual void clearRoomList();
			virtual void addRoomToRoomList(const std::string &handle, const std::string &name);
			virtual std::string setOAuth2Code(const std::string &code, const std::string &state);
			virtual std::string getOAuth2URL(const std::vector<std::string> &args);
			virtual std::string getRegistrationFields();
		
			void handleMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message);

			ThreadPool *getThreadPool() {
				return m_tp;
			}

		private:
			Config* m_config;
			Swift::JID m_jid;
			Component *m_transport;
			UserManager *m_userManager;
			ThreadPool *m_tp;
	};
}
