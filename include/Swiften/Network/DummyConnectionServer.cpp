/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Network/DummyConnectionServer.h>

#include <boost/bind.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/placeholders.hpp>

#include <Swiften/EventLoop/EventLoop.h>

namespace Swift {

DummyConnectionServer::DummyConnectionServer(EventLoop* eventLoop) : eventLoop(eventLoop) {
}

void DummyConnectionServer::start() {
}


void DummyConnectionServer::stop() {
	
}

void DummyConnectionServer::acceptConnection(std::shared_ptr<Swift::Connection> connection) {
		eventLoop->postEvent(
				boost::bind(boost::ref(onNewConnection), connection));
// 		connection->listen();
}


HostAddressPort DummyConnectionServer::getAddressPort() const {
	return HostAddressPort();
}

}
