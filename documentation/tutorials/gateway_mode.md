---
layout: page
title: Spectrum 2
---

### Before you start

* Check that you really want to run Spectrum 2 in gateway mode and not in server mode. The difference is documentation on [What is Spectrum 2](http://spectrum.im/documentation/about.html) page.
* Install Spectrum 2.  Installation is covered in [Installation section on the main page](http://spectrum.im/documentation/).

### Configure your XMPP server

You have to change your XMPP server configuration and set JID of the Spectrum 2 instance and the password it will use there.

* [Adding components with Prosody](http://prosody.im/doc/components)
* [Adding components with Ejabberd](http://www.ejabberd.im/node/5134)

### Configure Spectrum 2

Default config file is located at `/etc/spectrum2/transports/spectrum.cfg.example`. To successfully connect Spectrum 2 to your server, change the following options in the following categories and remove the .example suffix (you can name the file as you want, but it has to have .cfg suffix):

Category|Option|Change to value|Meaning
--------|------|---------------|--------
service|jid|Jabber ID of your Spectrum 2 transport|You have configured the Jabber ID in your Jabber server config file in previous step.
service|password|Password to connect the Jabber server|You have configured the password in your Jabber server config file in previous step.
service|server|Your Jabber server IP od hostname|Spectrum 2 will connect that IP/hostname.
service|port|Jabber server port to which Spectrum 2 connects|You have configured the port in your Jabber server config file in previous step.

All config options with description are located at [Config file page](http://spectrum.im/documentation/configuration/config_file.html).

#### Choose the Spectrum 2 backend and legacy network

You have to choose the Spectrum 2 backend and legacy network to which this Spectrum 2 gateway will connect the users.

The default backend is [Libpurple backend](http://spectrum.im/documentation/backends/libpurple.html). Read the [Libpurple backend documentation](http://hanzz.github.com/libtransport/documentation/backends/libpurple.html) to see how to choose particular legacy network. If you want to use different backend, change the path to it according to [List of backends](http://hanzz.github.com/libtransport/documentation/backends/backends.html).

#### Choose the storage backend

By default, Spectrum 2 uses SQLite3. You don't have to configure it explicitely.
You can also use [MySQL](http://spectrum.im/documentation/configuration/mysql.html) or [PostgreSQL](http://hanzz.github.com/libtransport/documentation/configuration/postgresql.html) instead of SQLite3 backend.

### Start Spectrum 2

The following command should now start your first Spectrum 2 instance:

	spectrum2_manager start


### If something goes wrong

Spectrum 2 logs important messages into `/var/log/spectrum2/<jabber_id>/spectrum2.log`. It's always good to check that log if something goes wrong.
