#include "carbonresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/ErrorPayload.h"
#include "transport/Logging.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(carbonResponderLogger, "CarbonResponder");

CarbonResponder::CarbonResponder(Swift::IQRouter *router)
	: Swift::SetResponder<Swift::CarbonsEnable>(router),
	Swift::SetResponder<Swift::CarbonsDisable>(router)
{
	LOG4CXX_TRACE(carbonResponderLogger, "Initialized");
}

CarbonResponder::~CarbonResponder() {
}

void CarbonResponder::start() {
	LOG4CXX_TRACE(carbonResponderLogger, "start()");
	CarbonsEnableResponder::start();
	CarbonsDisableResponder::start();
}

//Call when DiscoInfoResponder is available
void CarbonResponder::setDiscoInfoResponder(DiscoInfoResponder *discoInfoResponder) {
	LOG4CXX_TRACE(carbonResponderLogger, "Registering disco#info features");
	discoInfoResponder->addTransportFeature("urn:xmpp:carbons:2");
}

bool CarbonResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CarbonsEnable> payload) {
	LOG4CXX_TRACE(carbonResponderLogger, "handle(CarbonsEnable) -> Confirm");
	//Always confirm
	CarbonsEnableResponder::getIQRouter()->sendIQ(Swift::IQ::createResult(from, to, id));
	return true;
}

bool CarbonResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CarbonsDisable> payload) {
	LOG4CXX_TRACE(carbonResponderLogger, "handle(CarbonsDisable) -> NotImplemented");
	//Never allow
	CarbonsDisableResponder::sendError(from, id, Swift::ErrorPayload::FeatureNotImplemented, Swift::ErrorPayload::Cancel);
	return true;
}

}
