#include "main.h"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#ifdef WITH_LOG4CXX
#include "log4cxx/logger.h"
#include "log4cxx/consoleappender.h"
#include "log4cxx/patternlayout.h"
using namespace log4cxx;
#endif

#include "transport/protocol.pb.h"
#include "transport/HTTPRequest.h"



int main (int argc, char* argv[])
{
#ifdef WITH_LOG4CXX
	LoggerPtr root = Logger::getRootLogger();
#ifndef _MSC_VER
	#ifdef LOG4CXX_VERSION
		auto patternlayout = std::make_shared<PatternLayout>("%d %-5p %c: %m%n");
		auto fileappender = std::make_shared<FileAppender>(patternlayout, "libtransport_test.log", false);
		root->addAppender(fileappender);
	#else
		root->addAppender(new FileAppender(new PatternLayout("%d %-5p %c: %m%n"), "libtransport_test.log", false));
	#endif
#else
	root->addAppender(new FileAppender(new PatternLayout(L"%d %-5p %c: %m%n"), L"libtransport_test.log", false));
#endif
    root->setLevel(log4cxx::Level::getWarn());
#endif

	std::vector<std::string> testsToRun;
	for (int i = 1; i < argc; ++i) {
		std::string param(argv[i]);
		testsToRun.push_back(param);
	}

	if (testsToRun.empty()) {
		testsToRun.push_back("");
	}

	// informs test-listener about testresults
	CPPUNIT_NS :: TestResult testresult;

	// register listener for collecting the test-results
	CPPUNIT_NS :: TestResultCollector collectedresults;
	testresult.addListener (&collectedresults);

	// register listener for per-test progress output
	CPPUNIT_NS :: BriefTestProgressListener progress;
	testresult.addListener (&progress);

	// insert test-suite at test-runner by registry
	CPPUNIT_NS :: TestRunner testrunner;
	testrunner.addTest (CPPUNIT_NS :: TestFactoryRegistry :: getRegistry ().makeTest ());
	for (std::vector<std::string>::const_iterator i = testsToRun.begin(); i != testsToRun.end(); ++i) {
		try {
			testrunner.run(testresult, *i);
		}
		catch (const std::exception& e) {
			google::protobuf::ShutdownProtobufLibrary();
			std::cerr << "Error: " << e.what() << std::endl;
			return -1;
		}
	}

	// output results in compiler-format
	CPPUNIT_NS :: CompilerOutputter compileroutputter (&collectedresults, std::cerr);
	compileroutputter.write ();

	google::protobuf::ShutdownProtobufLibrary();

	// return 0 if tests were successful
	return collectedresults.wasSuccessful () ? 0 : 1;
}
