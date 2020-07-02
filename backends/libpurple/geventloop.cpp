/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "geventloop.h"
#ifdef _WIN32
#include "win32/win32dep.h"
#undef read
#undef write
#endif
#ifdef WITH_LIBEVENT
#include "event.h"
#endif

#include "purple_defs.h"

#include "transport/Logging.h"

DEFINE_LOGGER(eventLoopLogger, "EventLoop");

typedef struct _PurpleIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
#ifdef WITH_LIBEVENT
	GSourceFunc function2;
	struct timeval timeout;
	struct event evfifo;
#endif
} PurpleIOClosure;

static gboolean io_invoke(GIOChannel *source,
										GIOCondition condition,
										gpointer data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;

	int tmp = 0;
	if (condition & READ_COND)
	{
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (condition & WRITE_COND)
	{
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}

	closure->function(closure->data, g_io_channel_unix_get_fd(source), purple_cond);

	return TRUE;
}

static void io_destroy(gpointer data)
{
	g_free(data);
}

static guint input_add(gint fd,
								PurpleInputCondition condition,
								PurpleInputFunction function,
								gpointer data)
{
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;
	closure->function = function;
	closure->data = data;

	int tmp = 0;
	if (condition & PURPLE_INPUT_READ)
	{
		tmp |= READ_COND;
		cond = (GIOCondition)tmp;
	}
	if (condition & PURPLE_INPUT_WRITE)
	{
		tmp |= WRITE_COND;
		cond = (GIOCondition)tmp;
	}

#ifdef WIN32
	channel = wpurple_g_io_channel_win32_new_socket_wrapped(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
	io_invoke, closure, io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}

static PurpleEventLoopUiOps eventLoopOps =
{
	g_timeout_add,
	g_source_remove,
	input_add,
	g_source_remove,
	NULL,
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif

	NULL,
	NULL,
	NULL
};

#ifdef WITH_LIBEVENT

static GHashTable *events = NULL;
static unsigned long id = 0;

static void event_io_destroy(gpointer data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	event_del(&closure->evfifo);
	g_free(data);
}

static void event_io_invoke(int fd, short event, void *data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;
	int tmp = 0;
	if (event & EV_READ)
	{
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (event & EV_WRITE)
	{
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (event & EV_TIMEOUT)
	{
// 		tmp |= PURPLE_INPUT_WRITE;
// 		purple_cond = (PurpleInputCondition)tmp;
		LOG4CXX_INFO(eventLoopLogger, "before timer callback " << closure->function2);
		if (closure->function2(closure->data))
			evtimer_add(&closure->evfifo, &closure->timeout);
		LOG4CXX_INFO(eventLoopLogger, "after timer callback" << closure->function2);
// 		else
// 			event_io_destroy(data);
		return;
	}
	LOG4CXX_INFO(eventLoopLogger, "before callback " << closure->function);
	closure->function(closure->data, fd, purple_cond);
	LOG4CXX_INFO(eventLoopLogger, "after callback" << closure->function);
}

static gboolean event_input_remove(guint handle)
{
	PurpleIOClosure *closure = (PurpleIOClosure *) g_hash_table_lookup(events, &handle);
	if (closure)
		event_io_destroy(closure);
	return TRUE;
}

static guint event_input_add(gint fd,
								PurpleInputCondition condition,
								PurpleInputFunction function,
								gpointer data)
{
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;
	closure->function = function;
	closure->data = data;

	int tmp = EV_PERSIST;
	if (condition & PURPLE_INPUT_READ)
	{
		tmp |= EV_READ;
	}
	if (condition & PURPLE_INPUT_WRITE)
	{
		tmp |= EV_WRITE;
	}

	event_set(&closure->evfifo, fd, tmp, event_io_invoke, closure);
	event_add(&closure->evfifo, NULL);
	
	int *f = (int *) g_malloc(sizeof(int));
	*f = id;
	id++;
	g_hash_table_replace(events, f, closure);
	
	return *f;
}

static guint event_timeout_add (guint interval, GSourceFunc function, gpointer data) {
	struct timeval timeout;
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	closure->function2 = function;
	closure->data = data;
	
	timeout.tv_sec = interval/1000;
	timeout.tv_usec = (interval%1000)*1000;
	evtimer_set(&closure->evfifo, event_io_invoke, closure);
	evtimer_add(&closure->evfifo, &timeout);
	closure->timeout = timeout;
	
	guint *f = (guint *) g_malloc(sizeof(guint));
	*f = id;
	id++;
	g_hash_table_replace(events, f, closure);
	return *f;
}

static PurpleEventLoopUiOps libEventLoopOps =
{
	event_timeout_add,
	event_input_remove,
	event_input_add,
	event_input_remove,
	NULL,
// #if GLIB_CHECK_VERSION(2,14,0)
// 	g_timeout_add_seconds,
// #else
	NULL,
// #endif

	NULL,
	NULL,
	NULL
};

#endif /* WITH_LIBEVENT*/

PurpleEventLoopUiOps * getEventLoopUiOps(bool libev){
#ifdef WITH_LIBEVENT
	if (libev) {
		event_init();
		events = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);
		return &libEventLoopOps;
	}
	else {
		return &eventLoopOps;
	}
#endif
	return &eventLoopOps;
}
