/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/GatewayPayload.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class GatewayPayloadSerializer : public GenericPayloadSerializer<GatewayPayload> {
		public:
			GatewayPayloadSerializer();

			virtual std::string serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<GatewayPayload> item)  const;
	};
}
