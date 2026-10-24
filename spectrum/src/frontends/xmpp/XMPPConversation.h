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

#include "transport/Conversation.h"

#include <string>
#include <list>
#include <map>
#include <vector>
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/Presence.h"

namespace Transport {

class ConversationManager;
class XMPPFrontend;

/// Conversation shown to XMPP clients: renders messages and room participants
/// as stanzas, tracks the user's resources and caches messages while nobody
/// can receive them.
class XMPPConversation : public Conversation {
	public:
		XMPPConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false);

		virtual ~XMPPConversation();

		/// Turns the message from the legacy network into a stanza (see handleMessage below).
		void handleMessage(pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message);

		/// Handles new message from Legacy network and forwards it to XMPP.

		/// \param message Message received from legacy network.
		/// \param nickname For MUC conversation this is nickname of room participant who sent this message.
		void handleMessage(std::shared_ptr<Swift::Message> &message, const std::string &nickname = "", const bool carbon = false);

		//Generates a carbon <sent> wrapper <message> around the given payload and delivers it
		void forwardAsCarbonSent(
			const std::shared_ptr<Swift::Message> &payload,
			const Swift::JID& to);

		//Generates a carbon <received> wrapper <message> around the given payload and delivers it
		void forwardAsCarbonReceived(
			const std::shared_ptr<Swift::Message> &payload,
			const Swift::JID& to);

		//Sends a <received> carbon of an incoming message to every resource except the one it was delivered to
		void forwardReceivedCopies(const std::shared_ptr<Swift::Message> &copy, const Swift::JID &delivered);

		//Resource an incoming 1:1 message is delivered to
		Swift::JID getDeliveryJID();

		//Generates a impersonation request <message> arount the given payload and delivers it
		void forwardImpersonated(
			std::shared_ptr<Swift::Message> payload,
			const Swift::JID& server);

		void handleRawMessage(std::shared_ptr<Swift::Message> &message);
		void handleRawPresence(Swift::Presence::ref presence);

		void setNickname(const std::string &nickname);

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

		void destroyRoom();

		void sendParticipants(const Swift::JID &to, const std::string &nickname);

		void sendCachedMessages();
		void sendCachedMessages(const Swift::JID &to);

		void setMUCEscaping(bool mucEscaping);

		/// Wraps the media (<img>) in the message's XHTML as XEP-0066 OOB payloads
		/// (service.oob_split, service.oob_replace_body).
		/// \return the messages to deliver.
		std::vector<std::shared_ptr<Swift::Message> > wrapIncomingMedia(std::shared_ptr<Swift::Message> &msg);

	protected:
		void doHandleParticipantChanged(const pbnetwork::Participant &participant);

	private:
		XMPPFrontend *getFrontend();
		Swift::Presence::ref generatePresence(const std::string &nick, int flag, int status, const std::string &statusMessage, const std::string &newname = "", const std::string &iconhash = "");
		void cacheMessage(std::shared_ptr<Swift::Message> &message);
		void addOobPayload(Swift::Message::ref message, const std::string &url, const std::string &description);

	private:
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
		std::shared_ptr<Swift::Message> m_subject;
		std::list<std::shared_ptr<Swift::Message> > m_cachedMessages;

		// Last presence of each participant, by nickname.
		std::map<std::string, Swift::Presence::ref> m_participantPresences;
};

}
