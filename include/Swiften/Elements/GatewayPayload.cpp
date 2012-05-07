/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Elements/GatewayPayload.h>

namespace Swift {

GatewayPayload::GatewayPayload(const JID &jid, const std::string &desc, const std::string &prompt) :
	jid(jid), desc(desc), prompt(prompt) {
		
	}

}
