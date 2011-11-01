#include "managerconfig.h"
#include "transport/config.h"
#include "Swiften/Swiften.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>
#include "signal.h"
#include "sys/wait.h"


using namespace Transport;

using namespace boost::filesystem;

using namespace boost;

static int finished;
static std::string *m;

static void handleDisconnected(Swift::Client *client, const boost::optional<Swift::ClientError> &, const std::string &server) {
	std::cout << "[ DISCONNECTED ] " << server << "\n";
	if (--finished == 0) {
		exit(0);
	}
}

static void handleConnected(Swift::Client *client, const std::string &server) {
	boost::shared_ptr<Swift::Message> message(new Swift::Message());
	message->setTo(server);
	message->setFrom(client->getJID());
	message->setBody(*m);

	client->sendMessage(message);
}

static void handleMessageReceived(Swift::Client *client, Swift::Message::ref message, const std::string &server) {
	std::string body = message->getBody();
	boost::replace_all(body, "\n", "\n[      OK      ] " + server + ": ");
	std::cout << "[      OK      ] " << server << ": " << body <<  "\n";
	if (--finished == 0) {
		exit(0);
	}
}

static std::string searchForBinary(const std::string &binary) {
	std::vector<std::string> path_list;
	char * env_path = getenv("PATH");

	if (env_path != NULL) {
		std::string buffer = "";
		for (int s = 0; s < strlen(env_path); s++) {
			if (env_path[s] == ':') {
				path_list.insert(path_list.end(), std::string(buffer));
				buffer = "";
			}
			else {
				buffer += env_path[s];
			}
		}

		if (buffer != "") {
			path_list.insert(path_list.end(), std::string(buffer));
			buffer = "";
		}

		for (std::vector<std::string>::iterator dit = path_list.begin(); dit < path_list.end(); dit++) {
			std::string bpath = *dit;
			bpath += "/";
			bpath += binary;
			path p(bpath);
			if (exists(p) && !is_directory(p)) {
				return bpath;
			}
		}
	}
	return "";
}

// Executes new backend
static unsigned long exec_(std::string path, std::string config) {
	// fork and exec
	pid_t pid = fork();
	if ( pid == 0 ) {
		// child process
		exit(execl(path.c_str(), path.c_str(), config.c_str(), NULL));
	} else if ( pid < 0 ) {
		// fork failed
	}
	else {
		waitpid(pid, 0, 0);
	}

	return (unsigned long) pid;
}

static int isRunning(const std::string &pidfile) {
	path p(pidfile);
	if (!exists(p) || is_directory(p)) {
		return 0;
	}

	std::ifstream f(p.string().c_str(), std::ios_base::in);
	std::string pid;
	f >> pid;

	if (pid.empty())
		return 0;

	if (kill(boost::lexical_cast<int>(pid), 0) != 0)
		return 0;

	return boost::lexical_cast<int>(pid);
}

static void start_all_instances(ManagerConfig *config) {
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(6);
		}

		if (!is_directory(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(7);
		}

		std::string spectrum2_binary = searchForBinary("spectrum2");
		if (spectrum2_binary.empty()) {
			std::cerr << "spectrum2 binary not found in PATH\n";
			exit(8);
		}

		directory_iterator end_itr;
		for (directory_iterator itr(p); itr != end_itr; ++itr) {
			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
				Config cfg;
				if (cfg.load(itr->path().string()) == false) {
					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
				}

				int pid = isRunning(CONFIG_STRING(&cfg, "service.pidfile"));
				if (pid == 0) {
					std::cout << "Starting " << itr->path() << ": OK\n";
					exec_(spectrum2_binary, itr->path().string());
				}
				else {
					std::cout << "Starting " << itr->path() << ": Already started (PID=" << pid << ")\n";
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

static void stop_all_instances(ManagerConfig *config) {
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(6);
		}

		if (!is_directory(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(7);
		}

		directory_iterator end_itr;
		for (directory_iterator itr(p); itr != end_itr; ++itr) {
			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
				Config cfg;
				if (cfg.load(itr->path().string()) == false) {
					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
				}

				int pid = isRunning(CONFIG_STRING(&cfg, "service.pidfile"));
				if (pid) {
					std::cout << "Stopping " << itr->path() << ": OK\n";
					kill(pid, SIGTERM);
				}
				else {
					std::cout << "Stopping " << itr->path() << ": Not running\n";
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

void ask_local_servers(ManagerConfig *config, Swift::BoostNetworkFactories &networkFactories, const std::string &message) {
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(6);
		}

		if (!is_directory(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			exit(7);
		}

		directory_iterator end_itr;
		for (directory_iterator itr(p); itr != end_itr; ++itr) {
			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
				Config cfg;
				if (cfg.load(itr->path().string()) == false) {
					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
				}

				finished++;
				Swift::Client *client = new Swift::Client(CONFIG_STRING(&cfg, "service.admin_username"), CONFIG_STRING(&cfg, "service.admin_password"), &networkFactories);
				client->setAlwaysTrustCertificates();
				client->onConnected.connect(boost::bind(&handleConnected, client, CONFIG_STRING(&cfg, "service.jid")));
				client->onDisconnected.connect(bind(&handleDisconnected, client, _1, CONFIG_STRING(&cfg, "service.jid")));
				client->onMessageReceived.connect(bind(&handleMessageReceived, client, _1, CONFIG_STRING(&cfg, "service.jid")));
				Swift::ClientOptions opt;
				opt.allowPLAINWithoutTLS = true;
				client->connect(opt);
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}


int main(int argc, char **argv)
{
	ManagerConfig config;
	std::string config_file;
	std::string command;
	boost::program_options::variables_map vm;

	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <COMMAND>\nCommands:\n"
													 " start - start all local Spectrum2 instances\n"
													 " stop  - stop all  local Spectrum2 instances\n"
													 " <other> - send command to all local + remote Spectrum2 instances and print output\n"
													 "Allowed options");
	desc.add_options()
		("help,h", "Show help output")
		("config,c", boost::program_options::value<std::string>(&config_file)->default_value("/etc/spectrum2/spectrum_manager.cfg"), "Spectrum manager config file")
		("command", boost::program_options::value<std::string>(&command)->default_value(""), "Command")
		;
	try
	{
		boost::program_options::positional_options_description p;
		p.add("command", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
		boost::program_options::notify(vm);

		if(vm.count("help"))
		{
			std::cout << desc << "\n";
			return 1;
		}
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		return 2;
	}
	catch (...)
	{
		std::cout << desc << "\n";
		return 3;
	}

	if (!config.load(config_file)) {
		std::cerr << "Can't load configuration file.\n";
		return 4;
	}

	if (command.empty()) {
		std::cout << desc << "\n";
		return 1;
	}

	if (command == "start") {
		start_all_instances(&config);
	}
	else if (command == "stop") {
		stop_all_instances(&config);
	}
	else {
		Swift::SimpleEventLoop eventLoop;
		Swift::BoostNetworkFactories networkFactories(&eventLoop);

		std::string message = command;
		m = &message;

		ask_local_servers(&config, networkFactories, message);

		std::vector<std::string> servers = CONFIG_VECTOR(&config, "servers.server");
		for (std::vector<std::string>::const_iterator it = servers.begin(); it != servers.end(); it++) {
			finished++;
			Swift::Client *client = new Swift::Client(CONFIG_STRING(&config, "service.admin_username") + "@" + *it, CONFIG_STRING(&config, "service.admin_password"), &networkFactories);
			client->setAlwaysTrustCertificates();
			client->onConnected.connect(boost::bind(&handleConnected, client, *it));
			client->onDisconnected.connect(bind(&handleDisconnected, client, _1, *it));
			client->onMessageReceived.connect(bind(&handleMessageReceived, client, _1, *it));
			Swift::ClientOptions opt;
			opt.allowPLAINWithoutTLS = true;
			client->connect(opt);
	// 		std::cout << *it << "\n";
		}

		eventLoop.run();
	}
}
