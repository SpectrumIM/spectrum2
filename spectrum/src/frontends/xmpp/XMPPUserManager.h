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

#include <string>
#include <map>
#include "transport/UserManager.h"
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/VCard.h"
#include "Swiften/Network/Timer.h"

namespace Transport {

class Component;
class StorageBackend;
class StorageResponder;
class VCardResponder;
class XMPPUserRegistration;
class GatewayResponder;
class AdHocManager;
class SettingsAdHocCommandFactory;
class RosterResponder;
class DiscoItemsResponder;
class XMPPFrontend;

/// Manages online XMPP Users.

/// This class handles presences and creates User classes when new user connects.
/// It also removes the User class once the last user's resource disconnected.
class XMPPUserManager : public UserManager {
	public:
		XMPPUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL);

		virtual ~XMPPUserManager();

		virtual void sendVCard(unsigned int id, const pbnetwork::VCard &vcard);

		UserRegistration *getUserRegistration();

		DiscoItemsResponder *getDiscoItemsResponder() {
			return m_discoItemsResponder;
		}

		/// Handles presence addressed to the transport itself (for example to="j2j.domain.tld")
		/// or to a room (for example to="#chat%irc.freenode.org@irc.domain.tld").
		void handlePresence(Swift::Presence::ref presence);

		/// Handles every presence the frontend receives.
		void handleGeneralPresenceReceived(Swift::Presence::ref presence);

		void handleMessageReceived(Swift::Message::ref message);

		void handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info);

		/// Connects user manually.
		/// \param user JID of user.
		void connectUser(const Swift::JID &user);

		/// Disconnects user manually.
		/// \param user JID of user.
		void disconnectUser(const Swift::JID &user);

	protected:
		void doRemoveUser(User *user);

	private:
		XMPPFrontend *getFrontend();
		void handleProbePresence(Swift::Presence::ref presence);
		void handleErrorPresence(Swift::Presence::ref presence);
		void handleSubscription(Swift::Presence::ref presence);
		void handleMUCPresence(Swift::Presence::ref presence);
		void handleRemoveTimeout(const std::string jid, User *user, bool reconnect);
		void handleVCardRequired(User *, const std::string &name, unsigned int id);
		void handleVCardUpdated(User *, std::shared_ptr<Swift::VCard> vcard);

		StorageResponder *m_storageResponder;
		VCardResponder *m_vcardResponder;
		Component *m_component;
		XMPPUserRegistration *m_userRegistration;
		GatewayResponder *m_gatewayResponder;
		AdHocManager *m_adHocManager;
		SettingsAdHocCommandFactory *m_settings;
		RosterResponder *m_rosterResponder;
		DiscoItemsResponder *m_discoItemsResponder;
		Swift::Timer::ref m_removeTimer;
};

}
