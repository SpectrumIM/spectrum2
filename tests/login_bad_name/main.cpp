#include <iostream>
#include <boost/bind.hpp>

#include <Swiften/Swiften.h>
#include <Swiften/Client/ClientOptions.h>

using namespace Swift;
using namespace boost;

Client* client;

static void handleDisconnected(const boost::optional<ClientError> &error) {
	exit(error->getType() != ClientError::AuthenticationFailedError);
}

static void handleConnected() {
	exit(1);
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

	JID jid(JID(argv[1]).getNode() + "something", JID(argv[1]).getDomain());
	client = new Client(jid, argv[2], &networkFactories);
	client->setAlwaysTrustCertificates();
	client->onConnected.connect(&handleConnected);
	client->onDisconnected.connect(bind(&handleDisconnected, _1));
	client->onMessageReceived.connect(bind(&handleMessageReceived, _1));
	ClientOptions opt;
	opt.allowPLAINOverNonTLS = true;
	client->connect(opt);

	eventLoop.run();

	delete client;
	return 0;
}
