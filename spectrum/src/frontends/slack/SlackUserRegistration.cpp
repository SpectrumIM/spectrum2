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

#include "SlackUserRegistration.h"
#include "SlackRosterManager.h"
#include "SlackFrontend.h"

#include "transport/UserManager.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Buddy.h"
#include "transport/Config.h"
#include "transport/OAuth2.h"
#include "transport/Util.h"
#include "transport/HTTPRequest.h"

#include <json/json.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp> 

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(slackUserRegistrationLogger, "SlackUserRegistration");

SlackUserRegistration::SlackUserRegistration(Component *component, UserManager *userManager,
								   StorageBackend *storageBackend)
: UserRegistration(component, userManager, storageBackend) {
	m_component = component;
	m_config = m_component->getConfig();
	m_storageBackend = storageBackend;
	m_userManager = userManager;

}

SlackUserRegistration::~SlackUserRegistration(){
	
}

std::string SlackUserRegistration::createOAuth2URL(const std::vector<std::string> &args) {
	std::string redirect_url = "https://slack.spectrum.im/oauth2/" + CONFIG_STRING(m_config, "service.jid");
	OAuth2 *oauth2 = new OAuth2(CONFIG_STRING_DEFAULTED(m_config, "service.client_id",""),
						  CONFIG_STRING_DEFAULTED(m_config, "service.client_secret",""),
						  "https://slack.com/oauth/authorize",
						  "https://slack.com/api/oauth.access",
						  redirect_url,
						  "channels:read channels:write team:read im:read im:write chat:write:bot bot");
	std::string url = oauth2->generateAuthURL();

	m_auths[oauth2->getState()] = oauth2;
	m_authsData[oauth2->getState()] = args;

	if (args.size() >= 3) {
		LOG4CXX_INFO(slackUserRegistrationLogger, "Generating OAUth2 URL with slack_channel=" << args[1] << ", 3rd_party_account=" << args[2]);
	}
	else {
		LOG4CXX_WARN(slackUserRegistrationLogger, "Generating OAUth2 URL with too few arguments");
	}

	return url;
}

std::string SlackUserRegistration::getTeamDomain(const std::string &token) {
	std::string url = "https://slack.com/api/team.info?token=" + Util::urlencode(token);

	Json::Value resp;
	HTTPRequest req(HTTPRequest::Get, url);
	if (!req.execute(resp)) {
		LOG4CXX_ERROR(slackUserRegistrationLogger, url);
		LOG4CXX_ERROR(slackUserRegistrationLogger, req.getError());
		return "";
	}

	Json::Value &team = resp["team"];
	if (!team.isObject()) {
		LOG4CXX_ERROR(slackUserRegistrationLogger, "No 'team' object in the reply.");
		LOG4CXX_ERROR(slackUserRegistrationLogger, url);
		LOG4CXX_ERROR(slackUserRegistrationLogger, req.getRawData());
		return "";
	}

	Json::Value &domain = team["domain"];
	if (!domain.isString()) {
		LOG4CXX_ERROR(slackUserRegistrationLogger, "No 'domain' string in the reply.");
		LOG4CXX_ERROR(slackUserRegistrationLogger, url);
		LOG4CXX_ERROR(slackUserRegistrationLogger, req.getRawData());
		return "";
	}

	return domain.asString();
}

std::string SlackUserRegistration::handleOAuth2Code(const std::string &code, const std::string &state) {
	OAuth2 *oauth2 = NULL;
	std::string token;
	std::string access_token;
	std::vector<std::string> data;
	std::string value;
	int type = (int) TYPE_STRING;

	if (state == "use_bot_token") {
		token = code;
		access_token = code;
	}
	else {
		if (m_auths.find(state) != m_auths.end()) {
			oauth2 = m_auths[state];
			data = m_authsData[state];
		}
		else {
			return "Received state code '" + state + "' not found in state codes list.";
		}

		std::string error = oauth2->requestToken(code, access_token, token);
		if (!error.empty())  {
			return error;
		}

		if (token.empty()) {
			LOG4CXX_INFO(slackUserRegistrationLogger, "Using 'token' as 'bot_access_token'");
			token = access_token;
		}
	}

	std::string domain = getTeamDomain(access_token);
	if (domain.empty()) {
		return "The token you have provided is invalid";
	}

	std::string slackChannel;
	std::string uin;
	std::string password;
	if (data.size() >= 3) {
		slackChannel = data[1];
		uin = data[2];
	}
	if (data.size() == 4) {
		password = data[3];
	}

	UserInfo user;
	user.uin = "";
	user.password = "";
	user.id = 0;
	m_storageBackend->getUser(domain, user);

	user.jid = domain;
	user.uin = uin;
	user.password = password;
	user.language = "en";
	user.encoding = "";
	user.vip = 0;

	registerUser(user, true);

	m_storageBackend->getUser(user.jid, user);

	if (!slackChannel.empty()) {
		if (slackChannel[0] == '#') {
			slackChannel.erase(0, 1);
		}
		m_storageBackend->getUserSetting(user.id, "slack_channel", type, slackChannel);
	}

	value = token;
	m_storageBackend->getUserSetting(user.id, "bot_token", type, value);

	value = access_token;
	m_storageBackend->getUserSetting(user.id, "access_token", type, value);

	LOG4CXX_INFO(slackUserRegistrationLogger, "Registered Slack user " << user.jid << ", slack_channel=" << slackChannel);

	if (oauth2) {
		m_auths.erase(state);
		delete oauth2;

		m_authsData.erase(state);
	}

	m_component->getFrontend()->reconnectUser(user.jid);

	return "Registered as " + domain;
}

bool SlackUserRegistration::doUserRegistration(const UserInfo &row) {
	return true;
}

bool SlackUserRegistration::doUserUnregistration(const UserInfo &row) {
	return true;
}



}
