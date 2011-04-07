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
#define CHECK_DB_RESPONSE(stmt) \
	if(stmt) { \
		sqlite3_exec(m_db, "ROLLBACK;", NULL, NULL, NULL); \
		return 0; \
	}

// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	if(sqlite3_prepare_v2(m_db, std::string(str).c_str(), -1, &sql, NULL)) { \
		onStorageError(str, (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db))); \
		return false; \
	}

// Finalize the prepared statement
#define FINALIZE_STMT(prep) \
	if(prep != NULL) { \
		sqlite3_finalize(prep); \
	}
	
#define BEGIN(STATEMENT) 	sqlite3_reset(m_addBuddy);\
							int STATEMENT##_id = 1;

#define BIND_INT(STATEMENT, VARIABLE) sqlite3_bind_int(STATEMENT, STATEMENT##_id++, VARIABLE);
#define BIND_STR(STATEMENT, VARIABLE) sqlite3_bind_text(STATEMENT, STATEMENT##_id++, VARIABLE.c_str(), -1, SQLITE_STATIC);
#define EXECUTE_STATEMENT(STATEMENT, NAME) 	if(sqlite3_step(STATEMENT) != SQLITE_DONE) {\
		onStorageError(NAME, (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));\
			}

using namespace boost;

namespace Transport {

SQLite3Backend::SQLite3Backend(Config *config) {
	m_config = config;
	m_db = NULL;
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
}

SQLite3Backend::~SQLite3Backend(){
	if (m_db) {
		// Would be nice to use this:
		//
		//   sqlite3_stmt *pStmt;
		//   while((pStmt = sqlite3_next_stmt(db, 0)) != 0 ) {
		//    sqlite3_finalize(pStmt);
		//   }
		//
		// But requires SQLite3 >= 3.6.0 beta
	
		FINALIZE_STMT(m_setUser);
		sqlite3_close(m_db);
	}
}

bool SQLite3Backend::connect() {
	if (sqlite3_open(CONFIG_STRING(m_config, "database.database").c_str(), &m_db)) {
		sqlite3_close(m_db);
		return false;
	}

	if (createDatabase() == false)
		return false;

	PREP_STMT(m_setUser, "INSERT INTO " + m_prefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES (?, ?, ?, ?, ?, DATETIME('NOW'), ?)");
	PREP_STMT(m_getUser, "SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?");

	PREP_STMT(m_removeUser, "DELETE FROM " + m_prefix + "users WHERE id=?");
	PREP_STMT(m_removeUserBuddies, "DELETE FROM " + m_prefix + "buddies WHERE user_id=?");
	PREP_STMT(m_removeUserSettings, "DELETE FROM " + m_prefix + "users_settings WHERE user_id=?");
	PREP_STMT(m_removeUserBuddiesSettings, "DELETE FROM " + m_prefix + "buddies_settings WHERE user_id=?");

	PREP_STMT(m_addBuddy, "INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)");
	PREP_STMT(m_updateBuddy, "UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?");

	return true;
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
	sqlite3_reset(m_setUser);
	sqlite3_bind_text(m_setUser, 1, user.jid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 2, user.uin.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 3, user.password.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 4, user.language.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 5, user.encoding.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int (m_setUser, 6, user.vip);

	if(sqlite3_step(m_setUser) != SQLITE_DONE) {
		onStorageError("setUser query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
	}
}

bool SQLite3Backend::getUser(const std::string &barejid, UserInfo &user) {
// 	SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?
	sqlite3_reset(m_getUser);
	sqlite3_bind_text(m_getUser, 1, barejid.c_str(), -1, SQLITE_TRANSIENT);

	int ret;
	while((ret = sqlite3_step(m_getUser)) == SQLITE_ROW) {
		user.id = sqlite3_column_int(m_getUser, 0);
		user.jid = (const char *) sqlite3_column_text(m_getUser, 1);
		user.uin = (const char *) sqlite3_column_text(m_getUser, 2);
		user.password = (const char *) sqlite3_column_text(m_getUser, 3);
		user.encoding = (const char *) sqlite3_column_text(m_getUser, 4);
		user.language = (const char *) sqlite3_column_text(m_getUser, 5);
		user.vip = sqlite3_column_int(m_getUser, 6);
		return true;
	}

	if (ret != SQLITE_DONE) {
		onStorageError("getUser query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
	}

	return false;
}

void SQLite3Backend::setUserOnline(long id, bool online) {
	
}

long SQLite3Backend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)"
	BEGIN(m_addBuddy);
	BIND_INT(m_addBuddy, userId);
	BIND_STR(m_addBuddy, buddyInfo.legacyName);
	BIND_STR(m_addBuddy, buddyInfo.subscription);
	BIND_STR(m_addBuddy, buddyInfo.groups[0]); // TODO: serialize groups
	BIND_STR(m_addBuddy, buddyInfo.alias);
	BIND_INT(m_addBuddy, buddyInfo.flags);

	if(sqlite3_step(m_addBuddy) != SQLITE_DONE) {
		onStorageError("addBuddy query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return -1;
	}
	return (long) sqlite3_last_insert_rowid(m_db);
}

void SQLite3Backend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?
	BEGIN(m_updateBuddy);
	BIND_STR(m_updateBuddy, buddyInfo.groups[0]); // TODO: serialize groups
	BIND_STR(m_updateBuddy, buddyInfo.alias);
	BIND_INT(m_updateBuddy, buddyInfo.flags);
	BIND_STR(m_updateBuddy, buddyInfo.subscription);
	BIND_INT(m_updateBuddy, userId);
	BIND_STR(m_updateBuddy, buddyInfo.legacyName);

	EXECUTE_STATEMENT(m_updateBuddy, "updateBuddy query");
}

bool SQLite3Backend::getBuddies(long id, std::list<std::string> &roster) {
	return true;
}

bool SQLite3Backend::removeUser(long id) {
	sqlite3_reset(m_removeUser);
	sqlite3_bind_int(m_removeUser, 1, id);
	if(sqlite3_step(m_removeUser) != SQLITE_DONE) {
		onStorageError("removeUser query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserSettings);
	sqlite3_bind_int(m_removeUserSettings, 1, id);
	if(sqlite3_step(m_removeUserSettings) != SQLITE_DONE) {
		onStorageError("removeUserSettings query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserBuddies);
	sqlite3_bind_int(m_removeUserBuddies, 1, id);
	if(sqlite3_step(m_removeUserBuddies) != SQLITE_DONE) {
		onStorageError("removeUserBuddies query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserBuddiesSettings);
	sqlite3_bind_int(m_removeUserBuddiesSettings, 1, id);
	if(sqlite3_step(m_removeUserBuddiesSettings) != SQLITE_DONE) {
		onStorageError("removeUserBuddiesSettings query", (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	return true;
}

void SQLite3Backend::beginTransaction() {
	exec("BEGIN TRANSACTION;");
}

void SQLite3Backend::commitTransaction() {
	exec("COMMIT TRANSACTION;");
}

}
