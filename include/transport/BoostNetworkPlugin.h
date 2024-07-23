/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011-2023, Jan Kaluza <hanzz.k@gmail.com>
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

#include <transport/Config.h>
#include <transport/Logging.h>
#include <transport/NetworkPlugin.h>

#include <boost/asio.hpp>

#include <memory>

namespace Transport {
    class BoostNetworkPlugin: public NetworkPlugin {
        public:
            BoostNetworkPlugin(Config *config, const std::string &host, int port);
            void sendData(const std::string &string);
            void handleExitRequest();
            void run();
        protected:
            Transport::Config *config;
            boost::asio::io_context io_context;
        private:
		    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
		    bool is_connected;
    };
}
