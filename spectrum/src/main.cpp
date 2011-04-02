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
#include "transport/abstractconversation.h"
#include "spectrumeventloop.h"
#include "spectrumbuddy.h"
#include "spectrumconversation.h"
#include "geventloop.h"

#define Log(X, STRING) std::cout << "[SPECTRUM] " << X << " " << STRING << "\n";


using namespace Transport;

Logger *_logger;

static gboolean nodaemon = FALSE;
static gchar *logfile = NULL;
static gchar *lock_file = NULL;
static gboolean ver = FALSE;
static gboolean upgrade_db = FALSE;
static gboolean check_db_version = FALSE;
static gboolean list_purple_settings = FALSE;

static GOptionEntry options_entries[] = {
	{ "nodaemon", 'n', 0, G_OPTION_ARG_NONE, &nodaemon, "Disable background daemon mode", NULL },
	{ "logfile", 'l', 0, G_OPTION_ARG_STRING, &logfile, "Set file to log", NULL },
	{ "pidfile", 'p', 0, G_OPTION_ARG_STRING, &lock_file, "File where to write transport PID", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Shows Spectrum version", NULL },
	{ "list-purple-settings", 's', 0, G_OPTION_ARG_NONE, &list_purple_settings, "Lists purple settings which can be used in config file", NULL },
	{ "upgrade-db", 'u', 0, G_OPTION_ARG_NONE, &upgrade_db, "Upgrades Spectrum database", NULL },
	{ "check-db-GlooxMessageHandler::version", 'c', 0, G_OPTION_ARG_NONE, &check_db_version, "Checks Spectrum database version", NULL },
	{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, "", NULL }
};

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	User *user = (User *) account->ui_data;

	if (!user)
		return;

	SpectrumBuddy *s_buddy = NULL;
	GSList *list = purple_find_buddies(account, purple_buddy_get_name(buddy));
	while (list) {
		PurpleBuddy *b = (PurpleBuddy *) list->data;
		if (b->node.ui_data)
			s_buddy = (SpectrumBuddy *) b->node.ui_data;
		list = g_slist_delete_link(list, list);
	}

	if (s_buddy) {
		buddy->node.ui_data = s_buddy;
		s_buddy->addBuddy(buddy);
	}
	else {
		buddy->node.ui_data = (void *) new SpectrumBuddy(user->getRosterManager(), -1, buddy);
		SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
		s_buddy->setFlags(BUDDY_JID_ESCAPING);
	}
}

static void buddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	User *user = (User *) account->ui_data;

	if (!user || !s_buddy)
		return;

	s_buddy->buddyChanged();
}

static void buddySignedOn(PurpleBuddy *buddy) {
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	User *user = (User *) account->ui_data;

	if (!user || !s_buddy)
		return;

	s_buddy->buddyChanged();
}

static void buddySignedOff(PurpleBuddy *buddy) {
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	User *user = (User *) account->ui_data;

	if (!user || !s_buddy)
		return;

	s_buddy->buddyChanged();
}

static void NodeRemoved(PurpleBlistNode *node, void *data) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	
// 	PurpleAccount *account = purple_buddy_get_account(buddy);
// 	User *user = (User *) account->ui_data;
	if (buddy->node.ui_data) {
		SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
		s_buddy->removeBuddy(buddy);
		buddy->node.ui_data = NULL;
		if (s_buddy->getBuddiesCount() == 0) {
			delete s_buddy;
		}
	}
}

static PurpleBlistUiOps blistUiOps =
{
	NULL,
	buddyListNewNode,
	NULL,
	NULL, // buddyListUpdate,
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

static void conv_new(PurpleConversation *conv) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) account->ui_data;

	if (!user)
		return;

	std::string name = purple_conversation_get_name(conv);
	size_t pos = name.find("/");
	if (pos != std::string::npos)
		name.erase((int) pos, name.length() - (int) pos);

	SpectrumConversation *s_conv = new SpectrumConversation(user->getConversationManager(), name, conv);
	conv->ui_data = s_conv;
}

static void conv_destroy(PurpleConversation *conv) {
	SpectrumConversation *s_conv = (SpectrumConversation *) conv->ui_data;
	if (s_conv) {
		delete s_conv;
	}
}

static void conv_write_im(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	// Don't forwards our own messages.
	if (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)
		return;
	SpectrumConversation *s_conv = (SpectrumConversation *) conv->ui_data;
	if (!s_conv)
		return;

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());

	char *striped = purple_markup_strip_html(message);
	msg->setBody(message);
	g_free(striped);

	s_conv->handleMessage(msg);
}

static PurpleConversationUiOps conversation_ui_ops =
{
	conv_new,
	conv_destroy,
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

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
// 	purple_accounts_set_ui_ops(&accountUiOps);
// 	purple_notify_set_ui_ops(&notifyUiOps);
// 	purple_request_set_ui_ops(&requestUiOps);
// 	purple_xfers_set_ui_ops(getXferUiOps());
// 	purple_connections_set_ui_ops(&conn_ui_ops);
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
		static int conversation_handle;
		static int conn_handle;
		static int blist_handle;

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
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typed", &conversation_handle, PURPLE_CALLBACK(buddyTyped), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
// 		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
// 
// 		purple_commands_init();

	}
	return ret;
}

static void handleUserReadyToConnect(User *user) {
	PurpleAccount *account = (PurpleAccount *) user->getData();
	purple_account_set_enabled(account, "spectrum", TRUE);
	
	const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account, PURPLE_STATUS_AVAILABLE);
	if (status_type != NULL) {
		purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, NULL);
	}
}

static void handleUserCreated(User *user, UserManager *userManager, Config *config) {
	UserInfo userInfo = user->getUserInfo();
	PurpleAccount *account = NULL;
	const char *protocol = CONFIG_STRING(config, "service.protocol").c_str();
	if (purple_accounts_find(userInfo.uin.c_str(), protocol) != NULL){
		Log(userInfo.jid, "this account already exists");
		account = purple_accounts_find(userInfo.uin.c_str(), protocol);
		User *u = (User *) account->ui_data;
		if (u && u != user) {
			Log(userInfo.jid, "This account is already connected by another jid " << user->getJID());
			return;
		}
	}
	else {
		Log(userInfo.jid, "creating new account");
		account = purple_account_new(userInfo.uin.c_str(), protocol);

		purple_accounts_add(account);
	}
// 	Transport::instance()->collector()->stopCollecting(m_account);

// 	PurplePlugin *plugin = purple_find_prpl(protocol);
// 	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
// 	for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
// 		PurpleAccountOption *option = (PurpleAccountOption *) l->data;
// 		purple_account_remove_setting(account, purple_account_option_get_setting(option));
// 	}
// 
// 	std::map <std::string, PurpleAccountSettingValue> &settings = Transport::instance()->getConfiguration().purple_account_settings;
// 	for (std::map <std::string, PurpleAccountSettingValue>::iterator it = settings.begin(); it != settings.end(); it++) {
// 		PurpleAccountSettingValue v = (*it).second;
// 		std::string key((*it).first);
// 		switch (v.type) {
// 			case PURPLE_PREF_BOOLEAN:
// 				purple_account_set_bool(m_account, key.c_str(), v.b);
// 				break;
// 
// 			case PURPLE_PREF_INT:
// 				purple_account_set_int(m_account, key.c_str(), v.i);
// 				break;
// 
// 			case PURPLE_PREF_STRING:
// 				if (v.str)
// 					purple_account_set_string(m_account, key.c_str(), v.str);
// 				else
// 					purple_account_remove_setting(m_account, key.c_str());
// 				break;
// 
// 			case PURPLE_PREF_STRING_LIST:
// 				// TODO:
// 				break;
// 
// 			default:
// 				continue;
// 		}
// 	}

	purple_account_set_string(account, "encoding", userInfo.encoding.empty() ? CONFIG_STRING(config, "registration.encoding").c_str() : userInfo.encoding.c_str());
	purple_account_set_bool(account, "use_clientlogin", false);
// 	purple_account_set_bool(account, "require_tls",  Transport::instance()->getConfiguration().require_tls);
// 	purple_account_set_bool(account, "use_ssl",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(account, "direct_connect", false);
// 	purple_account_set_bool(account, "check-mail", purple_value_get_boolean(getSetting("enable_notify_email")));

	account->ui_data = user;
	user->setData(account);

	user->onReadyToConnect.connect(boost::bind(&handleUserReadyToConnect, user));
	_logger->setRosterManager(user->getRosterManager());
	
// 	Transport::instance()->protocol()->onPurpleAccountCreated(m_account);

// 	m_loadingBuddiesFromDB = true;
// 	loadRoster();
// 	m_loadingBuddiesFromDB = false;

// 	m_connectionStart = time(NULL);
// 	m_readyForConnect = false;
	purple_account_set_password(account, userInfo.password.c_str());
// 	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);
}

static void handleUserDestroyed(User *user, UserManager *userManager, Config *config) {
	PurpleAccount *account = (PurpleAccount *) user->getData();
	if (account) {
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

		account->ui_data = NULL;
// 		Transport::instance()->collector()->collect(m_account);
	}
}

class SpectrumFactory : public Factory {
	public:
		AbstractConversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName) {
			PurpleConversation *conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, (PurpleAccount *) conversationManager->getUser()->getData() , legacyName.c_str());
			return (AbstractConversation *) conv->ui_data;
		}
};

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
		SpectrumFactory factory;
		Component transport(&eventLoop, &config, &factory);
		Logger logger(&transport);
		_logger = &logger;

		SQLite3Backend sql(&config);
		logger.setStorageBackend(&sql);
		if (!sql.connect()) {
			std::cout << "Can't connect to database.\n";
		}

		UserManager userManager(&transport, &sql);
		userManager.onUserCreated.connect(boost::bind(&handleUserCreated, _1, &userManager, &config));
		userManager.onUserDestroyed.connect(boost::bind(&handleUserDestroyed, _1, &userManager, &config));

		UserRegistration userRegistration(&transport, &userManager, &sql);
		logger.setUserRegistration(&userRegistration);
		logger.setUserManager(&userManager);

		transport.connect();
		eventLoop.run();
	}

	g_option_context_free(context);
}
