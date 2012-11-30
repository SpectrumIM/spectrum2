#include "transport/userregistry.h"
#include "transport/userregistration.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"
#include "transport/settingsadhoccommand.h"
#include "transport/adhocmanager.h"
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

class DiscoItemsResponderTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(DiscoItemsResponderTest);
	CPPUNIT_TEST(roomList);
	CPPUNIT_TEST(roomInfo);
	CPPUNIT_TEST(clearRooms);
	CPPUNIT_TEST_SUITE_END();

	public:

		void setUp (void) {
			setMeUp();
		}

		void tearDown (void) {
			received.clear();
			tearMeDown();
		}

	void roomList() {
		itemsResponder->addRoom("#room@localhost", "#room");

		boost::shared_ptr<Swift::DiscoItems> payload(new Swift::DiscoItems());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoItems>());
		CPPUNIT_ASSERT_EQUAL(std::string("#room@localhost"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[0].getJID().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[0].getName());
	}

	void roomInfo() {
		itemsResponder->addRoom("#room@localhost", "#room");

		boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		iq->setTo("#room@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		CPPUNIT_ASSERT_EQUAL(std::string("#room"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getName());
		CPPUNIT_ASSERT_EQUAL(std::string("conference"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getCategory());
		CPPUNIT_ASSERT_EQUAL(std::string("text"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getType());
	}

	void clearRooms() {
		itemsResponder->addRoom("#room@localhost", "#room");
		itemsResponder->clearRooms();

		boost::shared_ptr<Swift::DiscoItems> payload(new Swift::DiscoItems());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoItems>());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems().empty());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (DiscoItemsResponderTest);
