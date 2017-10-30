#pragma once

#include <boost/optional.hpp>

#include "Swiften/SwiftenCompat.h"
#include "Swiften/Elements/PrivateStorage.h"
#include "Swiften/Parser/GenericPayloadParser.h"

namespace Transport {

	class StorageParser : public Swift::GenericPayloadParser<Swift::PrivateStorage> {
		public:
			StorageParser();

		private:
			virtual void handleStartElement(const std::string& element, const std::string&, const Swift::AttributeMap& attributes);
			virtual void handleEndElement(const std::string& element, const std::string&);
			virtual void handleCharacterData(const std::string& data);

		private:
			int level;
			SWIFTEN_UNIQUE_PTR<Swift::PayloadParser> currentPayloadParser;
	};
}
