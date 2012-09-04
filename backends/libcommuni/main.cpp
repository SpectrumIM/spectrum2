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
#include "singleircnetworkplugin.h"

using namespace boost::program_options;
using namespace Transport;

NetworkPlugin * np = NULL;

int main (int argc, char* argv[]) {
	std::string host;
	int port;


	std::string configFile;
	boost::program_options::variables_map vm;
	boost::program_options::options_description desc("Usage: spectrum <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help", "help")
		("host,h", boost::program_options::value<std::string>(&host)->default_value(""), "Host to connect to")
		("port,p", boost::program_options::value<int>(&port)->default_value(10000), "Port to connect to")
		("config", boost::program_options::value<std::string>(&configFile)->default_value(""), "Config file")
		;

	try
	{
		boost::program_options::positional_options_description p;
		p.add("config", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
			options(desc).positional(p).allow_unregistered().run(), vm);
		boost::program_options::notify(vm);
			
		if(vm.count("help"))
		{
			std::cout << desc << "\n";
			return 1;
		}

		if(vm.count("config") == 0) {
			std::cout << desc << "\n";
			return 1;
		}
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		return 1;
	}
	catch (...)
	{
		std::cout << desc << "\n";
		return 1;
	}

	Config config(argc, argv);
	if (!config.load(configFile)) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}
	QCoreApplication app(argc, argv);

	Logging::initBackendLogging(&config);

	Swift::QtEventLoop eventLoop;

	if (!CONFIG_HAS_KEY(&config, "service.irc_server")) {
		np = new IRCNetworkPlugin(&config, &eventLoop, host, port);
	}
	else {
		np = new SingleIRCNetworkPlugin(&config, &eventLoop, host, port);
	}

	return app.exec();
}
