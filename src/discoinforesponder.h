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
#include "Swiften/Swiften.h"
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/CapsInfo.h"

namespace Transport {

class Config;

class DiscoInfoResponder : public Swift::GetResponder<Swift::DiscoInfo> {
	public:
		DiscoInfoResponder(Swift::IQRouter *router, Config *config);
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
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::DiscoInfo> payload);

		Swift::DiscoInfo m_transportInfo;
		Swift::DiscoInfo m_buddyInfo;
		Config *m_config;
		Swift::CapsInfo m_capsInfo;
		std::map<std::string, std::string> m_rooms;
		std::map<std::string, std::string> m_commands;
};

}