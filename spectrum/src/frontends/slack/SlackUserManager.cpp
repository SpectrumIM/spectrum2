/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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

#include "SlackUserManager.h"
#include "SlackUserRegistration.h"
#include "SlackFrontend.h"
#include "SlackSession.h"
#include "SlackUser.h"

#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/AdminInterface.h"
#include "transport/AdminInterfaceCommand.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

namespace Transport {

DEFINE_LOGGER(slackUserManagerLogger, "SlackUserManager");

class ListRoomsCommand : public AdminInterfaceCommand {
	public:
		
		ListRoomsCommand(StorageBackend *storageBackend) : AdminInterfaceCommand("list_rooms",
							AdminInterfaceCommand::Frontend,
							AdminInterfaceCommand::UserContext,
							AdminInterfaceCommand::UserMode,
							AdminInterfaceCommand::Execute,
							"List joined 3rd-party network rooms") {
			m_storageBackend = storageBackend;
			setDescription("List connected rooms");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (uinfo.id == -1) {
				return "Error: Unknown user";
			}

			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(uinfo.id, "rooms", type, rooms);
			return rooms;
		}

	private:
		StorageBackend *m_storageBackend;
};

class JoinRoomCommand : public AdminInterfaceCommand {
	public:
		
		JoinRoomCommand(StorageBackend *storageBackend, Config *cfg) : AdminInterfaceCommand("join_room",
							AdminInterfaceCommand::Frontend,
							AdminInterfaceCommand::UserContext,
							AdminInterfaceCommand::UserMode,
							AdminInterfaceCommand::Execute,
							"Join 3rd-party network room") {
			m_storageBackend = storageBackend;
			setDescription("Join the room");

			std::string legacyRoomLabel = CONFIG_STRING_DEFAULTED(cfg, "service.join_room_room_label", "3rd-party room name");
			if (legacyRoomLabel[0] == '%') {
				legacyRoomLabel[0] = '#';
			}

			std::string legacyRoomExample = CONFIG_STRING_DEFAULTED(cfg, "service.join_room_room_example", "3rd-party room name");
			if (legacyRoomExample[0] == '%') {
				legacyRoomExample[0] = '#';
			}

			addArg("nickname",
				   CONFIG_STRING_DEFAULTED(cfg, "service.join_room_nickname_label", "Nickname in 3rd-party room"),
				   "string",
				   CONFIG_STRING_DEFAULTED(cfg, "service.join_room_nickname_example", "BotNickname"));
			addArg("legacy_room", legacyRoomLabel, "string", legacyRoomExample);
			addArg("legacy_server",
				   CONFIG_STRING_DEFAULTED(cfg, "service.join_room_server_label", "3rd-party server"),
				   "string",
				   CONFIG_STRING_DEFAULTED(cfg, "service.join_room_server_example", "3rd.party.server.org"));
			addArg("slack_channel", "Slack Chanel", "string", "mychannel");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *u, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, u, args);
			if (!ret.empty()) {
				return ret;
			}

			if (uinfo.id == -1) {
				return "Error: Unknown user";
			}

			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(uinfo.id, "rooms", type, rooms);
			// 'unknown' is here to stay compatible in args.size() with older version.
			rooms += "connected room " + args[0] + " " + args[1] + " " + args[2] + " " + args[3] + "\n";
			m_storageBackend->updateUserSetting(uinfo.id, "rooms", rooms);

			SlackUser *user = static_cast<SlackUser *>(u);
			if (user) {
				user->getSession()->handleJoinMessage("", args, true);
			}
			return "Joined the room";
		}

	private:
		StorageBackend *m_storageBackend;
};

class LeaveRoomCommand : public AdminInterfaceCommand {
	public:
		
		LeaveRoomCommand(StorageBackend *storageBackend) : AdminInterfaceCommand("leave_room",
							AdminInterfaceCommand::Frontend,
							AdminInterfaceCommand::UserContext,
							AdminInterfaceCommand::UserMode,
							AdminInterfaceCommand::Execute,
							"Leave 3rd-party network room") {
			m_storageBackend = storageBackend;
			setDescription("Leave the room");

			addArg("slack_channel", "Slack Chanel", "string", "mychannel");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *u, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, u, args);
			if (!ret.empty()) {
				return ret;
			}

			if (uinfo.id == -1) {
				return "Error: Unknown user";
			}

			std::string rooms = "";
			int type = (int) TYPE_STRING;
			m_storageBackend->getUserSetting(uinfo.id, "rooms", type, rooms);

			std::vector<std::string> commands;
			boost::split(commands, rooms, boost::is_any_of("\n"));
			rooms = "";

			BOOST_FOREACH(const std::string &command, commands) {
				if (command.size() > 5) {
					std::vector<std::string> args2;
					boost::split(args2, command, boost::is_any_of(" "));
					if (args2.size() == 6) {
						if (args[0] != args2[5]) {
							rooms += command + "\n";
						}
					}
				}
			}

			m_storageBackend->updateUserSetting(uinfo.id, "rooms", rooms);

			SlackUser *user = static_cast<SlackUser *>(u);
			if (user) {
				user->getSession()->leaveRoom(args[0]);
			}
			return "Left the room";
		}

	private:
		StorageBackend *m_storageBackend;
};

SlackUserManager::SlackUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) : UserManager(component, userRegistry, storageBackend) {
	m_component = component;
	m_storageBackend = storageBackend;
    m_userRegistration = new SlackUserRegistration(component, this, storageBackend);

	onUserCreated.connect(boost::bind(&SlackUserManager::handleUserCreated, this, _1));
	m_component->onAdminInterfaceSet.connect(boost::bind(&SlackUserManager::handleAdminInterfaceSet, this));
}

SlackUserManager::~SlackUserManager() {
    delete m_userRegistration;
}

void SlackUserManager::handleAdminInterfaceSet() {
	AdminInterface *adminInterface = m_component->getAdminInterface();
	if (adminInterface) {
		adminInterface->addCommand(new ListRoomsCommand(m_storageBackend));
		adminInterface->addCommand(new JoinRoomCommand(m_storageBackend, m_component->getConfig()));
		adminInterface->addCommand(new LeaveRoomCommand(m_storageBackend));
	}
}

void SlackUserManager::reconnectUser(const std::string &user) {
	UserInfo uinfo;
	if (!m_storageBackend->getUser(user, uinfo)) {
		LOG4CXX_ERROR(slackUserManagerLogger, "User " << user << " tried to reconnect, but he's not registered.");
		return;
	}

	LOG4CXX_INFO(slackUserManagerLogger, "Connecting user " << user << " to Slack network.");
	m_tempSessions[user] = new SlackSession(m_component, m_storageBackend, uinfo);
}

void SlackUserManager::sendVCard(unsigned int id, Swift::VCard::ref vcard) {

}

void SlackUserManager::sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message) {
	User *user = getUser(message->getTo().toBare().toString());
	if (!user) {
		LOG4CXX_ERROR(slackUserManagerLogger, "Received message for unknown user " << message->getTo().toBare().toString());
		return;
	}

	static_cast<SlackUser *>(user)->getSession()->sendMessage(message);
}

SlackSession *SlackUserManager::moveTempSession(const std::string &user) {
	if (m_tempSessions.find(user) != m_tempSessions.end()) {
		SlackSession *session = m_tempSessions[user];
		m_tempSessions.erase(user);
		return session;
	}
	return NULL;
}

void SlackUserManager::moveTempSession(const std::string &user, SlackSession *session) {
	m_tempSessions[user] = session;
	session->setUser(NULL);
}


UserRegistration *SlackUserManager::getUserRegistration() {
	return m_userRegistration;
}

std::string SlackUserManager::handleOAuth2Code(const std::string &code, const std::string &state) {
	return static_cast<SlackUserRegistration *>(m_userRegistration)->handleOAuth2Code(code, state);
}

std::string SlackUserManager::getOAuth2URL(const std::vector<std::string> &args) {
	return static_cast<SlackUserRegistration *>(m_userRegistration)->createOAuth2URL(args);
}

void SlackUserManager::handleUserCreated(User *user) {
	LOG4CXX_INFO(slackUserManagerLogger, "handleUserCreated");
	static_cast<SlackUser *>(user)->getSession()->handleConnected();
}


}
