#include "basictest.h"
#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/userregistration.h"
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

#include "Swiften/Serializer/GenericPayloadSerializer.h"

#include "../storageparser.h"
#include "Swiften/Parser/PayloadParsers/AttentionParser.h"
#include "Swiften/Serializer/PayloadSerializers/AttentionSerializer.h"
#include "Swiften/Parser/PayloadParsers/XHTMLIMParser.h"
#include "Swiften/Serializer/PayloadSerializers/XHTMLIMSerializer.h"
#include "Swiften/Parser/PayloadParsers/StatsParser.h"
#include "Swiften/Serializer/PayloadSerializers/StatsSerializer.h"
#include "Swiften/Parser/PayloadParsers/GatewayPayloadParser.h"
#include "Swiften/Serializer/PayloadSerializers/GatewayPayloadSerializer.h"
#include "Swiften/Serializer/PayloadSerializers/SpectrumErrorSerializer.h"
#include "Swiften/Parser/PayloadParsers/MUCPayloadParser.h"
#include "transport/BlockParser.h"
#include "transport/BlockSerializer.h"
#include "Swiften/Parser/PayloadParsers/InvisibleParser.h"
#include "Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h"

using namespace Transport;

void BasicTest::setMeUp (void) {
	streamEnded = false;
	std::istringstream ifs("service.server_mode = 1\nservice.jid=localhost\nservice.more_resources=1\n");
	cfg = new Config();
	cfg->load(ifs);

	factory = new TestingFactory();

	storage = new TestingStorageBackend();

	loop = new Swift::DummyEventLoop();
	factories = new Swift::DummyNetworkFactories(loop);

	userRegistry = new UserRegistry(cfg, factories);

	component = new Component(loop, factories, cfg, factory, userRegistry);
	component->start();

	itemsResponder = new DiscoItemsResponder(component);
	itemsResponder->start();

	userManager = new UserManager(component, userRegistry, itemsResponder, storage);

	userRegistration = new UserRegistration(component, userManager, storage);
	userRegistration->start();

	payloadSerializers = new Swift::FullPayloadSerializerCollection();
	payloadParserFactories = new Swift::FullPayloadParserFactoryCollection();

	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<StorageParser>("private", "jabber:iq:private"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::AttentionParser>("attention", "urn:xmpp:attention:0"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::XHTMLIMParser>("html", "http://jabber.org/protocol/xhtml-im"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Transport::BlockParser>("block", "urn:xmpp:block:0"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::InvisibleParser>("invisible", "urn:xmpp:invisible:0"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::StatsParser>("query", "http://jabber.org/protocol/stats"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::GatewayPayloadParser>("query", "jabber:iq:gateway"));
	payloadParserFactories->addFactory(new Swift::GenericPayloadParserFactory<Swift::MUCPayloadParser>("x", "http://jabber.org/protocol/muc"));

	payloadSerializers->addSerializer(new Swift::AttentionSerializer());
	payloadSerializers->addSerializer(new Swift::XHTMLIMSerializer());
	payloadSerializers->addSerializer(new Transport::BlockSerializer());
	payloadSerializers->addSerializer(new Swift::InvisibleSerializer());
	payloadSerializers->addSerializer(new Swift::StatsSerializer());
	payloadSerializers->addSerializer(new Swift::SpectrumErrorSerializer());
	payloadSerializers->addSerializer(new Swift::GatewayPayloadSerializer());

	parser = new Swift::XMPPParser(this, payloadParserFactories, factories->getXMLParserFactory());
	parser2 = new Swift::XMPPParser(this, payloadParserFactories, factories->getXMLParserFactory());

	serverFromClientSession = boost::shared_ptr<Swift::ServerFromClientSession>(new Swift::ServerFromClientSession("id", factories->getConnectionFactory()->createConnection(),
			payloadParserFactories, payloadSerializers, userRegistry, factories->getXMLParserFactory(), Swift::JID("user@localhost/resource")));
	serverFromClientSession->startSession();

	serverFromClientSession->onDataWritten.connect(boost::bind(&BasicTest::handleDataReceived, this, _1));

	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->addSession(serverFromClientSession);
	parser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
	parser2->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
	received.clear();
	received2.clear();
	receivedData.clear();
	loop->processEvents();
}

void BasicTest::tearMeDown (void) {
	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->removeSession(serverFromClientSession);
	if (serverFromClientSession2) {
		dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->removeSession(serverFromClientSession2);
		serverFromClientSession2.reset();
	}
	delete component;
	delete userRegistry;
	delete factories;
	delete factory;
	delete loop;
	delete cfg;
	delete parser;
	delete parser2;
	delete storage;
	delete userRegistration;
	delete itemsResponder;
	received.clear();
	received2.clear();
	receivedData.clear();
	receivedData2.clear();
}

void BasicTest::handleDataReceived(const Swift::SafeByteArray &data) {
// 	std::cout << safeByteArrayToString(data) << "\n";
	stream1_active = true;
	receivedData += safeByteArrayToString(data) + "\n";
	parser->parse(safeByteArrayToString(data));
}

void BasicTest::handleDataReceived2(const Swift::SafeByteArray &data) {
// 	std::cout << safeByteArrayToString(data) << "\n";
	stream1_active = false;
	receivedData2 += safeByteArrayToString(data) + "\n";
	parser2->parse(safeByteArrayToString(data));
}

void BasicTest::handleStreamStart(const Swift::ProtocolHeader&) {

}

void BasicTest::dumpReceived() {
	std::cout << "\nStream1:\n";
	std::cout << receivedData << "\n";
	std::cout << "Stream2:\n";
	std::cout << receivedData2 << "\n";
}

void BasicTest::handleElement(boost::shared_ptr<Swift::Element> element) {
	if (stream1_active) {
		received.push_back(element);
	}
	else {
		received2.push_back(element);
	}
}

void BasicTest::handleStreamEnd() {
	streamEnded = true;
}

void BasicTest::injectPresence(boost::shared_ptr<Swift::Presence> &response) {
	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onPresenceReceived(response);
}

void BasicTest::injectIQ(boost::shared_ptr<Swift::IQ> iq) {
	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onIQReceived(iq);
}

void BasicTest::injectMessage(boost::shared_ptr<Swift::Message> msg) {
	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->onMessageReceived(msg);
}

Swift::Stanza *BasicTest::getStanza(boost::shared_ptr<Swift::Element> element) {
	Swift::Stanza *stanza = dynamic_cast<Swift::Stanza *>(element.get());
	CPPUNIT_ASSERT(stanza);
	return stanza;
}

void BasicTest::connectUser() {
	CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
	userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
	userRegistry->onPasswordValid(Swift::JID("user@localhost/resource"));
	loop->processEvents();
	CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());

	User *user = userManager->getUser("user@localhost");
	CPPUNIT_ASSERT(user);

	UserInfo userInfo = user->getUserInfo();
	CPPUNIT_ASSERT_EQUAL(std::string("password"), userInfo.password);
	CPPUNIT_ASSERT(user->isReadyToConnect() == true);
	CPPUNIT_ASSERT(user->isConnected() == false);

	user->setConnected(true);
	CPPUNIT_ASSERT(user->isConnected() == true);

	CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
	CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());

	received.clear();
	receivedData.clear();
}

void BasicTest::connectSecondResource() {
	serverFromClientSession2 = boost::shared_ptr<Swift::ServerFromClientSession>(new Swift::ServerFromClientSession("id", factories->getConnectionFactory()->createConnection(),
			payloadParserFactories, payloadSerializers, userRegistry, factories->getXMLParserFactory(), Swift::JID("user@localhost/resource2")));
	serverFromClientSession2->startSession();

	serverFromClientSession2->onDataWritten.connect(boost::bind(&BasicTest::handleDataReceived2, this, _1));

	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->addSession(serverFromClientSession2);

	userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource2"), serverFromClientSession2.get(), Swift::createSafeByteArray("password"));
	userRegistry->onPasswordValid(Swift::JID("user@localhost/resource2"));

	loop->processEvents();

	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo("localhost");
	response->setFrom("user@localhost/resource2");
	injectPresence(response);
	loop->processEvents();

	CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());

	User *user = userManager->getUser("user@localhost");
	CPPUNIT_ASSERT(user);
	CPPUNIT_ASSERT_EQUAL(2, user->getResourceCount());

	CPPUNIT_ASSERT(getStanza(received2[1])->getPayload<Swift::DiscoInfo>());
}

void BasicTest::disconnectUser() {
	received.clear();
	userManager->disconnectUser("user@localhost");
	dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
	loop->processEvents();

	CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
	CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
	CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
}

void BasicTest::add2Buddies() {
	User *user = userManager->getUser("user@localhost");
	CPPUNIT_ASSERT(user);

	std::vector<std::string> grp;
	grp.push_back("group1");
	LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1", "Buddy 1", grp, BUDDY_JID_ESCAPING);
	buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status1");
	user->getRosterManager()->setBuddy(buddy);

	std::vector<std::string> grp2;
	grp2.push_back("group2");
	buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy2", "Buddy 2", grp2, BUDDY_JID_ESCAPING);
	buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status2");
	user->getRosterManager()->setBuddy(buddy);
}

