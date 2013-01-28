---
layout: page
title: Spectrum 2
---

### Description

Skype is supported by Spectrum 2, but in quite specific way. It's not possible to connect the Skype network without official Skype client running. Therefore you have to have official Skype client installed. Official Skype client is then run for every connected user and Spectrum 2 communicate with it using the DBus interface. One Skype client instance needs approximately 50MB of RAM, therefore Skype transport needs lot of memory (50MB per user).

### Configuration

You have to have:
* Official Skype client installed in Linux PATH
* DBus installed and have running DBus daemon
* xvfb-run tool installed

If you have those depencencies ready, you just have to set the proper backend configuration variable:

	[service]
	backend=/usr/bin/xvfb-run -a -s "-screen 0 10x10x8" -f /tmp/x-skype-gw /usr/bin/spectrum2_skype_backend
