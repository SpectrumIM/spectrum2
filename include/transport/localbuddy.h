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

#ifndef SPECTRUM_BUDDY_H
#define SPECTRUM_BUDDY_H

#include <string>
#include <algorithm>
#include "transport/buddy.h"
#include "transport/rostermanager.h"

namespace Transport {

class LocalBuddy : public Buddy {
	public:
		LocalBuddy(RosterManager *rosterManager, long id);
		virtual ~LocalBuddy();

		std::string getAlias() { return m_alias; }
		void setAlias(const std::string &alias);

		std::string getName() { return m_name; }
		void setName(const std::string &name) { m_name = name; }

		bool getStatus(Swift::StatusShow &status, std::string &statusMessage) {
			status = m_status;
			statusMessage = m_statusMessage;
			return true;
		}

		void setStatus(const Swift::StatusShow &status, const std::string &statusMessage) {
			m_status = status;
			m_statusMessage = statusMessage;
		}

		std::string getIconHash() { return m_iconHash; }
		void setIconHash(const std::string &iconHash) { m_iconHash = iconHash; }

		std::vector<std::string> getGroups() { return m_groups; }
		void setGroups(const std::vector<std::string> &groups) { m_groups = groups; }

	private:
		std::string m_name;
		std::string m_alias;
		std::vector<std::string> m_groups;
		std::string m_statusMessage;
		std::string m_iconHash;
		Swift::StatusShow m_status;
};

}

#endif
