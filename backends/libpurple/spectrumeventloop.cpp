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

#include "spectrumeventloop.h"
#include "purple.h"

#include <iostream>

#ifdef WITH_LIBEVENT
#include <event.h>
#endif


using namespace Swift;

// Fires the event's callback and frees the event
static gboolean processEvent(void *data) {
	Event *ev = (Event *) data;
	ev->callback();
	delete ev;
	return FALSE;
}

SpectrumEventLoop::SpectrumEventLoop() : m_isRunning(false) {
	m_loop = NULL;
	if (true) {
		m_loop = g_main_loop_new(NULL, FALSE);
	}
#ifdef WITH_LIBEVENT
	else {
		/*struct event_base *base = (struct event_base *)*/
		event_init();
	}
#endif
}

SpectrumEventLoop::~SpectrumEventLoop() {
	stop();
}

void SpectrumEventLoop::run() {
	m_isRunning = true;
	if (m_loop) {
		g_main_loop_run(m_loop);
	}
#ifdef WITH_LIBEVENT
	else {
		event_loop(0);
	}
#endif
}

void SpectrumEventLoop::stop() {
	std::cout << "stopped loop\n";
	if (!m_isRunning)
		return;
	if (m_loop) {
		g_main_loop_quit(m_loop);
		g_main_loop_unref(m_loop);
		m_loop = NULL;
	}
#ifdef WITH_LIBEVENT
	else {
		event_loopexit(NULL);
	}
#endif
}

void SpectrumEventLoop::post(const Event& event) {
	// pass copy of event to main thread
	Event *ev = new Event(event.owner, event.callback);
	purple_timeout_add(0, processEvent, ev);
}
