/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#pragma once

#include "Swiften/Base/boost_bsignals.h"
#include <boost/shared_ptr.hpp>

#include "Swiften/Base/SafeByteArray.h"
#include "Swiften/TLS/Certificate.h"
#include "Swiften/TLS/CertificateVerificationError.h"

namespace Swift {
	class PKCS12Certificate;

	class TLSServerContext {
		public:
			virtual ~TLSServerContext();

			virtual void connect() = 0;

			virtual bool setServerCertificate(const PKCS12Certificate& cert) = 0;

			virtual void handleDataFromNetwork(const SafeByteArray&) = 0;
			virtual void handleDataFromApplication(const SafeByteArray&) = 0;

			virtual Certificate::ref getPeerCertificate() const = 0;
			virtual CertificateVerificationError::ref getPeerCertificateVerificationError() const = 0;

			virtual ByteArray getFinishMessage() const = 0;

		public:
			boost::signal<void (const SafeByteArray&)> onDataForNetwork;
			boost::signal<void (const SafeByteArray&)> onDataForApplication;
			boost::signal<void ()> onError;
			boost::signal<void ()> onConnected;
	};
}
