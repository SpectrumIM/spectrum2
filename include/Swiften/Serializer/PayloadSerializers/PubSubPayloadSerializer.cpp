/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PubSubPayloadSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

PubSubPayloadSerializer::PubSubPayloadSerializer(PayloadSerializerCollection *serializers)
	: GenericPayloadSerializer<PubSubPayload>(),
	serializers(serializers) {
}

std::string PubSubPayloadSerializer::serializePayload(boost::shared_ptr<PubSubPayload> payload)  const {
	XMLElement pubsub("pubsub", "http://jabber.org/protocol/pubsub");

	if (!payload->getPayloads().empty()) {		
		foreach(boost::shared_ptr<Payload> subPayload, payload->getPayloads()) {
			PayloadSerializer* serializer = serializers->getPayloadSerializer(subPayload);
			if (serializer) {
				pubsub.addNode(boost::shared_ptr<XMLRawTextNode>(new XMLRawTextNode(serializer->serialize(subPayload))));
			}
		}
	}

	return pubsub.serialize();
}

}
