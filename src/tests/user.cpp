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

using namespace Transport;

class UserTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(UserTest);
	CPPUNIT_TEST(sendCurrentPresence);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			setMeUp();
			connectUser();
			received.clear();
		}

		void tearDown (void) {
			disconnectUser();
			tearMeDown();
		}

	void connectUser() {
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());

		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		UserInfo userInfo = user->getUserInfo();
		CPPUNIT_ASSERT_EQUAL(std::string("password"), userInfo.password);
		CPPUNIT_ASSERT(user->isReadyToConnect() == true);
		CPPUNIT_ASSERT(user->isConnected() == false);

		user->setConnected(true);
		CPPUNIT_ASSERT(user->isConnected() == true);

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
	}

	void sendCurrentPresence() {
		User *user = userManager->getUser("user@localhost");
		user->sendCurrentPresence();

		// We're not forwarding current presence in server-mode
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

	void disconnectUser() {
		userManager->disconnectUser("user@localhost");
		dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserTest);
