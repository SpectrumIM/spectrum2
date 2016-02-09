---
layout: page
title: Spectrum 2
---

Web Interface allows admins to control Spectrum 2 instances and users to register the transports and for example manage rooms they are connected to.

## Configuration

The Web Interface is part of Spectrum 2 Manager tool, so it shares the same `/etc/spectrum2/spectrum_manager.cfg` configuration file. There are following configuration variables available:

Section | Key | Type | Default | Description
----|------|---------|------------
service | admin_username | string | | Admin username for the Web Interface.
service | admin_password | string | | Admin password for the Web Interface.
service | port | int | 8080 | Port on which the Web Interface will be accesible.
service | base_location | string | / | Base location (directory) on which the Web interface is served. It must ends up with slash character. If you for example set it to "/spectrum/", then the Web interface will be served on http://localhost:$port/spectrum/.
service | data_dir | string | /var/lib/spectrum2_manager/html | Path to the Web Interface static data.
service | cert | string | | Web interface certificate in PEM format when TLS should be used. Empty otherwise.
logging | config | string | /etc/spectrum2/manager_logging.cfg | Path to the logging configuration file in the same format as for the Spectrum 2.
database | ... | ... | ... | Database to store the Web Interface data. The variables are the same as in the [Spectrum 2 config files](http://spectrum.im/documentation/configuration/config_file.html#databasedatabase).

## Starting the Web Interface

For now the only way how to start Web Interface is running it on foreground using:

`$ spectrum2_manager server`

In the future, we plan to support Spectrum 2 Web Interface as standalone server.
