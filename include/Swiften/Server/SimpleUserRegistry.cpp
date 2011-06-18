/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Server/SimpleUserRegistry.h>

namespace Swift {

SimpleUserRegistry::SimpleUserRegistry() {
}

bool SimpleUserRegistry::isValidUserPassword(const JID& user, const SafeByteArray& password) const {
	std::map<JID,SafeByteArray>::const_iterator i = users.find(user);
	return i != users.end() ? i->second == password : false;
}

void SimpleUserRegistry::addUser(const JID& user, const std::string& password) {
	users.insert(std::make_pair(user, createSafeByteArray(password)));
}

}
