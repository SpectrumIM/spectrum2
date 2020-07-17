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

#ifdef WITH_PQXX

#include "transport/PQXXBackend.h"
#include "transport/Util.h"
#include <boost/bind.hpp>
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

static LoggerPtr pxxLogger = Logger::getLogger("PQXXBackend");

PQXXBackend::PQXXBackend(Config *config) {
	m_config = config;
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
}

PQXXBackend::~PQXXBackend(){
	disconnect();
}

void PQXXBackend::disconnect() {
	LOG4CXX_INFO(pxxLogger, "Disconnecting");

	delete m_conn;
}

bool PQXXBackend::connect() {
	std::string connection_str;
	connection_str = CONFIG_STRING_DEFAULTED(m_config, "database.connectionstring", "");
	if (connection_str.empty()) {
		LOG4CXX_INFO(pxxLogger, "Connecting PostgreSQL server " << CONFIG_STRING(m_config, "database.server") << ", user " <<
			CONFIG_STRING(m_config, "database.user") << ", dbname " << CONFIG_STRING(m_config, "database.database") <<
			", port " << CONFIG_INT(m_config, "database.port")
		);
		connection_str = "dbname=";
		connection_str += CONFIG_STRING(m_config, "database.database");
		if (!CONFIG_STRING(m_config, "database.server").empty()) {
			connection_str += " host=" + CONFIG_STRING(m_config, "database.server");
		}
		if (!CONFIG_STRING(m_config, "database.user").empty()) {
			connection_str += " user=" + CONFIG_STRING(m_config, "database.user");
		}
		if (!CONFIG_STRING(m_config, "database.password").empty()) {
			connection_str += " password=" + CONFIG_STRING(m_config, "database.password");
		}
		if (CONFIG_INT(m_config, "database.port") != 0) {
			connection_str += " port=" + boost::lexical_cast<std::string>(CONFIG_INT(m_config, "database.port"));
		}
	}
	else {
		LOG4CXX_INFO(pxxLogger, "Connecting PostgreSQL server via provided connection string.");
	}

	try {
		m_conn = new pqxx::connection(connection_str);
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
		return false;
	}

	createDatabase();

	return true;
}

bool PQXXBackend::createDatabase() {
	
	int exist = exec("SELECT * FROM " + m_prefix + "buddies_settings LIMIT 1;", false);

	if (!exist) {
		exec("CREATE TABLE " + m_prefix + "buddies_settings ("
				"user_id integer NOT NULL,"
				"buddy_id integer NOT NULL,"
				"var varchar(50) NOT NULL,"
				"type smallint NOT NULL,"
				"value varchar(255) NOT NULL,"
				"PRIMARY KEY (buddy_id,var)"
			");");
		
		exec("CREATE TYPE Subscription AS ENUM ('to','from','both','ask','none');");
		exec("CREATE TABLE " + m_prefix + "buddies ("
							"id SERIAL,"
							"user_id integer NOT NULL,"
							"uin varchar(255) NOT NULL,"
							"subscription Subscription NOT NULL,"
							"nickname varchar(255) NOT NULL,"
							"groups varchar(255) NOT NULL,"
							"flags smallint NOT NULL DEFAULT '0',"
							"PRIMARY KEY (id),"
							"UNIQUE (user_id,uin)"
						");");
 
		exec("CREATE TABLE " + m_prefix + "users ("
				"id SERIAL,"
				"jid varchar(255) NOT NULL,"
				"uin varchar(4095) NOT NULL,"
				"password varchar(255) NOT NULL,"
				"language varchar(25) NOT NULL,"
				"encoding varchar(50) NOT NULL default 'utf8',"
				"last_login timestamp,"
				"vip boolean NOT NULL  default 'false',"
				"online boolean NOT NULL  default 'false',"
				"PRIMARY KEY (id),"
				"UNIQUE (jid)"
			");");

		exec("CREATE TABLE " + m_prefix + "users_settings ("
				"user_id integer NOT NULL,"
				"var varchar(50) NOT NULL,"
				"type smallint NOT NULL,"
				"value varchar(255) NOT NULL,"
				"PRIMARY KEY (user_id,var)"
			");");

		exec("CREATE TABLE " + m_prefix + "db_version ("
				"ver integer NOT NULL default '1',"
				"UNIQUE (ver)"
			");");

 		exec("INSERT INTO " + m_prefix + "db_version (ver) VALUES ('1');");
	}

	return true;
}

template<typename T>
std::string PQXXBackend::quote(pqxx::nontransaction &txn, const T &t) {
	return "'" + txn.esc(pqxx::to_string(t)) + "'";
}

bool PQXXBackend::exec(const std::string &query, bool show_error) {
	pqxx::nontransaction txn(*m_conn);
	return exec(txn, query, show_error);
}

bool PQXXBackend::exec(pqxx::nontransaction &txn, const std::string &query, bool show_error) {
	try {
		txn.exec(query);
		txn.commit();
	}
	catch (std::exception& e) {
		if (show_error)
			LOG4CXX_ERROR(pxxLogger, e.what());
		return false;
	}
	return true;
}

void PQXXBackend::setUser(const UserInfo &user) {
	std::string encrypted = user.password;
	if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
		encrypted = StorageBackend::encryptPassword(encrypted, CONFIG_STRING(m_config, "database.encryption_key"));
	}
	try {
		pqxx::nontransaction txn(*m_conn);
		txn.exec("INSERT INTO " + m_prefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES "
			+ "(" + quote(txn, user.jid) + ","
			+ quote(txn, user.uin) + ","
			+ quote(txn, encrypted) + ","
			+ quote(txn, user.language) + ","
			+ quote(txn, user.encoding) + ","
			+ "NOW(),"
			+ (user.vip ? "'true'" : "'false'") +")");
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
}

bool PQXXBackend::getUser(const std::string &barejid, UserInfo &user) {
	try {
		pqxx::nontransaction txn(*m_conn);

		pqxx::result r = txn.exec("SELECT id, jid, uin, password, encoding, language, vip FROM " + m_prefix + "users WHERE jid="
			+ quote(txn, barejid));

		if (r.size() == 0) {
			return false;
		}

		user.id = r[0][0].as<int>();
		user.jid = r[0][1].as<std::string>();
		user.uin = r[0][2].as<std::string>();
		user.password = r[0][3].as<std::string>();
		user.encoding = r[0][4].as<std::string>();
		user.language = r[0][5].as<std::string>();
		user.vip = r[0][6].as<bool>();
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
		return false;
	}

	return true;
}

void PQXXBackend::setUserOnline(long id, bool online) {
	try {
		pqxx::nontransaction txn(*m_conn);
		txn.exec("UPDATE " + m_prefix + "users SET online=" + (online ? "'true'" : "'false'") + ", last_login=NOW() WHERE id=" + pqxx::to_string(id));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
}

bool PQXXBackend::getOnlineUsers(std::vector<std::string> &users) {
	try {
		pqxx::nontransaction txn(*m_conn);
		pqxx::result r = txn.exec("SELECT jid FROM " + m_prefix + "users WHERE online='true'");

		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			users.push_back((*it)[0].as<std::string>());
		}
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
		return false;
	}

	return true;
}

bool PQXXBackend::getUsers(std::vector<std::string> &users) {
	try {
		pqxx::nontransaction txn(*m_conn);
		pqxx::result r = txn.exec("SELECT jid FROM " + m_prefix + "users");

		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			users.push_back((*it)[0].as<std::string>());
		}
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
		return false;
	}

	return true;
}

long PQXXBackend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
	try {
		pqxx::nontransaction txn(*m_conn);
		pqxx::result r = txn.exec("INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES "
			+ "(" + pqxx::to_string(userId) + ","
			+ quote(txn, buddyInfo.legacyName) + ","
			+ quote(txn, buddyInfo.subscription) + ","
			+ quote(txn, StorageBackend::serializeGroups(buddyInfo.groups)) + ","
			+ quote(txn, buddyInfo.alias) + ","
			+ pqxx::to_string(buddyInfo.flags) + ") RETURNING id");

		long id = r[0][0].as<long>();

		r = txn.exec("UPDATE " + m_prefix + "buddies_settings SET var = " + quote(txn, buddyInfo.settings.find("icon_hash")->first) + ", type = " + pqxx::to_string((int)TYPE_STRING) + ", value = " + quote(txn, buddyInfo.settings.find("icon_hash")->second.s) + " WHERE user_id = " + pqxx::to_string(userId) + " AND buddy_id = " + pqxx::to_string(id));
		if (r.affected_rows() == 0) {
			txn.exec("INSERT INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES "
				+ "(" + pqxx::to_string(userId) + ","
				+ pqxx::to_string(id) + ","
				+ quote(txn, buddyInfo.settings.find("icon_hash")->first) + ","
				+ pqxx::to_string((int)TYPE_STRING) + ","
				+ quote(txn,  buddyInfo.settings.find("icon_hash")->second.s) + ")");
		}

		return id;
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
		return -1;
	}
}

void PQXXBackend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
	try {
		pqxx::nontransaction txn(*m_conn);
		txn.exec("UPDATE " + m_prefix + "buddies SET groups=" + quote(txn, StorageBackend::serializeGroups(buddyInfo.groups)) + ", nickname=" + quote(txn, buddyInfo.alias) + ", flags=" + pqxx::to_string(buddyInfo.flags) + ", subscription=" + quote(txn, buddyInfo.subscription) + " WHERE user_id=" + pqxx::to_string(userId) + " AND uin=" + quote(txn, buddyInfo.legacyName));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
}

bool PQXXBackend::getBuddies(long id, std::list<BuddyInfo> &roster) {
	try {
		pqxx::nontransaction txn(*m_conn);

		pqxx::result r = txn.exec("SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=" + pqxx::to_string(id) + " ORDER BY id ASC");
		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			BuddyInfo b;
			std::string group;

			b.id = (*it)[0].as<long>();
			b.legacyName = (*it)[1].as<std::string>();
			b.subscription = (*it)[2].as<std::string>();
			b.alias = (*it)[3].as<std::string>();
			group = (*it)[4].as<std::string>();
			b.flags = (*it)[5].as<long>();

			if (!group.empty()) {
				b.groups = StorageBackend::deserializeGroups(group);
			}

			roster.push_back(b);
		}


		r = txn.exec("SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=" + pqxx::to_string(id) + " ORDER BY buddy_id ASC");
		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			SettingVariableInfo var;
			long buddy_id = -1;
			std::string key;
			std::string val;

			buddy_id = (*it)[0].as<long>();
			var.type = (*it)[1].as<long>();
			key = (*it)[2].as<std::string>();
			val = (*it)[3].as<std::string>();
			switch (var.type) {
				case TYPE_BOOLEAN:
					var.b = atoi(val.c_str());
					break;
				case TYPE_STRING:
					var.s = val;
					break;
				default:
					continue;
					break;
			}

			BOOST_FOREACH(BuddyInfo &b, roster) {
				if (buddy_id == b.id) {
					b.settings[key] = var;
					break;
				}
			}
		}

		return true;
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}

	return false;
}

bool PQXXBackend::removeUser(long id) {
	try {
		pqxx::nontransaction txn(*m_conn);
		txn.exec("DELETE FROM " + m_prefix + "users WHERE id=" + pqxx::to_string(id));
		txn.exec("DELETE FROM " + m_prefix + "buddies WHERE user_id=" + pqxx::to_string(id));
		txn.exec("DELETE FROM " + m_prefix + "users_settings WHERE user_id=" + pqxx::to_string(id));
		txn.exec("DELETE FROM " + m_prefix + "buddies_settings WHERE user_id=" + pqxx::to_string(id));

		return true;
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
	return false;
}

void PQXXBackend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {
	try {
		pqxx::nontransaction txn(*m_conn);

		pqxx::result r = txn.exec("SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=" + pqxx::to_string(id) + " AND var=" + quote(txn, variable));
		if (r.size() == 0) {
			txn.exec("INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES(" + pqxx::to_string(id) + "," + quote(txn, variable) + "," + pqxx::to_string((int)type) + "," + quote(txn, value) + ")");
		}
		else {
			type = r[0][0].as<int>();
			value = r[0][1].as<std::string>();
		}
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
}

void PQXXBackend::updateUserSetting(long id, const std::string &variable, const std::string &value) {
	try {
		pqxx::nontransaction txn(*m_conn);
		txn.exec("UPDATE " + m_prefix + "users_settings SET value=" + quote(txn, value) + " WHERE user_id=" + pqxx::to_string(id) + " AND var=" + quote(txn, variable));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(pxxLogger, e.what());
	}
}

void PQXXBackend::beginTransaction() {
	exec("BEGIN;");
}

void PQXXBackend::commitTransaction() {
	exec("COMMIT;");
}

}

#endif
