/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#include <Swiften/TLS/SecureTransport/SecureTransportServerContext.h>

#include <boost/type_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <Swiften/Base/Algorithm.h>
#include <Swiften/Base/Log.h>
#include <Swiften/TLS/SecureTransport/SecureTransportCertificate.h>
#include <Swiften/TLS/PKCS12Certificate.h>
#include <Swiften/TLS/CertificateWithKey.h>

#include <Cocoa/Cocoa.h>

#import <Security/SecCertificate.h>
#import <Security/SecImportExport.h>

namespace {
	typedef boost::remove_pointer<CFArrayRef>::type CFArray;
	typedef boost::remove_pointer<SecTrustRef>::type SecTrust;
}

template <typename T, typename S>
T bridge_cast(S source) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
	return (__bridge T)(source);
#pragma clang diagnostic pop
}

namespace Swift {

namespace {

	
CFArrayRef CreateClientCertificateChainAsCFArrayRef(CertificateWithKey::ref key) {
	boost::shared_ptr<PKCS12Certificate> pkcs12 = boost::dynamic_pointer_cast<PKCS12Certificate>(key);
	if (!key) {
		return NULL;
	}

	SafeByteArray safePassword = pkcs12->getPassword();
	CFIndex passwordSize = 0;
	try {
		passwordSize = boost::numeric_cast<CFIndex>(safePassword.size());
	} catch (...) {
		return NULL;
	}

	CFMutableArrayRef certChain = CFArrayCreateMutable(NULL, 0, 0);

	OSStatus securityError = errSecSuccess;
	CFStringRef password = CFStringCreateWithBytes(kCFAllocatorDefault, safePassword.data(), passwordSize, kCFStringEncodingUTF8, false);
	const void* keys[] = { kSecImportExportPassphrase };
	const void* values[] = { password };

	CFDictionaryRef options = CFDictionaryCreate(NULL, keys, values, 1, NULL, NULL);

	CFArrayRef items = NULL;
	CFDataRef pkcs12Data = bridge_cast<CFDataRef>([NSData dataWithBytes: static_cast<const void *>(pkcs12->getData().data()) length:pkcs12->getData().size()]);
	securityError = SecPKCS12Import(pkcs12Data, options, &items);
	CFRelease(options);
	NSArray* nsItems = bridge_cast<NSArray*>(items);

	switch(securityError) {
		case errSecSuccess:
			break;
		case errSecAuthFailed:
			// Password did not work for decoding the certificate.
			SWIFT_LOG(warning) << "Invalid password." << std::endl;
			break;
		case errSecDecode:
			// Other decoding error.
			SWIFT_LOG(warning) << "PKCS12 decoding error." << std::endl;
			break;
		default:
			SWIFT_LOG(warning) << "Unknown error." << std::endl;
	}
	
	if (securityError != errSecSuccess) {
		if (items) {
			CFRelease(items);
			items = NULL;
		}
		CFRelease(certChain);
		certChain = NULL;
	}

	if (certChain) {
		CFArrayAppendValue(certChain, nsItems[0][@"identity"]);

		for (CFIndex index = 0; index < CFArrayGetCount(bridge_cast<CFArrayRef>(nsItems[0][@"chain"])); index++) {
			CFArrayAppendValue(certChain, CFArrayGetValueAtIndex(bridge_cast<CFArrayRef>(nsItems[0][@"chain"]), index));
		}
	}
	return certChain;
}

}

SecureTransportContext::SecureTransportServerContext(bool checkCertificateRevocation) : state_(None), checkCertificateRevocation_(checkCertificateRevocation) {
	sslContext_ = boost::shared_ptr<SSLContext>(SSLCreateContext(NULL, kSSLClientSide, kSSLStreamType), CFRelease);

	OSStatus error = noErr;
	// set IO callbacks
	error = SSLSetIOFuncs(sslContext_.get(), &SecureTransportContext::SSLSocketReadCallback, &SecureTransportContext::SSLSocketWriteCallback);
	if (error != noErr) {
		SWIFT_LOG(error) << "Unable to set IO functions to SSL context." << std::endl;
		sslContext_.reset();
	}

	error = SSLSetConnection(sslContext_.get(), this);
	if (error != noErr) {
		SWIFT_LOG(error) << "Unable to set connection to SSL context." << std::endl;
		sslContext_.reset();
	}


	error = SSLSetSessionOption(sslContext_.get(), kSSLSessionOptionBreakOnServerAuth, true);
	if (error != noErr) {
		SWIFT_LOG(error) << "Unable to set kSSLSessionOptionBreakOnServerAuth on session." << std::endl;
		sslContext_.reset();
	}
}

SecureTransportServerContext::~SecureTransportServerContext() {
	if (sslContext_) {
		SSLClose(sslContext_.get());
	}
}

std::string SecureTransportContext::stateToString(State state) {
	std::string returnValue;
	switch(state) {
		case Handshake:
			returnValue = "Handshake";
			break;
		case HandshakeDone:
			returnValue = "HandshakeDone";
			break;
		case None:
			returnValue = "None";
			break;
		case Error:
			returnValue = "Error";
			break;
	}
	return returnValue;
}

void SecureTransportServerContext::setState(State newState) {
	SWIFT_LOG(debug) << "Switch state from " << stateToString(state_) << " to " << stateToString(newState) << "." << std::endl;
	state_ = newState;
}

void SecureTransportServerContext::connect() {
	SWIFT_LOG_ASSERT(state_ == None, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;
	if (clientCertificate_) {
		CFArrayRef certs = CreateClientCertificateChainAsCFArrayRef(clientCertificate_);
		if (certs) {
			boost::shared_ptr<CFArray> certRefs(certs, CFRelease);
			OSStatus result = SSLSetCertificate(sslContext_.get(), certRefs.get());
			if (result != noErr) {
				SWIFT_LOG(error) << "SSLSetCertificate failed with error " << result << "." << std::endl;
			}
		}
	}
	processHandshake();
}

void SecureTransportServerContext::processHandshake() {
	SWIFT_LOG_ASSERT(state_ == None || state_ == Handshake, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;
	OSStatus error = SSLHandshake(sslContext_.get());
	if (error == errSSLWouldBlock) {
		setState(Handshake);
	}
	else if (error == noErr) {
		SWIFT_LOG(debug) << "TLS handshake successful." << std::endl;
		setState(HandshakeDone);
		onConnected();
	}
	else if (error == errSSLPeerAuthCompleted) {
		SWIFT_LOG(debug) << "Received server certificate. Start verification." << std::endl;
		setState(Handshake);
		verifyServerCertificate();
	}
	else {
		SWIFT_LOG(debug) << "Error returned from SSLHandshake call is " << error << "." << std::endl;
		fatalError(nativeToTLSError(error), boost::make_shared<CertificateVerificationError>());
	}
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void SecureTransportServerContext::verifyServerCertificate() {
	SecTrustRef trust = NULL;
	OSStatus error = SSLCopyPeerTrust(sslContext_.get(), &trust);
	if (error != noErr) {
		fatalError(boost::make_shared<TLSError>(), boost::make_shared<CertificateVerificationError>());
		return;
	}
	boost::shared_ptr<SecTrust> trustRef = boost::shared_ptr<SecTrust>(trust, CFRelease);

	if (checkCertificateRevocation_) {
		error = SecTrustSetOptions(trust, kSecTrustOptionRequireRevPerCert | kSecTrustOptionFetchIssuerFromNet);
		if (error != noErr) {
			fatalError(boost::make_shared<TLSError>(), boost::make_shared<CertificateVerificationError>());
			return;
		}
	}

	SecTrustResultType trustResult;
	error = SecTrustEvaluate(trust, &trustResult);
	if (error != errSecSuccess) {
		fatalError(boost::make_shared<TLSError>(), boost::make_shared<CertificateVerificationError>());
		return;
	}

	OSStatus cssmResult = 0;
	switch(trustResult) {
		case kSecTrustResultUnspecified:
			SWIFT_LOG(warning) << "Successful implicit validation. Result unspecified." << std::endl;
			break;
		case kSecTrustResultProceed:
			SWIFT_LOG(warning) << "Validation resulted in explicitly trusted." << std::endl;
			break;
		case kSecTrustResultRecoverableTrustFailure:
			SWIFT_LOG(warning) << "recoverable trust failure" << std::endl;
			error = SecTrustGetCssmResultCode(trust, &cssmResult);
			if (error == errSecSuccess) {
				verificationError_ = CSSMErrorToVerificationError(cssmResult);
				if (cssmResult == CSSMERR_TP_VERIFY_ACTION_FAILED || cssmResult == CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK ) {
					// Find out the reason why the verification failed.
					CFArrayRef certChain;
					CSSM_TP_APPLE_EVIDENCE_INFO* statusChain;
					error = SecTrustGetResult(trustRef.get(), &trustResult, &certChain, &statusChain);
					if (error == errSecSuccess) {
						boost::shared_ptr<CFArray> certChainRef = boost::shared_ptr<CFArray>(certChain, CFRelease);
						for (CFIndex index = 0; index < CFArrayGetCount(certChainRef.get()); index++) {
							for (CFIndex n = 0; n < statusChain[index].NumStatusCodes; n++) {
								// Even though Secure Transport reported CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK on the whole certificate
								// chain, the actual cause can be that a revocation check for a specific cert returned CSSMERR_TP_CERT_REVOKED.
								if (!verificationError_ || verificationError_->getType() == CertificateVerificationError::RevocationCheckFailed) {
									verificationError_ = CSSMErrorToVerificationError(statusChain[index].StatusCodes[n]);
								}
							}
						}
					}
					else {

					}
				}
			}
			else {
				verificationError_ = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
			}
			break;
		case kSecTrustResultOtherError:
			verificationError_ = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
			break;
		default:
			SWIFT_LOG(warning) << "Unhandled trust result " << trustResult << "." << std::endl;
			break;
	}
	
	if (verificationError_) {
		setState(Error);
		SSLClose(sslContext_.get());
		sslContext_.reset();
		onError(boost::make_shared<TLSError>());
	}
	else {
		// proceed with handshake
		processHandshake();
	}
}

#pragma clang diagnostic pop

bool SecureTransportServerContext::setClientCertificate(CertificateWithKey::ref cert) {
	CFArrayRef nativeClientChain = CreateClientCertificateChainAsCFArrayRef(cert);
	if (nativeClientChain) {
		clientCertificate_ = cert;
		CFRelease(nativeClientChain);
		return true;
	}
	else {
		return false;
	}
}

void SecureTransportServerContext::handleDataFromNetwork(const SafeByteArray& data) {
	SWIFT_LOG(debug) << std::endl;
	SWIFT_LOG_ASSERT(state_ == HandshakeDone || state_ == Handshake, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;

	append(readingBuffer_, data);

	size_t bytesRead = 0;
	OSStatus error = noErr;
	SafeByteArray applicationData;

	switch(state_) {
		case None:
			assert(false && "Invalid state 'None'.");
			break;
		case Handshake:
			processHandshake();
			break;
		case HandshakeDone:
			while (error == noErr) {
				applicationData.resize(readingBuffer_.size());
				error = SSLRead(sslContext_.get(), applicationData.data(), applicationData.size(), &bytesRead);
				if (error == noErr) {
					// Read successful.
				}
				else if (error == errSSLWouldBlock) {
					// Secure Transport does not want more data.
					break;
				}
				else {
					SWIFT_LOG(error) << "SSLRead failed with error " << error << ", read bytes: " << bytesRead << "." << std::endl;
					fatalError(boost::make_shared<TLSError>(), boost::make_shared<CertificateVerificationError>());
					return;
				}

				if (bytesRead > 0) {
					applicationData.resize(bytesRead);
					onDataForApplication(applicationData);
				}
				else {
					break;	
				}
			}
			break;
		case Error:
			SWIFT_LOG(debug) << "Igoring received data in error state." << std::endl;
			break;
	}
}


void SecureTransportServerContext::handleDataFromApplication(const SafeByteArray& data) {
	size_t processedBytes = 0;
	OSStatus error = SSLWrite(sslContext_.get(), data.data(), data.size(), &processedBytes);
	switch(error) {
		case errSSLWouldBlock:
			SWIFT_LOG(warning) << "Unexpected because the write callback does not block." << std::endl;
			return;
		case errSSLClosedGraceful:
		case noErr:
			return;
		default:
			SWIFT_LOG(warning) << "SSLWrite returned error code: " << error << ", processed bytes: " << processedBytes << std::endl;
			fatalError(boost::make_shared<TLSError>(), boost::shared_ptr<CertificateVerificationError>());
	}
}

std::vector<Certificate::ref> SecureTransportServerContext::getPeerCertificateChain() const {
	std::vector<Certificate::ref> peerCertificateChain;

	if (sslContext_) {
			typedef boost::remove_pointer<SecTrustRef>::type SecTrust;
			boost::shared_ptr<SecTrust> securityTrust;

			SecTrustRef secTrust = NULL;;
			OSStatus error = SSLCopyPeerTrust(sslContext_.get(), &secTrust);
			if (error == noErr) {
				securityTrust = boost::shared_ptr<SecTrust>(secTrust, CFRelease);

				CFIndex chainSize = SecTrustGetCertificateCount(securityTrust.get());
				for (CFIndex n = 0; n < chainSize; n++) {
					SecCertificateRef certificate = SecTrustGetCertificateAtIndex(securityTrust.get(), n);
					if (certificate) {
						peerCertificateChain.push_back(boost::make_shared<SecureTransportCertificate>(certificate));
					}
				}
			}
			else {
				SWIFT_LOG(warning) << "Failed to obtain peer trust structure; error = " << error << "." << std::endl;
			}
	}

	return peerCertificateChain;
}

CertificateVerificationError::ref SecureTransportServerContext::getPeerCertificateVerificationError() const {
	return verificationError_;
}

ByteArray SecureTransportServerContext::getFinishMessage() const {
	SWIFT_LOG(warning) << "Access to TLS handshake finish message is not part of OS X Secure Transport APIs." << std::endl;
	return ByteArray();
}

/**
 *	This I/O callback simulates an asynchronous read to the read buffer of the context. If it is empty, it returns errSSLWouldBlock; else
 *  the data within the buffer is returned.
 */
OSStatus SecureTransportServerContext::SSLSocketReadCallback(SSLConnectionRef connection, void *data, size_t *dataLength) {
	SecureTransportContext* context = const_cast<SecureTransportContext*>(static_cast<const SecureTransportContext*>(connection));
	OSStatus retValue = noErr;

	if (context->readingBuffer_.size() < *dataLength) {
		// Would block because Secure Transport is trying to read more data than there currently is available in the buffer.
		*dataLength = 0;
		retValue = errSSLWouldBlock;
	}
	else {
		size_t bufferLen = *dataLength;
		size_t copyToBuffer = bufferLen < context->readingBuffer_.size() ? bufferLen : context->readingBuffer_.size();

		memcpy(data, context->readingBuffer_.data(), copyToBuffer);
			
		context->readingBuffer_ = SafeByteArray(context->readingBuffer_.data() + copyToBuffer, context->readingBuffer_.data() + context->readingBuffer_.size());
		*dataLength = copyToBuffer;
	}
	return retValue;
}

OSStatus SecureTransportServerContext::SSLSocketWriteCallback(SSLConnectionRef connection, const void *data, size_t *dataLength) {
	SecureTransportContext* context = const_cast<SecureTransportContext*>(static_cast<const SecureTransportContext*>(connection));
	OSStatus retValue = noErr;
	
	SafeByteArray safeData;
	safeData.resize(*dataLength);
	memcpy(safeData.data(), data, safeData.size());
	
	context->onDataForNetwork(safeData);
	return retValue;
}

boost::shared_ptr<TLSError> SecureTransportServerContext::nativeToTLSError(OSStatus /* error */) {
	boost::shared_ptr<TLSError> swiftenError;
	swiftenError = boost::make_shared<TLSError>();
	return swiftenError;
}

boost::shared_ptr<CertificateVerificationError> SecureTransportServerContext::CSSMErrorToVerificationError(OSStatus resultCode) {
	boost::shared_ptr<CertificateVerificationError> error;
	switch(resultCode) {
		case CSSMERR_TP_NOT_TRUSTED:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_NOT_TRUSTED" << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::Untrusted);
			break;
		case CSSMERR_TP_CERT_NOT_VALID_YET:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_NOT_VALID_YET" << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::NotYetValid);
			break;
		case CSSMERR_TP_CERT_EXPIRED:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_EXPIRED" << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::Expired);
			break;
		case CSSMERR_TP_CERT_REVOKED:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_REVOKED" << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::Revoked);
			break;
		case CSSMERR_TP_VERIFY_ACTION_FAILED:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_VERIFY_ACTION_FAILED" << std::endl;
			break;
		case CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK" << std::endl;
			if (checkCertificateRevocation_) {
				error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::RevocationCheckFailed);
			}
			break;
		case CSSMERR_APPLETP_OCSP_UNAVAILABLE:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_OCSP_UNAVAILABLE" << std::endl;
			if (checkCertificateRevocation_) {
				error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::RevocationCheckFailed);
			}
			break;
		case CSSMERR_APPLETP_SSL_BAD_EXT_KEY_USE:
			SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_SSL_BAD_EXT_KEY_USE" << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::InvalidPurpose);
			break;
		default:
			SWIFT_LOG(warning) << "unhandled CSSM error: " << resultCode << ", CSSM_TP_BASE_TP_ERROR: " << CSSM_TP_BASE_TP_ERROR << std::endl;
			error = boost::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
			break;
	}
	return error;
}

void SecureTransportServerContext::fatalError(boost::shared_ptr<TLSError> error, boost::shared_ptr<CertificateVerificationError> certificateError) {
	setState(Error);
	if (sslContext_) {
		SSLClose(sslContext_.get());
	}
	verificationError_ = certificateError;
	onError(error);
}

}
