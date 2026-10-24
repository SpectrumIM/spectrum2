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

#include "transport/Event.h"
#include "transport/protocol.pb.h"

#include <string>
#include <algorithm>
#include <list>
#include <map>

namespace Transport {

class ConversationManager;

/// Represents one user - legacy network conversation (1:1 chat or room).
///
/// Messages are pbnetwork::ConversationMessage tagged with the WrapperMessage
/// type they travel as to and from the backend: TYPE_CONV_MESSAGE,
/// TYPE_ROOM_SUBJECT_CHANGED, TYPE_ATTENTION, TYPE_CONV_MESSAGE_ACK and the
/// chat states TYPE_BUDDY_TYPING, TYPE_BUDDY_TYPED and TYPE_BUDDY_STOPPED_TYPING.
///
/// The frontend creates conversations (Frontend::createConversation) and shows
/// them to the user.
class Conversation {
	public:
		typedef enum {
			PARTICIPANT_FLAG_NONE = 0,
			PARTICIPANT_FLAG_MODERATOR = 1,
			PARTICIPANT_FLAG_CONFLICT = 2,
			PARTICIPANT_FLAG_BANNED = 4,
			PARTICIPANT_FLAG_NOT_AUTHORIZED = 8,
			PARTICIPANT_FLAG_ME = 16,
			PARTICIPANT_FLAG_KICKED = 32,
			PARTICIPANT_FLAG_ROOM_NOT_FOUD = 64
		} ParticipantFlag;

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

		/// Handles new message from Legacy network and shows it to the user.

		/// \param type kind of the message (see the class description).
		/// \param message Message received from legacy network. For MUC, message.nickname()
		/// is the nickname of the room participant who sent the message.
		virtual void handleMessage(pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message) = 0;

		/// Handles participant change in MUC. Updates the participant list and calls
		/// doHandleParticipantChanged.

		/// \param participant changed participant; newname() is set if the participant was renamed.
		void handleParticipantChanged(const pbnetwork::Participant &participant);

		/// Sets user nickname in MUC rooms.

		/// \param nickname user nickname in MUC rooms.
		virtual void setNickname(const std::string &nickname);

		const std::string &getNickname() {
			return m_nickname;
		}

		/// Sends message from the user to Legacy network (onMessageToSend).

		/// \param type kind of the message (see the class description).
		/// \param message Message.
		void sendMessage(pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message);

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

		/// Tells the user the room is gone (the transport is shutting down).
		virtual void destroyRoom() {}

		/// Returns comma separated list of participants' names.
		std::string getParticipants();

		/// Participants by nickname.
		const std::map<std::string, pbnetwork::Participant> &getParticipantList() {
			return m_participants;
		}

		/// Sends messages the frontend cached while the user could not receive them.
		virtual void sendCachedMessages() {}

		/// Message from the user for the legacy network.
		Event<void (Conversation *, pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message)> onMessageToSend;

	protected:
		/// Shows the participant change to the user.
		virtual void doHandleParticipantChanged(const pbnetwork::Participant &participant) {}

	private:
		ConversationManager *m_conversationManager;
		std::string m_legacyName;
		std::string m_nickname;
		std::string m_room;
		bool m_muc;
		std::map<std::string, pbnetwork::Participant> m_participants;
};

}
