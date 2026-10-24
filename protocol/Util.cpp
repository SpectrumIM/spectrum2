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
#include "transport/utf8.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <random>
#include <cstdint>

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

using namespace std::filesystem;

namespace Transport {

namespace Util {

void createDirectories(Transport::Config *config, const std::filesystem::path& ph) {
	if (ph.empty() || exists(ph)) {
		return;
	}

	// First create branch, by calling ourself recursively
	createDirectories(config, ph.parent_path());
	
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
	for (const std::string &dir : dirs) {
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
				// file_clock has no portable epoch; convert through the current time of both clocks.
				auto ftime = last_write_time(itr->path());
				auto stime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - file_time_type::clock::now() + std::chrono::system_clock::now());
				if (std::chrono::system_clock::to_time_t(stime) < t) {
					try {
						if (is_regular_file(itr->path())) {
							remove(itr->path());
						}
						else if (is_directory(itr->path())) {
							std::vector<std::string> nextDirs;
							nextDirs.push_back(itr->path().string());
							removeEverythingOlderThan(nextDirs, t);
							if (std::filesystem::is_empty(itr->path())) {
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
	for (char c : s) {
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
	int numRequiredBytes = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	std::wstring result;
	result.resize(numRequiredBytes);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), result.data(), numRequiredBytes);
	return result;
}
#endif // _WIN32

static const char base64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::string &input) {
	std::string out;
	out.reserve((input.size() + 2) / 3 * 4);
	size_t i = 0;
	for (; i + 2 < input.size(); i += 3) {
		uint32_t n = (uint8_t) input[i] << 16 | (uint8_t) input[i + 1] << 8 | (uint8_t) input[i + 2];
		out += base64Alphabet[(n >> 18) & 63];
		out += base64Alphabet[(n >> 12) & 63];
		out += base64Alphabet[(n >> 6) & 63];
		out += base64Alphabet[n & 63];
	}
	if (i < input.size()) {
		uint32_t n = (uint8_t) input[i] << 16;
		if (i + 1 < input.size()) {
			n |= (uint8_t) input[i + 1] << 8;
		}
		out += base64Alphabet[(n >> 18) & 63];
		out += base64Alphabet[(n >> 12) & 63];
		out += i + 1 < input.size() ? base64Alphabet[(n >> 6) & 63] : '=';
		out += '=';
	}
	return out;
}

std::string base64Decode(const std::string &input) {
	std::string out;
	out.reserve(input.size() / 4 * 3);
	uint32_t buffer = 0;
	int bits = 0;
	for (char c : input) {
		int value;
		if (c >= 'A' && c <= 'Z') value = c - 'A';
		else if (c >= 'a' && c <= 'z') value = c - 'a' + 26;
		else if (c >= '0' && c <= '9') value = c - '0' + 52;
		else if (c == '+') value = 62;
		else if (c == '/') value = 63;
		else if (c == '=') break;
		else continue; // skip whitespace and line breaks
		buffer = (buffer << 6) | value;
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out += (char) ((buffer >> bits) & 0xFF);
		}
	}
	return out;
}

void replaceAll(std::string &str, std::string_view from, std::string_view to) {
	if (from.empty()) {
		return;
	}
	size_t pos = 0;
	while ((pos = str.find(from, pos)) != std::string::npos) {
		str.replace(pos, from.size(), to);
		pos += to.size();
	}
}

std::string replaceAllCopy(std::string str, std::string_view from, std::string_view to) {
	replaceAll(str, from, to);
	return str;
}

std::vector<std::string> split(std::string_view str, char delim) {
	std::vector<std::string> out;
	size_t start = 0;
	size_t pos;
	while ((pos = str.find(delim, start)) != std::string_view::npos) {
		out.emplace_back(str.substr(start, pos - start));
		start = pos + 1;
	}
	out.emplace_back(str.substr(start));
	return out;
}

static char asciiLower(char c) {
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static bool iEqualsASCII(std::string_view a, std::string_view b) {
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) {
		return asciiLower(x) == asciiLower(y);
	});
}

bool iStartsWith(std::string_view str, std::string_view prefix) {
	return str.size() >= prefix.size() && iEqualsASCII(str.substr(0, prefix.size()), prefix);
}

bool iEndsWith(std::string_view str, std::string_view suffix) {
	return str.size() >= suffix.size() && iEqualsASCII(str.substr(str.size() - suffix.size()), suffix);
}

// Maps the upper case letters of a pair-encoded block (upper and lower case
// alternating) to lower case. 'upperIsEven' tells which of the pair is upper.
static bool inPairBlock(uint32_t c, uint32_t first, uint32_t last, bool upperIsEven, uint32_t &lower) {
	if (c < first || c > last) {
		return false;
	}
	lower = ((c % 2 == 0) == upperIsEven) ? c + 1 : c;
	return true;
}

static uint32_t lowerCodePoint(uint32_t c) {
	uint32_t l;
	if (c < 0x80) return (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
	// Latin-1 Supplement
	if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 0x20;
	// Latin Extended-A
	if (c == 0x130) return 'i';
	if (c == 0x178) return 0xFF;
	if (c == 0x138 || c == 0x149 || c == 0x17F) return c;
	if (inPairBlock(c, 0x139, 0x148, false, l) || inPairBlock(c, 0x179, 0x17E, false, l)) return l;
	if (inPairBlock(c, 0x100, 0x17F, true, l)) return l;
	// Latin Extended-B (the regular parts)
	if (c == 0x1C4 || c == 0x1C5) return 0x1C6;
	if (c == 0x1C7 || c == 0x1C8) return 0x1C9;
	if (c == 0x1CA || c == 0x1CB) return 0x1CC;
	if (c == 0x1F1 || c == 0x1F2) return 0x1F3;
	if (inPairBlock(c, 0x1CD, 0x1DC, false, l)) return l;
	if (inPairBlock(c, 0x1DE, 0x1EF, true, l) || inPairBlock(c, 0x1F8, 0x21F, true, l)) return l;
	if (inPairBlock(c, 0x222, 0x233, true, l) || inPairBlock(c, 0x246, 0x24F, true, l)) return l;
	// Greek
	if (c == 0x386) return 0x3AC;
	if (c >= 0x388 && c <= 0x38A) return c + 0x25;
	if (c == 0x38C) return 0x3CC;
	if (c == 0x38E || c == 0x38F) return c + 0x3F;
	if (c >= 0x391 && c <= 0x3AB && c != 0x3A2) return c + 0x20;
	if (inPairBlock(c, 0x3D8, 0x3EF, true, l)) return l;
	// Cyrillic
	if (c >= 0x400 && c <= 0x40F) return c + 0x50;
	if (c >= 0x410 && c <= 0x42F) return c + 0x20;
	if (c == 0x4C0) return 0x4CF;
	if (inPairBlock(c, 0x4C1, 0x4CE, false, l)) return l;
	if (inPairBlock(c, 0x460, 0x481, true, l) || inPairBlock(c, 0x48A, 0x4BF, true, l) || inPairBlock(c, 0x4D0, 0x52F, true, l)) return l;
	// Armenian
	if (c >= 0x531 && c <= 0x556) return c + 0x30;
	// Georgian
	if (c >= 0x10A0 && c <= 0x10C5) return c + 0x1C60;
	if (c == 0x10C7 || c == 0x10CD) return c + 0x1C60;
	// Latin Extended Additional
	if (c == 0x1E9E) return 0xDF;
	if (inPairBlock(c, 0x1E00, 0x1E95, true, l) || inPairBlock(c, 0x1EA0, 0x1EFF, true, l)) return l;
	// Fullwidth Latin
	if (c >= 0xFF21 && c <= 0xFF3A) return c + 0x20;
	return c;
}

std::string toLowerUTF8(std::string_view str) {
	std::string out;
	out.reserve(str.size());
	if (!utf8::is_valid(str.begin(), str.end())) {
		for (char c : str) {
			out += asciiLower(c);
		}
		return out;
	}
	for (auto it = str.begin(); it != str.end();) {
		utf8::unchecked::append(lowerCodePoint(utf8::unchecked::next(it)), std::back_inserter(out));
	}
	return out;
}

std::string generateUUID() {
	static thread_local std::mt19937_64 rng(std::random_device{}());
	uint8_t bytes[16];
	for (int i = 0; i < 16; i += 8) {
		uint64_t r = rng();
		for (int j = 0; j < 8; j++) {
			bytes[i + j] = (uint8_t) (r >> (j * 8));
		}
	}
	bytes[6] = (bytes[6] & 0x0F) | 0x40; // version 4
	bytes[8] = (bytes[8] & 0x3F) | 0x80; // RFC 4122 variant

	static const char hex[] = "0123456789abcdef";
	std::string out;
	for (int i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			out += '-';
		}
		out += hex[bytes[i] >> 4];
		out += hex[bytes[i] & 0x0F];
	}
	return out;
}

}

}
