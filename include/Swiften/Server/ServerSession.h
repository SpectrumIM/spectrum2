/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include <Swiften/Elements/Stanza.h>

namespace Swift {
	class ServerSession {
		public:
			virtual ~ServerSession();

			virtual const JID& getJID() const = 0;
			virtual int getPriority() const = 0;

			virtual void sendStanza(boost::shared_ptr<Stanza>) = 0;
	};
}
