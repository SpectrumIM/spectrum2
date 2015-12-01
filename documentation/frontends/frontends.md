---
layout: page
title: Spectrum 2
---

## Spectrum 2 frontends and backends

Spectrum 2 supports multiple IM networks. Spectrum 2 distinguishes between **Frontend** and **Backend** networks.

If some network is supported as **Frontend**, it means that Spectrum 2 allows its users to use this network to communicate with other users using the **backend** network.

#### Example

If you for example use **Slack** as a frontend and **IRC** as a backend, the Spectrum 2 users can add Spectrum 2 to their Slack team, join the IRC network and communicate with their friends on IRC network.

This page contains the list of currently supported frontends with the basic information about them.

### How to change frontends

Frontends are switched in Spectrum 2 config file using the following option:

Section| Option| Value
-------|-------|------
service|frontend|Name of the frontend.

### List of frontends

Name| Supported network
----|-------------------
`xmpp`| XMPP (Jabber)
`slack`| Slack
