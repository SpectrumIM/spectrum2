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

	m_rtm = new SlackRTM(component, storageBackend, uinfo);
	m_rtm->onRTMStarted.connect(boost::bind(&SlackInstallation::handleRTMStarted, this));
	m_rtm->onMessageReceived.connect(boost::bind(&SlackInstallation::handleMessageReceived, this, _1, _2, _3));
}

SlackInstallation::~SlackInstallation() {
	delete m_rtm;
}

void SlackInstallation::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message) {
	if (m_ownerChannel == channel) {
		LOG4CXX_INFO(logger, "Owner message received " << channel << " " << user << " " << message);
	}
}

void SlackInstallation::handleImOpen(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	m_ownerChannel = m_rtm->getAPI()->getChannelId(req, ok, resp, data);
	LOG4CXX_INFO(logger, "Opened channel with team owner: " << m_ownerChannel);

	std::string msg;
	msg = "Hi, it seems you have enabled Spectrum 2 transport for your Team. As a Team owner, you should now configure it.";
	m_rtm->sendMessage(m_ownerChannel, msg);

	msg = "To configure IRC network you want to connect to, type: \".spectrum2 register <bot_name>@<ircnetwork>\". For example for Freenode, the command looks like \".spectrum2 register MySlackBot@irc.freenode.net\".";
	m_rtm->sendMessage(m_ownerChannel, msg);
}

void SlackInstallation::handleRTMStarted() {
	std::string ownerId;
	std::map<std::string, SlackUserInfo> &users = m_rtm->getUsers();
	for (std::map<std::string, SlackUserInfo>::iterator it = users.begin(); it != users.end(); it++) {
		SlackUserInfo &info = it->second;
		if (info.isPrimaryOwner) {
			ownerId = it->first;
			break;
		}
	}

	m_rtm->getAPI()->imOpen(ownerId, boost::bind(&SlackInstallation::handleImOpen, this, _1, _2, _3, _4));
}


}
