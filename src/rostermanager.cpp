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
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"

namespace Transport {

RosterManager::RosterManager(User *user, Component *component){
	m_user = user;
	m_component = component;
	m_setBuddyTimer = m_component->getFactories()->getTimerFactory()->createTimer(1000);
}

RosterManager::~RosterManager() {
}

void RosterManager::setBuddy(AbstractBuddy *buddy) {
	m_setBuddyTimer->onTick.connect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));
	m_setBuddyTimer->start();
}

void RosterManager::sendBuddyRosterPush(AbstractBuddy *buddy) {
	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(buddy->getJID().toBare());
	item.setName(buddy->getAlias());
	item.setGroups(buddy->getGroups());

	payload->addItem(item);

	Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, m_component->getIQRouter(), m_user->getJID().toBare());
	request->onResponse.connect(boost::bind(&RosterManager::handleBuddyRosterPushResponse, this, _1, buddy->getName()));
	request->send();
}

void RosterManager::setBuddyCallback(AbstractBuddy *buddy) {
	m_setBuddyTimer->onTick.disconnect(boost::bind(&RosterManager::setBuddyCallback, this, buddy));

	m_buddies[buddy->getName()] = buddy;
	onBuddySet(buddy);

	if (m_component->inServerMode()) {
		sendBuddyRosterPush(buddy);
	}

	if (m_setBuddyTimer->onTick.empty()) {
		m_setBuddyTimer->stop();
	}
}

void RosterManager::unsetBuddy(AbstractBuddy *buddy) {
	m_buddies.erase(buddy->getName());
	onBuddyUnset(buddy);
}

void RosterManager::handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, const std::string &key) {
	if (m_buddies[key] != NULL) {
		m_buddies[key]->buddyChanged();
	}
}

AbstractBuddy *RosterManager::getBuddy(const std::string &name) {
	return m_buddies[name];
}

}