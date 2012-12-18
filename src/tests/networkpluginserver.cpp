#include "transport/userregistry.h"
#include "transport/userregistration.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"
#include "transport/settingsadhoccommand.h"
#include "transport/adhocmanager.h"
#include "transport/protocol.pb.h"
#include "transport/networkpluginserver.h"
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
#include <cppunit/TestListener.h>
#include <cppunit/Test.h>
#include <time.h>    // for clock()

using namespace Transport;

class Clock {
	public:
		double m_beginTime;
		double m_elapsedTime;

		void start() {
			m_beginTime = clock();
		}

		void end() {
			m_elapsedTime = double(clock() - m_beginTime) / CLOCKS_PER_SEC;
		}

		double elapsedTime() const {
			return m_elapsedTime;
		}
};

class NetworkPluginServerTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(NetworkPluginServerTest);
	CPPUNIT_TEST(handleBuddyChangedPayload);
	CPPUNIT_TEST(handleBuddyChangedPayloadNoEscaping);
	CPPUNIT_TEST(handleBuddyChangedPayloadUserContactInRoster);
	CPPUNIT_TEST(handleMessageHeadline);

	CPPUNIT_TEST(benchmarkHandleBuddyChangedPayload);
	CPPUNIT_TEST_SUITE_END();

	public:
		NetworkPluginServer *serv;

		void setUp (void) {
			setMeUp();

			serv = new NetworkPluginServer(component, cfg, userManager, NULL, NULL);
			connectUser();
			received.clear();
		}

		void tearDown (void) {
			received.clear();
			disconnectUser();
			delete serv;
			tearMeDown();
		}

		void benchmarkHandleBuddyChangedPayload() {
			Clock clk;
			std::vector<std::string> lst;
			for (int i = 0; i < 2000; i++) {
				pbnetwork::Buddy buddy;
				buddy.set_username("user@localhost");
				buddy.set_buddyname("buddy" + boost::lexical_cast<std::string>(i)  + "@test");

				std::string message;
				buddy.SerializeToString(&message);
				lst.push_back(message);
			}

			std::vector<std::string> lst2;
			for (int i = 0; i < 2000; i++) {
				pbnetwork::Buddy buddy;
				buddy.set_username("user@localhost");
				buddy.set_buddyname("buddy" + boost::lexical_cast<std::string>(i)  + "@test");
				buddy.set_status((pbnetwork::StatusType) 2);

				std::string message;
				buddy.SerializeToString(&message);
				lst2.push_back(message);
			}

			clk.start();
			for (int i = 0; i < 2000; i++) {
				serv->handleBuddyChangedPayload(lst[i]);
				received.clear();
			}
			for (int i = 0; i < 2000; i++) {
				serv->handleBuddyChangedPayload(lst2[i]);
				received.clear();
			}
			clk.end();
			std::cerr << " " << clk.elapsedTime() << " s";
		}

		void handleBuddyChangedPayload() {
			User *user = userManager->getUser("user@localhost");

			pbnetwork::Buddy buddy;
			buddy.set_username("user@localhost");
			buddy.set_buddyname("buddy1@test");

			std::string message;
			buddy.SerializeToString(&message);

			serv->handleBuddyChangedPayload(message);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			Swift::RosterPayload::ref payload1 = getStanza(received[0])->getPayload<Swift::RosterPayload>();
			CPPUNIT_ASSERT_EQUAL(1, (int) payload1->getItems().size());
			Swift::RosterItemPayload item = payload1->getItems()[0];
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost"), item.getJID().toString());
		}

		void handleBuddyChangedPayloadNoEscaping() {
			std::istringstream ifs("service.server_mode = 1\nservice.jid_escaping=0\nservice.jid=localhost\nservice.more_resources=1\n");
			cfg->load(ifs);
			User *user = userManager->getUser("user@localhost");

			pbnetwork::Buddy buddy;
			buddy.set_username("user@localhost");
			buddy.set_buddyname("buddy1@test");

			std::string message;
			buddy.SerializeToString(&message);

			serv->handleBuddyChangedPayload(message);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			Swift::RosterPayload::ref payload1 = getStanza(received[0])->getPayload<Swift::RosterPayload>();
			CPPUNIT_ASSERT_EQUAL(1, (int) payload1->getItems().size());
			Swift::RosterItemPayload item = payload1->getItems()[0];
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1%test@localhost"), item.getJID().toString());

			std::istringstream ifs2("service.server_mode = 1\nservice.jid_escaping=1\nservice.jid=localhost\nservice.more_resources=1\n");
			cfg->load(ifs2);
		}

		void handleBuddyChangedPayloadUserContactInRoster() {
			User *user = userManager->getUser("user@localhost");

			pbnetwork::Buddy buddy;
			buddy.set_username("user@localhost");
			buddy.set_buddyname("user");

			std::string message;
			buddy.SerializeToString(&message);

			serv->handleBuddyChangedPayload(message);
			CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		}

		void handleMessageHeadline() {
			User *user = userManager->getUser("user@localhost");

			pbnetwork::ConversationMessage m;
			m.set_username("user@localhost");
			m.set_buddyname("user");
			m.set_message("msg");
			m.set_nickname("");
			m.set_xhtml("");
			m.set_timestamp("");
			m.set_headline(true);

			std::string message;
			m.SerializeToString(&message);

			serv->handleConvMessagePayload(message, false);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Message::Chat, dynamic_cast<Swift::Message *>(getStanza(received[0]))->getType());

			received.clear();
			user->addUserSetting("send_headlines", "1");
			serv->handleConvMessagePayload(message, false);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Message::Headline, dynamic_cast<Swift::Message *>(getStanza(received[0]))->getType());

		}
};

CPPUNIT_TEST_SUITE_REGISTRATION (NetworkPluginServerTest);
