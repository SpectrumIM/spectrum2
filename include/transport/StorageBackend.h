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

#pragma once

#include <string>
#include <map>
#include <list>
#include <vector>
#include <boost/bind.hpp>
// #include <boost/signal.hpp>

namespace Transport {

/// Represents all data needed to be stored in database.
struct UserInfo {
	int id;				///< id of user used as primary key in database
	std::string jid;		///< barejid of XMPP user
	std::string uin;		///< legacy network username
	std::string password;	///< password for legacy network
	std::string language;	///< user's preferred language
	std::string encoding;	///< user's preferred encoding
	bool vip;				///< true if user is VIP
};

typedef enum
{
	TYPE_UNKNOWN = 0,  /**< Unknown type.                     */
	TYPE_SUBTYPE,      /**< Subtype.                          */
	TYPE_CHAR,         /**< Character.                        */
	TYPE_UCHAR,        /**< Unsigned character.               */
	TYPE_BOOLEAN,      /**< Boolean.                          */
	TYPE_SHORT,        /**< Short integer.                    */
	TYPE_USHORT,       /**< Unsigned short integer.           */
	TYPE_INT,          /**< Integer.                          */
	TYPE_UINT,         /**< Unsigned integer.                 */
	TYPE_LONG,         /**< Long integer.                     */
	TYPE_ULONG,        /**< Unsigned long integer.            */
	TYPE_INT64,        /**< 64-bit integer.                   */
	TYPE_UINT64,       /**< 64-bit unsigned integer.          */
	TYPE_STRING,       /**< String.                           */
	TYPE_OBJECT,       /**< Object pointer.                   */
	TYPE_POINTER,      /**< Generic pointer.                  */
	TYPE_ENUM,         /**< Enum.                             */
	TYPE_BOXED         /**< Boxed pointer with specific type. */

} SettingType;

struct SettingVariableInfo {
	int type;
	std::string s;
	int i;
	bool b;
};

struct BuddyInfo {
	long id;
	std::string alias;
	std::string legacyName;
	std::string subscription;
	std::vector<std::string> groups;
	std::map<std::string, SettingVariableInfo> settings;
	int flags;
};

class Config;

/// Abstract class defining storage backends.
class StorageBackend
{
	public:
		static std::string encryptPassword(const std::string &password, const std::string &key);

		static std::string decryptPassword(std::string &encrypted, const std::string &key);

		static std::string serializeGroups(const std::vector<std::string> &groups);

		static std::vector<std::string> deserializeGroups(std::string &groups);

		/// Virtual desctructor.
		virtual ~StorageBackend() {}

		static StorageBackend *createBackend(Config *config, std::string &error);

		/// connect
		virtual bool connect() = 0;

		/// createDatabase
		virtual bool createDatabase() = 0;

		/// setUser
		virtual void setUser(const UserInfo &user) = 0;

		/// getuser
		virtual bool getUser(const std::string &barejid, UserInfo &user) = 0;

		/// setUserOnline
		virtual void setUserOnline(long id, bool online) = 0;

		/// removeUser
		virtual bool removeUser(long id) = 0;

		/// getBuddies
		virtual bool getBuddies(long id, std::list<BuddyInfo> &roster) = 0;

		/// getOnlineUsers
		virtual bool getOnlineUsers(std::vector<std::string> &users) = 0;

		virtual bool getUsers(std::vector<std::string> &users) = 0;

		virtual bool getLegacyNetworkUsers(std::vector<std::string> &users) = 0;

		virtual long addBuddy(long userId, const BuddyInfo &buddyInfo) = 0;
		virtual void updateBuddy(long userId, const BuddyInfo &buddyInfo) = 0;
		virtual void removeBuddy(long id) = 0;

		virtual void getBuddySetting(long userId, long buddyId, const std::string &variable, int &type, std::string &value) = 0;
		virtual void updateBuddySetting(long userId, long buddyId, const std::string &variable, int type, const std::string &value) = 0;

		virtual void getUserSetting(long userId, const std::string &variable, int &type, std::string &value) = 0;
		virtual void updateUserSetting(long userId, const std::string &variable, const std::string &value) = 0;

		virtual void beginTransaction() = 0;
		virtual void commitTransaction() = 0;

		/// onStorageError
// 		boost::signal<void (const std::string &statement, const std::string &error)> onStorageError;

};

}
