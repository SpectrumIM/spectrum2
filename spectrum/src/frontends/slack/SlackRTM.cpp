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

#include "SlackRTM.h"
#include "SlackFrontend.h"
#include "SlackUser.h"
#include "SlackIdManager.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"
#include "transport/WebSocketClient.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(slackRTMLogger, "SlackRTM");

SlackRTM::SlackRTM(Component *component, StorageBackend *storageBackend, SlackIdManager *idManager, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_counter = 0;
	m_started = false;
	m_idManager = idManager;
	m_client = new WebSocketClient(component, m_uinfo.jid);
	m_client->onPayloadReceived.connect(boost::bind(&SlackRTM::handlePayloadReceived, this, _1));
	m_client->onWebSocketConnected.connect(boost::bind(&SlackRTM::handleWebSocketConnected, this));

	m_pingTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_pingTimer->onTick.connect(boost::bind(&SlackRTM::sendPing, this));

	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "bot_token", type, m_token);

	m_api = new SlackAPI(component, m_idManager, m_token, m_uinfo.jid);
}

SlackRTM::~SlackRTM() {
	delete m_client;
	delete m_api;
	m_pingTimer->stop();
}

void SlackRTM::start() {
	std::string url = "https://slack.com/api/rtm.start?";
	url += "token=" + Util::urlencode(m_token);

	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, boost::bind(&SlackRTM::handleRTMStart, this, _1, _2, _3, _4));
	req->execute();
}

#define STORE_STRING(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.isString()) {  \
		LOG4CXX_ERROR(slackRTMLogger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(slackRTMLogger, payload); \
		return; \
	} \
	std::string NAME = NAME##_tmp.asString();

#define STORE_STRING_OPTIONAL(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	std::string NAME; \
	if (NAME##_tmp.isString()) {  \
		 NAME = NAME##_tmp.asString(); \
	}

#define GET_OBJECT(FROM, NAME) Json::Value &NAME = FROM[#NAME]; \
	if (!NAME.isObject()) { \
		LOG4CXX_ERROR(slackRTMLogger, "No '" << #NAME << "' object in the reply."); \
		return; \
	}

#define STORE_INT(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.isInt()) {  \
		LOG4CXX_ERROR(slackRTMLogger, "No '" << #NAME << "' number in the reply."); \
		LOG4CXX_ERROR(slackRTMLogger, payload); \
		return; \
	} \
	int NAME = NAME##_tmp.asInt();

void SlackRTM::handlePayloadReceived(const std::string &payload) {
	Json::Value d;
	Json::CharReaderBuilder rbuilder;
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
	if (!reader->parse(payload.c_str(), payload.c_str() + payload.size(), &d, NULL)) {
		LOG4CXX_ERROR(slackRTMLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(slackRTMLogger, payload);
		return;
	}

	STORE_STRING(d, type);

	if (type == "message") {
		STORE_STRING(d, channel);
		STORE_STRING(d, text);
		STORE_STRING(d, ts);
		STORE_STRING_OPTIONAL(d, subtype);
		STORE_STRING_OPTIONAL(d, purpose);

		Json::Value &attachments = d["attachments"];
		if (attachments.isArray()) {
			for (unsigned i = 0; i < attachments.size(); i++) {
				STORE_STRING_OPTIONAL(attachments[i], fallback);
				if (!fallback.empty()) {
					text += fallback;
				}
			}
		}

		if (subtype == "bot_message") {
			STORE_STRING(d, bot_id);
			onMessageReceived(channel, bot_id, text, ts);
		}
		else if (subtype == "me_message") {
			text = "/me " + text;
			STORE_STRING(d, user);
			onMessageReceived(channel, user, text, ts);
		}
		else if (subtype == "channel_join") {
			
		}
		else if (!purpose.empty()) {
			
		}
		else {
			STORE_STRING(d, user);
			onMessageReceived(channel, user, text, ts);
		}
	}
	else if (type == "channel_joined"
		  || type == "channel_created") {
		std::map<std::string, SlackChannelInfo> &channels = m_idManager->getChannels();
		SlackAPI::getSlackChannelInfo(NULL, true, d, payload, channels);
	}
	else if (type == "error") {
		GET_OBJECT(d, error);
		STORE_INT(error, code);

		if (code == 1) {
			LOG4CXX_INFO(slackRTMLogger, "Reconnecting to Slack network");
			m_pingTimer->stop();
			m_client->disconnectServer();
			start();
		}
	}
}

void SlackRTM::sendMessage(const std::string &channel, const std::string &message) {
	m_counter++;

	std::string m = message;
	boost::replace_all(m, "\"", "\\\"");
	std::string msg = "{\"id\": " + boost::lexical_cast<std::string>(m_counter) + ", \"type\": \"message\", \"channel\":\"" + channel + "\", \"text\":\"" + m + "\"}";
	m_client->write(msg);
}

void SlackRTM::sendPing() {
	m_counter++;
	std::string msg = "{\"id\": " + boost::lexical_cast<std::string>(m_counter) + ", \"type\": \"ping\"}";
	m_client->write(msg);
	m_pingTimer->start();
}

void SlackRTM::handleRTMStart(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(slackRTMLogger, req->getError());
		LOG4CXX_ERROR(slackRTMLogger, data);
		return;
	}
	STORE_STRING_OPTIONAL(resp, error);
	if (!error.empty()) {
		if (error == "account_inactive") {
			LOG4CXX_INFO(slackRTMLogger, "Account inactive, will not try connecting again");
			m_pingTimer->stop();
			m_client->disconnectServer();
			return;
		}
	}
	Json::Value &url = resp["url"];
	if (!url.isString()) {
		LOG4CXX_ERROR(slackRTMLogger, "No 'url' object in the reply.");
		LOG4CXX_ERROR(slackRTMLogger, data);
		return;
	}

	Json::Value &self = resp["self"];
	if (!self.isObject()) {
		LOG4CXX_ERROR(slackRTMLogger, "No 'self' object in the reply.");
		LOG4CXX_ERROR(slackRTMLogger, data);
		return;
	}

	Json::Value &selfName = self["name"];
	if (!selfName.isString()) {
		LOG4CXX_ERROR(slackRTMLogger, "No 'name' string in the reply.");
		LOG4CXX_ERROR(slackRTMLogger, data);
		return;
	}

	m_idManager->setSelfName(selfName.asString());

	Json::Value &selfId = self["id"];
	if (!selfId.isString()) {
		LOG4CXX_ERROR(slackRTMLogger, "No 'id' string in the reply.");
		LOG4CXX_ERROR(slackRTMLogger, data);
		return;
	}

	m_idManager->setSelfId(selfId.asString());

	SlackAPI::getSlackChannelInfo(req, ok, resp, data, m_idManager->getChannels());
	SlackAPI::getSlackImInfo(req, ok, resp, data, m_idManager->getIMs());
	SlackAPI::getSlackUserInfo(req, ok, resp, data, m_idManager->getUsers());

	std::string u = url.asString();
	LOG4CXX_INFO(slackRTMLogger, "Started RTM, WebSocket URL is " << u);
	LOG4CXX_INFO(slackRTMLogger, data);

#ifndef LIBTRANSPORT_TEST
	m_client->connectServer(u);
#endif
}

void SlackRTM::handleWebSocketConnected() {
	if (!m_started) {
		onRTMStarted();
		m_started = true;
	}

	m_pingTimer->start();
}

void SlackRTM::handleWebSocketDisconnected(const boost::optional<Swift::Connection::Error> &error) {
	m_pingTimer->stop();
}


}
