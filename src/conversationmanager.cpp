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

#include "transport/conversationmanager.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/buddy.h"
#include "transport/factory.h"
#include "transport/user.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("ConversationManager");

ConversationManager::ConversationManager(User *user, Component *component){
	m_user = user;
	m_component = component;
}

ConversationManager::~ConversationManager() {
	while(!m_convs.empty()) {
		LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Removing conversation " << (*m_convs.begin()).first);
		delete (*m_convs.begin()).second;
		m_convs.erase(m_convs.begin());
	}
}

void ConversationManager::addConversation(Conversation *conv) {
	m_convs[conv->getLegacyName()] = conv;
	LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Adding conversation " << conv->getLegacyName());
}

void ConversationManager::removeConversation(Conversation *conv) {
	for (std::map<std::string, Conversation *>::const_iterator it = m_convs.begin(); it != m_convs.end(); it++) {
		if ((*it).second->getRoom() == conv->getLegacyName()) {
			(*it).second->setRoom("");
		}
	}
	m_convs.erase(conv->getLegacyName());
}

void ConversationManager::resetResources() {
	for (std::map<std::string, Conversation *>::const_iterator it = m_convs.begin(); it != m_convs.end(); it++) {
		(*it).second->setJID(m_user->getJID().toBare());
	}
}

void ConversationManager::handleMessageReceived(Swift::Message::ref message) {
// 	std::string name = message->getTo().getUnescapedNode();
// 	if (name.find_last_of("%") != std::string::npos) { // OK when commented
// 		name.replace(name.find_last_of("%"), 1, "@"); // OK when commented
// 	}
	std::string name = Buddy::JIDToLegacyName(message->getTo());
	if (name.empty()) {
		LOG4CXX_WARN(logger, m_user->getJID().toString() << ": Tried to create empty conversation");
		return;
	}

	// create conversation if it does not exist.
	if (!m_convs[name]) {
		m_convs[name] = m_component->getFactory()->createConversation(this, name);
	}
	// if it exists and it's MUC, but this message is PM, get PM conversation or create new one.
	else if (m_convs[name]->isMUC() && message->getType() != Swift::Message::Groupchat) {
		std::string room_name = name;
		name = message->getTo().getResource();
		if (!m_convs[name]) {
			m_convs[name] = m_component->getFactory()->createConversation(this, name);
			m_convs[name]->setRoom(room_name);
		}
	}

	// update resource and send the message
	m_convs[name]->setJID(message->getFrom());
	m_convs[name]->sendMessage(message);
}

}
