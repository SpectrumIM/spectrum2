/*
 * Copyright (c) 2012 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PubSubItemParser.h>

#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void PubSubItemParser::handleTree(ParserElement::ref root) {
	std::string id = root->getAttributes().getAttribute("id");
	if (!id.empty()) {
		getPayloadInternal()->setId(id);
	}

	foreach (ParserElement::ref child, root->getAllChildren()) {
		getPayloadInternal()->addPayload(TreeReparser::parseTree(child, factories));
	}
}

}
