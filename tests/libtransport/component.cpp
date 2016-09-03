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
			onUserPresenceReceived = false;
			onCapabilitiesReceived = false;

			setMeUp();
			component->onUserPresenceReceived.connect(boost::bind(&ComponentTest::handleUserPresenceReceived, this, _1));
			component->getFrontend()->onCapabilitiesReceived.connect(boost::bind(&ComponentTest::handleUserDiscoInfoReceived, this, _1, _2));
		}

		void tearDown (void) {
			tearMeDown();
		}

	void handleUserDiscoInfoReceived(const Swift::JID& jid, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> info) {
		onCapabilitiesReceived = true;
	}

	void handleUserPresenceReceived(Swift::Presence::ref presence) {
		onUserPresenceReceived = true;
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
		CPPUNIT_ASSERT(onUserPresenceReceived);
	}

	private:
		bool onUserPresenceReceived;
		bool onCapabilitiesReceived;
};

CPPUNIT_TEST_SUITE_REGISTRATION (ComponentTest);
