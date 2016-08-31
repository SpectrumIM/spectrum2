/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/GatewayPayloadSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

GatewayPayloadSerializer::GatewayPayloadSerializer()
	: GenericPayloadSerializer<GatewayPayload>() {
}

std::string GatewayPayloadSerializer::serializePayload(std::shared_ptr<GatewayPayload> payload)  const {
	XMLElement query("query", "jabber:iq:gateway");

	if (payload->getJID().isValid()) {
		std::shared_ptr<XMLElement> jid(new XMLElement("jid", "", payload->getJID().toBare().toString()));
		query.addNode(jid);
	}

	if (!payload->getDesc().empty()) {
		std::shared_ptr<XMLElement> desc(new XMLElement("desc", "", payload->getDesc()));
		query.addNode(desc);
	}

	if (!payload->getPrompt().empty()) {
		std::shared_ptr<XMLElement> prompt(new XMLElement("prompt", "", payload->getPrompt()));
		query.addNode(prompt);
	}

	return query.serialize();
}

}
