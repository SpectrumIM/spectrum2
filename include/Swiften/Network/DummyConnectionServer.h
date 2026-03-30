/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>

#include <Swiften/Network/DummyConnection.h>
#include <Swiften/Network/ConnectionServer.h>
#include <Swiften/EventLoop/EventOwner.h>
#include <Swiften/Version.h>

namespace Swift {
	class DummyConnectionServer : public ConnectionServer {
		public:
			enum Error {
				Conflict,
				UnknownError
			};

			static std::shared_ptr<Swift::DummyConnectionServer> create(EventLoop* eventLoop) {
				return std::shared_ptr<Swift::DummyConnectionServer>(new DummyConnectionServer(eventLoop));
			}

			void acceptConnection(std::shared_ptr<Swift::Connection> connection);

			virtual boost::optional<ConnectionServer::Error> tryStart() {
				return boost::optional<ConnectionServer::Error>();
			}

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
