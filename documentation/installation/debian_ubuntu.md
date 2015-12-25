---
layout: page
title: Spectrum 2
---
### Using spectrum repository
We have APT repositories for Debian and Ubuntu that make it very easy to install Spectrum 2.

To use the repositories, just add the following lines to `/etc/apt/sources.list`:

	deb http://repo.spectrum.im $dist main

where $dist is either lucid, oneiric, precise, quantal, sid, squeeze, wheezy. If you are unsure, you can usually find your distribution in the file /etc/lsb-release. We also have a source repository at the same location if you want to build the package yourself.

	apt-get update
	
### Building packages for different distributions/architectures
At now we do not have resources to support all different debian-based distributions and/or architectures, but you always can create packages for desired distribution yourself.
All required files for building Spectrum debian packages are present in the `packaging/debian` directory of spectrum Git repository. Most of required dependencies should be in the main Debian/Ubuntu repositories, except:

1. Swiften - the old Swiften library used by Spectrum is present in stable [Debian](https://packages.debian.org/jessie/libswiften-dev) and [Ubuntu LTS](http://packages.ubuntu.com/trusty/libswiften-dev) but may be missing in unstable/rolling Debian/Ubuntu trees. However, you can use their own [Debian/Ubuntu repository](https://github.com/hanzz/spectrum2/blob/master/.travis.yml#L10) or create packages yourself using the same method described here for Spectrum :)
2. Communi - this library is unavailable in Debian/Ubuntu, but we provide a special [repository](https://github.com/vitalyster/libcommuni-gbp) from which you can build libcommuni

#### Packaging for your current distribution
[TBD]
#### Creating package for different debian-based distro
We will show how to build Spectrum for the popular [Raspberry Pi](https://www.raspberrypi.org/) (or any similar) ARM-based machine with Debian-based distribution installed on it

[TBD]

### Install spectrum2

After you have done that, simply do:

	apt-get install spectrum2 spectrum2-backend-libpurple

Note that these repositories pull in quite a few dependencies, depending on the distribution you use.
