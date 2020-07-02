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

#include "XMPPRosterManager.h"
#include "XMPPFrontend.h"
#include "XMPPUser.h"
#include "transport/Buddy.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Factory.h"
#include "transport/PresenceOracle.h"
#include "transport/Transport.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "Swiften/Elements/RosterItemExchangePayload.h"
#include "Swiften/Elements/Nickname.h"
#include "Swiften/Queries/IQRouter.h"
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(xmppRosterManagerLogger, "XMPPRosterManager");

XMPPRosterManager::XMPPRosterManager(User *user, Component *component) : RosterManager(user, component){
	m_user = user;
	m_component = component;

	m_supportRemoteRoster = false;
	m_RIETimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(5000);
	m_RIETimer->onTick.connect(boost::bind(&XMPPRosterManager::sendRIE, this));

	if (!m_component->inServerMode()) {
		m_remoteRosterRequest = AddressedRosterRequest::ref(new AddressedRosterRequest(static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter(), m_user->getJID().toBare()));
		m_remoteRosterRequest->onResponse.connect(boost::bind(&XMPPRosterManager::handleRemoteRosterResponse, this, _1, _2));
		m_remoteRosterRequest->send();
	}
}

XMPPRosterManager::~XMPPRosterManager() {
	m_RIETimer->stop();
	if (m_remoteRosterRequest) {
		m_remoteRosterRequest->onResponse.disconnect_all_slots();
		static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter()->removeHandler(m_remoteRosterRequest);
	}

	if (m_requests.size() != 0) {
		LOG4CXX_INFO(xmppRosterManagerLogger, m_user->getJID().toString() <<  ": Removing " << m_requests.size() << " unresponded IQs");
		BOOST_FOREACH(Swift::SetRosterRequest::ref request, m_requests) {
			request->onResponse.disconnect_all_slots();
			static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter()->removeHandler(request);
		}
		m_requests.clear();
	}
}

void XMPPRosterManager::doRemoveBuddy(Buddy *buddy) {
	if (m_component->inServerMode() || m_supportRemoteRoster) {
		sendBuddyRosterRemove(buddy);
	}
	else {
		sendBuddyUnsubscribePresence(buddy);
	}
}

void XMPPRosterManager::sendBuddyRosterRemove(Buddy *buddy) {
	Swift::RosterPayload::ref p = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(buddy->getJID().toBare());
	item.setSubscription(Swift::RosterItemPayload::Remove);

	p->addItem(item);

	// In server mode we have to send pushes to all resources, but in gateway-mode we send it only to bare JID
	if (m_component->inServerMode()) {
		std::vector<Swift::Presence::ref> presences = m_component->getPresenceOracle()->getAllPresence(m_user->getJID().toBare());
		BOOST_FOREACH(Swift::Presence::ref presence, presences) {
			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, presence->getFrom(), static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
			request->send();
		}
	}
	else {
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, m_user->getJID().toBare(), static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
		request->send();
	}
}

void XMPPRosterManager::sendBuddyRosterPush(Buddy *buddy) {
	// user can't receive anything in server mode if he's not logged in.
	// He will ask for roster later (handled in rosterreponsder.cpp)
	if (m_component->inServerMode() && (!m_user->isConnected() || m_user->shouldCacheMessages()))
		return;

	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(buddy->getJID().toBare());
	item.setName(buddy->getAlias());
	item.setGroups(buddy->getGroups());
	item.setSubscription(Swift::RosterItemPayload::Both);

	payload->addItem(item);

	// In server mode we have to send pushes to all resources, but in gateway-mode we send it only to bare JID
	if (m_component->inServerMode()) {
		std::vector<Swift::Presence::ref> presences = m_component->getPresenceOracle()->getAllPresence(m_user->getJID().toBare());
		BOOST_FOREACH(Swift::Presence::ref presence, presences) {
			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, presence->getFrom(), static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
			request->onResponse.connect(boost::bind(&XMPPRosterManager::handleBuddyRosterPushResponse, this, _1, request, buddy->getName()));
			request->send();
			m_requests.push_back(request);
		}
	}
	else {
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, m_user->getJID().toBare(), static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
		request->onResponse.connect(boost::bind(&XMPPRosterManager::handleBuddyRosterPushResponse, this, _1, request, buddy->getName()));
		request->send();
		m_requests.push_back(request);
	}

	if (buddy->getSubscription() != Buddy::Both) {
		buddy->setSubscription(Buddy::Both);
		storeBuddy(buddy);
	}
}


void XMPPRosterManager::handleBuddyChanged(Buddy *buddy) {
}

void XMPPRosterManager::doAddBuddy(Buddy *buddy) {
	// In server mode the only way is to send jabber:iq:roster push.
	// In component mode we send RIE or Subscribe presences, based on features.
	if (m_component->inServerMode()) {
		sendBuddyRosterPush(buddy);
	}
	else {
		if (buddy->getSubscription() == Buddy::Both) {
			LOG4CXX_INFO(xmppRosterManagerLogger, m_user->getJID().toString() << ": Not forwarding this buddy, because subscription=both");
			return;
		}

		if (m_supportRemoteRoster) {
			sendBuddyRosterPush(buddy);
		}
		else {
			m_RIETimer->start();
		}
	}
}

void XMPPRosterManager::doUpdateBuddy(Buddy *buddy) {
	if (m_component->inServerMode() || m_supportRemoteRoster) {
		sendBuddyRosterPush(buddy);
	}
}

void XMPPRosterManager::handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key) {
	LOG4CXX_INFO(xmppRosterManagerLogger, "handleBuddyRosterPushResponse called for buddy " << key);
	Buddy *b = getBuddy(key);
	if (b) {
		if (b->isAvailable()) {
			std::vector<Swift::Presence::ref> &presences = b->generatePresenceStanzas(255);
			BOOST_FOREACH(Swift::Presence::ref &presence, presences) {
				m_component->getFrontend()->sendPresence(presence);
			}
		}
	}
	else {
		LOG4CXX_WARN(xmppRosterManagerLogger, "handleBuddyRosterPushResponse called for unknown buddy " << key);
	}

	m_requests.remove(request);
	request->onResponse.disconnect_all_slots();
}

void XMPPRosterManager::handleRemoteRosterResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref error) {
	m_remoteRosterRequest.reset();
	if (error) {
		m_supportRemoteRoster = false;
		LOG4CXX_INFO(xmppRosterManagerLogger, m_user->getJID().toString() << ": This server does not allow us to modify your roster, consider enabling XEP-0321 or XEP-0356 support");
		return;
	}

	LOG4CXX_INFO(xmppRosterManagerLogger, m_user->getJID().toString() << ": Roster modification is allowed");
	m_supportRemoteRoster = true;

	//If we receive empty RosterPayload on login (not register) initiate full RosterPush
	if (!m_buddies.empty() && payload->getItems().empty()){
			LOG4CXX_INFO(xmppRosterManagerLogger, "Received empty Roster upon login. Pushing full Roster.");
			for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::const_iterator c_it = m_buddies.begin();
					c_it != m_buddies.end(); c_it++) {
				sendBuddyRosterPush(c_it->second);
			}
	}
	return;

	BOOST_FOREACH(const Swift::RosterItemPayload &item, payload->getItems()) {
		std::string legacyName = Buddy::JIDToLegacyName(item.getJID(), m_user);
		Buddy *b = getBuddy(legacyName);
		if (b) {
			continue;
		}

		if (legacyName.empty()) {
			continue;
		}

		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = item.getName();
		buddyInfo.legacyName = legacyName;
		buddyInfo.subscription = "both";
		buddyInfo.flags = Buddy::buddyFlagsFromJID(item.getJID());
		buddyInfo.groups = item.getGroups();

		Buddy *buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
		if (buddy) {
			setBuddy(buddy);
		}
	}
}


void XMPPRosterManager::sendRIE() {
	m_RIETimer->stop();

	// Check the feature, because proper resource could logout during RIETimer.
	std::vector<Swift::JID> jidWithRIE = static_cast<XMPPUser *>(m_user)->getJIDWithFeature("http://jabber.org/protocol/rosterx");

	// fallback to normal subscribe
	if (jidWithRIE.empty()) {
		for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
			Buddy *buddy = (*it).second;
			if (!buddy) {
				continue;
			}
			sendBuddySubscribePresence(buddy);
		}
		return;
	}

	Swift::RosterItemExchangePayload::ref payload = Swift::RosterItemExchangePayload::ref(new Swift::RosterItemExchangePayload());
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::RosterItemExchangePayload::Item item;
		item.setJID(buddy->getJID().toBare());
		item.setName(buddy->getAlias());
		item.setAction(Swift::RosterItemExchangePayload::Item::Add);
		item.setGroups(buddy->getGroups());

		payload->addItem(item);
	}

	BOOST_FOREACH(Swift::JID &jid, jidWithRIE) {
		LOG4CXX_INFO(xmppRosterManagerLogger, "Sending RIE stanza to " << jid.toString());
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GenericRequest<Swift::RosterItemExchangePayload> > request(new Swift::GenericRequest<Swift::RosterItemExchangePayload>(Swift::IQ::Set, jid, payload, static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter()));
		request->send();
	}
}


}
