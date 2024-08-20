FROM debian:trixie as base

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y dpkg-dev devscripts curl git
RUN echo "deb-src http://deb.debian.org/debian/ trixie main" | tee -a /etc/apt/sources.list
RUN apt-get update -qq
RUN apt-get build-dep --no-install-recommends -y spectrum2
RUN apt-get install --no-install-recommends -y libminiupnpc-dev libnatpmp-dev

RUN apt-get install --no-install-recommends -y cmake

#TODO include in Build-Depends
RUN apt-get install --no-install-recommends -y libssl-dev

# Spectrum 2
COPY . spectrum2/

FROM base as test

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR /spectrum2

RUN apt-get install --no-install-recommends -y prosody ngircd libcppunit-dev purple-xmpp-carbons libglib2.0-dev psmisc
RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_QT4=OFF -DCMAKE_UNITY_BUILD=ON . && make -j4

ENTRYPOINT ["make", "test"]

FROM base as test-clang

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq

RUN apt-get install --no-install-recommends -y libcppunit-dev clang-16 lld-16

WORKDIR /spectrum2

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_QT4=OFF -DCMAKE_UNITY_BUILD=ON -DCMAKE_C_COMPILER=/usr/bin/clang-13 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-13 -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld . && make -j4

ENTRYPOINT ["make", "test"]

FROM ghcr.io/spectrumim/alpine:1.0.1 as test-musl

COPY . /spectrum2/

WORKDIR /spectrum2

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_QT4=OFF -DENABLE_WEBUI=OFF -DCMAKE_UNITY_BUILD=ON . && make -j4

ENTRYPOINT ["make", "test"]

FROM base as staging

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR /spectrum2/packaging/debian/

RUN /bin/bash ./build_spectrum2.sh

RUN apt-get install --no-install-recommends -y libjson-glib-dev \
		graphicsmagick-imagemagick-compat libsecret-1-dev libnss3-dev \
		libwebp-dev libgcrypt20-dev libpng-dev libglib2.0-dev \
		libprotobuf-c-dev protobuf-c-compiler libmarkdown2-dev libopusfile-dev
		
RUN echo "---> Installing purple-instagram" && \
		git clone https://github.com/EionRobb/purple-instagram.git && \
		cd purple-instagram && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Installing icyque" && \
		git clone https://github.com/EionRobb/icyque.git && \
		cd icyque && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Steam" && \
		git clone https://github.com/EionRobb/pidgin-opensteamworks.git && \
		cd pidgin-opensteamworks/steam-mobile && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Teams" && \
		git clone https://github.com/EionRobb/purple-teams.git && \
		cd purple-teams && \
		git checkout 1177f13c88822d75396196391058deab2d5b461a && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Skypeweb" && \
		git clone https://github.com/EionRobb/skype4pidgin.git && \
		cd skype4pidgin/skypeweb && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-gowhatsapp" && \
		apt-get -y install golang && \
		git clone https://github.com/hoehermann/purple-gowhatsapp.git && \
		cd purple-gowhatsapp && \
		git checkout v1.16.0 && \
		git submodule update --init && \
		cmake . && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-battlenet" && \
git clone --recursive https://github.com/EionRobb/purple-battlenet && \
		cd purple-battlenet && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-hangouts" && \
git clone --recursive https://github.com/EionRobb/purple-hangouts && \
		cd purple-hangouts && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-mattermost" && \
git clone --recursive https://github.com/EionRobb/purple-mattermost && \
		cd purple-mattermost && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> tdlib-purple" && \
		apt install -y gperf && \
		cd /tmp && ( git clone https://github.com/tdlib/td.git ; cd td && git checkout 8d08b34 && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd) .. && make && make install/strip ) && ( git clone -b tdlib-1.8.34 https://github.com/667bdrm/tdlib-purple.git ; mkdir -p tdlib-purple/build && cd tdlib-purple/build && cmake -DCMAKE_BUILD_TYPE=Release -DTd_DIR=/tmp/td/build/lib/cmake/Td/ -DNoVoip=1 .. && make install/strip DESTDIR=/tmp/out install)

		
FROM debian:trixie-slim as production

EXPOSE 8080
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y curl
RUN apt-get update -qq

COPY --from=staging spectrum2/packaging/debian/*.deb /tmp/

RUN echo "---> Installing libpurple plugins" && \
		DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y \
		purple-discord \
		purple-facebook \
		libmarkdown2 \
		libogg0 libopusfile0 \
		/tmp/*.deb \
		nodejs \
		&& rm -rf /var/lib/apt/lists/*


COPY --from=staging /tmp/out/* /usr/

COPY --from=staging spectrum2/packaging/docker/run.sh /run.sh

RUN rm -rf /tmp/*.deb

ENTRYPOINT ["/run.sh"]
