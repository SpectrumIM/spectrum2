/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Network/DummyConnectionServer.h>

#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/placeholders.hpp>

#include <Swiften/EventLoop/EventLoop.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {

DummyConnectionServer::DummyConnectionServer(EventLoop* eventLoop) : eventLoop(eventLoop) {
}

void DummyConnectionServer::start() {
}


void DummyConnectionServer::stop() {
	
}

void DummyConnectionServer::acceptConnection(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> connection) {
		eventLoop->postEvent(
				boost::bind(boost::ref(onNewConnection), connection), 
				SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DummyConnectionServer>(this));
// 		connection->listen();
}


HostAddressPort DummyConnectionServer::getAddressPort() const {
	return HostAddressPort();
}

}
