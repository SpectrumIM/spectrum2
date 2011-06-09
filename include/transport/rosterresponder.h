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
#include "Swiften/Queries/Responder.h"
#include "Swiften/Elements/RosterPayload.h"

namespace Transport {

class UserManager;
class Buddy;

class RosterResponder : public Swift::Responder<Swift::RosterPayload> {
	public:
		RosterResponder(Swift::IQRouter *router, UserManager *userManager);
		~RosterResponder();

		boost::signal<void (Buddy *, const Swift::RosterItemPayload &item)> onBuddyUpdated;

		boost::signal<void (Buddy *)> onBuddyRemoved;

		boost::signal<void (Buddy *, const Swift::RosterItemPayload &item)> onBuddyAdded;

	private:
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::RosterPayload> payload);
		virtual bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::RosterPayload> payload);
		UserManager *m_userManager;
};

}