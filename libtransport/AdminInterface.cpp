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
#include "transport/AdminInterfaceCommand.h"
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

#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

namespace Transport {

DEFINE_LOGGER(adminInterfaceLogger, "AdminInterface");

// currently unused
#if 0
static std::string getArg(const std::string &body) {
	std::string ret;
	if (body.find(" ") == std::string::npos)
		return ret;

	return body.substr(body.find(" ") + 1);
}
#endif

class StatusCommand : public AdminInterfaceCommand {
	public:

		StatusCommand(NetworkPluginServer *server, UserManager *userManager) :
												AdminInterfaceCommand("status",
												AdminInterfaceCommand::General,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			m_userManager = userManager;
			setDescription("Shows instance status");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			int users = m_userManager->getUserCount();
			int backends = m_server->getBackendCount();
			ret = "Running (" + boost::lexical_cast<std::string>(users) + " users connected using " + boost::lexical_cast<std::string>(backends) + " backends)";
			return ret;
		}

	private:
		NetworkPluginServer *m_server;
		UserManager *m_userManager;
};

class UptimeCommand : public AdminInterfaceCommand {
	public:

		UptimeCommand() : AdminInterfaceCommand("uptime",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Get) {
			m_start = time(NULL);
			setDescription("Returns ptime in seconds");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			return boost::lexical_cast<std::string>(time(0) - m_start);
		}

	private:
		time_t m_start;
};

class OnlineUsersCommand : public AdminInterfaceCommand {
	public:

		OnlineUsersCommand(UserManager *userManager) : AdminInterfaceCommand("online_users",
							AdminInterfaceCommand::Users,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute,
							"Online users") {
			m_userManager = userManager;
			setDescription("Returns list of all online users");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			const std::map<std::string, User *> &users = m_userManager->getUsers();
			if (users.empty()) {
				ret = "hanzz@njs.netlab.cz \"3rd-party network username:\" \"me\"\n";
			}

			for (std::map<std::string, User *>::const_iterator it = users.begin(); it != users.end(); it ++) {
				ret += (*it).first + " \"3rd-party network username:\" \"" + user->getUserInfo().uin + "\"\n";
			}
			return ret;
		}

	private:
		UserManager *m_userManager;
};

class OnlineUsersCountCommand : public AdminInterfaceCommand {
	public:

		OnlineUsersCountCommand(UserManager *userManager) : AdminInterfaceCommand("online_users_count",
							AdminInterfaceCommand::Users,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Get) {
			m_userManager = userManager;
			setDescription("Number of online users");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			int users = m_userManager->getUserCount();
			return boost::lexical_cast<std::string>(users);
		}

	private:
		UserManager *m_userManager;
};

class OnlineUsersPerBackendCommand : public AdminInterfaceCommand {
	public:

		OnlineUsersPerBackendCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("online_users_per_backend",
												AdminInterfaceCommand::General,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Shows online users per backends");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

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

			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class BackendsCountCommand : public AdminInterfaceCommand {
	public:

		BackendsCountCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("backends_count",
												AdminInterfaceCommand::Backends,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Number of active backends");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			int backends = m_server->getBackendCount();
			return boost::lexical_cast<std::string>(backends);
		}

	private:
		NetworkPluginServer *m_server;
};

class ReloadCommand : public AdminInterfaceCommand {
	public:

		ReloadCommand(Component *component) : AdminInterfaceCommand("reload",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute,
							"Reload Spectrum 2 configuration") {
			m_component = component;
			setDescription("Reloads config file");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			bool done = m_component->getConfig()->reload();
			if (done) {
				return "Config reloaded";
			}
			else {
				return "Error: Error during config reload";
			}
		}

	private:
		Component *m_component;
};

class HasOnlineUserCommand : public AdminInterfaceCommand {
	public:

		HasOnlineUserCommand(UserManager *userManager) : AdminInterfaceCommand("has_online_user",
							AdminInterfaceCommand::Users,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute, "Has online user") {
			m_userManager = userManager;
			setDescription("Returns 1 if user is online");
			addArg("username", "Username", "string", "user@domain.tld");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (args.empty()) {
				return "Error: Missing user name as an argument";
			}

			user = m_userManager->getUser(args[0]);
			return boost::lexical_cast<std::string>(user != NULL);
		}

	private:
		UserManager *m_userManager;
};

class ResMemoryCommand : public AdminInterfaceCommand {
	public:

		ResMemoryCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("res_memory",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Total RESident memory Spectrum 2 and its backends use in KB");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			double shared = 0;
			double rss = 0;
			process_mem_usage(shared, rss);
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				rss += backend->res;
			}

			return boost::lexical_cast<std::string>(rss);
		}

	private:
		NetworkPluginServer *m_server;
};

class ShrMemoryCommand : public AdminInterfaceCommand {
	public:

		ShrMemoryCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("shr_memory",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Total SHaRed memory spectrum2 backends share together in KB");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			double shared = 0;
			double rss = 0;
			process_mem_usage(shared, rss);
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				shared += backend->shared;
			}

			return boost::lexical_cast<std::string>(shared);
		}

	private:
		NetworkPluginServer *m_server;
};

class UsedMemoryCommand : public AdminInterfaceCommand {
	public:

		UsedMemoryCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("used_memory",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("(res_memory - shr_memory)");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			double shared = 0;
			double rss = 0;
			process_mem_usage(shared, rss);
			rss -= shared;

			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				rss += backend->res - backend->shared;
			}

			return boost::lexical_cast<std::string>(rss);
		}

	private:
		NetworkPluginServer *m_server;
};

class AverageMemoryPerUserCommand : public AdminInterfaceCommand {
	public:

		AverageMemoryPerUserCommand(NetworkPluginServer *server, UserManager *userManager) :
												AdminInterfaceCommand("average_memory_per_user",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			m_userManager = userManager;
			setDescription("(memory_used_without_any_user - res_memory)");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (m_userManager->getUserCount() == 0) {
				return boost::lexical_cast<std::string>(0);
			}
			else {
				unsigned long per_user = 0;
				const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
				BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
					if (backend->res >= backend->init_res) {
						per_user += (backend->res - backend->init_res);
					}
				}

				return boost::lexical_cast<std::string>(per_user / m_userManager->getUserCount());
			}
		}

	private:
		NetworkPluginServer *m_server;
		UserManager *m_userManager;
};

class ResMemoryPerBackendCommand : public AdminInterfaceCommand {
	public:

		ResMemoryPerBackendCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("res_memory_per_backend",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("RESident memory used by backends in KB");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string lst;
			int id = 1;
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				lst += "Backend " + boost::lexical_cast<std::string>(id) + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->res) + "\n";
				id++;
			}

			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class ShrMemoryPerBackendCommand : public AdminInterfaceCommand {
	public:

		ShrMemoryPerBackendCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("shr_memory_per_backend",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("SHaRed memory used by backends in KB");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string lst;
			int id = 1;
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				lst += "Backend " + boost::lexical_cast<std::string>(id)  + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->shared) + "\n";
				id++;
			}

			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class UsedMemoryPerBackendCommand : public AdminInterfaceCommand {
	public:

		UsedMemoryPerBackendCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("used_memory_per_backend",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("(res_memory - shr_memory) per backend");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string lst;
			int id = 1;
			const std::list <NetworkPluginServer::Backend *> &backends = m_server->getBackends();
			BOOST_FOREACH(NetworkPluginServer::Backend * backend, backends) {
				lst += "Backend " + boost::lexical_cast<std::string>(id)  + " (ID=" + backend->id + "): " + boost::lexical_cast<std::string>(backend->res - backend->shared) + "\n";
				id++;
			}

			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class AverageMemoryPerUserPerBackendCommand : public AdminInterfaceCommand {
	public:

		AverageMemoryPerUserPerBackendCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("average_memory_per_user_per_backend",
												AdminInterfaceCommand::Memory,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("(memory_used_without_any_user - res_memory) per backend");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

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

			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class CrashedBackendsCountCommand : public AdminInterfaceCommand {
	public:

		CrashedBackendsCountCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("crashed_backends_count",
												AdminInterfaceCommand::Backends,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Returns number of crashed backends");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			return boost::lexical_cast<std::string>(m_server->getCrashedBackends().size());
		}

	private:
		NetworkPluginServer *m_server;
};

class CrashedBackendsCommand : public AdminInterfaceCommand {
	public:

		CrashedBackendsCommand(NetworkPluginServer *server) :
												AdminInterfaceCommand("crashed_backends",
												AdminInterfaceCommand::Backends,
												AdminInterfaceCommand::GlobalContext,
												AdminInterfaceCommand::AdminMode,
												AdminInterfaceCommand::Get) {
			m_server = server;
			setDescription("Returns IDs of crashed backends");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string lst;
			const std::vector<std::string> &backends = m_server->getCrashedBackends();
			BOOST_FOREACH(const std::string &backend, backends) {
				lst += backend + "\n";
			}
			return lst;
		}

	private:
		NetworkPluginServer *m_server;
};

class MessagesFromXMPPCommand : public AdminInterfaceCommand {
	public:

		MessagesFromXMPPCommand(UserManager *userManager) : AdminInterfaceCommand("messages_from_xmpp",
							AdminInterfaceCommand::Messages,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Get) {
			m_userManager = userManager;
			setDescription("Returns number of messages received from frontend network");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			int msgCount = m_userManager->getMessagesToBackend();
			return boost::lexical_cast<std::string>(msgCount);
		}

	private:
		UserManager *m_userManager;
};

class MessagesToXMPPCommand : public AdminInterfaceCommand {
	public:

		MessagesToXMPPCommand(UserManager *userManager) : AdminInterfaceCommand("messages_to_xmpp",
							AdminInterfaceCommand::Messages,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Get) {
			m_userManager = userManager;
			setDescription("Returns number of messages sent to Front network");
		}

		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleGetRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			int msgCount = m_userManager->getMessagesToXMPP();
			return boost::lexical_cast<std::string>(msgCount);
		}

	private:
		UserManager *m_userManager;
};

class RegisterCommand : public AdminInterfaceCommand {
	public:
		RegisterCommand(UserRegistration *userRegistration, Component *component) : AdminInterfaceCommand("register",
							AdminInterfaceCommand::Users,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::UserMode,
							AdminInterfaceCommand::Execute,
							"Register") {
			m_userRegistration = userRegistration;
			setDescription("Registers the new user");

			std::string fields = component->getFrontend()->getRegistrationFields();
			std::vector<std::string> args;
			boost::split(args, fields, boost::is_any_of("\n"));
			addArg("username", args[0]);
			if (fields.size() > 1) {
				addArg("legacy_username", args[1]);
			}
			if (fields.size() > 2) {
				addArg("legacy_password", args[2], "password");
			}
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (args.size() != 2 && args.size() != 3) {
				return "Error: Bad argument count";
			}

			UserInfo res;
			res.jid = args[0];
			res.uin = args[1];
			if (args.size() == 2) {
				res.password = "";
			}
			else {
				res.password = args[2];
			}
			res.language = "en";
			res.encoding = "utf-8";
			res.vip = 0;

			if (m_userRegistration->registerUser(res)) {
				return "User registered";
			}
			else {
				return "Error: User is already registered";
			}
		}

	private:
		UserRegistration *m_userRegistration;
};

class UnregisterCommand : public AdminInterfaceCommand {
	public:

		UnregisterCommand(UserRegistration *userRegistration, Component *component) : AdminInterfaceCommand("unregister",
							AdminInterfaceCommand::Users,
							AdminInterfaceCommand::UserContext,
							AdminInterfaceCommand::UserMode,
							AdminInterfaceCommand::Execute,
							"Unregister") {
			m_userRegistration = userRegistration;
			setDescription("Unregisters existing user");

// 			std::string fields = component->getFrontend()->getRegistrationFields();
// 			std::vector<std::string> args;
// 			boost::split(args, fields, boost::is_any_of("\n"));
// 			addArg("username", args[0]);
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (m_userRegistration->unregisterUser(uinfo.jid)) {
				return "User '" + args[0] + "' unregistered.";
			}
			else {
				return "Error: User '" + args[0] + "' is not registered";
			}
		}

	private:
		UserRegistration *m_userRegistration;
};

class SetOAuth2CodeCommand : public AdminInterfaceCommand {
	public:

		SetOAuth2CodeCommand(Component *component) : AdminInterfaceCommand("set_oauth2_code",
							AdminInterfaceCommand::Frontend,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute) {
			m_component = component;
			setDescription("set_oauth2_code <code> <state> - sets the OAuth2 code and state for this instance");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			if (args.size() != 2) {
				return "Error: Bad argument count";
			}

			ret = m_component->getFrontend()->setOAuth2Code(args[0], args[1]);
			if (ret.empty()) {
				return ret;

			}
			return "OAuth2 code and state set.";
		}

	private:
		Component *m_component;
};

class GetOAuth2URLCommand : public AdminInterfaceCommand {
	public:

		GetOAuth2URLCommand(Component *component) : AdminInterfaceCommand("get_oauth2_url",
							AdminInterfaceCommand::Frontend,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute) {
			m_component = component;
			setDescription("get_oauth2_code - Get OAUth2 URL");
			std::string fields = component->getFrontend()->getRegistrationFields();
			std::vector<std::string> args;
			boost::split(args, fields, boost::is_any_of("\n"));
			addArg("username", args[0]);
			if (fields.size() > 1) {
				addArg("legacy_username", args[1]);
			}
			if (fields.size() > 2) {
				addArg("legacy_password", args[2], "password");
			}
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string url = m_component->getFrontend()->getOAuth2URL(args);
			return url;
		}

	private:
		Component *m_component;
};

class HelpCommand : public AdminInterfaceCommand {
	public:

		HelpCommand(std::map<std::string, AdminInterfaceCommand *> *commands) : AdminInterfaceCommand("help",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute,
							"Help") {
			m_commands = commands;
			setDescription("Shows help message");
		}

		void generateCategory(AdminInterfaceCommand::Category category, std::string &output) {
			output += getCategoryName(category) + ":\n";

			for (std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands->begin(); it != m_commands->end(); it++) {
				AdminInterfaceCommand *command = it->second;
				if (command->getCategory() != category) {
					continue;
				}

				if (command->getActions() & AdminInterfaceCommand::Execute) {
					output += "   CMD   ";
				}
				else {
					output += "   VAR   ";
				}

				output += command->getName() + " - ";

				output += command->getDescription() + "\n";
			}
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string help;
			generateCategory(AdminInterfaceCommand::General, help);
			generateCategory(AdminInterfaceCommand::Users, help);
			generateCategory(AdminInterfaceCommand::Messages, help);
			generateCategory(AdminInterfaceCommand::Frontend, help);
			generateCategory(AdminInterfaceCommand::Backends, help);
			generateCategory(AdminInterfaceCommand::Memory, help);
			return help;
		}

	private:
		std::map<std::string, AdminInterfaceCommand *> *m_commands;
};

class CommandsCommand : public AdminInterfaceCommand {
	public:

		CommandsCommand(std::map<std::string, AdminInterfaceCommand *> *commands) : AdminInterfaceCommand("commands",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute,
							"Available commands") {
			m_commands = commands;
			setDescription("Shows all the available commands with extended information.");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string output;
			for (std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands->begin(); it != m_commands->end(); it++) {
				AdminInterfaceCommand *command = it->second;
				if ((command->getActions() & AdminInterfaceCommand::Execute) == 0) {
					continue;
				}

				output += command->getName();
				output += " - \"" + command->getDescription() + "\"";
				output += " Category: " + command->getCategoryName(command->getCategory());

				output += " AccesMode:";
				if (command->getAccessMode() == AdminInterfaceCommand::UserMode) {
					output += " User";
				}
				else {
					output += " Admin";
				}

				output += " Context:";
				if (command->getContext() == AdminInterfaceCommand::UserContext) {
					output += " User";
				}
				else {
					output += " Global";
				}

				output += " Label: \"" + (command->getLabel().empty() ? command->getName() : command->getLabel()) + "\"";

				output += "\n";
			}

			return output;
		}

	private:
		std::map<std::string, AdminInterfaceCommand *> *m_commands;
};


class VariablesCommand : public AdminInterfaceCommand {
	public:

		VariablesCommand(std::map<std::string, AdminInterfaceCommand *> *commands) : AdminInterfaceCommand("variables",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute,
							"Available variables") {
			m_commands = commands;
			setDescription("Shows all the available variables.");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::string output;
			for (std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands->begin(); it != m_commands->end(); it++) {
				AdminInterfaceCommand *command = it->second;
				if ((command->getActions() & AdminInterfaceCommand::Get) == 0) {
					continue;
				}

				output += command->getName();
				output += " - \"" + command->getDescription() + "\"";
				output += " Value: \"" + command->handleGetRequest(uinfo, user, args) + "\"";

				if ((command->getActions() & AdminInterfaceCommand::Set) == 0) {
					output += " Read-only: true";
				}
				else {
					output += " Read-only: false";
				}

				output += " Category: " + command->getCategoryName(command->getCategory());

				output += " AccesMode:";
				if (command->getAccessMode() == AdminInterfaceCommand::UserMode) {
					output += " User";
				}
				else {
					output += " Admin";
				}

				output += " Context:";
				if (command->getContext() == AdminInterfaceCommand::UserContext) {
					output += " User";
				}
				else {
					output += " Global";
				}

				output += "\n";
			}

			return output;
		}

	private:
		std::map<std::string, AdminInterfaceCommand *> *m_commands;
};

class ArgsCommand : public AdminInterfaceCommand {
	public:

		ArgsCommand(std::map<std::string, AdminInterfaceCommand *> *commands) : AdminInterfaceCommand("args",
							AdminInterfaceCommand::General,
							AdminInterfaceCommand::GlobalContext,
							AdminInterfaceCommand::AdminMode,
							AdminInterfaceCommand::Execute, "Command's arguments") {
			m_commands = commands;
			setDescription("Shows descripton of arguments for command");
			addArg("command", "Command", "string", "register");
		}

		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
			std::string ret = AdminInterfaceCommand::handleExecuteRequest(uinfo, user, args);
			if (!ret.empty()) {
				return ret;
			}

			std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands->find(args[0]);
			if (it == m_commands->end()) {
				return "Error: Unknown command passed as an argument.";
			}
			AdminInterfaceCommand *command = it->second;

			BOOST_FOREACH(const AdminInterfaceCommand::Arg &arg, command->getArgs()) {
				ret += arg.name + " - \"" + arg.label + "\" " + "Example: \"" + arg.example + "\" Type: \"" + arg.type + "\"\n";
			}

			return ret;
		}

	private:
		std::map<std::string, AdminInterfaceCommand *> *m_commands;
};


AdminInterface::AdminInterface(Component *component, UserManager *userManager, NetworkPluginServer *server, StorageBackend *storageBackend, UserRegistration *userRegistration) {
	m_component = component;
	m_storageBackend = storageBackend;
	m_userManager = userManager;
	m_server = server;
	m_userRegistration = userRegistration;

	m_component->getFrontend()->onMessageReceived.connect(bind(&AdminInterface::handleMessageReceived, this, _1));

	addCommand(new StatusCommand(m_server, m_userManager));
	addCommand(new UptimeCommand());
	addCommand(new OnlineUsersCommand(m_userManager));
	addCommand(new OnlineUsersCountCommand(m_userManager));
	addCommand(new ReloadCommand(m_component));
// 	addCommand(new OnlineUsersPerBackendCommand(m_server));
	addCommand(new HasOnlineUserCommand(m_userManager));
	addCommand(new BackendsCountCommand(m_server));
	addCommand(new ResMemoryCommand(m_server));
	addCommand(new ShrMemoryCommand(m_server));
	addCommand(new UsedMemoryCommand(m_server));
	addCommand(new AverageMemoryPerUserCommand(m_server, m_userManager));
// 	addCommand(new ResMemoryPerBackendCommand(m_server));
// 	addCommand(new ShrMemoryPerBackendCommand(m_server));
// 	addCommand(new UsedMemoryPerBackendCommand(m_server));
// 	addCommand(new AverageMemoryPerUserPerBackendCommand(m_server));
	addCommand(new CrashedBackendsCountCommand(m_server));
	addCommand(new CrashedBackendsCommand(m_server));
	addCommand(new MessagesFromXMPPCommand(m_userManager));
	addCommand(new MessagesToXMPPCommand(m_userManager));
	addCommand(new SetOAuth2CodeCommand(m_component));
	addCommand(new GetOAuth2URLCommand(m_component));
	addCommand(new HelpCommand(&m_commands));
	addCommand(new ArgsCommand(&m_commands));
	addCommand(new CommandsCommand(&m_commands));
	addCommand(new VariablesCommand(&m_commands));

	if (m_userRegistration) {
		addCommand(new RegisterCommand(m_userRegistration, m_component));
		addCommand(new UnregisterCommand(m_userRegistration, m_component));
	}
}

AdminInterface::~AdminInterface() {
	for (std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands.begin(); it != m_commands.end(); it++) {
		delete it->second;
	}
}

void AdminInterface::addCommand(AdminInterfaceCommand *command) {
	m_commands[command->getName()] = command;
}

void AdminInterface::handleQuery(Swift::Message::ref message) {
#if HAVE_SWIFTEN_3
	std::string msg = message->getBody().get_value_or("");
#else
	std::string msg = message->getBody();
#endif
	LOG4CXX_INFO(adminInterfaceLogger, "Message from admin received: '" << msg << "'");
	message->setTo(message->getFrom());
	message->setFrom(m_component->getJID());

	std::string body = msg;
	std::vector<std::string> args;
	boost::split(args, body, boost::is_any_of(" "));

	if (args.empty()) {
		message->setBody("Error: Unknown variable or command");
		return;
	}

	// Check for 'get' and 'set' command.
	AdminInterfaceCommand::Actions action = AdminInterfaceCommand::None;
	if (args[0] == "set") {
		action = AdminInterfaceCommand::Set;
		args.erase(args.begin());
	}
	else if (args[0] == "get") {
		action = AdminInterfaceCommand::Get;
		args.erase(args.begin());
	}

	// We removed the 'get' and 'set' from args, so check we have more
	// data there.
	if (args.empty()) {
		message->setBody("Error: Unknown variable or command");
		return;
	}

	// Find the right AdminInterfaceCommand
	std::map<std::string, AdminInterfaceCommand *>::iterator it = m_commands.find(args[0]);
	if (it == m_commands.end()) {
		message->setBody("Error: Unknown variable or command");
		return;
	}
	AdminInterfaceCommand *command = it->second;
	args.erase(args.begin());

	// If action is None, then it's Get or Execute according to command type.
	if (action == AdminInterfaceCommand::None) {
		if (command->getActions() & AdminInterfaceCommand::Get) {
			action = AdminInterfaceCommand::Get;
		}
		else if (command->getActions() & AdminInterfaceCommand::Execute) {
			action = AdminInterfaceCommand::Execute;
		}
	}

	User *user = NULL;
	UserInfo uinfo;
	uinfo.id = -1;

	if (command->getContext() == AdminInterfaceCommand::UserContext) {
		if (args.empty()) {
			message->setBody("Error: No username given");
			return;
		}

		if (!m_storageBackend || !m_storageBackend->getUser(args[0], uinfo)) {
			uinfo.id = -1;
		}

		user = m_userManager->getUser(args[0]);
		args.erase(args.begin());
	}

	// Execute the command
	switch (action) {
		case AdminInterfaceCommand::None:
			message->setBody("Error: Unknown variable or command");
			break;
		case AdminInterfaceCommand::Get:
			message->setBody(command->handleGetRequest(uinfo, user, args));
			break;
		case AdminInterfaceCommand::Set:
			message->setBody(command->handleSetRequest(uinfo, user, args));
			break;
		case AdminInterfaceCommand::Execute:
			message->setBody(command->handleExecuteRequest(uinfo, user, args));
			break;
	}

	return;
// 	else if (m_component->getFrontend()->handleAdminMessage(message)) {
// 		LOG4CXX_INFO(adminInterfaceLogger, "Message handled by frontend");
// 	}
}

void AdminInterface::handleMessageReceived(Swift::Message::ref message) {
	if (!message->getTo().getNode().empty())
		return;

	std::vector<std::string> const &x = CONFIG_VECTOR(m_component->getConfig(),"service.admin_jid");
	if (std::find(x.begin(), x.end(), message->getFrom().toBare().toString()) == x.end()) {
	    LOG4CXX_WARN(adminInterfaceLogger, "Message not from admin user, but from " << message->getFrom().toBare().toString());
	    return;

	}

	// Ignore empty messages
#if HAVE_SWIFTEN_3
	if (message->getBody().get_value_or("").empty()) {
		return;
	}
#else
	if (message->getBody().empty()) {
		return;
	}
#endif

	handleQuery(message);

	m_component->getFrontend()->sendMessage(message);
}

}
