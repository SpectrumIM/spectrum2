/*
 * Copyright (c) 2012 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PubSubPublishPayloadParser.h>

#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void PubSubPublishPayloadParser::handleTree(ParserElement::ref root) {
	std::string node = root->getAttributes().getAttribute("node");
	if (!node.empty()) {
		getPayloadInternal()->setNode(node);
	}

	foreach (ParserElement::ref child, root->getAllChildren()) {
		getPayloadInternal()->addItem(TreeReparser::parseTree(child, factories));
	}
}

}
