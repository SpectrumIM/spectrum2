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

class LocalBuddyTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(LocalBuddyTest);
	CPPUNIT_TEST(createWithInvalidName);
	CPPUNIT_TEST(buddyFlagsFromJID);
	CPPUNIT_TEST(JIDToLegacyName);
	CPPUNIT_TEST(handleBuddyChanged);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			setMeUp();
			connectUser();
			received.clear();
		}

		void tearDown (void) {
			received.clear();
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
		received.clear();
	}

	void createWithInvalidName() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group");

		// with BUDDY_JID_ESCAPING it escapes /
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "msn/something", "Buddy 1", grp, BUDDY_JID_ESCAPING);
		CPPUNIT_ASSERT(buddy->isValid());
		CPPUNIT_ASSERT_EQUAL(std::string("msn\\2fsomething@localhost/bot"), buddy->getJID().toString());
		delete buddy;

		// without BUDDY_JID_ESCAPING it shoudl fail
		buddy = new LocalBuddy(user->getRosterManager(), -1, "msn/something", "Buddy 1", grp);
		CPPUNIT_ASSERT(!buddy->isValid());
		delete buddy;

		buddy = new LocalBuddy(user->getRosterManager(), -1, "\xd7\x92\xd7\x9c\xd7\x99\xd7\x9d@nimbuzz.com", "Buddy 1", grp);
		CPPUNIT_ASSERT(!buddy->isValid());
		delete buddy;
	}

	void JIDToLegacyName() {
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz@test"), Buddy::JIDToLegacyName("hanzz\\40test@localhost/bot"));
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz@test"), Buddy::JIDToLegacyName("hanzz%test@localhost/bot"));
	}

	void buddyFlagsFromJID() {
		CPPUNIT_ASSERT_EQUAL(BUDDY_JID_ESCAPING, Buddy::buddyFlagsFromJID("hanzz\\40test@localhost/bot"));
		CPPUNIT_ASSERT_EQUAL(BUDDY_NO_FLAG, Buddy::buddyFlagsFromJID("hanzz%test@localhost/bot"));
	}

	void handleBuddyChanged() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group1");
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1", "Buddy 1", grp, BUDDY_JID_ESCAPING);
		buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status1");
		user->getRosterManager()->setBuddy(buddy);
		received.clear();

		buddy->handleBuddyChanged();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status1"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getStatus());
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

CPPUNIT_TEST_SUITE_REGISTRATION (LocalBuddyTest);
