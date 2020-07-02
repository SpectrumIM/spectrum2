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

#include "SlackIdManager.h"
#include "SlackFrontend.h"
#include "SlackUser.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"
#include "transport/WebSocketClient.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <map>
#include <iterator>

namespace Transport {

SlackIdManager::SlackIdManager() {

}

SlackIdManager::~SlackIdManager() {

}

const std::string &SlackIdManager::getName(const std::string &id) {
	if (m_users.find(id) == m_users.end()) {
		return id;
	}

	return m_users[id].name;
}

const std::string &SlackIdManager::getId(const std::string &name) {
	if (m_channels.find(name) == m_channels.end()) {
		return name;
	}

	return m_channels[name].id;
}

bool SlackIdManager::hasMember(const std::string &channelId, const std::string &userId) {

	for (std::map<std::string, SlackChannelInfo>::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it) {
		if (it->second.id == channelId) {
			return std::find(it->second.members.begin(), it->second.members.end(), userId) != it->second.members.end();
		}
	}

	return false;
}


}
