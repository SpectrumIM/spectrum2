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

#include "XMPPUser.h"
#include "XMPPFrontend.h"
#include "transport/Transport.h"
#include "transport/UserManager.h"
#include "transport/Logging.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include <boost/foreach.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace boost;

#define foreach         BOOST_FOREACH

namespace Transport {

DEFINE_LOGGER(logger, "XMPPUser");

XMPPUser::XMPPUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) : User(jid, userInfo, component, userManager) {
	m_jid = jid.toBare();

	m_component = component;
	m_userManager = userManager;
	m_userInfo = userInfo;
}

XMPPUser::~XMPPUser(){
	if (m_component->inServerMode()) {
#if HAVE_SWIFTEN_3
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend*>(m_component->getFrontend())->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::ToplevelElement>());
#else
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend*>(m_component->getFrontend())->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::Element>());
#endif
	}
}

void XMPPUser::disconnectUser(const std::string &error, Swift::SpectrumErrorPayload::Error e) {
	// In server mode, server finishes the session and pass unavailable session to userManager if we're connected to legacy network,
	// so we can't removeUser() in server mode, because it would be removed twice.
	// Once in finishSession and once in m_userManager->removeUser.
	if (m_component->inServerMode()) {
		// Remove user later just to be sure there won't be double-free.
		// We can't be sure finishSession sends unavailable presence everytime, so check if user gets removed
		// in finishSession(...) call and if not, remove it here.
		std::string jid = m_jid.toBare().toString();
#if HAVE_SWIFTEN_3
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend*>(m_component->getFrontend())->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::ToplevelElement>(new Swift::StreamError(Swift::StreamError::UndefinedCondition, error)));
#else
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend*>(m_component->getFrontend())->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::Element>(new Swift::StreamError(Swift::StreamError::UndefinedCondition, error)));
#endif
	}
}


}
