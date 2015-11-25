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

#include "SlackSession.h"
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

DEFINE_LOGGER(logger, "SlackSession");

SlackSession::SlackSession(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
	m_storageBackend = storageBackend;

	m_rtm = new SlackRTM(component, storageBackend, uinfo);
	m_rtm->onRTMStarted.connect(boost::bind(&SlackSession::handleRTMStarted, this));
	m_rtm->onMessageReceived.connect(boost::bind(&SlackSession::handleMessageReceived, this, _1, _2, _3, false));

}

SlackSession::~SlackSession() {
	delete m_rtm;
}

void SlackSession::sendMessage(boost::shared_ptr<Swift::Message> message) {
	if (message->getFrom().getResource() == m_uinfo.uin) {
		return;
	}

	std::string channel = m_jid2channel[message->getFrom().toBare().toString()];
	if (channel.empty()) {
		LOG4CXX_ERROR(logger, m_uinfo.jid << ": Received message for unknown channel from " << message->getFrom().toBare().toString());
		return;
	}

	m_rtm->getAPI()->sendMessage(message->getFrom().getResource(), channel, message->getBody());
}

void SlackSession::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message, bool quiet) {
	if (m_ownerChannel != channel) {
		std::string to = m_channel2jid[channel];
		if (!to.empty()) {
			boost::shared_ptr<Swift::Message> msg(new Swift::Message());
			msg->setType(Swift::Message::Groupchat);
			msg->setTo(to);
			msg->setFrom(Swift::JID("", m_uinfo.jid, "default"));
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

	if (args[1] == "join.room") {
		// .spectrum2 join.room BotName #room irc.freenode.net channel
		if (args.size() == 6) {
			std::string &name = args[2];
			std::string legacyRoom = SlackAPI::SlackObjectToPlainText(args[3], true);
			std::string legacyServer = SlackAPI::SlackObjectToPlainText(args[4]);
			std::string slackChannel = SlackAPI::SlackObjectToPlainText(args[5], true);

			m_uinfo.uin = name;
			m_storageBackend->setUser(m_uinfo);

			std::string to = legacyRoom + "%" + legacyServer + "@" + m_component->getJID().toString();
			m_jid2channel[to] = slackChannel;
			m_channel2jid[slackChannel] = to;

			LOG4CXX_INFO(logger, "Setting transport between " << to << " and " << slackChannel);

			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);
			rooms += message + "\n";
			m_storageBackend->updateUserSetting(m_uinfo.id, "rooms", rooms);

			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setFrom(Swift::JID("", m_uinfo.jid, "default"));
			presence->setTo(Swift::JID(to + "/" + name));
			presence->setType(Swift::Presence::Available);
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			m_component->getFrontend()->onPresenceReceived(presence);

			if (!quiet) {
				std::string msg;
				msg += "Spectrum 2 is now joining the room. To leave the room later to disable transporting, you can use `.spectrum2 leave.room #" + SlackAPI::SlackObjectToPlainText(args[5], true, true) + "`.";
				m_rtm->sendMessage(m_ownerChannel, msg);
			}
		}
	}
	else if (args[1] == "leave.room") {
		// .spectrum2 leave.room channel
		if (args.size() == 3) {
			std::string slackChannel = SlackAPI::SlackObjectToPlainText(args[2], true);
			std::string to = m_channel2jid[slackChannel];
			if (to.empty()) {
				m_rtm->sendMessage(m_ownerChannel, "Spectrum 2 is not configured to transport this Slack channel.");
				return;
			}

			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

			std::vector<std::string> commands;
			boost::split(commands, rooms, boost::is_any_of("\n"));
			rooms = "";

			BOOST_FOREACH(const std::string &command, commands) {
				if (command.size() > 5) {
					std::vector<std::string> args2;
					boost::split(args2, command, boost::is_any_of(" "));
					if (args2.size() == 6) {
						if (slackChannel != SlackAPI::SlackObjectToPlainText(args2[5], true)) {
							rooms += command + "\n";
						}
					}
				}
			}

			m_storageBackend->updateUserSetting(m_uinfo.id, "rooms", rooms);

			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setFrom(Swift::JID("", m_uinfo.jid, "default"));
			presence->setTo(Swift::JID(to + "/" + m_uinfo.uin));
			presence->setType(Swift::Presence::Unavailable);
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
			m_component->getFrontend()->onPresenceReceived(presence);
		}
	}
	else if (args[1] == "list.rooms") {
		// .spectrum2 list.rooms
		if (args.size() == 2) {
			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

			std::string msg = "Spectrum 2 is configured for following channels:\\n";
			msg += "```" + rooms  + "```";
			m_rtm->sendMessage(m_ownerChannel, msg);
		}
	}
	else if (args[1] == "help") {
		std::string msg;
		msg =  "Following commands are supported:\\n";
		msg += "```.spectrum2 help``` Shows this help message.\\n";
		msg += "```.spectrum2 join.room <3rdPartyBotName> <#3rdPartyRoom> <3rdPartyServer> <#SlackChannel>``` Starts transport between 3rd-party room and Slack channel.";
		msg += "```.spectrum2 leave.room <#SlackChannel>``` Leaves the 3rd-party room connected with the given Slack channel.";
		msg += "```.spectrum2 list.rooms``` List all the transported rooms.";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
	else {
		m_rtm->sendMessage(m_ownerChannel, "Unknown command. Use `.spectrum2 help` for help.");
	}
}

void SlackSession::handleImOpen(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	m_ownerChannel = m_rtm->getAPI()->getChannelId(req, ok, resp, data);
	LOG4CXX_INFO(logger, "Opened channel with team owner: " << m_ownerChannel);

	std::string rooms = "";
	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

	if (rooms.empty()) {
		std::string msg;
		msg =  "Hi, it seems you have enabled Spectrum 2 transport for your Team. As a Team owner, you should now configure it:\\n";
		msg += "1. At first, create new channel in which you want this Spectrum 2 transport to send the messages, or choose the existing one.\\n";
		msg += "2. Invite this Spectrum 2 bot into this channel.\\n";
		msg += "3. Configure the transportation between 3rd-party network and this channel by executing following command in this chat:\\n";
		msg += "```.spectrum2 join.room NameOfYourBotIn3rdPartyNetwork #3rdPartyRoom hostname_of_3rd_party_server #SlackChannel```\\n";
		msg += "For example to join #test123 channel on Freenode IRC server as MyBot and transport it into #slack_channel, the command would look like this:\\n";
		msg += "```.spectrum2 join.room MyBot #test123 adams.freenode.net #slack_channel```\\n";
		msg += "To get full list of available commands, executa `.spectrum2 help`\\n";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
	else {
		std::vector<std::string> commands;
		boost::split(commands, rooms, boost::is_any_of("\n"));

		BOOST_FOREACH(const std::string &command, commands) {
			if (command.size() > 5) {
				LOG4CXX_INFO(logger, m_uinfo.jid << ": Sending command from storage: " << command);
				handleMessageReceived(m_ownerChannel, "owner", command, true);
			}
		}
	}
}

void SlackSession::handleRTMStarted() {
	std::string ownerId;
	std::map<std::string, SlackUserInfo> &users = m_rtm->getUsers();
	for (std::map<std::string, SlackUserInfo>::iterator it = users.begin(); it != users.end(); it++) {
		SlackUserInfo &info = it->second;
		if (info.isPrimaryOwner) {
			ownerId = it->first;
			break;
		}
	}

	m_rtm->getAPI()->imOpen(ownerId, boost::bind(&SlackSession::handleImOpen, this, _1, _2, _3, _4));
}


}
