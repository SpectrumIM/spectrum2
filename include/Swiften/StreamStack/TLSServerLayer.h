/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/Base/boost_bsignals.h"

#include "Swiften/Base/SafeByteArray.h"
#include "Swiften/StreamStack/StreamLayer.h"
#include "Swiften/TLS/Certificate.h"
#include <Swiften/TLS/CertificateWithKey.h>
#include "Swiften/TLS/CertificateVerificationError.h"

namespace Swift {
	class TLSServerContext;
	class TLSServerContextFactory;
	class PKCS12Certificate;

	class TLSServerLayer : public StreamLayer {
		public:
			TLSServerLayer(TLSServerContextFactory*);
			~TLSServerLayer();

			void connect();
			bool setServerCertificate(CertificateWithKey::ref cert);

			Certificate::ref getPeerCertificate() const;
			boost::shared_ptr<CertificateVerificationError> getPeerCertificateVerificationError() const;

			void writeData(const SafeByteArray& data);
			void handleDataRead(const SafeByteArray& data);

			TLSServerContext* getContext() const {
				return context;
			}

		public:
			boost::signal<void ()> onError;
			boost::signal<void ()> onConnected;

		private:
			TLSServerContext* context;
	};
}
