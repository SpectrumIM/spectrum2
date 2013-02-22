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
	CPPUNIT_TEST(localBuddySize);
	CPPUNIT_TEST(createWithInvalidName);
	CPPUNIT_TEST(buddyFlagsFromJID);
	CPPUNIT_TEST(JIDToLegacyName);
	CPPUNIT_TEST(getSafeName);
	CPPUNIT_TEST(sendPresence);
	CPPUNIT_TEST(setAlias);
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

	void localBuddySize() {
		std::cout << " = " << sizeof(LocalBuddy) << " B";
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

	void getSafeName() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group1");
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1@test", "Buddy 1", grp, BUDDY_JID_ESCAPING);

		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test"), buddy->getSafeName());

		buddy->setFlags(BUDDY_NO_FLAG);
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1%test"), buddy->getSafeName());
	}

	void buddyFlagsFromJID() {
		CPPUNIT_ASSERT_EQUAL(BUDDY_JID_ESCAPING, Buddy::buddyFlagsFromJID("hanzz\\40test@localhost/bot"));
		CPPUNIT_ASSERT_EQUAL(BUDDY_NO_FLAG, Buddy::buddyFlagsFromJID("hanzz%test@localhost/bot"));
	}

	void sendPresence() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group1");
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1", "Buddy 1", grp, BUDDY_JID_ESCAPING);
		buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status1");
		user->getRosterManager()->setBuddy(buddy);
		received.clear();

		buddy->sendPresence();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status1"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getStatus());
	}

	void setAlias() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group1");
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1", "Buddy 1", grp, BUDDY_JID_ESCAPING);
		buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status1");
		user->getRosterManager()->setBuddy(buddy);
		received.clear();

		buddy->setAlias("Buddy 2");
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		Swift::RosterPayload::ref payload1 = getStanza(received[0])->getPayload<Swift::RosterPayload>();
		CPPUNIT_ASSERT(payload1);
		CPPUNIT_ASSERT_EQUAL(1, (int) payload1->getItems().size());
		Swift::RosterItemPayload item = payload1->getItems()[0];
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1"), Buddy::JIDToLegacyName(item.getJID()));
		CPPUNIT_ASSERT_EQUAL(std::string("Buddy 2"), item.getName());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (LocalBuddyTest);
