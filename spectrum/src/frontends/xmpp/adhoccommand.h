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
#include <time.h>

#include "Swiften/Elements/FormField.h"
#include "Swiften/Elements/Command.h"
#include "Swiften/SwiftenCompat.h"

namespace Transport {

class Component;
class UserManager;
class StorageBackend;

class AdHocCommand {
	public:
		/// Creates new AdHocManager.

		/// \param component Transport instance associated with this AdHocManager.
		AdHocCommand(Component *component, UserManager *userManager, StorageBackend *storageBackend, const Swift::JID &initiator, const Swift::JID &to);

		/// Destructor.
		virtual ~AdHocCommand();

		virtual SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> handleRequest(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> payload) = 0;

		const std::string &getId() {
			return m_id;
		}

		void refreshLastActivity() {
			m_lastActivity = time(NULL);
		}

		time_t getLastActivity() {
			return m_lastActivity;
		}

	protected:
		Component *m_component;
		UserManager *m_userManager;
		StorageBackend *m_storageBackend;
		Swift::JID m_initiator;
		Swift::JID m_to;
		std::string m_id;

	private:
		// This is used to remove AdHocCommand after long inactivity to prevent memory leaks
		// caused by users which disconnect before they finish the command.
		// AdHocManager uses this to garbage collect old AdHocCommands.
		time_t m_lastActivity;
};

}
