/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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

#include <vector>
#include <list>
#include <boost/signal.hpp>
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/CapsInfo.h"
#include "Swiften/SwiftenCompat.h"
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)
#if HAVE_SWIFTEN_3
#include <Swiften/Crypto/CryptoProvider.h>
#include <Swiften/Crypto/PlatformCryptoProvider.h>
#endif


namespace Transport {

class Config;
class UserManager;

class DiscoInfoResponder : public Swift::GetResponder<Swift::DiscoInfo> {
	public:
		DiscoInfoResponder(Swift::IQRouter *router, Config *config, UserManager *userManager);
		~DiscoInfoResponder();

		void setTransportFeatures(std::list<std::string> &features);
		void setBuddyFeatures(std::list<std::string> &features);

		void addRoom(const std::string &jid, const std::string &name);
		void clearRooms();

		void addAdHocCommand(const std::string &node, const std::string &name);

		boost::signal<void (const Swift::CapsInfo &capsInfo)> onBuddyCapsInfoChanged;

		Swift::CapsInfo &getBuddyCapsInfo() {
				return m_capsInfo;
		}

	private:
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DiscoInfo> payload);
		void updateFeatures();

		Swift::DiscoInfo m_transportInfo;
		Swift::DiscoInfo *m_buddyInfo;
		Config *m_config;
		Swift::CapsInfo m_capsInfo;
		std::map<std::string, std::string> m_rooms;
		std::map<std::string, std::string> m_commands;
#if HAVE_SWIFTEN_3
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CryptoProvider> crypto;
#endif
		UserManager *m_userManager;
};

}
