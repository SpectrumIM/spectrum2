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
	CPPUNIT_TEST(connectUserVipOnlyNonVip);
	CPPUNIT_TEST(handleProbePresence);
	CPPUNIT_TEST(disconnectUser);
	CPPUNIT_TEST(disconnectUserBouncer);
	CPPUNIT_TEST_SUITE_END();

	public:
		Swift::Presence::ref changedPresence;

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

	void connectUserVipOnlyNonVip() {
		addUser();
		std::istringstream ifs("service.server_mode = 1\nservice.jid_escaping=0\nservice.jid=localhost\nservice.vip_only=1\nservice.vip_message=Ahoj\n");
		cfg->load(ifs);
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(3, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[1])));
		CPPUNIT_ASSERT_EQUAL(std::string("Ahoj"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getBody());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("localhost"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getFrom().toString());

		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		CPPUNIT_ASSERT(streamEnded);
		std::istringstream ifs2("service.server_mode = 1\nservice.jid_escaping=1\nservice.jid=localhost\nservice.more_resources=1\n");
		cfg->load(ifs2);
	}

	void handleProbePresence() {
		UserInfo info;
		info.id = 1;
		info.jid = "user@localhost";
		storage->setUser(info);

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
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Probe, presence->getType());

		received.clear();
		response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost");
		response->setType(Swift::Presence::Unsubscribed);
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		presence = dynamic_cast<Swift::Presence *>(getStanza(received[1]));
		CPPUNIT_ASSERT(presence);
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, presence->getType());

		received.clear();
		response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost");
		response->setType(Swift::Presence::Error);
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost");
		response->setType(Swift::Presence::Error);
		response->addPayload(boost::shared_ptr<Swift::ErrorPayload>(new Swift::ErrorPayload(Swift::ErrorPayload::SubscriptionRequired)));
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		presence = dynamic_cast<Swift::Presence *>(getStanza(received[0]));
		CPPUNIT_ASSERT(presence);
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, presence->getType());

		storage->removeUser(1);
		received.clear();
		response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost");
		response->setType(Swift::Presence::Unsubscribed);
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
	}

	void handleUserPresenceChanged(User *user, Swift::Presence::ref presence) {
		changedPresence = presence;
	}

	void connectTwoResources() {
		connectUser();
		add2Buddies();
		connectSecondResource();

		User *user = userManager->getUser("user@localhost");
		user->onPresenceChanged.connect(boost::bind(&UserManagerTest::handleUserPresenceChanged, this, user, _1));

		// we should get presences
		CPPUNIT_ASSERT_EQUAL(4, (int) received2.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[2])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[2]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status1"), dynamic_cast<Swift::Presence *>(getStanza(received2[2]))->getStatus());

		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[3])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[3]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("status2"), dynamic_cast<Swift::Presence *>(getStanza(received2[3]))->getStatus());

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);
		injectPresence(response);

		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Available, changedPresence->getType());

		Swift::Presence::ref response2 = Swift::Presence::create();
		response2->setTo("localhost");
		response2->setFrom("user@localhost/resource2");
		response2->setType(Swift::Presence::Unavailable);
		injectPresence(response2);

		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, changedPresence->getType());
	}

	void disconnectUserBouncer() {
		connectUser();
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
		received.clear();
		userManager->disconnectUser("user@localhost");
		dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT(userManager->getUser("user@localhost"));

		userManager->removeAllUsers();
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserManagerTest);
