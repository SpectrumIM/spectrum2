/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/PubSubPayload.h>

namespace Swift {
	class PayloadSerializerCollection;

	class PubSubPayloadSerializer : public GenericPayloadSerializer<PubSubPayload> {
		public:
			PubSubPayloadSerializer(PayloadSerializerCollection *serializers);

			virtual std::string serializePayload(boost::shared_ptr<PubSubPayload> item)  const;
		private:
			PayloadSerializerCollection *serializers;
	};
}
