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

#include "SlackRosterManager.h"
#include "SlackFrontend.h"
#include "SlackUser.h"
#include "SlackSession.h"

#include "transport/Buddy.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Factory.h"
#include "transport/PresenceOracle.h"
#include "transport/Transport.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(slackRosterManagerLogger, "SlackRosterManager");

SlackRosterManager::SlackRosterManager(User *user, Component *component) : RosterManager(user, component){
	m_user = user;
	m_component = component;

	m_onlineBuddiesTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_onlineBuddiesTimer->onTick.connect(boost::bind(&SlackRosterManager::sendOnlineBuddies, this));
	m_onlineBuddiesTimer->start();
}

SlackRosterManager::~SlackRosterManager() {
	m_onlineBuddiesTimer->stop();
}

void SlackRosterManager::sendOnlineBuddies() {
	std::string onlineBuddies = "Online users: ";
	Swift::StatusShow s;
	std::string statusMessage;

	const RosterManager::BuddiesMap &roster = getBuddies();
	for (RosterManager::BuddiesMap::const_iterator bt = roster.begin(); bt != roster.end(); bt++) {
		Buddy *b = (*bt).second;
		if (!b) {
			continue;
		}

		if (!(b->getStatus(s, statusMessage))) {
			continue;
		}

		if (s.getType() == Swift::StatusShow::None) {
			continue;
		}

		onlineBuddies += b->getAlias() + ", ";
	}

	if (m_onlineBuddies != onlineBuddies) {
		m_onlineBuddies = onlineBuddies;
		SlackSession *session = static_cast<SlackUser *>(m_user)->getSession();
		if (session) {
			session->setPurpose(m_onlineBuddies);
		}
	}

	m_onlineBuddiesTimer->start();
}

void SlackRosterManager::doRemoveBuddy(Buddy *buddy) {

}

void SlackRosterManager::doAddBuddy(Buddy *buddy) {

}

void SlackRosterManager::doUpdateBuddy(Buddy *buddy) {

}


}
