#include "glib.h"
#include "purple.h"
#include <iostream>

#include "transport/config.h"
#include "transport/transport.h"
#include "transport/usermanager.h"
#include "transport/logger.h"
#include "transport/sqlite3backend.h"
#include "transport/userregistration.h"
#include "spectrumeventloop.h"
#include "geventloop.h"



using namespace Transport;

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

static void transport_core_ui_init(void)
{
// 	purple_blist_set_ui_ops(&blistUiOps);
// 	purple_accounts_set_ui_ops(&accountUiOps);
// 	purple_notify_set_ui_ops(&notifyUiOps);
// 	purple_request_set_ui_ops(&requestUiOps);
// 	purple_xfers_set_ui_ops(getXferUiOps());
// 	purple_connections_set_ui_ops(&conn_ui_ops);
// 	purple_conversations_set_ui_ops(&conversation_ui_ops);
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

static bool initPurple(Config &cfg) {
	bool ret;

	purple_util_set_user_dir("./");

// 	if (m_configuration.logAreas & LOG_AREA_PURPLE)
// 		purple_debug_set_ui_ops(&debugUiOps);

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
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
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
			std::cout << "Can't open sample.cfg configuration file.\n";
			return 1;
		}

		initPurple(config);

		SpectrumEventLoop eventLoop;
		Component transport(&eventLoop, &config);
		Logger logger(&transport);
		transport.connect();
		
		eventLoop.run();
	}

	g_option_context_free(context);
}
