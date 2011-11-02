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

#ifdef WITH_MYSQL

#include "transport/mysqlbackend.h"
#include "transport/util.h"
#include <boost/bind.hpp>
#include "log4cxx/logger.h"

using namespace log4cxx;

#define MYSQL_DB_VERSION 2
#define CHECK_DB_RESPONSE(stmt) \
	if(stmt) { \
		sqlite3_EXEC(m_db, "ROLLBACK;", NULL, NULL, NULL); \
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

#define EXEC(STMT, METHOD) \
	{\
	int ret = STMT->execute(); \
	if (ret == 0) \
		exec_ok = true; \
	else if (ret == 2013) { \
		LOG4CXX_INFO(logger, "MySQL connection lost. Reconnecting...");\
		disconnect(); \
		connect(); \
		return METHOD; \
	} \
	else \
		exec_ok = false; \
	}

using namespace boost;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("MySQLBackend");
static bool exec_ok;

MySQLBackend::Statement::Statement(MYSQL *conn, const std::string &format, const std::string &statement) {
	m_resultOffset = -1;
	m_conn = conn;
	m_offset = 0;
	m_string = statement;
	m_stmt = mysql_stmt_init(conn);
	if (mysql_stmt_prepare(m_stmt, statement.c_str(), statement.size())) {
		LOG4CXX_ERROR(logger, statement << " " << mysql_error(conn));
		return;
	}

	for (int i = 0; i < format.length() && m_resultOffset == -1; i++) {
		switch (format.at(i)) {
			case 's':
				m_params.resize(m_params.size() + 1);
				memset(&m_params.back(), 0, sizeof(MYSQL_BIND));

				m_params.back().buffer_type= MYSQL_TYPE_STRING;
				m_params.back().buffer= (char *) malloc(sizeof(char) * 4096);
				m_params.back().buffer_length= 4096;
				m_params.back().is_null= 0;
				m_params.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
			case 'i':
				m_params.resize(m_params.size() + 1);
				memset(&m_params.back(), 0, sizeof(MYSQL_BIND));

				m_params.back().buffer_type= MYSQL_TYPE_LONG;
				m_params.back().buffer= (int *) malloc(sizeof(int));
				m_params.back().is_null= 0;
				m_params.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
			case 'b':
				m_params.resize(m_params.size() + 1);
				memset(&m_params.back(), 0, sizeof(MYSQL_BIND));

				m_params.back().buffer_type= MYSQL_TYPE_TINY;
				m_params.back().buffer= (int *) malloc(sizeof(int));
				m_params.back().is_null= 0;
				m_params.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
			case '|':
				m_resultOffset = i;
				break;
		}
	}

	for (int i = m_resultOffset; i >= 0 && i < format.length(); i++) {
		switch (format.at(i)) {
			case 's':
				m_results.resize(m_results.size() + 1);
				memset(&m_results.back(), 0, sizeof(MYSQL_BIND));

				m_results.back().buffer_type= MYSQL_TYPE_STRING;
				m_results.back().buffer= (char *) malloc(sizeof(char) * 4096);
				m_results.back().buffer_length= 4096;
				m_results.back().is_null= 0;
				m_results.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
			case 'i':
				m_results.resize(m_results.size() + 1);
				memset(&m_results.back(), 0, sizeof(MYSQL_BIND));

				m_results.back().buffer_type= MYSQL_TYPE_LONG;
				m_results.back().buffer= (int *) malloc(sizeof(int));
				m_results.back().is_null= 0;
				m_results.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
			case 'b':
				m_results.resize(m_results.size() + 1);
				memset(&m_results.back(), 0, sizeof(MYSQL_BIND));

				m_results.back().buffer_type= MYSQL_TYPE_TINY;
				m_results.back().buffer= (int *) malloc(sizeof(int));
				m_results.back().is_null= 0;
				m_results.back().length= (unsigned long *) malloc(sizeof(unsigned long));
				break;
		}
	}

	if (mysql_stmt_bind_param(m_stmt, &m_params.front())) {
		LOG4CXX_ERROR(logger, statement << " " << mysql_error(conn));
	}

	if (m_resultOffset < 0)
		m_resultOffset = format.size();
	else {
		if (mysql_stmt_bind_result(m_stmt, &m_results.front())) {
			LOG4CXX_ERROR(logger, statement << " " << mysql_error(conn));
		}
	}
	m_resultOffset = 0;
}

MySQLBackend::Statement::~Statement() {
	for (int i = 0; i < m_params.size(); i++) {
		free(m_params[i].buffer);
		free(m_params[i].length);
	}
	for (int i = 0; i < m_results.size(); i++) {
		free(m_results[i].buffer);
		free(m_results[i].length);
	}
	FINALIZE_STMT(m_stmt);
}

int MySQLBackend::Statement::execute() {
	// If statement has some input and doesn't have any output, we have
	// to clear the offset now, because operator>> will not be called.
	m_offset = 0;
	m_resultOffset = 0;
	int ret;

	if ((ret = mysql_stmt_execute(m_stmt)) != 0) {
		LOG4CXX_ERROR(logger, m_string << " " << mysql_stmt_error(m_stmt) << "; " << mysql_error(m_conn));
		return mysql_stmt_errno(m_stmt);
	}
	return 0;
}

int MySQLBackend::Statement::fetch() {
	return mysql_stmt_fetch(m_stmt);
}

template <typename T>
MySQLBackend::Statement& MySQLBackend::Statement::operator << (const T& t) {
	if (m_offset >= m_params.size())
		return *this;
	int *data = (int *) m_params[m_offset].buffer;
	*data = (int) t;

// 	LOG4CXX_INFO(logger, "adding " << m_offset << ":" << (int) t);
	m_offset++;
	return *this;
}

MySQLBackend::Statement& MySQLBackend::Statement::operator << (const std::string& str) {
	if (m_offset >= m_params.size())
		return *this;
// 	LOG4CXX_INFO(logger, "adding " << m_offset << ":" << str << "(" << str.size() << ")");
	strncpy((char*) m_params[m_offset].buffer, str.c_str(), 4096);
	*m_params[m_offset].length = str.size();
	m_offset++;
	return *this;
}

template <typename T>
MySQLBackend::Statement& MySQLBackend::Statement::operator >> (T& t) {
	if (m_resultOffset > m_results.size())
		return *this;

	if (!m_results[m_resultOffset].is_null) {
		int *data = (int *) m_results[m_resultOffset].buffer;
		t = (int) *data;
// 		std::cout << "getting " << m_resultOffset << " " << (int) t << "\n";
	}

	if (++m_resultOffset == m_results.size())
		m_resultOffset = 0;
	return *this;
}

MySQLBackend::Statement& MySQLBackend::Statement::operator >> (std::string& t) {
// 	std::cout << "getting " << m_resultOffset << "\n";
	if (m_resultOffset > m_results.size())
		return *this;

	if (!m_results[m_resultOffset].is_null) {
		t = (char *) m_results[m_resultOffset].buffer;
	}

	if (++m_resultOffset == m_results.size())
		m_resultOffset = 0;
	return *this;
}

MySQLBackend::MySQLBackend(Config *config) {
	m_config = config;
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
	mysql_init(&m_conn);
	my_bool my_true = 1;
	mysql_options(&m_conn, MYSQL_OPT_RECONNECT, &my_true);
}

MySQLBackend::~MySQLBackend(){
	disconnect();
}

void MySQLBackend::disconnect() {
	LOG4CXX_INFO(logger, "Disconnecting");
	delete m_setUser;
	delete m_getUser;
	delete m_removeUser;
	delete m_removeUserBuddies;
	delete m_removeUserSettings;
	delete m_removeUserBuddiesSettings;
	delete m_addBuddy;
	delete m_updateBuddy;
	delete m_getBuddies;
	delete m_getBuddiesSettings;
	delete m_getUserSetting;
	delete m_setUserSetting;
	delete m_updateUserSetting;
	delete m_updateBuddySetting;
	delete m_setUserOnline;
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

	createDatabase();

	m_setUser = new Statement(&m_conn, "sssssbs", "INSERT INTO " + m_prefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES (?, ?, ?, ?, ?, NOW(), ?) ON DUPLICATE KEY UPDATE password=?");
	m_getUser = new Statement(&m_conn, "s|isssssb", "SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid=?");

	m_removeUser = new Statement(&m_conn, "i", "DELETE FROM " + m_prefix + "users WHERE id=?");
	m_removeUserBuddies = new Statement(&m_conn, "i", "DELETE FROM " + m_prefix + "buddies WHERE user_id=?");
	m_removeUserSettings = new Statement(&m_conn, "i", "DELETE FROM " + m_prefix + "users_settings WHERE user_id=?");
	m_removeUserBuddiesSettings = new Statement(&m_conn, "i", "DELETE FROM " + m_prefix + "buddies_settings WHERE user_id=?");

	m_addBuddy = new Statement(&m_conn, "issssi", "INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)");
	m_updateBuddy = new Statement(&m_conn, "ssisis", "UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?");
	m_getBuddies = new Statement(&m_conn, "i|issssi", "SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC");
	m_getBuddiesSettings = new Statement(&m_conn, "i|iiss", "SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC");
	m_updateBuddySetting = new Statement(&m_conn, "iisiss", "INSERT INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE value=?");
	
	m_getUserSetting = new Statement(&m_conn, "is|is", "SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=? AND var=?");
	m_setUserSetting = new Statement(&m_conn, "isis", "INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)");
	m_updateUserSetting = new Statement(&m_conn, "sis", "UPDATE " + m_prefix + "users_settings SET value=? WHERE user_id=? AND var=?");

	m_setUserOnline = new Statement(&m_conn, "bi", "UPDATE " + m_prefix + "users SET online=?, last_login=NOW()  WHERE id=?");

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

		exec("INSERT IGNORE INTO db_version (ver) VALUES ('2');");
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
	std::string encrypted = user.password;
	if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
		encrypted = Util::encryptPassword(encrypted, CONFIG_STRING(m_config, "database.encryption_key"));
	}
	*m_setUser << user.jid << user.uin << encrypted << user.language << user.encoding << user.vip << user.password;
	EXEC(m_setUser, setUser(user));
}

bool MySQLBackend::getUser(const std::string &barejid, UserInfo &user) {
	*m_getUser << barejid;
	EXEC(m_getUser, getUser(barejid, user));
	if (!exec_ok)
		return false;

	int ret = false;
	while (m_getUser->fetch() == 0) {
		ret = true;
		*m_getUser >> user.id >> user.jid >> user.uin >> user.password >> user.encoding >> user.language >> user.vip;

		if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
			user.password = Util::decryptPassword(user.password, CONFIG_STRING(m_config, "database.encryption_key"));
		}
	}

	return ret;
}

void MySQLBackend::setUserOnline(long id, bool online) {
	*m_setUserOnline << online << id;
	EXEC(m_setUserOnline, setUserOnline(id, online));
}

long MySQLBackend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)"
	*m_addBuddy << userId << buddyInfo.legacyName << buddyInfo.subscription;
	*m_addBuddy << (buddyInfo.groups.size() == 0 ? "" : buddyInfo.groups[0]);
	*m_addBuddy << buddyInfo.alias << buddyInfo.flags;

	EXEC(m_addBuddy, addBuddy(userId, buddyInfo));

	long id = (long) mysql_insert_id(&m_conn);

// 	INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)
	if (!buddyInfo.settings.find("icon_hash")->second.s.empty()) {
		*m_updateBuddySetting << userId << id << buddyInfo.settings.find("icon_hash")->first << (int) TYPE_STRING << buddyInfo.settings.find("icon_hash")->second.s << buddyInfo.settings.find("icon_hash")->second.s;
		EXEC(m_updateBuddySetting, addBuddy(userId, buddyInfo));
	}

	return id;
}

void MySQLBackend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?"
	*m_updateBuddy << (buddyInfo.groups.size() == 0 ? "" : buddyInfo.groups[0]);
	*m_updateBuddy << buddyInfo.alias << buddyInfo.flags << buddyInfo.subscription;
	*m_updateBuddy << userId << buddyInfo.legacyName;

	EXEC(m_updateBuddy, updateBuddy(userId, buddyInfo));
}

bool MySQLBackend::getBuddies(long id, std::list<BuddyInfo> &roster) {
//	SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC
	*m_getBuddies << id;

// 	"SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC"
	*m_getBuddiesSettings << id;

	SettingVariableInfo var;
	long buddy_id = -1;
	std::string key;

	EXEC(m_getBuddies, getBuddies(id, roster));
	if (!exec_ok)
		return false;

	while (m_getBuddies->fetch() == 0) {
		BuddyInfo b;

		std::string group;
		*m_getBuddies >> b.id >> b.legacyName >> b.subscription >> b.alias >> group >> b.flags;

		if (!group.empty())
			b.groups.push_back(group);

		roster.push_back(b);
	}

	EXEC(m_getBuddiesSettings, getBuddies(id, roster));
	if (!exec_ok)
		return false;

	BOOST_FOREACH(BuddyInfo &b, roster) {
		if (buddy_id == b.id) {
// 			std::cout << "Adding buddy info setting " << key << "\n";
			b.settings[key] = var;
			buddy_id = -1;
		}

		while(buddy_id == -1 && m_getBuddiesSettings->fetch() == 0) {
			std::string val;
			*m_getBuddiesSettings >> buddy_id >> var.type >> key >> val;

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
// 				std::cout << "Adding buddy info setting " << key << "=" << val << "\n";
				b.settings[key] = var;
				buddy_id = -1;
			}
		}
	}

	while(m_getBuddiesSettings->fetch() == 0) {
		// TODO: probably remove those settings, because there's no buddy for them.
		// It should not happend, but one never know...
	}
	
	return true;
}

bool MySQLBackend::removeUser(long id) {
	*m_removeUser << (int) id;
	EXEC(m_removeUser, removeUser(id));
	if (!exec_ok)
		return false;

	*m_removeUserSettings << (int) id;
	EXEC(m_removeUserSettings, removeUser(id));
	if (!exec_ok)
		return false;

	*m_removeUserBuddies << (int) id;
	EXEC(m_removeUserBuddies, removeUser(id));
	if (!exec_ok)
		return false;

	*m_removeUserBuddiesSettings << (int) id;
	EXEC(m_removeUserBuddiesSettings, removeUser(id));
	if (!exec_ok)
		return false;

	return true;
}

void MySQLBackend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {
// 	"SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=? AND var=?"
	*m_getUserSetting << id << variable;
	EXEC(m_getUserSetting, getUserSetting(id, variable, type, value));
	if (m_getUserSetting->fetch() != 0) {
// 		"INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)"
		*m_setUserSetting << id << variable << type << value;
		EXEC(m_setUserSetting, getUserSetting(id, variable, type, value));
	}
	else {
		*m_getUserSetting >> type >> value;
	}
}

void MySQLBackend::updateUserSetting(long id, const std::string &variable, const std::string &value) {
// 	"UPDATE " + m_prefix + "users_settings SET value=? WHERE user_id=? AND var=?"
	*m_updateUserSetting << value << id << variable;
	EXEC(m_updateUserSetting, updateUserSetting(id, variable, value));
}

void MySQLBackend::beginTransaction() {
	exec("START TRANSACTION;");
}

void MySQLBackend::commitTransaction() {
	exec("COMMIT;");
}

}

#endif
