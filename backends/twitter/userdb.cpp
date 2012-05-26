#include "userdb.h"

DEFINE_LOGGER(logger, "Twitter Backend Database");

UserDB::UserDB(std::string database): errMsg(0), rc(0), dbOpen(false) 
{
	rc = sqlite3_open(database.c_str(), &db);
	if( rc ) {
		LOG4CXX_ERROR(logger, "Failed to open database" << database);
		sqlite3_close(db);
		exit(0);
	}
	
	LOG4CXX_INFO(logger, "Checking if table users is present")	
	if(exe(std::string("select * from users limit 1;")) != SQLITE_OK) {
		exe("create table users (user text primarykey, key text, secret text);");
		LOG4CXX_INFO(logger, "Created table users in the database");
	}
	dbOpen = true;
}

int UserDB::exe(std::string s_exe) 
{
	data.clear();

	//LOG4CXX_INFO(logger, "Executing: " << s_exe)	
	rc = sqlite3_get_table(db, s_exe.c_str(), &result, &nRow, &nCol, &errMsg);
	if( rc == SQLITE_OK ) {	
		int col = nCol; //Skip past the headers
		for(int i = 0; i < nRow; ++i) {
			std::vector<std::string> row;
			for(int j = 0 ; j < nCol ; j++) row.push_back(result[col++]);
			data.push_back(row);
		}
	} 
	sqlite3_free_table(result);
	return rc;
}

void UserDB::insert(UserData info)
{
	std::string q = "insert into users (user,key,secret) values ('" + info.user + "','" + info.accessTokenKey + "','" + info.accessTokenSecret + "');";
	if(exe(q) !=  SQLITE_OK) {
		LOG4CXX_ERROR(logger, "Failed to insert into database!");
		exit(0);	
	}
}

void UserDB::fetch(std::string user, std::vector<std::string> &row)
{
	std::string q =  "select key,secret from users where user='" + user + "'";
	if(exe(q) !=  SQLITE_OK) {
		LOG4CXX_ERROR(logger, "Failed to fetch data from database!");
		exit(0);
	}
	row = data[0];
}

std::set<std::string> UserDB::getRegisteredUsers()
{
	std::string q = "select user from users";
	if(exe(q) != SQLITE_OK) {
		LOG4CXX_ERROR(logger, "Failed to registered users from database!");
		exit(0);
	}
	
	std::set<std::string> users;
	for(int i=0 ; i<data.size() ; i++)
		users.insert(data[i][0]);
	return users;
}

UserDB::~UserDB()
{
	sqlite3_close(db);
}
