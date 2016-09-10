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

#include "XMPPFrontendPlugin.h"
#include "XMPPFrontend.h"

#include <boost/dll/alias.hpp>

namespace Transport {

XMPPFrontendPlugin::XMPPFrontendPlugin() { 

}

XMPPFrontendPlugin::~XMPPFrontendPlugin() {
	
}

std::string XMPPFrontendPlugin::name() const {
	return "xmpp";
}

Frontend *XMPPFrontendPlugin::createFrontend() {
	return new XMPPFrontend();
}

// XMPPFrontendPlugin plugin;

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<FrontendPlugin> get_xmpp_frontend_plugin() {
	return SWIFTEN_SHRPTR_NAMESPACE::make_shared<XMPPFrontendPlugin>();
}

// BOOST_DLL_AUTO_ALIAS(plugin)

}
