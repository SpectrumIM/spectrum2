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
#include "Swiften/Elements/ErrorPayload.h"
#include <boost/shared_ptr.hpp>
#include "log4cxx/logger.h"

using namespace Swift;
using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("UserRegistration");

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

	Swift::Presence::ref response = Swift::Presence::create();
	response->setFrom(m_component->getJID());
	response->setTo(Swift::JID(row.jid));
	response->setType(Swift::Presence::Subscribe);
	m_component->getStanzaChannel()->sendPresence(response);

	onUserRegistered(row);
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

void UserRegistration::handleUnregisterRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> payload, Swift::ErrorPayload::ref remoteRosterNotSupported /*error*/, const std::string &barejid) {
	UserInfo userInfo;
	bool registered = m_storageBackend->getUser(barejid, userInfo);
	// This user is not registered
	if (!registered)
		return;

	if (remoteRosterNotSupported) {
		std::list <BuddyInfo> roster;
		m_storageBackend->getBuddies(userInfo.id, roster);
		for(std::list<BuddyInfo>::iterator u = roster.begin(); u != roster.end() ; u++){
			std::string name = Swift::JID::getEscapedNode((*u).legacyName);

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

	if (remoteRosterNotSupported) {
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
}

bool UserRegistration::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	// TODO: backend should say itself if registration is needed or not...
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::list<std::string> const &x = CONFIG_LIST(m_config,"service.allowed_servers");
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
	if (CONFIG_STRING(m_config, "service.protocol") != "twitter" && CONFIG_STRING(m_config, "service.protocol") != "bonjour")
		reg->setPassword(res.password);


	// form
	Form::ref form(new Form(Form::FormType));
	form->setTitle((("Registration")));
	form->setInstructions((instructions));

	HiddenFormField::ref type = HiddenFormField::create();
	type->setName("FORM_TYPE");
	type->setValue("jabber:iq:register");
	form->addField(type);

	TextSingleFormField::ref username = TextSingleFormField::create();
	username->setName("username");
	username->setLabel((usernameField));
	username->setValue(res.uin);
	username->setRequired(true);
	form->addField(username);

	if (CONFIG_STRING(m_config, "service.protocol") != "twitter" && CONFIG_STRING(m_config, "service.protocol") != "bonjour") {
		TextPrivateFormField::ref password = TextPrivateFormField::create();
		password->setName("password");
		password->setLabel((("Password")));
		password->setRequired(true);
		form->addField(password);
	}

	ListSingleFormField::ref language = ListSingleFormField::create();
	language->setName("language");
	language->setLabel((("Language")));
	language->addOption(Swift::FormField::Option(CONFIG_STRING(m_config, "registration.language"), CONFIG_STRING(m_config, "registration.language")));
	if (registered)
		language->setValue(res.language);
	else
		language->setValue(CONFIG_STRING(m_config, "registration.language"));
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
		BooleanFormField::ref boolean = BooleanFormField::create();
		boolean->setName("unregister");
		boolean->setLabel((("Remove your registration")));
		boolean->setValue(0);
		form->addField(boolean);
	} else {
		if (CONFIG_BOOL(m_config,"registration.require_local_account")) {
			std::string localUsernameField = CONFIG_STRING(m_config, "registration.local_username_label");
			TextSingleFormField::ref local_username = TextSingleFormField::create();
			local_username->setName("local_username");
			local_username->setLabel((localUsernameField));
			local_username->setRequired(true);
			form->addField(local_username);
			TextPrivateFormField::ref local_password = TextPrivateFormField::create();
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
		std::list<std::string> const &x = CONFIG_LIST(m_config,"service.allowed_servers");
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
			TextSingleFormField::ref textSingle = boost::dynamic_pointer_cast<TextSingleFormField>(*it);
			if (textSingle) {
				if (textSingle->getName() == "username") {
					payload->setUsername(textSingle->getValue());
				}
				else if (textSingle->getName() == "encoding") {
					encoding = textSingle->getValue();
				}
				// Pidgin sends it as textSingle, not sure why...
				else if (textSingle->getName() == "password") {
					payload->setPassword(textSingle->getValue());
				}
				else if (textSingle->getName() == "local_username") {
					local_username = textSingle->getValue();
				}
				// Pidgin sends it as textSingle, not sure why...
				else if (textSingle->getName() == "local_password") {
					local_password = textSingle->getValue();
				}
				continue;
			}

			TextPrivateFormField::ref textPrivate = boost::dynamic_pointer_cast<TextPrivateFormField>(*it);
			if (textPrivate) {
				if (textPrivate->getName() == "password") {
					payload->setPassword(textPrivate->getValue());
				}
				else if (textPrivate->getName() == "local_password") {
					local_password = textPrivate->getValue();
				}
				continue;
			}

			ListSingleFormField::ref listSingle = boost::dynamic_pointer_cast<ListSingleFormField>(*it);
			if (listSingle) {
				if (listSingle->getName() == "language") {
					language = listSingle->getValue();
				}
				continue;
			}

			BooleanFormField::ref boolean = boost::dynamic_pointer_cast<BooleanFormField>(*it);
			if (boolean) {
				if (boolean->getName() == "unregister") {
					if (boolean->getValue()) {
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
		} else if (local_username != "heinz" || local_password != "heinz") {
			// TODO: Check local password and username
			sendError(from, id, ErrorPayload::NotAuthorized, ErrorPayload::Modify);
			return true;
		}
	}

	printf("here\n");

	if (!payload->getUsername() || !payload->getPassword()) {
		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
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

//TODO: Part of spectrum1 registration stuff, this should be potentially rewritten for S2 too
// #if GLIB_CHECK_VERSION(2,14,0)
// 	if (!CONFIG_STRING(m_config, "registration.reg_allowed_usernames").empty() &&
// 		!g_regex_match_simple(CONFIG_STRING(m_config, "registration.reg_allowed_usernames"), newUsername.c_str(),(GRegexCompileFlags) (G_REGEX_CASELESS | G_REGEX_EXTENDED), (GRegexMatchFlags) 0)) {
// 		Log("UserRegistration", "This is not valid username: "<< newUsername);
// 		sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
// 		return true;
// 	}
// #endif
	if (!registered) {
		res.jid = barejid;
		res.uin = username;
		res.password = *payload->getPassword();
		res.language = language;
		res.encoding = encoding;
		res.vip = 0;
		registerUser(res);
	}
	else {
		res.jid = barejid;
		res.uin = username;
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
