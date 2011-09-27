#include "glib.h"
#include "purple.h"
#include <iostream>

#include "transport/config.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "transport/user.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/conversation.h"
#include "transport/networkplugin.h"
#include "spectrumeventloop.h"
#include "geventloop.h"
#include "log4cxx/logger.h"
#include "log4cxx/consoleappender.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/properties.h"
#include "log4cxx/helpers/fileinputstream.h"
#include "sys/wait.h"
#include "sys/signal.h"
// #include "valgrind/memcheck.h"
#include "malloc.h"


using namespace log4cxx;

static LoggerPtr logger_libpurple = Logger::getLogger("backend.libpurple");
static LoggerPtr logger = Logger::getLogger("backend");

using namespace Transport;

class SpectrumNetworkPlugin;


SpectrumNetworkPlugin *np;

static gboolean nodaemon = FALSE;
static gchar *logfile = NULL;
static gchar *lock_file = NULL;
static gchar *host = NULL;
static int port = 10000;
static gboolean ver = FALSE;
static gboolean list_purple_settings = FALSE;

static GOptionEntry options_entries[] = {
	{ "nodaemon", 'n', 0, G_OPTION_ARG_NONE, &nodaemon, "Disable background daemon mode", NULL },
	{ "logfile", 'l', 0, G_OPTION_ARG_STRING, &logfile, "Set file to log", NULL },
	{ "pidfile", 'p', 0, G_OPTION_ARG_STRING, &lock_file, "File where to write transport PID", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Shows Spectrum version", NULL },
	{ "list-purple-settings", 's', 0, G_OPTION_ARG_NONE, &list_purple_settings, "Lists purple settings which can be used in config file", NULL },
	{ "host", 'h', 0, G_OPTION_ARG_STRING, &host, "Host to connect to", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to connect to", NULL },
	{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, "", NULL }
};

static void *notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info);
static GHashTable *ui_info = NULL;

static GHashTable *spectrum_ui_get_info(void)
{
	if(NULL == ui_info) {
		ui_info = g_hash_table_new(g_str_hash, g_str_equal);

		g_hash_table_insert(ui_info, g_strdup("name"), g_strdup("Spectrum"));
		g_hash_table_insert(ui_info, g_strdup("version"), g_strdup("0.5"));
		g_hash_table_insert(ui_info, g_strdup("website"), g_strdup("http://spectrum.im"));
		g_hash_table_insert(ui_info, g_strdup("dev_website"), g_strdup("http://spectrum.im"));
		g_hash_table_insert(ui_info, g_strdup("client_type"), g_strdup("pc"));

		/*
		 * This is the client key for "Pidgin."  It is owned by the AIM
		 * account "markdoliner."  Please don't use this key for other
		 * applications.  You can either not specify a client key, in
		 * which case the default "libpurple" key will be used, or you
		 * can register for your own client key at
		 * http://developer.aim.com/manageKeys.jsp
		 */
		g_hash_table_insert(ui_info, g_strdup("prpl-aim-clientkey"), g_strdup("ma1cSASNCKFtrdv9"));
		g_hash_table_insert(ui_info, g_strdup("prpl-icq-clientkey"), g_strdup("ma1cSASNCKFtrdv9"));

		/*
		 * This is the distid for Pidgin, given to us by AOL.  Please
		 * don't use this for other applications.  You can just not
		 * specify a distid and libpurple will use a default.
		 */
		g_hash_table_insert(ui_info, g_strdup("prpl-aim-distid"), GINT_TO_POINTER(1550));
		g_hash_table_insert(ui_info, g_strdup("prpl-icq-distid"), GINT_TO_POINTER(1550));
	}

	return ui_info;
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
	else if (purple_buddy_get_alias(m_buddy)) {
		alias = (std::string) purple_buddy_get_alias(m_buddy);
	}
	else {
		alias = (std::string) purple_buddy_get_server_alias(m_buddy);
	}
	return alias;
}

class SpectrumNetworkPlugin : public NetworkPlugin {
	public:
		SpectrumEventLoop *m_loop;
		SpectrumNetworkPlugin(Config *config, SpectrumEventLoop *loop, const std::string &host, int port) : NetworkPlugin(loop, host, port) {
			this->config = config;
			m_loop = loop;
		}

		void handleExit() {
			m_loop->stop();
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
				purple_buddy_icons_set_account_icon(account, (guchar *) contents, length);
			}
		}

		void setDefaultAccountOptions(PurpleAccount *account) {
			for (std::map<std::string,std::string>::const_iterator it = config->getUnregistered().begin();
				it != config->getUnregistered().end(); it++) {
				if ((*it).first.find("purple.") == 0) {
					std::string key = (*it).first.substr((*it).first.find(".") + 1);

					PurplePlugin *plugin = purple_find_prpl(purple_account_get_protocol_id(account));
					PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
					bool found = false;
					for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
						PurpleAccountOption *option = (PurpleAccountOption *) l->data;
						PurplePrefType type = purple_account_option_get_type(option);
						std::string key2(purple_account_option_get_setting(option));
						std::cout << key << " " << key2 << " " << (*it).second << "\n";
						if (key != key2)
							continue;
						
						found = true;
						switch (type) {
							case PURPLE_PREF_BOOLEAN:
								purple_account_set_bool(account, key.c_str(), boost::lexical_cast<bool>((*it).second));
								break;

							case PURPLE_PREF_INT:
								purple_account_set_int(account, key.c_str(), boost::lexical_cast<int>((*it).second));
								break;

							case PURPLE_PREF_STRING:
							case PURPLE_PREF_STRING_LIST:
								purple_account_set_string(account, key.c_str(), (*it).second.c_str());
								break;
							default:
								continue;
						}
						break;
					}

					if (!found) {
						purple_account_set_string(account, key.c_str(), (*it).second.c_str());
					}
				}
			}
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			PurpleAccount *account = NULL;
			
			std::string name;
			std::string protocol;
			getProtocolAndName(legacyName, name, protocol);

			if (password.empty()) {
				np->handleDisconnected(user, 0, "Empty password.");
				return;
			}

			if (!purple_find_prpl(protocol.c_str())) {
				np->handleDisconnected(user, 0, "Invalid protocol " + protocol);
				return;
			}

			LOG4CXX_INFO(logger,  "Creating account with name '" << name.c_str() << "' and protocol '" << protocol << "'");
			if (purple_accounts_find(name.c_str(), protocol.c_str()) != NULL){
				account = purple_accounts_find(name.c_str(), protocol.c_str());
			}
			else {
				account = purple_account_new(name.c_str(), protocol.c_str());
				purple_accounts_add(account);
			}

			m_sessions[user] = account;
			m_accounts[account] = user;

			// Default avatar
			setDefaultAvatar(account, legacyName);

			purple_account_set_password(account, password.c_str());
			purple_account_set_bool(account, "custom_smileys", FALSE);
			purple_account_set_bool(account, "direct_connect", FALSE);

			setDefaultAccountOptions(account);

			purple_account_set_enabled(account, "spectrum", TRUE);
			if (CONFIG_BOOL(np->config, "service.enable_privacy_lists")) {
				purple_account_set_privacy_type(account, PURPLE_PRIVACY_DENY_USERS);
			}

			const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account, PURPLE_STATUS_AVAILABLE);
			if (status_type != NULL) {
				purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, NULL);
			}
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
// 				VALGRIND_DO_LEAK_CHECK;
				m_sessions.erase(user);
				purple_account_disconnect(account);
				purple_account_set_enabled(account, "spectrum", FALSE);

				g_free(account->ui_data);
				account->ui_data = NULL;
				m_accounts.erase(account);

				purple_accounts_delete(account);
// 
// 				// Remove conversations.
// 				// This has to be called before m_account->ui_data = NULL;, because it uses
// 				// ui_data to call SpectrumMessageHandler::purpleConversationDestroyed() callback.
// 				GList *iter;
// 				for (iter = purple_get_conversations(); iter; ) {
// 					PurpleConversation *conv = (PurpleConversation*) iter->data;
// 					iter = iter->next;
// 					if (purple_conversation_get_account(conv) == account)
// 						purple_conversation_destroy(conv);
// 				}
// 
// 				g_free(account->ui_data);
// 				account->ui_data = NULL;
// 				m_accounts.erase(account);
// 
// 				purple_notify_close_with_handle(account);
// 				purple_request_close_with_handle(account);
// 
// 				purple_accounts_remove(account);
// 
// 				GSList *buddies = purple_find_buddies(account, NULL);
// 				while(buddies) {
// 					PurpleBuddy *b = (PurpleBuddy *) buddies->data;
// 					purple_blist_remove_buddy(b);
// 					buddies = g_slist_delete_link(buddies, buddies);
// 				}
// 
// 				/* Remove any open conversation for this account */
// 				for (GList *it = purple_get_conversations(); it; ) {
// 					PurpleConversation *conv = (PurpleConversation *) it->data;
// 					it = it->next;
// 					if (purple_conversation_get_account(conv) == account)
// 						purple_conversation_destroy(conv);
// 				}
// 
// 				/* Remove this account's pounces */
// 					// purple_pounce_destroy_all_by_account(account);
// 
// 				/* This will cause the deletion of an old buddy icon. */
// 				purple_buddy_icons_set_account_icon(account, NULL, 0);
// 
// 				purple_account_destroy(account);
				// force returning of memory chunks allocated by libxml2 to kernel
				malloc_trim(0);
// 				VALGRIND_DO_LEAK_CHECK;
			}
		}

		void handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				int st;
				switch(status) {
					case Swift::StatusShow::Away: {
						st = PURPLE_STATUS_AWAY;
						if (!purple_account_get_status_type_with_primitive(account, PURPLE_STATUS_AWAY))
							st = PURPLE_STATUS_EXTENDED_AWAY;
						else
							st = PURPLE_STATUS_AWAY;
						break;
					}
					case Swift::StatusShow::DND: {
						st = PURPLE_STATUS_UNAVAILABLE;
						break;
					}
					case Swift::StatusShow::XA: {
						if (!purple_account_get_status_type_with_primitive(account, PURPLE_STATUS_EXTENDED_AWAY))
							st = PURPLE_STATUS_AWAY;
						else
							st = PURPLE_STATUS_EXTENDED_AWAY;
						break;
					}
					case Swift::StatusShow::None: {
						st = PURPLE_STATUS_OFFLINE;
						break;
					}
					case 255:
						st = PURPLE_STATUS_INVISIBLE;
						break;
					default:
						st = PURPLE_STATUS_AVAILABLE;
						break;
				}
				gchar *_markup = purple_markup_escape_text(statusMessage.c_str(), -1);
				std::string markup(_markup);
				g_free(_markup);

				// we are already connected so we have to change status
				const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account, (PurpleStatusPrimitive) st);
				if (status_type != NULL) {
					// send presence to legacy network
					if (!markup.empty()) {
						purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, "message", markup.c_str(), NULL);
					}
					else {
						purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, NULL);
					}
				}
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, legacyName.c_str(), account);
				if (!conv) {
					conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, legacyName.c_str());
				}
				if (xhtml.empty()) {
					gchar *_markup = purple_markup_escape_text(message.c_str(), -1);
					purple_conv_im_send(PURPLE_CONV_IM(conv), _markup);
					g_free(_markup);
				}
				else {
					purple_conv_im_send(PURPLE_CONV_IM(conv), xhtml.c_str());
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

				std::cout << name << " " << purple_account_get_username(account) << "\n";
				if (CONFIG_BOOL(config, "backend.no_vcard_fetch") && name != purple_account_get_username(account)) {
					PurpleNotifyUserInfo *user_info = purple_notify_user_info_new();
					notify_user_info(purple_account_get_connection(account), name.c_str(), user_info);
					purple_notify_user_info_destroy(user_info);
				}
				else {
					serv_get_info(purple_account_get_connection(account), name.c_str());
				}
				
			}
		}

		void handleVCardUpdatedRequest(const std::string &user, const std::string &image) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				gssize size = image.size();
				// this will be freed by libpurple
				guchar *photo = (guchar *) g_malloc(size * sizeof(guchar));
				memcpy(photo, image.c_str(), size);

				if (!photo)
					return;
				purple_buddy_icons_set_account_icon(account, photo, size);
			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::string &groups) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				if (m_authRequests.find(user + buddyName) != m_authRequests.end()) {
					m_authRequests[user + buddyName]->deny_cb(m_authRequests[user + buddyName]->user_data);
					m_authRequests.erase(user + buddyName);
				}
				PurpleBuddy *buddy = purple_find_buddy(account, buddyName.c_str());
				if (buddy) {
					purple_account_remove_buddy(account, buddy, purple_buddy_get_group(buddy));
					purple_blist_remove_buddy(buddy);
				}
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::string &groups) {
			PurpleAccount *account = m_sessions[user];
			if (account) {

				if (m_authRequests.find(user + buddyName) != m_authRequests.end()) {
					m_authRequests[user + buddyName]->authorize_cb(m_authRequests[user + buddyName]->user_data);
					m_authRequests.erase(user + buddyName);
				}

				PurpleBuddy *buddy = purple_find_buddy(account, buddyName.c_str());
				if (buddy) {
					if (getAlias(buddy) != alias) {
						purple_blist_alias_buddy(buddy, alias.c_str());
						purple_blist_server_alias_buddy(buddy, alias.c_str());
						serv_alias_buddy(buddy);
					}

					PurpleGroup *group = purple_find_group(groups.c_str());
					if (!group) {
						group = purple_group_new(groups.c_str());
					}
					purple_blist_add_contact(purple_buddy_get_contact(buddy), group ,NULL);
				}
				else {
					PurpleBuddy *buddy = purple_buddy_new(account, buddyName.c_str(), alias.c_str());

					// Add newly created buddy to legacy network roster.
					PurpleGroup *group = purple_find_group(groups.c_str());
					if (!group) {
						group = purple_group_new(groups.c_str());
					}
					purple_blist_add_buddy(buddy, NULL, group ,NULL);
					purple_account_add_buddy(account, buddy);
				}
			}
		}

		void handleBuddyBlockToggled(const std::string &user, const std::string &buddyName, bool blocked) {
			if (CONFIG_BOOL(np->config, "service.enable_privacy_lists")) {
				PurpleAccount *account = m_sessions[user];
				if (account) {
					if (blocked) {
						purple_privacy_deny(account, buddyName.c_str(), FALSE, FALSE);
					}
					else {
						purple_privacy_allow(account, buddyName.c_str(), FALSE, FALSE);
					}
				}
			}
		}

		void handleTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing(purple_account_get_connection(account), buddyName.c_str(), PURPLE_TYPING);
			}
		}

		void handleTypedRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing(purple_account_get_connection(account), buddyName.c_str(), PURPLE_TYPED);
			}
		}

		void handleStoppedTypingRequest(const std::string &user, const std::string &buddyName) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_send_typing(purple_account_get_connection(account), buddyName.c_str(), PURPLE_NOT_TYPING);
			}
		}

		void handleAttentionRequest(const std::string &user, const std::string &buddyName, const std::string &message) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				purple_prpl_send_attention(purple_account_get_connection(account), buddyName.c_str(), 0);
			}
		}

		std::map<std::string, PurpleAccount *> m_sessions;
		std::map<PurpleAccount *, std::string> m_accounts;
		std::map<std::string, unsigned int> m_vcards;
		std::map<std::string, authRequest *> m_authRequests;
		Config *config;
		
};

static bool getStatus(PurpleBuddy *m_buddy, Swift::StatusShow &status, std::string &statusMessage) {
	PurplePresence *pres = purple_buddy_get_presence(m_buddy);
	if (pres == NULL)
		return false;
	PurpleStatus *stat = purple_presence_get_active_status(pres);
	if (stat == NULL)
		return false;
	int st = purple_status_type_get_primitive(purple_status_get_type(stat));

	switch(st) {
		case PURPLE_STATUS_AVAILABLE: {
			break;
		}
		case PURPLE_STATUS_AWAY: {
			status = Swift::StatusShow::Away;
			break;
		}
		case PURPLE_STATUS_UNAVAILABLE: {
			status = Swift::StatusShow::DND;
			break;
		}
		case PURPLE_STATUS_EXTENDED_AWAY: {
			status = Swift::StatusShow::XA;
			break;
		}
		case PURPLE_STATUS_OFFLINE: {
			status = Swift::StatusShow::None;
			break;
		}
		default:
			break;
	}

	const char *message = purple_status_get_attr_string(stat, "message");

	if (message != NULL) {
		char *stripped = purple_markup_strip_html(message);
		statusMessage = std::string(stripped);
		g_free(stripped);
	}
	else
		statusMessage = "";
	return true;
}

static std::string getIconHash(PurpleBuddy *m_buddy) {
	char *avatarHash = NULL;
	PurpleBuddyIcon *icon = purple_buddy_icons_find(purple_buddy_get_account(m_buddy), purple_buddy_get_name(m_buddy));
	if (icon) {
		avatarHash = purple_buddy_icon_get_full_path(icon);
		purple_buddy_icon_unref(icon);
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
	groups.push_back((purple_buddy_get_group(m_buddy) && purple_group_get_name(purple_buddy_get_group(m_buddy))) ? std::string(purple_group_get_name(purple_buddy_get_group(m_buddy))) : std::string("Buddies"));
	return groups;
}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account(buddy);

	// Status
	Swift::StatusShow status;
	std::string message;
	getStatus(buddy, status, message);

	// Tooltip
	PurplePlugin *prpl = purple_find_prpl(purple_account_get_protocol_id(account));
	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	bool blocked = false;
	if (CONFIG_BOOL(np->config, "service.enable_privacy_lists")) {
		if (prpl_info && prpl_info->tooltip_text) {
			PurpleNotifyUserInfo *user_info = purple_notify_user_info_new();
			prpl_info->tooltip_text(buddy, user_info, true);
			GList *entries = purple_notify_user_info_get_entries(user_info);

			while (entries) {
				PurpleNotifyUserInfoEntry *entry = (PurpleNotifyUserInfoEntry *)(entries->data);
				if (purple_notify_user_info_entry_get_label(entry) && purple_notify_user_info_entry_get_value(entry)) {
					std::string label = purple_notify_user_info_entry_get_label(entry);
					if (label == "Blocked" ) {
						if (std::string(purple_notify_user_info_entry_get_value(entry)) == "Yes") {
							blocked = true;
							break;
						}
					}
				}
				entries = entries->next;
			}
			purple_notify_user_info_destroy(user_info);
		}

		if (!blocked) {
			blocked = purple_privacy_check(account, purple_buddy_get_name(buddy)) == false;
		}
		else {
			bool purpleBlocked = purple_privacy_check(account, purple_buddy_get_name(buddy)) == false;
			if (blocked != purpleBlocked) {
				purple_privacy_deny(account, purple_buddy_get_name(buddy), FALSE, FALSE);
			}
		}
	}

	std::cout << "BLOCKED?" << (purple_privacy_check(account, purple_buddy_get_name(buddy)) == false) << "\n";
	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], status.getType(), message, getIconHash(buddy),
		blocked
	);
}

static void buddyListUpdate(PurpleBuddyList *list, PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	buddyListNewNode(node);
}

static void buddyPrivacyChanged(PurpleBlistNode *node, void *data) {
	std::cout << "PRIVACY CHANGED\n";
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	buddyListUpdate(NULL, node);
}

static void NodeRemoved(PurpleBlistNode *node, void *data) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
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
	if (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);

// 	char *striped = purple_markup_strip_html(message);
// 	std::string msg = striped;
// 	g_free(striped);

	std::string w = purple_normalize(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);

	// Escape HTML characters.
	char *newline = purple_strdup_withhtml(msg);
	char *strip, *xhtml, *xhtml_linkified;
	purple_markup_html_to_xhtml(newline, &xhtml, &strip);
	xhtml_linkified = purple_markup_linkify(xhtml);
	std::string message_(strip);

	std::string xhtml_(xhtml_linkified);
	g_free(newline);
	g_free(xhtml);
	g_free(xhtml_linkified);
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

// 	LOG4CXX_INFO(logger, "Received message body='" << message_ << "' xhtml='" << xhtml_ << "'");

	np->handleMessage(np->m_accounts[account], w, message_, "", xhtml_);
}

static PurpleConversationUiOps conversation_ui_ops =
{
	NULL,
	NULL,
	NULL,//conv_write_chat,                              /* write_chat           */
	conv_write_im,             /* write_im             */
	NULL,//conv_write_conv,           /* write_conv           */
	NULL,//conv_chat_add_users,       /* chat_add_users       */
	NULL,//conv_chat_rename_user,     /* chat_rename_user     */
	NULL,//conv_chat_remove_users,    /* chat_remove_users    */
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
	PurpleAccount *account = purple_accounts_find(d->name.c_str(), d->protocol.c_str());
	delete d;

	if (account) {
		np->handleLogoutRequest(np->m_accounts[account], purple_account_get_username(account));
	}
	return FALSE;
}

static void connection_report_disconnect(PurpleConnection *gc, PurpleConnectionError reason, const char *text){
	PurpleAccount *account = purple_connection_get_account(gc);
	np->handleDisconnected(np->m_accounts[account], (int) reason, text ? text : "");
	Dis *d = new Dis;
	d->name = purple_account_get_username(account);
	d->protocol = purple_account_get_protocol_id(account);
	purple_timeout_add_seconds(10, disconnectMe, d);
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
	PurpleAccount *account = purple_connection_get_account(gc);
	std::string name(purple_normalize(account, who));
	std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);

	size_t pos = name.find("/");
	if (pos != std::string::npos)
		name.erase((int) pos, name.length() - (int) pos);

	
	GList *vcardEntries = purple_notify_user_info_get_entries(user_info);
	PurpleNotifyUserInfoEntry *vcardEntry;
	std::string firstName;
	std::string lastName;
	std::string fullName;
	std::string nickname;
	std::string header;
	std::string label;
	Swift::ByteArray photo;

	while (vcardEntries) {
		vcardEntry = (PurpleNotifyUserInfoEntry *)(vcardEntries->data);
		if (purple_notify_user_info_entry_get_label(vcardEntry) && purple_notify_user_info_entry_get_value(vcardEntry)){
			label = purple_notify_user_info_entry_get_label(vcardEntry);
			if (label == "Given Name" || label == "First Name") {
				firstName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label == "Family Name" || label == "Last Name") {
				lastName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Nickname" || label == "Nick") {
				nickname = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Full Name") {
				fullName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else {
				LOG4CXX_WARN(logger, "Unhandled VCard Label '" << purple_notify_user_info_entry_get_label(vcardEntry) << "' " << purple_notify_user_info_entry_get_value(vcardEntry));
			}
		}
		vcardEntries = vcardEntries->next;
	}

	if ((!firstName.empty() || !lastName.empty()) && fullName.empty())
		fullName = firstName + " " + lastName;

	if (nickname.empty() && !fullName.empty()) {
		nickname = fullName;
	}

	bool ownInfo = name == purple_account_get_username(account);
	std::cout << "RECEIVED " << name << " " << purple_account_get_username(account) << "\n";

	if (ownInfo) {
		const gchar *displayname = purple_connection_get_display_name(gc);
		if (!displayname) {
			displayname = purple_account_get_name_for_display(account);
		}

		if (displayname && nickname.empty()) {
			nickname = displayname;
		}

		// avatar
		PurpleStoredImage *avatar = purple_buddy_icons_find_account_icon(account);
		if (avatar) {
			const gchar * data = (const gchar *) purple_imgstore_get_data(avatar);
			size_t len = purple_imgstore_get_size(avatar);
			if (len < 300000 && data) {
				photo = Swift::createByteArray(data, len);
			}
			purple_imgstore_unref(avatar);
		}
	}

	PurpleBuddy *buddy = purple_find_buddy(purple_connection_get_account(gc), who);
	if (buddy && photo.size() == 0) {
		gsize len;
		PurpleBuddyIcon *icon = NULL;
		icon = purple_buddy_icons_find(purple_connection_get_account(gc), name.c_str());
		if (icon) {
			if (true) {
				gchar *data;
				gchar *path = purple_buddy_icon_get_full_path(icon);
				if (g_file_get_contents (path, &data, &len, NULL)) {
					photo = Swift::createByteArray(data, len);
					free(data);
				}
				free(path);
			}
			else {
				const gchar * data = (gchar*)purple_buddy_icon_get_data(icon, &len);
				if (len < 300000 && data) {
					photo = Swift::createByteArray(data, len);
				}
			}
			purple_buddy_icon_unref(icon);
		}
	}

	np->handleVCard(np->m_accounts[account], np->m_vcards[np->m_accounts[account] + name], name, fullName, nickname, Swift::byteArrayToString(photo));
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

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
	purple_accounts_set_ui_ops(&accountUiOps);
	purple_notify_set_ui_ops(&notifyUiOps);
	purple_request_set_ui_ops(&requestUiOps);
// 	purple_xfers_set_ui_ops(getXferUiOps());
	purple_connections_set_ui_ops(&conn_ui_ops);
	purple_conversations_set_ui_ops(&conversation_ui_ops);
// #ifndef WIN32
// 	purple_dnsquery_set_ui_ops(getDNSUiOps());
// #endif
}

static PurpleCoreUiOps coreUiOps =
{
	NULL,
// 	debug_init,
	NULL,
	transport_core_ui_init,
	NULL,
	spectrum_ui_get_info,
	NULL,
	NULL,
	NULL
};

static void signed_on(PurpleConnection *gc, gpointer unused) {
	PurpleAccount *account = purple_connection_get_account(gc);
	np->handleConnected(np->m_accounts[account]);
	// force returning of memory chunks allocated by libxml2 to kernel
	malloc_trim(0);
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
	std::string w = purple_normalize(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyping(np->m_accounts[account], w);
}

static void buddyTyped(PurpleAccount *account, const char *who, gpointer null) {
	std::string w = purple_normalize(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyped(np->m_accounts[account], w);
}

static void buddyTypingStopped(PurpleAccount *account, const char *who, gpointer null){
	std::string w = purple_normalize(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyStoppedTyping(np->m_accounts[account], w);
}

static void gotAttention(PurpleAccount *account, const char *who, PurpleConversation *conv, guint type) {
	std::string w = purple_normalize(account, who);
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleAttention(np->m_accounts[account], w, "");
}

static bool initPurple(Config &cfg) {
	bool ret;

	purple_util_set_user_dir("./");
	remove("./accounts.xml");
	remove("./blist.xml");

// 	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_ui_ops(&debugUiOps);
		purple_debug_set_verbose(true);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	ret = purple_core_init("spectrum");
	if (ret) {
		static int blist_handle;
		static int conversation_handle;

		purple_set_blist(purple_blist_new());
		purple_blist_load();

		purple_prefs_load();

		/* Good default preferences */
		/* The combination of these two settings mean that libpurple will never
		 * (of its own accord) set all the user accounts idle.
		 */
		purple_prefs_set_bool("/purple/away/away_when_idle", false);
		/*
		 * This must be set to something not "none" for idle reporting to work
		 * for, e.g., the OSCAR prpl. We don't implement the UI ops, so this is
		 * okay for now.
		 */
		purple_prefs_set_string("/purple/away/idle_reporting", "system");

		/* Disable all logging */
		purple_prefs_set_bool("/purple/logging/log_ims", false);
		purple_prefs_set_bool("/purple/logging/log_chats", false);
		purple_prefs_set_bool("/purple/logging/log_system", false);


// 		purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typed", &conversation_handle, PURPLE_CALLBACK(buddyTyped), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-privacy-changed", &conversation_handle, PURPLE_CALLBACK(buddyPrivacyChanged), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "got-attention", &conversation_handle, PURPLE_CALLBACK(gotAttention), NULL);
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &blist_handle,PURPLE_CALLBACK(signed_on), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
// 
// 		purple_commands_init();

	}
	return ret;
}

static void spectrum_sigchld_handler(int sig)
{
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);

	if ((pid == (pid_t) - 1) && (errno != ECHILD)) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
}

int main(int argc, char **argv) {
	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new("config_file_name or profile name");
	g_option_context_add_main_entries(context, options_entries, "");
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		std::cout << "option parsing failed: " << error->message << "\n";
		return -1;
	}

	if (ver) {
// 		std::cout << VERSION << "\n";
		std::cout << "verze\n";
		g_option_context_free(context);
		return 0;
	}

	if (argc != 2) {
#ifdef WIN32
		std::cout << "Usage: spectrum.exe <configuration_file.cfg>\n";
#else

#if GLIB_CHECK_VERSION(2,14,0)
	std::cout << g_option_context_get_help(context, FALSE, NULL);
#else
	std::cout << "Usage: spectrum <configuration_file.cfg>\n";
	std::cout << "See \"man spectrum\" for more info.\n";
#endif
		
#endif
	}
	else {
#ifndef WIN32
		signal(SIGPIPE, SIG_IGN);

		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
			std::cout << "SIGCHLD handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}
// 
// 		if (signal(SIGINT, spectrum_sigint_handler) == SIG_ERR) {
// 			std::cout << "SIGINT handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
// 		}
// 
// 		if (signal(SIGTERM, spectrum_sigterm_handler) == SIG_ERR) {
// 			std::cout << "SIGTERM handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
// 		}
// 
// 		struct sigaction sa;
// 		memset(&sa, 0, sizeof(sa)); 
// 		sa.sa_handler = spectrum_sighup_handler;
// 		if (sigaction(SIGHUP, &sa, NULL)) {
// 			std::cout << "SIGHUP handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
//		}
#endif
		Config config;
		if (!config.load(argv[1])) {
			std::cout << "Can't open " << argv[1] << " configuration file.\n";
			return 1;
		}

		if (CONFIG_STRING(&config, "logging.backend_config").empty()) {
			LoggerPtr root = log4cxx::Logger::getRootLogger();
			root->addAppender(new ConsoleAppender(new PatternLayout("%d %-5p %c: %m%n")));
		}
		else {
			log4cxx::helpers::Properties p;
			log4cxx::helpers::FileInputStream *istream = new log4cxx::helpers::FileInputStream(CONFIG_STRING(&config, "logging.backend_config"));

			p.load(istream);
			p.setProperty("pid", boost::lexical_cast<std::string>(getpid()));
			log4cxx::PropertyConfigurator::configure(p);
		}

		initPurple(config);

		SpectrumEventLoop eventLoop;
		np = new SpectrumNetworkPlugin(&config, &eventLoop, host, port);
		eventLoop.run();
	}

	g_option_context_free(context);
}
