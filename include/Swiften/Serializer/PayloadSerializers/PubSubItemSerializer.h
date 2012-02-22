/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/PubSubItem.h>

namespace Swift {
	class PubSubItemSerializer : public GenericPayloadSerializer<PubSubItem> {
		public:
			PubSubItemSerializer();

			virtual std::string serializePayload(boost::shared_ptr<PubSubItem> item)  const;
	};
}
