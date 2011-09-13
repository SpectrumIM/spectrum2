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

#include "transport/util.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

using namespace boost;

namespace Transport {

namespace Util {

void removeEverythingOlderThan(const std::vector<std::string> &dirs, time_t t) {
	BOOST_FOREACH(const std::string &dir, dirs) {
		path p(dir);

		try {
			if (!exists(p)) {
				continue;
			}
			if (!is_directory(p)) {
				continue;
			}

			directory_iterator end_itr;
			for (directory_iterator itr(p); itr != end_itr; ++itr) {
				if (last_write_time(itr->path()) < t) {
					try {
						if (is_regular(itr->path())) {
							remove(itr->path());
						}
						else if (is_directory(itr->path())) {
							std::vector<std::string> nextDirs;
							nextDirs.push_back(itr->path().string());
							removeEverythingOlderThan(nextDirs, t);
							if (is_empty(itr->path())) {
								remove_all(itr->path());
							}
						}
					}
					catch (const filesystem_error& ex) {
						
					}
				}
			}


		}
		catch (const filesystem_error& ex) {
			
		}
	}
}

}

}
