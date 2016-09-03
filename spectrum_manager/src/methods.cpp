#include "methods.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include "signal.h"
#include "sys/wait.h"

#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);


using namespace Transport;

using namespace boost::filesystem;

std::string _data;
static std::string response;

std::string get_response() {
	return response;
}

std::string searchForBinary(const std::string &binary) {
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
unsigned long exec_(std::string path, std::string config, std::string jid, int &rc) {
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
		waitpid(pid, &rc, 0);
	}

	return (unsigned long) pid;
}

int getPort(const std::string &portfile) {
	path p(portfile);
	if (!exists(p) || is_directory(p)) {
		return 0;
	}

	std::ifstream f(p.string().c_str(), std::ios_base::in);
	std::string port;
	f >> port;

	if (port.empty())
		return 0;

	int iport;
	try {
		iport = boost::lexical_cast<int>(port);
	}
	catch (const boost::bad_lexical_cast &e) {
		std::cout << "Error: Error converting port \"" << port << "\" to integer.";
		return 0;
	}

	return iport;
}

int isRunning(const std::string &pidfile) {
	path p(pidfile);
	if (!exists(p) || is_directory(p)) {
		return 0;
	}

	std::ifstream f(p.string().c_str(), std::ios_base::in);
	std::string pid;
	f >> pid;

	if (pid.empty())
		return 0;

	int ipid = 0;
	try {
		ipid = boost::lexical_cast<int>(pid);
	}
	catch (const boost::bad_lexical_cast &e) {
		std::cout << "Error: Error converting pid \"" << pid << "\" to integer.";
		return -1;
	}

	if (kill(ipid, 0) != 0)
		return 0;

	return ipid;
}

int start_instances(ManagerConfig *config, const std::string &_jid) {
	int rv = 0;
	response = "";
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return 6;
		}

		if (!is_directory(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return 7;
		}

		std::string spectrum2_binary = searchForBinary("spectrum2");
		if (spectrum2_binary.empty()) {
			response = "Error: spectrum2 binary not found in PATH\n";
			return 8;
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
						int rc;
						response = "Starting " + itr->path().string() + ": OK\n";
						exec_(spectrum2_binary, itr->path().string(), vhost, rc);
						if (rv == 0) {
							rv = rc;
						}
					}
					else {
						response = "Starting " + itr->path().string() + ": Already started (PID=" + boost::lexical_cast<std::string>(pid) + ")\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		response = "Error: Filesystem error: " + std::string(ex.what()) + "\n";
		return 6;
	}
	return rv;
}

void stop_instances(ManagerConfig *config, const std::string &_jid) {
	response = "";
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return;
		}

		if (!is_directory(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return;
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
						response ="Stopping " + itr->path().string() + ": ";
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
							response += "ERROR (timeout)\n";
							std::cout << " ERROR (timeout)\n";
						}
						else {
							response += "OK\n";
							std::cout << " OK\n";
						}
					}
					else {
						response = "Stopping " + itr->path().string() + ": Not running\n";
						std::cout << "Stopping " << itr->path() << ": Not running\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		response = "Error: Filesystem error: " + std::string(ex.what()) + "\n";
		return;
	}
}

int restart_instances(ManagerConfig *config, const std::string &_jid) {
	response = "";
	path p(CONFIG_STRING(config, "service.config_directory"));
	int rv = 0;
	int rc;

	try {
		if (!exists(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return 1;
		}

		if (!is_directory(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return 2;
		}

		std::string spectrum2_binary = searchForBinary("spectrum2");
		if (spectrum2_binary.empty()) {
			response = "Error: spectrum2 binary not found in PATH\n";
			return 8;
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
						response ="Stopping " + itr->path().string() + ": ";
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
							response += "ERROR (timeout)\n";
							std::cout << " ERROR (timeout)\n";
						}
						else {
							response += "OK\n";
							std::cout << " OK\n";

							response = "Starting " + itr->path().string() + ": OK\n";
							std::cout << "Starting " << itr->path() << ": OK\n";
							exec_(spectrum2_binary, itr->path().string(), vhost, rc);
							if (rv == 0) {
								rv = rc;
							}
						}

						
					}
					else {
						response = "Stopping " + itr->path().string() + ": Not running\n";
						std::cout << "Stopping " << itr->path() << ": Not running\n";
					}
				}
			}
		}
	}
	catch (const filesystem_error& ex) {
		response = "Error: Filesystem error: " + std::string(ex.what()) + "\n";
		return 1;
	}

	return rv;
}

int show_status(ManagerConfig *config) {
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
		std::cerr << "Filesystem error: " << ex.what() << "\n";
		exit(5);
	}
	return ret;
}

static void handleDataRead(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::SafeByteArray> data) {
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
			m_conn->onDataRead.disconnect(boost::bind(&handleDataRead, m_conn, _1));
			m_conn->disconnect();
			response = payload.config();
			if (response.empty()) {
				response = "Empty response";
			}
			std::cout << payload.config() << "\n";
// 			exit(0);
		}
	}
}

static void handleConnected(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn, const std::string &msg, bool error) {
		m_conn->onConnectFinished.disconnect(boost::bind(&handleConnected, m_conn, msg, _1));
	if (error) {
		std::cerr << "Can't connect the server\n";
		response = "Can't connect the server\n";
		m_conn->onDataRead.disconnect(boost::bind(&handleDataRead, m_conn, _1));

// 		exit(50);
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

std::string get_config(ManagerConfig *config, const std::string &jid, const std::string &key) {
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			return "";
		}

		if (!is_directory(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			return "";
		}

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

				return CONFIG_STRING(&cfg, key);
			}
		}

	}
	catch (const filesystem_error& ex) {
		return "";
	}

	return "";
}

void ask_local_server(ManagerConfig *config, Swift::BoostNetworkFactories &networkFactories, const std::string &jid, const std::string &message) {
	response = "";
	path p(CONFIG_STRING(config, "service.config_directory"));

	try {
		if (!exists(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return;
		}

		if (!is_directory(p)) {
			response = "Error: Config directory " + CONFIG_STRING(config, "service.config_directory") + " does not exist\n";
			return;
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

				SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn;
				m_conn = networkFactories.getConnectionFactory()->createConnection();
				m_conn->onDataRead.connect(boost::bind(&handleDataRead, m_conn, _1));
				m_conn->onConnectFinished.connect(boost::bind(&handleConnected, m_conn, message, _1));
				m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(CONFIG_STRING(&cfg, "service.backend_host")), getPort(CONFIG_STRING(&cfg, "service.portfile"))));
			}
		}

		if (!found) {
			response = "Error: Config file for Spectrum instance with this JID was not found\n";
		}
	}
	catch (const filesystem_error& ex) {
		response = "Error: Filesystem error: " + std::string(ex.what()) + "\n";
	}
}

std::vector<std::string> show_list(ManagerConfig *config, bool show) {
	path p(CONFIG_STRING(config, "service.config_directory"));
	std::vector<std::string> list;

	try {
		if (!exists(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			return list;
		}

		if (!is_directory(p)) {
			std::cerr << "Config directory " << CONFIG_STRING(config, "service.config_directory") << " does not exist\n";
			return list;
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

				if (show) {
					std::cout << CONFIG_STRING(&cfg, "service.jid") << "\n";
				}
				list.push_back(CONFIG_STRING(&cfg, "service.jid"));
			}
		}
	}
	catch (const filesystem_error& ex) {
		std::cerr << "Filesystem error: " << ex.what() << "\n";
	}
	return list;
}





