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
	CPPUNIT_TEST(roomListUser);
	CPPUNIT_TEST(roomInfo);
	CPPUNIT_TEST(roomInfoUser);
	CPPUNIT_TEST(roomListEscaping);
	CPPUNIT_TEST(roomInfoEscaping);
	CPPUNIT_TEST(clearRooms);
	CPPUNIT_TEST(receipts);
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

	void roomListUser() {
		connectUser();
		User *user = userManager->getUser("user@localhost");
		user->addRoomToRoomList("#room2@localhost", "#room2");
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
		CPPUNIT_ASSERT_EQUAL(std::string("#room2@localhost"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[1].getJID().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room2"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[1].getName());
	}

	void roomInfoUser() {
		connectUser();
		User *user = userManager->getUser("user@localhost");
		user->addRoomToRoomList("#room2@localhost", "#room2");
		itemsResponder->addRoom("#room@localhost", "#room");

		boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		iq->setTo("#room2@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		CPPUNIT_ASSERT_EQUAL(std::string("#room2"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getName());
		CPPUNIT_ASSERT_EQUAL(std::string("conference"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getCategory());
		CPPUNIT_ASSERT_EQUAL(std::string("text"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getType());
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

	void roomListEscaping() {
		itemsResponder->addRoom(Swift::JID::getEscapedNode("19:room@localhost") + "@" + component->getJID().toString(), "#room");

		boost::shared_ptr<Swift::DiscoItems> payload(new Swift::DiscoItems());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoItems>());
		CPPUNIT_ASSERT_EQUAL(std::string("19\\3aroom\\40localhost@localhost"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[0].getJID().toString());
		CPPUNIT_ASSERT_EQUAL(std::string("#room"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[0].getName());
	}

	void roomInfoEscaping() {
		itemsResponder->addRoom(Swift::JID::getEscapedNode("19:room@localhost") + "@" + component->getJID().toString(), "#room");

		boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		iq->setTo("19\\3aroom\\40localhost@localhost");
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

	void receipts() {
		boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
		boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
		iq->setFrom("user@localhost");
		iq->setTo("buddy@localhost");
		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		CPPUNIT_ASSERT(!getStanza(received[0])->getPayload<Swift::DiscoInfo>()->hasFeature("urn:xmpp:receipts"));
		received.clear();

		cfg->updateBackendConfig("[features]\nreceipts=1\n");

		injectIQ(iq);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>()->hasFeature("urn:xmpp:receipts"));
		received.clear();
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (DiscoItemsResponderTest);
