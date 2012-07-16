/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2012, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "transport/adhoccommand.h"
#include "transport/adhoccommandfactory.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/buddy.h"
#include "transport/factory.h"
#include "transport/user.h"
#include "transport/logging.h"

namespace Transport {

DEFINE_LOGGER(logger, "AdHocCommand");

AdHocCommand::AdHocCommand(Component *component, const Swift::JID &initiator, const Swift::JID &to) {
	m_component = component;
	m_initiator = initiator;
	m_to = to;

	std::string bucket = "abcdefghijklmnopqrstuvwxyz";
	for (int i = 0; i < 32; i++) {
		m_id += bucket[rand() % bucket.size()];
	}
}

AdHocCommand::~AdHocCommand() {
}

}
