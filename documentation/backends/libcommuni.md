---
layout: page
title: Spectrum 2
---

## Description

LibCommuni backend is IRC backend which uses [Communi IRC library](https://github.com/communi/communi/wiki). It's specialized IRC backend and it should replace libpurple IRC support.

### Configuration

You have to choose this backend in Spectrum 2 configuration file to use it:

	[service]
	backend=/usr/bin/spectrum2_libcommuni_backend

LibCommuni backend can then work in two modes.

### One transport per one IRC network

This is preferred way if you know that you or your users will need to connect just one IRC network. It's also good mode of you maintain IRC server and want to provide XMPP access to it.

In this mode users can:

* connect the IRC network without joining the IRC channel.
* identify to NickServ (or any other service like that) using username and password from transport registration.
* have IRC contacts in their rosters. (Not done yet, but it's planned)
* see channel list in the service discovery. (Not done yet, but it's planned)

To use this mode, you have to configure irc_server variable like this:

	[service]
	irc_server=irc.freenode.org

### One transport for more IRC networks

In this mode users can connect more IRC networks, but they can't connect the network without being in the room. To connect the network, user has to join the room in following format: #room%irc.freenode.org@irc.domain.tld. The nickname used in the first join request is used as a nickname for the IRC connection.

The port of IRC server can be also used, but it in that case, the Jabber ID of the room has to be encoded using [JID Escaping](http://www.xmpp.org/extensions/xep-0106.html). For example to join the IRC network on port 6697, you have to use following Jabber ID: #room\40irc.freenode.org\3a6697@irc.domain.tld.

###  All configuration variables

Key | Type | Default | Description
----|------|---------|------------
`service.irc_server` | string | | IRC server hostname for "One transport per one IRC network" mode. The port can be specified in the `service.irc_server` variable. If you use "+" character before the port number (For example "irc.freenode.org:+7000"), then the SSL is used. SSL is also used by default when port is configured to 6697.
`service.irc_identify` | string | NickServ identify $name $password | The fiirst word is nickname of service used for identifying. After the nickname there's a message sent to that service. $name is replaced by the username defined by user in the registration. $password is replaced by password.
`service.irc_send_pass` | bool | false | When set to true, the password used when registering the account is used as a server password when connecting the server.

