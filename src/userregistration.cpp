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
#include "transport/user.h"
#include "Swiften/Elements/ErrorPayload.h"
#include <boost/shared_ptr.hpp>

using namespace Swift;

namespace Transport {

UserRegistration::UserRegistration(Component *component, UserManager *userManager, StorageBackend *storageBackend) : Swift::GetResponder<Swift::InBandRegistrationPayload>(component->m_component->getIQRouter()), Swift::SetResponder<Swift::InBandRegistrationPayload>(component->m_component->getIQRouter()) {
	m_component = component;
	m_config = m_component->m_config;
	m_storageBackend = storageBackend;
	m_userManager = userManager;
	Swift::GetResponder<Swift::InBandRegistrationPayload>::start();
	Swift::SetResponder<Swift::InBandRegistrationPayload>::start();
}

UserRegistration::~UserRegistration(){
}

bool UserRegistration::registerUser(const UserInfo &row) {
	// TODO: move this check to sql()->addUser(...) and let it return bool
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

	m_component->m_component->sendPresence(response);

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

	Swift::Presence::ref response;

	User *user = m_userManager->getUser(barejid);

	// roster contains already escaped jids
	std::list <std::string> roster;
	m_storageBackend->getBuddies(userInfo.id, roster);

	for(std::list<std::string>::iterator u = roster.begin(); u != roster.end() ; u++){
		std::string name = *u;

		response = Swift::Presence::create();
		response->setTo(Swift::JID(barejid));
		response->setFrom(Swift::JID(name + "@" + m_component->getJID().toString()));
		response->setType(Swift::Presence::Unsubscribe);
		m_component->m_component->sendPresence(response);

		response = Swift::Presence::create();
		response->setTo(Swift::JID(barejid));
		response->setFrom(Swift::JID(name + "@" + m_component->getJID().toString()));
		response->setType(Swift::Presence::Unsubscribed);
		m_component->m_component->sendPresence(response);
	}

	// Remove user from database
	m_storageBackend->removeUser(userInfo.id);

	// Disconnect the user
	if (user) {
		m_userManager->removeUser(user);
	}

	response = Swift::Presence::create();
	response->setTo(Swift::JID(barejid));
	response->setFrom(m_component->getJID());
	response->setType(Swift::Presence::Unsubscribe);
	m_component->m_component->sendPresence(response);

	response = Swift::Presence::create();
	response->setTo(Swift::JID(barejid));
	response->setFrom(m_component->getJID());
	response->setType(Swift::Presence::Unsubscribed);
	m_component->m_component->sendPresence(response);

	return true;
}

bool UserRegistration::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		Swift::GetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

// 	User *user = m_userManager->getUserByJID(barejid);
	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::list<std::string> const &x = CONFIG_LIST(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
// 			Log("UserRegistration", "This user has no permissions to register an account");
			Swift::GetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

// 	const char *_language = user ? user->getLang() : CONFIG_STRING(m_config, "registration.language").c_str();

	boost::shared_ptr<InBandRegistrationPayload> reg(new InBandRegistrationPayload());

	UserInfo res;
	bool registered = m_storageBackend->getUser(barejid, res);

	std::string instructions = CONFIG_STRING(m_config, "registration.instructions");

	reg->setInstructions(instructions);
	reg->setRegistered(registered);
	reg->setUsername(res.uin);
	if (CONFIG_STRING(m_config, "service.protocol") != "twitter" && CONFIG_STRING(m_config, "service.protocol") != "bonjour")
		reg->setPassword(res.password);

	std::string usernameField = CONFIG_STRING(m_config, "registration.username_field");

	Form::ref form(new Form(Form::FormType));
	form->setTitle(tr(_language, _("Registration")));
	form->setInstructions(tr(_language, instructions));

	HiddenFormField::ref type = HiddenFormField::create();
	type->setName("FORM_TYPE");
	type->setValue("jabber:iq:register");
	form->addField(type);

	TextSingleFormField::ref username = TextSingleFormField::create();
	username->setName("username");
	username->setLabel(tr(_language, usernameField));
	username->setValue(res.uin);
	username->setRequired(true);
	form->addField(username);

	if (CONFIG_STRING(m_config, "service.protocol") != "twitter" && CONFIG_STRING(m_config, "service.protocol") != "bonjour") {
		TextPrivateFormField::ref password = TextPrivateFormField::create();
		password->setName("password");
		password->setLabel(tr(_language, _("Password")));
		password->setRequired(true);
		form->addField(password);
	}

	ListSingleFormField::ref language = ListSingleFormField::create();
	language->setName("language");
	language->setLabel(tr(_language, _("Language")));
	if (registered)
		language->setValue(res.language);
	else
		language->setValue(CONFIG_STRING(m_config, "registration.language"));
// 	std::map <std::string, std::string> languages = localization.getLanguages();
// 	for (std::map <std::string, std::string>::iterator it = languages.begin(); it != languages.end(); it++) {
// 		language->addOption(FormField::Option((*it).second, (*it).first));
// 	}
	form->addField(language);

	TextSingleFormField::ref encoding = TextSingleFormField::create();
	encoding->setName("encoding");
	encoding->setLabel(tr(_language, _("Encoding")));
	if (registered)
		encoding->setValue(res.encoding);
	else
		encoding->setValue(CONFIG_STRING(m_config, "registration.encoding"));
	form->addField(encoding);

	if (registered) {
		BooleanFormField::ref boolean = BooleanFormField::create();
		boolean->setName("unregister");
		boolean->setLabel(tr(_language, _("Remove your registration")));
		boolean->setValue(0);
		form->addField(boolean);
	}

	reg->setForm(form);

	Swift::GetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, reg);

	return true;
}

bool UserRegistration::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	if (CONFIG_STRING(m_config, "service.protocol") == "irc") {
		Swift::GetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString();

// 	AbstractUser *user = m_component->userManager()->getUserByJID(barejid);
	if (!CONFIG_BOOL(m_config,"registration.enable_public_registration")) {
		std::list<std::string> const &x = CONFIG_LIST(m_config,"service.allowed_servers");
		if (std::find(x.begin(), x.end(), from.getDomain()) == x.end()) {
// 			Log("UserRegistration", "This user has no permissions to register an account");
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	UserInfo res;
	bool registered = m_storageBackend->getUser(barejid, res);

	std::string encoding;
	std::string language;

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
				continue;
			}

			TextPrivateFormField::ref textPrivate = boost::dynamic_pointer_cast<TextPrivateFormField>(*it);
			if (textPrivate) {
				if (textPrivate->getName() == "password") {
					payload->setPassword(textPrivate->getValue());
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
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, InBandRegistrationPayload::ref());
		return true;
	}

	if (!payload->getUsername() || !payload->getPassword()) {
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	// Register or change password
	if (payload->getUsername()->empty() ||
		(payload->getPassword()->empty() && CONFIG_STRING(m_config, "service.protocol") != "twitter" && CONFIG_STRING(m_config, "service.protocol") != "bonjour")
// 		|| localization.getLanguages().find(language) == localization.getLanguages().end()
	)
	{
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	if (CONFIG_STRING(m_config, "service.protocol") == "xmpp") {
		// User tries to register himself.
		if ((Swift::JID(*payload->getUsername()).toBare() == from.toBare())) {
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}

		// User tries to register someone who's already registered.
		UserInfo user_row;
		bool registered = m_storageBackend->getUser(Swift::JID(*payload->getUsername()).toBare().toString(), user_row);
		if (registered) {
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}
	}

	std::string username = *payload->getUsername();
// 	m_component->protocol()->prepareUsername(username);

	std::string newUsername(username);
	if (!CONFIG_STRING(m_config, "registration.username_mask").empty()) {
		newUsername = CONFIG_STRING(m_config, "registration.username_mask");
// 		replace(newUsername, "$username", username.c_str());
	}

// 	if (!m_component->protocol()->isValidUsername(newUsername)) {
// 		Log("UserRegistration", "This is not valid username: "<< newUsername);
// 		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
// 		return true;
// 	}

// #if GLIB_CHECK_VERSION(2,14,0)
// 	if (!CONFIG_STRING(m_config, "registration.reg_allowed_usernames").empty() &&
// 		!g_regex_match_simple(CONFIG_STRING(m_config, "registration.reg_allowed_usernames"), newUsername.c_str(),(GRegexCompileFlags) (G_REGEX_CASELESS | G_REGEX_EXTENDED), (GRegexMatchFlags) 0)) {
// 		Log("UserRegistration", "This is not valid username: "<< newUsername);
// 		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
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
		// change passwordhttp://soumar.jabbim.cz/phpmyadmin/index.php
// 		Log("UserRegistration", "changing user password: "<< barejid << ", " << username);
		res.jid = barejid;
		res.password = *payload->getPassword();
		res.language = language;
		res.encoding = encoding;
		m_storageBackend->setUser(res);
		onUserUpdated(res);
	}

	Swift::SetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, InBandRegistrationPayload::ref());
	return true;
}

}
