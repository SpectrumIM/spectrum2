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

#include "transport/presenceoracle.h"
#include "Swiften/Elements/MUCPayload.h"

#include <boost/bind.hpp>

using namespace Swift;

namespace Transport {

PresenceOracle::PresenceOracle(StanzaChannel* stanzaChannel) {
	stanzaChannel_ = stanzaChannel;
	stanzaChannel_->onPresenceReceived.connect(boost::bind(&PresenceOracle::handleIncomingPresence, this, _1));
	stanzaChannel_->onAvailableChanged.connect(boost::bind(&PresenceOracle::handleStanzaChannelAvailableChanged, this, _1));
}

PresenceOracle::~PresenceOracle() {
	stanzaChannel_->onPresenceReceived.disconnect(boost::bind(&PresenceOracle::handleIncomingPresence, this, _1));
	stanzaChannel_->onAvailableChanged.disconnect(boost::bind(&PresenceOracle::handleStanzaChannelAvailableChanged, this, _1));
}

void PresenceOracle::handleStanzaChannelAvailableChanged(bool available) {
	if (available) {
		entries_.clear();
	}
}

void PresenceOracle::clearPresences(const Swift::JID& bareJID) {
	std::map<JID, boost::shared_ptr<Presence> > jidMap = entries_[bareJID];
	jidMap.clear();
	entries_[bareJID] = jidMap;
}

void PresenceOracle::handleIncomingPresence(Presence::ref presence) {
	// ignore presences for some contact, we're checking only presences for the transport itself here.
	bool isMUC = presence->getPayload<MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	// filter out login/logout presence spam
	if (!presence->getTo().getNode().empty() && isMUC == false)
		return;

	JID bareJID(presence->getFrom().toBare());
	if (presence->getType() == Presence::Subscribe || presence->getType() == Presence::Subscribed) {
	}
	else {
		Presence::ref passedPresence = presence;
		if (!isMUC) {
			if (presence->getType() == Presence::Unsubscribe || presence->getType() == Presence::Unsubscribed) {
				/* 3921bis says that we don't follow up with an unavailable, so simulate this ourselves */
				passedPresence = Presence::ref(new Presence());
				passedPresence->setType(Presence::Unavailable);
				passedPresence->setFrom(bareJID);
				passedPresence->setStatus(presence->getStatus());
			}
			std::map<JID, boost::shared_ptr<Presence> > jidMap = entries_[bareJID];
			if (passedPresence->getFrom().isBare() && presence->getType() == Presence::Unavailable) {
				/* Have a bare-JID only presence of offline */
				jidMap.clear();
			} else if (passedPresence->getType() == Presence::Available) {
				/* Don't have a bare-JID only offline presence once there are available presences */
				jidMap.erase(bareJID);
			}
			if (passedPresence->getType() == Presence::Unavailable && jidMap.size() > 1) {
				jidMap.erase(passedPresence->getFrom());
			} else {
				jidMap[passedPresence->getFrom()] = passedPresence;
			}
			entries_[bareJID] = jidMap;
		}
		onPresenceChange(passedPresence);
	}
}

Presence::ref PresenceOracle::getLastPresence(const JID& jid) const {
	PresencesMap::const_iterator i = entries_.find(jid.toBare());
	if (i == entries_.end()) {
		return Presence::ref();
	}
	PresenceMap presenceMap = i->second;
	PresenceMap::const_iterator j = presenceMap.find(jid);
	if (j != presenceMap.end()) {
		return j->second;
	}
	else {
		return Presence::ref();
	}
}

std::vector<Presence::ref> PresenceOracle::getAllPresence(const JID& bareJID) const {
	std::vector<Presence::ref> results;
	PresencesMap::const_iterator i = entries_.find(bareJID);
	if (i == entries_.end()) {
		return results;
	}
	PresenceMap presenceMap = i->second;
	PresenceMap::const_iterator j = presenceMap.begin();
	for (; j != presenceMap.end(); ++j) {
		Presence::ref current = j->second;
		results.push_back(current);
	}
	return results;
}

Presence::ref PresenceOracle::getHighestPriorityPresence(const JID& bareJID) const {
	PresencesMap::const_iterator i = entries_.find(bareJID);
	if (i == entries_.end()) {
		return Presence::ref();
	}
	PresenceMap presenceMap = i->second;
	PresenceMap::const_iterator j = presenceMap.begin();
	Presence::ref highest;
	for (; j != presenceMap.end(); ++j) {
		Presence::ref current = j->second;
		if (!highest
				|| current->getPriority() > highest->getPriority()
				|| (current->getPriority() == highest->getPriority()
						&& StatusShow::typeToAvailabilityOrdering(current->getShow()) > StatusShow::typeToAvailabilityOrdering(highest->getShow()))) {
			highest = current;
		}

	}
	return highest;
}

}
