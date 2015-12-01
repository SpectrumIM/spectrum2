---
layout: config
title: Spectrum 2
---

> **\[database\]**
>
> * [database](#databasedatabase)
> * [password](#databasepassword)
> * [port](#databaseport)
> * [prefix](#databaseprefix)
> * [server](#databaseserver)
> * [type](#databasetype)
> * [user](#databaseuser)
>
> **\[logging\]**
>
> * [backend_config](#loggingbackend_config)
> * [config](#loggingconfig)
>
> **\[registration\]**
>
> * [auto_register](#registrationautoregister)
> * [enable_public_registration](#registrationenablepublicregistration)
> * [instructions](#registrationinstructions)
> * [username_label](#registrationusernamelabel)
> * [username_mask](#registrationusernamemask)
>
> **\[service\]**
>
> * [admin_jid](#serviceadminjid)
> * [admin_password](#serviceadminpassword)
> * [backend](#servicebackend)
> * [backend_host](#servicebackendhost)
> * [backend_port](#servicebackendport)
> * [group](#servicegroup)
> * [jid](#servicejid)
> * [password](#servicepassword)
> * [pidfile](#servicepidfile)
> * [user](#serviceuser)
> * [users_per_backend](#servicusersperbackend)
> * [working_dir](#serviceworkingdir)



### Types of configuration fields

Following types are used:

integer - Examples: key=0

string - Examples: key=something

boolean - Examples: key=0 or key=1

list - List of strings (or Jabber IDs). You can specify this options more than once:

	allowed_servers=domain.tld
	allowed_servers=example.com

#### database.database

Key | val
----|----
Description:|Configures full path to database or the name of database.
Context:|server-mode and gateway-mode
Type:|string
Default:|/var/lib/spectrum2/$jid/database.sql

This option configures full path to database (in case of SQLite3) or the name of database.

#### database.password

Key | val
----|----
Description:|Configures password to connect the database.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

For server-client databases, this option configures password to be used to connect the database.

#### database.port

Key | val
----|----
Description:|Configures port on which database listens for incoming connections.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

For server-client databases, this option configures port on which database listens for incoming connections.

#### database.prefix

Key | val
----|----
Description:|Configures the prefix for the Spectrum 2 tables in database.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures the prefix for the Spectrum 2 tables in database. When tables are created, they are prefixed
with this prefix.

#### database.server

Key | val
----|----
Description:|Configures the hostname of database server.
Context:|server-mode and gateway-mode
Type:|string
Default:|localhost

For server-client databases, this option configures the hostname of server.

#### database.type

Key | val
----|----
Description:|Configures type of database where Spectrum 2 stores its data.
Context:|server-mode and gateway-mode
Type:|string
Default:|none

This option configures type of database where Spectrum 2 stores its data. It can be `none`, `mysql`, `sqlite3` or `pqxx` for PostgreSQL.

#### database.user

Key | val
----|----
Description:|Configures username to connect the database.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

For server-client databases, this option configures username to be used to connect the database.

#### logging.config

Key | val
----|----
Description:|Configures full path to log4cxx config file which is used for Spectrum 2 instance logs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures full path to log4cxx config file which is used for Spectrum 2 instance logs.

#### logging.backend_config

Key | val
----|----
Description:|Configures full path to log4cxx config file which is used for backends logs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures full path to log4cxx config file which is used for backends logs.

#### registration.auto_register

Key | val
----|----
Description:|Configures automatic user registration.
Context:|gateway-mode
Type:|boolean
Default:|0

If this option is enabled and user sends available presence to Spectrum 2 instance (tries to login it), he is registered
automatically. If the available presence was join-the-room request, legacy network name used for registration is determined
according to resource of this presence. This basically means that if user wants to join for example IRC room #test%irc.freenode.org
as "HanzZ", then Spectrum 2 registers him as "HanzZ" automatically and he does not have to fill registration form manually.

#### registration.enable_public_registration

Key | val
----|----
Description:|Configures if registration is allowed publicly.
Context:|gateway-mode
Type:|boolean
Default:|1

This option configures if registration is allowed publicly. This option can be used to disable public
registration or, together with [service.allowed_servers](#serviceallowed_servers) to create white-list of
domains from which users can register.

#### registration.instructions

Key | val
----|----
Description:|Configures instructions string showed in registration form.
Context:|gateway-mode
Type:|string
Default:|Enter your legacy network username and password.

This option configures instructions string showed in registration form.

#### registration.username_label

Key | val
----|----
Description:|Configures label for username field in registration form.
Context:|gateway-mode
Type:|string
Default:|Legacy network username:

This option configures label for username field in registration form.

#### registration.username_mask

Key | val
----|----
Description:|Configures mask which is used to generate legacy network username from username field in registration form.
Context:|gateway-mode
Type:|string
Default:|emptry string

This option configures mask which is used to generate legacy network username from username field in registration form.
An example of this option could be `username_mask=$username@chat.facebook.com`. If the user registers account "my_name", Spectrum 2
will use "my_name@chat.facebook.com" as legacy network username.

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

#### service.backend

Key | val
----|----
Description:|Configures path to backend executable.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures path to backend executable. When user connects, Spectrum 2 spawns new backend process to handle this user's
session. Usually single backend is shared by more users. This can be configured using service.users_per_backend.

#### service.backend_host

Key | val
----|----
Description:|Configures hostname or IP to which backend connects.
Context:|server-mode and gateway-mode
Type:|string
Default:|localhost

This option configures hostname or IP to which backend connects. When Spectrum 2 is started, it listens on service.backend_port
to communicate with backends. When new backend is started, it connects to the hostname configured by this option and starts handling
new users sessions.

#### service.backend_port

Key | val
----|----
Description:|Configures port on which Spectrum 2 listens on for backends connections.
Context:|server-mode and gateway-mode
Type:|integer
Default:|0

This option configures port on which Spectrum 2 listens on for backends connections. If the value of this option is 0, then Spectrum 2
uses randomly generated port number.


#### service.group

Key | val
----|----
Description:|Configures group under which Spectrum 2 instance runs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures name of group under which Spectrum 2 instance runs. When Spectrum 2 is started, it switches from current group
to this group.


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

#### service.pidfile

Key | val
----|----
Description:|Configures path to file when Spectrum 2 stores its process ID.
Context:|server-mode and gateway-mode
Type:|string
Default:|/var/run/spectrum2/$jid.pid

This option configures path to file when Spectrum 2 stores its process ID. This file is later used by `spectrum2_manager` to determine,
if this particular Spectrum 2 instance runs.

#### service.user

Key | val
----|----
Description:|Configures user under which Spectrum 2 instance runs.
Context:|server-mode and gateway-mode
Type:|string
Default:|empty string

This option configures name of user under which Spectrum 2 instance runs. When Spectrum 2 is started, it switches from current user
(usually root) to this user.


#### service.users_per_backend

Key | val
----|----
Description:|Configures maximum number of users who share single backend process.
Context:|server-mode and gateway-mode
Type:|integer
Default:|100

This option configures maximum number of users who share single backend process. Value of this option depends on number of online
users on your Spectrum 2 instance in the same time. There is no strict consensus on what is the best value. Every backend is separated
process and you probably want to keep the number of processes per Spectrum 2 instance less than 30. So if you have 1000 users, it
could be sane to use `users_per_backend=50`. Another idea is to have the same number of backends as the number of CPU cores of the
machine where Spectrum 2 is running.

If you have too big value of users_per_backend, you have just single backend process for all users and Spectrum 2 will not perform so well
as if you have more backends running for the same user-base. Also, if there is some problem with single backend, it will affects only users
connected using this particular backend, so it is good idea to have more backends running. However, the more backends you have
the more processes the machine has and since some point, it will become less useful. There is also some memory overhead, so if you run
500 users on single backend, it will take little bit less memory than 500 users on 5 backends (but it is not significant unless you decide
to have hundreds of backends).

If you presume to have just few users on your personal server, it can be useful to configure `service.users_per_backend=1`. Ever user will
get his own separated backend process.

For bigger user-base, you have to increase the `service.users_per_backend` value, but there is no optimal value to be suggested yet.


#### service.working_dir

Key | val
----|----
Description:|Configures path to directory when Spectrum 2 stores its data files.
Context:|server-mode and gateway-mode
Type:|string
Default:|/var/lib/spectrum2/$jid

This option configures path to directory when Spectrum 2 stores its data files like for example SQLite3 database.

