/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <Security/SecureTransport.h>
#include <Swiften/TLS/TLSError.h>
#include "Swiften/TLS/TLSServerContext.h"

namespace Swift {

class SecureTransportServerContext : public TLSServerContext {
	public:
		SecureTransportServerContext(bool checkCertificateRevocation);
		virtual ~SecureTransportServerContext();

		virtual void connect();

		virtual bool setClientCertificate(CertificateWithKey::ref cert);

		virtual void handleDataFromNetwork(const SafeByteArray&);
		virtual void handleDataFromApplication(const SafeByteArray&);

		virtual std::vector<Certificate::ref> getPeerCertificateChain() const;
		virtual CertificateVerificationError::ref getPeerCertificateVerificationError() const;

		virtual ByteArray getFinishMessage() const;
	
	private:
		static OSStatus SSLSocketReadCallback(SSLConnectionRef connection, void *data, size_t *dataLength); 
		static OSStatus SSLSocketWriteCallback(SSLConnectionRef connection, const void *data, size_t *dataLength);

	private:
		enum State { None, Handshake, HandshakeDone, Error};
		static std::string stateToString(State state);
		void setState(State newState);

		static boost::shared_ptr<TLSError> nativeToTLSError(OSStatus error);
		boost::shared_ptr<CertificateVerificationError> CSSMErrorToVerificationError(OSStatus resultCode);

		void processHandshake();
		void verifyServerCertificate();

		void fatalError(boost::shared_ptr<TLSError> error, boost::shared_ptr<CertificateVerificationError> certificateError);

	private:
		boost::shared_ptr<SSLContext> sslContext_;
		SafeByteArray readingBuffer_;
		State state_;
		CertificateVerificationError::ref verificationError_;
		CertificateWithKey::ref clientCertificate_;
		bool checkCertificateRevocation_;
};

}
