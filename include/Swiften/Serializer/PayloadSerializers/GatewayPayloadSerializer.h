/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/GatewayPayload.h>

namespace Swift {
	class GatewayPayloadSerializer : public GenericPayloadSerializer<GatewayPayload> {
		public:
			GatewayPayloadSerializer();

			virtual std::string serializePayload(std::shared_ptr<GatewayPayload> item)  const;
	};
}
