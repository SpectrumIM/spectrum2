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

class UserManagerTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(UserManagerTest);
	CPPUNIT_TEST(connectUser);
	CPPUNIT_TEST(connectTwoResources);
	CPPUNIT_TEST(connectUserTransportDisabled);
	CPPUNIT_TEST(connectUserRegistrationNeeded);
	CPPUNIT_TEST(connectUserRegistrationNeededRegistered);
	CPPUNIT_TEST(handleProbePresence);
	CPPUNIT_TEST(disconnectUser);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			setMeUp();
		}

		void tearDown (void) {
			tearMeDown();
		}

	void connectUserTransportDisabled() {
		addUser();
		storage->updateUserSetting(1, "enable_transport", "0");
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());

		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(!user);
	}

	void connectUserRegistrationNeeded() {
		cfg->updateBackendConfig("[registration]\nneedRegistration=1\n");
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		CPPUNIT_ASSERT(streamEnded);
	}

	void connectUserRegistrationNeededRegistered() {
		addUser();
		cfg->updateBackendConfig("[registration]\nneedRegistration=1\n");
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());
		CPPUNIT_ASSERT(!streamEnded);
	}

	void handleProbePresence() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Probe);
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(3, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

		Swift::Presence *presence = dynamic_cast<Swift::Presence *>(getStanza(received[1]));
		CPPUNIT_ASSERT(presence);
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, presence->getType());

		presence = dynamic_cast<Swift::Presence *>(getStanza(received[2]));
		CPPUNIT_ASSERT(presence);
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, presence->getType());

	}

	void connectTwoResources() {
		connectUser();
		add2Buddies();
		connectSecondResource();

		// we should get presences
		CPPUNIT_ASSERT_EQUAL(4, (int) received2.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[2])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[2]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status1"), dynamic_cast<Swift::Presence *>(getStanza(received2[2]))->getStatus());

		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[3])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[3]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status2"), dynamic_cast<Swift::Presence *>(getStanza(received2[3]))->getStatus());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserManagerTest);
