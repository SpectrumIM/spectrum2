#include "managerconfig.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/networkpluginserver.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

static int finished;

static void handleDisconnected(Swift::Client *client, const boost::optional<Swift::ClientError> &) {
	std::cout << "[ DISCONNECTED ] " << client->getJID().getDomain() << "\n";
	if (--finished == 0) {
		exit(0);
	}
}

static void handleConnected(Swift::Client *client) {
	std::cout << "[      OK      ] " << client->getJID().getDomain() << "\n";
	if (--finished == 0) {
		exit(0);
	}
}

int main(int argc, char **argv)
{
	ManagerConfig config;

	boost::program_options::options_description desc("Usage: spectrum_manager <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help,h", "help")
		;
	try
	{
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
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
		return 1;
	}
	catch (...)
	{
		std::cout << desc << "\n";
		return 1;
	}

	if (argc != 2) {
		std::cout << desc << "\n";
		return 1;
	}


	if (!config.load(argv[1])) {
		std::cerr << "Can't load configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	Swift::BoostNetworkFactories networkFactories(&eventLoop);

	std::vector<std::string> servers = CONFIG_VECTOR(&config, "servers.server");
	for (std::vector<std::string>::const_iterator it = servers.begin(); it != servers.end(); it++) {
		finished++;
		Swift::Client *client = new Swift::Client(CONFIG_STRING(&config, "service.admin_username") + "@" + (*it), CONFIG_STRING(&config, "service.admin_password"), &networkFactories);
		client->setAlwaysTrustCertificates();
 		client->onConnected.connect(boost::bind(&handleConnected, client));
		client->onDisconnected.connect(bind(&handleDisconnected, client, _1));
// 		client->onMessageReceived.connect(bind(&handleMessageReceived, _1));
		Swift::ClientOptions opt;
		opt.allowPLAINWithoutTLS = true;
		client->connect(opt);
// 		std::cout << *it << "\n";
	}

	eventLoop.run();
}
