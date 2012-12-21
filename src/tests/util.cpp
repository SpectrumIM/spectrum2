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
	CPPUNIT_TEST(serializeGroups);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void encryptDecryptPassword() {
		std::string encrypted = StorageBackend::encryptPassword("password", "key");
		CPPUNIT_ASSERT_EQUAL(std::string("password"), StorageBackend::decryptPassword(encrypted, "key"));
	}

	void serializeGroups() {
		std::vector<std::string> groups;
		std::string g = "";
		
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(0, (int) StorageBackend::deserializeGroups(g).size());

		groups.push_back("Buddies");
		g = "Buddies";
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(1, (int) StorageBackend::deserializeGroups(g).size());
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::deserializeGroups(g)[0]);

		groups.push_back("Buddies2");
		g = "Buddies\nBuddies2";
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(2, (int) StorageBackend::deserializeGroups(g).size());
		CPPUNIT_ASSERT_EQUAL(std::string("Buddies"), StorageBackend::deserializeGroups(g)[0]);
		CPPUNIT_ASSERT_EQUAL(std::string("Buddies2"), StorageBackend::deserializeGroups(g)[1]);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UtilTest);
