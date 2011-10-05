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

#include "transport/rostermanager.h"
#include "transport/rosterstorage.h"
#include "transport/storagebackend.h"
#include "transport/buddy.h"
#include "transport/usermanager.h"
#include "transport/buddy.h"
#include "transport/user.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "Swiften/Elements/RosterItemExchangePayload.h"
#include "log4cxx/logger.h"
#include <boost/foreach.hpp>

#include <map>
#include <iterator>

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("RosterManager");

RosterManager::RosterManager(User *user, Component *component){
	m_rosterStorage = NULL;
	m_user = user;
	m_component = component;
	m_setBuddyTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1000);
	m_RIETimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(5000);
	m_RIETimer->onTick.connect(boost::bind(&RosterManager::sendRIE, this));

	m_supportRemoteRoster = false;

	if (!m_component->inServerMode()) {
		AddressedRosterRequest::ref request = AddressedRosterRequest::ref(new AddressedRosterRequest(m_component->getIQRouter(), m_user->getJID().toBare()));
		request->onResponse.connect(boost::bind(&RosterManager::handleRemoteRosterResponse, this, _1, _2));
		request->send();
	}
}

RosterManager::~RosterManager() {
	m_setBuddyTimer->stop();
	m_RIETimer->stop();
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddies();
	}

	sendUnavailablePresences(m_user->getJID().toBare());

	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		delete buddy;
	}

	if (m_requests.size() != 0) {
		LOG4CXX_INFO(logger, m_user->getJID().toString() <<  ": Removing " << m_requests.size() << " unresponded IQs");
		BOOST_FOREACH(Swift::SetRosterRequest::ref request, m_requests) {
			request->onResponse.disconnect_all_slots();
			m_component->getIQRouter()->removeHandler(request);
		}
		m_requests.clear();
	}

	boost::singleton_pool<boost::pool_allocator_tag, sizeof(unsigned int)>::release_memory();

	if (m_rosterStorage)
		delete m_rosterStorage;
}

void RosterManager::setBuddy(Buddy *buddy) {
// 	m_setBuddyTimer->onTick.connect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));
// 	m_setBuddyTimer->start();
	setBuddyCallback(buddy);
}

void RosterManager::sendBuddyRosterPush(Buddy *buddy) {
	// user can't receive anything in server mode if he's not logged in.
	// He will ask for roster later (handled in rosterreponsder.cpp)
	if (m_component->inServerMode() && !m_user->isConnected())
		return;

	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(buddy->getJID().toBare());
	item.setName(buddy->getAlias());
	item.setGroups(buddy->getGroups());
	item.setSubscription(Swift::RosterItemPayload::Both);

	payload->addItem(item);

	std::vector<Swift::Presence::ref> presences = m_component->getPresenceOracle()->getAllPresence(m_user->getJID().toBare());
	BOOST_FOREACH(Swift::Presence::ref presence, presences) {
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, presence->getFrom(), m_component->getIQRouter());
		request->onResponse.connect(boost::bind(&RosterManager::handleBuddyRosterPushResponse, this, _1, request, buddy->getName()));
		request->send();
		m_requests.push_back(request);
	}
}

void RosterManager::sendBuddySubscribePresence(Buddy *buddy) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_user->getJID());
	response->setFrom(buddy->getJID());
	response->setType(Swift::Presence::Subscribe);
// 	TODO: NICKNAME
	m_component->getStanzaChannel()->sendPresence(response);
}

void RosterManager::handleBuddyChanged(Buddy *buddy) {
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddy(buddy);
	}
}

void RosterManager::setBuddyCallback(Buddy *buddy) {
	m_setBuddyTimer->onTick.disconnect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));

	LOG4CXX_INFO(logger, "Associating buddy " << buddy->getName() << " with " << m_user->getJID().toString());
	m_buddies[buddy->getName()] = buddy;
	onBuddySet(buddy);

	// In server mode the only way is to send jabber:iq:roster push.
	// In component mode we send RIE or Subscribe presences, based on features.
	if (m_component->inServerMode()) {
		sendBuddyRosterPush(buddy);
	}
	else {
		if (m_setBuddyTimer->onTick.empty()) {
			m_setBuddyTimer->stop();

			if (m_supportRemoteRoster) {
				sendBuddyRosterPush(buddy);
			}
			else {
				// Send RIE only if there's resource which supports it.
				Swift::JID jidWithRIE = m_user->getJIDWithFeature("http://jabber.org/protocol/rosterx");
				if (jidWithRIE.isValid()) {
					m_RIETimer->start();
				}
				else {
					sendBuddySubscribePresence(buddy);
				}
			}
		}
	}

	if (m_rosterStorage)
		m_rosterStorage->storeBuddy(buddy);
}

void RosterManager::unsetBuddy(Buddy *buddy) {
	m_buddies.erase(buddy->getName());
	if (m_rosterStorage)
		m_rosterStorage->removeBuddyFromQueue(buddy);
	onBuddyUnset(buddy);
}

void RosterManager::storeBuddy(Buddy *buddy) {
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddy(buddy);
	}
}

void RosterManager::handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key) {
	LOG4CXX_INFO(logger, "handleBuddyRosterPushResponse called for buddy " << key);
	if (m_buddies[key] != NULL) {
		m_buddies[key]->handleBuddyChanged();
	}
	else {
		LOG4CXX_WARN(logger, "handleBuddyRosterPushResponse called for unknown buddy " << key);
	}

	m_requests.remove(request);
	request->onResponse.disconnect_all_slots();
}

void RosterManager::handleRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref error) {
	if (error) {
		m_supportRemoteRoster = false;
		LOG4CXX_INFO(logger, m_user->getJID().toString() << ": This server does not support remote roster protoXEP");
		return;
	}

	LOG4CXX_INFO(logger, m_user->getJID().toString() << ": This server supports remote roster protoXEP");
	m_supportRemoteRoster = true;
	return;

	BOOST_FOREACH(const Swift::RosterItemPayload &item, payload->getItems()) {
		std::string legacyName = Buddy::JIDToLegacyName(item.getJID());
		if (m_buddies.find(legacyName) != m_buddies.end()) {
			continue;
		}
		std::cout << "LEGACYNAME " << legacyName << "\n";

		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = item.getName();
		buddyInfo.legacyName = legacyName;
		buddyInfo.subscription = "both";
		buddyInfo.flags = Buddy::buddFlagsFromJID(item.getJID());

		Buddy *buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
		setBuddy(buddy);
	}
}

Buddy *RosterManager::getBuddy(const std::string &name) {
	return m_buddies[name];
}

void RosterManager::sendRIE() {
	m_RIETimer->stop();

	// Check the feature, because proper resource could logout during RIETimer.
	Swift::JID jidWithRIE = m_user->getJIDWithFeature("http://jabber.org/protocol/rosterx");

	// fallback to normal subscribe
	if (!jidWithRIE.isValid()) {
		for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
			Buddy *buddy = (*it).second;
			if (!buddy) {
				continue;
			}
			sendBuddySubscribePresence(buddy);
		}
		return;
	}

	LOG4CXX_INFO(logger, "Sending RIE stanza to " << jidWithRIE.toString());

	Swift::RosterItemExchangePayload::ref payload = Swift::RosterItemExchangePayload::ref(new Swift::RosterItemExchangePayload());
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
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

	boost::shared_ptr<Swift::GenericRequest<Swift::RosterItemExchangePayload> > request(new Swift::GenericRequest<Swift::RosterItemExchangePayload>(Swift::IQ::Set, jidWithRIE, payload, m_component->getIQRouter()));
	request->send();
}

void RosterManager::handleSubscription(Swift::Presence::ref presence) {
	std::string legacyName = Buddy::JIDToLegacyName(presence->getTo());
	// For server mode the subscription changes are handler in rosterresponder.cpp
	// using roster pushes.
	if (m_component->inServerMode()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(presence->getFrom());
		response->setFrom(presence->getTo());
		Buddy *buddy = getBuddy(Buddy::JIDToLegacyName(presence->getTo()));
		if (buddy) {
			LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Subscription received and buddy " << Buddy::JIDToLegacyName(presence->getTo()) << " is already there => answering");
			switch (presence->getType()) {
				case Swift::Presence::Subscribe:
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					break;
				case Swift::Presence::Unsubscribe:
					onBuddyRemoved(buddy);
					response->setType(Swift::Presence::Unsubscribed);
					break;
				case Swift::Presence::Subscribed:
					onBuddyAdded(buddy);
					break;
				default:
					return;
			}
			m_component->getStanzaChannel()->sendPresence(response);
			
		}
		else {
			BuddyInfo buddyInfo;
			switch (presence->getType()) {
				// buddy is not in roster, so add him
				case Swift::Presence::Subscribe:
					buddyInfo.id = -1;
					buddyInfo.alias = "";
					buddyInfo.legacyName = Buddy::JIDToLegacyName(presence->getTo());
					buddyInfo.subscription = "both";
					buddyInfo.flags = Buddy::buddFlagsFromJID(presence->getTo());
					LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Subscription received for new buddy " << buddyInfo.legacyName << " => adding to legacy network");

					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					setBuddy(buddy);
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					break;
				case Swift::Presence::Subscribed:
					onBuddyAdded(buddy);
					break;
				// buddy is already there, so nothing to do, just answer
				case Swift::Presence::Unsubscribe:
					response->setType(Swift::Presence::Unsubscribed);
					break;
				default:
					return;
			}
			m_component->getStanzaChannel()->sendPresence(response);
		}
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		Swift::Presence::ref currentPresence;
		response->setTo(presence->getFrom());
		response->setFrom(presence->getTo());

		Buddy *buddy = getBuddy(Buddy::JIDToLegacyName(presence->getTo()));
		if (buddy) {
			switch (presence->getType()) {
				// buddy is already there, so nothing to do, just answer
				case Swift::Presence::Subscribe:
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					currentPresence = buddy->generatePresenceStanza(255);
					if (currentPresence) {
						currentPresence->setTo(presence->getFrom());
						m_component->getStanzaChannel()->sendPresence(currentPresence);
					}
					break;
				// remove buddy
				case Swift::Presence::Unsubscribe:
					response->setType(Swift::Presence::Unsubscribed);
					onBuddyRemoved(buddy);
					break;
				// just send response
				case Swift::Presence::Unsubscribed:
// 					response->setType(Swift::Presence::Unsubscribe);
					break;
				default:
					return;
			}
		}
		else {
			BuddyInfo buddyInfo;
			switch (presence->getType()) {
				// buddy is not in roster, so add him
				case Swift::Presence::Subscribe:
					buddyInfo.id = -1;
					buddyInfo.alias = "";
					buddyInfo.legacyName = Buddy::JIDToLegacyName(presence->getTo());
					buddyInfo.subscription = "both";
					buddyInfo.flags = Buddy::buddFlagsFromJID(presence->getTo());

					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					setBuddy(buddy);
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					break;
				// buddy is already there, so nothing to do, just answer
				case Swift::Presence::Unsubscribe:
					response->setType(Swift::Presence::Unsubscribed);
					onBuddyRemoved(buddy);
					break;
				// just send response
				case Swift::Presence::Unsubscribed:
					response->setType(Swift::Presence::Unsubscribe);
					break;
				// just send response
				default:
					return;
			}
		}

		m_component->getStanzaChannel()->sendPresence(response);

		// We have to act as buddy and send its subscribe/unsubscribe just to be sure...
		switch (response->getType()) {
			case Swift::Presence::Unsubscribed:
				response->setType(Swift::Presence::Unsubscribe);
				m_component->getStanzaChannel()->sendPresence(response);
				break;
			case Swift::Presence::Subscribed:
				response->setType(Swift::Presence::Subscribe);
				m_component->getStanzaChannel()->sendPresence(response);
				break;
			default:
				break;
		}
	}
}

void RosterManager::setStorageBackend(StorageBackend *storageBackend) {
	if (m_rosterStorage || !storageBackend) {
		return;
	}
	m_rosterStorage = new RosterStorage(m_user, storageBackend);

	std::list<BuddyInfo> roster;
	storageBackend->getBuddies(m_user->getUserInfo().id, roster);

	for (std::list<BuddyInfo>::const_iterator it = roster.begin(); it != roster.end(); it++) {
		Buddy *buddy = m_component->getFactory()->createBuddy(this, *it);
		LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Adding cached buddy " << buddy->getName() << " fom database");
		m_buddies[buddy->getName()] = buddy;
		onBuddySet(buddy);
	}
}

Swift::RosterPayload::ref RosterManager::generateRosterPayload() {
	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());

	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::RosterItemPayload item;
		item.setJID(buddy->getJID().toBare());
		item.setName(buddy->getAlias());
		item.setGroups(buddy->getGroups());
		item.setSubscription(Swift::RosterItemPayload::Both);
		payload->addItem(item);
	}
	return payload;
}

void RosterManager::sendCurrentPresences(const Swift::JID &to) {
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::Presence::ref presence = buddy->generatePresenceStanza(255);
		if (presence) {
			presence->setTo(to);
			m_component->getStanzaChannel()->sendPresence(presence);
		}
	}
}

void RosterManager::sendCurrentPresence(const Swift::JID &from, const Swift::JID &to) {
	Buddy *buddy = getBuddy(Buddy::JIDToLegacyName(from));
	if (buddy) {
		Swift::Presence::ref presence = buddy->generatePresenceStanza(255);
		if (presence) {
			presence->setTo(to);
			m_component->getStanzaChannel()->sendPresence(presence);
		}
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(to);
		response->setFrom(from);
		response->setType(Swift::Presence::Unavailable);
		m_component->getStanzaChannel()->sendPresence(response);
	}
}

void RosterManager::sendUnavailablePresences(const Swift::JID &to) {
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::Presence::ref presence = buddy->generatePresenceStanza(255);
		if (presence) {
			presence->setTo(to);
			presence->setType(Swift::Presence::Unavailable);
			m_component->getStanzaChannel()->sendPresence(presence);
		}
	}

	// in gateway mode, we have to send unavailable presence for transport
	// contact
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(to);
	response->setFrom(m_component->getJID());
	response->setType(Swift::Presence::Unavailable);
	m_component->getStanzaChannel()->sendPresence(response);
}

}
