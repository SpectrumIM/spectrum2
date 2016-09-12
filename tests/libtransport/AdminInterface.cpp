#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/SwiftenCompat.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "BasicSlackTest.h"
#include "transport/AdminInterface.h"

#if !HAVE_SWIFTEN_3
#define get_value_or(X) substr()
#endif

using namespace Transport;

class AdminInterfaceTest : public CPPUNIT_NS :: TestFixture, public BasicSlackTest {
	CPPUNIT_TEST_SUITE(AdminInterfaceTest);
	CPPUNIT_TEST(helpCommand);
	CPPUNIT_TEST(statusCommand);
	CPPUNIT_TEST(joinRoomArgs);
	CPPUNIT_TEST(getOAuth2URLCommand);
	CPPUNIT_TEST(unknownCommand);
	CPPUNIT_TEST(listJoinLeaveRoomsCommand);
	CPPUNIT_TEST(badArgCount);
	CPPUNIT_TEST(commandsCommand);
	CPPUNIT_TEST(variablesCommand);
	CPPUNIT_TEST_SUITE_END();

	public:
		AdminInterface *admin;
		NetworkPluginServer *serv;

		void setUp (void) {
			setMeUp();
			serv = new NetworkPluginServer(component, cfg, userManager, NULL);
			admin = new AdminInterface(component, userManager, serv, storage, NULL);
			component->setAdminInterface(admin);
		}

		void tearDown (void) {
			delete admin;
			delete serv;
			tearMeDown();
		}

	std::string sendAdminMessage(const std::string &cmd) {
		Swift::Message::ref msg = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message>(new Swift::Message());
		msg->setFrom(Swift::JID("me@localhost"));
		msg->setTo(Swift::JID("localhost"));
		msg->setBody(cmd);

		admin->handleMessageReceived(msg);
		return msg->getBody().get_value_or("");
	}

	void helpCommand() {
		std::string resp = sendAdminMessage("help");
		CPPUNIT_ASSERT(resp.find("   VAR   status - Shows instance status\n") != std::string::npos);
	}

	void statusCommand() {
		std::string resp = sendAdminMessage("status");
		CPPUNIT_ASSERT_EQUAL(std::string("Running (0 users connected using 0 backends)"), resp);
	}

	void joinRoomArgs() {
		std::string resp = sendAdminMessage("args join_room");
		CPPUNIT_ASSERT_EQUAL(std::string("nickname - \"Nickname in 3rd-party room\" Example: \"BotNickname\" Type: \"string\"\n"
							"legacy_room - \"3rd-party room name\" Example: \"3rd-party room name\" Type: \"string\"\n"
							"legacy_server - \"3rd-party server\" Example: \"3rd.party.server.org\" Type: \"string\"\n"
							"slack_channel - \"Slack Chanel\" Example: \"mychannel\" Type: \"string\"\n"), resp);
	}

	void getOAuth2URLCommand() {
		std::string resp = sendAdminMessage("get_oauth2_url x y z");
		CPPUNIT_ASSERT(resp.find("https://slack.com/oauth/authorize?client_id=&scope=channels%3Aread%20channels%3Awrite%20team%3Aread%20im%3Aread%20im%3Awrite%20chat%3Awrite%3Abot%20bot&redirect_uri=https%3A%2F%2Fslack.spectrum.im%2Foauth2%2Flocalhost&state=") != std::string::npos);
	}

	void unknownCommand() {
		std::string resp = sendAdminMessage("unknown_command test");
		CPPUNIT_ASSERT_EQUAL(std::string("Error: Unknown variable or command"), resp);
	}

	void listJoinLeaveRoomsCommand() {
		addUser();

		std::string resp = sendAdminMessage("list_rooms user@localhost");
		CPPUNIT_ASSERT_EQUAL(std::string(""), resp);

		resp = sendAdminMessage("join_room user@localhost SlackBot spectrum conference.spectrum.im slack_channel");
		CPPUNIT_ASSERT_EQUAL(std::string("Joined the room"), resp);

		resp = sendAdminMessage("list_rooms user@localhost");
		CPPUNIT_ASSERT_EQUAL(std::string("connected room SlackBot spectrum conference.spectrum.im slack_channel\n"), resp);

		resp = sendAdminMessage("leave_room user@localhost slack_channel");
		CPPUNIT_ASSERT_EQUAL(std::string("Left the room"), resp);

		resp = sendAdminMessage("list_rooms user@localhost");
		CPPUNIT_ASSERT_EQUAL(std::string(""), resp);
	}

	void badArgCount() {
		addUser();
		std::string resp;
		resp = sendAdminMessage("join_room user@localhost SlackBot spectrum conference.spectrum.im slack_channel unknown");
		CPPUNIT_ASSERT_EQUAL(std::string("Error: Too many arguments."), resp);

		resp = sendAdminMessage("join_room user@localhost SlackBot spectrum conference.spectrum.im");
		CPPUNIT_ASSERT_EQUAL(std::string("Error: Argument is missing."), resp);
	}

	void commandsCommand() {
		addUser();
		std::string resp;
		resp = sendAdminMessage("commands");
		CPPUNIT_ASSERT(resp.find("join_room - \"Join the room\" Category: Frontend AccesMode: User Context: User") != std::string::npos);
	}

	void variablesCommand() {
		addUser();
		std::string resp;
		resp = sendAdminMessage("variables");
		CPPUNIT_ASSERT(resp.find("backends_count - \"Number of active backends\" Value: \"0\" Read-only: true") != std::string::npos);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (AdminInterfaceTest);
