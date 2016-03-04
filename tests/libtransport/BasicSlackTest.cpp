#include "BasicSlackTest.h"
#include "XMPPFrontend.h"
#include "XMPPUserRegistration.h"
#include "XMPPUserManager.h"
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

#include "storageparser.h"
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
#include "BlockParser.h"
#include "BlockSerializer.h"
#include "Swiften/Parser/PayloadParsers/InvisibleParser.h"
#include "Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h"

using namespace Transport;

void BasicSlackTest::setMeUp (void) {
	std::istringstream ifs("service.server_mode = 1\nservice.jid=localhost\nservice.more_resources=1\nservice.admin_jid=me@localhost\n");
	cfg = new Config();
	cfg->load(ifs);

	factory = new TestingFactory();

	storage = new TestingStorageBackend();

	loop = new Swift::DummyEventLoop();
	factories = new Swift::DummyNetworkFactories(loop);

	userRegistry = new UserRegistry(cfg, factories);

	frontend = new SlackFrontend();

	component = new Component(frontend, loop, factories, cfg, factory, userRegistry);
	component->start();

	userManager = frontend->createUserManager(component, userRegistry, storage);
}

void BasicSlackTest::tearMeDown (void) {
	delete component;
	delete frontend;
	delete userRegistry;
	delete factories;
	delete factory;
	delete loop;
	delete cfg;
	delete storage;
}


