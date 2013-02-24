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

#include "skypedb.h"

#include "transport/config.h"
#include "transport/logging.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/memoryusage.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/user.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/conversation.h"
#include "transport/networkplugin.h"
#include <boost/filesystem.hpp>
#include "sys/wait.h"
#include "sys/signal.h"
// #include "valgrind/memcheck.h"
#ifndef __FreeBSD__
#include "malloc.h"
#endif

#include "skypeplugin.h"

// Prepare the SQL statement
#define PREP_STMT(sql, str) \
	if(sqlite3_prepare_v2(db, std::string(str).c_str(), -1, &sql, NULL)) { \
		LOG4CXX_ERROR(logger, str<< (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db))); \
		sql = NULL; \
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
#define GET_BLOB(STATEMENT)	(const void *) sqlite3_column_blob(STATEMENT, STATEMENT##_id_get++)
#define EXECUTE_STATEMENT(STATEMENT, NAME) 	if(sqlite3_step(STATEMENT) != SQLITE_DONE) {\
		LOG4CXX_ERROR(logger, NAME<< (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));\
			}

using namespace Transport;

DEFINE_LOGGER(logger, "SkypeDB");

namespace SkypeDB {

bool getAvatar(const std::string &db_path, const std::string &name, std::string &photo) {
	bool ret = false;
	sqlite3 *db;
	LOG4CXX_INFO(logger, "Opening database " << db_path);
	if (sqlite3_open(db_path.c_str(), &db)) {
		sqlite3_close(db);
		LOG4CXX_ERROR(logger, "Can't open database");
	}
	else {
		sqlite3_stmt *stmt;
		PREP_STMT(stmt, "SELECT avatar_image FROM Contacts WHERE skypename=?");
		if (stmt) {
			BEGIN(stmt);
			BIND_STR(stmt, name);
			if(sqlite3_step(stmt) == SQLITE_ROW) {
				int size = sqlite3_column_bytes(stmt, 0);
				const void *data = sqlite3_column_blob(stmt, 0);
				photo = std::string((const char *)data + 1, size - 1);
				ret = true;
			}
			else {
				LOG4CXX_ERROR(logger, (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));
			}

			int ret;
			while((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
			}
			FINALIZE_STMT(stmt);
		}
		else {
			LOG4CXX_ERROR(logger, "Can't create prepared statement");
			LOG4CXX_ERROR(logger, (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));
		}
		sqlite3_close(db);
	}
	return ret;
}

bool loadBuddies(SkypePlugin *np, const std::string &db_path, std::string &user, std::map<std::string, std::string> &group_map) {
	bool ret = false;
	sqlite3 *db;
	LOG4CXX_INFO(logger, "Opening database " << db_path);
	if (sqlite3_open(db_path.c_str(), &db)) {
		sqlite3_close(db);
		LOG4CXX_ERROR(logger, "Can't open database");
	}
	else {
		sqlite3_stmt *stmt;
// 		aliases, fullname, 
		PREP_STMT(stmt, "select skypename, displayname, mood_text from Contacts;");
		if (stmt) {
			BEGIN(stmt);
			int ret2;
			while((ret2 = sqlite3_step(stmt)) == SQLITE_ROW) {
				std::string buddy = (const char *) sqlite3_column_text(stmt, 0);
				std::string alias = (const char *) sqlite3_column_text(stmt, 1);
				std::string mood_text = (const char *) sqlite3_column_text(stmt, 2);

				std::vector<std::string> groups;
				if (group_map.find(buddy) != group_map.end()) {
					groups.push_back(group_map[buddy]);
				}
				np->handleBuddyChanged(user, buddy, alias, groups, pbnetwork::STATUS_NONE, mood_text);
			}
			if (ret2 != SQLITE_DONE) {
				FINALIZE_STMT(stmt);
				ret = false;
			}
			else {
				ret = true;
			}
		}
		else {
			LOG4CXX_ERROR(logger, "Can't create prepared statement");
			LOG4CXX_ERROR(logger, (sqlite3_errmsg(db) == NULL ? "" : sqlite3_errmsg(db)));
		}
		sqlite3_close(db);
	}
	return ret;
}

}
