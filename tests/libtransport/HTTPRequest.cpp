#include <boost/thread/thread.hpp>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include <Swiften/Elements/VCardUpdate.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "basictest.h"

#include "transport/ThreadPool.h"
#include "transport/HTTPRequest.h"

using namespace Transport;

class HTTPRequestTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(HTTPRequestTest);
	CPPUNIT_TEST(GETThreadPool);
	CPPUNIT_TEST_SUITE_END();

	public:
		ThreadPool *tp;
		bool result;

		void setUp (void) {
			setMeUp();
			tp = new ThreadPool(loop, 10);
			result = false;
		}

		void tearDown (void) {
			tearMeDown();
			delete tp;
		}

	void handleResult(HTTPRequest *req, bool ok, Json::Value &resp, const std::string &data) {
		result = true;
	}

	void GET() {
		Json::Value resp;
		HTTPRequest *req = new HTTPRequest(tp, HTTPRequest::Get, "http://spectrum.im/params.json", boost::bind(&HTTPRequestTest::handleResult, this, _1, _2, _3, _4));
		req->execute(resp);
		delete req;
	}

	void GETThreadPool() {
		HTTPRequest *req = new HTTPRequest(tp, HTTPRequest::Get, "http://spectrum.im/params.json", boost::bind(&HTTPRequestTest::handleResult, this, _1, _2, _3, _4));
		req->execute();

		int i = 0;
		while (result == false && i < 5) {
			boost::system_time time = boost::get_system_time();
			time += boost::posix_time::seconds(1);
			boost::thread::sleep(time);
			loop->processEvents();
			i++;
		}
		CPPUNIT_ASSERT(result);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (HTTPRequestTest);

