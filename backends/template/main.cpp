#include "plugin.h"

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

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	Swift::SimpleEventLoop eventLoop;
	Plugin p(cfg, &eventLoop, host, port);
	eventLoop.run();

	return 0;
}
