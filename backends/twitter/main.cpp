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

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	StorageBackend *storagebackend;
	storagebackend = StorageBackend::createBackend(cfg, error);
	if (storagebackend == NULL) {
		LOG4CXX_ERROR(logger, "Error creating StorageBackend! " << error);
		LOG4CXX_ERROR(logger, "Twitter backend needs storage backend configured to work! " << error);
		return -2;
	}

	else if (!storagebackend->connect()) {
		LOG4CXX_ERROR(logger, "Can't connect to database!")
		return -1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new TwitterPlugin(cfg, &eventLoop, storagebackend, host, port);
	loop_->run();

	return 0;
}
