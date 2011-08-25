/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <Swiften/Network/NetworkFactories.h>

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

		private:
			TimerFactory* timerFactory;
			ConnectionFactory* connectionFactory;
			DomainNameResolver* domainNameResolver;
			ConnectionServerFactory* connectionServerFactory;
	};
}
