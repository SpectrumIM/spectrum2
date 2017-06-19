/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2013, Jan Kaluza <hanzz.k@gmail.com>
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
#include <QtCore>
#include <QtNetwork>
#include "transport/Config.h"
#include "transport/NetworkPlugin.h"
#include "transport/Logging.h"
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
