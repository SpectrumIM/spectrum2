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

#include "transport/userregistration.h"
#include "transport/usermanager.h"
#include "transport/storagebackend.h"
#include "transport/transport.h"
#include "transport/rostermanager.h"
#include "transport/user.h"
#include "transport/logging.h"
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

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(logger, "UserRegistration");

UserRegistration::UserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend) : Swift::Responder<Swift::InBandRegistrationPayload>(component->m_iqRouter) {
	m_component = component;
	m_config = m_component->m_config;
	m_storageBackend = storageBackend;
	m_userManager = userManager;
}

UserRegistration::~UserRegistration(){
}

bool UserRegistration::registerUser(const UserInfo &row) {
	UserInfo user;
	bool registered = m_storageBackend->getUser(row.jid, user);
	// This user is already registered
	if (registered)
		return false;

	m_storageBackend->setUser(row);

	//same as in unregisterUser but here we have to pass UserInfo to handleRegisterRRResponse
	AddressedRosterRequest::ref request = AddressedRosterRequest::ref(new AddressedRosterRequest(m_component->getIQRouter(),row.jid));
	request->onResponse.connect(boost::bind(&UserRegistration::handleRegisterRemoteRosterResponse, this, _1, _2, row));
	request->send();

	return true;
}

bool UserRegistration::unregisterUser(const std::string &barejid) {
	UserInfo userInfo;
	bool registered = m_storageBackend->getUser(barejid, userInfo);
	// This user is not registered
	if (!registered)
		return false;

	onUserUnregistered(userInfo);

	// We have to check if server supports remoteroster XEP and use it if it's supported or fallback to unsubscribe otherwise
	AddressedRosterRequest::ref request = AddressedRosterRequest::ref(new AddressedRosterRequest(m_component->getIQRouter(), barejid));
	request->onResponse.connect(boost::bind(&UserRegistration::handleUnregisterRemoteRosterResponse, this, _1, _2, barejid));
	request->send();

	return true;
}

void UserRegistration::handleRegisterRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref remoteRosterNotSupported /*error*/, const UserInfo &row){
	if (remoteRosterNotSupported || !payload) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setFrom(m_component->getJID());
		response->setTo(Swift::JID(row.jid));
		response->setType(Swift::Presence::Subscribe);
		m_component->getStanzaChannel()->sendPresence(response);
	}
	else{
		Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
		Swift::RosterItemPayload item;
		item.setJID(m_component->getJID());
		item.setSubscription(Swift::RosterItemPayload::Both);
		payload->addItem(item);
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, row.jid, m_component->getIQRouter());
		request->send();
	}
	onUserRegistered(row);

	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"registration.notify_jid");
	BOOST_FOREACH(const std::string &notify_jid, x) {
		boost::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody(std::string("registered: ") + row.jid);
		msg->setTo(notify_jid);
		msg->setFrom(m_component->getJID());
		m_component->getStanzaChannel()->sendMessage(msg);
	}
}

void UserRegistration::handleUnregisterRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref remoteRosterNotSupported /*error*/, const std::string &barejid) {
	UserInfo userInfo;
	bool registered = m_storageBackend->getUser(barejid, userInfo);
	// This user is not registered
	if (!registered)
		return;

	if (remoteRosterNotSupported || !payload) {
		std::list <BuddyInfo> roster;
		m_storageBackend->getBuddies(userInfo.id, roster);
		for(std::list<BuddyInfo>::iterator u = roster.begin(); u != roster.end() ; u++){
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
			response->setTo(Swift::JID(barejid));
			response->setFrom(Swift::JID(name, m_component->getJID().toString()));
			response->setType(Swift::Presence::Unsubscribe);
			m_component->getStanzaChannel()->sendPresence(response);

			response = Swift::Presence::create();
			response->setTo(Swift::JID(barejid));
			response->setFrom(Swift::JID(name, m_component->getJID().toString()));
			response->setType(Swift::Presence::Unsubscribed);
			m_component->getStanzaChannel()->sendPresence(response);
		}
	}
	else {
		BOOST_FOREACH(Swift::RosterItemPayload it, payload->getItems()) {
			Swift::RosterPayload::ref p = Swift::RosterPayload::ref(new Swift::RosterPayload());
			Swift::RosterItemPayload item;
			item.setJID(it.getJID());
			item.setSubscription(Swift::RosterItemPayload::Remove);

			p->addItem(item);

			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, barejid, m_component->getIQRouter());
			request->send();
		}
	}

	// Remove user from database
	m_storageBackend->removeUser(userInfo.id);

	// Disconnect the user
	User *user = m_userManager->getUser(barejid);
	if (user) {
		m_userManager->removeUser(user);
	}

	if (remoteRosterNotSupported || !payload) {
		Swift::Presence::ref response;
		response = Swift::Presence::create();
		response->setTo(Swift::JID(barejid));
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unsubscribe);
		m_component->getStanzaChannel()->sendPresence(response);

		response = Swift::Presence::create();
		response->setTo(Swift::JID(barejid));
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unsubscribed);
		m_component->getStanzaChannel()->sendPresence(response);
	}
	else {
		Swift::RosterPayload::ref payload = Swift::RosterPayload::ref(new Swift::RosterPayload());
		Swift::RosterItemPayload item;
		item.setJID(m_component->getJID());
		item.setSubscription(Swift::RosterItemPayload::Remove);
		payload->addItem(item);

		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, barejid, m_component->getIQRouter());
		request->send();
	}

	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"registration.notify_jid");
	BOOST_FOREACH(const std::string &notify_jid, x) {
		boost::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody(std::string("unregistered: ") + barejid);
		msg->setTo(notify_jid);
		msg->setFrom(m_component->getJID());
		m_component->getStanzaChannel()->sendMessage(msg);
	}
}

bool UserRegistration::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	// TODO: backend should say itself if registration is needed or not...
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::vector<std::string> const &x = CONFIG_VECTOR(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
			LOG4CXX_INFO(logger, barejid << ": This user has no permissions to register an account")
			sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	boost::shared_ptr<InBandRegistrationPayload> reg(new InBandRegistrationPayload());

	UserInfo res;
	bool registered = m_storageBackend->getUser(barejid, res);

	std::string instructions = CONFIG_STRING(m_config, "registration.instructions");
	std::string usernameField = CONFIG_STRING(m_config, "registration.username_label");

	// normal jabber:iq:register
	reg->setInstructions(instructions);
	reg->setRegistered(registered);
	reg->setUsername(res.uin);
	if (CONFIG_BOOL_DEFAULTED(m_config, "registration.needPassword", true)) {
		reg->setPassword("");
	}


	// form
	Form::ref form(new Form(Form::FormType));
	form->setTitle((("Registration")));
	form->setInstructions((instructions));
#if HAVE_SWIFTEN_3
	FormField::ref type = boost::make_shared<FormField>(FormField::HiddenType, "jabber:iq:register");	
#else
	HiddenFormField::ref type = HiddenFormField::create();
	type->setValue("jabber:iq:register");
#endif
	type->setName("FORM_TYPE");
	form->addField(type);
#if HAVE_SWIFTEN_3
	FormField::ref username = boost::make_shared<FormField>(FormField::TextSingleType, res.uin);
#else
	TextSingleFormField::ref username = TextSingleFormField::create();
	username->setValue(res.uin);
#endif
	username->setName("username");
	username->setLabel((usernameField));
	username->setRequired(true);
	form->addField(username);

	if (CONFIG_BOOL_DEFAULTED(m_config, "registration.needPassword", true)) {
#if HAVE_SWIFTEN_3
		FormField::ref password = boost::make_shared<FormField>(FormField::TextPrivateType);
#else
		TextPrivateFormField::ref password = TextPrivateFormField::create();
#endif
		password->setName("password");
		password->setLabel((("Password")));
		password->setRequired(true);
		form->addField(password);
	}
#if HAVE_SWIFTEN_3
	FormField::ref language = boost::make_shared<FormField>(FormField::ListSingleType);
#else
	ListSingleFormField::ref language = ListSingleFormField::create();
#endif
	language->setName("language");
	language->setLabel((("Language")));
	language->addOption(Swift::FormField::Option(CONFIG_STRING(m_config, "registration.language"), CONFIG_STRING(m_config, "registration.language")));
	if (registered)
#if HAVE_SWIFTEN_3
		language->addValue(res.language);
#else
		language->setValue(res.language);
#endif
	else
#if HAVE_SWIFTEN_3
		language->addValue(CONFIG_STRING(m_config, "registration.language"));
#else
		language->setValue(CONFIG_STRING(m_config, "registration.language"));
#endif
	form->addField(language);

//	TextSingleFormField::ref encoding = TextSingleFormField::create();
//	encoding->setName("encoding");
//	encoding->setLabel((("Encoding")));
//	if (registered)
//		encoding->setValue(res.encoding);
//	else
//		encoding->setValue(CONFIG_STRING(m_config, "registration.encoding"));
//	form->addField(encoding);

	if (registered) {
#if HAVE_SWIFTEN_3
		FormField::ref boolean = boost::make_shared<FormField>(FormField::BooleanType, "0");
#else
		BooleanFormField::ref boolean = BooleanFormField::create();
		boolean->setValue(0);
#endif
		boolean->setName("unregister");
		boolean->setLabel((("Remove your registration")));		
		form->addField(boolean);
	} else {
		if (CONFIG_BOOL(m_config,"registration.require_local_account")) {
			std::string localUsernameField = CONFIG_STRING(m_config, "registration.local_username_label");
#if HAVE_SWIFTEN_3
			FormField::ref local_username = boost::make_shared<FormField>(FormField::TextSingleType);
#else
			TextSingleFormField::ref local_username = TextSingleFormField::create();
#endif
			local_username->setName("local_username");
			local_username->setLabel((localUsernameField));
			local_username->setRequired(true);
			form->addField(local_username);
#if HAVE_SWIFTEN_3
			FormField::ref local_password = boost::make_shared<FormField>(FormField::TextPrivateType);
#else
			TextPrivateFormField::ref local_password = TextPrivateFormField::create();
#endif
			local_password->setName("local_password");
			local_password->setLabel((("Local Password")));
			local_password->setRequired(true);
			form->addField(local_password);
		}
	}

	reg->setForm(form);

	sendResponse(from, id, reg);

	return true;
}

bool UserRegistration::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	// TODO: backend should say itself if registration is needed or not...
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::vector<std::string> const &x = CONFIG_VECTOR(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
			LOG4CXX_INFO(logger, barejid << ": This user has no permissions to register an account")
			sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	UserInfo res;
	bool registered = m_storageBackend->getUser(barejid, res);

	std::string encoding;
	std::string language;
	std::string local_username("");
	std::string local_password("");

	Form::ref form = payload->getForm();
	if (form) {
		const std::vector<FormField::ref> fields = form->getFields();
		for (std::vector<FormField::ref>::const_iterator it = fields.begin(); it != fields.end(); it++) {
#if HAVE_SWIFTEN_3
			FormField::ref textSingle = *it;
			if (textSingle->getType() == FormField::TextSingleType || textSingle->getType() == FormField::UnknownType) {
#else
			TextSingleFormField::ref textSingle = boost::dynamic_pointer_cast<TextSingleFormField>(*it);
			if (textSingle) {
#endif
				if (textSingle->getName() == "username") {
#if HAVE_SWIFTEN_3
					payload->setUsername(textSingle->getTextSingleValue());
#else
					payload->setUsername(textSingle->getValue());
#endif
				}
				else if (textSingle->getName() == "encoding") {
#if HAVE_SWIFTEN_3
					encoding = textSingle->getTextSingleValue();
#else
					encoding = textSingle->getValue();
#endif
				}
				// Pidgin sends it as textSingle, not sure why...
				else if (textSingle->getName() == "password") {
#if HAVE_SWIFTEN_3
					payload->setPassword(textSingle->getTextSingleValue());
#else
					payload->setPassword(textSingle->getValue());
#endif
				}
				else if (textSingle->getName() == "local_username") {
#if HAVE_SWIFTEN_3
					local_username = textSingle->getTextSingleValue();
#else
					local_username = textSingle->getValue();
#endif
				}
				// Pidgin sends it as textSingle, not sure why...
				else if (textSingle->getName() == "local_password") {
#if HAVE_SWIFTEN_3
					local_password = textSingle->getTextSingleValue();
#else
					local_password = textSingle->getValue();
#endif
				}
				// Pidgin sends it as textSingle, not sure why...
				else if (textSingle->getName() == "unregister") {
#if HAVE_SWIFTEN_3
					if (textSingle->getTextSingleValue() == "1" || textSingle->getTextSingleValue() == "true") {
#else
					if (textSingle->getValue() == "1" || textSingle->getValue() == "true") {
#endif
						payload->setRemove(true);
					}
				}
				continue;
			}
#if HAVE_SWIFTEN_3
			FormField::ref textPrivate = *it;
			if (textPrivate->getType() == FormField::TextPrivateType) {
#else
			TextPrivateFormField::ref textPrivate = boost::dynamic_pointer_cast<TextPrivateFormField>(*it);
			if (textPrivate) {
#endif
				if (textPrivate->getName() == "password") {
#if HAVE_SWIFTEN_3
					payload->setPassword(textPrivate->getTextPrivateValue());
#else
					payload->setPassword(textPrivate->getValue());
#endif
				}
				else if (textPrivate->getName() == "local_password") {
#if HAVE_SWIFTEN_3
					local_password = textPrivate->getTextPrivateValue();
#else
					local_password = textPrivate->getValue();
#endif
				}
				continue;
			}
#if HAVE_SWIFTEN_3
			FormField::ref listSingle = *it;
			if (listSingle->getType() == FormField::ListSingleType) {
#else
			ListSingleFormField::ref listSingle = boost::dynamic_pointer_cast<ListSingleFormField>(*it);
			if (listSingle) {
#endif
				if (listSingle->getName() == "language") {
#if HAVE_SWIFTEN_3
					language = listSingle->getValues()[0];
#else
					language = listSingle->getValue();
#endif
				}
				continue;
			}
#if HAVE_SWIFTEN_3
			FormField::ref boolean = *it;
			if (boolean->getType() == FormField::BooleanType) {
#else
			BooleanFormField::ref boolean = boost::dynamic_pointer_cast<BooleanFormField>(*it);
			if (boolean) {
#endif
				if (boolean->getName() == "unregister") {
#if HAVE_SWIFTEN_3
					if (boolean->getBoolValue()) {
#else
					if (boolean->getValue()) {
#endif
						payload->setRemove(true);
					}
				}
				continue;
			}
		}
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
// 		Log("UserRegistration", "This is not valid username: "<< newUsername);
// 		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
// 		return true;
// 	}

	if (!CONFIG_STRING(m_config, "registration.allowed_usernames").empty()) {
		boost::regex expression(CONFIG_STRING(m_config, "registration.allowed_usernames"));
		if (!regex_match(newUsername, expression)) {
			LOG4CXX_INFO(logger, "This is not valid username: " << newUsername);
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
		onUserUpdated(res);
	}

	sendResponse(from, id, InBandRegistrationPayload::ref());
	return true;
}

}
