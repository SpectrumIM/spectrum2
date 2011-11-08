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
	class StatsPayload : public Payload {
		public:
			class Item {
				public:
					Item(const std::string &name = "", const std::string &units = "", const std::string &value = "") :
						name(name), units(units), value(value) { }

					void setName(const std::string &name) {
						this->name = name;
					}

					const std::string &getName() const {
						return name;
					}

					void setUnits(const std::string &units) {
						this->units = units;
					}

					const std::string &getUnits() const {
						return units;
					}

					void setValue(const std::string &value) {
						this->value = value;
					}

					const std::string &getValue() const {
						return value;
					}

				private:
					std::string name;
					std::string units;
					std::string value;
			};

			typedef std::vector<StatsPayload::Item> StatsPayloadItems;

			StatsPayload();

			void addItem(const StatsPayload::Item &item) {
				items.push_back(item);
			}

			const StatsPayloadItems &getItems() const {
				return items;
			}

		private:
			StatsPayloadItems items;
	};
}
