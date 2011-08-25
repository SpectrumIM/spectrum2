/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Network/DummyConnectionFactory.h>
#include <Swiften/Network/DummyConnection.h>

namespace Swift {

DummyConnectionFactory::DummyConnectionFactory(EventLoop* eventLoop) : eventLoop(eventLoop) {
}

boost::shared_ptr<Connection> DummyConnectionFactory::createConnection() {
	return boost::shared_ptr<Connection>(new DummyConnection(eventLoop));
}

}
