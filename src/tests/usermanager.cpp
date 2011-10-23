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

using namespace Transport;

class TestingConversation : public Conversation {
	public:
		TestingConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
		}

		// Called when there's new message to legacy network from XMPP network
		void sendMessage(boost::shared_ptr<Swift::Message> &message) {
			
		}
};

class TestingFactory : public Factory {
	public:
		TestingFactory() {
		}

		// Creates new conversation (NetworkConversation in this case)
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName) {
			Conversation *nc = new TestingConversation(conversationManager, legacyName);
			return nc;
		}

		// Creates new LocalBuddy
		Buddy *createBuddy(RosterManager *rosterManager, const BuddyInfo &buddyInfo) {
			LocalBuddy *buddy = new LocalBuddy(rosterManager, buddyInfo.id);
			buddy->setAlias(buddyInfo.alias);
			buddy->setName(buddyInfo.legacyName);
			buddy->setSubscription(buddyInfo.subscription);
			buddy->setGroups(buddyInfo.groups);
			buddy->setFlags((BuddyFlag) buddyInfo.flags);
			if (buddyInfo.settings.find("icon_hash") != buddyInfo.settings.end())
				buddy->setIconHash(buddyInfo.settings.find("icon_hash")->second.s);
			return buddy;
		}
};

class UserManagerTest : public CPPUNIT_NS :: TestFixture, public Swift::XMPPParserClient {
	CPPUNIT_TEST_SUITE(UserManagerTest);
	CPPUNIT_TEST(connectUser);
	CPPUNIT_TEST(handleProbePresence);
	CPPUNIT_TEST(disconnectUser);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			streamEnded = false;
			std::istringstream ifs("service.server_mode = 1\n");
			cfg = new Config();
			cfg->load(ifs);

			factory = new TestingFactory();

			loop = new Swift::DummyEventLoop();
			factories = new Swift::DummyNetworkFactories(loop);

			userRegistry = new UserRegistry(cfg, factories);

			component = new Component(loop, factories, cfg, factory, userRegistry);
			component->start();

			userManager = new UserManager(component, userRegistry);

			payloadSerializers = new Swift::FullPayloadSerializerCollection();
			payloadParserFactories = new Swift::FullPayloadParserFactoryCollection();
			parser = new Swift::XMPPParser(this, payloadParserFactories, factories->getXMLParserFactory());

			serverFromClientSession = boost::shared_ptr<Swift::ServerFromClientSession>(new Swift::ServerFromClientSession("id", factories->getConnectionFactory()->createConnection(), 
					payloadParserFactories, payloadSerializers, userRegistry, factories->getXMLParserFactory(), Swift::JID("user@localhost/resource")));
			serverFromClientSession->startSession();

			serverFromClientSession->onDataWritten.connect(boost::bind(&UserManagerTest::handleDataReceived, this, _1));

			dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->addSession(serverFromClientSession);
			parser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
			received.clear();
			loop->processEvents();
		}

		void tearDown (void) {
			dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->removeSession(serverFromClientSession);
			delete component;
			delete userRegistry;
			delete factories;
			delete factory;
			delete loop;
			delete cfg;
			delete parser;
			received.clear();
		}

	void handleDataReceived(const Swift::SafeByteArray &data) {
		parser->parse(safeByteArrayToString(data));
	}

	void handleStreamStart(const Swift::ProtocolHeader&) {
		
	}

	void handleElement(boost::shared_ptr<Swift::Element> element) {
		received.push_back(element);
	}

	void handleStreamEnd() {
		streamEnded = true;
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

	void disconnectUser() {
		connectUser();
		received.clear();

		userManager->disconnectUser("user@localhost");
		dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
	}

	void handleProbePresence() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Probe);
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

		Swift::Presence *presence = dynamic_cast<Swift::Presence *>(getStanza(received[1]));
		CPPUNIT_ASSERT(presence);
		CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unavailable, presence->getType());
	}

	Swift::Stanza *getStanza(boost::shared_ptr<Swift::Element> element) {
		Swift::Stanza *stanza = dynamic_cast<Swift::Stanza *>(element.get());
		CPPUNIT_ASSERT(stanza);
		return stanza;
	}

	private:
		bool streamEnded;
		UserManager *userManager;
		boost::shared_ptr<Swift::ServerFromClientSession> serverFromClientSession;
		Swift::FullPayloadSerializerCollection* payloadSerializers;
		Swift::FullPayloadParserFactoryCollection* payloadParserFactories;
		Swift::XMPPParser *parser;
		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		TestingFactory *factory;
		Component *component;
		std::vector<boost::shared_ptr<Swift::Element> > received;
};

CPPUNIT_TEST_SUITE_REGISTRATION (UserManagerTest);
