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

namespace Swift {
	class AttentionPayload : public Payload {
		public:
			typedef boost::shared_ptr<AttentionPayload> ref;

		public:
			AttentionPayload();
	};
}
