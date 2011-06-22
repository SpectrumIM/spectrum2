/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Elements/AttentionPayload.h>
#include <Swiften/Parser/GenericPayloadParser.h>

namespace Swift {
	class AttentionParser : public GenericPayloadParser<AttentionPayload> {
		public:
			AttentionParser();

			virtual void handleStartElement(const std::string& element, const std::string&, const AttributeMap& attributes);
			virtual void handleEndElement(const std::string& element, const std::string&);
			virtual void handleCharacterData(const std::string& data);

// 		private:
// 			int level_;
	};
}
