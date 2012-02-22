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
	class PubSubItem : public Payload {
		public:
			PubSubItem(const std::string &body = "");

			const std::string& getData() const { return body_; }

			void setData(const std::string& body) { 
				body_ = body;
			}

			const std::string& getId() const { return id; }

			void setId(const std::string& id) { 
				this->id = id;
			}

		private:
			std::string body_;
			std::string id;
	};
}
