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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301 USA
 */

#include "transport/BoostNetworkPlugin.h"
#include "transport/Logging.h"

#include <boost/thread.hpp>
#include <boost/version.hpp>

DEFINE_LOGGER(pluginLogger, "Backend");

namespace Transport {
BoostNetworkPlugin::BoostNetworkPlugin(Config *config, const std::string &host,
                                       int port)
    : NetworkPlugin(), io_context() {
  this->config = config;
  LOG4CXX_INFO(pluginLogger, "Starting the plugin.");
#if BOOST_VERSION >= 106600
  boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(host), port);
#else
  boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(host), port);
#endif
  socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
  socket_->async_connect(ep, [this, host](boost::system::error_code ec) {
    if (!ec) {
      LOG4CXX_INFO(pluginLogger, "Connected to " << host);
      this->is_connected = true;
      boost::asio::streambuf sb;
      boost::system::error_code read_ec;
      while (is_connected &&
             boost::asio::read(*socket_, sb, boost::asio::transfer_at_least(1),
                               read_ec)) {
        if (read_ec) {
          LOG4CXX_ERROR(pluginLogger, "Connection read error: " << read_ec.message());
          break;
        } else {
          std::string s((std::istreambuf_iterator<char>(&sb)),
                        std::istreambuf_iterator<char>());
          handleDataRead(s);
        }
      }
    } else {
      LOG4CXX_ERROR(pluginLogger, "Connection error: " << ec.message());
    }
  });
}
// NetworkPlugin uses this method to send the data to networkplugin server
void BoostNetworkPlugin::sendData(const std::string &string) {
  if (!is_connected) return;
  boost::asio::write(*socket_, boost::asio::buffer(string));
}
void BoostNetworkPlugin::handleExitRequest() {
  this->is_connected = false;
  this->io_context.stop();
}

void BoostNetworkPlugin::run() {
  boost::thread thread([this]() { io_context.run(); });
  thread.join();
}
} // namespace Transport
