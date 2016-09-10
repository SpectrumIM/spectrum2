/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include "Swiften/Base/IDGenerator.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Client/StanzaChannel.h"
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/IQ.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/TLS/Certificate.h"
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

namespace Swift {
	class Error;
	class ServerStanzaChannel : public StanzaChannel {
		public:
			ServerStanzaChannel(const JID &selfJID) : StanzaChannel() {
				m_jid = selfJID;
			}
			void addSession(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session);
			void removeSession(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> session);

			void sendIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<IQ> iq);
			void sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Message> message);
			void sendPresence(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Presence> presence);
#if HAVE_SWIFTEN_3
			void finishSession(const JID& to, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ToplevelElement> element, bool last = false);
#else
			void finishSession(const JID& to, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Element> element, bool last = false);
#endif
			bool getStreamManagementEnabled() const {
				return false;
			}
	
			bool isAvailable() const {
				return true;
			}
			
			std::vector<Certificate::ref> getPeerCertificateChain() const {
				return std::vector<Certificate::ref>();
			}

		private:
			std::string getNewIQID();
			void send(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza);
			void handleSessionFinished(const boost::optional<Session::SessionError>&, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> &session);
			void handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Element> element, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> &session);
			void handleDataRead(const SafeByteArray &data, const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> &session);
			void handleSessionInitialized();

		private:
			JID m_jid;
			IDGenerator idGenerator;
			// [JID][resources][ServerFromClientSession]
			std::map<std::string, std::list<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ServerFromClientSession> > > sessions;
	};

}
