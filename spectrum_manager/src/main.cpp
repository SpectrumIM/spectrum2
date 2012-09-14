#include "managerconfig.h"
#include "methods.h"
#include "server.h"
#include "transport/config.h"
#include "transport/protocol.pb.h"
#include "Swiften/Swiften.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include "signal.h"
#include "sys/wait.h"


using namespace Transport;

using namespace boost::filesystem;

using namespace boost;


// static void ask_local_servers(ManagerConfig *config, Swift::BoostNetworkFactories &networkFactories, const std::string &message) {
// 	path p(CONFIG_STRING(config, "service.config_directory"));
// 
// 	try {
// 		if (!exists(p)) {
// 			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
// 			exit(6);
// 		}
// 
// 		if (!is_directory(p)) {
// 			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
// 			exit(7);
// 		}
// 
// 		directory_iterator end_itr;
// 		for (directory_iterator itr(p); itr != end_itr; ++itr) {
// 			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
// 				Config cfg;
// 				if (cfg.load(itr->path().string()) == false) {
// 					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
// 					continue;
// 				}
// 
// 				if (CONFIG_VECTOR(&cfg, "service.admin_jid").empty() || CONFIG_STRING(&cfg, "service.admin_password").empty()) {
// 					std::cerr << itr->path().string() << ": service.admin_jid or service.admin_password empty. This server can't be queried over XMPP.\n";
// 					continue;
// 				}
// 
// 				finished++;
// 				Swift::Client *client = new Swift::Client(CONFIG_VECTOR(&cfg, "service.admin_jid")[0], CONFIG_STRING(&cfg, "service.admin_password"), &networkFactories);
// 				client->setAlwaysTrustCertificates();
// 				client->onConnected.connect(boost::bind(&handleConnected, client, CONFIG_STRING(&cfg, "service.jid")));
// 				client->onDisconnected.connect(bind(&handleDisconnected, client, _1, CONFIG_STRING(&cfg, "service.jid")));
// 				client->onMessageReceived.connect(bind(&handleMessageReceived, client, _1, CONFIG_STRING(&cfg, "service.jid")));
// 				Swift::ClientOptions opt;
// 				opt.allowPLAINWithoutTLS = true;
// 				client->connect(opt);
// 			}
// 		}
// 	}
// 	catch (const filesystem_error& ex) {
// 		std::cerr << "boost filesystem error\n";
// 		exit(5);
// 	}
// }


int main(int argc, char **argv)
{
	ManagerConfig config;
	std::string config_file;
	std::vector<std::string> command;
	boost::program_options::variables_map vm;

	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <COMMAND>\n"
													 "       spectrum [OPTIONS] <instance_JID> <other>\nCommands:\n"
													 " start - start all local Spectrum2 instances\n"
													 " stop  - stop all local Spectrum2 instances\n"
													 " status - status of local Spectrum2 instances\n"
													 " <other> - send command to local Spectrum2 instance and print output\n"
													 "Allowed options");
	desc.add_options()
		("help,h", "Show help output")
		("config,c", boost::program_options::value<std::string>(&config_file)->default_value("/etc/spectrum2/spectrum_manager.cfg"), "Spectrum manager config file")
		("command", boost::program_options::value<std::vector<std::string> >(&command), "Command")
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

	if (command[0] == "start") {
		return start_instances(&config);
	}
	else if (command[0] == "stop") {
		stop_instances(&config);
	}
	else if (command[0] == "status") {
		return show_status(&config);
	}
	else if (command[0] == "list") {
		std::vector<std::string> list = show_list(&config);
	}
	else if (command[0] == "server") {
		Server server(&config);
		server.start();
		while (1) { sleep(10); }
	}
	else {
		if (command.size() < 2) {
			std::cout << desc << "\n";
			return 11;
		}
		Swift::SimpleEventLoop eventLoop;
		Swift::BoostNetworkFactories networkFactories(&eventLoop);

		std::string jid = command[0];
		command.erase(command.begin());
		std::string cmd = boost::algorithm::join(command, " ");

		if (cmd == "start") {
			return start_instances(&config, jid);
		}
		else if (cmd == "stop") {
			stop_instances(&config, jid);
			return 0;
		}

		ask_local_server(&config, networkFactories, jid, cmd);
// 		std::string message = command;
// 		m = &message;

// 		ask_local_server(&config, networkFactories, message);

		eventLoop.runUntilEvents();


		struct timeval td_start,td_end;
		float elapsed = 0; 
		gettimeofday(&td_start, NULL);
	
		time_t started = time(NULL);
		while(get_response().empty()) {
			eventLoop.runUntilEvents();
		}
		if (!get_response().empty()) {
			gettimeofday(&td_end, NULL);
			elapsed = 1000000.0 * (td_end.tv_sec -td_start.tv_sec); \
			elapsed += (td_end.tv_usec - td_start.tv_usec); \
			elapsed = elapsed / 1000 / 1000; \
// 			std::cout << "Response received after " << (elapsed) << " seconds\n";
		}
	}
}
