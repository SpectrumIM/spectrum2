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
	class PubSubSubscribePayload : public Payload {
		public:
			PubSubSubscribePayload(const JID &jid, const std::string &node = "");

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

		private:
			JID jid;
			std::string node;
	};
}
