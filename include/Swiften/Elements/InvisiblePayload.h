/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>
#include <string>
#include <boost/signals2.hpp>
#include <Swiften/Elements/Payload.h>


// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
namespace Swift {
	class InvisiblePayload : public Payload {
		public:
			typedef std::shared_ptr<InvisiblePayload> ref;

		public:
			InvisiblePayload();
	};
}
