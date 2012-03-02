/*
 * Copyright (c) 2012 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/PubSubSubscriptionPayloadParser.h>

#include <boost/lexical_cast.hpp>

#include <Swiften/Parser/PayloadParserFactoryCollection.h>
#include <Swiften/Parser/PayloadParserFactory.h>
#include <Swiften/Base/foreach.h>
#include <Swiften/Elements/MUCOccupant.h>
#include <Swiften/Parser/Tree/TreeReparser.h>

namespace Swift {

void PubSubSubscriptionPayloadParser::handleTree(ParserElement::ref root) {
	std::string node = root->getAttributes().getAttribute("node");
	if (!node.empty()) {
		getPayloadInternal()->setNode(node);
	}

	std::string jid = root->getAttributes().getAttribute("jid");
	if (!jid.empty()) {
		getPayloadInternal()->setJID(jid);
	}

	std::string id = root->getAttributes().getAttribute("subid");
	if (!id.empty()) {
		getPayloadInternal()->setId(id);
	}

	std::string type = root->getAttributes().getAttribute("subscription");
	if (type == "none") {
		getPayloadInternal()->setType(PubSubSubscriptionPayload::None);
	}
	else if (type == "subscribed") {
		getPayloadInternal()->setType(PubSubSubscriptionPayload::Subscribed);
	}
	else if (type == "pending") {
		getPayloadInternal()->setType(PubSubSubscriptionPayload::Pending);
	}
	else if (type == "unconfigured") {
		getPayloadInternal()->setType(PubSubSubscriptionPayload::Unconfigured);
	}

}

}
