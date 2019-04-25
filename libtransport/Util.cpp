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

#include "transport/Util.h"
#include "transport/Config.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/cast.hpp>

#ifndef WIN32
#include "sys/signal.h"
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/resource.h>
#include "libgen.h"
#else
#include <windows.h>
#include <process.h>
#define getpid _getpid
#endif

using namespace boost::filesystem;

namespace Transport {

namespace Util {

void createDirectories(Transport::Config *config, const boost::filesystem::path& ph) {
	if (ph.empty() || exists(ph)) {
		return;
	}

	// First create branch, by calling ourself recursively
	createDirectories(config, ph.branch_path());
	
	// Now that parent's path exists, create the directory
	create_directory(ph);

#ifndef WIN32
	if (!CONFIG_STRING(config, "service.group").empty() && !CONFIG_STRING(config, "service.user").empty()) {
		struct group *gr;
		if ((gr = getgrnam(CONFIG_STRING(config, "service.group").c_str())) == NULL) {
			std::cerr << "Invalid service.group name " << CONFIG_STRING(config, "service.group") << "\n";
		}
		struct passwd *pw;
		if ((pw = getpwnam(CONFIG_STRING(config, "service.user").c_str())) == NULL) {
			std::cerr << "Invalid service.user name " << CONFIG_STRING(config, "service.user") << "\n";
		}
		chown(ph.string().c_str(), pw->pw_uid, gr->gr_gid);
	}
#endif
}

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
							if (boost::filesystem::is_empty(itr->path())) {
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

int getRandomPort(const std::string &s) {
	unsigned long r = 0;
	BOOST_FOREACH(char c, s) {
		r += (int) c;
	}
	srand(time(NULL) + r);
	return 30000 + rand() % 10000;
}

std::string char2hex( char dec )
{
	char dig1 = (dec&0xF0)>>4;
	char dig2 = (dec&0x0F);
	if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48 in ascii
	if (10<= dig1 && dig1<=15) dig1+=65-10; //A,65 in ascii
	if ( 0<= dig2 && dig2<= 9) dig2+=48;
	if (10<= dig2 && dig2<=15) dig2+=65-10;

    std::string r;
	r.append( &dig1, 1);
	r.append( &dig2, 1);
	return r;
}

std::string urlencode( const std::string &c )
{

    std::string escaped;
	int max = c.length();
	for (int i=0; i<max; i++)
	{
		if ( (48 <= c[i] && c[i] <= 57) ||//0-9
			(65 <= c[i] && c[i] <= 90) ||//ABC...XYZ
			(97 <= c[i] && c[i] <= 122) || //abc...xyz
			(c[i]=='~' || c[i]=='-' || c[i]=='_' || c[i]=='.')
			)
		{
			escaped.append( &c[i], 1);
		}
		else
		{
			escaped.append("%");
			escaped.append( char2hex(c[i]) );//converts char 255 to string "FF"
		}
	}
	return escaped;
}

#ifdef _WIN32
std::wstring utf8ToUtf16(const std::string& str)
{
	try
	{
		if (str.empty())
			return L"";

		// First request the size of the required UTF-16 buffer
		int numRequiredBytes = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), boost::numeric_cast<int>(str.size()), NULL, 0);
		if (!numRequiredBytes)
			return L"";

		// Allocate memory for the UTF-16 string
		std::vector<wchar_t> utf16Str(numRequiredBytes);

		int numConverted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), boost::numeric_cast<int>(str.size()), &utf16Str[0], numRequiredBytes);
		if (!numConverted)
			return L"";

		std::wstring wstr(&utf16Str[0], numConverted);
		return wstr;
	}
	catch (...)
	{
		// I don't believe libtransport is exception-safe so we'll just return an empty string instead
		return L"";
	}
}
#endif // _WIN32


}

}
