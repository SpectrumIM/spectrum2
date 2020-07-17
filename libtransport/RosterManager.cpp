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

#include "transport/Config.h"
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
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(rosterManagerLogger, "RosterManager");

RosterManager::RosterManager(User *user, Component *component){
	m_rosterStorage = NULL;
	m_user = user;
	m_component = component;

	boost::locale::generator gen;
	std::locale::global(gen(""));
}

RosterManager::~RosterManager() {
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddies();
	}

	sendUnavailablePresences(m_user->getJID().toBare());

	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
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

void RosterManager::removeBuddy(const std::string &_name) {
	std::string name = _name;
	name = boost::locale::to_lower(name);
	Buddy *buddy = getBuddy(name);
	if (!buddy) {
		LOG4CXX_WARN(rosterManagerLogger, m_user->getJID().toString() << ": Tried to remove unknown buddy " << name);
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
		response->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::Nickname>(buddy->getAlias()));
	}
	m_component->getFrontend()->sendPresence(response);
}

void RosterManager::sendBuddyPresences(Buddy *buddy, const Swift::JID &to) {
	Swift::Presence::ref currentPresence;	
	std::vector<Swift::Presence::ref> presences = buddy->generatePresenceStanzas(255);
	BOOST_FOREACH(Swift::Presence::ref &currentPresence, presences) {
		currentPresence->setTo(to);
		m_component->getFrontend()->sendPresence(currentPresence);
	}
}

void RosterManager::handleBuddyChanged(Buddy *buddy) {
}

void RosterManager::setBuddy(Buddy *buddy) {
	std::string name = buddy->getName();
	name = boost::locale::to_lower(name);
	LOG4CXX_INFO(rosterManagerLogger, "Associating buddy " << name << " with " << m_user->getJID().toString());
	m_buddies[name] = buddy;
	onBuddySet(buddy);

	doAddBuddy(buddy);

	if (m_rosterStorage)
		m_rosterStorage->storeBuddy(buddy);
}

void RosterManager::unsetBuddy(Buddy *buddy) {
	std::string name = buddy->getName();
	name = boost::locale::to_lower(name);
	m_buddies.erase(name);
	if (m_rosterStorage)
		m_rosterStorage->removeBuddyFromQueue(buddy);
	onBuddyUnset(buddy);
}

void RosterManager::storeBuddy(Buddy *buddy) {
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddy(buddy);
	}
}

Buddy *RosterManager::getBuddy(const std::string &_name) {
	std::string name = _name;
	name = boost::locale::to_lower(name);
	return m_buddies[name];
}


/*
Buddy handling is similar to Pidgin. A buddy can only be:
  Buddy::Both == mutual
  Buddy::Ask  == friend request from them
  no entry    == not a buddy/rejected by user
In gateway mode, anyone the user approves is automatically a Buddy::Both as far as we are concerned.
Forward all friend requests to the backend but ignore the results.
*/
void RosterManager::handleSubscription(Swift::Presence::ref presence) {
	std::string legacyName = Buddy::JIDToLegacyName(presence->getTo(), m_user);
	if (legacyName.empty())
		return;
	
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(presence->getFrom().toBare());
	response->setFrom(presence->getTo().toBare());
	response->setType(Swift::Presence::Unavailable); //== do not send
	
	Buddy *buddy = getBuddy(legacyName);
	bool newBuddy = (!buddy);
	if (newBuddy) {
		//Need a temporary buddy for most operations
		LOG4CXX_TRACE(rosterManagerLogger, "handleSubscription(): unknown buddy");
		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = "";
		buddyInfo.legacyName = legacyName;
		buddyInfo.subscription = "both";
		buddyInfo.flags = Buddy::buddyFlagsFromJID(presence->getTo());
		buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
	} else
		LOG4CXX_TRACE(rosterManagerLogger, "handleSubscription(): known buddy");


	// For server mode the subscription changes are handler in rosterresponder.cpp
	// using roster pushes.
	if (m_component->inServerMode()) {
		if (buddy)
			LOG4CXX_INFO(rosterManagerLogger, m_user->getJID().toString() << ": Subscription received and buddy " << legacyName << " is already there => answering");
		switch (presence->getType()) {
			case Swift::Presence::Subscribe:
				if (newBuddy) {
					// buddy is not in roster, so add him
					LOG4CXX_INFO(rosterManagerLogger, m_user->getJID().toString() << ": Subscription received for new buddy " << legacyName << " => adding to legacy network");
					setBuddy(buddy);
					newBuddy=false; //passed on, do not autodelete
				}
				onBuddyAdded(buddy);
				response->setType(Swift::Presence::Subscribed);
				break;

			case Swift::Presence::Subscribed:
				if (!newBuddy)
					onBuddyAdded(buddy);
//				else
// 					onBuddyAdded(buddy);
				break;

			case Swift::Presence::Unsubscribe:
				onBuddyRemoved(buddy);
				if (!newBuddy) {
					removeBuddy(buddy->getName());
					buddy = NULL;
				}
				response->setType(Swift::Presence::Unsubscribed);
				break;

			case Swift::Presence::Available:
			case Swift::Presence::Error:
			case Swift::Presence::Probe:
			case Swift::Presence::Unavailable:
			case Swift::Presence::Unsubscribed:
				break;
		}
	}
	else {
		switch (presence->getType()) {
			case Swift::Presence::Subscribe:	// Add a friend on the backend network & send friend request.
			case Swift::Presence::Subscribed:   // Accept friend request / Mark buddy as a friend
				//Add buddy as Buddy::Both, confirm friend requests now and henceforth
				LOG4CXX_TRACE(rosterManagerLogger, "handleSubscription(): Subscribe/Subscribed");
				if (newBuddy) {
					// buddy is not in roster, so add him
					LOG4CXX_INFO(rosterManagerLogger, m_user->getJID().toString() << ": Subscription received for new buddy " << legacyName << " => adding to legacy network");
					setBuddy(buddy);
					newBuddy=false; //do not autodelete
				} else if (buddy->getSubscription() != Buddy::Both) {
					buddy->setSubscription(Buddy::Both);
					storeBuddy(buddy);
				}
				onBuddyAdded(buddy); //re-confirm, even if the buddy has already been Buddy::Both - it won't hurt
				sendBuddyPresences(buddy, presence->getFrom());
				//Auto-accept, as far as XMPP is concerned
				if (presence->getType() == Swift::Presence::Subscribe)
					response->setType(Swift::Presence::Subscribed);
				break;

			case Swift::Presence::Unsubscribe:   // Throw away the friend privileges that the contact has given us.
				LOG4CXX_TRACE(rosterManagerLogger, "handleSubscription(): Unsubscribe");
				//Delete buddy and reject any friend requests
				//If delete-protection is enabled, only delete from backend if the friend is Buddy::Ask or new.
				if (newBuddy || (buddy->getSubscription() == Buddy::Ask) || CONFIG_BOOL(m_component->getConfig(), "service.enable_remove_buddy"))
					onBuddyRemoved(buddy);
				if (!newBuddy) {
					removeBuddy(buddy->getName());
					buddy = NULL;
				}
				//XEP says the buddy should send "unsubscribed" to this request
				response->setType(Swift::Presence::Unsubscribed);
				break;
			
			case Swift::Presence::Unsubscribed:  // Remove contact's friend privileges / Deny friend request / Confirm UNSUBSCRIBE
				LOG4CXX_TRACE(rosterManagerLogger, "handleSubscription(): Unsubscribed");
				//Only remove-buddy if this is a real friend request rejection
				//XMPP may send <unsubscribed> in other cases, e.g. when the user refuses the formal <subscribe>
				//that we've sent to inform them of existing ::Both buddies. We should not delete existing buddies then!
				if (buddy->getSubscription() == Buddy::Ask) {
					onBuddyRemoved(buddy);
					removeBuddy(buddy->getName());
					buddy = NULL;
					//XEP says the buddy should send "unsubscribe" if his attempt to "subscribe" had been rejected
					response->setType(Swift::Presence::Unsubscribe);
				}
				break;

			case Swift::Presence::Available:
			case Swift::Presence::Error:
			case Swift::Presence::Probe:
			case Swift::Presence::Unavailable:
				break;
		}
	}

	if (response->getType() != Swift::Presence::Unavailable)
		m_component->getFrontend()->sendPresence(response);

	if (!m_component->inServerMode())
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

	//Delete the temporary buddy
	if (newBuddy && buddy)
		delete buddy;
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
			LOG4CXX_INFO(rosterManagerLogger, m_user->getJID().toString() << ": Adding cached buddy " << buddy->getName() << " fom database");
			std::string name = buddy->getName();
			name = boost::locale::to_lower(name);
			m_buddies[name] = buddy;
			onBuddySet(buddy);
		}
	}

	m_rosterStorage = storage;
}

Swift::RosterPayload::ref RosterManager::generateRosterPayload() {
	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());

	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
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
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
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
	Buddy *buddy = Buddy::JIDToBuddy(from, m_user);
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
	for (std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<const std::string, Buddy *> > >::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
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
