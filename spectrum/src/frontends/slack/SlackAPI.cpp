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

#include "SlackAPI.h"
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

DEFINE_LOGGER(logger, "SlackAPI");

SlackAPI::SlackAPI(Component *component, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
}

SlackAPI::~SlackAPI() {
}

void SlackAPI::handleSendMessage(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	LOG4CXX_INFO(logger, data);
}

void SlackAPI::sendMessage(const std::string &from, const std::string &to, const std::string &text) {
	std::string url = "https://slack.com/api/chat.postMessage?";
	url += "&username=" + Util::urlencode(from);
	url += "&channel=" + Util::urlencode(to);
	url += "&text=" + Util::urlencode(text);
	url += "&token=" + Util::urlencode(m_uinfo.encoding);

	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url,
			boost::bind(&SlackAPI::handleSendMessage, this, _1, _2, _3, _4));
	queueRequest(req);
}

std::string SlackAPI::getChannelId(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		LOG4CXX_ERROR(logger, data);
		return "";
	}

	rapidjson::Value &channel = resp["channel"];
	if (!channel.IsObject()) {
		LOG4CXX_ERROR(logger, "No 'channel' object in the reply.");
		LOG4CXX_ERROR(logger, data);
		return "";
	}

	rapidjson::Value &id = channel["id"];
	if (!id.IsString()) {
		LOG4CXX_ERROR(logger, "No 'id' string in the reply.");
		LOG4CXX_ERROR(logger, data);
		return "";
	}

	return id.GetString();
}

void SlackAPI::imOpen(const std::string &uid, HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/im.open?user=" + Util::urlencode(uid) + "&token=" + Util::urlencode(m_uinfo.encoding);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

std::string SlackAPI::getOwnerId(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		return "";
	}

	rapidjson::Value &members = resp["members"];
	if (!members.IsArray()) {
		LOG4CXX_ERROR(logger, "No 'members' object in the reply.");
		return "";
	}

	for (int i = 0; i < members.Size(); i++) {
		if (!members[i].IsObject()) {
			continue;
		}

		rapidjson::Value &is_primary_owner = members[i]["is_primary_owner"];
		if (!is_primary_owner.IsBool()) {
			continue;
		}

		if (is_primary_owner.GetBool()) {
			rapidjson::Value &name = members[i]["id"];
			if (!name.IsString()) {
				LOG4CXX_ERROR(logger, "No 'name' string in the reply.");
				return "";
			}
			return name.GetString();
		}
	}

	return "";
}

void SlackAPI::usersList(HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/users.list?presence=0&token=" + Util::urlencode(m_uinfo.encoding);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

}
