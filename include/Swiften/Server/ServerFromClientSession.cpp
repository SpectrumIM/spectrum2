/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Server/ServerFromClientSession.h>

#include <boost/bind.hpp>

#include <Swiften/Elements/ProtocolHeader.h>
#include <Swiften/Server/UserRegistry.h>
#include <Swiften/Network/Connection.h>
#include <Swiften/StreamStack/XMPPLayer.h>
#include <Swiften/Elements/StreamFeatures.h>
#include <Swiften/Elements/ResourceBind.h>
#include <Swiften/Elements/StartSession.h>
#include <Swiften/Elements/IQ.h>
#include <Swiften/Elements/AuthSuccess.h>
#include <Swiften/Elements/AuthFailure.h>
#include <Swiften/Elements/AuthRequest.h>
#include <Swiften/SASL/PLAINMessage.h>
#include <Swiften/StreamStack/StreamStack.h>
#include <Swiften/StreamStack/TLSServerLayer.h>
#include <Swiften/Elements/StartTLSRequest.h>
#include <Swiften/Elements/TLSProceed.h>

namespace Swift {

ServerFromClientSession::ServerFromClientSession(
		const std::string& id,
		boost::shared_ptr<Connection> connection, 
		PayloadParserFactoryCollection* payloadParserFactories, 
		PayloadSerializerCollection* payloadSerializers,
		UserRegistry* userRegistry) : 
			Session(connection, payloadParserFactories, payloadSerializers),
			id_(id),
			userRegistry_(userRegistry),
			authenticated_(false),
			initialized(false),
			allowSASLEXTERNAL(false),
			tlsLayer(0),
			tlsConnected(false) {
	userRegistry->onPasswordValid.connect(boost::bind(&ServerFromClientSession::handlePasswordValid, this, _1));
	userRegistry->onPasswordInvalid.connect(boost::bind(&ServerFromClientSession::handlePasswordInvalid, this, _1));
}

ServerFromClientSession::~ServerFromClientSession() {
	if (tlsLayer) {
		delete tlsLayer;
	}
}

void ServerFromClientSession::handlePasswordValid(const std::string &user) {
	if (user != JID(user_, getLocalJID().getDomain()).toString())
		return;
	if (!isInitialized()) {
		getXMPPLayer()->writeElement(boost::shared_ptr<AuthSuccess>(new AuthSuccess()));
		authenticated_ = true;
		getXMPPLayer()->resetParser();
	}
}

void ServerFromClientSession::handlePasswordInvalid(const std::string &user) {
	if (user != JID(user_, getLocalJID().getDomain()).toString() || authenticated_)
		return;
	if (!isInitialized()) {
		getXMPPLayer()->writeElement(boost::shared_ptr<AuthFailure>(new AuthFailure));
		finishSession(AuthenticationFailedError);
	}
}

void ServerFromClientSession::handleElement(boost::shared_ptr<Element> element) {
	if (isInitialized()) {
		onElementReceived(element);
	}
	else {
		if (AuthRequest* authRequest = dynamic_cast<AuthRequest*>(element.get())) {
			if (authRequest->getMechanism() == "PLAIN" || (allowSASLEXTERNAL && authRequest->getMechanism() == "EXTERNAL")) {
				if (authRequest->getMechanism() == "EXTERNAL") {
						getXMPPLayer()->writeElement(boost::shared_ptr<AuthSuccess>(new AuthSuccess()));
						authenticated_ = true;
						getXMPPLayer()->resetParser();
				}
				else {
					PLAINMessage plainMessage(authRequest->getMessage() ? *authRequest->getMessage() : createSafeByteArray(""));
					user_ = plainMessage.getAuthenticationID();
					if (userRegistry_->isValidUserPassword(JID(plainMessage.getAuthenticationID(), getLocalJID().getDomain()), plainMessage.getPassword())) {
						// we're waiting for usermanager signal now
// 						authenticated_ = true;
// 						getXMPPLayer()->resetParser();
					}
					else {
						getXMPPLayer()->writeElement(boost::shared_ptr<AuthFailure>(new AuthFailure));
						finishSession(AuthenticationFailedError);
					}
				}
			}
			else {
				getXMPPLayer()->writeElement(boost::shared_ptr<AuthFailure>(new AuthFailure));
				finishSession(NoSupportedAuthMechanismsError);
			}
		}
		else if (dynamic_cast<StartTLSRequest*>(element.get()) != NULL) {
			getXMPPLayer()->writeElement(boost::shared_ptr<TLSProceed>(new TLSProceed));
			getStreamStack()->addLayer(tlsLayer);
			tlsLayer->connect();
			getXMPPLayer()->resetParser();
		}
		else if (IQ* iq = dynamic_cast<IQ*>(element.get())) {
			if (boost::shared_ptr<ResourceBind> resourceBind = iq->getPayload<ResourceBind>()) {
				std::string bucket = "abcdefghijklmnopqrstuvwxyz";
				std::string uuid;
				for (int i = 0; i < 10; i++) {
					uuid += bucket[rand() % bucket.size()];
				}
				setRemoteJID(JID(user_, getLocalJID().getDomain(), uuid));
				boost::shared_ptr<ResourceBind> resultResourceBind(new ResourceBind());
				resultResourceBind->setJID(getRemoteJID());
				getXMPPLayer()->writeElement(IQ::createResult(JID(), iq->getID(), resultResourceBind));
			}
			else if (iq->getPayload<StartSession>()) {
				getXMPPLayer()->writeElement(IQ::createResult(getRemoteJID(), iq->getID()));
				setInitialized();
			}
		}
	}
}

void ServerFromClientSession::handleStreamStart(const ProtocolHeader& incomingHeader) {
	setLocalJID(JID("", incomingHeader.getTo()));
	ProtocolHeader header;
	header.setFrom(incomingHeader.getTo());
	header.setID(id_);
	getXMPPLayer()->writeHeader(header);

	boost::shared_ptr<StreamFeatures> features(new StreamFeatures());

	if (!authenticated_) {
		if (tlsLayer && !tlsConnected) {
			features->setHasStartTLS();
		}
		features->addAuthenticationMechanism("PLAIN");
		if (allowSASLEXTERNAL) {
			features->addAuthenticationMechanism("EXTERNAL");
		}
	}
	else {
		features->setHasResourceBind();
		features->setHasSession();
	}
	getXMPPLayer()->writeElement(features);
}

void ServerFromClientSession::setInitialized() {
	initialized = true;
	onSessionStarted();
}

void ServerFromClientSession::setAllowSASLEXTERNAL() {
	allowSASLEXTERNAL = true;
}

void ServerFromClientSession::addTLSEncryption(TLSServerContextFactory* tlsContextFactory, const PKCS12Certificate& cert) {
	tlsLayer = new TLSServerLayer(tlsContextFactory);
	if (!tlsLayer->setServerCertificate(cert)) {
// 		std::cout << "error\n";
		// TODO:
// 		onClosed(boost::shared_ptr<Error>(new Error(Error::InvalidTLSCertificateError)));
	}
	else {
		tlsLayer->onError.connect(boost::bind(&ServerFromClientSession::handleTLSError, this));
		tlsLayer->onConnected.connect(boost::bind(&ServerFromClientSession::handleTLSConnected, this));
// 		getStreamStack()->addLayer(tlsLayer);
// 		tlsLayer->onError.connect(boost::bind(&BasicSessionStream::handleTLSError, this));
// 		tlsLayer->onConnected.connect(boost::bind(&BasicSessionStream::handleTLSConnected, this));
// 		tlsLayer->connect();
	}
}

}
