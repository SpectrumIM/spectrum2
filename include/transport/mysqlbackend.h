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

#include <string>
#include <map>
#include "Swiften/Swiften.h"
#include "transport/storagebackend.h"
#include "transport/config.h"
#include "mysql.h"

namespace Transport {

/// Used to store transport data into SQLite3 database.
class MySQLBackend : public StorageBackend
{
	public:
		/// Creates new MySQLBackend instance.
		/// \param config cofiguration, this class uses following Config values:
		/// 	- database.database - path to SQLite3 database file, database file is created automatically
		/// 	- service.prefix - prefix for tables created by createDatabase method
		MySQLBackend(Config *config);

		/// Destructor.
		~MySQLBackend();

		/// Connects to the database and creates it if it's needed. This method call createDatabase() function
		/// automatically.
		/// \return true if database is opened successfully.
		bool connect();

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

		long addBuddy(long userId, const BuddyInfo &buddyInfo);

		void updateBuddy(long userId, const BuddyInfo &buddyInfo);
		void removeBuddy(long id) {}

		void getUserSetting(long userId, const std::string &variable, int &type, std::string &value);
		void updateUserSetting(long userId, const std::string &variable, const std::string &value);

		void beginTransaction();
		void commitTransaction();

	private:
		bool exec(const std::string &query);

		MYSQL *m_conn;
		Config *m_config;
		std::string m_prefix;

		// statements
// 		sqlite3_stmt *m_setUser;
// 		sqlite3_stmt *m_getUser;
// 		sqlite3_stmt *m_getUserSetting;
// 		sqlite3_stmt *m_setUserSetting;
// 		sqlite3_stmt *m_updateUserSetting;
// 		sqlite3_stmt *m_removeUser;
// 		sqlite3_stmt *m_removeUserBuddies;
// 		sqlite3_stmt *m_removeUserSettings;
// 		sqlite3_stmt *m_removeUserBuddiesSettings;
// 		sqlite3_stmt *m_addBuddy;
// 		sqlite3_stmt *m_updateBuddy;
// 		sqlite3_stmt *m_updateBuddySetting;
// 		sqlite3_stmt *m_getBuddies;
// 		sqlite3_stmt *m_getBuddiesSettings;
};

}
