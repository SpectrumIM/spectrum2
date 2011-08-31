/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */
#include "Swiften/Base/Platform.h"

#ifdef SWIFTEN_PLATFORM_WINDOWS
#include <windows.h>
#include <wincrypt.h>
#endif

#include <vector>
#include <openssl/err.h>
#include <openssl/pkcs12.h>


#include "Swiften/TLS/OpenSSL/OpenSSLServerContext.h"
#include "Swiften/TLS/OpenSSL/OpenSSLCertificate.h"
#include "Swiften/TLS/PKCS12Certificate.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Swift {

static const int MAX_FINISHED_SIZE = 4096;
static const int SSL_READ_BUFFERSIZE = 8192;

static void freeX509Stack(STACK_OF(X509)* stack) {
	sk_X509_free(stack);
}

static int _sx_ssl_verify_callback(int preverify_ok, X509_STORE_CTX *ctx) {
	return 1;
}

OpenSSLServerContext::OpenSSLServerContext() : state_(Start), context_(0), handle_(0), readBIO_(0), writeBIO_(0) {
	ensureLibraryInitialized();
	context_ = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_set_verify(context_, SSL_VERIFY_PEER, _sx_ssl_verify_callback);

	// Load system certs
#if defined(SWIFTEN_PLATFORM_WINDOWS)
	X509_STORE* store = SSL_CTX_get_cert_store(context_);
	HCERTSTORE systemStore = CertOpenSystemStore(0, "ROOT");
	if (systemStore) {
		PCCERT_CONTEXT certContext = NULL;
		while (true) {
			certContext = CertFindCertificateInStore(systemStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, certContext);
			if (!certContext) {
				break;
			}
			ByteArray certData(certContext->pbCertEncoded, certContext->cbCertEncoded);
			OpenSSLCertificate cert(certData);
			if (store && cert.getInternalX509()) {
				X509_STORE_add_cert(store, cert.getInternalX509().get());
			}
		}
	}
#elif !defined(SWIFTEN_PLATFORM_MACOSX)
	SSL_CTX_load_verify_locations(context_, NULL, "/etc/ssl/certs");
#endif
}

OpenSSLServerContext::~OpenSSLServerContext() {
	SSL_free(handle_);
	SSL_CTX_free(context_);
}

void OpenSSLServerContext::ensureLibraryInitialized() {
	static bool isLibraryInitialized = false;
	if (!isLibraryInitialized) {
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();

		// Disable compression
		/*
		STACK_OF(SSL_COMP)* compressionMethods = SSL_COMP_get_compression_methods();
		sk_SSL_COMP_zero(compressionMethods);*/

		isLibraryInitialized = true;
	}
}

void OpenSSLServerContext::connect() {
	handle_ = SSL_new(context_);
	// Ownership of BIOs is ransferred
	readBIO_ = BIO_new(BIO_s_mem());
	writeBIO_ = BIO_new(BIO_s_mem());
	SSL_set_bio(handle_, readBIO_, writeBIO_);

	state_ = Connecting;
	doConnect();
}

void OpenSSLServerContext::doConnect() {
	int connectResult = SSL_accept(handle_);
	int error = SSL_get_error(handle_, connectResult);
// 	std::cout << "DO CONNECT\n";
	switch (error) {
		case SSL_ERROR_NONE: {
			if (SSL_is_init_finished(handle_)) {
// 				std::cout << "FINISHED\n";
				state_ = Connected;
				//std::cout << x->name << std::endl;
				//const char* comp = SSL_get_current_compression(handle_);
				//std::cout << "Compression: " << SSL_COMP_get_name(comp) << std::endl;
				onConnected();
				ERR_print_errors_fp(stdout);
				sendPendingDataToNetwork();
			}
			break;
		}
		case SSL_ERROR_WANT_READ:
			sendPendingDataToNetwork();
			break;
		default:
			state_ = Error;
// 			std::cout << "AAAAAAAA 1 " << error << " " << connectResult <<  "\n";
			ERR_print_errors_fp(stdout);
			onError();
	}
}

void OpenSSLServerContext::sendPendingDataToNetwork() {
	int size = BIO_pending(writeBIO_);
	if (size > 0) {
		SafeByteArray data;
		data.resize(size);
		BIO_read(writeBIO_, vecptr(data), size);
		onDataForNetwork(data);
	}
}

void OpenSSLServerContext::handleDataFromNetwork(const SafeByteArray& data) {
	BIO_write(readBIO_, vecptr(data), data.size());
// 	std::cout << "handleDataFromNetwork\n";
	switch (state_) {
		case Connecting:
			doConnect();
			break;
		case Connected:
			sendPendingDataToApplication();
			break;
		case Start: assert(false); break;
		case Error: /*assert(false);*/ break;
	}
}

void OpenSSLServerContext::handleDataFromApplication(const SafeByteArray& data) {
// 	std::cout << "SSL_WRITE\n";
	if (SSL_write(handle_, vecptr(data), data.size()) >= 0) {
		sendPendingDataToNetwork();
	}
	else {
		state_ = Error;
// 		std::cout << "AAAAAAAA 2\n";
		onError();
	}
}

void OpenSSLServerContext::sendPendingDataToApplication() {
	SafeByteArray data;
	data.resize(SSL_READ_BUFFERSIZE);
	int ret = SSL_read(handle_, vecptr(data), data.size());
	while (ret > 0) {
		data.resize(ret);
		onDataForApplication(data);
		data.resize(SSL_READ_BUFFERSIZE);
		ret = SSL_read(handle_, vecptr(data), data.size());
	}
	if (ret < 0 && SSL_get_error(handle_, ret) != SSL_ERROR_WANT_READ) {
		state_ = Error;
// 		std::cout << "AAAAAAAA 3\n";
		onError();
	}
}

bool OpenSSLServerContext::setServerCertificate(const PKCS12Certificate& certificate) {
	if (certificate.isNull()) {
// 		std::cout << "error 1\n";
		return false;
	}

	// Create a PKCS12 structure
	BIO* bio = BIO_new(BIO_s_mem());
	BIO_write(bio, vecptr(certificate.getData()), certificate.getData().size());
	boost::shared_ptr<PKCS12> pkcs12(d2i_PKCS12_bio(bio, NULL), PKCS12_free);
	BIO_free(bio);
	if (!pkcs12) {
// 		std::cout << "error 2\n";
		return false;
	}

	// Parse PKCS12
	X509 *certPtr = 0;
	EVP_PKEY* privateKeyPtr = 0;
	STACK_OF(X509)* caCertsPtr = 0;
	int result = PKCS12_parse(pkcs12.get(), reinterpret_cast<const char*>(vecptr(certificate.getPassword())), &privateKeyPtr, &certPtr, &caCertsPtr);
	if (result != 1) { 
// 		std::cout << "error 3\n";
		return false;
	}
	boost::shared_ptr<X509> cert(certPtr, X509_free);
	boost::shared_ptr<EVP_PKEY> privateKey(privateKeyPtr, EVP_PKEY_free);
	boost::shared_ptr<STACK_OF(X509)> caCerts(caCertsPtr, freeX509Stack);

	// Use the key & certificates
	if (SSL_CTX_use_certificate(context_, cert.get()) != 1) {
// 		std::cout << "error 4\n";
		return false;
	}
	if (SSL_CTX_use_PrivateKey(context_, privateKey.get()) != 1) {
// 		std::cout << "error 5\n";
		return false;
	}
	return true;
}

Certificate::ref OpenSSLServerContext::getPeerCertificate() const {
	boost::shared_ptr<X509> x509Cert(SSL_get_peer_certificate(handle_), X509_free);
	if (x509Cert) {
		return Certificate::ref(new OpenSSLCertificate(x509Cert));
	}
	else {
		return Certificate::ref();
	}
}

boost::shared_ptr<CertificateVerificationError> OpenSSLServerContext::getPeerCertificateVerificationError() const {
	int verifyResult = SSL_get_verify_result(handle_);
	if (verifyResult != X509_V_OK) {
		return boost::shared_ptr<CertificateVerificationError>(new CertificateVerificationError(getVerificationErrorTypeForResult(verifyResult)));
	}
	else {
		return boost::shared_ptr<CertificateVerificationError>();
	}
}

ByteArray OpenSSLServerContext::getFinishMessage() const {
	ByteArray data;
	data.resize(MAX_FINISHED_SIZE);
	size_t size = SSL_get_finished(handle_, vecptr(data), data.size());
	data.resize(size);
	return data;
}

CertificateVerificationError::Type OpenSSLServerContext::getVerificationErrorTypeForResult(int result) {
	assert(result != 0);
	switch (result) {
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			return CertificateVerificationError::NotYetValid;

		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			return CertificateVerificationError::Expired;

		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			return CertificateVerificationError::SelfSigned;

		case X509_V_ERR_CERT_UNTRUSTED:
			return CertificateVerificationError::Untrusted;

		case X509_V_ERR_CERT_REJECTED:
			return CertificateVerificationError::Rejected;

		case X509_V_ERR_INVALID_PURPOSE:
			return CertificateVerificationError::InvalidPurpose;

		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
			return CertificateVerificationError::PathLengthExceeded;

		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
			return CertificateVerificationError::InvalidSignature;

		case X509_V_ERR_INVALID_CA:
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
			return CertificateVerificationError::InvalidCA;

		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		case X509_V_ERR_AKID_SKID_MISMATCH:
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
			return CertificateVerificationError::UnknownError;

		// Unused / should not happen
		case X509_V_ERR_CERT_REVOKED:
		case X509_V_ERR_OUT_OF_MEM:
		case X509_V_ERR_UNABLE_TO_GET_CRL:
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		case X509_V_ERR_CRL_NOT_YET_VALID:
		case X509_V_ERR_CRL_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		case X509_V_ERR_APPLICATION_VERIFICATION:
		default:
			return CertificateVerificationError::UnknownError;
	}
}

}
