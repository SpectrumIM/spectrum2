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

#include "XMPPConversation.h"
#include "XMPPFrontend.h"
#include "PresenceOracle.h"
#include "jidutils.h"
#include "transport/ConversationManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/Buddy.h"
#include "transport/RosterManager.h"
#include "transport/Config.h"
#include "transport/Logging.h"
#include "transport/Util.h"

#include "Swiften/Elements/MUCItem.h"
#include "Swiften/Elements/MUCOccupant.h"
#include "Swiften/Elements/MUCUserPayload.h"
#include "Swiften/Elements/Delay.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/VCardUpdate.h"
#include "Swiften/Elements/CarbonsSent.h"
#include "Swiften/Elements/CarbonsReceived.h"
#include "Swiften/Elements/Forwarded.h"
#include "Swiften/Elements/HintPayload.h"
#include "Swiften/Elements/Privilege.h"
#include "Swiften/Elements/ChatState.h"
#include "Swiften/Elements/DeliveryReceipt.h"
#include "Swiften/Elements/AttentionPayload.h"
#include "Swiften/Elements/XHTMLIMPayload.h"
#include "Swiften/Elements/RawXMLPayload.h"
#include "Swiften/Elements/ErrorPayload.h"
#include <Swiften/Serializer/XML/XMLElement.h>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <regex>

namespace Transport {

DEFINE_LOGGER(conversationLogger, "Conversation");

XMPPConversation::XMPPConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) : Conversation(conversationManager, legacyName, isMUC) {
	m_jid = Swift::JID(conversationManager->getUser()->getUserName()).toBare();
	m_sentInitialPresence = false;
	m_nicknameChanged = false;
	m_mucEscaping = false;
	m_sentInitialSubject = false;

	if (CONFIG_BOOL_DEFAULTED(conversationManager->getComponent()->getConfig(), "features.rawxml", false)) {
		m_sentInitialPresence = true;
	}
}

XMPPConversation::~XMPPConversation() {
}

XMPPFrontend *XMPPConversation::getFrontend() {
	return static_cast<XMPPFrontend *>(getConversationManager()->getComponent()->getFrontend());
}

void XMPPConversation::setMUCEscaping(bool mucEscaping) {
	LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Setting MUC escaping to " << mucEscaping);
	m_mucEscaping = mucEscaping;
}

void XMPPConversation::destroyRoom() {
	if (isMUC()) {
		Swift::Presence::ref presence = Swift::Presence::create();
		std::string legacyName = getLegacyName();
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);
		presence->setFrom(Swift::JID(legacyName, getConversationManager()->getComponent()->getJID(), getNickname()));
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

		presence->addPayload(std::shared_ptr<Swift::Payload>(p));
		for (const Swift::JID &jid : m_jids) {
			presence->setTo(jid);
			getFrontend()->sendPresence(presence);
		}
	}
}

void XMPPConversation::cacheMessage(std::shared_ptr<Swift::Message> &message) {
	boost::posix_time::ptime timestamp = boost::posix_time::second_clock::universal_time();
	std::shared_ptr<Swift::Delay> delay(std::make_shared<Swift::Delay>());
	delay->setStamp(timestamp);
	message->addPayload(delay);
	m_cachedMessages.push_back(message);
	if (m_cachedMessages.size() > 100) {
		m_cachedMessages.pop_front();
	}
}

void XMPPConversation::handleRawMessage(std::shared_ptr<Swift::Message> &message) {
	if (message->getType() != Swift::Message::Groupchat) {
		if (getConversationManager()->getComponent()->inServerMode() && getConversationManager()->getUser()->shouldCacheMessages()) {
			cacheMessage(message);
		}
		else {
			getFrontend()->sendMessage(message);
		}
	}
	else {
		if (m_jids.empty()) {
			cacheMessage(message);
		}
		else {
			for (const Swift::JID &jid : m_jids) {
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
				getFrontend()->sendMessage(message);
			}
		}
	}
}

void XMPPConversation::handleMessage(pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &payload) {
	std::shared_ptr<Swift::Message> msg(new Swift::Message());

	switch (type) {
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING:
			msg->addPayload(std::make_shared<Swift::ChatState>(Swift::ChatState::Composing));
			handleMessage(msg);
			return;
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED:
			msg->addPayload(std::make_shared<Swift::ChatState>(Swift::ChatState::Paused));
			handleMessage(msg);
			return;
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING:
			msg->addPayload(std::make_shared<Swift::ChatState>(Swift::ChatState::Active));
			handleMessage(msg);
			return;
		case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE_ACK:
			msg->addPayload(std::make_shared<Swift::DeliveryReceipt>(payload.id()));
			handleMessage(msg);
			return;
		case pbnetwork::WrapperMessage_Type_TYPE_ATTENTION:
			msg->setBody(payload.message());
			msg->addPayload(std::make_shared<Swift::AttentionPayload>());
			handleMessage(msg);
			return;
		case pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED:
			msg->setSubject(payload.message());
			break;
		case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE:
			msg->setBody(payload.message());
			break;
		default:
			return;
	}

	// Add xhtml-im payload.
	if (CONFIG_BOOL(getConversationManager()->getComponent()->getConfig(), "service.enable_xhtml") && !payload.xhtml().empty()) {
		msg->addPayload(std::make_shared<Swift::XHTMLIMPayload>(payload.xhtml()));
	}

	for (const pbnetwork::Attachment &att : payload.attachment()) {
		addOobPayload(msg, att.url(), att.description());
	}

	// Split the message if configured, or just preprocess
	LOG4CXX_TRACE(conversationLogger, "handleMessage: wrapping media");
	typedef std::vector<std::shared_ptr<Swift::Message> > MsgList;
	MsgList msgs = wrapIncomingMedia(msg);

	// Forward all parts
	for (MsgList::iterator it=msgs.begin(); it!=msgs.end(); it++) {
		if (payload.headline()) {
			(*it)->setType(Swift::Message::Headline);
		}

		(*it)->addPayload(std::make_shared<Swift::ChatState>(Swift::ChatState::Active));

		if (!payload.timestamp().empty()) {
			boost::posix_time::ptime timestamp = boost::posix_time::from_iso_string(payload.timestamp());
			std::shared_ptr<Swift::Delay> delay(std::make_shared<Swift::Delay>());
			delay->setStamp(timestamp);
			(*it)->addPayload(delay);
		}

		handleMessage((*it), payload.nickname(), payload.carbon());
	}
}

void XMPPConversation::handleMessage(std::shared_ptr<Swift::Message> &message, const std::string &nickname, const bool carbon) {
	if (isMUC()) {
		message->setType(Swift::Message::Groupchat);
	}
	else {
		if (message->getType() == Swift::Message::Headline) {
			if (getConversationManager()->getUser()->getUserSetting("send_headlines") != "1") {
				message->setType(Swift::Message::Chat);
			}
		}
		else {
			message->setType(Swift::Message::Chat);
		}
	}

	const std::string &transportJID = getConversationManager()->getComponent()->getJID();

	std::string n = nickname;
	if (n.empty() && !getRoom().empty() && !isMUC()) {
		n = getNickname();
	}

	if (message->getType() != Swift::Message::Groupchat) {
		message->setTo(m_jid);
		// normal message
		if (n.empty()) {
			Buddy *buddy = getConversationManager()->getUser()->getRosterManager()->getBuddy(getLegacyName());
			if (buddy) {
				message->setFrom(JIDUtils::getBuddyJID(buddy));
			}
			else {
				std::string name = getLegacyName();
				if (CONFIG_BOOL_DEFAULTED(getConversationManager()->getComponent()->getConfig(), "service.jid_escaping", true)) {
					name = Swift::JID::getEscapedNode(getLegacyName());
				}
				else {
					if (name.find_last_of("@") != std::string::npos) {
						name.replace(name.find_last_of("@"), 1, "%");
					}
				}

				message->setFrom(Swift::JID(name, transportJID, "bot"));
			}
		}
		// PM message
		else {
			if (getRoom().empty()) {
				message->setFrom(Swift::JID(n, transportJID, "user"));
			}
			else {
				std::string legacyName = getRoom();
				if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
					legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
				}
				legacyName = Swift::JID::getEscapedNode(legacyName);
				message->setFrom(Swift::JID(legacyName, transportJID, n));
			}
		}
	}
	else {
		std::string legacyName = getLegacyName();
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);

		std::string n = nickname;
		if (n.empty()) {
			n = " ";
		}

		const std::map<std::string, pbnetwork::Participant> &participants = getParticipantList();
		std::map<std::string, pbnetwork::Participant>::const_iterator it = participants.find(n);
		if (it != participants.end() && !it->second.alias().empty()) {
			n = it->second.alias();
		}

		message->setFrom(Swift::JID(legacyName, transportJID, n));
		LOG4CXX_INFO(conversationLogger, "MSG FROM " << message->getFrom().toString());
	}


	if (carbon) {
		LOG4CXX_DEBUG(conversationLogger, "CARBON MSG");
		//Swap from and to. The message was sent from a legacy client, not from any XMPP resource,
		//so it comes from the user's bare JID.
		Swift::JID from = message->getTo().toBare();
		message->setTo(message->getFrom());
		message->setFrom(from);

		//Carbons should be sent to every resource directly.
		//Even if we tried to send to bare jid, the server would at best route it
		//as it would normal message (usually to the highest priority resource),
		//but won't produce carbons to other resources as it does with normal messages.
		std::vector<Swift::Presence::ref> presences = getFrontend()->getPresenceOracle()->getAllPresence(this->m_jid.toBare());
		if (presences.empty()) {
			LOG4CXX_INFO(conversationLogger, "Carbon: No presences for JID " << this->m_jid.toString()
			    << ", will send to bare JID for archival.");
			this->forwardAsCarbonSent(message, this->m_jid.toBare());
		} else
		for (const Swift::Presence::ref &it : presences) {
			this->forwardAsCarbonSent(message, it->getFrom());
		}
	} else {
		// In gateway mode the XMPP server copies the message to the other resources itself
		bool copyToOthers = getConversationManager()->getComponent()->inServerMode()
			&& !isMUC() && message->getType() != Swift::Message::Groupchat;
		std::shared_ptr<Swift::Message> copy;
		if (copyToOthers) {
			copy = std::shared_ptr<Swift::Message>(new Swift::Message(*message));
		}
		handleRawMessage(message);
		if (copyToOthers) {
			forwardReceivedCopies(copy, getDeliveryJID());
		}
	}
}

// The resource an incoming 1:1 message goes to: m_jid, or the highest-priority resource while m_jid is still bare
Swift::JID XMPPConversation::getDeliveryJID() {
	if (this->m_jid.isBare()) {
		Swift::Presence::ref highest = getFrontend()->getPresenceOracle()->getHighestPriorityPresence(this->m_jid.toBare());
		if (highest) {
			return highest->getFrom();
		}
	}
	return this->m_jid;
}

// Every resource of the user except the one the message was delivered to gets a XEP-0280 <received> carbon
void XMPPConversation::forwardReceivedCopies(const std::shared_ptr<Swift::Message> &copy, const Swift::JID &delivered) {
	std::vector<Swift::Presence::ref> presences = getFrontend()->getPresenceOracle()->getAllPresence(this->m_jid.toBare());
	for (const Swift::Presence::ref &it : presences) {
		if (it->getFrom() == delivered) {
			continue;
		}
		this->forwardAsCarbonReceived(copy, it->getFrom());
	}
}

void XMPPConversation::forwardAsCarbonReceived(
	const std::shared_ptr<Swift::Message> &payload,
	const Swift::JID& to)
{
	LOG4CXX_INFO(conversationLogger, "Carbon <received> to -> " << to.toString());

	//Message envelope
	std::shared_ptr<Swift::Message> message(new Swift::Message());
	message->setType(payload->getType());
	message->setFrom(m_jid.toBare());
	message->setTo(to);

	//Wrap the payload in a <received><forwarded>
	std::shared_ptr<Swift::Forwarded> forwarded(new Swift::Forwarded());
	forwarded->setStanza(payload);
	std::shared_ptr<Swift::CarbonsReceived> received(new Swift::CarbonsReceived());
	received->setForwarded(forwarded);
	message->addPayload(received);

	//Add no-copy to prevent some servers from creating carbons of carbons
	Swift::HintPayload::ref noCopy(new Swift::HintPayload(Swift::HintPayload::NoCopy));
	message->addPayload(noCopy);

	// Only used in server mode: in gateway mode the XMPP server generates the carbons
	handleRawMessage(message);
}

void XMPPConversation::forwardAsCarbonSent(
	const std::shared_ptr<Swift::Message> &payload,
	const Swift::JID& to)
{
	LOG4CXX_INFO(conversationLogger, "Carbon <sent> to -> " << to.toString());

	//Message envelope
	std::shared_ptr<Swift::Message> message(new Swift::Message());

	//Type MUST be equal to the original message type
	message->setType(payload->getType());

	//XEP-0280 docs say "from" MUST be the carbon subscriber's JID,
	//so a transport will need to wrap this in another <privilege> wrapper.
	message->setFrom(m_jid.toBare());
	message->setTo(to);

	//Wrap the payload in a <sent><forwarded>
	std::shared_ptr<Swift::Forwarded> forwarded(new Swift::Forwarded());
	forwarded->setStanza(payload);
	std::shared_ptr<Swift::CarbonsSent> sent(new Swift::CarbonsSent());
	sent->setForwarded(forwarded);
	message->addPayload(sent);

	//Add no-copy to prevent some servers from creating carbons of carbons
	Swift::HintPayload::ref noCopy(new Swift::HintPayload(Swift::HintPayload::NoCopy));
	message->addPayload(noCopy);

	if (getConversationManager()->getComponent()->inServerMode()) {
		LOG4CXX_DEBUG(conversationLogger, "Sending carbon as is (server mode)");
		handleRawMessage(message);
	} else
		this->forwardImpersonated(message, Swift::JID("", message->getFrom().getDomain()));
}

//Generates a XEP-0356 privilege wrapper asking to impersonate a user from a given domain
void XMPPConversation::forwardImpersonated(
	std::shared_ptr<Swift::Message> payload,
	const Swift::JID& server)
{
	LOG4CXX_DEBUG(conversationLogger, "Impersonate to -> " << server.toString());

	//Message envelope
	std::shared_ptr<Swift::Message> message(new Swift::Message());
	// "from" MUST be our bare jid
	message->setFrom(Swift::JID(getConversationManager()->getComponent()->getJID()));
	// "to" MUST be the bare jid of the server we're asking to impersonate
	message->setTo(server);
	message->setType(Swift::Message::Normal);

	std::shared_ptr<Swift::Forwarded> forwarded(new Swift::Forwarded());
	forwarded->setStanza(payload);

	Swift::Privilege::ref privilege(new Swift::Privilege());
	privilege->setForwarded(forwarded);

	message->addPayload(privilege);
	LOG4CXX_DEBUG(conversationLogger, "Impersonate: sending message");
	handleRawMessage(message);
}

void XMPPConversation::sendParticipants(const Swift::JID &to, const std::string &nickname) {
	// When user tries to join this room from another resource using
	// different nickname than the original one has, we have to rename
	// him.
	LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Sending participants to " << to.toString() << ", Nickname:" << nickname << ", Conversation nickname:" << getNickname());
	if (getNickname() != nickname && !nickname.empty()) {
		Swift::Presence::ref presence;
		std::string tmp = getNickname();

		// At first connect the user.
		Conversation::setNickname(nickname);
		presence = generatePresence(nickname, 0, (int) Swift::StatusShow::Online, "", "", "");
		presence->setTo(to);
		getFrontend()->sendPresence(presence);

		// Now change his nickname to the right one.
		m_nicknameChanged = true;
		presence = generatePresence(nickname, 0, (int) Swift::StatusShow::Online, "", tmp, "");
		presence->setTo(to);
		getFrontend()->sendPresence(presence);

	}

	// Self presence has to be sent as first.
	Swift::Presence::ref presence = generatePresence(getNickname(), 0, (int) Swift::StatusShow::Online, "", "", "");
	presence->setTo(to);
	getFrontend()->sendPresence(presence);

	for (std::map<std::string, Swift::Presence::ref>::iterator it = m_participantPresences.begin(); it != m_participantPresences.end(); it++) {
		(*it).second->setTo(to);
		getFrontend()->sendPresence((*it).second);
	}
}

void XMPPConversation::sendCachedMessages() {
	sendCachedMessages(Swift::JID());
}

void XMPPConversation::sendCachedMessages(const Swift::JID &to) {
	for (std::list<std::shared_ptr<Swift::Message> >::const_iterator it = m_cachedMessages.begin(); it != m_cachedMessages.end(); it++) {
		if (to.isValid()) {
			(*it)->setTo(to);
		}
		else {
			(*it)->setTo(m_jid.toBare());
		}
		getFrontend()->sendMessage(*it);
	}

	if (m_subject) {
		if (to.isValid()) {
			m_subject->setTo(to);
		}
		else {
			m_subject->setTo(m_jid.toBare());
		}
		getFrontend()->sendMessage(m_subject);
	}

	m_cachedMessages.clear();
}

Swift::Presence::ref XMPPConversation::generatePresence(const std::string &nick, int flag, int status, const std::string &statusMessage, const std::string &newname, const std::string &iconhash) {
	std::string nickname = nick;
	Swift::Presence::ref presence = Swift::Presence::create();
	std::string legacyName = getLegacyName();
	if (isMUC()) {
		if (!m_mucEscaping && legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		legacyName = Swift::JID::getEscapedNode(legacyName);
	}
	presence->setFrom(Swift::JID(legacyName, getConversationManager()->getComponent()->getJID(), nickname));
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	Swift::StatusShow s((Swift::StatusShow::Type) status);

	if (s.getType() == Swift::StatusShow::None) {
		presence->setType(Swift::Presence::Unavailable);
	}

	presence->setShow(s.getType());

	Swift::MUCUserPayload *p = new Swift::MUCUserPayload ();
	if (getNickname() == nickname) {
		if (flag & PARTICIPANT_FLAG_CONFLICT) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::Conflict)));
			LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Generating error presence: PARTICIPANT_FLAG_CONFLICT");
			return presence;
		}
		else if (flag & PARTICIPANT_FLAG_NOT_AUTHORIZED) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::NotAuthorized, Swift::ErrorPayload::Auth, statusMessage)));
			LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Generating error presence: PARTICIPANT_FLAG_NOT_AUTHORIZED");
			return presence;
		}
		else if (flag & PARTICIPANT_FLAG_ROOM_NOT_FOUD) {
			delete p;
			presence->setType(Swift::Presence::Error);
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::ErrorPayload(Swift::ErrorPayload::ItemNotFound, Swift::ErrorPayload::Cancel, statusMessage)));
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
		presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (iconhash)));
	}

	p->addItem(item);
	presence->addPayload(std::shared_ptr<Swift::Payload>(p));
	return presence;
}


void XMPPConversation::setNickname(const std::string &nickname) {
	if (!nickname.empty() && getNickname() != nickname) {
		m_nicknameChanged = true;
	}
	Conversation::setNickname(nickname);
}

void XMPPConversation::handleRawPresence(Swift::Presence::ref presence) {
	// TODO: Detect nickname change.
	getFrontend()->sendPresence(presence);
	m_participantPresences[presence->getFrom().getResource()] = presence;
}

void XMPPConversation::removeJID(const Swift::JID &jid) {
	if (isMUC()) {
		Swift::Presence::ref presence = generatePresence(getNickname(), 0, Swift::StatusShow::None, "");
		presence->setTo(jid);
		getFrontend()->sendPresence(presence);
	}
	m_jids.remove(jid);
}

void XMPPConversation::doHandleParticipantChanged(const pbnetwork::Participant &participant) {
	const std::string &nick = participant.nickname();
	const std::string &alias = participant.alias();
	Swift::Presence::ref presence = generatePresence(alias.empty() ? nick : alias, participant.flag(), participant.status(), participant.statusmessage(), participant.newname(), participant.iconhash());

	if (presence->getType() == Swift::Presence::Unavailable) {
		m_participantPresences.erase(nick);
	}
	else {
		m_participantPresences[nick] = presence;
	}

	for (const Swift::JID &jid : m_jids) {
		presence->setTo(jid);
		getFrontend()->sendPresence(presence);
	}
	if (!participant.newname().empty()) {
		pbnetwork::Participant renamed(participant);
		renamed.set_nickname(participant.newname());
		renamed.clear_newname();
		renamed.clear_alias();
		handleParticipantChanged(renamed);
	}

	if (m_sentInitialPresence && !m_sentInitialSubject && m_subject) {
		m_sentInitialSubject = true;
		getFrontend()->sendMessage(m_subject);
	}

	// We send error presences only to inform user that he is disconnected
	// from the room. This code must be extended in case we start sending error
	// presences in other situations.
	if (presence->getType() == Swift::Presence::Error) {
		LOG4CXX_INFO(conversationLogger, m_jid.toString() << ": Leaving the conversation " << getLegacyName() << " because of error.");
		getConversationManager()->getUser()->leaveRoom(getLegacyName());
	}
}

// Trims linebreaks and spaces from XHTML chunk of text
inline std::string xhtml_trim(std::string &s) {
	static const std::string t[] = {" ", "\n", "\t", "<br>", "<br/>"};
	bool match = true;
	while (match) {
		match = false;
		for (int i=0; i<sizeof(t)/sizeof(t[0]); i++)
			if (Util::iStartsWith(s, t[i])) {
				s.erase(0, t[i].size());
				match = true;
			}
	}
	match = true;
	while (match) {
		match = false;
		for (int i=0; i<sizeof(t)/sizeof(t[0]); i++)
			if (Util::iEndsWith(s, t[i])) {
				s.erase(s.size() - t[i].size());
				match = true;
			}
	}
	return s;
}
//Same for plaintext
static std::string plaintext_trim(std::string &text) {
	static std::string t = " \n\t";
	text.erase(text.find_last_not_of(t) + 1);
	text.erase(0, text.find_first_not_of(t));
	return text;
}

//Duplicates a Swift::Message and replaces its body and XHTML parts
static Swift::Message::ref copySwiftMessage(const Swift::Message* msg, const std::string& xhtml, const std::string& body) {
    Swift::Message::ref this_msg(new Swift::Message());
    //Copy all payloads by reference, except for body and XHTML
    std::vector<Swift::Payload::ref> payloads = msg->getPayloads();
    for (size_t i=0; i<payloads.size(); i++) {
        if(!dynamic_cast<Swift::Body*>(payloads[i].get()) && !dynamic_cast<Swift::XHTMLIMPayload*>(payloads[i].get()))
            this_msg->addPayload(payloads[i]);
    }
    //Add new body and XHTML tags
    this_msg->setBody(body);
    this_msg->addPayload(std::make_shared<Swift::XHTMLIMPayload>(xhtml));
    LOG4CXX_TRACE(conversationLogger, "Adding partial message: '" << xhtml << "', '" << body << "'");
    return this_msg;
}

/*
Process all media links (<img>, ..) in the message XHTML and apply required wrappers
and mitigations to ensure broader support for inplace display.
Returns a list of messages to deliver.

Copies any additional payloads into each of messages by reference (modify one == modify all).

Possible modes:
*/
enum OobMode {
	OobWrapAll	= 1,	//1. Wrap all media as OOB tags. (Standards-compliant)
	OobExclusive	= 2,	//2. Make the first media the only content of the message.
	OobSplit	= 3,	//3. Split into multiple text-only/media-only messages.
};

void XMPPConversation::addOobPayload(Swift::Message::ref message, const std::string &url, const std::string &description) {
	const std::string OOB_NAMESPACE = "jabber:x:oob";
	std::shared_ptr<Swift::XMLElement> oobElement = std::make_shared<Swift::XMLElement>("x", OOB_NAMESPACE);
	std::shared_ptr<Swift::XMLElement> urlNode = std::make_shared<Swift::XMLElement>("url", "", url);
	oobElement->addNode(urlNode);
	if (!description.empty()) {
		std::shared_ptr<Swift::XMLElement> descNode = std::make_shared<Swift::XMLElement>("desc");
		oobElement->addNode(descNode);
	}
	std::shared_ptr<Swift::RawXMLPayload> oobPayload = std::make_shared<Swift::RawXMLPayload>(oobElement->serialize());
	message->addPayload(oobPayload);
}

std::vector<std::shared_ptr<Swift::Message> >
XMPPConversation::wrapIncomingMedia(std::shared_ptr<Swift::Message>& msg) {
    std::vector<std::shared_ptr<Swift::Message> > result;

    std::shared_ptr<Swift::XHTMLIMPayload> xhtmlPayload =
        msg->getPayload<Swift::XHTMLIMPayload>();
    if (!xhtmlPayload) { //XHTML not available or disabled
        result.push_back(msg);
        return result;
    }

    const std::string& xhtml = xhtmlPayload->getBody();
    if (xhtml.find("<img") == std::string::npos) { //Clearly no media
        result.push_back(msg);
        return result;
    }
    const std::string body = msg->getBody().get_value_or("");

    Config *config = getConversationManager()->getComponent()->getConfig();
    OobMode oobMode = OobWrapAll;
    if (CONFIG_BOOL_DEFAULTED(config, "service.oob_split", false))
        //Split the message into parts so that each part only contains one media instance or one chunk of text
        oobMode = OobSplit;
    else if (CONFIG_BOOL_DEFAULTED(config, "service.oob_replace_body", false))
        // Some clients require plaintext to contain only the OOB URL to be displayed
        // This is not required by XEP and we lose parts of plaintext (e.g. captions).
        oobMode = OobExclusive;

    LOG4CXX_TRACE(conversationLogger, "wrapIncomingMedia: mode=" << (int) oobMode);
    LOG4CXX_TRACE(conversationLogger, "xhtml = " << xhtml);
    LOG4CXX_TRACE(conversationLogger, "body = " << body);


    //Find all <img...> entries
    //Handles: spaces, unrelated tags
    //Doesn't handle: unquoted src, escaped "'>s, quotes in quotes ("quote: 'text' end quote")
    static std::regex image_expr("<img\\s+[^>]*src\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>");

    int matchCount = 0;
    std::string firstUrl;

    std::string::const_iterator xhtml_pos = xhtml.begin();
    std::string::size_type body_pos = 0;

    std::smatch match;
    while(std::regex_search(xhtml_pos, xhtml.end(), match, image_expr)) {
        matchCount++;
        const std::string& image_tag = match[0];
        const std::string& image_url = match[1];
        if (firstUrl.empty())
            firstUrl = image_url;
        LOG4CXX_TRACE(conversationLogger, "match: image_tag=" << image_tag << ", image_url="<< image_url);

        //Process the part before the match
        if ((oobMode == OobSplit) && (match[0].first != xhtml_pos)) {
            std::string xhtml_prev(xhtml_pos, match[0].first);
            xhtml_trim(xhtml_prev);

            //Find the same URI in the plaintext body
            std::string body_prev = "";
            std::string::size_type body_match = body.find(image_url, body_pos);
            if (body_match != std::string::npos) {
                body_prev = body.substr(body_pos, body_match-body_pos);
                plaintext_trim(body_prev);
                body_pos = body_match + image_url.size();
            }

            //If anything of that is not empty, post as a message
            if (!xhtml_prev.empty() || !body_prev.empty())
                result.push_back(copySwiftMessage(msg.get(), xhtml_prev, body_prev));
        }

        //Now the match
        std::shared_ptr<Swift::Message> this_msg;
        if (oobMode == OobSplit) {
            this_msg = copySwiftMessage(msg.get(), image_tag, image_url);
            result.push_back(this_msg);
        } else {
            this_msg = msg;
        }

        addOobPayload(this_msg, image_url, "");

        //In single-OOB mode there's no point to process further media
        if (oobMode == OobExclusive)
            break;

        xhtml_pos = match[0].second;
    }

    //Post the text remainder
    if ((oobMode == OobSplit) && (xhtml_pos != xhtml.end())) {
        std::string xhtml_prev(xhtml_pos, xhtml.end());
        xhtml_trim(xhtml_prev);

        std::string body_prev = body.substr(body_pos, body.size()-body_pos);
        plaintext_trim(body_prev);

        if (!xhtml_prev.empty() || !body_prev.empty())
            result.push_back(copySwiftMessage(msg.get(), xhtml_prev, body_prev));
    }

    LOG4CXX_DEBUG(conversationLogger, "wrapIncomingMedia: matchCount==" << matchCount);

    if (oobMode != OobSplit)
        result.push_back(msg); //Push the non-split message only

    if (matchCount==0) {
        LOG4CXX_WARN(conversationLogger, "xhtml seems to contain an image, but doesn't match: " + xhtml);
    } else {
        // Replace the plaintext.
        // Normally it's up to the backend to provide us with <body> matching the <xhtml> version.
        if (oobMode == OobExclusive)
            msg->setBody(firstUrl);
    }
    return result;
}

}
