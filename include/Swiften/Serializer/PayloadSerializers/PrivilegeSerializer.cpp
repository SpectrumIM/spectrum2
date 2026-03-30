/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PrivilegeSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

#include <Swiften/Elements/Forwarded.h>
#include <Swiften/Serializer/PayloadSerializers/ForwardedSerializer.h>

namespace Swift {

PrivilegeSerializer::PrivilegeSerializer(PayloadSerializerCollection* serializers) : GenericPayloadSerializer<Privilege>(), serializers_(serializers) {
}

PrivilegeSerializer::~PrivilegeSerializer() {
}

std::string PrivilegeSerializer::serializePayload(std::shared_ptr<Privilege> payload)  const {
	if (!payload) {
		return "";
	}

	XMLElement element("privilege", "urn:xmpp:privilege:1");

	std::shared_ptr<Privilege::Forwarded> forwarded(payload->getForwarded());
	if (forwarded) {
		std::string forwardedStr = "";
		forwardedStr = ForwardedSerializer(serializers_).serialize(forwarded);
		element.addNode(std::make_shared<XMLRawTextNode>(forwardedStr));
	}

	return element.serialize();
}

}
