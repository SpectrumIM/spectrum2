/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>

#include <string>
#include <Swiften/Elements/Payload.h>

namespace Swift {
	class SpectrumErrorPayload : public Payload {
		public:
			enum Error {
				CONNECTION_ERROR_NETWORK_ERROR = 0,
				CONNECTION_ERROR_INVALID_USERNAME = 1,
				CONNECTION_ERROR_AUTHENTICATION_FAILED = 2,
				CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE = 3,
				CONNECTION_ERROR_NO_SSL_SUPPORT = 4,
				CONNECTION_ERROR_ENCRYPTION_ERROR = 5,
				CONNECTION_ERROR_NAME_IN_USE = 6,
				CONNECTION_ERROR_INVALID_SETTINGS = 7,
				CONNECTION_ERROR_CERT_NOT_PROVIDED = 8,
				CONNECTION_ERROR_CERT_UNTRUSTED = 9,
				CONNECTION_ERROR_CERT_EXPIRED = 10,
				CONNECTION_ERROR_CERT_NOT_ACTIVATED = 11,
				CONNECTION_ERROR_CERT_HOSTNAME_MISMATCH = 12,
				CONNECTION_ERROR_CERT_FINGERPRINT_MISMATCH = 13,
				CONNECTION_ERROR_CERT_SELF_SIGNED = 14,
				CONNECTION_ERROR_CERT_OTHER_ERROR = 15,
				CONNECTION_ERROR_OTHER_ERROR = 16,
			};

			SpectrumErrorPayload(Error error = CONNECTION_ERROR_OTHER_ERROR);

			Error getError() const {
				return error_; 
			}

			void setError(Error error) {
				error_ = error;
			}

		private:
			Error error_;
	};
}
