---
layout: page
title: Spectrum 2
redirect_from: "/documentation/installation/debian_ubuntu.html"
---

## Installing on Debian Stretch from our packages repository

At the moment we only support AMD64 binary packages:

        $ apt-key adv --keyserver hkp://pool.sks-keyservers.net --recv-keys 1AFDEA51
        $ curl https://swift.im/keys/packages.key | sudo apt-key add -
        # echo "deb http://packages.spectrum.im/spectrum2/ stretch main" >> /etc/apt/sources.list.d/spectrum.list
        # echo "deb https://swift.im/packages/debian/stretch beta main" >> /etc/apt/sources.list.d/spectrum.list
        # apt-get install apt-transport-https
        # apt-get update 
        # apt-get install spectrum2 spectrum2-backend-libpurple spectrum2-backend-libcommuni spectrum2-backend-twitter


## Installing on other Debian/Ubuntu-based distributions

You need to rebuild source libcommuni and spectrum packages from our source package repository:

        $ apt-key adv --keyserver hkp://pool.sks-keyservers.net --recv-keys 1AFDEA51
        $ curl https://swift.im/keys/packages.key | sudo apt-key add -
        $ apt-get install devscripts fakeroot libssl-dev libqt4-dev apt-transport-https
        $ echo "deb-src http://packages.spectrum.im/spectrum2/ stretch main" | sudo tee /etc/apt/sources.list.d/spectrum.list
        $ echo "deb http://swift.im/packages/debian/sid beta main" | sudo tee -a /etc/apt/sources.list.d/spectrum.list
        $ apt-get update
        # apt-get install libpurple-dev libswiften-dev libprotobuf-dev libmariadbclient-dev liblog4cxx10-dev protobuf-compiler libpopt-dev libdbus-glib-1-dev libpqxx3-dev cmake libevent-dev libboost-all-dev libidn11-dev libxml2-dev libavahi-client-dev libavahi-common-dev libcurl4-openssl-dev libsqlite3-dev
        $ apt-get source communi spectrum2
        $ cd communi_3.5.0-1 && DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc  && cd ..
        $ cd spectrum2_2.0.9-1 && DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc  && cd ..

When the compilation process has ended the .deb packages for libcommuni and spectrum will be generated in the current directory and can be installed with `dpkg -i < filename.deb >`.

### Troubleshooting
1. If you got gpg verification error, then `dscverify` can not find appropriate keystore, see http://askubuntu.com/a/215008 for fix. This shouldn't happened if you are install keys and build packages from the same account (Note, building doesn't require root)
2. There is an unresolved issue with [MySQL private headers](https://github.com/SpectrumIM/spectrum2/issues/150) - please switch to MariaDB or follow [instructions](https://github.com/SpectrumIM/spectrum2/issues/150#issuecomment-273991724) to patch libpurple

## Quick packaging with CPack

If you want to test latest changes and save time on full rebuild of all packages, you can quickly create a single package from usual build tree, like:

        # cpack -G DEB -D CPACK_PACKAGE_CONTACT="Your Name <your@email.address>" -D CPACK_PACKAGE_NAME="spectrum2-nightly" -D CPACK_PACKAGE_FILE_NAME="spectrum2-nightly" -D CPACK_PACKAGE_VERSION="2.0.x" -D CPACK_DEBIAN_PACKAGE_DEPENDS="libboost-all-dev (>= 1.49), libc6 (>= 2.14), libswiften2 | libswiften3, libcurl3, liblog4cxx10, libpurple0" -D CPACK_DEBIAN_PACKAGE_CONFLICTS="spectrum2, spectrum2-backend-libpurple, spectrum2-backend-twitter, spectrum2-backend-swiften, spectrum2-dbg, libtransport2.0, libtransport-plugin2.0"

