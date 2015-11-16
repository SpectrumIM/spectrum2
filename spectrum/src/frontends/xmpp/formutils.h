/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#pragma once

#include "Swiften/Queries/Responder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"
#include "Swiften/Elements/RosterPayload.h"
#include <boost/signal.hpp>
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

namespace Transport {

	class AdHocCommand;

namespace FormUtils {

	void addHiddenField(Swift::Form::ref form, const std::string &name, const std::string &value);
	void addTextSingleField(Swift::Form::ref form, const std::string &name, const std::string &value,
							const std::string &label, bool required = false);
	void addTextPrivateField(Swift::Form::ref form, const std::string &name, const std::string &label,
							 bool required = false);
	void addListSingleField(Swift::Form::ref form, const std::string &name, Swift::FormField::Option value,
							const std::string &label, const std::string &def, bool required = false);
	void addBooleanField(Swift::Form::ref form, const std::string &name, const std::string &value,
							const std::string &label, bool required = false);
	void addTextFixedField(Swift::Form::ref form, const std::string &value);

	std::string fieldValue(Swift::FormField::ref);

	std::string fieldValue(Swift::Form::ref, const std::string &key, const std::string &def);
}
}