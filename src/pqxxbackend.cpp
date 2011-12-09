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
}

bool PQXXBackend::connect() {
	LOG4CXX_INFO(logger, "Connecting PostgreSQL server " << CONFIG_STRING(m_config, "database.server") << ", user " <<
		CONFIG_STRING(m_config, "database.user") << ", database " << CONFIG_STRING(m_config, "database.database") <<
		", port " << CONFIG_INT(m_config, "database.port")
	);

	createDatabase();

	return true;
}

bool PQXXBackend::createDatabase() {
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

bool PQXXBackend::exec(const std::string &query) {
//	if (mysql_query(&m_conn, query.c_str())) {
//		LOG4CXX_ERROR(logger, query << " " << mysql_error(&m_conn));
//		return false;
//	}
	return true;
}

void PQXXBackend::setUser(const UserInfo &user) {
//	std::string encrypted = user.password;
//	if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
//		encrypted = Util::encryptPassword(encrypted, CONFIG_STRING(m_config, "database.encryption_key"));
//	}
//	*m_setUser << user.jid << user.uin << encrypted << user.language << user.encoding << user.vip << user.uin << encrypted;
//	EXEC(m_setUser, setUser(user));
}

bool PQXXBackend::getUser(const std::string &barejid, UserInfo &user) {
//	*m_getUser << barejid;
//	EXEC(m_getUser, getUser(barejid, user));
//	if (!exec_ok)
//		return false;

	int ret = false;
//	while (m_getUser->fetch() == 0) {
//		ret = true;
//		*m_getUser >> user.id >> user.jid >> user.uin >> user.password >> user.encoding >> user.language >> user.vip;

//		if (!CONFIG_STRING(m_config, "database.encryption_key").empty()) {
//			user.password = Util::decryptPassword(user.password, CONFIG_STRING(m_config, "database.encryption_key"));
//		}
//	}

	return ret;
}

void PQXXBackend::setUserOnline(long id, bool online) {
//	*m_setUserOnline << online << id;
//	EXEC(m_setUserOnline, setUserOnline(id, online));
}

bool PQXXBackend::getOnlineUsers(std::vector<std::string> &users) {
//	EXEC(m_getOnlineUsers, getOnlineUsers(users));
//	if (!exec_ok)
//		return false;

//	std::string jid;
//	while (m_getOnlineUsers->fetch() == 0) {
//		*m_getOnlineUsers >> jid;
//		users.push_back(jid);
//	}

	return true;
}

long PQXXBackend::addBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"INSERT INTO " + m_prefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)"
//	std::string groups = Util::serializeGroups(buddyInfo.groups);
//	*m_addBuddy << userId << buddyInfo.legacyName << buddyInfo.subscription;
//	*m_addBuddy << groups;
//	*m_addBuddy << buddyInfo.alias << buddyInfo.flags;

//	EXEC(m_addBuddy, addBuddy(userId, buddyInfo));

//	long id = (long) mysql_insert_id(&m_conn);

// 	INSERT OR REPLACE INTO " + m_prefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)
//	if (!buddyInfo.settings.find("icon_hash")->second.s.empty()) {
//		*m_updateBuddySetting << userId << id << buddyInfo.settings.find("icon_hash")->first << (int) TYPE_STRING << buddyInfo.settings.find("icon_hash")->second.s << buddyInfo.settings.find("icon_hash")->second.s;
//		EXEC(m_updateBuddySetting, addBuddy(userId, buddyInfo));
//	}

	return 0;
}

void PQXXBackend::updateBuddy(long userId, const BuddyInfo &buddyInfo) {
// 	"UPDATE " + m_prefix + "buddies SET groups=?, nickname=?, flags=?, subscription=? WHERE user_id=? AND uin=?"
//	std::string groups = Util::serializeGroups(buddyInfo.groups);
//	*m_updateBuddy << groups;
//	*m_updateBuddy << buddyInfo.alias << buddyInfo.flags << buddyInfo.subscription;
//	*m_updateBuddy << userId << buddyInfo.legacyName;

//	EXEC(m_updateBuddy, updateBuddy(userId, buddyInfo));
}

bool PQXXBackend::getBuddies(long id, std::list<BuddyInfo> &roster) {
//	SELECT id, uin, subscription, nickname, groups, flags FROM " + m_prefix + "buddies WHERE user_id=? ORDER BY id ASC
//	*m_getBuddies << id;

// 	"SELECT buddy_id, type, var, value FROM " + m_prefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC"
//	*m_getBuddiesSettings << id;

//	SettingVariableInfo var;
//	long buddy_id = -1;
//	std::string key;

//	EXEC(m_getBuddies, getBuddies(id, roster));
//	if (!exec_ok)
//		return false;

//	while (m_getBuddies->fetch() == 0) {
//		BuddyInfo b;

//		std::string group;
//		*m_getBuddies >> b.id >> b.legacyName >> b.subscription >> b.alias >> group >> b.flags;

//		if (!group.empty()) {
//			b.groups = Util::deserializeGroups(group);
//		}

//		roster.push_back(b);
//	}

//	EXEC(m_getBuddiesSettings, getBuddies(id, roster));
//	if (!exec_ok)
//		return false;

//	BOOST_FOREACH(BuddyInfo &b, roster) {
//		if (buddy_id == b.id) {
//// 			std::cout << "Adding buddy info setting " << key << "\n";
//			b.settings[key] = var;
//			buddy_id = -1;
//		}

//		while(buddy_id == -1 && m_getBuddiesSettings->fetch() == 0) {
//			std::string val;
//			*m_getBuddiesSettings >> buddy_id >> var.type >> key >> val;

//			switch (var.type) {
//				case TYPE_BOOLEAN:
//					var.b = atoi(val.c_str());
//					break;
//				case TYPE_STRING:
//					var.s = val;
//					break;
//				default:
//					if (buddy_id == b.id) {
//						buddy_id = -1;
//					}
//					continue;
//					break;
//			}
//			if (buddy_id == b.id) {
//// 				std::cout << "Adding buddy info setting " << key << "=" << val << "\n";
//				b.settings[key] = var;
//				buddy_id = -1;
//			}
//		}
//	}

//	while(m_getBuddiesSettings->fetch() == 0) {
//		// TODO: probably remove those settings, because there's no buddy for them.
//		// It should not happend, but one never know...
//	}
	
	return true;
}

bool PQXXBackend::removeUser(long id) {
//	*m_removeUser << (int) id;
//	EXEC(m_removeUser, removeUser(id));
//	if (!exec_ok)
//		return false;

//	*m_removeUserSettings << (int) id;
//	EXEC(m_removeUserSettings, removeUser(id));
//	if (!exec_ok)
//		return false;

//	*m_removeUserBuddies << (int) id;
//	EXEC(m_removeUserBuddies, removeUser(id));
//	if (!exec_ok)
//		return false;

//	*m_removeUserBuddiesSettings << (int) id;
//	EXEC(m_removeUserBuddiesSettings, removeUser(id));
//	if (!exec_ok)
//		return false;

	return true;
}

void PQXXBackend::getUserSetting(long id, const std::string &variable, int &type, std::string &value) {
//// 	"SELECT type, value FROM " + m_prefix + "users_settings WHERE user_id=? AND var=?"
//	*m_getUserSetting << id << variable;
//	EXEC(m_getUserSetting, getUserSetting(id, variable, type, value));
//	if (m_getUserSetting->fetch() != 0) {
//// 		"INSERT INTO " + m_prefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)"
//		*m_setUserSetting << id << variable << type << value;
//		EXEC(m_setUserSetting, getUserSetting(id, variable, type, value));
//	}
//	else {
//		*m_getUserSetting >> type >> value;
//	}
}

void PQXXBackend::updateUserSetting(long id, const std::string &variable, const std::string &value) {
//// 	"UPDATE " + m_prefix + "users_settings SET value=? WHERE user_id=? AND var=?"
//	*m_updateUserSetting << value << id << variable;
//	EXEC(m_updateUserSetting, updateUserSetting(id, variable, value));
}

void PQXXBackend::beginTransaction() {
//	exec("START TRANSACTION;");
}

void PQXXBackend::commitTransaction() {
//	exec("COMMIT;");
}

}

#endif
