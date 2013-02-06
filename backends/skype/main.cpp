#include "glib.h"
#include <dbus-1.0/dbus/dbus-glib-lowlevel.h>
#include "sqlite3.h"
#include <iostream>

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

#include "skype.h"


DEFINE_LOGGER(logger, "backend");

using namespace Transport;

class SkypePlugin;

#define GET_RESPONSE_DATA(RESP, DATA) ((RESP.find(std::string(DATA) + " ") != std::string::npos) ? RESP.substr(RESP.find(DATA) + strlen(DATA) + 1) : "");
#define GET_PROPERTY(VAR, OBJ, WHICH, PROP) std::string VAR = sk->send_command(std::string("GET ") + OBJ + " " + WHICH + " " + PROP); \
					try {\
						VAR = GET_RESPONSE_DATA(VAR, PROP);\
					}\
					catch (std::out_of_range& oor) {\
						VAR="";\
					}
					

					
// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	if(sqlite3_prepare_v2(db, std::string(str).c_str(), -1, &sql, NULL)) { \
		LOG4CXX_ERROR(logger, str<< (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db))); \
		sql = NULL; \
	}

// Finalize the prepared statement
#define FINALIZE_STMT(prep) \
	if(prep != NULL) { \
		sqlite3_finalize(prep); \
	}
	
#define BEGIN(STATEMENT) 	sqlite3_reset(STATEMENT);\
							int STATEMENT##_id = 1;\
							int STATEMENT##_id_get = 0;\
							(void)STATEMENT##_id_get;

#define BIND_INT(STATEMENT, VARIABLE) sqlite3_bind_int(STATEMENT, STATEMENT##_id++, VARIABLE)
#define BIND_STR(STATEMENT, VARIABLE) sqlite3_bind_text(STATEMENT, STATEMENT##_id++, VARIABLE.c_str(), -1, SQLITE_STATIC)
#define RESET_GET_COUNTER(STATEMENT)	STATEMENT##_id_get = 0;
#define GET_INT(STATEMENT)	sqlite3_column_int(STATEMENT, STATEMENT##_id_get++)
#define GET_STR(STATEMENT)	(const char *) sqlite3_column_text(STATEMENT, STATEMENT##_id_get++)
#define GET_BLOB(STATEMENT)	(const void *) sqlite3_column_blob(STATEMENT, STATEMENT##_id_get++)
#define EXECUTE_STATEMENT(STATEMENT, NAME) 	if(sqlite3_step(STATEMENT) != SQLITE_DONE) {\
		LOG4CXX_ERROR(logger, NAME<< (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));\
			}

SkypePlugin *np;

int m_sock;
static int writeInput;

static std::string host;
static int port = 10000;

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

class SkypePlugin : public NetworkPlugin {
	public:
		SkypePlugin(Config *config, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			LOG4CXX_INFO(logger, "Starting the backend.");
		}

		~SkypePlugin() {
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

			Skype *skype = new Skype(this, user, name, password);
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
				Logging::shutdownLogging();
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
				skype->send_command("SET USER " + buddyName + " ISAUTHORIZED FALSE");
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "") {
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

				if (photo.empty()) {
					sqlite3 *db;
					std::string db_path = std::string("/tmp/skype/") + skype->getUsername() + "/" + skype->getUsername() + "/main.db";
					LOG4CXX_INFO(logger, "Opening database " << db_path);
					if (sqlite3_open(db_path.c_str(), &db)) {
						sqlite3_close(db);
						LOG4CXX_ERROR(logger, "Can't open database");
					}
					else {
						sqlite3_stmt *stmt;
						PREP_STMT(stmt, "SELECT avatar_image FROM Contacts WHERE skypename=?");
						if (stmt) {
							BEGIN(stmt);
							BIND_STR(stmt, name);
							if(sqlite3_step(stmt) == SQLITE_ROW) {
								int size = sqlite3_column_bytes(stmt, 0);
								const void *data = sqlite3_column_blob(stmt, 0);
								photo = std::string((const char *)data + 1, size - 1);
							}
							else {
								LOG4CXX_ERROR(logger, (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));
							}

							int ret;
							while((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
							}
							FINALIZE_STMT(stmt);
						}
						else {
							LOG4CXX_ERROR(logger, "Can't created prepared statement");
							LOG4CXX_ERROR(logger, (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));
						}
						sqlite3_close(db);
					}
				}

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

static int create_socket(const char *host, int portno) {
	struct sockaddr_in serv_addr;
	
	int m_sock = socket(AF_INET, SOCK_STREAM, 0);
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	hostent *hos;  // Resolve name
	if ((hos = gethostbyname(host)) == NULL) {
		// strerror() will not work for gethostbyname() and hstrerror() 
		// is supposedly obsolete
		Logging::shutdownLogging();
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
		Logging::shutdownLogging();
		exit(errno);
	}
	std::string d = std::string(buffer, n);
	np->handleDataRead(d);
	return TRUE;
}

static void io_destroy(gpointer data) {
	Logging::shutdownLogging();
	exit(1);
}

static void log_glib_error(const gchar *string) {
	LOG4CXX_ERROR(logger, "GLIB ERROR:" << string);
}

int main(int argc, char **argv) {
#ifndef WIN32
		signal(SIGPIPE, SIG_IGN);

		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
			std::cout << "SIGCHLD handler can't be set\n";
			return -1;
		}
#endif

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	g_type_init();
	

	m_sock = create_socket(host.c_str(), port);

	g_set_printerr_handler(log_glib_error);

	GIOChannel *channel;
	GIOCondition cond = (GIOCondition) G_IO_IN;
	channel = g_io_channel_unix_new(m_sock);
	g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond, transportDataReceived, NULL, io_destroy);

	dbus_threads_init_default();

	np = new SkypePlugin(cfg, host, port);

	GMainLoop *m_loop;
	m_loop = g_main_loop_new(NULL, FALSE);

	if (m_loop) {
		g_main_loop_run(m_loop);
	}

}
