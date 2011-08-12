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

#include "blockresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/BlockPayload.h"
#include "Swiften/Swiften.h"
#include "transport/usermanager.h"
#include "transport/user.h"
#include "transport/buddy.h"
#include "transport/rostermanager.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

using namespace Swift;
using namespace boost;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("BlockResponder");

BlockResponder::BlockResponder(Swift::IQRouter *router, UserManager *userManager) : Swift::SetResponder<BlockPayload>(router) {
	m_userManager = userManager;
}

BlockResponder::~BlockResponder() {
	
}

bool BlockResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::BlockPayload> info) {
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		LOG4CXX_WARN(logger, from.toBare().toString() << ": User is not logged in");
		return true;
	}

	Buddy *buddy = user->getRosterManager()->getBuddy(Buddy::JIDToLegacyName(to));
	if (!buddy) {
		LOG4CXX_WARN(logger, from.toBare().toString() << ": Buddy " << Buddy::JIDToLegacyName(to) << " does not exist");
		return true;
	}

	if (buddy->isBlocked()) {
		LOG4CXX_INFO(logger, from.toBare().toString() << ": Unblocking buddy " << Buddy::JIDToLegacyName(to));
	}
	else {
		LOG4CXX_INFO(logger, from.toBare().toString() << ": Blocking buddy " << Buddy::JIDToLegacyName(to));
	}

	onBlockToggled(buddy);

	return true;
}

}
