#include "transport/StorageBackend.h"
#include "transport/Config.h"

#include "transport/SQLite3Backend.h"
#include "transport/MySQLBackend.h"
#include "transport/PQXXBackend.h"


#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

std::string decode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
    });
}

std::string encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

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

std::string StorageBackend::encryptPassword(const std::string &password, const std::string &key) {
	std::string encrypted;
	encrypted.resize(password.size());
	for (unsigned i = 0; i < password.size(); i++) {
		char c = password[i];
		char keychar = key[i % key.size()];
		c += keychar;
		encrypted[i] = c;
	}

	encrypted = encode64(encrypted);
	return encrypted;
}

std::string StorageBackend::decryptPassword(std::string &encrypted, const std::string &key) {
	encrypted = decode64(encrypted);
	std::string password;
	password.resize(encrypted.size());
	for (unsigned i = 0; i < encrypted.size(); i++) {
		char c = encrypted[i];
		char keychar = key[i % key.size()];
		c -= keychar;
		password[i] = c;
	}

	return password;
}

std::string StorageBackend::serializeGroups(const std::vector<std::string> &groups) {
	std::string ret;
	BOOST_FOREACH(const std::string &group, groups) {
		ret += group + "\n";
	}
	if (!ret.empty()) {
		ret.erase(ret.end() - 1);
	}
	return ret;
}

std::vector<std::string> StorageBackend::deserializeGroups(std::string &groups) {
	std::vector<std::string> ret;
	if (groups.empty()) {
		return ret;
	}

	boost::split(ret, groups, boost::is_any_of("\n"));
	if (ret.back().empty()) {
		ret.erase(ret.end() - 1);
	}
	return ret;
}

}
