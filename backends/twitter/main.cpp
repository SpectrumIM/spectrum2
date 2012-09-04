#include "TwitterPlugin.h"
DEFINE_LOGGER(logger, "Twitter Backend");

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


int main (int argc, char* argv[]) {
	std::string host;
	int port;

	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}

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
	
	std::string error;
	StorageBackend *storagebackend;
	
	storagebackend = StorageBackend::createBackend(&config, error);
	if (storagebackend == NULL) {
		LOG4CXX_ERROR(logger, "Error creating StorageBackend! " << error)
		return -2;
	}

	else if (!storagebackend->connect()) {
		LOG4CXX_ERROR(logger, "Can't connect to database!")
		return -1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new TwitterPlugin(&config, &eventLoop, storagebackend, host, port);
	loop_->run();

	return 0;
}
