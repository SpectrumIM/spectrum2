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
#include "transport/Conversation.h"
#include "transport/ConversationManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/Buddy.h"
#include "transport/PresenceOracle.h"
#include "transport/RosterManager.h"
#include "transport/Frontend.h"
#include "transport/Config.h"
#include "transport/Logging.h"

#include "Swiften/Elements/MUCItem.h"
#include "Swiften/Elements/MUCOccupant.h"
#include "Swiften/Elements/MUCUserPayload.h"
#include "Swiften/Elements/Delay.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/VCardUpdate.h"

#include "Swiften/SwiftenCompat.h"
#ifdef SWIFTEN_SUPPORTS_CARBONS
#include "Swiften/Elements/CarbonsSent.h"
#include "Swiften/Elements/Forwarded.h"
#include "Swiften/Elements/HintPayload.h"
#endif

#ifdef SWIFTEN_SUPPORTS_PRIVILEGE
#include "Swiften/Elements/Privilege.h"
#endif

namespace Transport {
	
DEFINE_LOGGER(conversationLogger, "Conversation");

Conversation::Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) : m_conversationManager(conversationManager) {
	m_legacyName = legacyName;
	m_muc = isMUC;
	m_jid = m_conversationManager->getUser()->getJID().toBare();
	m_sentInitialPresence = false;
	m_nicknameChanged = false;
	m_mucEscaping = false;
	m_sentInitialSubject = false;

	if (CONFIG_BOOL_DEFAULTED(conversationManager->getComponent()->getConfig(), "features.rawxml", false)) {
		m_sentInitialPresence = true;
	}
}

Conversation::~Conversation() {
}

void Conversation::setMUCEscaping(bool mucEscaping) {
	LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Setting MUC escaping to " << mucEscaping);
	m_mucEscaping = mucEscaping;
}

void Conversation::destroyRoom() {
	if (m_muc) {
		Swift::Presence::ref presence = Swift::Presence::create();
		std::string legacyName = m_legacyName;
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);
		presence->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), m_nickname));
		presence->setType(Swift::Presence::Unavailable);

		Swift::MUCItem item;
		item.affiliation = Swift::MUCOccupant::NoAffiliation;
		item.role = Swift::MUCOccupant::NoRole;
		item.actor = "Transport";
		item.reason = "Spectrum 2 transport is being shut down.";
		Swift::MUCUserPayload *p = new Swift::MUCUserPayload ();
		p->addItem(item);

		Swift::MUCUserPayload::StatusCode c;
		c.code = 332;
		p->addStatusCode(c);
		Swift::MUCUserPayload::StatusCode c2;
		c2.code = 307;
		p->addStatusCode(c2);

		presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(p));
		BOOST_FOREACH(const Swift::JID &jid, m_jids) {
			presence->setTo(jid);
			m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);
		}
	}
}

void Conversation::setRoom(const std::string &room) {
	m_room = room;
	m_legacyName = m_room + "/" + m_legacyName;
}

void Conversation::cacheMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message) {
	boost::posix_time::ptime timestamp = boost::posix_time::second_clock::universal_time();
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Delay> delay(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::Delay>());
	delay->setStamp(timestamp);
	message->addPayload(delay);
	m_cachedMessages.push_back(message);
	if (m_cachedMessages.size() > 100) {
		m_cachedMessages.pop_front();
	}
}

void Conversation::handleRawMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message) {
	if (message->getType() != Swift::Message::Groupchat) {
		if (m_conversationManager->getComponent()->inServerMode() && m_conversationManager->getUser()->shouldCacheMessages()) {
			cacheMessage(message);
		}
		else {
			m_conversationManager->getComponent()->getFrontend()->sendMessage(message);
		}
	}
	else {
		if (m_jids.empty()) {
			cacheMessage(message);
		}
		else {
			BOOST_FOREACH(const Swift::JID &jid, m_jids) {
				message->setTo(jid);
				// Subject has to be sent after our own presence (the one with code 110)
				if (!message->getSubject().empty()) {
					m_subject = message;
					if (m_sentInitialPresence == false) {
						LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Caching subject message, initial presence not sent yet.");
						return;
					}
					else {
						LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Forwarding subject message.");
					}
				}
				m_conversationManager->getComponent()->getFrontend()->sendMessage(message);
			}
		}
	}
}

void Conversation::handleMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message, const std::string &nickname, const bool carbon) {
	if (m_muc) {
		message->setType(Swift::Message::Groupchat);
	}
	else {
		if (message->getType() == Swift::Message::Headline) {
			if (m_conversationManager->getUser()->getUserSetting("send_headlines") != "1") {
				message->setType(Swift::Message::Chat);
			}
		}
		else {
			message->setType(Swift::Message::Chat);
		}
	}

	std::string n = nickname;
	if (n.empty() && !m_room.empty() && !m_muc) {
		n = m_nickname;
	}

	if (message->getType() != Swift::Message::Groupchat) {
		message->setTo(m_jid);
		// normal message
		if (n.empty()) {
			Buddy *buddy = m_conversationManager->getUser()->getRosterManager()->getBuddy(m_legacyName);
			if (buddy) {
				message->setFrom(buddy->getJID());
			}
			else {
				std::string name = m_legacyName;
				if (CONFIG_BOOL_DEFAULTED(m_conversationManager->getComponent()->getConfig(), "service.jid_escaping", true)) {
					name = Swift::JID::getEscapedNode(m_legacyName);
				}
				else {
					if (name.find_last_of("@") != std::string::npos) {
						name.replace(name.find_last_of("@"), 1, "%");
					}
				}

				message->setFrom(Swift::JID(name, m_conversationManager->getComponent()->getJID().toBare(), "bot"));
			}
		}
		// PM message
		else {
			if (m_room.empty()) {
				message->setFrom(Swift::JID(n, m_conversationManager->getComponent()->getJID().toBare(), "user"));
			}
			else {
				std::string legacyName = m_room;
				if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
					legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
				}
				legacyName = Swift::JID::getEscapedNode(legacyName);
				message->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), n));
			}
		}
	}
	else {
		std::string legacyName = m_legacyName;
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);

		std::string n = nickname;
		if (n.empty()) {
			n = " ";
		}

		std::map<std::string, Participant>::iterator it = m_participants.find(n);
		if (it != m_participants.end() && !it->second.alias.empty()) {
			n = it->second.alias;
		}

		message->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), n));
		LOG4CXX_INFO(conversationLogger, "MSG FROM " << message->getFrom().toString());
	}


	if (carbon) {
#ifdef SWIFTEN_SUPPORTS_CARBONS
		LOG4CXX_DEBUG(conversationLogger, "CARBON MSG");
		//Swap from and to
		Swift::JID from = message->getTo();
		if (from.getResource().empty()) {
		    //If no resource is specified, set the same that is used for legacy network contacts
		    from = Swift::JID(from.getNode(), from.getDomain(), JID_DEFAULT_RESOURCE);
		}
		message->setTo(message->getFrom());
		message->setFrom(from);

		//Carbons should be sent to every resource directly.
		//Even if we tried to send to bare jid, the server would at best route it
		//as it would normal message (usually to the highest priority resource),
		//but won't produce carbons to other resources as it does with normal messages.
		Component* transport = this->getConversationManager()->getComponent();
		std::vector<Swift::Presence::ref> presences = transport->getPresenceOracle()->getAllPresence(this->m_jid.toBare());
		if (presences.empty()) {
			LOG4CXX_INFO(conversationLogger, "Carbon: No presences for JID " << this->m_jid.toString()
			    << ", will send to bare JID for archival.");
			this->forwardAsCarbonSent(message, this->m_jid.toBare());
		} else
		BOOST_FOREACH(const Swift::Presence::ref &it, presences) {
			this->forwardAsCarbonSent(message, it->getFrom());
		}
#else //!SWIFTEN_SUPPORTS_CARBONS
		//Ignore the message.
#endif
	} else {
		handleRawMessage(message);
	}
}

void Conversation::forwardAsCarbonSent(
	const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &payload,
	const Swift::JID& to)
{
#ifdef SWIFTEN_SUPPORTS_CARBONS
	LOG4CXX_INFO(conversationLogger, "Carbon <sent> to -> " << to.toString());

	//Message envelope
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message(new Swift::Message());

	//Type MUST be equal to the original message type
	message->setType(payload->getType());

	//XEP-0280 docs say "from" MUST be the carbon subscriber's JID,
	//so a transport will need to wrap this in another <privilege> wrapper.
	message->setFrom(m_jid.toBare());
	message->setTo(to);

	//Wrap the payload in a <sent><forwarded>
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Forwarded> forwarded(new Swift::Forwarded());
	forwarded->setStanza(payload);
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CarbonsSent> sent(new Swift::CarbonsSent());
	sent->setForwarded(forwarded);
	message->addPayload(sent);

	//Add no-copy to prevent some servers from creating carbons of carbons
	Swift::HintPayload::ref noCopy(new Swift::HintPayload(Swift::HintPayload::NoCopy));
	message->addPayload(noCopy);

	Component* transport = this->getConversationManager()->getComponent();
	if (transport->inServerMode()) {
		LOG4CXX_DEBUG(conversationLogger, "Sending carbon as is (server mode)");
		handleRawMessage(message);
	} else
		this->forwardImpersonated(message, Swift::JID("", message->getFrom().getDomain()));
#else
	//We cannot send the carbon.
#endif
}

//Generates a XEP-0356 privilege wrapper asking to impersonate a user from a given domain
void Conversation::forwardImpersonated(
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> payload,
	const Swift::JID& server)
{
#ifdef SWIFTEN_SUPPORTS_PRIVILEGE
	LOG4CXX_DEBUG(conversationLogger, "Impersonate to -> " << server.toString());
	Component* transport = this->getConversationManager()->getComponent();

	//Message envelope
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message(new Swift::Message());
	// "from" MUST be our bare jid
	message->setFrom(transport->getJID());
	// "to" MUST be the bare jid of the server we're asking to impersonate
	message->setTo(server);
	message->setType(Swift::Message::Normal);

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Forwarded> forwarded(new Swift::Forwarded());
	forwarded->setStanza(payload);
	
	Swift::Privilege::ref privilege(new Swift::Privilege());
	privilege->setForwarded(forwarded);

	message->addPayload(privilege);
	LOG4CXX_DEBUG(conversationLogger, "Impersonate: sending message");
	handleRawMessage(message);
#else
	//Try to send the message as is -- some servers can be configured to accept this
	handleRawMessage(payload);
#endif
}

std::string Conversation::getParticipants() {
	std::string ret;
	for (std::map<std::string, Participant>::iterator it = m_participants.begin(); it != m_participants.end(); it++) {
		ret += (*it).second.presence->getFrom().getResource() + ", ";
	}
	return ret;
}

void Conversation::sendParticipants(const Swift::JID &to, const std::string &nickname) {
	// When user tries to join this room from another resource using
	// different nickname than the original one has, we have to rename
	// him.
	LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Sending participants to " << to.toString() << ", Nickname:" << nickname << ", Conversation nickname:" << m_nickname);
	if (m_nickname != nickname && !nickname.empty()) {
		Swift::Presence::ref presence;
		std::string tmp = m_nickname;

		// At first connect the user.
		m_nickname = nickname;
		presence = generatePresence(nickname, 0, (int) Swift::StatusShow::Online, "", "", "");
		presence->setTo(to);
		m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);

		// Now change his nickname to the right one.
		m_nicknameChanged = true;
		presence = generatePresence(nickname, 0, (int) Swift::StatusShow::Online, "", tmp, "");
		presence->setTo(to);
		m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);

	}

	// Self presence has to be sent as first.
	Swift::Presence::ref presence = generatePresence(m_nickname, 0, (int) Swift::StatusShow::Online, "", "", "");
	presence->setTo(to);
	m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);

	for (std::map<std::string, Participant>::iterator it = m_participants.begin(); it != m_participants.end(); it++) {
		(*it).second.presence->setTo(to);
		m_conversationManager->getComponent()->getFrontend()->sendPresence((*it).second.presence);
	}
}

void Conversation::sendCachedMessages(const Swift::JID &to) {
	for (std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> >::const_iterator it = m_cachedMessages.begin(); it != m_cachedMessages.end(); it++) {
		if (to.isValid()) {
			(*it)->setTo(to);
		}
		else {
			(*it)->setTo(m_jid.toBare());
		}
		m_conversationManager->getComponent()->getFrontend()->sendMessage(*it);
	}

	if (m_subject) {
		if (to.isValid()) {
			m_subject->setTo(to);
		}
		else {
			m_subject->setTo(m_jid.toBare());
		}
		m_conversationManager->getComponent()->getFrontend()->sendMessage(m_subject);
	}

	m_cachedMessages.clear();
}

Swift::Presence::ref Conversation::generatePresence(const std::string &nick, int flag, int status, const std::string &statusMessage, const std::string &newname, const std::string &iconhash) {
	std::string nickname = nick;
	Swift::Presence::ref presence = Swift::Presence::create();
	std::string legacyName = m_legacyName;
	if (m_muc) {
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);
	}
	presence->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), nickname));
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	Swift::StatusShow s((Swift::StatusShow::Type) status);

	if (s.getType() == Swift::StatusShow::None) {
		presence->setType(Swift::Presence::Unavailable);
	}

	presence->setShow(s.getType());

	Swift::MUCUserPayload *p = new Swift::MUCUserPayload ();
	if (m_nickname == nickname) {
		if (flag & PARTICIPANT_FLAG_CONFLICT) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::Conflict)));
			LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Generating error presence: PARTICIPANT_FLAG_CONFLICT");
			return presence;
		}
		else if (flag & PARTICIPANT_FLAG_NOT_AUTHORIZED) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::NotAuthorized, Swift::ErrorPayload::Auth, statusMessage)));
			LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Generating error presence: PARTICIPANT_FLAG_NOT_AUTHORIZED");
			return presence;
		}
		else if (flag & PARTICIPANT_FLAG_ROOM_NOT_FOUD) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::ItemNotFound, Swift::ErrorPayload::Cancel, statusMessage)));
			LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Generating error presence: PARTICIPANT_FLAG_ROOM_NOT_FOUND");
			return presence;
		}
		else {
			Swift::MUCUserPayload::StatusCode c;
			c.code = 110;
			p->addStatusCode(c);
			if (m_nicknameChanged) {
				Swift::MUCUserPayload::StatusCode c;
				c.code = 210;
				p->addStatusCode(c);
				m_nicknameChanged = false;
			}
			m_sentInitialPresence = true;
		}
	}


	Swift::MUCItem item;
	
	item.affiliation = Swift::MUCOccupant::Member;
	item.role = Swift::MUCOccupant::Participant;

	if (flag & PARTICIPANT_FLAG_MODERATOR) {
		item.affiliation = Swift::MUCOccupant::Admin;
		item.role = Swift::MUCOccupant::Moderator;
	}

	if (!newname.empty()) {
		item.nick = newname;
		Swift::MUCUserPayload::StatusCode c;
		c.code = 303;
		p->addStatusCode(c);
		presence->setType(Swift::Presence::Unavailable);
	}

	if (!iconhash.empty()) {
		presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (iconhash)));
	}
	
	p->addItem(item);
	presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(p));
	return presence;
}


void Conversation::setNickname(const std::string &nickname) {
	if (!nickname.empty() && m_nickname != nickname) {
		m_nicknameChanged = true;
	}
	m_nickname = nickname;
}

void Conversation::handleRawPresence(Swift::Presence::ref presence) {
	// TODO: Detect nickname change.
	m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);
	m_participants[presence->getFrom().getResource()].presence = presence;
}

void Conversation::removeJID(const Swift::JID &jid) {
	if (m_muc) {
		Swift::Presence::ref presence = generatePresence(m_nickname, 0, Swift::StatusShow::None, "");
		presence->setTo(jid);
		m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);
	}
	m_jids.remove(jid);
}

void Conversation::handleParticipantChanged(const std::string &nick, Conversation::ParticipantFlag flag, int status, const std::string &statusMessage, const std::string &newname, const std::string &iconhash, const std::string &alias) {
	Swift::Presence::ref presence = generatePresence(alias.empty() ? nick : alias, flag, status, statusMessage, newname, iconhash);

	if (presence->getType() == Swift::Presence::Unavailable) {
		m_participants.erase(nick);
	}
	else {
		m_participants[nick].presence = presence;
		m_participants[nick].alias = alias;
	}

	BOOST_FOREACH(const Swift::JID &jid, m_jids) {
		presence->setTo(jid);
		m_conversationManager->getComponent()->getFrontend()->sendPresence(presence);
	}
	if (!newname.empty()) {
		handleParticipantChanged(newname, flag, status, statusMessage, "", iconhash);
	}

	if (m_sentInitialPresence && !m_sentInitialSubject && m_subject) {
		m_sentInitialSubject = true;
		m_conversationManager->getComponent()->getFrontend()->sendMessage(m_subject);
	}

	// We send error presences only to inform user that he is disconnected
	// from the room. This code must be extended in case we start sending error
	// presences in other situations.
	if (presence->getType() == Swift::Presence::Error) {
		LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Leaving the conversation " << m_legacyName << " because of error.");
		m_conversationManager->getUser()->leaveRoom(m_legacyName);
	}
}

}
