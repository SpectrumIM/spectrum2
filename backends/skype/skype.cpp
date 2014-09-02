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

#include "skype.h"
#include "skypeplugin.h"
#include "skypedb.h"

#include "transport/config.h"
#include "transport/logging.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/memoryusage.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/user.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/conversation.h"
#include "transport/networkplugin.h"
#include <boost/filesystem.hpp>
#include "sys/wait.h"
#include "sys/signal.h"
// #include "valgrind/memcheck.h"
#ifndef __FreeBSD__
#include "malloc.h"
#endif


DEFINE_LOGGER(logger, "Skype");

Skype::Skype(SkypePlugin *np, const std::string &user, const std::string &username, const std::string &password) {
	m_username = username;
	m_user = user;
	m_password = password;
	m_pid = 0;
	m_connection = 0;
	m_proxy = 0;
	m_timer = -1;
	m_counter = 0;
	m_np = np;
}

static gboolean skype_check_missedmessages(gpointer data) {
	Skype *skype = (Skype *) data;

	skype->send_command("SEARCH MISSEDCHATMESSAGES");

	return TRUE;
}

static gboolean load_skype_buddies(gpointer data) {
	Skype *skype = (Skype *) data;
	return skype->loadSkypeBuddies();
}

static gboolean create_dbus_proxy(gpointer data) {
	Skype *skype = (Skype *) data;
	return skype->createDBusProxy();
}

static pbnetwork::StatusType getStatus(const std::string &st) {
	pbnetwork::StatusType status = pbnetwork::STATUS_ONLINE;
	if (st == "SKYPEOUT" || st == "OFFLINE") {
		status = pbnetwork::STATUS_NONE;
	}
	else if (st == "DND") {
		status = pbnetwork::STATUS_DND;
	}
	else if (st == "NA") {
		status = pbnetwork::STATUS_XA;
	}
	else if (st == "AWAY") {
		status = pbnetwork::STATUS_AWAY;
	}
	return status;
}

DBusHandlerResult skype_notify_handler(DBusConnection *connection, DBusMessage *message, gpointer data) {
	Skype *skype = (Skype *) data;
	return skype->dbusMessageReceived(connection, message);
}

void Skype::login() {
	// Do not allow usernames with unsecure symbols
	if (m_username.find("..") == 0 || m_username.find("/") != std::string::npos) {
		m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Invalid username");
		return;
	}

	m_db = createSkypeDirectory();

	bool spawned = spawnSkype(m_db);
	if (!spawned) {
		m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Error spawning the Skype instance.");
		return;
	}

	m_db += "/" + getUsername() + "/main.db";

	if (m_connection == NULL) {
		LOG4CXX_INFO(logger, "Creating DBUS connection.");
		GError *error = NULL;
		m_connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if (m_connection == NULL && error != NULL)
		{
			LOG4CXX_INFO(logger,  m_username << ": Creating DBUS Connection error: " << error->message);
			g_error_free(error);
			return;
		}
	}

	m_timer = g_timeout_add_seconds(1, create_dbus_proxy, this);
}

bool Skype::createDBusProxy() {
	if (m_proxy == NULL) {
		LOG4CXX_INFO(logger, "Creating DBus proxy for com.Skype.Api.");
		m_counter++;

		GError *error = NULL;
		m_proxy = dbus_g_proxy_new_for_name_owner(m_connection, "com.Skype.API", "/com/Skype", "com.Skype.API", &error);
		if (m_proxy == NULL && error != NULL) {
			LOG4CXX_INFO(logger,  m_username << ":" << error->message);

			if (m_counter == 15) {
				LOG4CXX_ERROR(logger, "Logging out, proxy couldn't be created: " << error->message);
				m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, error->message);
				logout();
				g_error_free(error);
				return FALSE;
			}
			g_error_free(error);
		}

		if (m_proxy) {
			LOG4CXX_INFO(logger, "Proxy created.");
			DBusObjectPathVTable vtable;
			vtable.message_function = &skype_notify_handler;
			dbus_connection_register_object_path(dbus_g_connection_get_connection(m_connection), "/com/Skype/Client", &vtable, this);

			m_counter = 0;
			m_timer = g_timeout_add_seconds(1, load_skype_buddies, this);
			g_timeout_add_seconds(10, skype_check_missedmessages, this);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

bool Skype::loadSkypeBuddies() {
//	std::string re = "CONNSTATUS OFFLINE";
//	while (re == "CONNSTATUS OFFLINE" || re.empty()) {
//		sleep(1);

	gchar buffer[1024];
	int bytes_read = read(fd_output, buffer, 1023);
	if (bytes_read > 0) {
		buffer[bytes_read] = 0;
		std::string b(buffer);
		LOG4CXX_WARN(logger, "Skype wrote this on stdout '" << b << "'");
		if (b.find("Incorrect Password") != std::string::npos) {
			LOG4CXX_INFO(logger, "Incorrect password, logging out")
			m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_FAILED, "Incorrect password");
			close(fd_output);
			logout();
			return FALSE;
		}
	}

	std::string re = send_command("NAME Spectrum");
	if (m_counter++ > 15) {
		LOG4CXX_ERROR(logger, "Logging out, because we tried to connect the Skype over DBUS 15 times without success");
		m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Skype is not ready. This issue have been logged and admins will check it and try to fix it soon.");
		close(fd_output);
		logout();
		return FALSE;
	}

	if (re.empty() || re == "CONNSTATUS OFFLINE" || re == "ERROR 68") {
		return TRUE;
	}

	close(fd_output);

	if (send_command("PROTOCOL 7") != "PROTOCOL 7") {
		LOG4CXX_ERROR(logger, "PROTOCOL 7 failed, logging out");
		m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Skype is not ready. This issue have been logged and admins will check it and try to fix it soon.");
		logout();
		return FALSE;
	}

	m_np->handleConnected(m_user);

	std::map<std::string, std::string> group_map;
	std::string groups = send_command("SEARCH GROUPS CUSTOM");
	if (groups.find(' ') != std::string::npos) {
		groups = groups.substr(groups.find(' ') + 1);
		std::vector<std::string> grps;
		boost::split(grps, groups, boost::is_any_of(","));
		BOOST_FOREACH(std::string grp, grps) {
			std::vector<std::string> data;
			std::string name = send_command("GET GROUP " + grp + " DISPLAYNAME");

			if (name.find("ERROR") == 0) {
				continue;
			}

			boost::split(data, name, boost::is_any_of(" "));
			name = GET_RESPONSE_DATA(name, "DISPLAYNAME");

			std::string users = send_command("GET GROUP " + data[1] + " USERS");
			try {
				users = GET_RESPONSE_DATA(users, "USERS");
			}
			catch (std::out_of_range& oor) {
				continue;
			}
			boost::split(data, users, boost::is_any_of(","));
			BOOST_FOREACH(std::string u, data) {
				group_map[u] = grp;
			}
		}
	}

	// Try to load skype buddies from database, if it fails
	// fallback to old method.
	if (!SkypeDB::loadBuddies(m_np, m_db, m_user, group_map)) {
		std::string friends = send_command("GET AUTH_CONTACTS_PROFILES");

		char **full_friends_list = g_strsplit((strchr(friends.c_str(), ' ')+1), ";", 0);
		if (full_friends_list && full_friends_list[0])
		{
			//in the format of: username;full name;phone;office phone;mobile phone;
			//                  online status;friendly name;voicemail;mood
			// (comma-seperated lines, usernames can have comma's)

			for (int i=0; full_friends_list[i] && full_friends_list[i+1] && *full_friends_list[i] != '\0'; i+=8)
			{
				std::string buddy = full_friends_list[i];

				if (buddy[0] == ',') {
					buddy.erase(buddy.begin());
				}

				if (buddy.rfind(",") != std::string::npos) {
					buddy = buddy.substr(buddy.rfind(","));
				}

				if (buddy[0] == ',') {
					buddy.erase(buddy.begin());
				}

				LOG4CXX_INFO(logger, "Got buddy " << buddy);
				std::string st = full_friends_list[i + 5];

				pbnetwork::StatusType status = getStatus(st);

				std::string alias = full_friends_list[i + 6];

				std::string mood_text = "";
				if (full_friends_list[i + 8] && *full_friends_list[i + 8] != '\0' && *full_friends_list[i + 8] != ',') {
					mood_text = full_friends_list[i + 8];
				}

				std::vector<std::string> groups;
				if (group_map.find(buddy) != group_map.end()) {
					groups.push_back(group_map[buddy]);
				}
				m_np->handleBuddyChanged(m_user, buddy, alias, groups, status, mood_text);
			}
		}
		g_strfreev(full_friends_list);
	}

	send_command("SET AUTOAWAY OFF");
	send_command("SET USERSTATUS ONLINE");
	return FALSE;
}

void Skype::logout() {
	if (m_pid != 0) {
		if (m_proxy) {
			send_command("SET USERSTATUS INVISIBLE");
			send_command("SET USERSTATUS OFFLINE");
			sleep(2);
			g_object_unref(m_proxy);
		}
		LOG4CXX_INFO(logger,  m_username << ": Terminating Skype instance (SIGTERM)");
		kill((int) m_pid, SIGTERM);
		// Give skype a chance
		sleep(2);
		LOG4CXX_INFO(logger,  m_username << ": Killing Skype instance (SIGKILL)");
		kill((int) m_pid, SIGKILL);
		m_pid = 0;
	}
}

std::string Skype::send_command(const std::string &message) {
	GError *error = NULL;
	gchar *str = NULL;
// 			int message_num;
// 			gchar error_return[30];

	LOG4CXX_INFO(logger, "Sending: '" << message << "'");
	if (!dbus_g_proxy_call (m_proxy, "Invoke", &error, G_TYPE_STRING, message.c_str(), G_TYPE_INVALID,
						G_TYPE_STRING, &str, G_TYPE_INVALID))
	{
			if (error && error->message)
			{
			LOG4CXX_INFO(logger,  m_username << ": DBUS Error: " << error->message);
			g_error_free(error);
			return "";
		} else {
			LOG4CXX_INFO(logger,  m_username << ": DBUS no response");
			return "";
		}

	}
	if (str != NULL)
	{
		LOG4CXX_INFO(logger,  m_username << ": DBUS:'" << str << "'");
	}
	return str ? std::string(str) : std::string();
}

void Skype::handleSkypeMessage(std::string &message) {
	std::vector<std::string> cmd;
	boost::split(cmd, message, boost::is_any_of(" "));

	if (cmd[0] == "USER") {
		if (cmd[1] == getUsername()) {
			return;
		}

		if (cmd[2] == "ONLINESTATUS") {
			if (cmd[3] == "SKYPEOUT" || cmd[3] == "UNKNOWN") {
				return;
			}
			else {
				pbnetwork::StatusType status = getStatus(cmd[3]);
				GET_PROPERTY(mood_text, "USER", cmd[1], "MOOD_TEXT");
				GET_PROPERTY(alias, "USER", cmd[1], "FULLNAME");

				std::vector<std::string> groups;
				m_np->handleBuddyChanged(getUser(), cmd[1], alias, groups, status, mood_text);
			}
		}
		else if (cmd[2] == "MOOD_TEXT") {
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::string mood_text = GET_RESPONSE_DATA(message, "MOOD_TEXT");

			std::vector<std::string> groups;
			m_np->handleBuddyChanged(getUser(), cmd[1], "", groups, status, mood_text);
		}
		else if (cmd[2] == "BUDDYSTATUS" && cmd[3] == "3") {
			GET_PROPERTY(mood_text, "USER", cmd[1], "MOOD_TEXT");
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::vector<std::string> groups;
			m_np->handleBuddyChanged(getUser(), cmd[1], "", groups, status, mood_text);
		}
		else if (cmd[2] == "FULLNAME") {
			GET_PROPERTY(alias, "USER", cmd[1], "FULLNAME");
			GET_PROPERTY(mood_text, "USER", cmd[1], "MOOD_TEXT");
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::vector<std::string> groups;
			m_np->handleBuddyChanged(getUser(), cmd[1], alias, groups, status, mood_text);
		}
		else if(cmd[2] == "RECEIVEDAUTHREQUEST") {
			m_np->handleAuthorization(getUser(), cmd[1]);
		}
	}
	else if (cmd[0] == "GROUP") {
// 		if (cmd[2] == "DISPLAYNAME") {
// 			//GROUP 810 DISPLAYNAME My Friends
// 			std::string grp = GET_RESPONSE_DATA(message, "DISPLAYNAME");
// 			std::string users = send_command("GET GROUP " + cmd[1] + " USERS");
// 			try {
// 				users = GET_RESPONSE_DATA(users, "USERS");
// 			}
// 			catch (std::out_of_range& oor) {
// 				return;
// 			}
// 
// 			std::vector<std::string> data;
// 			boost::split(data, users, boost::is_any_of(","));
// 			BOOST_FOREACH(std::string u, data) {
// 				GET_PROPERTY(alias, "USER", u, "FULLNAME");
// 				GET_PROPERTY(mood_text, "USER", u, "MOOD_TEXT");
// 				GET_PROPERTY(st, "USER", u, "ONLINESTATUS");
// 				pbnetwork::StatusType status = getStatus(st);
// 
// 				std::vector<std::string> groups;
// 				groups.push_back(grp);
// 				m_np->handleBuddyChanged(getUser(), u, alias, groups, status, mood_text);
// 			}
// 		}
		if (cmd[2] == "NROFUSERS" && cmd[3] != "0") {
			GET_PROPERTY(grp, "GROUP", cmd[1], "DISPLAYNAME");
			std::string users = send_command("GET GROUP " + cmd[1] + " USERS");
			try {
				users = GET_RESPONSE_DATA(users, "USERS");
			}
			catch (std::out_of_range& oor) {
				return;
			}

			std::vector<std::string> data;
			boost::split(data, users, boost::is_any_of(","));
			BOOST_FOREACH(std::string u, data) {
				GET_PROPERTY(alias, "USER", u, "FULLNAME");
				GET_PROPERTY(mood_text, "USER", u, "MOOD_TEXT");
				GET_PROPERTY(st, "USER", u, "ONLINESTATUS");
				pbnetwork::StatusType status = getStatus(st);

				std::vector<std::string> groups;
				groups.push_back(grp);
				m_np->handleBuddyChanged(getUser(), u, alias, groups, status, mood_text);
			}
		}
	}
	else if ((cmd[0] == "MESSAGES") || (cmd[0] == "CHATMESSAGES")) {
		std::string msgs = GET_RESPONSE_DATA(message, "CHATMESSAGES");
		std::vector<std::string> data;
		boost::split(data, msgs, boost::is_any_of(","));
		BOOST_FOREACH(std::string str, data) {
			boost::trim(str);
			if (!str.empty()) {
			    std::string re = send_command("GET CHATMESSAGE " + str + " STATUS");
			    handleSkypeMessage(re);
			}
		}
	}
	else if (cmd[0] == "CHATMESSAGE") {
		if (cmd[3] == "RECEIVED") {
			GET_PROPERTY(body, "CHATMESSAGE", cmd[1], "BODY");
			GET_PROPERTY(from_handle, "CHATMESSAGE", cmd[1], "FROM_HANDLE");

			if (from_handle == getUsername()) {
				send_command("SET CHATMESSAGE " + cmd[1] + " SEEN");
				return;
			}

			m_np->handleMessage(getUser(), from_handle, body);

			send_command("SET CHATMESSAGE " + cmd[1] + " SEEN");
		}
	}
	else if (cmd[0] == "CALL") {
		// CALL 884 STATUS RINGING
		if (cmd[2] == "STATUS") {
			if (cmd[3] == "RINGING" || cmd[3] == "MISSED") {
				// handle only incoming calls
				GET_PROPERTY(type, "CALL", cmd[1], "TYPE");
				if (type.find("INCOMING") != 0) {
					return;
				}

				GET_PROPERTY(from, "CALL", cmd[1], "PARTNER_HANDLE");
				GET_PROPERTY(dispname, "CALL", cmd[1], "PARTNER_DISPNAME");

				if (cmd[3] == "RINGING") {
					m_np->handleMessage(getUser(), from, "User " + dispname + " is calling you.");
				}
				else {
					m_np->handleMessage(getUser(), from, "You have missed call from user " + dispname + ".");
				}
			}
		}
	}
}

DBusHandlerResult Skype::dbusMessageReceived(DBusConnection *connection, DBusMessage *message) {
	DBusMessageIter iterator;
	gchar *message_temp;
	DBusMessage *temp_message;
	
	temp_message = dbus_message_ref(message);
	dbus_message_iter_init(temp_message, &iterator);
	if (dbus_message_iter_get_arg_type(&iterator) != DBUS_TYPE_STRING)
	{
		dbus_message_unref(message);
		return (DBusHandlerResult) FALSE;
	}
	
	do {
		dbus_message_iter_get_basic(&iterator, &message_temp);
		std::string m(message_temp);
		LOG4CXX_INFO(logger,"DBUS message: " << m);
		handleSkypeMessage(m);
	} while(dbus_message_iter_has_next(&iterator) && dbus_message_iter_next(&iterator));
	
	dbus_message_unref(message);
	
	return DBUS_HANDLER_RESULT_HANDLED;
}

std::string Skype::createSkypeDirectory() {
	std::string tmpdir = std::string("/tmp/skype/") + m_username;

	// This should not be needed anymore...
	// boost::filesystem::remove_all(std::string("/tmp/skype/") + m_username);

	boost::filesystem::path	path(tmpdir);
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		boost::filesystem::path	path2(tmpdir + "/" + m_username );
		boost::filesystem::create_directories(path2);
	}

	std::string shared_xml = "<?xml version=\"1.0\"?>\n"
							"<config version=\"1.0\" serial=\"28\" timestamp=\"" + boost::lexical_cast<std::string>(time(NULL)) + ".0\">\n"
							"<UI>\n"
								"<Installed>2</Installed>\n"
								"<Language>en</Language>\n"
							"</UI>\n"
							"</config>\n";
	g_file_set_contents(std::string(tmpdir + "/shared.xml").c_str(), shared_xml.c_str(), -1, NULL);

	std::string config_xml = "<?xml version=\"1.0\"?>\n"
							"<config version=\"1.0\" serial=\"7\" timestamp=\"" + boost::lexical_cast<std::string>(time(NULL)) + ".0\">\n"
								"<Lib>\n"
									"<Account>\n"
									"<IdleTimeForAway>30000000</IdleTimeForAway>\n"
									"<IdleTimeForNA>300000000</IdleTimeForNA>\n"
									"<LastUsed>" + boost::lexical_cast<std::string>(time(NULL)) + "</LastUsed>\n"
									"</Account>\n"
								"</Lib>\n"
								"<UI>\n"
									"<API>\n"
									"<Authorizations>Spectrum</Authorizations>\n"
									"<BlockedPrograms></BlockedPrograms>\n"
									"</API>\n"
								"</UI>\n"
							"</config>\n";
	g_file_set_contents(std::string(tmpdir + "/" + m_username +"/config.xml").c_str(), config_xml.c_str(), -1, NULL);

	return tmpdir;
}

bool Skype::spawnSkype(const std::string &db_path) {
	char *db = (char *) malloc(db_path.size() + 1);
	strcpy(db, db_path.c_str());
	LOG4CXX_INFO(logger,  m_username << ": Spawning new Skype instance dbpath=" << db);
	gchar* argv[6] = {"skype", "--disable-cleanlooks", "--pipelogin", "--dbpath", db, 0};

	int fd;
	GError *error = NULL;
	bool spawned = g_spawn_async_with_pipes(NULL,
		argv,
		NULL /*envp*/,
		G_SPAWN_SEARCH_PATH,
		NULL /*child_setup*/,
		NULL /*user_data*/,
		&m_pid /*child_pid*/,
		&fd,
		NULL,
		&fd_output,
		&error);

	if (!spawned) {
		LOG4CXX_ERROR(logger, "Error spawning the Skype instance: " << error->message)
		return false;
	}

	std::string login_data = std::string(m_username + " " + m_password + "\n");
	LOG4CXX_INFO(logger,  m_username << ": Login data=" << m_username);
	write(fd, login_data.c_str(), login_data.size());
	close(fd);

	fcntl (fd_output, F_SETFL, O_NONBLOCK);

	free(db);

	return true;
}
