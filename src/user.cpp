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

#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/usermanager.h"
#include "transport/conversationmanager.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/MUCPayload.h"
#include "log4cxx/logger.h"
#include <boost/foreach.hpp>

using namespace log4cxx;
using namespace boost;

#define foreach         BOOST_FOREACH

namespace Transport {

static LoggerPtr logger = Logger::getLogger("User");

User::User(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) {
	m_jid = jid;
	m_data = NULL;

	m_component = component;
	m_presenceOracle = component->m_presenceOracle;
	m_entityCapsManager = component->m_entityCapsManager;
	m_userManager = userManager;
	m_userInfo = userInfo;
	m_connected = false;
	m_readyForConnect = false;
	m_ignoreDisconnect = false;

	m_reconnectTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(10000);
	m_reconnectTimer->onTick.connect(boost::bind(&User::onConnectingTimeout, this)); 

	m_rosterManager = new RosterManager(this, m_component);
	m_conversationManager = new ConversationManager(this, m_component);
	LOG4CXX_INFO(logger, m_jid.toString() << ": Created");
	updateLastActivity();
}

User::~User(){
	LOG4CXX_INFO(logger, m_jid.toString() << ": Destroying");
	if (m_component->inServerMode()) {
		dynamic_cast<Swift::ServerStanzaChannel *>(m_component->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::Element>());
	}

	m_reconnectTimer->stop();
	delete m_rosterManager;
	delete m_conversationManager;
}

const Swift::JID &User::getJID() {
	return m_jid;
}

Swift::JID User::getJIDWithFeature(const std::string &feature) {
	Swift::JID jid;
	std::vector<Swift::Presence::ref> presences = m_presenceOracle->getAllPresence(m_jid);

	foreach(Swift::Presence::ref presence, presences) {
		if (presence->getType() == Swift::Presence::Unavailable)
			continue;

		Swift::DiscoInfo::ref discoInfo = m_entityCapsManager->getCaps(presence->getFrom());
		if (!discoInfo)
			continue;

		if (discoInfo->hasFeature(feature)) {
			LOG4CXX_INFO(logger, m_jid.toString() << ": Found JID with " << feature << " feature: " << presence->getFrom().toString());
			return presence->getFrom();
		}
	}

	LOG4CXX_INFO(logger, m_jid.toString() << ": No JID with " << feature << " feature");
	return jid;
}

void User::handlePresence(Swift::Presence::ref presence) {
	std::cout << "PRESENCE " << presence->getFrom().toString() << "\n";
	if (!m_connected) {
		// we are not connected to legacy network, so we should do it when disco#info arrive :)
		if (m_readyForConnect == false) {
			
			// Forward status message to legacy network, but only if it's sent from active resource
// 					if (m_activeResource == presence->getFrom().getResource().getUTF8String()) {
// 						forwardStatus(presenceShow, stanzaStatus);
// 					}
			boost::shared_ptr<Swift::CapsInfo> capsInfo = presence->getPayload<Swift::CapsInfo>();
			if (capsInfo && capsInfo->getHash() == "sha-1") {
				if (m_entityCapsManager->getCaps(presence->getFrom()) != Swift::DiscoInfo::ref()) {
					LOG4CXX_INFO(logger, m_jid.toString() << ": Ready to be connected to legacy network");
					m_readyForConnect = true;
					onReadyToConnect();
				}
			}
			else if (m_component->inServerMode()) {
				LOG4CXX_INFO(logger, m_jid.toString() << ": Ready to be connected to legacy network");
				m_readyForConnect = true;
				onReadyToConnect();
			}
			else {
				m_reconnectTimer->start();
			}
		}
	}
	bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	if (isMUC) {
		if (presence->getType() == Swift::Presence::Unavailable) {
			LOG4CXX_INFO(logger, m_jid.toString() << ": Going to left room " << presence->getTo().getNode());
			onRoomLeft(presence->getTo().getNode());
		}
		else {
			// force connection to legacy network to let backend to handle auto-join on connect.
			if (!m_readyForConnect) {
				LOG4CXX_INFO(logger, m_jid.toString() << ": Ready to be connected to legacy network");
				m_readyForConnect = true;
				onReadyToConnect();
			}
			LOG4CXX_INFO(logger, m_jid.toString() << ": Going to join room " << presence->getTo().getNode() << " as " << presence->getTo().getResource());
			onRoomJoined(presence->getTo().getNode(), presence->getTo().getResource(), "");
		}
		return;
	}

	Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(m_jid.toBare());
	if (highest) {
		Swift::Presence::ref response = Swift::Presence::create(highest);
		response->setTo(presence->getFrom().toBare());
		response->setFrom(m_component->getJID());
		m_component->getStanzaChannel()->sendPresence(response);
		LOG4CXX_INFO(logger, m_jid.toString() << ": Changing legacy network presence to " << response->getType());
		onPresenceChanged(highest);
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(m_jid.toBare());
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unavailable);
		m_component->getStanzaChannel()->sendPresence(response);
		onPresenceChanged(response);
	}
}

void User::handleSubscription(Swift::Presence::ref presence) {
	m_rosterManager->handleSubscription(presence);
}

void User::onConnectingTimeout() {
	if (m_connected || m_readyForConnect)
		return;
	m_reconnectTimer->stop();
	m_readyForConnect = true;
	onReadyToConnect();
}

void User::setIgnoreDisconnect(bool ignoreDisconnect) {
	m_ignoreDisconnect = ignoreDisconnect;
	LOG4CXX_INFO(logger, m_jid.toString() << ": Setting ignoreDisconnect=" << m_ignoreDisconnect);
}

void User::handleDisconnected(const std::string &error) {
	if (m_ignoreDisconnect) {
		LOG4CXX_INFO(logger, m_jid.toString() << ": Disconnecting from legacy network ignored (probably moving between backends)");
		return;
	}

	if (error.empty()) {
		LOG4CXX_INFO(logger, m_jid.toString() << ": Disconnected from legacy network");
	}
	else {
		LOG4CXX_INFO(logger, m_jid.toString() << ": Disconnected from legacy network with error " << error);
	}
	onDisconnected();

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(error);
	msg->setTo(m_jid.toBare());
	msg->setFrom(m_component->getJID());
	m_component->getStanzaChannel()->sendMessage(msg);

	// In server mode, server finishes the session and pass unavailable session to userManager if we're connected to legacy network,
	// so we can't removeUser() in server mode, because it would be removed twice.
	// Once in finishSession and once in m_userManager->removeUser.
	if (m_component->inServerMode()) {
		// Remove user later just to be sure there won't be double-free.
		// We can't be sure finishSession sends unavailable presence everytime, so check if user gets removed
		// in finishSession(...) call and if not, remove it here.
		std::string jid = m_jid.toBare().toString();		
		dynamic_cast<Swift::ServerStanzaChannel *>(m_component->getStanzaChannel())->finishSession(m_jid, boost::shared_ptr<Swift::Element>(new Swift::StreamError()));
		if (m_userManager->getUser(jid) != NULL) {
			m_userManager->removeUser(this);
		}
	}
	else {
		m_userManager->removeUser(this);
	}
}

}
