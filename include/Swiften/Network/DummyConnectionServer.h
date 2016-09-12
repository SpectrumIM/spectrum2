/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <Swiften/Network/DummyConnection.h>
#include <Swiften/Network/ConnectionServer.h>
#include <Swiften/EventLoop/EventOwner.h>
#include <Swiften/Version.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class DummyConnectionServer : public ConnectionServer, public EventOwner, public boost::enable_shared_from_this<DummyConnectionServer> {
		public:
			typedef SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<DummyConnectionServer> ref;

			enum Error {
				Conflict,
				UnknownError
			};

			static SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DummyConnectionServer> create(EventLoop* eventLoop) {
				return SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DummyConnectionServer>(new DummyConnectionServer(eventLoop));
			}

			void acceptConnection(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> connection);

#if (SWIFTEN_VERSION >= 0x030000)
			virtual boost::optional<ConnectionServer::Error> tryStart() {
				return boost::optional<ConnectionServer::Error>();
			}
#endif

			virtual void start();
			virtual void stop();

			virtual HostAddressPort getAddressPort() const;


		private:
			DummyConnectionServer(EventLoop* eventLoop);


		private:
			HostAddress address_;
			EventLoop* eventLoop;
	};
}
