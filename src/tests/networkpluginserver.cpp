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

using namespace Transport;

class NetworkPluginServerTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(NetworkPluginServerTest);
	CPPUNIT_TEST(handleBuddyChangedPayload);
	CPPUNIT_TEST(handleBuddyChangedPayloadNoEscaping);
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

};

CPPUNIT_TEST_SUITE_REGISTRATION (NetworkPluginServerTest);
