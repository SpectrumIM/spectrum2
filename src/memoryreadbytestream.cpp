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

#include "transport/memoryreadbytestream.h"
#include "log4cxx/logger.h"
#include <boost/foreach.hpp>

using namespace log4cxx;

namespace Transport {
	
MemoryReadBytestream::MemoryReadBytestream(unsigned long size) {
	neededData = false;
	m_finished = false;
	m_sent = 0;
	m_size = size;
}

MemoryReadBytestream::~MemoryReadBytestream() {
	
}

unsigned long MemoryReadBytestream::appendData(const std::string &data) {
	m_data += data;
	onDataAvailable();
	neededData = false;
	return m_data.size();
}

boost::shared_ptr<std::vector<unsigned char> > MemoryReadBytestream::read(size_t size) {
	if (m_data.empty()) {
		onDataNeeded();
		return boost::shared_ptr<std::vector<unsigned char> >(new std::vector<unsigned char>());
	}

	if (m_data.size() < size) {
		boost::shared_ptr<std::vector<unsigned char> > ptr(new std::vector<unsigned char>(m_data.begin(), m_data.end()));
		m_sent += m_data.size();
		m_data.clear();
		if (m_sent == m_size)
			m_finished = true;
		onDataNeeded();
		return ptr;
	}
	boost::shared_ptr<std::vector<unsigned char> > ptr(new std::vector<unsigned char>(m_data.begin(), m_data.begin() + size));
	m_data.erase(m_data.begin(), m_data.begin() + size);
	m_sent += size;
	if (m_sent == m_size)
		m_finished = true;
	if (m_data.size() < 500000 && !neededData) {
		neededData = true;
		onDataNeeded();
	}
	return ptr;
}

bool MemoryReadBytestream::isFinished() const {
// 	std::cout << "finished? " << m_finished << "\n";
	return m_finished;
}

}
