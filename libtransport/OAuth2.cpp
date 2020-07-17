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

#include "transport/OAuth2.h"
#include "transport/Config.h"
#include "transport/Util.h"
#include "transport/HTTPRequest.h"
#include "transport/Logging.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Transport {

DEFINE_LOGGER(oauth2Logger, "OAuth2");

OAuth2::OAuth2(const std::string &clientId, const std::string &clientSecret,
			   const std::string &authURL, const std::string &tokenURL,
			   const std::string &redirectURL, const std::string &scope)
			: m_clientId(clientId), m_clientSecret(clientSecret),
			  m_authURL(authURL), m_tokenURL(tokenURL),
			  m_redirectURL(redirectURL), m_scope(scope) {

}

OAuth2::~OAuth2() {
	
}

std::string OAuth2::generateAuthURL() {
	std::string url = m_authURL + "?";

	url += "client_id=" + Util::urlencode(m_clientId);

	if (!m_scope.empty()) {
		url += "&scope=" + Util::urlencode(m_scope);
	}

	if (!m_redirectURL.empty()) {
		url += "&redirect_uri=" + Util::urlencode(m_redirectURL);
	}

	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	m_state = boost::lexical_cast<std::string>(uuid);
	url += "&state=" + Util::urlencode(m_state);

	return url;
}

std::string OAuth2::requestToken(const std::string &code, std::string &token, std::string &bot_token) {
	std::string url = m_tokenURL + "?";
	url += "client_id=" + Util::urlencode(m_clientId);
	url += "&client_secret=" + Util::urlencode(m_clientSecret);
	url += "&code=" + Util::urlencode(code);

	if (!m_redirectURL.empty()) {
		url += "&redirect_uri=" + Util::urlencode(m_redirectURL);
	}

	Json::Value resp;
	HTTPRequest req(HTTPRequest::Get, url);
	if (!req.execute(resp)) {
		LOG4CXX_ERROR(oauth2Logger, url);
		LOG4CXX_ERROR(oauth2Logger, req.getError());
		return req.getError();
	}

	LOG4CXX_ERROR(oauth2Logger, req.getRawData());
	Json::Value& access_token = resp["access_token"];
	if (!access_token.isString()) {
		LOG4CXX_ERROR(oauth2Logger, "No 'access_token' object in the reply.");
		LOG4CXX_ERROR(oauth2Logger, url);
		LOG4CXX_ERROR(oauth2Logger, req.getRawData());
		return "No 'access_token' object in the reply.";
	}

	token = access_token.asString();
	if (token.empty()) {
		LOG4CXX_ERROR(oauth2Logger, "Empty 'access_token' object in the reply.");
		LOG4CXX_ERROR(oauth2Logger, url);
		LOG4CXX_ERROR(oauth2Logger, req.getRawData());
		return "Empty 'access_token' object in the reply.";
	}

	Json::Value& bot = resp["bot"];
	if (bot.isObject()) {
		Json::Value& bot_access_token = bot["bot_access_token"];
		if (bot_access_token.isString()) {
			bot_token = bot_access_token.asString();
		}
	}

	return "";
}

}
