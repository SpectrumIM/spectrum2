/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/PubSubSubscribePayload.h>

namespace Swift {
	class PubSubSubscribePayloadSerializer : public GenericPayloadSerializer<PubSubSubscribePayload> {
		public:
			PubSubSubscribePayloadSerializer();

			virtual std::string serializePayload(boost::shared_ptr<PubSubSubscribePayload> item)  const;
	};
}
