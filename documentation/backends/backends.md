---
layout: page
title: Spectrum 2
---

Spectrum 2 introduces backends. Backends are external application which provides a way how to connect to legacy networks, so it's possible to use even different libraries than Libpurple to connect the legacy network. However, Libpurple is still the best library to use so far.

This page contains the list of currently supported backends with the basic information about them.

### How to change backend

Backends are switched in Spectrum 2 config file using the following option:

Section| Option| Value
-------|-------|------
service|backend|Full path to the backend binary

### List of backends

Name| Supported networks| Default path to backend
----|-------------------|-------------------------
Libpurple backend|AIM, Jabber, ICQ, MSN, Yahoo, Skype, Telegram, Facebook|`/usr/bin/spectrum2_libpurple_backend`
LibCommuni backend|IRC|`/usr/bin/spectrum2_libcommuni_backend`
Frotz backend|Allows playing interactive-fiction games|`/usr/bin/spectrum2_frotz_backend`
SMSTools3 backend|SMS using connected mobile phone|`/usr/bin/spectrum2_smstools3_backend`
Twitter backend|Twitter|`/usr/bin/spectrum2_twitter_backend`
Transwhat backend|Whatsapp|`/opt/transwhat/transwhat.py`
