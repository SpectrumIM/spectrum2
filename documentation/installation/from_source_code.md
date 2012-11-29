---
layout: page
title: Spectrum 2
---

## Install Boost and libidn

You have to have boost-devel and libidn-devel installed before compiling Swiften, otherwise it will compile against bundled version of Boost and libidn and Spectrum compilation will fail.

## Install Swiften from git repository:

	git clone git://swift.im/swift
	cd swift
	git checkout swift-2.0beta1
	./scons V=1 swiften_dll=1 Swiften SWIFTEN_INSTALLDIR=/usr/local force-configure=1
	sudo ./scons V=1 swiften_dll=1 Swiften SWIFTEN_INSTALLDIR=/usr/local /usr/local

*Note* - If the output of "./scons" command contains following during the configure stage, you don't have boost-devel or libidn-devel installed during the compilation and *Swiften won't work properly*:

	Checking for C++ header file boost/signals.hpp... no
	....
	Checking for C library idn... no


The proper configure script output looks like this:

	scons: Reading SConscript files ...
	Checking whether the C++ compiler worksyes
	Checking whether the C compiler worksyes
	Checking for C library z... yes
	Checking for C library resolv... yes
	Checking for C library pthread... yes
	Checking for C library dl... yes
	Checking for C library m... yes
	Checking for C library c... yes
	Checking for C library stdc++... yes
	Checking for C++ header file boost/signals.hpp... yes
	Checking for C library boost_signals... yes
	Checking for C++ header file boost/thread.hpp... yes
	Checking for C library boost_thread... no
	Checking for C library boost_thread-mt... yes
	Checking for C++ header file boost/regex.hpp... yes
	Checking for C library boost_regex... yes
	Checking for C++ header file boost/program_options.hpp... yes
	Checking for C library boost_program_options... yes
	Checking for C++ header file boost/filesystem.hpp... yes
	Checking for C library boost_filesystem... yes
	Checking for C++ header file boost/system/system_error.hpp... yes
	Checking for C library boost_system... yes
	Checking for C++ header file boost/date_time/date.hpp... yes
	Checking for C library boost_date_time... yes
	Checking for C++ header file boost/uuid/uuid.hpp... yes
	Checking for C function XScreenSaverQueryExtension()... yes
	Checking for package gconf-2.0... yes
	Checking for C header file gconf/gconf-client.h... yes
	Checking for C library gconf-2... yes
	Checking for C header file libxml/parser.h... no
	Checking for C header file libxml/parser.h... yes
	Checking for C library xml2... yes
	Checking for C header file idna.h... yes
	Checking for C library idn... yes
	Checking for C header file readline/readline.h... yes
	Checking for C library readline... yes
	Checking for C header file avahi-client/client.h... yes
	Checking for C library avahi-client... yes
	Checking for C library avahi-common... yes
	Checking for C header file openssl/ssl.h... yes

Note that you have to have at least Python 2.5 to build Swiften.

## Install Google protobuf

In Fedora, you just have to install following packages:

	sudo yum install protobuf protobuf protobuf-devel

## Install Libpurple for libpurple backend

You should definitely have latest libpurple, so download Pidgin and compile it, because your distribution probably doesn't have the latest one.

## Install libCommuni for libCommuni IRC backend

The instructions are defined on "libCommuni wiki":https://github.com/communi/communi/wiki.

## Install Spectrum 2

	git clone git://github.com/hanzz/libtransport.git
	cd libtransport
	cmake . -DCMAKE_BUILD_TYPE=Debug
	make

Before running make, check cmake output if the supported features are OK for you. If not, install libraries needed by Spectrum to provide specific feature.

You can also install spectrum using "sudo make install"
