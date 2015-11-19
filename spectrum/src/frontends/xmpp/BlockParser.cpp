/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <BlockParser.h>

namespace Transport {

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
BlockParser::BlockParser() /*: level_(0)*/ {
}

void BlockParser::handleStartElement(const std::string& /*element*/, const std::string& /*ns*/, const Swift::AttributeMap& /*attributes*/) {
}

void BlockParser::handleEndElement(const std::string&, const std::string&) {
}

void BlockParser::handleCharacterData(const std::string&) {

}

}
