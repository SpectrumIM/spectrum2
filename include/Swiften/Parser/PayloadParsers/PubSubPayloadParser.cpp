/*
 * Copyright (c) 2012 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PubSubPayloadParser.h>

#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void PubSubPayloadParser::handleTree(ParserElement::ref root) {
	foreach (ParserElement::ref child, root->getAllChildren()) {
		getPayloadInternal()->addPayload(TreeReparser::parseTree(child, factories));
	}
}

}
