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

#include "SlackAPI.h"

#include "transport/StorageBackend.h"
#include "rapidjson/document.h"

#include <Swiften/Network/TLSConnectionFactory.h>
#include <Swiften/Network/HostAddressPort.h>
#include <Swiften/TLS/PlatformTLSFactories.h>
#include <Swiften/Network/DomainNameResolveError.h>
#include <Swiften/Network/DomainNameAddressQuery.h>
#include <Swiften/Network/DomainNameResolver.h>
#include <Swiften/Network/HostAddress.h>
#include <Swiften/Base/SafeByteArray.h>
#include "Swiften/Network/Timer.h"
#include "Swiften/Version.h"

#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#if HAVE_SWIFTEN_3
#include <Swiften/TLS/TLSOptions.h>
#endif

#include <string>
#include <algorithm>
#include <map>

#include <boost/signal.hpp>

namespace Transport {

class Component;
class StorageBackend;
class HTTPRequest;
class WebSocketClient;
class SlackAPI;

class SlackRTM {
	public:
		SlackRTM(Component *component, StorageBackend *storageBackend, UserInfo uinfo);

		virtual ~SlackRTM();

		void sendPing();

		void sendMessage(const std::string &channel, const std::string &message);

		boost::signal<void ()> onRTMStarted;

		std::map<std::string, SlackUserInfo> &getUsers() {
			return m_users;
		}

		std::map<std::string, SlackChannelInfo> &getChannels() {
			return m_channels;
		}

		SlackAPI *getAPI() {
			return m_api;
		}

		boost::signal<void (const std::string &channel, const std::string &user, const std::string &text)> onMessageReceived;

	private:
		void handlePayloadReceived(const std::string &payload);
		void handleRTMStart(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data);
		void handleWebSocketConnected();

	private:
		std::map<std::string, SlackChannelInfo> m_channels;
		std::map<std::string, SlackImInfo> m_ims;
		std::map<std::string, SlackUserInfo> m_users;

	private:
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserInfo m_uinfo;
		WebSocketClient *m_client;
		std::string m_token;
		unsigned long m_counter;
		Swift::Timer::ref m_pingTimer;
		SlackAPI *m_api;
};

}
