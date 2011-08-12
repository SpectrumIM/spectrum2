/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/InvisibleParser.h>

namespace Swift {

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
InvisibleParser::InvisibleParser() /*: level_(0)*/ {
}

void InvisibleParser::handleStartElement(const std::string& /*element*/, const std::string& /*ns*/, const AttributeMap& /*attributes*/) {
}

void InvisibleParser::handleEndElement(const std::string&, const std::string&) {
}

void InvisibleParser::handleCharacterData(const std::string&) {

}

}
