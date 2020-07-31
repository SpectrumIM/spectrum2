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
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "Swiften/Disco/DiscoInfoResponder.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "transport/Config.h"
#include "transport/Logging.h"
#include "Swiften/Disco/CapsInfoGenerator.h"
#include "XMPPFrontend.h"
#include "transport/UserManager.h"
#include "XMPPUser.h"

using namespace Swift;

DEFINE_LOGGER(logger, "DiscoInfoResponder");

namespace Transport {

DiscoInfoResponder::DiscoInfoResponder(Swift::IQRouter *router, Config *config, UserManager *userManager) : Swift::GetResponder<DiscoInfo>(router) {
	m_config = config;
	m_config->onBackendConfigUpdated.connect(boost::bind(&DiscoInfoResponder::updateFeatures, this));
	m_buddyInfo = NULL;
	m_transportInfo.addIdentity(DiscoInfo::Identity(CONFIG_STRING(m_config, "identity.name"),
													CONFIG_STRING(m_config, "identity.category"),
													CONFIG_STRING(m_config, "identity.type")));
	crypto = std::shared_ptr<CryptoProvider>(PlatformCryptoProvider::create());

	updateFeatures();
	m_userManager = userManager;
}

DiscoInfoResponder::~DiscoInfoResponder() {
	delete m_buddyInfo;
}

//Adds an advertised transport feature for the duration of this session
void DiscoInfoResponder::addTransportFeature(const std::string &feature)
{
	std::vector<std::string>::iterator it = std::find(this->m_transportFeatures.begin(), this->m_transportFeatures.end(), feature);
	if (it != this->m_transportFeatures.end())
		return; //already present
	LOG4CXX_TRACE(logger, "Adding transport feature: '" << feature << "'");
	this->m_transportFeatures.push_back(feature);
	this->updateFeatures();
}

void DiscoInfoResponder::removeTransportFeature(const std::string &feature)
{
	std::vector<std::string>::iterator it = std::find(this->m_transportFeatures.begin(), this->m_transportFeatures.end(), feature);
	if (it == this->m_transportFeatures.end())
		return; //already missing
	LOG4CXX_TRACE(logger, "Removing transport feature: '" << feature << "'");
	this->m_transportFeatures.erase(it);
	this->updateFeatures();
}

//Compiles and re-sets the resulting list of transport and buddy features
void DiscoInfoResponder::updateFeatures() {
	std::list<std::string> features2;
	features2.push_back("jabber:iq:register");
	features2.push_back("jabber:iq:gateway");
	features2.push_back("jabber:iq:private");
	features2.push_back("http://jabber.org/protocol/disco#info");
	features2.push_back("http://jabber.org/protocol/commands");
	if (CONFIG_BOOL_DEFAULTED(m_config, "features.muc", false)) {
		features2.push_back("http://jabber.org/protocol/muc");
	}
	features2.insert(features2.end(), this->m_transportFeatures.begin(), this->m_transportFeatures.end());
	setTransportFeatures(features2);

	std::list<std::string> features;
	features.push_back("http://jabber.org/protocol/disco#items");
	features.push_back("http://jabber.org/protocol/disco#info");
	features.push_back("http://jabber.org/protocol/chatstates");
	features.push_back("http://jabber.org/protocol/xhtml-im");
	if (CONFIG_BOOL_DEFAULTED(m_config, "features.receipts", false)) {
		features.push_back("urn:xmpp:receipts");
	}
	setBuddyFeatures(features);
}

void DiscoInfoResponder::setTransportFeatures(std::list<std::string> &features) {
	LOG4CXX_TRACE(logger, "Setting transport features");
	for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
		if (!m_transportInfo.hasFeature(*it)) {
			m_transportInfo.addFeature(*it);
		}
	}
}

void DiscoInfoResponder::setBuddyFeatures(std::list<std::string> &f) {
	delete m_buddyInfo;
	m_buddyInfo = new Swift::DiscoInfo;
	m_buddyInfo->addIdentity(DiscoInfo::Identity(CONFIG_STRING(m_config, "identity.name"), "client", "pc"));

	for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
		if (!m_buddyInfo->hasFeature(*it)) {
			m_buddyInfo->addFeature(*it);
		}
	}
	CapsInfoGenerator caps("spectrum", crypto.get());
	m_capsInfo = caps.generateCapsInfo(*m_buddyInfo);
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

void DiscoInfoResponder::addAdHocCommand(const std::string &node, const std::string &name) {
	m_commands[node] = node;
}

bool DiscoInfoResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, std::shared_ptr<Swift::DiscoInfo> info) {
	// disco#info for transport
	if (to.getNode().empty()) {
		// Adhoc command
		if (m_commands.find(info->getNode()) != m_commands.end()) {
			std::shared_ptr<DiscoInfo> res(new DiscoInfo());
			res->addFeature("http://jabber.org/protocol/commands");
			res->addFeature("jabber:x:data");
			res->addIdentity(DiscoInfo::Identity(m_commands[info->getNode()], "automation", "command-node"));
			res->setNode(info->getNode());
			sendResponse(from, to, id, res);
		}
		else if (info->getNode() == "http://jabber.org/protocol/commands") {
			std::shared_ptr<DiscoInfo> res(new DiscoInfo());
			res->addIdentity(DiscoInfo::Identity("Commands", "automation", "command-list"));
			res->setNode(info->getNode());
			sendResponse(from, to, id, res);
		}
		else {
			if (!info->getNode().empty()) {
				sendError(from, id, ErrorPayload::ItemNotFound, ErrorPayload::Cancel);
				return true;
			}

			LOG4CXX_TRACE(logger, "handleGetRequest(): Sending features");
			std::shared_ptr<DiscoInfo> res(new DiscoInfo(m_transportInfo));
			res->setNode(info->getNode());
			sendResponse(from, id, res);
		}
		return true;
	}

	// disco#info for room
	if (m_rooms.find(to.toBare().toString()) != m_rooms.end()) {
		std::shared_ptr<DiscoInfo> res(new DiscoInfo());
		res->addIdentity(DiscoInfo::Identity(m_rooms[to.toBare().toString()], "conference", "text"));
		res->addFeature("http://jabber.org/protocol/muc");
		res->setNode(info->getNode());
		sendResponse(from, to, id, res);
		return true;
	}

	// disco#info for per-user rooms (like Skype/Facebook groupchats)
	XMPPUser *user = static_cast<XMPPUser *>(m_userManager->getUser(from.toBare().toString()));
	if (user) {
		BOOST_FOREACH(const DiscoItems::Item &item, user->getRoomList()->getItems()) {
			LOG4CXX_INFO(logger, "XXX " << item.getNode() << " " << to.toBare().toString());
			if (item.getJID().toString() == to.toBare().toString()) {
				std::shared_ptr<DiscoInfo> res(new DiscoInfo());
				res->addIdentity(DiscoInfo::Identity(item.getName(), "conference", "text"));
				res->addFeature("http://jabber.org/protocol/muc");
				res->setNode(info->getNode());
				sendResponse(from, to, id, res);
				return true;
			}
		}
	}

	// disco#info for buddy
	std::shared_ptr<DiscoInfo> res(new DiscoInfo(*m_buddyInfo));
	res->setNode(info->getNode());
	sendResponse(from, to, id, res);
	return true;
}

}
