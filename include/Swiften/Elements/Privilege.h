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
#include <Swiften/Elements/Forwarded.h>

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class Stanza;

	class Privilege : public Payload {
	public:
		typedef SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege> ref;

	public:
		Privilege();

		void setForwarded(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded> forwarded) { forwarded_ = forwarded; }
		const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded>& getForwarded() const { return forwarded_; }

	private:
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded> forwarded_;
	};
}
