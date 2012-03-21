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

#include "transport/rosterstorage.h"
#include "transport/buddy.h"
#include "transport/user.h"
#include "transport/storagebackend.h"

namespace Transport {

// static void save_settings(gpointer k, gpointer v, gpointer data) {
// 	PurpleValue *value = (PurpleValue *) v;
// 	std::string key((char *) k);
// 	SaveData *s = (SaveData *) data;
// 	AbstractUser *user = s->user;
// 	long id = s->id;
// 	if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
// 		if (purple_value_get_boolean(value))
// 			Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, "1", purple_value_get_type(value));
// 		else
// 			Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, "0", purple_value_get_type(value));
// 	}
// 	else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
// 		const char *str = purple_value_get_string(value);
// 		Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, str ? str : "", purple_value_get_type(value));
// 	}
// }
// 
// static gboolean storeAbstractSpectrumBuddy(gpointer key, gpointer v, gpointer data) {
// 	AbstractUser *user = (AbstractUser *) data;
// 	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
// 	if (s_buddy->getFlags() & SPECTRUM_BUDDY_IGNORE)
// 		return TRUE;
// 	
// 	// save PurpleBuddy
// 	std::string alias = s_buddy->getAlias();
// 	std::string name = s_buddy->getName();
// 	long id = s_buddy->getId();
// 
// 	// Buddy is not in DB
// 	if (id != -1) {
// 		Transport::instance()->sql()->addBuddy(user->storageId(), name, s_buddy->getSubscription(), s_buddy->getGroup(), alias, s_buddy->getFlags());
// 	}
// 	else {
// 		id = Transport::instance()->sql()->addBuddy(user->storageId(), name, s_buddy->getSubscription(), s_buddy->getGroup(), alias, s_buddy->getFlags());
// 		s_buddy->setId(id);
// 	}
// 	Log("buddyListSaveNode", id << " " << name << " " << alias << " " << s_buddy->getSubscription());
// 	if (s_buddy->getBuddy() && id != -1) {
// 		PurpleBuddy *buddy = s_buddy->getBuddy();
// 		SaveData *s = new SaveData;
// 		s->user = user;
// 		s->id = id;
// 		g_hash_table_foreach(buddy->node.settings, save_settings, s);
// 		delete s;
// 	}
// 	return TRUE;
// }

RosterStorage::RosterStorage(User *user, StorageBackend *storageBackend) {
	m_user = user;
	m_storageBackend = storageBackend;
	m_storageTimer = m_user->getComponent()->getNetworkFactories()->getTimerFactory()->createTimer(5000);
	m_storageTimer->onTick.connect(boost::bind(&RosterStorage::storeBuddies, this));
}

RosterStorage::~RosterStorage() {
	m_storageTimer->stop();
}

void RosterStorage::storeBuddy(Buddy *buddy) {
	m_buddies[buddy->getName()] = buddy;
	m_storageTimer->start();
}

bool RosterStorage::storeBuddies() {
	if (m_buddies.size() == 0) {
		return false;
	}
	
	m_storageBackend->beginTransaction();

	for (std::map<std::string, Buddy *>::const_iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		BuddyInfo buddyInfo;
		buddyInfo.alias = buddy->getAlias();
		buddyInfo.legacyName = buddy->getName();
		buddyInfo.groups = buddy->getGroups();
		buddyInfo.subscription = buddy->getSubscription() == Buddy::Ask ? "ask" : "both";
		buddyInfo.id = buddy->getID();
		buddyInfo.flags = buddy->getFlags();
		buddyInfo.settings["icon_hash"].s = buddy->getIconHash();
		buddyInfo.settings["icon_hash"].type = TYPE_STRING;

		// Buddy is in DB
		if (buddyInfo.id != -1) {
			m_storageBackend->updateBuddy(m_user->getUserInfo().id, buddyInfo);
		}
		else {
			buddyInfo.id = m_storageBackend->addBuddy(m_user->getUserInfo().id, buddyInfo);
			buddy->setID(buddyInfo.id);
		}

// 		Log("buddyListSaveNode", id << " " << name << " " << alias << " " << s_buddy->getSubscription());
// 		if (s_buddy->getBuddy() && id != -1) {
// 			PurpleBuddy *buddy = s_buddy->getBuddy();
// 			SaveData *s = new SaveData;
// 			s->user = user;
// 			s->id = id;
// 			g_hash_table_foreach(buddy->node.settings, save_settings, s);
// 			delete s;
// 		}
	}

	m_storageBackend->commitTransaction();
	return true;
}

void RosterStorage::removeBuddyFromQueue(Buddy *buddy) {
	m_buddies.erase(buddy->getName());
}

}
