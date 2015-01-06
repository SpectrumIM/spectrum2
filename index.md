---
layout: page
title: Spectrum 2
---

## Looking for developers

As HanzZ (the original main developer of Spectrum) is not able to continue with Spectrum development, we are looking for maintainer and developers for this project. Currently, the development is stalled and only the most urgent problems are fixed.

## About

Spectrum 2 is an XMPP transport/gateway and also simple server.
It allows XMPP users to communicate with their friends who are using one of the supported networks.
It supports a wide range of different networks such as ICQ, XMPP (Jabber, GTalk), AIM, MSN, Facebook, Twitter, or IRC.
Spectrum 2 is written in C++ and uses the [Swiften](http://swift.im/swiften) library and various different libraries for "legacy networks".
Spectrum 2 is open source and released under the GNU GPL.


Spectrum 2 is not intended for strictly desktop users, ie those who have no familiarity with running server side applications.  It is intended to provide a service to desktop users, but be managed by server administrators.  If you are interested in the project but have no familiarity with running a server (and no interest in learning), please speak with your local system administrators about making Spectrum 2 available.

## News

ICQ has changed its server and protocol. Some really old versions of libpurple
in some cases fail to login. If (and only if) the default configuration does not work for you,
you should add these settings to your config file:

	[purple]
	server=login.icq.com
	use_clientlogin=1
	encryption=no_encryption

This will disallow encryption from Spectrum to ICQ server, but there is currently no other way how to make it work.



