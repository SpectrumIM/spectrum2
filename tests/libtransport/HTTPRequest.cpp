#include <boost/thread/thread.hpp>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <json/json.h>
#include "basictest.h"
#include "transport/HTTPRequest.h"

using namespace Transport;

class HTTPRequestTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(HTTPRequestTest);
	CPPUNIT_TEST(GET);
	CPPUNIT_TEST_SUITE_END();

	public:

		void setUp (void) {
			setMeUp();
		}

		void tearDown (void) {
			tearMeDown();
		}

	void GET() {
		HTTPRequest req;
		req.init();
		std::string responseData;
		req.GET("http://spectrum.im/params.json", responseData);
		Json::Value resp(responseData);
		CPPUNIT_ASSERT(resp);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (HTTPRequestTest);

