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

#include "skypeplugin.h"
#include "skype.h"
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

using namespace Transport;

DEFINE_LOGGER(logger, "SkypePlugin");

static int m_sock;

static gboolean transportDataReceived(GIOChannel *source, GIOCondition condition, gpointer data) {
	SkypePlugin *np = (SkypePlugin *) data;
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

static void io_destroy(gpointer data) {
	Logging::shutdownLogging();
	exit(1);
}

SkypePlugin::SkypePlugin(Config *config, const std::string &host, int port) : NetworkPlugin() {
	this->config = config;
	LOG4CXX_INFO(logger, "Starting the backend.");

	m_sock = create_socket(host.c_str(), port);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition) G_IO_IN;
	channel = g_io_channel_unix_new(m_sock);
	g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond, transportDataReceived, this, io_destroy);
}

SkypePlugin::~SkypePlugin() {
	for (std::map<Skype *, std::string>::iterator it = m_accounts.begin(); it != m_accounts.end(); it++) {
		delete (*it).first;
	}
}

void SkypePlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
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

void SkypePlugin::handleMemoryUsage(double &res, double &shared) {
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

void SkypePlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	Skype *skype = m_sessions[user];
	if (skype) {
		LOG4CXX_INFO(logger, "User wants to logout, logging out");
		skype->logout();
		Logging::shutdownLogging();
		exit(1);
	}
}

void SkypePlugin::handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage) {
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

void SkypePlugin::handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
	Skype *skype = m_sessions[user];
	if (skype) {
		skype->send_command("SET USER " + buddyName + " BUDDYSTATUS 2 Please authorize me");
		skype->send_command("SET USER " + buddyName + " ISAUTHORIZED TRUE");
	}
}

void SkypePlugin::handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
	Skype *skype = m_sessions[user];
	if (skype) {
		skype->send_command("SET USER " + buddyName + " BUDDYSTATUS 1");
		skype->send_command("SET USER " + buddyName + " ISAUTHORIZED FALSE");
	}
}

void SkypePlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml, const std::string &id) {
	Skype *skype = m_sessions[user];
	if (skype) {
		skype->send_command("MESSAGE " + legacyName + " " + message);
	}
	
}

void SkypePlugin::handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
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
			std::string db_path = std::string("/tmp/skype/") + skype->getUsername() + "/" + skype->getUsername() + "/main.db";
			SkypeDB::getAvatar(db_path, name, photo);
		}

		std::string alias;
		std::cout << skype->getUsername() << " " << name << "\n";
		if (skype->getUsername() == name) {
			alias = skype->send_command("GET PROFILE FULLNAME");
			alias = GET_RESPONSE_DATA(alias, "FULLNAME")
		}
		handleVCard(user, id, legacyName, "", alias, photo);
	}
}

void SkypePlugin::sendData(const std::string &string) {
	write(m_sock, string.c_str(), string.size());
// 			if (writeInput == 0)
// 				writeInput = purple_input_add(m_sock, PURPLE_INPUT_WRITE, &transportDataReceived, NULL);
}

void SkypePlugin::handleVCardUpdatedRequest(const std::string &user, const std::string &p, const std::string &nickname) {
}

void SkypePlugin::handleBuddyBlockToggled(const std::string &user, const std::string &buddyName, bool blocked) {

}

void SkypePlugin::handleTypingRequest(const std::string &user, const std::string &buddyName) {

}

void SkypePlugin::handleTypedRequest(const std::string &user, const std::string &buddyName) {

}

void SkypePlugin::handleStoppedTypingRequest(const std::string &user, const std::string &buddyName) {

}

void SkypePlugin::handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message) {

}

