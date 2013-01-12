---
layout: page
title: Spectrum 2
---

Spectrum 2 can work as a [bouncer](http://en.wikipedia.org/wiki/BNC_%28software%29). When this feature is enabled and
XMPP user disconnects, it stays connected to legacy network. All messages (even those from multi user chat rooms) are cached
and once the XMPP user connects again, they are forwarded to him.

### Enabling bouncer as a user

Your client has to support Ad-Hoc commands. Then you just have to execute "Transport settings" command provided by the
Spectrum 2 transport. You will see checkbox with "Stay connected to legacy network when offline on XMPP" label. When you check
it and finish this command, bouncer mode will be enabled.

If you now join some room, there is no other way to left it than disabling bouncer mode, leave the room and enabling it again.
