#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Parser/StringTreeParser.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "basictest.h"

#include "transport/util.h"

using namespace Transport;

class StringTreeParserTest : public CPPUNIT_NS :: TestFixture{
	CPPUNIT_TEST_SUITE(StringTreeParserTest);
	CPPUNIT_TEST(parseEscapedCharacters);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void parseEscapedCharacters() {
		Swift::ParserElement::ref root = Swift::StringTreeParser::parse("<body>&lt;test&gt;</body>");
		CPPUNIT_ASSERT_EQUAL(std::string("<test>"), root->getText());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (StringTreeParserTest);
