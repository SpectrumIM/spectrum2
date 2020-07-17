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
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

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
			m_rtm->getAPI()->sendMessage("Spectrum 2", it->second, msg);
		}
	}
}

void SlackSession::sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message) {
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
	if (channel.empty()) {
		if (m_slackChannel.empty()) {
			LOG4CXX_ERROR(slackSessionLogger, m_uinfo.jid << ": Received message for unknown channel from " << message->getFrom().toBare().toString());
			return;
		}
		channel = m_slackChannel;
		from = Buddy::JIDToLegacyName(message->getFrom(), m_user);

		Buddy *b;
		if (m_user && (b = m_user->getRosterManager()->getBuddy(from)) != NULL) {
			from = b->getAlias() + " (" + from + ")";
		}
	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << "Sending message to Slack channel " << channel << " from " << from);
#if HAVE_SWIFTEN_3
	std::string body = message->getBody().get_value_or("");
#else
	std::string body = message->getBody();
#endif
	m_rtm->getAPI()->sendMessage(from, channel, body);
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

	std::string to = legacyRoom + "%" + legacyServer + "@" + m_component->getJID().toString();
// 	if (!CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true)) {
// 		m_uinfo.uin = name;
// 		m_storageBackend->setUser(m_uinfo);
// 	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Channel " << args[5] << " is created. Joining the room on legacy network.");

	m_jid2channel[to] = slackChannel;
	m_channel2jid[slackChannel] = to;

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID(m_uinfo.jid + "/default"));
	presence->setTo(Swift::JID(to + "/" + name));
	presence->setType(Swift::Presence::Available);
	presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
	m_component->getFrontend()->onPresenceReceived(presence);

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
	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID(m_uinfo.jid + "/default"));
	presence->setTo(m_component->getJID());
	presence->setType(Swift::Presence::Available);
	presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
	m_component->getFrontend()->onPresenceReceived(presence);
}

void SlackSession::leaveRoom(const std::string &channel_) {
	std::string channel = channel_;
	if (channel[0] == '#') {
		channel.erase(0, 1);
	}
	std::string channelId = m_idManager->getId(channel);
	std::string to = m_channel2jid[channelId];
	if (to.empty()) {
		LOG4CXX_ERROR(slackSessionLogger, "Spectrum 2 is not configured to transport this Slack channel.");
		return;
	}

	LOG4CXX_INFO(slackSessionLogger, m_uinfo.jid << ": Leaving the legacy network room " << to);

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID(m_uinfo.jid + "/default"));
	presence->setTo(Swift::JID(to + "/" + m_uinfo.uin));
	presence->setType(Swift::Presence::Unavailable);
	presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::MUCPayload()));
	m_component->getFrontend()->onPresenceReceived(presence);
}

void SlackSession::handleMessageReceived(const std::string &channel, const std::string &user, const std::string &message, const std::string &ts, bool quiet) {
	std::string to = m_channel2jid[channel];
	if (m_idManager->getName(user) == m_idManager->getSelfName()) {
		return;
	}

	if (!to.empty()) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setType(Swift::Message::Groupchat);
		msg->setTo(to);
		msg->setFrom(Swift::JID(m_uinfo.jid + "/default"));
		msg->setBody("<" + m_idManager->getName(user) + "> " + message);
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
		for (RosterManager::BuddiesMap::const_iterator bt = roster.begin(); bt != roster.end(); bt++) {
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

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
			msg->setTo(b->getJID());
			msg->setFrom(Swift::JID(m_uinfo.jid + "/default"));
			msg->setBody("<" + m_idManager->getName(user) + "> " + message);
			m_component->getFrontend()->onMessageReceived(msg);
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
