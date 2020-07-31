/*
 * Implements XEP-0334: Message Processing Hints
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>
#include <string>

#include <Swiften/Elements/Payload.h>

namespace Swift {
	class HintPayload : public Payload {
	public:
		typedef std::shared_ptr<HintPayload> ref;

		enum Type { NoPermanentStore, NoStore, NoCopy, Store };

	public:
		HintPayload(Type type = NoCopy);

		void setType(Type type) { type_ = type; }
		const Type getType() { return type_; }

	private:
		Type type_;
	};
}
