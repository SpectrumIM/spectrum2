---
layout: page
title: Spectrum 2
---

### Compatibility with Spectrum 1

Spectrum 2 config file is not compatible with Spectrum 1, although some important config options are named the same as in Spectrum 1.

### [service] section

#### General settings

Key | Type | Default | Description
----|------|---------|------------
server_mode | boolean | 0 | True if Spectrum should run as server in [server-mode](http://spectrum.im/projects/spectrum/wiki/Spectrum_2_Admin_-_New_design#Server-mode).
jid | string | | Jabber ID of Spectrum2 instance. For example "localhost", "icq.domain.tld".
server | string | | Hostname or IP address of server to which Spectrum connects in gateway-mode.
port | integer | 0 | Port on which Spectrum listens to in server-mode or to which connects in gateway-mode.
password | string | | Password used to connect Jabber server in gateway-mode.
cert | string | | Full path to PKCS#12 certificate which is used for TLS in server-mode.
cert_password | string | | PKCS#12 certificate password.
admin_jid | JID | | Jabber ID of administrator with admin rights.
admin_password | string | | Administrator password.
enable_privacy_lists | boolean | 1 | True if privacy lists should be enabled.

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
