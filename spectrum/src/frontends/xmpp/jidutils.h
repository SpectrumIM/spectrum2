/**
 * libtransport -- C++ library for easy XMPP Transports development
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

#include <string>

#include "Swiften/JID/JID.h"
#include "transport/Buddy.h"

//Default resource name which contacts use
#define JID_DEFAULT_RESOURCE "bot"

namespace Transport {

class User;
class Component;

/// Mapping between the core's legacy names and XMPP JIDs.
namespace JIDUtils {

	/// Returns the JID node for the legacy name: escaped (BUDDY_JID_ESCAPING) or with the last '@' replaced by '%'.
	std::string getSafeName(const std::string &legacyName, BuddyFlag flags);

	/// Returns the JID of the buddy: <safe name>@<transport JID>/bot.
	Swift::JID getBuddyJID(Buddy *buddy);

	/// Returns the JID of the buddy with this legacy name and flags.
	Swift::JID getBuddyJID(const std::string &legacyName, BuddyFlag flags, const std::string &transportJID);

	/// Returns legacy name of buddy from JID.

	/// If user is given and has the buddy in his roster, the buddy's name (case-sensitive) is returned.
	/// \param jid Jabber ID.
	/// \return legacy name of buddy from JID.
	std::string JIDToLegacyName(const Swift::JID &jid, User *user = NULL);

	Buddy *JIDToBuddy(const Swift::JID &jid, User *user);

	BuddyFlag buddyFlagsFromJID(const Swift::JID &jid);
}

}
