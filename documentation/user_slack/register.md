---
layout: page
title: Spectrum 2
---



## Registering with 3rd-party network account

Once you have [added Spectrum 2 transport to your team](http://slack.spectrum.im/), Spectrum 2 bot should contact you with instructions how to register with 3rd-party network. This document only sums up the instructions from the Spectrum 2 bot message and in some case documents them further. Note that these steps are not needed for interconnecting with IRC network, because it does not need registrationg. You can skip these instructions in this case.

All those steps have to be done by the primary team owner.


#### 1. Create channel for the 3rd-party network contact list

As described in the [How does Spectrum 2 interact with my team?](user_slack/workflow.html) document, Spectrum 2 needs a channel where the messages sent to the 3rd-party network account will be forwaded. You therefore have to create a new Slack channel in your team and invite Spectrum 2 bot there.

#### 2. Register Spectrum 2 with the 3rd-party network

You have to have 3rd-party account ready and have to know the password for that account to be able to configure Spectrum 2 further. The the primary team owner can send following message to Spectrum 2 bot to register the 3rd-party network account:

	.spectrum2 register 3rdPartyAccount 3rdPartyPassword #SlackChannel

Example for Jabber:

	.spectrum2 register test@xmpp.tld mypassword #slack_channel
