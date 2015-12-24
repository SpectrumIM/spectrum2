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
		self.responder_password = "password"
		self.client_password = "password"
		self.client_room = room
		self.responder_room = room
		self.client_nick = "client"
		self.responder_nick = "responder"
		self.responder_roompassword = ""

	def skip_test(self, test):
		return False

	def start(self, Client, Responder):
		os.system("../../spectrum/src/spectrum2 -n ./" + self.config + " > spectrum2.log &")
		self.pre_test()
		time.sleep(1)

		responder = Responder(self.responder_jid, self.responder_password, self.responder_room, self.responder_roompassword, self.responder_nick)
		responder.register_plugin('xep_0030')  # Service Discovery
		responder.register_plugin('xep_0045')  # Multi-User Chat
		responder.register_plugin('xep_0199')  # XMPP Ping
		responder['feature_mechanisms'].unencrypted_plain = True

		to = ("127.0.0.1", 5223)
		if self.responder_password != "password":
			to = ()
		if responder.connect(to):
			responder.process(block=False)
		else:
			print "connect() failed"
			os.system("killall spectrum2")
			self.post_test()
			sys.exit(1)

		client = Client(self.client_jid, self.client_password, self.client_room, self.client_nick)
		client.register_plugin('xep_0030')  # Service Discovery
		client.register_plugin('xep_0045')  # Multi-User Chat
		client.register_plugin('xep_0199')  # XMPP Ping
		client['feature_mechanisms'].unencrypted_plain = True

		time.sleep(2)

		to = ("127.0.0.1", 5223)
		if self.responder_password != "password":
			to = ("127.0.0.1", 5222)
		if client.connect(to):
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
		BaseTest.__init__(self, "../libcommuni/irc_test.cfg", True, "#channel@localhost")
		self.directory = "../libcommuni/"

	def pre_test(self):
		os.system("ngircd -f ../libcommuni/ngircd.conf &")

	def post_test(self):
		os.system("killall ngircd 2>/dev/null")
		os.system("killall spectrum2_libcommuni_backend 2>/dev/null")

class LibcommuniServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../libcommuni/irc_test2.cfg", True, "#channel%localhost@localhost")
		self.directory = "../libcommuni/"

	def pre_test(self):
		os.system("ngircd -f ../libcommuni/ngircd.conf &")

	def post_test(self):
		os.system("killall ngircd 2>/dev/null")
		os.system("killall spectrum2_libcommuni_backend 2>/dev/null")

class JabberServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../libpurple_jabber/jabber_test.cfg", True, "room%conference.localhost@localhostxmpp")
		self.directory = "../libpurple_jabber/"
		self.client_jid = "client%localhost@localhostxmpp"
		self.responder_jid = "responder%localhost@localhostxmpp"

	def skip_test(self, test):
		if test in ["muc_whois.py", "muc_change_topic.py"]:
			return True
		return False

	def pre_test(self):
		os.system("prosody --config ../libpurple_jabber/prosody.cfg.lua >prosody.log &")
		time.sleep(3)
		os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register client%localhost@localhostxmpp client@localhost password 2>/dev/null >/dev/null")
		os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register responder%localhost@localhostxmpp responder@localhost password 2>/dev/null >/dev/null")

	def post_test(self):
		os.system("killall lua-5.1 2>/dev/null")
		os.system("killall spectrum2_libpurple_backend 2>/dev/null")

class JabberSlackServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../slack_jabber/jabber_slack_test.cfg", True, "room%conference.localhost@localhostxmpp")
		self.directory = "../slack_jabber/"
		self.client_jid = "client@localhost"
		self.client_room = "room@conference.localhost"
		self.responder_jid = "owner@spectrum2tests.xmpp.slack.com"
		self.responder_password = "spectrum2tests.rkWHkOrjYucxsmBVkA9K"
		self.responder_room = "spectrum2_room@conference.spectrum2tests.xmpp.slack.com"
		self.responder_nick = "owner"
		self.responder_roompassword = "spectrum2tests.rkWHkOrjYucxsmBVkA9K"

	def skip_test(self, test):
		os.system("cp ../slack_jabber/slack.sql .")
		if test in ["muc_whois.py", "muc_change_topic.py", "muc_join_leave.py", "muc_pm.py"]:
			return True
		return False

	def pre_test(self):
		os.system("prosody --config ../slack_jabber/prosody.cfg.lua > prosody.log &")
		#time.sleep(3)
		#os.system("../../../spectrum_manager/src/spectrum2_manager -c manager.conf localhostxmpp set_oauth2_code xoxb-17213576196-VV2K8kEwwrJhJFfs5YWv6La6 use_bot_token 2>/dev/null >/dev/null")

	def post_test(self):
		os.system("killall lua-5.1 2>/dev/null")
		os.system("killall spectrum2_libpurple_backend 2>/dev/null")

configurations = []
configurations.append(LibcommuniServerModeSingleServerConf())
configurations.append(LibcommuniServerModeConf())
configurations.append(JabberServerModeConf())
configurations.append(JabberSlackServerModeConf())

exitcode = 0

for conf in configurations:
	for f in os.listdir(conf.directory):
		if not f.endswith(".py") or f == "start.py":
			continue

		if len(sys.argv) == 2 and sys.argv[1] != f:
			continue

		print conf.__class__.__name__ + ": Starting " + f + " test ..."
		test = imp.load_source('test', conf.directory + f)
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

