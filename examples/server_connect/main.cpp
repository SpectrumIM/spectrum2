#include "transport/config.h"
#include "transport/transport.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

int main(void)
{
	Config::Variables config;
	if (!Config::load("sample.cfg", config)) {
		std::cout << "Can't open sample.cfg configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	Transport::Transport transport(&eventLoop, config);
}
