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
#include "Swiften/Swiften.h"

namespace Transport {

class Conversation;
class User;
class Component;

class ConversationManager {
	public:
		/// Creates new ConversationManager.
		/// \param user User associated with this ConversationManager.
		/// \param component Transport instance associated with this roster.
		ConversationManager(User *user, Component *component);

		/// Destructor.
		virtual ~ConversationManager();

		/// Returns user associated with this manager.
		/// \return User
		User *getUser() { return m_user; }

		Component *getComponent() { return m_component; }

		Conversation *getConversation(const std::string &name) {
			return m_convs[name];
		}

		void setConversation(Conversation *conv);

		void unsetConversation(Conversation *conv);

	private:
		void handleMessageReceived(Swift::Message::ref message);

		Component *m_component;
		User *m_user;

		std::map<std::string, Conversation *> m_convs;
		friend class UserManager;
};

}
