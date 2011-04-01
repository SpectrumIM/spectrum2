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
#include "transport/abstractconversation.h"
#include "transport/conversationmanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/abstractbuddy.h"
#include "transport/rostermanager.h"

namespace Transport {

AbstractConversation::AbstractConversation(ConversationManager *conversationManager, const std::string &legacyName) : m_conversationManager(conversationManager) {
	m_conversationManager->setConversation(this);
	m_legacyName = legacyName;
}

AbstractConversation::~AbstractConversation() {
	m_conversationManager->unsetConversation(this);
}

void AbstractConversation::handleMessage(boost::shared_ptr<Swift::Message> &message) {
	message->setTo(m_conversationManager->getUser()->getJID().toBare());
	AbstractBuddy *buddy = m_conversationManager->getUser()->getRosterManager()->getBuddy(m_legacyName);
	if (buddy) {
		std::cout << m_legacyName << " 222222\n";
		message->setFrom(buddy->getJID());
	}
	else {
		std::cout << m_legacyName << " 1111111\n";
		// TODO: escape from and setFrom
	}
	m_conversationManager->getComponent()->getStanzaChannel()->sendMessage(message);
}

}
