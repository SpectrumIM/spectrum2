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
#include <boost/bind.hpp>
#include "XMPPFrontend.h"
#include "XMPPRosterManager.h"
#include "XMPPConversation.h"
#include "PresenceOracle.h"
#include "jidutils.h"
#include "transport/Transport.h"
#include "transport/UserManager.h"
#include "transport/ConversationManager.h"
#include "transport/StorageBackend.h"
#include "transport/Logging.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/CapsInfo.h"
#include "Swiften/Elements/VCardUpdate.h"
#include "Swiften/Elements/InvisiblePayload.h"
#include "Swiften/Queries/IQRouter.h"
#include <boost/foreach.hpp>
#include <stdio.h>
#include <stdlib.h>

namespace Transport {

DEFINE_LOGGER(xmppUserLogger, "XMPPUser");

XMPPUser::XMPPUser(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager) : User(user, userInfo, component, userManager) {
	m_jid = Swift::JID(user).toBare();

	m_component = component;
	m_userManager = userManager;
	m_resources = 0;
	m_rooms = std::shared_ptr<Swift::DiscoItems>(new Swift::DiscoItems());

	m_reconnectTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(5000);
	m_reconnectTimer->onTick.connect(boost::bind(&XMPPUser::onConnectingTimeout, this));
}

XMPPUser::~XMPPUser(){
	m_reconnectTimer->stop();

	if (m_component->inServerMode()) {
		dynamic_cast<Swift::ServerStanzaChannel *>(getFrontend()->getStanzaChannel())->finishSession(m_jid, std::shared_ptr<Swift::ToplevelElement>());
	}

	if (m_vcardRequests.size() != 0) {
		LOG4CXX_INFO(xmppUserLogger, m_jid.toString() <<  ": Removing " << m_vcardRequests.size() << " unresponded IQs");
		BOOST_FOREACH(Swift::GetVCardRequest::ref request, m_vcardRequests) {
			request->onResponse.disconnect_all_slots();
			getFrontend()->getIQRouter()->removeHandler(request);
		}
		m_vcardRequests.clear();
	}
}

XMPPFrontend *XMPPUser::getFrontend() {
	return static_cast<XMPPFrontend *>(m_component->getFrontend());
}

void XMPPUser::disconnectUser(const std::string &error, pbnetwork::ConnectionError e) {
	std::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(error);
	msg->setTo(m_jid.toBare());
	msg->setFrom(getFrontend()->getJID());
	msg->addPayload(std::make_shared<Swift::SpectrumErrorPayload>((Swift::SpectrumErrorPayload::Error) e));
	getFrontend()->sendMessage(msg);

	// In server mode, server finishes the session and pass unavailable session to userManager if we're connected to legacy network,
	// so we can't removeUser() in server mode, because it would be removed twice.
	// Once in finishSession and once in m_userManager->removeUser.
	if (m_component->inServerMode()) {
		// Remove user later just to be sure there won't be double-free.
		// We can't be sure finishSession sends unavailable presence everytime, so check if user gets removed
		// in finishSession(...) call and if not, remove it here.
		dynamic_cast<Swift::ServerStanzaChannel *>(getFrontend()->getStanzaChannel())->finishSession(m_jid, std::shared_ptr<Swift::ToplevelElement>(new Swift::StreamError(Swift::StreamError::UndefinedCondition, error)));
	}
}

void XMPPUser::handleVCardReceived(std::shared_ptr<Swift::VCard> vcard, Swift::ErrorPayload::ref error, Swift::GetVCardRequest::ref request) {
	m_vcardRequests.remove(request);
	request->onResponse.disconnect_all_slots();
	if (!vcard) {
		LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Received empty VCard");
		return;
	}
	m_component->getFrontend()->onVCardUpdated(this, XMPPFrontend::toVCard(this, vcard));
}

void XMPPUser::requestVCard() {
	LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Requesting VCard");

	Swift::GetVCardRequest::ref request = Swift::GetVCardRequest::create(m_jid, getFrontend()->getIQRouter());
	request->onResponse.connect(boost::bind(&XMPPUser::handleVCardReceived, this, _1, _2, request));
	request->send();
	m_vcardRequests.push_back(request);
}

void XMPPUser::clearRoomList() {
	m_rooms = std::shared_ptr<Swift::DiscoItems>(new Swift::DiscoItems());
}

void XMPPUser::addRoomToRoomList(const std::string &room, const std::string &name) {
	std::string handle = Swift::JID::getEscapedNode(room) + "@" + m_component->getJID();
	m_rooms->addItem(Swift::DiscoItems::Item(name, handle));
}

void XMPPUser::sendCurrentPresence() {
	if (m_component->inServerMode()) {
		return;
	}

	PresenceOracle *presenceOracle = getFrontend()->getPresenceOracle();
	std::vector<Swift::Presence::ref> presences = presenceOracle->getAllPresence(m_jid);
	for (Swift::Presence::ref presence : presences) {
		if (presence->getType() == Swift::Presence::Unavailable) {
			continue;
		}

		if (isConnected()) {
			Swift::Presence::ref highest = presenceOracle->getHighestPriorityPresence(m_jid.toBare());
			if (highest) {
				Swift::Presence::ref response = Swift::Presence::create(highest);
				response->setTo(presence->getFrom());
				response->setFrom(getFrontend()->getJID());
				getFrontend()->sendPresence(response);
			}
			else {
				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(getFrontend()->getJID());
				response->setType(Swift::Presence::Unavailable);
				getFrontend()->sendPresence(response);
			}
		}
		else {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(presence->getFrom());
			response->setFrom(getFrontend()->getJID());
			response->setType(Swift::Presence::Unavailable);
			response->setStatus("Connecting");
			getFrontend()->sendPresence(response);
		}
	}
}

void XMPPUser::leaveRoom(const std::string &room) {
	m_joinedRooms.erase(room);
	User::leaveRoom(room);
}

void XMPPUser::handleRoomJoined(Conversation *conv) {
	std::map<std::string, Swift::Presence::ref>::iterator it = m_joinedRooms.find(conv->getLegacyName());
	if (it == m_joinedRooms.end()) {
		return;
	}

	Swift::Presence::ref presence = it->second;
	XMPPConversation *xconv = static_cast<XMPPConversation *>(conv);
	if (std::find(xconv->getJIDs().begin(), xconv->getJIDs().end(), presence->getFrom()) != xconv->getJIDs().end()) {
		LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": User has already tried to join room " << conv->getLegacyName() << " as " << presence->getTo().getResource());
	}
	else {
		xconv->addJID(presence->getFrom());
	}

	if (presence->getTo().toString().find("\\40") != std::string::npos) {
		xconv->setMUCEscaping(true);
	}

	getFrontend()->sendRawStanza(this, presence);
}

void XMPPUser::resetResources() {
	for (const auto &it : getConversationManager()->getConversations()) {
		if (!it.second || it.second->isMUC()) {
			continue;
		}
		static_cast<XMPPConversation *>(it.second)->setJID(m_jid.toBare());
	}
}

void XMPPUser::removeJID(const Swift::JID &jid) {
	std::vector<std::string> toRemove;
	for (const auto &it : getConversationManager()->getConversations()) {
		if (it.first.empty() || !it.second) {
			continue;
		}

		XMPPConversation *conv = static_cast<XMPPConversation *>(it.second);
		conv->removeJID(jid);
		if (conv->getJIDs().empty() && conv->isMUC()) {
			toRemove.push_back(it.first);
		}
	}

	if (getUserSetting("stay_connected") != "1") {
		while (!toRemove.empty()) {
			LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Leaving room " << toRemove.back() << ".");
			leaveRoom(toRemove.back());
			toRemove.pop_back();
		}
	}
}

void XMPPUser::clearJIDs() {
	for (const auto &it : getConversationManager()->getConversations()) {
		if (it.second) {
			static_cast<XMPPConversation *>(it.second)->clearJIDs();
		}
	}
}

void XMPPUser::handlePresence(Swift::Presence::ref presence) {
	LOG4CXX_INFO(xmppUserLogger, "PRESENCE " << presence->getFrom().toString() << " " << presence->getTo().toString());

	std::shared_ptr<Swift::VCardUpdate> vcardUpdate = presence->getPayload<Swift::VCardUpdate>();
	if (vcardUpdate) {
		std::string value = "";
		int type = (int) TYPE_STRING;
		if (getStorageBackend()) {
			getStorageBackend()->getUserSetting(getUserInfo().id, "photohash", type, value);
		}
		if (value != vcardUpdate->getPhotoHash()) {
			LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Requesting VCard");
			if (getStorageBackend()) {
				getStorageBackend()->updateUserSetting(getUserInfo().id, "photohash", vcardUpdate->getPhotoHash());
			}
			requestVCard();
		}
	}

	if (!isConnected()) {
		// we are not connected to legacy network, so we should do it when disco#info arrive :)
		if (!isReadyToConnect()) {
			std::shared_ptr<Swift::CapsInfo> capsInfo = presence->getPayload<Swift::CapsInfo>();
			if (capsInfo && capsInfo->getHash() == "sha-1") {
				if (getFrontend()->sendCapabilitiesRequest(presence->getFrom()) != Swift::DiscoInfo::ref()) {
					setReadyToConnect();
				}
				else {
					m_reconnectTimer->start();
				}
			}
			else if (m_component->inServerMode()) {
				setReadyToConnect();
			}
			else {
				m_reconnectTimer->start();
			}
		}
	}

	if (!presence->getTo().getNode().empty()) {
		bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
		if (presence->getType() == Swift::Presence::Unavailable) {
			std::string room = JIDUtils::JIDToLegacyName(presence->getTo());
			XMPPConversation *conv = static_cast<XMPPConversation *>(getConversationManager()->getConversation(room));
			if (conv) {
				conv->removeJID(presence->getFrom());
				if (!conv->getJIDs().empty()) {
					return;
				}
			}
			else {
				return;
			}

			if (getUserSetting("stay_connected") != "1") {
				LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Going to left room " << room);
				getFrontend()->sendRawStanza(this, presence);
				leaveRoom(room);
			}

			return;
		}
		else if (isMUC) {
			std::string room = JIDUtils::JIDToLegacyName(presence->getTo());
			std::string password = "";
			if (presence->getPayload<Swift::MUCPayload>() != NULL) {
				password = presence->getPayload<Swift::MUCPayload>()->getPassword() ? *presence->getPayload<Swift::MUCPayload>()->getPassword() : "";
			}

			XMPPConversation *conv = static_cast<XMPPConversation *>(getConversationManager()->getConversation(room));
			if (conv != NULL) {
				// Force the connection to the legacy network, as joinRoom does.
				setReadyToConnect();
				if (std::find(conv->getJIDs().begin(), conv->getJIDs().end(), presence->getFrom()) != conv->getJIDs().end()) {
					LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": User has already tried to join room " << room << " as " << presence->getTo().getResource());
				}
				else {
					conv->addJID(presence->getFrom());
					conv->sendParticipants(presence->getFrom(), presence->getTo().getResource());
					conv->sendCachedMessages(presence->getFrom());
				}
				return;
			}

			// handleRoomJoined adds this resource to the room once it is joined.
			if (m_joinedRooms.find(room) == m_joinedRooms.end()) {
				m_joinedRooms[room] = presence;
			}

			joinRoom(room, presence->getTo().getResource(), password);
			return;
		}

		getFrontend()->sendRawStanza(this, presence);
	}

	PresenceOracle *presenceOracle = getFrontend()->getPresenceOracle();
	int currentResourcesCount = presenceOracle->getAllPresence(m_jid).size();

	resetResources();


	if (presence->getType() == Swift::Presence::Unavailable) {
		removeJID(presence->getFrom());

		std::string presences;
		std::vector<Swift::Presence::ref> ps = presenceOracle->getAllPresence(m_jid);
		for (Swift::Presence::ref p : ps) {
			if (p != presence) {
				presences += p->getFrom().toString() + " ";
			}
		};

		if (!presences.empty()) {
			LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": User is still connected from following clients: " << presences);
		}
		else {
			LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Last client disconnected");
		}
	}


	XMPPRosterManager *rosterManager = static_cast<XMPPRosterManager *>(getRosterManager());

	// User wants to disconnect this resource
	if (!m_component->inServerMode()) {
		if (presence->getType() == Swift::Presence::Unavailable) {
				// Send unavailable presences for online contacts
				rosterManager->sendUnavailablePresences(presence->getFrom());

				// Send unavailable presence for transport contact itself
				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(getFrontend()->getJID());
				response->setType(Swift::Presence::Unavailable);
				getFrontend()->sendPresence(response);
		}
		else {
			sendCurrentPresence();
		}
	}

	// This resource is new, so we have to send buddies presences
	if (presence->getType() != Swift::Presence::Unavailable && currentResourcesCount != m_resources) {
		rosterManager->sendCurrentPresences(presence->getFrom());
	}

	m_resources = currentResourcesCount;

	// Change legacy network presence
	if (isReadyToConnect()) {
		Swift::Presence::ref highest = presenceOracle->getHighestPriorityPresence(m_jid.toBare());
		if (highest) {
			if (highest->getType() == Swift::Presence::Unavailable && getUserSetting("stay_connected") == "1") {
				m_resources = 0;
				clearJIDs();
				setCacheMessages(true);
				return;
			}
			LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Changing legacy network presence to " << highest->getType());
			sendStatus(highest);
			setCacheMessages(false);
		}
		else {
			if (getUserSetting("stay_connected") == "1") {
				m_resources = 0;
				clearJIDs();
				setCacheMessages(true);
				return;
			}
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(m_jid.toBare());
			response->setFrom(getFrontend()->getJID());
			response->setType(Swift::Presence::Unavailable);
			sendStatus(response);
		}
	}
}

void XMPPUser::sendStatus(Swift::Presence::ref presence) {
	pbnetwork::Status status;
	status.set_username(getUserName());

	if (presence->getShow() == Swift::StatusShow::None) {
		status.set_status(pbnetwork::STATUS_NONE);
		onStatusChanged(status);
		return;
	}

	getFrontend()->sendRawStanza(this, presence);

	bool isInvisible = presence->getPayload<Swift::InvisiblePayload>() != NULL;
	if (isInvisible) {
		LOG4CXX_INFO(xmppUserLogger, "This presence is invisible");
		status.set_status(pbnetwork::STATUS_INVISIBLE);
	}
	else {
		status.set_status((pbnetwork::StatusType) presence->getShow());
	}

	status.set_statusmessage(presence->getStatus());
	onStatusChanged(status);
}

void XMPPUser::handleSubscription(Swift::Presence::ref presence) {
	static_cast<XMPPRosterManager *>(getRosterManager())->handleSubscription(presence);
}

void XMPPUser::handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info) {
	LOG4CXX_INFO(xmppUserLogger, jid.toString() << ": got disco#info");
#ifdef SUPPORT_LEGACY_CAPS
	m_legacyCaps[jid] = info;
#endif

	onConnectingTimeout();
}

void XMPPUser::onConnectingTimeout() {
	if (isConnected() || isReadyToConnect())
		return;
	m_reconnectTimer->stop();
	setReadyToConnect();

	Swift::Presence::ref highest = getFrontend()->getPresenceOracle()->getHighestPriorityPresence(m_jid.toBare());
	if (highest) {
		LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Changing legacy network presence to " << highest->getType());
		sendStatus(highest);
	}
}

std::vector<Swift::JID> XMPPUser::getJIDWithFeature(const std::string &feature) {
       std::vector<Swift::JID> jid;
       std::vector<Swift::Presence::ref> presences = getFrontend()->getPresenceOracle()->getAllPresence(m_jid);

       for (Swift::Presence::ref presence : presences) {
               if (presence->getType() == Swift::Presence::Unavailable)
                       continue;

               Swift::DiscoInfo::ref discoInfo = getFrontend()->sendCapabilitiesRequest(presence->getFrom());
               if (!discoInfo) {
#ifdef SUPPORT_LEGACY_CAPS
                       if (m_legacyCaps.find(presence->getFrom()) != m_legacyCaps.end()) {
                               discoInfo = m_legacyCaps[presence->getFrom()];
                       }
                       else {
                               continue;
                       }

                       if (!discoInfo) {
                               continue;
                       }
#else
                       continue;
#endif
               }

               if (discoInfo->hasFeature(feature)) {
                       LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": Found JID with " << feature << " feature: " << presence->getFrom().toString());
                       jid.push_back(presence->getFrom());
               }
       }

       if (jid.empty()) {
               LOG4CXX_INFO(xmppUserLogger, m_jid.toString() << ": No JID with " << feature << " feature " << m_legacyCaps.size());
       }
       return jid;
}

}
