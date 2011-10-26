#include "basictest.h"
#include "transport/userregistry.h"
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

using namespace Transport;

void BasicTest::setMeUp (void) {
	streamEnded = false;
	std::istringstream ifs("service.server_mode = 1\nservice.jid=localhost");
	cfg = new Config();
	cfg->load(ifs);

	factory = new TestingFactory();

	loop = new Swift::DummyEventLoop();
	factories = new Swift::DummyNetworkFactories(loop);

	userRegistry = new UserRegistry(cfg, factories);

	component = new Component(loop, factories, cfg, factory, userRegistry);
	component->start();

	userManager = new UserManager(component, userRegistry);

	payloadSerializers = new Swift::FullPayloadSerializerCollection();
	payloadParserFactories = new Swift::FullPayloadParserFactoryCollection();
	parser = new Swift::XMPPParser(this, payloadParserFactories, factories->getXMLParserFactory());

	serverFromClientSession = boost::shared_ptr<Swift::ServerFromClientSession>(new Swift::ServerFromClientSession("id", factories->getConnectionFactory()->createConnection(),
			payloadParserFactories, payloadSerializers, userRegistry, factories->getXMLParserFactory(), Swift::JID("user@localhost/resource")));
	serverFromClientSession->startSession();

	serverFromClientSession->onDataWritten.connect(boost::bind(&BasicTest::handleDataReceived, this, _1));

	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->addSession(serverFromClientSession);
	parser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
	received.clear();
	loop->processEvents();
}

void BasicTest::tearMeDown (void) {
	dynamic_cast<Swift::ServerStanzaChannel *>(component->getStanzaChannel())->removeSession(serverFromClientSession);
	delete component;
	delete userRegistry;
	delete factories;
	delete factory;
	delete loop;
	delete cfg;
	delete parser;
	received.clear();
}

void BasicTest::handleDataReceived(const Swift::SafeByteArray &data) {
// 	std::cout << safeByteArrayToString(data) << "\n";
	parser->parse(safeByteArrayToString(data));
}

void BasicTest::handleStreamStart(const Swift::ProtocolHeader&) {

}

void BasicTest::handleElement(boost::shared_ptr<Swift::Element> element) {
received.push_back(element);
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

Swift::Stanza *BasicTest::getStanza(boost::shared_ptr<Swift::Element> element) {
	Swift::Stanza *stanza = dynamic_cast<Swift::Stanza *>(element.get());
	CPPUNIT_ASSERT(stanza);
	return stanza;
}

