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

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"
#include "transport/WebSocketClient.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "SlackRTM");

SlackRTM::SlackRTM(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_counter = 0;
	m_client = new WebSocketClient(component);
	m_client->onPayloadReceived.connect(boost::bind(&SlackRTM::handlePayloadReceived, this, _1));
	m_pingTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_pingTimer->onTick.connect(boost::bind(&SlackRTM::sendPing, this));

	int type = (int) TYPE_STRING;
	m_storageBackend->getUserSetting(m_uinfo.id, "bot_token", type, m_token);

	m_api = new SlackAPI(component, m_token);

	std::string url = "https://slack.com/api/rtm.start?";
	url += "token=" + Util::urlencode(m_token);

	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, boost::bind(&SlackRTM::handleRTMStart, this, _1, _2, _3, _4));
	req->execute();
}

SlackRTM::~SlackRTM() {
	delete m_client;
	delete m_api;
	m_pingTimer->stop();
}

#define STORE_STRING(FROM, NAME) rapidjson::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.IsString()) {  \
		LOG4CXX_ERROR(logger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(logger, payload); \
		return; \
	} \
	std::string NAME = NAME##_tmp.GetString();

void SlackRTM::handlePayloadReceived(const std::string &payload) {
	rapidjson::Document d;
	if (d.Parse<0>(payload.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON");
		LOG4CXX_ERROR(logger, payload);
		return;
	}

	STORE_STRING(d, type);

	if (type == "message") {
		STORE_STRING(d, channel);
		STORE_STRING(d, user);
		STORE_STRING(d, text);
		onMessageReceived(channel, user, text);
	}
}

void SlackRTM::sendMessage(const std::string &channel, const std::string &message) {
	m_counter++;
	std::string msg = "{\"id\": " + boost::lexical_cast<std::string>(m_counter) + ", \"type\": \"message\", \"channel\":\"" + channel + "\", \"text\":\"" + message + "\"}";
	m_client->write(msg);
}

void SlackRTM::sendPing() {
	m_counter++;
	std::string msg = "{\"id\": " + boost::lexical_cast<std::string>(m_counter) + ", \"type\": \"ping\"}";
	m_client->write(msg);
	m_pingTimer->start();
}

void SlackRTM::handleRTMStart(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		LOG4CXX_ERROR(logger, data);
		return;
	}

	rapidjson::Value &url = resp["url"];
	if (!url.IsString()) {
		LOG4CXX_ERROR(logger, "No 'url' object in the reply.");
		LOG4CXX_ERROR(logger, data);
		return;
	}

	SlackAPI::getSlackChannelInfo(req, ok, resp, data, m_channels);
	SlackAPI::getSlackImInfo(req, ok, resp, data, m_ims);
	SlackAPI::getSlackUserInfo(req, ok, resp, data, m_users);

	std::string u = url.GetString();
	LOG4CXX_INFO(logger, "Started RTM, WebSocket URL is " << u);
	LOG4CXX_INFO(logger, data);

	m_client->connectServer(u);
	m_pingTimer->start();

	onRTMStarted();
}


}
