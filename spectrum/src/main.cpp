#include "transport/config.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/networkpluginserver.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

int main(int argc, char **argv)
{
	Config config;

	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help,h", "help")
		("no-daemonize,n", "Do not run spectrum as daemon")
		;
	try
	{
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
		if(vm.count("help"))
		{
			std::cout << desc << "\n";
			exit(1);
		}
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		exit(1);
	}
	catch (...)
	{
		std::cout << desc << "\n";
		exit(1);
	}

	if (argc != 2) {
		std::cout << desc << "\n";
		exit(1);
	}


	if (!config.load(argv[1])) {
		std::cerr << "Can't load configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	Component transport(&eventLoop, &config, NULL);
	Logger logger(&transport);

	SQLite3Backend sql(&config);
	logger.setStorageBackend(&sql);
	if (!sql.connect()) {
		std::cerr << "Can't connect to database.\n";
	}

	UserManager userManager(&transport, &sql);
	UserRegistration userRegistration(&transport, &userManager, &sql);
	logger.setUserRegistration(&userRegistration);
	logger.setUserManager(&userManager);

	NetworkPluginServer plugin(&transport, &config, &userManager);

	transport.connect();
	eventLoop.run();
}
