/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/PubSubSubscriptionPayload.h>

namespace Swift {
	class PubSubSubscriptionPayloadSerializer : public GenericPayloadSerializer<PubSubSubscriptionPayload> {
		public:
			PubSubSubscriptionPayloadSerializer();

			virtual std::string serializePayload(boost::shared_ptr<PubSubSubscriptionPayload> item)  const;
	};
}
