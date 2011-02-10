#include "transport/config.h"
#include "transport/transport.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"

using namespace Transport;

static void onConnected() {
	std::cout << "Connected to Jabber Server!\n";
}

static void onConnectionError(const Swift::ComponentError&) {
	std::cout << "Connection Error!\n";
}

static void onXMLIn(const std::string &data) {
	std::cout << "[XML IN]" << data << "\n";
}

static void onXMLOut(const std::string &data) {
	std::cout << "[XML OUT]" << data << "\n";
}

int main(void)
{
	Config::Variables config;
	if (!Config::load("sample.cfg", config)) {
		std::cout << "Can't open sample.cfg configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	Transport::Transport transport(&eventLoop, config);

	transport.onConnected.connect(&onConnected);
	transport.onConnectionError.connect(bind(&onConnectionError, _1));
	transport.onXMLIn.connect(bind(&onXMLIn, _1));
	transport.onXMLOut.connect(bind(&onXMLOut, _1));

	transport.connect();
	eventLoop.run();
}
