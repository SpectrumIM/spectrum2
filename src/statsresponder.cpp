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

#include "transport/statsresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "transport/BlockPayload.h"
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

static LoggerPtr logger = Logger::getLogger("StatsResponder");

StatsResponder::StatsResponder(Component *component, UserManager *userManager, NetworkPluginServer *server, StorageBackend *storageBackend) : Swift::Responder<StatsPayload>(component->getIQRouter()) {
	m_component = component;
	m_userManager = userManager;
	m_server = server;
	m_storageBackend = storageBackend;
	m_start = time(0);
}

StatsResponder::~StatsResponder() {
	
}

bool StatsResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<StatsPayload> stats) {
	boost::shared_ptr<StatsPayload> response(new StatsPayload());

	if (stats->getItems().empty()) {
		response->addItem(StatsPayload::Item("uptime"));
	}
	else {
		BOOST_FOREACH(const StatsPayload::Item &item, stats->getItems()) {
			if (item.getName() == "uptime") {
				response->addItem(StatsPayload::Item("uptime", "seconds", boost::lexical_cast<std::string>(time(0) - m_start)));
			}
		}
	}

	sendResponse(from, id, response);

	return true;
}

bool StatsResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<StatsPayload> stats) {
	return false;
}

}
