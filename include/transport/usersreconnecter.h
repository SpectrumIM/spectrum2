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
#include <vector>

#include "Swiften/Network/Timer.h"

namespace Transport {

class StorageBackend;
class Component;

/// Tries to reconnect users who have been online before crash/restart.
class UsersReconnecter {
	public:
		/// Creates new UsersReconnecter.
		/// \param component Transport instance associated with this roster.
		/// \param storageBackend StorageBackend from which the users will be fetched.
		UsersReconnecter(Component *component, StorageBackend *storageBackend);

		/// Destructor.
		virtual ~UsersReconnecter();

		void reconnectNextUser();

	private:
		void handleConnected();

		Component *m_component;
		StorageBackend *m_storageBackend;
		bool m_started;
		std::vector<std::string> m_users;
		Swift::Timer::ref m_nextUserTimer;
};

}
