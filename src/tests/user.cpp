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
	CPPUNIT_TEST(handlePresenceJoinRoomTwoResources);
	CPPUNIT_TEST(handlePresenceLeaveRoom);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResources);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesOneDisconnects);
	CPPUNIT_TEST(handlePresenceLeaveRoomBouncer);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesBouncer);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesOneDisconnectsBouncer);
	CPPUNIT_TEST(leaveJoinedRoom);
	CPPUNIT_TEST(joinRoomBeforeConnected);
	CPPUNIT_TEST(handleDisconnected);
	CPPUNIT_TEST(handleDisconnectedReconnect);
	CPPUNIT_TEST(joinRoomHandleDisconnectedRejoin);
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
			userManager->removeAllUsers();
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
		conv->addJID("user@localhost/resource");
		user->getConversationManager()->addConversation(conv);

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

	void handlePresenceJoinRoomTwoResources() {
		handlePresenceJoinRoom();
		User *user = userManager->getUser("user@localhost");

		// Add 1 participant
		Conversation *conv = user->getConversationManager()->getConversation("#room");
		conv->handleParticipantChanged("anotheruser", 0, Swift::StatusShow::Away, "my status message");

		// Connect 2nd resource
		connectSecondResource();
		received2.clear();
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource2");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		CPPUNIT_ASSERT_EQUAL(2, (int) received2.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[1])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[1]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource2"), dynamic_cast<Swift::Presence *>(getStanza(received2[1]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received2[1]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received2[1])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received2[1])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received2[1])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
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

	void handlePresenceLeaveRoomTwoResources() {
		handlePresenceJoinRoomTwoResources();
		received.clear();

		// User is still connected from resource2, so he should not leave the room
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

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		// disconnect also from resource
		// User is still connected from resource2, so he should not leave the room
		response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource2");
		response->setType(Swift::Presence::Unavailable);

		payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string("#room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceLeaveRoomTwoResourcesOneDisconnects() {
		handlePresenceJoinRoomTwoResources();
		received.clear();
		User *user = userManager->getUser("user@localhost");

		// User is still connected from resource2, so he should not leave the room
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);
		injectPresence(response);
		loop->processEvents();


		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		Conversation *conv = user->getConversationManager()->getConversation("#room");
		CPPUNIT_ASSERT_EQUAL(1, (int) conv->getJIDs().size());
		CPPUNIT_ASSERT_EQUAL(Swift::JID("user@localhost/resource2"), conv->getJIDs().front());
	}

	void handlePresenceLeaveRoomBouncer() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
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

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceLeaveRoomTwoResourcesBouncer() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
		handlePresenceJoinRoomTwoResources();
		received.clear();

		// User is still connected from resource2, so he should not leave the room
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

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		room = "something";
		// disconnect also from resource
		// User is still connected from resource2, so he should not leave the room
		response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource2");
		response->setType(Swift::Presence::Unavailable);

		payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string("something"), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceLeaveRoomTwoResourcesOneDisconnectsBouncer() {
		room = "something";
		handlePresenceJoinRoomTwoResources();
		received.clear();
		User *user = userManager->getUser("user@localhost");

		// User is still connected from resource2, so he should not leave the room
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);
		injectPresence(response);
		loop->processEvents();


		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		Conversation *conv = user->getConversationManager()->getConversation("#room");
		CPPUNIT_ASSERT_EQUAL(1, (int) conv->getJIDs().size());
		CPPUNIT_ASSERT_EQUAL(Swift::JID("user@localhost/resource2"), conv->getJIDs().front());
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

	void handleDisconnectedReconnect() {
		readyToConnect = false;
		User *user = userManager->getUser("user@localhost");
		user->handleDisconnected("Connection error");
		loop->processEvents();

		CPPUNIT_ASSERT(!streamEnded);
		user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);
		CPPUNIT_ASSERT(readyToConnect);

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		injectPresence(response);
		loop->processEvents();
	}

	void joinRoomBeforeConnected() {
		User *user = userManager->getUser("user@localhost");
		user->setConnected(false);

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

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		user->setConnected(true);
		CPPUNIT_ASSERT_EQUAL(std::string("#room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string("password"), roomPassword);
	}

	void joinRoomHandleDisconnectedRejoin() {
		User *user = userManager->getUser("user@localhost");
		handlePresenceJoinRoom();
		handleDisconnectedReconnect();
		room = "";
		roomNickname = "";
		roomPassword = "";
		received.clear();
		user->setConnected(true);

		CPPUNIT_ASSERT_EQUAL(std::string("#room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string("password"), roomPassword);
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserTest);
