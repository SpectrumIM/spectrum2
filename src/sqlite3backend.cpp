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

#include "transport/sqlite3backend.h"
#include <boost/bind.hpp>

#define SQLITE_DB_VERSION 3

using namespace boost;

namespace Transport {

SQLite3Backend::SQLite3Backend(Config *config) {
	m_config = config;
	m_db = NULL;
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
}

SQLite3Backend::~SQLite3Backend(){
	if (m_db) {
		sqlite3_close(m_db);
	}
}

bool SQLite3Backend::connect() {
	if (sqlite3_open(CONFIG_STRING(m_config, "database.database").c_str(), &m_db)) {
		 sqlite3_close(m_db);
		 return false;
	}
	return createDatabase();
}

bool SQLite3Backend::createDatabase() {
	int not_exist = exec("CREATE TABLE " + m_prefix + "buddies ("
				"  id INTEGER PRIMARY KEY NOT NULL,"
				"  user_id int(10) NOT NULL,"
				"  uin varchar(255) NOT NULL,"
				"  subscription varchar(20) NOT NULL,"
				"  nickname varchar(255) NOT NULL,"
				"  groups varchar(255) NOT NULL,"
				"  flags int(4) NOT NULL DEFAULT '0'"
				");");

	if (not_exist) {
		exec("CREATE UNIQUE INDEX IF NOT EXISTS user_id ON " + m_prefix + "buddies (user_id, uin);");

		exec("CREATE TABLE IF NOT EXISTS " + m_prefix + "buddies_settings ("
					"  user_id int(10) NOT NULL,"
					"  buddy_id int(10) NOT NULL,"
					"  var varchar(50) NOT NULL,"
					"  type int(4) NOT NULL,"
					"  value varchar(255) NOT NULL,"
					"  PRIMARY KEY (buddy_id, var)"
					");");

		exec("CREATE INDEX IF NOT EXISTS user_id02 ON " + m_prefix + "buddies_settings (user_id);");

		exec("CREATE TABLE IF NOT EXISTS " + m_prefix + "users ("
					"  id INTEGER PRIMARY KEY NOT NULL,"
					"  jid varchar(255) NOT NULL,"
					"  uin varchar(4095) NOT NULL,"
					"  password varchar(255) NOT NULL,"
					"  language varchar(25) NOT NULL,"
					"  encoding varchar(50) NOT NULL DEFAULT 'utf8',"
					"  last_login datetime,"
					"  vip int(1) NOT NULL DEFAULT '0',"
					"  online int(1) NOT NULL DEFAULT '0'"
					");");

		exec("CREATE UNIQUE INDEX IF NOT EXISTS jid ON " + m_prefix + "users (jid);");

		exec("CREATE TABLE " + m_prefix + "users_settings ("
					"  user_id int(10) NOT NULL,"
					"  var varchar(50) NOT NULL,"
					"  type int(4) NOT NULL,"
					"  value varchar(255) NOT NULL,"
					"  PRIMARY KEY (user_id, var)"
					");");
					
		exec("CREATE INDEX IF NOT EXISTS user_id03 ON " + m_prefix + "users_settings (user_id);");

		exec("CREATE TABLE IF NOT EXISTS " + m_prefix + "db_version ("
			"  ver INTEGER NOT NULL DEFAULT '3'"
			");");
		exec("REPLACE INTO " + m_prefix + "db_version (ver) values(3)");
	}
	return true;
}

bool SQLite3Backend::exec(const std::string &query) {
	char *errMsg = 0;
	int rc = sqlite3_exec(m_db, query.c_str(), NULL, 0, &errMsg);
	if (rc != SQLITE_OK) {
		onStorageError(query, errMsg);
		sqlite3_free(errMsg);
		return false;
	}
	return true;
}

void SQLite3Backend::setUser(const UserInfo &user) {
	
}

bool SQLite3Backend::getUser(const std::string &barejid, UserInfo &user) {
	return true;
}

void SQLite3Backend::setUserOnline(long id, bool online) {
	
}

bool SQLite3Backend::getBuddies(long id, std::list<std::string> &roster) {
	return true;
}

bool SQLite3Backend::removeUser(long id) {
	return true;
}

}
