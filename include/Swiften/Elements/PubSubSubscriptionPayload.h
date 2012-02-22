/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>

#include <string>
#include <Swiften/Elements/Payload.h>
#include <Swiften/JID/JID.h>

namespace Swift {
	class PubSubSubscriptionPayload : public Payload {
		public:
			enum Type { None, Pending, Subscribed, Unconfigured };

			PubSubSubscriptionPayload(const JID &jid = JID(), const std::string &node = "");

			void setJID(const JID &jid) {
				this->jid = jid;
			}

			const JID &getJID() const {
				return jid;
			}

			void setNode(const std::string &node) {
				this->node = node;
			}

			const std::string &getNode() const {
				return node;
			}

			void setId(const std::string &id) {
				this->id = id;
			}

			const std::string &getId() const {
				return id;
			}

			void setType(const Type &type) {
				this->type = type;
			}

			const Type &getType() const {
				return type;
			}

		private:
			JID jid;
			std::string node;
			std::string id;
			Type type;
	};
}
