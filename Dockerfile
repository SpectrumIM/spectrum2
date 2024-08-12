FROM debian:trixie as base

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y dpkg-dev devscripts curl git
#RUN echo "deb [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bullseye main" | tee -a /etc/apt/sources.list
#RUN echo "deb-src [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bullseye main" | tee -a /etc/apt/sources.list
#RUN curl https://packages.spectrum.im/packages.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/spectrumim.gpg

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y libminiupnpc-dev libnatpmp-dev spectrum2 libpurple-dev cmake libssl-dev

FROM base as staging

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR /spectrum2/packaging/debian/

RUN apt-get install --no-install-recommends -y libjson-glib-dev \
		graphicsmagick-imagemagick-compat libsecret-1-dev libnss3-dev \
		libwebp-dev libgcrypt20-dev libpng-dev libglib2.0-dev \
		libprotobuf-c-dev protobuf-c-compiler libmarkdown2-dev libopusfile-dev build-essential
		
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
		git checkout c0b5d9947e359c6cc8d54ee76af8dba116e0ec72 && \
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


FROM debian:trixie as production

EXPOSE 8080
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y curl ca-certificates gnupg1 gpg gpg-agent

COPY --from=staging spectrum2/packaging/debian/*.deb /tmp/

RUN curl -L https://buildbot.hehoe.de/purple-whatsmeow/builds/libwhatsmeow.so -o /usr/local/lib/libwhatsmeow.so && ldconfig

RUN echo "---> Installing libpurple plugins" && \
		DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y \
		pidgin-sipe \
		#libpurple-telegram-tdlib \
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

RUN curl -L https://github.com/SpectrumIM/spectrum2/blob/54801b3e7e2c0f13d5aac668bc45a1d8e95a2855/packaging/docker/run.sh -o /run.sh

RUN rm -rf /tmp/*.deb

ENTRYPOINT ["/run.sh"]
