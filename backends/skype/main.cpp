#include "glib.h"
#include <iostream>

#include "transport/config.h"
#include "transport/logging.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/memoryusage.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/user.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/conversation.h"
#include "transport/networkplugin.h"
#include "transport/logger.h"
#include <boost/filesystem.hpp>
#include "sys/wait.h"
#include "sys/signal.h"
// #include "valgrind/memcheck.h"
#include "malloc.h"
#include <dbus-1.0/dbus/dbus-glib-lowlevel.h>


DEFINE_LOGGER(logger, "backend");

using namespace Transport;

class SpectrumNetworkPlugin;

#define GET_RESPONSE_DATA(RESP, DATA) ((RESP.find(std::string(DATA) + " ") != std::string::npos) ? RESP.substr(RESP.find(DATA) + strlen(DATA) + 1) : "");
#define GET_PROPERTY(VAR, OBJ, WHICH, PROP) std::string VAR = sk->send_command(std::string("GET ") + OBJ + " " + WHICH + " " + PROP); \
					try {\
						VAR = GET_RESPONSE_DATA(VAR, PROP);\
					}\
					catch (std::out_of_range& oor) {\
						VAR="";\
					}
					


SpectrumNetworkPlugin *np;

static gboolean nodaemon = FALSE;
static gchar *logfile = NULL;
static gchar *lock_file = NULL;
static gchar *host = NULL;
static int port = 10000;
static gboolean ver = FALSE;
static gboolean list_purple_settings = FALSE;

int m_sock;
static int writeInput;

static GOptionEntry options_entries[] = {
	{ "nodaemon", 'n', 0, G_OPTION_ARG_NONE, &nodaemon, "Disable background daemon mode", NULL },
	{ "logfile", 'l', 0, G_OPTION_ARG_STRING, &logfile, "Set file to log", NULL },
	{ "pidfile", 'p', 0, G_OPTION_ARG_STRING, &lock_file, "File where to write transport PID", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Shows Spectrum version", NULL },
	{ "list-purple-settings", 's', 0, G_OPTION_ARG_NONE, &list_purple_settings, "Lists purple settings which can be used in config file", NULL },
	{ "host", 'h', 0, G_OPTION_ARG_STRING, &host, "Host to connect to", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to connect to", NULL },
	{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, "", NULL }
};

DBusHandlerResult skype_notify_handler(DBusConnection *connection, DBusMessage *message, gpointer user_data);

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

class Skype {
	public:
		Skype(const std::string &user, const std::string &username, const std::string &password);
		~Skype() { LOG4CXX_INFO(logger, "Skype instance desctuctor"); logout(); }
		void login();
		void logout();
		std::string send_command(const std::string &message);

		const std::string &getUser() {
			return m_user;
		}

		const std::string &getUsername() {
			return m_username;
		}

		bool createDBusProxy();
		bool loadSkypeBuddies();

		int getPid() {
			return (int) m_pid;
		}

	private:
		std::string m_username;
		std::string m_password;
		GPid m_pid;
		DBusGConnection *m_connection;
		DBusGProxy *m_proxy;
		std::string m_user;
		int m_timer;
		int m_counter;
		int fd_output;
};

class SpectrumNetworkPlugin : public NetworkPlugin {
	public:
		SpectrumNetworkPlugin(Config *config, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			LOG4CXX_INFO(logger, "Starting the backend.");
		}

		~SpectrumNetworkPlugin() {
			for (std::map<Skype *, std::string>::iterator it = m_accounts.begin(); it != m_accounts.end(); it++) {
				delete (*it).first;
			}
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			std::string name = legacyName;
			if (name.find("skype.") == 0 || name.find("prpl-skype.") == 0) {
				name = name.substr(name.find(".") + 1);
			}
			LOG4CXX_INFO(logger,  "Creating account with name '" << name << "'");

			Skype *skype = new Skype(user, name, password);
			m_sessions[user] = skype;
			m_accounts[skype] = user;

			skype->login();
		}

		void handleMemoryUsage(double &res, double &shared) {
			res = 0;
			shared = 0;
			for(std::map<std::string, Skype *>::const_iterator it = m_sessions.begin(); it != m_sessions.end(); it++) {
				Skype *skype = it->second;
				if (skype) {
					double r;
					double s;
					process_mem_usage(s, r, skype->getPid());
					res += r;
					shared += s;
				}
			}
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			Skype *skype = m_sessions[user];
			if (skype) {
				LOG4CXX_INFO(logger, "User wants to logout, logging out");
				skype->logout();
				exit(1);
			}
		}

		void handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage) {
			Skype *skype = m_sessions[user];
			if (!skype)
				return;

			std::string st;
			switch(status) {
				case Swift::StatusShow::Away: {
					st = "AWAY";
					break;
				}
				case Swift::StatusShow::DND: {
					st = "DND";
					break;
				}
				case Swift::StatusShow::XA: {
					st = "NA";
					break;
				}
				case Swift::StatusShow::None: {
					break;
				}
				case pbnetwork::STATUS_INVISIBLE:
					st = "INVISIBLE";
					break;
				default:
					st = "ONLINE";
					break;
			}
			skype->send_command("SET USERSTATUS " + st);

			if (!statusMessage.empty()) {
				skype->send_command("SET PROFILE MOOD_TEXT " + statusMessage);
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			Skype *skype = m_sessions[user];
			if (skype) {
				skype->send_command("SET USER " + buddyName + " BUDDYSTATUS 2 Please authorize me");
				skype->send_command("SET USER " + buddyName + " ISAUTHORIZED TRUE");
			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
			Skype *skype = m_sessions[user];
			if (skype) {
				skype->send_command("SET USER " + buddyName + " BUDDYSTATUS 1");
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml) {
			Skype *skype = m_sessions[user];
			if (skype) {
				skype->send_command("MESSAGE " + legacyName + " " + message);
			}
			
		}

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
			Skype *skype = m_sessions[user];
			if (skype) {
				std::string name = legacyName;
				if (name.find("skype.") == 0) {
					name = name.substr(6);
				}
				std::string photo;
				gchar *filename = NULL;
				gchar *new_filename = NULL;
				gchar *image_data = NULL;
				gsize image_data_len = 0;
				gchar *ret;
				int fh;
				GError *error;
				const gchar *userfiles[] = {"user256", "user1024", "user4096", "user16384", "user32768", "user65536",
											"profile256", "profile1024", "profile4096", "profile16384", "profile32768", 
											NULL};
				char *username = g_strdup_printf("\x03\x10%s", name.c_str());
				for (fh = 0; userfiles[fh]; fh++) {
					filename = g_strconcat("/tmp/skype/", skype->getUsername().c_str(), "/", skype->getUsername().c_str(), "/", userfiles[fh], ".dbb", NULL);
					std::cout << "getting filename:" << filename << "\n";
					if (g_file_get_contents(filename, &image_data, &image_data_len, NULL))
					{
						std::cout << "got\n";
						char *start = (char *)memmem(image_data, image_data_len, username, strlen(username)+1);
						if (start != NULL)
						{
							char *next = image_data;
							char *last = next;
							//find last index of l33l
							while ((next = (char *)memmem(next+4, start-next-4, "l33l", 4)))
							{
								last = next;
							}
							start = last;
							if (start != NULL)
							{
								char *img_start;
								//find end of l33l block
								char *end = (char *)memmem(start+4, image_data+image_data_len-start-4, "l33l", 4);
								if (!end) end = image_data+image_data_len;
								
								//look for start of JPEG block
								img_start = (char *)memmem(start, end-start, "\xFF\xD8", 2);
								if (img_start)
								{
									//look for end of JPEG block
									char *img_end = (char *)memmem(img_start, end-img_start, "\xFF\xD9", 2);
									if (img_end)
									{
										image_data_len = img_end - img_start + 2;
										photo = std::string(img_start, image_data_len);
									}
								}
							}
						}
						g_free(image_data);
					}
					g_free(filename);
				}
				g_free(username);
				
				std::string alias = "";
				std::cout << skype->getUsername() << " " << name << "\n";
				if (skype->getUsername() == name) {
					alias = skype->send_command("GET PROFILE FULLNAME");
					alias = GET_RESPONSE_DATA(alias, "FULLNAME")
				}
				handleVCard(user, id, legacyName, "", alias, photo);
			}
		}

		void sendData(const std::string &string) {
			write(m_sock, string.c_str(), string.size());
// 			if (writeInput == 0)
// 				writeInput = purple_input_add(m_sock, PURPLE_INPUT_WRITE, &transportDataReceived, NULL);
		}

		void handleVCardUpdatedRequest(const std::string &user, const std::string &p, const std::string &nickname) {
		}

		void handleBuddyBlockToggled(const std::string &user, const std::string &buddyName, bool blocked) {

		}

		void handleTypingRequest(const std::string &user, const std::string &buddyName) {

		}

		void handleTypedRequest(const std::string &user, const std::string &buddyName) {

		}

		void handleStoppedTypingRequest(const std::string &user, const std::string &buddyName) {

		}

		void handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message) {

		}

		std::map<std::string, Skype *> m_sessions;
		std::map<Skype *, std::string> m_accounts;
		std::map<std::string, unsigned int> m_vcards;
		Config *config;
		
};


Skype::Skype(const std::string &user, const std::string &username, const std::string &password) {
	m_username = username;
	m_user = user;
	m_password = password;
	m_pid = 0;
	m_connection = 0;
	m_proxy = 0;
	m_timer = -1;
	m_counter = 0;
}


static gboolean load_skype_buddies(gpointer data) {
	Skype *skype = (Skype *) data;
	return skype->loadSkypeBuddies();
}

bool Skype::createDBusProxy() {
	if (m_proxy == NULL) {
		LOG4CXX_INFO(logger, "Creating DBus proxy for com.Skype.Api.");
		m_counter++;

		GError *error = NULL;
		m_proxy = dbus_g_proxy_new_for_name_owner (m_connection, "com.Skype.API", "/com/Skype", "com.Skype.API", &error);
		if (m_proxy == NULL && error != NULL) {
			LOG4CXX_INFO(logger,  m_username << ":" << error->message);

			if (m_counter == 15) {
				LOG4CXX_ERROR(logger, "Logging out, proxy couldn't be created");
				np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, error->message);
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
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean create_dbus_proxy(gpointer data) {
	Skype *skype = (Skype *) data;
	return skype->createDBusProxy();
}

void Skype::login() {
	if (m_username.find("..") == 0 || m_username.find("/") != std::string::npos) {
		np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Invalid username");
		return;
	}
	boost::filesystem::remove_all(std::string("/tmp/skype/") + m_username);

	boost::filesystem::path	path(std::string("/tmp/skype/") + m_username);
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		boost::filesystem::path	path2(std::string("/tmp/skype/") + m_username + "/" + m_username );
		boost::filesystem::create_directories(path2);
	}

	std::string shared_xml = "<?xml version=\"1.0\"?>\n"
							"<config version=\"1.0\" serial=\"28\" timestamp=\"" + boost::lexical_cast<std::string>(time(NULL)) + ".0\">\n"
							"<UI>\n"
								"<Installed>2</Installed>\n"
								"<Language>en</Language>\n"
							"</UI>\n"
							"</config>\n";
	g_file_set_contents(std::string(std::string("/tmp/skype/") + m_username + "/shared.xml").c_str(), shared_xml.c_str(), -1, NULL);

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
	g_file_set_contents(std::string(std::string("/tmp/skype/") + m_username + "/" + m_username +"/config.xml").c_str(), config_xml.c_str(), -1, NULL);

	sleep(1);
	std::string db_path = std::string("/tmp/skype/") + m_username;
	char *db = (char *) malloc(db_path.size() + 1);
	strcpy(db, db_path.c_str());
	LOG4CXX_INFO(logger,  m_username << ": Spawning new Skype instance dbpath=" << db);
	gchar* argv[6] = {"skype", "--disable-cleanlooks", "--pipelogin", "--dbpath", db, 0};

	int fd;
	g_spawn_async_with_pipes(NULL,
		argv,
		NULL /*envp*/,
		G_SPAWN_SEARCH_PATH,
		NULL /*child_setup*/,
		NULL /*user_data*/,
		&m_pid /*child_pid*/,
		&fd,
		NULL,
		&fd_output,
		NULL /*error*/);
	std::string login_data = std::string(m_username + " " + m_password + "\n");
	LOG4CXX_INFO(logger,  m_username << ": Login data=" << login_data);
	write(fd, login_data.c_str(), login_data.size());
	close(fd);

	fcntl (fd_output, F_SETFL, O_NONBLOCK);

	free(db);

	//Initialise threading
	dbus_threads_init_default();

	if (m_connection == NULL)
	{
		LOG4CXX_INFO(logger, "Creating DBus connection.");
		GError *error = NULL;
		m_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
		if (m_connection == NULL && error != NULL)
		{
			LOG4CXX_INFO(logger,  m_username << ": DBUS Error: " << error->message);
			g_error_free(error);
			return;
		}
	}

	sleep(1);
	m_timer = g_timeout_add_seconds(1, create_dbus_proxy, this);
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
			np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_FAILED, "Incorrect password");
			close(fd_output);
			logout();
			return FALSE;
		}
	}

	std::string re = send_command("NAME Spectrum");
	if (m_counter++ > 15) {
		LOG4CXX_ERROR(logger, "Logging out, because we tried to connect the Skype over DBUS 15 times without success");
		np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Skype is not ready. This issue have been logged and admins will check it and try to fix it soon.");
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
		np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE, "Skype is not ready. This issue have been logged and admins will check it and try to fix it soon.");
		logout();
		return FALSE;
	}

	np->handleConnected(m_user);

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
			np->handleBuddyChanged(m_user, buddy, alias, groups, status, mood_text);
		}
	}
	g_strfreev(full_friends_list);

	send_command("SET AUTOAWAY OFF");
	send_command("SET USERSTATUS ONLINE");
	return FALSE;
}

void Skype::logout() {
	if (m_pid != 0) {
		send_command("SET USERSTATUS INVISIBLE");
		send_command("SET USERSTATUS OFFLINE");
		sleep(2);
		g_object_unref(m_proxy);
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

static void handle_skype_message(std::string &message, Skype *sk) {
	std::vector<std::string> cmd;
	boost::split(cmd, message, boost::is_any_of(" "));

	if (cmd[0] == "USER") {
		if (cmd[1] == sk->getUsername()) {
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
				np->handleBuddyChanged(sk->getUser(), cmd[1], alias, groups, status, mood_text);
			}
		}
		else if (cmd[2] == "MOOD_TEXT") {
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::string mood_text = GET_RESPONSE_DATA(message, "MOOD_TEXT");

			std::vector<std::string> groups;
			np->handleBuddyChanged(sk->getUser(), cmd[1], "", groups, status, mood_text);
		}
		else if (cmd[2] == "BUDDYSTATUS" && cmd[3] == "3") {
			GET_PROPERTY(mood_text, "USER", cmd[1], "MOOD_TEXT");
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::vector<std::string> groups;
			np->handleBuddyChanged(sk->getUser(), cmd[1], "", groups, status, mood_text);
		}
		else if (cmd[2] == "FULLNAME") {
			GET_PROPERTY(alias, "USER", cmd[1], "FULLNAME");
			GET_PROPERTY(mood_text, "USER", cmd[1], "MOOD_TEXT");
			GET_PROPERTY(st, "USER", cmd[1], "ONLINESTATUS");
			pbnetwork::StatusType status = getStatus(st);

			std::vector<std::string> groups;
			np->handleBuddyChanged(sk->getUser(), cmd[1], alias, groups, status, mood_text);
		}
	}
	else if (cmd[0] == "CHATMESSAGE") {
		if (cmd[3] == "RECEIVED") {
			GET_PROPERTY(body, "CHATMESSAGE", cmd[1], "BODY");
			GET_PROPERTY(from_handle, "CHATMESSAGE", cmd[1], "FROM_HANDLE");

			if (from_handle == sk->getUsername())
				return;

			np->handleMessage(sk->getUser(), from_handle, body);

			sk->send_command("SET CHATMESSAGE " + cmd[1] + " SEEN");
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
					np->handleMessage(sk->getUser(), from, "User " + dispname + " is calling you.");
				}
				else {
					np->handleMessage(sk->getUser(), from, "You have missed call from user " + dispname + ".");
				}
			}
		}
	}
}

DBusHandlerResult skype_notify_handler(DBusConnection *connection, DBusMessage *message, gpointer user_data) {
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
		handle_skype_message(m, (Skype *) user_data);
	} while(dbus_message_iter_has_next(&iterator) && dbus_message_iter_next(&iterator));
	
	dbus_message_unref(message);
	
	return DBUS_HANDLER_RESULT_HANDLED;
}

static void spectrum_sigchld_handler(int sig)
{
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);

	if ((pid == (pid_t) - 1) && (errno != ECHILD)) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
}

static int create_socket(char *host, int portno) {
	struct sockaddr_in serv_addr;
	
	int m_sock = socket(AF_INET, SOCK_STREAM, 0);
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	hostent *hos;  // Resolve name
	if ((hos = gethostbyname(host)) == NULL) {
		// strerror() will not work for gethostbyname() and hstrerror() 
		// is supposedly obsolete
		exit(1);
	}
	serv_addr.sin_addr.s_addr = *((unsigned long *) hos->h_addr_list[0]);

	if (connect(m_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		close(m_sock);
		m_sock = 0;
	}

	int flags = fcntl(m_sock, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(m_sock, F_SETFL, flags);
	return m_sock;
}


static gboolean transportDataReceived(GIOChannel *source, GIOCondition condition, gpointer data) {
	char buffer[65535];
	char *ptr = buffer;
	ssize_t n = read(m_sock, ptr, sizeof(buffer));
	if (n <= 0) {
		LOG4CXX_INFO(logger, "Diconnecting from spectrum2 server");
		exit(errno);
	}
	std::string d = std::string(buffer, n);
	np->handleDataRead(d);
	return TRUE;
}

static void io_destroy(gpointer data) {
	exit(1);
}

static void log_glib_error(const gchar *string) {
	LOG4CXX_ERROR(logger, "GLIB ERROR:" << string);
}

int main(int argc, char **argv) {
	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new("config_file_name or profile name");
	g_option_context_add_main_entries(context, options_entries, "");
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		std::cout << "option parsing failed: " << error->message << "\n";
		return -1;
	}

	if (ver) {
// 		std::cout << VERSION << "\n";
		std::cout << "verze\n";
		g_option_context_free(context);
		return 0;
	}

	if (argc != 2) {
#ifdef WIN32
		std::cout << "Usage: spectrum.exe <configuration_file.cfg>\n";
#else

#if GLIB_CHECK_VERSION(2,14,0)
	std::cout << g_option_context_get_help(context, FALSE, NULL);
#else
	std::cout << "Usage: spectrum <configuration_file.cfg>\n";
	std::cout << "See \"man spectrum\" for more info.\n";
#endif
		
#endif
	}
	else {
#ifndef WIN32
		signal(SIGPIPE, SIG_IGN);

		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
			std::cout << "SIGCHLD handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}
// 
// 		if (signal(SIGINT, spectrum_sigint_handler) == SIG_ERR) {
// 			std::cout << "SIGINT handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
// 		}
// 
// 		if (signal(SIGTERM, spectrum_sigterm_handler) == SIG_ERR) {
// 			std::cout << "SIGTERM handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
// 		}
// 
// 		struct sigaction sa;
// 		memset(&sa, 0, sizeof(sa)); 
// 		sa.sa_handler = spectrum_sighup_handler;
// 		if (sigaction(SIGHUP, &sa, NULL)) {
// 			std::cout << "SIGHUP handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
//		}
#endif
		Config config;
		if (!config.load(argv[1])) {
			std::cout << "Can't open " << argv[1] << " configuration file.\n";
			return 1;
		}

		Logging::initBackendLogging(&config);

// 		initPurple(config);

		g_type_init();

		m_sock = create_socket(host, port);

		g_set_printerr_handler(log_glib_error);

	GIOChannel *channel;
	GIOCondition cond = (GIOCondition) G_IO_IN;
	channel = g_io_channel_unix_new(m_sock);
	g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond, transportDataReceived, NULL, io_destroy);

		np = new SpectrumNetworkPlugin(&config, host, port);

		GMainLoop *m_loop;
		m_loop = g_main_loop_new(NULL, FALSE);

		if (m_loop) {
			g_main_loop_run(m_loop);
		}
	}

	g_option_context_free(context);
}
