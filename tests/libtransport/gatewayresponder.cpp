#include "gatewayresponder.h"
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

class GatewayResponderTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(GatewayResponderTest);
	CPPUNIT_TEST(escape);
	CPPUNIT_TEST(noEscapeEscaped);
	CPPUNIT_TEST_SUITE_END();

	public:
		GatewayResponder *m_gatewayResponder;

		void setUp (void) {
			setMeUp();
			connectUser();

			m_gatewayResponder = new GatewayResponder(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter(), userManager);
			m_gatewayResponder->start();

			received.clear();
		}

		void tearDown (void) {
			received.clear();
			disconnectUser();
			delete m_gatewayResponder;
			tearMeDown();
		}

		void escape() {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq(new Swift::IQ(Swift::IQ::Set));
			iq->setTo("icq.localhost");
			iq->setFrom("user@localhost");
			iq->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GatewayPayload>(new Swift::GatewayPayload(Swift::JID(), "", "a@b")));
			iq->setID("123");
			injectIQ(iq);

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::GatewayPayload>());
			CPPUNIT_ASSERT_EQUAL(std::string("a\\40b@localhost"), getStanza(received[0])->getPayload<Swift::GatewayPayload>()->getJID().toString());
		}

		void noEscapeEscaped() {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq(new Swift::IQ(Swift::IQ::Set));
			iq->setTo("icq.localhost");
			iq->setFrom("user@localhost");
			iq->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::GatewayPayload>(new Swift::GatewayPayload(Swift::JID(), "", "a\\40b")));
			iq->setID("123");
			injectIQ(iq);
			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::GatewayPayload>());
			CPPUNIT_ASSERT_EQUAL(std::string("a\\40b@localhost"), getStanza(received[0])->getPayload<Swift::GatewayPayload>()->getJID().toString());
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (GatewayResponderTest);
