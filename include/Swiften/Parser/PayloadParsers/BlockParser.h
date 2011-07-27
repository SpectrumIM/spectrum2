/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Elements/BlockPayload.h>
#include <Swiften/Parser/GenericPayloadParser.h>

// This payload is NOT part of ANY XEP and it is only
// libtransport related extension.
namespace Swift {
	class BlockParser : public GenericPayloadParser<BlockPayload> {
		public:
			BlockParser();

			virtual void handleStartElement(const std::string& element, const std::string&, const AttributeMap& attributes);
			virtual void handleEndElement(const std::string& element, const std::string&);
			virtual void handleCharacterData(const std::string& data);

// 		private:
// 			int level_;
	};
}
