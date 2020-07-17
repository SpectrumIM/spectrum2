/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Base/Error.h"
#include <iostream>

#include <boost/bind.hpp>

namespace Swift {

namespace StanzaChannelUtils {
// 	struct PriorityLessThan {
// 		bool operator()(const ServerSession* s1, const ServerSession* s2) const {
// 			return s1->getPriority() < s2->getPriority();
// 		}
// 	};

	struct HasJID {
		HasJID(const JID& jid) : jid(jid) {}
		bool operator()(const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session) const {
			return session->getRemoteJID().equals(jid, JID::WithResource);
		}
		JID jid;
	};
}

void ServerStanzaChannel::addSession(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session) {
	sessions[session->getRemoteJID().toBare().toString()].push_back(session);
	session->onSessionFinished.connect(boost::bind(&ServerStanzaChannel::handleSessionFinished, this, _1, session));
	session->onElementReceived.connect(boost::bind(&ServerStanzaChannel::handleElement, this, _1, session));
	session->onDataRead.connect(boost::bind(&ServerStanzaChannel::handleDataRead, this, _1, session));
}

void ServerStanzaChannel::removeSession(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session) {
	session->onSessionFinished.disconnect(boost::bind(&ServerStanzaChannel::handleSessionFinished, this, _1, session));
	session->onElementReceived.disconnect(boost::bind(&ServerStanzaChannel::handleElement, this, _1, session));
	session->onDataRead.disconnect(boost::bind(&ServerStanzaChannel::handleDataRead, this, _1, session));
	std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> > &lst = sessions[session->getRemoteJID().toBare().toString()];
	lst.erase(std::remove(lst.begin(), lst.end(), session), lst.end());
}

void ServerStanzaChannel::sendIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<IQ> iq) {
	send(iq);
}

void ServerStanzaChannel::sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Message> message) {
	send(message);
}

void ServerStanzaChannel::sendPresence(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Presence> presence) {
	send(presence);
}

void ServerStanzaChannel::handleDataRead(const SafeByteArray &data, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> &session) {
	if (safeByteArrayToString(data).find("</stream:stream>") != std::string::npos) {
		Swift::Presence::ref presence = Swift::Presence::create();
		presence->setFrom(session->getRemoteJID());
		presence->setType(Swift::Presence::Unavailable);
		onPresenceReceived(presence);
	}
}
#if HAVE_SWIFTEN_3
void ServerStanzaChannel::finishSession(const JID& to, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ToplevelElement> element, bool last) {
#else
void ServerStanzaChannel::finishSession(const JID& to, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Element> element, bool last) {
#endif
	std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> > candidateSessions;
	for (std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> >::const_iterator i = sessions[to.toBare().toString()].begin(); i != sessions[to.toBare().toString()].end(); ++i) {
		candidateSessions.push_back(*i);
	}

	for (std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> >::const_iterator i = candidateSessions.begin(); i != candidateSessions.end(); ++i) {
		removeSession(*i);
		if (element) {
			(*i)->sendElement(element);
		}

		if (last && (*i)->getRemoteJID().isValid()) {
			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setFrom((*i)->getRemoteJID());
			presence->setType(Swift::Presence::Unavailable);
			onPresenceReceived(presence);
		}

		(*i)->finishSession();
// 		std::cout << "FINISH SESSION " << sessions[to.toBare().toString()].size() << "\n";
		if (last) {
			break;
		}
	}
}

std::string ServerStanzaChannel::getNewIQID() {
	return idGenerator.generateID();
}

void ServerStanzaChannel::send(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza) {
	JID to = stanza->getTo();
	assert(to.isValid());

	if (!stanza->getFrom().isValid()) {
		stanza->setFrom(m_jid);
	}

	// For a full JID, first try to route to a session with the full JID
	if (!to.isBare()) {
		std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> >::const_iterator i = std::find_if(sessions[stanza->getTo().toBare().toString()].begin(), sessions[stanza->getTo().toBare().toString()].end(), StanzaChannelUtils::HasJID(to));
		if (i != sessions[stanza->getTo().toBare().toString()].end()) {
			(*i)->sendElement(stanza);
			return;
		}
	}

	// Look for candidate sessions
	to = to.toBare();
	std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> > candidateSessions;
	for (std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> >::const_iterator i = sessions[stanza->getTo().toBare().toString()].begin(); i != sessions[stanza->getTo().toBare().toString()].end(); ++i) {
		if ((*i)->getRemoteJID().equals(to, JID::WithoutResource)) {
			candidateSessions.push_back(*i);
			(*i)->sendElement(stanza);
		}
	}
	if (candidateSessions.empty()) {
		return;
	}

	// Find the session with the highest priority
// 	std::vector<ServerSession*>::const_iterator i = std::max_element(sessions.begin(), sessions.end(), PriorityLessThan());
// 	(*i)->sendStanza(stanza);
	return;
}

void ServerStanzaChannel::handleSessionFinished(const boost::optional<Session::SessionError>&, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession>& session) {
	removeSession(session);

// 	if (!session->initiatedFinish()) {
		Swift::Presence::ref presence = Swift::Presence::create();
		presence->setFrom(session->getRemoteJID());
		presence->setType(Swift::Presence::Unavailable);
		onPresenceReceived(presence);
// 	}
}

void ServerStanzaChannel::handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Element> element, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession>& session) {
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Stanza>(element);
	if (!stanza) {
		return;
	}

	stanza->setFrom(session->getRemoteJID());

	if (!stanza->getFrom().isValid())
		return;
	

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Message> message = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Message>(stanza);
	if (message) {
		onMessageReceived(message);
		return;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Presence> presence = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Presence>(stanza);
	if (presence) {
		onPresenceReceived(presence);
		return;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<IQ> iq = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<IQ>(stanza);
	if (iq) {
		onIQReceived(iq);
		return;
	}
}

void ServerStanzaChannel::handleSessionInitialized() {
	onAvailableChanged(true);
}

}
