/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/StatsSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {

StatsSerializer::StatsSerializer() : GenericPayloadSerializer<StatsPayload>() {
}

std::string StatsSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<StatsPayload> stats)  const {
	XMLElement queryElement("query", "http://jabber.org/protocol/stats");
	for(const StatsPayload::Item& item: stats->getItems()) {
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<XMLElement> statElement(new XMLElement("stat"));
		statElement->setAttribute("name", item.getName());
		if (!item.getUnits().empty()) {
			statElement->setAttribute("units", item.getUnits());
		}
		if (!item.getValue().empty()) {
			statElement->setAttribute("value", item.getValue());
		}

		queryElement.addNode(statElement);
	}

	return queryElement.serialize();
}

}
