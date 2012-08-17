#include "transport/config.h"
#include "transport/transport.h"
#include "transport/filetransfermanager.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/mysqlbackend.h"
#include "transport/pqxxbackend.h"
#include "transport/userregistration.h"
#include "transport/networkpluginserver.h"
#include "transport/admininterface.h"
#include "transport/statsresponder.h"
#include "transport/usersreconnecter.h"
#include "transport/util.h"
#include "transport/gatewayresponder.h"
#include "transport/logging.h"
#include "transport/discoitemsresponder.h"
#include "transport/adhocmanager.h"
#include "transport/settingsadhoccommand.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#ifndef WIN32
#include "sys/signal.h"
#include <pwd.h>
#include <grp.h>
#include <sys/resource.h>
#include "libgen.h"
#include <malloc.h>
#else
#include <windows.h>
#endif
#include <sys/stat.h>

using namespace Transport;

DEFINE_LOGGER(logger, "Spectrum");

Swift::SimpleEventLoop *eventLoop_ = NULL;
Component *component_ = NULL;
UserManager *userManager_ = NULL;

static void stop_spectrum() {
	userManager_->removeAllUsers(false);
	component_->stop();
	eventLoop_->stop();
}

static void spectrum_sigint_handler(int sig) {
	eventLoop_->postEvent(&stop_spectrum);
}

static void spectrum_sigterm_handler(int sig) {
	eventLoop_->postEvent(&stop_spectrum);
}

static void removeOldIcons(std::string iconDir) {
	std::vector<std::string> dirs;
	dirs.push_back(iconDir);

	boost::thread thread(boost::bind(Util::removeEverythingOlderThan, dirs, time(NULL) - 3600*24*14));
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
		if (lock_file) {
			/* write our pid into it & close the file. */
			lock_file_f = fopen(lock_file, "w+");
			if (lock_file_f == NULL) {
				std::cerr << "Cannot create lock file " << lock_file << ". Exiting\n";
				exit(1);
			}
			sprintf(process_pid,"%d\n",pid);
			if (fwrite(process_pid,1,strlen(process_pid),lock_file_f) < strlen(process_pid)) {
				std::cerr << "Cannot write to lock file " << lock_file << ". Exiting\n";
				exit(1);
			}
			fclose(lock_file_f);
		}
		exit(0);
	}

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
	
	if (freopen( "/dev/null", "r", stdin) == NULL) {
		std::cout << "EE cannot open /dev/null. Exiting\n";
		exit(1);
	}
}

#endif

int main(int argc, char **argv)
{
	Config config(argc, argv);

	boost::program_options::variables_map vm;
	bool no_daemon = false;
	std::string config_file;
	std::string jid;

	setlocale(LC_ALL, "");
#ifndef WIN32
	mallopt(M_CHECK_ACTION, 2);
	mallopt(M_PERTURB, 0xb);
#endif

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
	boost::program_options::options_description desc(std::string("Spectrum version: ") + SPECTRUM_VERSION + "\nUsage: spectrum [OPTIONS] <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help,h", "help")
		("no-daemonize,n", "Do not run spectrum as daemon")
		("no-debug,d", "Create coredumps on crash")
		("jid,j", boost::program_options::value<std::string>(&jid)->default_value(""), "Specify JID of transport manually")
		("config", boost::program_options::value<std::string>(&config_file)->default_value(""), "Config file")
		("version,v", "Shows Spectrum version")
		;
	try
	{
		boost::program_options::positional_options_description p;
		p.add("config", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
          options(desc).positional(p).allow_unregistered().run(), vm);
		boost::program_options::notify(vm);

		if (vm.count("version")) {
			std::cout << SPECTRUM_VERSION << "\n";
			return 0;
		}

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

	if (!config.load(vm["config"].as<std::string>(), jid)) {
		std::cerr << "Can't load configuration file.\n";
		return 1;
	}

	// create directories
	try {
		boost::filesystem::create_directories(
			boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string()
		);
	}
	catch (...) {
		std::cerr << "Can't create service.pidfile directory " << boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string() << ".\n";
		return 1;
	}
	// create directories
	try {
		boost::filesystem::create_directories(CONFIG_STRING(&config, "service.working_dir"));
	}
	catch (...) {
		std::cerr << "Can't create service.working_dir directory " << CONFIG_STRING(&config, "service.working_dir") << ".\n";
		return 1;
	}
	// create directories
	try {
		boost::filesystem::create_directories(
			boost::filesystem::path(CONFIG_STRING(&config, "service.portfile")).parent_path().string()
		);
	}
	catch (...) {
		std::cerr << "Can't create service.portfile directory " << CONFIG_STRING(&config, "service.portfile") << ".\n";
		return 1;
	}

#ifndef WIN32
	if (!CONFIG_STRING(&config, "service.group").empty() ||!CONFIG_STRING(&config, "service.user").empty() ) {
		struct group *gr;
		if ((gr = getgrnam(CONFIG_STRING(&config, "service.group").c_str())) == NULL) {
			std::cerr << "Invalid service.group name " << CONFIG_STRING(&config, "service.group") << "\n";
			return 1;
		}
		struct passwd *pw;
		if ((pw = getpwnam(CONFIG_STRING(&config, "service.user").c_str())) == NULL) {
			std::cerr << "Invalid service.user name " << CONFIG_STRING(&config, "service.user") << "\n";
			return 1;
		}
		chown(CONFIG_STRING(&config, "service.working_dir").c_str(), pw->pw_uid, gr->gr_gid);
	}

	char backendport[20];
	FILE* port_file_f;
	port_file_f = fopen(CONFIG_STRING(&config, "service.portfile").c_str(), "w+");
	if (port_file_f == NULL) {
		std::cerr << "Cannot create port_file file " << CONFIG_STRING(&config, "service.portfile").c_str() << ". Exiting\n";
		exit(1);
	}
	sprintf(backendport,"%s\n",CONFIG_STRING(&config, "service.backend_port").c_str());
	if (fwrite(backendport,1,strlen(backendport),port_file_f) < strlen(backendport)) {
		std::cerr << "Cannot write to port file " << CONFIG_STRING(&config, "service.portfile") << ". Exiting\n";
		exit(1);
	}
	fclose(port_file_f);

	if (!no_daemon) {
		// daemonize
		daemonize(CONFIG_STRING(&config, "service.working_dir").c_str(), CONFIG_STRING(&config, "service.pidfile").c_str());
// 		removeOldIcons(CONFIG_STRING(&config, "service.working_dir") + "/icons");
    }
#endif

	Logging::initMainLogging(&config);

#ifndef WIN32
	if (!CONFIG_STRING(&config, "service.group").empty() ||!CONFIG_STRING(&config, "service.user").empty() ) {
		struct rlimit limit;
		getrlimit(RLIMIT_CORE, &limit);

		if (!CONFIG_STRING(&config, "service.group").empty()) {
			struct group *gr;
			if ((gr = getgrnam(CONFIG_STRING(&config, "service.group").c_str())) == NULL) {
				std::cerr << "Invalid service.group name " << CONFIG_STRING(&config, "service.group") << "\n";
				return 1;
			}

			if (((setgid(gr->gr_gid)) != 0) || (initgroups(CONFIG_STRING(&config, "service.user").c_str(), gr->gr_gid) != 0)) {
				std::cerr << "Failed to set service.group name " << CONFIG_STRING(&config, "service.group") << " - " << gr->gr_gid << ":" << strerror(errno) << "\n";
				return 1;
			}
		}

		if (!CONFIG_STRING(&config, "service.user").empty()) {
			struct passwd *pw;
			if ((pw = getpwnam(CONFIG_STRING(&config, "service.user").c_str())) == NULL) {
				std::cerr << "Invalid service.user name " << CONFIG_STRING(&config, "service.user") << "\n";
				return 1;
			}

			if ((setuid(pw->pw_uid)) != 0) {
				std::cerr << "Failed to set service.user name " << CONFIG_STRING(&config, "service.user") << " - " << pw->pw_uid << ":" << strerror(errno) << "\n";
				return 1;
			}
		}
		setrlimit(RLIMIT_CORE, &limit);
	}

	struct rlimit limit;
	limit.rlim_max = RLIM_INFINITY;
	limit.rlim_cur = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &limit);
#endif

	Swift::SimpleEventLoop eventLoop;

	Swift::BoostNetworkFactories *factories = new Swift::BoostNetworkFactories(&eventLoop);
	UserRegistry userRegistry(&config, factories);

	Component transport(&eventLoop, factories, &config, NULL, &userRegistry);
	component_ = &transport;
// 	Logger logger(&transport);

	std::string error;
	StorageBackend *storageBackend = StorageBackend::createBackend(&config, error);
	if (storageBackend == NULL) {
		if (!error.empty()) {
			std::cerr << error << "\n";
			return -2;
		}
	}
	else if (!storageBackend->connect()) {
		std::cerr << "Can't connect to database. Check the log to find out the reason.\n";
		return -1;
	}

	UserManager userManager(&transport, &userRegistry, storageBackend);
	userManager_ = &userManager;

	UserRegistration *userRegistration = NULL;
	UsersReconnecter *usersReconnecter = NULL;
	if (storageBackend) {
		userRegistration = new UserRegistration(&transport, &userManager, storageBackend);
		userRegistration->start();

		usersReconnecter = new UsersReconnecter(&transport, storageBackend);
	}

	FileTransferManager ftManager(&transport, &userManager);

	NetworkPluginServer plugin(&transport, &config, &userManager, &ftManager);

	AdminInterface adminInterface(&transport, &userManager, &plugin, storageBackend, userRegistration);
	plugin.setAdminInterface(&adminInterface);

	StatsResponder statsResponder(&transport, &userManager, &plugin, storageBackend);
	statsResponder.start();

	GatewayResponder gatewayResponder(transport.getIQRouter(), &userManager);
	gatewayResponder.start();

	DiscoItemsResponder discoItemsResponder(&transport);
	discoItemsResponder.start();

	AdHocManager adhocmanager(&transport, &discoItemsResponder, &userManager, storageBackend);
	adhocmanager.start();

	SettingsAdHocCommandFactory settings;
	adhocmanager.addAdHocCommand(&settings);

	eventLoop_ = &eventLoop;

	eventLoop.run();

	if (userRegistration) {
		userRegistration->stop();
		delete userRegistration;
	}

	if (usersReconnecter) {
		delete usersReconnecter;
	}

	delete storageBackend;
	delete factories;
	return 0;
}
