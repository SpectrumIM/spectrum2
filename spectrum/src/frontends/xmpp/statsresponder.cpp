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

#include "statsresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/RosterManager.h"
#include "transport/MemoryUsage.h"
#include "transport/NetworkPluginServer.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "transport/Transport.h"
#include "transport/Buddy.h"
#include "XMPPFrontend.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(statsResponderLogger, "StatsResponder");

StatsResponder::StatsResponder(Component *component, UserManager *userManager, NetworkPluginServer *server, StorageBackend *storageBackend) : Swift::Responder<StatsPayload>(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter()) {
	m_component = component;
	m_userManager = userManager;
	m_server = server;
	m_storageBackend = storageBackend;
	m_start = time(0);
}

StatsResponder::~StatsResponder() {
	
}

unsigned long StatsResponder::usedMemory() {
	double shared = 0;
	double rss = 0;
#ifndef WIN32
	process_mem_usage(shared, rss);
#endif
	rss -= shared;

	const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
	BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
		rss += backend->res - backend->shared;
	}

	return (unsigned long) rss;
}

bool StatsResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<StatsPayload> stats) {
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<StatsPayload> response(new StatsPayload());

	if (stats->getItems().empty()) {
		response->addItem(StatsPayload::Item("uptime"));
		response->addItem(StatsPayload::Item("users/online"));
		response->addItem(StatsPayload::Item("contacts/online"));
		response->addItem(StatsPayload::Item("contacts/total"));
		response->addItem(StatsPayload::Item("messages/from-xmpp"));
		response->addItem(StatsPayload::Item("messages/to-xmpp"));
		response->addItem(StatsPayload::Item("backends/running"));
		response->addItem(StatsPayload::Item("backends/crashed"));
		response->addItem(StatsPayload::Item("memory-usage"));
	}
	else {
		unsigned long contactsOnline = 0;
		unsigned long contactsTotal = 0;

		Swift::StatusShow s;
		std::string statusMessage;
		for (std::map<std::string, User *>::const_iterator it = m_userManager->getUsers().begin(); it != m_userManager->getUsers().end(); it++) {
			if (!(*it).second) {
				continue;
			}
			const RosterManager::BuddiesMap &buddies = (*it).second->getRosterManager()->getBuddies();
			contactsTotal += buddies.size();
			for (RosterManager::BuddiesMap::const_iterator bt = buddies.begin(); bt != buddies.end(); bt++) {
				if (!(*bt).second) {
					continue;
				}
				if (!(*bt).second->getStatus(s, statusMessage))
					continue;
				if (s.getType() != Swift::StatusShow::None) {
					contactsOnline++;
				}
			}
		}

		BOOST_FOREACH(const StatsPayload::Item &item, stats->getItems()) {
			if (item.getName() == "uptime") {
				response->addItem(StatsPayload::Item("uptime", "seconds", boost::lexical_cast<std::string>(time(0) - m_start)));
			}
			else if (item.getName() == "users/online") {
				response->addItem(StatsPayload::Item("users/online", "users", boost::lexical_cast<std::string>(m_userManager->getUserCount())));
			}
			else if (item.getName() == "backends/running") {
				response->addItem(StatsPayload::Item("backends/running", "backends", boost::lexical_cast<std::string>(m_server->getBackendCount())));
			}
			else if (item.getName() == "backends/crashed") {
				response->addItem(StatsPayload::Item("backends/crashed", "backends", boost::lexical_cast<std::string>(m_server->getCrashedBackends().size())));
			}
			else if (item.getName() == "memory-usage") {
				response->addItem(StatsPayload::Item("memory-usage", "KB", boost::lexical_cast<std::string>(usedMemory())));
			}
			else if (item.getName() == "contacts/online") {
				response->addItem(StatsPayload::Item("contacts/online", "contacts", boost::lexical_cast<std::string>(contactsOnline)));
			}
			else if (item.getName() == "contacts/total") {
				response->addItem(StatsPayload::Item("contacts/total", "contacts", boost::lexical_cast<std::string>(contactsTotal)));
			}
			else if (item.getName() == "messages/from-xmpp") {
				response->addItem(StatsPayload::Item("messages/from-xmpp", "messages", boost::lexical_cast<std::string>(m_userManager->getMessagesToBackend())));
			}
			else if (item.getName() == "messages/to-xmpp") {
				response->addItem(StatsPayload::Item("messages/to-xmpp", "messages", boost::lexical_cast<std::string>(m_userManager->getMessagesToXMPP())));
			}
		}
	}

	sendResponse(from, id, response);

	return true;
}

bool StatsResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<StatsPayload> stats) {
	return false;
}

}
