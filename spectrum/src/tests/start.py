import optparse
import sys
import time
import subprocess
import os

import sleekxmpp
import imp

def single_test(Client, Responder):
	os.system("../spectrum2 -n ./irc_test.cfg > spectrum2.log &")
	os.system("ngircd -f ngircd.conf &")
	time.sleep(1)
	responder = Responder("responder@localhost", "password", "#channel@localhost", "responder")
	responder.register_plugin('xep_0030')  # Service Discovery
	responder.register_plugin('xep_0045')  # Multi-User Chat
	responder.register_plugin('xep_0199')  # XMPP Ping
	responder['feature_mechanisms'].unencrypted_plain = True

	if responder.connect():
		responder.process(block=False)
	else:
		print "connect() failed"
		sys.exit(1)

	client = Client("client@localhost", "password", "#channel@localhost", "client")
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
	os.system("killall ngircd")
	os.system("killall spectrum2_libcommuni_backend 2>/dev/null")

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

exitcode = 0
for f in os.listdir("."):
	if not f.endswith(".py") or f == "start.py":
		continue

	print "Starting " + f + " test ..."
	test = imp.load_source('test', './' + f)
	ret = single_test(test.Client, test.Responder)
	if not ret:
		exitcode = -1

sys.exit(exitcode)

