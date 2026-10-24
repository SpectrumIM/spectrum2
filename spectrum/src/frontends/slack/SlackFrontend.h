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
	class SlackUserManager;

	class SlackFrontend : public Frontend {
		public:
			SlackFrontend();

			virtual ~SlackFrontend();

			virtual void init(Component *component, Config *config, UserRegistry *userRegistry);

			virtual void connectToServer();

			virtual void disconnectFromServer();

			virtual void reconnectUser(const std::string &user);

			virtual RosterManager *createRosterManager(User *user, Component *component);
			virtual User *createUser(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager);
			virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);
			virtual Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC = false);
			virtual void clearRoomList();
			virtual void addRoomToRoomList(const std::string &room, const std::string &name);
			virtual std::string setOAuth2Code(const std::string &code, const std::string &state);
			virtual std::string getOAuth2URL(const std::vector<std::string> &args);
			virtual std::string getRegistrationFields();

			ThreadPool *getThreadPool() {
				return m_tp;
			}

			SlackUserManager *getUserManager() {
				return m_userManager;
			}

		private:
			Config* m_config;
			Component *m_transport;
			SlackUserManager *m_userManager;
			ThreadPool *m_tp;
	};
}
