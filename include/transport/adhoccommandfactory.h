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

#include <string>
#include <algorithm>
#include <map>
#include "transport/adhoccommand.h"
#include "Swiften/Swiften.h"

namespace Transport {

class Component;

class AdHocCommandFactory {
	public:
		AdHocCommandFactory() {}

		/// Destructor.
		virtual ~AdHocCommandFactory() {}

		virtual AdHocCommand *createAdHocCommand(Component *component, const Swift::JID &initiator, const Swift::JID &to) = 0;

		virtual std::string getNode() = 0;

		virtual std::string getName() = 0;
};

}
