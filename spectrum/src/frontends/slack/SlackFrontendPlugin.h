/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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

#include "transport/Frontend.h"

#include <boost/dll/alias.hpp>
#include <boost/make_shared.hpp>

namespace Transport {

class SlackFrontendPlugin : public FrontendPlugin {
	public:
		SlackFrontendPlugin();
		virtual ~SlackFrontendPlugin();

		std::string name() const;
		Frontend *createFrontend();
};

std::shared_ptr<FrontendPlugin> get_slack_frontend_plugin();


}

BOOST_DLL_ALIAS(Transport::get_slack_frontend_plugin, create_slack_frontend_plugin)
