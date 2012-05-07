/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Elements/PubSubSubscriptionPayload.h>

namespace Swift {

PubSubSubscriptionPayload::PubSubSubscriptionPayload(const JID &jid, const std::string &node) :
	jid(jid), node(node), type(None) {
		
	}

}
