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

#include "discoitemsresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "transport/Transport.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "discoinforesponder.h"
#include "XMPPFrontend.h"
#include "transport/Frontend.h"
#include "transport/UserManager.h"
#include "XMPPUser.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(discoItemsResponderLogger, "DiscoItemsResponder");

DiscoItemsResponder::DiscoItemsResponder(Component *component, UserManager *userManager) : Swift::GetResponder<DiscoItems>(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter()) {
	m_component = component;
	m_commands = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems>(new DiscoItems());
	m_commands->setNode("http://jabber.org/protocol/commands");

	m_rooms = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems>(new DiscoItems());
	m_discoInfoResponder = new DiscoInfoResponder(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter(), component->getConfig(), userManager);
	m_discoInfoResponder->start();

	m_userManager = userManager;
}

DiscoItemsResponder::~DiscoItemsResponder() {
	delete m_discoInfoResponder;
}

void DiscoItemsResponder::addAdHocCommand(const std::string &node, const std::string &name) {
	m_commands->addItem(DiscoItems::Item(name, m_component->getJID(), node));
	m_discoInfoResponder->addAdHocCommand(node, name);
}

void DiscoItemsResponder::addRoom(const std::string &node, const std::string &name) {
	if ((int) m_rooms->getItems().size() > CONFIG_INT(m_component->getConfig(), "service.max_room_list_size")) {
		return;
	}
	m_rooms->addItem(DiscoItems::Item(name, node));
	m_discoInfoResponder->addRoom(node, name);
}

void DiscoItemsResponder::clearRooms() {
	m_rooms = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems>(new DiscoItems());
	m_discoInfoResponder->clearRooms();
}

Swift::CapsInfo &DiscoItemsResponder::getBuddyCapsInfo() {
	return m_discoInfoResponder->getBuddyCapsInfo();
}


bool DiscoItemsResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoItems> info) {
	LOG4CXX_INFO(discoItemsResponderLogger, "get request received with node " << info->getNode());
	if (info->getNode() == "http://jabber.org/protocol/commands") {
		sendResponse(from, id, m_commands);
	}
	else if (to.getNode().empty() && info->getNode().empty()) {
		XMPPUser *user = static_cast<XMPPUser *>(m_userManager->getUser(from.toBare().toString()));
		if (!user) {
			sendResponse(from, id, m_rooms);
			return true;
		}

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems> rooms = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems>(new DiscoItems());
		BOOST_FOREACH(const DiscoItems::Item &item, m_rooms->getItems()) {
			rooms->addItem(item);
		}
		BOOST_FOREACH(const DiscoItems::Item &item, user->getRoomList()->getItems()) {
			rooms->addItem(item);
		}

		sendResponse(from, id, rooms);
	}
	else {
		sendResponse(from, id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DiscoItems>(new DiscoItems()));
	}
	return true;
}

}
