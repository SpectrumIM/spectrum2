---
layout: config
title: Spectrum 2
---

* [service.server_mode](#serviceserver_mode)
* [service.jid](#servicejid)

### Types of configuration fields

Following types are used:

integer - Examples: key=0

string - Examples: key=something

boolean - Examples: key=0 or key=1

list - List of strings (or Jabber IDs). You can specify this options more than once:

	allowed_servers=domain.tld
	allowed_servers=example.com

#### service.server_mode

Key | val
----|----
Description:|Configures if Spectrum 2 works in server mode or gateway mode
Context:|server-mode and gateway-mode
Type:|boolean
Default:|0

If this option is true, Spectrum 2 works in server-mode and acts as standalone server.

User then logins legacy networks by logging XMPP account like this one: `my_msn_name%hotmail.com@msn.domain.tld`.

*Advantages:*
* Passwords are not stored on server.
* Roster synchronization is easy, because Spectrum 2 acts as normal server.
* If you want to use Spectrum 2 as wrapper between different networks, you don't need database or Jabber server as another layer.
* Using SRV records you can easily run Spectrum 2 on different machines to scale it.

*Disadvantages:*

* Clients have to support more accounts to connect more legacy networks (Therefore they will need have to use more TCP connections).

If this option is false, Spectrum 2 acts as normal XMPP component (gateway).

You then have to configure an external XMPP server (like Prosody or Ejabberd) to serve the subdomain you want to use for Spectrum 2 (for example "icq.domain.tld"). Spectrum 2 in gateway mode then connects the XMPP server as its component and users are able to find out "icq.domain.tld" in Service Discovery, register it and use it.

*Advantages:*
* Users can use more legacy networks using single XMPP account (and using single TCP connection).
* It's easy to extend existing XMPP servers using gateway mode.

*Disadvantages:*
* Passwords are stored (even in encrypted form) on server.
* Roster (contact list) synchronization can be problematic, because it depends on the client user uses. This can be improved by usage of Remote Roster protoXEP.
* You have to setup XMPP server and use database even if you only want to use Spectrum 2 as a tool to connect legacy networks using XMPP protocol.


#### service.jid

Key | val
----|----
Description:|Configures Jabber ID of Spectrum 2 instance
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures Jabber ID of particular Spectrum 2 instance. Usually it is a hostname on which this Spectrum 2 instance
runs, so for example `icq.domain.tld`. Note that you have to have DNS records configured for this hostname, so clients will be able
to find out your Jabber server (in case you are running Spectrum 2 in gateway-mode) or the Spectrum 2 itself (if you are running
Spectrum 2 in server-mode).

#### service.server

Key | val
----|----
Description:|Configures hostname or IP address of server to which Spectrum 2 connects to.
Context:|gateway-mode
Type:|string
Default:|empty string

This option configures hostname or IP address of server to which Spectrum 2 connects to. It is used only in gateway-mode and
you should configure it to point to hostname or IP of your XMPP server.

#### service.port

Key | val
----|----
Description:|Configures port on which Spectrum listens to in server-mode or to which connects in gateway-mode.
Context:|server-mode and gateway-mode
Type:|integer
Default:|0

This option configures port on which Spectrum listens to in server-mode or to which connects in gateway-mode. In server-mode
the default port for XMPP servers is 5222, so you should use this port. In gateway-mode, you have to at first configure your
server to allow Spectrum 2 to connect it as its component. On many servers, the default component port is 5347, but this option
depends on particular XMPP server and its configuration.

#### service.password

Key | val
----|----
Description:|Configures password to be used to connect XMPP server
Context:|gateway-mode
Type:|integer
Default:|0

This option configures port on which Spectrum listens to in server-mode or to which connects in gateway-mode. In server-mode
the default port for XMPP servers is 5222, so you should use this port. In gateway-mode, you have to at first configure your
server to allow Spectrum 2 to connect it as its component. On many servers, the default component port is 5347, but this option
depends on particular XMPP server and its configuration.

#### service.cert

Key | val
----|----
Description:|Configures certificate to be used for SSL encryption in server-mode.
Context:|server-mode
Type:|string
Default:|empty string

This option configures full path to PKCS#12 certificate which is used for SSL in server-mode. To find out, how to create
such certificate, please read [Using SSL in server mode](http://spectrum.im/documentation/configuration/server_ssl.html).

#### service.cert_password

Key | val
----|----
Description:|Configures password for certificate which is used for SSL encryption in server-mode.
Context:|server-mode
Type:|string
Default:|empty string

This option configures password which is used to decrypt PKCS#12 certificate (if it is encrypted). For more information about SSL
with Spectrum 2, read service.cert option description.

#### service.admin_jid

Key | val
----|----
Description:|Configures list of Jabber IDs, which has ability to use Admin Interface.
Context:|server-mode and gateway-mode
Type:|list of Jabber IDs
Default:|empty list

This option configures list of Jabber IDs, which has ability to use Admin Interface. In server-mode, you also have to specify
service.admin_password to set password for Admin Interface. In gatewa-mode, you just specify the list of Jabber IDs, Spectrum 2 does
not check any password in gateway-mode.

#### service.admin_password

Key | val
----|----
Description:|Configures password which is used by clients defined in service.admin_jid to use Admin Interface.
Context:|server-mode
Type:|string
Default:|empty string

This option configures the password which is used by clients defined in service.admin_jid to use Admin Interface.

#### service.allowed_servers

Key | val
----|----
Description:|Configures list of servers from which users can connect and register Spectrum 2 transport.
Context:|gateway-mode
Type:|list of JIDs
Default:|empty list

Configures list of servers from which users can connect and register Spectrum 2 transport. This option is used together with
registration.enable_public_registration option. If registration.enable_public_registration is set to 0, you can use this option
as a white-list, to enable users from particular domain to use your Spectrum 2 instance, but disallow it to any other users.

	[service]
	allowed_servers


#### Daemon related settings

Key | Type | Default | Description
----|------|---------|------------
user | string | | Name of user Spectrum switch to if run as daemon.
group | string | | Name of group Spectrum switch to if run as daemon.
pidfile | string | /var/run/spectrum2/$jid.pid | Full path to file to which the pid of Spectrum instance is stored if run as daemon.
working_dir | string | /var/run/spectrum2/$jid | Full path to directory where temporary files and coredumps will be stored if run as daemon.

#### Backends related settings

Key | Type | Default | Description
----|------|---------|------------
backend | string | | Full path to backend executable (for example "/usr/bin/spectrum2_libpurple_backend").
backend_host | string | localhost | Hostname to which backends connets.
backend_port | integer | 10000 | Port on which Spectrum listens for new backends.
users_per_backend | integer | 100 | Maximum number of users per one legacy network backend.
reuse_old_backends | boolean | 1 | True if Spectrum should use old backends which were full in the past.
idle_reconnect_time | time in seconds | 0 | Time in seconds after which idle users are reconnected to let their backend die.
memory_collector_time | time in seconds | 0 | Time in seconds after which backend with most memory is set to die.
protocol | string | | Used protocol in case of libpurple backend (prpl-icq, prpl-msn, prpl-jabber, ...).

### [identity] section

Key | Type | Default | Description
----|------|---------|------------
name | string | Spectrum 2 Transport | Name showed in service discovery.
category | string | gateway | Disco#info identity category. 'gateway' by default.
type | string | | Type of transport ('icq','msn','gg','irc', ...).

### [registration] section

Key | Type | Default | Description
----|------|---------|------------
enable_public_registration | boolean | 1 | True if users are able to register.
language | string | en | Default language for registration form.
instructions | string | Enter your legacy network username and password. | Instructions showed to user in registration form.
username_label | string | Legacy network username: | Label for username field.
username_mask | string | | Example: "$username@gmail.com" - users will register just "my_name" account and transport will connect them to my_name@gmail.com.
auto_register | boolean | 0 | When true, users are registered just by sending presence to transport. Password is set to empty string.

### [database] section

Key | Type | Default | Description
----|------|---------|------------
type | string | none | Database type - "none", "mysql", "sqlite3".
database | string | /var/lib/spectrum2/$jid/database.sql | Database used to store data. Path for SQLite3 or name for other types.
server | string | localhost | Database server.
user | string | | Database user.
password | string | | Database Password.
port | integer | | Database port.
prefix | string | | Prefix of tables in database.

### [logging] section

Key | Type | Default | Description
----|------|---------|------------
config | string | | Full path to log4cxx config file which is used for Spectrum 2 instance
backend_config | string | | Full path to log4cxx config file which is used for backends (if backend supports logging)
