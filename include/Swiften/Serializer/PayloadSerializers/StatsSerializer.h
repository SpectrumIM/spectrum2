/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/StatsPayload.h>

namespace Swift {
	class StatsSerializer : public GenericPayloadSerializer<StatsPayload> {
		public:
			StatsSerializer();

			virtual std::string serializePayload(std::shared_ptr<StatsPayload>)  const;
	};
}
