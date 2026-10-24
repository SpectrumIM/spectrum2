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

#include "XMPPUserManager.h"
#include "XMPPUserRegistration.h"
#include "XMPPUser.h"
#include "XMPPRosterManager.h"
#include "XMPPConversation.h"
#include "PresenceOracle.h"
#include "jidutils.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/ConversationManager.h"
#include "transport/UserRegistry.h"
#include "transport/Config.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "storageresponder.h"
#include "vcardresponder.h"
#include "XMPPFrontend.h"
#include "gatewayresponder.h"
#include "adhocmanager.h"
#include "settingsadhoccommand.h"
#include "RosterResponder.h"
#include "discoitemsresponder.h"

#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/ChatState.h"
#include "Swiften/Elements/AttentionPayload.h"
#include "Swiften/Elements/XHTMLIMPayload.h"
#include "Swiften/Elements/DeliveryReceiptRequest.h"
#include "Swiften/Elements/ErrorPayload.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"

namespace Transport {

DEFINE_LOGGER(xmppUserManagerLogger, "XMPPUserManager");

XMPPUserManager::XMPPUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) : UserManager(component, userRegistry, storageBackend) {
	m_component = component;

	XMPPFrontend *frontend = static_cast<XMPPFrontend *>(component->getFrontend());
	if (storageBackend) {
		m_storageResponder = new StorageResponder(frontend->getIQRouter(), storageBackend, this);
		m_storageResponder->start();
	}
	else {
		m_storageResponder = NULL;
	}

	m_vcardResponder = new VCardResponder(frontend->getIQRouter(), component->getNetworkFactories(), this);
	m_vcardResponder->onVCardRequired.connect(boost::bind(&XMPPUserManager::handleVCardRequired, this, _1, _2, _3));
	m_vcardResponder->onVCardUpdated.connect(boost::bind(&XMPPUserManager::handleVCardUpdated, this, _1, _2));
	m_vcardResponder->start();

	if (storageBackend) {
		m_userRegistration = new XMPPUserRegistration(component, this, storageBackend);
		m_userRegistration->start();
	}
	else {
		m_userRegistration = NULL;
	}

// 	StatsResponder statsResponder(component, this, &plugin, storageBackend);
// 	statsResponder.start();

	m_gatewayResponder = new GatewayResponder(frontend->getIQRouter(), this);
	m_gatewayResponder->start();

	m_rosterResponder = new RosterResponder(frontend->getIQRouter(), this);
	m_rosterResponder->start();

	m_discoItemsResponder = new DiscoItemsResponder(component, this);
	m_discoItemsResponder->start();

	m_adHocManager = new AdHocManager(component, m_discoItemsResponder, this, storageBackend);
	m_adHocManager->start();

	m_settings = new SettingsAdHocCommandFactory();
	m_adHocManager->addAdHocCommand(m_settings);

	if (userRegistry) {
		userRegistry->onConnectUser = [this](const Swift::JID &user) { connectUser(user); };
		userRegistry->onDisconnectUser = [this](const Swift::JID &user) { disconnectUser(user); };
	}

	m_removeTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1);
}

XMPPUserManager::~XMPPUserManager() {
	if (m_storageResponder) {
		m_storageResponder->stop();
		delete m_storageResponder;
	}

	if (m_userRegistration) {
		m_userRegistration->stop();
		delete m_userRegistration;
	}

	m_gatewayResponder->stop();
	delete m_gatewayResponder;

	m_adHocManager->stop();
	delete m_adHocManager;

	m_vcardResponder->stop();
	delete m_vcardResponder;

	m_rosterResponder->stop();
	delete m_rosterResponder;

	m_discoItemsResponder->stop();
	delete m_discoItemsResponder;

	delete m_settings;
}

XMPPFrontend *XMPPUserManager::getFrontend() {
	return static_cast<XMPPFrontend *>(m_component->getFrontend());
}

void XMPPUserManager::sendVCard(unsigned int id, const pbnetwork::VCard &v) {
	std::shared_ptr<Swift::VCard> vcard(new Swift::VCard());
	vcard->setFullName(v.fullname());
	vcard->setNickname(v.nickname());
	vcard->setPhoto(Swift::createByteArray(v.photo()));
	m_vcardResponder->sendVCard(id, vcard);
}

void XMPPUserManager::handleVCardRequired(User *user, const std::string &name, unsigned int id) {
	m_component->getFrontend()->onVCardRequired(user, name, id);
}

void XMPPUserManager::handleVCardUpdated(User *user, std::shared_ptr<Swift::VCard> vcard) {
	if (!vcard) {
		LOG4CXX_INFO(xmppUserManagerLogger, user->getUserName() << ": Received empty VCard");
		return;
	}
	m_component->getFrontend()->onVCardUpdated(user, XMPPFrontend::toVCard(user, vcard));
}

UserRegistration *XMPPUserManager::getUserRegistration() {
	return m_userRegistration;
}

void XMPPUserManager::doRemoveUser(User *user) {
	if (m_component->inServerMode()) {
		disconnectUser(Swift::JID(user->getUserName()));
	}
	else {
		// User could be disconnected by User::handleDisconnect() method, but
		// PresenceOracle could still contain his last presence.
		// We have to clear all received presences for this user in PresenceOracle.
		getFrontend()->getPresenceOracle()->clearPresences(Swift::JID(user->getUserName()).toBare());
	}
}

void XMPPUserManager::handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info) {
	XMPPUser *user = static_cast<XMPPUser *>(getUser(jid.toBare().toString()));
	if (!user) {
		return;
	}

	user->handleDiscoInfo(jid, info);
}

void XMPPUserManager::handlePresence(Swift::Presence::ref presence) {
	std::string barejid = presence->getTo().toBare().toString();
	std::string userkey = presence->getFrom().toBare().toString();
	StorageBackend *storageBackend = getStorageBackend();
	UserRegistry *userRegistry = getUserRegistry();

	User *user = getUser(userkey);
	// Create user class if it's not there
	if (!user) {
		// Admin user is not legacy network user, so do not create User class instance for him
		if (m_component->inServerMode()) {
			std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"service.admin_jid");
			if (std::find(x.begin(), x.end(), presence->getFrom().toBare().toString()) != x.end()) {
				// Send admin contact to the user.
				Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
				Swift::RosterItemPayload item;
				item.setJID(getFrontend()->getJID());
				item.setName("Admin");
				item.setSubscription(Swift::RosterItemPayload::Both);
				payload->addItem(item);

				getFrontend()->sendRosterRequest(payload, presence->getFrom());

				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(getFrontend()->getJID());
				getFrontend()->sendPresence(response);
				return;
		    }
		}

		UserInfo res;
		bool registered = storageBackend ? storageBackend->getUser(userkey, res) : false;

		// No user and unavailable presence -> answer with unavailable
		if (presence->getType() == Swift::Presence::Unavailable || presence->getType() == Swift::Presence::Probe) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setTo(presence->getFrom());
			response->setFrom(presence->getTo());
			response->setType(Swift::Presence::Unavailable);
			getFrontend()->sendPresence(response);

			// bother him with probe presence, just to be
			// sure he is subscribed to us.
			if (/*registered && */presence->getType() == Swift::Presence::Probe) {
				Swift::Presence::ref response = Swift::Presence::create();
				response->setTo(presence->getFrom());
				response->setFrom(presence->getTo());
				response->setType(Swift::Presence::Probe);
				getFrontend()->sendPresence(response);
			}

			// Set user offline in database
			if (storageBackend) {
				UserInfo res;
				bool registered = storageBackend->getUser(userkey, res);
				if (registered) {
					storageBackend->setUserOnline(res.id, false);
				}
			}
			return;
		}

		// In server mode, we don't need registration normally, but for networks like IRC
		// or Twitter where there's no real authorization using password, we have to force
		// registration otherwise some data (like bookmarked rooms) could leak.
		if (m_component->inServerMode()) {
			if (!registered) {
				// If we need registration, stop login process because user is not registered
				if (CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", false)
					&& CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needPassword", true)) {
					userRegistry->onPasswordInvalid(presence->getFrom());
					LOG4CXX_INFO(xmppUserManagerLogger, userkey << ": Tried to login, but is not registered.");
					return;
				}
				res.password = "";
				res.uin = presence->getFrom().getNode();
				res.jid = userkey;
				while (res.uin.find_last_of("%") != std::string::npos) { // OK
					res.uin.replace(res.uin.find_last_of("%"), 1, "@"); // OK
				}
				registered = autoRegisterUser(res);
			}
			res.password = userRegistry->getUserPassword(userkey);
		}

		// We allow auto_register feature in gateway-mode. This allows IRC user to register
		// the transport just by joining the room.
		if (!m_component->inServerMode()) {
			if (!registered && (CONFIG_BOOL(m_component->getConfig(), "registration.auto_register")
				 || !CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "registration.needRegistration", true))) {
				res.password = "";
				res.jid = userkey;

				bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
				if (isMUC) {
					res.uin = presence->getTo().getResource();
				}
				else {
					res.uin = presence->getFrom().toString();
				}
				LOG4CXX_INFO(xmppUserManagerLogger, "Auto-registering user " << userkey << " with uin=" << res.uin);

				registered = autoRegisterUser(res);
			}
		}

		// Unregistered users are not able to login
		if (!registered) {
			LOG4CXX_WARN(xmppUserManagerLogger, "Unregistered user " << userkey << " tried to login");
			return;
		}

		LoginError error;
		user = loginUser(userkey, res, error);
		if (error == LoginNotVIP) {
			if (!CONFIG_STRING(m_component->getConfig(), "service.vip_message").empty()) {
				std::shared_ptr<Swift::Message> msg(new Swift::Message());
				msg->setBody(CONFIG_STRING(m_component->getConfig(), "service.vip_message"));
				msg->setTo(presence->getFrom());
				msg->setFrom(getFrontend()->getJID());
				getFrontend()->sendMessage(msg);
			}

			if (m_component->inServerMode()) {
				userRegistry->onPasswordInvalid(presence->getFrom());
			}
			return;
		}
		else if (error == LoginTransportDisabled) {
			return;
		}
	}

	// User can be handleDisconnected in loginUser, so refresh the pointer
	user = getUser(userkey);
	if (!user) {
		if (userRegistry) {
			userRegistry->onPasswordInvalid(presence->getFrom());
		}
		return;
	}

	// Handle this presence
	static_cast<XMPPUser *>(user)->handlePresence(presence);

	// Unavailable MUC presence should not trigger whole account disconnection, so block it here.
	bool isMUC = presence->getPayload<Swift::MUCPayload>() != NULL || *presence->getTo().getNode().c_str() == '#';
	if (isMUC)
		return;

	// Unavailable presence could remove this user, because he could be unavailable
	if (presence->getType() == Swift::Presence::Unavailable) {
		if (user) {
			if (user->getUserSetting("stay_connected") == "1") {
				return;
			}

			Swift::Presence::ref highest = getFrontend()->getPresenceOracle()->getHighestPriorityPresence(presence->getFrom().toBare());
			// There's no presence for this user, so disconnect
			if (!highest || (highest && highest->getType() == Swift::Presence::Unavailable)) {
				m_removeTimer->onTick.connect(boost::bind(&XMPPUserManager::handleRemoveTimeout, this, user->getUserName(), user, false));
				m_removeTimer->start();
			}
		}
	}
}

void XMPPUserManager::handleRemoveTimeout(const std::string jid, User *u, bool reconnect) {
	m_removeTimer->onTick.disconnect(boost::bind(&XMPPUserManager::handleRemoveTimeout, this, jid, u, reconnect));

	// Maybe this User instance has been deleted in mean time and we would remove new one,
	// so better check for it and ignore deletion if "u" does not exist anymore.
	User *user = getUser(jid);
	if (user != u) {
		return;
	}

	// Remove user
	if (user) {
		removeUser(user);
	}

	// Connect the user again when we're reconnecting.
	if (reconnect) {
		connectUser(jid);
	}
}

void XMPPUserManager::handleMessageReceived(Swift::Message::ref message) {
	if (message->getType() == Swift::Message::Error) {
		return;
	}

	// Do not count chatstate notification...
	std::shared_ptr<Swift::ChatState> statePayload = message->getPayload<Swift::ChatState>();
	if (!statePayload) {
		messageToBackendSent();
	}

	std::string body = message->getBody().get_value_or("");
	if (body.empty() && !statePayload && message->getSubject().empty()) {
		return;
	}

	User *user = getUser(message->getFrom().toBare().toString());
	if (!user){
		return;
	}

	std::string name = JIDUtils::JIDToLegacyName(message->getTo(), user);
	bool pm = message->getType() != Swift::Message::Groupchat;
	XMPPConversation *conv = static_cast<XMPPConversation *>(user->getConversationManager()->getConversationForMessage(name, pm, message->getTo().getResource()));
	if (!conv) {
		return;
	}

	// update resource and send the message
	conv->setJID(message->getFrom());

	user->updateLastActivity();

	if (CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "features.rawxml", false)) {
		getFrontend()->sendRawStanza(user, message);
		return;
	}

	pbnetwork::ConversationMessage msg;
	msg.set_username(user->getUserName());
	msg.set_buddyname(conv->getLegacyName());
	msg.set_message("");

	if (statePayload) {
		switch (statePayload->getChatState()) {
			case Swift::ChatState::Active:
				conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING, msg);
				break;
			case Swift::ChatState::Composing:
				conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING, msg);
				break;
			case Swift::ChatState::Paused:
				conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED, msg);
				break;
			default:
				break;
		}
	}

	if (message->getPayload<Swift::AttentionPayload>()) {
		msg.set_message(body);
		conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_ATTENTION, msg);
		return;
	}

	if (!message->getSubject().empty()) {
		msg.set_message(message->getSubject());
		conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED, msg);
		return;
	}

	std::shared_ptr<Swift::XHTMLIMPayload> xhtmlPayload = message->getPayload<Swift::XHTMLIMPayload>();
	if (xhtmlPayload) {
		msg.set_xhtml(xhtmlPayload->getBody());
	}

	msg.set_message(body);
	if (message->getPayload<Swift::DeliveryReceiptRequest>() && !message->getID().empty()) {
		msg.set_id(message->getID());
	}
	conv->sendMessage(pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE, msg);
}

void XMPPUserManager::handleGeneralPresenceReceived(Swift::Presence::ref presence) {
	LOG4CXX_INFO(xmppUserManagerLogger, "PRESENCE2 " << presence->getTo().toString());
	switch(presence->getType()) {
		case Swift::Presence::Subscribe:
		case Swift::Presence::Subscribed:
		case Swift::Presence::Unsubscribe:
		case Swift::Presence::Unsubscribed:
			handleSubscription(presence);
			break;
		case Swift::Presence::Available:
		case Swift::Presence::Unavailable:
			handleMUCPresence(presence);
			break;
		case Swift::Presence::Probe:
			handleProbePresence(presence);
			break;
		case Swift::Presence::Error:
			handleErrorPresence(presence);
			break;
		default:
			break;
	};
}

void XMPPUserManager::handleMUCPresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
		return;
	}

	if (presence->getType() == Swift::Presence::Available) {
		handlePresence(presence);
	}
	else if (presence->getType() == Swift::Presence::Unavailable) {
		std::string userkey = presence->getFrom().toBare().toString();
		User *user = getUser(userkey);
		if (user) {
			static_cast<XMPPUser *>(user)->handlePresence(presence);
		}
	}
}

void XMPPUserManager::handleProbePresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
		return;
	}

	User *user = getUser(presence->getFrom().toBare().toString());

 	if (user) {
 		static_cast<XMPPRosterManager *>(user->getRosterManager())->sendCurrentPresence(presence->getTo(), presence->getFrom());
 	}
 	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Unavailable);
		getFrontend()->sendPresence(response);
	}
}

void XMPPUserManager::handleErrorPresence(Swift::Presence::ref presence) {
	// Don't let RosterManager to handle presences for us
	if (!presence->getTo().getNode().empty()) {
		return;
	}

	if (!presence->getPayload<Swift::ErrorPayload>()) {
		return;
	}

	if (presence->getPayload<Swift::ErrorPayload>()->getCondition() != Swift::ErrorPayload::SubscriptionRequired) {
		return;
	}

	std::string userkey = presence->getFrom().toBare().toString();
	UserInfo res;
	bool registered = getStorageBackend() ? getStorageBackend()->getUser(userkey, res) : false;
	if (registered) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo().toBare());
		response->setTo(presence->getFrom().toBare());
		response->setType(Swift::Presence::Subscribe);
		getFrontend()->sendPresence(response);
	}
}

void XMPPUserManager::handleSubscription(Swift::Presence::ref presence) {

	// answer to subscibe for transport itself
	if (presence->getType() == Swift::Presence::Subscribe && presence->getTo().getNode().empty()) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo().toBare());
		response->setTo(presence->getFrom().toBare());
		response->setType(Swift::Presence::Subscribed);
		getFrontend()->sendPresence(response);

// 		response = Swift::Presence::create();
// 		response->setFrom(presence->getTo());
// 		response->setTo(presence->getFrom());
// 		response->setType(Swift::Presence::Subscribe);
// 		getFrontend()->sendPresence(response);
		return;
	}
	else if (presence->getType() == Swift::Presence::Unsubscribed && presence->getTo().getNode().empty()) {
		std::string userkey = presence->getFrom().toBare().toString();
		UserInfo res;
		bool registered = getStorageBackend() ? getStorageBackend()->getUser(userkey, res) : false;
		if (registered) {
			Swift::Presence::ref response = Swift::Presence::create();
			response->setFrom(presence->getTo().toBare());
			response->setTo(presence->getFrom().toBare());
			response->setType(Swift::Presence::Subscribe);
			getFrontend()->sendPresence(response);
		}
		return;
	}

	// Don't let RosterManager to handle presences for us
	if (presence->getTo().getNode().empty()) {
		return;
	}

	User *user = getUser(presence->getFrom().toBare().toString());

 	if (user) {
 		static_cast<XMPPUser *>(user)->handleSubscription(presence);
 	}
 	else if (presence->getType() == Swift::Presence::Unsubscribe) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(presence->getTo());
		response->setTo(presence->getFrom());
		response->setType(Swift::Presence::Unsubscribed);
		getFrontend()->sendPresence(response);
	}
// 	else {
// // 		Log(presence->getFrom().toString().getUTF8String(), "Subscribe presence received, but this user is not logged in");
// 	}
}

void XMPPUserManager::connectUser(const Swift::JID &user) {
	UserRegistry *userRegistry = getUserRegistry();
	const std::map<std::string, User *> &users = getUsers();

	// Called by UserRegistry in server mode when user connects the server and wants
	// to connect legacy network
	if (users.find(user.toBare().toString()) != users.end()) {
		if (!m_component->inServerMode()) {
			return;
		}

		User *u = users.find(user.toBare().toString())->second;
		if (u->isConnected()) {
			// User is already logged in, so his password is OK, but this new user has different password => bad password.
			// We can't call m_userRegistry->onPasswordInvalid() here, because this fuction is called from Swift::Parser
			// and onPasswordInvalid destroys whole session together with parser itself, which leads to crash.
			if (userRegistry->getUserPassword(user.toBare().toString()) != u->getUserInfo().password) {
				userRegistry->removeLater(user);
				return;
			}
			if (CONFIG_BOOL(m_component->getConfig(), "service.more_resources")) {
				userRegistry->onPasswordValid(user);
			}
			else {
				// Send message to currently logged in session
				std::shared_ptr<Swift::Message> msg(new Swift::Message());
				msg->setBody("You have signed on from another location.");
				msg->setTo(user);
				msg->setFrom(getFrontend()->getJID());
				getFrontend()->sendMessage(msg);

				// Switch the session = accept new one, disconnect old one.
				// Unavailable presence from old session has to be ignored, otherwise it would disconnect the user from legacy network.
				userRegistry->onPasswordValid(user);
			}
		}
		else {
			// User is created, but not connected => he's loggin in or he just logged out, but hasn't been deleted yet.
			// Stop deletion process if there's any
			m_removeTimer->onTick.disconnect(boost::bind(&XMPPUserManager::handleRemoveTimeout, this, user.toBare().toString(), u, false));

			// Delete old User instance but create new one immediatelly
			m_removeTimer->onTick.connect(boost::bind(&XMPPUserManager::handleRemoveTimeout, this, user.toBare().toString(), u, true));
			m_removeTimer->start();
		}
	}
	else {
		// simulate initial available presence to start connecting this user.
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(getFrontend()->getJID());
		response->setFrom(user);
		response->setType(Swift::Presence::Available);
		getFrontend()->handleGeneralPresence(response);
	}
}


void XMPPUserManager::disconnectUser(const Swift::JID &user) {
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(getFrontend()->getJID());
	response->setFrom(user);
	response->setType(Swift::Presence::Unavailable);
	getFrontend()->handleGeneralPresence(response);
}

}
