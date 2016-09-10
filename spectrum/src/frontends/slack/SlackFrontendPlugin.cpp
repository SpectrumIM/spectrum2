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

#include "SlackFrontendPlugin.h"
#include "SlackFrontend.h"

#include <boost/dll/alias.hpp>

namespace Transport {

SlackFrontendPlugin::SlackFrontendPlugin() { 

}

SlackFrontendPlugin::~SlackFrontendPlugin() {
	
}

std::string SlackFrontendPlugin::name() const {
	return "slack";
}

Frontend *SlackFrontendPlugin::createFrontend() {
	return new SlackFrontend();
}

// SlackFrontendPlugin plugin;

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<FrontendPlugin> get_slack_frontend_plugin() {
	return SWIFTEN_SHRPTR_NAMESPACE::make_shared<SlackFrontendPlugin>();
}

// BOOST_DLL_AUTO_ALIAS(plugin)

}
