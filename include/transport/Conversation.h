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
#include <list>
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/SwiftenCompat.h"

namespace Transport {

class ConversationManager;

/// Represents one XMPP-Legacy network conversation.
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

		/// Handles new message from Legacy network and forwards it to XMPP.

		/// \param message Message received from legacy network.
		/// \param nickname For MUC conversation this is nickname of room participant who sent this message.
		void handleMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message, const std::string &nickname = "", const bool carbon = false);

		//Generates a carbon <sent> wrapper <message> around the given payload and delivers it
		void forwardAsCarbonSent(
			const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &payload,
			const Swift::JID& to);

		//Generates a impersonation request <message> arount the given payload and delivers it
		void forwardImpersonated(
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> payload,
			const Swift::JID& server);

		void handleRawMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message);
		void handleRawPresence(Swift::Presence::ref presence);

		/// Handles participant change in MUC.

		/// \param nickname Nickname of participant which changed.
		/// \param flag ParticipantFlag.
		/// \param status Current status of this participant.
		/// \param statusMessage Current status message of this participant.
		/// \param newname If participant was renamed, this variable contains his new name.
		void handleParticipantChanged(const std::string &nickname, ParticipantFlag flag, int status = Swift::StatusShow::None, const std::string &statusMessage = "", const std::string &newname = "", const std::string &iconhash = "", const std::string &alias = "");

		/// Sets XMPP user nickname in MUC rooms.

		/// \param nickname XMPP user nickname in MUC rooms.
		void setNickname(const std::string &nickname);

		const std::string &getNickname() {
			return m_nickname;
		}

		void setJID(const Swift::JID &jid) {
			m_jid = jid;
		}

		void addJID(const Swift::JID &jid) {
			m_jids.push_back(jid);
		}

		void clearJIDs() {
			m_jids.clear();
		}

		void removeJID(const Swift::JID &jid);

		const std::list<Swift::JID> &getJIDs() {
			return m_jids;
		}

		/// Sends message to Legacy network.

		/// \param message Message.
		virtual void sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message) = 0;

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

		void destroyRoom();

		std::string getParticipants();
		void sendParticipants(const Swift::JID &to, const std::string &nickname);

		void sendCachedMessages(const Swift::JID &to = Swift::JID());

		void setMUCEscaping(bool mucEscaping);

	private:
		Swift::Presence::ref generatePresence(const std::string &nick, int flag, int status, const std::string &statusMessage, const std::string &newname = "", const std::string &iconhash = "");
		void cacheMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message);

	private:
		ConversationManager *m_conversationManager;
		std::string m_legacyName;
		std::string m_nickname;
		std::string m_room;
		bool m_muc;
		Swift::JID m_jid;
		std::list<Swift::JID> m_jids;
		bool m_sentInitialPresence;
		bool m_nicknameChanged;
		bool m_mucEscaping;
		bool m_sentInitialSubject;

		// TODO: Move this to some extra class to cache the most used
		// rooms across different accounts. Just now if we have 10 users
		// connected to single room, we store all those things 10 times.
		// It would be also great to store last 100 messages per room
		// every time, so we can get history messages for IRC for example.
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> m_subject;
		std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> > m_cachedMessages;

		typedef struct {
			Swift::Presence::ref presence;
			std::string alias;
		} Participant;
		std::map<std::string, Participant> m_participants;
};

}
