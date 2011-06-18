/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <map>

#include <Swiften/JID/JID.h>
#include <string>
#include <Swiften/Server/UserRegistry.h>

namespace Swift {
	

	class SimpleUserRegistry : public UserRegistry {
		public:
			SimpleUserRegistry();

			virtual bool isValidUserPassword(const JID& user, const SafeByteArray& password) const;
			void addUser(const JID& user, const std::string& password);

		private:
			std::map<JID, SafeByteArray> users;
	};
}
