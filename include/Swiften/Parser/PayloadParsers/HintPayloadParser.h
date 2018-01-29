/*
 * Implements XEP-0334: Message Processing Hints
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Elements/HintPayload.h>
#include <Swiften/Parser/GenericPayloadParser.h>

namespace Swift {
	class HintPayloadParser : public GenericPayloadParser<HintPayload> {
		public:
			HintPayloadParser();

			virtual void handleStartElement(const std::string& element, const std::string&, const AttributeMap& attributes);
			virtual void handleEndElement(const std::string& element, const std::string&);
			virtual void handleCharacterData(const std::string& data);

 		private:
 			int level_;
	};
}
