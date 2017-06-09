/*
 * Copyright (c) 2010 Kevin Smith
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/MUCPayloadParser.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void MUCPayloadParser::handleTree(ParserElement::ref root) {
	BOOST_FOREACH (ParserElement::ref child, root->getAllChildren()) {
		if (child->getName() == "password" && child->getNamespace() == root->getNamespace()) {
			getPayloadInternal()->setPassword(child->getText());
		}
	}
}

}
