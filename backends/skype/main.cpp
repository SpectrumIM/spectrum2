#include "glib.h"
#include <dbus-1.0/dbus/dbus-glib-lowlevel.h>
#include "sqlite3.h"
#include <iostream>

#include "transport/config.h"
#include "transport/logging.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/memoryusage.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/user.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/conversation.h"
#include "transport/networkplugin.h"
#include <boost/filesystem.hpp>
#include "sys/wait.h"
#include "sys/signal.h"
// #include "valgrind/memcheck.h"
#ifndef __FreeBSD__
#include "malloc.h"
#endif

#include "skype.h"
#include "skypeplugin.h"


DEFINE_LOGGER(logger, "backend");

using namespace Transport;

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

static void log_glib_error(const gchar *string) {
	LOG4CXX_ERROR(logger, "GLIB ERROR:" << string);
}

int main(int argc, char **argv) {
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);

	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}
#endif

	std::string host;
	int port = 10000;


	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	g_type_init();

	g_set_printerr_handler(log_glib_error);

	dbus_threads_init_default();

	SkypePlugin *np = new SkypePlugin(cfg, host, port);

	GMainLoop *m_loop;
	m_loop = g_main_loop_new(NULL, FALSE);

	if (m_loop) {
		g_main_loop_run(m_loop);
	}

	return 0;
}
