/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/PubSubSubscriptionPayloadSerializer.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <Swiften/Serializer/PayloadSerializerCollection.h>

namespace Swift {

PubSubSubscriptionPayloadSerializer::PubSubSubscriptionPayloadSerializer()
	: GenericPayloadSerializer<PubSubSubscriptionPayload>() {
}

std::string PubSubSubscriptionPayloadSerializer::serializePayload(boost::shared_ptr<PubSubSubscriptionPayload> payload)  const {
	XMLElement subscription("subscription");

	if (!payload->getJID().isValid()) {
		subscription.setAttribute("jid", payload->getJID().toBare().toString());
	}

	if (!payload->getNode().empty()) {
		subscription.setAttribute("node", payload->getNode());
	}

	switch (payload->getType()) {
		case PubSubSubscriptionPayload::None:
			subscription.setAttribute("subscription", "none");
			break;
		case PubSubSubscriptionPayload::Subscribed:
			subscription.setAttribute("subscription", "subscribed");
			break;
		case PubSubSubscriptionPayload::Unconfigured:
			subscription.setAttribute("subscription", "unconfigured");
			break;
		case PubSubSubscriptionPayload::Pending:
			subscription.setAttribute("subscription", "pending");
			break;
	}

	return subscription.serialize();
}

}
