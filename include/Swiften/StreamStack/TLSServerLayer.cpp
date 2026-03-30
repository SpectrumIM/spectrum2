/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/StreamStack/TLSServerLayer.h"

#include <boost/bind.hpp>

#include "Swiften/TLS/TLSServerContextFactory.h"
#include "Swiften/TLS/TLSServerContext.h"

namespace Swift {

TLSServerLayer::TLSServerLayer(TLSServerContextFactory* factory) {
	context = factory->createTLSServerContext();
	context->onDataForNetwork.connect(boost::bind(&TLSServerLayer::writeDataToChildLayer, this, _1));
	context->onDataForApplication.connect(boost::bind(&TLSServerLayer::writeDataToParentLayer, this, _1));
	context->onConnected.connect(onConnected);
	context->onError.connect(onError);
}

TLSServerLayer::~TLSServerLayer() {
	delete context;
}

void TLSServerLayer::connect() {
	context->connect();
}

void TLSServerLayer::writeData(const SafeByteArray& data) {
	context->handleDataFromApplication(data);
}

void TLSServerLayer::handleDataRead(const SafeByteArray& data) {
	context->handleDataFromNetwork(data);
}

bool TLSServerLayer::setServerCertificate(CertificateWithKey::ref certificate) {
	return context->setServerCertificate(certificate);
}

Certificate::ref TLSServerLayer::getPeerCertificate() const {
	return context->getPeerCertificate();
}

std::shared_ptr<CertificateVerificationError> TLSServerLayer::getPeerCertificateVerificationError() const {
	return context->getPeerCertificateVerificationError();
}

}
