#pragma once

// Transport includes
#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

// Swiften
#include "Swiften/Swiften.h"
#include "Swiften/TLS/OpenSSL/OpenSSLContextFactory.h"

// Boost
#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

class HTTPFetch {
	public:
		HTTPFetch(Swift::BoostIOServiceThread *ioSerice, Swift::ConnectionFactory *factory);
		virtual ~HTTPFetch();

		bool fetchURL(const std::string &url);

		boost::signal<void (const std::string &data)> onURLFetched;

	private:
		void _connected(boost::shared_ptr<Swift::Connection> conn, const std::string url, bool error);
		void _disconnected(boost::shared_ptr<Swift::Connection> conn);
		void _read(boost::shared_ptr<Swift::Connection> conn, boost::shared_ptr<Swift::SafeByteArray> data);

		Swift::BoostIOServiceThread *m_ioService;
		Swift::ConnectionFactory *m_factory;
		std::string m_buffer;
		bool m_afterHeader;
};
