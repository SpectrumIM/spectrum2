#include "transport/storagebackend.h"
#include "transport/config.h"

#include "transport/sqlite3backend.h"
#include "transport/mysqlbackend.h"
#include "transport/pqxxbackend.h"

namespace Transport {

StorageBackend *StorageBackend::createBackend(Config *config, std::string &error) {
	StorageBackend *storageBackend = NULL;
#ifdef WITH_SQLITE
	if (CONFIG_STRING(config, "database.type") == "sqlite3") {
		storageBackend = new SQLite3Backend(config);
	}
#else
	if (CONFIG_STRING(config, "database.type") == "sqlite3") {
		error = "Libtransport is not compiled with sqlite3 backend support.";
	}
#endif

#ifdef WITH_MYSQL
	if (CONFIG_STRING(config, "database.type") == "mysql") {
		storageBackend = new MySQLBackend(config);
	}
#else
	if (CONFIG_STRING(config, "database.type") == "mysql") {
		error = "Spectrum2 is not compiled with mysql backend support.";
	}
#endif

#ifdef WITH_PQXX
	if (CONFIG_STRING(config, "database.type") == "pqxx") {
		storageBackend = new PQXXBackend(config);
	}
#else
	if (CONFIG_STRING(config, "database.type") == "pqxx") {
		error = "Spectrum2 is not compiled with pqxx backend support.";
	}
#endif

	if (CONFIG_STRING(config, "database.type") != "mysql" && CONFIG_STRING(config, "database.type") != "sqlite3"
		&& CONFIG_STRING(config, "database.type") != "pqxx" && CONFIG_STRING(config, "database.type") != "none") {
		error = "Unknown storage backend " + CONFIG_STRING(config, "database.type");
	}
	return storageBackend;
}

}
