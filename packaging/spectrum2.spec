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
install -p -D -m 755 packaging/spectrum2.init \
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
* Mon Sep 17 2012 bump - 2.0-392
- bump

* Sun Sep 16 2012 bump - 2.0-391
- bump

* Sat Sep 15 2012 bump - 2.0-390
- bump

* Fri Sep 14 2012 bump - 2.0-389
- bump

* Thu Sep 13 2012 bump - 2.0-388
- bump

* Wed Sep 12 2012 bump - 2.0-387
- bump

* Wed Sep 12 2012 bump - 2.0-386
- bump

* Tue Sep 11 2012 bump - 2.0-385
- bump

* Mon Sep 10 2012 bump - 2.0-384
- bump

* Sun Sep 09 2012 bump - 2.0-383
- bump

* Sat Sep 08 2012 bump - 2.0-382
- bump

* Fri Sep 07 2012 bump - 2.0-381
- bump

* Thu Sep 06 2012 bump - 2.0-380
- bump

* Wed Sep 05 2012 bump - 2.0-379
- bump

* Tue Sep 04 2012 bump - 2.0-378
- bump

* Mon Sep 03 2012 bump - 2.0-377
- bump

* Sun Sep 02 2012 bump - 2.0-376
- bump

* Sun Sep 02 2012 bump - 2.0-375
- bump

* Sat Sep 01 2012 bump - 2.0-374
- bump

* Sat Sep 01 2012 bump - 2.0-373
- bump

* Sat Sep 01 2012 bump - 2.0-372
- bump

* Fri Aug 31 2012 bump - 2.0-371
- bump

* Fri Aug 31 2012 bump - 2.0-370
- bump

* Thu Aug 30 2012 bump - 2.0-369
- bump

* Wed Aug 29 2012 bump - 2.0-368
- bump

* Tue Aug 28 2012 bump - 2.0-367
- bump

* Mon Aug 27 2012 bump - 2.0-366
- bump

* Sun Aug 26 2012 bump - 2.0-365
- bump

* Sat Aug 25 2012 bump - 2.0-364
- bump

* Fri Aug 24 2012 bump - 2.0-363
- bump

* Thu Aug 23 2012 bump - 2.0-362
- bump

* Wed Aug 22 2012 bump - 2.0-361
- bump

* Tue Aug 21 2012 bump - 2.0-360
- bump

* Mon Aug 20 2012 bump - 2.0-359
- bump

* Sun Aug 19 2012 bump - 2.0-358
- bump

* Sat Aug 18 2012 bump - 2.0-357
- bump

* Fri Aug 17 2012 bump - 2.0-356
- bump

* Thu Aug 16 2012 bump - 2.0-355
- bump

* Wed Aug 15 2012 bump - 2.0-354
- bump

* Tue Aug 14 2012 bump - 2.0-353
- bump

* Mon Aug 13 2012 bump - 2.0-352
- bump

* Sun Aug 12 2012 bump - 2.0-351
- bump

* Sat Aug 11 2012 bump - 2.0-350
- bump

* Fri Aug 10 2012 bump - 2.0-349
- bump

* Thu Aug 09 2012 bump - 2.0-348
- bump

* Thu Aug 09 2012 bump - 2.0-347
- bump

* Wed Aug 08 2012 bump - 2.0-346
- bump

* Tue Aug 07 2012 bump - 2.0-345
- bump

* Mon Aug 06 2012 bump - 2.0-344
- bump

* Sun Aug 05 2012 bump - 2.0-343
- bump

* Sat Aug 04 2012 bump - 2.0-342
- bump

* Fri Aug 03 2012 bump - 2.0-341
- bump

* Thu Aug 02 2012 bump - 2.0-340
- bump

* Wed Aug 01 2012 bump - 2.0-339
- bump

* Tue Jul 31 2012 bump - 2.0-338
- bump

* Mon Jul 30 2012 bump - 2.0-337
- bump

* Sun Jul 29 2012 bump - 2.0-336
- bump

* Sat Jul 28 2012 bump - 2.0-335
- bump

* Fri Jul 27 2012 bump - 2.0-334
- bump

* Thu Jul 26 2012 bump - 2.0-333
- bump

* Wed Jul 25 2012 bump - 2.0-332
- bump

* Tue Jul 24 2012 bump - 2.0-331
- bump

* Mon Jul 23 2012 bump - 2.0-330
- bump

* Sun Jul 22 2012 bump - 2.0-329
- bump

* Sat Jul 21 2012 bump - 2.0-328
- bump

* Fri Jul 20 2012 bump - 2.0-327
- bump

* Thu Jul 19 2012 bump - 2.0-326
- bump

* Wed Jul 18 2012 bump - 2.0-325
- bump

* Tue Jul 17 2012 bump - 2.0-324
- bump

* Mon Jul 16 2012 bump - 2.0-323
- bump

* Sun Jul 15 2012 bump - 2.0-322
- bump

* Sat Jul 14 2012 bump - 2.0-321
- bump

* Fri Jul 13 2012 bump - 2.0-320
- bump

* Thu Jul 12 2012 bump - 2.0-319
- bump

* Wed Jul 11 2012 bump - 2.0-318
- bump

* Tue Jul 10 2012 bump - 2.0-317
- bump

* Mon Jul 09 2012 bump - 2.0-316
- bump

* Sun Jul 08 2012 bump - 2.0-315
- bump

* Sat Jul 07 2012 bump - 2.0-314
- bump

* Fri Jul 06 2012 bump - 2.0-313
- bump

* Thu Jul 05 2012 bump - 2.0-312
- bump

* Wed Jul 04 2012 bump - 2.0-311
- bump

* Tue Jul 03 2012 bump - 2.0-310
- bump

* Mon Jul 02 2012 bump - 2.0-309
- bump

* Sun Jul 01 2012 bump - 2.0-308
- bump

* Sat Jun 30 2012 bump - 2.0-307
- bump

* Fri Jun 29 2012 bump - 2.0-306
- bump

* Thu Jun 28 2012 bump - 2.0-305
- bump

* Wed Jun 27 2012 bump - 2.0-304
- bump

* Tue Jun 26 2012 bump - 2.0-303
- bump

* Mon Jun 25 2012 bump - 2.0-302
- bump

* Sun Jun 24 2012 bump - 2.0-301
- bump

* Sat Jun 23 2012 bump - 2.0-300
- bump

* Fri Jun 22 2012 bump - 2.0-299
- bump

* Thu Jun 21 2012 bump - 2.0-298
- bump

* Wed Jun 20 2012 bump - 2.0-297
- bump

* Tue Jun 19 2012 bump - 2.0-296
- bump

* Mon Jun 18 2012 bump - 2.0-295
- bump

* Sun Jun 17 2012 bump - 2.0-294
- bump

* Sat Jun 16 2012 bump - 2.0-293
- bump

* Fri Jun 15 2012 bump - 2.0-292
- bump

* Thu Jun 14 2012 bump - 2.0-291
- bump

* Wed Jun 13 2012 bump - 2.0-290
- bump

* Tue Jun 12 2012 bump - 2.0-289
- bump

* Mon Jun 11 2012 bump - 2.0-288
- bump

* Sun Jun 10 2012 bump - 2.0-287
- bump

* Sat Jun 09 2012 bump - 2.0-286
- bump

* Fri Jun 08 2012 bump - 2.0-285
- bump

* Thu Jun 07 2012 bump - 2.0-284
- bump

* Thu Jun 07 2012 bump - 2.0-283
- bump

* Wed Jun 06 2012 bump - 2.0-282
- bump

* Wed Jun 06 2012 bump - 2.0-281
- bump

* Wed Jun 06 2012 bump - 2.0-280
- bump

* Wed Jun 06 2012 bump - 2.0-279
- bump

* Wed Jun 06 2012 bump - 2.0-278
- bump

* Tue Jun 05 2012 bump - 2.0-277
- bump

* Mon Jun 04 2012 bump - 2.0-276
- bump

* Sun Jun 03 2012 bump - 2.0-275
- bump

* Sat Jun 02 2012 bump - 2.0-274
- bump

* Fri Jun 01 2012 bump - 2.0-273
- bump

* Thu May 31 2012 bump - 2.0-272
- bump

* Wed May 30 2012 bump - 2.0-271
- bump

* Tue May 29 2012 bump - 2.0-270
- bump

* Mon May 28 2012 bump - 2.0-269
- bump

* Sun May 27 2012 bump - 2.0-268
- bump

* Sat May 26 2012 bump - 2.0-267
- bump

* Sat May 26 2012 bump - 2.0-266
- bump

* Sat May 26 2012 bump - 2.0-265
- bump

* Fri May 25 2012 bump - 2.0-264
- bump

* Fri May 25 2012 bump - 2.0-263
- bump

* Fri May 25 2012 bump - 2.0-262
- bump

* Thu May 24 2012 bump - 2.0-261
- bump

* Wed May 23 2012 bump - 2.0-260
- bump

* Tue May 22 2012 bump - 2.0-259
- bump

* Mon May 21 2012 bump - 2.0-258
- bump

* Sun May 20 2012 bump - 2.0-257
- bump

* Sat May 19 2012 bump - 2.0-256
- bump

* Fri May 18 2012 bump - 2.0-255
- bump

* Thu May 17 2012 bump - 2.0-254
- bump

* Wed May 16 2012 bump - 2.0-253
- bump

* Tue May 15 2012 bump - 2.0-252
- bump

* Mon May 14 2012 bump - 2.0-251
- bump

* Mon May 14 2012 bump - 2.0-250
- bump

* Sun May 13 2012 bump - 2.0-249
- bump

* Sat May 12 2012 bump - 2.0-248
- bump

* Fri May 11 2012 bump - 2.0-247
- bump

* Thu May 10 2012 bump - 2.0-246
- bump

* Wed May 09 2012 bump - 2.0-245
- bump

* Tue May 08 2012 bump - 2.0-244
- bump

* Mon May 07 2012 bump - 2.0-243
- bump

* Sun May 06 2012 bump - 2.0-242
- bump

* Sat May 05 2012 bump - 2.0-241
- bump

* Fri May 04 2012 bump - 2.0-240
- bump

* Thu May 03 2012 bump - 2.0-239
- bump

* Wed May 02 2012 bump - 2.0-238
- bump

* Tue May 01 2012 bump - 2.0-237
- bump

* Mon Apr 30 2012 bump - 2.0-236
- bump

* Sun Apr 29 2012 bump - 2.0-235
- bump

* Sat Apr 28 2012 bump - 2.0-234
- bump

* Fri Apr 27 2012 bump - 2.0-233
- bump

* Thu Apr 26 2012 bump - 2.0-232
- bump

* Wed Apr 25 2012 bump - 2.0-231
- bump

* Tue Apr 24 2012 bump - 2.0-230
- bump

* Mon Apr 23 2012 bump - 2.0-229
- bump

* Sun Apr 22 2012 bump - 2.0-228
- bump

* Sat Apr 21 2012 bump - 2.0-227
- bump

* Fri Apr 20 2012 bump - 2.0-226
- bump

* Thu Apr 19 2012 bump - 2.0-225
- bump

* Wed Apr 18 2012 bump - 2.0-224
- bump

* Tue Apr 17 2012 bump - 2.0-223
- bump

* Mon Apr 16 2012 bump - 2.0-222
- bump

* Sun Apr 15 2012 bump - 2.0-221
- bump

* Sat Apr 14 2012 bump - 2.0-220
- bump

* Sat Apr 14 2012 bump - 2.0-219
- bump

* Fri Apr 13 2012 bump - 2.0-218
- bump

* Thu Apr 12 2012 bump - 2.0-217
- bump

* Wed Apr 11 2012 bump - 2.0-216
- bump

* Tue Apr 10 2012 bump - 2.0-215
- bump

* Mon Apr 09 2012 bump - 2.0-214
- bump

* Sun Apr 08 2012 bump - 2.0-213
- bump

* Sat Apr 07 2012 bump - 2.0-212
- bump

* Fri Apr 06 2012 bump - 2.0-211
- bump

* Thu Apr 05 2012 bump - 2.0-210
- bump

* Wed Apr 04 2012 bump - 2.0-209
- bump

* Tue Apr 03 2012 bump - 2.0-208
- bump

* Mon Apr 02 2012 bump - 2.0-207
- bump

* Sun Apr 01 2012 bump - 2.0-206
- bump

* Sat Mar 31 2012 bump - 2.0-205
- bump

* Fri Mar 30 2012 bump - 2.0-204
- bump

* Thu Mar 29 2012 bump - 2.0-203
- bump

* Wed Mar 28 2012 bump - 2.0-202
- bump

* Tue Mar 27 2012 bump - 2.0-201
- bump

* Mon Mar 26 2012 bump - 2.0-200
- bump

* Sun Mar 25 2012 bump - 2.0-199
- bump

* Sat Mar 24 2012 bump - 2.0-198
- bump

* Fri Mar 23 2012 bump - 2.0-197
- bump

* Thu Mar 22 2012 bump - 2.0-196
- bump

* Wed Mar 21 2012 bump - 2.0-195
- bump

* Tue Mar 20 2012 bump - 2.0-194
- bump

* Mon Mar 19 2012 bump - 2.0-193
- bump

* Sun Mar 18 2012 bump - 2.0-192
- bump

* Sat Mar 17 2012 bump - 2.0-191
- bump

* Fri Mar 16 2012 bump - 2.0-190
- bump

* Thu Mar 15 2012 bump - 2.0-189
- bump

* Wed Mar 14 2012 bump - 2.0-188
- bump

* Tue Mar 13 2012 bump - 2.0-187
- bump

* Mon Mar 12 2012 bump - 2.0-186
- bump

* Sun Mar 11 2012 bump - 2.0-185
- bump

* Sat Mar 10 2012 bump - 2.0-184
- bump

* Fri Mar 09 2012 bump - 2.0-183
- bump

* Thu Mar 08 2012 bump - 2.0-182
- bump

* Wed Mar 07 2012 bump - 2.0-181
- bump

* Tue Mar 06 2012 bump - 2.0-180
- bump

* Mon Mar 05 2012 bump - 2.0-179
- bump

* Sun Mar 04 2012 bump - 2.0-178
- bump

* Sat Mar 03 2012 bump - 2.0-177
- bump

* Fri Mar 02 2012 bump - 2.0-176
- bump

* Thu Mar 01 2012 bump - 2.0-175
- bump

* Wed Feb 29 2012 bump - 2.0-174
- bump

* Tue Feb 28 2012 bump - 2.0-173
- bump

* Mon Feb 27 2012 bump - 2.0-172
- bump

* Sun Feb 26 2012 bump - 2.0-171
- bump

* Sun Feb 26 2012 bump - 2.0-170
- bump

* Sun Feb 26 2012 bump - 2.0-169
- bump

* Sun Feb 26 2012 bump - 2.0-168
- bump

* Sun Feb 26 2012 bump - 2.0-167
- bump

* Sat Feb 25 2012 bump - 2.0-166
- bump

* Sat Feb 25 2012 bump - 2.0-165
- bump

* Sat Feb 25 2012 bump - 2.0-164
- bump

* Fri Feb 24 2012 bump - 2.0-163
- bump

* Thu Feb 23 2012 bump - 2.0-162
- bump

* Wed Feb 22 2012 bump - 2.0-161
- bump

* Tue Feb 21 2012 bump - 2.0-160
- bump

* Mon Feb 20 2012 bump - 2.0-159
- bump

* Sun Feb 19 2012 bump - 2.0-158
- bump

* Sat Feb 18 2012 bump - 2.0-157
- bump

* Fri Feb 17 2012 bump - 2.0-156
- bump

* Thu Feb 16 2012 bump - 2.0-155
- bump

* Thu Feb 16 2012 bump - 2.0-154
- bump

* Thu Feb 16 2012 bump - 2.0-153
- bump

* Wed Feb 15 2012 bump - 2.0-152
- bump

* Tue Feb 14 2012 bump - 2.0-151
- bump

* Mon Feb 13 2012 bump - 2.0-150
- bump

* Sun Feb 12 2012 bump - 2.0-149
- bump

* Sat Feb 11 2012 bump - 2.0-148
- bump

* Fri Feb 10 2012 bump - 2.0-147
- bump

* Thu Feb 09 2012 bump - 2.0-146
- bump

* Wed Feb 08 2012 bump - 2.0-145
- bump

* Tue Feb 07 2012 bump - 2.0-144
- bump

* Mon Feb 06 2012 bump - 2.0-143
- bump

* Sun Feb 05 2012 bump - 2.0-142
- bump

* Sat Feb 04 2012 bump - 2.0-141
- bump

* Fri Feb 03 2012 bump - 2.0-140
- bump

* Thu Feb 02 2012 bump - 2.0-139
- bump

* Wed Feb 01 2012 bump - 2.0-138
- bump

* Tue Jan 31 2012 bump - 2.0-137
- bump

* Mon Jan 30 2012 bump - 2.0-136
- bump

* Sun Jan 29 2012 bump - 2.0-135
- bump

* Sat Jan 28 2012 bump - 2.0-134
- bump

* Fri Jan 27 2012 bump - 2.0-133
- bump

* Thu Jan 26 2012 bump - 2.0-132
- bump

* Wed Jan 25 2012 bump - 2.0-131
- bump

* Tue Jan 24 2012 bump - 2.0-130
- bump

* Tue Jan 24 2012 bump - 2.0-129
- bump

* Tue Jan 24 2012 bump - 2.0-128
- bump

* Mon Jan 23 2012 bump - 2.0-127
- bump

* Sun Jan 22 2012 bump - 2.0-126
- bump

* Sat Jan 21 2012 bump - 2.0-125
- bump

* Fri Jan 20 2012 bump - 2.0-124
- bump

* Fri Jan 20 2012 bump - 2.0-123
- bump

* Fri Jan 20 2012 bump - 2.0-122
- bump

* Thu Jan 19 2012 bump - 2.0-121
- bump

* Wed Jan 18 2012 bump - 2.0-120
- bump

* Wed Jan 18 2012 bump - 2.0-119
- bump

* Tue Jan 17 2012 bump - 2.0-118
- bump

* Mon Jan 16 2012 bump - 2.0-117
- bump

* Sun Jan 15 2012 bump - 2.0-116
- bump

* Sat Jan 14 2012 bump - 2.0-115
- bump

* Fri Jan 13 2012 bump - 2.0-114
- bump

* Thu Jan 12 2012 bump - 2.0-113
- bump

* Wed Jan 11 2012 bump - 2.0-112
- bump

* Tue Jan 10 2012 bump - 2.0-111
- bump

* Mon Jan 09 2012 bump - 2.0-110
- bump

* Sun Jan 08 2012 bump - 2.0-109
- bump

* Sat Jan 07 2012 bump - 2.0-108
- bump

* Fri Jan 06 2012 bump - 2.0-107
- bump

* Fri Jan 06 2012 bump - 2.0-106
- bump

* Fri Jan 06 2012 bump - 2.0-105
- bump

* Thu Jan 05 2012 bump - 2.0-104
- bump

* Wed Jan 04 2012 bump - 2.0-103
- bump

* Tue Jan 03 2012 bump - 2.0-102
- bump

* Mon Jan 02 2012 bump - 2.0-101
- bump

* Sun Jan 01 2012 bump - 2.0-100
- bump

* Sat Dec 31 2011 bump - 2.0-99
- bump

* Fri Dec 30 2011 bump - 2.0-98
- bump

* Thu Dec 29 2011 bump - 2.0-97
- bump

* Wed Dec 28 2011 bump - 2.0-96
- bump

* Tue Dec 27 2011 bump - 2.0-95
- bump

* Mon Dec 26 2011 bump - 2.0-94
- bump

* Sun Dec 25 2011 bump - 2.0-93
- bump

* Sun Dec 25 2011 bump - 2.0-92
- bump

* Sun Dec 25 2011 bump - 2.0-91
- bump

* Sun Dec 25 2011 bump - 2.0-90
- bump

* Sat Dec 24 2011 bump - 2.0-89
- bump

* Fri Dec 23 2011 bump - 2.0-88
- bump

* Thu Dec 22 2011 bump - 2.0-87
- bump

* Wed Dec 21 2011 bump - 2.0-86
- bump

* Wed Dec 21 2011 bump - 2.0-85
- bump

* Wed Dec 21 2011 bump - 2.0-84
- bump

* Wed Dec 21 2011 bump - 2.0-83
- bump

* Tue Dec 20 2011 bump - 2.0-82
- bump

* Tue Dec 20 2011 bump - 2.0-81
- bump

* Tue Dec 20 2011 bump - 2.0-80
- bump

* Mon Dec 19 2011 bump - 2.0-79
- bump

* Sun Dec 18 2011 bump - 2.0-78
- bump

* Sat Dec 17 2011 bump - 2.0-77
- bump

* Fri Dec 16 2011 bump - 2.0-76
- bump

* Thu Dec 15 2011 bump - 2.0-75
- bump

* Tue Dec 13 2011 bump - 2.0-74
- bump

* Mon Dec 12 2011 bump - 2.0-73
- bump

* Mon Dec 12 2011 bump - 2.0-72
- bump

* Sun Dec 11 2011 bump - 2.0-71
- bump

* Sat Dec 10 2011 bump - 2.0-70
- bump

* Fri Dec 09 2011 bump - 2.0-69
- bump

* Thu Dec 08 2011 bump - 2.0-68
- bump

* Wed Dec 07 2011 bump - 2.0-67
- bump

* Wed Dec 07 2011 bump - 2.0-66
- bump

* Mon Dec 05 2011 bump - 2.0-65
- bump

* Mon Dec 05 2011 bump - 2.0-64
- bump

* Sun Dec 04 2011 bump - 2.0-63
- bump

* Sat Dec 03 2011 bump - 2.0-62
- bump

* Sat Dec 03 2011 bump - 2.0-61
- bump

* Fri Dec 02 2011 bump - 2.0-60
- bump

* Thu Dec 01 2011 bump - 2.0-59
- bump

* Wed Nov 30 2011 bump - 2.0-58
- bump

* Tue Nov 29 2011 bump - 2.0-57
- bump

* Mon Nov 28 2011 bump - 2.0-56
- bump

* Sun Nov 27 2011 bump - 2.0-55
- bump

* Sat Nov 26 2011 bump - 2.0-54
- bump

* Fri Nov 25 2011 bump - 2.0-53
- bump

* Thu Nov 24 2011 bump - 2.0-52
- bump

* Wed Nov 23 2011 bump - 2.0-51
- bump

* Tue Nov 22 2011 bump - 2.0-50
- bump

* Mon Nov 21 2011 bump - 2.0-49
- bump

* Sun Nov 20 2011 bump - 2.0-48
- bump

* Sat Nov 19 2011 bump - 2.0-47
- bump

* Fri Nov 18 2011 bump - 2.0-46
- bump

* Thu Nov 17 2011 bump - 2.0-45
- bump

* Wed Nov 16 2011 bump - 2.0-44
- bump

* Tue Nov 15 2011 bump - 2.0-43
- bump

* Tue Nov 15 2011 bump - 2.0-42
- bump

* Tue Nov 15 2011 bump - 2.0-41
- bump

* Mon Nov 14 2011 bump - 2.0-40
- bump

* Mon Nov 14 2011 bump - 2.0-39
- bump

* Mon Nov 14 2011 bump - 2.0-38
- bump

* Mon Nov 14 2011 bump - 2.0-37
- bump

* Mon Nov 14 2011 bump - 2.0-36
- bump

* Sun Nov 13 2011 bump - 2.0-35
- bump

* Sun Nov 13 2011 bump - 2.0-34
- bump

* Sun Nov 13 2011 bump - 2.0-33
- bump

* Sun Nov 13 2011 bump - 2.0-32
- bump

* Sat Nov 12 2011 bump - 2.0-31
- bump

* Sat Nov 12 2011 bump - 2.0-30
- bump

* Fri Nov 11 2011 bump - 2.0-29
- bump

* Thu Nov 10 2011 bump - 2.0-28
- bump

* Wed Nov 09 2011 bump - 2.0-27
- bump

* Tue Nov 08 2011 bump - 2.0-26
- bump

* Mon Nov 07 2011 bump - 2.0-25
- bump

* Sun Nov 06 2011 bump - 2.0-24
- bump

* Sat Nov 05 2011 bump - 2.0-23
- bump

* Sat Nov 05 2011 bump - 2.0-22
- bump

* Sat Nov 05 2011 bump - 2.0-21
- bump

* Fri Nov 04 2011 bump - 2.0-20
- bump

* Thu Nov 03 2011 bump - 2.0-19
- bump

* Thu Nov 03 2011 bump - 2.0-18
- bump

* Wed Nov 02 2011 bump - 2.0-17
- bump

* Wed Nov 02 2011 bump - 2.0-16
- bump

* Wed Nov 02 2011 bump - 2.0-15
- bump

* Wed Nov 02 2011 bump - 2.0-14
- bump

* Tue Nov 01 2011 bump - 2.0-13
- bump

* Tue Nov 01 2011 bump - 2.0-12
- bump

* Tue Nov 01 2011 bump - 2.0-11
- bump

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
