/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/StatsParser.h>
#include <Swiften/Parser/SerializingParser.h>

namespace Swift {

StatsParser::StatsParser() : level_(TopLevel), inItem_(false) {
}

void StatsParser::handleStartElement(const std::string& element, const std::string& /*ns*/, const AttributeMap& attributes) {
	if (level_ == PayloadLevel) {
		if (element == "item") {
			inItem_ = true;

			currentItem_ = StatsPayload::Item();

			currentItem_.setName(attributes.getAttribute("name"));
			currentItem_.setValue(attributes.getAttribute("value"));
			currentItem_.setUnits(attributes.getAttribute("units"));
		}
	}
	++level_;
}

void StatsParser::handleEndElement(const std::string& element, const std::string& /*ns*/) {
	--level_;
	if (level_ == PayloadLevel) {
		if (inItem_) {
			getPayloadInternal()->addItem(currentItem_);
			inItem_ = false;
		}
	}
}

void StatsParser::handleCharacterData(const std::string& data) {
}

}
