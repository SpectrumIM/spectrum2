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
#include "Swiften/Swiften.h"
#include "transport/adhoccommand.h"
#include "transport/adhoccommandfactory.h"


namespace Transport {

class Component;

class SettingsAdHocCommand : public AdHocCommand {
	public:
		SettingsAdHocCommand(Component *component, const Swift::JID &initiator, const Swift::JID &to);

		/// Destructor.
		virtual ~SettingsAdHocCommand();

		virtual boost::shared_ptr<Swift::Command> handleRequest(boost::shared_ptr<Swift::Command> payload);
};

class SettingsAdHocCommandFactory : public AdHocCommandFactory {
	public:
		SettingsAdHocCommandFactory() {}
		virtual ~SettingsAdHocCommandFactory() {}

		AdHocCommand *createAdHocCommand(Component *component, const Swift::JID &initiator, const Swift::JID &to) {
			return new SettingsAdHocCommand(component, initiator, to);
		}

		std::string getNode() {
			return "settings";
		}

		std::string getName() {
			return "Transport settings";
		}
};

}
