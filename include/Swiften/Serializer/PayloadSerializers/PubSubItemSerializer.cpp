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

namespace Swift {

PubSubItemSerializer::PubSubItemSerializer() : GenericPayloadSerializer<PubSubItem>() {
}

std::string PubSubItemSerializer::serializePayload(boost::shared_ptr<PubSubItem> payload)  const {
	XMLElement item("item");
	if (!payload->getId().empty()) {
		item.setAttribute("id", payload->getId());
	}

	boost::shared_ptr<XMLElement> body(new XMLElement("body", "http://www.w3.org/1999/xhtml"));
	body->addNode(boost::shared_ptr<XMLRawTextNode>(new XMLRawTextNode(payload->getData())));
	item.addNode(body);

	return item.serialize();
}

}
