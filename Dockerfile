FROM frolvlad/alpine-glibc:alpine-3.6

EXPOSE 5222 5269
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

ADD . /usr/src/spectrum2

RUN apk add --no-cache ca-certificates && \
	apk add --no-cache --virtual .build-deps cmake make gcc g++ musl-dev boost-dev glib-dev protobuf-dev mariadb-dev sqlite-dev postgresql-dev pidgin-dev libev-dev qt-dev apr-util-dev automake autoconf libtool git popt-dev curl-dev openssl libevent-dev && \
	cd /usr/src/ && \

	wget https://github.com/communi/libcommuni/archive/v3.5.0.tar.gz -O libcommuni-3.5.0.tar.gz && \
	tar xfz libcommuni-*.tar.gz && \
	cd libcommuni-* && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf libcommuni-* && \

	git clone git://git.apache.org/logging-log4cxx.git && \
	cd logging-log4cxx && \
	./autogen.sh && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf logging-log4cxx && \

	wget http://pqxx.org/download/software/libpqxx/libpqxx-4.0.1.tar.gz && \
	tar xfz libpqxx-*.tar.gz && \
	cd libpqxx-* && \
	./autogen.sh && \
	./configure --enable-shared --disable-documentation && \
	make && \
	make install && \
	cd .. && \
	rm -rf libpqxx-* && \

	wget https://swift.im/downloads/releases/swift-4.0rc2/swift-4.0rc2.tar.gz && \
	tar xfz swift-*.tar.gz && \
	cd swift-* && \
	./scons Swiften SWIFTEN_INSTALLDIR=/usr/local /usr/local && \
	cd .. && \
	rm -rf swift-* && \

	cd spectrum2 && \
	cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr . && \
	make && \
	make install && \
	cd .. && \

	wget https://github.com/dequis/purple-facebook/releases/download/v0.9.4-c9b74a765767/purple-facebook-0.9.4-c9b74a765767.tar.gz && \
	tar -xf purple-facebook-0.9.4-c9b74a765767.tar.gz && \
	cd purple-facebook-0.9.4-c9b74a765767 && \
	./autogen.sh && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf purple-facebook* && \

	git clone git://github.com/EionRobb/skype4pidgin.git && \
	cd skype4pidgin/skypeweb && \
	make && \
	make install && \
	cd ../.. && \
	rm -rf skype4pidgin && \

	pip install --pre e4u protobuf python-dateutil yowsup2 Pillow==2.9.0 && \
	git clone git://github.com/stv0g/transwhat.git && \
	git clone git://github.com/tgalal/yowsup.git && \
	cd transwhat && \
	git worktree add ../transwhat && \
	cd .. && \
	cd yowsup && \
	cp -R yowsup ../transwhat/yowsup && \
	cd .. && \
	rm -r transwhat && \
	rm -r yowsup && \

	git clone --recursive https://github.com/majn/telegram-purple && \
	cd telegram-purple && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf telegram-purple && \

	git clone https://github.com/EionRobb/purple-discord.git && \
	cd purple-discord && \
	make && \
	make install && \
	cd .. && \
	rm -rf purple-discord && \

	git clone https://github.com/EionRobb/pidgin-opensteamworks.git && \
	cd pidgin-opensteamworks/steam-mobile && \
	make && \
	make install && \
	cd ../.. && \
	rm -rf pidgin-opensteamworks && \

	apk del .build-deps
