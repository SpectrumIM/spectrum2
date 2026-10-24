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
#include "transport/Factory.h"
#include "transport/Transport.h"
#include "transport/Util.h"

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(rosterManagerLogger, "RosterManager");

RosterManager::RosterManager(User *user, Component *component){
	m_rosterStorage = NULL;
	m_user = user;
	m_component = component;

}

RosterManager::~RosterManager() {
	if (m_rosterStorage) {
		m_rosterStorage->storeBuddies();
	}

	for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		delete buddy;
	}


	if (m_rosterStorage)
		delete m_rosterStorage;
}

void RosterManager::removeBuddy(const std::string &_name) {
	std::string name = _name;
	name = Util::toLowerUTF8(name);
	Buddy *buddy = getBuddy(name);
	if (!buddy) {
		LOG4CXX_WARN(rosterManagerLogger, m_user->getUserName() << ": Tried to remove unknown buddy " << name);
		return;
	}

	doRemoveBuddy(buddy);

	if (m_rosterStorage)
		m_rosterStorage->removeBuddy(buddy);

	unsetBuddy(buddy);
	delete buddy;
}

void RosterManager::setBuddy(Buddy *buddy) {
	std::string name = buddy->getName();
	name = Util::toLowerUTF8(name);
	LOG4CXX_INFO(rosterManagerLogger, "Associating buddy " << name << " with " << m_user->getUserName());
	m_buddies[name] = buddy;
	onBuddySet(buddy);

	doAddBuddy(buddy);

	if (m_rosterStorage)
		m_rosterStorage->storeBuddy(buddy);
}

void RosterManager::unsetBuddy(Buddy *buddy) {
	std::string name = buddy->getName();
	name = Util::toLowerUTF8(name);
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
	name = Util::toLowerUTF8(name);
	return m_buddies[name];
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
			LOG4CXX_INFO(rosterManagerLogger, m_user->getUserName() << ": Adding cached buddy " << buddy->getName() << " fom database");
			std::string name = buddy->getName();
			name = Util::toLowerUTF8(name);
			m_buddies[name] = buddy;
			onBuddySet(buddy);
		}
	}

	m_rosterStorage = storage;
}

}
