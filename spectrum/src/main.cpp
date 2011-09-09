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
#include <boost/filesystem.hpp>
#ifndef WIN32
#include "sys/signal.h"
#include <pwd.h>
#include <grp.h>
#else
#include <Windows.h>
#include <tchar.h>
#endif
#include "log4cxx/logger.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/consoleappender.h"
#include "libgen.h"

using namespace log4cxx;

using namespace Transport;

static LoggerPtr logger = log4cxx::Logger::getLogger("Spectrum");

Swift::SimpleEventLoop *eventLoop_ = NULL;

static void spectrum_sigint_handler(int sig) {
	eventLoop_->stop();
}

static void spectrum_sigterm_handler(int sig) {
	eventLoop_->stop();
}

#ifndef WIN32
static void daemonize(const char *cwd, const char *lock_file) {
	pid_t pid, sid;
	FILE* lock_file_f;
	char process_pid[20];

	/* already a daemon */
	if ( getppid() == 1 ) return;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(1);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(0);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		exit(1);
	}

	/* Change the current working directory.  This prevents the current
		directory from being locked; hence not being able to remove it. */
	if ((chdir(cwd)) < 0) {
		exit(1);
	}

	if (lock_file) {
		/* write our pid into it & close the file. */
		lock_file_f = fopen(lock_file, "w+");
		if (lock_file_f == NULL) {
			std::cout << "EE cannot create lock file " << lock_file << ". Exiting\n";
			exit(1);
		}
		sprintf(process_pid,"%d\n",getpid());
		if (fwrite(process_pid,1,strlen(process_pid),lock_file_f) < strlen(process_pid)) {
			std::cout << "EE cannot write to lock file " << lock_file << ". Exiting\n";
			exit(1);
		}
		fclose(lock_file_f);
	}
	
	if (freopen( "/dev/null", "r", stdin) == NULL) {
		std::cout << "EE cannot open /dev/null. Exiting\n";
		exit(1);
	}
}

#endif

int main(int argc, char **argv)
{
	Config config;

	boost::program_options::variables_map vm;
	bool no_daemon = false;
	std::string config_file;
	

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
		("config", boost::program_options::value<std::string>(&config_file)->default_value(""), "Config file")
		;
	try
	{
		boost::program_options::positional_options_description p;
		p.add("config", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
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

		if(vm.count("no-daemonize")) {
			no_daemon = true;
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

	if (!config.load(vm["config"].as<std::string>())) {
		std::cerr << "Can't load configuration file.\n";
		return 1;
	}

#ifndef WIN32
	if (!no_daemon) {
		// create directories
		try {
			boost::filesystem::create_directories(CONFIG_STRING(&config, "service.working_dir"));
		}
		catch (...) {
			std::cerr << "Can't create service.working_dir directory " << CONFIG_STRING(&config, "service.working_dir") << ".\n";
			return 1;
		}
		try {
			boost::filesystem::create_directories(
				boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string()
			);
		}
		catch (...) {
			std::cerr << "Can't create service.pidfile directory " << boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string() << ".\n";
			return 1;
		}

		// daemonize
		daemonize(CONFIG_STRING(&config, "service.working_dir").c_str(), CONFIG_STRING(&config, "service.pidfile").c_str());
    }
#endif

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

#ifndef WIN32
	if (!CONFIG_STRING(&config, "service.group").empty()) {
		struct group *gr;
		if ((gr = getgrnam(CONFIG_STRING(&config, "service.group").c_str())) == NULL) {
			LOG4CXX_ERROR(logger, "Invalid service.group name " << CONFIG_STRING(&config, "service.group"));
			return 1;
		}

		if (((setgid(gr->gr_gid)) != 0) || (initgroups(CONFIG_STRING(&config, "service.user").c_str(), gr->gr_gid) != 0)) {
			LOG4CXX_ERROR(logger, "Failed to set service.group name " << CONFIG_STRING(&config, "service.group") << " - " << gr->gr_gid << ":" << strerror(errno));
			return 1;
		}
	}

	if (!CONFIG_STRING(&config, "service.user").empty()) {
		struct passwd *pw;
		if ((pw = getpwnam(CONFIG_STRING(&config, "service.user").c_str())) == NULL) {
			LOG4CXX_ERROR(logger, "Invalid service.user name " << CONFIG_STRING(&config, "service.user"));
			return 1;
		}

		if ((setuid(pw->pw_uid)) != 0) {
			LOG4CXX_ERROR(logger, "Failed to set service.user name " << CONFIG_STRING(&config, "service.user") << " - " << pw->pw_uid << ":" << strerror(errno));
			return 1;
		}
	}
#endif

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
