/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <Swiften/Parser/Tree/ParserElement.h>
#include <Swiften/Parser/XMLParserClient.h>

namespace Swift {
	/**
	 * Generics parser offering something a bit like a DOM to work with.
	 */
	class StringTreeParser : public XMLParserClient {
		public:
			StringTreeParser() : XMLParserClient() {}
			
			virtual void handleStartElement(const std::string& element, const std::string& xmlns, const AttributeMap& attributes) {
				if (!root_) {
					root_ = boost::make_shared<ParserElement>(element, xmlns, attributes);
					elementStack_.push_back(root_);
				}
				else {
					ParserElement::ref current = *elementStack_.rbegin();
					elementStack_.push_back(current->addChild(element, xmlns, attributes));
				}
			}

			virtual void handleEndElement(const std::string& /*element*/, const std::string&) {
				elementStack_.pop_back();
				if (elementStack_.empty()) {
					handleTree(root_);
				}
			}

			virtual void handleCharacterData(const std::string& data) {
				ParserElement::ref current = *elementStack_.rbegin();
				current->appendCharacterData(data);
			}

			virtual void handleTree(ParserElement::ref root) = 0;

			static ParserElement::ref parse(const std::string &xml);

		private:
			std::deque<ParserElement::ref> elementStack_;
			ParserElement::ref root_;
	};
}
