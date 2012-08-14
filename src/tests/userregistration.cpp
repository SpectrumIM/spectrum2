#include "transport/userregistry.h"
#include "transport/userregistration.h"
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
#include "basictest.h"

using namespace Transport;

class UserRegistrationTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(UserRegistrationTest);
	CPPUNIT_TEST(getForm);
	CPPUNIT_TEST(getFormRegistered);
	CPPUNIT_TEST(registerUser);
	CPPUNIT_TEST(unregisterUser);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			setMeUp();
			received.clear();
		}

		void tearDown (void) {
			received.clear();
			tearMeDown();
		}

		void getForm() {
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", boost::shared_ptr<Swift::Payload>(new Swift::InBandRegistrationPayload()));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>());
			CPPUNIT_ASSERT_EQUAL(false, getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->isRegistered());
			CPPUNIT_ASSERT_EQUAL(std::string(""), *getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->getUsername());
			CPPUNIT_ASSERT_EQUAL(std::string(""), *getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->getPassword());
		}

		void getFormRegistered() {
			UserInfo user;
			user.id = -1;
			user.jid = "user@localhost";
			user.uin = "legacyname";
			user.password = "password";
			storage->setUser(user);

			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", boost::shared_ptr<Swift::Payload>(new Swift::InBandRegistrationPayload()));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>());
			CPPUNIT_ASSERT_EQUAL(true, getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->isRegistered());
			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), *getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->getUsername());
			CPPUNIT_ASSERT_EQUAL(std::string(""), *getStanza(received[0])->getPayload<Swift::InBandRegistrationPayload>()->getPassword());
		}

		void registerUser() {
			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("legacyname");
			reg->setPassword("password");
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", boost::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(true, storage->getUser("user@localhost", user));

			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), user.uin);
			CPPUNIT_ASSERT_EQUAL(std::string("password"), user.password);
		}

		void unregisterUser() {
			registerUser();
			received.clear();

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setRemove(true);
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", boost::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserRegistrationTest);
