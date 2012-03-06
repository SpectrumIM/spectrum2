/*
 * Copyright (c) 2012 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PubSubSubscribePayloadParser.h>

#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void PubSubSubscribePayloadParser::handleTree(ParserElement::ref root) {
	std::string node = root->getAttributes().getAttribute("node");
	if (!node.empty()) {
		getPayloadInternal()->setNode(node);
	}

	std::string jid = root->getAttributes().getAttribute("jid");
	if (!jid.empty()) {
		getPayloadInternal()->setJID(jid);
	}

}

}
