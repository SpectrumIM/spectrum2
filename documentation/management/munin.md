---
layout: page
title: Spectrum 2
---

Munin is tool for collecting various information from system and showing them in charts. Spectrum 2 contains munin plugin which can be used to generate useful charts in Munin.

### Configuration

There's Munin plugin installed in by default in `/usr/share/munin/plugins/spectrum2_`. You have to create symlinks pointing to that files in `/etc/munin/plugins` name like this:

Symlink name | Meaning
-------------|--------
`spectrum2_uptime` | Uptime
`spectrum2_backends_count` | Backends count
`spectrum2_crashed_backends_count` | Crashed backends count
`spectrum2_online` | Online users count
`spectrum2_messages` | Total messages send over spectrum since the last restart
`spectrum2_messages_sec` | Messages send per second over spectrum transports
`spectrum2_memory` | Memory usage of transports
`spectrum2_average_memory_per_user` | Average memory usage of per user

