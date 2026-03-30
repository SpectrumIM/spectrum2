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

// win32/libc_interface.h defines its own socket(), read() and so on.
// We don't want to use it here.
#define _LIBC_INTERFACE_H_ 1

#include "utils.h"

#include "glib.h"
#include "purple.h"
#include <algorithm>
#include <iostream>
#include <vector>

#include "errno.h"
#include <string.h>

#ifndef WIN32 
#include "sys/wait.h"
#include "sys/signal.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#else 
#include <process.h>
#define getpid _getpid 
#define ssize_t SSIZE_T
#include "win32/win32dep.h"
#define close closesocket
#endif

#include "purple_defs.h"

#include <boost/numeric/conversion/cast.hpp>

using std::vector;

static GHashTable *ui_info = NULL;

void execute_purple_plugin_action(PurpleConnection *gc, const std::string &name) {
	PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED_WRAPPED(gc) ? gc->prpl : NULL;
	if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
		PurplePluginAction *action = NULL;
		GList *actions, *l;

		actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

		for (l = actions; l != NULL; l = l->next) {
			if (l->data) {
				action = (PurplePluginAction *) l->data;
				action->plugin = plugin;
				action->context = gc;
				if ((std::string) action->label == name) {
					action->callback(action);
				}
				purple_plugin_action_free_wrapped(action);
			}
		}
	}
}

GHashTable *spectrum_ui_get_info(void)
{
	if (NULL == ui_info) {
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

#ifndef WIN32
void spectrum_sigchld_handler(int sig)
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
#endif

int create_socket(const char *host, int portno) {
	struct sockaddr_in stSockAddr;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (-1 == SocketFD) {
		return 0;
	}

	hostent *hos;
	if ((hos = gethostbyname(host)) == NULL) {
		// strerror() will not work for gethostbyname() and hstrerror() 
		// is supposedly obsolete
		return 0;
	}

	memset(&stSockAddr, 0, sizeof(stSockAddr));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(portno);
	memcpy(&(stSockAddr.sin_addr.s_addr), hos->h_addr,  hos->h_length);

	if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr))) {
		close(SocketFD);
		return 0;
	}

	return SocketFD;
}
