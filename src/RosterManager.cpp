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

#include "transport/RosterManager.h"
#include "transport/RosterStorage.h"
#include "transport/StorageBackend.h"
#include "transport/Buddy.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/Factory.h"
#include "transport/Transport.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "Swiften/Elements/RosterItemExchangePayload.h"
#include "Swiften/Elements/Nickname.h"
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "RosterManager");

RosterManager::RosterManager(User *user, Component *component){
	m_rosterStorage = NULL;
	m_user = user;
	m_component = component;
}

RosterManager::~RosterManager() {
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

	boost::singleton_pool<boost::pool_allocator_tag, sizeof(unsigned int)>::release_memory();

	if (m_rosterStorage)
		delete m_rosterStorage;
}

void RosterManager::removeBuddy(const std::string &name) {
	Buddy *buddy = getBuddy(name);
	if (!buddy) {
		LOG4CXX_WARN(logger, m_user->getJID().toString() << ": Tried to remove unknown buddy " << name);
		return;
	}

	doRemoveBuddy(buddy);

	if (m_rosterStorage)
		m_rosterStorage->removeBuddy(buddy);

	unsetBuddy(buddy);
	delete buddy;
}


void RosterManager::sendBuddyUnsubscribePresence(Buddy *buddy) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_user->getJID());
	response->setFrom(buddy->getJID());
	response->setType(Swift::Presence::Unsubscribe);
	m_component->getFrontend()->sendPresence(response);

	response = Swift::Presence::create();
	response->setTo(m_user->getJID());
	response->setFrom(buddy->getJID());
	response->setType(Swift::Presence::Unsubscribed);
	m_component->getFrontend()->sendPresence(response);
}

void RosterManager::sendBuddySubscribePresence(Buddy *buddy) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_user->getJID());
	response->setFrom(buddy->getJID().toBare());
	response->setType(Swift::Presence::Subscribe);
	if (!buddy->getAlias().empty()) {
		response->addPayload(boost::make_shared<Swift::Nickname>(buddy->getAlias()));
	}
	m_component->getFrontend()->sendPresence(response);
}

void RosterManager::handleBuddyChanged(Buddy *buddy) {
}

void RosterManager::setBuddy(Buddy *buddy) {
	LOG4CXX_INFO(logger, "Associating buddy " << buddy->getName() << " with " << m_user->getJID().toString());
	m_buddies[buddy->getName()] = buddy;
	onBuddySet(buddy);

	doAddBuddy(buddy);

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

Buddy *RosterManager::getBuddy(const std::string &name) {
	return m_buddies[name];
}


void RosterManager::handleSubscription(Swift::Presence::ref presence) {
	std::string legacyName = Buddy::JIDToLegacyName(presence->getTo());
	if (legacyName.empty()) {
		return;
	}
	
	// For server mode the subscription changes are handler in rosterresponder.cpp
	// using roster pushes.
	if (m_component->inServerMode()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(presence->getFrom().toBare());
		response->setFrom(presence->getTo().toBare());
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
					removeBuddy(buddy->getName());
					buddy = NULL;
					response->setType(Swift::Presence::Unsubscribed);
					break;
				case Swift::Presence::Subscribed:
					onBuddyAdded(buddy);
					break;
				default:
					return;
			}
			m_component->getFrontend()->sendPresence(response);
			
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
					buddyInfo.flags = Buddy::buddyFlagsFromJID(presence->getTo());
					LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Subscription received for new buddy " << buddyInfo.legacyName << " => adding to legacy network");

					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					setBuddy(buddy);
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					break;
				case Swift::Presence::Subscribed:
// 					onBuddyAdded(buddy);
					return;
				// buddy is not there, so nothing to do, just answer
				case Swift::Presence::Unsubscribe:
					buddyInfo.id = -1;
					buddyInfo.alias = "";
					buddyInfo.legacyName = Buddy::JIDToLegacyName(presence->getTo());
					buddyInfo.subscription = "both";
					buddyInfo.flags = Buddy::buddyFlagsFromJID(presence->getTo());

					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					onBuddyRemoved(buddy);
					delete buddy;
					response->setType(Swift::Presence::Unsubscribed);
					break;
				default:
					return;
			}
			m_component->getFrontend()->sendPresence(response);
		}
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		Swift::Presence::ref currentPresence;
		response->setTo(presence->getFrom().toBare());
		response->setFrom(presence->getTo().toBare());

		Buddy *buddy = getBuddy(Buddy::JIDToLegacyName(presence->getTo()));
		if (buddy) {
			std::vector<Swift::Presence::ref> &presences = buddy->generatePresenceStanzas(255);
			switch (presence->getType()) {
				// buddy is already there, so nothing to do, just answer
				case Swift::Presence::Subscribe:
					onBuddyAdded(buddy);
					response->setType(Swift::Presence::Subscribed);
					BOOST_FOREACH(Swift::Presence::ref &currentPresence, presences) {
						currentPresence->setTo(presence->getFrom());
						m_component->getFrontend()->sendPresence(currentPresence);
					}
					if (buddy->getSubscription() != Buddy::Both) {
						buddy->setSubscription(Buddy::Both);
						storeBuddy(buddy);
					}
					break;
				// remove buddy
				case Swift::Presence::Unsubscribe:
					response->setType(Swift::Presence::Unsubscribed);
					onBuddyRemoved(buddy);
					removeBuddy(buddy->getName());
					buddy = NULL;
					break;
				// just send response
				case Swift::Presence::Unsubscribed:
					response->setType(Swift::Presence::Unsubscribe);
					// We set both here, because this Unsubscribed can be response to
					// subscribe presence and we don't want that unsubscribe presence
					// to be send later again
					if (buddy->getSubscription() != Buddy::Both) {
						buddy->setSubscription(Buddy::Both);
						storeBuddy(buddy);
					}
					break;
				case Swift::Presence::Subscribed:
					if (buddy->getSubscription() != Buddy::Both) {
						buddy->setSubscription(Buddy::Both);
						storeBuddy(buddy);
					}
					return;
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
					buddyInfo.flags = Buddy::buddyFlagsFromJID(presence->getTo());

					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					setBuddy(buddy);
					onBuddyAdded(buddy);
					LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Subscription received for new buddy " << buddyInfo.legacyName << " => adding to legacy network");
					response->setType(Swift::Presence::Subscribed);
					break;
				case Swift::Presence::Unsubscribe:
					buddyInfo.id = -1;
					buddyInfo.alias = "";
					buddyInfo.legacyName = Buddy::JIDToLegacyName(presence->getTo());
					buddyInfo.subscription = "both";
					buddyInfo.flags = Buddy::buddyFlagsFromJID(presence->getTo());

					response->setType(Swift::Presence::Unsubscribed);
					
					buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
					onBuddyRemoved(buddy);
					delete buddy;
					buddy = NULL;
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

		m_component->getFrontend()->sendPresence(response);

		// We have to act as buddy and send its subscribe/unsubscribe just to be sure...
		switch (response->getType()) {
			case Swift::Presence::Unsubscribed:
				response->setType(Swift::Presence::Unsubscribe);
				m_component->getFrontend()->sendPresence(response);
				break;
			case Swift::Presence::Subscribed:
				response->setType(Swift::Presence::Subscribe);
				m_component->getFrontend()->sendPresence(response);
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
	RosterStorage *storage = new RosterStorage(m_user, storageBackend);

	std::list<BuddyInfo> roster;
	storageBackend->getBuddies(m_user->getUserInfo().id, roster);

	for (std::list<BuddyInfo>::const_iterator it = roster.begin(); it != roster.end(); it++) {
		Buddy *buddy = m_component->getFactory()->createBuddy(this, *it);
		if (buddy) {
			LOG4CXX_INFO(logger, m_user->getJID().toString() << ": Adding cached buddy " << buddy->getName() << " fom database");
			m_buddies[buddy->getName()] = buddy;
			onBuddySet(buddy);
		}
	}

	m_rosterStorage = storage;
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
		if (!buddy->isAvailable()) {
			continue;
		}
		std::vector<Swift::Presence::ref> &presences = buddy->generatePresenceStanzas(255);
		BOOST_FOREACH(Swift::Presence::ref &presence, presences) {
			presence->setTo(to);
			m_component->getFrontend()->sendPresence(presence);
		}
	}
}

void RosterManager::sendCurrentPresence(const Swift::JID &from, const Swift::JID &to) {
	Buddy *buddy = getBuddy(Buddy::JIDToLegacyName(from));
	if (buddy) {
		std::vector<Swift::Presence::ref> &presences = buddy->generatePresenceStanzas(255);
		BOOST_FOREACH(Swift::Presence::ref &presence, presences) {
			presence->setTo(to);
			m_component->getFrontend()->sendPresence(presence);
		}
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(to);
		response->setFrom(from);
		response->setType(Swift::Presence::Unavailable);
		m_component->getFrontend()->sendPresence(response);
	}
}

void RosterManager::sendUnavailablePresences(const Swift::JID &to) {
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}

		if (!buddy->isAvailable()) {
			continue;
		}

		std::vector<Swift::Presence::ref> &presences = buddy->generatePresenceStanzas(255);
		BOOST_FOREACH(Swift::Presence::ref &presence, presences) {
			Swift::Presence::Type type = presence->getType();
			presence->setTo(to);
			presence->setType(Swift::Presence::Unavailable);
			m_component->getFrontend()->sendPresence(presence);
			presence->setType(type);
		}
	}

	// in gateway mode, we have to send unavailable presence for transport
	// contact
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(to);
	response->setFrom(m_component->getJID());
	response->setType(Swift::Presence::Unavailable);
	m_component->getFrontend()->sendPresence(response);
}

}
