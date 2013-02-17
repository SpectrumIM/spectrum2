/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "transport/logging.h"
#include "transport/config.h"
#include "transport/util.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>


#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#ifndef WIN32
#include "sys/signal.h"
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/resource.h>
#include "libgen.h"
#else
#include <windows.h>
#include <process.h>
#define getpid _getpid
#endif

using namespace boost::filesystem;


namespace Transport {

namespace Logging {

#ifdef WITH_LOG4CXX
using namespace log4cxx;
static LoggerPtr root;


class intercept_stream : public std::streambuf{
public:
    intercept_stream(std::ostream& stream, char const* logger):
      _logger(log4cxx::Logger::getLogger(logger))
    {
		stream.rdbuf(this);
    }
    ~intercept_stream(){
    }
protected:
    virtual std::streamsize xsputn(const char *msg, std::streamsize count){

        //Output to log4cxx logger
        std::string s(msg, count - 1); // remove last \n
		LOG4CXX_INFO(_logger, s);
        return count;
    }

	int overflow(int c)
	{
		return 0;
	}

	int sync()
	{
		return 0;
	}

private:
    log4cxx::LoggerPtr _logger;
};

static intercept_stream* intercepter_cout;
static intercept_stream* intercepter_cerr;


static void initLogging(Config *config, std::string key, bool only_create_dir = false) {
	if (CONFIG_STRING(config, key).empty()) {
		if (only_create_dir) {
			return;
		}
		root = log4cxx::Logger::getRootLogger();
#ifdef _MSC_VER
		root->addAppender(new ConsoleAppender(new PatternLayout(L"%d %-5p %c: %m%n")));
#else
		root->addAppender(new ConsoleAppender(new PatternLayout("%d %-5p %c: %m%n")));
#endif
	}
	else {
		log4cxx::helpers::Properties p;

		log4cxx::helpers::FileInputStream *istream = NULL;
		try {
			istream = new log4cxx::helpers::FileInputStream(CONFIG_STRING(config, key));
		}
		catch(log4cxx::helpers::IOException &ex) {
			std::cerr << "Can't create FileInputStream logger instance: " << ex.what() << "\n";
		}
		catch (...) {
			std::cerr << "Can't create FileInputStream logger instance\n";
		}

		if (!istream) {
			return;
		}

		p.load(istream);
		LogString pid, jid, id;
		log4cxx::helpers::Transcoder::decode(boost::lexical_cast<std::string>(getpid()), pid);
		log4cxx::helpers::Transcoder::decode(CONFIG_STRING(config, "service.jid"), jid);
		log4cxx::helpers::Transcoder::decode(CONFIG_STRING_DEFAULTED(config, "service.backend_id", ""), id);
#ifdef _MSC_VER
		p.setProperty(L"pid", pid);
		p.setProperty(L"jid", jid);
		p.setProperty(L"id", id);
#else
		p.setProperty("pid", pid);
		p.setProperty("jid", jid);
		p.setProperty("id", id);
#endif

		std::vector<std::string> dirs;
		BOOST_FOREACH(const log4cxx::LogString &prop, p.propertyNames()) {
			if (boost::ends_with(prop, ".File")) {
				std::string dir;
				log4cxx::helpers::Transcoder::encode(p.get(prop), dir);
				boost::replace_all(dir, "${jid}", jid);
				boost::replace_all(dir, "${pid}", pid);
				boost::replace_all(dir, "${id}", id);
				dirs.push_back(dir);
			}
		}
#ifndef WIN32
		mode_t old_cmask;
		// create directories
		old_cmask = umask(0007);
#endif

		BOOST_FOREACH(std::string &dir, dirs) {
			if (!dir.empty()) {
				try {
					Transport::Util::createDirectories(config, boost::filesystem::path(dir).parent_path());
				}
				catch (const boost::filesystem::filesystem_error &e) {
					std::cerr << "Can't create logging directory directory " << boost::filesystem::path(dir).parent_path().string() << ": " << e.what() << ".\n";
				}
			}
		}

#ifndef WIN32
		umask(old_cmask);
#endif

		if (only_create_dir) {
			return;
		}

		log4cxx::PropertyConfigurator::configure(p);

		// Change owner of main log file
#ifndef WIN32
	BOOST_FOREACH(std::string &dir, dirs) {
		if (!CONFIG_STRING(config, "service.group").empty() && !CONFIG_STRING(config, "service.user").empty()) {
			struct group *gr;
			if ((gr = getgrnam(CONFIG_STRING(config, "service.group").c_str())) == NULL) {
				std::cerr << "Invalid service.group name " << CONFIG_STRING(config, "service.group") << "\n";
			}
			struct passwd *pw;
			if ((pw = getpwnam(CONFIG_STRING(config, "service.user").c_str())) == NULL) {
				std::cerr << "Invalid service.user name " << CONFIG_STRING(config, "service.user") << "\n";
			}
			chown(dir.c_str(), pw->pw_uid, gr->gr_gid);
		}
	}
#endif
	}
}

void initBackendLogging(Config *config) {
	initLogging(config, "logging.backend_config");

	redirect_stderr();
}

void initMainLogging(Config *config) {
	initLogging(config, "logging.config");
	initLogging(config, "logging.backend_config", true);
}

void redirect_stderr() {
	 intercepter_cerr = new intercept_stream(std::cerr, "cerr");
	 intercepter_cout = new intercept_stream(std::cout, "cout");
}

void shutdownLogging() {
	delete intercepter_cerr;
	delete intercepter_cout;
	log4cxx::LogManager::shutdown();
}

#else /* WITH_LOG4CXX */
void initBackendLogging(Config */*config*/) {
}

void initMainLogging(Config */*config*/) {
}

void shutdownLogging() {
	
}

void redirect_stderr() {
	
}
#endif /* WITH_LOG4CXX */

}

}
