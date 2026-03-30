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
	CPPUNIT_TEST(registerUserWithoutRR);
	CPPUNIT_TEST(unregisterUser);
	CPPUNIT_TEST(unregisterEmptyPayload);
	CPPUNIT_TEST(registerUserNotify);
	CPPUNIT_TEST(unregisterUserNotify);
	CPPUNIT_TEST(changePassword);
	CPPUNIT_TEST(registerUserEmpty);
	CPPUNIT_TEST(registerUserNoNeedPassword);
	CPPUNIT_TEST(registerUserFormUnknownType);
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
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(new Swift::InBandRegistrationPayload()));
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

			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(new Swift::InBandRegistrationPayload()));
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
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID(), std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()));
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT_EQUAL(std::string("localhost"), getStanza(received[0])->getPayload<Swift::RosterPayload>()->getItems()[0].getJID().toString());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(true, storage->getUser("user@localhost", user));

			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), user.uin);
			CPPUNIT_ASSERT_EQUAL(std::string("password"), user.password);
		}

		void registerUserWithoutRR(){
			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("legacyname");
			reg->setPassword("password");
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createError(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID());
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		}

		void unregisterUser() {
			registerUser();
			received.clear();

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setRemove(true);
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID(), std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()));
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(false, storage->getUser("user@localhost", user));
		}

		void registerUserNotify() {
			std::istringstream ifs("service.server_mode = 1\nregistration.notify_jid=user@localhost\nservice.jid=localhost\nservice.more_resources=1\n");
			cfg->load(ifs);

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("legacyname");
			reg->setPassword("password");
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID(), std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()));
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());
			CPPUNIT_ASSERT_EQUAL(std::string("localhost"), getStanza(received[0])->getPayload<Swift::RosterPayload>()->getItems()[0].getJID().toString());

			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(std::string("registered: user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getBody().get_value_or(""));

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(true, storage->getUser("user@localhost", user));

			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), user.uin);
			CPPUNIT_ASSERT_EQUAL(std::string("password"), user.password);
		}

		void unregisterUserNotify() {
			registerUserNotify();
			received.clear();

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setRemove(true);
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID(), std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()));
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT(dynamic_cast<Swift::Message *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(std::string("unregistered: user@localhost"), dynamic_cast<Swift::Message *>(getStanza(received[1]))->getBody().get_value_or(""));


			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(false, storage->getUser("user@localhost", user));
		}

		void changePassword() {
			registerUser();
			received.clear();

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("legacyname");
			reg->setPassword("another");
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(true, storage->getUser("user@localhost", user));

			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), user.uin);
			CPPUNIT_ASSERT_EQUAL(std::string("another"), user.password);
		}

		void registerUserEmpty() {
			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("");
			reg->setPassword("password");
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Error, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(false, storage->getUser("user@localhost", user));
		}

		void unregisterEmptyPayload() {
			registerUser();
			received.clear();

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setRemove(true);
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID());
			received.clear();
			injectIQ(iq);
			loop->processEvents();


			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unsubscribe, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Unsubscribed, dynamic_cast<Swift::Presence *>(getStanza(received[1]))->getType());
		}

		void registerUserNoNeedPassword() {
			cfg->updateBackendConfig("[registration]\nneedPassword=0\n");
			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setUsername("legacyname");
			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createResult(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID(), std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()));
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());

			CPPUNIT_ASSERT_EQUAL(std::string("localhost"), getStanza(received[0])->getPayload<Swift::RosterPayload>()->getItems()[0].getJID().toString());

			UserInfo user;
			CPPUNIT_ASSERT_EQUAL(true, storage->getUser("user@localhost", user));

			CPPUNIT_ASSERT_EQUAL(std::string("legacyname"), user.uin);
			CPPUNIT_ASSERT_EQUAL(std::string(""), user.password);
		}

		void registerUserFormUnknownType() {
			Swift::Form::ref form(new Swift::Form(Swift::Form::FormType));

			Swift::FormField::ref type = std::make_shared<Swift::FormField>(Swift::FormField::UnknownType, "jabber:iq:register");
			type->setName("FORM_TYPE");
			form->addField(type);

			Swift::FormField::ref username = std::make_shared<Swift::FormField>(Swift::FormField::UnknownType, "legacyname");
			username->setName("username");
			form->addField(username);

			Swift::FormField::ref password = std::make_shared<Swift::FormField>(Swift::FormField::UnknownType, "password");
			password->setName("password");
			form->addField(password);

			Swift::FormField::ref language = std::make_shared<Swift::FormField>(Swift::FormField::UnknownType, "language");
			language->setName("en");
			form->addField(language);

			Swift::InBandRegistrationPayload *reg = new Swift::InBandRegistrationPayload();
			reg->setForm(form);

			std::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", std::shared_ptr<Swift::Payload>(reg));
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::RosterPayload>());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[1])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[1]))->getType());

			iq = Swift::IQ::createError(Swift::JID("localhost"), getStanza(received[0])->getTo(), getStanza(received[0])->getID());
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::Presence::Subscribe, dynamic_cast<Swift::Presence *>(getStanza(received[0]))->getType());
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserRegistrationTest);
