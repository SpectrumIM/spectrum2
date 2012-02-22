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
			PubSubItem();

			void addPayload(boost::shared_ptr<Payload> payload) {
				payloads.push_back(payload);
			}

			const std::vector<boost::shared_ptr<Payload> > getPayloads() const {
				return payloads;
			}
			
			template<typename T>
			const std::vector<boost::shared_ptr<T> > getPayloads() const {
				std::vector<boost::shared_ptr<T> > matched_payloads;
				for (std::vector<boost::shared_ptr<Payload> >::const_iterator i = payloads.begin(); i != payloads.end(); ++i) {
					boost::shared_ptr<T> result = boost::dynamic_pointer_cast<T>(*i);
					if (result) {
						matched_payloads.push_back(result);
					}
				}
				
				return matched_payloads;
				
			}

			template<typename T>
			const boost::shared_ptr<T> getPayload() const {
				boost::shared_ptr<T> result;
				for (std::vector<boost::shared_ptr<Payload> >::const_iterator i = payloads.begin(); i != payloads.end(); ++i) {
					result = boost::dynamic_pointer_cast<T>(*i);
					if (result) {
						return result;
					}
				}
				
				return result;
			}

			const std::string& getId() const { return id; }

			void setId(const std::string& id) { 
				this->id = id;
			}

		private:
			std::vector<boost::shared_ptr<Payload> > payloads;
			std::string id;
	};
}
