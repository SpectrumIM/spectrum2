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
#include "transport/Buddy.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "Swiften/Elements/MUCPayload.h"

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

// 	m_api = new SlackAPI(component, m_uinfo.encoding);
}

SlackInstallation::~SlackInstallation() {
	delete m_rtm;
// 	delete m_api;
}

void SlackInstallation::sendMessage(boost::shared_ptr<Swift::Message> message) {
	LOG4CXX_INFO(logger, "SEND MESSAGE");
	if (message->getFrom().getResource() == "myfavouritebot") {
		return;
	}
	m_rtm->getAPI()->sendMessage(message->getFrom().getResource(), m_jid2channel[message->getFrom().toBare().toString()], message->getBody());
}

void SlackInstallation::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message) {
	if (m_ownerChannel != channel) {
		std::string to = m_channel2jid[channel];
		if (!to.empty()) {
			boost::shared_ptr<Swift::Message> msg(new Swift::Message());
			msg->setType(Swift::Message::Groupchat);
			msg->setTo(to);
			msg->setFrom(Swift::JID("", "spectrum2", "default"));
			msg->setBody("<" + user + "> " + message);
			m_component->getFrontend()->onMessageReceived(msg);
		}
		return;
	}

	std::vector<std::string> args;
	boost::split(args, message, boost::is_any_of(" "));

	if (args.size() < 2 || args[0] != ".spectrum2") {
		m_rtm->sendMessage(m_ownerChannel, "Unknown command. Use \".spectrum2 help\" for help.");
		return;
	}

	if (args[1] == "join") {
		// .spectrum2 join BotName #room irc.freenode.net channel
		LOG4CXX_INFO(logger, "Received JOIN request" << args.size());
		if (args.size() == 6) {
			LOG4CXX_INFO(logger, "Received JOIN request");
			if (args[4][0] == '<') {
				args[4] = args[4].substr(1, args[4].size() - 2);
				args[4] = args[4].substr(args[4].find("|") + 1);
				LOG4CXX_INFO(logger, args[4]);
			}

			if (args[5][0] == '<') {
				args[5] = args[5].substr(2, args[5].size() - 3);
			}


			m_uinfo.uin = args[2];
			m_storageBackend->setUser(m_uinfo);


			std::string to = args[3] + "%" + args[4] + "@localhost";
			m_jid2channel[to] = args[5];
			m_channel2jid[args[5]] = to;
			LOG4CXX_INFO(logger, "Setting transport between " << to << " and " << args[5]);
		// 	int type = (int) TYPE_STRING;
		// 	m_storageBackend->getUserSetting(user.id, "room", type, to);

			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setFrom(Swift::JID("", "spectrum2", "default"));
			presence->setTo(Swift::JID(to + "/" + args[2]));
			presence->setType(Swift::Presence::Available);
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			m_component->getFrontend()->onPresenceReceived(presence);
		}
	}
	else if (args[1] == "help") {
		
	}
	else {
		m_rtm->sendMessage(m_ownerChannel, "Unknown command. Use \".spectrum2 help\" for help.");
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
