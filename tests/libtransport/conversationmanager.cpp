#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include <Swiften/Elements/VCardUpdate.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "basictest.h"

using namespace Transport;

#if !HAVE_SWIFTEN_3
#define get_value_or(X) substr()
#endif

class ConversationManagerTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(ConversationManagerTest);
	CPPUNIT_TEST(conversationSize);
	CPPUNIT_TEST(handleNormalMessages);
	CPPUNIT_TEST(handleNormalMessagesInitiatedFromXMPP);
	CPPUNIT_TEST(handleNormalMessagesHeadline);
	CPPUNIT_TEST(handleGroupchatMessages);
	CPPUNIT_TEST(handleGroupchatMessagesAlias);
	CPPUNIT_TEST(handleGroupchatMessagesBouncer);
	CPPUNIT_TEST(handleGroupchatMessagesBouncerLeave);
	CPPUNIT_TEST(handleGroupchatMessagesTwoResources);
	CPPUNIT_TEST(handleChatstateMessages);
	CPPUNIT_TEST(handleSubjectMessages);
	CPPUNIT_TEST(handleParticipantChanged);
	CPPUNIT_TEST(handleParticipantChangedEscaped);
	CPPUNIT_TEST(handleParticipantChangedEscaped2);
	CPPUNIT_TEST(handleParticipantChangedTwoResources);
	CPPUNIT_TEST(handleParticipantChangedIconHash);
	CPPUNIT_TEST(handlePMFromXMPP);
	CPPUNIT_TEST(handleGroupchatRemoved);
	CPPUNIT_TEST(handleNicknameConflict);
	CPPUNIT_TEST(handleNotAuthorized);
	CPPUNIT_TEST(handleSetNickname);
	CPPUNIT_TEST_SUITE_END();

	public:
		TestingConversation *m_conv;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> m_msg;

		void setUp (void) {
			m_conv = NULL;
			m_msg.reset();
			setMeUp();
			connectUser();
			add2Buddies();
			factory->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
			received.clear();
		}

		void tearDown (void) {
			received.clear();
			disconnectUser();
			factory->onMessageToSend.disconnect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
			tearMeDown();
		}

	void conversationSize() {
		std::cout << " = " << sizeof(Conversation) << " B";
	}

	void handleMessageReceived(TestingConversation *_conv, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &_msg) {
		m_conv = _conv;
		m_msg = _msg;
	}

	void handleChatstateMessages() {
		User *user = userManager->getUser("user@localhost");

		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "buddy1");
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::ChatState>(Swift::ChatState::Composing));

		// Forward it
		conv->handleMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0]))->getPayload<Swift::ChatState>());
		received.clear();

		// send response
		msg->setFrom("user@localhost/resource");
		msg->setTo("buddy1@localhost/bot");
		injectMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT(m_msg->getPayload<Swift::ChatState>());

		received.clear();
	}

	void handleSubjectMessages() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setSubject("subject");
		msg->setType(Swift::Message::Groupchat);

		conv->handleMessage(msg);
		loop->processEvents();

		// No response, because presence with code 110 has not been sent yet and we must not send
		// subject before this one.
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());

		// this user presence - status code 110
		conv->handleParticipantChanged("nickname", Conversation::PARTICIPANT_FLAG_MODERATOR, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[1])));
		CPPUNIT_ASSERT_EQUAL(std::string("subject"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getSubject());
		received.clear();

		// send response
		msg->setFrom("user@localhost/resource");
		msg->setTo("#room@localhost");
		injectMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("subject"), m_msg->getSubject());

		received.clear();
		delete conv;
	}

	void handleNormalMessages() {
		User *user = userManager->getUser("user@localhost");

		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "buddy1@test");
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there<>!");

		// Forward it
		conv->handleMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there<>!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());
		
		received.clear();

		// send response
		msg->setFrom("user@localhost/resource");
		msg->setTo("buddy1\\40test@localhost/bot");
		msg->setBody("response<>!");
		injectMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("response<>!"), m_msg->getBody().get_value_or(""));

		// send another message from legacy network, should be sent to user@localhost/resource now
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg2(new Swift::Message());
		msg2->setBody("hi there!");

		// Forward it
		conv->handleMessage(msg2);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());
		
		received.clear();

		// disable jid_escaping
		std::istringstream ifs("service.server_mode = 1\nservice.jid_escaping=0\nservice.jid=localhost\nservice.more_resources=1\n");
		cfg->load(ifs);

		// and now to bare JID again...
		user->getConversationManager()->resetResources();
		conv->handleMessage(msg2);
		loop->processEvents();

		// enable jid_escaping again
		std::istringstream ifs2("service.server_mode = 1\nservice.jid_escaping=1\nservice.jid=localhost\nservice.more_resources=1\n");
		cfg->load(ifs2);
		
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1%test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());
		
		received.clear();
	}

	void handleNormalMessagesInitiatedFromXMPP() {
		User *user = userManager->getUser("user@localhost");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setFrom("user@localhost/resource");
		msg->setTo("buddy1@localhost/bot");
		msg->setBody("hi there<>!");
		injectMessage(msg);

		// Forward it
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("hi there<>!"), m_msg->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("BuddY1"), m_conv->getLegacyName());
		
		TestingConversation *conv = (TestingConversation *) user->getConversationManager()->getConversation("BuddY1");
		CPPUNIT_ASSERT(conv);
		CPPUNIT_ASSERT_EQUAL(std::string("BuddY1"), conv->getLegacyName());
	}

	void handleNormalMessagesHeadline() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("send_headlines", "1");

		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "buddy1@test");
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there<>!");
		msg->setType(Swift::Message::Headline);

		// Forward it
		conv->handleMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Message::Headline, dynamic_cast<Swift::Message *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("hi there<>!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());

		received.clear();
		user->addUserSetting("send_headlines", "0");
		// Forward it
		conv->handleMessage(msg);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Message::Chat, dynamic_cast<Swift::Message *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("hi there<>!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());

		received.clear();
		msg->setType(Swift::Message::Chat);
		user->addUserSetting("send_headlines", "1");
		// Forward it
		conv->handleMessage(msg);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Message::Chat, dynamic_cast<Swift::Message *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("hi there<>!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("buddy1\\40test@localhost/bot"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());
	}

	void handleGroupchatMessages() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// reset resources should not touch this resource
		user->getConversationManager()->resetResources();

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");

		// Forward it
		conv->handleMessage(msg, "anotheruser");

		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());

		received.clear();

		// send response
		msg->setFrom("user@localhost/resource");
		msg->setTo("#room@localhost");
		msg->setBody("response!");
		msg->setType(Swift::Message::Groupchat);
		injectMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("response!"), m_msg->getBody().get_value_or(""));
	}

	void handleGroupchatMessagesAlias() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message", "", "", "alias");
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/alias"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		received.clear();

		// reset resources should not touch this resource
		user->getConversationManager()->resetResources();

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");

		// Forward it
		conv->handleMessage(msg, "anotheruser");

		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/alias"), dynamic_cast<Swift::Message *>(getStanza(received[0]))->getFrom().toString());
	}

	void handleGroupchatMessagesBouncer() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg0(new Swift::Message());
		msg0->setSubject("subject");
		conv->handleMessage(msg0, "anotheruser");

		CPPUNIT_ASSERT(!user->shouldCacheMessages());

		// disconnectUser
		userManager->disconnectUser("user@localhost");
		dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
		loop->processEvents();

		CPPUNIT_ASSERT(user->shouldCacheMessages());

		// reset resources should not touch this resource
		user->getConversationManager()->resetResources();

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");
		conv->handleMessage(msg, "anotheruser");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg2(new Swift::Message());
		msg2->setBody("hi there2!");
		conv->handleMessage(msg2, "anotheruser");

		loop->processEvents();
		// Presence from the room when disconnecting the user
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		received.clear();

		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		userRegistry->onPasswordValid(Swift::JID("user@localhost/resource"));
		loop->processEvents();

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(7, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[2])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/hanzz"), dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[2])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(std::string("nickname"), *getStanza(received[2])->getPayload<Swift::MUCUserPayload>()->getItems()[0].nick);
		CPPUNIT_ASSERT_EQUAL(303, getStanza(received[2])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[2].code);

		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[4])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getFrom().toString());

		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[5])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there2!"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getFrom().toString());

		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[6])));
		CPPUNIT_ASSERT_EQUAL(std::string("subject"), dynamic_cast<Swift::Message *>(getStanza(received[6]))->getSubject());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[6]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[6]))->getFrom().toString());
	}

	void handleGroupchatMessagesBouncerLeave() {
		User *user = userManager->getUser("user@localhost");
		user->addUserSetting("stay_connected", "1");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		CPPUNIT_ASSERT(!user->shouldCacheMessages());

		Swift::Presence::ref response3 = Swift::Presence::create();
		response3->setType(Swift::Presence::Unavailable);
		response3->setTo("#room@localhost/hanzz");
		response3->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload3 = new Swift::MUCPayload();
		payload3->setPassword("password");
		response3->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(payload3));
		injectPresence(response3);
		loop->processEvents();

		CPPUNIT_ASSERT(!user->shouldCacheMessages());

		// reset resources should not touch this resource
		user->getConversationManager()->resetResources();

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");
		conv->handleMessage(msg, "anotheruser");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg2(new Swift::Message());
		msg2->setBody("hi there2!");
		conv->handleMessage(msg2, "anotheruser");

		loop->processEvents();
		// Presence to ack the user leave
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		received.clear();

		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		userRegistry->onPasswordValid(Swift::JID("user@localhost/resource"));
		loop->processEvents();

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("#room@localhost/hanzz");
		response->setFrom("user@localhost/resource");

		Swift::MUCPayload *payload = new Swift::MUCPayload();
		payload->setPassword("password");
		response->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(payload));
		injectPresence(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(6, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[2])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/hanzz"), dynamic_cast<Swift::Presence *>(getStanza(received[2]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[2])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(std::string("nickname"), *getStanza(received[2])->getPayload<Swift::MUCUserPayload>()->getItems()[0].nick);
		CPPUNIT_ASSERT_EQUAL(303, getStanza(received[2])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[2].code);

		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[4])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[4]))->getFrom().toString());

		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[5])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there2!"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received[5]))->getFrom().toString());

	}

	void handleGroupchatMessagesTwoResources() {
		connectSecondResource();
		received2.clear();
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");
		conv->addJID("user@localhost/resource2");

		// reset resources should not touch this resource
		user->getConversationManager()->resetResources();

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");

		// Forward it
		conv->handleMessage(msg, "anotheruser");

		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received2.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received2[0])));
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), dynamic_cast<Swift::Message *>(getStanza(received2[0]))->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource2"), dynamic_cast<Swift::Message *>(getStanza(received2[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Message *>(getStanza(received2[0]))->getFrom().toString());

		received.clear();

		// send response
		msg->setFrom("user@localhost/resource2");
		msg->setTo("#room@localhost");
		msg->setBody("response!");
		msg->setType(Swift::Message::Groupchat);
		injectMessage(msg);
		loop->processEvents();
		
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("response!"), m_msg->getBody().get_value_or(""));
	}

	void handleParticipantChanged() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);

		received.clear();

		// this user presence - status code 110
		conv->handleParticipantChanged("nickname", Conversation::PARTICIPANT_FLAG_MODERATOR, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/nickname"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Admin, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Moderator, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		CPPUNIT_ASSERT_EQUAL(110, getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[0].code);

		received.clear();

		// renamed - status code 303
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_MODERATOR, Swift::StatusShow::Away, "my status message", "hanzz");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Admin, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Moderator, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz"), *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].nick);
		CPPUNIT_ASSERT_EQUAL(303, getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[0].code);
		delete conv;
	}

	void handleParticipantChangedEscaped() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "19:70027094a9c84c518535a610766bed65@thread.skype", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("19\\3a70027094a9c84c518535a610766bed65%thread.skype@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		delete conv;
	}

	void handleParticipantChangedEscaped2() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "19:70027094a9c84c518535a610766bed65@thread.skype", true);
		conv->setMUCEscaping(true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("19\\3a70027094a9c84c518535a610766bed65\\40thread.skype@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		delete conv;
	}

	void handleParticipantChangedIconHash() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "19:70027094a9c84c518535a610766bed65@thread.skype", true);
		conv->setMUCEscaping(true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message", "", "hash");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("19\\3a70027094a9c84c518535a610766bed65\\40thread.skype@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);

		Swift::VCardUpdate::ref payload = getStanza(received[0])->getPayload<Swift::VCardUpdate>();
		CPPUNIT_ASSERT(payload);
		delete conv;
	}

	void handleParticipantChangedTwoResources() {
		connectSecondResource();
		received2.clear();
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");
		conv->addJID("user@localhost/resource2");

		// normal presence
		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received2.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received2[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::Away, dynamic_cast<Swift::Presence *>(getStanza(received2[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource2"), dynamic_cast<Swift::Presence *>(getStanza(received2[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/anotheruser"), dynamic_cast<Swift::Presence *>(getStanza(received2[0]))->getFrom().toString());
		CPPUNIT_ASSERT(getStanza(received2[0])->getPayload<Swift::MUCUserPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Member, *getStanza(received2[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].affiliation);
		CPPUNIT_ASSERT_EQUAL(Swift::MUCOccupant::Participant, *getStanza(received2[0])->getPayload<Swift::MUCUserPayload>()->getItems()[0].role);
		delete conv;
	}

	void handlePMFromXMPP() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->setJID("user@localhost/resource");

		conv->handleParticipantChanged("anotheruser", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		received.clear();
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
		msg->setBody("hi there!");
		msg->setFrom("user@localhost/resource");
		msg->setTo("#room@localhost/anotheruser");
		msg->setBody("hi there!");
		injectMessage(msg);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT(m_msg);
		CPPUNIT_ASSERT_EQUAL(std::string("hi there!"), m_msg->getBody().get_value_or(""));
		CPPUNIT_ASSERT_EQUAL(std::string("#room/anotheruser"), m_conv->getLegacyName());

		Conversation *pmconv = user->getConversationManager()->getConversation("#room/anotheruser");

		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg2(new Swift::Message());
		msg2->setBody("response!");

		pmconv->handleMessage(msg2);
	}

	void handleGroupchatRemoved() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");
		received.clear();
		conv->destroyRoom();
		delete conv;

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::StatusShow::None, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getShow());
		CPPUNIT_ASSERT_EQUAL(std::string("user@localhost/resource"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getTo().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost/nickname"), dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getFrom().toString());
		CPPUNIT_ASSERT_EQUAL(332, getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[0].code);
	}

	void handleNicknameConflict() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("nickname", Conversation::PARTICIPANT_FLAG_CONFLICT, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Error, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::ErrorPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::ErrorPayload::Conflict, getStanza(received[0])->getPayload<Swift::ErrorPayload>()->getCondition());
		delete conv;
	}

	void handleNotAuthorized() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");

		// normal presence
		conv->handleParticipantChanged("nickname", Conversation::PARTICIPANT_FLAG_NOT_AUTHORIZED, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Error, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::ErrorPayload>());
		CPPUNIT_ASSERT_EQUAL(Swift::ErrorPayload::NotAuthorized, getStanza(received[0])->getPayload<Swift::ErrorPayload>()->getCondition());
		delete conv;
	}

	void handleSetNickname() {
		User *user = userManager->getUser("user@localhost");
		TestingConversation *conv = new TestingConversation(user->getConversationManager(), "#room", true);
		
		conv->onMessageToSend.connect(boost::bind(&ConversationManagerTest::handleMessageReceived, this, _1, _2));
		conv->setNickname("nickname");
		conv->addJID("user@localhost/resource");
		loop->processEvents();

		conv->setNickname("nickname2");
		conv->handleParticipantChanged("nickname2", Conversation::PARTICIPANT_FLAG_NONE, Swift::StatusShow::Away, "my status message");
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(110, getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[0].code);
		CPPUNIT_ASSERT_EQUAL(210, getStanza(received[0])->getPayload<Swift::MUCUserPayload>()->getStatusCodes()[1].code);
		delete conv;
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (ConversationManagerTest);
