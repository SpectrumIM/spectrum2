#include "managerconfig.h"
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
#include "signal.h"
#include "sys/wait.h"

#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);


using namespace Transport;

using namespace boost::filesystem;

using namespace boost;

std::string _data;

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
static unsigned long exec_(std::string path, std::string config, std::string jid = "") {
	// fork and exec
	pid_t pid = fork();
	if ( pid == 0 ) {
		// child process
		if (jid.empty()) {
			exit(execl(path.c_str(), path.c_str(), config.c_str(), NULL));
		}
		else {
			exit(execl(path.c_str(), path.c_str(), "-j", jid.c_str(), config.c_str(), NULL));
		}
	} else if ( pid < 0 ) {
		// fork failed
	}
	else {
		waitpid(pid, 0, 0);
	}

	return (unsigned long) pid;
}

static int getPort(const std::string &portfile) {
	path p(portfile);
	if (!exists(p) || is_directory(p)) {
		return 0;
	}

	std::ifstream f(p.string().c_str(), std::ios_base::in);
	std::string port;
	f >> port;

	if (port.empty())
		return 0;

	return boost::lexical_cast<int>(port);
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

static void start_instances(ManagerConfig *config, const std::string &_jid = "") {
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
					continue;
				}

				
				std::vector<std::string> vhosts;
				if (CONFIG_HAS_KEY(&cfg, "vhosts.vhost"))
					vhosts = CONFIG_VECTOR(&cfg, "vhosts.vhost");
				vhosts.push_back(CONFIG_STRING(&cfg, "service.jid"));

				BOOST_FOREACH(std::string &vhost, vhosts) {
					Config vhostCfg;
					if (vhostCfg.load(itr->path().string(), vhost) == false) {
						std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
						continue;
					}

					if (!_jid.empty() && _jid != vhost) {
						continue;
					}

					int pid = isRunning(CONFIG_STRING(&vhostCfg, "service.pidfile"));
					if (pid == 0) {
						std::cout << "Starting " << itr->path() << ": OK\n";
						exec_(spectrum2_binary, itr->path().string(), vhost);
					}
					else {
						std::cout << "Starting " << itr->path() << ": Already started (PID=" << pid << ")\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

static void stop_instances(ManagerConfig *config, const std::string &_jid = "") {
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

				std::vector<std::string> vhosts;
				if (CONFIG_HAS_KEY(&cfg, "vhosts.vhost"))
					vhosts = CONFIG_VECTOR(&cfg, "vhosts.vhost");
				vhosts.push_back(CONFIG_STRING(&cfg, "service.jid"));

				BOOST_FOREACH(std::string &vhost, vhosts) {
					Config vhostCfg;
					if (vhostCfg.load(itr->path().string(), vhost) == false) {
						std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
						continue;
					}

					if (!_jid.empty() && _jid != vhost) {
						continue;
					}

					int pid = isRunning(CONFIG_STRING(&vhostCfg, "service.pidfile"));
					if (pid) {
						std::cout << "Stopping " << itr->path() << ": ";
						kill(pid, SIGTERM);

						sleep(1);
						int count = 20;
						while (kill(pid, 0) == 0 && count != 0) {
							std::cout << ".";
							sleep(1);
							count--;
						}
						if (count == 0) {
							std::cout << " ERROR (timeout)\n";
						}
						else {
							std::cout << " OK\n";
						}
					}
					else {
						std::cout << "Stopping " << itr->path() << ": Not running\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

static int show_status(ManagerConfig *config) {
	int ret = 0;
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

				std::vector<std::string> vhosts;
				if (CONFIG_HAS_KEY(&cfg, "vhosts.vhost"))
					vhosts = CONFIG_VECTOR(&cfg, "vhosts.vhost");
				vhosts.push_back(CONFIG_STRING(&cfg, "service.jid"));

				BOOST_FOREACH(std::string &vhost, vhosts) {
					Config vhostCfg;
					if (vhostCfg.load(itr->path().string(), vhost) == false) {
						std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
						continue;
					}

					int pid = isRunning(CONFIG_STRING(&vhostCfg, "service.pidfile"));
					if (pid) {
						std::cout << itr->path() << ": " << vhost << " Running\n";
					}
					else {
						ret = 3;
						std::cout << itr->path() << ": " << vhost << " Stopped\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
	return ret;
}

static void handleDataRead(boost::shared_ptr<Swift::Connection> m_conn, boost::shared_ptr<Swift::SafeByteArray> data) {
	_data += std::string(data->begin(), data->end());

	// Parse data while there are some
	while (_data.size() != 0) {
		// expected_size of wrapper message
		unsigned int expected_size;

		// if data is >= 4, we have whole header and we can
		// read expected_size.
		if (_data.size() >= 4) {
			expected_size = *((unsigned int*) &_data[0]);
			expected_size = ntohl(expected_size);
			// If we don't have whole wrapper message, wait for next
			// handleDataRead call.
			if (_data.size() - 4 < expected_size)
				return;
		}
		else {
			return;
		}

		// Parse wrapper message and erase it from buffer.
		pbnetwork::WrapperMessage wrapper;
		if (wrapper.ParseFromArray(&_data[4], expected_size) == false) {
			std::cout << "PARSING ERROR " << expected_size << "\n";
			_data.erase(_data.begin(), _data.begin() + 4 + expected_size);
			continue;
		}
		_data.erase(_data.begin(), _data.begin() + 4 + expected_size);

		if (wrapper.type() == pbnetwork::WrapperMessage_Type_TYPE_QUERY) {
			pbnetwork::BackendConfig payload;
			if (payload.ParseFromString(wrapper.payload()) == false) {
				std::cout << "PARSING ERROR\n";
				// TODO: ERROR
				continue;
			}

			std::cout << payload.config() << "\n";
			exit(0);
		}
	}
}

static void handleConnected(boost::shared_ptr<Swift::Connection> m_conn, const std::string &msg, bool error) {
	if (error) {
		std::cerr << "Can't connect the server\n";
		exit(50);
	}
	else {
		pbnetwork::BackendConfig m;
		m.set_config(msg);

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_QUERY);

		uint32_t size = htonl(message.size());
		char *header = (char *) &size;

		
		// send header together with wrapper message
		m_conn->write(Swift::createSafeByteArray(std::string(header, 4) + message));
	}
}

static void ask_local_server(ManagerConfig *config, Swift::BoostNetworkFactories &networkFactories, const std::string &jid, const std::string &message) {
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

		bool found = false;
		directory_iterator end_itr;
		for (directory_iterator itr(p); itr != end_itr; ++itr) {
			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
				Config cfg;
				if (cfg.load(itr->path().string()) == false) {
					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
					continue;
				}

				if (CONFIG_STRING(&cfg, "service.jid") != jid) {
					continue;
				}

				found = true;

				boost::shared_ptr<Swift::Connection> m_conn;
				m_conn = networkFactories.getConnectionFactory()->createConnection();
				m_conn->onDataRead.connect(boost::bind(&handleDataRead, m_conn, _1));
				m_conn->onConnectFinished.connect(boost::bind(&handleConnected, m_conn, message, _1));
				m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(CONFIG_STRING(&cfg, "service.backend_host")), getPort(CONFIG_STRING(&cfg, "service.portfile"))));

// 				finished++;
// 				Swift::Client *client = new Swift::Client(CONFIG_VECTOR(&cfg, "service.admin_jid")[0], CONFIG_STRING(&cfg, "service.admin_password"), &networkFactories);
// 				client->setAlwaysTrustCertificates();
// 				client->onConnected.connect(boost::bind(&handleConnected, client, CONFIG_STRING(&cfg, "service.jid")));
// 				client->onDisconnected.connect(bind(&handleDisconnected, client, _1, CONFIG_STRING(&cfg, "service.jid")));
// 				client->onMessageReceived.connect(bind(&handleMessageReceived, client, _1, CONFIG_STRING(&cfg, "service.jid")));
// 				Swift::ClientOptions opt;
// 				opt.allowPLAINWithoutTLS = true;
// 				client->connect(opt);
			}
		}

		if (!found) {
			std::cerr << "Config file for Spectrum instance with this JID was not found\n";
			exit(20);
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

static void show_list(ManagerConfig *config) {
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

		bool found = false;
		directory_iterator end_itr;
		for (directory_iterator itr(p); itr != end_itr; ++itr) {
			if (is_regular(itr->path()) && extension(itr->path()) == ".cfg") {
				Config cfg;
				if (cfg.load(itr->path().string()) == false) {
					std::cerr << "Can't load config file " << itr->path().string() << ". Skipping...\n";
					continue;
				}

				std::cout << CONFIG_STRING(&cfg, "service.jid") << "\n";
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "boost filesystem error\n";
		exit(5);
	}
}

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
		start_instances(&config);
	}
	else if (command[0] == "stop") {
		stop_instances(&config);
	}
	else if (command[0] == "status") {
		return show_status(&config);
	}
	else if (command[0] == "list") {
		show_list(&config);
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
			start_instances(&config, jid);
			return 0;
		}
		else if (cmd == "stop") {
			stop_instances(&config, jid);
			return 0;
		}

		ask_local_server(&config, networkFactories, jid, cmd);
// 		std::string message = command;
// 		m = &message;

// 		ask_local_server(&config, networkFactories, message);

		eventLoop.run();
	}
}
