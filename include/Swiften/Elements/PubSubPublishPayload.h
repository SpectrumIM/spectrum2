/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>

#include <string>
#include <Swiften/Elements/Payload.h>
#include <Swiften/Elements/PubSubItem.h>
#include <Swiften/JID/JID.h>

namespace Swift {
	class PubSubPublishPayload : public Payload {
		public:
			enum Type { None, Pending, Subscribed, Unconfigured };

			PubSubPublishPayload(const std::string &node = "");

			void setNode(const std::string &node) {
				this->node = node;
			}

			const std::string &getNode() const {
				return node;
			}

			void addItem(const boost::shared_ptr<Payload> &item) {
				items.push_back(item);
			}

			const std::vector<boost::shared_ptr<Payload> > &getItems() const {
				return items;
			}

		private:
			std::string node;
			std::vector<boost::shared_ptr<Payload> > items;
	};
}
