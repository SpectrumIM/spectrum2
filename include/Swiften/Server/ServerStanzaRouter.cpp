/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Server/ServerStanzaRouter.h>
#include <Swiften/Server/ServerSession.h>
#include <Swiften/Base/Algorithm.h>

#include <cassert>
#include <algorithm>

namespace Swift {

namespace StanzaRouter {
	struct PriorityLessThan {
		bool operator()(const ServerSession* s1, const ServerSession* s2) const {
			return s1->getPriority() < s2->getPriority();
		}
	};

	struct HasJID {
		HasJID(const JID& jid) : jid(jid) {}
		bool operator()(const ServerSession* session) const {
			return session->getJID().equals(jid, JID::WithResource);
		}
		JID jid;
	};
}

ServerStanzaRouter::ServerStanzaRouter() {
}

bool ServerStanzaRouter::routeStanza(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza) {
	JID to = stanza->getTo();
	assert(to.isValid());

	// For a full JID, first try to route to a session with the full JID
	if (!to.isBare()) {
		std::vector<ServerSession*>::const_iterator i = std::find_if(clientSessions_.begin(), clientSessions_.end(), StanzaRouter::HasJID(to));
		if (i != clientSessions_.end()) {
			(*i)->sendStanza(stanza);
			return true;
		}
	}

	// Look for candidate sessions
	to = to.toBare();
	std::vector<ServerSession*> candidateSessions;
	for (std::vector<ServerSession*>::const_iterator i = clientSessions_.begin(); i != clientSessions_.end(); ++i) {
		if ((*i)->getJID().equals(to, JID::WithoutResource) && (*i)->getPriority() >= 0) {
			candidateSessions.push_back(*i);
		}
	}
	if (candidateSessions.empty()) {
		return false;
	}

	// Find the session with the highest priority
	std::vector<ServerSession*>::const_iterator i = std::max_element(clientSessions_.begin(), clientSessions_.end(), StanzaRouter::PriorityLessThan());
	(*i)->sendStanza(stanza);
	return true;
}

void ServerStanzaRouter::addClientSession(ServerSession* clientSession) {
	clientSessions_.push_back(clientSession);
}

void ServerStanzaRouter::removeClientSession(ServerSession* clientSession) {
	erase(clientSessions_, clientSession);
}

}
