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

#ifdef WITH_SQLITE

#include "transport/SQLite3Backend.h"
#include "transport/Util.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include <boost/bind.hpp>

#define SQLITE_DB_VERSION 3
#define CHECK_DB_RESPONSE(stmt) \
	if (stmt) { \
		sqlite3_exec(m_db, "ROLLBACK;", NULL, NULL, NULL); \
		return 0; \
	}

// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	if (sqlite3_prepare_v2(m_db, std::string(str).c_str(), -1, &sql, NULL)) { \
		LOG4CXX_ERROR(sqlite3Logger, str<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db))); \
		return false; \
	}

// Finalize the prepared statement
#define FINALIZE_STMT(prep) \
	if (prep != NULL) { \
		sqlite3_finalize(prep); \
	}
	
#define BEGIN(STATEMENT) 	sqlite3_reset(STATEMENT);\
							int STATEMENT##_id = 1;\
							int STATEMENT##_id_get = 0;\
							(void)STATEMENT##_id_get;

#define BIND_INT(STATEMENT, VARIABLE) sqlite3_bind_int(STATEMENT, STATEMENT##_id++, VARIABLE)
#define BIND_STR(STATEMENT, VARIABLE) sqlite3_bind_text(STATEMENT, STATEMENT##_id++, VARIABLE.c_str(), -1, SQLITE_STATIC)
#define RESET_GET_COUNTER(STATEMENT)	STATEMENT##_id_get = 0;
#define GET_INT(STATEMENT)	sqlite3_column_int(STATEMENT, STATEMENT##_id_get++)
#define GET_STR(STATEMENT)	(const char *) sqlite3_column_text(STATEMENT, STATEMENT##_id_get++)
#define GET_BLOB(STATEMENT)	(const void *) sqlite3_column_blob(STATEMENT, STATEMENT##_id_get++)
#define EXECUTE_STATEMENT(STATEMENT, NAME) 	if (sqlite3_step(STATEMENT) != SQLITE_DONE) {\
		LOG4CXX_ERROR(sqlite3Logger, NAME<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));\
			}

namespace Transport {

DEFINE_LOGGER(sqlite3Logger, "SQLite3Backend");

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
		//   while ((pStmt = sqlite3_next_stmt(db, 0)) != 0 ) {
		//    sqlite3_finalize(pStmt);
		//   }
		//
		// But requires SQLite3 >= 3.6.0 beta
	
		FINALIZE_STMT(m_setUser);
		FINALIZE_STMT(m_getUser);
		FINALIZE_STMT(m_removeUser);
		FINALIZE_STMT(m_removeUserBuddies);
		FINALIZE_STMT(m_removeUserSettings);
		FINALIZE_STMT(m_removeUserBuddiesSettings);
		FINALIZE_STMT(m_removeBuddy);
		FINALIZE_STMT(m_removeBuddySettings);
		FINALIZE_STMT(m_addBuddy);
		FINALIZE_STMT(m_updateBuddy);
		FINALIZE_STMT(m_getBuddies);
		FINALIZE_STMT(m_getBuddiesSettings);
		FINALIZE_STMT(m_getUserSetting);
		FINALIZE_STMT(m_setUserSetting);
		FINALIZE_STMT(m_updateUserSetting);
		FINALIZE_STMT(m_updateBuddySetting);
		FINALIZE_STMT(m_getBuddySetting);
		FINALIZE_STMT(m_setUserOnline);
		FINALIZE_STMT(m_getOnlineUsers);
		FINALIZE_STMT(m_getUsers);
		sqlite3_close(m_db);
	}
}

bool SQLite3Backend::connect() {
	LOG4CXX_INFO(sqlite3Logger, "Opening database " << CONFIG_STRING(m_config, "database.database"));
	if (sqlite3_open(CONFIG_STRING(m_config, "database.database").c_str(), &m_db)) {
		sqlite3_close(m_db);
		return false;
	}

	sqlite3_busy_timeout(m_db, 1500);

	if (createDatabase() == false)
		return false;

	PREP_STMT(m_setUser, "INSERT OR REPLACE INTO " + m_prefix + "users (id, jid, uin, password, language, encoding, last_login, vip) VALUES ((SELECT id FROM " + m_prefix + "users WHERE jid = ?), ?, ?, ?, ?, ?, DATETIME('NOW'), ?)");
	PREP_STMT(m_getUser, "SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?");

	PREP_STMT(m_removeUser, "DELETE FROM " + m_prefix + "users WHERE id=?");
	PREP_STMT(m_removeUserBuddies, "DELETE FROM " + m_prefix + "buddies WHERE user_id=?");
	PREP_STMT(m_removeUserSettings, "DELETE FROM " + m_prefix + "users_settings WHERE user_id=?");
	PREP_STMT(m_removeUserBuddiesSettings, "DELETE FROM " + m_prefix + "buddies_settings WHERE user_id=?");

	PREP_STMT(m_removeBuddy, "DELETE FROM " + m_prefix + "buddies WHERE id=?");
	PREP_STMT(m_removeBuddySettings, "DELETE FROM " + m_prefix + "buddies_settings WHERE buddy_id=?");

	PREP_STMT(m_addBuddy, "INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)");
	PREP_STMT(m_updateBuddy, "UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?");
	PREP_STMT(m_getBuddies, "SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC");
	PREP_STMT(m_getBuddiesSettings, "SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC");
	PREP_STMT(m_updateBuddySetting, "INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)");
	PREP_STMT(m_getBuddySetting, "SELECT type, value FROM " + m_prefix + "buddies_settings WHERE user_id=? AND buddy_id=? AND var=?");
	
	PREP_STMT(m_getUserSetting, "SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=? AND var=?");
	PREP_STMT(m_setUserSetting, "INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)");
	PREP_STMT(m_updateUserSetting, "UPDATE " + m_prefix + "users_settings SET value=? WHERE user_id=? AND var=?");

	PREP_STMT(m_setUserOnline, "UPDATE " + m_prefix + "users SET online=?, last_login=DATETIME('NOW') WHERE id=?");
	PREP_STMT(m_getOnlineUsers, "SELECT jid FROM " + m_prefix + "users WHERE online=1");
	PREP_STMT(m_getUsers, "SELECT jid FROM " + m_prefix + "users");

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
					"  value varchar(4095) NOT NULL,"
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
		// This error is OK, because we try to create buddies table every time
		// to detect if DB is created properly.
		if (errMsg && std::string(errMsg).find("table buddies already exists") == std::string::npos) {
			LOG4CXX_ERROR(sqlite3Logger, errMsg << " during statement " << query);
		}
		sqlite3_free(errMsg);
		return false;
	}
	return true;
}

void SQLite3Backend::setUser(const UserInfo &user) {
	sqlite3_reset(m_setUser);
	sqlite3_bind_text(m_setUser, 1, user.jid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 2, user.jid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 3, user.uin.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 4, user.password.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 5, user.language.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(m_setUser, 6, user.encoding.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int (m_setUser, 7, user.vip);

	if (sqlite3_step(m_setUser) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "setUser query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
	}
}

bool SQLite3Backend::getUser(const std::string &barejid, UserInfo &user) {
// 	SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?
	sqlite3_reset(m_getUser);
	sqlite3_bind_text(m_getUser, 1, barejid.c_str(), -1, SQLITE_TRANSIENT);

	int ret;
	while ((ret = sqlite3_step(m_getUser)) == SQLITE_ROW) {
		user.id = sqlite3_column_int(m_getUser, 0);
		user.jid = (const char *) sqlite3_column_text(m_getUser, 1);
		user.uin = (const char *) sqlite3_column_text(m_getUser, 2);
		user.password = (const char *) sqlite3_column_text(m_getUser, 3);
		user.encoding = (const char *) sqlite3_column_text(m_getUser, 4);
		user.language = (const char *) sqlite3_column_text(m_getUser, 5);
		user.vip = sqlite3_column_int(m_getUser, 6) != 0;
		while ((ret = sqlite3_step(m_getUser)) == SQLITE_ROW) {
		}
		return true;
	}

	if (ret != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "getUser query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
	}

	return false;
}

void SQLite3Backend::setUserOnline(long id, bool online) {
	BEGIN(m_setUserOnline);
	BIND_INT(m_setUserOnline, (int)online);
	BIND_INT(m_setUserOnline, id);
	EXECUTE_STATEMENT(m_setUserOnline, "setUserOnline query");
}

bool SQLite3Backend::getOnlineUsers(std::vector<std::string> &users) {
	sqlite3_reset(m_getOnlineUsers);

	int ret;
	while ((ret = sqlite3_step(m_getOnlineUsers)) == SQLITE_ROW) {
		std::string jid = (const char *) sqlite3_column_text(m_getOnlineUsers, 0);
		users.push_back(jid);
	}

	if (ret != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "getOnlineUsers query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	return true;
}

bool SQLite3Backend::getUsers(std::vector<std::string> &users) {
	sqlite3_reset(m_getUsers);

	int ret;
	while ((ret = sqlite3_step(m_getUsers)) == SQLITE_ROW) {
		std::string jid = (const char *) sqlite3_column_text(m_getUsers, 0);
		users.push_back(jid);
	}

	if (ret != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "getUsers query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	return true;
}

long SQLite3Backend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)"
	std::string groups = StorageBackend::serializeGroups(buddyInfo.groups);
	BEGIN(m_addBuddy);
	BIND_INT(m_addBuddy, userId);
	BIND_STR(m_addBuddy, buddyInfo.legacyName);
	BIND_STR(m_addBuddy, buddyInfo.subscription);
	BIND_STR(m_addBuddy, groups);
	BIND_STR(m_addBuddy, buddyInfo.alias);
	BIND_INT(m_addBuddy, buddyInfo.flags);

	if (sqlite3_step(m_addBuddy) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "addBuddy query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return -1;
	}

	long id = (long) sqlite3_last_insert_rowid(m_db);

// 	INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)
	BEGIN(m_updateBuddySetting);
	BIND_INT(m_updateBuddySetting, userId);
	BIND_INT(m_updateBuddySetting, id);
	BIND_STR(m_updateBuddySetting, buddyInfo.settings.find("icon_hash")->first);
	BIND_INT(m_updateBuddySetting, TYPE_STRING);
	BIND_STR(m_updateBuddySetting, buddyInfo.settings.find("icon_hash")->second.s);

	EXECUTE_STATEMENT(m_updateBuddySetting, "updateBuddySetting query");
	return id;
}

void SQLite3Backend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?
	std::string groups = StorageBackend::serializeGroups(buddyInfo.groups);
	BEGIN(m_updateBuddy);
	BIND_STR(m_updateBuddy, groups);
	BIND_STR(m_updateBuddy, buddyInfo.alias);
	BIND_INT(m_updateBuddy, buddyInfo.flags);
	BIND_STR(m_updateBuddy, buddyInfo.subscription);
	BIND_INT(m_updateBuddy, userId);
	BIND_STR(m_updateBuddy, buddyInfo.legacyName);

	EXECUTE_STATEMENT(m_updateBuddy, "updateBuddy query");

// 	INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)
	BEGIN(m_updateBuddySetting);
	BIND_INT(m_updateBuddySetting, userId);
	BIND_INT(m_updateBuddySetting, buddyInfo.id);
	BIND_STR(m_updateBuddySetting, buddyInfo.settings.find("icon_hash")->first);
	BIND_INT(m_updateBuddySetting, TYPE_STRING);
	BIND_STR(m_updateBuddySetting, buddyInfo.settings.find("icon_hash")->second.s);

	EXECUTE_STATEMENT(m_updateBuddySetting, "updateBuddySetting query");
}

bool SQLite3Backend::getBuddies(long id, std::list<BuddyInfo> &roster) {
//	SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC
	BEGIN(m_getBuddies);
	BIND_INT(m_getBuddies, id);

// 	"SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC"
	BEGIN(m_getBuddiesSettings);
	BIND_INT(m_getBuddiesSettings, id);

	SettingVariableInfo var;
	long buddy_id = -1;
	std::string key;

	int ret;
	int ret2 = -10;
	while ((ret = sqlite3_step(m_getBuddies)) == SQLITE_ROW) {
		BuddyInfo b;
		RESET_GET_COUNTER(m_getBuddies);
		b.id = GET_INT(m_getBuddies);
		b.legacyName = GET_STR(m_getBuddies);
		b.subscription = GET_STR(m_getBuddies);
		b.alias = GET_STR(m_getBuddies);
		std::string groups = GET_STR(m_getBuddies);
		b.groups = StorageBackend::deserializeGroups(groups);
		b.flags = GET_INT(m_getBuddies);

		if (buddy_id == b.id) {
			std::cout << "Adding buddy info " << key << "\n";
			b.settings[key] = var;
			buddy_id = -1;
		}

		while (buddy_id == -1 && ret2 != SQLITE_DONE && ret2 != SQLITE_ERROR && (ret2 = sqlite3_step(m_getBuddiesSettings)) == SQLITE_ROW) {
			RESET_GET_COUNTER(m_getBuddiesSettings);
			buddy_id = GET_INT(m_getBuddiesSettings);
			
			var.type = GET_INT(m_getBuddiesSettings);
			key = GET_STR(m_getBuddiesSettings);
			std::string val = GET_STR(m_getBuddiesSettings);

			switch (var.type) {
				case TYPE_BOOLEAN:
					var.b = atoi(val.c_str());
					break;
				case TYPE_STRING:
					var.s = val;
					break;
				default:
					if (buddy_id == b.id) {
						buddy_id = -1;
					}
					continue;
					break;
			}
			if (buddy_id == b.id) {
				std::cout << "Adding buddy info " << key << "=" << val << "\n";
				b.settings[key] = var;
				buddy_id = -1;
			}
		}

// 		if (ret != SQLITE_DONE) {
// 			LOG4CXX_ERROR(sqlite3Logger, "getBuddiesSettings query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
// 			return false;
// 		}

		roster.push_back(b);
	}

	if (ret != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "getBuddies query "<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	if (ret2 != SQLITE_DONE) {
		if (ret2 == SQLITE_ERROR) {
			LOG4CXX_ERROR(sqlite3Logger, "getBuddiesSettings query "<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
			return false;
		}

		while ((ret2 = sqlite3_step(m_getBuddiesSettings)) == SQLITE_ROW) {
		}

		if (ret2 != SQLITE_DONE) {
			LOG4CXX_ERROR(sqlite3Logger, "getBuddiesSettings query "<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
			return false;
		}
	}
	
	return true;
}

void SQLite3Backend::removeBuddy(long id) {
	sqlite3_reset(m_removeBuddy);
	sqlite3_bind_int(m_removeBuddy, 1, id);
	if (sqlite3_step(m_removeBuddy) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeBuddy query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return;
	}

	sqlite3_reset(m_removeBuddySettings);
	sqlite3_bind_int(m_removeBuddySettings, 1, id);
	if (sqlite3_step(m_removeBuddySettings) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeBuddySettings query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return;
	}
}

bool SQLite3Backend::removeUser(long id) {
	sqlite3_reset(m_removeUser);
	sqlite3_bind_int(m_removeUser, 1, id);
	if (sqlite3_step(m_removeUser) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeUser query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserSettings);
	sqlite3_bind_int(m_removeUserSettings, 1, id);
	if (sqlite3_step(m_removeUserSettings) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeUserSettings query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserBuddies);
	sqlite3_bind_int(m_removeUserBuddies, 1, id);
	if (sqlite3_step(m_removeUserBuddies) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeUserBuddies query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	sqlite3_reset(m_removeUserBuddiesSettings);
	sqlite3_bind_int(m_removeUserBuddiesSettings, 1, id);
	if (sqlite3_step(m_removeUserBuddiesSettings) != SQLITE_DONE) {
		LOG4CXX_ERROR(sqlite3Logger, "removeUserBuddiesSettings query"<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));
		return false;
	}

	return true;
}

void SQLite3Backend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {
	BEGIN(m_getUserSetting);
	BIND_INT(m_getUserSetting, id);
	BIND_STR(m_getUserSetting, variable);
	if (sqlite3_step(m_getUserSetting) != SQLITE_ROW) {
		BEGIN(m_setUserSetting);
		BIND_INT(m_setUserSetting, id);
		BIND_STR(m_setUserSetting, variable);
		BIND_INT(m_setUserSetting, type);
		BIND_STR(m_setUserSetting, value);
		EXECUTE_STATEMENT(m_setUserSetting, "m_setUserSetting");
	}
	else {
		type = GET_INT(m_getUserSetting);
		value = GET_STR(m_getUserSetting);
	}

	int ret;
	while ((ret = sqlite3_step(m_getUserSetting)) == SQLITE_ROW) {
	}
}

void SQLite3Backend::updateUserSetting(long id, const std::string &variable, const std::string &value) {
	BEGIN(m_updateUserSetting);
	BIND_STR(m_updateUserSetting, value);
	BIND_INT(m_updateUserSetting, id);
	BIND_STR(m_updateUserSetting, variable);
	EXECUTE_STATEMENT(m_updateUserSetting, "m_updateUserSetting");
}

void SQLite3Backend::getBuddySetting(long userId, long buddyId, const std::string &variable, int &type, std::string &value) {
	BEGIN(m_getBuddySetting);
	BIND_INT(m_getBuddySetting, userId);
	BIND_INT(m_getBuddySetting, buddyId);
	BIND_STR(m_getBuddySetting, variable);
	if (sqlite3_step(m_getBuddySetting) == SQLITE_ROW) {
		type = GET_INT(m_getBuddySetting);
		value = GET_STR(m_getBuddySetting);
	}

	int ret;
	while ((ret = sqlite3_step(m_getBuddySetting)) == SQLITE_ROW) {
	}
}

void SQLite3Backend::updateBuddySetting(long userId, long buddyId, const std::string &variable, int type, const std::string &value) {
	BEGIN(m_updateBuddySetting);
	BIND_INT(m_updateBuddySetting, userId);
	BIND_INT(m_updateBuddySetting, buddyId);
	BIND_STR(m_updateBuddySetting, variable);
	BIND_INT(m_updateBuddySetting, type);
	BIND_STR(m_updateBuddySetting, value);
	EXECUTE_STATEMENT(m_updateBuddySetting, "m_updateBuddySetting");
}

void SQLite3Backend::beginTransaction() {
	exec("BEGIN TRANSACTION;");
}

void SQLite3Backend::commitTransaction() {
	exec("COMMIT TRANSACTION;");
}

}

#endif
