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
			
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserRegistrationTest);
