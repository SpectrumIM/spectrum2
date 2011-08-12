/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/InvisibleSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Swift {

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
InvisibleSerializer::InvisibleSerializer() : GenericPayloadSerializer<InvisiblePayload>() {
}

std::string InvisibleSerializer::serializePayload(boost::shared_ptr<InvisiblePayload> attention)  const {
	XMLElement attentionElement("invisible", "urn:xmpp:invisible:0");

	return attentionElement.serialize();
}

}
