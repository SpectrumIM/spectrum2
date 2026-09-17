/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <boost/asio/io_service.hpp>

#include <Swiften/Network/ConnectionServerFactory.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include <Swiften/Version.h>

#define HAVE_SWIFTEN_5  (SWIFTEN_VERSION >= 0x050000)

namespace Swift {
	class ConnectionServer;

	class DummyConnectionServerFactory : public ConnectionServerFactory {
		public:
			DummyConnectionServerFactory(EventLoop* eventLoop);

#if HAVE_SWIFTEN_5
			virtual std::shared_ptr<ConnectionServer> createConnectionServer(unsigned short port);

			virtual std::shared_ptr<ConnectionServer> createConnectionServer(const Swift::HostAddress &hostAddress, unsigned short port);
#else
			virtual std::shared_ptr<ConnectionServer> createConnectionServer(int port);

			virtual std::shared_ptr<ConnectionServer> createConnectionServer(const Swift::HostAddress &hostAddress, int port);
#endif

		private:
			EventLoop* eventLoop;
	};
}
