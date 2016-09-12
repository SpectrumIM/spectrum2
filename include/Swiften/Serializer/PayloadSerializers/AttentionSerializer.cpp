/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/AttentionSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Swift {

AttentionSerializer::AttentionSerializer() : GenericPayloadSerializer<AttentionPayload>() {
}

std::string AttentionSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<AttentionPayload> attention)  const {
	XMLElement attentionElement("attention", "urn:xmpp:attention:0");

	return attentionElement.serialize();
}

}
