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

#ifdef SWIFTEN_SUPPORTS_FORWARDED
#include <Swiften/Serializer/PayloadSerializers/ForwardedSerializer.h>
#else
#include <Swiften/Serializer/PayloadSerializerCollection.h>
#endif

namespace Swift {

PrivilegeSerializer::PrivilegeSerializer(PayloadSerializerCollection* serializers) : GenericPayloadSerializer<Privilege>(), serializers_(serializers) {
}

PrivilegeSerializer::~PrivilegeSerializer() {
}

std::string PrivilegeSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege> payload)  const {
	if (!payload) {
		return "";
	}

	XMLElement element("privilege", "urn:xmpp:privilege:1");

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege::Forwarded> forwarded(payload->getForwarded());
	if (forwarded) {
		std::string forwardedStr = "";
#ifdef SWIFTEN_SUPPORTS_FORWARDED
		forwardedStr = ForwardedSerializer(serializers_).serialize(forwarded);
#else
		PayloadSerializer* serializer = serializers_->getPayloadSerializer(payload);
		if(serializer) {
			forwardedStr = serializer->serialize(payload);
		}
#endif
		element.addNode(SWIFTEN_SHRPTR_NAMESPACE::make_shared<XMLRawTextNode>(forwardedStr));
	}

	return element.serialize();
}

}
