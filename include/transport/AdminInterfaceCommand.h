/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2016, Jan Kaluza <hanzz.k@gmail.com>
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

#include <string>
#include <map>

#include "Swiften/Elements/Message.h"
#include "transport/StorageBackend.h"

namespace Transport {

class User;

class AdminInterfaceCommand {
	public:
		typedef enum {
			GlobalContext,
			UserContext
		} Context;

		typedef enum {
			None = 0,
			Get = 1,
			Set = 2,
			Execute = 4
		} Actions;

		typedef enum {
			AdminMode,
			UserMode
		} AccessMode;

		typedef enum {
			General,
			Users,
			Messages,
			Frontend,
			Backends,
			Memory
		} Category;

		class Arg {
			public:
				Arg(const std::string &_name, const std::string &_label, const std::string &_type, const std::string &_example) :
					name(_name), label(_label), type(_type), example(_example) {}
				~Arg() {}

				std::string name;
				std::string label;
				std::string type;
				std::string example;
		};

		AdminInterfaceCommand(const std::string &name, Category category, Context context, AccessMode accessMode, Actions actions, const std::string &label = "");

		virtual ~AdminInterfaceCommand() { }

		void setDescription(const std::string &desc) {
			m_desc = desc;
		}

		const std::string &getDescription() {
			return m_desc;
		}

		const std::string &getName() {
			return m_name;
		}

		Actions getActions() {
			return m_actions;
		}

		Category getCategory() {
			return m_category;
		}

		const std::string getCategoryName(Category category);

		Context getContext() {
			return m_context;
		}

		AccessMode getAccessMode() {
			return m_accessMode;
		}

		void addArg(const std::string &name, const std::string &label, const std::string &type = "string", const std::string &example = "") {
			Arg arg(name, label, type, example);
			m_args.push_back(arg);
		}

		const std::list<Arg> &getArgs() {
			return m_args;
		}

		const std::string &getLabel() {
			return m_label;
		}

		virtual std::string handleSetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args);
		virtual std::string handleGetRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args);
		virtual std::string handleExecuteRequest(UserInfo &uinfo, User *user, std::vector<std::string> &args);

	private:
		std::string m_name;
		Category m_category;
		Context m_context;
		AccessMode m_accessMode;
		Actions m_actions;
		std::string m_desc;
		std::list<Arg> m_args;
		std::string m_label;
};

}
