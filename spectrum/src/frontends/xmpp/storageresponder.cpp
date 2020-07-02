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

#include "storageresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/RawXMLPayload.h"
#include "Swiften/Elements/Storage.h"
#include "Swiften/Serializer/PayloadSerializers/StorageSerializer.h"
#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Logging.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(storageResponderLogger, "StorageResponder");

StorageResponder::StorageResponder(Swift::IQRouter *router, StorageBackend *storageBackend, UserManager *userManager) : Swift::Responder<PrivateStorage>(router) {
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

StorageResponder::~StorageResponder() {
}

bool StorageResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::PrivateStorage> payload) {
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		LOG4CXX_WARN(storageResponderLogger, from.toBare().toString() << ": User is not logged in");
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Cancel);
		return true;
	}

	int type = 0;
	std::string value = "";
	m_storageBackend->getUserSetting(user->getUserInfo().id, "storage", type, value);
	LOG4CXX_INFO(storageResponderLogger, from.toBare().toString() << ": Sending jabber:iq:storage");

	sendResponse(from, id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<PrivateStorage>(new PrivateStorage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<RawXMLPayload>(new RawXMLPayload(value)))));
	return true;
}

bool StorageResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::PrivateStorage> payload) {
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Cancel);
		LOG4CXX_WARN(storageResponderLogger, from.toBare().toString() << ": User is not logged in");
		return true;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Storage> storage = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Storage>(payload->getPayload());

	if (storage) {
		StorageSerializer serializer;
		std::string value = serializer.serializePayload(SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Storage>(payload->getPayload()));
		m_storageBackend->updateUserSetting(user->getUserInfo().id, "storage", value);
		LOG4CXX_INFO(storageResponderLogger, from.toBare().toString() << ": Storing jabber:iq:storage");
		sendResponse(from, id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<PrivateStorage>());
	}
	else {
		LOG4CXX_INFO(storageResponderLogger, from.toBare().toString() << ": Unknown element. Libtransport does not support serialization of this.");
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Cancel);
	}
	return true;
}

}
