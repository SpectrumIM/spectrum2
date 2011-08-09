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

#include "transport/mysqlbackend.h"
#include <boost/bind.hpp>
#include "log4cxx/logger.h"

using namespace log4cxx;

#define MYSQL_DB_VERSION 2
#define CHECK_DB_RESPONSE(stmt) \
	if(stmt) { \
		sqlite3_exec(m_db, "ROLLBACK;", NULL, NULL, NULL); \
		return 0; \
	}

// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	sql = mysql_stmt_init(&m_conn);\
	if (mysql_stmt_prepare(sql, std::string(str).c_str(), std::string(str).size())) {\
		LOG4CXX_ERROR(logger, str << " " << mysql_error(&m_conn)); \
		return false; \
	}

// Finalize the prepared statement
#define FINALIZE_STMT(prep) \
	if(prep != NULL) { \
		mysql_stmt_close(prep); \
	}
	
#define BEGIN(STATEMENT, SIZE) 	MYSQL_BIND STATEMENT##_bind[SIZE]; \
							memset(STATEMENT##_bind, 0, sizeof(STATEMENT##_bind)); \
							int STATEMENT##_id = 1;\
							int STATEMENT##_id_get = 0;\
							(void)STATEMENT##_id_get;

#define BIND_INT(STATEMENT, VARIABLE) STATEMENT##_bind[STATEMENT##_id].buffer_type= MYSQL_TYPE_LONG;\
							STATEMENT##_bind[STATEMENT##_id].buffer= (char *)&VARIABLE;\
							STATEMENT##_bind[STATEMENT##_id].is_null= 0;\
							STATEMENT##_bind[STATEMENT##_id++].length= 0;
#define BIND_STR(STATEMENT, VARIABLE) STATEMENT##_bind[STATEMENT##_id].buffer_type= MYSQL_TYPE_STRING;\
							STATEMENT##_bind[STATEMENT##_id].buffer= VARIABLE.c_str();\
							STATEMENT##_bind[STATEMENT##_id].buffer_length= STRING_SIZE;\
							STATEMENT##_bind[STATEMENT##_id].is_null= 0;\
							STATEMENT##_bind[STATEMENT##_id++].length= VARIABLE.size();
#define RESET_GET_COUNTER(STATEMENT)	STATEMENT##_id_get = 0;
#define GET_INT(STATEMENT)	sqlite3_column_int(STATEMENT, STATEMENT##_id_get++)
#define GET_STR(STATEMENT)	(const char *) sqlite3_column_text(STATEMENT, STATEMENT##_id_get++)
#define EXECUTE_STATEMENT(STATEMENT, NAME) if (mysql_stmt_bind_param(STATEMENT, STATEMENT##_bind)) { \
		LOG4CXX_ERROR(logger, NAME << " " << mysql_error(&m_conn)); \
	} \
	if (mysql_stmt_execute(STATEMENT)) { \
		LOG4CXX_ERROR(logger, NAME << " " << mysql_error(&m_conn)); \
	}

using namespace boost;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("MySQLBackend");

MySQLBackend::MySQLBackend(Config *config) {
	m_config = config;
	mysql_init(&m_conn);
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
}

MySQLBackend::~MySQLBackend(){
	FINALIZE_STMT(m_setUser);
	FINALIZE_STMT(m_getUser);
	FINALIZE_STMT(m_removeUser);
	FINALIZE_STMT(m_removeUserBuddies);
	FINALIZE_STMT(m_removeUserSettings);
	FINALIZE_STMT(m_removeUserBuddiesSettings);
	FINALIZE_STMT(m_addBuddy);
	FINALIZE_STMT(m_updateBuddy);
	FINALIZE_STMT(m_getBuddies);
	FINALIZE_STMT(m_getBuddiesSettings);
	FINALIZE_STMT(m_getUserSetting);
	FINALIZE_STMT(m_setUserSetting);
	FINALIZE_STMT(m_updateUserSetting);
	FINALIZE_STMT(m_updateBuddySetting);
	mysql_close(&m_conn);
}

bool MySQLBackend::connect() {
	LOG4CXX_INFO(logger, "Connecting MySQL server " << CONFIG_STRING(m_config, "database.server") << ", user " <<
		CONFIG_STRING(m_config, "database.user") << ", database " << CONFIG_STRING(m_config, "database.database") <<
		", port " << CONFIG_INT(m_config, "database.port")
	);
	
	if (!mysql_real_connect(&m_conn, CONFIG_STRING(m_config, "database.server").c_str(),
					   CONFIG_STRING(m_config, "database.user").c_str(),
					   CONFIG_STRING(m_config, "database.password").c_str(),
					   CONFIG_STRING(m_config, "database.database").c_str(),
					   CONFIG_INT(m_config, "database.port"), NULL, 0)) {
		LOG4CXX_ERROR(logger, "Can't connect database: " << mysql_error(&m_conn));
		return false;
	}

	PREP_STMT(m_setUser, "INSERT INTO " + m_prefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES (?, ?, ?, ?, ?, DATETIME('NOW'), ?)");
	PREP_STMT(m_getUser, "SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?");

	PREP_STMT(m_removeUser, "DELETE FROM " + m_prefix + "users WHERE id=?");
	PREP_STMT(m_removeUserBuddies, "DELETE FROM " + m_prefix + "buddies WHERE user_id=?");
	PREP_STMT(m_removeUserSettings, "DELETE FROM " + m_prefix + "users_settings WHERE user_id=?");
	PREP_STMT(m_removeUserBuddiesSettings, "DELETE FROM " + m_prefix + "buddies_settings WHERE user_id=?");

	PREP_STMT(m_addBuddy, "INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)");
	PREP_STMT(m_updateBuddy, "UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?");
	PREP_STMT(m_getBuddies, "SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC");
	PREP_STMT(m_getBuddiesSettings, "SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC");
	PREP_STMT(m_updateBuddySetting, "INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)");
	
	PREP_STMT(m_getUserSetting, "SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=? AND var=?");
	PREP_STMT(m_setUserSetting, "INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)");
	PREP_STMT(m_updateUserSetting, "UPDATE " + m_prefix + "users_settings SET value=? WHERE user_id=? AND var=?");

	return true;
}

bool MySQLBackend::createDatabase() {
	int not_exist = exec("CREATE TABLE IF NOT EXISTS `" + m_prefix + "buddies` ("
							"`id` int(10) unsigned NOT NULL auto_increment,"
							"`user_id` int(10) unsigned NOT NULL,"
							"`uin` varchar(255) collate utf8_bin NOT NULL,"
							"`subscription` enum('to','from','both','ask','none') collate utf8_bin NOT NULL,"
							"`nickname` varchar(255) collate utf8_bin NOT NULL,"
							"`groups` varchar(255) collate utf8_bin NOT NULL,"
							"`flags` smallint(4) NOT NULL DEFAULT '0',"
							"PRIMARY KEY (`id`),"
							"UNIQUE KEY `user_id` (`user_id`,`uin`)"
						") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;");

	if (not_exist) {
		exec("CREATE TABLE IF NOT EXISTS `" + m_prefix + "buddies_settings` ("
				"`user_id` int(10) unsigned NOT NULL,"
				"`buddy_id` int(10) unsigned NOT NULL,"
				"`var` varchar(50) collate utf8_bin NOT NULL,"
				"`type` smallint(4) unsigned NOT NULL,"
				"`value` varchar(255) collate utf8_bin NOT NULL,"
				"PRIMARY KEY (`buddy_id`,`var`),"
				"KEY `user_id` (`user_id`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;");
 
		exec("CREATE TABLE IF NOT EXISTS `" + m_prefix + "users` ("
				"`id` int(10) unsigned NOT NULL auto_increment,"
				"`jid` varchar(255) collate utf8_bin NOT NULL,"
				"`uin` varchar(4095) collate utf8_bin NOT NULL,"
				"`password` varchar(255) collate utf8_bin NOT NULL,"
				"`language` varchar(25) collate utf8_bin NOT NULL,"
				"`encoding` varchar(50) collate utf8_bin NOT NULL default 'utf8',"
				"`last_login` datetime,"
				"`vip` tinyint(1) NOT NULL  default '0',"
				"`online` tinyint(1) NOT NULL  default '0',"
				"PRIMARY KEY (`id`),"
				"UNIQUE KEY `jid` (`jid`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;");

		exec("CREATE TABLE IF NOT EXISTS `" + m_prefix + "users_settings` ("
				"`user_id` int(10) unsigned NOT NULL,"
				"`var` varchar(50) collate utf8_bin NOT NULL,"
				"`type` smallint(4) unsigned NOT NULL,"
				"`value` varchar(255) collate utf8_bin NOT NULL,"
				"PRIMARY KEY (`user_id`,`var`),"
				"KEY `user_id` (`user_id`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;");

		exec("CREATE TABLE IF NOT EXISTS `" + m_prefix + "db_version` ("
				"`ver` int(10) unsigned NOT NULL default '1',"
				"UNIQUE KEY `ver` (`ver`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;");

		exec("INSERT INTO db_version (ver) VALUES ('2');");
	}

	return true;
}

bool MySQLBackend::exec(const std::string &query) {
	if (mysql_query(&m_conn, query.c_str())) {
		LOG4CXX_ERROR(logger, query << " " << mysql_error(&m_conn));
		return false;
	}
	return true;
}

void MySQLBackend::setUser(const UserInfo &user) {
}

bool MySQLBackend::getUser(const std::string &barejid, UserInfo &user) {
	return false;
}

void MySQLBackend::setUserOnline(long id, bool online) {
	
}

long MySQLBackend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
	return 1;
}

void MySQLBackend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
}

bool MySQLBackend::getBuddies(long id, std::list<BuddyInfo> &roster) {

	return true;
}

bool MySQLBackend::removeUser(long id) {

	return true;
}

void MySQLBackend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {

}

void MySQLBackend::updateUserSetting(long id, const std::string &variable, const std::string &value) {

}

void MySQLBackend::beginTransaction() {
// 	exec("BEGIN TRANSACTION;");
}

void MySQLBackend::commitTransaction() {
// 	exec("COMMIT TRANSACTION;");
}

}
