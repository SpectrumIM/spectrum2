---
layout: page
title: Spectrum 2
---



## Interonnecting Slack channels with 3rd-party rooms

Once you have [registered with 3rd-party network account](user_slack/register.html) (or if you are using IRC), you can also configure interconnection between Slack channel and 3rd-party network room.

The primary team owner can do it by sending following command to Spectrum 2 bot:

	.spectrum2 join.room NameOfYourBotIn3rdPartyNetwork #3rdPartyRoom hostname_of_3rd_party_server #SlackChannel

For example to join #test123 channel on Freenode IRC server as MyBot and transport it into #slack_channel, the command would look like this:

	.spectrum2 join.room MyBot #test123 adams.freenode.net #slack_channel

Another example to join Jabber room spectrum@conference.spectrum.im as SlackBot and transport it to #spectrum_muc Slack channel:

	.spectrum2 join.room SlackBot spectrum conference.spectrum.im #spectrum_muc
