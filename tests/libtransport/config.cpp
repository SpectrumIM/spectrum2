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


using namespace Transport;

class ConfigTest : public CPPUNIT_NS :: TestFixture{
	CPPUNIT_TEST_SUITE(ConfigTest);
	CPPUNIT_TEST(setStringTwice);
	CPPUNIT_TEST(setUnknownBool);
	CPPUNIT_TEST(enumerateConfigSection);
	CPPUNIT_TEST(updateBackendConfig);
	CPPUNIT_TEST(updateBackendConfigJIDEscaping);
	CPPUNIT_TEST(unregisteredList);
	CPPUNIT_TEST(unregisteredString);
	CPPUNIT_TEST(unregisteredListAsString);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void setStringTwice() {
		const char *argv[3] = {"binary", "--service.jids=localhost", NULL};
		Config cfg(2, const_cast<char **>(argv));
		std::istringstream ifs("service.jids = irc.freenode.org\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("localhost"), CONFIG_STRING(&cfg, "service.jids"));
	}

	void setUnknownBool() {
		const char *argv[3] = {"binary", "--service.jids=localhost", NULL};
		Config cfg(2, const_cast<char **>(argv));
		std::istringstream ifs("service.irc_send_pass = 1\npurple.group-chat-open=0\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(true, boost::lexical_cast<bool>(CONFIG_STRING_DEFAULTED(&cfg, "service.irc_send_pass", "false")));
		CPPUNIT_ASSERT_EQUAL(false, boost::lexical_cast<bool>(CONFIG_STRING_DEFAULTED(&cfg, "purple.group-chat-open", "true")));
	}

	void enumerateConfigSection() {
		const char *argv[3] = {"binary", "--service.jids=localhost", NULL};
		Config cfg(2, const_cast<char **>(argv));
		std::istringstream ifs("[purple]\nirc_send_pass=1\ngroup-chat-open=false\ntest=passed");
		cfg.load(ifs);
		Config::SectionValuesCont purpleConfigValues = cfg.getSectionValues("purple");
		CPPUNIT_ASSERT_EQUAL(std::string("1"), purpleConfigValues["purple.irc_send_pass"].as<std::string>());
		CPPUNIT_ASSERT_EQUAL(std::string("false"), purpleConfigValues["purple.group-chat-open"].as<std::string>());
		CPPUNIT_ASSERT_EQUAL(std::string("passed"), purpleConfigValues["purple.test"].as<std::string>());
	}

	void updateBackendConfig() {
		Config cfg;
		CPPUNIT_ASSERT(!cfg.hasKey("registration.needPassword"));

		cfg.updateBackendConfig("[registration]\nneedPassword=0\n");
		CPPUNIT_ASSERT(cfg.hasKey("registration.needPassword"));
		CPPUNIT_ASSERT_EQUAL(false, CONFIG_BOOL(&cfg, "registration.needPassword"));
	}

	void updateBackendConfigJIDEscaping() {
		Config cfg;
		std::istringstream ifs("service.jids = irc.freenode.org\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(true, CONFIG_BOOL(&cfg, "service.jid_escaping"));

		cfg.updateBackendConfig("[features]\ndisable_jid_escaping=1\n");
		CPPUNIT_ASSERT_EQUAL(false, CONFIG_BOOL(&cfg, "service.jid_escaping"));
	}

	void unregisteredList() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org\nservice.irc_server=localhost\n");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("irc.freenode.org,localhost"), CONFIG_STRING(&cfg, "service.irc_server"));
	}

	void unregisteredString() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("irc.freenode.org"), CONFIG_STRING(&cfg, "service.irc_server"));
	}

	void unregisteredListAsString() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org\nservice.irc_server = irc2.freenode.org");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("irc.freenode.org,irc2.freenode.org"), CONFIG_STRING_DEFAULTED(&cfg, "service.irc_server", ""));
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (ConfigTest);
