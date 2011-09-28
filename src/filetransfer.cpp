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

#include "transport/filetransfer.h"
#include "transport/usermanager.h"
#include "transport/transport.h"
#include "transport/user.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/RosterItemPayload.h"
#include "Swiften/Elements/RosterItemExchangePayload.h"
#include "Swiften/FileTransfer/FileTransferManagerImpl.h"
#include "log4cxx/logger.h"
#include <boost/foreach.hpp>

#include <map>
#include <iterator>

using namespace log4cxx;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("FileTransfer");

FileTransfer::FileTransfer(User *user, Component *component, const Swift::JID &from) {
	m_user = user;
	m_component = component;
	m_ftManager = new Swift::FileTransferManagerImpl(from, m_component->getJingleSessionManager(), m_component->getIQRouter(),
											  user, m_component->getPresenceOracle(),
											  m_component->getNetworkFactories()->getConnectionFactory(),
											  m_component->getNetworkFactories()->getConnectionServerFactory(),
											  m_component->getNetworkFactories()->getTimerFactory(),
											  m_component->getNetworkFactories()->getNATTraverser());
	LOG4CXX_INFO(logger, "FileTransfer " << this << " from '" << from.toString() << "' to '" << user->getJID().toString() << "' created");
}

FileTransfer::~FileTransfer() {
	LOG4CXX_INFO(logger, "FileTransfer " << this << " destroyed");
	delete m_ftManager;
}

}
