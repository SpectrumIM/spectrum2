FROM fedora:23

EXPOSE 5222
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

# Spectrum 2
RUN dnf install protobuf protobuf swiften gcc gcc-c++ make libpqxx-devel libpurple-devel protobuf-devel swiften-devel rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel log4cxx-devel json-glib-devel mysql-devel popt-devel git libevent-devel qt-devel dbus-glib-devel libcurl-devel wget -y && \
	echo "---> Installing Spectrum 2" && \
	git clone git://github.com/hanzz/spectrum2.git && \
	cd spectrum2 && \
	./packaging/fedora/build_rpm.sh && \
	rpm -U /root/rpmbuild/RPMS/x86_64/*.rpm && \
	cp ./packaging/docker/run.sh /run.sh && \
	cd .. && \
	rm -rf spectrum2 && \
	rm -rf ~/rpmbuild && \
	echo "---> Installing purple-facebook" && \
	wget https://github.com/jgeboski/purple-facebook/releases/download/6a0a79182ebc/purple-facebook-6a0a79182ebc.tar.gz && \
	tar -xf purple-facebook-6a0a79182ebc.tar.gz  && cd purple-facebook-6a0a79182ebc && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf purple-facebook* && \
	echo "---> cleanup" && \
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
	dnf remove protobuf-devel swiften-devel gcc gcc-c++ libpqxx-devel libevent-devel qt-devel dbus-glib-devel libpurple-devel make rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel json-glib-devel log4cxx-devel mysql-devel popt-devel libcurl-devel spectrum2-debuginfo yum perl wget -y && \
	dnf clean all -y && \
	rm -rf /var/lib/rpm/*

CMD "/run.sh"
