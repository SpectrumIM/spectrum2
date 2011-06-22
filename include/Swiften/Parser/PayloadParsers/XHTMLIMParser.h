/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Elements/XHTMLIMPayload.h>
#include <Swiften/Parser/GenericPayloadParser.h>

namespace Swift {
	class SerializingParser;

	class XHTMLIMParser : public GenericPayloadParser<XHTMLIMPayload> {
		public:
			XHTMLIMParser();

			virtual void handleStartElement(const std::string& element, const std::string&, const AttributeMap& attributes);
			virtual void handleEndElement(const std::string& element, const std::string&);
			virtual void handleCharacterData(const std::string& data);
			boost::shared_ptr<XHTMLIMPayload> getLabelPayload() const;
		private:
			enum Level { 
				TopLevel = 0, 
				PayloadLevel = 1,
				BodyLevel = 2,
				InsideBodyLevel = 3
			};
			int level_;
			SerializingParser* bodyParser_;
			std::string currentText_;
	};
}
