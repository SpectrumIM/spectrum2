---
layout: page
title: Troubleshooting
---

## Troubleshooting FAQ

> Q: Nothing worked!

A: Check Spectrum logs for error messages.
By default it is `/var/log/spectrum2/<transport jid>/spectrum2.log` for the transport itself 
and `/var/log/spectrum2/<transport jid>/backends/backend-N.log` for protocol plugin.


> Q: Some protocol plugin not worked!

A: If it is a libpurple protocol plugin - try it with Pidgin or Finch first directly without Spectrum. 
Compare `pidgin -d` and `/var/log/spectrum2/<transport jid>/backends/backend-N.log` - they should be identical.

If you get some info - [check known issues](https://github.com/SpectrumIM/spectrum2/issues) and file new issue if needed.


> Q: It works with Pidgin (directly) but not working with Spectrum!

A: In this case it would be good to study your client XMPP logs for some unusual cases.
Be prepare to show it when submitting an issue!