// Transport includes
#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

// Swiften
#include "Swiften/Swiften.h"

#ifndef _WIN32
// for signal handler
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#endif
// Boost
#include <boost/algorithm/string.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Backend Template");

// eventloop
Swift::SimpleEventLoop *loop_;

// Plugin
class TemplatePlugin;
TemplatePlugin * np = NULL;

class TemplatePlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;

		TemplatePlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&TemplatePlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));

			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		// NetworkPlugin uses this method to send the data to networkplugin server
		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		// This method has to call handleDataRead with all received data from network plugin server
		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			handleConnected(user);
			LOG4CXX_INFO(logger, user << ": Added buddy - Echo.");
			handleBuddyChanged(user, "echo", "Echo", std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") {
			LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");
			if (legacyName == "echo") {
				handleMessage(user, legacyName, message);
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}

	private:
		Config *config;
};

#ifndef _WIN32

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
#endif

int main (int argc, char* argv[]) {
	std::string host;
	int port;

#ifndef _WIN32
	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}
#endif
	std::string configFile;
	boost::program_options::variables_map vm;
	boost::program_options::options_description desc("Usage: spectrum <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help", "help")
		("host,h", boost::program_options::value<std::string>(&host)->default_value(""), "Host to connect to")
		("port,p", boost::program_options::value<int>(&port)->default_value(10000), "Port to connect to")
		("config", boost::program_options::value<std::string>(&configFile)->default_value(""), "Config file")
		;

	try
	{
		boost::program_options::positional_options_description p;
		p.add("config", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
			options(desc).positional(p).allow_unregistered().run(), vm);
		boost::program_options::notify(vm);
			
		if(vm.count("help"))
		{
			std::cout << desc << "\n";
			return 1;
		}

		if(vm.count("config") == 0) {
			std::cout << desc << "\n";
			return 1;
		}
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		return 1;
	}
	catch (...)
	{
		std::cout << desc << "\n";
		return 1;
	}

	Config config(argc, argv);
	if (!config.load(configFile)) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Logging::initBackendLogging(&config);

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new TemplatePlugin(&config, &eventLoop, host, port);
	loop_->run();

	return 0;
}
