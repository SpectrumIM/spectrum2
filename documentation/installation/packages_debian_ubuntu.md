---
layout: page
title: Spectrum 2
redirect_from: "/documentation/installation/debian_ubuntu.html"
---

## Installing on Debian Jessie from our packages repository

At the moment we support AMD64 and armhf (for ARM-based machines, like Raspberry PI) binary packages:

        # apt-key adv --keyserver hkp://pool.sks-keyservers.net --recv-keys 1AFDEA51
        # echo "deb http://packages.spectrum.im/spectrum2/ jessie main" >> /etc/apt/sources.list.d/spectrum.list
        # echo "deb-src http://packages.spectrum.im/spectrum2/ jessie main" >> /etc/apt/sources.list.d/spectrum.list
        # apt-get update 
        # apt-get install spectrum2 spectrum2-backend-libpurple spectrum2-backend-libcommuni spectrum2-backend-twitter


## Installing on other Debian/Ubuntu-based distributions

You need to rebuild source libcommuni and spectrum packages from our source package repository:

        # apt-key adv --keyserver hkp://pool.sks-keyservers.net --recv-keys 1AFDEA51 
        # apt-get install devscripts fakeroot libssl-dev libqt4-dev
        # dget -x http://packages.spectrum.im/spectrum2/pool/main/c/communi/communi_3.4.0-1.dsc
        # cd communi_3.4.0-1 && dpkg-buildpackage -rfakeroot -us -uc  && cd ..
        # apt-get install libpurple-dev libswiften-dev libprotobuf-dev libmysqlclient-dev liblog4cxx10-dev protobuf-compiler libpopt-dev libdbus-glib-1-dev libpqxx3-dev cmake libevent-dev libboost-all-dev libidn11-dev libxml2-dev libavahi-client-dev libavahi-common-dev libcurl4-openssl-dev libsqlite3-dev libcommuni-dev
        # dget -x http://packages.spectrum.im/spectrum2/pool/main/s/spectrum2/spectrum2_2.0.3-1.dsc
        # cd spectrum2_2.0.3-1 && DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc  && cd ..

When the compilation process has ended the .deb packages for libcommuni and spectrum will be generated in the current directory and can be installed with `dpkg -i < filename.deb >`.
