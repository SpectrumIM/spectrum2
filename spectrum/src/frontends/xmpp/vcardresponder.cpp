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

#include "vcardresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "transport/user.h"
#include "transport/buddy.h"
#include "transport/usermanager.h"
#include "transport/rostermanager.h"
#include "transport/transport.h"
#include "transport/logging.h"
#include "Swiften/Queries/IQRouter.h"

using namespace Swift;
using namespace boost;

namespace Transport {

DEFINE_LOGGER(logger, "VCardResponder");

VCardResponder::VCardResponder(Swift::IQRouter *router, Swift::NetworkFactories *factories, UserManager *userManager) : Swift::Responder<VCard>(router) {
	m_id = 0;
	m_userManager = userManager;
	m_collectTimer = factories->getTimerFactory()->createTimer(20000);
	m_collectTimer->onTick.connect(boost::bind(&VCardResponder::collectTimeouted, this));
	m_collectTimer->start();
}

VCardResponder::~VCardResponder() {
}

void VCardResponder::sendVCard(unsigned int id, boost::shared_ptr<Swift::VCard> vcard) {
	if (m_queries.find(id) == m_queries.end()) {
		LOG4CXX_WARN(logger, "Unexpected VCard from legacy network with id " << id);
		return;
	}

	LOG4CXX_INFO(logger, m_queries[id].from.toString() << ": Forwarding VCard of " << m_queries[id].to.toString() << " from legacy network");

	sendResponse(m_queries[id].from, m_queries[id].to, m_queries[id].id, vcard);
	m_queries.erase(id);
}

void VCardResponder::collectTimeouted() {
	time_t now = time(NULL);

	std::vector<unsigned int> candidates;
	for(std::map<unsigned int, VCardData>::iterator it = m_queries.begin(); it != m_queries.end(); it++) {
		if (now - (*it).second.received > 40) {
			candidates.push_back((*it).first);
		}
	}

	if (candidates.size() != 0) {
		LOG4CXX_INFO(logger, "Removing " << candidates.size() << " timeouted VCard requests");
	}

	BOOST_FOREACH(unsigned int id, candidates) {
		sendVCard(id, boost::shared_ptr<Swift::VCard>(new Swift::VCard()));
	}
	m_collectTimer->start();
}

bool VCardResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		LOG4CXX_WARN(logger, from.toBare().toString() << ": User is not logged in");
		return false;
	}

	Swift::JID to_ = to;

	std::string name = to_.getUnescapedNode();
	if (name.empty()) {
		to_ = user->getJID();
	}

	name = Buddy::JIDToLegacyName(to_);

	LOG4CXX_INFO(logger, from.toBare().toString() << ": Requested VCard of " << name);

	m_queries[m_id].from = from;
	m_queries[m_id].to = to;
	m_queries[m_id].id = id; 
	m_queries[m_id].received = time(NULL);
	onVCardRequired(user, name, m_id++);
	return true;
}

bool VCardResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	if (!to.getNode().empty()) {
		LOG4CXX_WARN(logger, from.toBare().toString() << ": Tried to set VCard of somebody else");
		return false;
	}

	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		LOG4CXX_WARN(logger, from.toBare().toString() << ": User is not logged in");
		return false;
	}

	LOG4CXX_INFO(logger, from.toBare().toString() << ": Setting VCard");
	onVCardUpdated(user, payload);

	sendResponse(from, id, boost::shared_ptr<VCard>(new VCard()));
	return true;
}

}
