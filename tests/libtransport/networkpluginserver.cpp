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
#include <stdint.h>
#include "transport/protocol.pb.h"

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
	CPPUNIT_TEST(handleConvMessageAckPayload);
	CPPUNIT_TEST(handleRawXML);
	CPPUNIT_TEST(handleRawXMLSplit);
	CPPUNIT_TEST(handleRawXMLIQ);

	CPPUNIT_TEST(benchmarkHandleBuddyChangedPayload);
	CPPUNIT_TEST(benchmarkSendUnavailablePresence);
	CPPUNIT_TEST_SUITE_END();

	public:
		NetworkPluginServer *serv;
		NetworkPluginServer::Backend backend;
		Swift::SafeByteArray protobufData;

		void setUp (void) {
			setMeUp();

			serv = new NetworkPluginServer(component, cfg, userManager, NULL);
			connectUser();
			User *user = userManager->getUser("user@localhost");
			user->setData(&backend);
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> client1 = factories->getConnectionFactory()->createConnection();
			dynamic_cast<Swift::DummyConnection *>(client1.get())->onDataSent.connect(boost::bind(&NetworkPluginServerTest::handleDataSent, this, _1));
			backend.connection = client1;

			received.clear();
		}

		void tearDown (void) {
			received.clear();
			disconnectUser();
			delete serv;
			tearMeDown();
		}

		void handleDataSent(const Swift::SafeByteArray &data) {
			protobufData = data;
		}

		void handleConvMessageAckPayload() {
			handleMessageHeadline();
			received.clear();
			User *user = userManager->getUser("user@localhost");

			pbnetwork::ConversationMessage m;
			m.set_username("user@localhost");
			m.set_buddyname("user");
			m.set_message("");
			m.set_nickname("");
			m.set_id("testingid");
			m.set_xhtml("");
			m.set_timestamp("");
			m.set_headline(true);

			std::string message;
			m.SerializeToString(&message);

			serv->handleConvMessageAckPayload(message);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0]))->getPayload<Swift::DeliveryReceipt>());
			CPPUNIT_ASSERT_EQUAL(std::string("testingid"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getPayload<Swift::DeliveryReceipt>()->getReceivedID());
		}

		void benchmarkHandleBuddyChangedPayload() {
			Clock clk;
			std::vector<std::string> lst;
			for (int i = 0; i < 2000; i++) {
				pbnetwork::Buddy buddy;
				buddy.set_username("user@localhost");
				buddy.set_buddyname("buddy" + boost::lexical_cast<std::string>(i)  + "@test");
				buddy.set_status((pbnetwork::StatusType) 5);

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

		void benchmarkSendUnavailablePresence() {
			Clock clk;
			std::vector<std::string> lst;
			for (int i = 0; i < 1000; i++) {
				pbnetwork::Buddy buddy;
				buddy.set_username("user@localhost");
				buddy.set_buddyname("buddy" + boost::lexical_cast<std::string>(i)  + "@test");
				buddy.set_status((pbnetwork::StatusType) 5);

				std::string message;
				buddy.SerializeToString(&message);
				lst.push_back(message);
			}

			std::vector<std::string> lst2;
			for (int i = 0; i < 1000; i++) {
				pbnetwork::Buddy buddy;
				buddy.set_username("user@localhost");
				buddy.set_buddyname("buddy" + boost::lexical_cast<std::string>(1000+i)  + "@test");
				buddy.set_status((pbnetwork::StatusType) 2);

				std::string message;
				buddy.SerializeToString(&message);
				lst2.push_back(message);
			}

			
			for (int i = 0; i < 1000; i++) {
				serv->handleBuddyChangedPayload(lst[i]);
				received.clear();
			}
			for (int i = 0; i < 1000; i++) {
				serv->handleBuddyChangedPayload(lst2[i]);
				received.clear();
			}

			User *user = userManager->getUser("user@localhost");
			clk.start();
			user->getRosterManager()->sendUnavailablePresences("user@localhost");
			clk.end();
			std::cerr << " " << clk.elapsedTime() << " s";
		}

		void handleBuddyChangedPayload() {
			User *user = userManager->getUser("user@localhost");

			pbnetwork::Buddy buddy;
			buddy.set_username("user@localhost");
			buddy.set_buddyname("buddy1@test");
			buddy.set_status(pbnetwork::STATUS_NONE);

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
			buddy.set_status(pbnetwork::STATUS_NONE);

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

		void handleRawXML() {
			cfg->updateBackendConfig("[features]\nrawxml=1\n");
			User *user = userManager->getUser("user@localhost");
			std::vector<std::string> grp;
			grp.push_back("group1");
			LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1@domain.tld", "Buddy 1", grp, BUDDY_JID_ESCAPING);
			user->getRosterManager()->setBuddy(buddy);
			received.clear();

			std::string xml = "<presence from='buddy1@domain.tld/res' to='user@localhost'/>";
			serv->handleRawXML(xml);

			std::string xml2 = "<presence from='buddy1@domain.tld/res2' to='user@localhost'/>";
			serv->handleRawXML(xml2);

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40domain.tld@localhost/res"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40domain.tld@localhost/res2"), dynamic_cast<Swift::Presence *>(getStanza(received[1]))->getFrom().toString());

			received.clear();
			user->getRosterManager()->sendUnavailablePresences("user@localhost");

			CPPUNIT_ASSERT_EQUAL(3, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40domain.tld@localhost/res"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40domain.tld@localhost/res2"), dynamic_cast<Swift::Presence *>(getStanza(received[1]))->getFrom().toString());
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, dynamic_cast<Swift::Presence *>(getStanza(received[1]))->getType());
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

		void handleRawXMLSplit() {
			cfg->updateBackendConfig("[features]\nrawxml=1\n");
			User *user = userManager->getUser("user@localhost");
			std::vector<std::string> grp;
			grp.push_back("group1");
			LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1@domain.tld", "Buddy 1", grp, BUDDY_JID_ESCAPING);
			user->getRosterManager()->setBuddy(buddy);
			received.clear();

			std::string xml = "<presence from='buddy1@domain.tld/res' ";
			serv->handleRawXML(xml);

			std::string xml2 = " to='user@localhost'/>";
			serv->handleRawXML(xml2);

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40domain.tld@localhost/res"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		}

		void handleRawXMLIQ() {
			cfg->updateBackendConfig("[features]\nrawxml=1\n");
			User *user = userManager->getUser("user@localhost");
			std::vector<std::string> grp;
			grp.push_back("group1");
			LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1@domain.tld", "Buddy 1", grp, BUDDY_JID_ESCAPING);
			user->getRosterManager()->setBuddy(buddy);
			received.clear();

			std::string xml = "<iq from='buddy1@domain.tld/res' to='user@localhost' type='get' id='1'/>";
			serv->handleRawXML(xml);

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Get, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			injectIQ(Swift::IQ::createResult(getStanza(received[0])->getFrom(), getStanza(received[0])->getTo(), getStanza(received[0])->getID()));
			loop->processEvents();
			
			pbnetwork::WrapperMessage wrapper;
			wrapper.ParseFromArray(&protobufData[4], protobufData.size());
			CPPUNIT_ASSERT_EQUAL(pbnetwork::WrapperMessage_Type_TYPE_RAW_XML, wrapper.type());
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION (NetworkPluginServerTest);
