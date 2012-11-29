---
layout: page
title: Spectrum 2
---

### Before you start

* Check that you really want to run Spectrum 2 in gateway mode and not in server mode. The difference is documentation on [What is Spectrum 2](about.html) page.
* Install Spectrum 2.  Installation is covered in [Installation section on the main page](index.html).

### Configure your XMPP server

XMPP server configuration is addressed on [Configure XMPP server page](configure_xmpp_server.html).

### Configure Spectrum 2

Default config file is located at `/etc/spectrum2/transports/spectrum.cfg.example`. To successfully connect Spectrum 2 to your server, change the following options in the following categories and remove the .example suffix (you can name the file as you want, but it has to have .cfg suffix):

Category|Option|Change to value|Meaning
--------|------|---------------|--------
service|server_mode|0| Spectrum 2 will act as gateway.
service|jid|Jabber ID of your Spectrum 2 transport|You have configured the Jabber ID in your Jabber server config file in previous step.
service|password|Password to connect the Jabber server|You have configured the password in your Jabber server config file in previous step.
service|server|Your Jabber server IP od hostname|Spectrum 2 will connect that IP/hostname.
service|port|Jabber server port to which Spectrum 2 connects|You have configured the port in your Jabber server config file in previous step.

All config options with description are located at [Config file page](config_file.html).

#### Choose the Spectrum 2 backend and legacy network

You have to choose the Spectrum 2 backend and legacy network to which this Spectrum 2 gateway will connect the users.

The default backend is [Libpurple backend](backends/libpurple.html). Read the [Libpurple backend documentation](backends/libpurple.html) to see how to choose particular legacy network. If you want to use different backend, change the path to it according to [List of backends](backends.html).

#### Choose the storage backend

Category| Option| Change to value| Meaning
-|-|-|-
database|type|sqlite3|Spectrum 2 will use SQLite3 as storage backend

You can also use [MySQL](mysql.html) or [PostgreSQL](postgresql.html) instead of SQLite3 backend.

### Start Spectrum 2

The following command should now start your first Spectrum 2 instance:

	spectrum2_manager start


### If something goes wrong

Spectrum 2 logs important messages into `/var/log/spectrum2/<jabber_id>/spectrum2.log`. It's always good to check that log if something goes wrong.
