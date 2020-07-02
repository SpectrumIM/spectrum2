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

#include "transport/ConversationManager.h"
#include "transport/Conversation.h"
#include "transport/Buddy.h"
#include "transport/Factory.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Transport.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"

namespace Transport {

DEFINE_LOGGER(conversationManagerLogger, "ConversationManager");

ConversationManager::ConversationManager(User *user, Component *component){
	m_user = user;
	m_component = component;
}

ConversationManager::~ConversationManager() {
	deleteAllConversations();
}

void ConversationManager::sendCachedChatMessages() {
	for (std::map<std::string, Conversation *>::const_iterator it = m_convs.begin(); it != m_convs.end(); it++) {
		if ((*it).second->isMUC()) {
			continue;
		}
		(*it).second->sendCachedMessages();
	}
}

void ConversationManager::deleteAllConversations() {
	while (!m_convs.empty()) {
		LOG4CXX_INFO(conversationManagerLogger, m_user->getJID().toString() << ": Removing conversation " << (*m_convs.begin()).first);
		(*m_convs.begin()).second->destroyRoom();
		delete (*m_convs.begin()).second;
		m_convs.erase(m_convs.begin());
	}
}

Conversation *ConversationManager::getConversation(const std::string &name) {
	if (m_convs.find(name) != m_convs.end())
		return m_convs[name];

	if (name.find("/") == std::string::npos) {
		return NULL;
	}

	// handle PMs
	std::string room = name.substr(0, name.find("/"));
	std::string nick = name.substr(name.find("/") + 1);

	if (getConversation(room) == NULL) {
		return NULL;
	}

	return getConversation(nick);
}

void ConversationManager::addConversation(Conversation *conv) {
	m_convs[conv->getLegacyName()] = conv;
	LOG4CXX_INFO(conversationManagerLogger, m_user->getJID().toString() << ": Adding conversation " << conv->getLegacyName());
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
		if ((*it).second->isMUC()) {
			continue;
		}
		(*it).second->setJID(m_user->getJID().toBare());
	}
}

void ConversationManager::removeJID(const Swift::JID &jid) {
	std::vector<std::string> toRemove;
	for (std::map<std::string, Conversation *>::const_iterator it = m_convs.begin(); it != m_convs.end(); it++) {
		if (it->first.empty() || !it->second) {
			continue;
		}

		(*it).second->removeJID(jid);
		if (it->second->getJIDs().empty() && (*it).second->isMUC()) {
			toRemove.push_back(it->first);
		}
	}

	if (m_user->getUserSetting("stay_connected") != "1") {
		while (!toRemove.empty()) {
			LOG4CXX_INFO(conversationManagerLogger, m_user->getJID().toString() << ": Leaving room " << toRemove.back() << ".");
			m_user->leaveRoom(toRemove.back());
			toRemove.pop_back();
		}
	}
}

void ConversationManager::clearJIDs() {
	for (std::map<std::string, Conversation *>::const_iterator it = m_convs.begin(); it != m_convs.end(); it++) {
		(*it).second->clearJIDs();
	}
}

void ConversationManager::handleMessageReceived(Swift::Message::ref message) {
// 	std::string name = message->getTo().getUnescapedNode();
// 	if (name.find_last_of("%") != std::string::npos) { // OK when commented
// 		name.replace(name.find_last_of("%"), 1, "@"); // OK when commented
// 	}
	std::string name = Buddy::JIDToLegacyName(message->getTo(), m_user);
	if (name.empty()) {
		LOG4CXX_WARN(conversationManagerLogger, m_user->getJID().toString() << ": Tried to create empty conversation");
		return;
	}

	// create conversation if it does not exist.
	if (!m_convs[name]) {
		Conversation *conv = m_component->getFactory()->createConversation(this, name);
		addConversation(conv);
	}
	// if it exists and it's MUC, but this message is PM, get PM conversation or create new one.
	else if (m_convs[name]->isMUC() && message->getType() != Swift::Message::Groupchat) {
		std::string room_name = name;
		name = room_name + "/" + message->getTo().getResource();
		if (m_convs.find(name) == m_convs.end()) {
			Conversation *conv = m_component->getFactory()->createConversation(this, message->getTo().getResource());
			conv->setRoom(room_name);
			conv->setNickname(name);
			addConversation(conv);
		}
	}

	// update resource and send the message
	m_convs[name]->setJID(message->getFrom());
	m_convs[name]->sendMessage(message);
}

}
