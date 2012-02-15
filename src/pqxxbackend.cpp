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

#include "transport/pqxxbackend.h"
#include "transport/util.h"
#include <boost/bind.hpp>
#include "log4cxx/logger.h"

using namespace log4cxx;
using namespace boost;

namespace Transport {

static LoggerPtr logger = Logger::getLogger("PQXXBackend");

PQXXBackend::PQXXBackend(Config *config) {
	m_config = config;
	m_prefix = CONFIG_STRING(m_config, "database.prefix");
}

PQXXBackend::~PQXXBackend(){
	disconnect();
}

void PQXXBackend::disconnect() {
	LOG4CXX_INFO(logger, "Disconnecting");

	delete m_conn;
}

bool PQXXBackend::connect() {
	LOG4CXX_INFO(logger, "Connecting PostgreSQL server " << CONFIG_STRING(m_config, "database.server") << ", user " <<
		CONFIG_STRING(m_config, "database.user") << ", database " << CONFIG_STRING(m_config, "database.database") <<
		", port " << CONFIG_INT(m_config, "database.port")
	);
	
	std::string str = "dbname=";
	str += CONFIG_STRING(m_config, "database.database") + " ";

	str += "user=" + CONFIG_STRING(m_config, "database.user") + " ";
	m_conn = new pqxx::connection(str);

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
				"vip boolean NOT NULL  default '0',"
				"online boolean NOT NULL  default '0',"
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

 		exec("INSERT INTO db_version (ver) VALUES ('1');");
	}

	return true;
}

template<typename T>
std::string PQXXBackend::quote(pqxx::work &txn, const T &t) {
	return "'" + txn.esc(pqxx::to_string(t)) + "'";
}

bool PQXXBackend::exec(const std::string &query, bool show_error) {
	pqxx::work txn(*m_conn);
	return exec(txn, query, show_error);
}

bool PQXXBackend::exec(pqxx::work &txn, const std::string &query, bool show_error) {
	try {
		txn.exec(query);
		txn.commit();
	}
	catch (std::exception& e) {
		if (show_error)
			LOG4CXX_ERROR(logger, e.what());
		return false;
	}
	return true;
}

void PQXXBackend::setUser(const UserInfo &user) {
	std::string encrypted = user.password;
	if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
		encrypted = Util::encryptPassword(encrypted, CONFIG_STRING(m_config, "database.encryption_key"));
	}
	pqxx::work txn(*m_conn);
	exec(txn, "INSERT INTO " + m_prefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES "
		 "(" + quote(txn, user.jid) + ","
		+ quote(txn, user.uin) + ","
		+ quote(txn, encrypted) + ","
		+ quote(txn, user.language) + ","
		+ quote(txn, user.encoding) + ","
		+ "NOW(),"
		+ (user.vip ? "'true'" : "'false'") +")");
}

bool PQXXBackend::getUser(const std::string &barejid, UserInfo &user) {
	try {
		pqxx::work txn(*m_conn);

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
		LOG4CXX_ERROR(logger, e.what());
		return false;
	}

	return true;
}

void PQXXBackend::setUserOnline(long id, bool online) {
	try {
		pqxx::work txn(*m_conn);
		exec(txn, "UPDATE " + m_prefix + "users SET online=" + (online ? "'true'" : "'false'") + ", last_login=NOW() WHERE id=" + pqxx::to_string(id));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
	}
}

bool PQXXBackend::getOnlineUsers(std::vector<std::string> &users) {
	try {
		pqxx::work txn(*m_conn);
		pqxx::result r = txn.exec("SELECT jid FROM " + m_prefix + "users WHERE online=1");

		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			users.push_back((*it)[0].as<std::string>());
		}
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
		return false;
	}

	return true;
}

long PQXXBackend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
	pqxx::work txn(*m_conn);
	pqxx::result r = txn.exec("INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES "
		+ "(" + pqxx::to_string(userId) + ","
		+ quote(txn, buddyInfo.legacyName) + ","
		+ quote(txn, buddyInfo.subscription) + ","
		+ quote(txn, Util::serializeGroups(buddyInfo.groups)) + ","
		+ quote(txn, buddyInfo.alias) + ","
		+ pqxx::to_string(buddyInfo.flags) + ") RETURNING id");

	long id = r[0][0].as<long>();

	r = txn.exec("UPDATE " + m_prefix + "buddies_settings SET var = " + quote(txn, buddyInfo.settings.find("icon_hash")->first) + ", type = " + pqxx::to_string((int)TYPE_STRING) + ", value = " + quote(txn, buddyInfo.settings.find("icon_hash")->second.s) + " WHERE user_id = " + pqxx::to_string(userId) + " AND buddy_id = " + pqxx::to_string(id));
	if (r.affected_rows() == 0) {
		exec("INSERT INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES "
			+ "(" + pqxx::to_string(userId) + ","
			+ pqxx::to_string(id) + ","
			+ quote(txn, buddyInfo.settings.find("icon_hash")->first) + ","
			+ pqxx::to_string((int)TYPE_STRING) + ","
			+ quote(txn,  buddyInfo.settings.find("icon_hash")->second.s) + ")");
	}

	return id;
}

void PQXXBackend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
	try {
		pqxx::work txn(*m_conn);
		exec(txn, "UPDATE " + m_prefix + "buddies SET groups=" + quote(txn, Util::serializeGroups(buddyInfo.groups)) + ", nickname=" + quote(txn, buddyInfo.alias) + ", flags=" + pqxx::to_string(buddyInfo.flags) + ", subscription=" + quote(txn, buddyInfo.subscription) + " WHERE user_id=" + pqxx::to_string(userId) + " AND uin=" + quote(txn, buddyInfo.legacyName));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
	}
}

bool PQXXBackend::getBuddies(long id, std::list<BuddyInfo> &roster) {
	try {
		pqxx::work txn(*m_conn);

		pqxx::result r = txn.exec("SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=" + pqxx::to_string(id) + " ORDER BY id ASC");
		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			BuddyInfo b;
			std::string group;

			b.id = r[0][0].as<long>();
			b.legacyName = r[0][1].as<std::string>();
			b.subscription = r[0][2].as<std::string>();
			b.alias = r[0][3].as<std::string>();
			group = r[0][4].as<std::string>();
			b.flags = r[0][5].as<long>();

			if (!group.empty()) {
				b.groups = Util::deserializeGroups(group);
			}

			roster.push_back(b);
		}


		r = txn.exec("SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=" + pqxx::to_string(id) + " ORDER BY buddy_id ASC");
		for (pqxx::result::const_iterator it = r.begin(); it != r.end(); it++)  {
			SettingVariableInfo var;
			long buddy_id = -1;
			std::string key;
			std::string val;

			buddy_id = r[0][0].as<long>();
			var.type = r[0][1].as<long>();
			key = r[0][2].as<std::string>();
			val = r[0][3].as<std::string>();
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
		LOG4CXX_ERROR(logger, e.what());
	}

	return false;
}

bool PQXXBackend::removeUser(long id) {
	try {
		pqxx::work txn(*m_conn);
		exec(txn, "DELETE FROM " + m_prefix + "users SET id=" + pqxx::to_string(id));
		exec(txn, "DELETE FROM " + m_prefix + "buddies SET user_id=" + pqxx::to_string(id));
		exec(txn, "DELETE FROM " + m_prefix + "user_settings SET user_id=" + pqxx::to_string(id));
		exec(txn, "DELETE FROM " + m_prefix + "buddies_settings SET user_id=" + pqxx::to_string(id));

		return true;
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
	}
	return false;
}

void PQXXBackend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {
	try {
		pqxx::work txn(*m_conn);

		pqxx::result r = txn.exec("SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=" + pqxx::to_string(id) + " AND var=" + quote(txn, variable));
		if (r.size() == 0) {
			exec(txn, "INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES(" + pqxx::to_string(id) + "," + quote(txn, variable) + "," + pqxx::to_string(type) + "," + quote(txn, value) + ")");
		}
		else {
			type = r[0][0].as<int>();
			value = r[0][1].as<std::string>();
		}
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
	}
}

void PQXXBackend::updateUserSetting(long id, const std::string &variable, const std::string &value) {
	try {
		pqxx::work txn(*m_conn);
		exec(txn, "UPDATE " + m_prefix + "users_settings SET value=" + quote(txn, value) + " WHERE user_id=" + pqxx::to_string(id) + " AND var=" + quote(txn, variable));
	}
	catch (std::exception& e) {
		LOG4CXX_ERROR(logger, e.what());
	}
}

void PQXXBackend::beginTransaction() {
	exec("START TRANSACTION;");
}

void PQXXBackend::commitTransaction() {
	exec("COMMIT;");
}

}

#endif
