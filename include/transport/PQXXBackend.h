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

#ifdef WITH_PQXX

#include <string>
#include <map>
#include "transport/StorageBackend.h"
#include "transport/Config.h"
#include <pqxx/pqxx>

namespace Transport {

/// Used to store transport data into SQLite3 database.
class PQXXBackend : public StorageBackend
{
	public:
		/// Creates new PQXXBackend instance.
		/// \param config cofiguration, this class uses following Config values:
		/// 	- database.database - path to SQLite3 database file, database file is created automatically
		/// 	- service.prefix - prefix for tables created by createDatabase method
		PQXXBackend(Config *config);

		/// Destructor.
		~PQXXBackend();

		/// Connects to the database and creates it if it's needed. This method call createDatabase() function
		/// automatically.
		/// \return true if database is opened successfully.
		bool connect();
		void disconnect();

		/// Creates database structure.
		/// \see connect()
		/// \return true if database structure has been created successfully. Note that it returns True also if database structure
		/// already exists.
		bool createDatabase();

		/// Stores user into database.
		/// \param user user struct containing all information about user which have to be stored
		void setUser(const UserInfo &user);

		/// Gets user data from database and stores them into user reference.
		/// \param barejid barejid of user
		/// \param user UserInfo object where user data will be stored
		/// \return true if user has been found in database
		bool getUser(const std::string &barejid, UserInfo &user);

		/// Changes users online state variable in database.
		/// \param id id of user - UserInfo.id
		/// \param online online state
		void setUserOnline(long id, bool online);

		/// Removes user and all connected data from database.
		/// \param id id of user - UserInfo.id
		/// \return true if user has been found in database and removed
		bool removeUser(long id);

		/// Returns JIDs of all buddies in user's roster.
		/// \param id id of user - UserInfo.id
		/// \param roster string list used to store user's roster
		/// \return true if user has been found in database and roster has been fetched
		bool getBuddies(long id, std::list<BuddyInfo> &roster);

		bool getOnlineUsers(std::vector<std::string> &users);

		bool getUsers(std::vector<std::string> &users);

		bool getLegacyNetworkUsers(std::vector<std::string> &users);

		long addBuddy(long userId, const BuddyInfo &buddyInfo);

		void updateBuddy(long userId, const BuddyInfo &buddyInfo);
		void removeBuddy(long id) {}

		void getBuddySetting(long userId, long buddyId, const std::string &variable, int &type, std::string &value) {}
		void updateBuddySetting(long userId, long buddyId, const std::string &variable, int type, const std::string &value) {}

		void getUserSetting(long userId, const std::string &variable, int &type, std::string &value);
		void updateUserSetting(long userId, const std::string &variable, const std::string &value);

		void beginTransaction();
		void commitTransaction();

	private:
		bool exec(const std::string &query, bool show_error = true);
		bool exec(pqxx::nontransaction &txn, const std::string &query, bool show_error = true);
		template<typename T>
		std::string quote(pqxx::nontransaction &txn, const T &t);

		Config *m_config;
		std::string m_prefix;

		pqxx::connection *m_conn;
};

}

#endif
