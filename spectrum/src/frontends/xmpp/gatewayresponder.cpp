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

#include "gatewayresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/RawXMLPayload.h"
#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/Logging.h"
#include "transport/Config.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(gatewayResponderLogger, "GatewayResponder");

GatewayResponder::GatewayResponder(Swift::IQRouter *router, UserManager *userManager) : Swift::Responder<GatewayPayload>(router) {
	m_userManager = userManager;
}

GatewayResponder::~GatewayResponder() {
}

bool GatewayResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GatewayPayload> payload) {
	std::string prompt = CONFIG_STRING(m_userManager->getComponent()->getConfig(), "gateway_responder.prompt");
	std::string label = CONFIG_STRING(m_userManager->getComponent()->getConfig(), "gateway_responder.label");
	sendResponse(from, id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<GatewayPayload>(new GatewayPayload(Swift::JID(), label, prompt)));
	return true;
}

bool GatewayResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GatewayPayload> payload) {
	std::string prompt = payload->getPrompt();

	std::string escaped = Swift::JID::getEscapedNode(prompt);
	if (!CONFIG_BOOL_DEFAULTED(m_userManager->getComponent()->getConfig(), "service.jid_escaping", true)) {
		escaped = prompt;
		if (escaped.find_last_of("@") != std::string::npos) {
			escaped.replace(escaped.find_last_of("@"), 1, "%");
		}
	}
	// This code is here to workaround Gajim (and probably other clients bug too) bug
	// https://trac.gajim.org/ticket/7277
	if (prompt.find("\\40") != std::string::npos) {
		LOG4CXX_WARN(gatewayResponderLogger, from.toString() << " Received already escaped JID " << prompt << ". Not escaping again.");
		escaped = prompt;
	}

	std::string jid = escaped + "@" + m_userManager->getComponent()->getJID().toBare().toString();

	sendResponse(from, id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<GatewayPayload>(new GatewayPayload(jid)));
	return true;
}

}
