FROM frolvlad/alpine-glibc:alpine-3.6

EXPOSE 5222 5269
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

ADD . /usr/src/spectrum2

RUN apk add --no-cache ca-certificates && \
    apk add --no-cache --virtual .build-deps cmake make gcc g++ musl-dev boost-dev glib-dev protobuf-dev mariadb-dev sqlite-dev postgresql-dev pidgin-dev libev-dev qt-dev apr-util-dev automake autoconf libtool git popt-dev curl-dev openssl && \
    cd /usr/src/ && \

    wget https://github.com/communi/libcommuni/archive/v3.5.0.tar.gz -O libcommuni-3.5.0.tar.gz && \
    tar xfz libcommuni-*.tar.gz && \
    cd libcommuni-* && \
    ./configure && \
    make && \
    make install && \
    cd .. && rm -rf libcommuni-* && \

    git clone git://git.apache.org/logging-log4cxx.git && \
    cd logging-log4cxx && \
    ./autogen.sh && \
    ./configure && \
    make && \
    make install && \
    cd .. && rm -rf logging-log4cxx && \

    wget http://pqxx.org/download/software/libpqxx/libpqxx-4.0.1.tar.gz && \
    cd libpqxx-* && \
    ./autogen.sh && \
    make && \
    make install && \
    cd .. && rm -rf libpqxx-* && \

    git clone https://github.com/swift/swift.git && \
    cd swift && \
    ./scons SWIFT_INSTALLDIR=/usr/local && \
    cd .. && rm -rf swift && \

    apk del .build-deps
