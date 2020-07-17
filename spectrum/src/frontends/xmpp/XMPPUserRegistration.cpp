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

#include "XMPPUserRegistration.h"
#include "XMPPRosterManager.h"
#include "transport/UserManager.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "formutils.h"
#include "transport/Buddy.h"
#include "transport/Config.h"
#include "Swiften/Elements/ErrorPayload.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"
#include "Swiften/Network/BoostNetworkFactories.h"
#include "Swiften/Client/Client.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#if HAVE_SWIFTEN_3
#include <Swiften/Elements/Form.h>
#endif
#include "XMPPFrontend.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(xmppUserRegistrationLogger, "XMPPUserRegistration");

XMPPUserRegistration::XMPPUserRegistration(Component *component, UserManager *userManager,
								   StorageBackend *storageBackend)
: UserRegistration(component, userManager, storageBackend), Swift::Responder<Swift::InBandRegistrationPayload>(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter()) {
	m_component = component;
	m_config = m_component->getConfig();
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

XMPPUserRegistration::~XMPPUserRegistration(){
}

bool XMPPUserRegistration::doUserRegistration(const UserInfo &row) {
	// Check if the server supports remoteroster XEP by sending request for the registered user's roster.
	AddressedRosterRequest::ref request = AddressedRosterRequest::ref(new AddressedRosterRequest(static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter(), row.jid));
	request->onResponse.connect(boost::bind(&XMPPUserRegistration::handleRegisterRemoteRosterResponse, this, _1, _2, row));
	request->send();

	return true;
}

bool XMPPUserRegistration::doUserUnregistration(const UserInfo &row) {
	// Check if the server supports remoteroster XEP by sending request for the registered user's roster.
	AddressedRosterRequest::ref request = AddressedRosterRequest::ref(new AddressedRosterRequest(static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter(), row.jid));
	request->onResponse.connect(boost::bind(&XMPPUserRegistration::handleUnregisterRemoteRosterResponse, this, _1, _2, row));
	request->send();

	return true;
}

void XMPPUserRegistration::handleRegisterRemoteRosterResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref remoteRosterNotSupported, const UserInfo &row){
	if (remoteRosterNotSupported || !payload) {
		// Remote roster is not support, so send normal Subscribe presence to add transport.
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(m_component->getJID());
		response->setTo(Swift::JID(row.jid));
		response->setType(Swift::Presence::Subscribe);
		m_component->getFrontend()->sendPresence(response);
	}
	else {
		// Remote roster is support, so use remoteroster XEP to add transport.
		Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
		Swift::RosterItemPayload item;
		item.setJID(m_component->getJID());
		item.setSubscription(Swift::RosterItemPayload::Both);
		payload->addItem(item);
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, row.jid, static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
		request->send();
	}

	onUserRegistered(row);

	// If the JID for registration notification is configured, send the notification message.
	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"registration.notify_jid");
	BOOST_FOREACH(const std::string &notify_jid, x) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody(std::string("registered: ") + row.jid);
		msg->setTo(notify_jid);
		msg->setFrom(m_component->getJID());
		m_component->getFrontend()->sendMessage(msg);
	}
}

void XMPPUserRegistration::handleUnregisterRemoteRosterResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref remoteRosterNotSupported, const UserInfo &userInfo) {
	if (remoteRosterNotSupported || !payload) {
		// Remote roster is ont support, so get the buddies from database
		// and send Unsubsribe and Unsubscribed presence to them.
		std::list<BuddyInfo> roster;
		m_storageBackend->getBuddies(userInfo.id, roster);
		for (std::list<BuddyInfo>::iterator u = roster.begin(); u != roster.end() ; u++){
			std::string name = (*u).legacyName;
			if ((*u).flags & BUDDY_JID_ESCAPING) {
				name = Swift::JID::getEscapedNode((*u).legacyName);
			}
			else {
				if (name.find_last_of("@") != std::string::npos) {
					name.replace(name.find_last_of("@"), 1, "%");
				}
			}

			Swift::Presence::ref response;
			response = Swift::Presence::create();
			response->setTo(Swift::JID(userInfo.jid));
			response->setFrom(Swift::JID(name, m_component->getJID().toString()));
			response->setType(Swift::Presence::Unsubscribe);
			m_component->getFrontend()->sendPresence(response);

			response = Swift::Presence::create();
			response->setTo(Swift::JID(userInfo.jid));
			response->setFrom(Swift::JID(name, m_component->getJID().toString()));
			response->setType(Swift::Presence::Unsubscribed);
			m_component->getFrontend()->sendPresence(response);
		}
	}
	else {
		// Remote roster is support, so iterate over all buddies we received
		// from the XMPP server and remove them using remote roster.
		BOOST_FOREACH(Swift::RosterItemPayload it, payload->getItems()) {
			if (it.getJID().getDomain() != m_component->getJID().getDomain()) continue;
			Swift::RosterPayload::ref p = Swift::RosterPayload::ref(new Swift::RosterPayload());
			Swift::RosterItemPayload item;
			item.setJID(it.getJID());
			item.setSubscription(Swift::RosterItemPayload::Remove);

			p->addItem(item);

			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, userInfo.jid, static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
			request->send();
		}
	}

	// Remove user from database
	m_storageBackend->removeUser(userInfo.id);

	// Disconnect the user
	User *user = m_userManager->getUser(userInfo.jid);
	if (user) {
		m_userManager->removeUser(user);
	}

	// Remove the transport contact itself the same way as the buddies.
	if (remoteRosterNotSupported || !payload) {
		Swift::Presence::ref response;
		response = Swift::Presence::create();
		response->setTo(Swift::JID(userInfo.jid));
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unsubscribe);
		m_component->getFrontend()->sendPresence(response);

		response = Swift::Presence::create();
		response->setTo(Swift::JID(userInfo.jid));
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unsubscribed);
		m_component->getFrontend()->sendPresence(response);
	}
	else {
		Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
		Swift::RosterItemPayload item;
		item.setJID(m_component->getJID());
		item.setSubscription(Swift::RosterItemPayload::Remove);
		payload->addItem(item);

		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, userInfo.jid, static_cast<XMPPFrontend *>(m_component->getFrontend())->getIQRouter());
		request->send();
	}

	// If the JID for registration notification is configured, send the notification message.
	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"registration.notify_jid");
	BOOST_FOREACH(const std::string &notify_jid, x) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody(std::string("unregistered: ") + userInfo.jid);
		msg->setTo(notify_jid);
		msg->setFrom(m_component->getJID());
		m_component->getFrontend()->sendMessage(msg);
	}
}

Form::ref XMPPUserRegistration::generateRegistrationForm(const UserInfo &res, bool registered) {
	Form::ref form(new Form(Form::FormType));
	form->setTitle("Registration");
	form->setInstructions(CONFIG_STRING(m_config, "registration.instructions"));

	FormUtils::addHiddenField(form, "FORM_TYPE", "jabber:iq:register");
	FormUtils::addTextSingleField(form, "username", res.uin,
								  CONFIG_STRING(m_config, "registration.username_label"),
								  true);

	if (CONFIG_BOOL_DEFAULTED(m_config, "registration.needPassword", true)) {
		FormUtils::addTextPrivateField(form, "password", "Password", true);
	}

	std::string defLanguage = CONFIG_STRING(m_config, "registration.language");
	Swift::FormField::Option languages(defLanguage, defLanguage);
	FormUtils::addListSingleField(form, "language", languages, "Language",
								  registered ? res.language : defLanguage);


	if (registered) {
		FormUtils::addBooleanField(form, "unregister", "0", "Remove your registration");
	}
	else if (CONFIG_BOOL(m_config,"registration.require_local_account")) {
		std::string localUsernameField = CONFIG_STRING(m_config, "registration.local_username_label");
		FormUtils::addTextSingleField(form, "local_username", "", localUsernameField, true);
		FormUtils::addTextSingleField(form, "local_password", "", "Local password", true);
	}

	return form;
}

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<InBandRegistrationPayload> XMPPUserRegistration::generateInBandRegistrationPayload(const Swift::JID& from) {
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<InBandRegistrationPayload> reg(new InBandRegistrationPayload());

	UserInfo res;
	bool registered = m_storageBackend->getUser(from.toBare().toString(), res);

	reg->setInstructions(CONFIG_STRING(m_config, "registration.instructions"));
	reg->setRegistered(registered);
	reg->setUsername(res.uin);

	if (CONFIG_BOOL_DEFAULTED(m_config, "registration.needPassword", true)) {
		reg->setPassword("");
	}

	Form::ref form = generateRegistrationForm(res, registered);
	reg->setForm(form);

	return reg;
}

bool XMPPUserRegistration::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	// TODO: backend should say itself if registration is needed or not...
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::vector<std::string> const &x = CONFIG_VECTOR(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
			LOG4CXX_INFO(xmppUserRegistrationLogger, from.toBare().toString() << ": This user has no permissions to register an account");
			sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<InBandRegistrationPayload> reg = generateInBandRegistrationPayload(from);
	sendResponse(from, id, reg);

	return true;
}

bool XMPPUserRegistration::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	// TODO: backend should say itself if registration is needed or not...
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::vector<std::string> const &x = CONFIG_VECTOR(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
			LOG4CXX_INFO(xmppUserRegistrationLogger, barejid << ": This user has no permissions to register an account");
			sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	UserInfo res;
	bool registered = m_storageBackend->getUser(barejid, res);

	std::string encoding;
	std::string language;
	std::string local_username;
	std::string local_password;

	Form::ref form = payload->getForm();
	if (form) {
		std::string value;

		value = FormUtils::fieldValue(form, "username", "");
		if (!value.empty()) {
			payload->setUsername(value);
		}

		value = FormUtils::fieldValue(form, "password", "");
		if (!value.empty()) {
			payload->setPassword(value);
		}

		value = FormUtils::fieldValue(form, "unregister", "");
		if (value == "1" || value == "true") {
			payload->setRemove(true);
		}

		encoding = FormUtils::fieldValue(form, "encoding", "");
		local_username = FormUtils::fieldValue(form, "local_username", "");
		local_password = FormUtils::fieldValue(form, "local_password", "");
		language = FormUtils::fieldValue(form, "language", "");
	}

	if (payload->isRemove()) {
		unregisterUser(barejid);
		sendResponse(from, id, InBandRegistrationPayload::ref());
		return true;
	}

	if (CONFIG_BOOL(m_config,"registration.require_local_account")) {
	/*	if (!local_username || !local_password) {
			sendResponse(from, id, InBandRegistrationPayload::ref());
			return true
		} else */ if (local_username == "" || local_password == "") {
			sendResponse(from, id, InBandRegistrationPayload::ref());
			return true;
		}
//		Swift::logging = true;
		bool validLocal = false;
		std::string localLookupServer = CONFIG_STRING(m_config, "registration.local_account_server");
		std::string localLookupJID = local_username + std::string("@") + localLookupServer;
		SimpleEventLoop localLookupEventLoop;
		BoostNetworkFactories localLookupNetworkFactories(&localLookupEventLoop);
		Client localLookupClient(localLookupJID, local_password, &localLookupNetworkFactories);

		// TODO: this is neccessary on my server ... but should maybe omitted
		localLookupClient.setAlwaysTrustCertificates();
		localLookupClient.connect();

		class SimpleLoopRunner {
			public:
				SimpleLoopRunner() {};

				static void run(SimpleEventLoop * loop) {
					loop->run();
				};
		};

		// TODO: Really ugly and hacky solution, any other ideas more than welcome!
		boost::thread thread(boost::bind(&(SimpleLoopRunner::run), &localLookupEventLoop));
		thread.timed_join(boost::posix_time::millisec(CONFIG_INT(m_config, "registration.local_account_server_timeout")));
		localLookupEventLoop.stop();
		thread.join();
		validLocal = localLookupClient.isAvailable();
		localLookupClient.disconnect();
		if (!validLocal) {
			sendError(from, id, ErrorPayload::NotAuthorized, ErrorPayload::Modify);
			return true;
		}
	}

	if (!payload->getUsername() || (!payload->getPassword() && CONFIG_BOOL_DEFAULTED(m_config, "registration.needPassword", true))) {
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	if (!payload->getPassword()) {
		payload->setPassword("");
	}

	// Register or change password
	if (payload->getUsername()->empty()) {
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	// TODO: Move this check to backend somehow
	if (CONFIG_STRING(m_config, "service.protocol") == "prpl-jabber") {
		// User tries to register himself.
		if ((Swift::JID(*payload->getUsername()).toBare() == from.toBare())) {
			sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}

		// User tries to register someone who's already registered.
		UserInfo user_row;
		bool registered = m_storageBackend->getUser(Swift::JID(*payload->getUsername()).toBare().toString(), user_row);
		if (registered) {
			sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}
	}

	std::string username = *payload->getUsername();

	std::string newUsername(username);
	if (!CONFIG_STRING(m_config, "registration.username_mask").empty()) {
		newUsername = CONFIG_STRING(m_config, "registration.username_mask");
		boost::replace_all(newUsername, "$username", username);
	}

//TODO: Part of spectrum1 registration stuff, this should be potentially rewritten for S2 too
// 	if (!m_component->protocol()->isValidUsername(newUsername)) {
// 		Log("XMPPUserRegistration", "This is not valid username: "<< newUsername);
// 		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
// 		return true;
// 	}

	if (!CONFIG_STRING(m_config, "registration.allowed_usernames").empty()) {
		boost::regex expression(CONFIG_STRING(m_config, "registration.allowed_usernames"));
		if (!regex_match(newUsername, expression)) {
			LOG4CXX_INFO(xmppUserRegistrationLogger, "This is not valid username: " << newUsername);
			sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}
	}

	if (!registered) {
		res.jid = barejid;
		res.uin = newUsername;
		res.password = *payload->getPassword();
		res.language = language;
		res.encoding = encoding;
		res.vip = 0;
		res.id = 0;
		registerUser(res);
	}
	else {
		res.jid = barejid;
		res.uin = newUsername;
		res.password = *payload->getPassword();
		res.language = language;
		res.encoding = encoding;
		m_storageBackend->setUser(res);
// 		onUserUpdated(res);
	}

	sendResponse(from, id, InBandRegistrationPayload::ref());
	return true;
}

}
