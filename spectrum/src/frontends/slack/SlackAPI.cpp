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
#include "SlackIdManager.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(slackAPILogger, "SlackAPI");

#define GET_ARRAY(FROM, NAME) Json::Value &NAME = FROM[#NAME]; \
	if (!NAME.isArray()) { \
		LOG4CXX_ERROR(slackAPILogger, "No '" << #NAME << "' object in the reply."); \
		return; \
	}
	
#define STORE_STRING(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.isString()) {  \
		LOG4CXX_ERROR(slackAPILogger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(slackAPILogger, data); \
		return; \
	} \
	std::string NAME = NAME##_tmp.asString();

#define STORE_BOOL(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	if (!NAME##_tmp.isBool()) {  \
		LOG4CXX_ERROR(slackAPILogger, "No '" << #NAME << "' string in the reply."); \
		LOG4CXX_ERROR(slackAPILogger, data); \
		return; \
	} \
	bool NAME = NAME##_tmp.asBool();

#define GET_OBJECT(FROM, NAME) Json::Value &NAME = FROM[#NAME]; \
	if (!NAME.isObject()) { \
		LOG4CXX_ERROR(slackAPILogger, "No '" << #NAME << "' object in the reply."); \
		return; \
	}

#define STORE_STRING_OPTIONAL(FROM, NAME) Json::Value &NAME##_tmp = FROM[#NAME]; \
	std::string NAME; \
	if (NAME##_tmp.isString()) {  \
		 NAME = NAME##_tmp.asString(); \
	}

SlackAPI::SlackAPI(Component *component, SlackIdManager *idManager, const std::string &token, const std::string &domain) : HTTPRequestQueue(component, domain) {
	m_component = component;
	m_token = token;
	m_idManager = idManager;
	m_domain = domain;
}

SlackAPI::~SlackAPI() {
}

void SlackAPI::handleSendMessage(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data) {
	LOG4CXX_INFO(slackAPILogger, data);
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
	LOG4CXX_INFO(slackAPILogger, "Deleting message " << channel << " " << ts);
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

std::string SlackAPI::getChannelId(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(slackAPILogger, req->getError());
		LOG4CXX_ERROR(slackAPILogger, data);
		return "";
	}

	Json::Value &channel = resp["channel"];
	if (!channel.isObject()) {
		LOG4CXX_ERROR(slackAPILogger, "No 'channel' object in the reply.");
		LOG4CXX_ERROR(slackAPILogger, data);
		return "";
	}

	Json::Value &id = channel["id"];
	if (!id.isString()) {
		LOG4CXX_ERROR(slackAPILogger, "No 'id' string in the reply.");
		LOG4CXX_ERROR(slackAPILogger, data);
		return "";
	}

	return id.asString();
}

void SlackAPI::channelsList( HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/channels.list?token=" + Util::urlencode(m_token);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

void SlackAPI::channelsInvite(const std::string &channel, const std::string &user, HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/channels.invite?";
	url += "&channel=" + Util::urlencode(channel);
	url += "&user=" + Util::urlencode(user);
	url += "&token=" + Util::urlencode(m_token);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
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

std::string SlackAPI::getOwnerId(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(slackAPILogger, req->getError());
		return "";
	}

	Json::Value &members = resp["members"];
	if (!members.isArray()) {
		LOG4CXX_ERROR(slackAPILogger, "No 'members' object in the reply.");
		return "";
	}

	for (unsigned i = 0; i < members.size(); i++) {
		if (!members[i].isObject()) {
			continue;
		}

		Json::Value &is_primary_owner = members[i]["is_primary_owner"];
		if (!is_primary_owner.isBool()) {
			continue;
		}

		if (is_primary_owner.asBool()) {
			Json::Value &name = members[i]["id"];
			if (!name.isString()) {
				LOG4CXX_ERROR(slackAPILogger, "No 'name' string in the reply.");
				return "";
			}
			return name.asString();
		}
	}

	return "";
}

void SlackAPI::usersList(HTTPRequest::Callback callback) {
	std::string url = "https://slack.com/api/users.list?presence=0&token=" + Util::urlencode(m_token);
	HTTPRequest *req = new HTTPRequest(THREAD_POOL(m_component), HTTPRequest::Get, url, callback);
	queueRequest(req);
}

void SlackAPI::getSlackChannelInfo(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, std::map<std::string, SlackChannelInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(slackAPILogger, req->getError());
		return;
	}

	Json::Value &channels = resp["channels"];
	if (!channels.isArray()) {
		Json::Value &channel = resp["channel"];
		if (channel.isObject()) {
			SlackChannelInfo info;

			STORE_STRING(channel, id);
			info.id = id;

			STORE_STRING(channel, name);
			info.name = name;

			Json::Value &members = channel["members"];
			for (unsigned y = 0; members.isArray() && y < members.size(); y++) {
				if (!members[y].isString()) {
					continue;
				}

				info.members.push_back(members[y].asString());
			}

			ret[info.name] = info;
		}
		return;
	}

	for (unsigned i = 0; i < channels.size(); i++) {
		if (!channels[i].isObject()) {
			continue;
		}

		SlackChannelInfo info;

		STORE_STRING(channels[i], id);
		info.id = id;

		STORE_STRING(channels[i], name);
		info.name = name;

		Json::Value &members = channels[i]["members"];
		for (unsigned y = 0; members.isArray() && y < members.size(); y++) {
			if (!members[y].isString()) {
				continue;
			}

			info.members.push_back(members[y].asString());
		}

		ret[info.name] = info;
	}

	return;
}

void SlackAPI::getSlackImInfo(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, std::map<std::string, SlackImInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(slackAPILogger, req->getError());
		return;
	}

	GET_ARRAY(resp, ims);

	for (unsigned i = 0; i < ims.size(); i++) {
		if (!ims[i].isObject()) {
			continue;
		}

		SlackImInfo info;

		STORE_STRING(ims[i], id);
		info.id = id;

		STORE_STRING(ims[i], user);
		info.user = user;

		ret[info.id] = info;
		LOG4CXX_INFO(slackAPILogger, info.id << " " << info.user);
	}

	return;
}

void SlackAPI::getSlackUserInfo(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, std::map<std::string, SlackUserInfo> &ret) {
	if (!ok) {
		LOG4CXX_ERROR(slackAPILogger, req->getError());
		return;
	}

	GET_ARRAY(resp, users);

	for (unsigned i = 0; i < users.size(); i++) {
		if (!users[i].isObject()) {
			continue;
		}

		SlackUserInfo info;

		STORE_STRING(users[i], id);
		info.id = id;

		STORE_STRING(users[i], name);
		info.name = name;

// 		STORE_BOOL(users[i], is_primary_owner);
		info.isPrimaryOwner = false;

		ret[info.id] = info;
		LOG4CXX_INFO(slackAPILogger, info.id << " " << info.name);

		GET_OBJECT(users[i], profile);
		STORE_STRING_OPTIONAL(profile, bot_id);
		if (!bot_id.empty()) {
			ret[bot_id] = info;
			LOG4CXX_INFO(slackAPILogger, bot_id << " " << info.name);
		}
	}

	GET_ARRAY(resp, bots);

	for (unsigned i = 0; i < bots.size(); i++) {
		if (!bots[i].isObject()) {
			continue;
		}

		SlackUserInfo info;

		STORE_STRING(bots[i], id);
		info.id = id;

		STORE_STRING(bots[i], name);
		info.name = name;

		info.isPrimaryOwner = 0;

		ret[info.id] = info;
		LOG4CXX_INFO(slackAPILogger, info.id << " " << info.name);
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

void SlackAPI::handleSlackChannelInvite(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, const std::string &channel, const std::string &userId, CreateChannelCallback callback) {
	callback(channel);
}

void SlackAPI::handleSlackChannelCreate(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, const std::string &channel, const std::string &userId, CreateChannelCallback callback) {
	std::map<std::string, SlackChannelInfo> &channels = m_idManager->getChannels();
	SlackAPI::getSlackChannelInfo(req, ok, resp, data, channels);

	std::string channelId = m_idManager->getId(channel);
	if (channelId == channel) {
		LOG4CXX_INFO(slackAPILogger, "Error creating channel " << channel << ".");
		return;
	}

	LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Channel " << channel << " created, going to invite " << userId << " there.");
	channelsInvite(channelId, userId, boost::bind(&SlackAPI::handleSlackChannelInvite, this, _1, _2, _3, _4, channelId, userId, callback));
}

void SlackAPI::handleSlackChannelList(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data, const std::string &channel, const std::string &userId, CreateChannelCallback callback) {
	std::map<std::string, SlackChannelInfo> &channels = m_idManager->getChannels();
	SlackAPI::getSlackChannelInfo(req, ok, resp, data, channels);

	std::string channelId = m_idManager->getId(channel);
	if (channelId != channel) {
		LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Channel " << channel << " already exists, will just invite " << userId << " there.");
		channelsInvite(channelId, userId, boost::bind(&SlackAPI::handleSlackChannelInvite, this, _1, _2, _3, _4, channelId, userId, callback));
	}
	else {
		LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Going to create channel " << channel << ".");
		channelsCreate(channel, boost::bind(&SlackAPI::handleSlackChannelCreate, this, _1, _2, _3, _4, channel, userId, callback));
	}
}

void SlackAPI::createChannel(const std::string &channel, const std::string &userId, CreateChannelCallback callback) {
	std::string channelId = m_idManager->getId(channel);
	if (channelId != channel) {
		if (m_idManager->hasMember(channelId, userId)) {
			LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Channel " << channel << " already exists and " << userId << " is already there.");
			callback(channelId);
		}
		else {
			LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Channel " << channel << " already exists, will just invite " << userId << " there.");
			channelsInvite(channelId, userId, boost::bind(&SlackAPI::handleSlackChannelInvite, this, _1, _2, _3, _4, channelId, userId, callback));
		}
	}
	else {
		LOG4CXX_INFO(slackAPILogger, m_domain << ": createChannel: Channel " << channel << " not found in the cache, will refresh the channels list.");
		channelsList(boost::bind(&SlackAPI::handleSlackChannelList, this, _1, _2, _3, _4, channel, userId, callback));
	}
}


}
