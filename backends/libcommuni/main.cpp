/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include "session.h"
#include <QtCore>
#include <QtNetwork>
#include "Swiften/EventLoop/Qt/QtEventLoop.h"
#include "ircnetworkplugin.h"

using namespace boost::program_options;
using namespace Transport;

NetworkPlugin * np = NULL;

int main (int argc, char* argv[]) {
	std::string host;
	int port;

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	QCoreApplication app(argc, argv);

	Swift::QtEventLoop eventLoop;


	np = new IRCNetworkPlugin(cfg, &eventLoop, host, port);
	return app.exec();
}
