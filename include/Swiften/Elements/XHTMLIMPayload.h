/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>

#include <string>
#include <Swiften/Elements/Payload.h>

namespace Swift {
	class XHTMLIMPayload : public Payload {
		public:
			XHTMLIMPayload(const std::string &body = "");

			const std::string& getBody() const { return body_; }

			void setBody(const std::string& body) { 
				body_ = body;
			}

		private:
			std::string body_;
	};
}
