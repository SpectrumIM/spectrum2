/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/StatsSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>

namespace Swift {

StatsSerializer::StatsSerializer() : GenericPayloadSerializer<StatsPayload>() {
}

std::string StatsSerializer::serializePayload(boost::shared_ptr<StatsPayload> stats)  const {
	XMLElement queryElement("query", "http://jabber.org/protocol/stats");
	foreach(const StatsPayload::Item& item, stats->getItems()) {
		boost::shared_ptr<XMLElement> statElement(new XMLElement("stat"));
		statElement->setAttribute("name", item.getName());
		if (!item.getUnits().empty()) {
			statElement->setAttribute("units", item.getUnits());
		}
		if (!item.getUnits().empty()) {
			statElement->setAttribute("value", item.getUnits());
		}

		queryElement.addNode(statElement);
	}

	return queryElement.serialize();
}

}
