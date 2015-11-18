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

#pragma once

#include <map>

#include <string>
#include <Swiften/Elements/Presence.h>
#include <Swiften/Client/StanzaChannel.h>

#include <Swiften/Base/boost_bsignals.h>

namespace Transport {

class Frontend;

class PresenceOracle {
	public:
		PresenceOracle(Frontend* frontend);
		~PresenceOracle();

		Swift::Presence::ref getLastPresence(const Swift::JID&) const;
		Swift::Presence::ref getHighestPriorityPresence(const Swift::JID& bareJID) const;
		std::vector<Swift::Presence::ref> getAllPresence(const Swift::JID& bareJID) const;

		void clearPresences(const Swift::JID& bareJID);

	public:
		boost::signal<void (Swift::Presence::ref)> onPresenceChange;

	private:
		void handleIncomingPresence(Swift::Presence::ref presence);
		void handleStanzaChannelAvailableChanged(bool);

	private:
		typedef std::map<Swift::JID, Swift::Presence::ref> PresenceMap;
		typedef std::map<Swift::JID, PresenceMap> PresencesMap;
		PresencesMap entries_;
		Frontend* frontend_;
};

}

