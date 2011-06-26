/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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

#include "transport/admininterface.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/conversationmanager.h"
#include "transport/rostermanager.h"
#include "storageresponder.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("AdminInterface");

AdminInterface::AdminInterface(Component *component, StorageBackend *storageBackend) {
	m_component = component;
	m_storageBackend = storageBackend;

	m_component->getStanzaChannel()->onMessageReceived.connect(bind(&AdminInterface::handleMessageReceived, this, _1));
}

AdminInterface::~AdminInterface() {
}

void AdminInterface::handleMessageReceived(Swift::Message::ref message) {
	if (!message->getTo().getNode().empty())
		return;

	if (message->getFrom().getNode() != CONFIG_STRING(m_component->getConfig(), "service.admin_username")) {
		LOG4CXX_WARN(logger, "Message not from admin user, but from " << message->getFrom().getNode());
		return;
	}

	LOG4CXX_INFO(logger, "Message from admin received");
	message->setTo(message->getFrom());
	message->setFrom(m_component->getJID());

	message->setBody("Unknown command");

	m_component->getStanzaChannel()->sendMessage(message);
}

}
