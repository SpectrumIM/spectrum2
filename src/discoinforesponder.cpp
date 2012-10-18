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

#include "discoinforesponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "Swiften/Disco/DiscoInfoResponder.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Swiften.h"
#include "transport/config.h"
#include "transport/logging.h"

using namespace Swift;
using namespace boost;

DEFINE_LOGGER(logger, "DiscoInfoResponder");

namespace Transport {

DiscoInfoResponder::DiscoInfoResponder(Swift::IQRouter *router, Config *config) : Swift::GetResponder<DiscoInfo>(router) {
	m_config = config;
	m_transportInfo.addIdentity(DiscoInfo::Identity(CONFIG_STRING(m_config, "identity.name"),
													CONFIG_STRING(m_config, "identity.category"),
													CONFIG_STRING(m_config, "identity.type")));

	m_buddyInfo.addIdentity(DiscoInfo::Identity(CONFIG_STRING(m_config, "identity.name"), "client", "pc"));
	std::list<std::string> features;
	features.push_back("jabber:iq:register");
	features.push_back("jabber:iq:gateway");
	features.push_back("jabber:iq:private");
	features.push_back("http://jabber.org/protocol/disco#info");
	features.push_back("http://jabber.org/protocol/commands");
	setTransportFeatures(features);

	features.clear();
	features.push_back("http://jabber.org/protocol/disco#items");
	features.push_back("http://jabber.org/protocol/disco#info");
	features.push_back("http://jabber.org/protocol/chatstates");
	features.push_back("http://jabber.org/protocol/xhtml-im");
	setBuddyFeatures(features);
}

DiscoInfoResponder::~DiscoInfoResponder() {
	
}

void DiscoInfoResponder::setTransportFeatures(std::list<std::string> &features) {
	for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
		if (!m_transportInfo.hasFeature(*it)) {
			m_transportInfo.addFeature(*it);
		}
	}
}

void DiscoInfoResponder::setBuddyFeatures(std::list<std::string> &f) {
	for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
		if (!m_buddyInfo.hasFeature(*it)) {
			m_buddyInfo.addFeature(*it);
		}
	}

	CapsInfoGenerator caps("spectrum");
	m_capsInfo = caps.generateCapsInfo(m_buddyInfo);
	onBuddyCapsInfoChanged(m_capsInfo);
}

void DiscoInfoResponder::addRoom(const std::string &jid, const std::string &name) {
	std::string j = jid;
	boost::algorithm::to_lower(j);
	m_rooms[j] = name;
}

void DiscoInfoResponder::clearRooms() {
	m_rooms.clear();
}

bool DiscoInfoResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::DiscoInfo> info) {
	if (!info->getNode().empty()) {
		sendError(from, id, ErrorPayload::ItemNotFound, ErrorPayload::Cancel);
		return true;
	}


	// disco#info for transport
	if (to.getNode().empty()) {
		boost::shared_ptr<DiscoInfo> res(new DiscoInfo(m_transportInfo));
		res->setNode(info->getNode());
		sendResponse(from, id, res);
	}
	// disco#info for room
	else if (m_rooms.find(to.toBare().toString()) != m_rooms.end()) {
		boost::shared_ptr<DiscoInfo> res(new DiscoInfo());
		res->addIdentity(DiscoInfo::Identity(m_rooms[to.toBare().toString()], "conference", "text"));
		res->setNode(info->getNode());
		sendResponse(from, to, id, res);
	}
	// disco#info for buddy
	else {
		boost::shared_ptr<DiscoInfo> res(new DiscoInfo(m_buddyInfo));
		res->setNode(info->getNode());
		sendResponse(from, to, id, res);
	}
	return true;
}

}
