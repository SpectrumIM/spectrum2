/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2012, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "transport/settingsadhoccommand.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/buddy.h"
#include "transport/factory.h"
#include "transport/user.h"
#include "transport/logging.h"
#include "transport/storagebackend.h"


namespace Transport {

DEFINE_LOGGER(logger, "SettingsAdHocCommand");

SettingsAdHocCommand::SettingsAdHocCommand(Component *component, UserManager *userManager, StorageBackend *storageBackend, const Swift::JID &initiator, const Swift::JID &to) : AdHocCommand(component, userManager, storageBackend, initiator, to) {
	m_state = Init;
#if HAVE_SWIFTEN_3
	Swift::FormField::ref field = boost::make_shared<Swift::FormField>(Swift::FormField::BooleanType, "1");
#else
	Swift::BooleanFormField::ref field;

	field = Swift::BooleanFormField::create(true);
#endif
	field->setName("enable_transport");
	field->setLabel("Enable transport");
	addFormField(field);
#if HAVE_SWIFTEN_3
	field = boost::make_shared<Swift::FormField>(Swift::FormField::BooleanType, CONFIG_STRING_DEFAULTED(component->getConfig(), "settings.send_headlines", "0"));
#else
	field = Swift::BooleanFormField::create(CONFIG_STRING_DEFAULTED(component->getConfig(), "settings.send_headlines", "0") == "1");
#endif
	field->setName("send_headlines");
	field->setLabel("Allow sending messages as headlines");
	addFormField(field);
#if HAVE_SWIFTEN_3
	field = boost::make_shared<Swift::FormField>(Swift::FormField::BooleanType, CONFIG_STRING_DEFAULTED(component->getConfig(), "settings.stay_connected", "0"));
#else
	field = Swift::BooleanFormField::create(CONFIG_STRING_DEFAULTED(component->getConfig(), "settings.stay_connected", "0") == "1");
#endif
	field->setName("stay_connected");
	field->setLabel("Stay connected to legacy network when offline on XMPP");
	addFormField(field);
}

SettingsAdHocCommand::~SettingsAdHocCommand() {
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::getForm() {
	if (!m_storageBackend) {
		boost::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
		boost::shared_ptr<Swift::Form> form(new Swift::Form());
#if HAVE_SWIFTEN_3
		form->addField(boost::make_shared<Swift::FormField>(Swift::FormField::FixedType, "This server does not support transport settings. There is no storage backend configured"));
#else
		form->addField(Swift::FixedFormField::create("This server does not support transport settings. There is no storage backend configured"));
#endif
		response->setForm(form);
		return response;
	}

	UserInfo user;
	if (m_storageBackend->getUser(m_initiator.toBare().toString(), user) == false) {
		boost::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
		boost::shared_ptr<Swift::Form> form(new Swift::Form());
#if HAVE_SWIFTEN_3
		form->addField(boost::make_shared<Swift::FormField>(Swift::FormField::FixedType, "You are not registered."));
#else
		form->addField(Swift::FixedFormField::create("You are not registered."));
#endif
		response->setForm(form);
		return response;
	}

	boost::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Executing));
	boost::shared_ptr<Swift::Form> form(new Swift::Form());

	BOOST_FOREACH(Swift::FormField::ref field, m_fields) {
		// FIXME: Support for more types than boolean
#if HAVE_SWIFTEN_3
		if (field->getType() == Swift::FormField::BooleanType) {
			std::string value = field->getBoolValue() ? "1" : "0";
			int type = (int) TYPE_BOOLEAN;
			m_storageBackend->getUserSetting(user.id, field->getName(), type, value);
			field->setBoolValue(value == "1");
		}
#else
		if (boost::dynamic_pointer_cast<Swift::BooleanFormField>(field)) {
			Swift::BooleanFormField::ref f(boost::dynamic_pointer_cast<Swift::BooleanFormField>(field));
			std::string value = f->getValue() ? "1" : "0";
			int type = (int)TYPE_BOOLEAN;
			m_storageBackend->getUserSetting(user.id, f->getName(), type, value);
			f->setValue(value == "1");
		}
#endif			

		form->addField(field);
	}

	response->setForm(form);
	return response;
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::handleResponse(boost::shared_ptr<Swift::Command> payload) {
	UserInfo user;
	bool registered = m_storageBackend->getUser(m_initiator.toBare().toString(), user);

	if (registered && payload->getForm()) {
		BOOST_FOREACH(Swift::FormField::ref field, m_fields) {
			Swift::FormField::ref received = payload->getForm()->getField(field->getName());
			if (!received) {
				continue;
			}
#if HAVE_SWIFTEN_3
			if (received->getType() == Swift::FormField::BooleanType) {
				std::string value = received->getBoolValue() ? "1" : "0";
				m_storageBackend->updateUserSetting(user.id, received->getName(), value);
			} else if (received->getType() == Swift::FormField::TextSingleType) {
				m_storageBackend->updateUserSetting(user.id, received->getName(), received->getTextSingleValue());
			}
#else
			// FIXME: Support for more types than boolean
			if (boost::dynamic_pointer_cast<Swift::BooleanFormField>(received)) {
				Swift::BooleanFormField::ref f(boost::dynamic_pointer_cast<Swift::BooleanFormField>(received));
				std::string value = f->getValue() ? "1" : "0";
				m_storageBackend->updateUserSetting(user.id, f->getName(), value);
			}
			else if (boost::dynamic_pointer_cast<Swift::TextSingleFormField>(received)) {
				Swift::TextSingleFormField::ref f(boost::dynamic_pointer_cast<Swift::TextSingleFormField>(received));
				m_storageBackend->updateUserSetting(user.id, f->getName(), f->getValue());
			}
#endif
		}
	}

	boost::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
	return response;
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::handleRequest(boost::shared_ptr<Swift::Command> payload) {
	boost::shared_ptr<Swift::Command> response;
	if (payload->getAction() == Swift::Command::Cancel) {
		response = boost::shared_ptr<Swift::Command>(new Swift::Command("settings", m_id, Swift::Command::Canceled));
		return response;
	}

	switch (m_state) {
		case Init:
			response = getForm();
			m_state = WaitingForResponse;
			break;
		case WaitingForResponse:
			response = handleResponse(payload);
			break;
		default:
			break;
	}
	
	return response;
}

}
