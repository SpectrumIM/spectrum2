#ifndef USERDB_H
#define USERDB_H

#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <set>
#include "transport/logging.h"

struct UserData
{
	std::string user;
	std::string accessTokenKey;
	std::string accessTokenSecret;
	UserData(){}
	UserData(std::string _user, std::string key, std::string secret) {
		user = _user;
		accessTokenKey = key;
		accessTokenSecret = secret;
	}
};

class UserDB {
	private:
		sqlite3 *db;
		char *errMsg;
		char **result;
		int rc;
		int nRow,nCol;
		bool dbOpen;
		std::vector< std::vector<std::string> > data;
		
	public:

		UserDB (std::string database);
		int exe(std::string s_exe);
		void insert(UserData info);
		void fetch(std::string user, std::vector<std::string> &row);
		std::set<std::string> getRegisteredUsers();
		~UserDB();
};

#endif
