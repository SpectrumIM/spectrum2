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

class ComponentTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(ComponentTest);
	CPPUNIT_TEST(handlePresenceWithNode);
	CPPUNIT_TEST(handlePresenceWithoutNode);
	CPPUNIT_TEST(handleErrorPresence);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			setMeUp();
		}

		void tearDown (void) {
			tearMeDown();
		}

	void handlePresenceWithNode() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("somebody@localhost");
		response->setFrom("user@localhost/resource");
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend *>(component->getFrontend())->getStanzaChannel())->onPresenceReceived(response);
		
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

	// Error presence should be ignored
	void handleErrorPresence() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		response->setType(Swift::Presence::Error);
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend *>(component->getFrontend())->getStanzaChannel())->onPresenceReceived(response);
		
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
	}

	void handlePresenceWithoutNode() {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo("localhost");
		response->setFrom("user@localhost/resource");
		dynamic_cast<Swift::ServerStanzaChannel *>(static_cast<XMPPFrontend *>(component->getFrontend())->getStanzaChannel())->onPresenceReceived(response);
		
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		// The presence reached the user manager, which logged the user in.
		CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (ComponentTest);
