/**
 * Spectrum 2 Slack Frontend
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
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

#pragma once

#include "transport/HTTPRequestQueue.h"
#include "transport/HTTPRequest.h"
#include "rapidjson/document.h"

#include <string>
#include <algorithm>
#include <map>

#include <boost/signal.hpp>

namespace Transport {

class Component;
class StorageBackend;
class HTTPRequest;

class SlackChannelInfo {
	public:
		SlackChannelInfo() {}
		virtual ~SlackChannelInfo() {}

		std::string id;
		std::string name;
		std::vector<std::string> members;
};

class SlackImInfo {
	public:
		SlackImInfo() {}
		virtual ~SlackImInfo() {}

		std::string id;
		std::string user;
};

class SlackUserInfo {
	public:
		SlackUserInfo() : isPrimaryOwner(false) {}
		virtual ~SlackUserInfo() {}

		std::string id;
		std::string name;
		bool isPrimaryOwner;
};


class SlackAPI : public HTTPRequestQueue {
	public:
		SlackAPI(Component *component, const std::string &token);

		virtual ~SlackAPI();

		void usersList(HTTPRequest::Callback callback);
		std::string getOwnerId(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data);

		void imOpen(const std::string &uid, HTTPRequest::Callback callback);
		std::string getChannelId(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data);

		void sendMessage(const std::string &from, const std::string &to, const std::string &text);

		static void getSlackChannelInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackChannelInfo> &channels);
		static void getSlackImInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackImInfo> &ims);
		static void getSlackUserInfo(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data, std::map<std::string, SlackUserInfo> &users);

	private:
		void handleSendMessage(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data);

	private:
		Component *m_component;
		std::string m_token;
};

}
