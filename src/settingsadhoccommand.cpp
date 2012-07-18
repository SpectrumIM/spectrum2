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

namespace Transport {

DEFINE_LOGGER(logger, "SettingsAdHocCommand");

SettingsAdHocCommand::SettingsAdHocCommand(Component *component, const Swift::JID &initiator, const Swift::JID &to) : AdHocCommand(component, initiator, to) {
	m_state = Init;
}

SettingsAdHocCommand::~SettingsAdHocCommand() {
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::getForm() {
	boost::shared_ptr<Swift::Command> response(new Swift::Command("settings", m_id, Swift::Command::Executing));
	boost::shared_ptr<Swift::Form> form(new Swift::Form());

	BOOST_FOREACH(Swift::FormField::ref field, m_fields) {
		form->addField(field);
	}

	response->setForm(form);
	return response;
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::handleResponse(boost::shared_ptr<Swift::Command> payload) {
	
	

	boost::shared_ptr<Swift::Command> response;
	response->setStatus(Swift::Command::Completed);
	return response;
}

boost::shared_ptr<Swift::Command> SettingsAdHocCommand::handleRequest(boost::shared_ptr<Swift::Command> payload) {
	boost::shared_ptr<Swift::Command> response;

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
