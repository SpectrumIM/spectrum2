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

#include "transport/StorageBackend.h"
#include "rapidjson/document.h"

#include <Swiften/Network/TLSConnectionFactory.h>
#include <Swiften/Network/HostAddressPort.h>
#include <Swiften/TLS/PlatformTLSFactories.h>
#include <Swiften/TLS/TLSOptions.h>
#include <Swiften/Network/DomainNameResolveError.h>
#include <Swiften/Network/DomainNameAddressQuery.h>
#include <Swiften/Network/DomainNameResolver.h>
#include <Swiften/Network/HostAddress.h>
#include <Swiften/Base/SafeByteArray.h>

#include <string>
#include <algorithm>
#include <map>

#include <boost/signal.hpp>

namespace Transport {

class Component;
class StorageBackend;
class HTTPRequest;

class SlackRTM {
	public:
		SlackRTM(Component *component, StorageBackend *storageBackend, UserInfo uinfo);

		virtual ~SlackRTM();

	private:
		void handleDNSResult(const std::vector<Swift::HostAddress>&, boost::optional<Swift::DomainNameResolveError>);
		void handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data);
		void handleConnected(bool error);
		void handleRTMStart(HTTPRequest *req, bool ok, rapidjson::Document &resp, const std::string &data);

	private:
		Component *m_component;
		StorageBackend *m_storageBackend;
		UserInfo m_uinfo;
		boost::shared_ptr<Swift::DomainNameAddressQuery> m_dnsQuery;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::TLSConnectionFactory *m_tlsConnectionFactory;
		std::string m_host;
		std::string m_path;
};

}
