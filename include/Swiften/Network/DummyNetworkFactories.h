/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <Swiften/Network/NetworkFactories.h>
#include <Swiften/Parser/PlatformXMLParserFactory.h>

namespace Swift {
	class EventLoop;

	class DummyNetworkFactories : public NetworkFactories {
		public:
			DummyNetworkFactories(EventLoop *eventLoop);
			~DummyNetworkFactories();

			virtual TimerFactory* getTimerFactory() const {
				return timerFactory;
			}

			virtual ConnectionFactory* getConnectionFactory() const {
				return connectionFactory;
			}

			DomainNameResolver* getDomainNameResolver() const {
				return domainNameResolver;
			}

			ConnectionServerFactory* getConnectionServerFactory() const {
				return connectionServerFactory;
			}

			virtual Swift::NATTraverser* getNATTraverser() const {
				return 0;
			}

			Swift::XMLParserFactory* getXMLParserFactory() const {
				return m_platformXMLParserFactory;
			}

            Swift::TLSContextFactory* getTLSContextFactory() const {
                return 0;
            }

            Swift::ProxyProvider* getProxyProvider() const {
                return 0;
            }

		private:
			PlatformXMLParserFactory *m_platformXMLParserFactory;
			TimerFactory* timerFactory;
			ConnectionFactory* connectionFactory;
			DomainNameResolver* domainNameResolver;
			ConnectionServerFactory* connectionServerFactory;
	};
}
