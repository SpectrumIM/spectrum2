import optparse
import sys
import time
import subprocess
import os
import traceback

import sleekxmpp
from importlib.machinery import SourceFileLoader
import logging

#logging.basicConfig(level=logging.DEBUG,
                        #format='%(levelname)-8s %(message)s')

def registerXMPPAccount(user, password):
	responder = sleekxmpp.ClientXMPP(user, password)
	responder.register_plugin('xep_0030')  # Service Discovery
	responder.register_plugin('xep_0077')
	responder['feature_mechanisms'].unencrypted_plain = True

	if responder.connect(("127.0.0.1", 5222)):
		responder.process(block=False)
	else:
		print("connect() failed")
		sys.exit(1)


class BaseTestCase:
	def __init__(self, env):
		self.env = env
		self.tests = []
		self.enabled = True

	def setup(self):
		pass

	def teardown(self):
		pass

	def finished(self):
		return true

# Default test case for one client and one responder
class ClientResponderTestCase(BaseTestCase):
	def __init__(self, env, Client, Responder):
		BaseTestCase.__init__(self, env)
		self.env = env
		self.Client = Client
		self.Responder = Responder
		self.enabled = True

	def setup(self):
		#print str([self.env.responder_jid, self.env.responder_password, self.env.responder_room, self.env.responder_roompassword, self.env.responder_nick])
		self.responder = self.Responder(self.env.responder_jid, self.env.responder_password, self.env.responder_room, self.env.responder_roompassword, self.env.responder_nick)
		self.responder.register_plugin('xep_0030')  # Service Discovery
		self.responder.register_plugin('xep_0045')  # Multi-User Chat
		self.responder.register_plugin('xep_0199')  # XMPP Ping
		self.responder['feature_mechanisms'].unencrypted_plain = True

		to = (self.env.spectrum.host, self.env.spectrum.port)
		if self.env.responder_password != "password":
			to = () # auto detect from jid
				# used by some tests to connect responder to legacy network
		if self.responder.connect(to):
			self.responder.process(block=False)
		else:
			raise Exception("connect() failed")

		#print str([self.env.client_jid, self.env.client_password, self.env.client_room, self.env.client_nick])
		self.client = self.Client(self.env.client_jid, self.env.client_password, self.env.client_room, self.env.client_nick)
		self.client.register_plugin('xep_0030')  # Service Discovery
		self.client.register_plugin('xep_0045')  # Multi-User Chat
		self.client.register_plugin('xep_0199')  # XMPP Ping
		self.client['feature_mechanisms'].unencrypted_plain = True

		time.sleep(1)

		to = (self.env.spectrum.host, self.env.spectrum.port)
		if self.env.responder_password != "password":
			to = ("127.0.0.1", 5222)
		if self.client.connect(to):
			self.client.process(block=False)
		else:
			raise Exception("connect() failed")

	def finished(self):
		#print "client.finished="+str(self.client.finished)
		#print "responder.finished="+str(self.responder.finished)
		return self.client.finished and self.responder.finished

	def teardown(self):
		self.tests = []
		if hasattr(self, 'client'):
			self.tests += self.client.tests.values()
			self.client.disconnect()
		if hasattr(self, 'responder'):
			self.tests += self.responder.tests.values()
			self.responder.disconnect()

class Params:
	pass

class BaseTest:
	def __init__(self, config, server_mode, room):
		self.config = config
		self.server_mode = server_mode

		# Spectrum connection parameters
		self.spectrum = Params()
		self.spectrum.host = "127.0.0.1"
		self.spectrum.port = 5223
		self.spectrum.hostname = "localhostxmpp"

		# Legacy network connection parameters (default: XMPP)
		self.backend = Params()
		self.backend.host = "127.0.0.1"
		self.backend.port = 5222
		self.backend.hostname = "localhost"

		# User accounts
		self.client_jid = "client@localhost"
		self.client_password = "password"
		self.client_nick = "client"
		self.client_backend_jid = ""
		self.client_backend_password = ""
		self.responder_jid = "responder@localhost"
		self.responder_password = "password"
		self.responder_nick = "responder"
		self.responder_backend_jid = ""
		self.responder_backend_password = ""
		self.room = room
		self.client_room = room
		self.responder_room = room
		self.responder_roompassword = ""

	def skip_test(self, test):
		return False

	def start(self, TestCaseFile):
		# Use ClientResponder by default, but files may declare their own TestCase()s
		try:
			testCase = TestCaseFile.TestCase(self)
		except (NameError, AttributeError):
			testCase = ClientResponderTestCase(self, TestCaseFile.Client, TestCaseFile.Responder)

		# Let the test self-disqualify
		if hasattr(testCase, 'enabled') and not testCase.enabled:
			print("Skipped (self-disqualified).")
			return True

		self.pre_test()
		time.sleep(1)
		try:
			testCase.setup()
			max_time = 60
			while not testCase.finished() and max_time > 0:
				if callable(getattr(testCase, "step", None)):
					testCase.step()
				time.sleep(1)
				max_time -= 1
		finally:
			testCase.teardown()
			self.post_test()

		ret = True
		for v in testCase.tests:
			if v[1]:
				print(v[0] + ": PASSED")
			else:
				print(v[0] + ": FAILED")
				ret = False

		if not ret:
			os.system("cat spectrum2.log")

		return ret
	
	def pre_test(self):
		os.system("../../spectrum/src/spectrum2 -n ./" + self.config + " > spectrum2.log &")
	
	def post_test(self):
		os.system("killall -w spectrum2")

class LibcommuniServerModeSingleServerConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../libcommuni/irc_test.cfg", True, "#channel@localhost")
		self.directory = "../libcommuni/"

	def skip_test(self, test):
		if test in ["muc_join_nickname_used.py"]:
			return True
		return False

	def pre_test(self):
		BaseTest.pre_test(self)
		os.system("/usr/sbin/ngircd -f ../libcommuni/ngircd.conf &")

	def post_test(self):
		os.system("killall -w ngircd 2>/dev/null")
		os.system("killall -w spectrum2_libcommuni_backend 2>/dev/null")
		BaseTest.post_test(self)

class LibcommuniServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../libcommuni/irc_test2.cfg", True, "#channel%localhost@localhost")
		self.directory = "../libcommuni/"

	def pre_test(self):
		BaseTest.pre_test(self)
		os.system("/usr/sbin/ngircd -f ../libcommuni/ngircd.conf &")

	def post_test(self):
		os.system("killall -w ngircd 2>/dev/null")
		os.system("killall -w spectrum2_libcommuni_backend 2>/dev/null")
		BaseTest.post_test(self)

class JabberServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../libpurple_jabber/jabber_test.cfg", True, "room%conference.localhost@localhostxmpp")
		self.directory = "../libpurple_jabber/"
		self.client_jid = "client%localhost@localhostxmpp"
		self.client_backend_jid = "client@localhost"
		self.client_backend_password = self.client_password
		self.responder_jid = "responder%localhost@localhostxmpp"
		self.responder_backend_jid = "responder@localhost"
		self.responder_backend_password = self.responder_password

	def skip_test(self, test):
		if test in ["muc_whois.py", "muc_change_topic.py"]:
			return True
		return False

	def pre_test(self):
		os.system("cp ../libpurple_jabber/prefs.xml ./ -f >/dev/null")
		BaseTest.pre_test(self)
		os.system("prosody --config ../libpurple_jabber/prosody.cfg.lua >prosody.log &")
		time.sleep(3)
		os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register client%localhost@localhostxmpp client@localhost password 2>/dev/null >/dev/null")
		os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register responder%localhost@localhostxmpp responder@localhost password 2>/dev/null >/dev/null")

	def post_test(self):
		os.system("killall -w -r lua.* 2>/dev/null")
		os.system("killall -w spectrum2_libpurple_backend 2>/dev/null")
		BaseTest.post_test(self)

class JabberSlackServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../slack_jabber/jabber_slack_test.cfg", True, "room%conference.localhost@localhostxmpp")
		self.directory = "../slack_jabber/"
		self.client_jid = "client@localhost"
		self.client_room = "room@conference.localhost"
		# Implicitly forces responder to connect to slack.com instead of localhost
		# by passing a nonstandard responder_roompassword
		self.responder_jid = "owner@spectrum2tests.xmpp.slack.com"
		self.responder_password = "spectrum2tests.e2zJwtKjLhLmt14VsMKq"
		self.responder_room = "spectrum2_room@conference.spectrum2tests.xmpp.slack.com"
		self.responder_nick = "owner"
		self.responder_roompassword = "spectrum2tests.e2zJwtKjLhLmt14VsMKq"

	def skip_test(self, test):
		os.system("cp ../slack_jabber/slack.sql .")
		if test.find("bad_password") != -1:
			print("Changing password to 'badpassword'")
			os.system("sqlite3 slack.sql \"UPDATE users SET password='badpassword' WHERE id=1\"")
		return False

	def pre_test(self):
		BaseTest.pre_test(self)
		os.system("prosody --config ../slack_jabber/prosody.cfg.lua > prosody.log &")

	def post_test(self):
		os.system("killall -w -r lua.* 2>/dev/null")
		os.system("killall -w spectrum2_libpurple_backend 2>/dev/null")
		BaseTest.post_test(self)

class TwitterServerModeConf(BaseTest):
	def __init__(self):
		BaseTest.__init__(self, "../twitter/twitter_test.cfg", True, "")
		self.directory = "../twitter/"
		self.client_password = "testpass123"

	def skip_test(self, test):
		os.system("cp ../twitter/twitter.sql .")

	def pre_test(self):
		BaseTest.pre_test(self)

	def post_test(self):
		os.system("killall -w spectrum2_twitter_backend 2>/dev/null")
		BaseTest.post_test(self)

configurations = []
configurations.append(LibcommuniServerModeSingleServerConf())
configurations.append(LibcommuniServerModeConf())
configurations.append(JabberServerModeConf())
#configurations.append(JabberSlackServerModeConf())
#configurations.append(TwitterServerModeConf())

exitcode = 0

# Pass file names to run only those tests
test_list = sys.argv[1:]

for conf in configurations:
	for f in os.listdir(conf.directory):
		if (len(test_list) > 0) and not (f in test_list):
			continue
		if not f.endswith(".py") or f == "start.py":
			continue

		if len(sys.argv) == 2 and sys.argv[1] != f:
			continue

		print(conf.__class__.__name__ + ": Starting " + f + " test ...")
		# Modules must have distinct module names or their clases will be merged by loader!
		modulename = (conf.directory+f).replace("/","_").replace("\\","_").replace(".","_")
		test = SourceFileLoader(modulename, conf.directory + f).load_module()

		if conf.skip_test(f):
			print("Skipped.")
			continue
		try:
			ret = conf.start(test)
		except Exception as e:
			print("Test ended in exception.")
			traceback.print_exc()
			sys.exit(-1)
		if not ret:
			exitcode = -1

sys.exit(exitcode)
