#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "basictest.h"

#include "transport/util.h"

using namespace Transport;

class ConfigTest : public CPPUNIT_NS :: TestFixture{
	CPPUNIT_TEST_SUITE(ConfigTest);
	CPPUNIT_TEST(setStringTwice);
	CPPUNIT_TEST(updateBackendConfig);
	CPPUNIT_TEST(unregisteredList);
	CPPUNIT_TEST(unregisteredString);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void setStringTwice() {
		char *argv[3] = {"binary", "--service.jids=localhost", NULL};
		Config cfg(2, argv);
		std::istringstream ifs("service.jids = irc.freenode.org\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("localhost"), CONFIG_STRING(&cfg, "service.jids"));
	}

	void updateBackendConfig() {
		Config cfg;
		CPPUNIT_ASSERT(!cfg.hasKey("registration.needPassword"));

		cfg.updateBackendConfig("[registration]\nneedPassword=0\n");
		CPPUNIT_ASSERT(cfg.hasKey("registration.needPassword"));
		CPPUNIT_ASSERT_EQUAL(false, CONFIG_BOOL(&cfg, "registration.needPassword"));
	}

	void unregisteredList() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org\nservice.irc_server=localhost\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(2, (int) CONFIG_LIST(&cfg, "service.irc_server").size());
	}

	void unregisteredString() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("irc.freenode.org"), CONFIG_STRING(&cfg, "service.irc_server"));
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (ConfigTest);
