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
	CPPUNIT_TEST(getInfo);
	CPPUNIT_TEST(getInfoBare);
	CPPUNIT_TEST(execute);
	CPPUNIT_TEST(executeTwoCommands);
	CPPUNIT_TEST(executeBadSessionID);
	CPPUNIT_TEST(executeNotRegistered);
	CPPUNIT_TEST(cancel);
	CPPUNIT_TEST(propagateUserSetting);
	CPPUNIT_TEST(defaultAccordingToConfig);
	CPPUNIT_TEST_SUITE_END();

	public:
		AdHocManager *adhoc;
		SettingsAdHocCommandFactory *settings;

		void setUp (void) {
			setMeUp();

			adhoc = new AdHocManager(component, itemsResponder, userManager, storage);
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

		void getInfo() {
			boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
			payload->setNode("settings");
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
			CPPUNIT_ASSERT_EQUAL(std::string("automation"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getCategory());
			CPPUNIT_ASSERT_EQUAL(std::string("command-node"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getType());
		}

		void getInfoBare() {
			boost::shared_ptr<Swift::DiscoInfo> payload(new Swift::DiscoInfo());
			payload->setNode("http://jabber.org/protocol/commands");
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Get, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
			CPPUNIT_ASSERT_EQUAL(std::string("automation"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getCategory());
			CPPUNIT_ASSERT_EQUAL(std::string("command-list"), getStanza(received[0])->getPayload<Swift::DiscoInfo>()->getIdentities()[0].getType());
		}

		void executeNotRegistered() {
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Completed, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());
		}

		void execute() {
			addUser();
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("enable_transport"));

			// set enabled_transport = 0
			Swift::FormField::ref f = getStanza(received[0])->getPayload<Swift:: Command>()->getForm()->getField("enable_transport");
			boost::dynamic_pointer_cast<Swift::BooleanFormField>(f)->setValue(false);

			std::string sessionId = getStanza(received[0])->getPayload<Swift::Command>()->getSessionID();

			{
			std::string value = "0";
			int type;
			storage->getUserSetting(1, "enable_transport", type, value);
			CPPUNIT_ASSERT_EQUAL(std::string("1"), value);
			}

			// finish the command
			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			payload->setSessionID(sessionId);
			payload->setForm(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Completed, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			{
			std::string value = "1";
			int type;
			storage->getUserSetting(1, "enable_transport", type, value);
			CPPUNIT_ASSERT_EQUAL(std::string("0"), value);
			}

			received.clear();

			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("enable_transport"));

			// set enabled_transport = 0
			f = getStanza(received[0])->getPayload<Swift:: Command>()->getForm()->getField("enable_transport");
			CPPUNIT_ASSERT_EQUAL(false, boost::dynamic_pointer_cast<Swift::BooleanFormField>(f)->getValue());
		}

		void executeTwoCommands() {
			addUser();
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("enable_transport"));

			received.clear();
			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("enable_transport"));
		}

		void executeBadSessionID() {
			addUser();
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());

			std::string sessionId = "somethingwrong";

			// finish the command
			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			payload->setSessionID(sessionId);
			payload->setForm(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Error, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		}

		void cancel() {
			addUser();
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());

			std::string sessionId = getStanza(received[0])->getPayload<Swift::Command>()->getSessionID();

			// cancel the command
			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			payload->setSessionID(sessionId);
			payload->setAction(Swift::Command::Cancel);
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Canceled, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());
		}

		void propagateUserSetting() {
			connectUser();
			User *user = userManager->getUser("user@localhost");
			CPPUNIT_ASSERT_EQUAL(std::string("0"), user->getUserSetting("send_headlines"));

			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("send_headlines"));

			// set enabled_transport = 0
			Swift::FormField::ref f = getStanza(received[0])->getPayload<Swift:: Command>()->getForm()->getField("send_headlines");
			boost::dynamic_pointer_cast<Swift::BooleanFormField>(f)->setValue(true);

			std::string sessionId = getStanza(received[0])->getPayload<Swift::Command>()->getSessionID();

			// finish the command
			payload = boost::shared_ptr<Swift::Command>(new Swift::Command("settings"));
			payload->setSessionID(sessionId);
			payload->setForm(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			received.clear();
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(std::string("1"), user->getUserSetting("send_headlines"));
		}

		void defaultAccordingToConfig() {
			std::istringstream ifs("service.server_mode = 1\nservice.jid_escaping=0\nservice.jid=localhost\nsettings.send_headlines=1\n");
			cfg->load(ifs);
			connectUser();
			User *user = userManager->getUser("user@localhost");
			CPPUNIT_ASSERT_EQUAL(std::string("1"), user->getUserSetting("send_headlines"));
			boost::shared_ptr<Swift::Command> payload(new Swift::Command("settings"));
			boost::shared_ptr<Swift::IQ> iq = Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID("localhost"), "id", payload);
			iq->setFrom("user@localhost");
			injectIQ(iq);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(1, (int) received.size());

			CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
			CPPUNIT_ASSERT_EQUAL(Swift::IQ::Result, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());

			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>());
			CPPUNIT_ASSERT_EQUAL(std::string("settings"), getStanza(received[0])->getPayload<Swift::Command>()->getNode());
			CPPUNIT_ASSERT_EQUAL(Swift::Command::Executing, getStanza(received[0])->getPayload<Swift::Command>()->getStatus());

			// form element
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm());
			CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::Command>()->getForm()->getField("send_headlines"));
			Swift::FormField::ref f = getStanza(received[0])->getPayload<Swift:: Command>()->getForm()->getField("send_headlines");
			CPPUNIT_ASSERT_EQUAL(true, boost::dynamic_pointer_cast<Swift::BooleanFormField>(f)->getValue());
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (SettingsAdHocCommandTest);
