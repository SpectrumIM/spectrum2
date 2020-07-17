/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/XHTMLIMSerializer.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {

XHTMLIMSerializer::XHTMLIMSerializer() : GenericPayloadSerializer<XHTMLIMPayload>() {
}

std::string XHTMLIMSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<XHTMLIMPayload> payload)  const {
	XMLElement html("html", "http://jabber.org/protocol/xhtml-im");

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<XMLElement> body(new XMLElement("body", "http://www.w3.org/1999/xhtml"));
	body->addNode(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<XMLRawTextNode>(new XMLRawTextNode(payload->getBody())));
	html.addNode(body);

	return html.serialize();
}

}
