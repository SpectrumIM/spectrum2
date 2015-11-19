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

#include "SlackUser.h"
#include "SlackFrontend.h"

#include "transport/Transport.h"
#include "transport/UserManager.h"
#include "transport/Logging.h"

#include <boost/foreach.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace boost;

namespace Transport {

DEFINE_LOGGER(logger, "SlackUser");

SlackUser::SlackUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) : User(jid, userInfo, component, userManager) {
	m_jid = jid.toBare();

	m_component = component;
	m_userManager = userManager;
	m_userInfo = userInfo;
}

SlackUser::~SlackUser(){
}

void SlackUser::disconnectUser(const std::string &error, Swift::SpectrumErrorPayload::Error e) {

}


}
