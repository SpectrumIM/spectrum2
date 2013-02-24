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

#define GET_RESPONSE_DATA(RESP, DATA) ((RESP.find(std::string(DATA) + " ") != std::string::npos) ? RESP.substr(RESP.find(DATA) + strlen(DATA) + 1) : "");
#define GET_PROPERTY(VAR, OBJ, WHICH, PROP) std::string VAR = send_command(std::string("GET ") + OBJ + " " + WHICH + " " + PROP); \
					try {\
						VAR = GET_RESPONSE_DATA(VAR, PROP);\
					}\
					catch (std::out_of_range& oor) {\
						VAR="";\
					}

class SkypePlugin;

class Skype {
	public:
		Skype(SkypePlugin *np, const std::string &user, const std::string &username, const std::string &password);

		virtual ~Skype() {
			logout();
		}

		void login();

		void logout();

		std::string send_command(const std::string &message);

		const std::string &getUser() {
			return m_user;
		}

		const std::string &getUsername() {
			return m_username;
		}

		int getPid() {
			return (int) m_pid;
		}

	public: // but do not use them, should be used only internally
		bool createDBusProxy();

		bool loadSkypeBuddies();

		void handleSkypeMessage(std::string &message);

		DBusHandlerResult dbusMessageReceived(DBusConnection *connection, DBusMessage *message);

	private:
		std::string createSkypeDirectory();
		bool spawnSkype(const std::string &db_path);

		std::string m_username;
		std::string m_password;
		GPid m_pid;
		DBusGConnection *m_connection;
		DBusGProxy *m_proxy;
		std::string m_user;
		int m_timer;
		int m_counter;
		int fd_output;
		std::map<std::string, std::string> m_groups;
		SkypePlugin *m_np;
		std::string m_db;
};

