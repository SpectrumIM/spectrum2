%global groupname spectrum
%global username spectrum

Summary: XMPP transport
Name: spectrum2
Version: 2.0
Release: %{?_release}%{!?_release:1}%{?dist}
Group: Applications/Internet
License: GPLv3
Source0: spectrum2.tar.gz
URL: http://swift.im/
# BuildRequires: cmake
# BuildRequires: boost-devel
# BuildRequires: mysql-devel
# BuildRequires: cppunit-devel
# BuildRequires: libsqlite3x-devel
# BuildRequires: protobuf-devel
# BuildRequires: protobuf-compiler
# BuildRequires: popt-devel
# BuildRequires: libidn-devel
# BuildRequires: expat-devel
# BuildRequires: avahi-devel
# BuildRequires: log4cxx-devel
Requires: boost
Requires: mysql-libs
%if 0%{?rhel}
Requires: sqlite
%else
Requires: libsqlite3x
%endif
Requires: protobuf
Requires: popt
Requires: libidn
Requires: expat
Requires: avahi
Requires: log4cxx
#----
Requires:      libtransport%{?_isa} = %{version}-%{release}
Requires:      swiften

%description
Spectrum 2.0

%prep
%setup -q -n spectrum2

%build
%cmake . -DCMAKE_BUILD_TYPE=Debug
make VERBOSE=1 %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
install -d %{buildroot}%{_localstatedir}/{lib,run,log}/spectrum2
install -p -D -m 755 packaging/fedora/spectrum2.init \
    %{buildroot}%{_initddir}/spectrum2

ln -s /usr/bin/spectrum2_libpurple_backend %{buildroot}/usr/bin/spectrum_libpurple_backend

%pre
getent group %{groupname} >/dev/null || groupadd -r %{groupname}
getent passwd %{username} >/dev/null || \
    useradd -r -g %{groupname} -d %{_localstatedir}/lib/spectrum \
        -s /sbin/nologin \
        -c "spectrum XMPP transport" %{username}
exit 0

%files
%defattr(-, root, root,-)
%doc README
%{_bindir}/spectrum2
%{_bindir}/spectrum2_manager
/etc/spectrum2/*
%{_initddir}/spectrum2
%attr(700, %{username}, %{groupname}) %{_localstatedir}/lib/spectrum2/
%attr(700, %{username}, %{groupname}) %{_localstatedir}/run/spectrum2/
%attr(700, %{username}, %{groupname}) %{_localstatedir}/log/spectrum2/

%package libpurple-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libpurple-hanzz
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description libpurple-backend
Spectrum2 libpurple backend

%files libpurple-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_libpurple_backend
/usr/bin/spectrum_libpurple_backend
/usr/bin/spectrum2_frotz_backend
/usr/bin/dfrotz

%package libcommuni-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   communi
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description libcommuni-backend
Spectrum2 libpurple backend

%files libcommuni-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_libcommuni_backend

%package smstools3-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description smstools3-backend
Spectrum2 SMSTools3 backend

%files smstools3-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_smstools3_backend

%package skype-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description skype-backend
Spectrum2 Skype backend

%files skype-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_skype_backend

%package swiften-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description swiften-backend
Spectrum2 Swiften backend

%files swiften-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_swiften_backend

%package libyahoo2-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description libyahoo2-backend
Spectrum2 libyahoo2 backend

%files libyahoo2-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_libyahoo2_backend

%package twitter-backend
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost
Requires:   libtransport%{?_isa} = %{version}-%{release}

%description twitter-backend
Spectrum2 libyahoo2 backend

%files twitter-backend
%defattr(-, root, root,-)
/usr/bin/spectrum2_twitter_backend

# %package libircclient-qt-backend
# Summary:    Libtransport
# Group:      Development/Libraries
# Requires:   boost
# Requires:   libpurple
# 
# %description libircclient-qt-backend
# Spectrum2 libircclient-qt backend
# 
# %files libircclient-qt-backend
# %defattr(-, root, root,-)
# /usr/bin/spectrum_libircclient-qt_backend

%package -n libtransport
Summary:    Libtransport
Group:      Development/Libraries
Requires:   boost

%description -n libtransport
Libtransport library

%files -n libtransport
%defattr(-, root, root,-)
%{_libdir}/libtransport*.so*
/usr/include/transport


%changelog
* Nov 30 2012 Jan Kaluza <jkaluza@redhat.com> - 2.0
- Work in progress

* Mon Jul 25 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-4
- rebuild for new boost

* Wed May 25 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-3
- fix #706719 - fixed another crash during login

* Tue Apr 26 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-2
- fix #697832 - fixed crash during login

* Tue Apr 19 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-1
- update to new upstream version 1.0

* Wed Apr 06 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.11.beta9
- rebuild for new boost

* Tue Mar 15 2011 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.10.beta9
- update to new upstream version 1.0-beta9

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0-0.9.beta8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sun Feb 06 2011 Thomas Spura <tomspur@fedoraproject.org> - 1.0-0.8.beta8
- rebuild for new boost

* Wed Nov 10 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.7.beta8
- update to new upstream version 1.0-beta8

* Tue Oct 19 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.6.beta7
- update to new upstream version 1.0-beta7

* Mon Aug 30 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.5.beta6
- update to new upstream version 1.0-beta6

* Tue Aug 10 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.4.beta5
- build with avahi-devel to get DNSSD Support

* Wed Aug 04 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.3.beta5
- build with optflags
- preserve swift.xpm timestamp
- fixed license and install COPYING file
- fixed Changelog entries

* Wed Jul 28 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.2.beta5
- delete all unused bundled libraries
- don't use deprecated BuildRoot tag
- swift.xpm replaced by icon from swift tarball

* Wed Jul 28 2010 Jan Kaluza <jkaluza@redhat.com> - 1.0-0.1.beta5
- created this SPEC file
