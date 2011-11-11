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

/// Represents one XMPP-Legacy network conversation.
class Conversation {
	public:
		/// Type of participants in MUC rooms.
		enum ParticipantFlag {None, Moderator};

		/// Creates new conversation.

		/// \param conversationManager ConversationManager associated with this Conversation.
		/// \param legacyName Legacy network name of recipient.
		/// \param muc True if this conversation is Multi-user chat.
		Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false);

		/// Destructor.
		virtual ~Conversation();

		/// Returns legacy network name of this conversation.

		/// \return legacy network name of this conversation.
		const std::string &getLegacyName() { return m_legacyName; }

		/// Handles new message from Legacy network and forwards it to XMPP.

		/// \param message Message received from legacy network.
		/// \param nickname For MUC conversation this is nickname of room participant who sent this message.
		void handleMessage(boost::shared_ptr<Swift::Message> &message, const std::string &nickname = "");

		/// Handles participant change in MUC.

		/// \param nickname Nickname of participant which changed.
		/// \param flag ParticipantFlag.
		/// \param status Current status of this participant.
		/// \param statusMessage Current status message of this participant.
		/// \param newname If participant was renamed, this variable contains his new name.
		void handleParticipantChanged(const std::string &nickname, int flag, int status = Swift::StatusShow::None, const std::string &statusMessage = "", const std::string &newname = "");

		/// Sets XMPP user nickname in MUC rooms.

		/// \param nickname XMPP user nickname in MUC rooms.
		void setNickname(const std::string &nickname) {
			m_nickname = nickname;
		}

		void setJID(const Swift::JID &jid) {
			m_jid = jid;
		}

		/// Sends message to Legacy network.

		/// \param message Message.
		virtual void sendMessage(boost::shared_ptr<Swift::Message> &message) = 0;

		/// Returns ConversationManager associated with this Conversation.

		/// \return  ConversationManager associated with this Conversation.
		ConversationManager *getConversationManager() {
			return m_conversationManager;
		}

		/// Returns True if this conversation is MUC room.

		/// \return  True if this conversation is MUC room.
		bool isMUC() {
			return m_muc;
		}

		/// Sets room name associated with this Conversation.

		/// This is used to detect Private messages associated with particular room.
		/// \param room room name associated with this Conversation.
		void setRoom(const std::string &room);

		/// Returns room name associated with this Conversation.

		/// \return room name associated with this Conversation.
		const std::string &getRoom() {
			return m_room;
		}

	private:
		ConversationManager *m_conversationManager;
		std::string m_legacyName;
		std::string m_nickname;
		std::string m_room;
		bool m_muc;
		Swift::JID m_jid;
};

}
