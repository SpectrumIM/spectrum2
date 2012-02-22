/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Elements/PubSubSubscribePayload.h>

namespace Swift {

PubSubSubscribePayload::PubSubSubscribePayload(const JID &jid, const std::string &node) :
	jid(jid), node(node) {
		
	}

}
