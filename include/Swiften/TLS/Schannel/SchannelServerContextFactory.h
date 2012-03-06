/*
 * Copyright (c) 2011 Soren Dreijer
 * Licensed under the simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include "Swiften/TLS/TLSServerContextFactory.h"

namespace Swift {
	class SchannelServerContextFactory : public TLSServerContextFactory {
		public:
			bool canCreate() const;
			virtual TLSServerContext* createTLSServerContext();
	};
}
