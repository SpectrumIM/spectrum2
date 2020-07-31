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
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Elements/DiscoItems.h"
#include "Swiften/Elements/CapsInfo.h"

namespace Transport {

class Component;
class DiscoInfoResponder;
class UserManager;

class DiscoItemsResponder : public Swift::GetResponder<Swift::DiscoItems> {
	public:
		DiscoItemsResponder(Component *component, UserManager *userManager);
		~DiscoItemsResponder();

		Swift::CapsInfo &getBuddyCapsInfo();

		void addAdHocCommand(const std::string &node, const std::string &name);
// 		void removeAdHocCommand(const std::string &node);

		void addRoom(const std::string &node, const std::string &name);
		void clearRooms();

		DiscoInfoResponder *getDiscoInfoResponder() {
			return m_discoInfoResponder;
		}

	private:
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, std::shared_ptr<Swift::DiscoItems> payload);

	private:
		Component *m_component;
		std::shared_ptr<Swift::DiscoItems> m_commands;
		std::shared_ptr<Swift::DiscoItems> m_rooms;
		DiscoInfoResponder *m_discoInfoResponder;
		UserManager *m_userManager;
};

}
