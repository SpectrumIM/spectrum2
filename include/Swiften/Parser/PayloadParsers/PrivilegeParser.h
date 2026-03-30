/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Base/API.h>
#include <Swiften/Elements/Privilege.h>
#include <Swiften/Parser/GenericPayloadParser.h>

namespace Swift {
	class PayloadParserFactoryCollection;
	class PayloadParser;

	class PrivilegeParser : public GenericPayloadParser<Privilege> {
	public:
		PrivilegeParser(PayloadParserFactoryCollection* factories);

		virtual void handleStartElement(const std::string& element, const std::string&, const AttributeMap& attributes);
		virtual void handleEndElement(const std::string& element, const std::string&);
		virtual void handleCharacterData(const std::string& data);

	enum Level {
		TopLevel = 0,
		PayloadLevel = 1
	};

	private:
		PayloadParserFactoryCollection* factories_;
		std::shared_ptr<PayloadParser> childParser_;
		int level_;
	};
}
