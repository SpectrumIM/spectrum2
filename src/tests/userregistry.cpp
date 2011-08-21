#include "transport/userregistry.h"
#include "transport/config.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace Transport;

class UserRegistryTest : public CPPUNIT_NS :: TestFixture {
	CPPUNIT_TEST_SUITE (UserRegistryTest);
// 	CPPUNIT_TEST (storeBuddies);
// 	CPPUNIT_TEST (storeBuddiesRemove);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void) {
			std::istringstream ifs;
			cfg = new Config();
			cfg->load(ifs);
			userRegistry = new UserRegistry(cfg);
		}

		void tearDown (void) {
			delete userRegistry;
			delete cfg;
		}

	private:
		UserRegistry *userRegistry;
		Config *cfg;

};

CPPUNIT_TEST_SUITE_REGISTRATION (UserRegistryTest);
