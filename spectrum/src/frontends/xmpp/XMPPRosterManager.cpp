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

#include "XMPPRosterManager.h"
#include "XMPPFrontend.h"
#include "XMPPUser.h"
#include "PresenceOracle.h"
#include "jidutils.h"
#include "transport/Buddy.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/Factory.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/Config.h"
#include "transport/Util.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "Swiften/Elements/RosterItemExchangePayload.h"
#include "Swiften/Elements/Nickname.h"
#include "Swiften/Elements/VCardUpdate.h"
#include "Swiften/Queries/IQRouter.h"
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(xmppRosterManagerLogger, "XMPPRosterManager");

XMPPRosterManager::XMPPRosterManager(User *user, Component *component) : RosterManager(user, component){
	m_user = user;
	m_component = component;
	m_userJID = Swift::JID(user->getUserName()).toBare();

	m_supportRemoteRoster = false;
	m_RIETimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(5000);
	m_RIETimer->onTick.connect(boost::bind(&XMPPRosterManager::sendRIE, this));

	if (!m_component->inServerMode()) {
		m_remoteRosterRequest = AddressedRosterRequest::ref(new AddressedRosterRequest(getFrontend()->getIQRouter(), m_userJID));
		m_remoteRosterRequest->onResponse.connect(boost::bind(&XMPPRosterManager::handleRemoteRosterResponse, this, _1, _2));
		m_remoteRosterRequest->send();
	}
}

XMPPRosterManager::~XMPPRosterManager() {
	m_RIETimer->stop();
	if (m_remoteRosterRequest) {
		m_remoteRosterRequest->onResponse.disconnect_all_slots();
		getFrontend()->getIQRouter()->removeHandler(m_remoteRosterRequest);
	}

	if (m_requests.size() != 0) {
		LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() <<  ": Removing " << m_requests.size() << " unresponded IQs");
		BOOST_FOREACH(Swift::SetRosterRequest::ref request, m_requests) {
			request->onResponse.disconnect_all_slots();
			getFrontend()->getIQRouter()->removeHandler(request);
		}
		m_requests.clear();
	}

	sendUnavailablePresences(m_userJID);
}

XMPPFrontend *XMPPRosterManager::getFrontend() {
	return static_cast<XMPPFrontend *>(m_component->getFrontend());
}

bool XMPPRosterManager::isValidBuddyName(const std::string &name, BuddyFlag flags) {
	Swift::JID jid = JIDUtils::getBuddyJID(name, flags, m_component->getJID());
	return jid.isValid() && JIDUtils::getSafeName(name, flags).find("/") == std::string::npos;
}

std::vector<Swift::Presence::ref> &XMPPRosterManager::generatePresenceStanzas(Buddy *buddy) {
	std::vector<Swift::Presence::ref> &presences = m_presences[Util::toLowerUTF8(buddy->getName())];
	Swift::JID jid = JIDUtils::getBuddyJID(buddy);

	pbnetwork::StatusType status;
	std::string statusMessage;
	if (!buddy->getStatus(status, statusMessage)) {
		for (std::vector<Swift::Presence::ref>::iterator it = presences.begin(); it != presences.end(); it++) {
			if ((*it)->getFrom() == jid) {
				presences.erase(it);
				break;
			}
		}
		return presences;
	}

	Swift::StatusShow s((Swift::StatusShow::Type) status);

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setTo(m_userJID);
	presence->setFrom(jid);
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	if (s.getType() == Swift::StatusShow::None)
		presence->setType(Swift::Presence::Unavailable);

	presence->setShow(s.getType());

	if (presence->getType() != Swift::Presence::Unavailable) {
		// caps
		presence->addPayload(std::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (buddy->getIconHash())));
	}

	for (Swift::Presence::ref &p : presences) {
		if (p->getFrom() == presence->getFrom()) {
			p = presence;
			return presences;
		}
	}

	presences.push_back(presence);
	return presences;
}

void XMPPRosterManager::handleRawPresence(Buddy *buddy, Swift::Presence::ref presence) {
	std::vector<Swift::Presence::ref> &presences = m_presences[Util::toLowerUTF8(buddy->getName())];
	for (std::vector<Swift::Presence::ref>::iterator it = presences.begin(); it != presences.end(); it++) {
		if ((*it)->getFrom() == presence->getFrom()) {
			presences.erase(it);
			break;
		}
	}

	presences.push_back(presence);
	getFrontend()->sendPresence(presence);
}

void XMPPRosterManager::doSendBuddyStatus(Buddy *buddy) {
	std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(buddy);
	for (Swift::Presence::ref presence : presences) {
		getFrontend()->sendPresence(presence);
	}
}

void XMPPRosterManager::doRequestAuthorization(const std::string &buddyName) {
	// Create subscribe presence and forward it to XMPP side
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_userJID);
	std::string name = buddyName;

	if (CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "service.jid_escaping", true)) {
		name = Swift::JID::getEscapedNode(name);
	}
	else {
		if (name.find_last_of("@") != std::string::npos) {
			name.replace(name.find_last_of("@"), 1, "%");
		}
	}

	response->setFrom(Swift::JID(name, m_component->getJID()));
	response->setType(Swift::Presence::Subscribe);
	getFrontend()->sendPresence(response);
}

void XMPPRosterManager::doRemoveBuddy(Buddy *buddy) {
	if (m_component->inServerMode() || m_supportRemoteRoster) {
		sendBuddyRosterRemove(buddy);
	}
	else {
		sendBuddyUnsubscribePresence(buddy);
	}
	m_presences.erase(Util::toLowerUTF8(buddy->getName()));
}

void XMPPRosterManager::sendBuddyRosterRemove(Buddy *buddy) {
	Swift::RosterPayload::ref p = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(JIDUtils::getBuddyJID(buddy).toBare());
	item.setSubscription(Swift::RosterItemPayload::Remove);

	p->addItem(item);

	// In server mode we have to send pushes to all resources, but in gateway-mode we send it only to bare JID
	if (m_component->inServerMode()) {
		std::vector<Swift::Presence::ref> presences = getFrontend()->getPresenceOracle()->getAllPresence(m_userJID);
		BOOST_FOREACH(Swift::Presence::ref presence, presences) {
			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, presence->getFrom(), getFrontend()->getIQRouter());
			request->send();
		}
	}
	else {
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, m_userJID, getFrontend()->getIQRouter());
		request->send();
	}
}

void XMPPRosterManager::sendBuddyRosterPush(Buddy *buddy) {
	// user can't receive anything in server mode if he's not logged in.
	// He will ask for roster later (handled in rosterreponsder.cpp)
	if (m_component->inServerMode() && (!m_user->isConnected() || m_user->shouldCacheMessages()))
		return;

	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
	Swift::RosterItemPayload item;
	item.setJID(JIDUtils::getBuddyJID(buddy).toBare());
	item.setName(buddy->getAlias());
	item.setGroups(buddy->getGroups());
	item.setSubscription(Swift::RosterItemPayload::Both);

	payload->addItem(item);

	// In server mode we have to send pushes to all resources, but in gateway-mode we send it only to bare JID
	if (m_component->inServerMode()) {
		std::vector<Swift::Presence::ref> presences = getFrontend()->getPresenceOracle()->getAllPresence(m_userJID);
		BOOST_FOREACH(Swift::Presence::ref presence, presences) {
			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, presence->getFrom(), getFrontend()->getIQRouter());
			request->onResponse.connect(boost::bind(&XMPPRosterManager::handleBuddyRosterPushResponse, this, _1, request, buddy->getName()));
			request->send();
			m_requests.push_back(request);
		}
	}
	else {
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, m_userJID, getFrontend()->getIQRouter());
		request->onResponse.connect(boost::bind(&XMPPRosterManager::handleBuddyRosterPushResponse, this, _1, request, buddy->getName()));
		request->send();
		m_requests.push_back(request);
	}

	if (buddy->getSubscription() != Buddy::Both) {
		buddy->setSubscription(Buddy::Both);
		storeBuddy(buddy);
	}
}

void XMPPRosterManager::sendBuddyUnsubscribePresence(Buddy *buddy) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_userJID);
	response->setFrom(JIDUtils::getBuddyJID(buddy));
	response->setType(Swift::Presence::Unsubscribe);
	getFrontend()->sendPresence(response);

	response = Swift::Presence::create();
	response->setTo(m_userJID);
	response->setFrom(JIDUtils::getBuddyJID(buddy));
	response->setType(Swift::Presence::Unsubscribed);
	getFrontend()->sendPresence(response);
}

void XMPPRosterManager::sendBuddySubscribePresence(Buddy *buddy) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(m_userJID);
	response->setFrom(JIDUtils::getBuddyJID(buddy).toBare());
	response->setType(Swift::Presence::Subscribe);
	if (!buddy->getAlias().empty()) {
		response->addPayload(std::make_shared<Swift::Nickname>(buddy->getAlias()));
	}
	getFrontend()->sendPresence(response);
}

void XMPPRosterManager::sendBuddyPresences(Buddy *buddy, const Swift::JID &to) {
	std::vector<Swift::Presence::ref> presences = generatePresenceStanzas(buddy);
	for (Swift::Presence::ref &currentPresence : presences) {
		currentPresence->setTo(to);
		getFrontend()->sendPresence(currentPresence);
	}
}

void XMPPRosterManager::doAddBuddy(Buddy *buddy) {
	// In server mode the only way is to send jabber:iq:roster push.
	// In component mode we send RIE or Subscribe presences, based on features.
	if (m_component->inServerMode()) {
		sendBuddyRosterPush(buddy);
	}
	else {
		if (buddy->getSubscription() == Buddy::Both) {
			LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": Not forwarding this buddy, because subscription=both");
			return;
		}

		if (m_supportRemoteRoster) {
			sendBuddyRosterPush(buddy);
		}
		else {
			m_RIETimer->start();
		}
	}
}

void XMPPRosterManager::doUpdateBuddy(Buddy *buddy) {
	if (m_component->inServerMode() || m_supportRemoteRoster) {
		sendBuddyRosterPush(buddy);
	}
}

void XMPPRosterManager::handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key) {
	LOG4CXX_INFO(xmppRosterManagerLogger, "handleBuddyRosterPushResponse called for buddy " << key);
	Buddy *b = getBuddy(key);
	if (b) {
		if (b->isAvailable()) {
			std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(b);
			BOOST_FOREACH(Swift::Presence::ref &presence, presences) {
				getFrontend()->sendPresence(presence);
			}
		}
	}
	else {
		LOG4CXX_WARN(xmppRosterManagerLogger, "handleBuddyRosterPushResponse called for unknown buddy " << key);
	}

	m_requests.remove(request);
	request->onResponse.disconnect_all_slots();
}

void XMPPRosterManager::handleRemoteRosterResponse(std::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref error) {
	m_remoteRosterRequest.reset();
	if (error) {
		m_supportRemoteRoster = false;
		LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": This server does not allow us to modify your roster, consider enabling XEP-0321 or XEP-0356 support");
		return;
	}

	LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": Roster modification is allowed");
	m_supportRemoteRoster = true;

	//If we receive empty RosterPayload on login (not register) initiate full RosterPush
	if (!m_buddies.empty() && payload->getItems().empty()){
			LOG4CXX_INFO(xmppRosterManagerLogger, "Received empty Roster upon login. Pushing full Roster.");
			for (BuddiesMap::const_iterator c_it = m_buddies.begin();
					c_it != m_buddies.end(); c_it++) {
				sendBuddyRosterPush(c_it->second);
			}
	}
	return;

	BOOST_FOREACH(const Swift::RosterItemPayload &item, payload->getItems()) {
		std::string legacyName = JIDUtils::JIDToLegacyName(item.getJID(), m_user);
		Buddy *b = getBuddy(legacyName);
		if (b) {
			continue;
		}

		if (legacyName.empty()) {
			continue;
		}

		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = item.getName();
		buddyInfo.legacyName = legacyName;
		buddyInfo.subscription = "both";
		buddyInfo.flags = JIDUtils::buddyFlagsFromJID(item.getJID());
		buddyInfo.groups = item.getGroups();

		Buddy *buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
		if (buddy) {
			setBuddy(buddy);
		}
	}
}

void XMPPRosterManager::sendRIE() {
	m_RIETimer->stop();

	// Check the feature, because proper resource could logout during RIETimer.
	std::vector<Swift::JID> jidWithRIE = static_cast<XMPPUser *>(m_user)->getJIDWithFeature("http://jabber.org/protocol/rosterx");

	// fallback to normal subscribe
	if (jidWithRIE.empty()) {
		for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
			Buddy *buddy = (*it).second;
			if (!buddy) {
				continue;
			}
			sendBuddySubscribePresence(buddy);
		}
		return;
	}

	Swift::RosterItemExchangePayload::ref payload = Swift::RosterItemExchangePayload::ref(new Swift::RosterItemExchangePayload());
	for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::RosterItemExchangePayload::Item item;
		item.setJID(JIDUtils::getBuddyJID(buddy).toBare());
		item.setName(buddy->getAlias());
		item.setAction(Swift::RosterItemExchangePayload::Item::Add);
		item.setGroups(buddy->getGroups());

		payload->addItem(item);
	}

	BOOST_FOREACH(Swift::JID &jid, jidWithRIE) {
		LOG4CXX_INFO(xmppRosterManagerLogger, "Sending RIE stanza to " << jid.toString());
		std::shared_ptr<Swift::GenericRequest<Swift::RosterItemExchangePayload> > request(new Swift::GenericRequest<Swift::RosterItemExchangePayload>(Swift::IQ::Set, jid, payload, getFrontend()->getIQRouter()));
		request->send();
	}
}

/*
Buddy handling is similar to Pidgin. A buddy can only be:
  Buddy::Both == mutual
  Buddy::Ask  == friend request from them
  no entry    == not a buddy/rejected by user
In gateway mode, anyone the user approves is automatically a Buddy::Both as far as we are concerned.
Forward all friend requests to the backend but ignore the results.
*/
void XMPPRosterManager::handleSubscription(Swift::Presence::ref presence) {
	std::string legacyName = JIDUtils::JIDToLegacyName(presence->getTo(), m_user);
	if (legacyName.empty())
		return;

	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(presence->getFrom().toBare());
	response->setFrom(presence->getTo().toBare());
	response->setType(Swift::Presence::Unavailable); //== do not send

	Buddy *buddy = getBuddy(legacyName);
	bool newBuddy = (!buddy);
	if (newBuddy) {
		//Need a temporary buddy for most operations
		LOG4CXX_TRACE(xmppRosterManagerLogger, "handleSubscription(): unknown buddy");
		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = "";
		buddyInfo.legacyName = legacyName;
		buddyInfo.subscription = "both";
		buddyInfo.flags = JIDUtils::buddyFlagsFromJID(presence->getTo());
		buddy = m_component->getFactory()->createBuddy(this, buddyInfo);
	} else
		LOG4CXX_TRACE(xmppRosterManagerLogger, "handleSubscription(): known buddy");


	// For server mode the subscription changes are handler in rosterresponder.cpp
	// using roster pushes.
	if (m_component->inServerMode()) {
		if (buddy)
			LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": Subscription received and buddy " << legacyName << " is already there => answering");
		switch (presence->getType()) {
			case Swift::Presence::Subscribe:
				if (newBuddy) {
					// buddy is not in roster, so add him
					LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": Subscription received for new buddy " << legacyName << " => adding to legacy network");
					setBuddy(buddy);
					newBuddy=false; //passed on, do not autodelete
				}
				onBuddyAdded(buddy);
				response->setType(Swift::Presence::Subscribed);
				break;

			case Swift::Presence::Subscribed:
				if (!newBuddy) {
					onBuddyAdded(buddy);
				}
//				else
// 					onBuddyAdded(buddy);
				break;

			case Swift::Presence::Unsubscribe:
				onBuddyRemoved(buddy);
				if (!newBuddy) {
					removeBuddy(buddy->getName());
					buddy = NULL;
				}
				response->setType(Swift::Presence::Unsubscribed);
				break;

			case Swift::Presence::Available:
			case Swift::Presence::Error:
			case Swift::Presence::Probe:
			case Swift::Presence::Unavailable:
			case Swift::Presence::Unsubscribed:
				break;
		}
	}
	else {
		switch (presence->getType()) {
			case Swift::Presence::Subscribe:	// Add a friend on the backend network & send friend request.
			case Swift::Presence::Subscribed:   // Accept friend request / Mark buddy as a friend
				//Add buddy as Buddy::Both, confirm friend requests now and henceforth
				LOG4CXX_TRACE(xmppRosterManagerLogger, "handleSubscription(): Subscribe/Subscribed");
				if (newBuddy) {
					// buddy is not in roster, so add him
					LOG4CXX_INFO(xmppRosterManagerLogger, m_userJID.toString() << ": Subscription received for new buddy " << legacyName << " => adding to legacy network");
					setBuddy(buddy);
					newBuddy=false; //do not autodelete
				} else if (buddy->getSubscription() != Buddy::Both) {
					buddy->setSubscription(Buddy::Both);
					storeBuddy(buddy);
				}
				onBuddyAdded(buddy); //re-confirm, even if the buddy has already been Buddy::Both - it won't hurt
				sendBuddyPresences(buddy, presence->getFrom());
				//Auto-accept, as far as XMPP is concerned
				if (presence->getType() == Swift::Presence::Subscribe)
					response->setType(Swift::Presence::Subscribed);
				break;

			case Swift::Presence::Unsubscribe:   // Throw away the friend privileges that the contact has given us.
				LOG4CXX_TRACE(xmppRosterManagerLogger, "handleSubscription(): Unsubscribe");
				//Delete buddy and reject any friend requests
				//If delete-protection is enabled, only delete from backend if the friend is Buddy::Ask or new.
				if (newBuddy || (buddy->getSubscription() == Buddy::Ask) || CONFIG_BOOL(m_component->getConfig(), "service.enable_remove_buddy")) {
					onBuddyRemoved(buddy);
				}
				if (!newBuddy) {
					removeBuddy(buddy->getName());
					buddy = NULL;
				}
				//XEP says the buddy should send "unsubscribed" to this request
				response->setType(Swift::Presence::Unsubscribed);
				break;

			case Swift::Presence::Unsubscribed:  // Remove contact's friend privileges / Deny friend request / Confirm UNSUBSCRIBE
				LOG4CXX_TRACE(xmppRosterManagerLogger, "handleSubscription(): Unsubscribed");
				//Only remove-buddy if this is a real friend request rejection
				//XMPP may send <unsubscribed> in other cases, e.g. when the user refuses the formal <subscribe>
				//that we've sent to inform them of existing ::Both buddies. We should not delete existing buddies then!
				if (buddy->getSubscription() == Buddy::Ask) {
					onBuddyRemoved(buddy);
					removeBuddy(buddy->getName());
					buddy = NULL;
					//XEP says the buddy should send "unsubscribe" if his attempt to "subscribe" had been rejected
					response->setType(Swift::Presence::Unsubscribe);
				}
				break;

			case Swift::Presence::Available:
			case Swift::Presence::Error:
			case Swift::Presence::Probe:
			case Swift::Presence::Unavailable:
				break;
		}
	}

	if (response->getType() != Swift::Presence::Unavailable)
		getFrontend()->sendPresence(response);

	if (!m_component->inServerMode())
		// We have to act as buddy and send its subscribe/unsubscribe just to be sure...
		switch (response->getType()) {
			case Swift::Presence::Unsubscribed:
				response->setType(Swift::Presence::Unsubscribe);
				getFrontend()->sendPresence(response);
				break;
			case Swift::Presence::Subscribed:
				response->setType(Swift::Presence::Subscribe);
				getFrontend()->sendPresence(response);
				break;
			default:
				break;
		}

	//Delete the temporary buddy
	if (newBuddy && buddy)
		delete buddy;
}

Swift::RosterPayload::ref XMPPRosterManager::generateRosterPayload() {
	Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());

	for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		Swift::RosterItemPayload item;
		item.setJID(JIDUtils::getBuddyJID(buddy).toBare());
		item.setName(buddy->getAlias());
		item.setGroups(buddy->getGroups());
		item.setSubscription(Swift::RosterItemPayload::Both);
		payload->addItem(item);
	}
	return payload;
}

void XMPPRosterManager::sendCurrentPresences(const Swift::JID &to) {
	for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}
		if (!buddy->isAvailable()) {
			continue;
		}
		std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(buddy);
		for (Swift::Presence::ref &presence : presences) {
			presence->setTo(to);
			getFrontend()->sendPresence(presence);
		}
	}
}

void XMPPRosterManager::sendCurrentPresence(const Swift::JID &from, const Swift::JID &to) {
	Buddy *buddy = JIDUtils::JIDToBuddy(from, m_user);
	if (buddy) {
		std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(buddy);
		for (Swift::Presence::ref &presence : presences) {
			presence->setTo(to);
			getFrontend()->sendPresence(presence);
		}
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(to);
		response->setFrom(from);
		response->setType(Swift::Presence::Unavailable);
		getFrontend()->sendPresence(response);
	}
}

void XMPPRosterManager::sendUnavailablePresences(const Swift::JID &to) {
	for (BuddiesMap::iterator it = m_buddies.begin(); it != m_buddies.end(); it++) {
		Buddy *buddy = (*it).second;
		if (!buddy) {
			continue;
		}

		if (!buddy->isAvailable()) {
			continue;
		}

		std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(buddy);
		for (Swift::Presence::ref &presence : presences) {
			Swift::Presence::Type type = presence->getType();
			presence->setTo(to);
			presence->setType(Swift::Presence::Unavailable);
			getFrontend()->sendPresence(presence);
			presence->setType(type);
		}
	}

	// in gateway mode, we have to send unavailable presence for transport
	// contact
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(to);
	response->setFrom(getFrontend()->getJID());
	response->setType(Swift::Presence::Unavailable);
	getFrontend()->sendPresence(response);
}

}
