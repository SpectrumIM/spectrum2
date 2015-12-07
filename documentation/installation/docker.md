---
layout: page
title: Spectrum 2
---

## Install Docker

At first you have to install Docker. This is very well described on the official [Docker Installation page](https://docs.docker.com/v1.8/installation/).

## Pull the Spectrum 2 Docker image

To download Spectrum 2 Docker image to your system, just run following command:

	$ docker pull spectrum2/spectrum2

You can also update Spectrum 2 using this command later.

## Create directory for Spectrum 2 configuration files

Now you have to decide where to store the configuration files for Spectrum 2. We will use `/opt/spectrum2/configuration` in our example:

	$ mkdir -p /opt/spectrum2/configuration

## Create configuration file

You can use following default configuration files as a starting place:

* [XMPP Frontend - gateway mode](https://github.com/hanzz/spectrum2/blob/spectrum/src/sample2_gateway.cfg)
* [XMPP Frontend - server mode](https://github.com/hanzz/spectrum2/blob/spectrum/src/sample.cfg)

Download the configuration file you chose into `/opt/spectrum2/configuration` directory you have created earlier and edit it as you want. Check the documentation and tutorials for configuration examples.

Note that the configuration files must have `.cfg` file extension.

## Create directory for Spectrum 2 data

You also have to create persistent directory to store various Spectrum 2 data like SQLite3 database and so on:

	$ mkdir -p /opt/spectrum2/data

## Start Spectrum 2

To start Spectrum 2 on background using Docker, all you have to do is running following Docker command:

	$ docker run --name="spectrum2" -d -v /opt/spectrum2/configuration:/etc/spectrum2/transports -v /opt/spectrum2/data:/var/lib/spectrum2 spectrum2/spectrum2

It will start Spectrum 2 and load the configuration files from `/opt/spectrum`. It also gives the spawned container name `spectrum2`.

## Checking the Spectrum 2 logs

To check the Spectrum 2 logs, use following Docker command:

	$ docker logs spectrum2

## Stopping the Spectrum 2

To stop the Spectrum 2 container, use following Docker command:

	$ docker stop spectrum2
