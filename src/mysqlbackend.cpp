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

#define SQLITE_DB_VERSION 3
#define CHECK_DB_RESPONSE(stmt) \
	if(stmt) { \
		sqlite3_exec(m_db, "ROLLBACK;", NULL, NULL, NULL); \
		return 0; \
	}

// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	if(sqlite3_prepare_v2(m_db, std::string(str).c_str(), -1, &sql, NULL)) { \
		LOG4CXX_ERROR(logger, str<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db))); \
		return false; \
	}

// Finalize the prepared statement
#define FINALIZE_STMT(prep) \
	if(prep != NULL) { \
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
#define EXECUTE_STATEMENT(STATEMENT, NAME) 	if(sqlite3_step(STATEMENT) != SQLITE_DONE) {\
		LOG4CXX_ERROR(logger, NAME<< (sqlite3_errmsg(m_db) == NULL ? "" : sqlite3_errmsg(m_db)));\
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

	return true;
}

bool MySQLBackend::createDatabase() {
	return true;
}

bool MySQLBackend::exec(const std::string &query) {
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
