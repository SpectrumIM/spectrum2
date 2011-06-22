/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/AttentionParser.h>

namespace Swift {

AttentionParser::AttentionParser() /*: level_(0)*/ {
}

void AttentionParser::handleStartElement(const std::string& /*element*/, const std::string& /*ns*/, const AttributeMap& /*attributes*/) {
}

void AttentionParser::handleEndElement(const std::string&, const std::string&) {
}

void AttentionParser::handleCharacterData(const std::string&) {

}

}
