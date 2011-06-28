/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <string>
#include <Swiften/Base/SafeByteArray.h>
#include <boost/signal.hpp>

namespace Swift {
	class JID;

	class UserRegistry {
		public:
			virtual ~UserRegistry();

			virtual bool isValidUserPassword(const JID& user, const SafeByteArray& password) = 0;

			boost::signal<void (const std::string &user)> onPasswordValid;
			boost::signal<void (const std::string &user)> onPasswordInvalid;

	};
}
