FROM debian:bookworm-backports as base

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

# NOTE: bookworm is still on the regular mirrors (unlike bullseye, which had to
# be pulled from archive.debian.org). No override of sources.list should be
# necessary here beyond what debian:bookworm-backports already ships with.
RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y dpkg-dev devscripts curl git
RUN echo "deb [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bookworm main" | tee -a /etc/apt/sources.list
RUN echo "deb-src [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bookworm main" | tee -a /etc/apt/sources.list
RUN curl https://packages.spectrum.im/packages.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/spectrumim.gpg

RUN apt-get update -qq
RUN apt-get build-dep --no-install-recommends -y spectrum2
RUN apt-get install --no-install-recommends -y libminiupnpc-dev libnatpmp-dev

RUN apt-get install --no-install-recommends -y cmake

#TODO include in Build-Depends
RUN apt-get install --no-install-recommends -y libssl-dev
RUN apt-get install --no-install-recommends -y ngircd libcppunit-dev purple-xmpp-carbons libglib2.0-dev psmisc

# Spectrum 2
COPY . spectrum2/

FROM base as test

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

WORKDIR /spectrum2

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_SLACK_FRONTEND=ON -DCMAKE_UNITY_BUILD=ON . && make -j4

ENTRYPOINT ["make", "extended_test"]

FROM base as test-clang

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_LISTCHANGES_FRONTEND=none

# Verified via packages.debian.org: clang-16 (1:16.0.6-15~deb12u1) ships natively
# in bookworm's own repos - no need for the apt.llvm.org third-party repo at all.
RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y libcppunit-dev clang-16 lld-16

WORKDIR /spectrum2

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_SLACK_FRONTEND=ON -DCMAKE_UNITY_BUILD=ON -DCMAKE_C_COMPILER=/usr/bin/clang-16 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-16 -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld . && make -j4

ENTRYPOINT ["make", "test"]

FROM ghcr.io/spectrumim/alpine:1.0.1 as test-musl

COPY . /spectrum2/

WORKDIR /spectrum2

RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_TESTS=ON -DENABLE_WEBUI=OFF -DENABLE_SLACK_FRONTEND=ON -DCMAKE_UNITY_BUILD=ON . && make -j4

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

RUN echo "---> Installing icyque" && \
		git clone https://github.com/EionRobb/icyque.git && \
		cd icyque && \
		git checkout 4d52dbe && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Steam" && \
		git clone https://github.com/EionRobb/pidgin-opensteamworks.git && \
		cd pidgin-opensteamworks/steam-mobile && \
		git checkout 4274774 && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> Install Teams" && \
		git clone https://github.com/EionRobb/purple-teams.git && \
		cd purple-teams && \
		git checkout 62f6fff && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-battlenet" && \
	git clone --recursive https://github.com/EionRobb/purple-battlenet && \
		cd purple-battlenet && \
		git checkout d2113e2 && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-hangouts" && \
	git clone --recursive https://github.com/EionRobb/purple-hangouts && \
		cd purple-hangouts && \
		git checkout 55b9f01 && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-mattermost" && \
	git clone --recursive https://github.com/EionRobb/purple-mattermost && \
		cd purple-mattermost && \
		git checkout 56a7484 && \
		make && \
		make DESTDIR=/tmp/out install

RUN echo "---> purple-discord" && \
	git clone --recursive https://github.com/EionRobb/purple-discord && \
		cd purple-discord && \
		git checkout 64faf80 && \
		make && \
		make DESTDIR=/tmp/out install


FROM debian:bookworm-slim as production

EXPOSE 8080
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

# purple-gowhatsapp / purple-whatsmeow nightly build: needs glibc >= 2.34.
# bookworm ships glibc 2.36, so this is fine (bullseye's 2.31 was NOT enough,
# see below). Bump these two ARGs to move to a newer nightly later.
ARG GOWHATSAPP_RELEASE=nightly-20260901
ARG GOWHATSAPP_DEB_AMD64=purple-whatsmeow_1.22.0_amd64.deb
ARG GOWHATSAPP_DEB_ARM64=purple-whatsmeow_1.22.0_arm64.deb

# NOTE: bookworm is still on the regular mirrors, no override needed here.
RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y curl ca-certificates gnupg1 gpg gpg-agent

RUN echo "deb [signed-by=/etc/apt/trusted.gpg.d/spectrumim.gpg] https://packages.spectrum.im/spectrum2/ bookworm main" | tee -a /etc/apt/sources.list
RUN curl -fsSL https://packages.spectrum.im/packages.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/spectrumim.gpg
RUN apt-get update -qq

COPY --from=staging spectrum2/packaging/debian/*.deb /tmp/

ARG TARGETARCH

RUN echo "---> purple-whatsmeow (gowhatsapp, nightly build, ${TARGETARCH})" && \
		case "${TARGETARCH}" in \
			amd64) GOWHATSAPP_DEB="${GOWHATSAPP_DEB_AMD64}" ;; \
			arm64) GOWHATSAPP_DEB="${GOWHATSAPP_DEB_ARM64}" ;; \
			*) echo "no purple-whatsmeow nightly build for arch ${TARGETARCH}" >&2; exit 1 ;; \
		esac && \
		curl -fL -o /tmp/purple-whatsmeow.deb \
			"https://github.com/hoehermann/purple-gowhatsapp/releases/download/${GOWHATSAPP_RELEASE}/${GOWHATSAPP_DEB}" && \
		apt-get install --no-install-recommends -y /tmp/purple-whatsmeow.deb && \
		rm -f /tmp/purple-whatsmeow.deb

# Telegram removed due to it being broken since 2024 anyway (libtdjson1.7.9 /
# libpurple-telegram-tdlib also aren't available for bookworm regardless).
RUN echo "---> Installing libpurple plugins" && \
		DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -y \
		pidgin-sipe \
		purple-facebook \
		libmarkdown2 \
		libogg0 libopusfile0 \
		frotz \
		/tmp/*.deb \
		nodejs \
		&& rm -rf /var/lib/apt/lists/*


COPY --from=staging /tmp/out/* /usr/

COPY --from=staging spectrum2/packaging/docker/run.sh /run.sh

RUN rm -rf /tmp/*.deb

ENTRYPOINT ["/run.sh"]
