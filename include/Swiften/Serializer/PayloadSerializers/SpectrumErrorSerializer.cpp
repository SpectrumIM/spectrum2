/*
 * Copyright (c) 2011 Jan Kaluza
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Serializer/PayloadSerializers/SpectrumErrorSerializer.h>

#include <boost/shared_ptr.hpp>

#include <Swiften/Base/foreach.h>
#include <Swiften/Serializer/XML/XMLTextNode.h>
#include <Swiften/Serializer/XML/XMLRawTextNode.h>
#include <Swiften/Serializer/XML/XMLElement.h>
#include <boost/lexical_cast.hpp>

namespace Swift {

SpectrumErrorSerializer::SpectrumErrorSerializer() : GenericPayloadSerializer<SpectrumErrorPayload>() {
}

std::string SpectrumErrorSerializer::serializePayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<SpectrumErrorPayload> error)  const {
	std::string data;
	switch (error->getError()) {
		case SpectrumErrorPayload::CONNECTION_ERROR_NETWORK_ERROR: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_INVALID_USERNAME: data = "CONNECTION_ERROR_INVALID_USERNAME"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_AUTHENTICATION_FAILED: data = "CONNECTION_ERROR_AUTHENTICATION_FAILED"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE: data = "CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_NO_SSL_SUPPORT: data = "CONNECTION_ERROR_NO_SSL_SUPPORT"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_ENCRYPTION_ERROR: data = "CONNECTION_ERROR_ENCRYPTION_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_NAME_IN_USE: data = "CONNECTION_ERROR_NAME_IN_USE"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_INVALID_SETTINGS: data = "CONNECTION_ERROR_INVALID_SETTINGS"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_NOT_PROVIDED: data = "CONNECTION_ERROR_CERT_NOT_PROVIDED"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_UNTRUSTED: data = "CONNECTION_ERROR_CERT_UNTRUSTED"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_EXPIRED: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_NOT_ACTIVATED: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_HOSTNAME_MISMATCH: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_FINGERPRINT_MISMATCH: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_SELF_SIGNED: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_CERT_OTHER_ERROR: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
		case SpectrumErrorPayload::CONNECTION_ERROR_OTHER_ERROR: data = "CONNECTION_ERROR_NETWORK_ERROR"; break;
	}

	XMLElement el("spectrumerror", "http://spectrum.im/error", data);

	el.setAttribute("error", boost::lexical_cast<std::string>(error->getError()));

	return el.serialize();
}

}
