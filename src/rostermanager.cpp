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
#include "transport/abstractbuddy.h"
#include "transport/usermanager.h"
#include "transport/abstractbuddy.h"
#include "transport/user.h"

namespace Transport {

RosterManager::RosterManager(User *user, Component *component){
	m_user = user;
	m_component = component;
	m_setBuddyTimer = m_component->getFactories()->getTimerFactory()->createTimer(10);
}

RosterManager::~RosterManager() {
}

void RosterManager::setBuddy(AbstractBuddy *buddy) {
	m_setBuddyTimer->onTick.connect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));
	m_setBuddyTimer->start();
}

void RosterManager::setBuddyCallback(AbstractBuddy *buddy) {
	m_setBuddyTimer->onTick.disconnect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));

	m_buddies[buddy->getSafeName()] = buddy;
	onBuddySet(buddy);

	if (m_setBuddyTimer->onTick.empty()) {
		m_setBuddyTimer->stop();
	}
}

void RosterManager::unsetBuddy(AbstractBuddy *buddy) {
	m_buddies.erase(buddy->getSafeName());
	onBuddyUnset(buddy);
}

}