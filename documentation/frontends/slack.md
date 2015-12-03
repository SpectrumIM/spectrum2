---
layout: config
title: Spectrum 2
---

## Slack frontend Description

Slack frontend allows users to use Slack as a frontend network for Spectrum 2.

### Configuration

You have to choose this frontend in Spectrum 2 configuration file to use it:

	[service]
	frontend=slack


## Usage

To use Slack 2 frontend with Spectrum2, you have to add new bot to your Slack team and get the API token. After that, you can use `spectrum2_manager` to set the API token:

	$ spectrum2_manager localhost set_oauth2_code API_TOKEN use_bot_token

Use the real JID of your Spectrum 2 instance instead of `localhost`. Use the API token provided by the Slack instead of `API_TOKEN`.

Once you do that, Spectrum 2 will contact the Slack team's primary owner with instructions how to configure it.


