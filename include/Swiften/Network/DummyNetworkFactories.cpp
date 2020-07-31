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
#include <Swiften/Crypto/CryptoProvider.h>
#include <Swiften/Crypto/PlatformCryptoProvider.h>
#include <Swiften/Network/NetworkEnvironment.h>
#include <Swiften/Network/PlatformNetworkEnvironment.h>

namespace Swift {

DummyNetworkFactories::DummyNetworkFactories(EventLoop* eventLoop) {
	timerFactory = new DummyTimerFactory();
	connectionFactory = new DummyConnectionFactory(eventLoop);
	idnConverter = std::shared_ptr<IDNConverter>(PlatformIDNConverter::create());
	domainNameResolver = new PlatformDomainNameResolver(idnConverter.get(), eventLoop);
	cryptoProvider = PlatformCryptoProvider::create();
	networkEnvironment = new PlatformNetworkEnvironment();
	connectionServerFactory = new DummyConnectionServerFactory(eventLoop);
	m_platformXMLParserFactory =  new PlatformXMLParserFactory();
	this->eventLoop = eventLoop;
}

DummyNetworkFactories::~DummyNetworkFactories() {
	delete connectionServerFactory;
	delete domainNameResolver;
	delete connectionFactory;
	delete timerFactory;
	delete m_platformXMLParserFactory;
	delete cryptoProvider;
	delete networkEnvironment;
}

}
