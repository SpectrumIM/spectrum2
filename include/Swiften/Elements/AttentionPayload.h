/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include <Swiften/Elements/Payload.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class AttentionPayload : public Payload {
		public:
			typedef SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<AttentionPayload> ref;

		public:
			AttentionPayload();
	};
}
