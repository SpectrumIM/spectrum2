#include "storageparser.h"
#include "Swiften/Parser/PayloadParsers/RawXMLPayloadParser.h"

using namespace Swift;

namespace Transport {

StorageParser::StorageParser() : level(0) {
}

void StorageParser::handleStartElement(const std::string& element, const std::string& ns, const AttributeMap& attributes) {
	if (level == 1) {
		currentPayloadParser.reset(new RawXMLPayloadParser());
	}

	if (level >= 1 && currentPayloadParser.get()) {
		currentPayloadParser->handleStartElement(element, ns, attributes);
	}
	++level;
}

void StorageParser::handleEndElement(const std::string& element, const std::string& ns) {
	--level;
	if (currentPayloadParser.get()) {
		if (level >= 1) {
			currentPayloadParser->handleEndElement(element, ns);
		}

		if (level == 1) {
			getPayloadInternal()->setPayload(currentPayloadParser->getPayload());
		}
	}
}

void StorageParser::handleCharacterData(const std::string& data) {
	if (level > 1 && currentPayloadParser.get()) {
		currentPayloadParser->handleCharacterData(data);
	}
}

}
