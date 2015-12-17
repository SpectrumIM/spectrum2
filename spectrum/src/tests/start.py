import optparse
import sys
import time
import subprocess
import os

import sleekxmpp
import imp

class BaseTest:
	def __init__(self, config, server_mode, room):
		self.config = config
		self.server_mode = server_mode
		self.room = room

	def start(self, Client, Responder):
		self.pre_test()
		os.system("../spectrum2 -n ./" + self.config + " > spectrum2.log &")
		time.sleep(1)

		responder = Responder("responder@localhost", "password", self.room, "responder")
		responder.register_plugin('xep_0030')  # Service Discovery
		responder.register_plugin('xep_0045')  # Multi-User Chat
		responder.register_plugin('xep_0199')  # XMPP Ping
		responder['feature_mechanisms'].unencrypted_plain = True

		if responder.connect():
			responder.process(block=False)
		else:
			print "connect() failed"
			sys.exit(1)

		client = Client("client@localhost", "password", self.room, "client")
		client.register_plugin('xep_0030')  # Service Discovery
		client.register_plugin('xep_0045')  # Multi-User Chat
		client.register_plugin('xep_0199')  # XMPP Ping
		client['feature_mechanisms'].unencrypted_plain = True

		time.sleep(2)

		if client.connect():
			client.process(block=False)
		else:
			print "connect() failed"
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

configurations = []
configurations.append(LibcommuniServerModeSingleServerConf())
configurations.append(LibcommuniServerModeConf())

exitcode = 0

for conf in configurations:
	for f in os.listdir("."):
		if not f.endswith(".py") or f == "start.py":
			continue

		print conf.__class__.__name__ + ": Starting " + f + " test ..."
		test = imp.load_source('test', './' + f)
		try:
			ret = conf.start(test.Client, test.Responder)
		except:
			ret = False
		if not ret:
			exitcode = -1

sys.exit(exitcode)

