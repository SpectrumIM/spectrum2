#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/rosterresponder.h"
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

class RosterResponderTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(RosterResponderTest);
	CPPUNIT_TEST(addEmptyBuddy);
	CPPUNIT_TEST_SUITE_END();

	public:
		RosterResponder *m_rosterResponder;
		std::string m_buddy;

		void setUp (void) {
			m_buddy = "none";
			setMeUp();
			connectUser();

			m_rosterResponder = new RosterResponder(component->getIQRouter(), userManager);
			m_rosterResponder->onBuddyAdded.connect(boost::bind(&RosterResponderTest::handleBuddyAdded, this, _1, _2));
			m_rosterResponder->onBuddyRemoved.connect(boost::bind(&RosterResponderTest::handleBuddyRemoved, this, _1));
			m_rosterResponder->onBuddyUpdated.connect(boost::bind(&RosterResponderTest::handleBuddyUpdated, this, _1, _2));
			m_rosterResponder->start();

			received.clear();
		}

		void tearDown (void) {
			received.clear();
			disconnectUser();
			tearMeDown();
		}

	void handleBuddyAdded(Buddy *buddy, const Swift::RosterItemPayload &item) {
		m_buddy = buddy->getName();
	}

	void handleBuddyRemoved(Buddy *buddy) {
		m_buddy = buddy->getName();
	}

	void handleBuddyUpdated(Buddy *buddy, const Swift::RosterItemPayload &item) {
		m_buddy = buddy->getName();
	}

	void addEmptyBuddy() {
		Swift::RosterPayload::ref p = Swift::RosterPayload::ref(new Swift::RosterPayload());
		Swift::RosterItemPayload item;
		item.setJID("icq.localhost");
		item.setSubscription(Swift::RosterItemPayload::Both);

		p->addItem(item);
		Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(p, "user@localhost", component->getIQRouter());

		boost::shared_ptr<Swift::IQ> iq(new Swift::IQ(Swift::IQ::Set));
		iq->setTo("icq.localhost");
		iq->setFrom("user@localhost");
		iq->addPayload(p);
		iq->setID("123");
		injectIQ(iq);

		CPPUNIT_ASSERT_EQUAL(std::string("none"), m_buddy);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterResponderTest);
