#pragma once

// Transport includes
#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

// Yahoo2
#include <yahoo2.h>
#include <yahoo2_callbacks.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// Swiften
#include "Swiften/Swiften.h"
#include "Swiften/TLS/OpenSSL/OpenSSLContextFactory.h"

// for signal handler
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"

// Boost
#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

class YahooHandler;

class YahooLocalAccount {
	public:
		YahooLocalAccount(const std::string &user, const std::string &legacyName, const std::string &password);
		virtual ~YahooLocalAccount();

		void login();

		void addHandler(YahooHandler *handler);
		void removeOldHandlers();
		void removeConn(int conn_tag);

		std::string user;
		int id;
		std::map<int, boost::shared_ptr<Swift::Connection> > conns;
		int conn_tag;
		std::map<int, YahooHandler *> handlers;
		std::map<int, std::map<int, YahooHandler *> > handlers_per_conn;
		std::map<std::string, std::string> urls;
		int handler_tag;
		int status;
		std::string msg;
		std::string buffer;
};
