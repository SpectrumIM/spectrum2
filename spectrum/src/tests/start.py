import optparse
import sys
import time
import subprocess
import os

import sleekxmpp
import imp

def registerXMPPAccount(user, password):
	responder = sleekxmpp.ClientXMPP(user, password)
	responder.register_plugin('xep_0030')  # Service Discovery
	responder.register_plugin('xep_0077')
	responder['feature_mechanisms'].unencrypted_plain = True

	if responder.connect(("127.0.0.1", 5222)):
		responder.process(block=False)
	else:
		print "connect() failed"
		sys.exit(1)

class BaseTest:
	def __init__(self, config, server_mode, room):
		self.config = config
		self.server_mode = server_mode
		self.room = room
		self.responder_jid = "responder@localhost"
		self.client_jid = "client@localhost"

	def skip_test(self, test):
		return False

	def start(self, Client, Responder):
		os.system("../spectrum2 -n ./" + self.config + " > spectrum2.log &")
		self.pre_test()
		time.sleep(1)

		responder = Responder(self.responder_jid, "password", self.room, "responder")
		responder.register_plugin('xep_0030')  # Service Discovery
		responder.register_plugin('xep_0045')  # Multi-User Chat
		responder.register_plugin('xep_0199')  # XMPP Ping
		responder['feature_mechanisms'].unencrypted_plain = True

		if responder.connect(("127.0.0.1", 5223)):
			responder.process(block=False)
		else:
			print "connect() failed"
			os.system("killall spectrum2")
			self.post_test()
			sys.exit(1)

		client = Client(self.client_jid, "password", self.room, "client")
		client.register_plugin('xep_0030')  # Service Discovery
		client.register_plugin('xep_0045')  # Multi-User Chat
		client.register_plugin('xep_0199')  # XMPP Ping
		client['feature_mechanisms'].unencrypted_plain = True

		time.sleep(2)

		if client.connect(("127.0.0.1", 5223)):
			client.process(block=False)
		else:
			print "connect() failed"
			os.system("killall spectrum2")
			self.post_test()
			sys.exit(1)

		max_time = 60
		while not client.finished and not responder.finished and max_time > 0:
			time.sleep(1)
			max_time -= 1
		client.disconnect()
		responder.disconnect()

		os.system("killall spectrum2")
		self.post_test()

		ret = True
		tests = []
		tests += client.tests.values()
		tests += responder.tests.values()
		for v in tests:
			if v[1]:
				print v[0] + ": PASSED"
			else:
				print v[0] + ": FAILED"
				ret = False

		if not ret:
			os.system("cat spectrum2.log")

		return ret

class LibcommuniServerModeSingleServerConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "irc_test.cfg", True, "#channel@localhost")

	def pre_test(self):
		os.system("ngircd -f ngircd.conf &")

	def post_test(self):
		os.system("killall ngircd 2>/dev/null")
		os.system("killall spectrum2_libcommuni_backend 2>/dev/null")

class LibcommuniServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "irc_test2.cfg", True, "#channel%localhost@localhost")

	def pre_test(self):
		os.system("ngircd -f ngircd.conf &")

	def post_test(self):
		os.system("killall ngircd 2>/dev/null")
		os.system("killall spectrum2_libcommuni_backend 2>/dev/null")

class JabberServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "jabber_test.cfg", True, "room%conference.localhost@localhostxmpp")
		self.client_jid = "client%localhost@localhostxmpp"
		self.responder_jid = "responder%localhost@localhostxmpp"

	def skip_test(self, test):
		if test in ["muc_whois.py", "muc_change_topic.py"]:
			return True
		return False

	def pre_test(self):
		os.system("prosody --config prosody.cfg.lua >prosody.log &")
		time.sleep(3)
		os.system("../../../spectrum_manager/src/spectrum2_manager -c manager.conf localhostxmpp register client%localhost@localhostxmpp client@localhost password 2>/dev/null >/dev/null")
		os.system("../../../spectrum_manager/src/spectrum2_manager -c manager.conf localhostxmpp register responder%localhost@localhostxmpp responder@localhost password 2>/dev/null >/dev/null")

	def post_test(self):
		os.system("cat prosody.log")
		os.system("killall lua-5.1 2>/dev/null")
		os.system("killall spectrum2_libpurple_backend 2>/dev/null")

configurations = []
#configurations.append(LibcommuniServerModeSingleServerConf())
#configurations.append(LibcommuniServerModeConf())
configurations.append(JabberServerModeConf())

exitcode = 0

for conf in configurations:
	for f in os.listdir("."):
		if not f.endswith(".py") or f == "start.py":
			continue

		if len(sys.argv) == 2 and sys.argv[1] != f:
			continue

		print conf.__class__.__name__ + ": Starting " + f + " test ..."
		test = imp.load_source('test', './' + f)
		if conf.skip_test(f):
			print "Skipped."
			continue
		#try:
		ret = conf.start(test.Client, test.Responder)
		#except:
			#ret = False
		if not ret:
			exitcode = -1

sys.exit(exitcode)

