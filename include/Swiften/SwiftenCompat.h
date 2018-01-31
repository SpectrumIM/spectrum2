/*
 * Swift compatibility
 *
 * Copyright (c) 2016, Vladimir Matena <vlada.matena@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <Swiften/Version.h>

/*
 * Define macros for Swiften compatible shared pointer and signal namespaces.
 *
 * Using these it is possible to declare shared pointers and signals like this:
 *
 * SWIFTEN_SIGNAL_NAMESPACE::signal signal;
 * SWIFTEN_SIGNAL_CONNECTION_NAMESPACE::connection &connection;
 * SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Type> ptr;
 *
 * These are guaranteed to be the same implementation as Swift uses internally,
 * thus can be used when passign/retrieveing data from/to swiften.
 *
 * This is due to Swift 4 moved from boost::shared_ptr to SWIFTEN_SHRPTR_NAMESPACE::shared_ptr
 * and from boost::signals to boost::signals2 .
 */

#if (SWIFTEN_VERSION >= 0x040000)
#define SWIFTEN_UNIQUE_PTR std::unique_ptr
#define SWIFTEN_SHRPTR_NAMESPACE std
#include <boost/signals2.hpp>
#define SWIFTEN_SIGNAL_NAMESPACE boost::signals2
#define SWIFTEN_SIGNAL_CONNECTION_NAMESPACE boost::signals2
#define SWIFT_HOSTADDRESS(x) *(Swift::HostAddress::fromString(x))
#else
#define SWIFTEN_UNIQUE_PTR std::auto_ptr
#define SWIFTEN_SHRPTR_NAMESPACE boost
#include <boost/signals.hpp>
#define SWIFTEN_SIGNAL_NAMESPACE boost
#define SWIFTEN_SIGNAL_CONNECTION_NAMESPACE boost::signals
#define SWIFT_HOSTADDRESS(x) Swift::HostAddress(x)
#endif

#if (SWIFTEN_VERSION >= 0x030000)
//Swiften supports carbon Sent and Received tags as well as Forwarded tags inside those
#define SWIFTEN_SUPPORTS_CARBONS
//Swiften supports Forwarded tag
#define SWIFTEN_SUPPORTS_FORWARDED
//Privilege tag is implemented locally, but it makes little sense without forwarded tag
#define SWIFTEN_SUPPORTS_PRIVILEGE
#endif