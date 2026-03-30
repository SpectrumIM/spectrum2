/*
 * Implements Privilege tag for XEP-0356: Privileged Entity
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <Swiften/Serializer/GenericPayloadSerializer.h>
#include <Swiften/Elements/Privilege.h>

namespace Swift {
	class PayloadSerializerCollection;

	class PrivilegeSerializer : public GenericPayloadSerializer<Privilege> {
	public:
		PrivilegeSerializer(PayloadSerializerCollection* serializers);
		virtual ~PrivilegeSerializer();

		virtual std::string serializePayload(std::shared_ptr<Privilege>) const;

	private:
		PayloadSerializerCollection* serializers_;
	};
}
