/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/InvisiblePayload.h>

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
namespace Swift {
	class InvisibleSerializer : public GenericPayloadSerializer<InvisiblePayload> {
		public:
			InvisibleSerializer();

			virtual std::string serializePayload(std::shared_ptr<InvisiblePayload>)  const;
	};
}
