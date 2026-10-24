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

#include "jidutils.h"
#include "transport/User.h"
#include "transport/RosterManager.h"
#include "transport/Transport.h"

namespace Transport {

namespace JIDUtils {

std::string getSafeName(const std::string &legacyName, BuddyFlag flags) {
	std::string name = legacyName;
	if (flags & BUDDY_JID_ESCAPING) {
		name = Swift::JID::getEscapedNode(name);
	}
	else {
		if (name.find_last_of("@") != std::string::npos) {
			name.replace(name.find_last_of("@"), 1, "%"); // OK
		}
	}
	return name;
}

Swift::JID getBuddyJID(const std::string &legacyName, BuddyFlag flags, const std::string &transportJID) {
	try {
		return Swift::JID(getSafeName(legacyName, flags), transportJID, JID_DEFAULT_RESOURCE);
	}
	catch (...) {
		return Swift::JID();
	}
}

Swift::JID getBuddyJID(Buddy *buddy) {
	return getBuddyJID(buddy->getName(), buddy->getFlags(), buddy->getRosterManager()->getUser()->getComponent()->getJID());
}

static std::string nodeToLegacyName(const Swift::JID &jid) {
	std::string name;
	if (jid.getUnescapedNode() == jid.getNode()) {
		name = jid.getNode();
		if (name.find_last_of("%") != std::string::npos) {
			name.replace(name.find_last_of("%"), 1, "@"); // OK
		}
	}
	else {
		name = jid.getUnescapedNode();
	}
	return name;
}

std::string JIDToLegacyName(const Swift::JID &jid, User *user) {
	std::string name = nodeToLegacyName(jid);

	// If we have User associated with this request, we will find the
	// buddy in his Roster, because JID received from network can be lower-case
	// version of the original legacy name, but in the roster, we store the
	// case-sensitive version of the legacy name.
	if (user) {
		Buddy *b = user->getRosterManager()->getBuddy(name);
		if (b) {
			return b->getName();
		}
	}

	return name;
}

Buddy *JIDToBuddy(const Swift::JID &jid, User *user) {
	std::string name = nodeToLegacyName(jid);

	// If we have User associated with this request, we will find the
	// buddy in his Roster, because JID received from network can be lower-case
	// version of the original legacy name, but in the roster, we store the
	// case-sensitive version of the legacy name.
	if (user) {
		Buddy *b = user->getRosterManager()->getBuddy(name);
		if (b) {
			return b;
		}
	}

	return NULL;
}

BuddyFlag buddyFlagsFromJID(const Swift::JID &jid) {
	if (jid.getUnescapedNode() == jid.getNode()) {
		return BUDDY_NO_FLAG;
	}
	return BUDDY_JID_ESCAPING;
}

}

}
