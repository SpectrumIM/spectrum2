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

#include "transport/Conversation.h"
#include "transport/ConversationManager.h"

namespace Transport {

Conversation::Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) : m_conversationManager(conversationManager) {
	m_legacyName = legacyName;
	m_muc = isMUC;
}

Conversation::~Conversation() {
}

void Conversation::setRoom(const std::string &room) {
	m_room = room;
	m_legacyName = m_room + "/" + m_legacyName;
}

void Conversation::setNickname(const std::string &nickname) {
	m_nickname = nickname;
}

void Conversation::sendMessage(pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message) {
	onMessageToSend(this, type, message);
}

std::string Conversation::getParticipants() {
	std::string ret;
	for (const auto &it : m_participants) {
		const pbnetwork::Participant &p = it.second;
		ret += (p.alias().empty() ? p.nickname() : p.alias()) + ", ";
	}
	return ret;
}

void Conversation::handleParticipantChanged(const pbnetwork::Participant &participant) {
	const std::string &name = participant.alias().empty() ? participant.nickname() : participant.alias();
	int errorFlags = PARTICIPANT_FLAG_CONFLICT | PARTICIPANT_FLAG_NOT_AUTHORIZED | PARTICIPANT_FLAG_ROOM_NOT_FOUD;
	// An error for our own nickname keeps the participant: the user is about to leave the room.
	bool error = name == m_nickname && (participant.flag() & errorFlags);
	bool unavailable = !error && (participant.status() == pbnetwork::STATUS_NONE || !participant.newname().empty());

	if (unavailable) {
		m_participants.erase(participant.nickname());
	}
	else {
		m_participants[participant.nickname()] = participant;
	}

	doHandleParticipantChanged(participant);
}

}
