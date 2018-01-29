/*
 * Implements XEP-0334: Message Processing Hints
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <string>

#include <Swiften/Serializer/PayloadSerializers/HintPayloadSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Swift {

HintPayloadSerializer::HintPayloadSerializer() : GenericPayloadSerializer<HintPayload>() {
}

std::string HintPayloadSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<HintPayload> hint)  const {
	std::string tagname = "";
	switch(hint->type) {
	case NoPermanentStore: tagname = "no-permanent-store"; break;
	case NoStore: tagname = "no-store"; break;
	case NoCopy: tagname = "no-copy"; break;
	case Store: tagname = "store"; break;
	}

	return xmlElement(tagname, "urn:xmpp:hints").serialize();
}

}
