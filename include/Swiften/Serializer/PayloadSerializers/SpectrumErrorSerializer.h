/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/SpectrumErrorPayload.h>

namespace Swift {
	class SpectrumErrorSerializer : public GenericPayloadSerializer<SpectrumErrorPayload> {
		public:
			SpectrumErrorSerializer();

			virtual std::string serializePayload(std::shared_ptr<SpectrumErrorPayload>)  const;
	};
}
