#include "transport/Config.h"
#include "transport/Transport.h"
#include "transport/FileTransferManager.h"
#include "transport/UserManager.h"
#include "transport/SQLite3Backend.h"
#include "transport/MySQLBackend.h"
#include "transport/PQXXBackend.h"
#include "transport/StorageBackend.h"
#include "transport/UserRegistration.h"
#include "transport/UserRegistry.h"
#include "transport/NetworkPluginServer.h"
#include "transport/AdminInterface.h"
#include "transport/UsersReconnecter.h"
#include "transport/Util.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "frontends/xmpp/XMPPFrontendPlugin.h"
#include "frontends/slack/SlackFrontendPlugin.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"
#include "Swiften/Network/BoostNetworkFactories.h"
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#ifndef WIN32
#include "sys/signal.h"
#include "sys/stat.h"
#include <pwd.h>
#include <grp.h>
#include <sys/resource.h>
#include "libgen.h"
#ifndef __FreeBSD__
#ifndef __MACH__
#include <malloc.h>
#endif
#endif
#else
#include <process.h>
#define getpid _getpid
#include "win32/ServiceWrapper.h"
#endif
#include <sys/stat.h>

#include <boost/dll/shared_library.hpp>         // for shared_library
#include <boost/dll/runtime_symbol_info.hpp>    // for program_location()

namespace dll = boost::dll;
using namespace Transport;
using namespace Transport::Util;

DEFINE_LOGGER(logger, "Spectrum");

Swift::SimpleEventLoop *eventLoop_ = NULL;
Component *component_ = NULL;
UserManager *userManager = NULL;
Config *config_ = NULL;

static void stop_spectrum() {
	userManager->removeAllUsers(false);
	component_->stop();
	eventLoop_->stop();
}

static void spectrum_sigint_handler(int sig) {
	eventLoop_->postEvent(&stop_spectrum);
}

static void spectrum_sigterm_handler(int sig) {
	eventLoop_->postEvent(&stop_spectrum);
}

#ifdef WIN32
BOOL spectrum_control_handler( DWORD fdwCtrlType ) {
	if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT) {
		eventLoop_->postEvent(&stop_spectrum);
		return TRUE;
	}
	return FALSE;
}
#endif

// currently unused
#if 0
static void removeOldIcons(std::string iconDir) {
	std::vector<std::string> dirs;
	dirs.push_back(iconDir);

	boost::thread thread(boost::bind(Util::removeEverythingOlderThan, dirs, time(NULL) - 3600*24*14));
}
#endif

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

static void _createDirectories(Transport::Config *config, boost::filesystem::path ph) {
	if (ph.empty() || exists(ph)) {
		return;
	}

	// First create branch, by calling ourself recursively
	_createDirectories(config, ph.branch_path());

	// Now that parent's path exists, create the directory
	boost::filesystem::create_directory(ph);

#ifndef WIN32
	if (!CONFIG_STRING(config, "service.group").empty() && !CONFIG_STRING(config, "service.user").empty()) {
		struct group *gr;
		if ((gr = getgrnam(CONFIG_STRING(config, "service.group").c_str())) == NULL) {
			std::cerr << "Invalid service.group name " << CONFIG_STRING(config, "service.group") << "\n";
		}
		struct passwd *pw;
		if ((pw = getpwnam(CONFIG_STRING(config, "service.user").c_str())) == NULL) {
			std::cerr << "Invalid service.user name " << CONFIG_STRING(config, "service.user") << "\n";
		}
		chown(ph.string().c_str(), pw->pw_uid, gr->gr_gid);
	}
#endif
}

int mainloop() {

#ifndef WIN32
	mode_t old_cmask = umask(0007);
#endif

	Logging::initMainLogging(config_);

#ifndef WIN32
	if (!CONFIG_STRING(config_, "service.group").empty() ||!CONFIG_STRING(config_, "service.user").empty() ) {
		struct rlimit limit;
		getrlimit(RLIMIT_CORE, &limit);

		if (!CONFIG_STRING(config_, "service.group").empty()) {
			struct group *gr;
			if ((gr = getgrnam(CONFIG_STRING(config_, "service.group").c_str())) == NULL) {
				std::cerr << "Invalid service.group name " << CONFIG_STRING(config_, "service.group") << "\n";
				return 1;
			}

			if (((setgid(gr->gr_gid)) != 0) || (initgroups(CONFIG_STRING(config_, "service.user").c_str(), gr->gr_gid) != 0)) {
				std::cerr << "Failed to set service.group name " << CONFIG_STRING(config_, "service.group") << " - " << gr->gr_gid << ":" << strerror(errno) << "\n";
				return 1;
			}
		}

		if (!CONFIG_STRING(config_, "service.user").empty()) {
			struct passwd *pw;
			if ((pw = getpwnam(CONFIG_STRING(config_, "service.user").c_str())) == NULL) {
				std::cerr << "Invalid service.user name " << CONFIG_STRING(config_, "service.user") << "\n";
				return 1;
			}

			if ((setuid(pw->pw_uid)) != 0) {
				std::cerr << "Failed to set service.user name " << CONFIG_STRING(config_, "service.user") << " - " << pw->pw_uid << ":" << strerror(errno) << "\n";
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
	UserRegistry userRegistry(config_, factories);


	Frontend *frontend = NULL;

	std::string frontend_name = CONFIG_STRING_DEFAULTED(config_, "service.frontend", "xmpp");
	std::string plugin_fc = "create_" + frontend_name + "_frontend_plugin";

	dll::shared_library self(dll::program_location());
	boost::function<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<FrontendPlugin>()> creator;

	try {
		creator = self.get_alias<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<FrontendPlugin>()>(plugin_fc);
	} catch(boost::system::system_error& e) {
		LOG4CXX_ERROR(logger, "Error when loading frontend " << e.what());
		return -3;
	}

	if (!creator) {
		LOG4CXX_ERROR(logger, "Unknown Frontend name " << frontend_name);
		return -3;
	}

	frontend = creator()->createFrontend();

	Component transport(frontend, &eventLoop, factories, config_, NULL, &userRegistry);
	component_ = &transport;

	std::string error;
	StorageBackend *storageBackend = StorageBackend::createBackend(config_, error);
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

	Logging::redirect_stderr();

	userManager = frontend->createUserManager(&transport, &userRegistry, storageBackend);;
	UserRegistration *userRegistration = userManager->getUserRegistration();

	UsersReconnecter *usersReconnecter = NULL;
	if (storageBackend) {
		usersReconnecter = new UsersReconnecter(&transport, storageBackend);
	}
	else if (!CONFIG_BOOL(config_, "service.server_mode")) {
		LOG4CXX_WARN(logger, "Registrations won't work, you have specified [database] type=none in config file.");
	}

	FileTransferManager ftManager(&transport, userManager);

	NetworkPluginServer plugin(&transport, config_, userManager, &ftManager);
	plugin.start();

	AdminInterface adminInterface(&transport, userManager, &plugin, storageBackend, userRegistration);
	plugin.setAdminInterface(&adminInterface);
	transport.setAdminInterface(&adminInterface);

	eventLoop_ = &eventLoop;

	eventLoop.run();

#ifndef WIN32
	umask(old_cmask);
#endif

	delete userManager;

	if (usersReconnecter) {
		delete usersReconnecter;
	}

	delete storageBackend;
	delete factories;
	return 0;
}


int main(int argc, char **argv)
{
	Config config(argc, argv);
	config_ = &config;
	boost::program_options::variables_map vm;
	bool no_daemon = false;
	std::string config_file;
	std::string jid;
#ifdef WIN32
	std::string install_service_name, uninstall_service_name, run_service_name;
	// determine the name of the currently executing file
	char szFilePath[MAX_PATH];
	GetModuleFileNameA(NULL, szFilePath, sizeof(szFilePath));
	std::string exe_file(szFilePath);
#endif
	setlocale(LC_ALL, "");
#ifndef WIN32
#ifndef __FreeBSD__
#ifndef __MACH__
#if defined (__GLIBC__)
	mallopt(M_CHECK_ACTION, 2);
	mallopt(M_PERTURB, 0xb);
#endif
#endif
#endif
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
#else
	if ( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE) spectrum_control_handler, TRUE ) )
	{
		std::cout << "control handler can't be set\n";
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
#ifdef WIN32
	desc.add_options()
		("install-service,i", boost::program_options::value<std::string>(&install_service_name)->default_value(""), "Install spectrum as Windows service")
		("uninstall-service,u", boost::program_options::value<std::string>(&uninstall_service_name)->default_value(""), "Uninstall Windows service")
		("run-as-service,r", boost::program_options::value<std::string>(&run_service_name)->default_value(""), "stub for Windows Service Manager");
#endif
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

		if (vm.count("help"))
		{
			std::cout << desc << "\n";
			return 1;
		}

		if (vm.count("config") == 0) {
			std::cout << desc << "\n";
			return 1;
		}

		if (vm.count("no-daemonize")) {
			no_daemon = true;
		}
#ifdef WIN32
		if (!install_service_name.empty()) {
			// build command line for Service Manager
			std::string service_path = exe_file + std::string(" --config ") + vm["config"].as<std::string>()
						+ std::string(" --run-as-service ") + install_service_name;

			ServiceWrapper ntservice((char *)install_service_name.c_str());
			if (!ntservice.IsInstalled()) {
				if (ntservice.Install((char *)service_path.c_str())) {
					std::cout << "Successfully installed " << install_service_name << std::endl;
					return 0;
				} else {
					std::cout << "Error installing service, are you an Administrator?" << std::endl;
					return 1;
				}
			} else {
				std::cout << "Already installed " << install_service_name << std::endl;
				return 1;
			}
		}
		if (!uninstall_service_name.empty()) {
			ServiceWrapper ntservice((char *)uninstall_service_name.c_str());
			if (ntservice.IsInstalled()) {
				if (ntservice.UnInstall()) {
					std::cout << "Successfully removed " << uninstall_service_name << std::endl;
					return 0;
				} else {
					std::cout << "Error removing service, are you an Administrator?" << std::endl;
					return 1;
				}
			} else {
				std::cout << "Service not installed: " << uninstall_service_name << std::endl;
				return 1;
			}
		}
#endif
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
		_createDirectories(&config, boost::filesystem::path(CONFIG_STRING(&config, "service.working_dir")));
	}
	catch (...) {
		std::cerr << "Can't create service.working_dir directory " << CONFIG_STRING(&config, "service.working_dir") << ".\n";
		return 1;
	}
#ifndef WIN32
	// create directories
	try {
		_createDirectories(&config,
			boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string()
		);
	}
	catch (...) {
		std::cerr << "Can't create service.pidfile directory " << boost::filesystem::path(CONFIG_STRING(&config, "service.pidfile")).parent_path().string() << ".\n";
		return 1;
	}
	// create directories
	try {
		_createDirectories(&config,
			boost::filesystem::path(CONFIG_STRING(&config, "service.portfile")).parent_path().string()
		);
	}
	catch (...) {
		std::cerr << "Can't create service.portfile directory " << CONFIG_STRING(&config, "service.portfile") << ".\n";
		return 1;
	}
#endif

#ifdef WIN32
	SetCurrentDirectory( utf8ToUtf16(CONFIG_STRING(&config, "service.working_dir")).c_str() );
#endif

#ifndef WIN32
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
//		removeOldIcons(CONFIG_STRING(&config, "service.working_dir") + "/icons");
	}
	else {
		if ((chdir(CONFIG_STRING(&config, "service.working_dir").c_str())) < 0) {
			std::cerr << "Cannot change directory to " << CONFIG_STRING(&config, "service.working_dir") << "\n";
			exit(1);
		}
	}
#endif
#ifdef WIN32
	if (!run_service_name.empty()) {
		ServiceWrapper ntservice((char *)run_service_name.c_str());
		if (ntservice.IsInstalled()) {
			ntservice.RunService();
		} else {
			std::cerr << "Service not installed: " << run_service_name << std::endl;
			return 1;
		}
	} else {
		mainloop();
	}
#else
	mainloop();
#endif
}
