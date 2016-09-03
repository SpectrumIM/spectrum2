/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <BlockSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Transport {

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
BlockSerializer::BlockSerializer() : Swift::GenericPayloadSerializer<BlockPayload>() {
}

std::string BlockSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<BlockPayload> attention)  const {
	Swift::XMLElement attentionElement("block", "urn:xmpp:block:0");

	return attentionElement.serialize();
}

}
