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
#include "Swiften/Network/Timer.h"
#include "Swiften/SwiftenCompat.h"

#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#if HAVE_SWIFTEN_3
#include <Swiften/TLS/TLSOptions.h>
#endif

#include <string>
#include <algorithm>
#include <map>

namespace Transport {

class Component;

class WebSocketClient {
	public:
		WebSocketClient(Component *component, const std::string &user);

		virtual ~WebSocketClient();

		void connectServer(const std::string &u);
		void disconnectServer();

		void write(const std::string &data);

		SWIFTEN_SIGNAL_NAMESPACE::signal<void (const std::string &payload)> onPayloadReceived;

		SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onWebSocketConnected;
		SWIFTEN_SIGNAL_NAMESPACE::signal<void (const boost::optional<Swift::Connection::Error> &error)> onWebSocketDisconnected;

	private:
		void handleDNSResult(const std::vector<Swift::HostAddress>&, boost::optional<Swift::DomainNameResolveError>);
		void handleDataRead(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::SafeByteArray> data);
		void handleConnected(bool error);
		void handleDisconnected(const boost::optional<Swift::Connection::Error> &error);

		void connectServer();

	private:
		Component *m_component;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DomainNameAddressQuery> m_dnsQuery;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn;
		Swift::TLSConnectionFactory *m_tlsConnectionFactory;
		Swift::PlatformTLSFactories *m_tlsFactory;
		std::string m_host;
		std::string m_path;
		std::string m_buffer;
		bool m_upgraded;
		Swift::Timer::ref m_reconnectTimer;
		std::string m_user;
};

}
