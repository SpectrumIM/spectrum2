/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include "Swiften/Base/SafeByteArray.h"
#include "Swiften/TLS/Certificate.h"
#include <Swiften/TLS/CertificateWithKey.h>
#include "Swiften/TLS/CertificateVerificationError.h"
#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class PKCS12Certificate;

	class TLSServerContext {
		public:
			virtual ~TLSServerContext();

			virtual void connect() = 0;

			virtual bool setServerCertificate(CertificateWithKey::ref cert) = 0;

			virtual void handleDataFromNetwork(const SafeByteArray&) = 0;
			virtual void handleDataFromApplication(const SafeByteArray&) = 0;

			virtual Certificate::ref getPeerCertificate() const = 0;
			virtual CertificateVerificationError::ref getPeerCertificateVerificationError() const = 0;

			virtual ByteArray getFinishMessage() const = 0;

		public:
			SWIFTEN_SIGNAL_NAMESPACE::signal<void (const SafeByteArray&)> onDataForNetwork;
			SWIFTEN_SIGNAL_NAMESPACE::signal<void (const SafeByteArray&)> onDataForApplication;
			SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onError;
			SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onConnected;
	};
}
