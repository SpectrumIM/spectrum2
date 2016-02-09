---
layout: config
title: Spectrum 2
---

## Slack frontend Description

Slack frontend allows users to use Slack as a frontend network for Spectrum 2. This page describes how to setup your own Spectrum 2 instance with Slack frontend.

### Configuration

You have to choose this frontend in Spectrum 2 configuration file to use it:

	[service]
	frontend=slack
	client_id=42132532467.11232153249
	client_secret=cfdw9erw9ew0gew9gds0sa9wqd9f8d6a

To obtain the `client_id` and `client_secret` values, you have to create new Slack application on [Slack website](https://slack.com/apps/build). You also have to setup Spectrum 2 Web Interface to allow Slack users to register the transport and configure it. While creating new Slack application, you also have to setup the `Redirect URI`. Spectrum 2 Web Interface expects the `Redirect URI` to be configured as `http://domain.tld/oauth2`. You can choose whatever domain you are running at, but you have to preserve the `/oauth2` location.

## Usage

Once you setup Spectrum 2 with proper `client_id` and `client_secret`, you can register as new user in the Spectrum 2 Web Interface.
