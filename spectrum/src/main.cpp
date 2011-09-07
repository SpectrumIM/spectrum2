#include "transport/config.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/mysqlbackend.h"
#include "transport/userregistration.h"
#include "transport/networkpluginserver.h"
#include "transport/admininterface.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"
#ifndef WIN32
#include "sys/signal.h"
#else
#include <Windows.h>
#include <tchar.h>
#endif
#include "log4cxx/logger.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/consoleappender.h"

using namespace log4cxx;

using namespace Transport;

Swift::SimpleEventLoop *eventLoop_ = NULL;

static void spectrum_sigint_handler(int sig) {
	eventLoop_->stop();
}

static void spectrum_sigterm_handler(int sig) {
	eventLoop_->stop();
}

int main(int argc, char **argv)
{
	Config config;
#ifndef WIN32
	if (signal(SIGINT, spectrum_sigint_handler) == SIG_ERR) {
		std::cout << "SIGINT handler can't be set\n";
		return -1;
	}

	if (signal(SIGTERM, spectrum_sigterm_handler) == SIG_ERR) {
		std::cout << "SIGTERM handler can't be set\n";
		return -1;
	}
#endif
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

	if (CONFIG_STRING(&config, "logging.config").empty()) {
		LoggerPtr root = log4cxx::Logger::getRootLogger();
#ifdef WIN32
		root->addAppender(new ConsoleAppender(new PatternLayout(L"%d %-5p %c: %m%n")));
#else
		root->addAppender(new ConsoleAppender(new PatternLayout("%d %-5p %c: %m%n")));
#endif
	}
	else {
		log4cxx::PropertyConfigurator::configure(CONFIG_STRING(&config, "logging.config"));
	}

	Swift::SimpleEventLoop eventLoop;

	Swift::BoostNetworkFactories *factories = new Swift::BoostNetworkFactories(&eventLoop);
	UserRegistry userRegistry(&config, factories);

	Component transport(&eventLoop, factories, &config, NULL, &userRegistry);
// 	Logger logger(&transport);

	StorageBackend *storageBackend = NULL;

#ifdef WITH_SQLITE
	if (CONFIG_STRING(&config, "database.type") == "sqlite3") {
		storageBackend = new SQLite3Backend(&config);
		if (!storageBackend->connect()) {
			std::cerr << "Can't connect to database.\n";
			return -1;
		}
	}
#endif
#ifdef WITH_MYSQL
	if (CONFIG_STRING(&config, "database.type") == "mysql") {
		storageBackend = new MySQLBackend(&config);
		if (!storageBackend->connect()) {
			std::cerr << "Can't connect to database.\n";
			return -1;
		}
	}
#endif

	UserManager userManager(&transport, &userRegistry, storageBackend);
	UserRegistration *userRegistration = NULL;
	if (storageBackend) {
		userRegistration = new UserRegistration(&transport, &userManager, storageBackend);
		userRegistration->start();
// 		logger.setUserRegistration(&userRegistration);
	}
// 	logger.setUserManager(&userManager);

	NetworkPluginServer plugin(&transport, &config, &userManager);

	AdminInterface adminInterface(&transport, &userManager, &plugin, storageBackend);

	eventLoop_ = &eventLoop;

	eventLoop.run();
	if (userRegistration) {
		userRegistration->stop();
		delete userRegistration;
	}
	delete storageBackend;
	delete factories;
}
