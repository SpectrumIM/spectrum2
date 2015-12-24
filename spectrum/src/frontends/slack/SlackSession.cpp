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
#include "SlackRosterManager.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"
#include "transport/Buddy.h"
#include "transport/Config.h"
#include "transport/ConversationManager.h"
#include "transport/Conversation.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "Swiften/Elements/MUCPayload.h"

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "SlackSession");

SlackSession::SlackSession(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo), m_user(NULL), m_disconnected(false) {
	m_component = component;
	m_storageBackend = storageBackend;

	m_rtm = new SlackRTM(component, storageBackend, uinfo);
	m_rtm->onRTMStarted.connect(boost::bind(&SlackSession::handleRTMStarted, this));
	m_rtm->onMessageReceived.connect(boost::bind(&SlackSession::handleMessageReceived, this, _1, _2, _3, _4, false));

	m_onlineBuddiesTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_onlineBuddiesTimer->onTick.connect(boost::bind(&SlackSession::sendOnlineBuddies, this));
}

SlackSession::~SlackSession() {
	delete m_rtm;
	m_onlineBuddiesTimer->stop();
}

void SlackSession::sendOnlineBuddies() {
	if (!m_user) {
		return;
	}
	std::map<std::string, Conversation *> convs = m_user->getConversationManager()->getConversations();
	for (std::map<std::string, Conversation *> ::const_iterator it = convs.begin(); it != convs.end(); it++) {
		Conversation *conv = it->second;
		if (!conv) {
			continue;
		}

		std::string onlineBuddies = "Online users: " + conv->getParticipants();

		if (m_onlineBuddies[it->first] != onlineBuddies) {
			m_onlineBuddies[it->first] = onlineBuddies;
			std::string legacyName = it->first;
			if (legacyName.find_last_of("@") != std::string::npos) {
				legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
			}


			std::string to = legacyName + "@" + m_component->getJID().toBare().toString();
			setPurpose(onlineBuddies, m_jid2channel[to]);
		}
	}
	m_onlineBuddiesTimer->start();
}

void SlackSession::sendMessageToAll(const std::string &msg) {
	std::vector<std::string> channels;
	for (std::map<std::string, std::string>::const_iterator it = m_jid2channel.begin(); it != m_jid2channel.end(); it++) {
		if (std::find(channels.begin(), channels.end(), it->second) == channels.end()) {
			channels.push_back(it->second);
			m_rtm->getAPI()->sendMessage("Soectrum 2", it->second, msg);
		}
	}
}

void SlackSession::sendMessage(boost::shared_ptr<Swift::Message> message) {
	if (m_user) {
		std::map<std::string, Conversation *> convs = m_user->getConversationManager()->getConversations();
		for (std::map<std::string, Conversation *> ::const_iterator it = convs.begin(); it != convs.end(); it++) {
			Conversation *conv = it->second;
			if (!conv) {
				continue;
			}

			if (conv->getNickname() == message->getFrom().getResource()) {
				return;
			}
		}
	}

	std::string from = message->getFrom().getResource();
	std::string channel = m_jid2channel[message->getFrom().toBare().toString()];
	LOG4CXX_INFO(logger, "JID is " << message->getFrom().toBare().toString() << " channel is " << channel);
	if (channel.empty()) {
		if (m_slackChannel.empty()) {
			LOG4CXX_ERROR(logger, m_uinfo.jid << ": Received message for unknown channel from " << message->getFrom().toBare().toString());
			return;
		}
		channel = m_slackChannel;
		from = Buddy::JIDToLegacyName(message->getFrom());

		Buddy *b;
		if (m_user && (b = m_user->getRosterManager()->getBuddy(from)) != NULL) {
			from = b->getAlias() + " (" + from + ")";
		}
	}

	LOG4CXX_INFO(logger, "FROM " << from);
	m_rtm->getAPI()->sendMessage(from, channel, message->getBody());
}

void SlackSession::setPurpose(const std::string &purpose, const std::string &channel) {
	std::string ch = channel;
	if (ch.empty()) {
		ch = m_slackChannel;
	}
	if (ch.empty()) {
		return;
	}

	LOG4CXX_INFO(logger, "Setting channel purppose: " << ch << " " << purpose);
	m_rtm->getAPI()->setPurpose(ch, purpose);
}

void SlackSession::handleJoinMessage(const std::string &message, std::vector<std::string> &args, bool quiet) {
	// .spectrum2 join.room BotName #room irc.freenode.net channel
	std::string &name = args[2];
	std::string legacyRoom = SlackAPI::SlackObjectToPlainText(args[3], true);
	std::string legacyServer = SlackAPI::SlackObjectToPlainText(args[4]);
	std::string slackChannel = SlackAPI::SlackObjectToPlainText(args[5], true);

	std::string to = legacyRoom + "%" + legacyServer + "@" + m_component->getJID().toString();
	if (!CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true)) {
		m_uinfo.uin = name;
		m_storageBackend->setUser(m_uinfo);
	}
// 	else {
// 		to =  legacyRoom + "\\40" + legacyServer + "@" + m_component->getJID().toString();
// 	}

	m_jid2channel[to] = slackChannel;
	m_channel2jid[slackChannel] = to;

	LOG4CXX_INFO(logger, "Setting transport between " << to << " and " << slackChannel);

	if (!quiet) {
		std::string rooms = "";
		int type = (int) TYPE_STRING;
		m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);
		rooms += message + "\n";
		m_storageBackend->updateUserSetting(m_uinfo.id, "rooms", rooms);
	}

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID("", m_uinfo.jid, "default"));
	presence->setTo(Swift::JID(to + "/" + name));
	presence->setType(Swift::Presence::Available);
	presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
	m_component->getFrontend()->onPresenceReceived(presence);

	m_onlineBuddiesTimer->start();

	if (!quiet) {
		std::string msg;
		msg += "Spectrum 2 is now joining the room. To leave the room later to disable transporting, you can use `.spectrum2 leave.room #" + SlackAPI::SlackObjectToPlainText(args[5], true, true) + "`.";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
}

void SlackSession::handleLeaveMessage(const std::string &message, std::vector<std::string> &args, bool quiet) {
	// .spectrum2 leave.room channel
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

void SlackSession::handleRegisterMessage(const std::string &message, std::vector<std::string> &args, bool quiet) {
	// .spectrum2 register test@xmpp.tld mypassword #slack_channel
	m_uinfo.uin = SlackAPI::SlackObjectToPlainText(args[2]);
	m_uinfo.password = args[3];
	std::string slackChannel = SlackAPI::SlackObjectToPlainText(args[4], true);

	m_storageBackend->setUser(m_uinfo);
	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "slack_channel", type, slackChannel);

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID("", m_uinfo.jid, "default"));
	presence->setTo(m_component->getJID());
	presence->setType(Swift::Presence::Available);
	presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
	m_component->getFrontend()->onPresenceReceived(presence);

	if (!quiet) {
		std::string msg;
		msg += "You have successfully registered 3rd-party account. Spectrum 2 is now connecting to the 3rd-party network.";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
}

void SlackSession::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message, const std::string &ts, bool quiet) {
	if (m_ownerChannel != channel) {
		std::string to = m_channel2jid[channel];
		if (m_rtm->getUserName(user) == m_rtm->getSelfName()) {
			return;
		}

		if (!to.empty()) {
			boost::shared_ptr<Swift::Message> msg(new Swift::Message());
			msg->setType(Swift::Message::Groupchat);
			msg->setTo(to);
			msg->setFrom(Swift::JID("", m_uinfo.jid, "default"));
			msg->setBody("<" + m_rtm->getUserName(user) + "> " + message);
			m_component->getFrontend()->onMessageReceived(msg);
		}
		else {
			// When changing the purpose, we do not want to spam to room with the info,
			// so remove the purpose message.
// 			if (message.find("set the channel purpose") != std::string::npos) {
// 				m_rtm->getAPI()->deleteMessage(channel, ts);
// 			}
			// TODO: MAP `user` to JID somehow and send the message to proper JID.
			// So far send to all online contacts

			if (!m_user || !m_user->getRosterManager()) {
				return;
			}

			Swift::StatusShow s;
			std::string statusMessage;
			const RosterManager::BuddiesMap &roster = m_user->getRosterManager()->getBuddies();
			for(RosterManager::BuddiesMap::const_iterator bt = roster.begin(); bt != roster.end(); bt++) {
				Buddy *b = (*bt).second;
				if (!b) {
					continue;
				}

				if (!(b->getStatus(s, statusMessage))) {
					continue;
				}

				if (s.getType() == Swift::StatusShow::None) {
					continue;
				}

				boost::shared_ptr<Swift::Message> msg(new Swift::Message());
				msg->setTo(b->getJID());
				msg->setFrom(Swift::JID("", m_uinfo.jid, "default"));
				msg->setBody("<" + m_rtm->getUserName(user) + "> " + message);
				m_component->getFrontend()->onMessageReceived(msg);
			}
		}
		return;
	}

	std::vector<std::string> args;
	boost::split(args, message, boost::is_any_of(" "));

	if (args.size() < 2 || args[0] != ".spectrum2") {
		m_rtm->sendMessage(m_ownerChannel, "Unknown command. Use \".spectrum2 help\" for help.");
		return;
	}

	if (args[1] == "join.room" && args.size() == 6) {
		handleJoinMessage(message, args, quiet);
	}
	else if (args[1] == "leave.room" && args.size() == 3) {
		handleLeaveMessage(message, args, quiet);
	}
	else if (args[1] == "register" && args.size() == 5) {
		handleRegisterMessage(message, args, quiet);
	}
	else if (args[1] == "list.rooms" && args.size() == 2) {
		// .spectrum2 list.rooms
		std::string rooms = "";
		int type = (int) TYPE_STRING;
		m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

		std::string msg = "Spectrum 2 is configured for following channels:\\n";
		msg += "```" + rooms  + "```";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
	else if (args[1] == "help") {
		std::string msg;
		msg =  "Following commands are supported:\\n";
		msg += "```.spectrum2 help``` Shows this help message.\\n";
		msg += "```.spectrum2 join.room <3rdPartyBotName> <#3rdPartyRoom> <3rdPartyServer> <#SlackChannel>``` Starts transport between 3rd-party room and Slack channel.";
		msg += "```.spectrum2 leave.room <#SlackChannel>``` Leaves the 3rd-party room connected with the given Slack channel.";
		msg += "```.spectrum2 list.rooms``` List all the transported rooms.";
		msg += "```.spectrum2 register <3rdPartyNetworkAccount> <3rdPartyPassword> <#SlackChannel> Registers 3rd-party account for transportation.";
		m_rtm->sendMessage(m_ownerChannel, msg);
	}
	else {
		m_rtm->sendMessage(m_ownerChannel, "Unknown command. Use `.spectrum2 help` for help.");
	}
}

void SlackSession::handleDisconnected() {
	m_disconnected = true;
}

void SlackSession::setUser(User *user) {
	m_user = user;
}


void SlackSession::handleConnected() {
	if (m_disconnected) {
		m_rtm->getAPI()->imOpen(m_ownerId, boost::bind(&SlackSession::handleImOpen, this, _1, _2, _3, _4));
		m_disconnected = false;
	}
}

void SlackSession::handleImOpen(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	m_ownerChannel = m_rtm->getAPI()->getChannelId(req, ok, resp, data);
	LOG4CXX_INFO(logger, "Opened channel with team owner: " << m_ownerChannel);

	std::string rooms = "";
	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

	if (!CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true)) {
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
	}
	else {
		if (m_uinfo.uin.empty()) {
			std::string msg;
			msg =  "Hi, it seems you have enabled Spectrum 2 transport for your Team. As a Team owner, you should now configure it:\\n";
			msg += "1. At first, create new channel in which you want this Spectrum 2 transport to send the messages, or choose the existing one.\\n";
			msg += "2. Invite this Spectrum 2 bot into this channel.\\n";
			msg += "3. Configure the transportation between 3rd-party network and this channel by executing following command in this chat:\\n";
			msg += "```.spectrum2 register 3rdPartyAccount 3rdPartyPassword #SlackChannel```\\n";
			msg += "For example to join XMPP account test@xmpp.tld and  transport it into #slack_channel, the command would look like this:\\n";
			msg += "```.spectrum2 register test@xmpp.tld mypassword #slack_channel```\\n";
			msg += "To get full list of available commands, executa `.spectrum2 help`\\n";
			m_rtm->sendMessage(m_ownerChannel, msg);
		}
		else {
			m_storageBackend->getUserSetting(m_uinfo.id, "slack_channel", type, m_slackChannel);
			if (!m_slackChannel.empty()) {
				Swift::Presence::ref presence = Swift::Presence::create();
				presence->setFrom(Swift::JID("", m_uinfo.jid, "default"));
				presence->setTo(m_component->getJID());
				presence->setType(Swift::Presence::Available);
				presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
				m_component->getFrontend()->onPresenceReceived(presence);
			}
		}
	}

	// Auto-join the rooms configured by the Slack channel owner.
	if (!rooms.empty()) {
		std::vector<std::string> commands;
		boost::split(commands, rooms, boost::is_any_of("\n"));

		BOOST_FOREACH(const std::string &command, commands) {
			if (command.size() > 5) {
				LOG4CXX_INFO(logger, m_uinfo.jid << ": Sending command from storage: " << command);
				handleMessageReceived(m_ownerChannel, "owner", command, "", true);
			}
		}
	}
}

void SlackSession::handleRTMStarted() {
	std::map<std::string, SlackUserInfo> &users = m_rtm->getUsers();
	for (std::map<std::string, SlackUserInfo>::iterator it = users.begin(); it != users.end(); it++) {
		SlackUserInfo &info = it->second;
		if (info.isPrimaryOwner) {
			m_ownerId = it->first;
			break;
		}
	}

	m_rtm->getAPI()->imOpen(m_ownerId, boost::bind(&SlackSession::handleImOpen, this, _1, _2, _3, _4));
}


}
