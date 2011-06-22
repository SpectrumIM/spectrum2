/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Parser/PayloadParsers/XHTMLIMParser.h>
#include <Swiften/Parser/SerializingParser.h>

namespace Swift {

XHTMLIMParser::XHTMLIMParser() : level_(TopLevel), bodyParser_(0) {
}

void XHTMLIMParser::handleStartElement(const std::string& element, const std::string& ns, const AttributeMap& attributes) {
	++level_;
	if (level_ == BodyLevel) {
		if (element == "body") {
			assert(!bodyParser_);
			bodyParser_ = new SerializingParser();
		}
	}
	else if (level_ >= InsideBodyLevel && bodyParser_) {
		bodyParser_->handleStartElement(element, "", attributes);
	}
}

void XHTMLIMParser::handleEndElement(const std::string& element, const std::string& ns) {
	if (level_ == BodyLevel) {
		if (bodyParser_) {
			if (element == "body") {
				getPayloadInternal()->setBody(bodyParser_->getResult());
			}
			delete bodyParser_;
			bodyParser_ = 0;
		}
	}
	else if (bodyParser_ && level_ >= InsideBodyLevel) {
		bodyParser_->handleEndElement(element, ns);
	}
	--level_;
}

void XHTMLIMParser::handleCharacterData(const std::string& data) {
	if (bodyParser_) {
		bodyParser_->handleCharacterData(data);
	}
	else {
		currentText_ += data;
	}
}

boost::shared_ptr<XHTMLIMPayload> XHTMLIMParser::getLabelPayload() const {
	return getPayloadInternal();
}

}
