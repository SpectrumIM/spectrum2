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

namespace Transport {

class Conversation;
class User;
class Component;
class DiscoItemsResponder;
class AdHocCommandFactory;
class AdHocCommand;

/// Listens for AdHoc commands and manages all AdHoc commands sessions
class AdHocManager : public Swift::Responder<Swift::Command> {
	public:
		typedef std::map<std::string, AdHocCommand *> CommandsMap;
		typedef std::map<Swift::JID, CommandsMap> SessionsMap;
		/// Creates new AdHocManager.

		/// \param component Transport instance associated with this AdHocManager.
		AdHocManager(Component *component, DiscoItemsResponder *discoItemsResponder);

		/// Destructor.
		virtual ~AdHocManager();

		/// Starts handling AdHoc commands payloads.
		void start();

		/// Stops handling AdHoc commands payloads and destroys all existing
		/// AdHoc commands sessions.
		void stop();

		/// Adds factory to create new AdHoc commands sessions of particular type.
		void addAdHocCommand(AdHocCommandFactory *factory);

		/// Remove sessions older than N seconds.
		void removeOldSessions();


	private:
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::Command> payload);
		virtual bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::Command> payload);

		Component *m_component;
		DiscoItemsResponder *m_discoItemsResponder;
		std::map<std::string, AdHocCommandFactory *> m_factories;
		SessionsMap m_sessions;
		Swift::Timer::ref m_collectTimer;
};

}
