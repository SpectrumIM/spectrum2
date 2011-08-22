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
#include "Swiften/Elements/VCard.h"

namespace Transport {

class StorageBackend;
class UserManager;
class User;

class VCardResponder : public Swift::Responder<Swift::VCard> {
	public:
		VCardResponder(Swift::IQRouter *router, Swift::NetworkFactories *factories, UserManager *userManager);
		~VCardResponder();

		void sendVCard(unsigned int id, boost::shared_ptr<Swift::VCard> vcard);

		boost::signal<void (User *, const std::string &name, unsigned int id)> onVCardRequired;
		boost::signal<void (User *, boost::shared_ptr<Swift::VCard> vcard)> onVCardUpdated;

		void collectTimeouted();

	private:
		struct VCardData {
			Swift::JID from;
			Swift::JID to;
			std::string id;
			time_t received;
		};

		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload);
		virtual bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::VCard> payload);
		UserManager *m_userManager;
		std::map<unsigned int, VCardData> m_queries;
		unsigned int m_id;
		Swift::Timer::ref m_collectTimer;
};

}