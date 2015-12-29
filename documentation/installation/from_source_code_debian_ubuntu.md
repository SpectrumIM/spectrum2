---
layout: page
title: Spectrum 2
---

## Installing from source-code on Debian/Ubuntu

If you need IRC support, build and install Communi, a required dependency of Spectrum 2:

	# apt-get install git-buildpackage libssl-dev libqt4-dev
	$ git clone git://github.com/vitalyster/libcommuni-gbp.git /tmp/libcommuni-gbp
	$ cd /tmp/libcommuni-gbp
	$ gbp buildpackage -b -uc -us
	$ cd /tmp

When the compilation process has ended the .deb packages for libcommuni and libcommuni-dev will be generated in the current directory and can be installed with `dpkg -i < filename.deb >`.

Next, build and install Spectrum 2:

	# apt-get install libpurple-dev libswiften-dev libprotobuf-dev libmysqlclient-dev liblog4cxx10-dev protobuf-compiler libpopt-dev libdbus-glib-1-dev libpqxx3-dev cmake libevent-dev libboost-all-dev libidn11-dev libxml2-dev libavahi-client-dev libavahi-common-dev libcurl4-openssl-dev libsqlite3-dev
	$ git clone git://github.com/hanzz/spectrum2.git /tmp/spectrum2
	$ cd /tmp/spectrum2/packaging/debian
	$ sh build_spectrum2.sh

When the compilation process has ended the .deb packages will be generated in the current directory and can be installed with `dpkg -i < filename.deb >`.
