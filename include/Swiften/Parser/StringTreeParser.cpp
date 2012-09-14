/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/StringTreeParser.h>
#include <Swiften/Parser/PlatformXMLParserFactory.h>
#include <Swiften/Parser/Tree/ParserElement.h>
#include <Swiften/Parser/XMLParser.h>

namespace Swift {

class DefaultStringTreeParser : public StringTreeParser {
	public:
		void handleTree(ParserElement::ref root) {
			root_ = root;
		}

		ParserElement::ref getRoot() {
			return root_;
		}

	private:
		ParserElement::ref root_;
};
	
ParserElement::ref StringTreeParser::parse(const std::string &xml) {
	PlatformXMLParserFactory factory;
	DefaultStringTreeParser client;
	XMLParser *parser = factory.createXMLParser(&client);
	
	parser->parse(xml);
	ParserElement::ref root = client.getRoot();
	delete parser;
	return root;
}

}
