#include "transport/config.h"
#include "transport/transport.h"
#include "transport/logger.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include "Swiften/Network/BoostNetworkFactories.h"
#include "Swiften/Server/UserRegistry.h"
#include "Swiften/Server/Server.h"
#include "Swiften/Swiften.h"

using namespace Transport;

class DummyUserRegistry : public Swift::UserRegistry {
	public:
		DummyUserRegistry() {}

		virtual bool isValidUserPassword(const Swift::JID&, const std::string&) const {
			return true;
		}
};

int main(void)
{
	Swift::logging = true;

	Swift::SimpleEventLoop loop;

	Swift::BoostNetworkFactories *m_factories = new Swift::BoostNetworkFactories(&loop);
	DummyUserRegistry dummyregistry;
	Swift::Server server(&loop, m_factories, &dummyregistry, "localhost", 5222);
	server.start();

	loop.run();
}
