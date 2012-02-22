/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PubSubSubscribePayloadSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

PubSubSubscribePayloadSerializer::PubSubSubscribePayloadSerializer()
	: GenericPayloadSerializer<PubSubSubscribePayload>() {
}

std::string PubSubSubscribePayloadSerializer::serializePayload(boost::shared_ptr<PubSubSubscribePayload> payload)  const {
	XMLElement subscribe("subscribe");

	if (!payload->getJID().isValid()) {
		subscribe.setAttribute("jid", payload->getJID().toBare().toString());
	}

	if (!payload->getNode().empty()) {
		subscribe.setAttribute("node", payload->getNode());
	}

	return subscribe.serialize();
}

}
