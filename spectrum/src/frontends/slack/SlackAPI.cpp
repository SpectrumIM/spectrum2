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

#define GET_ARRAY(FROM, NAME) rapidjson::Value &NAME = FROM[#NAME]; \
	if (!NAME.IsArray()) { \
		LOG4CXX_ERROR(logger, "No '" << #NAME << "' object in the reply."); \
		return; \
	}
	
#define STORE_STRING(FROM, NAME) rapidjson::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.IsString()) {  \
		LOG4CXX_ERROR(logger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(logger, data); \
		return; \
	} \
	std::string NAME = NAME##_tmp.GetString();

#define STORE_BOOL(FROM, NAME) rapidjson::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.IsBool()) {  \
		LOG4CXX_ERROR(logger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(logger, data); \
		return; \
	} \
	bool NAME = NAME##_tmp.GetBool();

#define GET_OBJECT(FROM, NAME) rapidjson::Value &NAME = FROM[#NAME]; \
	if (!NAME.IsObject()) { \
		LOG4CXX_ERROR(logger, "No '" << #NAME << "' object in the reply."); \
		return; \
	}

#define STORE_STRING_OPTIONAL(FROM, NAME) rapidjson::Value &NAME##_tmp = FROM[#NAME]; \
	std::string NAME; \
	if (NAME##_tmp.IsString()) {  \
		 NAME = NAME##_tmp.GetString(); \
	}

SlackAPI::SlackAPI(Component *component, const std::string &token) : HTTPRequestQueue(component) {
	m_component = component;
	m_token = token;
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
	url += "&token=" + Util::urlencode(m_token);

	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url,
			boost::bind(&SlackAPI::handleSendMessage, this, _1, _2, _3, _4));
	queueRequest(req);
}

void SlackAPI::deleteMessage(const std::string &channel, const std::string &ts) {
	LOG4CXX_INFO(logger, "Deleting message " << channel << " " << ts);
	std::string url = "https://slack.com/api/chat.delete?";
	url += "&channel=" + Util::urlencode(channel);
	url += "&ts=" + Util::urlencode(ts);
	url += "&token=" + Util::urlencode(m_token);

	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url,
			boost::bind(&SlackAPI::handleSendMessage, this, _1, _2, _3, _4));
	queueRequest(req);
}

void SlackAPI::setPurpose(const std::string &channel, const std::string &purpose) {
	std::string url = "https://slack.com/api/channels.setPurpose?";
	url += "&channel=" + Util::urlencode(channel);
	url += "&purpose=" + Util::urlencode(purpose);
	url += "&token=" + Util::urlencode(m_token);

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

void SlackAPI::channelsCreate(const std::string &name, HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/channels.create?name=" + Util::urlencode(name) + "&token=" + Util::urlencode(m_token);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

void SlackAPI::imOpen(const std::string &uid, HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/im.open?user=" + Util::urlencode(uid) + "&token=" + Util::urlencode(m_token);
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
	std::string url = "https://slack.com/api/users.list?presence=0&token=" + Util::urlencode(m_token);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

void SlackAPI::getSlackChannelInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackChannelInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		return;
	}

	GET_ARRAY(resp, channels);

	for (int i = 0; i < channels.Size(); i++) {
		if (!channels[i].IsObject()) {
			continue;
		}

		SlackChannelInfo info;

		STORE_STRING(channels[i], id);
		info.id = id;

		STORE_STRING(channels[i], name);
		info.name = name;

		rapidjson::Value &members = channels[i]["members"];
		for (int y = 0; members.IsArray() && y < members.Size(); y++) {
			if (!members[y].IsString()) {
				continue;
			}

			info.members.push_back(members[y].GetString());
		}

		ret[info.name] = info;
	}

	return;
}

void SlackAPI::getSlackImInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackImInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		return;
	}

	GET_ARRAY(resp, ims);

	for (int i = 0; i < ims.Size(); i++) {
		if (!ims[i].IsObject()) {
			continue;
		}

		SlackImInfo info;

		STORE_STRING(ims[i], id);
		info.id = id;

		STORE_STRING(ims[i], user);
		info.user = user;

		ret[info.id] = info;
		LOG4CXX_INFO(logger, info.id << " " << info.user);
	}

	return;
}

void SlackAPI::getSlackUserInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackUserInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		return;
	}

	GET_ARRAY(resp, users);

	for (int i = 0; i < users.Size(); i++) {
		if (!users[i].IsObject()) {
			continue;
		}

		SlackUserInfo info;

		STORE_STRING(users[i], id);
		info.id = id;

		STORE_STRING(users[i], name);
		info.name = name;

		STORE_BOOL(users[i], is_primary_owner);
		info.isPrimaryOwner = is_primary_owner;

		ret[info.id] = info;
		LOG4CXX_INFO(logger, info.id << " " << info.name);

		GET_OBJECT(users[i], profile);
		STORE_STRING_OPTIONAL(profile, bot_id);
		if (!bot_id.empty()) {
			ret[bot_id] = info;
			LOG4CXX_INFO(logger, bot_id << " " << info.name);
		}
	}

	return;
}

std::string SlackAPI::SlackObjectToPlainText(const std::string &object, bool isChannel, bool returnName) {
	std::string ret = object;
	if (isChannel) {
		if (ret[0] == '<') {
			ret = ret.substr(2, ret.size() - 3);
		}
	} else {
		if (ret[0] == '<') {
			ret = ret.substr(1, ret.size() - 2);
			if (returnName) {
				ret = ret.substr(0, ret.find("|"));
			}
			else {
				ret = ret.substr(ret.find("|") + 1);
			}
		}
	}

	return ret;
}



}
