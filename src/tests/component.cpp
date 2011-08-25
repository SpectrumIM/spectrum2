#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/localbuddy.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>

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

class ComponentTest : public CPPUNIT_NS :: TestFixture {
	CPPUNIT_TEST_SUITE(ComponentTest);
	CPPUNIT_TEST(presence);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			std::istringstream ifs("service.server_mode = 1\n");
			cfg = new Config();
			cfg->load(ifs);

			factory = new TestingFactory();

			loop = new Swift::DummyEventLoop();
			factories = new Swift::DummyNetworkFactories(loop);

			userRegistry = new UserRegistry(cfg, factories);

			component = new Component(loop, factories, cfg, factory, userRegistry);
			component->start();

			loop->processEvents();
		}

		void tearDown (void) {
			delete component;
			delete userRegistry;
			delete factories;
			delete factory;
			delete loop;
			delete cfg;
			received.clear();
		}

	void presence() {
		
	}

	private:
		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		TestingFactory *factory;
		Component *component;
		std::vector<std::string> received;
};

CPPUNIT_TEST_SUITE_REGISTRATION (ComponentTest);
