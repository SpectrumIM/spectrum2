---
layout: page
title: Spectrum 2
---

# Spectrum 2 setup guide for beginners

I've created this guide to help beginners like me setup Spectrum 2. If you don't know what Spectrum 2 is, use this [link](http://spectrum.im/) to learn more about it. If you already know about XMPP and Spectrum 2, the [official documentation](http://spectrum.im/documentation/admin.html) will be better suited for you.

## Requirements
* An XMPP server (in this guide I'll be using Prosody)
* Spectrum 2 installed (steps to do so are in the official documentation)

*I'm pretty sure these steps can be applied to any linux distribution but I'll be using Debian Jessie*

## Configuration of Prosody

### Server setup
To configure Prosody would need to edit the default configuration file located at ``/etc/prosody/prosody.cfg.lua``.

First you'll need to create a ``VirtualHost``, this will be used later on by the different accounts on the server. To create a ``VirtualHost``, head close to the end of the file and you will see a line written ``VirtualHost "localhost"``. You can rename *localhost* to anything you want, for the purpose of this guide I will rename it to *server* so the result will be ``VirtualHost "server"``. 

After this you'll need to add an admin account for the server. This can be done by adding an account to the *admins* list at the beginning of the file. The line look like this ``admins = { }`` and the the way the account is added is by writing the ID of the account like this ``*username*@*domain*`` where *domain* is the *VirtualHost* created earlier and *username* a username of you choice. The line should look like this at the end, ``admins = { "admin@server" }``

The last things to add to the configuration file are external components for the different Spectrum services we plan to use. To this, go to the end of the file and add these lines:
	
	Component "name-of-service.domain"
		component_secret "magicalpassword"

In this case if I wanted to run a Twitter transport, the component would be,
	
	Component "twitter.server"
		component_secret "magicaltwitterpassword"

The ``component_secret`` is a password for the Twitter transport and not the actual password of the Twitter account you plan to use. Everything should be okay now and the only thing left to do with Prosody is to create the accounts on the server.

### Account creation
To create an account on the Prosody server you'll need to use the ``prosodyctl`` command. First, let's add the admin account define in the admins list by typing ``sudo prosodyctl adduser admin@server``, this account will be available for administrative purposes only and won't really be used to connect to services using an XMPP client.

You can now created an account for one of the components you added to the ``/etc/prosody/prosody.cfg.lua`` file. For the Twitter transport, a simple account like *twittteruser@server* can be used. **DO NOT USE** something like _twitteruser@**twitter.server**_ it won't work.

## Configuration of Spectrum 2
Before creating configuration files for the Spectrum 2 transports, create a new UNIX user named *spectrum*. This use will execute the transports. After creating it, simply type to ``sudo su spectrum`` to log as this user.

This is an example of a transport config file located at ``/etc/spectrum2/transports``, the file should end with *.cfg*. In this case, this file would be called *twitter.cfg* and it's full path would be ``/etc/spectrum2/transports/twitter.cfg``.

	[service]
	frontend=xmpp
	
	# The name of user/group Spectrum runs as.
	#user=spectrum
	#group=spectrum
	
	# JID of Spectrum instance.
	jid = twitter.server
	admin_jid = admin@server
	# Password used to connect the XMPP server.
	password = magicaltwitterpassword
	
	# XMPP server to which Spectrum connects in gateway mode.
	server = 127.0.0.1
	
	# XMPP server port.
	port = 5347
	
	# Interface on which Spectrum listens for backends.
	backend_host = 127.0.0.1
	
	# Port on which Spectrum listens for backends.
	# By default Spectrum chooses random backend port and there's
	# no need to change it normally
	#backend_port=10001
	
	# Number of users per one legacy network backend.
	users_per_backend=10
	# For Skype - must be =1
	#users_per_backend=1
	
	# Full path to backend binary.
	backend=/usr/bin/spectrum2_twitter_backend
	
	[identity]
	# Name of Spectrum instance in service discovery
	name=Spectrum Twitter Transport
	
	# Type of transport ("msn", "icq", "xmpp").
	# Check http://xmpp.org/registrar/disco-categories.html#gateway
	type=xmpp
	
	# Category of transport, default is "gateway
	category=gateway
	
	[logging]
	# log4cxx/log4j logging configuration file in ini format used for main spectrum2 instance.
	config = /etc/spectrum2/logging-twitter.cfg
	
	# log4cxx/log4j logging configuration file in ini format used for backends.
	backend_config = /etc/spectrum2/backend-logging.cfg
	
	[database]
	# Database backend type
	# "sqlite3", "mysql", "pqxx", or "none" without database backend
	type = sqlite3
	
	[registration]
	# Enable public registrations
	enable_public_registration=1

Please visit the Spectrum 2 and read the official documentation for more configuration options.
