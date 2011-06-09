#include <iostream>
#include <boost/bind.hpp>

#include <Swiften/Swiften.h>
#include <Swiften/Client/ClientOptions.h>

using namespace Swift;
using namespace boost;

Client* client;

static void handleDisconnected(const boost::optional<ClientError> &) {
	exit(1);
}

static void handleConnected() {
	exit(0);
}

static void handleMessageReceived(Message::ref message) {
	// Echo back the incoming message
	message->setTo(message->getFrom());
	message->setFrom(JID());
	client->sendMessage(message);
}

int main(int, char **argv) {
	SimpleEventLoop eventLoop;
	BoostNetworkFactories networkFactories(&eventLoop);

	client = new Client(argv[1], argv[2], &networkFactories);
	client->setAlwaysTrustCertificates();
	client->onConnected.connect(&handleConnected);
	client->onDisconnected.connect(bind(&handleDisconnected, _1));
	client->onMessageReceived.connect(bind(&handleMessageReceived, _1));
	Swift::ClientOptions opt;
	opt.allowPLAINWithoutTLS = true;
	client->connect(opt);

	eventLoop.run();

	delete client;
	return 0;
}
