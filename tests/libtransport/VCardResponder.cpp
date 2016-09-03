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
#include "vcardresponder.h"

using namespace Transport;

class VCardResponderTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(VCardResponderTest);
	CPPUNIT_TEST(handleGetRequestMUC);
	CPPUNIT_TEST_SUITE_END();

	public:
		std::string vcardName;
		unsigned int vcardId;

		void setUp (void) {
			setMeUp();
			connectUser();
			received.clear();
			component->getFrontend()->onVCardRequired.connect(boost::bind(&VCardResponderTest::handleVCardRequired, this, _1, _2, _3));
		}

		void tearDown (void) {
			tearMeDown();
		}

	void handleVCardRequired(User *user, const std::string &name, unsigned int id) {
		vcardName = name;
		vcardId = id;
	}

	void handleGetRequestMUC() {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::VCard> payload(new Swift::VCard());
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "foobar", payload);
		iq->setFrom("user@localhost/me");
		iq->setTo("#room@localhost/user");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, (int) received.size());
		CPPUNIT_ASSERT_EQUAL(std::string("#room/user"), vcardName);

		userManager->sendVCard(vcardId, payload);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::VCard>());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (VCardResponderTest);
