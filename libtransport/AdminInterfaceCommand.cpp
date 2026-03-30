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

#include "transport/AdminInterfaceCommand.h"
#include "transport/User.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Transport {

AdminInterfaceCommand::AdminInterfaceCommand(const std::string &name, Category category, Context context, AccessMode accessMode, Actions actions, const std::string &label) {
	m_name = name;
	m_category = category;
	m_context = context;
	m_accessMode = accessMode;
	m_actions = actions;
	m_label = label;
}

const std::string AdminInterfaceCommand::getCategoryName(Category category) {
	switch (category) {
		case AdminInterfaceCommand::General:
			return "General";
		case AdminInterfaceCommand::Users:
			return "Users";
		case AdminInterfaceCommand::Messages:
			return "Messages";
		case AdminInterfaceCommand::Frontend:
			return "Frontend";
		case AdminInterfaceCommand::Backends:
			return "Backends";
		case AdminInterfaceCommand::Memory:
			return "Memory";
		default:
			return "Unknown";
	}
}

std::string AdminInterfaceCommand::handleSetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
	if ((m_actions & Set) == 0) {
		return "Error: This variable cannot be set.";
	}

	if (user && (m_accessMode & AdminMode) != 0) {
		return "Error: You do not have rights to set this variable.";
	}

	if ((!user && uinfo.id == -1) && (m_context & UserContext)) {
		return "Error: This variable can be set only in user context.";
	}

	if (args.empty()) {
		return "Error: Value is missing.";
	}

	return "";
}

std::string AdminInterfaceCommand::handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
	if ((m_actions & Get) == 0) {
		return "Error: This variable cannot be get.";
	}

	if (user && (m_accessMode & AdminMode) != 0) {
		return "Error: You do not have rights to get this variable.";
	}

	if ((!user && uinfo.id == -1) && (m_context & UserContext)) {
		return "Error: This variable can be get only in user context.";
	}

	return "";
}

std::string AdminInterfaceCommand::handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args) {
	if ((m_actions & Execute) == 0) {
		return "Error: This is not a command, but a variable.";
	}

	if (user && (m_accessMode & AdminMode) != 0) {
		return "Error: You do not have rights to execute this command.";
	}

	if ((!user && uinfo.id == -1) && (m_context & UserContext)) {
		return "Error: This command can be executed only in user context.";
	}

	if (m_args.size() > args.size()) {
		return "Error: Argument is missing.";
	}

	if (m_args.size() < args.size()) {
		return "Error: Too many arguments.";
	}

	return "";
}

}
