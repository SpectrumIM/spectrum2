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

#include "transport/discoitemsresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Swiften.h"
#include "transport/transport.h"
#include "transport/logging.h"

using namespace Swift;
using namespace boost;

namespace Transport {

DEFINE_LOGGER(logger, "DiscoItemsResponder");

DiscoItemsResponder::DiscoItemsResponder(Component *component) : Swift::GetResponder<DiscoItems>(component->getIQRouter()) {
	m_component = component;
	m_commands = boost::shared_ptr<DiscoItems>(new DiscoItems());
	m_commands->setNode("http://jabber.org/protocol/commands");

	m_rooms = boost::shared_ptr<DiscoItems>(new DiscoItems());
}

DiscoItemsResponder::~DiscoItemsResponder() {
	
}

void DiscoItemsResponder::addAdHocCommand(const std::string &node, const std::string &name) {
	m_commands->addItem(DiscoItems::Item(name, m_component->getJID(), node));
}

void DiscoItemsResponder::addRoom(const std::string &node, const std::string &name) {
	m_rooms->addItem(DiscoItems::Item(name, m_component->getJID(), node));
}

void DiscoItemsResponder::clearRooms() {
	m_rooms = boost::shared_ptr<DiscoItems>(new DiscoItems());
}


bool DiscoItemsResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::DiscoItems> info) {
	LOG4CXX_INFO(logger, "get request received with node " << info->getNode());
	if (info->getNode() == "http://jabber.org/protocol/commands") {
		sendResponse(from, id, m_commands);
	}
	else if (to.getNode().empty()) {
		sendResponse(from, id, m_rooms);
	}
	else {
		sendResponse(from, id, boost::shared_ptr<DiscoItems>(new DiscoItems()));
	}
	return true;
}

}
