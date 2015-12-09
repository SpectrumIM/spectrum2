FROM fedora:23

EXPOSE 5222
VOLUME ["/etc/spectrum2/transports", "/var/lib/spectrum2"]

# Spectrum 2
RUN dnf install protobuf protobuf swiften gcc gcc-c++ make libpqxx-devel libpurple-devel protobuf-devel swiften-devel rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel log4cxx-devel json-glib-devel mysql-devel popt-devel git libevent-devel qt-devel dbus-glib-devel libcurl-devel wget -y

RUN git clone git://github.com/hanzz/spectrum2.git && \
	cd spectrum2 && \
	./packaging/fedora/build_rpm.sh && \
	rpm -U /root/rpmbuild/RPMS/x86_64/*.rpm && \
	cp ./packaging/docker/run.sh /run.sh && \
	cd .. && \
	rm -rf spectrum2 && \
	rm -rf ~/rpmbuild

# purple-facebook
RUN wget https://github.com/jgeboski/purple-facebook/releases/download/6a0a79182ebc/purple-facebook-6a0a79182ebc.tar.gz && \
	tar -xf purple-facebook-6a0a79182ebc.tar.gz  && cd purple-facebook-6a0a79182ebc && \
	./configure && \
	make && \
	make install && \
	cd .. && \
	rm -rf purple-facebook

# Cleanup
RUN rm -rf /usr/share/locale/* && \
	rm -rf /usr/share/doc/* && \
	dnf remove protobuf-devel swiften-devel gcc gcc-c++ libpqxx-devel libevent-devel qt-devel dbus-glib-devel libpurple-devel make rpm-build avahi-devel boost-devel cmake cppunit-devel expat-devel libcommuni-devel libidn-devel libsqlite3x-devel json-glib-devel log4cxx-devel mysql-devel popt-devel libcurl-devel spectrum2-debuginfo yum perl wget -y && \
	dnf clean all -y

CMD "/run.sh"
