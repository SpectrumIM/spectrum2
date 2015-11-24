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

#include "SlackRTM.h"
#include "SlackFrontend.h"
#include "SlackUser.h"

#include "transport/Transport.h"
#include "transport/HTTPRequest.h"
#include "transport/Util.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <iterator>

namespace Transport {

DEFINE_LOGGER(logger, "SlackRTM");

SlackRTM::SlackRTM(Component *component, StorageBackend *storageBackend, UserInfo uinfo) : m_uinfo(uinfo) {
	m_component = component;
	m_storageBackend = storageBackend;


#if HAVE_SWIFTEN_3
	Swift::TLSOptions o;
#endif
	Swift::PlatformTLSFactories *m_tlsFactory = new Swift::PlatformTLSFactories();
#if HAVE_SWIFTEN_3
	m_tlsConnectionFactory = new Swift::TLSConnectionFactory(m_tlsFactory->getTLSContextFactory(), component->getNetworkFactories()->getConnectionFactory(), o);
#else
	m_tlsConnectionFactory = new Swift::TLSConnectionFactory(m_tlsFactory->getTLSContextFactory(), component->getNetworkFactories()->getConnectionFactory());
#endif


	std::string url = "https://slack.com/api/rtm.start?";
	url += "token=" + Util::urlencode(m_uinfo.encoding);

// 	HTTPRequest *req = new HTTPRequest();
// 	req->GET(THREAD_POOL(m_component), url,
// 			boost::bind(&SlackRTM::handleRTMStart, this, _1, _2, _3, _4));
}

SlackRTM::~SlackRTM() {

}

void SlackRTM::handleRTMStart(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data) {
	if (!ok) {
		LOG4CXX_ERROR(logger, req->getError());
		LOG4CXX_ERROR(logger, data);
		return;
	}

	rapidjson::Value &url = resp["url"];
	if (!url.IsString()) {
		LOG4CXX_ERROR(logger, "No 'url' object in the reply.");
		LOG4CXX_ERROR(logger, data);
		return;
	}

	std::string u = url.GetString();
	LOG4CXX_INFO(logger, "Started RTM, WebSocket URL is " << u);

	u = u.substr(6);
	m_host = u.substr(0, u.find("/"));
	m_path = u.substr(u.find("/"));

	LOG4CXX_INFO(logger, "Starting DNS query for " << m_host << " " << m_path);
	m_dnsQuery = m_component->getNetworkFactories()->getDomainNameResolver()->createAddressQuery(m_host);
	m_dnsQuery->onResult.connect(boost::bind(&SlackRTM::handleDNSResult, this, _1, _2));
	m_dnsQuery->run();
}

void SlackRTM::handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
	LOG4CXX_INFO(logger, "data read");
	std::string d = Swift::safeByteArrayToString(*data);
	LOG4CXX_INFO(logger, d);
}

void SlackRTM::handleConnected(bool error) {
	if (error) {
		LOG4CXX_ERROR(logger, "Connection to " << m_host << " failed");
		return;
	}

	LOG4CXX_INFO(logger, "Connected to " << m_host);

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

void SlackRTM::handleDNSResult(const std::vector<Swift::HostAddress> &addrs, boost::optional<Swift::DomainNameResolveError>) {
	m_conn = m_tlsConnectionFactory->createConnection();
	m_conn->onDataRead.connect(boost::bind(&SlackRTM::handleDataRead, this, _1));
	m_conn->onConnectFinished.connect(boost::bind(&SlackRTM::handleConnected, this, _1));
	m_conn->connect(Swift::HostAddressPort(addrs[0], 443));
}

}
