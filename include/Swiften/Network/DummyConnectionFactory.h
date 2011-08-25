/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/asio/io_service.hpp>

#include <Swiften/Network/ConnectionFactory.h>
#include <Swiften/Network/DummyConnection.h>

namespace Swift {
	class DummyConnection;

	class DummyConnectionFactory : public ConnectionFactory {
		public:
			DummyConnectionFactory(EventLoop* eventLoop);

			virtual boost::shared_ptr<Connection> createConnection();

		private:
			EventLoop* eventLoop;
	};
}
