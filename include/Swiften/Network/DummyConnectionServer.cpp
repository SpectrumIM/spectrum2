/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Network/DummyConnectionServer.h>

namespace Swift {

DummyConnectionServer::DummyConnectionServer(EventLoop* eventLoop) : eventLoop(eventLoop) {
}

void DummyConnectionServer::start() {
}


void DummyConnectionServer::stop() {
	
}

HostAddressPort DummyConnectionServer::getAddressPort() const {
	return HostAddressPort();
}

}
