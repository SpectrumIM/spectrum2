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

#pragma once

#include <string>
#include <algorithm>
#include <map>
#include "transport/adhoccommand.h"
#include "transport/adhoccommandfactory.h"
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)


namespace Transport {

class Component;
class UserManager;
class StorageBackend;
class UserInfo;

class SettingsAdHocCommand : public AdHocCommand {
	public:
		typedef enum { Init, WaitingForResponse } State;

		SettingsAdHocCommand(Component *component, UserManager *userManager, StorageBackend *storageBackend, const Swift::JID &initiator, const Swift::JID &to);

		/// Destructor.
		virtual ~SettingsAdHocCommand();

		virtual boost::shared_ptr<Swift::Command> handleRequest(boost::shared_ptr<Swift::Command> payload);

	private:
		void updateUserSetting(Swift::Form::ref form, UserInfo &user, const std::string &name);

		boost::shared_ptr<Swift::Command> getForm();
		boost::shared_ptr<Swift::Command> handleResponse(boost::shared_ptr<Swift::Command> payload);
		State m_state;
};

class SettingsAdHocCommandFactory : public AdHocCommandFactory {
	public:
		SettingsAdHocCommandFactory() {
			m_userSettings["send_headlines"] = "0";
			m_userSettings["stay_connected"] = "0";
		}

		virtual ~SettingsAdHocCommandFactory() {}

		AdHocCommand *createAdHocCommand(Component *component, UserManager *userManager, StorageBackend *storageBackend, const Swift::JID &initiator, const Swift::JID &to) {
			return new SettingsAdHocCommand(component, userManager, storageBackend, initiator, to);
		}

		std::string getNode() {
			return "settings";
		}

		std::string getName() {
			return "Transport settings";
		}
};

}
