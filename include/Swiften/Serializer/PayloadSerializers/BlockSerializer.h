/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/BlockPayload.h>

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
namespace Swift {
	class BlockSerializer : public GenericPayloadSerializer<BlockPayload> {
		public:
			BlockSerializer();

			virtual std::string serializePayload(boost::shared_ptr<BlockPayload>)  const;
	};
}
