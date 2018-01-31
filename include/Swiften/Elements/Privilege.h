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

#include <Swiften/Version.h>
#if (SWIFTEN_VERSION >= 0x030000)
#define SWIFTEN_SUPPORTS_FORWARDED
#include <Swiften/Elements/Forwarded.h>
#endif

#include "Swiften/SwiftenCompat.h"

namespace Swift {
	class Stanza;

	class Privilege : public Payload {
	public:
		typedef SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Privilege> ref;
#ifdef SWIFTEN_SUPPORTS_FORWARDED
		typedef Swift::Forwarded Forwarded;
#else
		typedef Payload Forwarded;
#endif

	public:
		Privilege();

		void setForwarded(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded> forwarded) { forwarded_ = forwarded; }
		const SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded>& getForwarded() const { return forwarded_; }

	private:
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Forwarded> forwarded_;
	};
}
