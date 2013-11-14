---
layout: page
title: Spectrum 2
---


Thanks to layman, it is very easy to install Spectrum2.

To use other repositories, please firt installing layman:

	# emerge layman

Please make sure the **git** USE-Flag ist set.

Now, you need to add the overlay by creating a new file, let's call it `/etc/layman/overlays/spectrum2.xml`:

	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE repositories SYSTEM "/dtd/repositories.dtd">
	<repositories xmlns="" version="1.0">
	  <repo quality="experimental" status="unofficial">
	    <name><![CDATA[spectrum2]]></name>
	    <description><![CDATA[Spectrum 2 and others]]></description>
	    <homepage>http://pubgit.rpgfiction.net/?p=portage-overlay.git;a=summary</homepage>
	    <owner>
	      <email>chain@rpgfiction.net</email>
	      <name><![CDATA[chain (Richard H.)]]></name>
	    </owner>
	    <source type="git">git://pubgit.rpgfiction.net/portage-overlay.git</source>
	    <feed></feed>
	  </repo>
	</repositories>

Now you need to add the second line to `/etc/layman/layman.cfg`:

	overlays  : http://www.gentoo.org/proj/en/overlays/repositories.xml
	            file:///etc/layman/overlays/spectrum2.xml

All done! Add the overlay by issuing

	# layman -S
	# layman -a spectrum2


## Install spectrum2

Emerge spectrum2:

	# emerge spectrum:2 --autounmask-write -a

	# dispatch-conf
	(or)
	# etc-update

	# emerge spectrum:2 -a	

This will tell portage to unmask the needed packages, all dependencies should be pulled in automatically, you can change the needed features by the USE-Flags provided.


Alternatively, you might want to use the live ebuild to compile the latest master snapshot:

	# emerge =net-im/spectrum:2.9999 --autounmask-write -a
	
	
If you want to use the swift live ebuild, you have to install it from the 'swift-2.x' branch before emerging spectrum2: 

	# swift_LIVE_BRANCH="swift-2.x" emerge =net-im/swift-9999 --autounmask-write -a
