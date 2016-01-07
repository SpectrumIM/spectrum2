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

#include "transport/AdminInterface.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/UserManager.h"
#include "transport/NetworkPluginServer.h"
#include "transport/Logging.h"
#include "transport/UserRegistration.h"
#include "transport/Frontend.h"
#include "transport/MemoryUsage.h"
#include "transport/Config.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

namespace Transport {

DEFINE_LOGGER(logger, "AdminInterface");

static std::string getArg(const std::string &body) {
	std::string ret;
	if (body.find(" ") == std::string::npos)
		return ret;

	return body.substr(body.find(" ") + 1);
}

AdminInterface::AdminInterface(Component *component, UserManager *userManager, NetworkPluginServer *server, StorageBackend *storageBackend, UserRegistration *userRegistration) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_userManager = userManager;
	m_server = server;
	m_userRegistration = userRegistration;
	m_start = time(NULL);

	m_component->getFrontend()->onMessageReceived.connect(bind(&AdminInterface::handleMessageReceived, this, _1));
}

AdminInterface::~AdminInterface() {
}

void AdminInterface::handleQuery(Swift::Message::ref message) {
	LOG4CXX_INFO(logger, "Message from admin received: '" << message->getBody() << "'");
	message->setTo(message->getFrom());
	message->setFrom(m_component->getJID());

	if (message->getBody() == "status") {
		int users = m_userManager->getUserCount();
		int backends = m_server->getBackendCount();
		message->setBody("Running (" + boost::lexical_cast<std::string>(users) + " users connected using " + boost::lexical_cast<std::string>(backends) + " backends)");
	}
	else if (message->getBody() == "uptime") {
		message->setBody(boost::lexical_cast<std::string>(time(0) - m_start));
	}
	else if (message->getBody() == "online_users") {
		std::string lst;
		const std::map<std::string, User *> &users = m_userManager->getUsers();
		if (users.size() == 0)
			lst = "0";

		for (std::map<std::string, User *>::const_iterator it = users.begin(); it != users.end(); it ++) {
			lst += (*it).first + "\n";
		}

		message->setBody(lst);
	}
	else if (message->getBody() == "online_users_count") {
		int users = m_userManager->getUserCount();
		message->setBody(boost::lexical_cast<std::string>(users));
	}
	else if (message->getBody() == "reload") {
		bool done = m_component->getConfig()->reload();
		if (done) {
			message->setBody("Config reloaded");
		}
		else {
			message->setBody("Error during config reload");
		}
	}
	else if (message->getBody() == "online_users_per_backend") {
		std::string lst;
		int id = 1;

		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		for (std::list <NetworkPluginServer::Backend *>::const_iterator b = backends.begin(); b != backends.end(); b++) {
			NetworkPluginServer::Backend *backend = *b;
			lst += "Backend " + boost::lexical_cast<std::string>(id) + " (ID=" + backend->id + ")";
			lst += backend->acceptUsers ? "" : " - not-accepting";
			lst += backend->longRun ? " - long-running" : "";
			lst += ":\n";
			if (backend->users.size() == 0) {
				lst += "   waiting for users\n";
			}
			else {
				time_t now = time(NULL);
				for (std::list<User *>::const_iterator u = backend->users.begin(); u != backend->users.end(); u++) {
					User *user = *u;
					lst += "   " + user->getJID().toBare().toString();
					lst += " - non-active for " + boost::lexical_cast<std::string>(now - user->getLastActivity()) + " seconds";
					lst += "\n";
				}
			}
			id++;
		}

		message->setBody(lst);
	}
	else if (message->getBody().find("has_online_user") == 0) {
		User *user = m_userManager->getUser(getArg(message->getBody()));
		std::cout << getArg(message->getBody()) << "\n";
		message->setBody(boost::lexical_cast<std::string>(user != NULL));
	}
	else if (message->getBody() == "backends_count") {
		int backends = m_server->getBackendCount();
		message->setBody(boost::lexical_cast<std::string>(backends));
	}
	else if (message->getBody() == "res_memory") {
		double shared = 0;
		double rss = 0;
		process_mem_usage(shared, rss);
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			rss += backend->res;
		}

		message->setBody(boost::lexical_cast<std::string>(rss));
	}
	else if (message->getBody() == "shr_memory") {
		double shared = 0;
		double rss = 0;
		process_mem_usage(shared, rss);
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			shared += backend->shared;
		}

		message->setBody(boost::lexical_cast<std::string>(shared));
	}
	else if (message->getBody() == "used_memory") {
		double shared = 0;
		double rss = 0;
		process_mem_usage(shared, rss);
		rss -= shared;

		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			rss += backend->res - backend->shared;
		}

		message->setBody(boost::lexical_cast<std::string>(rss));
	}
	else if (message->getBody() == "average_memory_per_user") {
		if (m_userManager->getUserCount() == 0) {
			message->setBody(boost::lexical_cast<std::string>(0));
		}
		else {
			unsigned long per_user = 0;
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				if (backend->res >= backend->init_res) {
					per_user += (backend->res - backend->init_res);
				}
			}

			message->setBody(boost::lexical_cast<std::string>(per_user / m_userManager->getUserCount()));
		}
	}
	else if (message->getBody() == "res_memory_per_backend") {
		std::string lst;
		int id = 1;
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			lst += "Backend " + boost::lexical_cast<std::string>(id) + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->res) + "\n";
			id++;
		}

		message->setBody(lst);
	}
	else if (message->getBody() == "shr_memory_per_backend") {
		std::string lst;
		int id = 1;
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			lst += "Backend " + boost::lexical_cast<std::string>(id)  + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->shared) + "\n";
			id++;
		}

		message->setBody(lst);
	}
	else if (message->getBody() == "used_memory_per_backend") {
		std::string lst;
		int id = 1;
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			lst += "Backend " + boost::lexical_cast<std::string>(id)  + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->res - backend->shared) + "\n";
			id++;
		}

		message->setBody(lst);
	}
	else if (message->getBody() == "average_memory_per_user_per_backend") {
		std::string lst;
		int id = 1;
		const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
		BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
			if (backend->users.size() == 0 || backend->res < backend->init_res) {
				lst += "Backend " + boost::lexical_cast<std::string>(id)  + " (ID=" + backend->id + "): 0\n";
			}
			else {
				lst += "Backend " + boost::lexical_cast<std::string>(id) + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>((backend->res - backend->init_res) / backend->users.size()) + "\n";
			}
			id++;
		}

		message->setBody(lst);
	}
	else if (message->getBody() == "collect_backend") {
		m_server->collectBackend();
	}
	else if (message->getBody() == "crashed_backends") {
		std::string lst;
		const std::vector<std::string> &backends = m_server->getCrashedBackends();
		BOOST_FOREACH(const std::string &backend, backends) {
			lst += backend + "\n";
		}
		message->setBody(lst);
	}
	else if (message->getBody() == "crashed_backends_count") {
		message->setBody(boost::lexical_cast<std::string>(m_server->getCrashedBackends().size()));
	}
	else if (message->getBody() == "messages_from_xmpp") {
		int msgCount = m_userManager->getMessagesToBackend();
		message->setBody(boost::lexical_cast<std::string>(msgCount));
	}
	else if (message->getBody() == "messages_to_xmpp") {
		int msgCount = m_userManager->getMessagesToXMPP();
		message->setBody(boost::lexical_cast<std::string>(msgCount));
	}
	else if (message->getBody().find("register ") == 0 && m_userRegistration) {
		std::string body = message->getBody();
		std::vector<std::string> args;
		boost::split(args, body, boost::is_any_of(" "));
		if (args.size() == 4) {
			UserInfo res;
			res.jid = args[1];
			res.uin = args[2];
			res.password = args[3];
			res.language = "en";
			res.encoding = "utf-8";
			res.vip = 0;

			if (m_userRegistration->registerUser(res)) {
				message->setBody("User registered.");
			}
			else {
				message->setBody("Registration failed: User is already registered");
			}
		}
		else {
			message->setBody("Bad argument count. See 'help'.");
		}
	}
	else if (message->getBody().find("unregister ") == 0 && m_userRegistration) {
		std::string body = message->getBody();
		std::vector<std::string> args;
		boost::split(args, body, boost::is_any_of(" "));
		if (args.size() == 2) {
			if (m_userRegistration->unregisterUser(args[1])) {
				message->setBody("User '" + args[1] + "' unregistered.");
			}
			else {
				message->setBody("Unregistration failed: User '" + args[1] + "' is not registered");
			}
		}
		else {
			message->setBody("Bad argument count. See 'help'.");
		}
	}
	else if (message->getBody().find("set_oauth2_code ") == 0) {
		std::string body = message->getBody();
		std::vector<std::string> args;
		boost::split(args, body, boost::is_any_of(" "));
		if (args.size() == 3) {
			std::string error = m_component->getFrontend()->setOAuth2Code(args[1], args[2]);
			if (error.empty()) {
				message->setBody("OAuth2 code and state set.");
			}
			else {
				message->setBody(error);
			}
		}
		else {
			message->setBody("Bad argument count. See 'help'.");
		}
	}
	else if (message->getBody().find("get_oauth2_url") == 0) {
		std::string body = message->getBody();
		std::vector<std::string> args;
		boost::split(args, body, boost::is_any_of(" "));
		std::string url = m_component->getFrontend()->getOAuth2URL(args);
		message->setBody(url);
	}
	else if (message->getBody() == "registration_fields") {
		std::string fields = m_component->getFrontend()->getRegistrationFields();
		message->setBody(fields);
	}
	else if (message->getBody().find("help") == 0) {
		std::string help;
		help += "General:\n";
		help += "    status - shows instance status\n";
		help += "    reload - Reloads config file\n";
		help += "    uptime - returns ptime in seconds\n";
		help += "Users:\n";
		help += "    online_users - returns list of all online users\n";
		help += "    online_users_count - number of online users\n";
		help += "    online_users_per_backend - shows online users per backends\n";
		help += "    has_online_user <bare_JID> - returns 1 if user is online\n";
		if (m_userRegistration) {
			help += "    register <bare_JID> <legacyName> <password> - registers the new user\n";
			help += "    unregister <bare_JID> - unregisters existing user\n";
		}
		help += "Messages:\n";
		help += "    messages_from_xmpp - get number of messages received from XMPP users\n";
		help += "    messages_to_xmpp - get number of messages sent to XMPP users\n";
		help += "Frontend:\n";
		help += "    set_oauth2_code <code> <state> - sets the OAuth2 code and state for this instance\n";
		help += "Backends:\n";
		help += "    backends_count - number of active backends\n";
		help += "    crashed_backends - returns IDs of crashed backends\n";
		help += "    crashed_backends_count - returns number of crashed backends\n";
		help += "Memory:\n";
		help += "    res_memory - Total RESident memory spectrum2 and its backends use in KB\n";
		help += "    shr_memory - Total SHaRed memory spectrum2 backends share together in KB\n";
		help += "    used_memory - (res_memory - shr_memory)\n";
		help += "    average_memory_per_user - (memory_used_without_any_user - res_memory)\n";
		help += "    res_memory_per_backend - RESident memory used by backends in KB\n";
		help += "    shr_memory_per_backend - SHaRed memory used by backends in KB\n";
		help += "    used_memory_per_backend - (res_memory - shr_memory) per backend\n";
		help += "    average_memory_per_user_per_backend - (memory_used_without_any_user - res_memory) per backend\n";
		
		
		message->setBody(help);
	}
	else {
		message->setBody("Unknown command \"" + message->getBody() + "\". Try \"help\"");
	}
}

void AdminInterface::handleMessageReceived(Swift::Message::ref message) {
	if (!message->getTo().getNode().empty())
		return;

	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"service.admin_jid");
	if (std::find(x.begin(), x.end(), message->getFrom().toBare().toString()) == x.end()) {
	    LOG4CXX_WARN(logger, "Message not from admin user, but from " << message->getFrom().toBare().toString());
	    return;
	
	}
	
	// Ignore empty messages
	if (message->getBody().empty()) {
		return;
	}

	handleQuery(message);

	m_component->getFrontend()->sendMessage(message);
}

}
