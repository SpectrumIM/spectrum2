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

class YahooLocalAccount;

class YahooHandler {
	public:
		YahooHandler(YahooLocalAccount *account, int conn_tag, int handler_tag, void *data, yahoo_input_condition cond);
		virtual ~YahooHandler();

		void ready(std::string *buffer = NULL);

		int handler_tag;
		int conn_tag;
		void *data;
		yahoo_input_condition cond;
		bool remove_later;
		YahooLocalAccount *account;
};

