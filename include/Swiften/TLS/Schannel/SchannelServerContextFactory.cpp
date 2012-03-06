/*
 * Copyright (c) 2011 Soren Dreijer
 * Licensed under the simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include "Swiften/TLS/Schannel/SchannelServerContextFactory.h"
#include "Swiften/TLS/Schannel/SchannelServerContext.h"

namespace Swift {

bool SchannelServerContextFactory::canCreate() const {
	return true;
}

TLSServerContext* SchannelServerContextFactory::createTLSServerContext() {
	return new SchannelServerContext();
}

}
