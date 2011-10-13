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

#include "transport/networkplugin.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/usermanager.h"
#include "transport/conversationmanager.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "pbnetwork.pb.h"
#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "memoryusage.h"

using namespace log4cxx;

static LoggerPtr logger = Logger::getLogger("NetworkPlugin");

namespace Transport {

#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);

NetworkPlugin::NetworkPlugin(Swift::EventLoop *loop, const std::string &host, int port) {
// 	m_factories = new Swift::BoostNetworkFactories(loop);
	m_host = host;
	m_port = port;
	m_pingReceived = false;
	m_loop = loop;
// 	m_conn = m_factories->getConnectionFactory()->createConnection();
// 	m_conn->onDataRead.connect(boost::bind(&NetworkPlugin::handleDataRead, this, _1));
// 	m_conn->onDataWritten.connect(boost::bind(&NetworkPlugin::readyForData, this));
// 	m_conn->onConnectFinished.connect(boost::bind(&NetworkPlugin::_handleConnected, this, _1));
// 	m_conn->onDisconnected.connect(boost::bind(&NetworkPlugin::handleDisconnected, this));

// 	m_pingTimer = m_factories->getTimerFactory()->createTimer(30000);
// 	m_pingTimer->onTick.connect(boost::bind(&NetworkPlugin::pingTimeout, this)); 
	connect();

	double shared;
#ifndef WIN32
	process_mem_usage(shared, m_init_res);
#endif
}

NetworkPlugin::~NetworkPlugin() {
	delete m_factories;
}

void NetworkPlugin::handleMessage(const std::string &user, const std::string &legacyName, const std::string &msg, const std::string &nickname, const std::string &xhtml) {
	pbnetwork::ConversationMessage m;
	m.set_username(user);
	m.set_buddyname(legacyName);
	m.set_message(msg);
	m.set_nickname(nickname);
	m.set_xhtml(xhtml);

	std::string message;
	m.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE);

	send(message);
}

void NetworkPlugin::handleAttention(const std::string &user, const std::string &buddyName, const std::string &msg) {
	pbnetwork::ConversationMessage m;
	m.set_username(user);
	m.set_buddyname(buddyName);
	m.set_message(msg);

	std::string message;
	m.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ATTENTION);

	send(message);
}

void NetworkPlugin::handleVCard(const std::string &user, unsigned int id, const std::string &legacyName, const std::string &fullName, const std::string &nickname, const std::string &photo) {
	pbnetwork::VCard vcard;
	vcard.set_username(user);
	vcard.set_buddyname(legacyName);
	vcard.set_id(id);
	vcard.set_fullname(fullName);
	vcard.set_nickname(nickname);
	vcard.set_photo(photo);

	std::string message;
	vcard.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_VCARD);
	send(message);
}

void NetworkPlugin::handleSubject(const std::string &user, const std::string &legacyName, const std::string &msg, const std::string &nickname) {
	pbnetwork::ConversationMessage m;
	m.set_username(user);
	m.set_buddyname(legacyName);
	m.set_message(msg);
	m.set_nickname(nickname);

	std::string message;
	m.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED);
// 	std::cout << "SENDING MESSAGE\n";

	send(message);
}

void NetworkPlugin::handleBuddyChanged(const std::string &user, const std::string &buddyName, const std::string &alias,
			const std::string &groups, Swift::StatusShow::Type status, const std::string &statusMessage, const std::string &iconHash, bool blocked) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user);
	buddy.set_buddyname(buddyName);
	buddy.set_alias(alias);
	buddy.set_groups(groups);
	buddy.set_status((pbnetwork::StatusType) status);
	buddy.set_statusmessage(statusMessage);
	buddy.set_iconhash(iconHash);
	buddy.set_blocked(blocked);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED);

	send(message);
}

void NetworkPlugin::handleBuddyTyping(const std::string &user, const std::string &buddyName) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user);
	buddy.set_buddyname(buddyName);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING);

	send(message);
}

void NetworkPlugin::handleBuddyTyped(const std::string &user, const std::string &buddyName) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user);
	buddy.set_buddyname(buddyName);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED);

	send(message);
}

void NetworkPlugin::handleBuddyStoppedTyping(const std::string &user, const std::string &buddyName) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user);
	buddy.set_buddyname(buddyName);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING);

	send(message);
}

void NetworkPlugin::handleAuthorization(const std::string &user, const std::string &buddyName) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user);
	buddy.set_buddyname(buddyName);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_AUTH_REQUEST);

	send(message);
}

void NetworkPlugin::handleConnected(const std::string &user) {
	std::cout << "LOGIN SENT\n";
	pbnetwork::Connected d;
	d.set_user(user);

	std::string message;
	d.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_CONNECTED);

	send(message);	
}

void NetworkPlugin::handleDisconnected(const std::string &user, int error, const std::string &msg) {
	pbnetwork::Disconnected d;
	d.set_user(user);
	d.set_error(error);
	d.set_message(msg);

	std::string message;
	d.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_DISCONNECTED);

	send(message);
}

void NetworkPlugin::handleParticipantChanged(const std::string &user, const std::string &nickname, const std::string &room, Conversation::ParticipantFlag flags, Swift::StatusShow::Type status, const std::string &statusMessage, const std::string &newname) {
	pbnetwork::Participant d;
	d.set_username(user);
	d.set_nickname(nickname);
	d.set_room(room);
	d.set_flag(flags);
	d.set_newname(newname);
	d.set_status((pbnetwork::StatusType) status);
	d.set_statusmessage(statusMessage);

	std::string message;
	d.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_PARTICIPANT_CHANGED);

	send(message);
}

void NetworkPlugin::handleRoomNicknameChanged(const std::string &user, const std::string &r, const std::string &nickname) {
	pbnetwork::Room room;
	room.set_username(user);
	room.set_nickname(nickname);
	room.set_room(r);
	room.set_password("");

	std::string message;
	room.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ROOM_NICKNAME_CHANGED);
 
	send(message);
}

void NetworkPlugin::handleFTStart(const std::string &user, const std::string &buddyName, const std::string fileName, unsigned long size) {
	pbnetwork::File room;
	room.set_username(user);
	room.set_buddyname(buddyName);
	room.set_filename(fileName);
	room.set_size(size);

	std::string message;
	room.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_START);
 
	send(message);
}

void NetworkPlugin::handleFTFinish(const std::string &user, const std::string &buddyName, const std::string fileName, unsigned long size, unsigned long ftid) {
	pbnetwork::File room;
	room.set_username(user);
	room.set_buddyname(buddyName);
	room.set_filename(fileName);
	room.set_size(size);
	if (ftid) {
		room.set_ftid(ftid);
	}

	std::string message;
	room.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_FINISH);
 
	send(message);
}

void NetworkPlugin::handleFTData(unsigned long ftID, const std::string &data) {
	pbnetwork::FileTransferData d;
	d.set_ftid(ftID);
	d.set_data(data);

	std::string message;
	d.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_DATA);
 
	send(message);
}

void NetworkPlugin::_handleConnected(bool error) {
	if (error) {
// 		LOG4CXX_ERROR(logger, "Connecting error. Exiting");
// 		m_pingTimer->stop();
		handleExit();
	}
	else {
// 		LOG4CXX_INFO(logger, "Connected to NetworkPluginServer");
// 		m_pingTimer->start();
	}
}

void NetworkPlugin::handleDisconnected() {
// 	LOG4CXX_INFO(logger, "Disconnected from NetworkPluginServer. Exiting.");
// 	m_pingTimer->stop();
	handleExit();
}

void NetworkPlugin::connect() {
	LOG4CXX_INFO(logger, "Connecting NetworkPluginServer host " << m_host << " port " << m_port);
// 	m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(m_host), m_port));
}

void NetworkPlugin::handleLoginPayload(const std::string &data) {
	pbnetwork::Login payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	handleLoginRequest(payload.user(), payload.legacyname(), payload.password());
}

void NetworkPlugin::handleLogoutPayload(const std::string &data) {
	pbnetwork::Logout payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	handleLogoutRequest(payload.user(), payload.legacyname());
}

void NetworkPlugin::handleStatusChangedPayload(const std::string &data) {
	pbnetwork::Status payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleStatusChangeRequest(payload.username(), payload.status(), payload.statusmessage());
}

void NetworkPlugin::handleConvMessagePayload(const std::string &data) {
	pbnetwork::ConversationMessage payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleMessageSendRequest(payload.username(), payload.buddyname(), payload.message(), payload.xhtml());
}

void NetworkPlugin::handleAttentionPayload(const std::string &data) {
	pbnetwork::ConversationMessage payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleAttentionRequest(payload.username(), payload.buddyname(), payload.message());
}

void NetworkPlugin::handleFTStartPayload(const std::string &data) {
	pbnetwork::File payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleFTStartRequest(payload.username(), payload.buddyname(), payload.filename(), payload.size(), payload.ftid());
}

void NetworkPlugin::handleFTFinishPayload(const std::string &data) {
	pbnetwork::File payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleFTFinishRequest(payload.username(), payload.buddyname(), payload.filename(), payload.size(), payload.ftid());
}

void NetworkPlugin::handleFTPausePayload(const std::string &data) {
	pbnetwork::FileTransferData payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleFTPauseRequest(payload.ftid());
}

void NetworkPlugin::handleFTContinuePayload(const std::string &data) {
	pbnetwork::FileTransferData payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleFTContinueRequest(payload.ftid());
}

void NetworkPlugin::handleJoinRoomPayload(const std::string &data) {
	pbnetwork::Room payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleJoinRoomRequest(payload.username(), payload.room(), payload.nickname(), payload.password());
}

void NetworkPlugin::handleLeaveRoomPayload(const std::string &data) {
	pbnetwork::Room payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleLeaveRoomRequest(payload.username(), payload.room());
}

void NetworkPlugin::handleVCardPayload(const std::string &data) {
	pbnetwork::VCard payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	if (payload.has_photo()) {
		handleVCardUpdatedRequest(payload.username(), payload.photo(), payload.nickname());
	}
	else if (!payload.buddyname().empty()) {
		handleVCardRequest(payload.username(), payload.buddyname(), payload.id());
	}
}

void NetworkPlugin::handleBuddyChangedPayload(const std::string &data) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	if (payload.has_blocked()) {
		handleBuddyBlockToggled(payload.username(), payload.buddyname(), payload.blocked());
	}
	else {
		handleBuddyUpdatedRequest(payload.username(), payload.buddyname(), payload.alias(), payload.groups());
	}
}

void NetworkPlugin::handleBuddyRemovedPayload(const std::string &data) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	handleBuddyRemovedRequest(payload.username(), payload.buddyname(), payload.groups());
}

void NetworkPlugin::handleChatStatePayload(const std::string &data, Swift::ChatState::ChatStateType type) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	switch(type) {
		case Swift::ChatState::Composing:
			handleTypingRequest(payload.username(), payload.buddyname());
			break;
		case Swift::ChatState::Paused:
			handleTypedRequest(payload.username(), payload.buddyname());
			break;
		case Swift::ChatState::Active:
			handleStoppedTypingRequest(payload.username(), payload.buddyname());
			break;
		default:
			break;
	}
}

void NetworkPlugin::handleDataRead(Swift::SafeByteArray &data) {
	m_data.insert(m_data.end(), data.begin(), data.end());

	while (m_data.size() != 0) {
		unsigned int expected_size;

		if (m_data.size() >= 4) {
			expected_size = *((unsigned int*) &m_data[0]);
			expected_size = ntohl(expected_size);
			if (m_data.size() - 4 < expected_size)
				return;
		}
		else {
			return;
		}

		pbnetwork::WrapperMessage wrapper;
		if (wrapper.ParseFromArray(&m_data[4], expected_size) == false) {
			m_data.erase(m_data.begin(), m_data.begin() + 4 + expected_size);
			return;
		}
		m_data.erase(m_data.begin(), m_data.begin() + 4 + expected_size);

		switch(wrapper.type()) {
			case pbnetwork::WrapperMessage_Type_TYPE_LOGIN:
				handleLoginPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_LOGOUT:
				handleLogoutPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_PING:
				LOG4CXX_INFO(logger, "PING RECEIVED");
				sendPong();
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE:
				handleConvMessagePayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_JOIN_ROOM:
				handleJoinRoomPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_LEAVE_ROOM:
				handleLeaveRoomPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_VCARD:
				handleVCardPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED:
				handleBuddyChangedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_REMOVED:
				handleBuddyRemovedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_STATUS_CHANGED:
				handleStatusChangedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Composing);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Paused);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Active);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_ATTENTION:
				handleAttentionPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_START:
				handleFTStartPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_FINISH:
				handleFTFinishPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_PAUSE:
				handleFTPausePayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_CONTINUE:
				handleFTContinuePayload(wrapper.payload());
				break;
			default:
				return;
		}
	}
}

void NetworkPlugin::send(const std::string &data) {
	char header[4];
	*((int*)(header)) = htonl(data.size());
	sendData(std::string(header, 4) + data);
}

void NetworkPlugin::sendPong() {
	m_pingReceived = true;
	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PONG);
	wrap.SerializeToString(&message);

	send(message);
	LOG4CXX_INFO(logger, "PONG");
	sendMemoryUsage();
}

void NetworkPlugin::sendMemoryUsage() {
	pbnetwork::Stats stats;

	stats.set_init_res(m_init_res);
	double res;
	double shared;
#ifndef WIN32
	process_mem_usage(shared, res);
#endif
	stats.set_res(res);
	stats.set_shared(shared);

	std::string message;
	stats.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_STATS);

	send(message);
}

void NetworkPlugin::pingTimeout() {
	if (m_pingReceived == false) {
// 		LOG4CXX_ERROR(logger, "No PING received for long time. Exiting");
		handleExit();
	}
	m_pingReceived = false;
// 	m_pingTimer->start();
}

}
