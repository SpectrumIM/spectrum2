/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <boost/foreach.hpp>

#include <Swiften/Serializer/PayloadSerializers/StatsSerializer.h>

#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Swift {

StatsSerializer::StatsSerializer() : GenericPayloadSerializer<StatsPayload>() {
}

std::string StatsSerializer::serializePayload(std::shared_ptr<StatsPayload> stats)  const {
	XMLElement queryElement("query", "http://jabber.org/protocol/stats");
	BOOST_FOREACH(const StatsPayload::Item& item, stats->getItems()) {
		std::shared_ptr<XMLElement> statElement = std::make_shared<XMLElement>("stat");
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
