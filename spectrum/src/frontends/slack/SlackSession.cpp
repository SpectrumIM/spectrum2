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
#include "SlackIdManager.h"

#include "SlackUserManager.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"
#include "transport/Buddy.h"
#include "transport/Config.h"
#include "transport/ConversationManager.h"
#include "transport/Conversation.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/UserManager.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(slackSessionLogger, "SlackSession");

SlackSession::SlackSession(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo), m_user(NULL), m_disconnected(false) {
	m_component = component;
	m_storageBackend = storageBackend;

	m_idManager = new SlackIdManager();

	m_rtm = new SlackRTM(component, storageBackend, m_idManager, uinfo);
	m_rtm->onRTMStarted.connect(boost::bind(&SlackSession::handleRTMStarted, this));
	m_rtm->onMessageReceived.connect(boost::bind(&SlackSession::handleMessageReceived, this, _1, _2, _3, _4, false));
	m_rtm->start();

	m_onlineBuddiesTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_onlineBuddiesTimer->onTick.connect(boost::bind(&SlackSession::sendOnlineBuddies, this));

	int type = (int) TYPE_STRING;
	std::string token;
	m_storageBackend->getUserSetting(m_uinfo.id, "access_token", type, token);
	m_api = new SlackAPI(m_component, m_idManager, token, m_uinfo.jid);

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": SlackSession created.");
}

SlackSession::~SlackSession() {
	delete m_rtm;
	delete m_api;
	delete m_idManager;
	m_onlineBuddiesTimer->stop();

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": SlackSession destroyed.");
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
			setPurpose(onlineBuddies, m_room2channel[it->first]);
		}
	}
	m_onlineBuddiesTimer->start();
}

void SlackSession::sendMessageToAll(const std::string &msg) {
	std::vector<std::string> channels;
	for (std::map<std::string, std::string>::const_iterator it = m_room2channel.begin(); it != m_room2channel.end(); it++) {
		if (std::find(channels.begin(), channels.end(), it->second) == channels.end()) {
			channels.push_back(it->second);
			m_rtm->getAPI()->sendMessage("Spectrum 2", it->second, msg);
		}
	}
}

void SlackSession::sendMessage(Conversation *conv, const pbnetwork::ConversationMessage &message) {
	std::string from;
	std::string channel;
	if (conv->isMUC()) {
		// Our own message coming back from the room.
		if (message.nickname() == conv->getNickname()) {
			return;
		}
		from = message.nickname();
		channel = m_room2channel[conv->getLegacyName()];
	}

	if (channel.empty()) {
		if (m_slackChannel.empty()) {
			LOG4CXX_ERROR(slackSessionLogger, m_uinfo.jid << ": Received message for unknown channel from " << conv->getLegacyName());
			return;
		}
		channel = m_slackChannel;
		from = conv->getLegacyName();

		Buddy *b;
		if (m_user && (b = m_user->getRosterManager()->getBuddy(from)) != NULL) {
			from = b->getAlias() + " (" + from + ")";
		}
	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << "Sending message to Slack channel " << channel << " from " << from);
	m_rtm->getAPI()->sendMessage(from, channel, message.message());
}

User *SlackSession::loginUser() {
	if (m_user) {
		return m_user;
	}

	UserManager *userManager = static_cast<SlackFrontend *>(m_component->getFrontend())->getUserManager();
	User *user = userManager->getUser(m_uinfo.jid);
	if (!user) {
		UserInfo res = m_uinfo;
		m_storageBackend->getUser(m_uinfo.jid, res);
		UserManager::LoginError error;
		user = userManager->loginUser(m_uinfo.jid, res, error);
		if (!user) {
			LOG4CXX_WARN(slackSessionLogger, m_uinfo.jid << ": Login refused");
			return NULL;
		}
	}

	// The Slack user is always online.
	user->setReadyToConnect();
	pbnetwork::Status status;
	status.set_username(m_uinfo.jid);
	status.set_status(pbnetwork::STATUS_ONLINE);
	user->onStatusChanged(status);
	return user;
}

void SlackSession::setPurpose(const std::string &purpose, const std::string &channel) {
	std::string ch = channel;
	if (ch.empty()) {
		ch = m_slackChannel;
	}
	if (ch.empty()) {
		return;
	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Setting channel purppose: " << ch << " " << purpose);
	m_api->setPurpose(ch, purpose);
}

void SlackSession::handleJoinRoomCreated(const std::string &channelId, std::vector<std::string> args) {
	args[5] = channelId;
	std::string &name = args[2];
	std::string &legacyRoom = args[3];
	std::string &legacyServer = args[4];
	std::string &slackChannel = args[5];

	std::string room = legacyRoom + "@" + legacyServer;
// 	if (!CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true)) {
// 		m_uinfo.uin = name;
// 		m_storageBackend->setUser(m_uinfo);
// 	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Channel " << args[5] << " is created. Joining the room on legacy network.");

	m_room2channel[room] = slackChannel;
	m_channel2room[slackChannel] = room;

	User *user = loginUser();
	if (user) {
		user->joinRoom(room, name, "");
	}

	m_onlineBuddiesTimer->start();
}

void SlackSession::handleJoinMessage(const std::string &message, std::vector<std::string> &args, bool quiet) {
	if (args[5][0] == '#') {
		args[5].erase(0, 1);
	}
	boost::algorithm::to_lower(args[3]);
	boost::algorithm::to_lower(args[4]);
	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Going to join the room " << args[3] << "@" << args[4] << ", transporting it to channel " << args[5]);
	m_api->createChannel(args[5], m_idManager->getSelfId(), boost::bind(&SlackSession::handleJoinRoomCreated, this, _1, args));
}

void SlackSession::handleSlackChannelCreated(const std::string &channelId) {
	m_slackChannel = channelId;

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Main Slack Channel created, connecting the legacy network");
	loginUser();
}

void SlackSession::leaveRoom(const std::string &channel_) {
	std::string channel = channel_;
	if (channel[0] == '#') {
		channel.erase(0, 1);
	}
	std::string channelId = m_idManager->getId(channel);
	std::string room = m_channel2room[channelId];
	if (room.empty()) {
		LOG4CXX_ERROR(slackSessionLogger, "Spectrum 2 is not configured to transport this Slack channel.");
		return;
	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Leaving the legacy network room " << room);

	if (m_user) {
		m_user->leaveRoom(room);
	}
}

void SlackSession::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message, const std::string &ts, bool quiet) {
	std::string room = m_channel2room[channel];
	if (m_idManager->getName(user) == m_idManager->getSelfName()) {
		return;
	}

	if (!m_user) {
		return;
	}

	pbnetwork::ConversationMessage msg;
	msg.set_username(m_uinfo.jid);
	msg.set_message("<" + m_idManager->getName(user) + "> " + message);

	if (!room.empty()) {
		m_user->getUserManager()->messageToBackendSent();
		msg.set_buddyname(room);
		m_user->getConversationManager()->handleMessageReceived(pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE, msg);
	}
	else {
		// When changing the purpose, we do not want to spam to room with the info,
		// so remove the purpose message.
// 			if (message.find("set the channel purpose") != std::string::npos) {
// 				m_rtm->getAPI()->deleteMessage(channel, ts);
// 			}
		// TODO: MAP `user` to JID somehow and send the message to proper JID.
		// So far send to all online contacts

		if (!m_user->getRosterManager()) {
			return;
		}

		pbnetwork::StatusType s;
		std::string statusMessage;
		const RosterManager::BuddiesMap &roster = m_user->getRosterManager()->getBuddies();
		for (RosterManager::BuddiesMap::const_iterator bt = roster.begin(); bt != roster.end(); bt++) {
			Buddy *b = (*bt).second;
			if (!b) {
				continue;
			}

			if (!(b->getStatus(s, statusMessage))) {
				continue;
			}

			if (s == pbnetwork::STATUS_NONE) {
				continue;
			}

			m_user->getUserManager()->messageToBackendSent();
			msg.set_buddyname(b->getName());
			m_user->getConversationManager()->handleMessageReceived(pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE, msg);
		}
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
		handleRTMStarted();
		m_disconnected = false;
	}
}

void SlackSession::handleRTMStarted() {
	std::string rooms = "";
	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "rooms", type, rooms);

	m_storageBackend->getUserSetting(m_uinfo.id, "slack_channel", type, m_slackChannel);
	if (!m_slackChannel.empty()) {
		m_api->createChannel(m_slackChannel, m_idManager->getSelfId(), boost::bind(&SlackSession::handleSlackChannelCreated, this, _1));
	}
	else {
		LOG4CXX_WARN(slackSessionLogger, m_uinfo.jid << ": There is no Main Slack Channel set for this user.");
	}

	// Auto-join the rooms configured by the Slack channel owner.
	if (!rooms.empty()) {
		std::vector<std::string> commands;
		boost::split(commands, rooms, boost::is_any_of("\n"));

		BOOST_FOREACH(const std::string &command, commands) {
			if (command.size() > 5) {
				std::vector<std::string> args;
				boost::split(args, command, boost::is_any_of(" "));
				handleJoinMessage("", args, false);
			}
		}
	}
}


}
