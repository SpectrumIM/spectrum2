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

#define Log(X, STRING) std::cout << "[SPECTRUM] " << X << " " << STRING << "\n";


using namespace Transport;

class SpectrumNetworkPlugin;

Logger *_logger;
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

static void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value, gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	std::cout << "REQUEST INPUT\n";
	if (primary) {
		std::string primaryString(primary);
		std::cout << "REQUEST INPUT " << primaryString << "\n";
		if (primaryString == "Authorization Request Message:") {
			std::cout << "AUTHORIZING\n";
			((PurpleRequestInputCb) ok_cb)(user_data, "Please authorize me.");
			return NULL;
		}
	}
	return NULL;
}

static void *requestAction(const char *title, const char *primary, const char *secondary, int default_action, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data, size_t action_count, va_list actions){
	std::string t(title ? title : "NULL");
	if (t == "SSL Certificate Verification") {
		Log("purple", "accepting SSL certificate");
		va_arg(actions, char *);
		((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
	}
	else {
		if (title) {
			std::string headerString(title);
			Log("purple", "header string: " << headerString);
			if (headerString == "SSL Certificate Verification") {
				va_arg(actions, char *);
				((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
			}
		}
	}
	return NULL;
}

class SpectrumNetworkPlugin : public NetworkPlugin {
	public:
		SpectrumNetworkPlugin(Config *config, SpectrumEventLoop *loop, const std::string &host, int port) : NetworkPlugin(loop, host, port) {
			this->config = config;
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			PurpleAccount *account = NULL;
			const char *protocol = CONFIG_STRING(config, "service.protocol").c_str();
			if (purple_accounts_find(legacyName.c_str(), protocol) != NULL){
				Log(user, "this account already exists");
				account = purple_accounts_find(legacyName.c_str(), protocol);
// 				User *u = (User *) account->ui_data;
// 				if (u && u != user) {
// 					Log(userInfo.jid, "This account is already connected by another jid " << user->getJID());
// 					return;
// 				}
			}
			else {
				Log(user, "creating new account");
				account = purple_account_new(legacyName.c_str(), protocol);

				purple_accounts_add(account);
			}

			m_sessions[user] = account;
			purple_account_set_password(account, password.c_str());
			purple_account_set_enabled(account, "spectrum", TRUE);
			
			const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account, PURPLE_STATUS_AVAILABLE);
			if (status_type != NULL) {
				purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, NULL);
			}
			m_accounts[account] = user;
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			const char *protocol = CONFIG_STRING(config, "service.protocol").c_str();
			PurpleAccount *account = purple_accounts_find(legacyName.c_str(), protocol);
			if (account) {
				m_sessions[user] = NULL;
				purple_account_set_enabled(account, "spectrum", FALSE);

				// Remove conversations.
				// This has to be called before m_account->ui_data = NULL;, because it uses
				// ui_data to call SpectrumMessageHandler::purpleConversationDestroyed() callback.
				GList *iter;
				for (iter = purple_get_conversations(); iter; ) {
					PurpleConversation *conv = (PurpleConversation*) iter->data;
					iter = iter->next;
					if (purple_conversation_get_account(conv) == account)
						purple_conversation_destroy(conv);
				}

				g_free(account->ui_data);
				account->ui_data = NULL;
				m_accounts.erase(account);
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
					default:
						st = PURPLE_STATUS_AVAILABLE;
						break;
				}
				gchar *_markup = purple_markup_escape_text(statusMessage.c_str(), -1);
				std::string markup(_markup);
				g_free(_markup);

				std::cout << st << " < STATUS\n";

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

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, legacyName.c_str(), account);
				if (!conv) {
					conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, legacyName.c_str());
				}
				gchar *_markup = purple_markup_escape_text(message.c_str(), -1);
				purple_conv_im_send(PURPLE_CONV_IM(conv), _markup);
				g_free(_markup);
			}
		}

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
				serv_get_info(purple_account_get_connection(account), legacyName.c_str());
				m_vcards[user + legacyName] = id;
			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::string &groups) {
			PurpleAccount *account = m_sessions[user];
			if (account) {
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
				PurpleBuddy *buddy = purple_find_buddy(account, buddyName.c_str());
				if (buddy) {
					purple_blist_alias_buddy(buddy, alias.c_str());
					purple_blist_server_alias_buddy(buddy, alias.c_str());
					serv_alias_buddy(buddy);
				}
				else {
					std::cout << "ADDING NEW BUDDY\n";
					PurpleBuddy *buddy = purple_buddy_new(account, buddyName.c_str(), alias.c_str());

					// Add newly created buddy to legacy network roster.
					PurpleGroup *group = purple_find_group(groups.c_str());
					purple_blist_add_buddy(buddy, NULL, group ,NULL);
					purple_account_add_buddy(account, buddy);
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

		std::map<std::string, PurpleAccount *> m_sessions;
		std::map<PurpleAccount *, std::string> m_accounts;
		std::map<std::string, unsigned int> m_vcards;
	private:
		Config *config;
};

static std::string getAlias(PurpleBuddy *m_buddy) {
	std::string alias;
	if (purple_buddy_get_server_alias(m_buddy))
		alias = (std::string) purple_buddy_get_server_alias(m_buddy);
	else
		alias = (std::string) purple_buddy_get_alias(m_buddy);
	return alias;
}

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
	groups.push_back(purple_group_get_name(purple_buddy_get_group(m_buddy)) ? std::string(purple_group_get_name(purple_buddy_get_group(m_buddy))) : std::string("Buddies"));
	return groups;
}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account(buddy);

	Swift::StatusShow status;
	std::string message;
	getStatus(buddy, status, message);

	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], (int) status.getType(), message, getIconHash(buddy));
}

// static void buddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status_, PurpleStatus *old_status) {
// 	PurpleAccount *account = purple_buddy_get_account(buddy);
// 
// 	Swift::StatusShow status;
// 	std::string message;
// 	getStatus(buddy, status, message);
// 
// 	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], (int) status.getType(), message, getIconHash(buddy));
// }
// 
// static void buddySignedOn(PurpleBuddy *buddy) {
// 	PurpleAccount *account = purple_buddy_get_account(buddy);
// 
// 	Swift::StatusShow status;
// 	std::string message;
// 	getStatus(buddy, status, message);
// 
// 	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], (int) status.getType(), message, getIconHash(buddy));
// }
// 
// static void buddySignedOff(PurpleBuddy *buddy) {
// 	PurpleAccount *account = purple_buddy_get_account(buddy);
// 
// 	Swift::StatusShow status;
// 	std::string message;
// 	getStatus(buddy, status, message);
// 
// 	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], (int) status.getType(), message, getIconHash(buddy));
// }

static void buddyListUpdate(PurpleBuddyList *list, PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	Swift::StatusShow status;
	std::string message;
	getStatus(buddy, status, message);

	np->handleBuddyChanged(np->m_accounts[account], purple_buddy_get_name(buddy), getAlias(buddy), getGroups(buddy)[0], (int) status.getType(), message, getIconHash(buddy));
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

static void conv_write_im(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	// Don't forwards our own messages.
	if (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);

	char *striped = purple_markup_strip_html(message);
	std::string msg = striped;
	g_free(striped);

	std::string w = who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);

	np->handleMessage(np->m_accounts[account], w, msg);
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

static void connection_report_disconnect(PurpleConnection *gc, PurpleConnectionError reason, const char *text){
	PurpleAccount *account = purple_connection_get_account(gc);
	np->handleDisconnected(np->m_accounts[account], purple_account_get_username(account), (int) reason, text ? text : "");
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
	std::string name(who);
	PurpleAccount *account = purple_connection_get_account(gc);
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
			if (label == "Given Name"){
				firstName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label == "Family Name"){
				lastName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Nickname"){
				nickname = purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Full Name"){
				fullName = purple_notify_user_info_entry_get_value(vcardEntry);
			}
		}
		vcardEntries = vcardEntries->next;
	}

	if ((!firstName.empty() || !lastName.empty()) && fullName.empty())
		fullName = firstName + " " + lastName;

	PurpleBuddy *buddy = purple_find_buddy(purple_connection_get_account(gc), who);
	if (buddy) {
		gsize len;
		PurpleBuddyIcon *icon = NULL;
		icon = purple_buddy_icons_find(purple_connection_get_account(gc), name.c_str());
		if (icon) {
			const gchar * data = (gchar*)purple_buddy_icon_get_data(icon, &len);
			// Sometimes libpurple returns really broken pointers here
			// They weren't able to do anything with that and I don't know what to do too,
			// so it's better to hack through it by not trying to forward really broken things...
			if (len < 300000 && data) {
				photo = Swift::createByteArray(data, len);
// 				const gchar *ext = (gchar*)purple_buddy_icon_get_extension(icon);
// 				if (ext) {
// 					std::string extension(ext);
// 					if (extension != "icon") {
// 						if (extension == "jpg") {
// 							extension = "jpeg";
// 						}
// 						photo->addChild( new Tag("TYPE", "image/" + extension) );
// 					}
// 				}
			}
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

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
// 	purple_accounts_set_ui_ops(&accountUiOps);
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
	NULL,
	NULL,
	NULL,
	NULL
};

static void signed_on(PurpleConnection *gc, gpointer unused) {
	for (int i = 0; i<1500; i++) {
		std::cout << "A\n";
	}
	PurpleAccount *account = purple_connection_get_account(gc);
	np->handleConnected(np->m_accounts[account]);
}

static void printDebug(PurpleDebugLevel level, const char *category, const char *arg_s) {
	std::string c("[LIBPURPLE");

	if (category) {
		c.push_back('/');
		c.append(category);
	}

	c.push_back(']');

	std::cout << c << " " << arg_s;
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
	std::string w = who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyping(np->m_accounts[account], w);
}

static void buddyTyped(PurpleAccount *account, const char *who, gpointer null) {
	std::string w = who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyTyped(np->m_accounts[account], w);
}

static void buddyTypingStopped(PurpleAccount *account, const char *who, gpointer null){
	std::string w = who;
	size_t pos = w.find("/");
	if (pos != std::string::npos)
		w.erase((int) pos, w.length() - (int) pos);
	np->handleBuddyStoppedTyping(np->m_accounts[account], w);
}

static bool initPurple(Config &cfg) {
	bool ret;

	purple_util_set_user_dir("./");
	remove("./accounts.xml");
	remove("./blist.xml");

// 	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_ui_ops(&debugUiOps);

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
// 		signal(SIGPIPE, SIG_IGN);
// 
// 		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
// 			std::cout << "SIGCHLD handler can't be set\n";
// 			g_option_context_free(context);
// 			return -1;
// 		}
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

		initPurple(config);

		SpectrumEventLoop eventLoop;
		np = new SpectrumNetworkPlugin(&config, &eventLoop, host, port);
		eventLoop.run();
	}

	g_option_context_free(context);
}
