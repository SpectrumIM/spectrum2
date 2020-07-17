#include "utils.h"

#include "glib.h"

// win32/libc_interface.h defines its own socket(), read() and so on.
// We don't want to use it here.
#define _LIBC_INTERFACE_H_ 1

#include "purple.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <utime.h>

#include "transport/NetworkPlugin.h"
#include "transport/Logging.h"
#include "transport/Config.h"
#include "transport/StorageBackend.h"
#include "geventloop.h"
#include "Swiften/SwiftenCompat.h"

// #include "valgrind/memcheck.h"
#if !defined(__FreeBSD__) && !defined(__APPLE__)
#include "malloc.h"
#endif
#include "errno.h"
#include <boost/make_shared.hpp>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/thread/mutex.hpp>

#ifdef WITH_LIBEVENT
#include <event.h>
#endif

#ifdef WIN32
#include "win32/win32dep.h"
#define close closesocket
#define ssize_t SSIZE_T
#include <process.h>
#define getpid _getpid
#endif

#include "purple_defs.h"

DEFINE_LOGGER(logger_libpurple, "libpurple");
DEFINE_LOGGER(logger, "backend");

/* Additional PURPLE_MESSAGE_* flags as a hack to track the origin of the message. */
typedef enum {
    PURPLE_MESSAGE_SPECTRUM2_ORIGINATED = 0x80000000,
} PurpleMessageSpectrum2Flags;

int main_socket;
static int writeInput;
bool firstPing = true;

using namespace Transport;

template <class T> T fromString(const std::string &str) {
	T i;
	std::istringstream os(str);
	os >> i;
	return i;
}

template <class T> std::string stringOf(T object) {
	std::ostringstream os;
	os << object;
	return (os.str());
}

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

static void transportDataReceived(gpointer data, gint source, PurpleInputCondition cond);

class SpectrumNetworkPlugin;

SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Config> config;
SpectrumNetworkPlugin *np;
StorageBackend *storagebackend;

static std::string host;
static int port = 10000;

struct FTData {
	unsigned long id;
	unsigned long timer;
	bool paused;
};

struct NodeCache {
	PurpleAccount *account;
	std::map<PurpleBlistNode *, int> nodes;
	int timer;
};

bool caching = true;

static void *notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info);

static gboolean ft_ui_ready(void *data) {
	PurpleXfer *xfer = (PurpleXfer *) data;
	FTData *ftdata = (FTData *) xfer->ui_data;
	ftdata->timer = 0;
	purple_xfer_ui_ready_wrapped((PurpleXfer *) data);
	return FALSE;
}

/*
Authorization requests from buddies are cached for the duration of the session.
To authorize or deny, call the cached callbacks.
*/
struct authRequest {
	PurpleAccountRequestAuthorizationCb authorize_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *user_data;
	std::string who;
	PurpleAccount *account;
	std::string mainJID;	// JID of user connected with this request
};
class AuthRequestList : public std::map<std::string, authRequest*> {
	public:
		bool accept(const std::string &user, const std::string &buddyName) {
			iterator it = this->find(user + buddyName);
			if (it == this->end()) {
				LOG4CXX_TRACE(logger, "AuthRequestList::accept(" << user << ", " << buddyName << ")"
					<< ": No such request.");
				return false;
			}
			LOG4CXX_TRACE(logger, "AuthRequestList::accept(" << user << ", " << buddyName << ")");
			it->second->authorize_cb(it->second->user_data);
			this->erase(user + buddyName);
		}
		

		bool deny(const std::string &user, const std::string &buddyName) {
			iterator it = this->find(user + buddyName);
			if (it == this->end()) {
				LOG4CXX_TRACE(logger, "AuthRequestList::deny(" << user << ", " << buddyName << ")"
					<< ": No such request.");
				return false;
			}
			LOG4CXX_TRACE(logger, "AuthRequestList::deny(" << user << ", " << buddyName << ")");
			it->second->deny_cb(it->second->user_data);
			this->erase(user + buddyName);
		}
};


struct inputRequest {
    PurpleRequestInputCb ok_cb;
    void *user_data;
    std::string who;
    PurpleAccount *account;
    std::string mainJID;	// JID of user connected with this request
};

static void *requestAction(const char *title, const char *primary, const char *secondary, int default_action, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data, size_t action_count, va_list actions){
	std::string t(title ? title : "NULL");
	if (t == "SSL Certificate Verification") {
		if (CONFIG_BOOL_DEFAULTED(config, "service.verify_certs", false)) {
			LOG4CXX_INFO(logger,  "rejecting SSL certificate");
			va_arg(actions, char *);
			va_arg(actions, GCallback);
		} else {
			LOG4CXX_INFO(logger,  "accepting SSL certificate");
		}
		va_arg(actions, char *);
		((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
	}
	else if (t == "Plaintext Authentication") {
		LOG4CXX_INFO(logger,  "Rejecting plaintext authentification");
		va_arg(actions, char *);
		va_arg(actions, GCallback);
		va_arg(actions, char *);
		((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
	}
	else {
		if (title) {
			std::string headerString(title);
			LOG4CXX_INFO(logger,  "header string: " << headerString);
			if (headerString == "SSL Certificate Verification") {
				if (CONFIG_BOOL_DEFAULTED(config, "service.verify_certs", false)) {
					va_arg(actions, char *);
					va_arg(actions, GCallback);
				}
				va_arg(actions, char *);
				((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
			}
		}
	}
	
	if (CONFIG_STRING(config, "service.protocol") == "prpl-jabber") {
		//prpl-jabber newly created MUCs need to be configured before they can work
		if ((t == "Create New Room") && (action_count == 2)) {
			va_arg(actions, char*);
			va_arg(actions, GCallback);
			if (std::string(va_arg(actions, char*)) != "Accept Defaults") {
				LOG4CXX_DEBUG(logger, "New room: Accepting default room settings");
				((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
			} else
				LOG4CXX_ERROR(logger, "New room: Unexpected prpl-jabber request format, cannot configure new room");
		}
	}
	
	return NULL;
}

static std::string getAlias(PurpleBuddy *m_buddy) {
	std::string alias;
	PurpleContact *contact = PURPLE_CONTACT(PURPLE_BLIST_NODE(m_buddy)->parent);
	if (contact && contact->alias) {
		alias = contact->alias;
	}
	else if (purple_buddy_get_alias_wrapped(m_buddy)) {
		alias = (std::string) purple_buddy_get_alias_wrapped(m_buddy);
	}
	else {
		alias = (std::string) purple_buddy_get_server_alias_wrapped(m_buddy);
	}
	return alias;
}

static boost::mutex dblock;
static std::string OAUTH_TOKEN = "hangouts_oauth_token";
static std::string STEAM_ACCESS_TOKEN = "steammobile_access_token";
static std::string DISCORD_ACCESS_TOKEN = "discord_access_token";

static bool getUserToken(const std::string user, const std::string token_name, std::string &token_value)
{
	boost::mutex::scoped_lock lock(dblock);
	UserInfo info;
	if (storagebackend->getUser(user, info) == false) {
		LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!");
		return false;
	}
	token_value = "";
	int type = TYPE_STRING;
	storagebackend->getUserSetting((long)info.id, token_name, type, token_value);
	return true;
}

static bool storeUserToken(const std::string user, const std::string token_name, const std::string token_value)
{
	boost::mutex::scoped_lock lock(dblock);
	UserInfo info;
	if (storagebackend->getUser(user, info) == false) {
		LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!");
		return false;
	}
	storagebackend->updateUserSetting((long)info.id, token_name, token_value);
	return true;
}

class SpectrumNetworkPlugin : public NetworkPlugin {
	public:
		SpectrumNetworkPlugin() : NetworkPlugin() {
			LOG4CXX_INFO(logger, "Starting libpurple backend " << SPECTRUM_VERSION);
		}

		void handleExitRequest() {
			LOG4CXX_INFO(logger, "Exiting...");
			exit(0);
		}

		void getProtocolAndName(const std::string &legacyName, std::string &name, std::string &protocol) {
			name = legacyName;
			protocol = CONFIG_STRING(config, "service.protocol");
			if (protocol == "any") {
				protocol = name.substr(0, name.find("."));
				name = name.substr(name.find(".") + 1);
			}
		}

		void setDefaultAvatar(PurpleAccount *account, const std::string &legacyName) {
			char* contents;
			gsize length;
			gboolean ret = false;
			if (!CONFIG_STRING(config, "backend.avatars_directory").empty()) {
				std::string f = CONFIG_STRING(config, "backend.avatars_directory") + "/" + legacyName;
				ret = g_file_get_contents (f.c_str(), &contents, &length, NULL);
			}

			if (!CONFIG_STRING(config, "backend.default_avatar").empty() && !ret) {
				ret = g_file_get_contents (CONFIG_STRING(config, "backend.default_avatar").c_str(),
											&contents, &length, NULL);
			}

			if (ret) {
				purple_buddy_icons_set_account_icon_wrapped(account, (guchar *) contents, length);
			}
		}

		void setDefaultAccountOptions(PurpleAccount *account) {
			int i = 0;
			Config::SectionValuesCont purpleConfigValues = config->getSectionValues("purple");

			BOOST_FOREACH ( const Config::SectionValuesCont::value_type & keyItem, purpleConfigValues )
			{
				std::string key = keyItem.first;
				std::string strippedKey = boost::erase_first_copy(key, "purple.");

				if (strippedKey == "fb_api_key" || strippedKey == "fb_api_secret") {
					purple_account_set_bool_wrapped(account, "auth_fb", TRUE);
 				}

				PurplePlugin *plugin = purple_find_prpl_wrapped(purple_account_get_protocol_id_wrapped(account));
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
				bool found = false;
				for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
					PurpleAccountOption *option = (PurpleAccountOption *) l->data;
					PurplePrefType type = purple_account_option_get_type_wrapped(option);
					std::string key2(purple_account_option_get_setting_wrapped(option));
					if (strippedKey != key2) {
						continue;
					}

					found = true;
					switch (type) {
						case PURPLE_PREF_BOOLEAN:
							purple_account_set_bool_wrapped(account, strippedKey.c_str(), keyItem.second.as<bool>());
							break;

						case PURPLE_PREF_INT:
							purple_account_set_int_wrapped(account, strippedKey.c_str(), fromString<int>(keyItem.second.as<std::string>()));
							break;

						case PURPLE_PREF_STRING:
						case PURPLE_PREF_STRING_LIST:
							purple_account_set_string_wrapped(account, strippedKey.c_str(), keyItem.second.as<std::string>().c_str());
							break;
						default:
							continue;
					}
					break;
				}

				if (!found) {
					purple_account_set_string_wrapped(account, strippedKey.c_str(), keyItem.second.as<std::string>().c_str());
				}
				i++;
			}

			char* contents;
			gsize length;
			gboolean ret = g_file_get_contents ("gfire.cfg", &contents, &length, NULL);
			if (ret) {
				purple_account_set_int_wrapped(account, "version", fromString<int>(std::string(contents, length)));
			}


			if (CONFIG_STRING(config, "service.protocol") == "prpl-novell") {
				std::string username(purple_account_get_username_wrapped(account));
				std::vector <std::string> u = split(username, '@');
				purple_account_set_username_wrapped(account, (const char*) u.front().c_str());
				std::vector <std::string> s = split(u.back(), ':');
				purple_account_set_string_wrapped(account, "server", s.front().c_str());
				purple_account_set_int_wrapped(account, "port", atoi(s.back().c_str()));
			}

			if (!CONFIG_STRING_DEFAULTED(config, "proxy.type", "").empty()) {
				PurpleProxyInfo *info = purple_proxy_info_new_wrapped();
				if (CONFIG_STRING_DEFAULTED(config, "proxy.type", "") == "http") {
					purple_proxy_info_set_type_wrapped(info, PURPLE_PROXY_HTTP);
				}
				else if (CONFIG_STRING_DEFAULTED(config, "proxy.type", "") == "socks4") {
					purple_proxy_info_set_type_wrapped(info, PURPLE_PROXY_SOCKS4);
				}
				else if (CONFIG_STRING_DEFAULTED(config, "proxy.type", "") == "socks5") {
					purple_proxy_info_set_type_wrapped(info, PURPLE_PROXY_SOCKS5);
				}
				else {
					LOG4CXX_ERROR(logger, "Unknown proxy.type " << CONFIG_STRING_DEFAULTED(config, "proxy.type", ""));
				}

				info->username = NULL;
				info->password = NULL;

				purple_proxy_info_set_type_wrapped(info, PURPLE_PROXY_SOCKS5);
				purple_proxy_info_set_host_wrapped(info, CONFIG_STRING_DEFAULTED(config, "proxy.host", "").c_str());
				if (CONFIG_INT_DEFAULTED(config, "proxy.port", 0)) {
					purple_proxy_info_set_port_wrapped(info, CONFIG_INT_DEFAULTED(config, "proxy.port", 0));
				}
				if (!CONFIG_STRING_DEFAULTED(config, "proxy.username", "").empty()) {
					purple_proxy_info_set_username_wrapped(info, CONFIG_STRING_DEFAULTED(config, "proxy.username", "").c_str());
				}

				if (!CONFIG_STRING_DEFAULTED(config, "proxy.password", "").empty()) {
					purple_proxy_info_set_password_wrapped(info, CONFIG_STRING_DEFAULTED(config, "proxy.password", "").c_str());
				}

				purple_account_set_proxy_info_wrapped(account, info);
			}
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			PurpleAccount *account = NULL;

			std::string name;
			std::string protocol;
			getProtocolAndName(legacyName, name, protocol);

			if (protocol == "prpl-hangouts") {
				adminLegacyName = "hangouts";
				adminAlias = "hangouts";
			}
			else if (protocol == "prpl-steam-mobile") {
				adminLegacyName = "steam-mobile";
				adminAlias = "steam-mobile";
			}
			else if (protocol == "prpl-eionrobb-discord") {
				adminLegacyName = "discord";
				adminAlias = "discord";
			}
			else if (protocol == "telegram-tdlib") {
				adminLegacyName = "telegram";
				adminAlias = "telegram";
			}
			if (!purple_find_prpl_wrapped(protocol.c_str())) {
				LOG4CXX_INFO(logger,  name.c_str() << ": Invalid protocol '" << protocol << "'");
				np->handleDisconnected(user, 1, "Invalid protocol " + protocol);
				return;
			}

			if (purple_accounts_find_wrapped(name.c_str(), protocol.c_str()) != NULL) {
				account = purple_accounts_find_wrapped(name.c_str(), protocol.c_str());
				if (m_accounts.find(account) != m_accounts.end() && m_accounts[account] != user) {
					LOG4CXX_INFO(logger, "Account '" << name << "' is already used by '" << m_accounts[account] << "'");
					np->handleDisconnected(user, 1, "Account '" + name + "' is already used by '" + m_accounts[account] + "'");
					return;
				}
				LOG4CXX_INFO(logger, "Using previously created account with name '" << name.c_str() << "' and protocol '" << protocol << "'");
			}
			else {
				LOG4CXX_INFO(logger, "Creating account with name '" << name.c_str() << "' and protocol '" << protocol << "'");
				account = purple_account_new_wrapped(name.c_str(), protocol.c_str());
				purple_accounts_add_wrapped(account);
			}

			m_sessions[user] = account;
			m_accounts[account] = user;

			// Default avatar
			setDefaultAvatar(account, legacyName);

			purple_account_set_password_wrapped(account, password.c_str());
			purple_account_set_bool_wrapped(account, "custom_smileys", FALSE);
			purple_account_set_bool_wrapped(account, "direct_connect", FALSE);
			purple_account_set_bool_wrapped(account, "compat-verification", TRUE);
			if (protocol == "prpl-hangouts") {
				std::string token;
				if (getUserToken(user, OAUTH_TOKEN, token)) {
					purple_account_set_password_wrapped(account, token.c_str());
				}
			}
			else if (protocol == "prpl-steam-mobile") {
				std::string token;
				getUserToken(user, STEAM_ACCESS_TOKEN, token);
				if (!token.empty()) {
					purple_account_set_string_wrapped(account, "access_token", token.c_str());
				}
			}
			else if (protocol == "prpl-eionrobb-discord") {
				std::string token;
				getUserToken(user, DISCORD_ACCESS_TOKEN, token);
				if (!token.empty()) {
					purple_account_set_string_wrapped(account, "token", token.c_str());
				}
			}

			setDefaultAccountOptions(account);

			// Enable account + privacy lists
			purple_account_set_enabled_wrapped(account, "spectrum", TRUE);

#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 7
			if (CONFIG_BOOL(config, "service.enable_privacy_lists")) {
				purple_account_set_privacy_type_wrapped(account, PURPLE_PRIVACY_DENY_USERS);
			}
#endif

			// Set the status
			const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive_wrapped(account, PURPLE_STATUS_AVAILABLE);
			if (status_type != NULL) {
				purple_account_set_status_wrapped(account, purple_status_type_get_id_wrapped(status_type), TRUE, NULL);
			}
			// OAuth helper
			if (protocol == "prpl-hangouts" || protocol == "prpl-steam-mobile" || protocol == "prpl-eionrobb-discord") {
				LOG4CXX_INFO(logger, user << ": Adding Buddy " << adminLegacyName << " " << adminAlias);
				handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
			}
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				if (account->ui_data) {
					NodeCache *cache = (NodeCache *) account->ui_data;
					purple_timeout_remove_wrapped(cache->timer);
					delete cache;
					account->ui_data = NULL;
				}
				if (purple_account_get_int_wrapped(account, "version", 0) != 0) {
					std::string data = stringOf(purple_account_get_int_wrapped(account, "version", 0));
					g_file_set_contents ("gfire.cfg", data.c_str(), data.size(), NULL);
				}
// 				VALGRIND_DO_LEAK_CHECK;
				m_sessions.erase(user);
				purple_account_disconnect_wrapped(account);
				purple_account_set_enabled_wrapped(account, "spectrum", FALSE);

				m_accounts.erase(account);

				purple_accounts_delete_wrapped(account);
#ifndef WIN32
#if !defined(__FreeBSD__) && !defined(__APPLE__)
				malloc_trim(0);
#endif
#endif
// 				VALGRIND_DO_LEAK_CHECK;
			}
		}

		void handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				int st;
				switch(status) {
					case pbnetwork::STATUS_AWAY: {
						st = PURPLE_STATUS_AWAY;
						if (!purple_account_get_status_type_with_primitive_wrapped(account, PURPLE_STATUS_AWAY))
							st = PURPLE_STATUS_EXTENDED_AWAY;
						else
							st = PURPLE_STATUS_AWAY;
						break;
					}
					case pbnetwork::STATUS_DND: {
						st = PURPLE_STATUS_UNAVAILABLE;
						break;
					}
					case pbnetwork::STATUS_XA: {
						if (!purple_account_get_status_type_with_primitive_wrapped(account, PURPLE_STATUS_EXTENDED_AWAY))
							st = PURPLE_STATUS_AWAY;
						else
							st = PURPLE_STATUS_EXTENDED_AWAY;
						break;
					}
					case pbnetwork::STATUS_NONE: {
						st = PURPLE_STATUS_OFFLINE;
						break;
					}
					case pbnetwork::STATUS_INVISIBLE:
						st = PURPLE_STATUS_INVISIBLE;
						break;
					default:
						st = PURPLE_STATUS_AVAILABLE;
						break;
				}
				gchar *_markup = purple_markup_escape_text_wrapped(statusMessage.c_str(), -1);
				std::string markup(_markup);
				g_free(_markup);

				// we are already connected so we have to change status
				const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive_wrapped(account, (PurpleStatusPrimitive) st);
				if (status_type != NULL) {
					// send presence to legacy network
					if (!markup.empty()) {
						purple_account_set_status_wrapped(account, purple_status_type_get_id_wrapped(status_type), TRUE, "message", markup.c_str(), NULL);
					}
					else {
						purple_account_set_status_wrapped(account, purple_status_type_get_id_wrapped(status_type), TRUE, NULL);
					}
				}
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml, const std::string &id) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				LOG4CXX_INFO(logger, "Sending message to '" << legacyName << "'");
				PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, LegacyNameToName(account, legacyName).c_str(), account);
				if (legacyName == adminLegacyName) {
					// expect OAuth code
					if (m_inputRequests.find(user) != m_inputRequests.end()) {
						LOG4CXX_INFO(logger, "Updating token for '" << user << "'");
						m_inputRequests[user]->ok_cb(m_inputRequests[user]->user_data, message.c_str());
						m_inputRequests.erase(user);
					}
					return;
				}

				if (!conv) {
					conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_IM, LegacyNameToName(account, legacyName).c_str(), account);
					if (!conv) {
						conv = purple_conversation_new_wrapped(PURPLE_CONV_TYPE_IM, account, LegacyNameToName(account, legacyName).c_str());
					}
				}
				if (xhtml.empty()) {
					gchar *_markup = purple_markup_escape_text_wrapped(message.c_str(), -1);
					if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
						purple_conv_im_send_with_flags_wrapped(PURPLE_CONV_IM_WRAPPED(conv), _markup, static_cast<PurpleMessageFlags>(PURPLE_MESSAGE_SPECTRUM2_ORIGINATED));
					}
					else if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_CHAT) {
						purple_conv_chat_send_with_flags_wrapped(PURPLE_CONV_CHAT_WRAPPED(conv), _markup, static_cast<PurpleMessageFlags>(PURPLE_MESSAGE_SPECTRUM2_ORIGINATED));
					}
					g_free(_markup);
				}
				else {
					if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
						purple_conv_im_send_with_flags_wrapped(PURPLE_CONV_IM_WRAPPED(conv), xhtml.c_str(), static_cast<PurpleMessageFlags>(PURPLE_MESSAGE_SPECTRUM2_ORIGINATED));
					}
					else if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_CHAT) {
						purple_conv_chat_send_with_flags_wrapped(PURPLE_CONV_CHAT_WRAPPED(conv), xhtml.c_str(), static_cast<PurpleMessageFlags>(PURPLE_MESSAGE_SPECTRUM2_ORIGINATED));
					}
				}
			}
		}

		void handleRoomSubjectChangedRequest(const std::string &user, const std::string &legacyName, const std::string &message) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, LegacyNameToName(account, legacyName).c_str(), account);
				if (!conv) {
					LOG4CXX_ERROR(logger, user << ": Cannot set room subject. There is now conversation " << legacyName);
					return;
				}

				PurplePlugin *prpl = purple_find_prpl_wrapped(purple_account_get_protocol_id_wrapped(account));
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
				bool support_set_chat_topic = prpl_info && prpl_info->set_chat_topic;
				if (support_set_chat_topic) {
					LOG4CXX_INFO(logger, user << ": Setting room subject for room " << legacyName);
					prpl_info->set_chat_topic(purple_account_get_connection_wrapped(account),
											  purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)),
											  message.c_str());
				}
			}
		}

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				std::string name = legacyName;
				if (CONFIG_STRING(config, "service.protocol") == "any" && legacyName.find("prpl-") == 0) {
					name = name.substr(name.find(".") + 1);
				}
				m_vcards[user + name] = id;

				PurplePlugin *prpl = purple_find_prpl_wrapped(purple_account_get_protocol_id_wrapped(account));
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
				bool support_get_info = prpl_info && prpl_info->get_info;

				if (!support_get_info || (CONFIG_BOOL(config, "backend.no_vcard_fetch") && name != purple_account_get_username_wrapped(account))) {
					PurpleNotifyUserInfo *user_info = purple_notify_user_info_new_wrapped();
					notify_user_info(purple_account_get_connection_wrapped(account), name.c_str(), user_info);
					purple_notify_user_info_destroy_wrapped(user_info);
				}
				else {
					serv_get_info_wrapped(purple_account_get_connection_wrapped(account), name.c_str());
				}
			}
		}

		void handleVCardUpdatedRequest(const std::string &user, const std::string &image, const std::string &nickname) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				purple_account_set_alias_wrapped(account, nickname.c_str());
#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 7
				purple_account_set_public_alias_wrapped(account, nickname.c_str(), NULL, NULL);
#endif
				gssize size = image.size();
				// this will be freed by libpurple
				guchar *photo = (guchar *) g_malloc(size * sizeof(guchar));
				memcpy(photo, image.c_str(), size);

				if (!photo)
					return;
				purple_buddy_icons_set_account_icon_wrapped(account, photo, size);
			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
			PurpleAccount *account = m_sessions[user];
			if (!account)
				return;
			LOG4CXX_DEBUG(logger, "handleBuddyRemovedRequest(): removing buddy from authRequests");
			m_authRequests.deny(user, buddyName); //deny any outstanding friend requests
			PurpleBuddy *buddy = purple_find_buddy_wrapped(account, buddyName.c_str());
			if (buddy) {
				if (CONFIG_BOOL(config, "service.enable_remove_buddy")) {
					LOG4CXX_INFO(logger, "handleBuddyRemovedRequest(): removing buddy from the legacy contact list");
					purple_account_remove_buddy_wrapped(account, buddy, purple_buddy_get_group_wrapped(buddy));
					purple_blist_remove_buddy_wrapped(buddy);
				} else
					LOG4CXX_INFO(logger, "handleBuddyRemovedRequest(): service.enable_remove_buddy is off, leaving buddy in the legacy contact list");
			}
		}

		//Called when the frontend wants to:
		// 1. Update buddy params
		// 2. Accept their friend request (=> add to legacy contact list)
		// 3. Add them to legacy contact list
		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups_) {
			LOG4CXX_DEBUG(logger, "handleBuddyUpdatedRequest(): user= " << user << ", buddyName=" << buddyName);
			PurpleAccount *account = m_sessions[user];
			if (account) {
				std::string groups = groups_.empty() ? "" : groups_[0];

				m_authRequests.accept(user, buddyName);

				PurpleBuddy *buddy = purple_find_buddy_wrapped(account, buddyName.c_str());
				if (buddy) {
					if (getAlias(buddy) != alias) {
						purple_blist_alias_buddy_wrapped(buddy, alias.c_str());
						purple_blist_server_alias_buddy_wrapped(buddy, alias.c_str());
						serv_alias_buddy_wrapped(buddy);
					}

					PurpleGroup *group = purple_find_group_wrapped(groups.c_str());
					if (!group) {
						group = purple_group_new_wrapped(groups.c_str());
					}
					purple_blist_add_contact_wrapped(purple_buddy_get_contact_wrapped(buddy), group ,NULL);
				}
				else {
					PurpleBuddy *buddy = purple_buddy_new_wrapped(account, buddyName.c_str(), alias.c_str());

					// Add newly created buddy to legacy network roster.
					PurpleGroup *group = purple_find_group_wrapped(groups.c_str());
					if (!group) {
						group = purple_group_new_wrapped(groups.c_str());
					}
					purple_blist_add_buddy_wrapped(buddy, NULL, group ,NULL);
					purple_account_add_buddy_wrapped(account, buddy);
					LOG4CXX_INFO(logger, "Adding new buddy " << buddyName.c_str() << " to legacy network roster");
				}
			}
		}

		void handleBuddyBlockToggled(const std::string &user, const std::string &buddyName, bool blocked) {
			if (CONFIG_BOOL(config, "service.enable_privacy_lists")) {
				PurpleAccount *account = m_sessions[user];
				if (account) {
					if (blocked) {
						purple_privacy_deny_wrapped(account, buddyName.c_str(), FALSE, FALSE);
					}
					else {
						purple_privacy_allow_wrapped(account, buddyName.c_str(), FALSE, FALSE);
					}
				}
			}
		}

		void updateConversationActivity(PurpleAccount *account, const std::string &buddyName) {
			PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, buddyName.c_str(), account);
			if (!conv) {
				conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_IM, buddyName.c_str(), account);
			}
			if (conv) {
				purple_conversation_set_data_wrapped(conv, "unseen_count", 0);
				purple_conversation_update_wrapped(conv, PURPLE_CONV_UPDATE_UNSEEN);
			}
		}

		void handleTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				LOG4CXX_INFO(logger, user << ": sending typing notify to " << buddyName);
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_TYPING);
				updateConversationActivity(account, buddyName);
			}
		}

		void handleTypedRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_TYPED);
				updateConversationActivity(account, buddyName);
			}
		}

		void handleStoppedTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_NOT_TYPING);
				updateConversationActivity(account, buddyName);
			}
		}

		void handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				purple_prpl_send_attention_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), 0);
			}
		}

		std::string LegacyNameToName(PurpleAccount *account, const std::string &legacyName) {
			std::string conversationName = legacyName;
			BOOST_FOREACH(std::string _room, m_rooms[np->m_accounts[account]]) {
				std::string lowercased_room = boost::locale::to_lower(_room);
				if (lowercased_room.compare(conversationName) == 0) {
					conversationName = _room;
					break;
				}
			}
			return conversationName;
		}

		std::string NameToLegacyName(PurpleAccount *account, const std::string &legacyName) {
			std::string conversationName = legacyName;
			BOOST_FOREACH(std::string _room, m_rooms[np->m_accounts[account]]) {
				if (_room.compare(conversationName) == 0) {
					conversationName = boost::locale::to_lower(legacyName);
					break;
				}
			}
			return conversationName;
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &pasword) {
			PurpleAccount *account = m_sessions[user];
			if (!account) {
				return;
			}

			PurpleConnection *gc = purple_account_get_connection_wrapped(account);
			GHashTable *comps = NULL;
			std::string roomName = LegacyNameToName(account, room);
			// Check if the PurpleChat is not stored in buddy list
			PurpleChat *chat = purple_blist_find_chat_wrapped(account, roomName.c_str());
			if (chat) {
				comps = purple_chat_get_components_wrapped(chat);
			}
			else if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
				if (CONFIG_STRING(config, "service.protocol") == "prpl-jabber") {
					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, (roomName + "/" + nickname).c_str());
				} else {
					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, roomName.c_str());
				}
			}

			if (CONFIG_STRING(config, "service.protocol") != "prpl-jabber") {
                               np->handleParticipantChanged(np->m_accounts[account], nickname, room, 0, pbnetwork::STATUS_ONLINE);
                               const char *disp;
                               if ((disp = purple_account_get_name_for_display(account)) == NULL) {
                                       if ((disp = purple_connection_get_display_name(account->gc)) == NULL) {
                                               disp = purple_account_get_username(account);
                                       }
                               }
                               LOG4CXX_INFO(logger, user << ": Display name is " << disp << ", nickname is " << nickname);
                               if (nickname != disp) {
                                       handleRoomNicknameChanged(np->m_accounts[account], room, disp);
                                       np->handleParticipantChanged(np->m_accounts[account], nickname, room, 0, pbnetwork::STATUS_ONLINE, "", disp);
                               }
			}

			LOG4CXX_INFO(logger, user << ": Joining the room " << roomName);
			if (comps) {
				serv_join_chat_wrapped(gc, comps);
			}
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
			PurpleAccount *account = m_sessions[user];
			if (!account) {
				return;
			}

			PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, LegacyNameToName(account, room).c_str(), account);
			purple_conversation_destroy_wrapped(conv);
		}

		void handleFTStartRequest(const std::string &user, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long ftID) {
			PurpleXfer *xfer = m_unhandledXfers[user + fileName + buddyName];
			if (xfer) {
				m_unhandledXfers.erase(user + fileName + buddyName);
				FTData *ftData = (FTData *) xfer->ui_data;

				ftData->id = ftID;
				m_xfers[ftID] = xfer;
				purple_xfer_request_accepted_wrapped(xfer, fileName.c_str());
				purple_xfer_ui_ready_wrapped(xfer);
			}
		}

		void handleFTFinishRequest(const std::string &user, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long ftID) {
			PurpleXfer *xfer = m_unhandledXfers[user + fileName + buddyName];
			if (xfer) {
				m_unhandledXfers.erase(user + fileName + buddyName);
				purple_xfer_request_denied_wrapped(xfer);
			}
		}

		void handleFTPauseRequest(unsigned long ftID) {
			PurpleXfer *xfer = m_xfers[ftID];
			if (!xfer)
				return;
			FTData *ftData = (FTData *) xfer->ui_data;
			ftData->paused = true;
		}

		void handleFTContinueRequest(unsigned long ftID) {
			PurpleXfer *xfer = m_xfers[ftID];
			if (!xfer)
				return;
			FTData *ftData = (FTData *) xfer->ui_data;
			ftData->paused = false;
			purple_xfer_ui_ready_wrapped(xfer);
		}

		void sendData(const std::string &string) {
#ifdef WIN32
			::send(main_socket, string.c_str(), string.size(), 0);
#else
			write(main_socket, string.c_str(), string.size());
#endif
			if (writeInput == 0)
				writeInput = purple_input_add_wrapped(main_socket, PURPLE_INPUT_WRITE, &transportDataReceived, NULL);
		}

		void readyForData() {
			if (m_waitingXfers.empty())
				return;
			std::vector<PurpleXfer *> tmp;
			tmp.swap(m_waitingXfers);

			for (std::vector<PurpleXfer *>::const_iterator it = tmp.begin(); it != tmp.end(); it++) {
				FTData *ftData = (FTData *) (*it)->ui_data;
				if (ftData->timer == 0) {
					ftData->timer = purple_timeout_add_wrapped(1, ft_ui_ready, *it);
				}
// 				purple_xfer_ui_ready_wrapped(xfer);
			}
		}

		std::map<std::string, PurpleAccount *> m_sessions;
		std::map<PurpleAccount *, std::string> m_accounts;
		std::map<std::string, unsigned int> m_vcards;
		AuthRequestList m_authRequests;
		std::map<std::string, inputRequest *> m_inputRequests;
		std::map<std::string, std::list<std::string> > m_rooms;
		std::map<unsigned long, PurpleXfer *> m_xfers;
		std::map<std::string, PurpleXfer *> m_unhandledXfers;
		std::vector<PurpleXfer *> m_waitingXfers;
		std::string adminLegacyName;
		std::string adminAlias;
};

static bool getStatus(PurpleBuddy *m_buddy, pbnetwork::StatusType &status, std::string &statusMessage) {
	PurplePresence *pres = purple_buddy_get_presence_wrapped(m_buddy);
	if (pres == NULL)
		return false;
	PurpleStatus *stat = purple_presence_get_active_status_wrapped(pres);
	if (stat == NULL)
		return false;
	int st = purple_status_type_get_primitive_wrapped(purple_status_get_type_wrapped(stat));

	switch(st) {
		case PURPLE_STATUS_AVAILABLE: {
			status = pbnetwork::STATUS_ONLINE;
			break;
		}
		case PURPLE_STATUS_AWAY: {
			status = pbnetwork::STATUS_AWAY;
			break;
		}
		case PURPLE_STATUS_UNAVAILABLE: {
			status = pbnetwork::STATUS_DND;
			break;
		}
		case PURPLE_STATUS_EXTENDED_AWAY: {
			status = pbnetwork::STATUS_XA;
			break;
		}
		case PURPLE_STATUS_OFFLINE: {
			status = pbnetwork::STATUS_NONE;
			break;
		}
		default:
			status = pbnetwork::STATUS_ONLINE;
			break;
	}

	const char *message = purple_status_get_attr_string_wrapped(stat, "message");

	if (message != NULL) {
		char *stripped = purple_markup_strip_html_wrapped(message);
		statusMessage = std::string(stripped);
		g_free(stripped);
	}
	else
		statusMessage = "";
	return true;
}

static std::string getIconHash(PurpleBuddy *m_buddy) {
	char *avatarHash = NULL;
	PurpleBuddyIcon *icon = purple_buddy_icons_find_wrapped(purple_buddy_get_account_wrapped(m_buddy), purple_buddy_get_name_wrapped(m_buddy));
	if (icon) {
		avatarHash = purple_buddy_icon_get_full_path_wrapped(icon);
		purple_buddy_icon_unref_wrapped(icon);
	}

	if (avatarHash) {
		// Check if it's patched libpurple which saves icons to directories
		char *hash = strrchr(avatarHash,'/');
		std::string h;
		if (hash) {
			char *dot;
			hash++;
			dot = strchr(hash, '.');
			if (dot)
				*dot = '\0';

			std::string ret(hash);
			g_free(avatarHash);
			return ret;
		}
		else {
			std::string ret(avatarHash);
			g_free(avatarHash);
			return ret;
		}
	}

	return "";
}

static std::vector<std::string> getGroups(PurpleBuddy *m_buddy) {
	std::vector<std::string> groups;
	if (purple_buddy_get_name_wrapped(m_buddy)) {
		GSList *buddies = purple_find_buddies_wrapped(purple_buddy_get_account_wrapped(m_buddy), purple_buddy_get_name_wrapped(m_buddy));
		while (buddies) {
			PurpleGroup *g = purple_buddy_get_group_wrapped((PurpleBuddy *) buddies->data);
			buddies = g_slist_delete_link(buddies, buddies);

			if (g && purple_group_get_name_wrapped(g)) {
				groups.push_back(purple_group_get_name_wrapped(g));
			}
		}
	}

	if (groups.empty()) {
		groups.push_back("Buddies");
	}

	return groups;
}

void buddyListNewNode(PurpleBlistNode *node);

static gboolean new_node_cache(void *data) {
	NodeCache *cache = (NodeCache *) data;
	caching = false;
	for (std::map<PurpleBlistNode *, int>::const_iterator it = cache->nodes.begin(); it != cache->nodes.end(); it++) {
		buddyListNewNode(it->first);
	}
	caching = true;

	cache->account->ui_data = NULL;
	delete cache;

	return FALSE;
}

static void buddyNodeRemoved(PurpleBuddyList *list, PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account_wrapped(buddy);

	if (!account->ui_data) {
		return;
	}

	NodeCache *cache = (NodeCache *) account->ui_data;
	cache->nodes.erase(node);
}

void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account_wrapped(buddy);

	if (caching) {
		if (!account->ui_data) {
			NodeCache *cache = new NodeCache;
			cache->account = account;
			cache->timer = purple_timeout_add_wrapped(400, new_node_cache, cache);
			account->ui_data = (void *) cache;
		}

		NodeCache *cache = (NodeCache *) account->ui_data;
		cache->nodes[node] = 1;
		return;
	}


	std::vector<std::string> groups = getGroups(buddy);
	LOG4CXX_INFO(logger, "Buddy updated " << np->m_accounts[account] << " " << purple_buddy_get_name_wrapped(buddy) << " " << getAlias(buddy) << " group (" << groups.size() << ")=" << groups[0]);

	// Status
	pbnetwork::StatusType status = pbnetwork::STATUS_NONE;
	std::string message;
	getStatus(buddy, status, message);

	// Tooltip
	PurplePlugin *prpl = purple_find_prpl_wrapped(purple_account_get_protocol_id_wrapped(account));
	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	bool blocked = false;
	if (CONFIG_BOOL(config, "service.enable_privacy_lists")) {
		if (prpl_info && prpl_info->tooltip_text) {
			PurpleNotifyUserInfo *user_info = purple_notify_user_info_new_wrapped();
			prpl_info->tooltip_text(buddy, user_info, true);
			GList *entries = purple_notify_user_info_get_entries_wrapped(user_info);

			while (entries) {
				PurpleNotifyUserInfoEntry *entry = (PurpleNotifyUserInfoEntry *)(entries->data);
				if (purple_notify_user_info_entry_get_label_wrapped(entry) && purple_notify_user_info_entry_get_value_wrapped(entry)) {
					std::string label = purple_notify_user_info_entry_get_label_wrapped(entry);
					if (label == "Blocked" ) {
						if (std::string(purple_notify_user_info_entry_get_value_wrapped(entry)) == "Yes") {
							blocked = true;
							break;
						}
					}
				}
				entries = entries->next;
			}
			purple_notify_user_info_destroy_wrapped(user_info);
		}

		if (!blocked) {
			blocked = purple_privacy_check_wrapped(account, purple_buddy_get_name_wrapped(buddy)) == false;
		}
		else {
			bool purpleBlocked = purple_privacy_check_wrapped(account, purple_buddy_get_name_wrapped(buddy)) == false;
			if (blocked != purpleBlocked) {
				purple_privacy_deny_wrapped(account, purple_buddy_get_name_wrapped(buddy), FALSE, FALSE);
			}
		}
	}

	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name_wrapped(buddy), getAlias(buddy), getGroups(buddy), status, message, getIconHash(buddy),
		blocked
	);
}

static void buddyListUpdate(PurpleBuddyList *list, PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
	buddyListNewNode(node);
}

static void buddyPrivacyChanged(PurpleBlistNode *node, void *data) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
	buddyListUpdate(NULL, node);
}

static void NodeRemoved(PurpleBlistNode *node, void *data) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
// 	PurpleBuddy *buddy = (PurpleBuddy *) node;
}

static void buddyListSaveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
}

static void buddyListSaveAccount(PurpleAccount *account) {
}

static void buddyListRemoveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
}

static PurpleBlistUiOps blistUiOps =
{
	NULL,
	buddyListNewNode,
	NULL,
	buddyListUpdate,
	buddyNodeRemoved,
	NULL,
	NULL,
	NULL, // buddyListAddBuddy,
	NULL,
	NULL,
	buddyListSaveNode,
	buddyListRemoveNode,
	buddyListSaveAccount,
	NULL
};

static void conv_write_im(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);

static void conv_write(PurpleConversation *conv, const char *who, const char *alias, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	LOG4CXX_INFO(logger, "conv_write()");

	if (flags & PURPLE_MESSAGE_SYSTEM && CONFIG_STRING(config, "service.protocol") == "prpl-telegram") {
		PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	// 	char *striped = purple_markup_strip_html_wrapped(message);
	// 	std::string msg = striped;
	// 	g_free(striped);


		// Escape HTML characters.
		char *newline = purple_strdup_withhtml_wrapped(msg);
		char *strip, *xhtml;
		purple_markup_html_to_xhtml_wrapped(newline, &xhtml, &strip);
	// 	xhtml_linkified = spectrum_markup_linkify(xhtml);
		std::string message_(strip);

		std::string xhtml_(xhtml);
		g_free(newline);
		g_free(xhtml);
	// 	g_free(xhtml_linkified);
		g_free(strip);

		// AIM and XMPP adds <body>...</body> here...
		if (xhtml_.find("<body>") == 0) {
			xhtml_ = xhtml_.substr(6);
			if (xhtml_.find("</body>") != std::string::npos) {
				xhtml_ = xhtml_.substr(0, xhtml_.find("</body>"));
			}
		}

		if (xhtml_ == message_) {
			xhtml_ = "";
		}

		std::string timestamp;
		if (mtime && (unsigned long) time(NULL)-10 > (unsigned long) mtime/* && (unsigned long) time(NULL) - 31536000 < (unsigned long) mtime*/) {
			char buf[80];
			strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", gmtime(&mtime));
			timestamp = buf;
		}

		if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
			std::string w = purple_normalize_wrapped(account, who);
			size_t pos = w.find("/");
			if (pos != std::string::npos)
				w.erase((int) pos, w.length() - (int) pos);
			LOG4CXX_TRACE(logger, "Received message body='" << message_ << "' xhtml='" << xhtml_ << "' name='" << w << "'");
			np->handleMessage(np->m_accounts[account], w, message_, "", xhtml_, timestamp);
		}
		else {
			std::string conversationName = purple_conversation_get_name_wrapped(conv);
			LOG4CXX_TRACE(logger, "Received message body='" << message_ << "' name='" << conversationName << "' " << who);
			np->handleMessage(np->m_accounts[account], np->NameToLegacyName(account, conversationName), message_, who, xhtml_, timestamp);
		}
	}
	else {
	    //Handle all non-special cases by just passing them to conv_write_im
	    conv_write_im(conv, who, msg, flags, mtime);
	}
}

static char *calculate_data_hash(guchar *data, size_t len,
    const gchar *hash_algo)
{
	PurpleCipherContext *context;
	static gchar digest[129]; /* 512 bits hex + \0 */

	context = purple_cipher_context_new_by_name(hash_algo, NULL);
	if (context == NULL)
	{
		purple_debug_error("jabber", "Could not find %s cipher\n", hash_algo);
		g_return_val_if_reached(NULL);
	}

	/* Hash the data */
	purple_cipher_context_append(context, data, len);
	if (!purple_cipher_context_digest_to_str(context, sizeof(digest), digest, NULL))
	{
		purple_debug_error("jabber", "Failed to get digest for %s cipher.\n",
		    hash_algo);
		g_return_val_if_reached(NULL);
	}
	purple_cipher_context_destroy(context);

	return g_strdup(digest);
}


/*
Downloads an image by its purple_imgstore's ID and rehosts it locally
in the web_* image store.
Returns the rehosted image URI, or empty string if rehosting is disabled,
the image could not be downloaded or is subject to restrictions.
*/
std::string web_rehost_image_by_id(int id)
{
	std::string web_dir = CONFIG_STRING(config, "service.web_directory");
	std::string web_url = CONFIG_STRING(config, "service.web_url");
	if (web_dir.empty() || web_url.empty()) {
		LOG4CXX_DEBUG(logger, "Won't rehost image, web_directory is disabled.");
		return std::string();
	}

	PurpleStoredImage *image = purple_imgstore_find_by_id(id);
	if (!image) {
		LOG4CXX_ERROR(logger, "Cannot find image with id " << id << ".");
		return std::string();
	}

	std::string ext = "icon";
	std::string name;
	guchar * data = (guchar *) purple_imgstore_get_data_wrapped(image);
	size_t len = purple_imgstore_get_size_wrapped(image);
	if (len < CONFIG_INT(config, "service.web_maximgsize") && data) {
		ext = purple_imgstore_get_extension(image);
		char *hash = calculate_data_hash(data, len, "sha1");
		if (!hash) {
			LOG4CXX_WARN(logger, "Cannot compute hash for the image.");
			return std::string();
		}
		name = hash;
		g_free(hash);

		std::ofstream output;
		std::string fpath (web_dir + "/" + name + "." + ext);
		LOG4CXX_DEBUG(logger, "Storing image to " << fpath);
		struct stat buffer;
		if (stat(fpath.c_str(), &buffer) == 0) {
			LOG4CXX_DEBUG(logger, "File already exists, skipping.");
			//If the file exists, skip writing but make sure to update mtime:
			//otherwise people can't rely on it to trim cache (newer messages may reference older images)
			utime(fpath.c_str(), NULL);
		} else {
			output.open(fpath.c_str(), std::ios::out | std::ios::binary);
			if (output.fail()) {
				LOG4CXX_ERROR(logger, "Open file failure: " << strerror(errno));
				return std::string();
			}
			output.write((char *)data, len);
			output.close();
		}
	}
	else {
		LOG4CXX_WARN(logger, "Image bigger than the allowed size (web_maximgsize).");
		purple_imgstore_unref_wrapped(image);
		return std::string();
	}
	purple_imgstore_unref_wrapped(image);
	return web_url + "/" + name + "." + ext;
}


/*
Converts HTML message body to XHTML and plain text representations.
Requires and populates:
  xhtml_
  plain_
May return empty strings if nothing is left after filtering out invalid contents.
*/
static void conv_msg_to_plain(const char* msg, std::string* xhtml_, std::string* plain_)
{
	//LOG4CXX_TRACE(logger, "conv_message_to_plain(): msg='" << msg << "'");
	char *newline = purple_strdup_withhtml_wrapped(msg); //Escape HTML characters.
	char *strip, *xhtml;
	purple_markup_html_to_xhtml_wrapped(newline, &xhtml, &strip);
	*plain_ = strip;
	*xhtml_ = xhtml;
	g_free(newline);
	g_free(xhtml);
	g_free(strip);
	//LOG4CXX_TRACE(logger, "conv_message_to_plain(): plain='" << plain_ << "' xhtml='" << xhtml_ << "'");
}

/*
Converts a PURPLE_MESSAGE_IMAGE by storing the image in the image store and adjusting
the message text to point to the new image URI / replacement text.
Requires and populates:
  xhtml_
  plain_
Returns false if the adjustment for this image cannot be done.
*/
static bool conv_msg_to_image(const char* msg, std::string* xhtml_, std::string* plain_)
{
	/*
	There could be one or more <img id=> instances. We need to parse each separately.
	If web_directory is disabled, we still need to replace each with a replacement text 
	*/

	LOG4CXX_DEBUG(logger, "Received image body='" << msg << "'");
	std::string body = msg;
	std::string plain = msg;

	size_t tag_from = -1;
	while ((tag_from = body.find("<img id=", tag_from+1)) != std::string::npos) {
		//Different plugins use different quote marks, or maybe none
		int id_from = tag_from + strlen("<img id=");
		char quoteMark = 0x00;
		if ((body[id_from] == '"') || (body[id_from] == '\'') || (body[id_from] == '`')) {
			quoteMark = body[id_from];
			id_from++;
		}

		int id_to = 0; //last char + 1
		int attr_to = 0; //last char incl. quotes + 1
		if (quoteMark != 0x00) {
			id_to = body.find(quoteMark, id_from + 1);
			attr_to = id_to + 1;
		} else { //without quotes the id ends on tag end or a space
			id_to = body.find_first_of(" >/", id_from + 1);
			attr_to = id_to;
		}
		int tag_to = body.find('>', attr_to);
		if ((id_to == std::string::npos) || (tag_to == std::string::npos)) {
			LOG4CXX_ERROR(logger, "Malformed image tag, cannot find attribute/tag end.");
			continue; //maybe there are other tags
		}
		tag_to++;

		std::string id = body.substr(id_from, id_to - id_from);
		std::string attr = body.substr(tag_from, attr_to - tag_from); //without tag end
		std::string tag = body.substr(tag_from, tag_to - tag_from);
		LOG4CXX_DEBUG(logger, "Image ID = '" << id << "' " << id_from << " " << id_to);

		std::string new_uri = web_rehost_image_by_id(atoi(id.c_str()));
		if (!new_uri.empty()) {
			std::string img = "<img src=\"" + new_uri + "\""; //keep the rest of the tag
			boost::replace_all(body, attr, img);
			boost::replace_all(plain, tag, new_uri); //the entire tag
		} else {
			//Replace with placeholder text
			std::string img_text = "[The user has sent you an image]";
			boost::replace_all(body, tag, img_text);
			boost::replace_all(plain, tag, img_text);
		}
	}
	LOG4CXX_TRACE(logger, "New body='" << body << "', plain='" << plain << "'");

	//We've processed <img> tags but still need to sanitize the rest of the markup
	//Convert this adjusted HTML to XHTML
	char *strip, *xhtml;
	purple_markup_html_to_xhtml_wrapped(body.c_str(), &xhtml, &strip);
	*xhtml_ = xhtml;
	g_free(xhtml);
	g_free(strip);
	LOG4CXX_TRACE(logger, "New xhtml='" << xhtml_ << "'");

	//For plaintext use our version with plain URIs or they'll be stripped
	purple_markup_html_to_xhtml_wrapped(plain.c_str(), &xhtml, &strip);
	*plain_ = strip;
	g_free(xhtml);
	g_free(strip);
	LOG4CXX_TRACE(logger, "New plaintext='" << plain_ << "'");

	return true;
}


static void conv_write_im(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	LOG4CXX_INFO(logger, "conv_write_im()");
	bool isCarbon = false;

	if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
		//Don't forwards our own messages, but do forward messages "from=us" which originated elsewhere
		//(such as carbons of our messages from other legacy network clients)
		if (flags & PURPLE_MESSAGE_SPECTRUM2_ORIGINATED) {
			LOG4CXX_INFO(logger, "conv_write_im(): ignoring a message generated by us");
			return;
		}

		//If this is a carbon of a message from us, mark it as such
		if (flags & PURPLE_MESSAGE_SEND)
			isCarbon = true;

		//Ignore system messages as those are normally not true messages in the XMPP sense
		if (flags & PURPLE_MESSAGE_SYSTEM) {
			LOG4CXX_INFO(logger, "conv_write_im(): ignoring a system message");
			return;
		}
	}
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	std::string message_; //plain text
	std::string xhtml_;   //enhanced xhtml, if available

	LOG4CXX_DEBUG(logger, "conv_write_im(): msg='" << msg << "', flags=" << flags);

	if (flags & PURPLE_MESSAGE_IMAGES) {
		//Store image locally and adjust the message
		if (!conv_msg_to_image(msg, &xhtml_, &message_))
		{
			//Fallback to plaintext treatment which is likely to be empty
			conv_msg_to_plain(msg, &xhtml_, &message_);
			if (message_.empty())
				message_ = "[The user has sent you an image]";
		}
	}
	else {
		conv_msg_to_plain(msg, &xhtml_, &message_);
	}

	// AIM and XMPP adds <body>...</body> here...
	if (xhtml_.find("<body>") == 0) {
		xhtml_ = xhtml_.substr(6);
		if (xhtml_.find("</body>") != std::string::npos) {
			xhtml_ = xhtml_.substr(0, xhtml_.find("</body>"));
		}
	}

	if (xhtml_ == message_) {
		xhtml_ = "";
	}

	std::string timestamp;
	if (mtime && (unsigned long) time(NULL)-10 > (unsigned long) mtime/* && (unsigned long) time(NULL) - 31536000 < (unsigned long) mtime*/) {
		char buf[80];
		strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", gmtime(&mtime));
		timestamp = buf;
	}

	if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
		std::string w = purple_normalize_wrapped(account, who);
		std::string n;
		size_t pos = w.find("/");
		if (pos != std::string::npos) {
			n = w.substr((int) pos + 1, w.length() - (int) pos);
			w.erase((int) pos, w.length() - (int) pos);
		}
		LOG4CXX_TRACE(logger, "Received message body='" << message_ << "' xhtml='" << xhtml_ << "' name='" << w << "'");
		np->handleMessage(np->m_accounts[account], w, message_, n, xhtml_, timestamp, false, false, isCarbon);
	}
	else {
		std::string conversationName = purple_conversation_get_name_wrapped(conv);
		LOG4CXX_TRACE(logger, "Received message body='" << message_ << "' xhtml='" << xhtml_ << "' name='" << conversationName << "' " << who);
		np->handleMessage(np->m_accounts[account], np->NameToLegacyName(account, conversationName), message_, who, xhtml_, timestamp, false, false, isCarbon);
	}
}

static void conv_chat_add_users(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	GList *l = cbuddies;
	while (l != NULL) {
		PurpleConvChatBuddy *cb = (PurpleConvChatBuddy *)l->data;
		std::string name(cb->name);
		std::string alias = cb->alias ? cb->alias : cb->name;
		int flags = GPOINTER_TO_INT(cb->flags);
		if (flags & PURPLE_CBFLAGS_OP || flags & PURPLE_CBFLAGS_HALFOP) {
// 			item->addAttribute("affiliation", "admin");
// 			item->addAttribute("role", "moderator");
			flags = 1;
		}
		else if (flags & PURPLE_CBFLAGS_FOUNDER) {
// 			item->addAttribute("affiliation", "owner");
// 			item->addAttribute("role", "moderator");
			flags = 1;
		}
		else {
			flags = 0;
// 			item->addAttribute("affiliation", "member");
// 			item->addAttribute("role", "participant");
		}
		std::string conversationName = purple_conversation_get_name_wrapped(conv);
		np->handleParticipantChanged(np->m_accounts[account], name, np->NameToLegacyName(account, conversationName), (int) flags, pbnetwork::STATUS_ONLINE, "", "", alias);

		l = l->next;
	}
}

static void conv_chat_remove_users(PurpleConversation *conv, GList *users) {
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	GList *l = users;
	while (l != NULL) {
		std::string name((char *) l->data);
		std::string conversationName = purple_conversation_get_name_wrapped(conv);
		np->handleParticipantChanged(np->m_accounts[account], name, np->NameToLegacyName(account, conversationName), 0, pbnetwork::STATUS_NONE);

		l = l->next;
	}
}

static gboolean conv_has_focus(PurpleConversation *conv) {
	return TRUE;
}

static void conv_chat_topic_changed(PurpleConversation *conv, const char *who, const char *topic) {
	LOG4CXX_INFO(logger, "Conversation topic changed");
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);
	np->handleSubject(np->m_accounts[account], np->NameToLegacyName(account, purple_conversation_get_name_wrapped(conv)), topic ? topic : "", who ? who : "Spectrum 2");
}

static void conv_present(PurpleConversation *conv) {
	if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_CHAT) {
		LOG4CXX_INFO(logger, "Conversation presented");
		conv_chat_add_users(conv, PURPLE_CONV_CHAT_WRAPPED(conv)->in_room, TRUE);
		const char *topic = purple_conv_chat_get_topic(PURPLE_CONV_CHAT_WRAPPED(conv));
		if (topic && *topic != '\0') {
			conv_chat_topic_changed(conv, topic, PURPLE_CONV_CHAT_WRAPPED(conv)->who);
		}
		else {
			LOG4CXX_INFO(logger, "Conversation created with an empty topic");
		}
	}
}

static PurpleConversationUiOps conversation_ui_ops =
{
	NULL,
	NULL,
	conv_write_im,//conv_write_chat,                              /* write_chat           */
	conv_write_im,             /* write_im             */
	conv_write,//conv_write_conv,           /* write_conv           */
	conv_chat_add_users,       /* chat_add_users       */
	NULL,//conv_chat_rename_user,     /* chat_rename_user     */
	conv_chat_remove_users,    /* chat_remove_users    */
	NULL,//pidgin_conv_chat_update_user,     /* chat_update_user     */
	conv_present,//pidgin_conv_present_conversation, /* present              */
	conv_has_focus,//pidgin_conv_has_focus,            /* has_focus            */
	NULL,//pidgin_conv_custom_smiley_add,    /* custom_smiley_add    */
	NULL,//pidgin_conv_custom_smiley_write,  /* custom_smiley_write  */
	NULL,//pidgin_conv_custom_smiley_close,  /* custom_smiley_close  */
	NULL,//pidgin_conv_send_confirm,         /* send_confirm         */
	NULL,
	NULL,
	NULL,
	NULL
};

struct Dis {
	std::string name;
	std::string protocol;
};

// currently unused
#if 0
static gboolean disconnectMe(void *data) {
	Dis *d = (Dis *) data;
	PurpleAccount *account = purple_accounts_find_wrapped(d->name.c_str(), d->protocol.c_str());
	delete d;

	if (account) {
		np->handleLogoutRequest(np->m_accounts[account], purple_account_get_username_wrapped(account));
	}
	return FALSE;
}
#endif

static gboolean pingTimeout(void *data) {
	np->checkPing();
	return TRUE;
}

static void connection_report_disconnect(PurpleConnection *gc, PurpleConnectionError reason, const char *text){
	PurpleAccount *account = purple_connection_get_account_wrapped(gc);
	np->handleDisconnected(np->m_accounts[account], (int) reason, text ? text : "");
// 	Dis *d = new Dis;
// 	d->name = purple_account_get_username_wrapped(account);
// 	d->protocol = purple_account_get_protocol_id_wrapped(account);
// 	purple_timeout_add_seconds_wrapped(10, disconnectMe, d);
}

static PurpleConnectionUiOps conn_ui_ops =
{
	NULL,
	NULL,
	NULL,//connection_disconnected,
	NULL,
	NULL,
	NULL,
	NULL,
	connection_report_disconnect,
	NULL,
	NULL,
	NULL
};

static void *notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info) {
	PurpleAccount *account = purple_connection_get_account_wrapped(gc);
	std::string name(purple_normalize_wrapped(account, who));

	size_t pos = name.find("/");
	if (pos != std::string::npos)
		name.erase((int) pos, name.length() - (int) pos);


	GList *vcardEntries = purple_notify_user_info_get_entries_wrapped(user_info);
	PurpleNotifyUserInfoEntry *vcardEntry;
	std::string firstName;
	std::string lastName;
	std::string fullName;
	std::string nickname;
	std::string header;
	std::string label;
	std::string photo;

	while (vcardEntries) {
		vcardEntry = (PurpleNotifyUserInfoEntry *)(vcardEntries->data);
		if (purple_notify_user_info_entry_get_label_wrapped(vcardEntry) && purple_notify_user_info_entry_get_value_wrapped(vcardEntry)){
			label = boost::locale::to_lower(purple_notify_user_info_entry_get_label_wrapped(vcardEntry));
			if (label == "given name" || label == "first name") {
				firstName = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label == "family name" || label == "last name") {
				lastName = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label=="nickname" || label == "nick") {
				nickname = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label=="full name" || label == "display name") {
				fullName = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else {
				LOG4CXX_WARN(logger, "Unhandled VCard Label '" << purple_notify_user_info_entry_get_label_wrapped(vcardEntry) << "' " << purple_notify_user_info_entry_get_value_wrapped(vcardEntry));
			}
		}
		vcardEntries = vcardEntries->next;
	}

	if ((!firstName.empty() || !lastName.empty()) && fullName.empty())
		fullName = firstName + " " + lastName;

	if (nickname.empty() && !fullName.empty()) {
		nickname = fullName;
	}

	bool ownInfo = name == purple_account_get_username_wrapped(account);

	if (ownInfo) {
		const gchar *displayname = purple_connection_get_display_name_wrapped(gc);
#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 7
		if (!displayname) {
			displayname = purple_account_get_name_for_display_wrapped(account);
		}
#endif

		if (displayname && nickname.empty()) {
			nickname = displayname;
		}

		// avatar
		PurpleStoredImage *avatar = purple_buddy_icons_find_account_icon_wrapped(account);
		if (avatar) {
			const gchar * data = (const gchar *) purple_imgstore_get_data_wrapped(avatar);
			size_t len = purple_imgstore_get_size_wrapped(avatar);
			if (len < 300000 && data) {
				photo = std::string(data, len);
			}
			purple_imgstore_unref_wrapped(avatar);
		}
	}

	PurpleBuddy *buddy = purple_find_buddy_wrapped(purple_connection_get_account_wrapped(gc), who);
	if (buddy && photo.size() == 0) {
		gsize len;
		PurpleBuddyIcon *icon = NULL;
		icon = purple_buddy_icons_find_wrapped(purple_connection_get_account_wrapped(gc), name.c_str());
		if (icon) {
			if (true) {
				gchar *data;
				gchar *path = purple_buddy_icon_get_full_path_wrapped(icon);
				if (path) {
					if (g_file_get_contents(path, &data, &len, NULL)) {
						photo = std::string(data, len);
						g_free(data);
					}
					g_free(path);
				}
			}
			else {
				const gchar * data = (gchar*)purple_buddy_icon_get_data_wrapped(icon, &len);
				if (len < 300000 && data) {
					photo = std::string(data, len);
				}
			}
			purple_buddy_icon_unref_wrapped(icon);
		}
	}

	np->handleVCard(np->m_accounts[account], np->m_vcards[np->m_accounts[account] + name], name, fullName, nickname, photo);
	np->m_vcards.erase(np->m_accounts[account] + name);

	return NULL;
}

void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value, gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	if (primary) {
		std::string primaryString(primary);
		if (primaryString == "Authorization Request Message:") {
			LOG4CXX_INFO(logger, "Authorization Request Message: calling ok_cb(...)");
			((PurpleRequestInputCb) ok_cb)(user_data, "Please authorize me.");
			return NULL;
		}
		else if (primaryString == "Authorization Request Message:") {
			LOG4CXX_INFO(logger, "Authorization Request Message: calling ok_cb(...)");
			((PurpleRequestInputCb) ok_cb)(user_data, "Please authorize me.");
			return NULL;
		}
		else if (primaryString == "Authorization Denied Message:") {
			LOG4CXX_INFO(logger, "Authorization Deined Message: calling ok_cb(...)");
			((PurpleRequestInputCb) ok_cb)(user_data, "Authorization denied.");
			return NULL;
		}
		else if (boost::starts_with(primaryString, "https://accounts.google.com/o/oauth2/auth") ||
                                boost::starts_with(primaryString, "https://www.youtube.com/watch?v=hlDhp-eNLMU")) {
			LOG4CXX_INFO(logger, "prpl-hangouts oauth request");
			np->handleMessage(np->m_accounts[account], np->adminLegacyName, std::string("Please visit the following link and authorize this application: ") + primaryString, "");
			np->handleMessage(np->m_accounts[account], np->adminLegacyName, std::string("Reply with code provided by Google: "));
			inputRequest *req = new inputRequest;
			req->ok_cb = (PurpleRequestInputCb)ok_cb;
			req->user_data = user_data;
			req->account = account;
			req->mainJID = np->m_accounts[account];
			np->m_inputRequests[req->mainJID] = req;
			return NULL;
		}
		else if (primaryString == "Set your Steam Guard Code" || primaryString == "Steam two-factor authentication") {
			LOG4CXX_INFO(logger, "prpl-steam-mobile steam guard request");
			np->handleMessage(np->m_accounts[account], np->adminLegacyName, std::string("Steam Guard code: "));
			inputRequest *req = new inputRequest;
			req->ok_cb = (PurpleRequestInputCb)ok_cb;
			req->user_data = user_data;
			req->account = account;
			req->mainJID = np->m_accounts[account];
			np->m_inputRequests[req->mainJID] = req;
			return NULL;
		}
		else if (primaryString == "Enter Discord auth code") {
			LOG4CXX_INFO(logger, "prpl-discord 2FA request");
			np->handleMessage(np->m_accounts[account], np->adminLegacyName, std::string("2FA code: "));
			inputRequest *req = new inputRequest;
			req->ok_cb = (PurpleRequestInputCb)ok_cb;
			req->user_data = user_data;
			req->account = account;
			req->mainJID = np->m_accounts[account];
			np->m_inputRequests[req->mainJID] = req;
			return NULL;
		}
		else if (boost::starts_with(primaryString, "Enter authentication code")) {
			LOG4CXX_INFO(logger, "telegram-tdlib 2FA request");
			np->handleMessage(np->m_accounts[account], np->adminLegacyName, std::string("Authentication code: "));
			inputRequest *req = new inputRequest;
			req->ok_cb = (PurpleRequestInputCb)ok_cb;
			req->user_data = user_data;
			req->account = account;
			req->mainJID = np->m_accounts[account];
			np->m_inputRequests[req->mainJID] = req;
			return NULL;
		}
		else {
			LOG4CXX_WARN(logger, "Unhandled request input. primary=" << primaryString);
		}
	}
    else if (title) {
        std::string titleString(title);
        if (titleString == "Xfire Invitation Message") {
            LOG4CXX_INFO(logger, "Authorization Request Message: calling ok_cb(...)");
            ((PurpleRequestInputCb) ok_cb)(user_data, "Please authorize me.");
            return NULL;
        }
        else {
            LOG4CXX_WARN(logger, "Unhandled request input. title=" << titleString);
        }
    }
    else {
        LOG4CXX_WARN(logger, "Request input without primary string");
    }
    return NULL;
}

static PurpleNotifyUiOps notifyUiOps =
{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		notify_user_info,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};

static PurpleRequestUiOps requestUiOps =
{
	requestInput,
	NULL,
	requestAction,
	NULL,
	NULL,
	NULL, //requestClose,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void * accountRequestAuth(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data) {
	authRequest *req = new authRequest;
	req->authorize_cb = authorize_cb;
	req->deny_cb = deny_cb;
	req->user_data = user_data;
	req->account = account;
	req->who = remote_user;
	req->mainJID = np->m_accounts[account];
	np->m_authRequests[req->mainJID + req->who] = req;

	np->handleAuthorization(req->mainJID, req->who);

	return req;
}

static void accountRequestClose(void *data){
	authRequest *req = (authRequest *) data;
	np->m_authRequests.erase(req->mainJID + req->who);
}


static PurpleAccountUiOps accountUiOps =
{
	NULL,
	NULL,
	NULL,
	accountRequestAuth,
	accountRequestClose,
	NULL,
	NULL,
	NULL,
	NULL
};

static void XferCreated(PurpleXfer *xfer) {
	if (!xfer) {
		return;
	}

 	PurpleAccount *account = purple_xfer_get_account_wrapped(xfer);
 	np->handleFTStart(np->m_accounts[account], xfer->who, purple_xfer_get_filename_wrapped(xfer), purple_xfer_get_size_wrapped(xfer));
}

static void XferDestroyed(PurpleXfer *xfer) {
	std::remove(np->m_waitingXfers.begin(), np->m_waitingXfers.end(), xfer);
	FTData *ftdata = (FTData *) xfer->ui_data;
	if (ftdata && ftdata->timer) {
		purple_timeout_remove_wrapped(ftdata->timer);
	}
	if (ftdata) {
		np->m_xfers.erase(ftdata->id);
	}
}

static void xferCanceled(PurpleXfer *xfer) {
	PurpleAccount *account = purple_xfer_get_account_wrapped(xfer);
	std::string filename(xfer ? purple_xfer_get_filename_wrapped(xfer) : "");
	std::string w = xfer->who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);

	FTData *ftdata = (FTData *) xfer->ui_data;

	np->handleFTFinish(np->m_accounts[account], w, filename, purple_xfer_get_size_wrapped(xfer), ftdata ? ftdata->id : 0);
	std::remove(np->m_waitingXfers.begin(), np->m_waitingXfers.end(), xfer);
	if (ftdata && ftdata->timer) {
		purple_timeout_remove_wrapped(ftdata->timer);
	}
	purple_xfer_unref_wrapped(xfer);
}

static void fileSendStart(PurpleXfer *xfer) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	repeater->fileSendStart();
}

static void fileRecvStart(PurpleXfer *xfer) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	repeater->fileRecvStart();
	FTData *ftData = (FTData *) xfer->ui_data;
	if (ftData->timer == 0) {
		ftData->timer = purple_timeout_add_wrapped(1, ft_ui_ready, xfer);
	}
}

static void newXfer(PurpleXfer *xfer) {
	PurpleAccount *account = purple_xfer_get_account_wrapped(xfer);
	std::string filename(xfer ? purple_xfer_get_filename_wrapped(xfer) : "");
	purple_xfer_ref_wrapped(xfer);
	std::string w = xfer->who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);

	FTData *ftdata = new FTData;
	ftdata->paused = false;
	ftdata->id = 0;
	ftdata->timer = 0;
	xfer->ui_data = (void *) ftdata;

	np->m_unhandledXfers[np->m_accounts[account] + filename + w] = xfer;

	np->handleFTStart(np->m_accounts[account], w, filename, purple_xfer_get_size_wrapped(xfer));
}

static void XferReceiveComplete(PurpleXfer *xfer) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	repeater->_tryToDeleteMe();
// 	GlooxMessageHandler::instance()->ftManager->handleXferFileReceiveComplete(xfer);
	std::remove(np->m_waitingXfers.begin(), np->m_waitingXfers.end(), xfer);
	FTData *ftdata = (FTData *) xfer->ui_data;
	if (ftdata && ftdata->timer) {
		purple_timeout_remove_wrapped(ftdata->timer);
	}
	purple_xfer_unref_wrapped(xfer);
}

static void XferSendComplete(PurpleXfer *xfer) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	repeater->_tryToDeleteMe();
	std::remove(np->m_waitingXfers.begin(), np->m_waitingXfers.end(), xfer);
	FTData *ftdata = (FTData *) xfer->ui_data;
	if (ftdata && ftdata->timer) {
		purple_timeout_remove_wrapped(ftdata->timer);
	}
	purple_xfer_unref_wrapped(xfer);
}

static gssize XferWrite(PurpleXfer *xfer, const guchar *buffer, gssize size) {
	FTData *ftData = (FTData *) xfer->ui_data;
	std::string data((const char *) buffer, (size_t) size);
// 	std::cout << "xferwrite\n";
	if (!ftData->paused) {
// 		std::cout << "adding xfer to waitingXfers queue\n";
		np->m_waitingXfers.push_back(xfer);
	}
	np->handleFTData(ftData->id, data);
	return size;
}

static void XferNotSent(PurpleXfer *xfer, const guchar *buffer, gsize size) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	repeater->handleDataNotSent(buffer, size);
}

static gssize XferRead(PurpleXfer *xfer, guchar **buffer, gssize size) {
// 	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
// 	int data_size = repeater->getDataToSend(buffer, size);
// 	if (data_size == 0)
// 		return 0;
//
// 	return data_size;
	return 0;
}

static PurpleXferUiOps xferUiOps =
{
	XferCreated,
	XferDestroyed,
	NULL,
	NULL,
	xferCanceled,
	xferCanceled,
	XferWrite,
	XferRead,
	XferNotSent,
	NULL
};

static void RoomlistProgress(PurpleRoomlist *list, gboolean in_progress)
{
	if (!in_progress) {
		GList *fields = purple_roomlist_get_fields(list);
		GList *field;
		int topicId = -1;
		int usersId = -1;
		int id = 0;
		for (field = fields; field != NULL; field = field->next, id++) {
			PurpleRoomlistField *f = (PurpleRoomlistField *) field->data;

			// Use the first visible string field as a fallback topic
			if (topicId == -1 && id != 0 && !f->hidden &&
				f->type == PURPLE_ROOMLIST_FIELD_STRING) {
				topicId = id;
			}

			if (!f || !f->name) {
				continue;
			}
			std::string fstring = f->name;
			if (fstring == "topic" || fstring == "description") {
				topicId = id;
			}
			else if (fstring == "users") {
				usersId = id;
			}
			else {
				LOG4CXX_INFO(logger, "Unknown RoomList field " << fstring);
			}
		}

		GList *rooms;
		std::list<std::string> m_topics;
		PurplePlugin *plugin = purple_find_prpl_wrapped(purple_account_get_protocol_id_wrapped(list->account));
		PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
		for (rooms = list->rooms; rooms != NULL; rooms = rooms->next) {
			PurpleRoomlistRoom *room = (PurpleRoomlistRoom *)rooms->data;
			if (room->type == PURPLE_ROOMLIST_ROOMTYPE_CATEGORY) continue;
			std::string roomId = prpl_info && prpl_info->roomlist_room_serialize ?
				prpl_info->roomlist_room_serialize(room)
				: room->name;
			np->m_rooms[np->m_accounts[list->account]].push_back(roomId);

			if (topicId == -1) {
				m_topics.push_back(room->name);
			}
			else {
				char *topic = (char *) g_list_nth_data(purple_roomlist_room_get_fields(room), topicId);
				if (topic) {
					m_topics.push_back(topic);
				}
				else {
					if (usersId) {
						char *users = (char *) g_list_nth_data(purple_roomlist_room_get_fields(room), usersId);
						if (users) {
							m_topics.push_back(users);
						}
						else {
							LOG4CXX_WARN(logger, "RoomList topic and users is NULL");
							m_topics.push_back(room->name);
						}
					}
					else {
						LOG4CXX_WARN(logger, "RoomList topic is NULL");
						m_topics.push_back(room->name);
					}
				}
			}
		}

		std::string user = "";
		if (list->account) {
			user = np->m_accounts[list->account];
		}

		LOG4CXX_INFO(logger, "RoomList is fetched for user " << user);
		np->handleRoomList(user, np->m_rooms[user], m_topics);
	}
	else {
		LOG4CXX_INFO(logger, "RoomList is still in progress");
	}
}

static PurpleRoomlistUiOps roomlist_ui_ops =
{
	NULL,
	NULL,
	NULL,
	NULL,
	RoomlistProgress,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops_wrapped(&blistUiOps);
	purple_accounts_set_ui_ops_wrapped(&accountUiOps);
	purple_notify_set_ui_ops_wrapped(&notifyUiOps);
	purple_request_set_ui_ops_wrapped(&requestUiOps);
	purple_xfers_set_ui_ops_wrapped(&xferUiOps);
	purple_connections_set_ui_ops_wrapped(&conn_ui_ops);
	purple_conversations_set_ui_ops_wrapped(&conversation_ui_ops);
	purple_roomlist_set_ui_ops_wrapped(&roomlist_ui_ops);

// #ifndef WIN32
// 	purple_dnsquery_set_ui_ops_wrapped(getDNSUiOps());
// #endif
}

/***** Core Ui Ops *****/
static void
spectrum_glib_log_handler(const gchar *domain,
						  GLogLevelFlags flags,
						  const gchar *message,
						  gpointer user_data)
{
	const char *level;
	char *new_msg = NULL;
	char *new_domain = NULL;

	if ((flags & G_LOG_LEVEL_ERROR) == G_LOG_LEVEL_ERROR)
		level = "ERROR";
	else if ((flags & G_LOG_LEVEL_CRITICAL) == G_LOG_LEVEL_CRITICAL)
		level = "CRITICAL";
	else if ((flags & G_LOG_LEVEL_WARNING) == G_LOG_LEVEL_WARNING)
		level = "WARNING";
	else if ((flags & G_LOG_LEVEL_MESSAGE) == G_LOG_LEVEL_MESSAGE)
		level = "MESSAGE";
	else if ((flags & G_LOG_LEVEL_INFO) == G_LOG_LEVEL_INFO)
		level = "INFO";
	else if ((flags & G_LOG_LEVEL_DEBUG) == G_LOG_LEVEL_DEBUG)
		level = "DEBUG";
	else {
		LOG4CXX_ERROR(logger, "Unknown glib logging level in " << (guint)flags);
		level = "UNKNOWN"; /* This will never happen. */
	}

	if (message != NULL)
		new_msg = purple_utf8_try_convert_wrapped(message);

	if (domain != NULL)
		new_domain = purple_utf8_try_convert_wrapped(domain);

	if (new_msg != NULL) {
		std::string area("glib");
		area.push_back('/');
		area.append(level);

		std::string message(new_domain ? new_domain : "g_log");
		message.push_back(' ');
		message.append(new_msg);

		LOG4CXX_ERROR(logger, message);
		g_free(new_msg);
	}

	g_free(new_domain);
}

static void
debug_init(void)
{
#define REGISTER_G_LOG_HANDLER(name) \
	g_log_set_handler((name), \
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL \
										  | G_LOG_FLAG_RECURSION), \
					  spectrum_glib_log_handler, NULL)

	REGISTER_G_LOG_HANDLER(NULL);
	REGISTER_G_LOG_HANDLER("GLib");
	REGISTER_G_LOG_HANDLER("GModule");
	REGISTER_G_LOG_HANDLER("GLib-GObject");
	REGISTER_G_LOG_HANDLER("GThread");
	REGISTER_G_LOG_HANDLER("GConf");


#undef REGISTER_G_LOD_HANDLER
}

static PurpleCoreUiOps coreUiOps =
{
	NULL,
	debug_init,
	transport_core_ui_init,
	NULL,
	spectrum_ui_get_info,
	NULL,
	NULL,
	NULL
};

static void signed_on(PurpleConnection *gc, gpointer unused) {
	PurpleAccount *account = purple_connection_get_account_wrapped(gc);
	np->handleConnected(np->m_accounts[account]);
#ifndef WIN32
#if !defined(__FreeBSD__) && !defined(__APPLE__)
	// force returning of memory chunks allocated by libxml2 to kernel
	malloc_trim(0);
#endif
#endif
	purple_roomlist_get_list_wrapped(gc);

	// For prpl-gg
	execute_purple_plugin_action(gc, "Download buddylist from Server");
	if (CONFIG_STRING(config, "service.protocol") == "prpl-hangouts") {
		storeUserToken(np->m_accounts[account], OAUTH_TOKEN, purple_account_get_password_wrapped(account));
	}
	else if (CONFIG_STRING(config, "service.protocol") == "prpl-steam-mobile") {
		storeUserToken(np->m_accounts[account], STEAM_ACCESS_TOKEN, purple_account_get_string_wrapped(account, "access_token", NULL));
	}
    else if (CONFIG_STRING(config, "service.protocol") == "prpl-eionrobb-discord") {
        storeUserToken(np->m_accounts[account], DISCORD_ACCESS_TOKEN, purple_account_get_string_wrapped(account, "token", NULL));
    }
}

static void printDebug(PurpleDebugLevel level, const char *category, const char *arg_s) {
	std::string c("");
	std::string args(arg_s);
	args.erase(args.size() - 1);

	if (category) {
		c.append(category);
	}

	c.push_back(':');

	LOG4CXX_INFO(logger_libpurple, c << args);
}

/*
 * Ops....
 */
static PurpleDebugUiOps debugUiOps =
{
	printDebug,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void buddyTyping(PurpleAccount *account, const char *who, gpointer null) {
	std::string w = purple_normalize_wrapped(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyping(np->m_accounts[account], w);
}

static void buddyTyped(PurpleAccount *account, const char *who, gpointer null) {
	std::string w = purple_normalize_wrapped(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyped(np->m_accounts[account], w);
}

static void buddyTypingStopped(PurpleAccount *account, const char *who, gpointer null){
	std::string w = purple_normalize_wrapped(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyStoppedTyping(np->m_accounts[account], w);
}

static void gotAttention(PurpleAccount *account, const char *who, PurpleConversation *conv, guint type) {
	std::string w = purple_normalize_wrapped(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleAttention(np->m_accounts[account], w, "");
}

static bool initPurple() {
	bool ret;

	std::string libPurpleDllPath = CONFIG_STRING_DEFAULTED(config, "purple.libpurple_dll_path", "");

	if (!resolvePurpleFunctions()) {
		LOG4CXX_ERROR(logger, "Unable to load libpurple.dll or some of the needed methods");
		return false;
	}

	std::string pluginsDir = CONFIG_STRING_DEFAULTED(config, "purple.plugins_dir", "./plugins");
	LOG4CXX_INFO(logger, "Setting libpurple plugins directory to: " << pluginsDir);
	purple_plugins_add_search_path_wrapped(pluginsDir.c_str());

	std::string cacertsDir = CONFIG_STRING_DEFAULTED(config, "purple.cacerts_dir", "./ca-certs");
	LOG4CXX_INFO(logger, "Setting libpurple cacerts directory to: " << cacertsDir);
	purple_certificate_add_ca_search_path_wrapped(cacertsDir.c_str());

	std::string userDir = CONFIG_STRING_DEFAULTED(config, "service.working_dir", "./");
	LOG4CXX_INFO(logger, "Setting libpurple user directory to: " << userDir);

	purple_util_set_user_dir_wrapped(userDir.c_str());
	remove("./accounts.xml");
	remove("./blist.xml");

	purple_debug_set_ui_ops_wrapped(&debugUiOps);
// 	purple_debug_set_verbose_wrapped(true);

	purple_core_set_ui_ops_wrapped(&coreUiOps);
	if (CONFIG_STRING_DEFAULTED(config, "service.eventloop", "") == "libev") {
		LOG4CXX_INFO(logger, "Will use libev based event loop");
	}
	else {
		LOG4CXX_INFO(logger, "Will use glib based event loop");
	}
	purple_eventloop_set_ui_ops_wrapped(getEventLoopUiOps(CONFIG_STRING_DEFAULTED(config, "service.eventloop", "") == "libev"));

	ret = purple_core_init_wrapped("spectrum");
	if (ret) {
		static int blist_handle;
		static int conversation_handle;

		purple_set_blist_wrapped(purple_blist_new_wrapped());
		purple_blist_load_wrapped();

		purple_prefs_load_wrapped();

		/* Good default preferences */
		/* The combination of these two settings mean that libpurple will never
		 * (of its own accord) set all the user accounts idle.
		 */
		purple_prefs_set_bool_wrapped("/purple/away/away_when_idle", false);
		/*
		 * This must be set to something not "none" for idle reporting to work
		 * for, e.g., the OSCAR prpl. We don't implement the UI ops, so this is
		 * okay for now.
		 */
		purple_prefs_set_string_wrapped("/purple/away/idle_reporting", "system");

		/* Disable all logging */
		purple_prefs_set_bool_wrapped("/purple/logging/log_ims", false);
		purple_prefs_set_bool_wrapped("/purple/logging/log_chats", false);
		purple_prefs_set_bool_wrapped("/purple/logging/log_system", false);

        purple_plugins_load_saved_wrapped("/spectrum/plugins/loaded");

// 		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "buddy-typed", &conversation_handle, PURPLE_CALLBACK(buddyTyped), NULL);
		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "buddy-privacy-changed", &conversation_handle, PURPLE_CALLBACK(buddyPrivacyChanged), NULL);
		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "got-attention", &conversation_handle, PURPLE_CALLBACK(gotAttention), NULL);
		purple_signal_connect_wrapped(purple_connections_get_handle_wrapped(), "signed-on", &blist_handle,PURPLE_CALLBACK(signed_on), NULL);
// 		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
// 		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
// 		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
// 		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
		purple_signal_connect_wrapped(purple_blist_get_handle_wrapped(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
		static int xfer_handle;
		purple_signal_connect_wrapped(purple_xfers_get_handle_wrapped(), "file-send-start", &xfer_handle, PURPLE_CALLBACK(fileSendStart), NULL);
		purple_signal_connect_wrapped(purple_xfers_get_handle_wrapped(), "file-recv-start", &xfer_handle, PURPLE_CALLBACK(fileRecvStart), NULL);
		purple_signal_connect_wrapped(purple_xfers_get_handle_wrapped(), "file-recv-request", &xfer_handle, PURPLE_CALLBACK(newXfer), NULL);
		purple_signal_connect_wrapped(purple_xfers_get_handle_wrapped(), "file-recv-complete", &xfer_handle, PURPLE_CALLBACK(XferReceiveComplete), NULL);
		purple_signal_connect_wrapped(purple_xfers_get_handle_wrapped(), "file-send-complete", &xfer_handle, PURPLE_CALLBACK(XferSendComplete), NULL);
//
// 		purple_commands_init();

	}
	return ret;
}


static void transportDataReceived(gpointer data, gint source, PurpleInputCondition cond) {
	if (cond & PURPLE_INPUT_READ) {
		char buffer[65535];
		char *ptr = buffer;
#ifdef WIN32
		ssize_t n = recv(source, ptr, sizeof(buffer), 0);
#else
		ssize_t n = read(source, ptr, sizeof(buffer));
#endif
		if (n <= 0) {
			if (errno == EAGAIN) {
				return;
			}
			LOG4CXX_INFO(logger, "Diconnecting from spectrum2 server");
			exit(errno);
		}
		std::string d = std::string(buffer, n);

		if (firstPing) {
			firstPing = false;
			NetworkPlugin::PluginConfig cfg;
			cfg.setSupportMUC(true);
			if (CONFIG_STRING(config, "service.protocol") == "prpl-telegram") {
				cfg.setNeedPassword(false);
			}
			if (CONFIG_STRING(config, "service.protocol") == "prpl-hangouts") {
				cfg.setNeedPassword(false);
			}
			if (CONFIG_BOOL(config, "service.server_mode") || CONFIG_STRING(config, "service.protocol") == "prpl-irc") {
				cfg.setNeedRegistration(false);
			}
			else {
				PurplePlugin *plugin = purple_find_prpl_wrapped(CONFIG_STRING(config, "service.protocol").c_str());
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
				bool noPassword = prpl_info->options & OPT_PROTO_NO_PASSWORD;
				LOG4CXX_INFO(logger, "passwordless backend: " << noPassword);
				cfg.setNeedPassword(!needPassword);
				cfg.setNeedRegistration(true);
			}
			np->sendConfig(cfg);
		}

		np->handleDataRead(d);
	}
	else {
		if (writeInput != 0) {
			purple_input_remove_wrapped(writeInput);
			writeInput = 0;
		}
		np->readyForData();
	}
}

int main(int argc, char **argv) {
	boost::locale::generator gen;
	std::locale::global(gen(""));
#ifndef WIN32
#if !defined(__FreeBSD__) && !defined(__APPLE__)
		mallopt(M_CHECK_ACTION, 2);
		mallopt(M_PERTURB, 0xb);
#endif

		signal(SIGPIPE, SIG_IGN);

		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
			std::cout << "SIGCHLD handler can't be set\n";
			return -1;
		}
#endif

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	config = SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Config>(cfg);

	Logging::initBackendLogging(config.get());
	if (CONFIG_STRING(config, "service.protocol") == "prpl-hangouts" || CONFIG_STRING(config, "service.protocol") == "prpl-steam-mobile" || CONFIG_STRING(config, "service.protocol") == "prpl-eionrobb-discord") {
		storagebackend = StorageBackend::createBackend(config.get(), error);
		if (storagebackend == NULL) {
			LOG4CXX_ERROR(logger, "Error creating StorageBackend! " << error);
			LOG4CXX_ERROR(logger, "Selected libpurple protocol need storage backend configured to work! " << error);
			return NetworkPlugin::StorageBackendNeeded;
		}
		else if (!storagebackend->connect()) {
			LOG4CXX_ERROR(logger, "Can't connect to database!");
			return -1;
		}
	}

	initPurple();

	main_socket = create_socket(host.c_str(), port);
	purple_input_add_wrapped(main_socket, PURPLE_INPUT_READ, &transportDataReceived, NULL);
	purple_timeout_add_seconds_wrapped(30, pingTimeout, NULL);

	np = new SpectrumNetworkPlugin();

	GMainLoop *m_loop;
#ifdef WITH_LIBEVENT
	bool libev = CONFIG_STRING_DEFAULTED(config, "service.eventloop", "") == "libev";
	if (!libev) {
		m_loop = g_main_loop_new(NULL, FALSE);
	}
	else {
		event_init();
	}
#endif
	m_loop = g_main_loop_new(NULL, FALSE);
	if (m_loop) {
		g_main_loop_run(m_loop);
	}
#ifdef WITH_LIBEVENT
	else {
		event_loop(0);
	}
#endif

	return 0;
}
