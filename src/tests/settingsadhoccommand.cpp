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

class SettingsAdHocCommandTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(SettingsAdHocCommandTest);
	CPPUNIT_TEST(getItems);
	CPPUNIT_TEST(execute);
	CPPUNIT_TEST_SUITE_END();

	public:
		AdHocManager *adhoc;
		SettingsAdHocCommandFactory *settings;

		void setUp (void) {
			setMeUp();

			adhoc = new AdHocManager(component, itemsResponder);
			adhoc->start();
			settings = new SettingsAdHocCommandFactory();
			adhoc->addAdHocCommand(settings);

			received.clear();
		}

		void tearDown (void) {
			received.clear();
			delete adhoc;
			delete settings;
			tearMeDown();
		}

		void getItems() {
			boost::shared_ptr<Swift::DiscoItems> payload(new Swift::DiscoItems());
			payload->setNode("http://jabber.org/protocol/commands");
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoItems>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::DiscoItems>()->getItems()[0].getNode());
		}

		void execute() {
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (SettingsAdHocCommandTest);
