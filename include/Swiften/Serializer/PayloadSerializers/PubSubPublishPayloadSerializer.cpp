/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PubSubPublishPayloadSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

PubSubPublishPayloadSerializer::PubSubPublishPayloadSerializer(PayloadSerializerCollection *serializers)
	: GenericPayloadSerializer<PubSubPublishPayload>(),
	serializers(serializers) {
}

std::string PubSubPublishPayloadSerializer::serializePayload(boost::shared_ptr<PubSubPublishPayload> payload)  const {
	XMLElement publish("publish");

	if (!payload->getNode().empty()) {
		publish.setAttribute("node", payload->getNode());
	}

	if (!payload->getItems().empty()) {		
		foreach(boost::shared_ptr<Payload> subPayload, payload->getItems()) {
			PayloadSerializer* serializer = serializers->getPayloadSerializer(subPayload);
			if (serializer) {
				publish.addNode(boost::shared_ptr<XMLRawTextNode>(new XMLRawTextNode(serializer->serialize(subPayload))));
			}
		}
	}

	return publish.serialize();
}

}
