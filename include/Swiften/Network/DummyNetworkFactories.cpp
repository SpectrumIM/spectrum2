/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyTimerFactory.h>
#include <Swiften/Network/DummyConnectionFactory.h>
#include <Swiften/Network/PlatformDomainNameResolver.h>
#include <Swiften/Network/DummyConnectionServerFactory.h>

namespace Swift {

DummyNetworkFactories::DummyNetworkFactories(EventLoop* eventLoop) {
	timerFactory = new DummyTimerFactory();
	connectionFactory = new DummyConnectionFactory(eventLoop);
	domainNameResolver = new PlatformDomainNameResolver(eventLoop);
	connectionServerFactory = new DummyConnectionServerFactory(eventLoop);
}

DummyNetworkFactories::~DummyNetworkFactories() {
	delete connectionServerFactory;
	delete domainNameResolver;
	delete connectionFactory;
	delete timerFactory;
}

}
