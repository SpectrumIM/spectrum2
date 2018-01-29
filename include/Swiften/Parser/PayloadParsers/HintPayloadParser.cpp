/*
 * Implements XEP-0334: Message Processing Hints
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/HintPayloadParser.h>

namespace Swift {

HintPayloadParser::HintPayloadParser() : level_(0) {
}

void HintPayloadParser::handleStartElement(const std::string& element, const std::string& /*ns*/, const AttributeMap& /*attributes*/) {
	if (level_ == 0) {
		HintPayload::Type type = HintPayload::NoCopy;
		if (element == "no-permanent-store") {
			type = HintPayload::NoPermanentStore;
		} else if (element == "no-store") {
			type = HintPayload::NoStore;
		} else if (element == "no-copy") {
			type = HintPayload::NoCopy;
		} else if (element == "store") {
			type = HintPayload::Store;
		}
		getPayloadInternal()->setType(type);
	}
	++level_;
}

void HintPayloadParser::handleEndElement(const std::string&, const std::string&) {
	--level_;
}

void HintPayloadParser::handleCharacterData(const std::string&) {

}

}
