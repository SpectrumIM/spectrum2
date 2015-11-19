FROM fedora:23

EXPOSE 5222
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

RUN dnf install protobuf protobuf swiften gcc gcc-c++ make libpqxx-devel libpurple-devel protobuf-devel swiften-devel rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel log4cxx-devel mysql-devel popt-devel git libevent-devel qt-devel dbus-glib-devel libcurl-devel -y && \
	git clone git://github.com/hanzz/libtransport.git && \
	cd libtransport && \
	./packaging/fedora/build_rpm.sh && \
	rpm -U /root/rpmbuild/RPMS/x86_64/*.rpm && \
	cp ./packaging/docker/run.sh /run.sh && \
	cd .. && \
	rm -rf libtransport && \
	rm -rf ~/rpmbuild && \
	rm -rf /usr/share/locale/* && \
	rm -rf /usr/share/doc/* && \
	dnf remove protobuf-devel swiften-devel gcc gcc-c++ libpqxx-devel libevent-devel qt-devel dbus-glib-devel libpurple-devel make rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel log4cxx-devel mysql-devel popt-devel libcurl-devel spectrum2-debuginfo yum perl -y && \
	dnf clean all -y

CMD "/run.sh"
