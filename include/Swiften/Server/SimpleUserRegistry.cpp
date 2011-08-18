/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Server/SimpleUserRegistry.h>

namespace Swift {

SimpleUserRegistry::SimpleUserRegistry() {
}

void SimpleUserRegistry::isValidUserPassword(const JID& user, ServerFromClientSession *session, const SafeByteArray& password) {
	std::map<JID,SafeByteArray>::const_iterator i = users.find(user);

	
	if (i != users.end() && i->second == password) {
		session->handlePasswordValid();
	}
	else {
		session->handlePasswordInvalid();
	}
}

void SimpleUserRegistry::addUser(const JID& user, const std::string& password) {
	users.insert(std::make_pair(user, createSafeByteArray(password)));
}

}
