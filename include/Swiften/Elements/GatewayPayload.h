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
	class GatewayPayload : public Payload {
		public:
			GatewayPayload(const JID &jid = JID(), const std::string &desc = "", const std::string &prompt = "");

			void setJID(const JID &jid) {
				this->jid = jid;
			}

			const JID &getJID() const {
				return jid;
			}

			void setDesc(const std::string &desc) {
				this->desc = desc;
			}

			const std::string &getDesc() const {
				return desc;
			}

			void setPrompt(const std::string &prompt) {
				this->prompt = prompt;
			}

			const std::string &getPrompt() const {
				return prompt;
			}

		private:
			JID jid;
			std::string desc;
			std::string prompt;
	};
}
