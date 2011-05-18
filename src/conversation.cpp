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

#include <iostream>
#include "transport/conversation.h"
#include "transport/conversationmanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/buddy.h"
#include "transport/rostermanager.h"

namespace Transport {

Conversation::Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) : m_conversationManager(conversationManager) {
	m_legacyName = legacyName;
	m_conversationManager->setConversation(this);
	m_muc = isMUC;
}

Conversation::~Conversation() {
	m_conversationManager->unsetConversation(this);
}

void Conversation::handleMessage(boost::shared_ptr<Swift::Message> &message, const std::string &nickname) {
	if (m_muc) {
		message->setType(Swift::Message::Groupchat);
	}
	else {
		message->setType(Swift::Message::Chat);
	}
	if (message->getType() != Swift::Message::Groupchat) {
		
		message->setTo(m_conversationManager->getUser()->getJID().toBare());
		// normal message
		if (nickname.empty()) {
			Buddy *buddy = m_conversationManager->getUser()->getRosterManager()->getBuddy(m_legacyName);
			if (buddy) {
				std::cout << m_legacyName << " 222222\n";
				message->setFrom(buddy->getJID());
			}
			else {
				std::cout << m_legacyName << " 1111111\n";
				// TODO: escape from and setFrom
			}
		}
		// PM message
		else {
			if (m_room.empty()) {
				message->setFrom(Swift::JID(nickname, m_conversationManager->getComponent()->getJID().toBare(), "user"));
			}
			else {
				message->setFrom(Swift::JID(m_room, m_conversationManager->getComponent()->getJID().toBare(), nickname));
			}
		}
		m_conversationManager->getComponent()->getStanzaChannel()->sendMessage(message);
	}
	else {
		message->setTo(m_conversationManager->getUser()->getJID().toString());
		message->setFrom(Swift::JID(m_legacyName, m_conversationManager->getComponent()->getJID().toBare(), nickname));
		m_conversationManager->getComponent()->getStanzaChannel()->sendMessage(message);
	}
}

void Conversation::handleParticipantChanged(const std::string &nick, int flag, const std::string &newname) {
	std::string nickname = nick;
	if (nickname.find("@") == 0) {
		nickname = nickname.substr(1);
	}
	Swift::Presence::ref presence = Swift::Presence::create();
 	presence->setFrom(Swift::JID(m_legacyName, m_conversationManager->getComponent()->getJID().toBare(), nickname));
	presence->setTo(m_conversationManager->getUser()->getJID().toString());
	presence->setType(Swift::Presence::Available);

	Swift::MUCUserPayload *p = new Swift::MUCUserPayload ();
	if (m_nickname == nickname) {
		Swift::MUCUserPayload::StatusCode c;
		c.code = 110;
		p->addStatusCode(c);
	}

	Swift::MUCUserPayload::Item item(Swift::MUCOccupant::Member, Swift::MUCOccupant::Participant);
	if (!newname.empty()) {
		item.nick = newname;
		Swift::MUCUserPayload::StatusCode c;
		c.code = 303;
		p->addStatusCode(c);
	}

	p->addItem(item);

	presence->addPayload(boost::shared_ptr<Swift::Payload>(p));
	m_conversationManager->getComponent()->getStanzaChannel()->sendPresence(presence);
}

}
