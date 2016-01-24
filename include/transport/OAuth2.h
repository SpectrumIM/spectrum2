/**
 * libtransport -- C++ library for easy XMPP Transports development
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

#include <string>

namespace Transport {

class OAuth2 {
	public:

		OAuth2(const std::string &clientId, const std::string &clientSecret,
			   const std::string &authURL, const std::string &tokenURL,
			   const std::string &redirectURL = "", const std::string &scope = "");

		virtual ~OAuth2();

		std::string generateAuthURL();

		const std::string &getState() {
			return m_state;
		}

		std::string requestToken(const std::string &code, std::string &token, std::string &bot_token);

	private:
		std::string m_clientId;
		std::string m_clientSecret;
		std::string m_authURL;
		std::string m_tokenURL;
		std::string m_redirectURL;
		std::string m_scope;
		std::string m_state;
};

}
