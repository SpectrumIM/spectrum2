/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <map>

#include <Swiften/JID/JID.h>
#include <Swiften/Elements/Stanza.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class ServerSession;

	class ServerStanzaRouter {
		public:
			ServerStanzaRouter();

			bool routeStanza(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza>);

			void addClientSession(ServerSession*);
			void removeClientSession(ServerSession*);

		private:
			std::vector<ServerSession*> clientSessions_;
	};
}
