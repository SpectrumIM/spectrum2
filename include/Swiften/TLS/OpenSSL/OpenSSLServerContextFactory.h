/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include "Swiften/TLS/TLSServerContextFactory.h"

namespace Swift {
	class OpenSSLServerContextFactory : public TLSServerContextFactory {
		public:
			bool canCreate() const;
			virtual TLSServerContext* createTLSServerContext();
	};
}
