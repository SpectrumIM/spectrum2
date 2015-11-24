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

#include "SlackInstallation.h"
#include "SlackFrontend.h"
#include "SlackUser.h"
#include "SlackRTM.h"
#include "SlackAPI.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "SlackInstallation");

SlackInstallation::SlackInstallation(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_api = new SlackAPI(component, uinfo);


	m_api->usersList(boost::bind(&SlackInstallation::handleUsersList, this, _1, _2, _3, _4));
// 	m_rtm = new SlackRTM(component, storageBackend, uinfo);
}

SlackInstallation::~SlackInstallation() {
// 	delete m_rtm;
	delete m_api;
}

void SlackInstallation::handleImOpen(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	std::string channel = m_api->getChannelId(req, ok, resp, data);
	LOG4CXX_INFO(logger, "Opened channel with team owner: " << channel);

	std::string msg;
	msg = "Hi, It seems you have authorized Spectrum 2 transport for your team. "
		"As a team owner, you should now configure it. You should provide username and "
		"password you want to use to connect your team to legacy network of your choice.";
	m_api->sendMessage("Spectrum 2", channel, msg);

	msg = "You can do it by typing \".spectrum2 register <username> <password>\". Password may be optional.";
	m_api->sendMessage("Spectrum 2", channel, msg);

	msg = "For example to connect the Freenode IRC network, just type \".spectrum2 register irc.freenode.net\".";
	m_api->sendMessage("Spectrum 2", channel, msg);
}

void SlackInstallation::handleUsersList(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	std::string ownerId = m_api->getOwnerId(req, ok, resp, data);
	LOG4CXX_INFO(logger, "Team owner ID is " << ownerId);

	m_api->imOpen(ownerId, boost::bind(&SlackInstallation::handleImOpen, this, _1, _2, _3, _4));
}


}
