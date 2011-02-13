#include "transport/config.h"
#include "transport/transport.h"
#include "transport/logger.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

int main(void)
{
	Config config;
	if (!config.load("sample.cfg")) {
		std::cout << "Can't open sample.cfg configuration file.\n";
		return 1;
	}
	Swift::logging = true;

	Swift::SimpleEventLoop eventLoop;
	Component transport(&eventLoop, &config);

	Logger logger(&transport);

	transport.connect();
	eventLoop.run();
}
