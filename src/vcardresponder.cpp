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

#include "transport/vcardresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Swiften.h"
#include "transport/user.h"
#include "transport/usermanager.h"
#include "transport/rostermanager.h"
#include "transport/transport.h"

using namespace Swift;
using namespace boost;

namespace Transport {

VCardResponder::VCardResponder(Swift::IQRouter *router, UserManager *userManager) : Swift::Responder<VCard>(router) {
	m_id = 0;
	m_userManager = userManager;
}

VCardResponder::~VCardResponder() {
}

void VCardResponder::sendVCard(unsigned int id, boost::shared_ptr<Swift::VCard> vcard) {
	std::cout << "RECEIVED VCARD FROM BACKEND\n";
	if (m_queries.find(id) == m_queries.end()) {
		std::cout << "ERROR\n";
		return;
	}
	std::cout << "SENT " << m_queries[id].to << " " << m_queries[id].from << " " << m_queries[id].id << "\n";
	sendResponse(m_queries[id].from, m_queries[id].to, m_queries[id].id, vcard);
	m_queries.erase(id);
}

bool VCardResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	// Get means we're in server mode and user wants to fetch his roster.
	// For now we send empty reponse, but TODO: Get buddies from database and send proper stored roster.
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		return false;
	}

	Swift::JID to_ = to;

	std::string name = to_.getUnescapedNode();
	if (name.empty()) {
		to_ = user->getComponent()->getJID();
		std::string name = to_.getUnescapedNode();
	}

	if (name.find_last_of("%") != std::string::npos) {
		name.replace(name.find_last_of("%"), 1, "@");
	}

	m_queries[m_id].from = from;
	m_queries[m_id].to = to_;
	m_queries[m_id].id = id; 
	onVCardRequired(user, name, m_id++);
	return true;
}

bool VCardResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	if (!to.getNode().empty()) {
		return false;
	}

	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		return false;
	}

	onVCardUpdated(user, payload);

	sendResponse(from, id, boost::shared_ptr<VCard>(new VCard()));
	return true;
}

}
