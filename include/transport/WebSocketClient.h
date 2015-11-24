/**
 * Spectrum 2 Slack Frontend
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#pragma once

#include <Swiften/Network/TLSConnectionFactory.h>
#include <Swiften/Network/HostAddressPort.h>
#include <Swiften/TLS/PlatformTLSFactories.h>
#include <Swiften/Network/DomainNameResolveError.h>
#include <Swiften/Network/DomainNameAddressQuery.h>
#include <Swiften/Network/DomainNameResolver.h>
#include <Swiften/Network/HostAddress.h>
#include <Swiften/Network/Connection.h>
#include <Swiften/Base/SafeByteArray.h>
#include "Swiften/Version.h"

#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#if HAVE_SWIFTEN_3
#include <Swiften/TLS/TLSOptions.h>
#endif

#include <string>
#include <algorithm>
#include <map>

#include <boost/signal.hpp>

namespace Transport {

class Component;

class WebSocketClient {
	public:
		WebSocketClient(Component *component);

		virtual ~WebSocketClient();

		void connectServer(const std::string &u);

		void write(const std::string &data);

		boost::signal<void (const std::string &payload)> onPayloadReceived;

	private:
		void handleDNSResult(const std::vector<Swift::HostAddress>&, boost::optional<Swift::DomainNameResolveError>);
		void handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data);
		void handleConnected(bool error);

	private:
		Component *m_component;
		boost::shared_ptr<Swift::DomainNameAddressQuery> m_dnsQuery;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::TLSConnectionFactory *m_tlsConnectionFactory;
		Swift::PlatformTLSFactories *m_tlsFactory;
		std::string m_host;
		std::string m_path;
		std::string m_buffer;
		bool m_upgraded;
};

}
