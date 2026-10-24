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

#include "transport/Buddy.h"
#include "transport/RosterManager.h"

namespace Transport {

Buddy::Buddy(RosterManager *rosterManager, long id, BuddyFlag flags) : m_id(id), m_flags(flags), m_rosterManager(rosterManager),
	m_subscription(Ask) {
}

Buddy::~Buddy() {
}

void Buddy::sendPresence() {
	m_rosterManager->doSendBuddyStatus(this);
}

void Buddy::setID(long id) {
	m_id = id;
}

long Buddy::getID() {
	return m_id;
}

void Buddy::setFlags(BuddyFlag flags) {
	m_flags = flags;
}

BuddyFlag Buddy::getFlags() {
	return m_flags;
}

void Buddy::setSubscription(Subscription subscription) {
	m_subscription = subscription;
}

Buddy::Subscription Buddy::getSubscription() {
	return m_subscription;
}

}
