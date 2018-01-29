/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PrivilegeSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Elements/Forwarded.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

#include <Swiften/Serializer/ForwardedSerializer.h>

namespace Swift {

PrivilegeSerializer::PrivilegeSerializer(PayloadSerializerCollection* serializers) : GenericPayloadSerializer<Privilege>(), serializers_(serializers) {
}

PrivilegeSerializer::~PrivilegeSerializer() {
}

std::string PrivilegeSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege> payload)  const {
	if (!payload)
		return;

	XMLElement element("privilege", "urn:xmpp:privilege:1");

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza(payload->getStanza());
	if (stanza) {
		if (SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded> forwarded = std::dynamic_pointer_cast<Forwarded>(stanza)) {
			element.addNode(SWIFTEN_SHRPTR_NAMESPACE::make_shared<XMLRawTextNode>(safeByteArrayToString(ForwardedSerializer(serializers_).serialize(forwarded))))
		}
	}

	return element.serialize();
}

}
