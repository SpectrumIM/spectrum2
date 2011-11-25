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
#include "basictest.h"

#include "transport/util.h"

using namespace Transport;

class UtilTest : public CPPUNIT_NS :: TestFixture{
	CPPUNIT_TEST_SUITE(UtilTest);
	CPPUNIT_TEST(encryptDecryptPassword);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void encryptDecryptPassword() {
		std::string encrypted = Util::encryptPassword("password", "key");
		CPPUNIT_ASSERT_EQUAL(std::string("password"), Util::decryptPassword(encrypted, "key"));
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UtilTest);
