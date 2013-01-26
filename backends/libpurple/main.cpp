#include "utils.h"

#include "glib.h"

// win32/libc_interface.h defines its own socket(), read() and so on.
// We don't want to use it here.
#define _LIBC_INTERFACE_H_ 1

#include "purple.h"
#include <algorithm>
#include <iostream>

#include "transport/networkplugin.h"
#include "transport/logging.h"
#include "transport/config.h"
#include "transport/logging.h"
#include "geventloop.h"

// #include "valgrind/memcheck.h"
#ifndef __FreeBSD__
#include "malloc.h"
#endif
#include <algorithm>
#include "errno.h"
#include <boost/make_shared.hpp>

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
    while(std::getline(ss, item, delim)) {
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

boost::shared_ptr<Config> config;
SpectrumNetworkPlugin *np;

static std::string host;
static int port = 10000;

struct FTData {
	unsigned long id;
	unsigned long timer;
	bool paused;
};

static void *notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info);

static gboolean ft_ui_ready(void *data) {
	PurpleXfer *xfer = (PurpleXfer *) data;
	FTData *ftdata = (FTData *) xfer->ui_data;
	ftdata->timer = 0;
	purple_xfer_ui_ready_wrapped((PurpleXfer *) data);
	return FALSE;
}

struct authRequest {
	PurpleAccountRequestAuthorizationCb authorize_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *user_data;
	std::string who;
	PurpleAccount *account;
	std::string mainJID;	// JID of user connected with this request
};

static void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value, gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
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

static void *requestAction(const char *title, const char *primary, const char *secondary, int default_action, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data, size_t action_count, va_list actions){
	std::string t(title ? title : "NULL");
	if (t == "SSL Certificate Verification") {
		LOG4CXX_INFO(logger,  "accepting SSL certificate");
		va_arg(actions, char *);
		((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
	}
	else {
		if (title) {
			std::string headerString(title);
			LOG4CXX_INFO(logger,  "header string: " << headerString);
			if (headerString == "SSL Certificate Verification") {
				va_arg(actions, char *);
				((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
			}
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

class SpectrumNetworkPlugin : public NetworkPlugin {
	public:
		SpectrumNetworkPlugin() : NetworkPlugin() {

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
							purple_account_set_bool_wrapped(account, strippedKey.c_str(), fromString<bool>(keyItem.second.as<std::string>()));
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

		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			PurpleAccount *account = NULL;

			std::string name;
			std::string protocol;
			getProtocolAndName(legacyName, name, protocol);

			if (password.empty()) {
				LOG4CXX_INFO(logger,  name.c_str() << ": Empty password");
				np->handleDisconnected(user, 0, "Empty password.");
				return;
			}

			if (!purple_find_prpl_wrapped(protocol.c_str())) {
				LOG4CXX_INFO(logger,  name.c_str() << ": Invalid protocol '" << protocol << "'");
				np->handleDisconnected(user, 0, "Invalid protocol " + protocol);
				return;
			}

			if (purple_accounts_find_wrapped(name.c_str(), protocol.c_str()) != NULL) {
				account = purple_accounts_find_wrapped(name.c_str(), protocol.c_str());
				if (m_accounts.find(account) != m_accounts.end() && m_accounts[account] != user) {
					LOG4CXX_INFO(logger, "Account '" << name << "' is already used by '" << m_accounts[account] << "'");
					np->handleDisconnected(user, 0, "Account '" + name + "' is already used by '" + m_accounts[account] + "'");
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
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
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
#ifndef __FreeBSD__
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
				PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, legacyName.c_str(), account);
				if (!conv) {
					conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_IM, legacyName.c_str(), account);
					if (!conv) {
						conv = purple_conversation_new_wrapped(PURPLE_CONV_TYPE_IM, account, legacyName.c_str());
					}
				}
				if (xhtml.empty()) {
					gchar *_markup = purple_markup_escape_text_wrapped(message.c_str(), -1);
					if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
						purple_conv_im_send_wrapped(PURPLE_CONV_IM_WRAPPED(conv), _markup);
					}
					else if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_CHAT) {
						purple_conv_chat_send_wrapped(PURPLE_CONV_CHAT_WRAPPED(conv), _markup);
					}
					g_free(_markup);
				}
				else {
					if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
						purple_conv_im_send_wrapped(PURPLE_CONV_IM_WRAPPED(conv), xhtml.c_str());
					}
					else if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_CHAT) {
						purple_conv_chat_send_wrapped(PURPLE_CONV_CHAT_WRAPPED(conv), xhtml.c_str());
					}
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

				if (CONFIG_BOOL(config, "backend.no_vcard_fetch") && name != purple_account_get_username_wrapped(account)) {
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
			if (account) {
				if (m_authRequests.find(user + buddyName) != m_authRequests.end()) {
					m_authRequests[user + buddyName]->deny_cb(m_authRequests[user + buddyName]->user_data);
					m_authRequests.erase(user + buddyName);
				}
				PurpleBuddy *buddy = purple_find_buddy_wrapped(account, buddyName.c_str());
				if (buddy) {
					purple_account_remove_buddy_wrapped(account, buddy, purple_buddy_get_group_wrapped(buddy));
					purple_blist_remove_buddy_wrapped(buddy);
				}
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups_) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				std::string groups = groups_.empty() ? "" : groups_[0];

				if (m_authRequests.find(user + buddyName) != m_authRequests.end()) {
					m_authRequests[user + buddyName]->authorize_cb(m_authRequests[user + buddyName]->user_data);
					m_authRequests.erase(user + buddyName);
				}

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

		void handleTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				LOG4CXX_INFO(logger, user << ": sending typing notify to " << buddyName);
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_TYPING);
			}
		}

		void handleTypedRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_TYPED);
			}
		}

		void handleStoppedTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), PURPLE_NOT_TYPING);
			}
		}

		void handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				purple_prpl_send_attention_wrapped(purple_account_get_connection_wrapped(account), buddyName.c_str(), 0);
			}
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &pasword) {
			PurpleAccount *account = m_sessions[user];
			if (!account) {
				return;
			}

			PurpleConnection *gc = purple_account_get_connection_wrapped(account);
			GHashTable *comps = NULL;

			// Check if the PurpleChat is not stored in buddy list
			PurpleChat *chat = purple_blist_find_chat_wrapped(account, room.c_str());
			if (chat) {
				comps = purple_chat_get_components_wrapped(chat);
			}
			else if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
				comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, room.c_str());
			}

			LOG4CXX_INFO(logger, user << ": Joining the room " << room);
			if (comps) {
				serv_join_chat_wrapped(gc, comps);
				g_hash_table_destroy(comps);
			}
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
			PurpleAccount *account = m_sessions[user];
			if (!account) {
				return;
			}

			PurpleConversation *conv = purple_find_conversation_with_account_wrapped(PURPLE_CONV_TYPE_CHAT, room.c_str(), account);
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
		std::map<std::string, authRequest *> m_authRequests;
		std::map<unsigned long, PurpleXfer *> m_xfers;
		std::map<std::string, PurpleXfer *> m_unhandledXfers;
		std::vector<PurpleXfer *> m_waitingXfers;
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
		while(buddies) {
			PurpleGroup *g = purple_buddy_get_group_wrapped((PurpleBuddy *) buddies->data);
			buddies = g_slist_delete_link(buddies, buddies);

			if(g && purple_group_get_name_wrapped(g)) {
				groups.push_back(purple_group_get_name_wrapped(g));
			}
		}
	}

	if (groups.empty()) {
		groups.push_back("Buddies");
	}

	return groups;
}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account_wrapped(buddy);

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

static PurpleBlistUiOps blistUiOps =
{
	NULL,
	buddyListNewNode,
	NULL,
	buddyListUpdate,
	NULL, //NodeRemoved,
	NULL,
	NULL,
	NULL, // buddyListAddBuddy,
	NULL,
	NULL,
	NULL, //buddyListSaveNode,
	NULL, //buddyListRemoveNode,
	NULL, //buddyListSaveAccount,
	NULL
};

static void conv_write_im(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	// Don't forwards our own messages.
	if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM && (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)) {
		return;
	}
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

// 	LOG4CXX_INFO(logger, "Received message body='" << message_ << "' xhtml='" << xhtml_ << "'");

	if (purple_conversation_get_type_wrapped(conv) == PURPLE_CONV_TYPE_IM) {
		std::string w = purple_normalize_wrapped(account, who);
		size_t pos = w.find("/");
		if (pos != std::string::npos)
			w.erase((int) pos, w.length() - (int) pos);
		np->handleMessage(np->m_accounts[account], w, message_, "", xhtml_, timestamp);
	}
	else {
		LOG4CXX_INFO(logger, "Received message body='" << message_ << "' name='" << purple_conversation_get_name_wrapped(conv) << "' " << who);
		np->handleMessage(np->m_accounts[account], purple_conversation_get_name_wrapped(conv), message_, who, xhtml_, timestamp);
	}
}

static void conv_chat_add_users(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	GList *l = cbuddies;
	while (l != NULL) {
		PurpleConvChatBuddy *cb = (PurpleConvChatBuddy *)l->data;
		std::string name(cb->name);
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

		np->handleParticipantChanged(np->m_accounts[account], name, purple_conversation_get_name_wrapped(conv), (int) flags, pbnetwork::STATUS_ONLINE);

		l = l->next;
	}
}

static void conv_chat_remove_users(PurpleConversation *conv, GList *users) {
	PurpleAccount *account = purple_conversation_get_account_wrapped(conv);

	GList *l = users;
	while (l != NULL) {
		std::string name((char *) l->data);
		np->handleParticipantChanged(np->m_accounts[account], name, purple_conversation_get_name_wrapped(conv), 0, pbnetwork::STATUS_NONE);

		l = l->next;
	}
}

static PurpleConversationUiOps conversation_ui_ops =
{
	NULL,
	NULL,
	conv_write_im,//conv_write_chat,                              /* write_chat           */
	conv_write_im,             /* write_im             */
	NULL,//conv_write_conv,           /* write_conv           */
	conv_chat_add_users,       /* chat_add_users       */
	NULL,//conv_chat_rename_user,     /* chat_rename_user     */
	conv_chat_remove_users,    /* chat_remove_users    */
	NULL,//pidgin_conv_chat_update_user,     /* chat_update_user     */
	NULL,//pidgin_conv_present_conversation, /* present              */
	NULL,//pidgin_conv_has_focus,            /* has_focus            */
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

static gboolean disconnectMe(void *data) {
	Dis *d = (Dis *) data;
	PurpleAccount *account = purple_accounts_find_wrapped(d->name.c_str(), d->protocol.c_str());
	delete d;

	if (account) {
		np->handleLogoutRequest(np->m_accounts[account], purple_account_get_username_wrapped(account));
	}
	return FALSE;
}

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
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

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
			label = purple_notify_user_info_entry_get_label_wrapped(vcardEntry);
			if (label == "Given Name" || label == "First Name") {
				firstName = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label == "Family Name" || label == "Last Name") {
				lastName = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label=="Nickname" || label == "Nick") {
				nickname = purple_notify_user_info_entry_get_value_wrapped(vcardEntry);
			}
			else if (label=="Full Name") {
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

// 	PurpleAccount *account = purple_xfer_get_account_wrapped(xfer);
// 	np->handleFTStart(np->m_accounts[account], xfer->who, xfer, "", xhtml_);
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

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops_wrapped(&blistUiOps);
	purple_accounts_set_ui_ops_wrapped(&accountUiOps);
	purple_notify_set_ui_ops_wrapped(&notifyUiOps);
	purple_request_set_ui_ops_wrapped(&requestUiOps);
	purple_xfers_set_ui_ops_wrapped(&xferUiOps);
	purple_connections_set_ui_ops_wrapped(&conn_ui_ops);
	purple_conversations_set_ui_ops_wrapped(&conversation_ui_ops);
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
#ifndef __FreeBSD__
	// force returning of memory chunks allocated by libxml2 to kernel
	malloc_trim(0);
#endif
#endif

	// For prpl-gg
	execute_purple_plugin_action(gc, "Download buddylist from Server");
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

	if (!resolvePurpleFunctions(libPurpleDllPath)) {
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
// 		purple_signal_connect_wrapped(purple_conversations_get_handle_wrapped(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
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
#ifndef WIN32
#ifndef __FreeBSD__
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

	config = boost::shared_ptr<Config>(cfg);
 
	Logging::initBackendLogging(config.get());
	initPurple();
 
	main_socket = create_socket(host.c_str(), port);
	purple_input_add_wrapped(main_socket, PURPLE_INPUT_READ, &transportDataReceived, NULL);
	purple_timeout_add_seconds_wrapped(30, pingTimeout, NULL);
 
	np = new SpectrumNetworkPlugin();
	bool libev = CONFIG_STRING_DEFAULTED(config, "service.eventloop", "") == "libev";

	GMainLoop *m_loop;
#ifdef WITH_LIBEVENT
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
