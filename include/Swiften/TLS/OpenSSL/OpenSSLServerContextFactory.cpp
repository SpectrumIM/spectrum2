/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/TLS/OpenSSL/OpenSSLServerContextFactory.h"
#include "Swiften/TLS/OpenSSL/OpenSSLServerContext.h"

namespace Swift {

bool OpenSSLServerContextFactory::canCreate() const {
	return true;
}

TLSServerContext* OpenSSLServerContextFactory::createTLSServerContext() {
	return new OpenSSLServerContext();
}

}
