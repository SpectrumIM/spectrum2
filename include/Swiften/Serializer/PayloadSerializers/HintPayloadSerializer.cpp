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
	switch(hint->getType()) {
	case HintPayload::NoPermanentStore: tagname = "no-permanent-store"; break;
	case HintPayload::NoStore: tagname = "no-store"; break;
	case HintPayload::NoCopy: tagname = "no-copy"; break;
	case HintPayload::Store: tagname = "store"; break;
	}

	return XMLElement(tagname, "urn:xmpp:hints").serialize();
}

}
