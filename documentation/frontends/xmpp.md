---
layout: config
title: Spectrum 2
---

> **\[identity\]**
>
> * [category](#identitycategory)
> * [name](#identityname)
> * [type](#identitytype)
>
>
>
> **\[service\]**
>
> * [allowed_servers](#serviceallowedservers)
> * [cert](#servicecert)
> * [cert_password](#servicecertpassword)
> * [port](#serviceport)
> * [server](#serviceserver)
> * [server_mode](#serviceservermode)



## XMPP frontend Description

XMPP frontend allows users to use XMPP as a frontend network for Spectrum 2.

### Configuration

You have to choose this frontend in Spectrum 2 configuration file to use it:

	[service]
	frontend=xmpp


## Gateway mode and server mode

XMPP frontend can work in two modes: Gateway mode and Server mode. This chapter describes differences betweeen these two modes. If you want to find out how to configure Spectrum 2 to run in gateway or server mode, read the Configuration part of this documentation.

### Gateway mode

Gateway mode represents the classic way how XMPP gateway works. You have to configure an external XMPP server (like Prosody or Ejabberd) to serve the subdomain you want to use for Spectrum 2 (for example "icq.domain.tld"). Spectrum 2 in gateway mode then connects the XMPP server as its component and users are able to find out "icq.domain.tld" in Service Discovery, register it and use it.

**Advantages:**

* Users can use more legacy networks using single XMPP account (and using single TCP connection).
* It's easy to extend existing XMPP servers using gateway mode.

**Disadvantages:**

* Passwords are stored (even in encrypted form) on server.
* Roster (contact list) synchronization can be problematic, because it depends on the client user uses. This can be improved by usage of Remote Roster protoXEP.
* You have to setup XMPP server and use database even if you only want to use Spectrum 2 as a tool to connect legacy networks using XMPP protocol.

### Server mode

In server mode, Spectrum 2 behaves as standalone server. User then logins legacy networks by logging XMPP account like this one: "my_msn_name%hotmail.com@msn.domain.tld".

**Advantages:**

* Passwords are not stored on server.
* Roster synchronization is easy, because Spectrum 2 acts as normal server.
* If you want to use Spectrum 2 as wrapper between different networks, you don't need database or Jabber server as another layer.
* Using SRV records you can easily run Spectrum 2 on different machines to scale it.

**Disadvantages:**

* Clients have to support more accounts to connect more legacy networks (Therefore they will need have to use more TCP connections).

## XMPP frontend configuration variables

This chapter contains available configuration variables for XMPP frontend. Note that these are only XMPP frontend specific. For list of general configuration variables, check [Configuration file description](/documentation/configuration/config_file.html).

#### identity.category

Key | val
----|----
Description:|Configures disco#info identity category.
Context:|gateway-mode
Type:|string
Default:|gateway

This option configures disco#info identity category.

#### identity.name

Key | val
----|----
Description:|Configures the name showed in service discovery.
Context:|gateway-mode
Type:|string
Default:|Spectrum 2 Transport

This option configures the name showed in service discovery.

#### identity.type

Key | val
----|----
Description:|Configures type of transport as showed in service discovery.
Context:|gateway-mode
Type:|string
Default:|empty string

This option configures type of transport as showed in service discovery. It is usually used by XMPP client to show proper
icons according to type of network. See [Disco Categories](http://xmpp.org/registrar/disco-categories.html#gateway) for
allowed values.

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

#### service.server

Key | val
----|----
Description:|Configures hostname or IP address of server to which Spectrum 2 connects to.
Context:|gateway-mode
Type:|string
Default:|empty string

This option configures hostname or IP address of server to which Spectrum 2 connects to. It is used only in gateway-mode and
you should configure it to point to hostname or IP of your XMPP server.

#### service.server_mode

Key | val
----|----
Description:|Configures if Spectrum 2 works in server mode or gateway mode
Context:|server-mode and gateway-mode
Type:|boolean
Default:|0

If this option is true, Spectrum 2 works in server-mode and acts as standalone server.

User then logins legacy networks by logging XMPP account like this one: `my_msn_name%hotmail.com@msn.domain.tld`.

**Advantages:**

* Passwords are not stored on server.
* Roster synchronization is easy, because Spectrum 2 acts as normal server.
* If you want to use Spectrum 2 as wrapper between different networks, you don't need database or Jabber server as another layer.
* Using SRV records you can easily run Spectrum 2 on different machines to scale it.

**Disadvantages:**

* Clients have to support more accounts to connect more legacy networks (Therefore they will need have to use more TCP connections).

If this option is false, Spectrum 2 acts as normal XMPP component (gateway).

You then have to configure an external XMPP server (like Prosody or Ejabberd) to serve the subdomain you want to use for Spectrum 2 (for example "icq.domain.tld"). Spectrum 2 in gateway mode then connects the XMPP server as its component and users are able to find out "icq.domain.tld" in Service Discovery, register it and use it.

**Advantages:**

* Users can use more legacy networks using single XMPP account (and using single TCP connection).
* It's easy to extend existing XMPP servers using gateway mode.

**Disadvantages:**

* Passwords are stored (even in encrypted form) on server.
* Roster (contact list) synchronization can be problematic, because it depends on the client user uses. This can be improved by usage of Remote Roster protoXEP.
* You have to setup XMPP server and use database even if you only want to use Spectrum 2 as a tool to connect legacy networks using XMPP protocol.


