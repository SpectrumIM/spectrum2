---
layout: page
title: Spectrum 2
---

Spectrum2 manager is tool for managing Spectrum 2 instances. It can manage local instances and also do some basic management of remote instances.

## Configuration

Spectrum 2 manager normally checks all configuration files (.cfg files) in /etc/spectrum2/transports and do some for Spectrum 2 instances declared there.
This directory can be changed by changing Spectrum 2 manager configuration file, which is stored in /etc/spectrum2/spectrum_manager.cfg by default.

### spectrum_manager.cfg - [service] section:

Key | Type | Default | Description
----|------|---------|------------
config_directory | string | /etc/spectrum2/spectrum_manager.cfg | Directory where Spectrum2 configuration files are stored.

## Managing all local instances

### spectrum2_manager start

Starts all Spectrum2 instances according to config files defined in config_directory. This command can be called repeatedly. It has no effect on already running instances.

### spectrum2_manager stop

Stops all Spectrum2 instances according to config files defined in config_directory.

### spectrum2_manager status

Checks if all local instances (defined in config files in config_directory) are running. Returns 0 if all instances are running. If some instances are not running, returns 3.

### Managing particular Spectrum 2 instance

Spectrum 2 manager can be also used to manage one particular Spectrum 2 instance. For example following command starts Spectrum 2 instance with JID "icq.domain.tld":

	spectrum2_manager icq.domain.tld start

Following command stops that instance:

	spectrum2_manager icq.domain.tld stop

## Querying Spectrum 2 instance

You can get various information from running Spectrum 2 instance. To check all information you can get from Spectrum 2 instance with JID "icq.domain.tld, just run:

	spectrum2_manager icq.domain.tld help

You will get something similar to this list of available commands:

	General:
		status - shows instance status
		reload - Reloads config file
		uptime - returns ptime in seconds
	Users:
		online_users - returns list of all online users
		online_users_count - number of online users
		online_users_per_backend - shows online users per backends
		has_online_user <bare_JID> - returns 1 if user is online
		register <bare_JID> <legacyName> <password> - registers the new user
		unregister <bare_JID> - unregisters existing user
	Messages:
		messages_from_xmpp - get number of messages received from XMPP users
		messages_to_xmpp - get number of messages sent to XMPP users
	Backends:
		backends_count - number of active backends
		crashed_backends - returns IDs of crashed backends
		crashed_backends_count - returns number of crashed backends
	Memory:
		res_memory - Total RESident memory spectrum2 and its backends use in KB
		shr_memory - Total SHaRed memory spectrum2 backends share together in KB
		used_memory - (res_memory - shr_memory)
		average_memory_per_user - (memory_used_without_any_user - res_memory)
		res_memory_per_backend - RESident memory used by backends in KB
		shr_memory_per_backend - SHaRed memory used by backends in KB
		used_memory_per_backend - (res_memory - shr_memory) per backend
		average_memory_per_user_per_backend - (memory_used_without_any_user - res_memory) per backend


