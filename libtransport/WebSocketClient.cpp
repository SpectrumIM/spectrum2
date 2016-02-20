/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "transport/WebSocketClient.h"
#include "transport/Transport.h"
#include "transport/Util.h"
#include "transport/Logging.h"

#include "Swiften/StringCodecs/Hexify.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "WebSocketClient");

WebSocketClient::WebSocketClient(Component *component, const std::string &user) {
	m_component = component;
	m_upgraded = false;
	m_user = user;

#if HAVE_SWIFTEN_3
	Swift::TLSOptions o;
#endif
	m_tlsFactory = new Swift::PlatformTLSFactories();
#if HAVE_SWIFTEN_3
	m_tlsConnectionFactory = new Swift::TLSConnectionFactory(m_tlsFactory->getTLSContextFactory(), component->getNetworkFactories()->getConnectionFactory(), o);
#else
	m_tlsConnectionFactory = new Swift::TLSConnectionFactory(m_tlsFactory->getTLSContextFactory(), component->getNetworkFactories()->getConnectionFactory());
#endif

	m_reconnectTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(1000);
	m_reconnectTimer->onTick.connect(boost::bind(&WebSocketClient::connectServer, this));
}

WebSocketClient::~WebSocketClient() {
	if (m_conn) {
		m_conn->onDataRead.disconnect(boost::bind(&WebSocketClient::handleDataRead, this, _1));
		m_conn->disconnect();
	}

	delete m_tlsFactory;
	delete m_tlsConnectionFactory;
}

void WebSocketClient::connectServer() {
	LOG4CXX_INFO(logger, m_user << ": Starting DNS query for " << m_host << " " << m_path);

	m_upgraded = false;
	m_buffer.clear();

	m_dnsQuery = m_component->getNetworkFactories()->getDomainNameResolver()->createAddressQuery(m_host);
	m_dnsQuery->onResult.connect(boost::bind(&WebSocketClient::handleDNSResult, this, _1, _2));
	m_dnsQuery->run();
	m_reconnectTimer->stop();
}

void WebSocketClient::connectServer(const std::string &url) {
	std::string u = url.substr(6);
	m_host = u.substr(0, u.find("/"));
	m_path = u.substr(u.find("/"));
	connectServer();
}

void WebSocketClient::disconnectServer() {
	if (m_conn) {
		m_conn->onDataRead.disconnect(boost::bind(&WebSocketClient::handleDataRead, this, _1));
		m_conn->disconnect();
	}
}

void WebSocketClient::write(const std::string &data) {
	if (!m_conn) {
		return;
	}

	uint8_t opcode = 129; // UTF8
	if (data.empty()) {
		opcode = 138; // PONG
	}

	// Mask the payload
	char mask_bits[4] = {0x11, 0x22, 0x33, 0x44};
	std::string payload = data;
	for (size_t i = 0; i < data.size(); i++ ) {
		payload[i] = payload[i] ^ mask_bits[i&3];
	}

	if (data.size() <= 125) {
		uint8_t size7 = data.size() + 128; // Mask bit
		m_conn->write(Swift::createSafeByteArray(std::string((char *) &opcode, 1)
			+ std::string((char *) &size7, 1)
			+ std::string((char *) &mask_bits[0], 4)
			+ payload));
	}
	else {
		uint8_t size7 = 126 + 128; // Mask bit
		uint16_t size16 = data.size();
		size16 = htons(size16);
		m_conn->write(Swift::createSafeByteArray(std::string((char *) &opcode, 1)
			+ std::string((char *) &size7, 1)
			+ std::string((char *) &size16, 2)
			+ std::string((char *) &mask_bits[0], 4)
			+ payload));
	}

	LOG4CXX_INFO(logger, m_user << ": > " << data);
}

void WebSocketClient::handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
	std::string d = Swift::safeByteArrayToString(*data);
	m_buffer += d;

	if (!m_upgraded) {
		if (m_buffer.find("\r\n\r\n") != std::string::npos) {
			m_buffer.erase(0, m_buffer.find("\r\n\r\n") + 4);
			m_upgraded = true;
			onWebSocketConnected();
		}
		else {
			return;
		}
	}

	while (m_buffer.size() > 0) {
		if (m_buffer.size() >= 2) {
			uint8_t opcode = *((uint8_t *) &m_buffer[0]) & 0xf;
			uint8_t size7 = *((uint8_t *) &m_buffer[1]) & 127;
			bool mask = *((uint8_t *) &m_buffer[1]) & 128;
			uint16_t size16 = 0;
			int header_size = 2;
			if (size7 == 126) {
				if (m_buffer.size() >= 4) {
					size16 = *((uint16_t *) &m_buffer[2]);
					size16 = ntohs(size16);
					header_size += 2;
				}
				else {
					return;
				}
			}

			// This seems to be Slack bug... sometimes we receive 0x89 followed by 0x81
			// For now, in that case we will just ignore the 0x89 and skip it...
			if (opcode == 9 && mask && size7 == 1) {
				LOG4CXX_WARN(logger, m_user << ": Applying Slack workaround because of partial data received from server");
				m_buffer.erase(0, 1);
				continue;
			}

			unsigned int size = (size16 == 0 ? size7 : size16);
			if (m_buffer.size() >= size + header_size) {
				std::string payload = m_buffer.substr(header_size, size);
				LOG4CXX_INFO(logger, m_user << ": < " << payload);
				onPayloadReceived(payload);
				m_buffer.erase(0, size + header_size);
				
			}
			else if (size == 0) {
				m_buffer.erase(0, header_size);
			}
			else {
				return;
			}
		}
		else {
			return;
		}
	}
}

void WebSocketClient::handleConnected(bool error) {
	if (error) {
		LOG4CXX_ERROR(logger, "Connection to " << m_host << " failed. Will reconnect in 1 second.");
		m_reconnectTimer->start();
		return;
	}

	LOG4CXX_INFO(logger, m_user << ": Connected to " << m_host);

	std::string req = "";
	req += "GET " + m_path + " HTTP/1.1\r\n";
	req += "Host: " + m_host + ":443\r\n";
	req += "Upgrade: websocket\r\n";
	req += "Connection: Upgrade\r\n";
	req += "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n";
	req += "Sec-WebSocket-Version: 13\r\n";
	req += "\r\n";

	m_conn->write(Swift::createSafeByteArray(req));
}

void WebSocketClient::handleDisconnected(const boost::optional<Swift::Connection::Error> &error) {
	if (!error) {
		return;
	}

	LOG4CXX_ERROR(logger, m_user << ": Disconected from " << m_host << ". Will reconnect in 1 second.");
	onWebSocketDisconnected(error);
	m_reconnectTimer->start();
}

void WebSocketClient::handleDNSResult(const std::vector<Swift::HostAddress> &addrs, boost::optional<Swift::DomainNameResolveError> error) {
	if (error) {
		LOG4CXX_ERROR(logger, m_user << ": DNS resolving error. Will try again in 1 second.");
		m_reconnectTimer->start();
		return;
	}

	if (addrs.empty()) {
		LOG4CXX_ERROR(logger, m_user << ": DNS name cannot be resolved. Will try again in 1 second.");
		m_reconnectTimer->start();
		return;
	}

	m_conn = m_tlsConnectionFactory->createConnection();
	m_conn->onDataRead.connect(boost::bind(&WebSocketClient::handleDataRead, this, _1));
	m_conn->onConnectFinished.connect(boost::bind(&WebSocketClient::handleConnected, this, _1));
	m_conn->onDisconnected.connect(boost::bind(&WebSocketClient::handleDisconnected, this, _1));
	m_conn->connect(Swift::HostAddressPort(addrs[0], 443));
}

}
