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

#include "XMPPUserManager.h"
#include "XMPPUserRegistration.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/StorageBackend.h"
#include "transport/Logging.h"
#include "transport/Frontend.h"
#include "storageresponder.h"
#include "vcardresponder.h"
#include "XMPPFrontend.h"
#include "gatewayresponder.h"
#include "adhocmanager.h"
#include "settingsadhoccommand.h"
#include "RosterResponder.h"
#include "discoitemsresponder.h"

#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Elements/MUCPayload.h"
#include "Swiften/Elements/ChatState.h"
#ifndef __FreeBSD__ 
#ifndef __MACH__
#include "malloc.h"
#endif
#endif
// #include "valgrind/memcheck.h"

namespace Transport {

DEFINE_LOGGER(xmppUserManagerLogger, "XMPPUserManager");

XMPPUserManager::XMPPUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend) : UserManager(component, userRegistry, storageBackend) {
	m_component = component;

	XMPPFrontend *frontend = static_cast<XMPPFrontend *>(component->getFrontend());
	if (storageBackend) {
		m_storageResponder = new StorageResponder(frontend->getIQRouter(), storageBackend, this);
		m_storageResponder->start();
	}
	else {
		m_storageResponder = NULL;
	}

	m_vcardResponder = new VCardResponder(frontend->getIQRouter(), component->getNetworkFactories(), this);
	m_vcardResponder->onVCardRequired.connect(boost::bind(&XMPPUserManager::handleVCardRequired, this, _1, _2, _3));
	m_vcardResponder->onVCardUpdated.connect(boost::bind(&XMPPUserManager::handleVCardUpdated, this, _1, _2));
	m_vcardResponder->start();

	if (storageBackend) {
		m_userRegistration = new XMPPUserRegistration(component, this, storageBackend);
		m_userRegistration->start();
	}
	else {
		m_userRegistration = NULL;
	}

// 	StatsResponder statsResponder(component, this, &plugin, storageBackend);
// 	statsResponder.start();

	m_gatewayResponder = new GatewayResponder(frontend->getIQRouter(), this);
	m_gatewayResponder->start();

	m_rosterResponder = new RosterResponder(frontend->getIQRouter(), this);
	m_rosterResponder->start();

	m_discoItemsResponder = new DiscoItemsResponder(component, this);
	m_discoItemsResponder->start();

	m_adHocManager = new AdHocManager(component, m_discoItemsResponder, this, storageBackend);
	m_adHocManager->start();

	m_settings = new SettingsAdHocCommandFactory();
	m_adHocManager->addAdHocCommand(m_settings);
}

XMPPUserManager::~XMPPUserManager() {
	if (m_storageResponder) {
		m_storageResponder->stop();
		delete m_storageResponder;
	}

	if (m_userRegistration) {
		m_userRegistration->stop();
		delete m_userRegistration;
	}

	m_gatewayResponder->stop();
	delete m_gatewayResponder;

	m_adHocManager->stop();
	delete m_adHocManager;

	m_vcardResponder->stop();
	delete m_vcardResponder;

	m_rosterResponder->stop();
	delete m_rosterResponder;

	m_discoItemsResponder->stop();
	delete m_discoItemsResponder;

	delete m_settings;
}

void XMPPUserManager::sendVCard(unsigned int id, Swift::VCard::ref vcard) {
	m_vcardResponder->sendVCard(id, vcard);
}

void XMPPUserManager::handleVCardRequired(User *user, const std::string &name, unsigned int id) {
	m_component->getFrontend()->onVCardRequired(user, name, id);
}

void XMPPUserManager::handleVCardUpdated(User *user, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::VCard> vcard) {
	m_component->getFrontend()->onVCardUpdated(user, vcard);
}

UserRegistration *XMPPUserManager::getUserRegistration() {
	return m_userRegistration;
}

}
