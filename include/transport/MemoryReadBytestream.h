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

#pragma once

#include <string>
#include <map>

#include "Swiften/FileTransfer/ReadBytestream.h"
#include "Swiften/SwiftenCompat.h"

namespace Transport {

class MemoryReadBytestream : public Swift::ReadBytestream {
	public:
		MemoryReadBytestream(unsigned long size);
		virtual ~MemoryReadBytestream();

		unsigned long appendData(const std::string &data);

		virtual SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<std::vector<unsigned char> > read(size_t size);

		void setFinished() { m_finished = true; }
		bool isFinished() const;

		SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onDataNeeded;

	private:
		bool m_finished;
		std::string m_data;
		bool neededData;
		unsigned long m_sent;
		unsigned long m_size;
};

}
