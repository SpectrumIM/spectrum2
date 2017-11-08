/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <string>
#include <Swiften/Base/SafeByteArray.h>
#include "Swiften/Server/ServerFromClientSession.h"

namespace Swift {
	class JID;

	class UserRegistry {
		public:
			virtual ~UserRegistry();

			virtual void isValidUserPassword(const JID& user, ServerFromClientSession *session, const SafeByteArray& password) = 0;

			virtual void stopLogin(const JID &/*user*/, ServerFromClientSession *) {};
	};
}
