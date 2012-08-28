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
    CPPUNIT_TEST(handlePresence);
	CPPUNIT_TEST(handlePresenceJoinRoom);
	CPPUNIT_TEST(handlePresenceLeaveRoom);
	CPPUNIT_TEST(leaveJoinedRoom);
	CPPUNIT_TEST(handleDisconnected);
	CPPUNIT_TEST_SUITE_END();

	public:
		std::string room;
		std::string roomNickname;
		std::string roomPassword;
		bool readyToConnect;
		bool disconnected;
		Swift::Presence::ref changedPresence;

		void setUp (void) {
			disconnected = false;
			readyToConnect = false;
			changedPresence = Swift::Presence::ref();
			room = "";
			roomNickname = "";
			roomPassword = "";

			setMeUp();
			userManager->onUserCreated.connect(boost::bind(&UserTest::handleUserCreated, this, _1));
			connectUser();
			received.clear();
		}

		void tearDown (void) {
			received.clear();
			if (!disconnected) {
				disconnectUser();
			}
			tearMeDown();
		}

	void handleUserCreated(User *user) {
		user->onReadyToConnect.connect(boost::bind(&UserTest::handleUserReadyToConnect, this, user));
		user->onPresenceChanged.connect(boost::bind(&UserTest::handleUserPresenceChanged, this, user, _1));
		user->onRoomJoined.connect(boost::bind(&UserTest::handleRoomJoined, this, user, _1, _2, _3, _4));
		user->onRoomLeft.connect(boost::bind(&UserTest::handleRoomLeft, this, user, _1));
	}

	void handleUserReadyToConnect(User *user) {
		readyToConnect = true;
	}

	void handleUserPresenceChanged(User *user, Swift::Presence::ref presence) {
		changedPresence = presence;
	}

	void handleRoomJoined(User *user, const std::string &jid, const std::string &r, const std::string &nickname, const std::string &password) {
		room = r;
		roomNickname = nickname;
		roomPassword = password;
	}

	void handleRoomLeft(User *user, const std::string &r) {
		room = r;
	}


	void sendCurrentPresence() {
		User *user = userManager->getUser("user@localhost");
		user->sendCurrentPresence();

		// We're not forwarding current presence in server-mode
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

	void handlePresence() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->setShow(Swift::StatusShow::Away);

		injectPresence(response);
		loop->processEvents();

		// no presence received in server mode, just disco#info
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

		CPPUNIT_ASSERT(changedPresence);
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, changedPresence->getShow());
	}

	void handlePresenceJoinRoom() {
		User *user = userManager->getUser("user@localhost");

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		// no presence received in server mode, just disco#info
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

		CPPUNIT_ASSERT_EQUAL(std::string("#room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string("password"), roomPassword);

		room = "";
		roomNickname = "";
		roomPassword = "";

		// simulate that backend joined the room
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);

		received.clear();
		injectPresence(response);
		loop->processEvents();

		// no presence received in server mode, just disco#info
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceLeaveRoom() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string("#room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void leaveJoinedRoom() {
		User *user = userManager->getUser("user@localhost");
		handlePresenceJoinRoom();

		CPPUNIT_ASSERT(user->getConversationManager()->getConversation("#room"));

		received.clear();
		handlePresenceLeaveRoom();

		CPPUNIT_ASSERT(!user->getConversationManager()->getConversation("#room"));
	}

	void handleDisconnected() {
		User *user = userManager->getUser("user@localhost");
		user->handleDisconnected("Connection error", Swift::SpectrumErrorPayload::CONNECTION_ERROR_AUTHENTICATION_FAILED);
		loop->processEvents();

		CPPUNIT_ASSERT(streamEnded);
		user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(!user);

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		Swift::Message *m = dynamic_cast<Swift::Message *>(getStanza(received[0]));
		CPPUNIT_ASSERT_EQUAL(std::string("Connection error"), m->getBody());

		CPPUNIT_ASSERT(dynamic_cast<Swift::StreamError *>(received[1].get()));
		CPPUNIT_ASSERT_EQUAL(std::string("Connection error"), dynamic_cast<Swift::StreamError *>(received[1].get())->getText());

		disconnected = true;
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserTest);
