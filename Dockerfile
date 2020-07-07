FROM debian:buster-backports as base

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y dpkg-dev devscripts curl git
RUN echo "deb https://packages.spectrum.im/spectrum2/ buster main" | tee -a /etc/apt/sources.list
RUN echo "deb-src https://packages.spectrum.im/spectrum2/ buster main" | tee -a /etc/apt/sources.list
RUN curl https://packages.spectrum.im/packages.key | apt-key add -

RUN apt-get update -qq
RUN apt-get build-dep --no-install-recommends -y spectrum2
RUN apt-get install --no-install-recommends -y libminiupnpc-dev libnatpmp-dev

#TODO include in Build-Depends
RUN apt-get install --no-install-recommends -y libssl-dev

# Spectrum 2
COPY . spectrum2/

FROM base as test

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR spectrum2

RUN apt-get install --no-install-recommends -y prosody ngircd python-sleekxmpp python-dateutil python-dnspython python-pil libcppunit-dev libpurple-xmpp-carbons1

RUN apt-get install -t buster-backports --no-install-recommends -y cmake

RUN cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON -DENABLE_QT4=OFF -DENABLE_FROTZ=OFF -DCMAKE_UNITY_BUILD=ON . && make

RUN apt-get install --no-install-recommends -y psmisc

ENTRYPOINT ["make", "extended_test"]

FROM base as staging

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR /spectrum2/packaging/debian/

RUN /bin/bash ./build_spectrum2.sh

RUN apt-get install --no-install-recommends -y libjson-glib-dev \
		graphicsmagick-imagemagick-compat libsecret-1-dev libnss3-dev \
		libwebp-dev libgcrypt20-dev libpng-dev

RUN echo "---> Installing purple-instagram" && \
		git clone https://github.com/EionRobb/purple-instagram.git && \
		cd purple-instagram && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Installing icyque" && \
		git clone git://github.com/EionRobb/icyque.git && \
		cd icyque && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Installing skype4pidgin" && \
		git clone git://github.com/EionRobb/skype4pidgin.git && \
		cd skype4pidgin/skypeweb && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Discord" && \
		git clone https://github.com/EionRobb/purple-discord.git && \
		cd purple-discord && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Steam" && \
		git clone https://github.com/EionRobb/pidgin-opensteamworks.git && \
		cd pidgin-opensteamworks/steam-mobile && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-gowhatsapp" && \
		apt-get -y install golang && \
		git clone https://github.com/hoehermann/purple-gowhatsapp && \
		cd purple-gowhatsapp && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-telegram" && \
git clone --recursive https://github.com/majn/telegram-purple && \
		cd telegram-purple && \
		./configure && \
		make && \
		make DESTDIR=/tmp/out install

FROM debian:10.4-slim as production

EXPOSE 5222
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y curl ca-certificates gnupg1

RUN echo "deb https://packages.spectrum.im/spectrum2/ buster main" | tee -a /etc/apt/sources.list
RUN curl -fsSL https://packages.spectrum.im/packages.key | apt-key add -
RUN echo "deb http://download.opensuse.org/repositories/home:/jgeboski/Debian_10/ /" | tee /etc/apt/sources.list.d/home:jgeboski.list
RUN curl -fsSL https://download.opensuse.org/repositories/home:jgeboski/Debian_10/Release.key | apt-key add
RUN echo "deb http://download.opensuse.org/repositories/home:/ars3n1y/Debian_10/ /" | tee /etc/apt/sources.list.d/home:ars3n1y.list
RUN curl -fsSL https://download.opensuse.org/repositories/home:ars3n1y/Debian_10/Release.key | apt-key add
RUN apt-get update -qq

RUN echo "---> Installing purple-facebook" && \
		apt-get install --no-install-recommends -y purple-facebook
RUN echo "---> Installing purple-telegram" && \
		apt-get install --no-install-recommends -y libpurple-telegram-tdlib libtdjson1.6.0


COPY --from=staging /tmp/out/* /usr/

COPY --from=staging spectrum2/packaging/docker/run.sh /run.sh
COPY --from=staging spectrum2/packaging/debian/*.deb /tmp/

RUN apt install --no-install-recommends -y /tmp/*.deb

RUN apt-get autoremove && apt-get clean

ENTRYPOINT ["/run.sh"]
