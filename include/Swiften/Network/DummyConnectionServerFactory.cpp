/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Network/DummyConnectionServerFactory.h>
#include <Swiften/Network/DummyConnectionServer.h>

namespace Swift {

DummyConnectionServerFactory::DummyConnectionServerFactory(EventLoop* eventLoop) : eventLoop(eventLoop) {
}

boost::shared_ptr<ConnectionServer> DummyConnectionServerFactory::createConnectionServer(int port) {
	return DummyConnectionServer::create(eventLoop);
}

boost::shared_ptr<ConnectionServer> DummyConnectionServerFactory::createConnectionServer(const Swift::HostAddress &hostAddress, int port) {
	return DummyConnectionServer::create(eventLoop);
}

}
