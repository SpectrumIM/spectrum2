---
layout: page
title: Spectrum 2
---

### Description

Libpurple backend is backend based on Librpurple library supporting all the networks supported by libpurple

### Configuration

You have to choose this backend in Spectrum 2 configuration file to use it:

	[service]
	backend=/usr/bin/spectrum2_libpurple_backend

There is also special configuration variable in "service" called @protocol@ which decides which Libpurple's protocol will be used:

Protocol variable| Description
-----------------|------------
prpl-jabber| Jabber/Facebook/GTalk
prpl-aim|AIM
prpl-icq|ICQ
prpl-msn|MSN
prpl-yahoo|Yahoo
prpl-gg|Gadu Gadu
prpl-novell|Groupwise

### Third-party plugins

Spectrum 2 should work with any third-party libpurple plugin which is properly installed. For example, popular pidgin-sipe plugin:

Protocol variable| Description
-----------------|------------
prpl-sipe| Microsoft OCS/LCS Messaging

Microsoft LCS account may require username, which is SIP URI, and server login, so use , in username field like this:

	username%domain.com,login%loginserver.tld
