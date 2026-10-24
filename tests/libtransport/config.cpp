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
#include "transport/CommandLine.h"


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
	CPPUNIT_TEST(unregisteredStringAsList);
	CPPUNIT_TEST(hashStartsComment);
	CPPUNIT_TEST(registeredSetTwice);
	CPPUNIT_TEST(registeredVector);
	CPPUNIT_TEST(registeredFromCommandLine);
	CPPUNIT_TEST(registeredTypes);
	CPPUNIT_TEST(invalidInt);
	CPPUNIT_TEST(invalidLine);
	CPPUNIT_TEST(jidSubstitution);
	CPPUNIT_TEST(reloadKeepsSetValues);
	CPPUNIT_TEST(commandLineOptions);
	CPPUNIT_TEST(commandLineMissingValue);
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
		CPPUNIT_ASSERT_EQUAL(true, CONFIG_BOOL_DEFAULTED(&cfg, "service.irc_send_pass", false));
		CPPUNIT_ASSERT_EQUAL(false, CONFIG_BOOL_DEFAULTED(&cfg, "purple.group-chat-open", true));
	}

	void enumerateConfigSection() {
		const char *argv[3] = {"binary", "--service.jids=localhost", NULL};
		Config cfg(2, const_cast<char **>(argv));
		std::istringstream ifs("[purple]\nirc_send_pass=1\ngroup-chat-open=false\ntest=passed");
		cfg.load(ifs);
		Config::SectionValuesCont purpleConfigValues = cfg.getSectionValues("purple");
		CPPUNIT_ASSERT_EQUAL(true, purpleConfigValues["purple.irc_send_pass"].as<bool>());
		CPPUNIT_ASSERT_EQUAL(false, purpleConfigValues["purple.group-chat-open"].as<bool>());
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
		CPPUNIT_ASSERT_EQUAL(2, (int) CONFIG_LIST(&cfg, "service.irc_server").size());
	}

	void unregisteredString() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string("irc.freenode.org"), CONFIG_STRING(&cfg, "service.irc_server"));
	}

	void unregisteredListAsString() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.orgn\nservice.irc_server = irc2.freenode.org");
		cfg.load(ifs);
		CPPUNIT_ASSERT_EQUAL(std::string(""), CONFIG_STRING_DEFAULTED(&cfg, "service.irc_server", ""));
	}

	void unregisteredStringAsList() {
		Config cfg;
		std::istringstream ifs("service.irc_server = irc.freenode.org");
		cfg.load(ifs);
		std::list<std::string> list;
		CPPUNIT_ASSERT_EQUAL(0, (int) CONFIG_LIST_DEFAULTED(&cfg, "service.irc_server", list).size());
	}

	void hashStartsComment() {
		Config cfg;
		std::istringstream ifs("# comment\n[identity]\nname = Spectrum#not part of the name\ntype=irc # comment\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		CPPUNIT_ASSERT_EQUAL(std::string("Spectrum"), CONFIG_STRING(&cfg, "identity.name"));
		CPPUNIT_ASSERT_EQUAL(std::string("irc"), CONFIG_STRING(&cfg, "identity.type"));
	}

	void registeredSetTwice() {
		Config cfg;
		std::istringstream ifs("service.jid = a.localhost\nservice.jid = b.localhost\n");
		CPPUNIT_ASSERT(!cfg.load(ifs));
	}

	void registeredVector() {
		Config cfg;
		std::istringstream ifs("service.admin_jid = a@localhost\nservice.admin_jid = b@localhost\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		std::vector<std::string> admins = CONFIG_VECTOR(&cfg, "service.admin_jid");
		CPPUNIT_ASSERT_EQUAL(2, (int) admins.size());
		CPPUNIT_ASSERT_EQUAL(std::string("b@localhost"), admins[1]);
		CPPUNIT_ASSERT_EQUAL(0, (int) CONFIG_VECTOR(&cfg, "service.allowed_servers").size());
	}

	void registeredFromCommandLine() {
		const char *argv[5] = {"binary", "--service.port=5347", "--identity.type", "xmpp", NULL};
		Config cfg(4, const_cast<char **>(argv));
		std::istringstream ifs("service.port = 5222\nidentity.type = irc\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		CPPUNIT_ASSERT_EQUAL(5347, CONFIG_INT(&cfg, "service.port"));
		CPPUNIT_ASSERT_EQUAL(std::string("xmpp"), CONFIG_STRING(&cfg, "identity.type"));
	}

	void registeredTypes() {
		Config cfg;
		std::istringstream ifs("[service]\nusers_per_backend = 5\nserver_mode = yes\nvip_only = off\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		CPPUNIT_ASSERT_EQUAL(5, CONFIG_INT(&cfg, "service.users_per_backend"));
		CPPUNIT_ASSERT_EQUAL(true, CONFIG_BOOL(&cfg, "service.server_mode"));
		CPPUNIT_ASSERT_EQUAL(false, CONFIG_BOOL(&cfg, "service.vip_only"));
		// defaults
		CPPUNIT_ASSERT_EQUAL(std::string("xmpp"), CONFIG_STRING(&cfg, "service.frontend"));
		CPPUNIT_ASSERT_EQUAL(1000000, CONFIG_INT(&cfg, "service.web_maximgsize"));
		CPPUNIT_ASSERT_EQUAL(true, CONFIG_BOOL(&cfg, "service.jid_escaping"));
	}

	void invalidInt() {
		Config cfg;
		std::istringstream ifs("service.port = 52a\n");
		CPPUNIT_ASSERT(!cfg.load(ifs));
	}

	void invalidLine() {
		Config cfg;
		std::istringstream ifs("service.jid = localhost\nthis line has no value\n");
		CPPUNIT_ASSERT(!cfg.load(ifs));
	}

	void jidSubstitution() {
		Config cfg;
		std::istringstream ifs("service.jid = irc.localhost\nservice.backend_port = 10001\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		CPPUNIT_ASSERT_EQUAL(std::string("/var/lib/spectrum2/irc.localhost"), CONFIG_STRING(&cfg, "service.working_dir"));
		CPPUNIT_ASSERT_EQUAL(std::string("/var/run/spectrum2/irc.localhost.pid"), CONFIG_STRING(&cfg, "service.pidfile"));
		CPPUNIT_ASSERT_EQUAL(std::string("10001"), CONFIG_STRING(&cfg, "service.backend_port"));

		Config cfg2;
		std::istringstream ifs2("service.jid = irc.localhost\n");
		CPPUNIT_ASSERT(cfg2.load(ifs2, "other.localhost"));
		CPPUNIT_ASSERT_EQUAL(std::string("other.localhost"), CONFIG_STRING(&cfg2, "service.jid"));
		CPPUNIT_ASSERT_EQUAL(std::string("/var/lib/spectrum2/other.localhost"), CONFIG_STRING(&cfg2, "service.working_dir"));
	}

	void reloadKeepsSetValues() {
		// as with boost::program_options: values set by an earlier load stay,
		// defaults are replaced
		Config cfg;
		std::istringstream ifs("service.port = 5222\n");
		CPPUNIT_ASSERT(cfg.load(ifs));
		std::istringstream ifs2("service.port = 5347\nservice.vip_only = 1\n");
		CPPUNIT_ASSERT(cfg.load(ifs2));
		CPPUNIT_ASSERT_EQUAL(5222, CONFIG_INT(&cfg, "service.port"));
		CPPUNIT_ASSERT_EQUAL(true, CONFIG_BOOL(&cfg, "service.vip_only"));
	}

	void commandLineOptions() {
		const char *argv[8] = {"binary", "-j", "irc.localhost", "--port=5000", "--service.backend_id=3", "-hlocalhost", "config.cfg", NULL};
		CommandLine cmdline;
		cmdline.addOption("help", 0, false, "help");
		cmdline.addOption("host", 'h', true, "Host to connect to", "");
		cmdline.addOption("port", 'p', true, "Port to connect to", "10000");
		cmdline.addOption("jid", 'j', true, "JID", "");
		cmdline.addOption("no-daemonize", 'n', false, "No daemon");
		cmdline.addOption("config", 0, true, "Config file", "");
		cmdline.setPositional("config");
		std::string error;
		CPPUNIT_ASSERT(cmdline.parse(7, const_cast<char **>(argv), error));
		CPPUNIT_ASSERT_EQUAL(std::string("irc.localhost"), cmdline.get("jid"));
		CPPUNIT_ASSERT_EQUAL(std::string("5000"), cmdline.get("port"));
		CPPUNIT_ASSERT_EQUAL(std::string("localhost"), cmdline.get("host"));
		CPPUNIT_ASSERT_EQUAL(std::string("config.cfg"), cmdline.get("config"));
		CPPUNIT_ASSERT(!cmdline.has("help"));
		CPPUNIT_ASSERT(!cmdline.has("no-daemonize"));
		CPPUNIT_ASSERT(cmdline.usage("Allowed options").find("-p [ --port ] arg (=10000)") != std::string::npos);
	}

	void commandLineMissingValue() {
		const char *argv[3] = {"binary", "--jid", NULL};
		CommandLine cmdline;
		cmdline.addOption("jid", 'j', true, "JID", "");
		std::string error;
		CPPUNIT_ASSERT(!cmdline.parse(2, const_cast<char **>(argv), error));
		CPPUNIT_ASSERT(!error.empty());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (ConfigTest);
