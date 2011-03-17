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

#ifndef SPECTRUM_EVENT_LOOP_H
#define SPECTRUM_EVENT_LOOP_H

#include <vector>
#include "Swiften/EventLoop/EventLoop.h"
#include "glib.h"

// Event loop implementation for Spectrum
class SpectrumEventLoop : public Swift::EventLoop {
	public:
		// Creates event loop according to CONFIG().eventloop settings.
		SpectrumEventLoop();
		~SpectrumEventLoop();

		// Executes the eventloop.
		void run();

		// Stops tht eventloop.
		void stop();

		// Posts new Swift::Event to main thread.
		virtual void post(const Swift::Event& event);

	private:
		bool m_isRunning;
		GMainLoop *m_loop;
};

#endif
