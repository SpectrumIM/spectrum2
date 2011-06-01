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
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Swiften.h"
#include "transport/user.h"
#include "transport/usermanager.h"
#include "transport/rostermanager.h"

using namespace Swift;
using namespace boost;

namespace Transport {

VCardResponder::VCardResponder(Swift::IQRouter *router, StorageBackend *storageBackend, UserManager *userManager) : Swift::Responder<VCard>(router) {
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

VCardResponder::~VCardResponder() {
}

bool VCardResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	// Get means we're in server mode and user wants to fetch his roster.
	// For now we send empty reponse, but TODO: Get buddies from database and send proper stored roster.
	onVCardRequired(from, to, id);
	return true;
}

bool VCardResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload) {
	sendResponse(from, id, boost::shared_ptr<VCard>(new VCard()));
	return true;
}

}
