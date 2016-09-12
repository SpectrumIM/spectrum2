/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/XHTMLIMPayload.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class XHTMLIMSerializer : public GenericPayloadSerializer<XHTMLIMPayload> {
		public:
			XHTMLIMSerializer();

			virtual std::string serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<XHTMLIMPayload> xhtml)  const;
	};
}
