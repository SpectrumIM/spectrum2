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
prpl-facebook| [https://github.com/jgeboski/purple-facebook](https://github.com/jgeboski/purple-facebook) | Facebook
prpl-telegram| [https://github.com/majn/telegram-purple](https://github.com/majn/telegram-purple) | Telegram
prpl-skypeweb| [https://github.com/EionRobb/skype4pidgin/tree/master/skypeweb](https://github.com/EionRobb/skype4pidgin/tree/master/skypeweb) | Skype

These plugins are included by default in our Docker image.

### Setting libpurple plugins configurations

Some libpurple protocol plugins allow setting configuration variables. Spectrum 2 passes every variable set in `purple` section to libpurple library. If you need to set such options, you can do it for example like this in your configuration file:

	[purple]
	clientlogin=1
	ssl=0

### Notes on Facebook support

- It may be a good idea to [set up an application password](https://www.facebook.com/help/249378535085386/) instead of using your real credentials, for security alerts (and facebook paranoid security alerts) reasons.
- Facebook stickers are supported using [Web Storage](../configuration/web_storage.html).
- Your XMPP client should support [XEP-0085: Chat states](https://xmpp.org/extensions/xep-0085.html) to mark messages as read on the Facebook side. Otherwise, messages aren't marked as read until you reply, and sometimes own messages aren't either, so you should set `show-unread=0` (see section above) if you want to avoid receiving duplicates
- Group chats aren't joined automatically. To join a group chat, get its ID through [Facebook messenger's web interface](https://www.messenger.com) and join it as a `GROUP_CHAT_ID@your_facebook_transport.yourdomain.yourtld`
