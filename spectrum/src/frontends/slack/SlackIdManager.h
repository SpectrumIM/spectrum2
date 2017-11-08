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
#include <Swiften/Network/Connection.h>
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

namespace Transport {

class SlackIdManager {
	public:
		SlackIdManager();

		virtual ~SlackIdManager();

		std::map<std::string, SlackUserInfo> &getUsers() {
			return m_users;
		}

		std::map<std::string, SlackChannelInfo> &getChannels() {
			return m_channels;
		}

		std::map<std::string, SlackImInfo> &getIMs() {
			return m_ims;
		}

		const std::string &getName(const std::string &id);
		const std::string &getId(const std::string &name);

		bool hasMember(const std::string &channelId, const std::string &userId);

		const std::string &getSelfName() {
			return m_selfName;
		}

		const std::string &getSelfId() {
			return m_selfId;
		}

		void setSelfName(const std::string &name) {
			m_selfName = name;
		}

		void setSelfId(const std::string &id) {
			m_selfId = id;
		}

	private:
		std::map<std::string, SlackChannelInfo> m_channels;
		std::map<std::string, SlackImInfo> m_ims;
		std::map<std::string, SlackUserInfo> m_users;
		std::string m_selfName;
		std::string m_selfId;
};

}
