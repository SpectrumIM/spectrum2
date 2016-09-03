/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/AttentionPayload.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class AttentionSerializer : public GenericPayloadSerializer<AttentionPayload> {
		public:
			AttentionSerializer();

			virtual std::string serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<AttentionPayload>)  const;
	};
}
