/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PrivilegeParser.h>
#include <Swiften/Parser/PayloadParsers/ForwardedParser.h>

namespace Swift {

PrivilegeParser::PrivilegeParser(PayloadParserFactoryCollection* factories) : factories_(factories), level_(TopLevel) {
}

void PrivilegeParser::handleStartElement(const std::string& element, const std::string& ns, const AttributeMap& attributes) {
	if (level_ == PayloadLevel) {
		if (element == "forwarded" && ns == "urn:xmpp:forward:0") {
			childParser_ = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<PayloadParser>(SWIFTEN_SHRPTR_NAMESPACE::make_shared<ForwardedParser>(factories_));
		};
	}
	if (childParser_) {
		childParser_->handleStartElement(element, ns, attributes);
	}
	++level_;
}

void PrivilegeParser::handleEndElement(const std::string& element, const std::string& ns) {
	--level_;
	if (childParser_ && level_ >= PayloadLevel) {
		childParser_->handleEndElement(element, ns);
	}
	if (childParser_ && level_ == PayloadLevel) {
		getPayloadInternal()->setForwarded(SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Forwarded>(childParser_->getPayload()));
		childParser_.reset();
	}
}

void PrivilegeParser::handleCharacterData(const std::string& data) {
	if (childParser_) {
		childParser_->handleCharacterData(data);
	}
}

}
