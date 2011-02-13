#include "transport/config.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

int main(void)
{
	Config config;
	if (!config.load("sample.cfg")) {
		std::cout << "Can't open sample.cfg configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	Component transport(&eventLoop, &config);
	Logger logger(&transport);

	SQLite3Backend sql(&config);
	logger.setStorageBackend(&sql);
	if (!sql.connect()) {
		std::cout << "Can't connect to database.\n";
	}

	transport.setStorageBackend(&sql);

	UserManager userManager(&transport);
	UserRegistration userRegistration(&transport, &userManager, &sql);

	transport.connect();
	eventLoop.run();
}
