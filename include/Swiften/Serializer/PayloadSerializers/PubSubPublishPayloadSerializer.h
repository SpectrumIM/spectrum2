/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/PubSubPublishPayload.h>

namespace Swift {
	class PayloadSerializerCollection;

	class PubSubPublishPayloadSerializer : public GenericPayloadSerializer<PubSubPublishPayload> {
		public:
			PubSubPublishPayloadSerializer(PayloadSerializerCollection *serializers);

			virtual std::string serializePayload(boost::shared_ptr<PubSubPublishPayload> item)  const;
		private:
			PayloadSerializerCollection *serializers;
	};
}
