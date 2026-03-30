admins = { "admin@localhost" }

use_libevent = true;

modules_enabled = {
		"roster"; -- Allow users to have a roster. Recommended ;)
		"saslauth"; -- Authentication for clients and servers. Recommended if you want to log in.
		"disco"; -- Service discovery
		"private"; -- Private XML storage (for room bookmarks, etc.)
		"vcard"; -- Allow users to set vCards
		"carbons"; -- Needed for carbon testing
		"version"; -- Replies to server version requests
		"uptime"; -- Report how long server has been running
		"time"; -- Let others know the time here on this server
		"ping"; -- Replies to XMPP pings with pongs
		"pep"; -- Enables users to publish their mood, activity, playing music and more
		"register"; -- Allow users to register on this server using a client and change passwords
		"admin_adhoc"; -- Allows administration via an XMPP client that supports ad-hoc commands
};

modules_disabled = {
	"s2s";
};

allow_registration = true

c2s_require_encryption = false
allow_unencrypted_plain_auth = true

authentication = "internal_plain"

log = "*console"

daemonize = false -- Default is "true"

plugin_paths = { "/usr/lib/prosody/modules", "/usr/lib/prosody-modules" }
component_interface = { "0.0.0.0" }
VirtualHost "localhost"
	modules_enabled = {
		"privilege"
        }
	privileged_entities = {
                ["irc.localhost"] = {
                        roster = "both",
                        message = "outgoing"
                },
		["tg.localhost"] = {
			roster = "both",
			message = "outgoing"
		},
		["wa.localhost"] = {
			roster = "both",
			message = "outgoing"
		}
        }
Component "irc.localhost"
	component_secret = "password"
        modules_enabled = {
            "privilege"
        }
Component "tg.localhost"
	component_secret = "password"
	modules_enabled = {
	    "privilege"
	}
Component "wa.localhost"
	component_secret = "password"
	modules_enabled = {
	    "privilege"
	}
