---
layout: page
title: Spectrum 2
---

We have APT repositories for Debian and Ubuntu that make it very easy to install Spectrum 2.

To use the repositories, just add the following lines to @/etc/apt/sources.list@:

	deb http://repo.spectrum.im $dist main

where $dist is either lucid, oneiric, precise, quantal, sid, squeeze, wheezy. If you are unsure, you can usually find your distribution in the file /etc/lsb-release. We also have a source repository at the same location if you want to build the package yourself.

	apt-get update

### Install spectrum2

After you have done that, simply do:

	apt-get install spectrum2 spectrum2-backend-libpurple

Note that these repositories pull in quite a few dependencies, depending on the distribution you use.
