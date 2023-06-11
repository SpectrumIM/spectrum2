FROM debian:bullseye-backports as base

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y dpkg-dev devscripts curl git
RUN echo "deb [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bullseye main" | tee -a /etc/apt/sources.list
RUN echo "deb-src [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bullseye main" | tee -a /etc/apt/sources.list
RUN curl https://packages.spectrum.im/packages.key | gpg --no-default-keyring --keyring=/etc/apt/trusted.gpg.d/spectrumim.gpg --import -

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

RUN apt-get install --no-install-recommends -y prosody ngircd python3-sleekxmpp python3-dateutil python3-dnspython libcppunit-dev purple-xmpp-carbons libglib2.0-dev psmisc

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_QT4=OFF -DCMAKE_UNITY_BUILD=ON . && make -j4

ENTRYPOINT ["make", "extended_test"]

FROM base as test-clang

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN echo 'deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-13 main' > /etc/apt/sources.list.d/llvm.list
RUN apt-get update -qq

RUN apt-get install --no-install-recommends -y libcppunit-dev clang-13 lld-13

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

RUN echo "---> purple-gowhatsapp" && \
		apt-get -y install -t bullseye-backports golang && \
		git clone https://github.com/hoehermann/purple-gowhatsapp.git && \
		cd purple-gowhatsapp && \
		git checkout v1.11.1 && \
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

		
FROM debian:bullseye-slim as production

EXPOSE 8080
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y curl ca-certificates gnupg1

RUN echo "deb https://packages.spectrum.im/spectrum2/ bullseye main" | tee -a /etc/apt/sources.list
RUN curl -fsSL https://packages.spectrum.im/packages.key | apt-key add -
RUN apt-get update -qq

COPY --from=staging spectrum2/packaging/debian/*.deb /tmp/

RUN echo "---> Installing libpurple plugins" && \
		DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y \
		pidgin-sipe \
		libpurple-telegram-tdlib \
		libtdjson1.7.9 \
		purple-discord \
		purple-facebook \
		libmarkdown2 \
		skypeweb \
		libogg0 libopusfile0 \
		/tmp/*.deb \
		nodejs \
		&& rm -rf /var/lib/apt/lists/*


COPY --from=staging /tmp/out/* /usr/

COPY --from=staging spectrum2/packaging/docker/run.sh /run.sh

RUN rm -rf /tmp/*.deb

ENTRYPOINT ["/run.sh"]
