#include "main.h"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#ifdef WITH_LOG4CXX
#include "log4cxx/logger.h"
#include "log4cxx/fileappender.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"

using namespace log4cxx;
#endif


int main (int argc, char* argv[])
{
#ifdef WITH_LOG4CXX
	LoggerPtr root = Logger::getRootLogger();
#ifndef _MSC_VER
	root->addAppender(new FileAppender(new PatternLayout("%d %-5p %c: %m%n"), "libtransport_test.log", false));
#else
	root->addAppender(new FileAppender(new PatternLayout(L"%d %-5p %c: %m%n"), L"libtransport_test.log", false));
#endif
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
			std::cerr << "Error: " << e.what() << std::endl;
			return -1;
		}
	}

	// output results in compiler-format
	CPPUNIT_NS :: CompilerOutputter compileroutputter (&collectedresults, std::cerr);
	compileroutputter.write ();

	// return 0 if tests were successful
	return collectedresults.wasSuccessful () ? 0 : 1;
}
