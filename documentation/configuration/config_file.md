---
layout: config
title: Spectrum 2
---

> * [service.server_mode](#serviceserver_mode)
> * [service.jid](#servicejid)
> * [service.server](#serviceserver)
> * [service.port](#serviceport)
> * [service.password](#servicepassword)

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

Following part of config file disables public registrations and allows only users from xmpp.org and jabber.org to use this Spectrum 2 instance:

	[service]
	allowed_servers=xmpp.org
	allowed_servers=jabber.org
	
	[registration]
	enable_public_registration=0

#### service.user

Key | val
----|----
Description:|Configures user under which Spectrum 2 instance runs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures name of user under which Spectrum 2 instance runs. When Spectrum 2 is started, it switches from current user
(usually root) to this user.

#### service.group

Key | val
----|----
Description:|Configures group under which Spectrum 2 instance runs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures name of group under which Spectrum 2 instance runs. When Spectrum 2 is started, it switches from current group
to this group.

#### service.pidfile

Key | val
----|----
_Description:_|Configures path to file when Spectrum 2 stores its process ID.
Context:|server-mode and gateway-mode
Type:|string
Default:|/var/run/spectrum2/$jid.pid

This option configures path to file when Spectrum 2 stores its process ID. This file is later used by `spectrum2_manager` to determine,
if this particular Spectrum 2 instance runs.

#### service.working_dir

Key | val
----|----
_Description:_|Configures path to directory when Spectrum 2 stores its data files.
Context:|server-mode and gateway-mode
Type:|string
Default:|/var/lib/spectrum2/$jid

This option configures path to directory when Spectrum 2 stores its data files like for example SQLite3 database.

#### service.backend

Key | val
----|----
_Description:_|Configures path to backend executable.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures path to backend executable. When user connects, Spectrum 2 spawns new backend process to handle this user's
session. Usually single backend is shared by more users. This can be configured using service.users_per_backend.

#### service.backend_host

Key | val
----|----
_Description:_|Configures hostname or IP to which backend connects.
Context:|server-mode and gateway-mode
Type:|string
Default:|localhost

This option configures hostname or IP to which backend connects. When Spectrum 2 is started, it listens on service.backend_port
to communicate with backends. When new backend is started, it connects to the hostname configured by this option and starts handling
new users sessions.

#### service.backend_port

Key | val
----|----
_Description:_|Configures port on which Spectrum 2 listens on for backends connections.
Context:|server-mode and gateway-mode
Type:|integer
Default:|0

This option configures port on which Spectrum 2 listens on for backends connections. If the value of this option is 0, then Spectrum 2
uses randomly generated port number.

#### service.users_per_backend

Key | val
----|----
_Description:_|Configures maximum number of users who share single backend process.
Context:|server-mode and gateway-mode
Type:|integer
Default:|100

This option configures maximum number of users who share single backend process. Value of this option depends on number of online
users you presume to use your Spectrum 2 instance in the same time.

#### Backends related settings

Key | Type | Default | Description
----|------|---------|------------
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
