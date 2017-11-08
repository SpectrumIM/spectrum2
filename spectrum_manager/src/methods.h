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

#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>

#include "managerconfig.h"
#include "transport/Config.h"
#include "transport/protocol.pb.h"
#include "Swiften/Swiften.h"
#include "Swiften/EventLoop/SimpleEventLoop.h"



std::string searchForBinary(const std::string &binary);

// Executes new backend
unsigned long exec_(std::string path, std::string config, std::string jid = "");

int getPort(const std::string &portfile);

int isRunning(const std::string &pidfile);
int start_instances(ManagerConfig *config, const std::string &_jid = "");
int restart_instances(ManagerConfig *config, const std::string &_jid = "");
void stop_instances(ManagerConfig *config, const std::string &_jid = "");

int show_status(ManagerConfig *config);

void ask_local_server(ManagerConfig *config, Swift::BoostNetworkFactories &networkFactories, const std::string &jid, const std::string &message);
std::string get_config(ManagerConfig *config, const std::string &jid, const std::string &key);

std::vector<std::string> show_list(ManagerConfig *config, bool show = true);

std::string get_response();
