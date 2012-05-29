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

#include "transport/localbuddy.h"
#include "transport/user.h"

namespace Transport {

LocalBuddy::LocalBuddy(RosterManager *rosterManager, long id) : Buddy(rosterManager, id) {
	m_status = Swift::StatusShow::None;
	m_firstSet = true;
}

LocalBuddy::~LocalBuddy() {
}

void LocalBuddy::setAlias(const std::string &alias) {
//	if (m_firstSet) {
//		m_firstSet = false;
//		m_alias = alias;
//		return;
//	}
	bool changed = m_alias != alias;
	m_alias = alias;

	if (changed) {
		if (getRosterManager()->getUser()->getComponent()->inServerMode() || getRosterManager()->isRemoteRosterSupported()) {
			getRosterManager()->sendBuddyRosterPush(this);
		}
		getRosterManager()->storeBuddy(this);
	}
}

void LocalBuddy::setGroups(const std::vector<std::string> &groups) {
	bool changed = m_groups.size() != groups.size();
	if (!changed) {
		for (int i = 0; i != m_groups.size(); i++) {
			if (m_groups[i] != groups[i]) {
				changed = true;
				break;
			}
		}
	}

	m_groups = groups;
	if (changed) {
		if (getRosterManager()->getUser()->getComponent()->inServerMode() || getRosterManager()->isRemoteRosterSupported()) {
			getRosterManager()->sendBuddyRosterPush(this);
		}
		getRosterManager()->storeBuddy(this);
	}
}

}
