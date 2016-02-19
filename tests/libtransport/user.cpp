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

#if !HAVE_SWIFTEN_3
#define get_value_or(X) substr()
#endif

class UserTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(UserTest);
	CPPUNIT_TEST(sendCurrentPresence);
    CPPUNIT_TEST(handlePresence);
	CPPUNIT_TEST(handlePresenceJoinRoom);
	CPPUNIT_TEST(handlePresenceJoinRoomTwoResources);
// 	CPPUNIT_TEST(handlePresenceLeaveRoom); // tested as part of other tests
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResources);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesOneDisconnects);
	CPPUNIT_TEST(handlePresenceLeaveRoomBouncer);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesBouncer);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesOneDisconnectsBouncer);
	CPPUNIT_TEST(handlePresenceLeaveRoomTwoResourcesAnotherOneDisconnects);
	CPPUNIT_TEST(leaveJoinedRoom);
	CPPUNIT_TEST(doNotLeaveNormalChat);
	CPPUNIT_TEST(joinRoomBeforeConnected);
	CPPUNIT_TEST(handleDisconnected);
	CPPUNIT_TEST(handleDisconnectedReconnect);
	CPPUNIT_TEST(joinRoomHandleDisconnectedRejoin);
	CPPUNIT_TEST(joinRoomAfterFlagNotAuthorized);
	CPPUNIT_TEST(requestVCard);
	CPPUNIT_TEST_SUITE_END();

	public:
		std::string room;
		std::string roomNickname;
		std::string roomPassword;
		std::string photo;
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
			photo = "";

			setMeUp();
			userManager->onUserCreated.connect(boost::bind(&UserTest::handleUserCreated, this, _1));
			connectUser();
			received.clear();

			frontend->onVCardUpdated.connect(boost::bind(&UserTest::handleVCardUpdated, this, _1, _2));
		}

		void tearDown (void) {
			received.clear();
			if (!disconnected) {
				disconnectUser();
			}
			userManager->removeAllUsers();
			tearMeDown();
		}

	void handleVCardUpdated(User *user, boost::shared_ptr<Swift::VCard> v) {
		photo = Swift::byteArrayToString(v->getPhoto());
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
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string("password"), roomPassword);

		room = "";
		roomNickname = "";
		roomPassword = "";

		// simulate that backend joined the room
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "room", true);
		conv->addJID("user@localhost/resource");
		conv->setNickname("hanzz");
		user->getConversationManager()->addConversation(conv);

		received.clear();
		injectPresence(response);
		loop->processEvents();

		// no presence received in server mode, just disco#info
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceJoinRoomTwoResources() {
		handlePresenceJoinRoom();
		User *user = userManager->getUser("user@localhost");

		// Add 1 participant
		Conversation *conv = user->getConversationManager()->getConversation("room");
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");

		// Connect 2nd resource
		connectSecondResource();
		received2.clear();
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
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
		CPPUNIT_ASSERT_EQUAL(std::string("room@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received2[1]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received2[1])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received2[1])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received2[1])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
	}

	void handlePresenceLeaveRoom() {
		received.clear();
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

// 		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
// 		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
// 		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
	}

	void handlePresenceLeaveRoomTwoResources() {
		handlePresenceJoinRoomTwoResources();
		received.clear();

		// User is still connected from resource2, so he should not leave the room
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		// disconnect also from resource
		// User is still connected from resource2, so he should not leave the room
		response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource2");
		response->setType(Swift::Presence::Unavailable);

		payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
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

		Conversation *conv = user->getConversationManager()->getConversation("room");
		CPPUNIT_ASSERT_EQUAL(1, (int) conv->getJIDs().size());
		CPPUNIT_ASSERT_EQUAL(Swift::JID("user@localhost/resource2"), conv->getJIDs().front());
	}

	void handlePresenceLeaveRoomTwoResourcesAnotherOneDisconnects() {
		handlePresenceJoinRoom();
		User *user = userManager->getUser("user@localhost");

		// Add 1 participant
		Conversation *conv = user->getConversationManager()->getConversation("room");
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");

		// Connect 2nd resource
		connectSecondResource();
		received2.clear();
		received.clear();

		// User is still connected from resource2, but not in room, so we should leave the room
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);
		injectPresence(response);
		loop->processEvents();


		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		conv = user->getConversationManager()->getConversation("room");
		CPPUNIT_ASSERT(!conv);
	}

	void handlePresenceLeaveRoomBouncer() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
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
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Unavailable);

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		room = "something";
		// disconnect also from resource
		// User is still connected from resource2, so he should not leave the room
		response = Swift::Presence::create();
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource2");
		response->setType(Swift::Presence::Unavailable);

		payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

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

		Conversation *conv = user->getConversationManager()->getConversation("room");
		CPPUNIT_ASSERT_EQUAL(1, (int) conv->getJIDs().size());
		CPPUNIT_ASSERT_EQUAL(Swift::JID("user@localhost/resource2"), conv->getJIDs().front());
	}

	void leaveJoinedRoom() {
		User *user = userManager->getUser("user@localhost");
		handlePresenceJoinRoom();

		CPPUNIT_ASSERT(user->getConversationManager()->getConversation("room"));

		received.clear();
		handlePresenceLeaveRoom();

		CPPUNIT_ASSERT(!user->getConversationManager()->getConversation("room"));
	}

	void doNotLeaveNormalChat() {
		User *user = userManager->getUser("user@localhost");

		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "buddy1@test");
		user->getConversationManager()->addConversation(conv);

		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		user->getConversationManager()->removeJID("user@localhost/resource");
		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);
		Conversation *cv = user->getConversationManager()->getConversation("buddy1@test");
		CPPUNIT_ASSERT(cv);
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
		CPPUNIT_ASSERT_EQUAL(std::string("Connection error"), m->getBody().get_value_or(""));

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
		response->setTo("room@localhost/hanzz");
		response->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(boost::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT_EQUAL(std::string(""), room);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string(""), roomPassword);

		user->setConnected(true);
		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
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

		CPPUNIT_ASSERT_EQUAL(std::string("room"), room);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), roomNickname);
		CPPUNIT_ASSERT_EQUAL(std::string("password"), roomPassword);
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

	void joinRoomAfterFlagNotAuthorized() {
		User *user = userManager->getUser("user@localhost");
		handlePresenceJoinRoom();

		Conversation *conv = user->getConversationManager()->getConversation("room");
		conv->handleParticipantChanged("hanzz", Conversation::PARTICIPANT_FLAG_NOT_AUTHORIZED, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Error, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::ErrorPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::ErrorPayload::NotAuthorized, getStanza(received[0])->getPayload<Swift::ErrorPayload>()->getCondition());

		received.clear();
		handlePresenceJoinRoom();
	}

	void requestVCard() {
		User *user = userManager->getUser("user@localhost");
		user->setStorageBackend(storage);

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::VCardUpdate("hash")));

		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		Swift::VCard::ref payload1 = getStanza(received[1])->getPayload<Swift::VCard>();
		CPPUNIT_ASSERT(payload1);

		boost::shared_ptr<Swift::VCard> vcard(new Swift::VCard());
		vcard->setPhoto(Swift::createByteArray("photo"));
		injectIQ(Swift::IQ::createResult(getStanza(received[1])->getFrom(), getStanza(received[1])->getTo(), getStanza(received[1])->getID(), vcard));
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		CPPUNIT_ASSERT_EQUAL(std::string("photo"), photo);

		received.clear();
		injectPresence(response);
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserTest);
