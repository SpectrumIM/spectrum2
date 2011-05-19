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
#include "transport/transport.h"

#include "Swiften/Swiften.h"
#include "Swiften/Elements/Message.h"

namespace Transport {

class ConversationManager;

class Conversation {
	public:
		enum ParticipantFlag {None, Moderator};

		/// Constructor.
		Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool m_muc = false);

		/// Destructor
		virtual ~Conversation();

		const std::string &getLegacyName() { return m_legacyName; }

		void handleMessage(boost::shared_ptr<Swift::Message> &message, const std::string &nickname = "");
		void handleParticipantChanged(const std::string &nickname, int flag, int status = Swift::StatusShow::None, const std::string &statusMessage = "", const std::string &newname = "");
		void setNickname(const std::string &nickname) {
			m_nickname = nickname;
		}

		virtual void sendMessage(boost::shared_ptr<Swift::Message> &message) = 0;

		ConversationManager *getConversationManager() {
			return m_conversationManager;
		}

		bool isMUC() {
			return m_muc;
		}

		void setRoom(const std::string &room) {
			m_room = room;
		}

		const std::string &getRoom() {
			return m_room;
		}

	private:
		ConversationManager *m_conversationManager;
		std::string m_legacyName;
		std::string m_nickname;
		std::string m_room;
		bool m_muc;
};

}
