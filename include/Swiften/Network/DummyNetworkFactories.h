/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  SWIFTEN_VERSION >= 0x030000

#include <Swiften/Network/NetworkFactories.h>
#include <Swiften/Parser/PlatformXMLParserFactory.h>
#if HAVE_SWIFTEN_3
#include <Swiften/IDN/IDNConverter.h>
#include <Swiften/IDN/PlatformIDNConverter.h>
#endif

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

#if HAVE_SWIFTEN_3
			IDNConverter* getIDNConverter() const {
				return idnConverter.get();
			}
			Swift::CryptoProvider* getCryptoProvider() const {
		        	return cryptoProvider;
			}
			Swift::NetworkEnvironment* getNetworkEnvironment() const {
				return networkEnvironment;
			}

#endif

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

			EventLoop *getEventLoop() const {
				return eventLoop;
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
#if HAVE_SWIFTEN_3
			boost::shared_ptr<IDNConverter> idnConverter;
			CryptoProvider* cryptoProvider;
			NetworkEnvironment* networkEnvironment;
#endif
			DomainNameResolver* domainNameResolver;
			ConnectionServerFactory* connectionServerFactory;
			EventLoop *eventLoop;
	};
}
