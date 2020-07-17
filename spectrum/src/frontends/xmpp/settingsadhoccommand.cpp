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

#include "settingsadhoccommand.h"
#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/Config.h"
#include "formutils.h"


namespace Transport {

DEFINE_LOGGER(settingsAdHocCommandLogger, "SettingsAdHocCommand");

SettingsAdHocCommand::SettingsAdHocCommand(Component *component, UserManager *userManager, StorageBackend *storageBackend, const Swift::JID &initiator, const Swift::JID &to) : AdHocCommand(component, userManager, storageBackend, initiator, to) {
	m_state = Init;
}

SettingsAdHocCommand::~SettingsAdHocCommand() {
}

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> SettingsAdHocCommand::getForm() {
	if (!m_storageBackend) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Form> form(new Swift::Form());
		FormUtils::addTextFixedField(form, "This server does not support transport settings. There is no storage backend configured");
		response->setForm(form);
		return response;
	}

	UserInfo user;
	if (m_storageBackend->getUser(m_initiator.toBare().toString(), user) == false) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Form> form(new Swift::Form());
		FormUtils::addTextFixedField(form, "You are not registered.");
		response->setForm(form);
		return response;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Executing));
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Form> form(new Swift::Form());

	std::string value;
	int type = (int) TYPE_BOOLEAN;

	value = "1";
	m_storageBackend->getUserSetting(user.id, "enable_transport", type, value);
	FormUtils::addBooleanField(form, "enable_transport", value, "Enable transport");

	value = CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "settings.send_headlines", false) ? "1" : "0";
	m_storageBackend->getUserSetting(user.id, "send_headlines", type, value);
	FormUtils::addBooleanField(form, "send_headlines", value, "Allow sending messages as headlines");

	value = CONFIG_BOOL_DEFAULTED(m_component->getConfig(), "settings.stay_connected", false) ? "1" : "0";
	m_storageBackend->getUserSetting(user.id, "stay_connected", type, value);
	FormUtils::addBooleanField(form, "stay_connected", value, "Stay connected to legacy network when offline on XMPP");

	response->setForm(form);
	return response;
}

void SettingsAdHocCommand::updateUserSetting(Swift::Form::ref form, UserInfo &user, const std::string &name) {
	std::string value = FormUtils::fieldValue(form, name, "");
	if (value.empty()) {
		return;
	}

	m_storageBackend->updateUserSetting(user.id, name, value);
}

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> SettingsAdHocCommand::handleResponse(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> payload) {
	UserInfo user;
	bool registered = m_storageBackend->getUser(m_initiator.toBare().toString(), user);

	if (registered && payload->getForm()) {
		updateUserSetting(payload->getForm(), user, "enable_transport");
		updateUserSetting(payload->getForm(), user, "send_headlines");
		updateUserSetting(payload->getForm(), user, "stay_connected");
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Completed));
	return response;
}

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> SettingsAdHocCommand::handleRequest(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> payload) {
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response;
	if (payload->getAction() == Swift::Command::Cancel) {
		response = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command>(new Swift::Command("settings", m_id, Swift::Command::Canceled));
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
