---
layout: page
title: Spectrum 2
---

### Description

Libpurple backend is backend based on Librpurple library supporting all the networks supported by libpurple.

### Configuration

You have to choose this backend in Spectrum 2 configuration file to use it:

	[service]
	backend=/usr/bin/spectrum2_libpurple_backend
	protocol=prpl-jabber

As showed above, there is also special configuration variable in `[service]` section called `protocol` which decides which Libpurple's protocol will be used:

Protocol variable| Description
-----------------|------------
prpl-jabber| Jabber
prpl-aim|AIM
prpl-icq|ICQ
prpl-msn|MSN
prpl-yahoo|Yahoo
prpl-gg|Gadu Gadu
prpl-novell|Groupwise

### Third-party plugins

Spectrum 2 should work with any third-party libpurple plugin which is properly installed. For example, popular plugins:

Protocol variable| website | Description
-----------------|------------
prpl-facebook| [https://github.com/jgeboski/purple-facebook](website) | Facebook
prpl-telegram| [https://github.com/majn/telegram-purple](website) | Telegram
prpl-skypeweb| [https://github.com/EionRobb/skype4pidgin/tree/master/skypeweb](website) | Skype

These plugins are included by default in our Docker image.

### Support for Facebook stickers

Facebook stickers are supported using [Web Storage](../configuration/web_storage.html).

### Setting libpurple plugins configurations

Some libpurple protocol plugins allow setting configuration variables. Spectrum 2 passes every variable set in `purple` section to libpurple library. If you need to set such options, you can do it for example like this in your configuration file:

	[purple]
	clientlogin=1
	ssl=0
