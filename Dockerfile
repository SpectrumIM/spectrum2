FROM fedora:27

EXPOSE 5222
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]
ARG commit=unknown
RUN echo $commit

# Spectrum 2
RUN dnf install ImageMagick protobuf swiften gcc gcc-c++ make libpqxx-devel libpurple-devel protobuf-devel swiften-devel rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel log4cxx-devel gettext libgcrypt-devel libwebp-devel libpurple-devel zlib-devel json-glib-devel python-pip zlib-devel libjpeg-devel python-devel mysql-devel popt-devel git libev-libevent-devel qt-devel dbus-glib-devel libcurl-devel wget mercurial libtool libgnome-keyring-devel nss-devel jsoncpp-devel purple-hangouts -y && \
	echo "---> Installing Spectrum 2" && \
		git clone git://github.com/hanzz/spectrum2.git && \
		cd spectrum2 && \
		./packaging/fedora/build_rpm.sh && \
		rpm -U /root/rpmbuild/RPMS/x86_64/*.rpm && \
		cp ./packaging/docker/run.sh /run.sh && \
		cd .. && \
		rm -rf spectrum2 && \
		rm -rf ~/rpmbuild && \
		dnf mark install json-glib

RUN echo "---> Installing purple-instagram" && \
		git clone https://github.com/EionRobb/purple-instagram.git && \
		cd purple-instagram && \
		make && \
		make install && \
		cd .. && \
		rm -rf purple-instagram
RUN echo "---> Installing icyque" && \
		git clone git://github.com/EionRobb/icyque.git && \
		cd icyque && \
		make && \
		make install && \
		cd .. && \
		rm -rf icyque
RUN echo "---> Installing purple-facebook" && \
		wget https://github.com/dequis/purple-facebook/releases/download/v0.9.6/purple-facebook-0.9.6.tar.gz && \
		tar -xf purple-facebook-0.9.6.tar.gz && \
		cd purple-facebook-0.9.6 && \
		./autogen.sh && \
		./configure && \
		make && \
		make install && \
		cd .. && \
		rm -rf purple-facebook*
RUN echo "---> Installing skype4pidgin" && \
		git clone git://github.com/EionRobb/skype4pidgin.git && \
		cd skype4pidgin/skypeweb && \
		make CFLAGS=-DFEDORA=1 && \
		make install && \
		cd ../.. && \
		rm -rf skype4pidgin
RUN echo "---> Installing transwhat" && \
		pip install --pre e4u six==1.10 protobuf python-dateutil yowsup Pillow==2.9.0 &&\
		git clone git://github.com/stv0g/transwhat.git &&\
		cd transwhat &&\
		git checkout yowsup-3 &&\
		python setup.py install &&\
		cd .. &&\
		rm -r transwhat
RUN echo "---> Installing Telegram" && \
		git clone --recursive https://github.com/majn/telegram-purple && \
		cd telegram-purple && \
		./configure && \
		make && \
		make install && \
		cd .. && \
		rm -rf telegram-purple
RUN echo "---> Install Discord" && \
		git clone https://github.com/EionRobb/purple-discord.git && \
		cd purple-discord && \
		make && \
		make install && \
		cd .. && \
		rm -rf purple-discord
RUN echo "---> Install Steam" && \
		git clone https://github.com/EionRobb/pidgin-opensteamworks.git && \
		cd pidgin-opensteamworks/steam-mobile && \
		make && \
		make install && \
		cd ../.. && \
		rm -rf pidgin-opensteamworks
RUN echo "---> purple-gowhatsapp" && \
		dnf -y install golang && \
		git clone https://github.com/hoehermann/purple-gowhatsapp && \
		cd purple-gowhatsapp && \
		make && \
		make install && \
		cd .. && \
		rm -rf purple-gowhatsapp && \
		dnf -y remove golang
RUN echo "---> cleanup" && \
		rm -rf /usr/share/locale/* && \
		rm -rf /usr/share/doc/* && \
		rm -rf /usr/share/icons/* && \
		rm -rf /usr/share/cracklib* && \
		rm -rf /usr/share/hwdata* && \
		rm -rf /usr/lib64/libQtGui* && \
		rm -rf /usr/lib64/libQtSvg* && \
		rm -rf /usr/lib64/libQtDeclarative* && \
		rm -rf /usr/lib64/libQtOpenGL* && \
		rm -rf /usr/lib64/libQtScriptTools* && \
		rm -rf /usr/lib64/libQtMultimedia* && \
		rm -rf /usr/lib64/libQtHelp* && \
		rm -rf /usr/lib64/libQtDesigner* && \
		rm -rf /usr/lib64/libQt3* && \
		dnf remove protobuf-devel swiften-devel gcc gcc-c++ libpqxx-devel libev-libevent-devel qt-devel dbus-glib-devel libpurple-devel rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel libgcrypt-devel libwebp-devel libpurple-devel zlib-devel json-glib-devel zlib-devel libjpeg-devel python-devel log4cxx-devel mysql-devel popt-devel libcurl-devel spectrum2-debuginfo wget -y && \
		dnf clean all -y && \
		rm -rf /var/lib/rpm/*

CMD "/run.sh"
