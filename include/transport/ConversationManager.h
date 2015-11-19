/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include <string>
#include <algorithm>
#include <map>

#include "Swiften/Elements/Message.h"

namespace Transport {

class Conversation;
class User;
class Component;

/// Manages all Conversations of particular User.
class ConversationManager {
	public:
		/// Creates new ConversationManager.

		/// \param user User associated with this ConversationManager.
		/// \param component Transport instance associated with this ConversationManager.
		ConversationManager(User *user, Component *component);

		/// Destructor.
		virtual ~ConversationManager();

		/// Returns user associated with this manager.

		/// \return User
		User *getUser() { return m_user; }

		/// Returns component associated with this ConversationManager.

		/// \return component associated with this ConversationManager.
		Component *getComponent() { return m_component; }

		/// Returns Conversation by its legacy network name (for example by UIN in case of ICQ).

		/// \param name legacy network name.
		/// \return Conversation or NULL.
		Conversation *getConversation(const std::string &name);

		void sendCachedChatMessages();

		/// Adds new Conversation to the manager.

		/// \param conv Conversation.
		void addConversation(Conversation *conv);

		/// Removes Conversation from the manager.

		/// \param conv Conversation.
		void removeConversation(Conversation *conv);

		void deleteAllConversations();

		void resetResources();
		void removeJID(const Swift::JID &jid);
		void clearJIDs();

	private:
		void handleMessageReceived(Swift::Message::ref message);

		Component *m_component;
		User *m_user;

		std::map<std::string, Conversation *> m_convs;
		friend class UserManager;
};

}
