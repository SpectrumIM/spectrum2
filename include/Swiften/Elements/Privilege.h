/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include <Swiften/Base/API.h>
#include <Swiften/Elements/Payload.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class Stanza;

	class Privilege : public Payload {
		public:
			typedef SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege> ref;

		public:
			Privilege();

			void setStanza(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza) { stanza_ = stanza; }
			const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza>& getStanza() const { return stanza_; }

		private:
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Stanza> stanza_;
	};
}
