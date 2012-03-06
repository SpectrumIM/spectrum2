/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PubSubItemSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

PubSubItemSerializer::PubSubItemSerializer(PayloadSerializerCollection *serializers) : 
	GenericPayloadSerializer<PubSubItem>(), serializers(serializers) {
}

std::string PubSubItemSerializer::serializePayload(boost::shared_ptr<PubSubItem> payload)  const {
	XMLElement item("item");
	if (!payload->getId().empty()) {
		item.setAttribute("id", payload->getId());
	}

	if (!payload->getPayloads().empty()) {		
		foreach(boost::shared_ptr<Payload> subPayload, payload->getPayloads()) {
			PayloadSerializer* serializer = serializers->getPayloadSerializer(subPayload);
			if (serializer) {
				item.addNode(boost::shared_ptr<XMLRawTextNode>(new XMLRawTextNode(serializer->serialize(subPayload))));
			}
		}
	}

	return item.serialize();
}

}
