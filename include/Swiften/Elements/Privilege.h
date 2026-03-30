/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <vector>
#include <string>

#include <Swiften/Base/API.h>
#include <Swiften/Elements/Payload.h>

#include <Swiften/Version.h>
#include <Swiften/Elements/Forwarded.h>

namespace Swift {
	class Stanza;

	class Privilege : public Payload {
	public:
		typedef std::shared_ptr<Privilege> ref;
		typedef Swift::Forwarded Forwarded;

	public:
		Privilege();

		void setForwarded(std::shared_ptr<Forwarded> forwarded) { forwarded_ = forwarded; }
		const std::shared_ptr<Forwarded>& getForwarded() const { return forwarded_; }

	private:
		std::shared_ptr<Forwarded> forwarded_;
	};
}
