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

#pragma once

#include "glib.h"
#include <dbus-1.0/dbus/dbus-glib-lowlevel.h>
#include "sqlite3.h"
#include <iostream>
#include <map>
#include "transport/networkplugin.h"
#include "transport/config.h"

class Skype;

class SkypePlugin : public Transport::NetworkPlugin {
	public:
		SkypePlugin(Transport::Config *config, const std::string &host, int port);

		virtual ~SkypePlugin();

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password);

		void handleMemoryUsage(double &res, double &shared);

		void handleLogoutRequest(const std::string &user, const std::string &legacyName);

		void handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage);

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups);

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups);

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "");

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id);

		void sendData(const std::string &string);

		void handleVCardUpdatedRequest(const std::string &user, const std::string &p, const std::string &nickname);

		void handleBuddyBlockToggled(const std::string &user, const std::string &buddyName, bool blocked);

		void handleTypingRequest(const std::string &user, const std::string &buddyName);

		void handleTypedRequest(const std::string &user, const std::string &buddyName);

		void handleStoppedTypingRequest(const std::string &user, const std::string &buddyName);

		void handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message);

		std::map<std::string, Skype *> m_sessions;
		std::map<Skype *, std::string> m_accounts;
		std::map<std::string, unsigned int> m_vcards;
		Transport::Config *config;
		
};
